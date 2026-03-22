// ============================================================================
// ergo_led.h - Control LED-uri
// ERGO GASALERT v5.0 - STM32C011 + A7682E
// ============================================================================

#ifndef ERGO_LED_H
#define ERGO_LED_H

// Variabile externe necesare din alte module
extern int reteaConectata;   // definit in gsm.c
extern int intrareActiva;    // definit in input.c

// Initializare GPIO LED-uri
void initLED(void);

// Faza boot: LED verde aprins fix, galben si rosu stinse
void ledBootStart(void);

// Sfarsit boot: LED verde trece la clipire
void ledBootEnd(void);

// Activeaza mod impuls: toate 3 LED-uri aprinse fix 3 secunde
// Apelata DUPA trimiteSMSAlarma() astfel incat timer-ul porneste
// de la terminarea efectiva a trimiterii.
void activeazaModImpulsLED(void);

// Actualizeaza starea LED-urilor (apelata din loop la 50ms)
void actualizeazaLeduri(void);

#endif // ERGO_LED_H
