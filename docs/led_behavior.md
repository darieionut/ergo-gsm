# Comportament LED-uri - ERGO GASALERT

## LED-uri disponibile

| LED | Culoare | Pin |
|-----|---------|-----|
| LED Verde | Verde | PIN_LED_VERDE (GPIO_01) |
| LED Galben | Galben | PIN_LED_GALBEN (GPIO_02) |

## Stari si comportament

### 1. Boot (initializare software)

| LED Verde | LED Galben |
|-----------|------------|
| **APRINS FIX** (continuu) | **STINS** |

Dureaza pana cand software-ul termina initializarea (incarcare config, conectare retea).

### 2. Software initializat, cauta retea 4G

| LED Verde | LED Galben |
|-----------|------------|
| Clipeste: ON 0.5s / OFF 0.5s | **STINS** |

LED verde clipeste = firmware OK.
LED galben stins = nu s-a conectat inca la retea.

### 3. Software initializat, conectat la retea 4G

| LED Verde | LED Galben |
|-----------|------------|
| Clipeste: ON 0.5s / OFF 0.5s | Clipeste: ON 0.5s / OFF 0.5s |

Ambele clipesc uniform = totul functioneaza normal.

### 4. Impuls valid detectat (trimitere SMS)

| LED Verde | LED Galben | Durata |
|-----------|------------|--------|
| **APRINS FIX** | **APRINS FIX** | **3 secunde** |

Dupa 3 secunde, ambele revin la clipirea normala.

### 5. Modul nealimentat / firmware neîncărcat

| LED Verde | LED Galben |
|-----------|------------|
| STINS | STINS |

## Rezumat vizual rapid

| Ce vezi | Ce inseamna |
|---------|-------------|
| Verde APRINS FIX + Galben STINS | Boot (se initializeaza) |
| Verde clipeste + Galben STINS | Firmware OK, cauta retea |
| Verde clipeste + Galben clipeste | Firmware OK, conectat 4G |
| Ambele aprinse fix 3s | Impuls detectat, trimite SMS |
| Nimic aprins | Nealimentat sau fara firmware |
