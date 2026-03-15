// ============================================================================
// network.c - Conectare retea GSM/LTE Orange Romania
// ERGO GASALERT - Modul GSM Notificare SMS (4G)
// ============================================================================

#include "simcom_os.h"
#include "simcom_common.h"
#include "simcom_debug.h"
#include "simcom_sms.h"
#include "simcom_network.h"
#include "simcom_wdt.h"

#include "../include/ergo_config.h"
#include "../include/ergo_network.h"
#include "../include/ergo_led.h"

// ============================================================================
// VARIABILA GLOBALA STARE RETEA
// ============================================================================
int reteaConectata = 0;

// ============================================================================
// INITIALIZARE RETEA
// ============================================================================
// Seteaza SMS text mode, charset GSM, apoi incearca conectarea.
// LED galben ramane STINS pana se conecteaza (controlat din led.c).
// ============================================================================

void initRetea(void)
{
    int tentative = 0;

    sAPI_Debug("[RETEA] Conectare Orange Romania...");
    sAPI_Debug("[RETEA] LED galben STINS pana la conectare.");

    // Setare SMS text mode (nu PDU)
    sAPI_SmsCfgMsgFormat(1);

    // Setare charset GSM (fara diacritice, compatibil SMS)
    sAPI_SmsCfgCharset("GSM");

    // Setare centru SMS Orange Romania (SMSC)
    sAPI_SmsCfgScaAddr(ORANGE_SMSC);
    sAPI_Debug("[RETEA] SMSC: %s", ORANGE_SMSC);

    // Incercare conectare (max 30 secunde)
    while (tentative < 15)
    {
        sAPI_WdtFeed();  // Alimenteaza WDT pe durata asteptarii retelei
        if (verificaConectareRetea())
        {
            sAPI_Debug("[RETEA] Conectat la Orange OK!");
            sAPI_Debug("[RETEA] LED galben -> clipire 0.5s.");
            return;
        }
        tentative++;
        delayMs(2000);
    }

    sAPI_Debug("[RETEA] TIMEOUT! LED galben ramane STINS.");
    sAPI_Debug("[RETEA] Va reincerca la fiecare 60s.");
}

// ============================================================================
// VERIFICARE STARE CONECTARE
// ============================================================================
// Returneaza 1 = conectat, 0 = neconectat.
// Actualizeaza variabila globala reteaConectata.
// La tranzitii (conectat<->deconectat) afiseaza mesaj debug.
// ============================================================================

int verificaConectareRetea(void)
{
    int regStatus = 0;
    int eraConectat = reteaConectata;

    sAPI_NetworkGetCgreg(&regStatus);

    // 1 = inregistrat home, 5 = inregistrat roaming
    if (regStatus == 1 || regStatus == 5)
    {
        reteaConectata = 1;

        // Tranzitie: neconectat -> conectat
        if (!eraConectat)
        {
            sAPI_Debug("[RETEA] 4G OK! LED galben -> clipire.");
        }
        return 1;
    }

    reteaConectata = 0;

    // Tranzitie: conectat -> neconectat
    if (eraConectat)
    {
        sAPI_Debug("[RETEA] 4G PIERDUT! LED galben -> STINS.");
    }

    return 0;
}

// ============================================================================
// RECONECTARE
// ============================================================================

void reconectareRetea(void)
{
    sAPI_Debug("[RETEA] Reconectare (o tentativa)...");

    // Fix #16: alimenteaza WDT inainte de sAPI_NetworkGetCgreg() care
    // poate bloca cateva secunde.
    sAPI_WdtFeed();

    // O singura tentativa - loop-ul principal reapeleaza la fiecare 60s.
    // Evita blocarea loop-ului principal cu retry + delayMs.
    if (verificaConectareRetea())
        sAPI_Debug("[RETEA] Reconectat OK!");
    else
        sAPI_Debug("[RETEA] Inca indisponibila. Va reincerca la 60s.");
}
