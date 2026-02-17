# ERGO GASALERT - Modul GSM Notificare SMS (4G)

**Versiune firmware:** v4.2
**Dezvoltat de:** Plato Global SRL (Romania)
**Partener:** Energoinstal Premium SRL (firma autorizata ANRE pentru instalatii gaz)

## Descriere

Modul pasiv de monitorizare alimentat la 230V AC, montat in casa scarii blocului rezidential. Detecteaza prezenta tensiunii 230V AC pe o intrare (conectata in paralel cu electrovalva de gaz) si transmite notificari SMS prin retea GSM/LTE 4G catre pana la 5 destinatari configurati.

**Scenariu tipic:** Cand sistemul de detectie gaz din cladire opreste gazul prin electrovalva (electrovalva primeste 230V), modulul detecteaza acea tensiune si notifica locatarii prin SMS.

**IMPORTANT:** Modulul este PASIV - nu influenteaza in niciun fel functionarea sistemului de detectie gaz sau a electrovalvei. Este un dispozitiv auxiliar de notificare.

## Hardware

| Componenta    | Detalii                                               |
|---------------|-------------------------------------------------------|
| **Placa**     | HXY-A7670E-V1.3                                       |
| **Modul**     | SIMCom A7670E cu OpenCPU integrat (Unisoc 8910DM, ARM Cortex-A5) |
| **Retea**     | Orange Romania, APN: `internet`, SMSC: `+40744000060` |
| **SIM**       | micro-SIM                                             |
| **Alimentare**| 230V AC prin sursa in comutatie izolata galvanic (SELV) |
| **Intrare**   | 230V AC prin optocuplor (izolat galvanic)             |
| **LED-uri**   | 2 (verde + galben)                                    |
| **Releu**     | Fara releu                                            |
| **Antena**    | Externa, conector SMA                                 |

### Conectori

| Conector | Functie |
|----------|---------|
| **CN1** (rigleta verde) | Alimentare 230V AC + intrare monitorizata |
| **J4** | Debug UART: TX, RX, GND - programare firmware |
| Slot micro-SIM | |
| Conector SMA | Antena externa |

### Riglete

| Rigleta | Functie | Tensiune |
|---------|---------|----------|
| Rigleta 1 | Alimentare modul | 230V AC / 50Hz |
| Rigleta 2 | Intrare monitorizata (detectare tensiune) | 230V AC |

## Structura proiect

```
ergo-gsm/
├── Makefile            # Template build ARM GCC + SIMCom OpenCPU SDK
├── src/
│   ├── main.c          # Entry point sAPP_MainTask() + loop principal
│   ├── config.c        # Configuratie: incarcare/salvare/fabrica + utilitare
│   ├── sms.c           # SMS: trimitere, comenzi, configurare
│   ├── input.c         # Intrare: detectare impuls 230V + cooldown
│   ├── led.c           # LED-uri: verde + galben
│   └── network.c       # Retea: conectare/reconectare Orange Romania
├── include/
│   ├── ergo_pins.h     # Definire pini GPIO
│   ├── ergo_config.h   # Constante, structuri, prototipuri utilitare
│   ├── ergo_led.h      # Prototipuri LED
│   ├── ergo_input.h    # Prototipuri intrare
│   ├── ergo_sms.h      # Prototipuri SMS
│   └── ergo_network.h  # Prototipuri retea
└── docs/
    ├── led_behavior.md # Comportament LED-uri (detaliat)
    ├── sms_commands.md # Comenzi SMS (detaliat)
    └── wiring.md       # Schema conectare
```

## Compilare si programare

**Compilare:** Necesita SIMCom OpenCPU SDK pentru A7670E si ARM GCC Toolchain.

```sh
make
```

**Programare:** Prin UART pe conectorul **J4** (TX, RX, GND) de pe placa HXY-A7670E-V1.3.

## Comportament LED-uri

