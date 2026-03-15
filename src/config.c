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
        sAPI_Debug("[CONFIG] Corupt -> fabrica.");
        initConfigFabrica();
        return;
    }

    // Sanitizare cooldown (fisier vechi poate avea 0)
    if (config.cooldownSecunde < MIN_COOLDOWN_S || config.cooldownSecunde > MAX_COOLDOWN_S)
        config.cooldownSecunde = FABRICA_COOLDOWN_S;

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
    config.flagValid = 0xA5;

    fd = sAPI_fopen(CONFIG_FILE_PATH, "wb");
    if (fd < 0)
    {
        sAPI_Debug("[CONFIG] EROARE scriere!");
        return;
    }

    sAPI_fwrite(fd, (unsigned char*)&config, sizeof(ConfigData));
    sAPI_fclose(fd);
    sAPI_Debug("[CONFIG] Salvat OK.");
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
void delayMs(unsigned long ms)
{
    sAPI_TaskSleep(ms / 5);
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
