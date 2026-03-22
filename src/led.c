// ============================================================================
// led.c - Control LED-uri (verde + galben + rosu)
// ERGO GASALERT v5.0 - STM32C011F4U6TR + SIMCom A7682E
// ============================================================================
//
// Logica identica cu v4.x. Singura diferenta: GPIO controlat via HAL_GPIO_WritePin.
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

#include "stm32c0xx_hal.h"

#include "../include/ergo_pins.h"
#include "../include/ergo_config.h"
#include "../include/ergo_led.h"

// ============================================================================
// MACROS GPIO (simplifica scrierea)
// ============================================================================

#define LED_ON(port, pin)   HAL_GPIO_WritePin((port), (pin), GPIO_PIN_SET)
#define LED_OFF(port, pin)  HAL_GPIO_WritePin((port), (pin), GPIO_PIN_RESET)

// ============================================================================
// VARIABILE LOCALE
// ============================================================================

static int           ledModImpuls        = 0;
static unsigned long timpStartModImpuls  = 0;

static unsigned long timpStartLedVerde   = 0;
static unsigned long timpStartLedGalben  = 0;

static int inBoot = 0;

// ============================================================================
// INITIALIZARE LED-URI
// GPIO configurat in MX_GPIO_Init() (output PP, initial LOW = stins).
// ============================================================================

void initLED(void)
{
    LED_OFF(LED_VERDE_PORT,  LED_VERDE_PIN);
    LED_OFF(LED_GALBEN_PORT, LED_GALBEN_PIN);
    LED_OFF(LED_ROSU_PORT,   LED_ROSU_PIN);
    dbg("[LED] Init OK.");
}

// ============================================================================
// BOOT: LED verde aprins fix, restul stinse
// ============================================================================

void ledBootStart(void)
{
    inBoot = 1;
    LED_ON (LED_VERDE_PORT,  LED_VERDE_PIN);
    LED_OFF(LED_GALBEN_PORT, LED_GALBEN_PIN);
    LED_OFF(LED_ROSU_PORT,   LED_ROSU_PIN);
}

// ============================================================================
// SFARSIT BOOT: LED verde trece la clipire
// ============================================================================

void ledBootEnd(void)
{
    inBoot = 0;
    timpStartLedVerde  = getTickMs();
    timpStartLedGalben = getTickMs();
}

// ============================================================================
// ACTIVEAZA MOD IMPULS: toate 3 aprinse fix 3 secunde
// Apelata DUPA trimiteSMSAlarma() (timer-ul curge de la sfarsitul trimiterii).
// ============================================================================

void activeazaModImpulsLED(void)
{
    ledModImpuls       = 1;
    timpStartModImpuls = getTickMs();

    LED_ON(LED_VERDE_PORT,  LED_VERDE_PIN);
    LED_ON(LED_GALBEN_PORT, LED_GALBEN_PIN);
    LED_ON(LED_ROSU_PORT,   LED_ROSU_PIN);

    dbg("[LED] MOD IMPULS: toate 3 aprinse fix 3s.");
}

// ============================================================================
// ACTUALIZARE LED-URI (apelata din loop la fiecare 50ms)
// ============================================================================

void actualizeazaLeduri(void)
{
    unsigned long acum = getTickMs();

    if (inBoot)
        return;

    // --- MOD IMPULS: toate 3 aprinse fix 3 secunde ---
    if (ledModImpuls)
    {
        if (acum - timpStartModImpuls >= LED_IMPULS_DURATA_MS)
        {
            ledModImpuls       = 0;
            timpStartLedVerde  = acum;
            timpStartLedGalben = acum;
            dbg("[LED] Revenire la clipire normala.");
            // Continua mai jos pentru a aplica imediat starea normala
        }
        else
        {
            LED_ON(LED_VERDE_PORT,  LED_VERDE_PIN);
            LED_ON(LED_GALBEN_PORT, LED_GALBEN_PIN);
            LED_ON(LED_ROSU_PORT,   LED_ROSU_PIN);
            return;
        }
    }

    // --- MOD NORMAL ---

    // LED VERDE: clipeste mereu ON 0.5s / OFF 0.5s (firmware OK)
    {
        unsigned long perioada = LED_VERDE_ON_MS + LED_VERDE_OFF_MS;
        unsigned long pozitie  = (acum - timpStartLedVerde) % perioada;

        if (pozitie < LED_VERDE_ON_MS)
            LED_ON(LED_VERDE_PORT, LED_VERDE_PIN);
        else
            LED_OFF(LED_VERDE_PORT, LED_VERDE_PIN);
    }

    // LED GALBEN: clipeste daca retea 4G, stins daca nu
    if (reteaConectata)
    {
        unsigned long perioada = LED_GALBEN_ON_MS + LED_GALBEN_OFF_MS;
        unsigned long pozitie  = (acum - timpStartLedGalben) % perioada;

        if (pozitie < LED_GALBEN_ON_MS)
            LED_ON(LED_GALBEN_PORT, LED_GALBEN_PIN);
        else
            LED_OFF(LED_GALBEN_PORT, LED_GALBEN_PIN);
    }
    else
    {
        LED_OFF(LED_GALBEN_PORT, LED_GALBEN_PIN);
    }

    // LED ROSU: reflecta starea fizica a intrarii in timp real
    if (intrareActiva)
        LED_ON(LED_ROSU_PORT, LED_ROSU_PIN);
    else
        LED_OFF(LED_ROSU_PORT, LED_ROSU_PIN);
}
