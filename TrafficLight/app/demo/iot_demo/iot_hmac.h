/*
 * Copyright (c) 2020 HiHope Community.
 * Description: IoT Platform
 * Author: HiSpark Product Team
 * Create: 2020-5-20
 */

/**
 * This file make use the hmac to make mqtt pwd.The method is use the date string to hash the device passwd .
 * Take care that this implement depends on the hmac of the mbedtls
*/

/**
 * This function used to generate the passwd for the mqtt to connect the HuaweiIoT platform
 * @param content: This is the content for the hmac,and usually it is the device passwd set or get from the Iot Platform
 * @param content_len: The length of the content
 * @param key: This is the key for the hmac, and usually it is the time used in the client_id:the format is：yearmonthdatehour:like 1970010100
 * @param key_len: The length of the key
 * 
 * @param buf: used to storage the hmac code
 * @param buf_len:the buf length
 * 
 * @return:0 success while others failed
*/
int HmacGeneratePwd(const unsigned char *content, int content_len,const unsigned char *key,int key_len, \
                    unsigned char *buf,int buf_len);
