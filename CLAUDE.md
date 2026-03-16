# CLAUDE.md - ERGO GASALERT

**Versiune firmware:** v4.2

## Despre proiect

Firmware OpenCPU pentru modulul GSM de notificare SMS bazat pe **SIMCom A7670E** (procesor Unisoc 8910DM, ARM Cortex-A5). Produsul se numeste **ERGO GASALERT** si este dezvoltat de **Plato Global SRL** (Romania) in parteneriat cu **Navoi Concept** pentru **Energoinstal Premium SRL** (firma autorizata ANRE pentru instalatii gaz).

**Scop:** Modul pasiv de monitorizare montat in casa scarii care detecteaza prezenta tensiunii 230V AC pe o intrare si trimite SMS de alarma la maxim 5 numere de telefon. Se instaleaza in paralel cu electrovalva de gaz din cladirile rezidentiale - cand sistemul de detectie gaz opreste gazul (electrovalva primeste 230V), modulul nostru detecteaza acea tensiune si notifica locatarii prin SMS.

**IMPORTANT:** Modulul este PASIV - nu influenteaza in niciun fel functionarea sistemului de detectie gaz sau a electrovalvei. Este un dispozitiv auxiliar de notificare.

## Hardware

- **Placa:** HXY-A7670E-V1.3
- **Procesor:** SIMCom A7670E cu OpenCPU integrat (ARM Cortex-A5) - NU exista microcontroller separat
- **Retea:** Orange Romania, APN "internet", SMSC +40744000060
- **SIM:** micro-SIM
- **Alimentare:** 230V AC prin sursa in comutatie izolata galvanic (SELV)
- **Intrare:** 230V AC prin optocuplor (izolat galvanic)
- **LED-uri:** 3 (verde + galben + rosu) - pe viitoarea versiune de PCB
- **Antena:** externa, conector SMA

### Conectori pe placa

- **CN1 (rigleta verde):** Alimentare 230V AC + intrare monitorizata
- **J4 (conector debug UART):** TX, RX, GND - pentru programare firmware
- **Slot micro-SIM**
- **Conector antena SMA**

### Riglete

| Rigleta | Functie | Tensiune |
|---------|---------|----------|
| Rigleta 1 | Alimentare modul | 230V AC / 50Hz |
| Rigleta 2 | Intrare monitorizata (detectare tensiune) | 230V AC |

## Limbaj si platforma

- **Limbaj:** C
- **Platforma:** SIMCom OpenCPU SDK pentru A7670E
- **Compilare:** ARM GCC Toolchain
- **Programare:** UART prin conectorul J4
- **API-uri SDK:** functii `sAPI_*` (GPIO, SMS, Network, Timer, Filesystem)
- **Entry point:** `sAPP_MainTask(void* pData)`

## Structura cod

```
Makefile          - Template build pentru ARM GCC + SIMCom OpenCPU SDK

src/
  main.c          - Functia principala sAPP_MainTask() + loop
  config.c        - Incarcare/salvare configuratie din filesystem + utilitare
                    (inclusiv ergo_strcasecmp/ergo_strncasecmp, getTickMs, delayMs)
  sms.c           - Trimitere SMS, procesare comenzi SMS, configurare
  input.c         - Monitorizare intrare 230V + detectare impuls + cooldown
  led.c           - Control LED-uri (verde + galben)
  network.c       - Conectare/reconectare retea Orange Romania

include/
  ergo_pins.h     - Definire pini GPIO
  ergo_config.h   - Constante timp, structura ConfigData, numere fabrica,
                    prototipuri utilitare si inlocuitori POSIX
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
- LED verde APRINS FIX = boot in curs (software se initializeaza)
- LED verde CLIPESTE = software initializat si ruleaza OK (cu sau fara semnal GSM)
- LED galben STINS = nu e conectat la retea 4G
- LED galben CLIPESTE = conectat la retea 4G
- LED rosu APRINS FIX = tensiune 230V prezenta fizic pe intrare (timp real, inclusiv zgomot sub 0.8s)
- LED rosu STINS = intrare inactiva
- La impuls valid: TOATE 3 aprinse fix 3 secunde, apoi revin la starea normala

## Logica detectare impuls si trimitere SMS

### Parametri:
- **Durata minima impuls:** 0.8 secunde (800ms) continuu
- **Cooldown dupa SMS:** 20 secunde
- **Frecventa maxima:** 1 SMS la 20 secunde

### Algoritm:
1. Monitorizeaza continuu intrarea (scanare la 10ms)
2. Cand apare tensiune 230V pe intrare -> start cronometru
3. Daca tensiunea ramane **minim 0.8s continuu** -> IMPULS VALID
4. Daca tensiunea dispare inainte de 0.8s -> zgomot, IGNORAT
5. La impuls valid, verifica cooldown:
   - NU in cooldown -> LED-uri aprinse 3s + TRIMITE SMS + cooldown 20s
   - DA in cooldown -> IGNORA impulsul
6. Dupa 20s cooldown -> accepta impulsuri noi

### Diagrama:
```
Intrare 230V:  ___████████___██___████████████___████████___
                   0.8s OK   <0.8  ignorat(cd)      0.8s OK
                      ↓        ↓       ↓               ↓
