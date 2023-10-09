# 直接使用make进行编译的使用方法
# make 或 make all : 删除所有交付件再重新编译所有模块
# make clean : 删除所有交付件
# make link : 链接
MAIN_TOPDIR = $(shell pwd)
include $(MAIN_TOPDIR)/build/make_scripts/config.mk
include $(MAIN_TOPDIR)/build/make_scripts/config_lib_factory.mk
export MAIN_TOPDIR

GCC_PATH := $(dir $(shell which $(CC)))
LIBPATH += -L$(GCC_PATH)../lib/gcc/riscv32-unknown-elf/$(GCC_VER_NUM)
LIBPATH += $(addprefix -Lbuild/build_tmp/libs/, $(foreach module, $(COMPILE_MODULE), $($(module)_dir)))
COMPILE_MODULE += boot loaderboot
SIGN_CFG = A B
A_SIGN = 0x0040d3c0
B_SIGN = 0x004f13c0
FACTORY_SIGN = 0x0054d3c0

CCFLAGS += -DCONFIG_FACTORY_TEST_MODE

FIRM_WARE_VER := $(shell if [ $(CONFIG_TARGET_KERNEL_VER) -gt -1 ] && [ $(CONFIG_TARGET_KERNEL_VER) -lt 49 ];then echo $(CONFIG_TARGET_KERNEL_VER);fi;)
ifneq ($(FIRM_WARE_VER), $(CONFIG_TARGET_KERNEL_VER))
$(error [ERROR]:FIRM_WARE_VER=$(FIRM_WARE_VER) Ranges From 0 To 48)
endif

BOOT_VER = $(shell if [ $(CONFIG_TARGET_BOOT_VER) -gt -1 ] && [ $(CONFIG_TARGET_BOOT_VER) -lt 17 ];then echo $(CONFIG_TARGET_BOOT_VER);fi;)
ifneq ($(BOOT_VER), $(CONFIG_TARGET_BOOT_VER))
$(error [ERROR]:BOOT_VER Ranges From 0 To 16)
endif

ifeq ($(CONFIG_TARGET_SIG_RSA_V15), y)
    SIGN_ALG = 0x0
else ifeq ($(CONFIG_TARGET_SIG_RSA_PSS), y)
    SIGN_ALG = 0x1
else ifeq ($(CONFIG_TARGET_SIG_ECC), y)
    SIGN_ALG = 0x10
else ifeq ($(CONFIG_TARGET_SIG_SHA256), y)
    SIGN_ALG = 0x3F
endif

OUT_DIRS = $(MAIN_TOPDIR)/$(LOG_PATH) $(MAIN_TOPDIR)/$(CACHE_PATH) $(MAIN_TOPDIR)/$(LINK_PATH) $(MAIN_TOPDIR)/$(NV_PATH) $(MAIN_TOPDIR)/$(BIN_PATH)
all:prepare build_libs_factory nv link_factory check_factory ota_factory
	$(Q)echo "< ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ >"
	$(Q)echo "                        FACTORY BIN BUILD SUCCESS                        "
	$(Q)echo "< ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ >"

build_libs_factory:
	$(Q)$(foreach module, $(COMPILE_MODULE), \
	$(foreach lib, $($(module)_libs), \
	$(MAKE) -C $(MAIN_TOPDIR)/$($(module)_dir) MODULE_DIR=$($(module)_dir) LIB=$(lib) FACTORY_MODE=y all;))

nv:
	$(NV_TOOL) $(MAIN_TOPDIR)/$(NV_PATH) $(TARGET_NAME) $(NV_CFG_NAME)

link_factory:
	$(CC) $(LINK_SCRIPTS_FLAG) -E $(MAIN_TOPDIR)/build/scripts/system_config.ld.S -o $(MAIN_TOPDIR)/$(LINK_PATH)/system_config.ld -P
	$(CC) $(LINK_SCRIPTS_FLAG) -DFLASH_FIRM_START=$(FACTORY_SIGN) -E $(MAIN_TOPDIR)/build/scripts/link.ld.S -o $(MAIN_TOPDIR)/$(LINK_PATH)/link_factory.lds -P;
	$(LINK) $(LINKFLAGS) $(LIBPATH) -T$(MAIN_TOPDIR)/$(LINK_PATH)/link_factory.lds -Map=$(MAIN_TOPDIR)/$(BIN_PATH)/$(TARGET_NAME)_factory.map -o $(MAIN_TOPDIR)/$(BIN_PATH)/$(TARGET_NAME)_factory.out --start-group $(sort $(LIBS)) --end-group;
	$(OBJCOPY) -O binary $(MAIN_TOPDIR)/$(BIN_PATH)/$(TARGET_NAME)_factory.out $(MAIN_TOPDIR)/$(CACHE_PATH)/$(TARGET_NAME)_factory.bin;
	$(OBJDUMP) -d $(MAIN_TOPDIR)/$(BIN_PATH)/$(TARGET_NAME)_factory.out >$(MAIN_TOPDIR)/$(BIN_PATH)/$(TARGET_NAME)_factory.asm;
	dd if=$(MAIN_TOPDIR)/$(CACHE_PATH)/$(TARGET_NAME)_factory.bin of=$(MAIN_TOPDIR)/$(CACHE_PATH)/$(TARGET_NAME)_base_factory.bin seek=0 count=278 bs=1K;
	dd if=$(MAIN_TOPDIR)/$(CACHE_PATH)/$(TARGET_NAME)_factory.bin of=$(MAIN_TOPDIR)/$(CACHE_PATH)/$(TARGET_NAME)_kernel_factory.bin skip=278 bs=1K

ota_factory:
	$(OTA_TOOL) factory_bin $(TARGET_NAME) $(SIGN_ALG) $(FIRM_WARE_VER) $(BOOT_VER) $(MAIN_TOPDIR)/$(BIN_PATH)/$(TARGET_NAME)_factory.bin usr_output=$(USR_OUTPUT_DIR) $(MAIN_TOPDIR)/$(CACHE_PATH)/$(TARGET_NAME)_kernel_factory.bin A
	cp -rf $(MAIN_TOPDIR)/$(BIN_PATH)/ $(MAIN_TOPDIR)/build/libs/factory_bin/

check_factory: link_factory
	TARGET_SUM=$(shell sha256sum $(MAIN_TOPDIR)/$(CACHE_PATH)/$(TARGET_NAME)_base_factory.bin | cut -d ' ' -f1) && \
	if [ "$$TARGET_SUM" == '$(BASE_NUM)' ]; \
	then echo "#########BASE BIN IS SAME WITH FIRST COMPILE!#########"; \
	else echo "\033[32m#########BASE BIN IS DIFFERENT WITH FIRST COMPILE!#########\033[0m"; exit 1; fi;

prepare:
	$(Q)$(foreach x, $(OUT_DIRS),\
	if [ ! -d $x ]; \
	then \
	mkdir -p $x; \
	echo create dir $(x) success; \
	fi;)

clean:
	$(Q)$(foreach x, $(OUT_DIRS),\
	if [ -d $(dir $x) ]; \
	then \
	$(RM) $(dir $x); \
	echo clean dir $(dir $x) success; \
	fi;)



