// ============================================================================
// uart.c - Layer UART pentru comunicatie AT cu A7682E
// ERGO GASALERT v5.0 - STM32C011 + A7682E
//
// USART1: PA2 (TX, AF1) -> A7682E RX
//         PA3 (RX, AF1) <- A7682E TX
// Baudrate: 115200, 8N1
// Receptie: buffer circular cu intrerupere RXNE
// Transmisie: blocare (polling pe TXE)
// ============================================================================

#include "stm32c0xx.h"
#include "../include/ergo_pins.h"
#include "../include/ergo_config.h"
#include "../include/ergo_uart.h"

// ============================================================================
// BUFFER CIRCULAR RECEPTIE
// ============================================================================

static volatile uint8_t  rxBuf[UART_RX_BUF_SIZE];
static volatile uint16_t rxHead = 0;   // index scriere (ISR)
static volatile uint16_t rxTail = 0;   // index citire (main)

// ============================================================================
// INITIALIZARE USART1
// ============================================================================

void initUART(uint32_t baudrate)
{
    // 1. Activeaza ceasul pentru GPIOA si USART1
    RCC->IOPENR  |= RCC_IOPENR_GPIOAEN;
    RCC->APBENR2 |= RCC_APBENR2_USART1EN;

    // 2. Configureaza PA2 ca AF1 (USART1_TX): MODER=10, AF=1
    //    Configureaza PA3 ca AF1 (USART1_RX): MODER=10, AF=1
    //    MODER: 2 biti per pin, pozitii PA2=bits[5:4], PA3=bits[7:6]
    GPIOA->MODER &= ~(GPIO_MODER_MODE2_Msk | GPIO_MODER_MODE3_Msk);
    GPIOA->MODER |=  (0x02U << GPIO_MODER_MODE2_Pos) |   // AF mode
                     (0x02U << GPIO_MODER_MODE3_Pos);     // AF mode

    // AFR[0]: AF pentru PA0-PA7. PA2=bits[11:8], PA3=bits[15:12]. AF1=0x1
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2_Msk | GPIO_AFRL_AFSEL3_Msk);
    GPIOA->AFR[0] |=  (1U << GPIO_AFRL_AFSEL2_Pos) |     // AF1 = USART1_TX
                      (1U << GPIO_AFRL_AFSEL3_Pos);       // AF1 = USART1_RX

    // PA2 push-pull fara pull: OTYPER=0, OSPEEDR=10 (high speed), PUPDR=00
    GPIOA->OTYPER  &= ~(GPIO_OTYPER_OT2 | GPIO_OTYPER_OT3);
    GPIOA->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED2_Msk | GPIO_OSPEEDR_OSPEED3_Msk);
    GPIOA->OSPEEDR |=  (0x02U << GPIO_OSPEEDR_OSPEED2_Pos) |
                       (0x02U << GPIO_OSPEEDR_OSPEED3_Pos);
    GPIOA->PUPDR   &= ~(GPIO_PUPDR_PUPD2_Msk | GPIO_PUPDR_PUPD3_Msk);

    // 3. Configureaza USART1: 8N1, baudrate dat
    //    Clock USART1 = PCLK2 = HCLK = 48MHz (HSI48 default)
    //    BRR = FCLK / BAUD. Ex: 48000000 / 115200 = 417
    USART1->CR1 = 0;     // Dezactiveaza USART inainte de configurare
    USART1->CR2 = 0;
    USART1->CR3 = 0;

    USART1->BRR = (uint32_t)(48000000UL / baudrate);

    // 4. Activeaza USART1 cu TX, RX si intrerupere RXNE
    USART1->CR1 = USART_CR1_UE        |   // USART enable
                  USART_CR1_TE        |   // Transmitter enable
                  USART_CR1_RE        |   // Receiver enable
                  USART_CR1_RXNEIE_RXFNEIE; // RX not empty interrupt

    // 5. Activeaza intreruperea USART1 in NVIC, prioritate medie
    NVIC_SetPriority(USART1_IRQn, 1);
    NVIC_EnableIRQ(USART1_IRQn);
}

// ============================================================================
// HANDLER INTRERUPERE RECEPTIE
// ============================================================================

void USART1_IRQHandler(void)
{
    if (USART1->ISR & USART_ISR_RXNE_RXFNE)
    {
        uint8_t ch = (uint8_t)(USART1->RDR & 0xFFU);
        uint16_t next = (rxHead + 1U) % UART_RX_BUF_SIZE;

        if (next != rxTail)
        {
            // Buffer are loc: salveaza caracterul
            rxBuf[rxHead] = ch;
            rxHead = next;
        }
        // Daca buffer-ul e plin, caracterul se pierde (overflow silentios)
    }

    // Sterge flag erori (framing, overrun, noise) pentru a nu bloca UART
    if (USART1->ISR & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE))
    {
        USART1->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF;
    }
}

// ============================================================================
// TRIMITERE
// ============================================================================

void uartSendChar(char c)
{
    while (!(USART1->ISR & USART_ISR_TXE_TXFNF))
        ;   // Asteapta buffer TX gol
    USART1->TDR = (uint8_t)c;
}

void uartSendString(const char* s)
{
    while (*s)
        uartSendChar(*s++);
}

void uartSendBytes(const uint8_t* data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
        uartSendChar((char)data[i]);
}

// ============================================================================
// RECEPTIE
// ============================================================================

int uartGetChar(char* c)
{
    if (rxHead == rxTail)
        return 0;   // Buffer gol

    *c = (char)rxBuf[rxTail];
    rxTail = (rxTail + 1U) % UART_RX_BUF_SIZE;
    return 1;
}

uint16_t uartAvailable(void)
{
    // Calcul corect inclusiv wrap-around
    return (uint16_t)((rxHead - rxTail + UART_RX_BUF_SIZE) % UART_RX_BUF_SIZE);
}

void uartFlushRx(void)
{
    rxTail = rxHead;
}

// ============================================================================
// CITIRE LINIE CU TIMEOUT
//
// Citeste caractere pana la '\n' sau pana la expirarea timeout-ului.
// Elimina '\r'. Pune null-terminator la final.
// Returneaza lungimea liniei (0 = linie goala), -1 la timeout.
// ============================================================================

int uartReadLine(char* buf, int maxLen, uint32_t timeoutMs)
{
    int pos = 0;
    uint32_t start = getTickMs();
    char c;

    while ((getTickMs() - start) < timeoutMs)
    {
        if (uartGetChar(&c))
        {
            if (c == '\n')
            {
                // Sfarsit linie
                buf[pos] = '\0';
                return pos;
            }
            if (c == '\r')
                continue;   // Ignora \r

            if (pos < (maxLen - 1))
            {
                buf[pos++] = c;
            }
            // Daca buffer-ul e plin, continua sa citeasca dar arunca caracterele
        }
        // Yield mic pentru a nu bloca WDT - feedWatchdog e in delayMs
        // Aici nu dormim, citim activ (polling)
    }

    // Timeout
    buf[pos] = '\0';
    return -1;
}
