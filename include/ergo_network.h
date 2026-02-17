// ============================================================================
// ergo_network.h - Conectare retea GSM/LTE Orange Romania
// ERGO GASALERT - Modul GSM Notificare SMS (4G)
// ============================================================================

#ifndef ERGO_NETWORK_H
#define ERGO_NETWORK_H

// Initializare retea (SMS text mode, charset, conectare Orange)
void initRetea(void);

// Verificare stare conectare (returneaza 1=conectat, 0=neconectat)
// Actualizeaza automat variabila globala reteaConectata
int verificaConectareRetea(void);

// Reconectare la retea (dupa pierdere semnal)
void reconectareRetea(void);

// Variabila globala stare retea (definita in network.c)
extern int reteaConectata;

#endif // ERGO_NETWORK_H
