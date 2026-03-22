# ERGO GASALERT - Modul GSM Notificare SMS (4G)

**Versiune firmware:** v4.2 (OpenCPU A7670E) / v5.0 (STM32C011 + A7682E)
**Dezvoltat de:** Plato Global SRL (Romania)
**Partener:** Navoi Concept pentru Energoinstal Premium SRL (firma autorizata ANRE pentru instalatii gaz)

## Descriere

Modul pasiv de monitorizare alimentat la 230V AC, montat in casa scarii blocului rezidential. Detecteaza prezenta tensiunii 230V AC pe o intrare (conectata in paralel cu electrovalva de gaz) si transmite notificari SMS prin retea GSM/LTE 4G catre pana la 5 destinatari configurati.

**Scenariu tipic:** Cand sistemul de detectie gaz din cladire opreste gazul prin electrovalva (electrovalva primeste 230V), modulul detecteaza acea tensiune si notifica locatarii prin SMS.

**IMPORTANT:** Modulul este PASIV - nu influenteaza in niciun fel functionarea sistemului de detectie gaz sau a electrovalvei. Este un dispozitiv auxiliar de notificare.

## Versiuni hardware

### v4.2 — HXY-A7670E-V1.3 (OpenCPU)

| Componenta    | Detalii                                               |
|---------------|-------------------------------------------------------|
| **Placa**     | HXY-A7670E-V1.3                                       |
| **Modul**     | SIMCom A7670E cu OpenCPU integrat (Unisoc 8910DM, ARM Cortex-A5) |
| **Retea**     | Orange Romania, APN: `internet`, SMSC: `+40744000060` |
| **SIM**       | micro-SIM                                             |
| **Alimentare**| 230V AC prin sursa in comutatie izolata galvanic (SELV) |
| **Intrare**   | 230V AC prin optocuplor (izolat galvanic)             |
| **LED-uri**   | 3 (verde + galben + rosu)                             |
| **Antena**    | Externa, conector SMA                                 |

### v5.0 — HXY-A7682E-STM32-V1.0 (MCU extern + modul GSM)

| Componenta    | Detalii                                               |
|---------------|-------------------------------------------------------|
| **MCU**       | STM32C011F4U6TR (ARM Cortex-M0+, 32KB Flash, 6KB RAM, UFQFPN20) |
| **Modul GSM** | SIMCom A7682E (LTE Cat 1) — comunicatie prin AT commands UART |
| **Retea**     | Orange Romania, SMSC: `+40744000060`                  |
| **SIM**       | micro-SIM (in A7682E)                                 |
| **Alimentare**| 230V AC prin sursa in comutatie izolata galvanic (SELV) |
| **Intrare**   | 230V AC prin optocuplor (izolat galvanic), PA5 pull-down |
| **LED-uri**   | 3 (verde PA0 + galben PA1 + rosu PA4)                 |
| **Antena**    | Externa, conector SMA                                 |

#### Pinout STM32C011F4U6TR (v5.0)

| Pin | Functie | Directie |
|-----|---------|----------|
| PA0 | LED_VERDE | Output |
| PA1 | LED_GALBEN | Output |
| PA4 | LED_ROSU | Output |
| PA5 | Intrare optocuplor 230V | Input pull-down |
| PA6 | A7682E PWRKEY | Output |
| PA2 | USART1_TX (AF1) → A7682E RX | Output AF |
| PA3 | USART1_RX (AF1) ← A7682E TX | Input AF |
| PA13 | SWDIO | SWD debug |
| PA14 | SWCLK | SWD debug |

### Conectori (comune ambelor versiuni)

