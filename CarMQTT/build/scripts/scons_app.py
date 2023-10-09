#!/usr/bin/env python3
# coding=utf-8

'''
* Copyright (C) HiSilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
* Description: APP utilities
* Author: HiSilicon
* Create: 2019-10-29
'''

import os
import json
from common_env import insert_module
from common_env import insert_module_dir
from common_env import insert_lib_cfg
from common_env import insert_env_defs
from common_env import insert_os_include
from common_env import insert_common_include
from common_env import insert_ar_flags
from common_env import insert_as_flags
from common_env import insert_ld_flags
from common_env import insert_cc_flags
from common_env import set_factory_mode
from scons_utils import colors
from scons_utils import scons_usr_bool_option

__all__ = ["AppTarget"]

class AppBuildError(Exception):
    """
    Build exception.
    """
    pass

class AppTarget:
    """
    APP config
    """
    def __init__(self, app_name, factory_mode):
        self.app_name = app_name
        self.factory_mode = factory_mode
        self.app_env = None
        self.app_cfg_file = None
        self.proj_root = os.path.realpath(os.path.join(__file__, '..', '..', '..'))
        self.app_root = self.app_lookup()
        self.settings = {}
        self.app_ld_dirs = []

    def app_lookup(self):
        """
        Found app folder.
        """
        dirs = os.listdir(os.path.join(self.proj_root, 'app'))
        if self.app_name in dirs:
            return os.path.join('app', self.app_name)
        raise AppBuildError("%s============== Can NOT found the APP project --- %s! =============%s" %
            (colors['red'], self.app_name, colors['end']))

    def read_env(self):
        """
        Parse app settings
        """
        if self.app_pre_check() is False:
            return;
        try:
            with open(self.app_cfg_file) as app_cfg:
                self.settings = json.load(app_cfg)
        except Exception as e:
            raise AppBuildError("%s============== Read APP settings error: %s! =============%s" %
                (colors['red'], e, colors['end']))

        self.app_env_parse(self.settings)
        pass

    def get_app_libs(self):
        """
        Parse app libraries
        """
        libs = []
        for folder in self.app_ld_dirs:
            libs.extend([lib[3:-2] for lib in os.listdir(folder) if lib.startswith('lib') and lib.endswith('.a')])
        return list(map(lambda x:'-l%s'%x, libs))

    def get_app_lib_path(self):
        """
        Parse app library path
        """
        if len(self.app_ld_dirs) > 0:
            return list(map(lambda x:'-L%s'%x, self.app_ld_dirs))
        return []

    def get_app_name(self):
        return self.app_name

    def get_app_cfg(self, key):
        return self.settings.get(key, None)

    def app_pre_check(self):
        app_scons = os.path.join(self.proj_root, self.app_root, 'SConscript')
        if os.path.isfile(app_scons) == False:
            raise AppBuildError("%s============== Can NOT found the APP %s! =============%s" %
                (colors['red'], app_scons, colors['end']))
        self.app_cfg_file = os.path.join(self.proj_root, self.app_root, 'app.json')
        if os.path.isfile(self.app_cfg_file) == False:
            raise AppBuildError("%s============== Can NOT found the APP %s! =============%s" %
                (colors['red'], self.app_cfg_file, colors['end']))
        return True

    def app_env_parse(self, settings):
        try:
            insert_module(self.app_name)
            insert_module_dir(self.app_name, self.app_root)
            insert_lib_cfg(self.app_name, {settings["TARGET_LIB"] : settings["APP_SRCS"]})

            if settings.get("DEFINES") is not None:
                insert_env_defs(self.app_name, settings["DEFINES"])

            if settings.get("OS_INCLUDE") is not None:
                insert_os_include(self.app_name, settings["OS_INCLUDE"])
            if settings.get("INCLUDE") is not None:
                incs = []
                for inc in settings["INCLUDE"]:
                    incs.append(os.path.join('#', inc));
                if scons_usr_bool_option('CONFIG_HISTREAMING_SUPPORT') == 'y':
                    incs.append("#\\components/histreaming/src")
                else:
                    pass
                insert_common_include(self.app_name, incs)

            if settings.get("LD_DIRS") is not None:
                self.app_ld_dirs.extend(settings.get("LD_DIRS"))
                if scons_usr_bool_option('CONFIG_HISTREAMING_SUPPORT') == 'y':
                    self.app_ld_dirs.append("components/histreaming/lib")
                else:
                    pass  
            if settings.get("AR_FLAGS") is not None:
                insert_ar_flags(self.app_name, settings["AR_FLAGS"])
            if settings.get("LD_FLAGS") is not None:
                insert_ld_flags(self.app_name, settings["LD_FLAGS"])
            if settings.get("CC_FLAGS") is not None:
                insert_cc_flags(self.app_name, settings["CC_FLAGS"])
            if settings.get("AS_FLAGS") is not None:
                insert_as_flags(self.app_name, settings["AS_FLAGS"])
            if self.factory_mode == 'yes':
                set_factory_mode()
        except Exception as e:
            raise AppBuildError("%s============== APP settings error: %s! =============%s" %
                (colors['red'], e, colors['end']))

"""
for test only
"""
def test():
    app_obj = AppTarget('demo')
    app_obj.read_env()

if __name__ == '__main__':
    test()

