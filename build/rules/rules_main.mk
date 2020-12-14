CONFIG_FILE := $(TOP_DIR)/.config
CUR_DIR := $(shell pwd)
ifeq ($(TOP_DIR), $(CUR_DIR))
OBJ_DIR := $(OUTPUT_OBJS_DIR)
else
OBJ_DIR := $(OUTPUT_OBJS_DIR)/$(strip $(subst $(TOP_DIR)/,, $(CUR_DIR)))
endif
target := $(OBJ_DIR)/built-in.o
obj-y :=

-include $(CONFIG_FILE)
-include $(CUR_DIR)/build.mk

ifneq (build.mk, $(wildcard build.mk))
obj-y += $(patsubst %.c, %.o, $(wildcard *.c))
# $(info "obj-y :  $(obj-y)")
endif

# contains Objs(Like *.o or path/*.o) and Sub-dirs(Like path/)
obj-y := $(sort $(obj-y))

# List Sub-dirs
sub-dirs := $(strip $(patsubst %/, %, $(filter %/, $(obj-y))))

# List Objs needed: sub-dir-path/built-in.o, *.o and path/*.o
obj-files := 
ifneq ($(sub-dirs), )
	obj-files += $(foreach n, $(sub-dirs), $(OBJ_DIR)/$(n)/built-in.o)
endif
obj-files += $(patsubst %.o, $(OBJ_DIR)/%.o, $(notdir $(filter %.o, $(obj-y))))
obj-files := $(strip $(obj-files))

# List Objs(*.o) which build from the src in current dir
objs_in_cur := $(filter $(notdir $(filter %.o, $(obj-y))), $(filter %.o, $(obj-y)))
# List Objs(path/*.o) which build from the src in sub-dir
objs_in_sub := $(filter-out $(objs_in_cur), $(filter %.o, $(obj-y)))

build-objs: $(OBJ_DIR) $(target)
$(target) : $(obj-files)
	$(LD) -r $^ -o $@

$(OBJ_DIR):
	@mkdir -p $@

$(filter %/built-in.o, $(obj-files)): $(OBJ_DIR)/%/built-in.o: % PHONY
	$(MAKE) -f $(TOP_DIR)/build/rules/rules_main.mk -C $<

ifneq ($(objs_in_cur), )
$(patsubst %, $(OBJ_DIR)/%, $(objs_in_cur)): $(OBJ_DIR)/%.o : $(CUR_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
endif

ifneq ($(objs_in_sub), )
define rule_of_objs_in_sub
$(OBJ_DIR)/$(notdir $(1)): $(CUR_DIR)/$(strip $(patsubst %.o, %.c, $(1)))
	$(CC) $(CFLAGS) $(INCLUDES) -c $$< -o $$@
endef

$(foreach n, $(objs_in_sub), $(eval $(call rule_of_objs_in_sub,$(n))))
endif

.PHONY : PHONY
	