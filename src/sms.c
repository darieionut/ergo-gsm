// ============================================================================
// sms.c - Trimitere SMS, procesare comenzi, configurare
// ERGO GASALERT v5.0 - STM32C011F4U6TR + SIMCom A7682E
// ============================================================================
//
// Logica identica cu v4.x.
// Diferente: sAPI_SmsSendMsg() -> gsm_trimite_sms()
//            sAPI_SmsReadMsg() -> gsm_citeste_sms()
//            sAPI_SmsDeleteMsg() -> gsm_sterge_sms()
//            sAPI_WdtFeed() -> alimenteazaWDT()
//
// ============================================================================

#include "stm32c0xx_hal.h"
#include <string.h>
#include <stdio.h>

#include "../include/ergo_config.h"
#include "../include/ergo_gsm.h"
#include "../include/ergo_sms.h"
#include "../include/ergo_network.h"

// ============================================================================
// PROTOTIPURI LOCALE
// ============================================================================

static void proceseazaComenziMultiple(const char *expeditor, const char *continut);
static void proceseazaComanda(const char *expeditor, const char *comanda);

// ============================================================================
// TRIMITERE SMS (un numar)
// ============================================================================

int trimiteSMS(const char *numar, const char *mesaj)
{
    if (!reteaConectata)
    {
        dbg("[SMS] EROARE: Retea indisponibila!");
        return 0;
    }

    if (strlen(numar) == 0 || strlen(mesaj) == 0)
    {
        dbg("[SMS] EROARE: Numar sau mesaj gol!");
        return 0;
    }

    dbg("[SMS] Trimitere SMS...");

    if (!gsm_trimite_sms(numar, mesaj))
    {
        dbg("[SMS] EROARE trimitere.");
        return 0;
    }

    dbg("[SMS] OK.");
    return 1;
}

// ============================================================================
// TRIMITERE SMS ALARMA (la toate numerele configurate)
// ============================================================================

void trimiteSMSAlarma(void)
{
    int i;
    int trimise = 0;
    int erori   = 0;
    unsigned long acum;

    if (strlen(config.mesajAlerta) == 0)
    {
        dbg("[ALARMA] Mesaj NESETAT! SMS nu se trimite.");
        return;
    }

    acum = getTickMs();

    // Dupa CALM_PERIOD_MS (2h) fara alarme -> reset contor
    if (config.ultimaAlarmaMs > 0 &&
        (acum - config.ultimaAlarmaMs) >= CALM_PERIOD_MS)
    {
        dbg("[ALARMA] 2h liniste -> reset contor alarme.");
        config.alarmeAziCount = 0;
    }

    // Verificare limita burst (protectie anti-spam la defectare)
    if (config.alarmeAziCount >= LIMITA_ALARME_BURST)
    {
        dbg("[ALARMA] LIMITA BURST ATINSA! SMS blocat.");
        return;
    }

    // Incrementeaza si salveaza INAINTE de trimitere
    // (contorul persista la reset watchdog)
    config.alarmeAziCount++;
    config.ultimaAlarmaMs = acum;
    salveazaConfig();

    dbg("[ALARMA] Trimitere SMS alarma...");

    for (i = 0; i < MAX_NUMERE; i++)
    {
        if (strlen(config.numere[i]) > 0)
        {
            alimenteazaWDT();   // WDT inainte de fiecare SMS (poate dura 10-30s)

            if (trimiteSMS(config.numere[i], config.mesajAlerta))
                trimise++;
            else
                erori++;

            delayMs(1000);      // Pauza 1s intre SMS-uri consecutive
        }
    }

    alimenteazaWDT();
    dbg("[ALARMA] Terminat.");
    (void)trimise; (void)erori;
}

// ============================================================================
// VERIFICARE SMS PRIMITE (apelata la fiecare 1s din loop)
// Itereaza sloturile 1-20 pana gaseste primul SMS disponibil.
// Proceseaza un singur SMS per apel pentru a nu bloca loop-ul.
// ============================================================================

#define SMS_MAX_SLOT    20

