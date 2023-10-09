cjson_srcs := cjson
cjson_utils_srcs := cjson_utils
CCFLAGS += 
CCFLAGS += -I$(MAIN_TOPDIR)/third_party/cjson
ifeq ($(CONFIG_CJSON), y)
	CCFLAGS += -DCJSON_NESTING_LIMIT=$(CONFIG_CJSON_NESTING_LIMIT)
endif

