#!/usr/bin/env python3
"""Genereaza documentul Word pentru ERGO GASALERT."""

from docx import Document
from docx.shared import Pt, RGBColor, Inches, Cm
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_ALIGN_VERTICAL
from docx.oxml.ns import qn
from docx.oxml import OxmlElement
import copy

doc = Document()

# --- Stiluri globale ---
style = doc.styles['Normal']
style.font.name = 'Calibri'
style.font.size = Pt(11)

# Margini pagina
for section in doc.sections:
    section.top_margin = Cm(2.5)
    section.bottom_margin = Cm(2.5)
    section.left_margin = Cm(2.5)
    section.right_margin = Cm(2.5)

def set_heading(doc, text, level=1, color=None):
    h = doc.add_heading(text, level=level)
    if color:
        for run in h.runs:
            run.font.color.rgb = RGBColor(*color)
    return h

def add_table(doc, headers, rows, col_widths=None):
    table = doc.add_table(rows=1 + len(rows), cols=len(headers))
    table.style = 'Table Grid'
    table.alignment = WD_TABLE_ALIGNMENT.LEFT

    # Header
    hdr = table.rows[0]
    for i, h in enumerate(headers):
        cell = hdr.cells[i]
        cell.text = h
        cell.paragraphs[0].runs[0].bold = True
        cell.paragraphs[0].alignment = WD_ALIGN_PARAGRAPH.CENTER
        # fundal albastru deschis pentru header
        tc = cell._tc
        tcPr = tc.get_or_add_tcPr()
        shd = OxmlElement('w:shd')
        shd.set(qn('w:val'), 'clear')
        shd.set(qn('w:color'), 'auto')
        shd.set(qn('w:fill'), '1F3864')
        tcPr.append(shd)
        cell.paragraphs[0].runs[0].font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)

    # Randuri
    for r_idx, row_data in enumerate(rows):
        row = table.rows[r_idx + 1]
        for c_idx, val in enumerate(row_data):
            cell = row.cells[c_idx]
            if isinstance(val, tuple):
                # (text, bold)
                cell.text = val[0]
                if val[1]:
                    cell.paragraphs[0].runs[0].bold = True
            else:
                cell.text = str(val)
            cell.paragraphs[0].alignment = WD_ALIGN_PARAGRAPH.LEFT

    # Latime coloane
    if col_widths:
        for i, w in enumerate(col_widths):
            for row in table.rows:
                row.cells[i].width = Cm(w)
    return table

def add_code_block(doc, text):
    p = doc.add_paragraph()
    p.style = doc.styles['Normal']
    run = p.add_run(text)
    run.font.name = 'Courier New'
    run.font.size = Pt(9)
    # fundal gri
    pPr = p._p.get_or_add_pPr()
    shd = OxmlElement('w:shd')
    shd.set(qn('w:val'), 'clear')
    shd.set(qn('w:color'), 'auto')
    shd.set(qn('w:fill'), 'F2F2F2')
    pPr.append(shd)
    # border
    pBdr = OxmlElement('w:pBdr')
    for side in ['top', 'left', 'bottom', 'right']:
        bdr = OxmlElement(f'w:{side}')
        bdr.set(qn('w:val'), 'single')
        bdr.set(qn('w:sz'), '4')
        bdr.set(qn('w:space'), '4')
        bdr.set(qn('w:color'), 'AAAAAA')
        pBdr.append(bdr)
    pPr.append(pBdr)
    return p

# ============================================================
# PAGINA DE TITLU
# ============================================================
doc.add_paragraph()
doc.add_paragraph()

title = doc.add_paragraph()
title.alignment = WD_ALIGN_PARAGRAPH.CENTER
run = title.add_run('ERGO GASALERT')
run.bold = True
run.font.size = Pt(32)
run.font.color.rgb = RGBColor(0x1F, 0x38, 0x64)

subtitle = doc.add_paragraph()
subtitle.alignment = WD_ALIGN_PARAGRAPH.CENTER
run2 = subtitle.add_run('Modul GSM de Notificare SMS pentru Sisteme de Detectie Gaz')
run2.font.size = Pt(16)
run2.font.color.rgb = RGBColor(0x44, 0x72, 0xC4)

doc.add_paragraph()

