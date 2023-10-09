mqtt_srcs := src
CCFLAGS += -funsigned-char
CCFLAGS += -I$(MAIN_TOPDIR)/third_party/paho.mqtt.c/src \
	-I$(MAIN_TOPDIR)/third_party/mbedtls/include \
	-I$(MAIN_TOPDIR)/platform/os/Huawei_LiteOS/kernel/include
