// ============================================================================
// sms.c - Trimitere SMS, procesare comenzi, configurare
// ERGO GASALERT v5.0 - STM32C011 + A7682E
//
// DIFERENTA fata de versiunea OpenCPU:
//   - Trimitere: AT+CMGS in loc de sAPI_SmsSendMsg()
//   - Receptie: prin URC +CMT: (gsm.c) in loc de sAPI_SmsReadMsg()
//
// COMENZI SMS SUPORTATE:
//   #msm*<text>#              - Setare mesaj alerta
//   #msm*#                    - Stergere mesaj alerta
//   #01*<numar># ... #05*#    - Setare/stergere numere (max 5)
//   #cd*<secunde>#            - Setare cooldown (10-3600s)
//   #cd*#                     - Reset cooldown la fabrica (20s)
//   #config#                  - Afisare configuratie curenta
//   #rsms#                    - Reset manual contor alarme
//   Comenzi multiple: separate prin virgula intr-un singur SMS
//
// ============================================================================

#include <string.h>
#include <stdio.h>

#include "../include/ergo_config.h"
#include "../include/ergo_uart.h"
#include "../include/ergo_gsm.h"
#include "../include/ergo_sms.h"

// ============================================================================
// PROTOTIPURI LOCALE
// ============================================================================

static void proceseazaComenziMultiple(const char* expeditor, const char* continut);
static void proceseazaComanda(const char* expeditor, const char* comanda);

// ============================================================================
// TRIMITERE SMS (AT+CMGS)
//
// Procedura AT:
//   1. AT+CMGS="numar"\r
//   2. A7682E raspunde cu prompt "> "
//   3. Trimitem mesajul + Ctrl+Z (0x1A)
//   4. A7682E raspunde cu +CMGS: <ref>\r\nOK
//
// Suporta atat numere standard cat si numere scurte.
// ============================================================================

int trimiteSMS(const char* numar, const char* mesaj)
{
    char cmd[64];
    char line[128];
    uint32_t start;
    int promptPrimit = 0;

    if (!reteaConectata)
        return 0;

    if (strlen(numar) == 0 || strlen(mesaj) == 0)
        return 0;

    // Trimite comanda AT+CMGS
    snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"", numar);
    atSend(cmd);

    // Asteapta promptul "> " sau ERROR (max 5 secunde)
    start = getTickMs();
    while ((getTickMs() - start) < 5000)
    {
        int len = uartReadLine(line, sizeof(line), 500);
        if (len < 0) continue;

        // Promptul "> " poate veni ca linie separata sau ca parte a unui raspuns
        if (strstr(line, ">") != NULL)
        {
            promptPrimit = 1;
            break;
        }
        if (strncmp(line, "ERROR", 5) == 0)
            return 0;
        if (strncmp(line, "+CMS ERROR", 10) == 0)
            return 0;
    }

    if (!promptPrimit)
        return 0;

    // Trimite mesajul + Ctrl+Z
    uartSendString(mesaj);
    uartSendChar(0x1A);   // Ctrl+Z = confirmare trimitere SMS

    // Asteapta confirmarea +CMGS: si OK (max 30 secunde - retea slaba)
    feedWatchdog();
    start = getTickMs();
    while ((getTickMs() - start) < 30000)
    {
        feedWatchdog();
        int len = uartReadLine(line, sizeof(line), 1000);
        if (len < 0) continue;

        if (strncmp(line, "+CMGS:", 6) == 0)
            continue;   // Referinta SMS - ignoram valoarea

        if (strncmp(line, "OK", 2) == 0)
            return 1;   // Succes

        if (strncmp(line, "ERROR", 5) == 0)
            return 0;

        if (strncmp(line, "+CMS ERROR", 10) == 0)
            return 0;
    }

    return 0;   // Timeout
}

// ============================================================================
// TRIMITERE SMS ALARMA (la toate numerele configurate)
// Cu protectie anti-spam (LIMITA_ALARME_BURST + CALM_PERIOD_MS)
// ============================================================================