version_p = doc.add_paragraph()
version_p.alignment = WD_ALIGN_PARAGRAPH.CENTER
version_p.add_run('Versiune firmware: ').bold = False
vr = version_p.add_run('v4.2')
vr.bold = True
vr.font.size = Pt(13)

doc.add_paragraph()
doc.add_paragraph()

# Info companii
info = doc.add_paragraph()
info.alignment = WD_ALIGN_PARAGRAPH.CENTER
info.add_run('Dezvoltat de: Plato Global SRL (Romania)\n')
info.add_run('Partener: Navoi Concept\n')
info.add_run('Beneficiar: Energoinstal Premium SRL (firma autorizata ANRE)\n')
info.add_run('Data documentului: 18 Martie 2026')

doc.add_paragraph()
doc.add_paragraph()

# Linie de separare
hr = doc.add_paragraph('─' * 80)
hr.alignment = WD_ALIGN_PARAGRAPH.CENTER

doc.add_page_break()

# ============================================================
# 1. DESCRIERE GENERALA
# ============================================================
set_heading(doc, '1. Descriere Generala', 1)

doc.add_paragraph(
    'ERGO GASALERT este un modul pasiv de monitorizare alimentat la 230V AC, conceput pentru '
    'montarea in casa scarii blocurilor rezidentiale. Dispozitivul detecteaza prezenta tensiunii '
    '230V AC pe o intrare dedicata si transmite notificari SMS prin retea GSM/LTE 4G catre pana '
    'la 5 destinatari configurati.'
)

doc.add_paragraph(
    'Scenariul tipic de utilizare: Cand sistemul de detectie gaz din cladire opreste gazul prin '
    'electrovalva (electrovalva primeste 230V), modulul detecteaza acea tensiune si notifica '
    'locatarii prin SMS in timp real.'
)

p = doc.add_paragraph()
run = p.add_run('IMPORTANT: ')
run.bold = True
run.font.color.rgb = RGBColor(0xC0, 0x00, 0x00)
p.add_run(
    'Modulul este PASIV - nu influenteaza in niciun fel functionarea sistemului de detectie '
    'gaz sau a electrovalvei. Este un dispozitiv auxiliar de notificare.'
)

doc.add_paragraph()

# ============================================================
# 2. SPECIFICATII TEHNICE HARDWARE
# ============================================================
set_heading(doc, '2. Specificatii Tehnice Hardware', 1)

add_table(doc,
    ['Componenta', 'Detalii'],
    [
        ('Placa PCB', 'HXY-A7670E-V1.3'),
        ('Modul GSM', 'SIMCom A7670E cu OpenCPU integrat'),
        ('Procesor', 'Unisoc 8910DM, ARM Cortex-A5'),
        ('Retea', 'Orange Romania, APN: internet, SMSC: +40744000060'),
        ('Cartela SIM', 'micro-SIM'),
        ('Alimentare', '230V AC / 50Hz prin sursa in comutatie izolata galvanic (SELV)'),
        ('Intrare monitorizata', '230V AC prin optocuplor (izolat galvanic)'),
        ('LED-uri', '3 (verde + galben + rosu)'),
        ('Antena', 'Externa, conector SMA'),
        ('Montaj', 'Interior (casa scarii), pe perete'),
        ('Grad protectie', 'IP20'),
    ],
    col_widths=[5, 12]
)

doc.add_paragraph()
set_heading(doc, '2.1 Conectori pe placa', 2)

add_table(doc,
    ['Conector', 'Functie', 'Detalii'],
    [
        ('CN1 (rigleta verde)', 'Alimentare 230V AC + intrare monitorizata', 'Pini N, U'),
        ('J4', 'Debug UART - programare firmware', 'TX, RX, GND'),
        ('Slot micro-SIM', 'Cartela SIM Orange Romania', '-'),
        ('Conector SMA', 'Antena externa GSM/LTE', '-'),
    ],
    col_widths=[4, 7, 6]
)

doc.add_paragraph()
set_heading(doc, '2.2 Riglete de conexiune', 2)

add_table(doc,
    ['Rigleta', 'Functie', 'Tensiune'],
    [
        ('Rigleta 1', 'Alimentare modul', '230V AC / 50Hz'),
        ('Rigleta 2', 'Intrare monitorizata (detectare tensiune)', '230V AC'),
    ],
    col_widths=[3.5, 9, 4]
)

doc.add_paragraph()

