#
# common makefile rules for msp430 project
#
MCU ?= msp430g2553

TARGET ?= msp430-
CC = $(TARGET)gcc
OBJCOPY = $(TARGET)objcopy

CFLAGS += -mmcu=$(MCU) -I/usr/cross/msp430/include -I/usr/cross/msp430/msp430/include -Os
LDFLAGS += -mmcu=$(MCU) -L/usr/cross/msp430/lib

PROJECT ?= main
SRCS    ?= main.c
OBJS    ?= main.o

.SUFFIXES: .c .o .elf .hex .bin

.c.o:
	@echo building $< to $@ ...
	@$(CC) -c $(CFLAGS) $< -o $@

.elf.hex:
	@echo generating $@
	@$(OBJCOPY) -O ihex $< $@

all: $(PROJECT).elf $(PROJECT).hex

$(PROJECT).elf: $(OBJS)
	@echo linking $^ to $@ ...
	@$(CC) $(LDFLAGS) -o $@ $^

flash: $(PROJECT).elf
	@echo downloading $< ...
	mspdebug rf2500 "prog $<"

clean:
	@echo clean up
	@rm -f *.elf *.hex *.o *.bin *~

