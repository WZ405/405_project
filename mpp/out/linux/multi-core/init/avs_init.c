#include <linux/module.h>
#include <linux/of_platform.h>
#include "hi_osal.h"


extern int AVS_ModInit(void);
extern void AVS_ModExit(void);

extern unsigned int avs_irq;
extern void* pAvsReg;

static int hi35xx_avs_probe(struct platform_device *pdev)
{
    struct resource *mem;

    mem = osal_platform_get_resource(pdev, IORESOURCE_MEM, 0);

    pAvsReg = devm_ioremap_resource(&pdev->dev, mem);

    if (IS_ERR(pAvsReg))
    {
        return PTR_ERR(pAvsReg);
    }

    avs_irq = osal_platform_get_irq(pdev, 0);

    if (avs_irq <= 0)
    {
        dev_err(&pdev->dev, "cannot find avs IRQ\n");
    }

    AVS_ModInit();

    return 0;
}

static int hi35xx_avs_remove(struct platform_device *pdev)
{
    AVS_ModExit();
    pAvsReg = NULL;
    return 0;
}


static const struct of_device_id hi35xx_avs_match[] = {
        { .compatible = "hisilicon,hisi-avs" },
        {},
};

MODULE_DEVICE_TABLE(of, hi35xx_avs_match);

static struct platform_driver hi35xx_avs_driver = {
        .probe          = hi35xx_avs_probe,
        .remove         = hi35xx_avs_remove,
        .driver         = {
                .name   = "hi35xx_avs",
                .of_match_table = hi35xx_avs_match,
        },
};

osal_module_platform_driver(hi35xx_avs_driver);

MODULE_LICENSE("Proprietary");
