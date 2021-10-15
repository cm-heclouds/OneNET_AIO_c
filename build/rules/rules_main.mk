CONFIG_FILE := $(TOP_DIR)/.config
CUR_DIR := $(shell pwd)
ifeq ($(TOP_DIR), $(CUR_DIR))
OBJ_DIR := $(OUTPUT_OBJS_DIR)
else
OBJ_DIR := $(OUTPUT_OBJS_DIR)/$(strip $(subst $(TOP_DIR)/,, $(CUR_DIR)))
endif
target := $(OBJ_DIR)/built-in.o
obj-y :=

sinclude $(CONFIG_FILE)
sinclude $(CUR_DIR)/build.mk

ifneq (build.mk, $(wildcard build.mk))
obj-y += $(patsubst %.c, %.o, $(wildcard *.c))
# $(info "obj-y :  $(obj-y)")
endif

# contains Objs(Like *.o or path/*.o) and Sub_dirs(Like path/)
obj-y := $(sort $(obj-y))

# List Sub-dirs
sub_dirs := $(strip $(patsubst %/, %, $(filter %/, $(obj-y))))

src_files := $(patsubst %.o, %.c, $(filter %.o, $(obj-y)))
obj-files := $(patsubst %,$(OBJ_DIR)/%/built-in.o,$(sub_dirs)) $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(src_files)))

build-objs: $(target)

sinclude $(patsubst %.c, $(OBJ_DIR)/%.d, $(notdir $(src_files)))


$(target): $(obj-files) 
	$(LD) $(LDFLAGS) -r $^ -o $@

ifneq ($(sub_dirs), )
$(patsubst %, $(OBJ_DIR)/%/built-in.o, $(sub_dirs)) : $(OBJ_DIR)/%/built-in.o : % PHONY
	@mkdir -p $(OBJ_DIR)/$<
	$(MAKE) -f $(TOP_DIR)/build/rules/rules_main.mk -C $<
endif


ifneq ($(src_files),)
define build_src_file
$(OBJ_DIR)/$(patsubst %.c,%.o,$(notdir $(1))) : $(CUR_DIR)/$(1)
	$(CC) $(CFLAGS) $(INCLUDES) -c $$< -o $$@

$(OBJ_DIR)/$(patsubst %.c,%.d,$(notdir $(1))) : $(CUR_DIR)/$(1)
	@set -e;rm -f $$@;\
	$(CC) -MM $(CFLAGS) $(INCLUDES) $$< > $$@.$$$$$$$$;\
	sed 's,\($$*\)\.o[ :],\1.o $$@ :,g;1 s,^,$(OBJ_DIR)/,g' < $$@.$$$$$$$$ > $$@;\
	rm -f $$@.$$$$$$$$

endef

$(foreach n, $(src_files), $(eval $(call build_src_file,$(n))))
endif

.PHONY : PHONY
	