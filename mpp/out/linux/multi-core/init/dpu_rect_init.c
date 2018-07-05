#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/version.h>
#include <linux/of_platform.h>
#include <asm/io.h>

#include "hi_comm_video.h"
#include "hi_common.h"
#include "hi_osal.h"

extern int DPU_RECT_ModInit(void);
extern void DPU_RECT_ModExit(void);

extern void* g_pstDpuRectReg;

extern unsigned int dpu_rect_irq;

#define REG_BASE  0x11630000

static int hi35xx_dpu_rect_probe(struct platform_device *pdev)
{
    HI_CHAR aDevName[16] = "dpu_rect";    
    HI_U32 u32RegLen = 0x10000;

    g_pstDpuRectReg = ioremap(REG_BASE, u32RegLen);
    if (IS_ERR(g_pstDpuRectReg))
            return PTR_ERR(g_pstDpuRectReg);
    
    snprintf(aDevName, sizeof(aDevName), "rect");
    dpu_rect_irq = osal_platform_get_irq_byname(pdev, aDevName);
    if (dpu_rect_irq <= 0) {
            dev_err(&pdev->dev, "cannot find vou IRQ\n");
    }

    DPU_RECT_ModInit();
    
    return 0;
}

static int hi35xx_dpu_rect_remove(struct platform_device *pdev)
{
    DPU_RECT_ModExit();

    if (g_pstDpuRectReg)
    {
        iounmap(g_pstDpuRectReg);
        g_pstDpuRectReg = NULL;
    }
    
    return 0;
}

static const struct of_device_id hi35xx_dpu_rect_match[] = {
        { .compatible = "hisilicon,hisi-dpu_rect" },
        {},
};
MODULE_DEVICE_TABLE(of, hi35xx_dpu_rect_match);

static struct platform_driver hi35xx_dpu_rect_driver = {
        .probe          = hi35xx_dpu_rect_probe,
        .remove         = hi35xx_dpu_rect_remove,
        .driver         = {
                .name   = "hi35xx_dpu_rect",
                .of_match_table = hi35xx_dpu_rect_match,
        },
};

osal_module_platform_driver(hi35xx_dpu_rect_driver);

MODULE_LICENSE("Proprietary");

