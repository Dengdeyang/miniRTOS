######################################
# project target name
######################################
TARGET = VScode_Test

#######################################
# build paths
#######################################
# build paths
BUILD_DIR = BUILD
# release paths
RELEASE_DIR = RELEASE
# debug paths
DEBUG_DIR = DEBUG

#######################################
# basic source
#######################################
# C sources
C_SOURCES =  \
miniRTOS/Driver/Src/device.c \
miniRTOS/Kernel/Src/event.c \
miniRTOS/Kernel/Src/kernel.c \
miniRTOS/Kernel/Src/list.c \
miniRTOS/Kernel/Src/semaphore.c \
miniRTOS/Kernel/Src/soft_timer.c \
miniRTOS/Memory/Src/heap.c \
miniRTOS/Port/Src/miniRTOSport.c \
miniRTOS/Service/Src/crc.c \
miniRTOS/Service/Src/mini_libc.c \
System/main.c \
System/stm32f10x_it.c \
System/system_stm32f10x.c \
System/led.c \
System/uart.c \
HAL/Src/misc.c \
HAL/Src/stm32f10x_adc.c \
HAL/Src/stm32f10x_bkp.c \
HAL/Src/stm32f10x_can.c \
HAL/Src/stm32f10x_cec.c \
HAL/Src/stm32f10x_crc.c \
HAL/Src/stm32f10x_dac.c \
HAL/Src/stm32f10x_dbgmcu.c \
HAL/Src/stm32f10x_dma.c \
HAL/Src/stm32f10x_exti.c \
HAL/Src/stm32f10x_flash.c \
HAL/Src/stm32f10x_fsmc.c \
HAL/Src/stm32f10x_gpio.c \
HAL/Src/stm32f10x_i2c.c \
HAL/Src/stm32f10x_iwdg.c \
HAL/Src/stm32f10x_pwr.c \
HAL/Src/stm32f10x_rcc.c \
HAL/Src/stm32f10x_rtc.c \
HAL/Src/stm32f10x_sdio.c \
HAL/Src/stm32f10x_spi.c \
HAL/Src/stm32f10x_tim.c \
HAL/Src/stm32f10x_usart.c \
HAL/Src/stm32f10x_wwdg.c \
CMSIS/core/core_cm3.c

# ASM sources
ASM_SOURCES =  \
startup_stm32f103xb.s \
miniRTOS/Port/Src/GCC/miniRTOSswitch.s

#######################################
# include
#######################################
# AS includes
AS_INCLUDES = 

# C includes
C_INCLUDES =  \
-ICMSIS/core \
-IHAL/Inc \
-ISystem \
-IminiRTOS/Driver/Inc \
-IminiRTOS/Kernel/Inc \
-IminiRTOS/Memory/Inc \
-IminiRTOS/Port/Inc \
-IminiRTOS/Service/Inc 
#######################################
# define
#######################################
# AS defines
AS_DEFS = 

# C defines
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32F103xB

######################################
# building config
######################################
# debug build?
DEBUG = 1
# optimization
OPT = -O0

######################################
# link config
######################################
# link libraries
LIBDIR = 
LIBS = -lc -lm -lnosys 
#LDLIBS         += /Users/ch-yanghl/gcc-arm-none-eabi/arm-none-eabi/lib/thumb/libc.a
#LDLIBS         += /Users/ch-yanghl/gcc-arm-none-eabi/arm-none-eabi/lib/thumb/libg.a
#LDLIBS         += /Users/ch-yanghl/gcc-arm-none-eabi/arm-none-eabi/lib/thumb/libm.a
#LDLIBS      += /Users/ch-yanghl/gcc-arm-none-eabi/lib/gcc/arm-none-eabi/5.4.1/libgcc.a
#LDLIBS      += /Users/ch-yanghl/gcc-arm-none-eabi/arm-none-eabi/lib/thumb/libnosys.a

# link script
LDSCRIPT = STM32F103C8Tx_FLASH.ld

################################################################################################################

#######################################
# gcc config
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
RE = $(GCC_PATH)/$(PREFIX)readelf #read elf file
LS = $(GCC_PATH)/$(PREFIX)objdump
ST = $(GCC_PATH)/$(PREFIX)nm
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
RE = $(PREFIX)readelf
LS = $(PREFIX)objdump
ST = $(PREFIX)nm
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m3

# fpu
# NONE for Cortex-M0/M0+/M3

# float-abi

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"


#######################################
# LDFLAGS
#######################################
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

#######################################
# DEBUGFLAGS
#######################################
SYMTABFLAGS =
READELFFLAGS = -a
LISTFLAGS = -S

#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
	@cp $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin $(RELEASE_DIR)
ifeq ($(DEBUG), 1)
	@-rm -fR $(DEBUG_DIR)
	@-mkdir $(DEBUG_DIR)
	@cp $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).map $(DEBUG_DIR)
	@$(ST) $(SYMTABFLAGS) $(DEBUG_DIR)/$(TARGET).elf> $(DEBUG_DIR)/$(TARGET).symtab
	@$(RE) $(READELFFLAGS) $(DEBUG_DIR)/$(TARGET).elf> $(DEBUG_DIR)/$(TARGET).elfinfo
	@$(LS) $(LISTFLAGS) $(DEBUG_DIR)/$(TARGET).elf> $(DEBUG_DIR)/$(TARGET).lst
endif
	@echo "build done"
    
#generate .o file   
$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@
 
$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

#generate .elf file 
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

#generate .lst file 
$(BUILD_DIR)/%.lst: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	arm-none-eabi-objdump -D $< $@

#generate .hex file 
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

#generate .bin file 
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	
	
$(BUILD_DIR):
	mkdir $@
	mkdir $(RELEASE_DIR)
	
#######################################
# printf build source
#######################################
C_SOURCES_LIST = $(C_SOURCES:.c=.c\n)
ASM_SOURCES_LIST = $(ASM_SOURCES:.s=.s\n)

printf:
	@echo "*******the source file have been built********"
	@echo -e $(C_SOURCES_LIST)
	@echo -e $(ASM_SOURCES_LIST) 

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR) $(RELEASE_DIR) $(DEBUG_DIR)
	@echo "clean done"
  
#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

# *** EOF ***