#include <linux/module.h>

#include "hi_defines.h"
#include "hi_type.h"
#include "hi_osal.h"
#include "vou_exp.h"

extern VOU_EXPORT_SYMBOL_S g_stVouExpSymbol;

EXPORT_SYMBOL(g_stVouExpSymbol);

extern int VOU_ModInit(void);
extern void VOU_ModExit(void);

#include <linux/of_platform.h>

extern void * pVoReg;
extern void * pVoHippReg;
extern int    vou_irq;

#define VO_DEV_NAME_LENGTH 10

static int hi35xx_vo_probe(struct platform_device *pdev)
{
    HI_CHAR VoDevName[VO_DEV_NAME_LENGTH] = "vo";
    HI_VOID * pReg;
    struct resource *mem;

    mem = osal_platform_get_resource_byname(pdev, IORESOURCE_MEM, VoDevName);
    pReg = devm_ioremap_resource(&pdev->dev, mem);
    if (IS_ERR(pReg))
    {
        osal_printk("devm_ioremap_resource fail for %d\n", PTR_ERR(pReg));
        return PTR_ERR(pReg);
    }
    pVoReg     = pReg;
    pVoHippReg = pReg;
    vou_irq     = osal_platform_get_irq_byname(pdev, VoDevName);
    if (vou_irq <= 0)
    {
        dev_err(&pdev->dev, "cannot find vo IRQ\n");
        return HI_FAILURE;
    }

    VOU_ModInit();

    return 0;
}

static int hi35xx_vo_remove(struct platform_device *pdev)
{
    VOU_ModExit();
    pVoReg     = NULL;
    pVoHippReg = NULL;

    return 0;
}


static const struct of_device_id hi35xx_vo_match[] = {
        { .compatible = "hisilicon,hisi-vo" },
        {},
};
MODULE_DEVICE_TABLE(of, hi35xx_vo_match);

static struct platform_driver hi35xx_vo_driver = {
        .probe          = hi35xx_vo_probe,
        .remove         = hi35xx_vo_remove,
        .driver         = {
                .name   = "hi35xx_vo",
                .of_match_table = hi35xx_vo_match,
        },
};

osal_module_platform_driver(hi35xx_vo_driver);

MODULE_LICENSE("Proprietary");
