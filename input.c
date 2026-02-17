// ============================================================================
// input.c - Monitorizare intrare 230V AC + detectare impuls + cooldown
// ERGO GASALERT - Modul GSM Notificare SMS (4G)
// ============================================================================
//
// LOGICA:
// 1. Cand apare tensiune pe intrare (optocuplor -> GPIO HIGH) -> start timer
// 2. Daca tensiunea ramane minim 0.8s continuu -> IMPULS VALID
// 3. Daca dispare inainte de 0.8s -> zgomot, ignorat
// 4. La impuls valid:
//    - NU cooldown -> LED-uri aprinse 3s + SMS alarma + cooldown 20s
//    - DA cooldown -> IGNORAT
//
// DIAGRAMA:
// Intrare:  ___████████___██___████████████___████████___
//               0.8s OK   <0.8  ignorat(cd)      0.8s OK
//                  ↓        ↓       ↓               ↓
// Actiune:    SMS TRIMIS  NIMIC   NIMIC         SMS TRIMIS
//                  |←─ 20s cooldown ─→|              |←─ 20s...
//
// ============================================================================

#include "simcom_os.h"
#include "simcom_common.h"
#include "simcom_debug.h"
#include "simcom_gpio.h"

#include "../include/ergo_pins.h"
#include "../include/ergo_config.h"
#include "../include/ergo_input.h"
#include "../include/ergo_led.h"
#include "../include/ergo_sms.h"

// ============================================================================
// VARIABILE LOCALE
// ============================================================================

// Monitorizare intrare
static int impulsInCurs = 0;
static unsigned long timpStartImpuls = 0;

// Cooldown
static int inCooldown = 0;
static unsigned long timpStartCooldown = 0;

// ============================================================================
// INITIALIZARE PIN INTRARE
// ============================================================================

void initIntrare(void)
{
    sAPI_Debug("[INPUT] Init pin intrare...");
    sAPI_GpioSetDirection(PIN_INTRARE, SC_MODULE_GPIO_INPUT);
    sAPI_Debug("[INPUT] OK.");
}

// ============================================================================
// MONITORIZARE INTRARE + DETECTARE IMPULS
// Apelata din loop principal la fiecare 10ms.
// ============================================================================

void monitorizareIntrare(void)
{
    int stareCurenta = 0;
    unsigned long acum = getTickMs();

    sAPI_GpioGetValue(PIN_INTRARE, &stareCurenta);

    if (stareCurenta == 1 && !impulsInCurs)
    {
        // INCEPUT IMPULS: tensiune tocmai a aparut
        impulsInCurs = 1;
        timpStartImpuls = acum;
    }
    else if (stareCurenta == 1 && impulsInCurs)
    {
        // IMPULS IN DESFASURARE: verificam durata
        unsigned long durataImpuls = acum - timpStartImpuls;

        if (durataImpuls >= DURATA_IMPULS_MS)
        {
            // IMPULS VALID (>= 0.8 secunde continuu)
            impulsInCurs = 0;

            if (!inCooldown)
            {
                sAPI_Debug("[!] IMPULS VALID (>= 0.8s) -> SMS ALARMA");

                // LED-uri aprinse fix 3 secunde
                activeazaModImpulsLED();

                // Trimitere SMS la toate numerele
                trimiteSMSAlarma();

                // Cooldown 20 secunde
                inCooldown = 1;
                timpStartCooldown = acum;
                sAPI_Debug("[COOLDOWN] Blocare 20s.");
            }
            else
            {
                unsigned long ramas = COOLDOWN_MS - (acum - timpStartCooldown);
                sAPI_Debug("[COOLDOWN] IGNORAT. Ramas: %lu s", ramas / 1000);
            }
        }
    }
    else if (stareCurenta == 0 && impulsInCurs)
    {
        // IMPULS PREA SCURT: tensiune disparuta inainte de 0.8s
        unsigned long durataImpuls = acum - timpStartImpuls;
        if (durataImpuls < DURATA_IMPULS_MS)
            sAPI_Debug("[INPUT] Prea scurt (%lu ms) - IGNORAT.", durataImpuls);
        impulsInCurs = 0;
    }
}

// ============================================================================
// GESTIONARE COOLDOWN
// Apelata din loop principal la fiecare 10ms.
// ============================================================================

void gestionareCooldown(void)
{
    if (inCooldown)
    {
        if (getTickMs() - timpStartCooldown >= COOLDOWN_MS)
        {
            inCooldown = 0;
            sAPI_Debug("[COOLDOWN] Expirat.");
        }
    }
}
