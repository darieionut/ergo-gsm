# ERGO GASALERT - Modul GSM Notificare SMS (4G)

Firmware pentru modulul GSM de notificare SMS bazat pe **SIMCom A7670E OpenCPU**.

## Descriere

Dispozitiv alimentat la 230V AC care monitorizeaza o intrare de 230V AC (detectare prezenta tensiune). La eveniment (impuls valid >= 0.8s), transmite notificari SMS prin retea GSM/LTE 4G catre pana la 5 destinatari configurati.

## Hardware

| Componenta    | Detalii                                          |
|---------------|--------------------------------------------------|
| **Placa**     | HXY-A7670E-V1.3                                  |
| **Modul**     | SIMCom A7670E (procesor Unisoc 8910DM, ARM Cortex-A5) |
| **Retea**     | Orange Romania                                   |
| **SIM**       | micro-SIM                                        |
| **Alimentare**| 230V AC (sursa in comutatie izolata galvanic)    |
| **Intrare**   | 230V AC (prin optocuplor)                        |
| **LED-uri**   | 2 (verde + galben)                               |
| **Releu**     | Fara releu                                       |

## Structura proiect

```
ergo-gasalert/
├── src/
│   ├── main.c          # Functia principala + loop
│   ├── config.c        # Configuratie: incarcare/salvare/fabrica
│   ├── sms.c           # SMS: trimitere, comenzi, configurare
│   ├── input.c         # Intrare: detectare impuls + cooldown
│   ├── led.c           # LED-uri: verde + galben
│   └── network.c       # Retea: conectare Orange Romania
├── include/
│   ├── ergo_pins.h     # Definire pini GPIO
│   ├── ergo_config.h   # Constante, structuri, prototipuri
│   ├── ergo_led.h      # Prototipuri LED
│   ├── ergo_input.h    # Prototipuri intrare
│   ├── ergo_sms.h      # Prototipuri SMS
│   └── ergo_network.h  # Prototipuri retea
└── docs/
    ├── led_behavior.md # Comportament LED-uri
    ├── sms_commands.md # Comenzi SMS
    └── wiring.md       # Schema conectare
```

## Compilare

Necesita **SIMCom OpenCPU SDK** pentru A7670E si **ARM GCC Toolchain**.

## Programare

Prin UART pe conectorul **J4** (TX, RX, GND) de pe placa HXY-A7670E-V1.3.

## Configuratie din fabrica

| Parametru | Valoare                        |
|-----------|--------------------------------|
| Nr01      | 0762862213 (presetat)          |
| Nr02      | (gol)                          |
| Nr03      | (gol)                          |
| Nr04      | (gol)                          |
| Nr05      | 1745 (numar scurt, presetat)   |
| Mesaj     | (gol) - trebuie configurat prin SMS |

## LED-uri

| Stare                     | LED Verde       | LED Galben      |
|---------------------------|-----------------|-----------------|
| Boot (initializare)       | APRINS FIX      | STINS           |
| Soft OK, fara retea       | Clipeste 0.5s   | STINS           |
| Soft OK, conectat 4G      | Clipeste 0.5s   | Clipeste 0.5s   |
| Impuls detectat           | APRINS FIX 3s   | APRINS FIX 3s   |

## Autor

**Plato Global SRL** - Romania
