# ERGO GASALERT v5.0 - STM32C011F4U6TR + SIMCom A7682E

## Schimbare arhitecturala fata de v4.2

| Aspect | v4.2 (OpenCPU) | v5.0 (STM32 + A7682E) |
|--------|---------------|----------------------|
| **Procesor** | ARM Cortex-A5 integrat in A7670E | STM32C011F4U6TR (Cortex-M0+) |
| **Modul GSM** | SIMCom A7670E (LTE Cat M1/NB1) | SIMCom A7682E (LTE Cat 1) |
| **API firmware** | sAPI_* (SIMCom OpenCPU SDK) | AT commands UART + CMSIS STM32 |
| **Comunicatie GSM** | Apeluri SDK directe | USART1 115200 baud AT commands |
| **Stocare config** | Filesystem intern A7670E | Flash STM32 (page 15, 0x08007800) |
| **Watchdog** | sAPI_WdtStart(60) | IWDG STM32 (30s) |
| **Timing** | sAPI_GetTicks() * 5ms | SysTick 1ms (48MHz HSI) |
| **Entry point** | sAPP_MainTask(void*) | main() standard C |
| **GPIO** | sAPI_GpioSetValue() | Registre STM32 CMSIS directe |

---

## Hardware

### MCU: STM32C011F4U6TR
- **Core**: ARM Cortex-M0+ @ 48MHz (HSI48)
- **Flash**: 32KB (30KB cod + 2KB configuratie)
- **RAM**: 6KB SRAM
- **Pachet**: UFQFPN20 (20 pini)

### Modul GSM: SIMCom A7682E
- **Retea**: LTE Cat 1 / UMTS / GSM
- **Comunicatie cu MCU**: UART 115200 baud (AT commands)
- **Comanda pornire**: PWRKEY activ LOW >1.5s

---

## Pinout STM32C011F4U6TR (UFQFPN20)

| Pin STM32 | Functie | Directie | Nota |
|-----------|---------|----------|------|
| PA0 | LED_VERDE | Output PP | Clipeste 0.5s = firmware OK |
| PA1 | LED_GALBEN | Output PP | Clipeste 0.5s = retea LTE OK |
| PA4 | LED_ROSU | Output PP | Aprins FIX = 230V pe intrare |
| PA5 | PIN_INTRARE | Input PD | Optocuplor 230V AC |
| PA6 | GSM_PWRKEY | Output PP | HIGH 1.5s -> pornire A7682E |
| PA2 | USART1_TX (AF1) | Output AF | -> A7682E RX |
| PA3 | USART1_RX (AF1) | Input AF | <- A7682E TX |
| PA13 | SWDIO | - | Programare/debug |
| PA14 | SWCLK | - | Programare/debug |

---

## Schema conectare STM32C011 <-> A7682E

```
STM32C011                          A7682E
---------                          ------
PA2 (USART1_TX) -----[3V3/5V]----> RXD
PA3 (USART1_RX) <------------------ TXD
PA6 (PWRKEY)    ----[NPN tranz]---> PWRKEY (activ LOW)
GND             <-----------------> GND
3V3             <-[LDO din A7682E]- VDD_EXT (optional)

Note:
- A7682E lucreaza la 1.8V logica pentru UART (verifica tensiunea!)
- Daca A7682E are UART la 1.8V si STM32 la 3.3V: translator de nivel obligatoriu
- PWRKEY: PA6 HIGH -> tranzistor NPN -> PWRKEY la masa -> pornire modul
```

---

## Structura cod (v5.0)

