# ============================================================================
# Makefile - ERGO GASALERT v5.0
# Target: STM32C011F4U6TR (Cortex-M0+, 32KB Flash, 6KB RAM)
# GSM:    SIMCom A7682E (AT commands UART)
# Toolchain: arm-none-eabi-gcc
#
# Utilizare:
#   make            - compileaza firmware-ul
#   make clean      - sterge fisierele generate
#   make flash      - programeaza prin ST-Link (openocd)
#   make size       - afiseaza dimensiunile sectiunilor
# ============================================================================

TARGET      = ergo_gasalert_stm32
BUILD_DIR   = build

# ---- Toolchain ----
CC          = arm-none-eabi-gcc
OBJCOPY     = arm-none-eabi-objcopy
SIZE        = arm-none-eabi-size

# ---- Surse ----
SRCS = \
    src/main.c       \
    src/config.c     \
    src/uart.c       \
    src/gsm.c        \
    src/led.c        \
    src/input.c      \
    src/sms.c

# ---- CMSIS STM32C0 ----
# Calea catre STM32CubeC0 CMSIS (ajusteaza dupa instalare)
# Download: https://github.com/STMicroelectronics/STM32CubeC0
CUBE_PATH   ?= /opt/STM32CubeC0

CMSIS_CORE  = $(CUBE_PATH)/Drivers/CMSIS/Include
CMSIS_DEVICE = $(CUBE_PATH)/Drivers/CMSIS/Device/ST/STM32C0xx/Include
STARTUP     = $(CUBE_PATH)/Drivers/CMSIS/Device/ST/STM32C0xx/Source/Templates/gcc/startup_stm32c011xx.s
SYSTEM_C    = $(CUBE_PATH)/Drivers/CMSIS/Device/ST/STM32C0xx/Source/Templates/system_stm32c0xx.c
LINKER      = $(CUBE_PATH)/Drivers/CMSIS/Device/ST/STM32C0xx/Source/Templates/gcc/linker/STM32C011F4UX_FLASH.ld

# Daca linker script-ul nu e in Cube, foloseste cel local
ifneq ("$(wildcard linker/STM32C011F4UX_FLASH.ld)","")
LINKER      = linker/STM32C011F4UX_FLASH.ld
endif

# ---- Include paths ----
INCLUDES    = \
    -I include           \
    -I $(CMSIS_CORE)     \
    -I $(CMSIS_DEVICE)

# ---- Definiri preprocessor ----
DEFINES     = \
    -DSTM32C011xx          \
    -DUSE_FULL_LL_DRIVER   \
    -DHSI_VALUE=48000000UL

# ---- Flaguri compilator ----
CPU_FLAGS   = -mcpu=cortex-m0plus -mthumb

CFLAGS      = $(CPU_FLAGS)             \
              -Os                      \
              -std=c99                 \
              -Wall -Wextra            \
              -ffunction-sections      \
              -fdata-sections          \
              -ffreestanding           \
              $(INCLUDES)              \
              $(DEFINES)

# ---- Flaguri linker ----
LDFLAGS     = $(CPU_FLAGS)             \
              -T $(LINKER)             \
              -Wl,--gc-sections        \
              -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
              -nostdlib                \
              -lc -lgcc

# ---- Obiect files ----
OBJS        = $(SRCS:src/%.c=$(BUILD_DIR)/%.o)
OBJS        += $(BUILD_DIR)/startup.o
OBJS        += $(BUILD_DIR)/system.o

# ============================================================================
# REGULI BUILD
# ============================================================================

.PHONY: all clean flash size

all: $(BUILD_DIR)/$(TARGET).bin
	@echo ""
	@echo "=== BUILD COMPLET: $(TARGET).bin ==="
	@$(SIZE) $(BUILD_DIR)/$(TARGET).elf

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) | $(BUILD_DIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# Surse C
$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Startup assembler
$(BUILD_DIR)/startup.o: $(STARTUP) | $(BUILD_DIR)
	$(CC) $(CPU_FLAGS) -c $< -o $@

# System init C
$(BUILD_DIR)/system.o: $(SYSTEM_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ============================================================================
# FLASH (prin OpenOCD + ST-Link V2)
# ============================================================================

flash: $(BUILD_DIR)/$(TARGET).bin
	openocd \
	    -f interface/stlink.cfg \
	    -f target/stm32c0x.cfg  \
	    -c "program $(BUILD_DIR)/$(TARGET).hex verify reset exit"

# ============================================================================
# SIZE
# ============================================================================

size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) -A $@
	@echo ""
	@echo "Flash disponibil: 32768 bytes (30720 bytes cod + 2048 bytes config Flash)"
	@echo "RAM disponibila:   6144 bytes"

# ============================================================================
# CLEAN
# ============================================================================

clean:
	rm -rf $(BUILD_DIR)
