mbedtls_srcs := library
CCFLAGS += -I$(MAIN_TOPDIR)/third_party/mbedtls/include \
	-I$(MAIN_TOPDIR)/third_party/mbedtls/include/mbedtls \
	-I$(MAIN_TOPDIR)/platform/drivers/cipher \
	-I$(MAIN_TOPDIR)/components/lwip_sack/include
