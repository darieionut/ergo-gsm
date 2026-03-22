// ============================================================================
// ergo_config.h - Configuratie, constante, structuri
// ERGO GASALERT v5.0 - STM32C011F4U6TR + SIMCom A7682E
// ============================================================================

#ifndef ERGO_CONFIG_H
#define ERGO_CONFIG_H

#include <string.h>
#include <ctype.h>

// ============================================================================
// CONSTANTE TIMP (milisecunde)
// ============================================================================

#define DURATA_IMPULS_MS        800     // 0.8s - impuls minim valid
#define FABRICA_COOLDOWN_S      20      // 20s  - valoare implicita din fabrica
#define MIN_COOLDOWN_S          10      // 10s  - minim acceptat
#define MAX_COOLDOWN_S          3600    // 3600s - maxim acceptat (1 ora)

// Intervale loop principal
#define TIMER_SCAN_MS           10      // Scanare intrare la 10ms
#define TIMER_LED_MS            50      // Actualizare LED-uri la 50ms
#define TIMER_RETEA_MS          60000   // Verificare retea la 60s
#define VERIFICARE_SMS_MS       1000    // Verificare SMS la 1s

// Watchdog hardware (IWDG STM32)
// LSI ~32kHz, prescaler 256 => ~125Hz, reload 3500 => ~28s timeout
#define WATCHDOG_PRESCALER      IWDG_PRESCALER_256
#define WATCHDOG_RELOAD         3500    // ~28 secunde

// Protectie anti-spam la defectare hardware/software
#define LIMITA_ALARME_BURST     20                  // max alarme inainte de blocare
#define CALM_PERIOD_MS          (2UL*3600UL*1000UL) // 2h liniste = reset automat contor

// ============================================================================
// CONSTANTE LED-URI
// ============================================================================

#define LED_VERDE_ON_MS         500     // ON  0.5s
#define LED_VERDE_OFF_MS        500     // OFF 0.5s
#define LED_GALBEN_ON_MS        500     // ON  0.5s
#define LED_GALBEN_OFF_MS       500     // OFF 0.5s
#define LED_IMPULS_DURATA_MS    3000    // 3 secunde aprinse fix la impuls valid

// ============================================================================
// CONSTANTE CONFIGURARE SMS
// ============================================================================
#define MAX_NUMERE              5
#define MAX_LUNGIME_NUMAR       20
#define MAX_LUNGIME_MESAJ       160     // Redus la 160 (un SMS standard GSM)
#define MIN_LUNGIME_NUMAR_SCURT 3

// ============================================================================
// STOCARE CONFIGURATIE IN FLASH STM32C011F4
// Flash: 16KB = 8 pagini x 2048 bytes; pagina 7 rezervata pentru config
// Adresa pagina 7: 0x08000000 + 7 * 0x800 = 0x08003800
// ============================================================================
#define CONFIG_FLASH_PAGE       7
#define CONFIG_FLASH_ADDR       0x08003800UL

// ============================================================================
// NUMERE PRESETATE DIN FABRICA
// ============================================================================
#define FABRICA_NUMAR_01        "0762862213"
#define FABRICA_NUMAR_05        "1745"
#define FABRICA_MESAJ_ALERTA    "ALARMA GAZ OPRIT"

// Retea Orange Romania
#define ORANGE_SMSC             "+40744000060"

// ============================================================================
// STRUCTURA CONFIGURATIE (salvata in Flash STM32)
// __attribute__((packed)) = fara padding -> dimensiune determinista
// ============================================================================
typedef struct __attribute__((packed)) {
    char mesajAlerta[MAX_LUNGIME_MESAJ + 1];        // 161 bytes
    char numere[MAX_NUMERE][MAX_LUNGIME_NUMAR + 1]; // 5 * 21 = 105 bytes
    unsigned int cooldownSecunde;                   // 4 bytes
    unsigned int alarmeAziCount;                    // 4 bytes
    unsigned long ultimaAlarmaMs;                   // 4 bytes (uint32_t pe Cortex-M0+)
    unsigned char flagValid;                        // 1 byte  (0xA5 = valid)
    unsigned char _pad[3];                          // 3 bytes padding -> total: 282 bytes (mult de 2)
} ConfigData;

// Verificare la compilare: config trebuie sa incapa in pagina flash de 2KB
_Static_assert(sizeof(ConfigData) <= 2048, "ConfigData prea mare pentru pagina flash!");

// Dimensiune rotunjita la multiplu de 8 pentru programarea DOUBLEWORD
#define CONFIG_FLASH_PADDED_SIZE  ((sizeof(ConfigData) + 7u) & ~7u)

// ============================================================================
// VARIABILA GLOBALA CONFIGURATIE (definita in config.c)
// ============================================================================
extern ConfigData config;

// ============================================================================
// PROTOTIPURI - config.c
// ============================================================================
void initConfigFabrica(void);
void incarcaConfig(void);
void salveazaConfig(void);

// Utilitare
void curataSir(char *sir);
int  esteNumarValid(const char *numar);
int  esteNumarScurt(const char *numar);
unsigned long getTickMs(void);
void delayMs(unsigned long ms);

// Inlocuitori POSIX (strcasecmp nu exista in toolchain-ul default STM32)
int ergo_strcasecmp(const char *a, const char *b);
int ergo_strncasecmp(const char *a, const char *b, int n);

// Watchdog - definita in main.c, apelata din orice modul
void alimenteazaWDT(void);

// Debug UART - definita in main.c
void dbg(const char *msg);

#endif // ERGO_CONFIG_H
