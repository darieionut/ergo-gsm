// ============================================================================
// ergo_uart.h - Layer UART pentru comunicatie AT cu A7682E
// ERGO GASALERT v5.0 - STM32C011 + A7682E
// ============================================================================

#ifndef ERGO_UART_H
#define ERGO_UART_H

#include <stdint.h>

// Dimensiune buffer circular receptie UART
#define UART_RX_BUF_SIZE    256U

// ============================================================================
// INITIALIZARE
// ============================================================================

// Initializeaza USART1 (PA2=TX, PA3=RX) la baudrate dat (ex: 115200)
// Configureaza GPIO, clock, NVIC, buffer circular
void initUART(uint32_t baudrate);

// ============================================================================
// TRIMITERE
// ============================================================================

void uartSendChar(char c);
void uartSendString(const char* s);
void uartSendBytes(const uint8_t* data, uint16_t len);

// ============================================================================
// RECEPTIE (buffer circular alimentat de USART1_IRQHandler)
// ============================================================================

// Citeste un caracter din buffer circular. Returneaza 1 daca disponibil, 0 daca gol.
int uartGetChar(char* c);

// Returneaza numarul de bytes disponibili in buffer
uint16_t uartAvailable(void);

// Sterge tot buffer-ul de receptie
void uartFlushRx(void);

// ============================================================================
// CITIRE LINIE (cu timeout)
//
// Citeste pana la '\n' sau timeout. Elimina '\r'. Pune null-terminator.
// Returneaza lungimea liniei citite, sau -1 la timeout.
// Linia returnata nu contine '\n', nu contine '\r'.
// ============================================================================
int uartReadLine(char* buf, int maxLen, uint32_t timeoutMs);

#endif // ERGO_UART_H