Actiune:         SMS TRIMIS  NIMIC   NIMIC         SMS TRIMIS
                      |←── 20s cooldown ──→|           |←── 20s...
```

## Configurare prin SMS

Toate comenzile se trimit prin SMS catre numarul SIM din modul. Dupa fiecare comanda, modulul raspunde automat cu configuratia curenta.

### Comenzi:
- `#msm*<text>#` - Setare mesaj alerta (max 300 caractere, fara diacritice)
- `#msm*#` - Stergere mesaj alerta
- `#01*<numar>#` ... `#05*<numar>#` - Setare numere 1-5
- `#01*#` ... `#05*#` - Stergere numere 1-5
- `#cd*<secunde>#` - Setare cooldown (10-3600s, ex: `#cd*300#` = 5 minute)
- `#cd*#` - Reset cooldown la fabrica (20 secunde)
- `#config#` - Afisare configuratie curenta (include `cd:<secunde>`)

### Comenzi multiple:
Separate prin virgula intr-un singur SMS. Exemplu:
`#msm*Atentie gaz oprit#, #01*0712345678#, #02*0798765432#`

### Reset complet:
`#msm*#, #01*#, #02*#, #03*#, #04*#, #05*#, #cd*#`

### Format raspuns configuratie:
```
01:0762862213,02:(gol),03:(gol),04:(gol),05:1745,msm:Alarma gaz oprit.,cd:20s,semnal:80%
```

## Configuratie din fabrica

Constante definite in `ergo_config.h`: `FABRICA_NUMAR_01`, `FABRICA_NUMAR_05`, `FABRICA_MESAJ_ALERTA`, `FABRICA_COOLDOWN_S`, `ORANGE_SMSC`.

| Parametru | Valoare |
|-----------|---------|
| Nr01 | **0762862213** (presetat) |
| Nr02 | (gol) |
| Nr03 | (gol) |
| Nr04 | (gol) |
| Nr05 | **1745** (numar scurt, presetat) |
| Mesaj | **ALARMA GAZ OPRIT TEST** (default din fabrica, definit ca `FABRICA_MESAJ_ALERTA`) |
| Cooldown | **20 secunde** (configurabil prin SMS `#cd*<s>#`, interval 10-3600s) |

Configuratia se salveaza in filesystem-ul intern A7670E la calea `/simcom/ergo_config.dat`. La prima pornire sau daca fisierul e corupt, se reinitializeaza cu valorile din fabrica.

## Tipuri numere suportate

- Standard Romania: `07XXXXXXXX` (10 cifre)
- Cu prefix international: `+407XXXXXXXX`
- Numere scurte: 3-6 cifre (ex: `1745`)
- Functia `esteNumarScurt()` detecteaza numere sub 7 cifre fara prefix `+`

## Intervale loop principal

| Actiune | Interval |
|---------|----------|
| Scanare intrare | 10ms |
| Actualizare LED-uri | 50ms |
| Verificare SMS primite | 1s |
| Verificare retea | 60s |

## Pini GPIO (de verificat pe schema)

Pinii sunt definiti in `include/ergo_pins.h` cu valori orientative:
- `PIN_LED_VERDE` = SC_MODULE_GPIO_01
- `PIN_LED_GALBEN` = SC_MODULE_GPIO_02
- `PIN_LED_ROSU` = SC_MODULE_GPIO_03
- `PIN_INTRARE` = SC_MODULE_GPIO_05

