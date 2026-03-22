# CLAUDE.md - ERGO GASALERT

**Versiune firmware:** v5.0

## Despre proiect

Firmware pentru modulul GSM de notificare SMS bazat pe **STM32C011F4U6TR** (ARM Cortex-M0+, 48MHz) ca MCU principal si **SIMCom A7682E** (LTE Cat-1) ca modul GSM controlat prin comenzi AT via UART. Produsul se numeste **ERGO GASALERT** si este dezvoltat de **Plato Global SRL** (Romania) in parteneriat cu **Navoi Concept** pentru **Energoinstal Premium SRL** (firma autorizata ANRE pentru instalatii gaz).

**Scop:** Modul pasiv de monitorizare montat in casa scarii care detecteaza prezenta tensiunii 230V AC pe o intrare si trimite SMS de alarma la maxim 5 numere de telefon. Se instaleaza in paralel cu electrovalva de gaz din cladirile rezidentiale.

**IMPORTANT:** Modulul este PASIV - nu influenteaza in niciun fel functionarea sistemului de detectie gaz sau a electrovalvei. Este un dispozitiv auxiliar de notificare.

## Hardware

- **MCU:** STM32C011F4U6TR (ARM Cortex-M0+, 48MHz, 16KB Flash, 6KB RAM, UFQFPN20)
- **Modul GSM:** SIMCom A7682E (LTE Cat-1/Cat-M1/NB-IoT, controlat prin AT commands via UART)
- **Retea:** Orange Romania, APN "internet", SMSC +40744000060
- **SIM:** nano-SIM (pe modulul A7682E)
- **Alimentare:** 230V AC prin sursa in comutatie izolata galvanic (SELV) → 3.3V STM32 + tensiune A7682E
- **Intrare:** 230V AC prin optocuplor (izolat galvanic) → GPIO STM32 (PA5)
- **LED-uri:** 3 (verde + galben + rosu) - controlate direct de STM32
- **Antena:** externa, conector SMA (pe A7682E)
- **Programare/Debug:** SWD (ST-Link, PA13/PA14) + UART debug optional (PB6)

### Arhitectura sistem

```
230V AC ─── Optocuplor ─── PA5 (GPIO input, pull-down)
                                     │
                               STM32C011F4U6TR
                               (MCU principal)
                                     │
                            PA9/PA10 USART1 (AT commands, 115200)
                                     │
                              SIMCom A7682E
                              (modul LTE Cat-1)
                                     │
                               Retea 4G Orange
                                     │
                               SMS destinatari
```

### Pini GPIO (orientativi, de verificat pe schema finala)

| Pin STM32 | Functie | Directie |
|-----------|---------|----------|
| PA0 | LED verde | Output PP |
| PA1 | LED galben | Output PP |
| PA4 | LED rosu | Output PP |
| PA5 | Intrare optocuplor (230V detect) | Input Pull-Down |
| PA6 | A7682E PWRKEY | Output PP |
| PA9 | USART1 TX → A7682E RX | AF1 |
| PA10 | USART1 RX ← A7682E TX | AF1 |
| PA13 | SWDIO (programare/debug) | Rezervat |
| PA14 | SWDCLK (programare/debug) | Rezervat |
| PB6 | USART2 TX (debug UART optional) | AF2 |

### Conectori pe placa

- **CN1 (rigleta verde):** Alimentare 230V AC + intrare monitorizata
- **J4 (SWD + UART debug):** SWDIO, SWDCLK, GND, TX debug - programare si debug
- **Slot nano-SIM** (pe modulul A7682E)
- **Conector antena SMA**

### Riglete

| Rigleta | Functie | Tensiune |
|---------|---------|----------|
| Rigleta 1 | Alimentare modul | 230V AC / 50Hz |
| Rigleta 2 | Intrare monitorizata (detectare tensiune) | 230V AC |

## Limbaj si platforma

