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
//    - NU cooldown -> SMS alarma + LED-uri aprinse 3s + cooldown 20s
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
// Fix #1: previne re-triggering-ul cand tensiunea ramane HIGH dupa impuls valid.
// Impulsul e "consumat" si nu se mai restarteaza ciclul pana cand intrarea nu
// revine la 0 (HIGH → LOW → HIGH = impuls nou).
static int impulsValidat = 0;

// Cooldown
static int inCooldown = 0;
static unsigned long timpStartCooldown = 0;

// Stare intrare in timp real (citita de led.c pentru LED rosu)
int intrareActiva = 0;

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

    // Actualizeaza starea intrarii pentru LED rosu
    intrareActiva = stareCurenta;

    if (stareCurenta == 0)
    {
        // INTRARE INACTIVA: reseteaza starea pentru urmatorul impuls
        if (impulsInCurs)
        {
            // Tensiune disparuta inainte de 0.8s = zgomot
            unsigned long durataImpuls = acum - timpStartImpuls;
            if (durataImpuls < DURATA_IMPULS_MS)
                sAPI_Debug("[INPUT] Prea scurt (%lu ms) - IGNORAT.", durataImpuls);
            impulsInCurs = 0;
        }
        // Fix #1: la coborarea tensiunii, impulsValidat se reseteaza -> permite
        // detectarea unui nou impuls la urmatoarea urcare a tensiunii.
        impulsValidat = 0;
    }
    else if (stareCurenta == 1 && !impulsInCurs && !impulsValidat)
    {
        // INCEPUT IMPULS NOU: tensiune tocmai a aparut (si nu avem impuls activ
        // sau deja validat in acest ciclu de tensiune)
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
            // Fix #1: marcam ca validat; nu se va re-triggera pana la HIGH->LOW->HIGH
            impulsInCurs = 0;
            impulsValidat = 1;

            if (!inCooldown)
            {
                sAPI_Debug("[!] IMPULS VALID (>= 0.8s) -> SMS ALARMA");

                // Trimitere SMS la toate numerele (poate dura 5-10s)
                trimiteSMSAlarma();

                // Fix #2: LED-urile se aprind DUPA trimitere, astfel incat
                // timer-ul de 3s nu expira in timp ce SMS-urile sunt trimise.
                activeazaModImpulsLED();

                // Fix #3: cooldown porneste dupa terminarea efectiva a trimiterii.
                inCooldown = 1;
                timpStartCooldown = getTickMs();
                sAPI_Debug("[COOLDOWN] Blocare %ds.", config.cooldownSecunde);
            }
            else
            {
                unsigned long cooldownMs = (unsigned long)config.cooldownSecunde * 1000;
                unsigned long ramas = cooldownMs - (getTickMs() - timpStartCooldown);
                sAPI_Debug("[COOLDOWN] IGNORAT. Ramas: %lu s", ramas / 1000);
            }
        }
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
        unsigned long cooldownMs = (unsigned long)config.cooldownSecunde * 1000;
        if (getTickMs() - timpStartCooldown >= cooldownMs)
        {
            inCooldown = 0;
            sAPI_Debug("[COOLDOWN] Expirat.");
        }
    }
}
