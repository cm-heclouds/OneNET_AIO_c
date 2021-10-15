CONFIG_FILE := $(TOP_DIR)/.config
CUR_DIR := $(patsubst %/, %, $(shell pwd))
RELATIVE_PATH := $(subst $(TOP_DIR)/,,$(CUR_DIR))
obj-y :=

-include $(CONFIG_FILE)
-include $(CUR_DIR)/build.mk

include $(TOP_DIR)/build/rules/rules_include.mk

inc-y := $(sort $(inc-y))

# contains Objs(Like *.o or path/*.o) and Sub-dirs(Like path/)
obj-y := $(sort $(obj-y))

# List Sub-dirs
sub_dirs := $(strip $(patsubst %/, %, $(filter %/, $(obj-y))))
export-srcs := $(strip $(patsubst %.o, %.c, $(filter %.o, $(obj-y))))

export-srcs :
	@if [ -n "$(export-srcs)" ];			\
	then									\
		for src in $(export-srcs);			\
		do									\
			echo $(RELATIVE_PATH)/$$src;	\
		done;								\
	fi

	@if [ -n "$(sub_dirs)" ];				\
	then									\
		for sub in $(sub_dirs);				\
		do									\
			$(MAKE) -s -C $$sub -f $(TOP_DIR)/build/rules/rules_export.mk;	\
		done;								\
	fi

.PHONY: export-srcs
