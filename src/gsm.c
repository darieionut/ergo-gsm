// ============================================================================
// gsm.c - Driver AT commands pentru SIMCom A7682E
// ERGO GASALERT v5.0 - STM32C011F4U6TR + SIMCom A7682E
// ============================================================================
//
// Comunicare USART1 (PA9=TX, PA10=RX, 115200 baud).
// RX: interrupt per byte -> ring buffer de 256 bytes.
// TX: blocking (HAL_UART_Transmit).
//
// Flux pornire A7682E:
//   1. PWRKEY LOW >= 500ms -> elibereaza -> modul porneste
//   2. Asteapta "RDY" pe UART (3-8s) sau timeout 10s
//   3. Configureaza: AT+CMGF=1, AT+CSCS="GSM", AT+CSCA=SMSC
//
// ============================================================================

#include "stm32c0xx_hal.h"
#include <string.h>
#include <stdio.h>

#include "../include/ergo_pins.h"
#include "../include/ergo_config.h"
#include "../include/ergo_gsm.h"

// ============================================================================
// RING BUFFER RX
// ============================================================================

static UART_HandleTypeDef *gsm_huart = NULL;
static uint8_t  gsm_rx_byte;                      // buffer intermediar ISR
static uint8_t  gsm_rxbuf[GSM_RX_BUF_SIZE];       // ring buffer
static volatile uint16_t gsm_rxhead = 0;
static volatile uint16_t gsm_rxtail = 0;
static volatile uint16_t gsm_rx_overrun = 0;       // contor bytes pierduti (overflow)

// ============================================================================
// CALLBACK ISR (apelat din HAL_UART_RxCpltCallback in main.c)
// ============================================================================

void gsm_uart_rx_callback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != gsm_huart->Instance)
        return;

    uint16_t next = (gsm_rxhead + 1) % GSM_RX_BUF_SIZE;
    if (next != gsm_rxtail)
    {
        gsm_rxbuf[gsm_rxhead] = gsm_rx_byte;
        gsm_rxhead = next;
    }
    else
    {
        gsm_rx_overrun++;   // byte pierdut - buffer plin
    }
    // Re-armeaza receptia urmatorului byte
    HAL_UART_Receive_IT(gsm_huart, &gsm_rx_byte, 1);
}

// ============================================================================
// UTILITARE INTERNE
// ============================================================================

// Citeste un byte din ring buffer; intoarce -1 daca gol
static int gsm_getchar(void)
{
    if (gsm_rxhead == gsm_rxtail)
        return -1;
    uint8_t c = gsm_rxbuf[gsm_rxtail];
    gsm_rxtail = (gsm_rxtail + 1) % GSM_RX_BUF_SIZE;
    return (int)c;
}

// Goleste ring buffer-ul RX (inaintea unei noi comenzi AT)
void gsm_rx_clear(void)
{
    if (gsm_rx_overrun > 0)
    {
        dbg("[GSM] AVERTISMENT: RX overrun detectat (bytes pierduti)!");
        gsm_rx_overrun = 0;
    }
    gsm_rxhead = gsm_rxtail = 0;
}

// Citeste o linie (pana la '\n' sau timeout_ms).
// Trimite '\r' si '\n' final. Intoarce 1 daca linie completa, 0 daca timeout.
static int gsm_readline(char *buf, uint16_t size, uint32_t timeout_ms)
{
    uint16_t pos = 0;
    uint32_t deadline = HAL_GetTick() + timeout_ms;

    while (HAL_GetTick() < deadline)
    {
        alimenteazaWDT();   // tine WDT viu pe durata asteptarii

        int c = gsm_getchar();
        if (c < 0)
        {
            HAL_Delay(1);
            continue;
        }

        if (c == '\n')
        {
            buf[pos] = '\0';
            if (pos > 0 && buf[pos - 1] == '\r')
                buf[--pos] = '\0';
            return 1;
        }

        if (pos < (uint16_t)(size - 1))
            buf[pos++] = (char)c;
    }

    buf[pos] = '\0';
    return 0;   // timeout
}

// Trimite string via UART (TX blocking)
static void gsm_send(const char *str)
{
    HAL_UART_Transmit(gsm_huart, (uint8_t *)str, strlen(str), 2000);
}

// ============================================================================
// COMANDA AT GENERALA
// Trimite 'cmd' + "\r\n", citeste linii pana gaseste 'expect' sau "ERROR".
// Daca 'raspuns' != NULL, copiaza linia care contine 'expect'.
// Intoarce 1 = succes, 0 = eroare/timeout.
// ============================================================================

