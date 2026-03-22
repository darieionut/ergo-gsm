// ============================================================================
// gsm.c - Layer GSM: AT commands, initializare A7682E, retea, URC
// ERGO GASALERT v5.0 - STM32C011 + A7682E
//
// Arhitectura comunicatie:
//   STM32C011 (master) <--USART1--> A7682E (slave AT)
//
// Initializare A7682E:
//   1. Apasa PWRKEY >1.5s (PA6 HIGH -> tranzistor -> PWRKEY LOW activ)
//   2. Asteapta boot modul (~5s)
//   3. Trimite AT pana primim OK (poll 500ms, max 15 incercari)
//   4. Configureaza SMS text mode, SMSC, CNMI
//   5. Verifica inregistrare retea LTE
//
// SMS primit: A7682E trimite URC +CMT: direct (fara stocare in SIM)
//   +CMT: "+40712345678","","26/03/22,12:00:00+08"\r\n
//   Continut mesaj\r\n
// ============================================================================

#include <string.h>
#include <stdio.h>

#include "../include/ergo_pins.h"
#include "../include/ergo_config.h"
#include "../include/ergo_uart.h"
#include "../include/ergo_gsm.h"

// ============================================================================
// VARIABILA GLOBALA
// ============================================================================
int reteaConectata = 0;

// ============================================================================
// BUFFER PENTRU URC SMS (+CMT:)
// A7682E trimite:
//   \r\n+CMT: "+40XXXXXXXXX","","data"\r\n
//   Continut\r\n
// Detectam +CMT: si stocam in aceste buffere pana la citire din sms.c
// ============================================================================
static char urcSMSSender[MAX_LUNGIME_NUMAR + 2] = {0};
static char urcSMSContent[512] = {0};
static volatile int urcSMSPending = 0;   // 1 = SMS nou disponibil

// ============================================================================
// AT COMMANDS - LAYER DE BAZA
// ============================================================================

void atSend(const char* cmd)
{
    uartSendString(cmd);
    uartSendChar('\r');
}

int atReadLine(char* buf, int maxLen, uint32_t timeoutMs)
{
    return uartReadLine(buf, maxLen, timeoutMs);
}

// Asteapta "OK" sau "ERROR" in buffer UART.
// Sare peste liniile goale si echo.
// Returneaza 1=OK primit, 0=ERROR sau timeout.
int atWaitOK(uint32_t timeoutMs)
{
    char line[128];
    uint32_t start = getTickMs();

    while ((getTickMs() - start) < timeoutMs)
    {
        int len = uartReadLine(line, sizeof(line), 200);

        if (len < 0)
            continue;   // timeout pe linie, incearca din nou in fereastra globala

        if (len == 0)
            continue;   // linie goala, skip

        if (strncmp(line, "OK", 2) == 0)
            return 1;

        if (strncmp(line, "ERROR", 5) == 0)
            return 0;

        if (strncmp(line, "+CME ERROR", 10) == 0)
            return 0;

        if (strncmp(line, "+CMS ERROR", 10) == 0)
            return 0;

        // URC primit in timp ce asteptam raspuns - proceseaza-l
        if (strncmp(line, "+CMT:", 5) == 0)
        {
            // Extrage expeditorul din linia +CMT: "+40XXXXXXXXX","","..."
            // Format: +CMT: "numar","","timestamp"
            char* p = strchr(line, '"');
            if (p)
            {
                p++;
                char* q = strchr(p, '"');
                if (q)
                {
                    int senderLen = (int)(q - p);
                    if (senderLen > MAX_LUNGIME_NUMAR)
                        senderLen = MAX_LUNGIME_NUMAR;
                    strncpy(urcSMSSender, p, senderLen);
                    urcSMSSender[senderLen] = '\0';
                }
            }
            // Citeste linia urmatoare = continutul SMS-ului
            char content[512];
            int clen = uartReadLine(content, sizeof(content), 2000);
            if (clen >= 0)
            {
                strncpy(urcSMSContent, content, sizeof(urcSMSContent) - 1);
                urcSMSContent[sizeof(urcSMSContent) - 1] = '\0';
                urcSMSPending = 1;
            }
            // Nu returnam eroare, continuam sa asteptam OK pentru comanda curenta
        }
    }

    return 0;   // Timeout
}

int atSendWaitOK(const char* cmd, uint32_t timeoutMs)
{
    atSend(cmd);
    return atWaitOK(timeoutMs);
}