- **Limbaj:** C (C11)
- **MCU:** STM32C011F4U6TR - ARM Cortex-M0+, 48MHz, 16KB Flash, 6KB RAM
- **SDK/HAL:** STM32 HAL (STM32CubeC0 - STM32Cube_FW_C0)
- **Compilare:** ARM GCC Toolchain (`arm-none-eabi-gcc`)
- **Programare:** SWD via ST-Link (J4)
- **Comunicare GSM:** UART1 (PA9/PA10) → A7682E, comenzi AT (AT+CMGF, AT+CMGS, AT+CMGR, AT+CMGD, AT+CREG, AT+CSQ, AT+CSCA)
- **Entry point:** `main()` standard C
- **Config storage:** Flash intern STM32 (pagina 7, adresa 0x08003800, 2KB)
- **Watchdog:** IWDG hardware STM32 (~28s timeout, LSI 32kHz / prescaler 256)
- **Tick:** `HAL_GetTick()` (1ms rezolutie, SysTick)

### Resurse STM32C011F4 utilizate

| Periferic | Utilizare |
|-----------|-----------|
| USART1 (PA9/PA10) | Comunicare AT cu A7682E, 115200 baud, RX interrupt |
| USART2 (PB6) | Debug UART optional (TX only), compilat cu -DDEBUG_UART_ENABLE |
| GPIOA (PA0,PA1,PA4) | LED-uri verde, galben, rosu |
| GPIOA PA5 | Intrare optocuplor (input pull-down) |
| GPIOA PA6 | A7682E PWRKEY |
| IWDG | Watchdog hardware (~28s timeout) |
| Flash pagina 7 (0x08003800) | Stocare configuratie persistenta (2KB) |
| SysTick | `HAL_GetTick()` - baza de timp 1ms |

**ATENTIE resurse limitate:**
- 16KB Flash: codul + HAL NU trebuie sa depaseasca 0x08003800 (14KB). Verifica sectiunea .text in fisierul .map dupa compilare!
- 6KB RAM: buffere mari sunt declarate `static` in sms.c (nu pe stiva). Nu folosi `malloc`.

## Structura cod

```
Makefile          - Build ARM GCC + STM32 HAL/LL

src/
  main.c          - main() + HAL init (GPIO, USART1, IWDG) + loop principal
                    Contine: alimenteazaWDT(), dbg(), ISR USART1, HAL_UART_RxCpltCallback
  config.c        - Configuratie in Flash STM32 (erase + program DOUBLEWORD) + utilitare
                    (inclusiv ergo_strcasecmp/ergo_strncasecmp, getTickMs, delayMs)
  gsm.c           - Driver AT commands pentru A7682E
                    Ring buffer RX 256 bytes, gsm_cmd(), gsm_trimite_sms(),
                    gsm_citeste_sms(), gsm_sterge_sms(), gsm_get_creg(), gsm_get_csq()
  sms.c           - Trimitere SMS, procesare comenzi, configurare (via gsm.c)
  input.c         - Monitorizare intrare 230V + detectare impuls + cooldown (via HAL_GPIO)
  led.c           - Control LED-uri verde + galben + rosu (via HAL_GPIO_WritePin)
  network.c       - Conectare/reconectare retea Orange Romania (via gsm_get_creg/csq)

include/
  ergo_pins.h     - Definire pini GPIO STM32 (port + pin HAL)
  ergo_config.h   - Constante timp, structura ConfigData, numere fabrica,
                    prototipuri utilitare, CONFIG_FLASH_ADDR, WATCHDOG_*
  ergo_gsm.h      - Prototipuri driver AT / A7682E
  ergo_led.h      - Prototipuri LED
  ergo_input.h    - Prototipuri intrare
  ergo_sms.h      - Prototipuri SMS
  ergo_network.h  - Prototipuri retea

docs/
  led_behavior.md  - Documentatie comportament LED-uri
  sms_commands.md  - Documentatie comenzi SMS
  wiring.md        - Documentatie cablare
```