| Conector | Functie |
|----------|---------|
| **CN1** (rigleta verde) | Alimentare 230V AC + intrare monitorizata |
| **J4** | Debug UART / programare firmware |
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
├── Makefile                    # Build ARM GCC + SIMCom OpenCPU SDK (v4.2)
├── src/                        # Surse v4.2 (OpenCPU A7670E)
│   ├── main.c                  # Entry point sAPP_MainTask() + loop
│   ├── config.c                # Configuratie filesystem + utilitare
│   ├── sms.c                   # SMS: trimitere, comenzi, configurare
│   ├── input.c                 # Intrare: detectare impuls 230V + cooldown
│   ├── led.c                   # LED-uri: verde + galben + rosu
│   └── network.c               # Retea: conectare/reconectare Orange Romania
├── include/                    # Headere v4.2
│   ├── ergo_pins.h
│   ├── ergo_config.h
│   ├── ergo_led.h
│   ├── ergo_input.h
│   ├── ergo_sms.h
│   └── ergo_network.h
├── stm32c011/                  # Surse v5.0 (STM32C011 + A7682E)
│   ├── Makefile                # Build ARM GCC pentru STM32C011
│   ├── ARHITECTURA.md          # Documentatie migrare v4.2 -> v5.0
│   ├── linker/
│   │   └── STM32C011F4UX_FLASH.ld  # Linker script (30KB cod + 2KB config)
│   ├── src/
│   │   ├── main.c              # Entry point main() + SysTick + IWDG + loop
│   │   ├── config.c            # Config in Flash STM32 (page 15) + utilitare
│   │   ├── uart.c              # USART1 buffer circular + IRQ RXNE
│   │   ├── gsm.c               # AT commands + initGSM (PWRKEY) + retea + URC
│   │   ├── led.c               # LED-uri (logica identica v4.2)
│   │   ├── input.c             # Intrare 230V (logica identica v4.2)
│   │   └── sms.c               # SMS via AT+CMGS (logica identica v4.2)
│   └── include/
│       ├── ergo_pins.h         # Pini GPIO STM32C011
│       ├── ergo_config.h       # Constante + ConfigData + Flash addr
│       ├── ergo_uart.h
│       ├── ergo_gsm.h
│       ├── ergo_led.h
│       ├── ergo_input.h
│       └── ergo_sms.h
└── docs/
    ├── led_behavior.md
    ├── sms_commands.md
    └── wiring.md
```

## Compilare si programare

### v4.2 — OpenCPU A7670E

Necesita SIMCom OpenCPU SDK pentru A7670E si ARM GCC Toolchain.

```sh
make
```

Programare prin UART pe conectorul **J4** (TX, RX, GND) de pe placa HXY-A7670E-V1.3.

### v5.0 — STM32C011F4U6TR + A7682E

Necesita [STM32CubeC0 CMSIS](https://github.com/STMicroelectronics/STM32CubeC0) si `arm-none-eabi-gcc`.

```sh
cd stm32c011
export CUBE_PATH=/opt/STM32CubeC0
make
```

Programare prin **ST-Link V2** (SWD: PA13/PA14):

```sh
make flash
```

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

**Reguli:** LED verde clipeste = software ruleaza OK. LED galben clipeste = conectat 4G. LED rosu = tensiune fizica prezenta pe intrare (timp real). La impuls valid, toate 3 aprinse fix 3 secunde.

## Logica detectare impuls

- **Durata minima impuls valid:** 0.8 secunde (800ms) continuu
- **Cooldown dupa SMS:** 20 secunde default (configurabil prin SMS `#cd*<s>#`, interval 10-3600s)
- **Scanare intrare:** la 10ms

```
Intrare 230V:  ___████████___██___████████████___████████___
                   0.8s OK   <0.8  ignorat(cd)      0.8s OK
                      ↓        ↓       ↓               ↓
Actiune:         SMS TRIMIS  NIMIC   NIMIC         SMS TRIMIS
                      |←── 20s cooldown ──→|           |←── 20s...
```

## Protectie anti-spam (defectare hardware/software)

In cazul unui defect (ex: GPIO blocat HIGH, bug software), modulul ar putea trimite SMS-uri la infinit. Mecanismul de protectie:

- **Limita burst:** maxim **20 alarme** consecutive inainte de blocare
- **Reset automat dupa 2h de liniste:** daca nu s-a trimis nicio alarma in ultimele 2 ore, contorul se reseteaza automat
- **Contorul persista** in filesystem la reset watchdog (nu se pierde la repornire)
- **Reset manual:** comanda SMS `#rsms#`

**Comportament:**

| Scenariu | Rezultat |
|----------|----------|
| GPIO defect (declanseaza continuu) | 20 alarme → blocat permanent (fara niciodata 2h liniste) |
| Alarme dimineata, tehnicianul repara | 20 alarme → 2h liniste → contor reset → alarma seara trimisa ✓ |
| Reset watchdog in timp ce e blocat | Contorul persista, 2h liniste necesare pentru deblocare |
| Reset manual de operator | `#rsms#` → deblocare imediata |