int gsm_cmd(const char *cmd, const char *expect, uint32_t timeout_ms,
            char *raspuns, uint16_t raspuns_size)
{
    char line[160];
    uint32_t deadline;

    gsm_rx_clear();
    gsm_send(cmd);
    gsm_send("\r\n");

    deadline = HAL_GetTick() + timeout_ms;
    while (HAL_GetTick() < deadline)
    {
        uint32_t now = HAL_GetTick();
        if (now >= deadline) break;
        uint32_t ramas = deadline - now;
        if (ramas > 300) ramas = 300;

        if (!gsm_readline(line, sizeof(line), ramas))
            continue;

        if (strlen(line) == 0)
            continue;

        if (strstr(line, "ERROR"))
            return 0;

        if (strstr(line, expect))
        {
            if (raspuns)
            {
                strncpy(raspuns, line, raspuns_size - 1);
                raspuns[raspuns_size - 1] = '\0';
            }
            return 1;
        }
    }

    return 0;   // timeout
}

// Asteapta caracterul 'ch' in stream (pentru promptul "> " la SMS)
int gsm_wait_char(uint8_t ch, uint32_t timeout_ms)
{
    uint32_t deadline = HAL_GetTick() + timeout_ms;
    while (HAL_GetTick() < deadline)
    {
        alimenteazaWDT();
        int c = gsm_getchar();
        if (c == (int)ch)
            return 1;
        HAL_Delay(1);
    }
    return 0;
}

// ============================================================================
// INITIALIZARE A7682E
// ============================================================================

// Pornire modul: PWRKEY LOW 600ms -> HIGH -> asteptare RDY (max 10s)
static void gsm_power_on(void)
{
    char line[80];

    dbg("[GSM] Pornire A7682E (PWRKEY)...");

    HAL_GPIO_WritePin(GSM_PWRKEY_PORT, GSM_PWRKEY_PIN, GPIO_PIN_RESET); // LOW
    HAL_Delay(600);
    HAL_GPIO_WritePin(GSM_PWRKEY_PORT, GSM_PWRKEY_PIN, GPIO_PIN_SET);   // HIGH

    // Asteapta "RDY" sau "Call Ready" pe UART (timeout 10s)
    gsm_rx_clear();
    uint32_t deadline = HAL_GetTick() + 10000;
    while (HAL_GetTick() < deadline)
    {
        alimenteazaWDT();
        if (gsm_readline(line, sizeof(line), 500))
        {
            if (strstr(line, "RDY") || strstr(line, "Call Ready"))
            {
                dbg("[GSM] A7682E pornit OK.");
                return;
            }
        }
    }

    dbg("[GSM] AVERTISMENT: timeout asteptare RDY. Continuam oricum.");
}

int gsm_init(UART_HandleTypeDef *huart)
{
    gsm_huart = huart;

    // Porneste receptia interrupt
    HAL_UART_Receive_IT(gsm_huart, &gsm_rx_byte, 1);

    // Porneste modul A7682E
    gsm_power_on();
    HAL_Delay(1000);    // pauza suplimentara dupa RDY

    // Verificare comunicare (AT echo off)
    if (!gsm_cmd("ATE0", "OK", 3000, NULL, 0))
    {
        // A doua incercare (modulul poate fi deja pornit)
        if (!gsm_cmd("ATE0", "OK", 3000, NULL, 0))
        {
            dbg("[GSM] EROARE: Nu raspunde la AT!");
            return 0;
        }
    }

    // SMS text mode (nu PDU)
    gsm_cmd("AT+CMGF=1", "OK", 3000, NULL, 0);

    // Charset GSM (fara diacritice, compatibil SMS Romania)
    gsm_cmd("AT+CSCS=\"GSM\"", "OK", 3000, NULL, 0);

    // Centru SMS Orange Romania
    gsm_cmd("AT+CSCA=\"" ORANGE_SMSC "\"", "OK", 3000, NULL, 0);

    dbg("[GSM] Init OK.");
    return 1;
}

// ============================================================================
// TRIMITERE SMS
// ============================================================================

int gsm_trimite_sms(const char *numar, const char *mesaj)
{
    char cmd[50];

    gsm_rx_clear();

    // AT+CMGS="numar"\r  (fara \n pentru a astepta promptul ">")
    snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"\r", numar);
    gsm_send(cmd);

    // Asteapta promptul "> " (A7682E trimite '>' urmat de ' ')
    if (!gsm_wait_char('>', 5000))
    {
        dbg("[GSM] Timeout prompt SMS >");
        uint8_t esc = 0x1B;     // ESC = anuleaza comanda
        HAL_UART_Transmit(gsm_huart, &esc, 1, 100);
        return 0;
    }

    // Trimite continutul SMS-ului + Ctrl-Z (0x1A) = sfarsit mesaj
    gsm_send(mesaj);
    uint8_t ctrlz = 0x1A;
    HAL_UART_Transmit(gsm_huart, &ctrlz, 1, 100);

    // Asteapta confirmare "+CMGS:" (timeout 60s - retea slaba)
    if (!gsm_cmd("", "+CMGS:", 60000, NULL, 0))
    {
        dbg("[GSM] Eroare trimitere SMS.");
        return 0;
    }

    return 1;
}

