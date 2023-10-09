#ifndef __APP_DEMO_GET_MAC_ADDR_H_
#define __APP_DEMO_GET_MAC_ADDR_H_
#include <hi_types_base.h>

#define MAC_ADDR "%02x:%02x:%02x:%02x:%02x:%02x"
#define mac2str(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define HISPARK_SSID_MIN_MAC_LEN   (6) 

hi_u32 hi3816_get_mac_addr(hi_void);
#endif