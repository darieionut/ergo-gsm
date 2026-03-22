// ============================================================================
// ergo_sms.h - Trimitere SMS, procesare comenzi
// ERGO GASALERT v5.0 - STM32C011 + A7682E
// ============================================================================

#ifndef ERGO_SMS_H
#define ERGO_SMS_H

// Trimite SMS la un singur numar via AT+CMGS.
// Returneaza 1=succes, 0=esec.
int trimiteSMS(const char* numar, const char* mesaj);

// Trimite SMS alarma la toate numerele configurate.
// Include protectie anti-spam (LIMITA_ALARME_BURST + CALM_PERIOD_MS).
void trimiteSMSAlarma(void);

// Proceseaza un SMS primit (apelata cu datele extrase din URC +CMT:)
// Parses comenzi: #msm*#, #01*#, #cd*#, #config#, #rsms#
void proceseazaSMSPrimit(const char* expeditor, const char* continut);

// Trimite configuratia curenta ca SMS la numar dat
void trimiteConfigCurenta(const char* numar);

// Verifica periodic SMS-urile primite (wrapper apelat din loop la 1s)
void verificaSMSPrimit(void);

#endif // ERGO_SMS_H