## Comportament LED-uri (CRITIC - respecta exact)

Modulul are **3 LED-uri**: verde, galben si rosu.

| Stare | LED Verde | LED Galben | LED Rosu |
|-------|-----------|------------|----------|
| **Boot (initializare software)** | **APRINS FIX** (continuu) | **STINS** | **STINS** |
| **Software OK, cauta retea 4G** | Clipeste ON 0.5s / OFF 0.5s | **STINS** | **STINS** |
| **Software OK, conectat la retea 4G** | Clipeste ON 0.5s / OFF 0.5s | Clipeste ON 0.5s / OFF 0.5s | **STINS** |
| **Tensiune pe intrare (< 0.8s, zgomot)** | Clipeste ON 0.5s / OFF 0.5s | Clipeste/Stins | **APRINS FIX** |
| **Impuls valid detectat** | **APRINS FIX 3 secunde** | **APRINS FIX 3 secunde** | **APRINS FIX 3 secunde** |
| **Dupa 3 secunde, intrare inactiva** | Revine la clipire | Revine (sau stins daca nu e retea) | **STINS** |
| **Nealimentat** | Stins | Stins | Stins |

### Reguli LED:
- LED verde APRINS FIX = boot in curs
- LED verde CLIPESTE = software initializat si ruleaza OK
- LED galben STINS = nu e conectat la retea 4G
- LED galben CLIPESTE = conectat la retea 4G
- LED rosu APRINS FIX = tensiune 230V prezenta fizic pe intrare (timp real)
- La impuls valid: TOATE 3 aprinse fix 3 secunde, apoi revin la starea normala

## Logica detectare impuls si trimitere SMS

### Parametri:
- **Durata minima impuls:** 0.8 secunde (800ms) continuu
- **Cooldown dupa SMS:** 20 secunde default (configurabil prin SMS)
- **Frecventa maxima:** 1 SMS la 20 secunde

### Algoritm:
1. Monitorizeaza continuu intrarea PA5 (scanare la 10ms)
2. Cand apare tensiune 230V → PA5=HIGH → start cronometru
3. Daca tensiunea ramane **minim 0.8s continuu** → IMPULS VALID
4. Daca dispare inainte de 0.8s → zgomot, IGNORAT
5. La impuls valid, verifica cooldown:
   - NU in cooldown → TRIMITE SMS + LED-uri 3s + cooldown 20s
   - DA in cooldown → IGNORA
6. Dupa cooldown → accepta impulsuri noi

## Protectie anti-spam

- **`LIMITA_ALARME_BURST`** = 20 alarme consecutive maxim
- **`CALM_PERIOD_MS`** = 2 ore fara alarme = reset automat contor
- Contor persistent in Flash (persista la reset watchdog)
- Reset manual: comanda SMS `#rsms#`

### Detectie reboot in `incarcaConfig()`:
Daca `ultimaAlarmaMs > getTickMs()` → reboot detectat (HAL_GetTick porneste de la 0).
Actiune: `ultimaAlarmaMs = getTickMs()` dar `alarmeAziCount` se pastreaza.

## Configurare prin SMS

### Comenzi:
- `#msm*<text>#` - Setare mesaj alerta (max 160 caractere, fara diacritice)
- `#msm*#` - Stergere mesaj alerta
- `#01*<numar>#` ... `#05*<numar>#` - Setare numere 1-5
- `#01*#` ... `#05*#` - Stergere numere 1-5
- `#cd*<secunde>#` - Setare cooldown (10-3600s)
- `#cd*#` - Reset cooldown la fabrica (20 secunde)
- `#config#` - Afisare configuratie curenta
- `#rsms#` - Reset manual contor alarme

### Comenzi multiple (intr-un singur SMS):
`#msm*Atentie gaz oprit#, #01*0712345678#, #02*0798765432#`

