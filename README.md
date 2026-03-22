# ERGO GASALERT - Modul GSM Notificare SMS (4G)

**Versiune firmware:** v5.0
**Dezvoltat de:** Plato Global SRL (Romania)
**Partener:** Navoi Concept pentru Energoinstal Premium SRL (firma autorizata ANRE pentru instalatii gaz)

## Descriere

Modul pasiv de monitorizare alimentat la 230V AC, montat in casa scarii blocului rezidential. Detecteaza prezenta tensiunii 230V AC pe o intrare (conectata in paralel cu electrovalva de gaz) si transmite notificari SMS prin retea GSM/LTE 4G catre pana la 5 destinatari configurati.

**Scenariu tipic:** Cand sistemul de detectie gaz din cladire opreste gazul prin electrovalva (electrovalva primeste 230V), modulul detecteaza acea tensiune si notifica locatarii prin SMS.

**IMPORTANT:** Modulul este PASIV - nu influenteaza in niciun fel functionarea sistemului de detectie gaz sau a electrovalvei. Este un dispozitiv auxiliar de notificare.

## Hardware

| Componenta    | Detalii                                               |
|---------------|-------------------------------------------------------|
| **MCU**       | STM32C011F4U6TR (ARM Cortex-M0+, 48MHz, 16KB Flash, 6KB RAM, UFQFPN20) |
| **Modul GSM** | SIMCom A7682E (LTE Cat-1, controlat prin comenzi AT via UART) |
| **Retea**     | Orange Romania, APN: `internet`, SMSC: `+40744000060` |
| **SIM**       | nano-SIM (pe modulul A7682E)                          |
| **Alimentare**| 230V AC prin sursa in comutatie izolata galvanic (SELV) |
| **Intrare**   | 230V AC prin optocuplor (izolat galvanic) → GPIO STM32 (PA5) |
| **LED-uri**   | 3 (verde + galben + rosu) - controlate direct de STM32 |
| **Antena**    | Externa, conector SMA (pe A7682E)                     |

### Arhitectura sistem

```
230V AC ─── Optocuplor ─── PA5 GPIO (STM32C011)
                                    │
                             STM32C011F4U6TR
                             (MCU principal, firmware)
                                    │
                          USART1 PA9/PA10 (AT commands)
                                    │
                             SIMCom A7682E
                             (modul LTE Cat-1)
                                    │
                              Retea 4G Orange
                                    │
                              SMS destinatari
```

### Pini GPIO (orientativi, de verificat pe schema finala)

| Pin | Functie |
|-----|---------|
| PA0 | LED verde |
| PA1 | LED galben |
| PA4 | LED rosu |
| PA5 | Intrare optocuplor (input pull-down) |
| PA6 | A7682E PWRKEY |
| PA9 | USART1 TX → A7682E RX (AF1) |
| PA10 | USART1 RX ← A7682E TX (AF1) |
| PA13/PA14 | SWD (SWDIO/SWDCLK) - programare/debug |
| PB6 | UART debug TX optional (AF2) |

### Conectori

| Conector | Functie |
|----------|---------|
| **CN1** (rigleta verde) | Alimentare 230V AC + intrare monitorizata |
| **J4** | SWD (SWDIO, SWDCLK, GND) + UART debug TX - programare si debug |
| Slot nano-SIM | Pe modulul A7682E |
| Conector SMA | Antena externa |

### Riglete

| Rigleta | Functie | Tensiune |
|---------|---------|----------|
| Rigleta 1 | Alimentare modul | 230V AC / 50Hz |
| Rigleta 2 | Intrare monitorizata (detectare tensiune) | 230V AC |

## Structura proiect

```
ergo-gsm/
├── Makefile
├── src/
│   ├── main.c      # main() + HAL init (GPIO, USART1, IWDG) + loop principal
│   ├── config.c    # Config in Flash STM32 (erase/write pagina 7) + utilitare
│   ├── gsm.c       # Driver AT commands A7682E (UART ring buffer, send/recv)
│   ├── sms.c       # SMS: trimitere, comenzi, configurare
│   ├── input.c     # Intrare: detectare impuls 230V + cooldown
│   ├── led.c       # LED-uri: verde + galben + rosu
│   └── network.c   # Retea: conectare/reconectare Orange Romania
├── include/
│   ├── ergo_pins.h     # Pini GPIO STM32 (port + pin HAL)
│   ├── ergo_config.h   # Constante, ConfigData, adresa Flash, prototipuri
│   ├── ergo_gsm.h      # Driver AT A7682E prototipuri
│   ├── ergo_led.h      # Prototipuri LED
│   ├── ergo_input.h    # Prototipuri intrare
│   ├── ergo_sms.h      # Prototipuri SMS
│   └── ergo_network.h  # Prototipuri retea
└── docs/
    ├── led_behavior.md
    ├── sms_commands.md
    └── wiring.md
```

## Compilare si programare

**Necesita:** STM32CubeC0 HAL (STM32Cube_FW_C0) + ARM GCC Toolchain (`arm-none-eabi-gcc`).

```sh
make
```

**Debug UART:** Adauga `-DDEBUG_UART_ENABLE` in Makefile pentru a activa output-ul de debug pe PB6 (115200 baud).

**Programare:** Via SWD cu ST-Link, conector **J4** (SWDIO, SWDCLK, GND).

**ATENTIE Flash:** Codul aplicatiei NU trebuie sa depaseasca 14KB (0x08003800). Pagina 7 (0x08003800-0x08003FFF) este rezervata pentru configuratie. Verifica fisierul `.map` dupa compilare.

## Comportament LED-uri

