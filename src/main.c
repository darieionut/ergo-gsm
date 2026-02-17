// ============================================================================
// main.c - Functia principala OpenCPU + loop
// ERGO GASALERT v4.2 - Modul GSM Notificare SMS (4G)
// Placa: HXY-A7670E-V1.3
// Retea: Orange Romania
// Autor: Plato Global SRL
// ============================================================================

#include "simcom_os.h"
#include "simcom_common.h"
#include "simcom_debug.h"

#include "../include/ergo_pins.h"
#include "../include/ergo_config.h"
#include "../include/ergo_led.h"
#include "../include/ergo_input.h"
#include "../include/ergo_sms.h"
#include "../include/ergo_network.h"

// ============================================================================
// FUNCTIA PRINCIPALA OpenCPU - ENTRY POINT
// ============================================================================

void sAPP_MainTask(void* pData)
{
    unsigned long acum;
    unsigned long timpUltimaScanare = 0;
    unsigned long timpUltimaLED = 0;
    unsigned long timpUltimaSMS = 0;
    unsigned long timpUltimaRetea = 0;

    sAPI_Debug("[ERGO] ==========================================");
    sAPI_Debug("[ERGO] MODUL GSM ERGO GASALERT v4.2");
    sAPI_Debug("[ERGO] Placa: HXY-A7670E-V1.3");
    sAPI_Debug("[ERGO] 2 LED-uri: VERDE + GALBEN");
    sAPI_Debug("[ERGO] FARA RELEU");
    sAPI_Debug("[ERGO] Retea: Orange Romania");
    sAPI_Debug("[ERGO] ==========================================");

    // ------------------------------------------
    // 1. INITIALIZARE LED-URI
    // ------------------------------------------
    initLED();

    // ------------------------------------------
    // 2. BOOT: LED verde aprins fix (soft se initializeaza)
    //          LED galben stins
    // ------------------------------------------
    ledBootStart();
    sAPI_Debug("[BOOT] LED verde APRINS FIX (initializare...)");

    // ------------------------------------------
    // 3. INITIALIZARE INTRARE
    // ------------------------------------------
    initIntrare();

    // ------------------------------------------
    // 4. INCARCARE CONFIGURATIE
    // ------------------------------------------
    incarcaConfig();

    // ------------------------------------------
    // 5. INITIALIZARE RETEA (LED galben ramane stins pana se conecteaza)
    // ------------------------------------------
    initRetea();

    // ------------------------------------------
    // 6. SFARSIT BOOT: LED verde trece la clipire
    //    LED galben: stins daca nu e retea, clipeste daca s-a conectat
    // ------------------------------------------
    ledBootEnd();
    sAPI_Debug("[BOOT] COMPLET. LED verde -> clipire.");

    if (reteaConectata)
        sAPI_Debug("[BOOT] Retea OK. LED galben -> clipire.");
    else
        sAPI_Debug("[BOOT] Fara retea. LED galben STINS.");

    sAPI_Debug("[ERGO] === SISTEM PORNIT ===");

    // ------------------------------------------
    // LOOP PRINCIPAL (infinit)
    // ------------------------------------------
    while (1)
    {
        acum = getTickMs();

        // ---- SCANARE INTRARE (10ms) ----
        if (acum - timpUltimaScanare >= TIMER_SCAN_MS)
        {
            timpUltimaScanare = acum;
            monitorizareIntrare();
            gestionareCooldown();
        }

        // ---- ACTUALIZARE LED-URI (50ms) ----
        if (acum - timpUltimaLED >= TIMER_LED_MS)
        {
            timpUltimaLED = acum;
            actualizeazaLeduri();
        }

        // ---- VERIFICARE SMS PRIMITE (1s) ----
        if (acum - timpUltimaSMS >= VERIFICARE_SMS_MS)
        {
            timpUltimaSMS = acum;
            verificaSMSPrimit();
        }

        // ---- VERIFICARE RETEA (60s) ----
        if (acum - timpUltimaRetea >= TIMER_RETEA_MS)
        {
            timpUltimaRetea = acum;
            if (!verificaConectareRetea())
            {
                sAPI_Debug("[RETEA] Pierduta! Reconectare...");
                reconectareRetea();
            }
        }

        // Yield CPU
        sAPI_TaskSleep(2);  // ~10ms
    }
}