void verificaSMSPrimit(void)
{
    // Buffere statice pentru a evita depasirea stivei (STM32C011 = 6KB RAM)
    static char expeditor[MAX_LUNGIME_NUMAR + 1];
    static char continut[200];     // max 160 chars SMS + margine
    int slot;

    for (slot = 1; slot <= SMS_MAX_SLOT; slot++)
    {
        expeditor[0] = '\0';
        continut[0]  = '\0';

        if (!gsm_citeste_sms(slot, expeditor, continut, sizeof(continut)))
            continue;   // slot gol sau eroare, urmatorul

        // Slot ocupat: sterge inainte de procesare
        if (!gsm_sterge_sms(slot))
        {
            dbg("[SMS] EROARE stergere slot - skip.");
            continue;
        }

        if (strlen(continut) == 0)
        {
            dbg("[SMS] SMS cu continut gol, sters.");
            continue;
        }

        curataSir(expeditor);
        curataSir(continut);

        dbg("[SMS PRIMIT] Procesare comanda...");

        proceseazaComenziMultiple(expeditor, continut);

        // Procesam un singur SMS per apel
        return;
    }
}

// ============================================================================
// PROCESARE COMENZI MULTIPLE (separate prin virgula)
// ============================================================================

static void proceseazaComenziMultiple(const char *expeditor, const char *continut)
{
    static char copie[200];     // static = nu consuma stiva
    char *token;
    char *sf;
    int comenziProcesate = 0;

    strncpy(copie, continut, sizeof(copie) - 1);
    copie[sizeof(copie) - 1] = '\0';

    token = strtok(copie, ",");
    while (token != NULL)
    {
        while (*token == ' ') token++;
        sf = token + strlen(token) - 1;
        while (sf > token && *sf == ' ') { *sf = '\0'; sf--; }

        if (strlen(token) > 0)
        {
            proceseazaComanda(expeditor, token);
            comenziProcesate++;
        }

        token = strtok(NULL, ",");
    }

    if (comenziProcesate > 0)
        trimiteConfigCurenta(expeditor);
}

// ============================================================================
// PROCESARE O SINGURA COMANDA
// ============================================================================