# ============================================================
# 3. FIRMWARE SI PLATFORMA SOFTWARE
# ============================================================
set_heading(doc, '3. Firmware si Platforma Software', 1)

add_table(doc,
    ['Parametru', 'Valoare'],
    [
        ('Versiune firmware', 'v4.2'),
        ('Limbaj de programare', 'C'),
        ('Platforma', 'SIMCom OpenCPU SDK pentru A7670E'),
        ('Compilator', 'ARM GCC Toolchain'),
        ('Programare', 'UART prin conectorul J4 (TX, RX, GND)'),
        ('Entry point', 'sAPP_MainTask(void* pData)'),
        ('API-uri SDK', 'Functii sAPI_* (GPIO, SMS, Network, Timer, Filesystem)'),
        ('Fisier configuratie', '/simcom/ergo_config.dat (filesystem intern A7670E)'),
    ],
    col_widths=[5, 12]
)

doc.add_paragraph()
set_heading(doc, '3.1 Structura proiectului', 2)

add_code_block(doc,
"""ergo-gsm/
├── Makefile            # Template build ARM GCC + SIMCom OpenCPU SDK
├── src/
│   ├── main.c          # Entry point sAPP_MainTask() + loop principal
│   ├── config.c        # Configuratie: incarcare/salvare/fabrica + utilitare
│   ├── sms.c           # SMS: trimitere, comenzi, configurare
│   ├── input.c         # Intrare: detectare impuls 230V + cooldown
│   ├── led.c           # LED-uri: verde + galben + rosu
│   └── network.c       # Retea: conectare/reconectare Orange Romania
├── include/
│   ├── ergo_pins.h     # Definire pini GPIO
│   ├── ergo_config.h   # Constante, structuri, prototipuri utilitare
│   ├── ergo_led.h      # Prototipuri LED
│   ├── ergo_input.h    # Prototipuri intrare
│   ├── ergo_sms.h      # Prototipuri SMS
│   └── ergo_network.h  # Prototipuri retea
└── docs/
    ├── led_behavior.md # Comportament LED-uri
    ├── sms_commands.md # Comenzi SMS
    └── wiring.md       # Schema conectare""")

doc.add_paragraph()
set_heading(doc, '3.2 Pini GPIO', 2)

add_table(doc,
    ['Constanta', 'GPIO', 'Functie'],
    [
        ('PIN_LED_VERDE', 'SC_MODULE_GPIO_01', 'LED verde'),
        ('PIN_LED_GALBEN', 'SC_MODULE_GPIO_02', 'LED galben'),
        ('PIN_LED_ROSU', 'SC_MODULE_GPIO_03', 'LED rosu'),
        ('PIN_INTRARE', 'SC_MODULE_GPIO_05', 'Intrare 230V (prin optocuplor)'),
    ],
    col_widths=[5, 5, 7]
)

doc.add_paragraph()
p = doc.add_paragraph()
run = p.add_run('Nota: ')
run.bold = True
p.add_run('Pinii exacti trebuie verificati pe schema electrica HXY-A7670E-V1.3.')

doc.add_paragraph()

# ============================================================
# 4. LOGICA DE FUNCTIONARE
# ============================================================
set_heading(doc, '4. Logica de Functionare', 1)

set_heading(doc, '4.1 Detectarea impulsului si trimiterea SMS', 2)

doc.add_paragraph(
    'Modulul monitorizeaza continuu intrarea (scanare la 10ms). Algoritmul de detectie:'
)

steps = [
    'Tensiune 230V apare pe intrare → porneste cronometrul',
    'Daca tensiunea ramane minim 0.8 secunde continuu → IMPULS VALID',
    'Daca tensiunea dispare inainte de 0.8s → zgomot, IGNORAT',
    'La impuls valid, se verifica cooldown-ul:',
    '   • NU in cooldown → LED-uri aprinse 3s + TRIMITE SMS + porneste cooldown 20s',
    '   • DA in cooldown → IGNORA impulsul',
    'Dupa 20s cooldown → accepta impulsuri noi',
]
for s in steps:
    p = doc.add_paragraph(s, style='List Bullet' if not s.startswith('   ') else 'Normal')

doc.add_paragraph()
set_heading(doc, '4.2 Parametri de detectie', 2)