void atFlush(uint32_t ms)
{
    uint32_t start = getTickMs();
    char c;
    while ((getTickMs() - start) < ms)
        uartGetChar(&c);   // Arunca tot ce vine
    uartFlushRx();
}

// ============================================================================
// PROCESARE URC (apelata din loop-ul principal)
//
// Verifica daca exista date UART disponibile care ar putea fi URC-uri.
// Gestioneaza URC +CMT: pentru SMS primit.
// ============================================================================

// Variabile pentru parsarea URC in curs
static char urcLineBuf[256];
static int  urcLinePos = 0;

// gsmCheckSMSReceived: returneaza 1 daca un SMS a fost primit prin URC,
// copiaza expeditor si continut, sterge flag-ul intern.
int gsmCheckSMSReceived(char* expeditor, char* continut, int maxContinut)
{
    // Procesam datele UART disponibile (non-blocant)
    char c;
    while (uartGetChar(&c))
    {
        if (c == '\n')
        {
            // Linie completa
            urcLineBuf[urcLinePos] = '\0';
            urcLinePos = 0;

            // Elimina '\r' de la sfarsit daca exista
            int len = strlen(urcLineBuf);
            while (len > 0 && (urcLineBuf[len-1] == '\r' || urcLineBuf[len-1] == ' '))
                urcLineBuf[--len] = '\0';

            if (len == 0)
                continue;

            // Detecteaza +CMT: pentru SMS direct
            if (strncmp(urcLineBuf, "+CMT:", 5) == 0)
            {
                // Extrage expeditorul
                char* p = strchr(urcLineBuf, '"');
                if (p)
                {
                    p++;
                    char* q = strchr(p, '"');
                    if (q)
                    {
                        int slen = (int)(q - p);
                        if (slen > MAX_LUNGIME_NUMAR) slen = MAX_LUNGIME_NUMAR;
                        strncpy(urcSMSSender, p, slen);
                        urcSMSSender[slen] = '\0';
                    }
                }

                // Citeste urmatoarea linie = continutul SMS-ului
                // uartReadLine blocheaza max 2s - acceptabil pentru URC
                int clen = uartReadLine(urcSMSContent, sizeof(urcSMSContent), 2000);
                if (clen >= 0)
                    urcSMSPending = 1;
            }
        }
        else if (c != '\r')
        {
            if (urcLinePos < (int)(sizeof(urcLineBuf) - 1))
                urcLineBuf[urcLinePos++] = c;
        }
    }

    // Returneaza SMS-ul pending daca exista
    if (urcSMSPending)
    {
        strncpy(expeditor, urcSMSSender, MAX_LUNGIME_NUMAR);
        expeditor[MAX_LUNGIME_NUMAR] = '\0';

        strncpy(continut, urcSMSContent, maxContinut - 1);
        continut[maxContinut - 1] = '\0';

        urcSMSPending = 0;
        return 1;
    }

    return 0;
}

// ============================================================================
// PORNIRE MODUL A7682E
//
// Secventa PWRKEY pentru A7682E:
// - PWRKEY activ LOW
// - PA6 HIGH = tranzistor ON = PWRKEY LOW = apasare buton
// - Mentinut 1.5 secunde, apoi eliberat
// - Dupa eliberare, modulul porneste in ~5 secunde
// ============================================================================

static void gsmPowerOn(void)
{
    // Elibereaza PWRKEY initial (asigura stare OFF/gol)
    GPIO_CLR(PIN_GSM_PWRKEY_PORT, PIN_GSM_PWRKEY_BIT);
    delayMs(500);

    // Apasa PWRKEY 1.5 secunde
    GPIO_SET(PIN_GSM_PWRKEY_PORT, PIN_GSM_PWRKEY_BIT);
    delayMs(1500);
    GPIO_CLR(PIN_GSM_PWRKEY_PORT, PIN_GSM_PWRKEY_BIT);

    // Asteapta boot modul (~5 secunde)
    delayMs(5000);
}

// Verifica daca modulul raspunde la AT (poll pana la timeout)
static int gsmWaitAlive(uint32_t timeoutMs)
{
    uint32_t start = getTickMs();
    char line[64];

    while ((getTickMs() - start) < timeoutMs)
    {
        feedWatchdog();
        uartFlushRx();
        atSend("AT");

        // Asteapta OK in 1 secunda
        uint32_t t0 = getTickMs();
        while ((getTickMs() - t0) < 1000)
        {
            int len = uartReadLine(line, sizeof(line), 200);
            if (len > 0 && strncmp(line, "OK", 2) == 0)
                return 1;
        }
    }
    return 0;
}

