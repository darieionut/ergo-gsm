// ============================================================================
// ergo_config.h - Configuratie, constante, structuri
// ERGO GASALERT - Modul GSM Notificare SMS (4G)
// ============================================================================

#ifndef ERGO_CONFIG_H
#define ERGO_CONFIG_H

// ============================================================================
// CONSTANTE TIMP (milisecunde)
// ============================================================================

// Detectare impuls
#define DURATA_IMPULS_MS        800     // 0.8s - impuls minim valid
#define COOLDOWN_MS             20000   // 20s  - blocare dupa SMS

// Retea
#define TIMEOUT_RETEA_MS        30000   // 30s  - timeout conectare

// Intervale loop principal
#define TIMER_SCAN_MS           10      // Scanare intrare la 10ms
#define TIMER_LED_MS            50      // Actualizare LED-uri la 50ms
#define TIMER_RETEA_MS          60000   // Verificare retea la 60s
#define VERIFICARE_SMS_MS       1000    // Verificare SMS la 1s

// Watchdog hardware
#define WATCHDOG_TIMEOUT_S      60      // Reset automat daca loop-ul se blocheaza > 60s

// ============================================================================
// CONSTANTE LED-URI
// ============================================================================

// LED VERDE - clipire (firmware OK, dupa initializare)
#define LED_VERDE_ON_MS         500     // ON  0.5s
#define LED_VERDE_OFF_MS        500     // OFF 0.5s

// LED GALBEN - conectat 4G (clipire)
#define LED_GALBEN_ON_MS        500     // ON  0.5s
#define LED_GALBEN_OFF_MS       500     // OFF 0.5s

// LED la impuls valid (ambele aprinse fix)
#define LED_IMPULS_DURATA_MS    3000    // 3 secunde aprinse fix

// ============================================================================
// CONSTANTE CONFIGURARE SMS
// ============================================================================
#define MAX_NUMERE              5
#define MAX_LUNGIME_NUMAR       20
#define MAX_LUNGIME_MESAJ       300
#define MIN_LUNGIME_NUMAR_SCURT 3

// Fisier configuratie in filesystem-ul intern A7670E
#define CONFIG_FILE_PATH        "/simcom/ergo_config.dat"

// ============================================================================
// NUMERE PRESETATE DIN FABRICA
// ============================================================================
#define FABRICA_NUMAR_01        "0762862213"
#define FABRICA_NUMAR_05        "1745"

// ============================================================================
// STRUCTURA CONFIGURATIE (salvata in fisier)
// ============================================================================
typedef struct {
    char mesajAlerta[MAX_LUNGIME_MESAJ + 1];
    char numere[MAX_NUMERE][MAX_LUNGIME_NUMAR + 1];
    unsigned char flagValid;    // 0xA5 = configuratie valida
} ConfigData;

// ============================================================================
// VARIABILA GLOBALA CONFIGURATIE (declarata extern, definita in config.c)
// ============================================================================
extern ConfigData config;

// ============================================================================
// PROTOTIPURI - config.c
// ============================================================================
void initConfigFabrica(void);
void incarcaConfig(void);
void salveazaConfig(void);

// ============================================================================
// UTILITARE (folosite in mai multe fisiere)
// ============================================================================
void curataSir(char* sir);
int esteNumarValid(const char* numar);
int esteNumarScurt(const char* numar);
unsigned long getTickMs(void);
void delayMs(unsigned long ms);

// Inlocuitori POSIX (strcasecmp/strncasecmp nu exista in SDK SIMCom)
int ergo_strcasecmp(const char* a, const char* b);
int ergo_strncasecmp(const char* a, const char* b, int n);

#endif // ERGO_CONFIG_H
