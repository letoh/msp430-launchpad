FREQ              ?= 16000000L
VARIANT           ?= launchpad   # launchpad / fraunchpad

SKETCH_SRCS       ?= $(wildcard *.ino) $(wildcard *.pde)
SRCS               = $(SKETCH_SRCS)

ENERGIA_ROOT      ?= $(HOME)/src/msp430/energia-0101E0008
ENERGIA_BASE      ?= $(ENERGIA_ROOT)/hardware/msp430
ENERGIA_CORE_PATH ?= $(ENERGIA_BASE)/cores/msp430
ENERGIA_LIB_PATH  ?= $(ENERGIA_BASE)/libraries
ENERGIA_VAR_PATH  ?= $(ENERGIA_BASE)/variants/$(VARIANT)

CROSS_ROOT        ?= $(ENERGIA_ROOT)/hardware/tools/msp430


IMPORT      ?= #IRremote LiquidCrystal MspFlash SPI Servo Stepper Wire

E_CORE       = $(TMPOBJ)/libenergia_core-$(MCU).a
E_CORE_SRCS  = $(call gen-src-list,$(ENERGIA_CORE_PATH))
E_CORE_OBJS  = $(call gen-obj-list,$(E_CORE_SRCS))

E_EXTRA      = $(if $(IMPORT),$(TMPOBJ)/libenergia_lib-$(MCU).a)
E_EXTRA_DIRS = $(foreach dir,$(IMPORT),$(shell find $(ENERGIA_LIB_PATH) -type d -iname "$(dir)"))
E_EXTRA_SRCS = $(call gen-src-list,$(E_EXTRA_DIRS))
E_EXTRA_OBJS = $(call gen-obj-list,$(E_EXTRA_SRCS))


INC         += $(addprefix -I,$(sort $(dir $(shell find $(ENERGIA_CORE_PATH) -iname "*.h")))) \
				$(addprefix -I,$(sort $(dir $(shell find $(ENERGIA_LIB_PATH) -iname "*.h")))) \
				$(addprefix -I,$(sort $(dir $(shell find $(ENERGIA_VAR_PATH) -iname "*.h"))))
OPT         += -DARDUINO=101 -DENERGIA=8
LIBS        += $(SKETCH_LIBS) \
			   $(if $(E_EXTRA),-lenergia_lib-$(MCU)) $(if $(E_CORE),-lenergia_core-$(MCU)) -lm

.SUFFIXES: .ino .pde

.ino.cpp:
	@echo generating $@
	@echo '#include "Energia.h"' > $@
	@cat $< >> $@

.pde.cpp:
	@echo generating $@
	@echo '#include "Energia.h"' > $@
	@cat $< >> $@

.PHONY: check-energia-root link-ecore-srcs link-elib-srcs e-build-lib before-clean

BEFORE_ALL   = check-energia-root link-ecore-srcs link-elib-srcs e-build-lib
BEFORE_CLEAN = before-clean

include ../make.inc

## private target

check-energia-root:
	@echo 'checking Energia path...'
	@if [ -z "$(ENERGIA_ROOT)" -o ! -d "$(ENERGIA_ROOT)" ]; then \
		echo "please setup ENERGIA_ROOT"; \
		exit 1; \
	fi
	@echo "ENERGIA_ROOT = "$(ENERGIA_ROOT)

e-build-lib: $(E_EXTRA) $(E_CORE)
	@echo "build lib '"$(notdir $^)"' ok"
	@echo

link-ecore-srcs:
	$(call link-src-tmp,$(E_CORE_SRCS))

link-elib-srcs:
	$(call link-src-tmp,$(E_EXTRA_SRCS))

$(E_CORE): $(E_CORE_OBJS)
	@$(AR) rcs $@ $^

$(E_EXTRA): $(E_EXTRA_OBJS)
	@$(AR) rcs $@ $^

before-clean:
	-@if [ -d "$(TMPOBJ)" ]; then \
		rm -f $(E_CORE_OBJS) $(E_EXTRA_OBJS) $(E_CORE) $(E_EXTRA); \
		rm -f $(subst .o,.*,$(E_CORE_OBJS) $(E_EXTRA_OBJS)); \
	fi

