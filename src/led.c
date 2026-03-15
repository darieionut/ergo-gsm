// ============================================================================
// led.c - Control LED-uri (verde + galben)
// ERGO GASALERT - Modul GSM Notificare SMS (4G)
// ============================================================================
//
// COMPORTAMENT LED-URI:
//
//  +--------------------------------+------------------+------------------+------------------+
//  | STARE                          | LED VERDE        | LED GALBEN       | LED ROSU         |
//  +--------------------------------+------------------+------------------+------------------+
//  | Boot (initializare soft)       | APRINS FIX       | STINS            | STINS            |
//  | Soft OK, cauta retea           | Clipeste 0.5s    | STINS            | STINS            |
//  | Soft OK, conectat 4G           | Clipeste 0.5s    | Clipeste 0.5s    | STINS            |
//  | Tensiune pe intrare (< 0.8s)   | Clipeste 0.5s    | Clipeste/Stins   | APRINS FIX       |
//  | Impuls detectat (3 secunde)    | APRINS FIX       | APRINS FIX       | APRINS FIX       |
//  | Dupa 3s, intrare inactiva      | Revine clipire   | Revine clipire   | STINS            |
//  +--------------------------------+------------------+------------------+------------------+
//
// ============================================================================

#include "simcom_os.h"
#include "simcom_common.h"
#include "simcom_debug.h"
#include "simcom_gpio.h"

#include "../include/ergo_pins.h"
#include "../include/ergo_config.h"
#include "../include/ergo_led.h"

// ============================================================================
// VARIABILE LOCALE
// ============================================================================

// Mod impuls: ambele aprinse fix 3 secunde
static int ledModImpuls = 0;
static unsigned long timpStartModImpuls = 0;

// Referinta timp pentru clipire
static unsigned long timpStartLedVerde = 0;
static unsigned long timpStartLedGalben = 0;

// Stare boot: 1 = in boot (verde aprins fix), 0 = boot finalizat (clipire)
static int inBoot = 0;

// ============================================================================
// INITIALIZARE LED-URI
// ============================================================================

void initLED(void)
{
    sAPI_Debug("[LED] Init: verde + galben + rosu...");

    // Verde - iesire, stins initial
    sAPI_GpioSetDirection(PIN_LED_VERDE, SC_MODULE_GPIO_OUTPUT);
    sAPI_GpioSetValue(PIN_LED_VERDE, 0);

    // Galben - iesire, stins initial
    sAPI_GpioSetDirection(PIN_LED_GALBEN, SC_MODULE_GPIO_OUTPUT);
    sAPI_GpioSetValue(PIN_LED_GALBEN, 0);

    // Rosu - iesire, stins initial
    sAPI_GpioSetDirection(PIN_LED_ROSU, SC_MODULE_GPIO_OUTPUT);
    sAPI_GpioSetValue(PIN_LED_ROSU, 0);

    sAPI_Debug("[LED] OK.");
}

// ============================================================================
// BOOT: LED verde aprins fix, galben stins
// ============================================================================

void ledBootStart(void)
{
    inBoot = 1;
    sAPI_GpioSetValue(PIN_LED_VERDE, 1);   // Verde APRINS FIX
    sAPI_GpioSetValue(PIN_LED_GALBEN, 0);  // Galben STINS
    sAPI_GpioSetValue(PIN_LED_ROSU, 0);    // Rosu STINS
}

// ============================================================================
// SFARSIT BOOT: LED verde trece la clipire
// ============================================================================

void ledBootEnd(void)
{
    inBoot = 0;
    timpStartLedVerde = getTickMs();
    timpStartLedGalben = getTickMs();
}

// ============================================================================
// ACTIVEAZA MOD IMPULS: ambele aprinse fix 3 secunde
// Fix #2: timpStartModImpuls se seteaza la TERMINAREA trimiterii SMS (apelantul
// apeleaza aceasta functie DUPA trimiteSMSAlarma), astfel incat cele 3 secunde
// de LED sunt calculate de la sfarsitul trimiterii, nu de la inceputul ei.
// ============================================================================

void activeazaModImpulsLED(void)
{
    ledModImpuls = 1;
    timpStartModImpuls = getTickMs();

    sAPI_GpioSetValue(PIN_LED_VERDE, 1);
    sAPI_GpioSetValue(PIN_LED_GALBEN, 1);
    sAPI_GpioSetValue(PIN_LED_ROSU, 1);

    sAPI_Debug("[LED] MOD IMPULS: toate 3 aprinse fix 3s.");
}

// ============================================================================
// ACTUALIZARE LED-URI (apelata din loop la fiecare 50ms)
// ============================================================================

void actualizeazaLeduri(void)
{
    unsigned long acum = getTickMs();

    // -----------------------------------------------------------
    // In timpul boot-ului nu facem nimic (verde e aprins fix)
    // -----------------------------------------------------------
    if (inBoot)
        return;

    // -----------------------------------------------------------
    // MOD IMPULS: toate 3 aprinse fix 3 secunde
    // -----------------------------------------------------------
    if (ledModImpuls)
    {
        if (acum - timpStartModImpuls >= LED_IMPULS_DURATA_MS)
        {
            // Revenire la clipire normala.
            // Fix #13: nu mai facem return - continuam mai jos pentru a aplica
            // imediat starea normala (inclusiv stingerea LED-ului rosu daca
            // intrarea e inactiva), fara a astepta 50ms pana la urmatoarea iteratie.
            ledModImpuls = 0;
            timpStartLedVerde = acum;
            timpStartLedGalben = acum;
            sAPI_Debug("[LED] Revenire la clipire normala.");
        }
        else
        {
            // Toate 3 aprinse fix
            sAPI_GpioSetValue(PIN_LED_VERDE, 1);
            sAPI_GpioSetValue(PIN_LED_GALBEN, 1);
            sAPI_GpioSetValue(PIN_LED_ROSU, 1);
            return;
        }
    }

    // -----------------------------------------------------------
    // MOD NORMAL
    // -----------------------------------------------------------

    // LED VERDE: clipeste mereu ON 0.5s / OFF 0.5s (firmware OK)
    {
        unsigned long perioadaVerde = LED_VERDE_ON_MS + LED_VERDE_OFF_MS;
        unsigned long pozitieVerde = (acum - timpStartLedVerde) % perioadaVerde;

        sAPI_GpioSetValue(PIN_LED_VERDE, pozitieVerde < LED_VERDE_ON_MS ? 1 : 0);
    }

    // LED GALBEN: depinde de starea retelei
    if (reteaConectata)
    {
        // CONECTAT 4G: clipeste ON 0.5s / OFF 0.5s
        unsigned long perioadaGalben = LED_GALBEN_ON_MS + LED_GALBEN_OFF_MS;
        unsigned long pozitieGalben = (acum - timpStartLedGalben) % perioadaGalben;

        sAPI_GpioSetValue(PIN_LED_GALBEN, pozitieGalben < LED_GALBEN_ON_MS ? 1 : 0);
    }
    else
    {
        // FARA RETEA: galben STINS complet
        sAPI_GpioSetValue(PIN_LED_GALBEN, 0);
    }

    // LED ROSU: reflecta starea fizica a intrarii in timp real
    // Aprins = tensiune prezenta pe intrare (230V detectat)
    // Stins  = intrare inactiva
    sAPI_GpioSetValue(PIN_LED_ROSU, intrareActiva ? 1 : 0);
}
