# ==============================================================================
# Makefile - ERGO GASALERT
# Firmware STM32C011F4U6TR + SIMCom A7682E (AT commands via UART)
#
# NOTA: Caile STM32CUBE_PATH si TOOLCHAIN_PATH trebuie adaptate la instalarea locala.
#
# Utilizare:
#   make          - compileaza proiectul
#   make clean    - sterge fisierele generate
#   make flash    - flashuieste firmware prin SWD (ST-Link)
#   make help     - afiseaza acest mesaj
# ==============================================================================

# ------------------------------------------------------------------------------
# Configurare cai (ADAPTEAZA la instalarea ta)
# ------------------------------------------------------------------------------

# Calea catre STM32CubeC0 HAL (STM32Cube_FW_C0)
STM32CUBE_PATH ?= /opt/st/STM32Cube_FW_C0

# Calea catre ARM GCC Toolchain (bare-metal)
TOOLCHAIN_PATH ?= /opt/arm-gcc/bin

# ------------------------------------------------------------------------------
# Toolchain ARM GCC (cross-compiler pentru Cortex-M0+)
# ------------------------------------------------------------------------------

PREFIX  = $(TOOLCHAIN_PATH)/arm-none-eabi-
CC      = $(PREFIX)gcc
LD      = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size

# ------------------------------------------------------------------------------
# Numele proiectului si fisierele de iesire
# ------------------------------------------------------------------------------

PROJECT = ergo_gasalert
ELF     = $(PROJECT).elf
BIN     = $(PROJECT).bin
HEX     = $(PROJECT).hex
MAP     = $(PROJECT).map

# ------------------------------------------------------------------------------
# Fisiere sursa
# ------------------------------------------------------------------------------

SRCS = \
    src/main.c      \
    src/config.c    \
    src/gsm.c       \
    src/sms.c       \
    src/input.c     \
    src/led.c       \
    src/network.c

# HAL sources (minim necesar)
HAL_SRC = $(STM32CUBE_PATH)/Drivers/STM32C0xx_HAL_Driver/Src
HAL_SRCS = \
    $(HAL_SRC)/stm32c0xx_hal.c              \
    $(HAL_SRC)/stm32c0xx_hal_cortex.c       \
    $(HAL_SRC)/stm32c0xx_hal_rcc.c          \
    $(HAL_SRC)/stm32c0xx_hal_gpio.c         \
    $(HAL_SRC)/stm32c0xx_hal_uart.c         \
    $(HAL_SRC)/stm32c0xx_hal_flash.c        \
    $(HAL_SRC)/stm32c0xx_hal_flash_ex.c     \
    $(HAL_SRC)/stm32c0xx_hal_iwdg.c

# Startup si system (CMSIS)
STARTUP = $(STM32CUBE_PATH)/Drivers/CMSIS/Device/ST/STM32C0xx/Source/Templates/gcc/startup_stm32c011xx.s
SYSTEM  = $(STM32CUBE_PATH)/Drivers/CMSIS/Device/ST/STM32C0xx/Source/Templates/system_stm32c0xx.c

# Fisiere obiect (generate in directorul build/)
BUILD_DIR = build
OBJS  = $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(SRCS))
OBJS += $(patsubst $(HAL_SRC)/%.c, $(BUILD_DIR)/hal_%.o, $(HAL_SRCS))
OBJS += $(BUILD_DIR)/system_stm32c0xx.o
OBJS += $(BUILD_DIR)/startup_stm32c011xx.o

# ------------------------------------------------------------------------------
# Include paths
# ------------------------------------------------------------------------------

INCLUDES = \
    -I./include \
    -I$(STM32CUBE_PATH)/Drivers/STM32C0xx_HAL_Driver/Inc \
    -I$(STM32CUBE_PATH)/Drivers/CMSIS/Device/ST/STM32C0xx/Include \
    -I$(STM32CUBE_PATH)/Drivers/CMSIS/Include

# ------------------------------------------------------------------------------
# Flaguri compilare C
# ------------------------------------------------------------------------------

# Arhitectura: ARM Cortex-M0+, thumb mode (M0+ nu are ARM mode)
ARCH_FLAGS = \
    -mcpu=cortex-m0plus  \
    -mthumb              \
    -mfloat-abi=soft

# Optimizare si standard C
OPT_FLAGS = \
    -Os                  \
    -std=c11             \
    -ffunction-sections  \
    -fdata-sections

# Avertismente
WARN_FLAGS = \
    -Wall           \
    -Wextra         \
    -Wno-unused-parameter

# Definitii preprocessor
DEFINES = \
    -DSTM32C011xx        \
    -DUSE_HAL_DRIVER

