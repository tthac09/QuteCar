mqtt_srcs := src src/samples
CCFLAGS += -funsigned-char
CCFLAGS += -I$(MAIN_TOPDIR)/third_party/paho.mqtt.c-1.3.0/src \
	-I$(MAIN_TOPDIR)/third_party/mbedtls-2.16.2/include \
	-I$(MAIN_TOPDIR)/platform/os/Huawei_LiteOS/kernel/include