```
stm32c011/
├── Makefile                    - Build ARM GCC pentru STM32C011
├── linker/
│   └── STM32C011F4UX_FLASH.ld  - Linker script (30KB cod + 2KB config)
├── include/
│   ├── ergo_pins.h             - Pini GPIO STM32
│   ├── ergo_config.h           - Constante, structuri, Flash addr
│   ├── ergo_uart.h             - Layer UART AT commands
│   ├── ergo_gsm.h              - Modul A7682E + retea
│   ├── ergo_led.h              - Control LED-uri
│   ├── ergo_input.h            - Monitorizare intrare 230V
│   └── ergo_sms.h              - Trimitere/receptie SMS
└── src/
    ├── main.c                  - Entry point + SysTick + IWDG + loop
    ├── config.c                - Config in Flash STM32 + utilitare
    ├── uart.c                  - USART1 circular buffer + IRQ
    ├── gsm.c                   - AT commands + initGSM + retea + URC
    ├── led.c                   - LED control (aceeasi logica v4.2)
    ├── input.c                 - Detectie impuls (aceeasi logica v4.2)
    └── sms.c                   - SMS via AT+CMGS (aceeasi logica v4.2)
```

---

## Cum functioneaza receptia SMS

In v4.2 (OpenCPU), SMS-urile se citeau cu `sAPI_SmsReadMsg()` din sloturile SIM.

In v5.0, se foloseste **livrare directa URC** (`AT+CNMI=2,2,0,0,0`):

1. A7682E primeste un SMS
2. Trimite automat pe UART:
   ```
   \r\n+CMT: "+40712345678","","26/03/22,12:00:00+08"\r\n
   Continut mesaj\r\n
   ```
3. `gsm.c` intercepteaza `+CMT:` in buffer-ul circular UART
4. Extrage expeditor + continut
5. Seteaza flag `urcSMSPending = 1`
6. La urmatoarea apelare `verificaSMSPrimit()` (1s), comanda e procesata

---

## Configuratie Flash (Page 15 = 0x08007800)

Configuratia se salveaza in ultima pagina de Flash (2KB):

```
0x08007800  [ConfigData struct - 424 bytes]
  - mesajAlerta[301]    : mesaj SMS alarma
  - numere[5][21]       : maxim 5 numere de telefon
  - cooldownSecunde     : cooldown configurabil
  - alarmeAziCount      : contor anti-spam (persista la reboot)
  - ultimaAlarmaMs      : timestamp ultima alarma
  - flagValid           : 0xA5 = valid
  - _padding[5]         : aliniere 8 bytes (Flash STM32C011 = dubla-cuvant)
0x08007BA8  [spatiu liber: ~1.6KB]
0x080087FF  [sfarsit pagina config]
```

La `salveazaConfig()`:
1. Dezactivare intreruperi (`__disable_irq()`)
2. Deblocare Flash (KEYR)
3. Stergere page 15 (~20ms)
4. Scriere bloc 424 bytes in dubla-cuvant (64-bit)
5. Blocare Flash
6. Reactivare intreruperi

---

## Compilare

### Cerinte
- `arm-none-eabi-gcc` >= 10.x
- STM32CubeC0 CMSIS package

### Instalare STM32CubeC0
```bash
# Cloneaza sau descarca din:
# https://github.com/STMicroelectronics/STM32CubeC0
# Sau instaleaza STM32CubeMX si genereaza pentru STM32C011

# Seteaza calea in Makefile:
export CUBE_PATH=/opt/STM32CubeC0
```

### Build
```bash
cd stm32c011
make
# Output: build/ergo_gasalert_stm32.bin
# Output: build/ergo_gasalert_stm32.hex
```

### Programare (ST-Link V2)
```bash
make flash
# SAU manual cu OpenOCD:
openocd -f interface/stlink.cfg -f target/stm32c0x.cfg \
  -c "program build/ergo_gasalert_stm32.hex verify reset exit"
```

---

## Logica pastrata identica din v4.2

- Detectie impuls: 0.8s minim continuu
- Cooldown: 20s default, configurabil 10-3600s prin SMS
- Protectie anti-spam: 20 alarme burst, reset dupa 2h liniste
- Persistenta reboot watchdog: contorul `alarmeAziCount` persista
- Comenzi SMS: identice (#msm*, #01-05*, #cd*, #config#, #rsms#)
- Comportament LED-uri: identic (boot, clipire, impuls 3s, retea)
- Numere scurte suportate (ex: 1745)