**IMPORTANT:** Pinii exacti trebuie verificati pe schema electrica HXY-A7670E-V1.3.

## Note pentru dezvoltare

- Tick rate SDK: 5ms/tick (folosit in `config.c`: `sAPI_GetTicks() * 5`) - confirmat in cod, dar verificati si in `sdk_config.h`
- Functii SDK: `sAPI_GetTicks()`, `sAPI_TaskSleep()`, `sAPI_GpioSetValue()`, `sAPI_GpioGetValue()`, `sAPI_SmsSendMsg()`, `sAPI_SmsReadMsg()`, `sAPI_SmsDeleteMsg()`, `sAPI_NetworkGetCgreg()`, `sAPI_NetworkGetCsq()`, `sAPI_WdtStart()`, `sAPI_WdtFeed()`, `sAPI_fopen()`, `sAPI_fread()`, `sAPI_fwrite()`, `sAPI_fclose()`
- SMS text mode (nu PDU), charset GSM, SMSC setat prin `sAPI_SmsCfgScaAddr(ORANGE_SMSC)` in `initRetea()`
- Variabila `reteaConectata` este globala, definita in `network.c`, folosita in `led.c`
- Variabila `intrareActiva` este globala, definita in `input.c`, folosita in `led.c` (pentru LED rosu)
- Structura `ConfigData` cu flag `0xA5` pentru validare; camp `cooldownSecunde` pentru cooldown configurabil
- `strcasecmp`/`strncasecmp` POSIX **nu exista** in SDK SIMCom - folositi inlocuitorii proprii `ergo_strcasecmp()` si `ergo_strncasecmp()` definiti in `config.c` si declarati in `ergo_config.h`
- Main loop: `sAPI_TaskSleep(2)` la final = 2 ticks * 5ms = ~10ms yield CPU
- `verificaSMSPrimit()` itereaza sloturile 1-20 pana gaseste primul SMS disponibil; buffer continut 512 bytes; proceseaza un singur SMS per apel pentru a nu bloca loop-ul
- `trimiteSMSAlarma()`: pauza 1 secunda intre SMS-uri consecutive (`delayMs(1000)`)
- `initRetea()`: 15 tentative cu delay 2s intre ele (max ~30s timeout initial)
- `reconectareRetea()`: 1 singura tentativa (apelata din loop la fiecare 60s daca retea pierduta; fara delay intern)
- `verificaConectareRetea()`: apelata si din `initRetea()` si din loop-ul principal (la 60s); actualizeaza `reteaConectata`; returneaza 1=conectat, 0=neconectat
- `obtiSemnalCSQ()`: returneaza CSQ 0-31 (31=maxim) sau -1 la eroare; valoarea 99 inseamna "necunoscut" conform GSM; folosita in `trimiteConfigCurenta()`
- Watchdog hardware: `sAPI_WdtStart(60)` pornit inainte de `initRetea()`; alimentat cu `sAPI_WdtFeed()` in loop si in `trimiteSMSAlarma()`; reseteaza modulul daca loop-ul se blocheaza > 60s
- Buffer raspuns config `trimiteConfigCurenta()`: 450 bytes (suficient pentru 5 numere + mesaj 300 chars + cd + semnal)

## Certificare (in curs)

Produsul este in faza prototip/pre-test. Directive UE vizate:
- RED 2014/53/EU (echipamente radio) - modulul GSM SIMCom A7670E este deja certificat
- LVD 2014/35/EU (siguranta electrica)
- EMC 2014/30/EU (compatibilitate electromagnetica)
- RoHS 2011/65/EU

Laboratorul de testare: ICPE-CA (Romania).

## Conventii cod

- Limba comentarii: romana (fara diacritice in cod)
- Nume variabile/functii: romana (camelCase)
- Debug: `sAPI_Debug("[MODUL] mesaj")` cu prefixe: [ERGO], [LED], [INPUT], [SMS], [RETEA], [CONFIG], [CMD], [COOLDOWN], [BOOT], [GPIO]
- Toate constantele in `ergo_config.h`
- Fiecare modul (.c) include headerul propriu + `ergo_config.h`
