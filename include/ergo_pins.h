// ============================================================================
// ergo_pins.h - Definire pini GPIO STM32C011F4U6TR
// ERGO GASALERT v5.0 - STM32C011F4U6TR + SIMCom A7682E
// ============================================================================
//
// IMPORTANT: Pinii trebuie verificati pe schema electrica a placii finale.
// Valorile de mai jos sunt orientative pentru STM32C011F4U6TR (UFQFPN20).
//
// Asignare pini propusa:
//   PA0  -> LED verde    (output PP)
//   PA1  -> LED galben   (output PP)
//   PA4  -> LED rosu     (output PP)
//   PA5  -> Intrare optocuplor  (input, pull-down intern)
//   PA6  -> A7682E PWRKEY       (output PP, activ LOW)
//   PA9  -> USART1 TX   -> A7682E RX  (AF1)
//   PA10 -> USART1 RX   <- A7682E TX  (AF1)
//   PA13 -> SWDIO  (SWD debug/programare - rezervat)
//   PA14 -> SWDCLK (SWD debug/programare - rezervat)
//   PB6  -> USART2 TX   -> Debug UART optional (AF2)
//
// ============================================================================

#ifndef ERGO_PINS_H
#define ERGO_PINS_H

#include "stm32c0xx_hal.h"

// ----------------------------------------------------------------------------
// LED-URI (3 bucati) - output push-pull, activ HIGH
// ----------------------------------------------------------------------------
#define LED_VERDE_PORT      GPIOA
#define LED_VERDE_PIN       GPIO_PIN_0

#define LED_GALBEN_PORT     GPIOA
#define LED_GALBEN_PIN      GPIO_PIN_1

#define LED_ROSU_PORT       GPIOA
#define LED_ROSU_PIN        GPIO_PIN_4

// ----------------------------------------------------------------------------
// INTRARE MONITORIZATA (de la optocuplor - detectare 230V AC)
// HIGH = tensiune 230V prezenta pe intrare
// ----------------------------------------------------------------------------
#define INTRARE_PORT        GPIOA
#define INTRARE_PIN         GPIO_PIN_5

// ----------------------------------------------------------------------------
// MODUL A7682E - control pornire
// Pull LOW minim 500ms pentru pornire; HIGH = inactiv
// ----------------------------------------------------------------------------
#define GSM_PWRKEY_PORT     GPIOA
#define GSM_PWRKEY_PIN      GPIO_PIN_6

// ----------------------------------------------------------------------------
// USART pentru A7682E (AT commands) si debug
// ----------------------------------------------------------------------------
#define GSM_UART            USART1   // PA9=TX, PA10=RX, 115200 baud
#define DBG_UART            USART2   // PB6=TX (optional), debug output

#endif // ERGO_PINS_H
