#
# common makefile rules for msp430 project
#
# options fo customization:
#
# PROJECT           project name
# SRCS              source list
# BIN
#
# INC               additional include path
# OPT               additional compile options, for both compiling c and c++ sources
# LIBS              additional linking lib
# CFLAGS
# CXXFLAGS
# LDFLAGS
#
# TARGET
# BOARD
# MCU
# FREQ              clock frequency
#
# CROSS_ROOT
# CROSS_TOOL_PATH   path for cross compiler
# PROG              programming tool
#
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
LIBS        +=

CFLAGS      += $(OPT) $(INC)
CXXFLAGS    += $(OPT) $(INC) -fno-rtti -fno-enforce-eh-specs -fno-exceptions
LDFLAGS     += -Os -Wl,-gc-sections,-u,main -mmcu=$(MCU) -L$(CROSS_ROOT)/lib -L$(TMPOBJ) $(LIBS)

link-src     = $(foreach src,$(1),$(shell ln -f -s "$(abspath $(src))" "$(2)/$(notdir $(src))"))
link-src-tmp = $(call link-src,$(1),$(TMPOBJ))

gen-src-list = $(foreach lib,$(1),$(shell find $(lib) -iname "[!.]*.c" -o -iname "[!.]*.cpp"))
gen-obj-list = $(addprefix $(TMPOBJ)/,$(addsuffix .o,$(basename $(notdir $(1)))))

SRCS        ?= $(call gen-src-list,$(PWD))
OBJS         = $(call gen-obj-list,$(SRCS))
EXTSRCS      = $(foreach src,$(abspath $(SRCS)),$(if $(subst $(PWD)/,,$(dir $(src))),$(src),))


BIN         ?= $(PROJECT).elf $(PROJECT).hex


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


.PHONY: pre-check check-cross-tool-path check-tmp-out link-proj-srcs clean

all: pre-check $(BEFORE_ALL) $(BIN) $(AFTER_ALL)
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

link-proj-srcs:
	$(call link-src-tmp,$(EXTSRCS))

pre-check: check-cross-tool-path check-tmp-out link-proj-srcs
	@echo checking finished
	@echo

$(PROJECT).elf: $(if $(wildcard $(TMPOBJ)),$(OBJS),pre-check $(BEFORE_ALL) $(OBJS) $(AFTER_ALL))
	@echo linking $^ to $@ ...
	@$(CC) -o $@ $(OBJS) $(LDFLAGS)

flash: $(PROJECT).elf
	@echo downloading $< ...
	@mspdebug rf2500 "prog $<"

clean: $(BEFORE_CLEAN)
	@echo clean up
	-@rm -f $(BIN) *.elf *.hex *.o *.bin $(OBJS) *~
	-@if [ -d "$(TMPOBJ)" ]; then \
		rm -f $(subst .o,.*,$(OBJS)); \
		rmdir "$(TMPOBJ)"; \
	fi

