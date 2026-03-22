// ============================================================================
// main.c - Functia principala STM32C011F4U6TR + loop
// ERGO GASALERT v5.0 - STM32C011F4U6TR + SIMCom A7682E
// Retea: Orange Romania
// Autor: Plato Global SRL
// ============================================================================
//
// NOTA COMPILARE:
//   Necesita STM32CubeC0 HAL (STM32Cube_FW_C0).
//   Configureaza SystemClock, GPIO, USART1, USART2, IWDG via CubeMX
//   sau manual prin functiile MX_* de mai jos.
//
//   Alternate functions (verifica in datasheet STM32C011F4):
//     USART1: PA9=TX(AF1), PA10=RX(AF1)
//     USART2: PB6=TX(AF2) - optional debug
//
// ============================================================================

#include "stm32c0xx_hal.h"
#include <string.h>
#include <stdio.h>

#include "../include/ergo_pins.h"
#include "../include/ergo_config.h"
#include "../include/ergo_gsm.h"
#include "../include/ergo_led.h"
#include "../include/ergo_input.h"
#include "../include/ergo_sms.h"
#include "../include/ergo_network.h"

// ============================================================================
// HANDLES PERIFERICE
// ============================================================================

static UART_HandleTypeDef huart1;   // A7682E AT commands
static UART_HandleTypeDef huart2;   // Debug UART (optional)
static IWDG_HandleTypeDef hiwdg;

// ============================================================================
// WATCHDOG SI DEBUG (apelate din orice modul via ergo_config.h)
// ============================================================================

void alimenteazaWDT(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}

void dbg(const char *msg)
{
#ifdef DEBUG_UART_ENABLE
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 200);
    HAL_UART_Transmit(&huart2, (uint8_t *)"\r\n", 2, 50);
#else
    (void)msg;
#endif
}

// ============================================================================
// ISR CALLBACK - redirectioneaza receptia UART catre gsm.c
// ============================================================================

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    gsm_uart_rx_callback(huart);
}

// ============================================================================
// INITIALIZARE CLOCK
// STM32C011 porneste implicit cu HSI48 la 48MHz - nu necesita configurare.
// ============================================================================

static void SystemClock_Config(void)
{
    // HSI48 este sursa default dupa reset pe STM32C011.
    // SysTick configurat de HAL_Init() pentru 1ms tick.
    // Nimic suplimentar necesar pentru 48MHz.
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                 | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1);
}

// ============================================================================
// INITIALIZARE GPIO
// ============================================================================

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // --- LED-uri: PA0 (verde), PA1 (galben), PA4 (rosu) - output, stins initial ---
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // --- Intrare optocuplor: PA5 - input, pull-down (LOW = fara tensiune) ---
    GPIO_InitStruct.Pin  = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // --- A7682E PWRKEY: PA6 - output, HIGH initial (inactiv) ---
    HAL_GPIO_WritePin(GSM_PWRKEY_PORT, GSM_PWRKEY_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = GSM_PWRKEY_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GSM_PWRKEY_PORT, &GPIO_InitStruct);

    // --- USART1: PA9=TX(AF1), PA10=RX(AF1) ---
    // NOTA: verifica GPIO_AF1_USART1 in stm32c0xx_hal_gpio_ex.h pentru placa ta!
    GPIO_InitStruct.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

#ifdef DEBUG_UART_ENABLE
    // --- USART2 debug: PB6=TX(AF2) ---
    GPIO_InitStruct.Pin       = GPIO_PIN_6;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_USART2;    // verifica AF in datasheet
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
#endif
}

// ============================================================================
// INITIALIZARE USART1 (A7682E AT commands, 115200 baud)
// ============================================================================

static void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);

    // Activeaza intreruperea USART1
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

// ============================================================================
// INITIALIZARE USART2 (debug, optional)
// ============================================================================

#ifdef DEBUG_UART_ENABLE
static void MX_USART2_UART_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();

    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX;    // doar TX pentru debug
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}
#endif