static void proceseazaComanda(const char *expeditor, const char *comanda)
{
    int lungime = (int)strlen(comanda);
    int i;

    // #config# - afisare configuratie (raspunsul se trimite oricum dupa)
    if (ergo_strcasecmp(comanda, "#config#") == 0)
        return;

    // #rsms# - reset manual contor alarme
    if (ergo_strcasecmp(comanda, "#rsms#") == 0)
    {
        dbg("[CMD] Reset contor alarme.");
        config.alarmeAziCount = 0;
        config.ultimaAlarmaMs = 0;
        salveazaConfig();
        return;
    }

    // #msm*<text># sau #msm*# - setare/stergere mesaj alerta
    if (ergo_strncasecmp(comanda, "#msm*", 5) == 0)
    {
        if (lungime < 6 || comanda[lungime - 1] != '#')
            return;

        if (lungime == 6)
        {
            memset(config.mesajAlerta, 0, sizeof(config.mesajAlerta));
            dbg("[CMD] Mesaj STERS.");
        }
        else
        {
            int lungimeText = lungime - 6;
            if (lungimeText > MAX_LUNGIME_MESAJ)
                lungimeText = MAX_LUNGIME_MESAJ;

            memset(config.mesajAlerta, 0, sizeof(config.mesajAlerta));
            strncpy(config.mesajAlerta, comanda + 5, lungimeText);
            config.mesajAlerta[lungimeText] = '\0';
            curataSir(config.mesajAlerta);
            dbg("[CMD] Mesaj setat.");
        }

        salveazaConfig();
        return;
    }

    // #01*<numar># ... #05*<numar># - setare/stergere numere
    for (i = 1; i <= MAX_NUMERE; i++)
    {
        char prefix[6];
        snprintf(prefix, sizeof(prefix), "#%02d*", i);

        if (ergo_strncasecmp(comanda, prefix, 4) == 0)
        {
            if (lungime < 5 || comanda[lungime - 1] != '#')
                return;

            int idx = i - 1;

            if (lungime == 5)
            {
                memset(config.numere[idx], 0, sizeof(config.numere[idx]));
                dbg("[CMD] Numar sters.");
            }
            else
            {
                int lungimeNumar = lungime - 5;
                if (lungimeNumar > MAX_LUNGIME_NUMAR)
                    lungimeNumar = MAX_LUNGIME_NUMAR;

                char numarNou[MAX_LUNGIME_NUMAR + 1] = {0};
                strncpy(numarNou, comanda + 4, lungimeNumar);
                numarNou[lungimeNumar] = '\0';
                curataSir(numarNou);

                if (!esteNumarValid(numarNou))
                {
                    static char errBuf[60];
                    snprintf(errBuf, sizeof(errBuf), "EROARE: Nr%02d invalid", i);
                    dbg("[CMD] Numar invalid.");
                    trimiteSMS(expeditor, errBuf);
                    return;
                }

                strncpy(config.numere[idx], numarNou, MAX_LUNGIME_NUMAR);
                dbg("[CMD] Numar setat.");
            }

            salveazaConfig();
            return;
        }
    }

    // #cd*<secunde># sau #cd*# - setare/reset cooldown
    if (ergo_strncasecmp(comanda, "#cd*", 4) == 0)
    {
        if (lungime < 5 || comanda[lungime - 1] != '#')
            return;

        if (lungime == 5)
        {
            config.cooldownSecunde = FABRICA_COOLDOWN_S;
            dbg("[CMD] Cooldown RESET la fabrica.");
        }
        else
        {
            int lungimeVal = lungime - 5;
            int val = 0;
            int k;

            if (lungimeVal > 4) lungimeVal = 4;

            for (k = 0; k < lungimeVal; k++)
            {
                char c = comanda[4 + k];
                if (c < '0' || c > '9')
                {
                    dbg("[CMD] Cooldown invalid (non-numeric).");
                    return;
                }
                val = val * 10 + (c - '0');
            }

            if (val < MIN_COOLDOWN_S) val = MIN_COOLDOWN_S;
            if (val > MAX_COOLDOWN_S) val = MAX_COOLDOWN_S;

            config.cooldownSecunde = (unsigned int)val;
            dbg("[CMD] Cooldown setat.");
        }

        salveazaConfig();
        return;
    }

    dbg("[CMD] Comanda necunoscuta.");
}

// ============================================================================
// TRIMITERE CONFIGURATIE CURENTA (raspuns automat dupa comenzi)
// Buffer static pentru a evita depasirea stivei.
// ============================================================================

void trimiteConfigCurenta(const char *numar)
{
    static char buf[380];   // 5*22 + 162 + labels + cd + alarme + semnal
    int pos = 0;
    int i;

    for (i = 0; i < MAX_NUMERE; i++)
    {
        if (i > 0)
            pos += snprintf(buf + pos, sizeof(buf) - pos, ",");

        pos += snprintf(buf + pos, sizeof(buf) - pos, "%02d:%s",
                        i + 1,
                        strlen(config.numere[i]) > 0 ? config.numere[i] : "(gol)");
    }

    pos += snprintf(buf + pos, sizeof(buf) - pos, ",msm:%s",
                    strlen(config.mesajAlerta) > 0 ? config.mesajAlerta : "(gol)");

    pos += snprintf(buf + pos, sizeof(buf) - pos, ",cd:%us", config.cooldownSecunde);

    pos += snprintf(buf + pos, sizeof(buf) - pos, ",alarme:%u/%d",
                    config.alarmeAziCount, LIMITA_ALARME_BURST);

    {
        int csq = obtiSemnalCSQ();
        if (csq >= 0 && csq <= 31)
            pos += snprintf(buf + pos, sizeof(buf) - pos, ",semnal:%d%%", (csq * 100) / 31);
        else
            pos += snprintf(buf + pos, sizeof(buf) - pos, ",semnal:N/A");
    }

    dbg("[CONFIG] Trimitere config curenta...");
    trimiteSMS(numar, buf);
}
