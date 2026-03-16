# Comenzi SMS - ERGO GASALERT

## Reguli generale

- Comenzile se trimit prin SMS catre numarul SIM din modul
- Dupa fiecare comanda, modulul raspunde cu configuratia curenta
- Mesajul configurat: max 300 caractere (recomandat max 120)
- **Nu se accepta diacritice** (a, a, i, s, t)
- Comenzi multiple: separate prin virgula intr-un singur SMS

## Comenzi disponibile

### Mesaj de alerta

| Actiune | Comanda | Exemplu |
|---------|---------|---------|
| Setare mesaj | `#msm*<text>#` | `#msm*Alarma gaz oprit#` |
| Stergere mesaj | `#msm*#` | `#msm*#` |

### Numere de telefon (max 5)

| Actiune | Comanda | Exemplu |
|---------|---------|---------|
| Setare numar 1 | `#01*<numar>#` | `#01*0762862213#` |
| Stergere numar 1 | `#01*#` | `#01*#` |
| Setare numar 2 | `#02*<numar>#` | `#02*0774469691#` |
| Stergere numar 2 | `#02*#` | `#02*#` |
| Setare numar 3 | `#03*<numar>#` | `#03*0762862765#` |
| Stergere numar 3 | `#03*#` | `#03*#` |
| Setare numar 4 | `#04*<numar>#` | `#04*0762862890#` |
| Stergere numar 4 | `#04*#` | `#04*#` |
| Setare numar 5 | `#05*<numar>#` | `#05*1745#` |
| Stergere numar 5 | `#05*#` | `#05*#` |

### Cooldown intre alarme

| Actiune | Comanda | Exemplu |
|---------|---------|---------|
| Setare cooldown | `#cd*<secunde>#` | `#cd*300#` (5 minute) |
| Reset cooldown la fabrica (20s) | `#cd*#` | `#cd*#` |

### Verificare si control alarme

| Actiune | Comanda |
|---------|---------|
| Afisare configuratie | `#config#` |
| Reset contor alarme zilnice | `#rsms#` |

> **`#rsms#`** - reseteaza contorul de alarme al ferestrei curente de 24h.
> Folosit cand modulul a atins limita zilnica din cauza unor alarme legitime
> (ex: testare repetata) si operatorul vrea sa reactiveze notificarile imediat.

## Comenzi multiple intr-un singur SMS

Comenzile se separa prin virgula:

| Actiune | Comanda |
|---------|---------|
| Configurare completa | `#msm*Atentie gaz oprit#, #01*0712345678#, #02*0798765432#` |
| Reset complet | `#msm*#, #01*#, #02*#, #03*#, #04*#, #05*#` |
| Stergere 2 numere | `#03*#, #04*#` |

## Format raspuns configuratie

Dupa fiecare comanda, modulul raspunde cu un SMS in formatul:

```
01:0762862213,02:(gol),03:(gol),04:(gol),05:1745,msm:Alarma gaz oprit.,cd:20s,alarme:3/20,semnal:80%
```

| Camp | Descriere |
|------|-----------|
| `01`..`05` | Numerele configurate (`(gol)` daca nesetat) |
| `msm` | Mesajul de alerta curent |
| `cd` | Cooldown intre alarme (secunde) |
| `alarme` | Alarme trimise / limita in fereastra curenta de 24h |
| `semnal` | Intensitate semnal GSM (%) |

## Tipuri numere suportate

| Tip | Format | Exemplu |
|-----|--------|---------|
| Standard Romania | 07XXXXXXXX (10 cifre) | 0762862213 |
| Numar scurt | 3-6 cifre | 1745 |
| Cu prefix international | +407XXXXXXXX | +40762862213 |

## Configuratie din fabrica

| Parametru | Valoare |
|-----------|---------|
| Nr01 | 0762862213 (presetat) |
| Nr02-04 | (gol) |
| Nr05 | 1745 (presetat, numar scurt) |
| Mesaj | (gol) - TREBUIE configurat! |
