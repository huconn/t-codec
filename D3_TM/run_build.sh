#!/bin/bash

# BUILD: VIDEO CODEC
source /home/B110100/ToolChain/DVRS_ADT/environment-setup-aarch64-telechips-linux

export SUPPORT_4K_VIDEO="enable"
export BUILD_TESTER="on"
export WO_TMEDIA="on"

export TCC_CHIPSET="tcc805x"
export LINUX_PLATFORM_KERNELDIR="/home/B110100/WorkWork/__REF__/20231108_131115_AUTOLINUX_DVRS/build-autolinux/build/tcc8050-dvrs/tmp/work/tcc8050_dvrs-telechips-linux/linux-telechips/4.14.202-r0/git"

make
