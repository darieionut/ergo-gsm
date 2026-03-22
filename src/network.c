// ============================================================================
// network.c - Conectare retea GSM/LTE Orange Romania (via AT commands A7682E)
// ERGO GASALERT v5.0 - STM32C011F4U6TR + SIMCom A7682E
// ============================================================================

#include "stm32c0xx_hal.h"
#include <string.h>

#include "../include/ergo_config.h"
#include "../include/ergo_gsm.h"
#include "../include/ergo_network.h"

// ============================================================================
// VARIABILA GLOBALA STARE RETEA
// ============================================================================

int reteaConectata = 0;

// ============================================================================
// INITIALIZARE RETEA
// Configureaza SMS text mode, charset GSM, SMSC Orange, apoi asteapta retea.
// 15 tentative x 2s = max ~30s timeout initial.
// ============================================================================

void initRetea(void)
{
    int tentative = 0;

    dbg("[RETEA] Conectare Orange Romania...");
    dbg("[RETEA] LED galben STINS pana la conectare.");

    // gsm_init() a configurat deja CMGF, CSCS, CSCA.
    // Asteptam inregistrarea in retea.

    while (tentative < 15)
    {
        alimenteazaWDT();
        if (verificaConectareRetea())
        {
            dbg("[RETEA] Conectat la Orange OK!");
            return;
        }
        tentative++;
        HAL_Delay(2000);
    }

    dbg("[RETEA] TIMEOUT! Va reincerca la fiecare 60s.");
}

// ============================================================================
// VERIFICARE STARE CONECTARE (AT+CREG?)
// Actualizeaza variabila globala reteaConectata.
// Returneaza 1 = conectat, 0 = neconectat.
// ============================================================================

int verificaConectareRetea(void)
{
    int eraConectat = reteaConectata;
    int conectat    = gsm_get_creg();

    reteaConectata = conectat;

    if (conectat && !eraConectat)
        dbg("[RETEA] 4G OK! LED galben -> clipire.");
    else if (!conectat && eraConectat)
        dbg("[RETEA] 4G PIERDUT! LED galben -> STINS.");

    return conectat;
}

// ============================================================================
// RECONECTARE (o singura tentativa, apelata din loop la 60s)
// ============================================================================

void reconectareRetea(void)
{
    alimenteazaWDT();
    dbg("[RETEA] Reconectare (o tentativa)...");

    if (verificaConectareRetea())
        dbg("[RETEA] Reconectat OK!");
    else
        dbg("[RETEA] Inca indisponibila. Va reincerca la 60s.");
}

// ============================================================================
// INTENSITATE SEMNAL GSM (AT+CSQ)
// Returneaza CSQ 0-31 (31=maxim) sau -1 la eroare.
// ============================================================================

int obtiSemnalCSQ(void)
{
    int csq = gsm_get_csq();
    if (csq < 0)
        dbg("[RETEA] Eroare citire CSQ.");
    return csq;
}
