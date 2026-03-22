// ============================================================================
// config.c - Configuratie: incarcare/salvare in Flash STM32, utilitare
// ERGO GASALERT v5.0 - STM32C011F4U6TR + SIMCom A7682E
// ============================================================================
//
// Configuratia se salveaza in ultima pagina de Flash a STM32C011F4 (pagina 7).
// Adresa: CONFIG_FLASH_ADDR = 0x08003800 (pagina 7, dimensiune 2KB).
// Flash STM32C011 se programeaza in double-words (64 biti = 8 bytes odata).
// Citirea flash-ului este memory-mapped (simpla dereferentiere pointer).
//
// ATENTIE: Codul aplicatiei NU trebuie sa depaseasca 0x08003800 (14KB)!
//          Verifica sectiunea .text in fisierul .map dupa compilare.
//
// ============================================================================

#include "stm32c0xx_hal.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "../include/ergo_config.h"

// ============================================================================
// VARIABILA GLOBALA CONFIGURATIE
// ============================================================================

ConfigData config;

// ============================================================================
// INITIALIZARE CONFIGURATIE DIN FABRICA
// ============================================================================

void initConfigFabrica(void)
{
    dbg("[CONFIG] Initializare FABRICA...");

    memset(&config, 0, sizeof(ConfigData));

    strncpy(config.numere[0], FABRICA_NUMAR_01, MAX_LUNGIME_NUMAR);
    strncpy(config.numere[4], FABRICA_NUMAR_05, MAX_LUNGIME_NUMAR);
    strncpy(config.mesajAlerta, FABRICA_MESAJ_ALERTA, MAX_LUNGIME_MESAJ);

    config.cooldownSecunde  = FABRICA_COOLDOWN_S;
    config.alarmeAziCount   = 0;
    config.ultimaAlarmaMs   = 0;
    config.flagValid        = 0xA5;

    dbg("[CONFIG] FABRICA: Nr01=" FABRICA_NUMAR_01
        " Nr05=" FABRICA_NUMAR_05
        " Msj=" FABRICA_MESAJ_ALERTA);

    salveazaConfig();
}

// ============================================================================
// INCARCARE CONFIGURATIE DIN FLASH
// Flash-ul STM32 este memory-mapped la adresa CONFIG_FLASH_ADDR.
// ============================================================================

void incarcaConfig(void)
{
    int i;
    unsigned long tickCurent;

    dbg("[CONFIG] Incarcare din Flash...");
    memset(&config, 0, sizeof(ConfigData));

    // Citire directa din flash (memory-mapped pe STM32)
    memcpy(&config, (const void *)CONFIG_FLASH_ADDR, sizeof(ConfigData));

    if (config.flagValid != 0xA5)
    {
        dbg("[CONFIG] Flash invalid (flag != 0xA5) -> fabrica.");
        initConfigFabrica();
        return;
    }

    // Garanteaza null-terminator (protectie coruptie partiala)
    config.mesajAlerta[MAX_LUNGIME_MESAJ] = '\0';
    for (i = 0; i < MAX_NUMERE; i++)
        config.numere[i][MAX_LUNGIME_NUMAR] = '\0';

    // Sanitizare cooldown
    if (config.cooldownSecunde < MIN_COOLDOWN_S || config.cooldownSecunde > MAX_COOLDOWN_S)
    {
        dbg("[CONFIG] Cooldown invalid -> reset fabrica.");
        config.cooldownSecunde = FABRICA_COOLDOWN_S;
    }

    // Sanitizare contor alarme
    if (config.alarmeAziCount > (unsigned int)(LIMITA_ALARME_BURST * 10))
    {
        dbg("[CONFIG] alarmeAziCount invalid -> reset.");
        config.alarmeAziCount = 0;
        config.ultimaAlarmaMs = 0;
    }

    // Detectie reboot watchdog: daca ultimaAlarmaMs > tickCurent,
    // tick-urile au pornit de la 0 (reset). Pastram contorul (protectia
    // anti-spam persista la reboot) dar resetam referinta temporala.
    tickCurent = getTickMs();
    if (config.ultimaAlarmaMs > tickCurent)
    {
        dbg("[CONFIG] Reboot detectat: ultimaAlarma resetata, contor pastrat.");
        config.ultimaAlarmaMs = tickCurent;
    }

    dbg("[CONFIG] OK.");
}