| Stare | LED Verde | LED Galben |
|-------|-----------|------------|
| Boot (initializare software) | APRINS FIX | STINS |
| Software OK, cauta retea 4G | Clipeste 0.5s/0.5s | STINS |
| Software OK, conectat la retea 4G | Clipeste 0.5s/0.5s | Clipeste 0.5s/0.5s |
| Impuls valid detectat (3 secunde) | APRINS FIX | APRINS FIX |
| Dupa 3 secunde | Revine la clipire | Revine (sau stins daca nu e retea) |
| Nealimentat | Stins | Stins |

**Reguli:** LED verde clipeste = software ruleaza OK. LED galben clipeste = conectat 4G. La impuls valid, ambele aprinse fix 3 secunde, apoi revin la normal.

## Logica detectare impuls

- **Durata minima impuls valid:** 0.8 secunde (800ms) continuu
- **Cooldown dupa SMS:** 20 secunde (maxim 1 SMS la 20s)
- **Scanare intrare:** la 10ms

```
Intrare 230V:  ___████████___██___████████████___████████___
                   0.8s OK   <0.8  ignorat(cd)      0.8s OK
                      ↓        ↓       ↓               ↓
Actiune:         SMS TRIMIS  NIMIC   NIMIC         SMS TRIMIS
                      |←── 20s cooldown ──→|           |←── 20s...
```

## Configurare prin SMS

Comenzile se trimit prin SMS catre numarul SIM din modul. Dupa fiecare comanda, modulul raspunde cu configuratia curenta.

### Comenzi disponibile

| Comanda | Actiune |
|---------|---------|
| `#msm*<text>#` | Setare mesaj alerta (max 300 caractere, fara diacritice) |
| `#msm*#` | Stergere mesaj alerta |
| `#01*<numar>#` ... `#05*<numar>#` | Setare numere destinatari 1-5 |
| `#01*#` ... `#05*#` | Stergere numere destinatari |
| `#config#` | Afisare configuratie curenta |

### Comenzi multiple (intr-un singur SMS)

```
#msm*Atentie gaz oprit#, #01*0712345678#, #02*0798765432#
```

### Reset complet

```
#msm*#, #01*#, #02*#, #03*#, #04*#, #05*#
```

### Format raspuns configuratie

```
01:0762862213,02:(gol),03:(gol),04:(gol),05:1745,msm:Alarma gaz oprit.
```

## Configuratie din fabrica

| Parametru | Valoare |
|-----------|---------|
| Nr01 | `0762862213` (presetat) |
| Nr02 | (gol) |
| Nr03 | (gol) |
| Nr04 | (gol) |
| Nr05 | `1745` (numar scurt, presetat) |
| Mesaj | **(gol)** - TREBUIE configurat prin SMS inainte de prima utilizare |

Configuratia se salveaza in filesystem-ul intern A7670E la `/simcom/ergo_config.dat`. La prima pornire sau fisier corupt, se reinitializeaza cu valorile din fabrica.

### Tipuri numere suportate

- Standard Romania: `07XXXXXXXX` (10 cifre)
- Cu prefix international: `+407XXXXXXXX`
- Numere scurte: 3-6 cifre (ex: `1745`)

## Intervale loop principal

| Actiune | Interval |
|---------|----------|
| Scanare intrare | 10ms |
| Actualizare LED-uri | 50ms |
| Verificare SMS primite | 1s |
| Verificare retea | 60s |

## Certificare (in curs)

Produsul este in faza prototip/pre-test. Directive UE vizate:
- RED 2014/53/EU (echipamente radio)
- LVD 2014/35/EU (siguranta electrica)
- EMC 2014/30/EU (compatibilitate electromagnetica)
- RoHS 2011/65/EU
- Potential ATEX (atmosfere explozive)

Laborator de testare: ICPE-CA (Romania).

## Autori

**Plato Global SRL** - Romania
In parteneriat cu **Energoinstal Premium SRL** (firma autorizata ANRE pentru instalatii gaz)
