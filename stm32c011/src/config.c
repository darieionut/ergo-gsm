// ============================================================================
// config.c - Configuratie: stocare in Flash STM32C011
// ERGO GASALERT v5.0 - STM32C011 + A7682E
//
// DIFERENTA fata de versiunea OpenCPU:
//   - OpenCPU: configuratie salvata in filesystem intern A7670E
//   - STM32C011: configuratie salvata in ultima pagina de Flash (page 15)
//                Adresa: 0x08007800, dimensiune: 2KB
//
// Procedura scriere Flash STM32C011:
//   1. Deblocare Flash (KEYR)
//   2. Stergere pagina (PER + PNB + STRT, asteapta BSY)
//   3. Programare dubla-cuvant 64 biti (PG, scrie 2x32bit, asteapta BSY)
//   4. Blocare Flash (LOCK)
//
// Configuratia se citeste direct din Flash (read-only, fara copiere RAM).
// La modificare: copiere in RAM -> modificare -> stergere pagina -> scriere.
// ============================================================================

#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "stm32c0xx.h"
#include "../include/ergo_config.h"

// ============================================================================
// VARIABILA GLOBALA CONFIGURATIE (in RAM)
// ============================================================================
ConfigData config;

// ============================================================================
// FLASH - DEBLOCARE / BLOCARE
// ============================================================================

static void flashUnlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK)
    {
        FLASH->KEYR = 0x45670123UL;
        FLASH->KEYR = 0xCDEF89ABUL;
    }
}

static void flashLock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

static void flashWaitBusy(void)
{
    while (FLASH->SR & FLASH_SR_BSY1)
        feedWatchdog();
}

// ============================================================================
// STERGERE PAGINA FLASH (page 15 = 0x08007800)
// ============================================================================

static void flashErasePage(uint32_t pageNum)
{
    flashWaitBusy();

    // Sterge flag-urile de eroare
    FLASH->SR = FLASH_SR_OPERR | FLASH_SR_WRPERR | FLASH_SR_PGAERR |
                FLASH_SR_SIZERR | FLASH_SR_PGSERR;

    // Configureaza stergere pagina
    FLASH->CR = (pageNum << FLASH_CR_PNB_Pos) | FLASH_CR_PER;
    FLASH->CR |= FLASH_CR_STRT;

    flashWaitBusy();

    // Dezactiveaza PER
    FLASH->CR &= ~(FLASH_CR_PER | FLASH_CR_PNB_Msk);
}

// ============================================================================
// SCRIERE DUBLA-CUVANT (64 biti) IN FLASH
//
// STM32C011 Flash: scriere 64-bit (2 x 32-bit consecutive).
// Adresa trebuie aliniata la 8 bytes.
// Procedura: seteaza PG, scrie word1, scrie word2, asteapta BSY, sterge PG.
// ============================================================================

static void flashWriteDword(uint32_t addr, uint32_t w1, uint32_t w2)
{
    flashWaitBusy();

    FLASH->CR |= FLASH_CR_PG;

    *(__IO uint32_t*)(addr)     = w1;
    *(__IO uint32_t*)(addr + 4) = w2;

    flashWaitBusy();

    FLASH->CR &= ~FLASH_CR_PG;
}

// ============================================================================
// SCRIERE BLOC DE DATE IN FLASH (la adresa aliniata la 8 bytes)
// ============================================================================

static void flashWriteBlock(uint32_t addr, const uint8_t* data, uint32_t len)
{
    uint32_t offset = 0;

    // Scrie cate 8 bytes (64 biti) per iteratie
    while (offset + 8 <= len)
    {
        uint32_t w1, w2;
        memcpy(&w1, data + offset,     4);
        memcpy(&w2, data + offset + 4, 4);
        flashWriteDword(addr + offset, w1, w2);
        offset += 8;
    }

    // Bytes ramasi: completeaza cu 0xFF pana la 8 bytes
    if (offset < len)
    {
        uint8_t tmp[8];
        memset(tmp, 0xFF, sizeof(tmp));
        memcpy(tmp, data + offset, len - offset);

        uint32_t w1, w2;
        memcpy(&w1, tmp, 4);
        memcpy(&w2, tmp + 4, 4);
        flashWriteDword(addr + offset, w1, w2);
    }
}

// ============================================================================
// INITIALIZARE CONFIGURATIE DIN FABRICA
// ============================================================================

