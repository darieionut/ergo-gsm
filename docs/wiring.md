# Schema de conectare - ERGO GASALERT

## Riglete

| Rigleta | Functie | Tensiune |
|---------|---------|----------|
| Rigleta 1 | Alimentare modul | 230V AC / 50Hz |
| Rigleta 2 | Intrare monitorizata (detectare tensiune) | 230V AC |

## Functia principala

Cand pe **Rigleta 2** apare tensiune 230V AC timp de minim **0.8 secunde** continuu, modulul trimite SMS la toate numerele configurate (max 5).

## Logica detectare impuls

1. Tensiune apare pe rigleta 2 -> start cronometru
2. Daca ramane minim 0.8s continuu -> **IMPULS VALID** -> trimite SMS
3. Daca dispare inainte de 0.8s -> zgomot, **IGNORAT**
4. Dupa impuls valid -> **cooldown 20 secunde** (ignora alte impulsuri)
5. Frecventa maxima: 1 SMS la 20 secunde

## Diagrama comportament

```
Intrare 230V:  ___████████___██___████████████___████████___
                   0.8s OK   <0.8  ignorat(cd)      0.8s OK
                      ↓        ↓       ↓               ↓
Actiune:         SMS TRIMIS  NIMIC   NIMIC         SMS TRIMIS
                      |←── 20s cooldown ──→|           |←── 20s...
```

## Conector pe placa HXY-A7670E-V1.3

| Conector | Pini | Functie |
|----------|------|---------|
| CN1 | N, U | Alimentare 230V AC + intrare monitorizata |
| J4 | TX, RX, GND | UART debug / programare firmware |
| Slot SIM | - | Cartela micro-SIM (Orange Romania) |
| SMA | - | Antena externa GSM/LTE |

## Note importante

- Montajul si cablarea la 230V AC se fac **numai de personal calificat**
- Intrerupeti alimentarea inainte de orice interventie
- Modulul este destinat montajului **interior** (casa scarii), pe perete
- Grad protectie: IP20
