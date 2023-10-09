mbedtls_srcs := library
CCFLAGS += -I$(MAIN_TOPDIR)/third_party/mbedtls-2.16.2/include \
	-I$(MAIN_TOPDIR)/third_party/mbedtls-2.16.2/include/mbedtls \
	-I$(MAIN_TOPDIR)/platform/drivers/cipher \
	-I$(MAIN_TOPDIR)/components/lwip_sack/include
