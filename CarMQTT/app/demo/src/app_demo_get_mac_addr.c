
#include <hi_types_base.h>
#include <app_demo_get_mac_addr.h>
#include <stdio.h>
#include <hi_wifi_api.h>
#include <hi_early_debug.h>
#include <string.h>

hi_u8 hi3861_mac_addr[7] = {0}; /* 6 mac len */
hi_u8 mac_addr_char[64] = {0};
hi_u8 data_value =8;
hi_u8 hispark_ssid[64] = "HiSpark_WiFi-AP-";

hi_u8 hex2str(hi_u8 hex_byte, hi_u8* str)
{
    hi_u8 in_byte = hex_byte;
    static hi_u8 m=0;

    if (str == HI_NULL) {
        return HI_ERR_FAILURE;
    }
    if (m >= 255) {
        m=0;
    }
    if (((in_byte>>4)&0x0f) <= 9) {
         *(str+m) = ((in_byte>>4)&0x0f) +'0';
    } else {
        *(str+m) = (((in_byte>>4)&0x0f) - 0x20) + 0x57;
    }
    if ((in_byte&0x0f) <= 9) {
        *(str+m+1) = (in_byte&0x0f) + '0';
    } else {
        *(str+m+1) = ((in_byte&0x0f) - 0x20) + 0x57;
    }
    *(str+m+2) = ':';
    m = m+3;
}

/*get hi3861 mac addr */
hi_u32 hi3816_get_mac_addr(hi_void)
{
    static int j=0;
    if (hi_wifi_get_macaddr((hi_char*)hi3861_mac_addr, 6) != HI_ERR_SUCCESS) { /* 6 mac len */
        return HI_ERR_FAILURE;
    }
    printf("+MAC:" MAC_ADDR "\r\n", mac2str(hi3861_mac_addr));
    for(int mac_cnt=0; mac_cnt<HISPARK_SSID_MIN_MAC_LEN; mac_cnt++) {
        hex2str(hi3861_mac_addr[mac_cnt], mac_addr_char);
        j = j+3;
    }
    printf("HiSpark_MAC_ADDR: %s\r\n", mac_addr_char);
    memcpy(&hispark_ssid[16], &mac_addr_char[13], 1);
    memcpy(&hispark_ssid[17], &mac_addr_char[15], 2);
    return HI_ERR_SUCCESS;
}