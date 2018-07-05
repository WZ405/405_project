#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/version.h>
#include <linux/of_platform.h>

#include "hi_common.h"
#include "hi_osal.h"


extern unsigned int gdc_en[GDC_IP_NUM];
extern HI_U32 max_gdc_job;
extern HI_U32 max_gdc_task;
extern HI_U32 max_gdc_node;

module_param_array(gdc_en, uint, HI_NULL, S_IRUGO);
module_param(max_gdc_job,  uint, S_IRUGO);
module_param(max_gdc_task, uint, S_IRUGO);
module_param(max_gdc_node, uint, S_IRUGO);

extern int  GDC_ModInit(void);
extern void GDC_ModExit(void);

extern void* pGdcReg[2];
extern unsigned int gdc_irq[2];

#define GDC_DEV_NAME_LENGTH 10
#define GDC_MAX_DEV_NUM 2

static int hi35xx_fisheye_probe(struct platform_device* pdev)
{
    HI_U32 i;
    HI_CHAR GdcDevName[GDC_DEV_NAME_LENGTH] = {'\0'};
    struct resource *mem;

    for(i = 0; i < GDC_MAX_DEV_NUM; i++)
    {
        snprintf(GdcDevName, GDC_DEV_NAME_LENGTH, "gdc%d", i);
        mem = osal_platform_get_resource_byname(pdev, IORESOURCE_MEM, GdcDevName);
        pGdcReg[i] = devm_ioremap_resource(&pdev->dev, mem);
        if (IS_ERR(pGdcReg[i]))
            return PTR_ERR(pGdcReg[i]);

        gdc_irq[i] = osal_platform_get_irq_byname(pdev, GdcDevName);
        if (gdc_irq[i] <= 0)
        {
            dev_err(&pdev->dev, "cannot find gdc%d IRQ\n",i);
        }
    }

    GDC_ModInit();

    return 0;
}

static int hi35xx_fisheye_remove(struct platform_device* pdev)
{
    HI_U32 i;

    GDC_ModExit();

    for (i = 0; i < GDC_MAX_DEV_NUM; i++)
    {
        pGdcReg[0] = HI_NULL;
    }

    return 0;
}


static const struct of_device_id hi35xx_fisheye_match[] =
{
    { .compatible = "hisilicon,hisi-gdc" },
    {},
};
MODULE_DEVICE_TABLE(of, hi35xx_fisheye_match);

static struct platform_driver hi35xx_fisheye_driver =
{
    .probe          = hi35xx_fisheye_probe,
    .remove         = hi35xx_fisheye_remove,
    .driver         = {
        .name   = "hi35xx_fisheye",
        .of_match_table = hi35xx_fisheye_match,
    },
};

osal_module_platform_driver(hi35xx_fisheye_driver);

MODULE_LICENSE("Proprietary");