// ============================================================================
// SALVARE CONFIGURATIE IN FLASH STM32
// Procedura: Unlock -> Erase pagina 7 -> Program double-words -> Lock
// ============================================================================

void salveazaConfig(void)
{
    // Buffer aliniat la 8 bytes pentru programarea DOUBLEWORD
    uint8_t  buf[CONFIG_FLASH_PADDED_SIZE];
    uint32_t addr = CONFIG_FLASH_ADDR;
    uint32_t i;
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t pageError;

    config.flagValid = 0xA5;

    // Pregateste bufferul cu 0xFF (valoarea flash sters) + config
    memset(buf, 0xFF, sizeof(buf));
    memcpy(buf, &config, sizeof(ConfigData));

    // --- Unlock flash ---
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        dbg("[CONFIG] EROARE: Flash unlock esuat!");
        return;
    }

    // --- Erase pagina de configuratie ---
    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.Page      = CONFIG_FLASH_PAGE;
    eraseInit.NbPages   = 1;
    status = HAL_FLASHEx_Erase(&eraseInit, &pageError);
    if (status != HAL_OK)
    {
        dbg("[CONFIG] EROARE: Flash erase esuat!");
        HAL_FLASH_Lock();
        return;
    }

    // --- Program double-words (8 bytes odata) ---
    for (i = 0; i < CONFIG_FLASH_PADDED_SIZE; i += 8)
    {
        uint64_t dword;
        memcpy(&dword, buf + i, 8);
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr + i, dword);
        if (status != HAL_OK)
        {
            dbg("[CONFIG] EROARE: Flash write esuat!");
            break;
        }
        alimenteazaWDT();   // tine WDT viu pe durata scrierii
    }

    // --- Lock flash ---
    HAL_FLASH_Lock();

    // Verificare: citeste inapoi si compara cu config curent
    if (memcmp((const void *)CONFIG_FLASH_ADDR, &config, sizeof(ConfigData)) != 0)
        dbg("[CONFIG] EROARE: Verificare post-write ESUATA!");
    else
        dbg("[CONFIG] Salvat OK in Flash.");
}

// ============================================================================
// UTILITARE (identice cu v4.x - logic nemodificata)
// ============================================================================

void curataSir(char *sir)
{
    char *start = sir;
    int len;

    while (*start == ' ' || *start == '\r' || *start == '\n' || *start == '\t')
        start++;
    if (start != sir)
        memmove(sir, start, strlen(start) + 1);

    len = (int)strlen(sir);
    while (len > 0 && (sir[len - 1] == ' ' || sir[len - 1] == '\r' ||
           sir[len - 1] == '\n' || sir[len - 1] == '\t'))
        sir[--len] = '\0';
}

int esteNumarValid(const char *numar)
{
    int len = (int)strlen(numar);
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

int esteNumarScurt(const char *numar)
{
    int len = (int)strlen(numar);
    if (len == 0 || numar[0] == '+' || len >= 7) return 0;
    return 1;
}

unsigned long getTickMs(void)
{
    return HAL_GetTick();   // SysTick HAL: 1ms per tick
}

void delayMs(unsigned long ms)
{
    HAL_Delay(ms);
}

int ergo_strcasecmp(const char *a, const char *b)
{
    while (*a && *b)
    {
        int diff = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (diff != 0) return diff;
        a++; b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

int ergo_strncasecmp(const char *a, const char *b, int n)
{
    while (n > 0 && *a && *b)
    {
        int diff = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (diff != 0) return diff;
        a++; b++;
        n--;
    }
    if (n == 0) return 0;
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}
