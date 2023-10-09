#!/usr/bin/env python3
# coding=utf-8
'''
* Copyright (C) HiSilicon Technologies Co., Ltd. 2019-2020. All rights reserved.
* Description: part of hupg build scripts
'''

from ctypes import *

###############################定义基本类型###############################################
hi_char=c_char
hi_s8=c_byte
hi_u8=c_ubyte
hi_s16=c_short
hi_u16=c_ushort
hi_s32=c_int
hi_u32=c_uint

#升级文件头中提供了32字节预留字段供用户使用。
#示例为默认填充了8字节,用于存储芯片产品类型;如不需要,屏蔽即可;或者填充其他内容。
class app_upg_user_info(Structure):
    _fields_ = [
        ("chip_product", hi_char*8),
        ("reserved", hi_u8*24),
        ]

def fill_user_info(chip_name):
    info_bin = bytearray(sizeof(app_upg_user_info))
    info = app_upg_user_info.from_buffer(info_bin)
    info.chip_product = bytes(chip_name, encoding='utf8')#如不需要填充芯片产品字段,屏蔽此行
    print("[upg fill user info]chip:%s"%(info.chip_product))

    return info_bin
