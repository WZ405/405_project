#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/version.h>
#include <linux/of_platform.h>
#include <asm/io.h>

#include "hi_comm_video.h"
#include "hi_common.h"
#include "hi_osal.h"

extern int DPU_MATCH_ModInit(void);
extern void DPU_MATCH_ModExit(void);

extern void* g_pastDpuMatchReg;

extern unsigned int dpu_match_irq;

#define REG_BASE  0x11630000

static int hi35xx_dpu_match_probe(struct platform_device *pdev)
{
    HI_CHAR aDevName[16] = "dpu_match";
    HI_U32 u32RegLen = 0x10000;

    g_pastDpuMatchReg = ioremap(REG_BASE, u32RegLen);
    if (IS_ERR(g_pastDpuMatchReg))
            return PTR_ERR(g_pastDpuMatchReg);

    snprintf(aDevName, sizeof(aDevName), "match");
    dpu_match_irq = osal_platform_get_irq_byname(pdev, aDevName);
    if (dpu_match_irq <= 0) {
            dev_err(&pdev->dev, "cannot find vou IRQ\n");
    }

    DPU_MATCH_ModInit();
    
    return 0;
}

static int hi35xx_dpu_match_remove(struct platform_device *pdev)
{
    DPU_MATCH_ModExit();

    if (g_pastDpuMatchReg)
    {
        iounmap(g_pastDpuMatchReg);
        g_pastDpuMatchReg = NULL;
    }
    
    return 0;
}

static const struct of_device_id hi35xx_dpu_match_match[] = {
        { .compatible = "hisilicon,hisi-dpu_match" },
        {},
};
MODULE_DEVICE_TABLE(of, hi35xx_dpu_match_match);

static struct platform_driver hi35xx_dpu_match_driver = {
        .probe          = hi35xx_dpu_match_probe,
        .remove         = hi35xx_dpu_match_remove,
        .driver         = {
                .name   = "hi35xx_dpu_match",
                .of_match_table = hi35xx_dpu_match_match,
        },
};

osal_module_platform_driver(hi35xx_dpu_match_driver);

MODULE_LICENSE("Proprietary");

