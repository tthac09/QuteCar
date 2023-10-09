/*
 * Copyright (c) 2020 HiHope Community.
 * Description: IoT platform
 * Author: HiSpark Product Team
 * Create: 2020-5-20
 */

/**
 * This file make use the hmac to make mqtt pwd.The method is use the date string to hash the device passwd .
 * Take care that this implement depends on the hmac of the mbedtls
*/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "md.h"
#include "md_internal.h"

#define CN_HMAC256_LEN   32

//make a byte to 2 ascii hex
static int byte2hexstr(unsigned char  *bufin, int len, char *bufout)
{
    int i = 0;
    unsigned char  tmp_l = 0x0;
    unsigned char  tmp_h = 0;
    if ((NULL == bufin )|| (len <= 0 )||( NULL == bufout))
    {
        return -1;
    }
    for(i = 0; i < len; i++)
    {
        tmp_h = (bufin[i]>>4)&0X0F;
        tmp_l = bufin[i] &0x0F;
        bufout[2*i] = (tmp_h > 9)? (tmp_h - 10 + 'a'):(tmp_h +'0');
        bufout[2*i + 1] = (tmp_l > 9)? (tmp_l - 10 + 'a'):(tmp_l +'0');
    }
    bufout[2*len] = '\0';

    return 0;
}


int HmacGeneratePwd(const unsigned char *content, int content_len,const unsigned char *key,int key_len, \
                         unsigned char *buf,int buf_len)
{
    int ret = -1;
    mbedtls_md_context_t mbedtls_md_ctx;
    const mbedtls_md_info_t *md_info;
    unsigned char hash[CN_HMAC256_LEN];

    if ( key == NULL || content == NULL || buf == NULL || \
         key_len == 0 || content_len == 0 || (buf_len < (CN_HMAC256_LEN*2 +1))){
        return ret;
    }

    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info == NULL || (size_t)md_info->size > CN_HMAC256_LEN){
        return ret;
    }

    mbedtls_md_init(&mbedtls_md_ctx);
    ret = mbedtls_md_setup(&mbedtls_md_ctx, md_info, 1);
    if (ret != 0){
        goto exit;
    }

    (void)mbedtls_md_hmac_starts(&mbedtls_md_ctx, key, key_len);
    (void)mbedtls_md_hmac_update(&mbedtls_md_ctx, content, content_len);
    (void)mbedtls_md_hmac_finish(&mbedtls_md_ctx, hash);

    ///<transfer the hash code to the string mode
    byte2hexstr(hash,CN_HMAC256_LEN,(char *)buf);
exit:
    mbedtls_md_free(&mbedtls_md_ctx);

    return ret;
}

