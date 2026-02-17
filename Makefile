# ==============================================================================
# Makefile - ERGO GASALERT
# Firmware OpenCPU pentru SIMCom A7670E (Unisoc 8910DM, ARM Cortex-A5)
#
# NOTA: Acesta este un Makefile template/generic.
#       Caile SDK_PATH si TOOLCHAIN_PATH trebuie adaptate la instalarea locala.
#
# Utilizare:
#   make          - compileaza proiectul
#   make clean    - sterge fisierele generate
#   make flash    - flashuieste firmware prin UART (J4)
#   make help     - afiseaza acest mesaj
# ==============================================================================

# ------------------------------------------------------------------------------
# Configurare cai (ADAPTEAZA la instalarea ta)
# ------------------------------------------------------------------------------

# Calea catre SDK-ul SIMCom A7670E OpenCPU
SDK_PATH ?= /opt/simcom/A7670E_SDK

# Calea catre ARM GCC Toolchain
TOOLCHAIN_PATH ?= /opt/arm-gcc/bin

# Portul serial pentru programare prin J4 UART
FLASH_PORT ?= /dev/ttyUSB0

# Baud rate pentru programare
FLASH_BAUD ?= 115200

# ------------------------------------------------------------------------------
# Toolchain ARM GCC (cross-compiler pentru Cortex-A5)
# ------------------------------------------------------------------------------

CC      = $(TOOLCHAIN_PATH)/arm-linux-gnueabihf-gcc
LD      = $(TOOLCHAIN_PATH)/arm-linux-gnueabihf-gcc
OBJCOPY = $(TOOLCHAIN_PATH)/arm-linux-gnueabihf-objcopy
SIZE    = $(TOOLCHAIN_PATH)/arm-linux-gnueabihf-size

# ------------------------------------------------------------------------------
# Numele proiectului si fisierele de iesire
# ------------------------------------------------------------------------------

PROJECT = ergo_gasalert
ELF     = $(PROJECT).elf
BIN     = $(PROJECT).bin
MAP     = $(PROJECT).map

# ------------------------------------------------------------------------------
# Fisiere sursa
# ------------------------------------------------------------------------------

SRCS = \
    src/main.c      \
    src/config.c    \
    src/sms.c       \
    src/input.c     \
    src/led.c       \
    src/network.c

# Fisiere obiect (generate in directorul build/)
BUILD_DIR = build
OBJS = $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# ------------------------------------------------------------------------------
# Include paths
# ------------------------------------------------------------------------------

INCLUDES = \
    -I./include                     \
    -I$(SDK_PATH)/include           \
    -I$(SDK_PATH)/include/simcom

# ------------------------------------------------------------------------------
# Flaguri compilare C
# ------------------------------------------------------------------------------

# Arhitectura: ARM Cortex-A5, thumb mode, soft-float
ARCH_FLAGS = \
    -mcpu=cortex-a5     \
    -mthumb             \
    -mfloat-abi=soft

# Optimizare si standard C
OPT_FLAGS = \
    -Os             \
    -std=c99

# Avertismente
WARN_FLAGS = \
    -Wall           \
    -Wextra         \
    -Wno-unused-parameter

# Definitii preprocessor
DEFINES = \
    -DSIMCOM_A7670E     \
    -DOPEN_CPU

CFLAGS = $(ARCH_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(DEFINES) $(INCLUDES)

# ------------------------------------------------------------------------------
# Flaguri linker
# ------------------------------------------------------------------------------

# Script linker furnizat de SDK
LDSCRIPT = $(SDK_PATH)/ld/opencpu.ld

# Biblioteci SDK SIMCom
LIBS = \
    -L$(SDK_PATH)/lib   \
    -lsimcom_opencpu    \
    -lc                 \
    -lm

LDFLAGS = \
    $(ARCH_FLAGS)               \
    -T$(LDSCRIPT)               \
    -Wl,-Map=$(MAP)             \
    -Wl,--gc-sections           \
    $(LIBS)

# ------------------------------------------------------------------------------
# Reguli build
# ------------------------------------------------------------------------------

# Regula default
.PHONY: all
all: $(BUILD_DIR) $(BIN)
	@echo ""
	@echo ">>> Build complet: $(BIN)"
	@$(SIZE) $(ELF)

# Creare director build
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compilare fisiere .c -> .o
$(BUILD_DIR)/%.o: src/%.c
	@echo "  CC  $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Linkare .o -> .elf
$(ELF): $(OBJS)
	@echo "  LD  $@"
	$(LD) $(OBJS) $(LDFLAGS) -o $@

# Conversie .elf -> .bin (pentru flashuire)
$(BIN): $(ELF)
	@echo "  BIN $@"
	$(OBJCOPY) -O binary $< $@

# ------------------------------------------------------------------------------
# Curatare
# ------------------------------------------------------------------------------

.PHONY: clean
clean:
	@echo "Stergere fisiere generate..."
	rm -rf $(BUILD_DIR) $(ELF) $(BIN) $(MAP)

# ------------------------------------------------------------------------------
# Flashuire prin UART (J4: TX, RX, GND)
# NOTA: Comanda exacta depinde de utilitarul de programare SIMCom
# ------------------------------------------------------------------------------

.PHONY: flash
flash: $(BIN)
	@echo "Flashuire $(BIN) pe $(FLASH_PORT) la $(FLASH_BAUD) baud..."
	@echo "NOTA: Inlocuieste comanda de mai jos cu utilitarul SIMCom real"
	# simcom_flash_tool --port $(FLASH_PORT) --baud $(FLASH_BAUD) --file $(BIN)

# ------------------------------------------------------------------------------
# Help
# ------------------------------------------------------------------------------

.PHONY: help
help:
	@echo "ERGO GASALERT - Makefile"
	@echo ""
	@echo "Tinte disponibile:"
	@echo "  make          - compileaza proiectul (produce $(BIN))"
	@echo "  make clean    - sterge fisierele generate"
	@echo "  make flash    - flashuieste firmware prin UART (J4)"
	@echo "  make help     - afiseaza acest mesaj"
	@echo ""
	@echo "Variabile configurabile:"
	@echo "  SDK_PATH      = $(SDK_PATH)"
	@echo "  TOOLCHAIN_PATH= $(TOOLCHAIN_PATH)"
	@echo "  FLASH_PORT    = $(FLASH_PORT)"
	@echo "  FLASH_BAUD    = $(FLASH_BAUD)"
	@echo ""
	@echo "Exemplu cu cai custom:"
	@echo "  make SDK_PATH=/home/user/simcom_sdk TOOLCHAIN_PATH=/usr/bin"
