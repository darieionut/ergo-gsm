// ============================================================================
// ergo_config.h - Configuratie, constante, structuri
// ERGO GASALERT v5.0 - STM32C011 + A7682E
// ============================================================================

#ifndef ERGO_CONFIG_H
#define ERGO_CONFIG_H

#include <stdint.h>
#include <string.h>
#include <ctype.h>

// ============================================================================
// CONSTANTE TIMP (milisecunde)
// ============================================================================

// Detectare impuls
#define DURATA_IMPULS_MS        800UL   // 0.8s - impuls minim valid

// Cooldown configurabil prin SMS
#define FABRICA_COOLDOWN_S      20U     // 20s  - valoare implicita din fabrica
#define MIN_COOLDOWN_S          10U     // 10s  - minim acceptat
#define MAX_COOLDOWN_S          3600U   // 3600s - maxim acceptat (1 ora)

// Timeout retea la initializare
#define TIMEOUT_RETEA_MS        30000UL // 30s

// Intervale loop principal
#define TIMER_SCAN_MS           10UL    // Scanare intrare la 10ms
#define TIMER_LED_MS            50UL    // Actualizare LED-uri la 50ms
#define TIMER_RETEA_MS          60000UL // Verificare retea la 60s
#define VERIFICARE_SMS_MS       1000UL  // Verificare SMS la 1s

// Watchdog hardware (IWDG STM32)
#define WATCHDOG_TIMEOUT_MS     30000UL // 30s reset daca loop-ul se blocheaza

// Protectie anti-spam
#define LIMITA_ALARME_BURST     20U
#define CALM_PERIOD_MS          (2UL * 3600UL * 1000UL)  // 2 ore

// ============================================================================
// CONSTANTE LED-URI
// ============================================================================

#define LED_VERDE_ON_MS         500UL
#define LED_VERDE_OFF_MS        500UL
#define LED_GALBEN_ON_MS        500UL
#define LED_GALBEN_OFF_MS       500UL
#define LED_IMPULS_DURATA_MS    3000UL  // 3 secunde aprinse fix la impuls valid

// ============================================================================
// CONSTANTE CONFIGURARE SMS
// ============================================================================

#define MAX_NUMERE              5
#define MAX_LUNGIME_NUMAR       20
#define MAX_LUNGIME_MESAJ       300
#define MIN_LUNGIME_NUMAR_SCURT 3

// ============================================================================
// CONFIGURATIE FLASH STM32C011
//
// STM32C011F4: 32KB Flash (16 pagini x 2KB)
// Ultima pagina (page 15) rezervata pentru configuratie.
// Adresa: 0x08007800
// ============================================================================
#define CONFIG_FLASH_ADDR       0x08007800UL
#define CONFIG_FLASH_PAGE       15U
#define CONFIG_FLASH_PAGE_SIZE  2048U
#define CONFIG_VALID_FLAG       0xA5U

// ============================================================================
// NUMERE PRESETATE DIN FABRICA
// ============================================================================
#define FABRICA_NUMAR_01        "0762862213"
#define FABRICA_NUMAR_05        "1745"
#define FABRICA_MESAJ_ALERTA    "ALARMA GAZ OPRIT TEST"

// Retea Orange Romania
#define ORANGE_SMSC             "+40744000060"

// ============================================================================
// STRUCTURA CONFIGURATIE (salvata in Flash STM32)
//
// __attribute__((packed)) previne padding-ul compilatorului.
// Dimensiune: 419 bytes -> rotunjit la multiplu de 8 = 424 bytes
// (Flash STM32C011 necesita scrieri de 64 biti, adrese aliniate la 8 bytes)
// ============================================================================
typedef struct __attribute__((packed)) {
    char     mesajAlerta[MAX_LUNGIME_MESAJ + 1];        // 301 bytes
    char     numere[MAX_NUMERE][MAX_LUNGIME_NUMAR + 1]; // 105 bytes
    uint32_t cooldownSecunde;                           // 4 bytes
    uint32_t alarmeAziCount;                            // 4 bytes
    uint32_t ultimaAlarmaMs;                            // 4 bytes (ms de la pornire)
    uint8_t  flagValid;                                 // 1 byte (0xA5 = valid)
    uint8_t  _padding[5];                               // 5 bytes padding -> total 424 bytes
} ConfigData;

// ============================================================================
// VARIABILA GLOBALA CONFIGURATIE
// ============================================================================
extern ConfigData config;

// ============================================================================
// PROTOTIPURI - config.c
// ============================================================================
void initConfigFabrica(void);
void incarcaConfig(void);
void salveazaConfig(void);

// ============================================================================
// UTILITARE
// ============================================================================
void     curataSir(char* sir);
int      esteNumarValid(const char* numar);
int      esteNumarScurt(const char* numar);
uint32_t getTickMs(void);
void     delayMs(uint32_t ms);

// Inlocuitori POSIX (strcasecmp/strncasecmp nu sunt in libc minimal ARM)
int ergo_strcasecmp(const char* a, const char* b);
int ergo_strncasecmp(const char* a, const char* b, int n);

// Alimentare watchdog (apelata din delayMs si din loop)
void feedWatchdog(void);

#endif // ERGO_CONFIG_H