add_table(doc,
    ['Parametru', 'Valoare', 'Descriere'],
    [
        ('Durata minima impuls', '0.8 secunde (800ms)', 'Impulsuri mai scurte sunt considerate zgomot'),
        ('Cooldown implicit', '20 secunde', 'Configurabil prin SMS: 10-3600 secunde'),
        ('Frecventa scanare', '10ms', 'Verificare stare intrare'),
        ('Frecventa maxima', '1 SMS / 20 secunde', 'Cu setarile implicite'),
    ],
    col_widths=[5, 4.5, 8]
)

doc.add_paragraph()
set_heading(doc, '4.3 Diagrama comportament detectie', 2)

add_code_block(doc,
"""Intrare 230V:  ___████████___██___████████████___████████___
                   0.8s OK   <0.8  ignorat(cd)      0.8s OK
                      ↓        ↓       ↓               ↓
Actiune:         SMS TRIMIS  NIMIC   NIMIC         SMS TRIMIS
                      |←── 20s cooldown ──→|           |←── 20s...""")

doc.add_paragraph()
set_heading(doc, '4.4 Intervale loop principal', 2)

add_table(doc,
    ['Actiune', 'Interval'],
    [
        ('Scanare intrare', '10ms'),
        ('Actualizare LED-uri', '50ms'),
        ('Verificare SMS primite', '1 secunda'),
        ('Verificare retea', '60 secunde'),
    ],
    col_widths=[8, 5]
)

doc.add_paragraph()

# ============================================================
# 5. COMPORTAMENT LED-URI
# ============================================================
set_heading(doc, '5. Comportament LED-uri', 1)

doc.add_paragraph(
    'Modulul dispune de 3 LED-uri (verde, galben, rosu) care indica starea operationala in timp real.'
)

add_table(doc,
    ['Stare', 'LED Verde', 'LED Galben', 'LED Rosu'],
    [
        ('Boot (initializare software)', 'APRINS FIX', 'STINS', 'STINS'),
        ('Software OK, cauta retea 4G', 'Clipeste 0.5s/0.5s', 'STINS', 'STINS'),
        ('Software OK, conectat la retea 4G', 'Clipeste 0.5s/0.5s', 'Clipeste 0.5s/0.5s', 'STINS'),
        ('Tensiune pe intrare (< 0.8s, zgomot)', 'Clipeste 0.5s/0.5s', 'Clipeste/Stins', 'APRINS FIX'),
        ('Impuls valid detectat (3 secunde)', 'APRINS FIX', 'APRINS FIX', 'APRINS FIX'),
        ('Dupa 3 secunde, intrare inactiva', 'Revine la clipire', 'Revine (sau stins)', 'STINS'),
        ('Nealimentat / fara firmware', 'STINS', 'STINS', 'STINS'),
    ],
    col_widths=[5.5, 3.5, 3.5, 3.5]
)

doc.add_paragraph()
set_heading(doc, '5.1 Reguli LED', 2)

rules = [
    'LED VERDE APRINS FIX = boot in curs (software se initializeaza)',
    'LED VERDE CLIPESTE = software initializat si ruleaza OK (cu sau fara semnal GSM)',
    'LED GALBEN STINS = nu este conectat la retea 4G',
    'LED GALBEN CLIPESTE = conectat la retea 4G',
    'LED ROSU APRINS FIX = tensiune 230V prezenta fizic pe intrare (timp real, inclusiv zgomot sub 0.8s)',
    'LED ROSU STINS = intrare inactiva',
    'La impuls valid: TOATE 3 LED-uri aprinse fix 3 secunde, apoi revin la starea normala',
]
for r in rules:
    doc.add_paragraph(r, style='List Bullet')

doc.add_paragraph()

# ============================================================
# 6. PROTECTIE ANTI-SPAM
# ============================================================
set_heading(doc, '6. Protectie Anti-Spam', 1)

doc.add_paragraph(
    'Mecanism de protectie impotriva trimiterii necontrolate de SMS-uri in caz de defectare '
    'hardware (GPIO blocat HIGH) sau bug software.'
)

set_heading(doc, '6.1 Parametri', 2)

add_table(doc,
    ['Parametru', 'Valoare', 'Descriere'],
    [
        ('LIMITA_ALARME_BURST', '20 alarme consecutive', 'Limita maxima inainte de blocare'),
        ('CALM_PERIOD_MS', '2 ore (7.200.000 ms)', 'Perioada de liniste pentru reset automat contor'),
    ],
    col_widths=[5, 5, 7]
)

