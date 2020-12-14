CONFIG_FILE := $(TOP_DIR)/.config
CUR_DIR := $(patsubst %/, %, $(shell pwd))
obj-y :=

-include $(CONFIG_FILE)
-include $(CUR_DIR)/build.mk

# contains Objs(Like *.o or path/*.o) and Sub-dirs(Like path/)
obj-y := $(sort $(obj-y))

# List Sub-dirs
sub-dirs := $(strip $(patsubst %/, %, $(filter %/, $(obj-y))))
export-srcs := $(strip $(patsubst %.o, %.c, $(filter %.o, $(obj-y))))

export-srcs : $(EXPORT_SRCS_DIR)
	@if [ -n "$(export-srcs)" ];			\
	then									\
		for src in $(export-srcs);			\
		do cp $$src $(EXPORT_SRCS_DIR)/;	\
		done;								\
	fi

	@if [ -n "$(sub-dirs)" ];				\
	then									\
		for sub in $(sub-dirs);				\
		do $(MAKE) -C $$sub -f $(TOP_DIR)/build/rules/rules_export.mk;	\
		done;								\
	fi

$(EXPORT_SRCS_DIR):
	@mkdir -p $@

.PHONY: export-files
