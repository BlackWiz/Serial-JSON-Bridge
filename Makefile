# Part 1: VARIABLES
#----------------------------------------------------
TARGET = firmware
CC = arm-none-eabi-gcc
OBJDUMP = arm-none-eabi-objdump

# Default sources for production build (JSON parser)
SRCS = main.c syscalls.c startup.c jsonprocess.c jsmn.c uart.c delay.c

# Automatically create lists of derived files
OBJS = $(SRCS:.c=.o)
PREPROCESSED = $(SRCS:.c=.i)
ASSEMBLY = $(SRCS:.c=.s)

# Part 2: Compiler and Linker Flags
#----------------------------------------------------
CPU = -mcpu=cortex-m0plus
MCU = $(CPU) -mthumb

# C flags
CFLAGS = -Wall -g -std=c11
CFLAGS += -ffunction-sections -fdata-sections

# Linker flags
LDFLAGS = -T Linker.ld
LDFLAGS += -Wl,-Map=$(TARGET).map
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -nostartfiles
LDFLAGS += --specs=nosys.specs

# Part 3: RULES
#----------------------------------------------------
# The default goal: build the final .elf file (production)
all: $(TARGET).elf

# Manual testing build
test-manual:
	$(MAKE) clean
	$(MAKE) SRCS="test.c startup.c uart.c delay.c" TARGET=test_manual all
	$(MAKE) TARGET=test_manual flash

# Unit testing build
test-unit:
	$(MAKE) clean
	$(MAKE) SRCS="uart_unit_test.c startup.c uart.c delay.c" TARGET=uart_unit_test all
	$(MAKE) TARGET=uart_unit_test flash

# Integration testing build
test-integration:
	$(MAKE) clean
	$(MAKE) SRCS="uart_integration_test.c startup.c uart.c delay.c" TARGET=uart_integration_test all
	$(MAKE) TARGET=uart_integration_test flash

# Auto TDD testing build
test-autoTDD:
	$(MAKE) clean
	$(MAKE) SRCS="uart_test.c startup.c uart.c delay.c" TARGET=uart_test all
	$(MAKE) TARGET=uart_test flash

# Rule to link all object files (.o) into the final executable (.elf)
$(TARGET).elf: $(OBJS)
	$(CC) $(MCU) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Generic rule to compile any .c file into a .o file
%.o: %.c
	$(CC) $(MCU) $(CFLAGS) -c -o $@ $<

# Generic rule to generate a preprocessed file (.i) from a .c file
%.i: %.c
	$(CC) $(MCU) $(CFLAGS) -E -o $@ $<

# Rule to generate an assembly file (.s) from a .c file
%.s: %.c
	$(CC) $(MCU) $(CFLAGS) -S -o $@ $<

# Part 4: TARGETS (The commands you can run)
#----------------------------------------------------
# Flash the program to the chip
flash: 
	openocd -f interface/stlink.cfg -f target/stm32g0x.cfg -c "program $(TARGET).elf verify reset exit"
	
# Disassemble the final .elf file into a .asm file
disasm: $(TARGET).elf
	$(OBJDUMP) -d -S $< > $(TARGET).asm

# Generate .s assembly files for all C sources
assembly: $(ASSEMBLY)

# Compile only, without linking
compile: $(OBJS)

# Generate all preprocessed files
preprocess: $(PREPROCESSED)

# Clean up all generated files
clean:
	rm -f *.elf *.o *.i *.s *.map *.asm