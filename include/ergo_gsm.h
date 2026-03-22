// ============================================================================
// ergo_gsm.h - Driver AT commands pentru SIMCom A7682E
// ERGO GASALERT v5.0 - STM32C011F4U6TR + SIMCom A7682E
// ============================================================================
//
// Comunicare prin USART1 (PA9=TX, PA10=RX) la 115200 baud.
// RX: interrupt-based cu ring buffer de 256 bytes.
// TX: blocking (HAL_UART_Transmit cu timeout).
//
// ============================================================================

#ifndef ERGO_GSM_H
#define ERGO_GSM_H

#include "stm32c0xx_hal.h"
#include <stdint.h>

// Dimensiune buffer RX ring
#define GSM_RX_BUF_SIZE     256

// ============================================================================
// INITIALIZARE
// ============================================================================

// Initializare driver: leaga UART handle, porneste A7682E (PWRKEY),
// configureaza SMS text mode + charset GSM + SMSC Orange.
// Apelata din main() dupa HAL init.
int  gsm_init(UART_HandleTypeDef *huart);

// ============================================================================
// INTERFATA UART (apelata din ISR - nu apela direct)
// ============================================================================

// Trebuie apelata din HAL_UART_RxCpltCallback() in main.c
void gsm_uart_rx_callback(UART_HandleTypeDef *huart);

// ============================================================================
// COMENZI AT DE BAZA
// ============================================================================

// Trimite comanda AT si asteapta linia de raspuns care contine 'expect'.
// Intoarce 1 daca gasit, 0 daca "ERROR" sau timeout.
// Daca 'raspuns' != NULL, copiaza linia cu 'expect' in buffer.
int gsm_cmd(const char *cmd, const char *expect, uint32_t timeout_ms,
            char *raspuns, uint16_t raspuns_size);

// Asteapta caracterul specific in stream (folosit pentru promptul "> " la SMS)
int gsm_wait_char(uint8_t ch, uint32_t timeout_ms);

// Goleste buffer-ul RX (inainte de o noua comanda)
void gsm_rx_clear(void);

// ============================================================================
// SMS
// ============================================================================

// Trimite SMS la 'numar' cu continut 'mesaj'.
// Intoarce 1 daca succes (+CMGS:), 0 daca eroare/timeout.
int  gsm_trimite_sms(const char *numar, const char *mesaj);

// Citeste SMS din slot 'slot' (1-20).
// Copiaza expeditorul in 'expeditor' (MAX_LUNGIME_NUMAR+1 bytes) si
// continutul in 'continut' (cont_size bytes).
// Intoarce 1 daca slot ocupat, 0 daca gol sau eroare.
int  gsm_citeste_sms(int slot, char *expeditor, char *continut, uint16_t cont_size);

// Sterge SMS din slot. Intoarce 1 daca OK, 0 daca eroare.
int  gsm_sterge_sms(int slot);

// ============================================================================
// RETEA
// ============================================================================

// Citeste starea inregistrarii in retea (AT+CREG?).
// Intoarce: 1 = inregistrat (home sau roaming), 0 = neinregistrat.
int  gsm_get_creg(void);

// Citeste intensitatea semnalului (AT+CSQ).
// Intoarce: 0-31 = CSQ, -1 = eroare sau necunoscut (99).
int  gsm_get_csq(void);

#endif // ERGO_GSM_H
