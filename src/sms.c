// ============================================================================
// sms.c - Trimitere SMS, procesare comenzi, configurare
// ERGO GASALERT - Modul GSM Notificare SMS (4G)
// ============================================================================
//
// COMENZI SMS SUPORTATE:
//   #msm*<text>#              - Setare mesaj alerta
//   #msm*#                    - Stergere mesaj alerta
//   #01*<numar># ... #05*#    - Setare/stergere numere (max 5)
//   #cd*<secunde>#            - Setare cooldown (10-3600s)
//   #cd*#                     - Reset cooldown la 20s (fabrica)
//   #config#                  - Afisare configuratie curenta
//   Comenzi multiple: separate prin virgula intr-un singur SMS
//
// FORMAT RASPUNS CONFIG:
//   01:0762862213,02:(gol),03:(gol),04:(gol),05:1745,msm:Alarma gaz oprit.,cd:20s,semnal:80%
//
// TIPURI NUMERE:
//   - Standard Romania: 07XXXXXXXX (10 cifre)
//   - Numere scurte: ex. 1745 (3-6 cifre)
//   - Cu prefix: +407XXXXXXXX
//
// ============================================================================

#include "simcom_os.h"
#include "simcom_common.h"
#include "simcom_debug.h"
#include "simcom_sms.h"
#include "simcom_wdt.h"

#include "../include/ergo_config.h"
#include "../include/ergo_sms.h"
#include "../include/ergo_network.h"

// ============================================================================
// PROTOTIPURI LOCALE
// ============================================================================
static void proceseazaComenziMultiple(const char* expeditor, const char* continut);
static void proceseazaComanda(const char* expeditor, const char* comanda);

// ============================================================================
// TRIMITERE SMS (numere standard + numere scurte)
// ============================================================================

int trimiteSMS(const char* numar, const char* mesaj)
{
    int rezultat;

    if (!reteaConectata)
    {
        sAPI_Debug("[SMS] EROARE: Retea indisponibila!");
        return 0;
    }

    if (strlen(numar) == 0 || strlen(mesaj) == 0)
    {
        sAPI_Debug("[SMS] EROARE: Numar sau mesaj gol!");
        return 0;
    }

    sAPI_Debug("[SMS] -> %s%s: %s", numar,
              esteNumarScurt(numar) ? " (scurt)" : "", mesaj);

    // Numarul se trimite exact asa cum e salvat
    // Orange Romania accepta atat 07XX cat si numere scurte
    rezultat = sAPI_SmsSendMsg((char*)numar, (char*)mesaj, strlen(mesaj));

    if (rezultat == 0)
    {
        sAPI_Debug("[SMS] OK.");
        return 1;
    }
    else
    {
        sAPI_Debug("[SMS] EROARE (cod: %d)", rezultat);
        return 0;
    }
}

// ============================================================================
// TRIMITERE SMS ALARMA (la toate numerele configurate)
// ============================================================================

void trimiteSMSAlarma(void)
{
    int i;
    int trimise = 0;
    int erori = 0;
    unsigned long acum;
    unsigned long fereastra24hMs = 24UL * 3600UL * 1000UL;  // 86400000 ms

    if (strlen(config.mesajAlerta) == 0)
    {
        sAPI_Debug("[ALARMA] Mesaj NESETAT! SMS nu se trimite.");
        return;
    }

    acum = getTickMs();

    // Verifica daca fereastra de 24h a expirat -> reseteaza contorul
    if (acum - config.alarmeFereastraStartMs >= fereastra24hMs)
    {
        sAPI_Debug("[ALARMA] Fereastra 24h expirata -> reset contor (era %u alarme).",
                   config.alarmeAziCount);
        config.alarmeAziCount = 0;
        config.alarmeFereastraStartMs = acum;
    }

    // Verifica limita zilnica (protectie anti-spam la defectare hardware/software)
    if (config.alarmeAziCount >= LIMITA_ALARME_ZI)
    {
        sAPI_Debug("[ALARMA] LIMITA ZILNICA ATINSA (%u/%d)! SMS blocat.",
                   config.alarmeAziCount, LIMITA_ALARME_ZI);
        return;
    }

    // Incrementeaza si salveaza INAINTE de trimitere:
    // contorul persista in filesystem chiar daca modulul se reseteaza in timpul trimiterii.
    config.alarmeAziCount++;
    salveazaConfig();
    sAPI_Debug("[ALARMA] Alarma %u/%d in fereastra 24h.", config.alarmeAziCount, LIMITA_ALARME_ZI);

    for (i = 0; i < MAX_NUMERE; i++)
    {
        if (strlen(config.numere[i]) > 0)
        {
            sAPI_Debug("[ALARMA] -> Nr%02d: %s", i + 1, config.numere[i]);

            // Alimenteaza WDT inainte de fiecare trimitere (sAPI_SmsSendMsg
            // poate bloca cateva secunde pe retea slaba)
            sAPI_WdtFeed();

            if (trimiteSMS(config.numere[i], config.mesajAlerta))
                trimise++;
            else
                erori++;

            delayMs(1000);  // Pauza 1s intre SMS-uri
        }
    }

    sAPI_WdtFeed();  // Alimenteaza WDT si dupa ultimul SMS
    sAPI_Debug("[ALARMA] %d trimise, %d erori.", trimise, erori);
}