doc.add_paragraph()
set_heading(doc, '6.2 Logica trimitere SMS alarma', 2)

logic_steps = [
    'Daca (acum - ultimaAlarmaMs) >= 2h SI ultimaAlarmaMs > 0 → reset contor (liniste = problema rezolvata)',
    'Daca alarmeAziCount >= 20 → BLOCAT, nu trimite',
    'alarmeAziCount++, ultimaAlarmaMs = acum, salveaza config → INAINTE de trimitere',
    'Trimite SMS-urile',
]
for i, s in enumerate(logic_steps, 1):
    doc.add_paragraph(f'{i}. {s}')

doc.add_paragraph()
set_heading(doc, '6.3 Persistenta la reboot (watchdog)', 2)

doc.add_paragraph(
    'In incarcaConfig(), dupa incarcare: daca ultimaAlarmaMs > getTickMs() → reboot detectat '
    '(tick-urile au pornit de la 0). Actiune: ultimaAlarmaMs = getTickMs() (resetam referinta '
    'temporala) dar alarmeAziCount se pastreaza. Rezultat: modulul defect care rebooteaza '
    'continuu nu isi reseteaza contorul; deblocare doar dupa 2h fara alarme.'
)

doc.add_paragraph()
set_heading(doc, '6.4 Scenarii', 2)

add_table(doc,
    ['Scenariu', 'Comportament'],
    [
        ('GPIO blocat HIGH (trigger continuu)', '20 alarme → blocat; fara 2h liniste → ramane blocat'),
        ('Alarme reale + reparatie', '20 alarme → 2h liniste → reset automat → alarma noua trimisa ✓'),
        ('Reboot watchdog repetat', 'Contorul persista; 2h de uptime linistit necesare pentru deblocare'),
        ('Operator deblocheaza manual', '#rsms# → alarmeAziCount=0, ultimaAlarmaMs=0, salvat'),
    ],
    col_widths=[6, 11]
)

doc.add_paragraph()

# ============================================================
# 7. CONFIGURARE PRIN SMS
# ============================================================
set_heading(doc, '7. Configurare prin SMS', 1)

doc.add_paragraph(
    'Toate comenzile se trimit prin SMS catre numarul SIM din modul. Dupa fiecare comanda, '
    'modulul raspunde automat cu configuratia curenta. Nu se accepta diacritice in mesaje.'
)

set_heading(doc, '7.1 Lista completa comenzi', 2)

add_table(doc,
    ['Comanda', 'Actiune', 'Exemplu'],
    [
        ('#msm*<text>#', 'Setare mesaj alerta (max 300 caractere, fara diacritice)', '#msm*Alarma gaz oprit#'),
        ('#msm*#', 'Stergere mesaj alerta', '#msm*#'),
        ('#01*<numar>#', 'Setare numar destinatar 1', '#01*0712345678#'),
        ('#02*<numar>#', 'Setare numar destinatar 2', '#02*0798765432#'),
        ('#03*<numar>#', 'Setare numar destinatar 3', '#03*0762862765#'),
        ('#04*<numar>#', 'Setare numar destinatar 4', '#04*0762862890#'),
        ('#05*<numar>#', 'Setare numar destinatar 5', '#05*1745#'),
        ('#01*# ... #05*#', 'Stergere numar destinatar', '#03*#'),
        ('#cd*<secunde>#', 'Setare cooldown intre alarme (10-3600s)', '#cd*300# (5 minute)'),
        ('#cd*#', 'Reset cooldown la valoarea din fabrica (20s)', '#cd*#'),
        ('#config#', 'Afisare configuratie curenta', '#config#'),
        ('#rsms#', 'Reset manual contor alarme (deblocare dupa 20)', '#rsms#'),
    ],
    col_widths=[4, 8, 5]
)

doc.add_paragraph()
set_heading(doc, '7.2 Comenzi multiple intr-un singur SMS', 2)

doc.add_paragraph('Comenzile se separa prin virgula:')

add_code_block(doc, '#msm*Atentie gaz oprit#, #01*0712345678#, #02*0798765432#')
doc.add_paragraph()
add_code_block(doc, '# Reset complet:\n#msm*#, #01*#, #02*#, #03*#, #04*#, #05*#, #cd*#')

