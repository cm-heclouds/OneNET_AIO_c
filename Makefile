CROSS_COMPILE ?= 
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
AR := $(CROSS_COMPILE)ar

TOP_DIR := $(patsubst %/, %, $(shell pwd))

OUTPUT_DIR := $(TOP_DIR)/.output
OUTPUT_LIBS_DIR := $(OUTPUT_DIR)/libs
OUTPUT_OBJS_DIR := $(OUTPUT_DIR)/objs

all: build-target

ifeq ($(MAKECMDGOALS), export)
include $(TOP_DIR)/build/rules/rules_export.mk
else
include $(TOP_DIR)/build/rules/rules_main.mk
endif

include $(TOP_DIR)/build/rules/rules_include.mk

inc-y := $(sort $(inc-y))
inc-sub-y := $(sort $(inc-sub-y))

INCLUDES := -I$(TOP_DIR) $(patsubst %, -I$(TOP_DIR)/%, $(inc-y))
CFLAGS := $(sort $(subst ", , $(CONFIG_C_FLAGS)))
LDFLAGS := $(sort $(subst ", , $(CONFIG_LD_FLAGS)))
LIBS := $(sort $(subst ", , $(CONFIG_LINK_LIBS)))

export CC
export LD
export AR
export TOP_DIR
export OUTPUT_OBJS_DIR
export INCLUDES
export CFLAGS
export LDFLAGS

target-list :=
target-list += build-objs
target-list += build-lib
ifeq ($(CONFIG_BUILD_EXAMPLE), y)
target-list += build-binary
endif
build-target: $(target-list) 

build-lib: $(filter-out %/examples/built-in.o, $(obj-files))
	@mkdir -p $(OUTPUT_LIBS_DIR)
	$(AR) -cr $(OUTPUT_LIBS_DIR)/libOneNET.a $^

build-binary: $(obj-files)
	$(CC) $(CFLAGS) -o onenet_aio $^ $(LIBS)

export: srcs_prefix export-srcs
	@echo -e "\nInclude Files and Paths:\n-----------------------------------------------------"
	@echo "config.h"
	@for inc in $(inc-y);								\
	do 													\
		echo $$inc;										\
	done

srcs_prefix:
	@echo -e "Source Files:\n-----------------------------------------------------"

.PHONY: all clean export srcs_prefix
clean:
	-@rm -rf $(OUTPUT_DIR)
	-@rm -f onenet_aio