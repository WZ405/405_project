#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/version.h>
#include <linux/of_platform.h>

#include "hi_common.h"
#include "hi_osal.h"
#include "hi_defines.h"

extern int VPSS_ModInit(void);
extern void VPSS_ModExit(void);

extern unsigned int vpss_en[VPSS_IP_NUM];
module_param_array(vpss_en, uint, HI_NULL, S_IRUGO);


extern void * pVpssReg[VPSS_IP_NUM];
extern unsigned int vpss_irq[VPSS_IP_NUM];

#define VPSS_DEV_NAME_LENGTH 10

static int hi35xx_vpss_probe(struct platform_device *pdev)
{
    HI_U32 i;
    HI_CHAR VpssDevName[VPSS_DEV_NAME_LENGTH] = {'\0'};
    struct resource* mem;

    for (i = 0; i < VPSS_IP_NUM; i++)
    {
        snprintf(VpssDevName, VPSS_DEV_NAME_LENGTH, "vpss%d", i);
        mem = osal_platform_get_resource_byname(pdev, IORESOURCE_MEM, VpssDevName);
        pVpssReg[i] = devm_ioremap_resource(&pdev->dev, mem);

        if (IS_ERR(pVpssReg[i]))
        { return PTR_ERR(pVpssReg[i]); }

        vpss_irq[i] = osal_platform_get_irq_byname(pdev, VpssDevName);

        if (vpss_irq[i] <= 0)
        {
            dev_err(&pdev->dev, "cannot find vpss%d IRQ\n", i);
        }
    }

    VPSS_ModInit();

    return 0;
}


static int hi35xx_vpss_remove(struct platform_device *pdev)
{
    HI_U32 i;

    VPSS_ModExit();

    for (i = 0; i < VPSS_IP_NUM; i++)
    {
        pVpssReg[i] = HI_NULL;
    }

    return 0;
}


static const struct of_device_id hi35xx_vpss_match[] = {
        { .compatible = "hisilicon,hisi-vpss" },
        {},
};

MODULE_DEVICE_TABLE(of, hi35xx_vpss_match);

static struct platform_driver hi35xx_vpss_driver = {
        .probe          = hi35xx_vpss_probe,
        .remove         = hi35xx_vpss_remove,
        .driver         = {
                .name   = "hi35xx_vpss",
                .of_match_table = hi35xx_vpss_match,
        },
};

osal_module_platform_driver(hi35xx_vpss_driver);

MODULE_LICENSE("Proprietary");