## Configurare prin SMS

Comenzile se trimit prin SMS catre numarul SIM din modul. Dupa fiecare comanda, modulul raspunde cu configuratia curenta.

### Comenzi disponibile

| Comanda | Actiune |
|---------|---------|
| `#msm*<text>#` | Setare mesaj alerta (max 300 caractere, fara diacritice) |
| `#msm*#` | Stergere mesaj alerta |
| `#01*<numar>#` ... `#05*<numar>#` | Setare numere destinatari 1-5 |
| `#01*#` ... `#05*#` | Stergere numere destinatari |
| `#cd*<secunde>#` | Setare cooldown (10-3600 secunde, ex: `#cd*300#` = 5 minute) |
| `#cd*#` | Reset cooldown la valoarea din fabrica (20 secunde) |
| `#config#` | Afisare configuratie curenta |
| `#rsms#` | Reset manual contor alarme (deblocare dupa atingerea limitei de 20) |

### Comenzi multiple (intr-un singur SMS)

```
#msm*Atentie gaz oprit#, #01*0712345678#, #02*0798765432#
```

### Reset complet

```
#msm*#, #01*#, #02*#, #03*#, #04*#, #05*#, #cd*#
```

### Format raspuns configuratie

```
01:0762862213,02:(gol),03:(gol),04:(gol),05:1745,msm:ALARMA GAZ OPRIT TEST,cd:20s,alarme:3/20,semnal:80%
```

Campul `alarme:3/20` indica cate alarme s-au trimis din limita curenta (reset dupa 2h de liniste).

## Configuratie din fabrica

| Parametru | Valoare |
|-----------|---------|
| Nr01 | `0762862213` (presetat) |
| Nr02 | (gol) |
| Nr03 | (gol) |
| Nr04 | (gol) |
| Nr05 | `1745` (numar scurt, presetat) |
| Mesaj | `ALARMA GAZ OPRIT TEST` (default din fabrica, configurabil prin SMS) |
| Cooldown | `20` secunde (default, configurabil prin SMS `#cd*<s>#`, interval 10-3600s) |

**v4.2:** Configuratia se salveaza in filesystem-ul intern A7670E la `/simcom/ergo_config.dat`.

**v5.0:** Configuratia se salveaza in Flash STM32C011, pagina 15 (adresa `0x08007800`, 2KB rezervata). La prima pornire sau Flash sters, se reinitializeaza cu valorile din fabrica.

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

## Diferente cheie v4.2 vs v5.0

| | v4.2 (OpenCPU) | v5.0 (STM32 + A7682E) |
|---|---|---|
| **Modul GSM** | A7670E (LTE Cat M1/NB1) | A7682E (LTE Cat 1) |
| **Firmware ruleaza pe** | ARM Cortex-A5 din A7670E | STM32C011 (MCU separat) |
| **API** | `sAPI_*` (SIMCom OpenCPU SDK) | AT commands + CMSIS STM32 |
| **Stocare config** | `/simcom/ergo_config.dat` | Flash STM32 page 15 |
| **Watchdog** | `sAPI_WdtStart(60)` 60s | IWDG STM32 30s |
| **Receptie SMS** | `sAPI_SmsReadMsg()` polling | URC `+CMT:` direct pe UART |
| **Programare** | UART J4 (OpenCPU loader) | ST-Link V2 (SWD) |
| **Logica functionala** | — | **identica** |

## Certificare (in curs)

Produsul este in faza prototip/pre-test. Directive UE vizate:
- RED 2014/53/EU (echipamente radio) - modulele GSM SIMCom sunt deja certificate
- LVD 2014/35/EU (siguranta electrica)
- EMC 2014/30/EU (compatibilitate electromagnetica)
- RoHS 2011/65/EU

Laborator de testare: ICPE-CA (Romania).

## Autori

**Plato Global SRL** - Romania
In parteneriat cu **Navoi Concept** pentru **Energoinstal Premium SRL** (firma autorizata ANRE pentru instalatii gaz)