### Format raspuns configuratie:
```
01:0762862213,02:(gol),03:(gol),04:(gol),05:1745,msm:ALARMA GAZ OPRIT,cd:20s,alarme:3/20,semnal:80%
```

## Configuratie din fabrica

| Parametru | Valoare |
|-----------|---------|
| Nr01 | **0762862213** (presetat) |
| Nr02-04 | (gol) |
| Nr05 | **1745** (numar scurt, presetat) |
| Mesaj | **ALARMA GAZ OPRIT** |
| Cooldown | **20 secunde** |

Configuratia se salveaza in Flash la 0x08003800 (pagina 7 STM32C011). La prima pornire sau Flash invalid (flag != 0xA5), se reinitializeaza cu valorile din fabrica.

## Tipuri numere suportate

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

## Note pentru dezvoltare

- **getTickMs()** = `HAL_GetTick()` (1ms/tick, SysTick 48MHz)
- **delayMs()** = `HAL_Delay(ms)`
- **alimenteazaWDT()** = `HAL_IWDG_Refresh(&hiwdg)` - definita in main.c
- **dbg()** = send string pe UART2 (TX only PB6); compilat doar cu `-DDEBUG_UART_ENABLE`
- **gsm_rx_callback()** = apelata din `HAL_UART_RxCpltCallback()` in main.c
- **Ring buffer UART RX:** 256 bytes in gsm.c; interrupt pe byte (HAL_UART_Receive_IT)
- **Trimitere SMS:** `AT+CMGS="numar"\r` → asteapta `>` → mesaj + 0x1A → asteapta `+CMGS:`
- **Citire SMS:** `AT+CMGR=<slot>` (slot 1-20); un singur SMS per apel verificaSMSPrimit
- **WDT in gsm.c:** `alimenteazaWDT()` apelata in gsm_readline() si gsm_wait_char() (operatii blocante)
- **Flash config:** programare DOUBLEWORD (8 bytes odata); struct padded la multiplu de 8
- **Alternate functions GPIO:** verifica AF1/AF2 pentru USART1/USART2 in datasheet STM32C011!
- **Pornire A7682E:** PWRKEY LOW 600ms → HIGH → asteapta "RDY" (timeout 10s)
- **SMS text mode (nu PDU):** `AT+CMGF=1`, charset `AT+CSCS="GSM"`, SMSC `AT+CSCA="+40744000060"`
- **`strcasecmp`/`strncasecmp` POSIX** nu exista in toolchain-ul default STM32 - folositi `ergo_strcasecmp()` / `ergo_strncasecmp()` din config.c
- **Buffere statice in sms.c:** `copie[]`, `buf[]`, `expeditor[]`, `continut[]` sunt `static` pentru a nu depasi stiva de ~1KB
- **MAX_LUNGIME_MESAJ = 160** (redus de la 300 la un SMS standard GSM; pastreaza RAM)
- **`reteaConectata`** = global definit in network.c, folosit in led.c si sms.c
- **`intrareActiva`** = global definit in input.c, folosit in led.c

## Certificare (in curs)

Produsul este in faza prototip/pre-test. Directive UE vizate:
- RED 2014/53/EU (echipamente radio) - modulul GSM SIMCom A7682E este deja certificat
- LVD 2014/35/EU (siguranta electrica)
- EMC 2014/30/EU (compatibilitate electromagnetica)
- RoHS 2011/65/EU

Laboratorul de testare: ICPE-CA (Romania).

## Conventii cod

- Limba comentarii: romana (fara diacritice in cod)
- Nume variabile/functii: romana (camelCase)
- Debug: `dbg("[MODUL] mesaj")` cu prefixe: [ERGO], [LED], [INPUT], [SMS], [RETEA], [CONFIG], [CMD], [COOLDOWN], [BOOT], [GSM], [ALARMA]
- Toate constantele in `ergo_config.h`
- Fiecare modul (.c) include headerul propriu + `ergo_config.h`
