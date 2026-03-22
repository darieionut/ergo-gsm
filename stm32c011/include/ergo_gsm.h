// ============================================================================
// ergo_gsm.h - Layer GSM: AT commands, retea, URC
// ERGO GASALERT v5.0 - STM32C011 + A7682E
// ============================================================================

#ifndef ERGO_GSM_H
#define ERGO_GSM_H

#include <stdint.h>

// ============================================================================
// STARE RETEA (globala, folosita de led.c si sms.c)
// ============================================================================
extern int reteaConectata;

// ============================================================================
// INITIALIZARE MODUL A7682E
// ============================================================================

// Porneste modulul A7682E (PWRKEY), asteapta boot, configureaza SMS text mode,
// charset GSM, SMSC Orange, CNMI pentru URC direct, inregistrare retea.
void initGSM(void);

// ============================================================================
// VERIFICARE / RECONECTARE RETEA
// ============================================================================

// Verifica inregistrarea LTE (AT+CEREG?). Actualizeaza reteaConectata.
// Returneaza 1=conectat, 0=neconectat.
int verificaConectareRetea(void);

// O singura tentativa de reconectare (apelata din loop la 60s)
void reconectareRetea(void);

// Intensitate semnal (AT+CSQ). Returneaza CSQ 0-31 sau -1 la eroare.
int obtiSemnalCSQ(void);

// ============================================================================
// LAYER AT COMMANDS (folosit intern si de sms.c)
// ============================================================================

// Trimite comanda AT (adauga \r automat)
void atSend(const char* cmd);

// Citeste o linie de raspuns AT (pana la '\n', fara '\r\n').
// Returneaza lungimea sau -1 la timeout.
int atReadLine(char* buf, int maxLen, uint32_t timeoutMs);

// Asteapta raspuns "OK" sau "ERROR" in timeoutMs.
// Returneaza 1=OK, 0=ERROR sau timeout.
int atWaitOK(uint32_t timeoutMs);

// Trimite comanda si asteapta OK. Returneaza 1=OK, 0=esec.
int atSendWaitOK(const char* cmd, uint32_t timeoutMs);

// Goleste buffer-ul UART (ignora date vechi)
void atFlush(uint32_t ms);

// ============================================================================
// PROCESARE URC (Unsolicited Result Code)
//
// Apelata din loop-ul principal. Verifica daca A7682E a trimis un URC
// (ex: +CMT: pentru SMS primit). Nu blocheaza (returneaza imediat daca nu e nimic).
// ============================================================================

// Returneaza 1 daca un SMS nou a fost receptionat prin URC +CMT:
// Populeaza expeditor (max 21 chars) si continut (max 512 chars)
int gsmCheckSMSReceived(char* expeditor, char* continut, int maxContinut);

#endif // ERGO_GSM_H
