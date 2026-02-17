// ============================================================================
// ergo_input.h - Monitorizare intrare 230V + detectare impuls
// ERGO GASALERT - Modul GSM Notificare SMS (4G)
// ============================================================================
//
// LOGICA:
// 1. Cand apare tensiune pe intrare -> start cronometru
// 2. Daca ramane minim 0.8s continuu -> IMPULS VALID -> SMS
// 3. Daca dispare inainte de 0.8s -> zgomot, ignorat
// 4. Dupa impuls valid: cooldown 20s (ignora alte impulsuri)
//
// ============================================================================

#ifndef ERGO_INPUT_H
#define ERGO_INPUT_H

// Initializare pin intrare (GPIO input)
void initIntrare(void);

// Monitorizare intrare + detectare impuls (apelata la fiecare 10ms)
void monitorizareIntrare(void);

// Gestionare cooldown 20s (apelata la fiecare 10ms)
void gestionareCooldown(void);

#endif // ERGO_INPUT_H