doc.add_paragraph()
set_heading(doc, '7.3 Format raspuns configuratie', 2)

doc.add_paragraph('Dupa fiecare comanda, modulul raspunde cu un SMS in formatul:')
add_code_block(doc, '01:0762862213,02:(gol),03:(gol),04:(gol),05:1745,msm:Alarma gaz oprit.,cd:20s,alarme:3/20,semnal:80%')

doc.add_paragraph()
add_table(doc,
    ['Camp', 'Descriere'],
    [
        ('01..05', 'Numerele configurate ("(gol)" daca nesetat)'),
        ('msm', 'Mesajul de alerta curent'),
        ('cd', 'Cooldown intre alarme (secunde)'),
        ('alarme', 'Alarme trimise / limita in fereastra curenta (ex: 3/20)'),
        ('semnal', 'Intensitate semnal GSM (%)'),
    ],
    col_widths=[3, 14]
)

doc.add_paragraph()
set_heading(doc, '7.4 Tipuri de numere suportate', 2)

add_table(doc,
    ['Tip', 'Format', 'Exemplu'],
    [
        ('Standard Romania', '07XXXXXXXX (10 cifre)', '0762862213'),
        ('Numar scurt', '3-6 cifre', '1745'),
        ('Cu prefix international', '+407XXXXXXXX', '+40762862213'),
    ],
    col_widths=[5, 5, 7]
)

doc.add_paragraph()

# ============================================================
# 8. CONFIGURATIE DIN FABRICA
# ============================================================
set_heading(doc, '8. Configuratie din Fabrica', 1)

doc.add_paragraph(
    'La prima pornire sau daca fisierul de configuratie este corupt, modulul se '
    'reinitializeaza cu valorile din fabrica definite in ergo_config.h.'
)

add_table(doc,
    ['Parametru', 'Valoare', 'Observatii'],
    [
        ('Nr01', '0762862213', 'Presetat din fabrica'),
        ('Nr02', '(gol)', 'De configurat'),
        ('Nr03', '(gol)', 'De configurat'),
        ('Nr04', '(gol)', 'De configurat'),
        ('Nr05', '1745', 'Numar scurt presetat (SMS Info)'),
        ('Mesaj alerta', 'ALARMA GAZ OPRIT TEST', 'Default fabrica - de personalizat'),
        ('Cooldown', '20 secunde', 'Configurabil prin SMS: 10-3600s'),
        ('Limita alarme', '20 consecutive', 'Reset dupa 2h fara alarme'),
        ('Fisier config', '/simcom/ergo_config.dat', 'Filesystem intern A7670E'),
        ('Flag validare', '0xA5', 'Detectie coruptie fisier'),
    ],
    col_widths=[4, 5.5, 7.5]
)

doc.add_paragraph()

# ============================================================
# 9. SCHEMA DE CONECTARE
# ============================================================
set_heading(doc, '9. Schema de Conectare', 1)

doc.add_paragraph(
    'IMPORTANT: Montajul si cablarea la 230V AC se efectueaza NUMAI de personal calificat. '
    'Intrerupeti intotdeauna alimentarea inainte de orice interventie.'
)

set_heading(doc, '9.1 Conectarea rigleta CN1', 2)

add_table(doc,
    ['Terminal', 'Functie', 'Tensiune', 'Note'],
    [
        ('Rigleta 1 - N', 'Neutru alimentare modul', '230V AC / 50Hz', 'Faza/Neutru retea'),
        ('Rigleta 1 - U', 'Faza alimentare modul', '230V AC / 50Hz', 'Faza/Neutru retea'),
        ('Rigleta 2', 'Intrare monitorizata', '230V AC', 'Conectat in paralel cu electrovalva'),
    ],
    col_widths=[3.5, 5, 3.5, 5]
)

doc.add_paragraph()
set_heading(doc, '9.2 Conectarea tipcala (bloc rezidential)', 2)

doc.add_paragraph(
    'Modulul se monteaza in paralel cu electrovalva de gaz din tabloul de detectie gaz al cladirii. '
    'Cand detectorul de gaz comanda inchiderea electrovalvei (aplica 230V pe aceasta), '
    'modulul ERGO GASALERT detecteaza simultan acea tensiune si trimite SMS-uri de alarma.'
)

