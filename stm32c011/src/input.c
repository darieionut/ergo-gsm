// ============================================================================
// input.c - Monitorizare intrare 230V AC + detectare impuls + cooldown
// ERGO GASALERT v5.0 - STM32C011 + A7682E
//
// Logica identica cu versiunea OpenCPU.
// Diferenta: citire GPIO prin registre STM32 (GPIO_READ macro)
//            in loc de sAPI_GpioGetValue().
//
// PA5 = intrare optocuplor (input pull-down)
// HIGH = 230V prezent pe intrare
// LOW  = intrare inactiva
//
// Algoritm:
//   1. Scanare la 10ms
//   2. HIGH apare -> start cronometru
//   3. Ramane HIGH >= 0.8s -> IMPULS VALID
//   4. Dispare < 0.8s -> zgomot, ignorat
//   5. Impuls valid + nu cooldown -> SMS + LED 3s + cooldown 20s
//   6. Impuls valid + in cooldown -> ignorat
// ============================================================================

#include "stm32c0xx.h"

#include "../include/ergo_pins.h"
#include "../include/ergo_config.h"
#include "../include/ergo_input.h"
#include "../include/ergo_led.h"
#include "../include/ergo_sms.h"

// ============================================================================
// VARIABILE LOCALE
// ============================================================================

static int      impulsInCurs    = 0;
static uint32_t timpStartImpuls = 0;

// Previne re-triggering cat timp intrarea ramane HIGH dupa impuls valid.
// Se reseteaza la HIGH->LOW (noua coborare a tensiunii).
static int impulsValidat = 0;

static int      inCooldown        = 0;
static uint32_t timpStartCooldown = 0;

// Stare intrare in timp real (citita de led.c pentru LED rosu)
int intrareActiva = 0;

// ============================================================================
// INITIALIZARE PIN INTRARE (PA5, input pull-down)
// ============================================================================

void initIntrare(void)
{
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

    // PA5: input mode (MODER=00)
    GPIOA->MODER &= ~GPIO_MODER_MODE5_Msk;

    // Pull-down (PUPDR=10 pentru PA5)
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD5_Msk;
    GPIOA->PUPDR |=  (0x02U << GPIO_PUPDR_PUPD5_Pos);
}

// ============================================================================
// MONITORIZARE INTRARE + DETECTARE IMPULS
// Apelata din loop la fiecare 10ms.
// ============================================================================

void monitorizareIntrare(void)
{
    uint32_t stareCurenta = GPIO_READ(PIN_INTRARE_PORT, PIN_INTRARE_BIT);
    uint32_t acum = getTickMs();

    intrareActiva = (int)stareCurenta;

    if (stareCurenta == 0)
    {
        // INTRARE INACTIVA
        impulsInCurs = 0;
        // Reseteaza impulsValidat -> urmatoarea urcare va fi impuls nou
        impulsValidat = 0;
    }
    else if (stareCurenta == 1 && !impulsInCurs && !impulsValidat)
    {
        // INCEPUT IMPULS NOU
        impulsInCurs    = 1;
        timpStartImpuls = acum;
    }
    else if (stareCurenta == 1 && impulsInCurs)
    {
        // IMPULS IN DESFASURARE
        uint32_t durataImpuls = acum - timpStartImpuls;

        if (durataImpuls >= DURATA_IMPULS_MS)
        {
            // IMPULS VALID (>= 0.8s continuu)
            impulsInCurs  = 0;
            impulsValidat = 1;   // Blocheaza re-triggering pana la LOW

            if (!inCooldown)
            {
                // Trimite SMS alarma
                trimiteSMSAlarma();

                // LED-urile se aprind DUPA trimitere (timer-ul de 3s
                // porneste de la terminarea efectiva a trimiterii)
                activeazaModImpulsLED();

                // Cooldown porneste dupa trimitere
                inCooldown        = 1;
                timpStartCooldown = getTickMs();
            }
            // Daca in cooldown: ignorat silentios
        }
    }
}

// ============================================================================
// GESTIONARE COOLDOWN
// Apelata din loop la fiecare 10ms.
// ============================================================================

void gestionareCooldown(void)
{
    if (inCooldown)
    {
        uint32_t cooldownMs = (uint32_t)config.cooldownSecunde * 1000UL;
        if ((getTickMs() - timpStartCooldown) >= cooldownMs)
            inCooldown = 0;
    }
}