void initConfigFabrica(void)
{
    memset(&config, 0, sizeof(ConfigData));

    strncpy(config.numere[0], FABRICA_NUMAR_01, MAX_LUNGIME_NUMAR);
    strncpy(config.numere[4], FABRICA_NUMAR_05, MAX_LUNGIME_NUMAR);

    strncpy(config.mesajAlerta, FABRICA_MESAJ_ALERTA, MAX_LUNGIME_MESAJ);

    config.cooldownSecunde = FABRICA_COOLDOWN_S;
    config.flagValid = CONFIG_VALID_FLAG;

    salveazaConfig();
}

// ============================================================================
// INCARCARE CONFIGURATIE DIN FLASH
// ============================================================================

void incarcaConfig(void)
{
    // Citeste direct din Flash (memorie mapata la CONFIG_FLASH_ADDR)
    const ConfigData* flashConfig = (const ConfigData*)CONFIG_FLASH_ADDR;

    memset(&config, 0, sizeof(ConfigData));

    // Verifica daca pagina e stersa (tot 0xFF = fara configuratie)
    // sau daca flag-ul de validare e incorect
    if (flashConfig->flagValid != CONFIG_VALID_FLAG)
    {
        // Prima pornire sau config corupta -> initializare din fabrica
        initConfigFabrica();
        return;
    }

    // Copiaza configuratia din Flash in RAM
    memcpy(&config, flashConfig, sizeof(ConfigData));

    // Garanteaza null-terminatoarele (protectie scriere partiala)
    config.mesajAlerta[MAX_LUNGIME_MESAJ] = '\0';
    for (int i = 0; i < MAX_NUMERE; i++)
        config.numere[i][MAX_LUNGIME_NUMAR] = '\0';

    // Sanitizare cooldown
    if (config.cooldownSecunde < MIN_COOLDOWN_S ||
        config.cooldownSecunde > MAX_COOLDOWN_S)
    {
        config.cooldownSecunde = FABRICA_COOLDOWN_S;
    }

    // Sanitizare contor alarme
    if (config.alarmeAziCount > (uint32_t)(LIMITA_ALARME_BURST * 10))
    {
        config.alarmeAziCount = 0;
        config.ultimaAlarmaMs = 0;
    }

    // Detectie reboot watchdog:
    // Daca ultimaAlarmaMs > tickCurent, tick-urile au pornit de la 0 dupa reset.
    // Pastram contorul dar resetam referinta temporala la tickCurent.
    {
        uint32_t tickCurent = getTickMs();
        if (config.ultimaAlarmaMs > tickCurent)
            config.ultimaAlarmaMs = tickCurent;
    }
}

// ============================================================================
// SALVARE CONFIGURATIE IN FLASH
//
// Procedura: deblocare -> stergere pagina 15 -> scriere bloc -> blocare
// ATENTIE: stergerea Flash dureaza ~20ms (STM32C011). Intreruperile NVIC
// raman active dar Flash nu e accesibil in timpul stergerii (prefetch blocat).
// ============================================================================

void salveazaConfig(void)
{
    config.flagValid = CONFIG_VALID_FLAG;

    // Dezactiveaza intreruperi pentru operatia critica de Flash
    __disable_irq();

    flashUnlock();
    flashErasePage(CONFIG_FLASH_PAGE);
    flashWriteBlock(CONFIG_FLASH_ADDR, (const uint8_t*)&config, sizeof(ConfigData));
    flashLock();

    __enable_irq();
}

// ============================================================================
// UTILITARE STRING
// ============================================================================

// Elimina spatii, \r, \n, \t de la inceput si sfarsit
void curataSir(char* sir)
{
    char* start = sir;
    int len;

    while (*start == ' ' || *start == '\r' || *start == '\n' || *start == '\t')
        start++;
    if (start != sir)
        memmove(sir, start, strlen(start) + 1);

    len = strlen(sir);
    while (len > 0 && (sir[len-1] == ' ' || sir[len-1] == '\r' ||
                       sir[len-1] == '\n' || sir[len-1] == '\t'))
        sir[--len] = '\0';
}

// Valideaza numar telefon (cifre + optional + la inceput)
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

// Numere scurte: sub 7 cifre, fara prefix +
int esteNumarScurt(const char* numar)
{
    int len = strlen(numar);
    if (len == 0 || numar[0] == '+' || len >= 7) return 0;
    return 1;
}

// ============================================================================
// INLOCUITORI POSIX (strcasecmp/strncasecmp)
// ============================================================================

int ergo_strcasecmp(const char* a, const char* b)
{
    while (*a && *b)
    {
        int diff = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (diff != 0) return diff;
        a++; b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

int ergo_strncasecmp(const char* a, const char* b, int n)
{
    while (n > 0 && *a && *b)
    {
        int diff = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (diff != 0) return diff;
        a++; b++; n--;
    }
    if (n == 0) return 0;
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}
