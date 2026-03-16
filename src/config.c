// ============================================================================
// config.c - Configuratie: incarcare, salvare, fabrica, utilitare
// ERGO GASALERT - Modul GSM Notificare SMS (4G)
// ============================================================================

#include "simcom_os.h"
#include "simcom_common.h"
#include "simcom_debug.h"
#include "simcom_filesystem.h"

#include "../include/ergo_config.h"

// ============================================================================
// VARIABILA GLOBALA CONFIGURATIE
// ============================================================================
ConfigData config;

// ============================================================================
// INITIALIZARE CONFIGURATIE DIN FABRICA
// ============================================================================
// Se apeleaza la prima pornire sau cand fisierul config este corupt.
// Numere presetate: Nr01 = 0762862213, Nr05 = 1745
// Mesaj: gol (se configureaza prin SMS)
// ============================================================================

void initConfigFabrica(void)
{
    sAPI_Debug("[CONFIG] Initializare FABRICA...");

    memset(&config, 0, sizeof(ConfigData));

    // Numere presetate din fabrica
    strncpy(config.numere[0], FABRICA_NUMAR_01, MAX_LUNGIME_NUMAR);  // Nr01
    strncpy(config.numere[4], FABRICA_NUMAR_05, MAX_LUNGIME_NUMAR);  // Nr05

    // Nr02, Nr03, Nr04: goale (se configureaza prin SMS)

    // Mesaj default din fabrica (se poate modifica prin SMS #msm*<text>#)
    strncpy(config.mesajAlerta, FABRICA_MESAJ_ALERTA, MAX_LUNGIME_MESAJ);

    config.cooldownSecunde = FABRICA_COOLDOWN_S;

    config.flagValid = 0xA5;

    sAPI_Debug("[CONFIG] FABRICA Nr01: %s", FABRICA_NUMAR_01);
    sAPI_Debug("[CONFIG] FABRICA Nr05: %s (scurt)", FABRICA_NUMAR_05);
    sAPI_Debug("[CONFIG] FABRICA Mesaj: %s", FABRICA_MESAJ_ALERTA);
    sAPI_Debug("[CONFIG] FABRICA Cooldown: %ds", FABRICA_COOLDOWN_S);

    salveazaConfig();
}

// ============================================================================
// INCARCARE CONFIGURATIE DIN FISIER
// ============================================================================

void incarcaConfig(void)
{
    int fd;
    int bytesRead;
    int i;

    sAPI_Debug("[CONFIG] Incarcare...");
    memset(&config, 0, sizeof(ConfigData));

    fd = sAPI_fopen(CONFIG_FILE_PATH, "rb");
    if (fd < 0)
    {
        sAPI_Debug("[CONFIG] Fisier inexistent -> fabrica.");
        initConfigFabrica();
        return;
    }

    bytesRead = sAPI_fread(fd, (unsigned char*)&config, sizeof(ConfigData));
    sAPI_fclose(fd);

    if (bytesRead != sizeof(ConfigData) || config.flagValid != 0xA5)
    {
        sAPI_Debug("[CONFIG] Corupt (citit %d/%d bytes, flag=0x%02X) -> fabrica.",
                   bytesRead, (int)sizeof(ConfigData), (unsigned char)config.flagValid);
        initConfigFabrica();
        return;
    }

    // Fix #11: garanteaza null-terminator la sfarsitul string-urilor,
    // in caz de scriere partiala sau coruptie partiala a fisierului.
    config.mesajAlerta[MAX_LUNGIME_MESAJ] = '\0';
    {
        int i;
        for (i = 0; i < MAX_NUMERE; i++)
            config.numere[i][MAX_LUNGIME_NUMAR] = '\0';
    }

    // Sanitizare cooldown (fisier vechi poate avea 0 sau valoare invalida)
    if (config.cooldownSecunde < MIN_COOLDOWN_S || config.cooldownSecunde > MAX_COOLDOWN_S)
    {
        sAPI_Debug("[CONFIG] Cooldown invalid (%u) -> reset fabrica.", config.cooldownSecunde);
        config.cooldownSecunde = FABRICA_COOLDOWN_S;
    }

    // Sanitizare contor alarme zilnice
    if (config.alarmeAziCount > (unsigned int)(LIMITA_ALARME_BURST * 10))
    {
        sAPI_Debug("[CONFIG] alarmeAziCount invalid (%u) -> reset.", config.alarmeAziCount);
        config.alarmeAziCount = 0;
        config.ultimaAlarmaMs = 0;
    }

    // Detectie reboot: daca ultimaAlarmaMs > tickCurent, tick-urile au pornit de la 0
    // dupa reset watchdog. Pastram contorul (protectia anti-spam persista) dar resetam
    // ultimaAlarmaMs la tickCurent, astfel incat perioada de liniste se masoara
    // de la momentul reboot-ului (conservativ - nu reseteaza contorul prematur).
    {
        unsigned long tickCurent = getTickMs();
        if (config.ultimaAlarmaMs > tickCurent)
        {
            sAPI_Debug("[CONFIG] Reboot detectat: ultimaAlarma resetata (count pastrat: %u/%d).",
                       config.alarmeAziCount, LIMITA_ALARME_BURST);
            config.ultimaAlarmaMs = tickCurent;
        }
    }

    // Afisare configuratie incarcata
    sAPI_Debug("[CONFIG] OK. Mesaj: %s",
              strlen(config.mesajAlerta) > 0 ? config.mesajAlerta : "(gol)");

    sAPI_Debug("[CONFIG] Cooldown: %ds", config.cooldownSecunde);

    for (i = 0; i < MAX_NUMERE; i++)
    {
        if (strlen(config.numere[i]) > 0)
            sAPI_Debug("[CONFIG] Nr%02d: %s%s", i + 1, config.numere[i],
                      esteNumarScurt(config.numere[i]) ? " (scurt)" : "");
    }
}