// ============================================================================
// INITIALIZARE GSM
// ============================================================================

void initGSM(void)
{
    // Configureaza pin PWRKEY ca iesire
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
    GPIOA->MODER &= ~GPIO_MODER_MODE6_Msk;
    GPIOA->MODER |=  (0x01U << GPIO_MODER_MODE6_Pos);  // Output
    GPIO_CLR(PIN_GSM_PWRKEY_PORT, PIN_GSM_PWRKEY_BIT);

    // Initializeaza UART
    initUART(115200);

    // Porneste modulul A7682E
    gsmPowerOn();

    // Asteapta pana modulul raspunde (max 30 secunde)
    if (!gsmWaitAlive(30000))
    {
        // Timeout: a doua incercare pornire (poate era deja pornit)
        gsmPowerOn();
        gsmWaitAlive(30000);
    }

    feedWatchdog();

    // Dezactiveaza echo
    atSendWaitOK("ATE0", 2000);

    // SMS text mode
    atSendWaitOK("AT+CMGF=1", 2000);

    // Charset GSM (fara diacritice)
    atSendWaitOK("AT+CSCS=\"GSM\"", 2000);

    // Setare centru SMS Orange Romania (SMSC)
    atSendWaitOK("AT+CSCA=\"" ORANGE_SMSC "\"", 2000);

    // URC direct: SMS primit livrat direct pe serial ca +CMT:
    // AT+CNMI=<mode>,<mt>,<bm>,<ds>,<bfr>
    // mode=2: buffer URC si livreaza; mt=2: SMS livrat direct
    atSendWaitOK("AT+CNMI=2,2,0,0,0", 2000);

    // Sterge orice SMS ramas in stocare (slot-urile 1-20)
    atSendWaitOK("AT+CMGD=1,4", 5000);

    feedWatchdog();

    // Verifica inregistrare retea (max 30s)
    uint32_t start = getTickMs();
    while ((getTickMs() - start) < TIMEOUT_RETEA_MS)
    {
        feedWatchdog();
        if (verificaConectareRetea())
            return;
        delayMs(2000);
    }
    // Timeout - fara retea; loop-ul principal va reincerca la 60s
}

// ============================================================================
// VERIFICARE RETEA LTE (AT+CEREG?)
// ============================================================================

int verificaConectareRetea(void)
{
    char line[64];
    int eraConectat = reteaConectata;
    int n = 0, stat = 0;

    atSend("AT+CEREG?");

    uint32_t start = getTickMs();
    while ((getTickMs() - start) < 3000)
    {
        int len = uartReadLine(line, sizeof(line), 500);
        if (len <= 0) continue;

        if (strncmp(line, "+CEREG:", 7) == 0)
        {
            // Format: +CEREG: <n>,<stat> sau +CEREG: <stat>
            if (sscanf(line + 7, " %d,%d", &n, &stat) != 2)
                sscanf(line + 7, " %d", &stat);
        }
        if (strncmp(line, "OK", 2) == 0)
            break;
    }

    // stat: 1 = inregistrat retea home, 5 = inregistrat roaming
    if (stat == 1 || stat == 5)
    {
        reteaConectata = 1;
        if (!eraConectat)
        {
            // Tranzitie: neconectat -> conectat -> configureaza din nou SMSC/CNMI
            atSendWaitOK("AT+CSCA=\"" ORANGE_SMSC "\"", 2000);
            atSendWaitOK("AT+CNMI=2,2,0,0,0", 2000);
        }
        return 1;
    }

    reteaConectata = 0;
    return 0;
}

// ============================================================================
// RECONECTARE (o singura tentativa, apelata din loop la 60s)
// ============================================================================

void reconectareRetea(void)
{
    feedWatchdog();
    verificaConectareRetea();
}

// ============================================================================
// INTENSITATE SEMNAL (AT+CSQ)
// ============================================================================

int obtiSemnalCSQ(void)
{
    char line[64];
    int csq = 99, ber = 0;

    atSend("AT+CSQ");

    uint32_t start = getTickMs();
    while ((getTickMs() - start) < 2000)
    {
        int len = uartReadLine(line, sizeof(line), 500);
        if (len <= 0) continue;

        if (strncmp(line, "+CSQ:", 5) == 0)
            sscanf(line + 5, " %d,%d", &csq, &ber);

        if (strncmp(line, "OK", 2) == 0)
            break;
    }

    if (csq == 99)
        return -1;   // Necunoscut

    return csq;   // 0-31
}
