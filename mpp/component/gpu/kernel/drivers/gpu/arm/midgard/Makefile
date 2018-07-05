#
# (C) COPYRIGHT 2010-2016, 2017 ARM Limited. All rights reserved.
#
# This program is free software and is provided to you under the terms of the
# GNU General Public License version 2 as published by the Free Software
# Foundation, and any use by you of this program is subject to the terms
# of such GNU licence.
#
# A copy of the licence is included with the program, and can also be obtained
# from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.
#
#



KDIR ?= /lib/modules/$(shell uname -r)/build

BUSLOG_PATH_RELATIVE = $(CURDIR)/../../../..
KBASE_PATH_RELATIVE = $(CURDIR)

SCONS_CFLAGS=-DMALI_GCC_WORKAROUND_MIDCOM_4598=0 -DCONFIG_MALI_DEBUG -DCONFIG_MALI_BACKEND=gpu -DCONFIG_MALI_DEVFREQ=y
SCONS_CONFIGS=MALI_GCC_WORKAROUND_MIDCOM_4598=0 CONFIG_MALI_MIDGARD=m CONFIG_MALI_DEBUG=y CONFIG_MALI_BACKEND=gpu CONFIG_MALI_DEVFREQ=y

ifeq ($(MALI_UNIT_TEST), 1)
	EXTRA_SYMBOLS += $(KBASE_PATH_RELATIVE)/tests/internal/src/kernel_assert_module/linux/Module.symvers
endif

ifeq ($(CONFIG_MALI_FPGA_BUS_LOGGER),y)
#Add bus logger symbols
EXTRA_SYMBOLS += $(BUSLOG_PATH_RELATIVE)/drivers/base/bus_logger/Module.symvers
endif

# we get the symbols from modules using KBUILD_EXTRA_SYMBOLS to prevent warnings about unknown functions
all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) EXTRA_CFLAGS="-I$(CURDIR)/../../../../include -I$(CURDIR)/../../../../tests/include $(SCONS_CFLAGS) -I$(SDK_DIR)/osal/include" $(SCONS_CONFIGS) KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)" modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
