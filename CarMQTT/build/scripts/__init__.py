#!/usr/bin/env python3
# coding=utf-8

'''
* Copyright (C) HiSilicon Technologies Co., Ltd. 2019-2020. All rights reserved.
* Description: package entry
* Author: HiSilicon
* Create: 2019-10-29
'''

import os
import sys

sys.path.append(os.path.join(os.getcwd(), 'build', 'scripts'))

__all__ = ['common_env', 'scons_env_cfg', 'scons_utils', 'hi_config_parser', 'scons_app', 'pkt_builder', 'make_upg_file']