steps_connect = [
    'Alimentati modulul de la o sursa 230V AC permanenta (Rigleta 1)',
    'Conectati Rigleta 2 in paralel cu bornele electrovalvei de gaz',
    'Montati cartela micro-SIM Orange Romania in slotul dedicat',
    'Conectati antena GSM/LTE externa la conectorul SMA',
    'Configurati numerele de telefon prin SMS (comenzile #01*..#05*)',
    'Configurati mesajul de alarma prin SMS (#msm*<text>#)',
]
for s in steps_connect:
    doc.add_paragraph(s, style='List Number')

doc.add_paragraph()

# ============================================================
# 10. WATCHDOG SI FIABILITATE
# ============================================================
set_heading(doc, '10. Watchdog Hardware si Fiabilitate', 1)

doc.add_paragraph(
    'Modulul include un mecanism de watchdog hardware pentru a preveni blocarea software:'
)

wdt_info = [
    'Watchdog pornit cu sAPI_WdtStart(60) inainte de initializarea retelei',
    'Watchdog alimentat cu sAPI_WdtFeed() in loop principal si in trimiteSMSAlarma()',
    'Daca loop-ul se blocheaza > 60 secunde → resetare automata hardware a modulului',
    'Contorul de alarme persista in fisierul de configuratie la resetare watchdog',
]
for w in wdt_info:
    doc.add_paragraph(w, style='List Bullet')

doc.add_paragraph()
set_heading(doc, '10.1 Comportament la reconectare retea', 2)

add_table(doc,
    ['Functie', 'Comportament'],
    [
        ('initRetea()', '15 tentative de conectare cu delay 2s intre ele (max ~30s)'),
        ('reconectareRetea()', '1 singura tentativa; apelata din loop la fiecare 60s daca retea pierduta'),
        ('verificaConectareRetea()', 'Actualizeaza variabila reteaConectata; returneaza 1=conectat, 0=neconectat'),
        ('obtiSemnalCSQ()', 'Returneaza CSQ 0-31 (31=maxim) sau -1 la eroare; 99=necunoscut'),
    ],
    col_widths=[5, 12]
)

doc.add_paragraph()

# ============================================================
# 11. CERTIFICARE
# ============================================================
set_heading(doc, '11. Certificare (in curs)', 1)

doc.add_paragraph(
    'Produsul este in faza prototip/pre-test. Directive UE vizate pentru certificare:'
)

cert_items = [
    'RED 2014/53/EU (echipamente radio) - modulul GSM SIMCom A7670E este deja certificat de producator',
    'LVD 2014/35/EU (siguranta electrica la tensiune joasa)',
    'EMC 2014/30/EU (compatibilitate electromagnetica)',
    'RoHS 2011/65/EU (restrictia substantelor periculoase)',
]
for c in cert_items:
    doc.add_paragraph(c, style='List Bullet')

doc.add_paragraph()
p = doc.add_paragraph()
p.add_run('Laborator de testare: ').bold = False
p.add_run('ICPE-CA (Romania)').bold = True

doc.add_paragraph()

# ============================================================
# 12. INFORMATII COMPANIE
# ============================================================
set_heading(doc, '12. Informatii Companie si Contact', 1)

add_table(doc,
    ['Entitate', 'Rol', 'Tara'],
    [
        ('Plato Global SRL', 'Dezvoltator firmware si produs', 'Romania'),
        ('Navoi Concept', 'Partener de proiect', 'Romania'),
        ('Energoinstal Premium SRL', 'Beneficiar final (firma autorizata ANRE instalatii gaz)', 'Romania'),
    ],
    col_widths=[5, 8, 4]
)

doc.add_paragraph()

# Footer
doc.add_paragraph()
hr2 = doc.add_paragraph('─' * 80)
hr2.alignment = WD_ALIGN_PARAGRAPH.CENTER

footer_p = doc.add_paragraph()
footer_p.alignment = WD_ALIGN_PARAGRAPH.CENTER
footer_p.add_run('ERGO GASALERT v4.2 | Plato Global SRL | 2026 | Document generat automat din codul sursa')
footer_p.runs[0].font.size = Pt(9)
footer_p.runs[0].font.color.rgb = RGBColor(0x80, 0x80, 0x80)

# Salvare
output_path = '/home/user/ergo-gsm/ERGO_GASALERT_Documentatie_Produs.docx'
doc.save(output_path)
print(f'Document salvat: {output_path}')
