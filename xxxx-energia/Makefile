PROJ   ?= serial_demo

TARGET ?= msp430
BOARD  ?= msp430
MCU    ?= msp430g2553
FREQ   ?= 16000000L

ENERGIA_ROOT      ?= $(HOME)/src/msp430/energia-0101E0008
ENERGIA_BASE      ?= $(ENERGIA_ROOT)/hardware/msp430
ENERGIA_CORE_PATH ?= $(ENERGIA_BASE)/cores/msp430
ENERGIA_LIB_PATH  ?= $(ENERGIA_BASE)/libraries

#CROSS_ROOT        ?= /usr/cross/msp430
CROSS_ROOT        ?= $(ENERGIA_ROOT)/hardware/tools/msp430
CROSS_TOOL_PATH   ?= $(CROSS_ROOT)/bin

OUT      = .objs
INC      = -I$(CROSS_ROOT)/include -I$(CROSS_ROOT)/msp430/include \
		   $(addprefix -I,$(sort $(dir $(shell find $(ENERGIA_BASE) -name "*.h"))))
OPT      = -g -Os -Wall -ffunction-sections -fdata-sections \
		   -mmcu=$(MCU) -DF_CPU=$(FREQ) -DARDUINO=101 -DENERGIA=8

CFLAGS   = $(OPT) $(INC)
CXXFLAGS = $(OPT) $(INC) -fno-rtti -fno-enforce-eh-specs -fno-exceptions
LDFLAGS  = -Os -Wl,-gc-sections,-u,main -mmcu=$(MCU) -L$(CROSS_ROOT)/lib -L$(OUT)

# add $(CROSS_TOOL_PATH) to path
CC       = $(TARGET)-gcc
CXX      = $(TARGET)-g++
AR       = $(TARGET)-ar
OBJCOPY  = $(TARGET)-objcopy
PROG     = mspdebug

E_CORE      = $(OUT)/libecore-$(MCU).a
E_CORE_SRCS = $(wildcard $(ENERGIA_CORE_PATH)/*.c*)
E_CORE_OBJS = $(addprefix $(OUT)/,$(addsuffix .o,$(basename $(notdir $(E_CORE_SRCS)))))

E_LIB       = $(OUT)/libelib-$(MCU).a
E_LIB_SRCS  = Servo/Servo.cpp
E_LIB_OBJS  = $(OUT)/Servo.o

BIN         = $(PROJ).elf $(PROJ).hex

.SUFFIXES: .cpp .o .elf .hex .ino

.ino.cpp:
	@echo generating $@
	@echo '#include "Energia.h"' > $@
	@cat $< >> $@

.elf.hex:
	@echo generating $@
	@$(OBJCOPY) -O ihex -R .eeprom $< $@

.cpp.o:
	@echo building $< to $@ ...
	@$(CXX) -c $(CXXFLAGS) $< -o $@

$(OUT)/%.o: $(ENERGIA_CORE_PATH)/%.c
	@echo building $(notdir $<) to $@ ...
	@$(CC) -c $(CFLAGS) $< -o $@

$(OUT)/%.o: $(ENERGIA_CORE_PATH)/%.cpp
	@echo building $(notdir $<) to $@ ...
	@$(CXX) -c $(CXXFLAGS) $< -o $@

$(OUT)/%.o: $(ENERGIA_LIB_PATH)/Servo/%.cpp
	@echo building $(notdir $<) to $@ ...
	@$(CXX) -c $(CXXFLAGS) $< -o $@

all: env-check $(BIN)
	@echo 'build finished'

env-check:
	@echo 'checking environment...'
	@if [ -z "$(ENERGIA_ROOT)" -o ! -d "$(ENERGIA_ROOT)" ]; then \
		echo "please setup ENERGIA_ROOT"; \
		exit 1; \
	fi
	@echo "ENERGIA_ROOT = "$(ENERGIA_ROOT)
	@if [ -z `which msp430-g++` ]; then \
		echo "please add "$(CROSS_TOOL_PATH)" to PATH"; \
	fi
	@echo 'found tools for '$(TARGET)
	@if [ "$(PWD)" != "$(abspath $(OUT))" ]; then \
		if [ ! -d "$(OUT)" ]; then \
			mkdir "$(OUT)"; \
			echo 'create tmp dir: '$(OUT); \
		fi; \
	fi

test:
	@echo $(OUT)
	@echo $(abspath $(OUT))
	@echo $(PWD)
	@echo $(ENERGIA_BASE)

$(E_CORE): $(E_CORE_OBJS)
	@$(AR) rcs $@ $^

$(E_LIB): $(E_LIB_OBJS)
	@$(AR) rcs $@ $^

$(PROJ).elf: $(PROJ).o $(E_LIB) $(E_CORE)
	@echo generating $@
	@$(CC) $(LDFLAGS) -o $@ $^ -lm 

flash: $(PROJ).hex
	@$(PROG) rf2500 "prog $<"

clean:
	-@rm -f $(E_CORE_OBJS) $(E_LIB_OBJS) $(E_CORE) $(E_LIB) $(BIN) *.elf *.hex *.o *.a *.so *~
	-@rmdir "$(OUT)"

