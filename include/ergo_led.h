// ============================================================================
// ergo_led.h - Control LED-uri
// ERGO GASALERT - Modul GSM Notificare SMS (4G)
// ============================================================================
//
// COMPORTAMENT LED-URI:
//
//  +--------------------------------+------------------+------------------+------------------+
//  | STARE                          | LED VERDE        | LED GALBEN       | LED ROSU         |
//  +--------------------------------+------------------+------------------+------------------+
//  | Boot (initializare soft)       | APRINS FIX       | STINS            | STINS            |
//  | Soft OK, cauta retea           | Clipeste 0.5s    | STINS            | STINS            |
//  | Soft OK, conectat 4G           | Clipeste 0.5s    | Clipeste 0.5s    | STINS            |
//  | Tensiune pe intrare (< 0.8s)   | Clipeste 0.5s    | Clipeste/Stins   | APRINS FIX       |
//  | Impuls detectat (3 secunde)    | APRINS FIX       | APRINS FIX       | APRINS FIX       |
//  | Dupa 3s, intrare inactiva      | Revine clipire   | Revine clipire   | STINS            |
//  +--------------------------------+------------------+------------------+------------------+
//
// ============================================================================

#ifndef ERGO_LED_H
#define ERGO_LED_H

// Initializare LED-uri (pini ca iesire, stinse)
void initLED(void);

// LED verde aprins fix in timpul boot-ului
void ledBootStart(void);

// LED verde trece la clipire (sfarsit boot)
void ledBootEnd(void);

// Actualizare LED-uri (apelata din loop la fiecare 50ms)
void actualizeazaLeduri(void);

// Activeaza modul impuls: ambele aprinse fix 3 secunde
void activeazaModImpulsLED(void);

// Variabila: retea conectata (folosita de LED galben)
// Setata din network.c, citita din led.c
extern int reteaConectata;

// Variabila: tensiune prezenta pe intrare (folosita de LED rosu)
// Setata din input.c, citita din led.c
extern int intrareActiva;

#endif // ERGO_LED_H
