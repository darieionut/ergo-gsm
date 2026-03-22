// ============================================================================
// ergo_input.h - Monitorizare intrare 230V AC
// ERGO GASALERT v5.0 - STM32C011 + A7682E
// ============================================================================

#ifndef ERGO_INPUT_H
#define ERGO_INPUT_H

// Stare intrare in timp real (1 = tensiune prezenta, 0 = inactiv)
// Folosita de led.c pentru controlul LED-ului rosu
extern int intrareActiva;

// Initializeaza pin intrare (PA5, input cu pull-down)
void initIntrare(void);

// Monitorizeaza intrarea si detecteaza impuls valid (>= 0.8s).
// Apelata din loop la fiecare 10ms.
void monitorizareIntrare(void);

// Gestioneaza expirarea cooldown-ului.
// Apelata din loop la fiecare 10ms.
void gestionareCooldown(void);

#endif // ERGO_INPUT_H