// ============================================================================
// CITIRE SMS DIN SLOT
// ============================================================================
// Format raspuns AT+CMGR=<slot>:
//   +CMGR: "REC UNREAD","+407XXXXXXXX","","26/03/22,10:00:00+08"
//   <continut mesaj>
//   (linie goala)
//   OK
// Slot gol: raspunde direct cu OK (fara linie +CMGR).

int gsm_citeste_sms(int slot, char *expeditor, char *continut, uint16_t cont_size)
{
    char cmd[20];
    char line[200];
    int got_header = 0;
    uint32_t deadline;

    expeditor[0] = '\0';
    continut[0]  = '\0';

    snprintf(cmd, sizeof(cmd), "AT+CMGR=%d", slot);
    gsm_rx_clear();
    gsm_send(cmd);
    gsm_send("\r\n");

    deadline = HAL_GetTick() + 5000;
    while (HAL_GetTick() < deadline)
    {
        alimenteazaWDT();

        uint32_t now = HAL_GetTick();
        if (now >= deadline) break;
        uint32_t ramas = deadline - now;
        if (ramas > 500) ramas = 500;

        if (!gsm_readline(line, sizeof(line), ramas))
            continue;

        if (strlen(line) == 0)
            continue;

        if (strncmp(line, "+CMGR:", 6) == 0)
        {
            // Extrage numarul expeditorului din linia header
            // Format: +CMGR: "stat","numar","alpha","timestamp"
            // Cautam al 3-lea ghilimele deschis (al 2-lea camp)
            char *p = strchr(line, '"');                 // ghilimele 1 deschis (stat)
            if (p) p = strchr(p + 1, '"');              // ghilimele 1 inchis
            if (p) p = strchr(p + 1, '"');              // ghilimele 2 deschis (numar)
            if (p)
            {
                char *sf = strchr(p + 1, '"');          // ghilimele 2 inchis
                if (sf)
                {
                    int len = (int)(sf - p) - 1;
                    if (len <= 0) len = 0;
                    if (len > MAX_LUNGIME_NUMAR) len = MAX_LUNGIME_NUMAR;
                    if (len > 0)
                        strncpy(expeditor, p + 1, len);
                    expeditor[len] = '\0';
                }
            }
            got_header = 1;
        }
        else if (got_header && continut[0] == '\0')
        {
            // Prima linie nevida dupa header = continutul SMS-ului
            strncpy(continut, line, cont_size - 1);
            continut[cont_size - 1] = '\0';
        }
        else if (strcmp(line, "OK") == 0)
        {
            return got_header ? 1 : 0;
        }
        else if (strstr(line, "ERROR"))
        {
            return 0;
        }
    }

    return 0;   // timeout
}

// ============================================================================
// STERGERE SMS DIN SLOT
// ============================================================================

int gsm_sterge_sms(int slot)
{
    char cmd[20];
    snprintf(cmd, sizeof(cmd), "AT+CMGD=%d", slot);
    return gsm_cmd(cmd, "OK", 3000, NULL, 0);
}

// ============================================================================
// STARE RETEA
// ============================================================================

// AT+CREG? -> "+CREG: 0,1" (1=home) sau "+CREG: 0,5" (5=roaming)
int gsm_get_creg(void)
{
    char raspuns[40];

    if (!gsm_cmd("AT+CREG?", "+CREG:", 3000, raspuns, sizeof(raspuns)))
        return 0;

    // Cauta ultimul caracter numeric din raspuns (status-ul)
    // "+CREG: 0,1" -> status = 1
    char *virgula = strchr(raspuns, ',');
    if (virgula && virgula[1] >= '0' && virgula[1] <= '9')
    {
        int status = (int)(virgula[1] - '0');
        return (status == 1 || status == 5) ? 1 : 0;
    }

    // Format fara virgula: "+CREG: 1"
    char *spatiu = strrchr(raspuns, ' ');
    if (spatiu && spatiu[1] >= '0' && spatiu[1] <= '9')
    {
        int status = (int)(spatiu[1] - '0');
        return (status == 1 || status == 5) ? 1 : 0;
    }

    return 0;
}

// AT+CSQ -> "+CSQ: 20,0" -> intoarce 20
int gsm_get_csq(void)
{
    char raspuns[30];

    if (!gsm_cmd("AT+CSQ", "+CSQ:", 3000, raspuns, sizeof(raspuns)))
        return -1;

    // "+CSQ: <rssi>,<ber>"
    char *p = strchr(raspuns, ' ');
    if (!p) return -1;

    int csq = 0;
    p++;    // skip spatiu
    while (*p >= '0' && *p <= '9')
        csq = csq * 10 + (*p++ - '0');

    if (csq == 99 || csq > 31)
        return -1;      // 99 = necunoscut, >31 = invalid conform GSM

    return csq;
}