// ============================================================================
// INITIALIZARE IWDG (Watchdog independent, ~28 secunde timeout)
// LSI ~32kHz, prescaler 256 => ~125Hz, reload 3500 => ~28s
// ============================================================================

static void MX_IWDG_Init(void)
{
    hiwdg.Instance       = IWDG;
    hiwdg.Init.Prescaler = WATCHDOG_PRESCALER;
    hiwdg.Init.Reload    = WATCHDOG_RELOAD;
    hiwdg.Init.Window    = IWDG_WINDOW_DISABLE;
    HAL_IWDG_Init(&hiwdg);
}

// ============================================================================
// ISR HANDLERS (redirectionate catre HAL)
// ============================================================================

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

// ============================================================================
// ENTRY POINT
// ============================================================================

int main(void)
{
    unsigned long acum;
    unsigned long timpUltimaScanare = 0;
    unsigned long timpUltimaLED     = 0;
    unsigned long timpUltimaSMS     = 0;
    unsigned long timpUltimaRetea   = 0;

    // --- Initializare HAL si clock ---
    HAL_Init();
    SystemClock_Config();

    // --- Initializare periferice ---
    MX_GPIO_Init();
    MX_USART1_UART_Init();
#ifdef DEBUG_UART_ENABLE
    MX_USART2_UART_Init();
#endif

    dbg("[ERGO] ==========================================");
    dbg("[ERGO] MODUL GSM ERGO GASALERT v5.0");
    dbg("[ERGO] MCU: STM32C011F4U6TR @ 48MHz");
    dbg("[ERGO] GSM: SIMCom A7682E (AT commands)");
    dbg("[ERGO] Retea: Orange Romania");
    dbg("[ERGO] ==========================================");

    // --- Boot: LED verde aprins fix ---
    initLED();
    ledBootStart();
    dbg("[BOOT] LED verde APRINS FIX (initializare...)");

    // --- Initializare intrare ---
    initIntrare();

    // --- Incarcare configuratie din Flash ---
    incarcaConfig();

    // --- Pornire Watchdog INAINTE de initGSM (poate dura pana la 15s) ---
    MX_IWDG_Init();
    dbg("[ERGO] Watchdog pornit (~28s timeout).");

    // --- Initializare modul GSM A7682E ---
    gsm_init(&huart1);

    // --- Initializare retea Orange Romania ---
    initRetea();

    // --- Sfarsit boot: LED verde trece la clipire ---
    ledBootEnd();
    dbg("[BOOT] COMPLET. LED verde -> clipire.");
    if (reteaConectata)
        dbg("[BOOT] Retea OK. LED galben -> clipire.");
    else
        dbg("[BOOT] Fara retea. LED galben STINS.");
    dbg("[ERGO] === SISTEM PORNIT ===");

    // ============================================================
    // LOOP PRINCIPAL (infinit)
    // ============================================================
    while (1)
    {
        acum = getTickMs();

        // ---- SCANARE INTRARE (10ms) ----
        if (acum - timpUltimaScanare >= TIMER_SCAN_MS)
        {
            timpUltimaScanare = acum;
            monitorizareIntrare();
            gestionareCooldown();
            alimenteazaWDT();
        }

        // ---- ACTUALIZARE LED-URI (50ms) ----
        if (acum - timpUltimaLED >= TIMER_LED_MS)
        {
            timpUltimaLED = acum;
            actualizeazaLeduri();
        }

        // ---- VERIFICARE SMS PRIMITE (1s) ----
        if (acum - timpUltimaSMS >= VERIFICARE_SMS_MS)
        {
            timpUltimaSMS = acum;
            verificaSMSPrimit();
        }

        // ---- VERIFICARE RETEA (60s) ----
        if (acum - timpUltimaRetea >= TIMER_RETEA_MS)
        {
            timpUltimaRetea = acum;
            if (!verificaConectareRetea())
            {
                dbg("[RETEA] Pierduta! Reconectare...");
                reconectareRetea();
            }
        }

        // Yield (10ms perioada minima a loop-ului)
        HAL_Delay(1);
    }
}