// ============================================================================
// VERIFICARE SMS PRIMITE (apelata la fiecare 1s din loop)
// Itereaza sloturile 1-20 pana gaseste primul SMS disponibil.
// Fix #4: SMS-urile cu continut gol sunt sterse (nu raman in bucla infinita).
// Fix #5: Nu se citeste exclusiv slot 1; se cauta primul slot ocupat.
// ============================================================================

#define SMS_MAX_SLOT    20

void verificaSMSPrimit(void)
{
    char expeditor[MAX_LUNGIME_NUMAR + 1];
    char continut[512];
    int rezultat;
    int slot;

    for (slot = 1; slot <= SMS_MAX_SLOT; slot++)
    {
        memset(expeditor, 0, sizeof(expeditor));
        memset(continut, 0, sizeof(continut));

        rezultat = sAPI_SmsReadMsg(slot, expeditor, continut, sizeof(continut));

        if (rezultat != 0)
            continue;  // slot gol sau eroare, trecem la urmatorul

        // Slot ocupat: sterge intotdeauna (inclusiv SMS-uri cu continut gol)
        // pentru a preveni bucla infinita de re-citire.
        if (sAPI_SmsDeleteMsg(slot) != 0)
        {
            sAPI_Debug("[SMS] EROARE stergere slot %d - skip.", slot);
            continue;
        }

        if (strlen(continut) == 0)
        {
            sAPI_Debug("[SMS] Slot %d: continut gol, sters.", slot);
            continue;
        }

        curataSir(expeditor);
        curataSir(continut);

        sAPI_Debug("[SMS PRIMIT] slot=%d %s: %s", slot, expeditor, continut);

        // Procesare comenzi
        proceseazaComenziMultiple(expeditor, continut);

        // Procesam un singur SMS per apel pentru a nu bloca loop-ul
        return;
    }
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
        // Trim spatii
        while (*token == ' ') token++;
        sf = token + strlen(token) - 1;
        while (sf > token && *sf == ' ') { *sf = '\0'; sf--; }

        if (strlen(token) > 0)
        {
            sAPI_Debug("[CMD] %s", token);
            proceseazaComanda(expeditor, token);
            comenziProcesate++;
        }

        token = strtok(NULL, ",");
    }

    // Dupa procesarea tuturor comenzilor, trimitem configuratia curenta
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

    // -----------------------------------------------------------
    // #config# - afisare configuratie
    // -----------------------------------------------------------
    if (ergo_strcasecmp(comanda, "#config#") == 0)
        return;  // config se trimite oricum dupa procesare

    // -----------------------------------------------------------
    // #rsms# - reset manual contor alarme zilnice
    // Util cand modulul a atins limita din cauza unor alarme legitime
    // si operatorul vrea sa reactiveze notificarile imediat.
    // -----------------------------------------------------------
    if (ergo_strcasecmp(comanda, "#rsms#") == 0)
    {
        sAPI_Debug("[CMD] Reset contor alarme zilnice (%u -> 0).", config.alarmeAziCount);
        config.alarmeAziCount = 0;
        config.alarmeFereastraStartMs = getTickMs();
        salveazaConfig();
        return;
    }

    // -----------------------------------------------------------
    // #msm*<text># - setare mesaj alerta
    // #msm*#       - stergere mesaj alerta
    // -----------------------------------------------------------
    if (ergo_strncasecmp(comanda, "#msm*", 5) == 0)
    {
        if (lungime < 6 || comanda[lungime - 1] != '#')
            return;

        if (lungime == 6)
        {
            // #msm*# = stergere mesaj
            memset(config.mesajAlerta, 0, sizeof(config.mesajAlerta));
            sAPI_Debug("[CMD] Mesaj STERS.");
        }
        else
        {
            // #msm*<text># = setare mesaj
            int lungimeText = lungime - 6;
            if (lungimeText > MAX_LUNGIME_MESAJ)
                lungimeText = MAX_LUNGIME_MESAJ;

            memset(config.mesajAlerta, 0, sizeof(config.mesajAlerta));
            strncpy(config.mesajAlerta, comanda + 5, lungimeText);
            config.mesajAlerta[lungimeText] = '\0';
            curataSir(config.mesajAlerta);
            sAPI_Debug("[CMD] Mesaj: %s", config.mesajAlerta);
        }

        salveazaConfig();
        return;
    }

    // -----------------------------------------------------------
    // #01*<numar># ... #05*<numar># - setare numere
    // #01*#       ... #05*#         - stergere numere
    // -----------------------------------------------------------
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
                // #0X*# = stergere numar
                memset(config.numere[idx], 0, sizeof(config.numere[idx]));
                sAPI_Debug("[CMD] Nr%02d STERS.", i);
            }
            else
            {
                // #0X*<numar># = setare numar
                int lungimeNumar = lungime - 5;
                if (lungimeNumar > MAX_LUNGIME_NUMAR)
                    lungimeNumar = MAX_LUNGIME_NUMAR;

                char numarNou[MAX_LUNGIME_NUMAR + 1] = {0};
                strncpy(numarNou, comanda + 4, lungimeNumar);
                numarNou[lungimeNumar] = '\0';
                curataSir(numarNou);

                if (!esteNumarValid(numarNou))
                {
                    // Fix #7: trimite raspuns de eroare explicit catre utilizator.
                    char errBuf[80];
                    snprintf(errBuf, sizeof(errBuf),
                             "EROARE: Nr%02d invalid: %s", i, numarNou);
                    sAPI_Debug("[CMD] %s", errBuf);
                    trimiteSMS(expeditor, errBuf);
                    return;
                }

                strncpy(config.numere[idx], numarNou, MAX_LUNGIME_NUMAR);
                sAPI_Debug("[CMD] Nr%02d: %s%s", i, config.numere[idx],
                          esteNumarScurt(config.numere[idx]) ? " (scurt)" : "");
            }

            salveazaConfig();
            return;
        }
    }

    // -----------------------------------------------------------
    // #cd*<secunde># - setare cooldown
    // #cd*#          - reset la valoarea din fabrica (20s)
    // -----------------------------------------------------------
    if (ergo_strncasecmp(comanda, "#cd*", 4) == 0)
    {
        if (lungime < 5 || comanda[lungime - 1] != '#')
            return;

        if (lungime == 5)
        {
            // #cd*# = reset la fabrica
            config.cooldownSecunde = FABRICA_COOLDOWN_S;
            sAPI_Debug("[CMD] Cooldown RESET: %ds.", config.cooldownSecunde);
        }
        else
        {
            // #cd*<secunde># = setare valoare
            int lungimeVal = lungime - 5;
            int val = 0;
            int k;

            if (lungimeVal > 4) lungimeVal = 4;  // max 4 cifre (9999)

            for (k = 0; k < lungimeVal; k++)
            {
                char c = comanda[4 + k];
                if (c < '0' || c > '9')
                {
                    sAPI_Debug("[CMD] Cooldown invalid: caractere non-numerice.");
                    return;
                }
                val = val * 10 + (c - '0');
            }

            if (val < MIN_COOLDOWN_S) val = MIN_COOLDOWN_S;
            if (val > MAX_COOLDOWN_S) val = MAX_COOLDOWN_S;

            config.cooldownSecunde = (unsigned int)val;
            sAPI_Debug("[CMD] Cooldown: %ds.", config.cooldownSecunde);
        }

        salveazaConfig();
        return;
    }

    sAPI_Debug("[CMD] Necunoscuta: %s", comanda);
}

// ============================================================================
// TRIMITERE CONFIGURATIE CURENTA (raspuns automat dupa comenzi)
// Format: 01:0762862213,02:(gol),03:(gol),04:(gol),05:1745,msm:text,cd:20s,semnal:80%
// ============================================================================

void trimiteConfigCurenta(const char* numar)
{
    char buf[480];  // 450 anterior + ~13 bytes pentru ",alarme:XX/YY"
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
                    config.alarmeAziCount, LIMITA_ALARME_ZI);

    // Intensitate semnal GSM (CSQ 0-31 convertit in procent)
    {
        int csq = obtiSemnalCSQ();
        if (csq >= 0 && csq <= 31)
        {
            int procent = (csq * 100) / 31;
            pos += snprintf(buf + pos, sizeof(buf) - pos, ",semnal:%d%%", procent);
        }
        else
            pos += snprintf(buf + pos, sizeof(buf) - pos, ",semnal:N/A");
    }

    sAPI_Debug("[CONFIG] %s", buf);
    trimiteSMS(numar, buf);
}
