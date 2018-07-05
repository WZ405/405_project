/*
 *
 * (C) COPYRIGHT 2015, 2017 ARM Limited. All rights reserved.
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



#include <mali_kbase_config.h>
#include <mali_kbase_proc.h>
#include <linux/io.h>

#define KBASE_HISI_GPU_REST 	0x120100d8

static volatile u32* g_GPUReset = NULL;

int kbase_clk_enable(void)
{
	if(NULL != g_GPUReset)
	{
		u32 gpuReset = *g_GPUReset;

		/* gpu clk enable */
		*g_GPUReset = gpuReset | 0x13;

		/* gpu cancel reset */
		gpuReset = *g_GPUReset;
		*g_GPUReset = gpuReset & 0xfffffffc;

		return 0;
	}
	printk("----ERROR---- Line=%d, func=%s cannot access clk_cfg_reg\n", __LINE__, __func__);
	return -1;
}

void kbase_clk_disable(void)
{
	if(NULL != g_GPUReset)
	{
		u32 gpuReset = *g_GPUReset;

		/* gpu reset */
		*g_GPUReset = gpuReset | 0x10;

		/* gpu disable */
		gpuReset = *g_GPUReset;
		*g_GPUReset = gpuReset & 0xffffffef;
	}
}


int kbase_platform_early_init(void)
{
	g_GPUReset = (volatile u32*)ioremap_nocache(KBASE_HISI_GPU_REST,  32);

	/* Nothing needed at this stage */
	return 0;
}

static int platform_callback_init(struct kbase_device *kbdev)
{
	//TODO: This clk enable and disable part should be done in kernel.
	//Driver uses clk_enable and clk_disable to call the implemetation in kernel.
#if 0
	clk_enable(kbdev->clock);
#else
	kbase_clk_enable();
#endif

	kbase_proc_create();
	return 0;
}

static void platform_callback_term(struct kbase_device *kbdev)
{
	kbase_proc_destroy();

	//TODO: This clk enable and disable part should be done in kernel.
	//Driver uses clk_enable and clk_disable to call the implemetation in kernel.
#if 0
	clk_disable(kbdev->clock);
#else
	kbase_clk_disable();
#endif

	iounmap(g_GPUReset);
}

struct kbase_platform_funcs_conf platform_callbacks = {
	.platform_init_func = platform_callback_init,
	.platform_term_func = platform_callback_term
};

static struct kbase_platform_config dummy_platform_config;

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &dummy_platform_config;
}

#ifndef CONFIG_OF
int kbase_platform_register(void)
{
	return 0;
}

void kbase_platform_unregister(void)
{
}
#endif