// ============================================================================
// SALVARE CONFIGURATIE IN FISIER
// ============================================================================

void salveazaConfig(void)
{
    int fd;
    int bytesWritten;
    config.flagValid = 0xA5;

    fd = sAPI_fopen(CONFIG_FILE_PATH, "wb");
    if (fd < 0)
    {
        sAPI_Debug("[CONFIG] EROARE deschidere fisier scriere!");
        return;
    }

    // Fix #10: verifica ca s-au scris exact toti bytes.
    // Daca sAPI_fwrite esueaza sau scrie partial, fisierul e corupt.
    bytesWritten = sAPI_fwrite(fd, (unsigned char*)&config, sizeof(ConfigData));
    sAPI_fclose(fd);

    if (bytesWritten != (int)sizeof(ConfigData))
    {
        sAPI_Debug("[CONFIG] EROARE scriere! (%d/%d bytes). Config pierduta!",
                   bytesWritten, (int)sizeof(ConfigData));
        return;
    }

    sAPI_Debug("[CONFIG] Salvat OK (%d bytes).", bytesWritten);
}

// ============================================================================
// UTILITARE
// ============================================================================

// Eliminare spatii, \r, \n, \t de la inceput si sfarsit
void curataSir(char* sir)
{
    char* start = sir;
    int len;

    while (*start == ' ' || *start == '\r' || *start == '\n' || *start == '\t')
        start++;
    if (start != sir)
        memmove(sir, start, strlen(start) + 1);

    len = strlen(sir);
    while (len > 0 && (sir[len - 1] == ' ' || sir[len - 1] == '\r' ||
           sir[len - 1] == '\n' || sir[len - 1] == '\t'))
        sir[--len] = '\0';
}

// Validare numar telefon (cifre + optional + la inceput)
int esteNumarValid(const char* numar)
{
    int len = strlen(numar);
    int i;

    if (len < MIN_LUNGIME_NUMAR_SCURT || len > MAX_LUNGIME_NUMAR)
        return 0;

    for (i = 0; i < len; i++)
    {
        if (i == 0 && numar[i] == '+') continue;
        if (numar[i] < '0' || numar[i] > '9') return 0;
    }
    return 1;
}

// Verificare numar scurt (sub 7 cifre, fara +)
int esteNumarScurt(const char* numar)
{
    int len = strlen(numar);
    if (len == 0 || numar[0] == '+' || len >= 7) return 0;
    return 1;
}

// Returneaza milisecundele de la pornire
unsigned long getTickMs(void)
{
    return sAPI_GetTicks() * 5;  // 1 tick = 5ms (verificati in SDK)
}

// Delay in milisecunde
// Fix #12: valori < 5ms (< 1 tick) se rotunjesc in sus la 1 tick (5ms)
// pentru a nu apela sAPI_TaskSleep(0) cu comportament nedefinit.
void delayMs(unsigned long ms)
{
    unsigned long ticks = ms / 5;
    if (ticks == 0 && ms > 0)
        ticks = 1;
    sAPI_TaskSleep(ticks);
}

// ============================================================================
// INLOCUITORI POSIX (strcasecmp/strncasecmp nu exista in SDK SIMCom)
// Implementare proprie folosind tolower() din <ctype.h> (C standard)
// ============================================================================

// Comparatie doua siruri ignorand majuscule/minuscule (ca strcasecmp POSIX)
int ergo_strcasecmp(const char* a, const char* b)
{
    while (*a && *b)
    {
        int diff = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (diff != 0) return diff;
        a++;
        b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

// Comparatie primele n caractere ignorand majuscule/minuscule (ca strncasecmp POSIX)
int ergo_strncasecmp(const char* a, const char* b, int n)
{
    while (n > 0 && *a && *b)
    {
        int diff = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (diff != 0) return diff;
        a++;
        b++;
        n--;
    }
    if (n == 0) return 0;
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}
