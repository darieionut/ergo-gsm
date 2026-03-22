// ============================================================================
// ergo_pins.h - Definitii pini GPIO STM32C011F4U6TR
// ERGO GASALERT v5.0 - STM32C011 + A7682E
//
// Placa: HXY-A7682E-STM32-V1.0
// MCU:   STM32C011F4U6TR (Cortex-M0+, 32KB Flash, 6KB RAM, UFQFPN20)
// GSM:   SIMCom A7682E (LTE Cat 1, comunicatie prin AT commands UART)
//
// Atentie: pinii exacti trebuie verificati pe schema electronica.
// ============================================================================

#ifndef ERGO_PINS_H
#define ERGO_PINS_H

#include "stm32c0xx.h"

// ============================================================================
// PINOUT STM32C011F4U6TR (UFQFPN20)
//
// PA0  - LED_VERDE     (GPIO output, push-pull)
// PA1  - LED_GALBEN    (GPIO output, push-pull)
// PA4  - LED_ROSU      (GPIO output, push-pull)
// PA5  - PIN_INTRARE   (GPIO input, pull-down, optocuplor 230V)
// PA6  - GSM_PWRKEY    (GPIO output, push-pull -> tranzistor -> PWRKEY A7682E)
// PA2  - USART1_TX     (AF1 -> A7682E RX)
// PA3  - USART1_RX     (AF1 <- A7682E TX)
// PA13 - SWDIO         (programare/debug SWD)
// PA14 - SWCLK         (programare/debug SWD)
// NRST - Reset MCU
// ============================================================================

// LED Verde: stare firmware (PIN PA0)
#define PIN_LED_VERDE_PORT   GPIOA
#define PIN_LED_VERDE_BIT    0U

// LED Galben: stare retea 4G (PIN PA1)
#define PIN_LED_GALBEN_PORT  GPIOA
#define PIN_LED_GALBEN_BIT   1U

// LED Rosu: tensiune prezenta pe intrare (PIN PA4)
#define PIN_LED_ROSU_PORT    GPIOA
#define PIN_LED_ROSU_BIT     4U

// Intrare optocuplor 230V AC (PIN PA5, pull-down)
#define PIN_INTRARE_PORT     GPIOA
#define PIN_INTRARE_BIT      5U

// Cheie pornire modul GSM A7682E (PIN PA6)
// PA6 HIGH -> tranzistor ON -> PWRKEY A7682E LOW (activ) -> pornire modul
#define PIN_GSM_PWRKEY_PORT  GPIOA
#define PIN_GSM_PWRKEY_BIT   6U

// USART1 pentru AT commands cu A7682E
// PA2 = USART1_TX (AF1), PA3 = USART1_RX (AF1)
#define GSM_USART            USART1
#define GSM_USART_IRQn       USART1_IRQn
#define GSM_USART_CLK_EN()   (RCC->APBENR2 |= RCC_APBENR2_USART1EN)
#define GSM_GPIOA_CLK_EN()   (RCC->IOPENR  |= RCC_IOPENR_GPIOAEN)

// ============================================================================
// MACRO-URI CONVENABILE GPIO
// ============================================================================
#define GPIO_SET(port, bit)    ((port)->BSRR = (1U << (bit)))
#define GPIO_CLR(port, bit)    ((port)->BSRR = (1U << ((bit) + 16U)))
#define GPIO_READ(port, bit)   (((port)->IDR >> (bit)) & 1U)
#define GPIO_TOGGLE(port, bit) ((port)->ODR ^= (1U << (bit)))

#endif // ERGO_PINS_H