# Debug UART (decomentati linia de mai jos pentru a activa UART2 debug pe PB6)
# DEFINES += -DDEBUG_UART_ENABLE

CFLAGS = $(ARCH_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(DEFINES) $(INCLUDES)

# Flaguri assembler (pentru startup)
ASFLAGS = $(ARCH_FLAGS) -x assembler-with-cpp $(DEFINES)

# ------------------------------------------------------------------------------
# Linker
# ------------------------------------------------------------------------------

# Script linker STM32C011F4: 16KB Flash, 6KB RAM
LDSCRIPT = STM32C011F4Ux_FLASH.ld

LDFLAGS = \
    $(ARCH_FLAGS)                \
    -T$(LDSCRIPT)                \
    -Wl,-Map=$(MAP)              \
    -Wl,--gc-sections            \
    -specs=nano.specs            \
    -specs=nosys.specs           \
    -lc                          \
    -lm                          \
    -lnosys

# ------------------------------------------------------------------------------
# Reguli build
# ------------------------------------------------------------------------------

.PHONY: all
all: $(BUILD_DIR) $(BIN) $(HEX)
	@echo ""
	@echo ">>> Build complet: $(BIN)"
	@$(SIZE) $(ELF)
	@echo ""
	@echo "ATENTIE: Verifica sectiunea .text sa NU depaseasca 0x3800 (14KB)!"

# Creare director build
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compilare fisiere sursa aplicatie
$(BUILD_DIR)/%.o: src/%.c
	@echo "  CC  $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compilare fisiere HAL
$(BUILD_DIR)/hal_%.o: $(HAL_SRC)/%.c
	@echo "  CC  $< (HAL)"
	$(CC) $(CFLAGS) -c $< -o $@

# Compilare system CMSIS
$(BUILD_DIR)/system_stm32c0xx.o: $(SYSTEM)
	@echo "  CC  $< (CMSIS)"
	$(CC) $(CFLAGS) -c $< -o $@

# Compilare startup assembler
$(BUILD_DIR)/startup_stm32c011xx.o: $(STARTUP)
	@echo "  AS  $<"
	$(CC) $(ASFLAGS) -c $< -o $@

# Linkare .o -> .elf
$(ELF): $(OBJS)
	@echo "  LD  $@"
	$(LD) $(OBJS) $(LDFLAGS) -o $@

# Conversie .elf -> .bin (pentru flashuire)
$(BIN): $(ELF)
	@echo "  BIN $@"
	$(OBJCOPY) -O binary $< $@

# Conversie .elf -> .hex (pentru ST-Link)
$(HEX): $(ELF)
	@echo "  HEX $@"
	$(OBJCOPY) -O ihex $< $@

# ------------------------------------------------------------------------------
# Curatare
# ------------------------------------------------------------------------------

.PHONY: clean
clean:
	@echo "Stergere fisiere generate..."
	rm -rf $(BUILD_DIR) $(ELF) $(BIN) $(HEX) $(MAP)

# ------------------------------------------------------------------------------
# Flashuire prin SWD (ST-Link) - necesita st-flash sau STM32_Programmer_CLI
# ------------------------------------------------------------------------------

.PHONY: flash
flash: $(BIN)
	@echo "Flashuire $(BIN) prin SWD..."
	st-flash write $(BIN) 0x08000000

.PHONY: flash-hex
flash-hex: $(HEX)
	@echo "Flashuire $(HEX) prin SWD..."
	STM32_Programmer_CLI -c port=SWD -w $(HEX) -v -rst

# ------------------------------------------------------------------------------
# Help
# ------------------------------------------------------------------------------

.PHONY: help
help:
	@echo "ERGO GASALERT - Makefile (STM32C011F4U6TR + A7682E)"
	@echo ""
	@echo "Tinte disponibile:"
	@echo "  make            - compileaza proiectul (produce $(BIN) si $(HEX))"
	@echo "  make clean      - sterge fisierele generate"
	@echo "  make flash      - flashuieste firmware prin SWD (st-flash)"
	@echo "  make flash-hex  - flashuieste firmware prin SWD (STM32_Programmer_CLI)"
	@echo "  make help       - afiseaza acest mesaj"
	@echo ""
	@echo "Variabile configurabile:"
	@echo "  STM32CUBE_PATH  = $(STM32CUBE_PATH)"
	@echo "  TOOLCHAIN_PATH  = $(TOOLCHAIN_PATH)"
	@echo ""
	@echo "Activare debug UART (PB6):"
	@echo "  Decomentati '-DDEBUG_UART_ENABLE' in DEFINES din Makefile"
