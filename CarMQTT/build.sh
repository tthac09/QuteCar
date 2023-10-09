#!/bin/bash
#
# Copyright (c) HiSilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
# Description: Menuconfig entry
# Author: HiSilicon
# Create: 2019-12-31
#
set -e
CROOT=$(pwd)

BUILD_SELECT=$1

if [ "$BUILD_SELECT" = "menuconfig" ]; then
	python3 $CROOT/tools/menuconfig/usr_config.py
elif [ "$BUILD_SELECT" = "clean" ]; then
	scons -c
elif [ "$BUILD_SELECT" = "all" ]; then
	if [ $(grep -cw "CONFIG_FACTORY_TEST_SUPPORT=y" $CROOT/build/config/usr_config.mk) != 0 ]; then
		echo Start build factory bin.
		rm -rf build/libs/factory_bin
		scons -c
		if [ ! -d $CROOT/build/build_tmp/logs/ ]; then
			mkdir -p $CROOT/build/build_tmp/logs
		fi
		scons -Q factory_mode='yes' 2>&1 | tee $CROOT/build/build_tmp/logs/build_kernel.log
		if [ -f build/libs/factory_bin/*_factory.bin ];  then
			scons -c
			if [ ! -d $CROOT/build/build_tmp/logs/ ]; then
				mkdir -p $CROOT/build/build_tmp/logs
			fi
			scons -Q 2>&1 | tee $CROOT/build/build_tmp/logs/build_kernel.log
			echo See build log from: $CROOT/build/build_tmp/logs/build_kernel.log
		fi
	else
		rm -rf build/libs/factory_bin
		if [ ! -d $CROOT/build/build_tmp/logs/ ]; then
			mkdir -p $CROOT/build/build_tmp/logs
		fi
		scons -Q 2>&1 | tee $CROOT/build/build_tmp/logs/build_kernel.log
		echo See build log from: $CROOT/build/build_tmp/logs/build_kernel.log
	fi
elif [ -z $BUILD_SELECT ]; then
	if [ -d "output/bin" ]; then
		rm -rf output/bin
	fi
	if [ $(grep -cw "CONFIG_FACTORY_TEST_SUPPORT=y" $CROOT/build/config/usr_config.mk) != 0 ]; then
		echo Start build factory bin.
		rm -rf build/libs/factory_bin
		scons -c
		if [ ! -d $CROOT/build/build_tmp/logs/ ]; then
			mkdir -p $CROOT/build/build_tmp/logs
		fi
		scons -Q factory_mode='yes' 2>&1 | tee $CROOT/build/build_tmp/logs/build_kernel.log
		if [ -f build/libs/factory_bin/*_factory.bin ];  then
			scons -c
			if [ ! -d $CROOT/build/build_tmp/logs/ ]; then
				mkdir -p $CROOT/build/build_tmp/logs
			fi
			scons -Q 2>&1 | tee $CROOT/build/build_tmp/logs/build_kernel.log
			echo See build log from: $CROOT/build/build_tmp/logs/build_kernel.log
		fi
	else
		rm -rf build/libs/factory_bin
		if [ ! -d $CROOT/build/build_tmp/logs/ ]; then
			mkdir -p $CROOT/build/build_tmp/logs
		fi
		scons -Q 2>&1 | tee $CROOT/build/build_tmp/logs/build_kernel.log
		echo See build log from: $CROOT/build/build_tmp/logs/build_kernel.log
	fi
else
	if [ $(grep -cw "CONFIG_FACTORY_TEST_SUPPORT=y" $CROOT/build/config/usr_config.mk) != 0 ]; then
		echo Start build factory bin.
		rm -rf build/libs/factory_bin
		scons -c
		if [ ! -d $CROOT/build/build_tmp/logs/ ]; then
			mkdir -p $CROOT/build/build_tmp/logs
		fi
		scons -Q app=$BUILD_SELECT factory_mode='yes' 2>&1 | tee $CROOT/build/build_tmp/logs/build_kernel.log
		if [ -f build/libs/factory_bin/*_factory.bin ];  then
			scons -c
			if [ ! -d $CROOT/build/build_tmp/logs/ ]; then
				mkdir -p $CROOT/build/build_tmp/logs
			fi
			scons -Q app=$BUILD_SELECT 2>&1 | tee $CROOT/build/build_tmp/logs/build_kernel.log
			echo See build log from: $CROOT/build/build_tmp/logs/build_kernel.log
		fi
	else
		rm -rf build/libs/factory_bin
		if [ ! -d $CROOT/build/build_tmp/logs/ ]; then
			mkdir -p $CROOT/build/build_tmp/logs
		fi
		scons -Q app=$BUILD_SELECT 2>&1 | tee $CROOT/build/build_tmp/logs/build_kernel.log
		echo See build log from: $CROOT/build/build_tmp/logs/build_kernel.log
	fi
fi
