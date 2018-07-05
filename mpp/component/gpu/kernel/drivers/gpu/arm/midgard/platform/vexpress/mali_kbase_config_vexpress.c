/*
 *
 * (C) COPYRIGHT 2011-2016 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */

#undef CONFIG_OF



#include <linux/ioport.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include "mali_kbase_cpu_vexpress.h"
#include "mali_kbase_config_platform.h"

#define HARD_RESET_AT_POWER_OFF 0
#define KBASE_HISI_GPU_REST 	0x120100d8

static volatile u32* g_pu32GPUReset = NULL;
#ifndef CONFIG_OF
static struct kbase_io_resources io_resources = {
	.job_irq_number = 149,
	.mmu_irq_number = 150,
	.gpu_irq_number = 148,
	.io_memory_region = {
	.start = 0x11640000,
	.end = 0x11640000 + (4096 * 4) - 1
	}
};
#endif /* CONFIG_OF */

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	u32 gpuReset;

	if(NULL != g_pu32GPUReset)
	{
		gpuReset = 0x11;
		*g_pu32GPUReset = gpuReset;

		gpuReset = 0x10;
		*g_pu32GPUReset = gpuReset;
	}
	return 0;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
	u32 gpuReset;

	if(NULL != g_pu32GPUReset)
	{
		gpuReset = 0x11;
		*g_pu32GPUReset = gpuReset;

		gpuReset = 0x01;
		*g_pu32GPUReset = gpuReset;
	}
#if HARD_RESET_AT_POWER_OFF
	/* Cause a GPU hard reset to test whether we have actually idled the GPU
	 * and that we properly reconfigure the GPU on power up.
	 * Usually this would be dangerous, but if the GPU is working correctly it should
	 * be completely safe as the GPU should not be active at this point.
	 * However this is disabled normally because it will most likely interfere with
	 * bus logging etc.
	 */
	//KBASE_TRACE_ADD(kbdev, CORE_GPU_HARD_RESET, NULL, NULL, 0u, 0);
	kbase_os_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND), GPU_COMMAND_HARD_RESET);
#endif
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback  = NULL,
	.power_resume_callback = NULL
};

static struct kbase_platform_config versatile_platform_config = {
#ifndef CONFIG_OF
	.io_resources = &io_resources
#endif
};

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &versatile_platform_config;
}


int kbase_platform_early_init(void)
{
	/* Nothing needed at this stage */
	g_pu32GPUReset = (volatile u32*)ioremap_nocache(KBASE_HISI_GPU_REST,  32);
	return 0;
}