| Stare | LED Verde | LED Galben | LED Rosu |
|-------|-----------|------------|----------|
| Boot (initializare software) | APRINS FIX | STINS | STINS |
| Software OK, cauta retea 4G | Clipeste 0.5s/0.5s | STINS | STINS |
| Software OK, conectat la retea 4G | Clipeste 0.5s/0.5s | Clipeste 0.5s/0.5s | STINS |
| Tensiune pe intrare (< 0.8s, zgomot) | Clipeste 0.5s/0.5s | Clipeste/Stins | APRINS FIX |
| Impuls valid detectat (3 secunde) | APRINS FIX | APRINS FIX | APRINS FIX |
| Dupa 3 secunde, intrare inactiva | Revine la clipire | Revine (sau stins) | STINS |
| Nealimentat | Stins | Stins | Stins |

**Reguli:** LED verde clipeste = firmware OK. LED galben clipeste = conectat 4G. LED rosu = tensiune fizica pe intrare. La impuls valid, toate 3 aprinse fix 3 secunde.

## Logica detectare impuls

- **Durata minima impuls valid:** 0.8 secunde (800ms) continuu
- **Cooldown dupa SMS:** 20 secunde default (configurabil `#cd*<s>#`, interval 10-3600s)
- **Scanare intrare:** la 10ms

```
Intrare 230V:  ___████████___██___████████████___████████___
                   0.8s OK   <0.8  ignorat(cd)      0.8s OK
                      ↓        ↓       ↓               ↓
Actiune:         SMS TRIMIS  NIMIC   NIMIC         SMS TRIMIS
                      |←── 20s cooldown ──→|           |←── 20s...
```

## Protectie anti-spam (defectare hardware/software)

- **Limita burst:** maxim **20 alarme** consecutive inainte de blocare
- **Reset automat dupa 2h de liniste:** contorul se reseteaza automat
- **Contorul persista** in Flash la reset watchdog (nu se pierde la repornire)
- **Reset manual:** comanda SMS `#rsms#`

| Scenariu | Rezultat |
|----------|----------|
| GPIO defect (declanseaza continuu) | 20 alarme → blocat permanent |
| Alarme reale + reparatie | 20 alarme → 2h liniste → contor reset → alarma noua ✓ |
| Reset watchdog in timp ce e blocat | Contorul persista, 2h liniste necesare |
| Reset manual de operator | `#rsms#` → deblocare imediata |

## Configurare prin SMS

Comenzile se trimit prin SMS catre numarul SIM din modul. Dupa fiecare comanda, modulul raspunde cu configuratia curenta.

### Comenzi disponibile

| Comanda | Actiune |
|---------|---------|
| `#msm*<text>#` | Setare mesaj alerta (max 160 caractere, fara diacritice) |
| `#msm*#` | Stergere mesaj alerta |
| `#01*<numar>#` ... `#05*<numar>#` | Setare numere destinatari 1-5 |
| `#01*#` ... `#05*#` | Stergere numere destinatari |
| `#cd*<secunde>#` | Setare cooldown (10-3600 secunde) |
| `#cd*#` | Reset cooldown la valoarea din fabrica (20 secunde) |
| `#config#` | Afisare configuratie curenta |
| `#rsms#` | Reset manual contor alarme |

### Comenzi multiple (intr-un singur SMS)

```
#msm*Atentie gaz oprit#, #01*0712345678#, #02*0798765432#
```

### Format raspuns configuratie

```
01:0762862213,02:(gol),03:(gol),04:(gol),05:1745,msm:ALARMA GAZ OPRIT,cd:20s,alarme:3/20,semnal:80%
```

## Configuratie din fabrica

| Parametru | Valoare |
|-----------|---------|
| Nr01 | `0762862213` (presetat) |
| Nr02-04 | (gol) |
| Nr05 | `1745` (numar scurt, presetat) |
| Mesaj | `ALARMA GAZ OPRIT` (configurabil prin SMS) |
| Cooldown | `20` secunde (configurabil prin SMS `#cd*<s>#`, interval 10-3600s) |

Configuratia se salveaza in Flash intern STM32C011 la adresa `0x08003800` (pagina 7, 2KB). La prima pornire sau Flash corupt (flag != 0xA5), se reinitializeaza cu valorile din fabrica.

### Tipuri numere suportate

- Standard Romania: `07XXXXXXXX` (10 cifre)
- Cu prefix international: `+407XXXXXXXX`
- Numere scurte: 3-6 cifre (ex: `1745`)

## Intervale loop principal

| Actiune | Interval |
|---------|----------|
| Scanare intrare + WDT feed | 10ms |
| Actualizare LED-uri | 50ms |
| Verificare SMS primite | 1s |
| Verificare retea | 60s |

## Watchdog

IWDG hardware STM32 cu timeout ~28 secunde (LSI ~32kHz, prescaler 256, reload 3500). Alimentat la fiecare 10ms din loop principal si in operatiile AT blocante din gsm.c. Daca loop-ul se blocheaza > 28s (ex: blocat pe UART), modulul se reseteaza automat.

## Certificare (in curs)

Produsul este in faza prototip/pre-test. Directive UE vizate:
- RED 2014/53/EU (echipamente radio) - modulul GSM SIMCom A7682E este deja certificat
- LVD 2014/35/EU (siguranta electrica)
- EMC 2014/30/EU (compatibilitate electromagnetica)
- RoHS 2011/65/EU

Laborator de testare: ICPE-CA (Romania).

## Autori

**Plato Global SRL** - Romania
In parteneriat cu **Navoi Concept** pentru **Energoinstal Premium SRL** (firma autorizata ANRE pentru instalatii gaz)
