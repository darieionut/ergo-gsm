// ============================================================================
// input.c - Monitorizare intrare 230V AC + detectare impuls + cooldown
// ERGO GASALERT v5.0 - STM32C011F4U6TR + SIMCom A7682E
// ============================================================================
//
// Logica identica cu v4.x. Singura diferenta: GPIO citit via HAL_GPIO_ReadPin.
//
// PIN_INTRARE: PA5, input cu pull-down intern.
// HIGH = tensiune 230V prezenta pe intrare (optocuplor conduce).
// LOW  = intrare inactiva.
//
// ============================================================================

#include "stm32c0xx_hal.h"

#include "../include/ergo_pins.h"
#include "../include/ergo_config.h"
#include "../include/ergo_input.h"
#include "../include/ergo_led.h"
#include "../include/ergo_sms.h"

// ============================================================================
// VARIABILE LOCALE
// ============================================================================

static int           impulsInCurs    = 0;
static unsigned long timpStartImpuls = 0;
static int           impulsValidat   = 0;   // fix anti-retrigger

static int           inCooldown         = 0;
static unsigned long timpStartCooldown  = 0;

// Stare intrare in timp real (citita de led.c pentru LED rosu)
int intrareActiva = 0;

// ============================================================================
// INITIALIZARE PIN INTRARE
// GPIO configurat in MX_GPIO_Init() din main.c (input + pull-down).
// Aceasta functie este un placeholder pentru debug.
// ============================================================================

void initIntrare(void)
{
    dbg("[INPUT] Pin intrare PA5 init OK (pull-down).");
}

// ============================================================================
// MONITORIZARE INTRARE + DETECTARE IMPULS (apelata la 10ms)
// ============================================================================

void monitorizareIntrare(void)
{
    GPIO_PinState pin;
    int stareCurenta;
    unsigned long acum = getTickMs();

    pin = HAL_GPIO_ReadPin(INTRARE_PORT, INTRARE_PIN);
    stareCurenta = (pin == GPIO_PIN_SET) ? 1 : 0;

    // Actualizeaza starea pentru LED rosu
    intrareActiva = stareCurenta;

    if (stareCurenta == 0)
    {
        // INTRARE INACTIVA: reseteaza starea pentru urmatorul impuls
        if (impulsInCurs)
        {
            unsigned long durata = acum - timpStartImpuls;
            if (durata < DURATA_IMPULS_MS)
                dbg("[INPUT] Zgomot ignorat (< 0.8s).");
            impulsInCurs = 0;
        }
        impulsValidat = 0;  // permite detectarea unui nou impuls
    }
    else if (stareCurenta == 1 && !impulsInCurs && !impulsValidat)
    {
        // INCEPUT IMPULS NOU
        impulsInCurs    = 1;
        timpStartImpuls = acum;
    }
    else if (stareCurenta == 1 && impulsInCurs)
    {
        // IMPULS IN DESFASURARE: verificam durata minima
        unsigned long durata = acum - timpStartImpuls;

        if (durata >= DURATA_IMPULS_MS)
        {
            // IMPULS VALID (>= 0.8s continuu)
            impulsInCurs  = 0;
            impulsValidat = 1;  // anti-retrigger pana la LOW->HIGH nou

            if (!inCooldown)
            {
                dbg("[!] IMPULS VALID (>= 0.8s) -> SMS ALARMA");

                // Trimitere SMS (blocant, poate dura 5-15s)
                trimiteSMSAlarma();

                // LED-uri aprinse fix 3s dupa terminarea trimiterii
                activeazaModImpulsLED();

                // Cooldown porneste dupa trimitere
                inCooldown        = 1;
                timpStartCooldown = getTickMs();
                dbg("[COOLDOWN] Blocare activa.");
            }
            else
            {
                dbg("[COOLDOWN] Impuls ignorat (in cooldown).");
            }
        }
    }
}

// ============================================================================
// GESTIONARE COOLDOWN (apelata la 10ms)
// ============================================================================

void gestionareCooldown(void)
{
    if (inCooldown)
    {
        unsigned long cooldownMs = (unsigned long)config.cooldownSecunde * 1000UL;
        if (getTickMs() - timpStartCooldown >= cooldownMs)
        {
            inCooldown = 0;
            dbg("[COOLDOWN] Expirat.");
        }
    }
}