void trimiteSMSAlarma(void)
{
    int i;
    uint32_t acum;

    if (strlen(config.mesajAlerta) == 0)
        return;

    acum = getTickMs();

    // Verifica calm period: 2h fara alarme -> reset contor
    if (config.ultimaAlarmaMs > 0 &&
        (acum - config.ultimaAlarmaMs) >= CALM_PERIOD_MS)
    {
        config.alarmeAziCount = 0;
    }

    // Verifica limita burst
    if (config.alarmeAziCount >= LIMITA_ALARME_BURST)
        return;

    // Incrementeaza si salveaza INAINTE de trimitere
    // (persista in Flash chiar daca modulul se reseteaza in timpul trimiterii)
    config.alarmeAziCount++;
    config.ultimaAlarmaMs = acum;
    salveazaConfig();

    for (i = 0; i < MAX_NUMERE; i++)
    {
        if (strlen(config.numere[i]) > 0)
        {
            feedWatchdog();
            trimiteSMS(config.numere[i], config.mesajAlerta);
            delayMs(1000);   // Pauza 1s intre SMS-uri
        }
    }

    feedWatchdog();
}

// ============================================================================
// VERIFICARE SMS PRIMITE (apelata din loop la 1s)
// Verifica URC-urile procesate de gsm.c
// ============================================================================

void verificaSMSPrimit(void)
{
    char expeditor[MAX_LUNGIME_NUMAR + 2] = {0};
    char continut[512] = {0};

    // gsmCheckSMSReceived verifica daca gsm.c a primit un +CMT: URC
    if (gsmCheckSMSReceived(expeditor, continut, sizeof(continut)))
    {
        curataSir(expeditor);
        curataSir(continut);

        if (strlen(continut) > 0)
            proceseazaSMSPrimit(expeditor, continut);
    }
}

// ============================================================================
// PROCESARE SMS PRIMIT (apelata si direct din main pentru URC imediate)
// ============================================================================

void proceseazaSMSPrimit(const char* expeditor, const char* continut)
{
    proceseazaComenziMultiple(expeditor, continut);
}

// ============================================================================
// PROCESARE COMENZI MULTIPLE (separate prin virgula)
// ============================================================================

static void proceseazaComenziMultiple(const char* expeditor, const char* continut)
{
    char copie[512];
    char* token;
    char* sf;
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

static void proceseazaComanda(const char* expeditor, const char* comanda)
{
    int lungime = strlen(comanda);
    int i;

    // #config# - afisare configuratie (trimisa oricum dupa procesare)
    if (ergo_strcasecmp(comanda, "#config#") == 0)
        return;

    // #rsms# - reset manual contor alarme
    if (ergo_strcasecmp(comanda, "#rsms#") == 0)
    {
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
                    char errBuf[80];
                    snprintf(errBuf, sizeof(errBuf),
                             "EROARE: Nr%02d invalid: %s", i, numarNou);
                    trimiteSMS(expeditor, errBuf);
                    return;
                }

                strncpy(config.numere[idx], numarNou, MAX_LUNGIME_NUMAR);
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
                if (c < '0' || c > '9') return;
                val = val * 10 + (c - '0');
            }

            if (val < (int)MIN_COOLDOWN_S) val = MIN_COOLDOWN_S;
            if (val > (int)MAX_COOLDOWN_S) val = MAX_COOLDOWN_S;

            config.cooldownSecunde = (uint32_t)val;
        }

        salveazaConfig();
        return;
    }
}

// ============================================================================
// TRIMITERE CONFIGURATIE CURENTA (raspuns automat dupa comenzi)
// Format: 01:0762862213,02:(gol),03:(gol),04:(gol),05:1745,msm:text,cd:20s,
//         alarme:3/20,semnal:80%
// ============================================================================

void trimiteConfigCurenta(const char* numar)
{
    char buf[480];
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

    pos += snprintf(buf + pos, sizeof(buf) - pos, ",cd:%us",
                    (unsigned int)config.cooldownSecunde);

    pos += snprintf(buf + pos, sizeof(buf) - pos, ",alarme:%u/%u",
                    (unsigned int)config.alarmeAziCount,
                    (unsigned int)LIMITA_ALARME_BURST);

    // Semnal GSM
    {
        int csq = obtiSemnalCSQ();
        if (csq >= 0 && csq <= 31)
            pos += snprintf(buf + pos, sizeof(buf) - pos, ",semnal:%d%%",
                            (csq * 100) / 31);
        else
            pos += snprintf(buf + pos, sizeof(buf) - pos, ",semnal:N/A");
    }

    trimiteSMS(numar, buf);
}
