// ============================================================================
// ergo_sms.h - Trimitere SMS, procesare comenzi, configurare
// ERGO GASALERT - Modul GSM Notificare SMS (4G)
// ============================================================================
//
// COMENZI SMS:
//   #msm*<text>#              - Setare mesaj alerta
//   #msm*#                    - Stergere mesaj alerta
//   #01*<numar># ... #05*#    - Setare/stergere numere (max 5)
//   #config#                  - Afisare configuratie curenta
//   Comenzi multiple: separate prin virgula intr-un singur SMS
//
// FORMAT RASPUNS CONFIG:
//   01:0762862213,02:(gol),03:(gol),04:(gol),05:1745,msm:Alarma gaz oprit.
//
// ============================================================================

#ifndef ERGO_SMS_H
#define ERGO_SMS_H

// Trimitere SMS la un numar (standard sau scurt)
int trimiteSMS(const char* numar, const char* mesaj);

// Trimitere SMS alarma la toate numerele configurate
void trimiteSMSAlarma(void);

// Verificare SMS primite si procesare comenzi (apelata la fiecare 1s)
void verificaSMSPrimit(void);

// Trimitere configuratie curenta ca raspuns SMS
void trimiteConfigCurenta(const char* numar);

#endif // ERGO_SMS_H
