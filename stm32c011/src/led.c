// ============================================================================
// led.c - Control LED-uri (verde + galben + rosu)
// ERGO GASALERT v5.0 - STM32C011 + A7682E
//
// Comportament identic cu versiunea OpenCPU:
//
//  +----------------------------------+------------------+------------------+------------------+
//  | STARE                            | LED VERDE        | LED GALBEN       | LED ROSU         |
//  +----------------------------------+------------------+------------------+------------------+
//  | Boot (initializare soft)         | APRINS FIX       | STINS            | STINS            |
//  | Soft OK, cauta retea LTE         | Clipeste 0.5s    | STINS            | STINS            |
//  | Soft OK, conectat LTE            | Clipeste 0.5s    | Clipeste 0.5s    | STINS            |
//  | Tensiune pe intrare (< 0.8s)     | Clipeste 0.5s    | Clipeste/Stins   | APRINS FIX       |
//  | Impuls valid detectat (3 sec)    | APRINS FIX       | APRINS FIX       | APRINS FIX       |
//  | Dupa 3s, intrare inactiva        | Revine clipire   | Revine clipire   | STINS            |
//  +----------------------------------+------------------+------------------+------------------+
//
// GPIO: PA0=verde, PA1=galben, PA4=rosu (push-pull output, active HIGH)
// ============================================================================

#include "stm32c0xx.h"

#include "../include/ergo_pins.h"
#include "../include/ergo_config.h"
#include "../include/ergo_led.h"

// ============================================================================
// VARIABILE LOCALE
// ============================================================================

static int           ledModImpuls       = 0;
static uint32_t      timpStartModImpuls = 0;
static uint32_t      timpStartLedVerde  = 0;
static uint32_t      timpStartLedGalben = 0;
static int           inBoot             = 0;

// ============================================================================
// INITIALIZARE GPIO LED-URI
// ============================================================================

void initLED(void)
{
    // Activeaza ceasul GPIOA (poate fi deja activ din uart.c, nu dauneaza)
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

    // PA0 (LED verde): output push-pull
    GPIOA->MODER &= ~GPIO_MODER_MODE0_Msk;
    GPIOA->MODER |=  (0x01U << GPIO_MODER_MODE0_Pos);
    GPIO_CLR(GPIOA, 0);

    // PA1 (LED galben): output push-pull
    GPIOA->MODER &= ~GPIO_MODER_MODE1_Msk;
    GPIOA->MODER |=  (0x01U << GPIO_MODER_MODE1_Pos);
    GPIO_CLR(GPIOA, 1);

    // PA4 (LED rosu): output push-pull
    GPIOA->MODER &= ~GPIO_MODER_MODE4_Msk;
    GPIOA->MODER |=  (0x01U << GPIO_MODER_MODE4_Pos);
    GPIO_CLR(GPIOA, 4);
}

// ============================================================================
// BOOT: LED verde aprins fix, galben si rosu stinse
// ============================================================================

void ledBootStart(void)
{
    inBoot = 1;
    GPIO_SET(PIN_LED_VERDE_PORT,  PIN_LED_VERDE_BIT);
    GPIO_CLR(PIN_LED_GALBEN_PORT, PIN_LED_GALBEN_BIT);
    GPIO_CLR(PIN_LED_ROSU_PORT,   PIN_LED_ROSU_BIT);
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
// ACTIVEAZA MOD IMPULS: toate 3 LED-uri aprinse fix 3 secunde
// Apelata DUPA trimiteSMSAlarma() - timer-ul porneste de la terminarea trimiterii.
// ============================================================================

void activeazaModImpulsLED(void)
{
    ledModImpuls       = 1;
    timpStartModImpuls = getTickMs();

    GPIO_SET(PIN_LED_VERDE_PORT,  PIN_LED_VERDE_BIT);
    GPIO_SET(PIN_LED_GALBEN_PORT, PIN_LED_GALBEN_BIT);
    GPIO_SET(PIN_LED_ROSU_PORT,   PIN_LED_ROSU_BIT);
}

// ============================================================================
// ACTUALIZARE LED-URI (apelata din loop la fiecare 50ms)
// ============================================================================

void actualizeazaLeduri(void)
{
    uint32_t acum = getTickMs();

    // In boot: verde e aprins fix, nu modificam nimic
    if (inBoot)
        return;

    // Mod impuls: toate 3 aprinse fix 3 secunde
    if (ledModImpuls)
    {
        if (acum - timpStartModImpuls >= LED_IMPULS_DURATA_MS)
        {
            // Expira mod impuls -> revenire la clipire normala
            ledModImpuls       = 0;
            timpStartLedVerde  = acum;
            timpStartLedGalben = acum;
            // Cade mai jos pentru a aplica imediat starea normala
        }
        else
        {
            GPIO_SET(PIN_LED_VERDE_PORT,  PIN_LED_VERDE_BIT);
            GPIO_SET(PIN_LED_GALBEN_PORT, PIN_LED_GALBEN_BIT);
            GPIO_SET(PIN_LED_ROSU_PORT,   PIN_LED_ROSU_BIT);
            return;
        }
    }

    // ---- MOD NORMAL ----

    // LED VERDE: clipeste mereu ON 0.5s / OFF 0.5s (firmware OK)
    {
        uint32_t pv = LED_VERDE_ON_MS + LED_VERDE_OFF_MS;
        uint32_t pos = (acum - timpStartLedVerde) % pv;
        if (pos < LED_VERDE_ON_MS)
            GPIO_SET(PIN_LED_VERDE_PORT, PIN_LED_VERDE_BIT);
        else
            GPIO_CLR(PIN_LED_VERDE_PORT, PIN_LED_VERDE_BIT);
    }

    // LED GALBEN: clipeste daca conectat LTE, stins daca nu
    if (reteaConectata)
    {
        uint32_t pg = LED_GALBEN_ON_MS + LED_GALBEN_OFF_MS;
        uint32_t pos = (acum - timpStartLedGalben) % pg;
        if (pos < LED_GALBEN_ON_MS)
            GPIO_SET(PIN_LED_GALBEN_PORT, PIN_LED_GALBEN_BIT);
        else
            GPIO_CLR(PIN_LED_GALBEN_PORT, PIN_LED_GALBEN_BIT);
    }
    else
    {
        GPIO_CLR(PIN_LED_GALBEN_PORT, PIN_LED_GALBEN_BIT);
    }

    // LED ROSU: reflecta starea fizica a intrarii in timp real
    if (intrareActiva)
        GPIO_SET(PIN_LED_ROSU_PORT, PIN_LED_ROSU_BIT);
    else
        GPIO_CLR(PIN_LED_ROSU_PORT, PIN_LED_ROSU_BIT);
}
