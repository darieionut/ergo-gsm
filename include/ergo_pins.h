// ============================================================================
// ergo_pins.h - Definire pini GPIO
// ERGO GASALERT - Modul GSM Notificare SMS (4G)
// Placa: HXY-A7670E-V1.3
// ============================================================================
//
// IMPORTANT: Pinii GPIO trebuie verificati pe schema electrica a placii
// HXY-A7670E-V1.3. Valorile de mai jos sunt orientative.
//
// ============================================================================

#ifndef ERGO_PINS_H
#define ERGO_PINS_H

// ----------------------------------------------------------------------------
// LED-URI (3 bucati)
// ----------------------------------------------------------------------------
#define PIN_LED_VERDE       SC_MODULE_GPIO_01    // Verde  - firmware OK
#define PIN_LED_GALBEN      SC_MODULE_GPIO_02    // Galben - conectat 4G
#define PIN_LED_ROSU        SC_MODULE_GPIO_03    // Rosu   - tensiune pe intrare

// ----------------------------------------------------------------------------
// INTRARE MONITORIZATA (de la optocuplor - detectare 230V AC)
// ----------------------------------------------------------------------------
#define PIN_INTRARE         SC_MODULE_GPIO_05    // HIGH = tensiune prezenta

#endif // ERGO_PINS_H
