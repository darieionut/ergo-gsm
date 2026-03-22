// ============================================================================
// main.c - Entry point STM32C011F4U6TR + A7682E
// ERGO GASALERT v5.0
// Placa: HXY-A7682E-STM32-V1.0
// MCU:   STM32C011F4U6TR (Cortex-M0+, 32KB Flash, 6KB RAM, UFQFPN20)
// GSM:   SIMCom A7682E (LTE Cat 1)
// Retea: Orange Romania, SMSC +40744000060
// Autor: Plato Global SRL
//
// ARHITECTURA (diferenta fata de v4.2 OpenCPU):
//   v4.2: firmware rulat direct pe SIMCom A7670E (OpenCPU, sAPI_*)
//   v5.0: STM32C011 (MCU extern) comunica cu A7682E prin AT commands UART
//
// INIT SECVENTA:
//   1. SysTick 1ms
//   2. IWDG 30s
//   3. LED-uri (PA0=verde, PA1=galben, PA4=rosu)
//   4. Verde aprins fix (boot in curs)
//   5. Intrare optocuplor (PA5, pull-down)
//   6. Configuratie din Flash (page 15)
//   7. GSM: PWRKEY -> boot A7682E -> AT config -> retea
//   8. Verde trece la clipire (boot finalizat)
//
// LOOP PRINCIPAL:
//   - 10ms:   scanare intrare + gestionare cooldown + feed WDT
//   - 50ms:   actualizare LED-uri
//   - 1s:     verificare SMS primit (URC pending)
//   - 60s:    verificare retea + reconectare daca necesara
// ============================================================================

#include "stm32c0xx.h"
#include <stdint.h>

#include "../include/ergo_pins.h"
#include "../include/ergo_config.h"
#include "../include/ergo_uart.h"
#include "../include/ergo_gsm.h"
#include "../include/ergo_led.h"
#include "../include/ergo_input.h"
#include "../include/ergo_sms.h"

// ============================================================================
// SISTICK - COUNTER MILISECUNDE
// ============================================================================

static volatile uint32_t sysTickMs = 0;

void SysTick_Handler(void)
{
    sysTickMs++;
}

uint32_t getTickMs(void)
{
    return sysTickMs;
}

void delayMs(uint32_t ms)
{
    uint32_t start = getTickMs();
    while ((getTickMs() - start) < ms)
        feedWatchdog();
}

// ============================================================================
// WATCHDOG HARDWARE (IWDG STM32)
//
// LSI ~32kHz, prescaler /256 = 125Hz
// Reload = 3750 -> timeout = 3750 / 125 = 30 secunde
// Daca loop-ul principal se blocheaza >30s, MCU se reseteaza automat.
// ============================================================================

static void initWatchdog(void)
{
    IWDG->KR  = 0x5555U;   // Deblocare registre IWDG
    IWDG->PR  = 0x06U;     // Prescaler /256
    IWDG->RLR = 3750U;     // Reload 30s
    IWDG->KR  = 0xAAAAU;   // Reload imediat
    IWDG->KR  = 0xCCCCU;   // Start IWDG
}

void feedWatchdog(void)
{
    IWDG->KR = 0xAAAAU;
}

// ============================================================================
// ENTRY POINT
// ============================================================================

int main(void)
{
    // Ceasul implicit dupa reset: HSI48 = 48MHz (implicit STM32C011)
    // SystemCoreClock = 48000000 (setat de system_stm32c0xx.c)

    // ---- SysTick: 1ms la 48MHz ----
    SysTick_Config(48000U);     // 48000 ticks @ 48MHz = 1ms
    __enable_irq();

    // ---- IWDG: 30s timeout ----
    initWatchdog();

    // ---- LED-uri ----
    initLED();
    ledBootStart();             // Verde aprins fix (boot in curs)

    // ---- Intrare optocuplor ----
    initIntrare();

    // ---- Configuratie din Flash ----
    incarcaConfig();

    // ---- GSM: pornire A7682E + configurare AT + retea ----
    // Poate dura pana la 30s (PWRKEY + boot + inregistrare retea)
    initGSM();

    // ---- Boot finalizat: verde trece la clipire ----
    ledBootEnd();

    // ---- Timere loop principal ----
    uint32_t tScanare = 0;
    uint32_t tLED     = 0;
    uint32_t tSMS     = 0;
    uint32_t tRetea   = 0;

    // ============================================================================
    // LOOP PRINCIPAL (infinit)
    // ============================================================================
    while (1)
    {
        uint32_t acum = getTickMs();

        // ---- SCANARE INTRARE (10ms) ----
        if ((acum - tScanare) >= TIMER_SCAN_MS)
        {
            tScanare = acum;
            monitorizareIntrare();
            gestionareCooldown();
            feedWatchdog();     // Confirma ca loop-ul ruleaza
        }

        // ---- ACTUALIZARE LED-URI (50ms) ----
        if ((acum - tLED) >= TIMER_LED_MS)
        {
            tLED = acum;
            actualizeazaLeduri();
        }

        // ---- VERIFICARE SMS PRIMIT (1s) ----
        if ((acum - tSMS) >= VERIFICARE_SMS_MS)
        {
            tSMS = acum;
            verificaSMSPrimit();
        }

        // ---- VERIFICARE RETEA (60s) ----
        if ((acum - tRetea) >= TIMER_RETEA_MS)
        {
            tRetea = acum;
            if (!verificaConectareRetea())
                reconectareRetea();
        }
    }
}
