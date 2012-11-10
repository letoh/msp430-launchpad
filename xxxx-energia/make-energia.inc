PWD               ?= `pwd`
PROJECT           ?= $(lastword $(subst /, ,$(PWD)))

TARGET            ?= msp430
BOARD             ?= launchpad
MCU               ?= msp430g2553
FREQ              ?= 16000000L

SKETCH_SRCS       ?= $(wildcard *.ino)
SKETCH_OBJS        = $(subst .ino,.o,$(SKETCH_SRCS))

ENERGIA_ROOT      ?= $(HOME)/src/msp430/energia-0101E0008
ENERGIA_BASE      ?= $(ENERGIA_ROOT)/hardware/msp430
ENERGIA_CORE_PATH ?= $(ENERGIA_BASE)/cores/msp430
ENERGIA_LIB_PATH  ?= $(ENERGIA_BASE)/libraries

CROSS_ROOT        ?= $(ENERGIA_ROOT)/hardware/tools/msp430
CROSS_TOOL_PATH   ?= $(CROSS_ROOT)/bin

# add $(CROSS_TOOL_PATH) to path
CC           = $(TARGET)-gcc
CXX          = $(TARGET)-g++
AR           = $(TARGET)-ar
OBJCOPY      = $(TARGET)-objcopy
PROG         = mspdebug


TMPOBJ       = .tmp
INC         += -I$(CROSS_ROOT)/include -I$(CROSS_ROOT)/msp430/include \
		       $(addprefix -I,$(sort $(dir $(shell find $(ENERGIA_BASE) -iname "*.h"))))
OPT         += -g -Os -Wall -ffunction-sections -fdata-sections \
		       -mmcu=$(MCU) -DF_CPU=$(FREQ) -DARDUINO=101 -DENERGIA=8

CFLAGS      += $(OPT) $(INC)
CXXFLAGS    += $(OPT) $(INC) -fno-rtti -fno-enforce-eh-specs -fno-exceptions
LDFLAGS     += -Os -Wl,-gc-sections,-u,main -mmcu=$(MCU) -L$(CROSS_ROOT)/lib -L$(TMPOBJ)

IMPORT      ?= #Servo SPI

gen-src-list = $(foreach lib,$(1),$(shell find $(lib) -iname "*.c*"))
gen-obj-list = $(addprefix $(TMPOBJ)/,$(addsuffix .o,$(basename $(notdir $(1)))))

E_CORE       = $(TMPOBJ)/libenergia_core-$(MCU).a
E_CORE_SRCS  = $(call gen-src-list,$(ENERGIA_CORE_PATH))
E_CORE_OBJS  = $(call gen-obj-list,$(E_CORE_SRCS))

E_EXTRA      = $(if $(IMPORT),$(TMPOBJ)/libenergia_lib-$(MCU).a)
E_EXTRA_DIRS = $(foreach dir,$(IMPORT),$(shell find $(ENERGIA_LIB_PATH) -type d -iname "$(dir)"))
E_EXTRA_SRCS = $(call gen-src-list,$(E_EXTRA_DIRS))
E_EXTRA_OBJS = $(call gen-obj-list,$(E_EXTRA_SRCS))


BIN          = $(PROJECT).elf $(PROJECT).hex

.SUFFIXES: .c .cpp .o .elf .hex .ino

.ino.cpp:
	@echo generating $@
	@echo '#include "Energia.h"' > $@
	@cat $< >> $@

.elf.hex:
	@echo generating $@
	@$(OBJCOPY) -O ihex -R .eeprom $< $@

.c.o:
	@echo building $(notdir $<) to $@ ...
	@$(CC) -c $(CFLAGS) $< -o $@

.cpp.o:
	@echo building $(notdir $<) to $@ ...
	@$(CXX) -c $(CXXFLAGS) $< -o $@


all: pre-check $(BIN)
	@echo 'build finished'

## private target

check-energia-root:
	@echo 'checking environment...'
	@if [ -z "$(ENERGIA_ROOT)" -o ! -d "$(ENERGIA_ROOT)" ]; then \
		echo "please setup ENERGIA_ROOT"; \
		exit 1; \
	fi
	@echo "ENERGIA_ROOT = "$(ENERGIA_ROOT)

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

link-src     = $(foreach src,$(1),$(shell ln -s "$(src)" "$(2)/$(notdir $(src))"))
link-src-tmp = $(call link-src,$(1),$(TMPOBJ))

link-ecore:
	$(call link-src-tmp,$(E_CORE_SRCS))

link-elib:
	$(call link-src-tmp,$(E_EXTRA_SRCS))

pre-check: check-energia-root check-cross-tool-path check-tmp-out link-ecore link-elib
	@echo checking finished
	@echo

$(E_CORE): $(E_CORE_OBJS)
	@$(AR) rcs $@ $^

$(E_EXTRA): $(E_EXTRA_OBJS)
	@$(AR) rcs $@ $^

$(PROJECT).elf: $(SKETCH_OBJS) $(E_EXTRA) $(E_CORE)
	@echo generating $@
	@$(CC) $(LDFLAGS) -o $@ $^ -lm 

flash: $(PROJECT).elf
	@$(PROG) rf2500 "prog $<"

clean:
	-@rm -f $(BIN) *.elf *.hex *.o *.a *.so *~
	-@if [ -d "$(TMPOBJ)" ]; then \
		rm -f $(E_CORE_OBJS) $(E_EXTRA_OBJS) $(E_CORE) $(E_EXTRA); \
		rm -f $(subst .o,.*,$(E_CORE_OBJS) $(E_EXTRA_OBJS)); \
		rmdir "$(TMPOBJ)"; \
	fi

