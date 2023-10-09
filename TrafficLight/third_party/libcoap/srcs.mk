include $(MAIN_TOPDIR)/build/config/usr_config.mk
coap_srcs = src/block.c \
src/coap_hashkey.c \
src/coap_io_lwip.c \
src/coap_session.c \
src/encode.c \
src/net.c \
src/option.c \
src/pdu.c \
src/resource.c \
src/str.c \
src/subscribe.c \
src/uri.c \
src/address.c
SRC_FILES = $(coap_srcs)