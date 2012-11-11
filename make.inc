#
# common makefile rules for msp430 project
#
SHELL              = /bin/sh
PWD               ?= `pwd`
PROJECT           ?= $(lastword $(subst /, ,$(PWD)))

TARGET            ?= msp430
BOARD             ?= launchpad
MCU               ?= msp430g2553
FREQ              ?= 16000000L

CROSS_ROOT        ?= /usr/cross/msp430
CROSS_TOOL_PATH   ?= $(CROSS_ROOT)/bin

# add $(CROSS_TOOL_PATH) to path
CC           = $(TARGET)-gcc
CXX          = $(TARGET)-g++
AR           = $(TARGET)-ar
OBJCOPY      = $(TARGET)-objcopy
PROG        ?= mspdebug


TMPOBJ       = .tmp
INC         += -I$(CROSS_ROOT)/include -I$(CROSS_ROOT)/msp430/include
OPT         += -mmcu=$(MCU) -Os -DF_CPU=$(FREQ) -Wall -ffunction-sections -fdata-sections

CFLAGS      += $(OPT) $(INC)
CXXFLAGS    += $(OPT) $(INC) -fno-rtti -fno-enforce-eh-specs -fno-exceptions
LDFLAGS     += -Os -Wl,-gc-sections,-u,main -mmcu=$(MCU) -L$(CROSS_ROOT)/lib -L$(TMPOBJ)

gen-src-list = $(foreach lib,$(1),$(shell find $(lib) -iname "*.c*"))
gen-obj-list = $(addprefix $(TMPOBJ)/,$(addsuffix .o,$(basename $(notdir $(1)))))

SRCS        ?= $(call gen-src-list,$(PWD))
OBJS         = $(call gen-obj-list,$(SRCS))


BIN         ?= $(PROJECT).elf $(PROJECT).hex

.SUFFIXES:
.SUFFIXES: .c .cpp .o .elf .hex .bin

.c.o:
	@echo building $(notdir $<) to $@ ...
	@$(CC) -c $(CFLAGS) $< -o $@

.cpp.o:
	@echo building $(notdir $<) to $@ ...
	@$(CXX) -c $(CXXFLAGS) $< -o $@

$(TMPOBJ)/%.o: %.c
	@echo building $(notdir $<) to $@ ...
	@$(CC) -c $(CFLAGS) $< -o $@

$(TMPOBJ)/%.o: %.cpp
	@echo building $(notdir $<) to $@ ...
	@$(CXX) -c $(CXXFLAGS) $< -o $@

.elf.hex:
	@echo generating $@
	@$(OBJCOPY) -O ihex -R .eeprom $< $@


.PHONY: pre-check check-cross-tool-path check-tmp-out clean

all: pre-check $(BIN)
	@echo 'build finished'

check-cross-tool-path:
	@if [ -z `which msp430-g++` ]; then \
		echo "please add "$(CROSS_TOOL_PATH)" to PATH"; \
		exit 1; \
	fi
	@echo 'found tools for '$(TARGET)

check-tmp-out:
	@if [ "$(PWD)" != "$(abspath $(TMPOBJ))" ]; then \
		if [ ! -d "$(TMPOBJ)" ]; then \
			mkdir "$(TMPOBJ)"; \
			echo 'create tmp dir: '$(TMPOBJ); \
		fi; \
	fi

pre-check: check-cross-tool-path check-tmp-out $(PRECHECK)
	@echo checking finished
	@echo

$(PROJECT).elf: $(OBJS)
	@echo linking $^ to $@ ...
	@$(CC) $(LDFLAGS) -o $@ $^

flash: $(PROJECT).elf
	@echo downloading $< ...
	mspdebug rf2500 "prog $<"

clean:
	@echo clean up
	@rm -f *.elf *.hex *.o *.bin $(OBJS) *~
	-@if [ -d "$(TMPOBJ)" ]; then \
		rmdir "$(TMPOBJ)"; \
	fi

