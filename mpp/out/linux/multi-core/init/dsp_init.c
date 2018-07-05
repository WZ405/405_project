#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/version.h>

#include "hi_common.h"
#include "hi_osal.h"
#include "hi_dsp.h"

extern HI_U16 g_u16SvpDspNodeNum;

extern int SVP_DSP_ModInit(void);
extern void SVP_DSP_ModExit(void);

module_param_named(max_node_num, g_u16SvpDspNodeNum, ushort, S_IRUGO);

#include <linux/of_platform.h>
#define SVP_DSP_DEV_NAME_LENGTH 10
extern volatile void* g_apstSvpDspReg[SVP_DSP_ID_BUTT];

static int hi35xx_svp_dsp_probe(struct platform_device *pdev)
{	
	HI_U32 i;    
	HI_CHAR szDevName[SVP_DSP_DEV_NAME_LENGTH] = {'\0'};   
	struct resource *pstMem = NULL;    

	for (i = 0; i < SVP_DSP_ID_BUTT; i++)    
	{        
		snprintf(szDevName, SVP_DSP_DEV_NAME_LENGTH, "dsp%d", i);   
		pstMem = osal_platform_get_resource_byname(pdev, IORESOURCE_MEM, szDevName);       
		g_apstSvpDspReg[i] = devm_ioremap_resource(&pdev->dev, pstMem);       
		if (IS_ERR((void*)g_apstSvpDspReg[i]))       
		{ 	printk("Line:%d,Func:%s,CoreId(%u)\n",__LINE__,__FUNCTION__,i);

			return PTR_ERR((const void*)g_apstSvpDspReg[i]); 
		}    
		/*do not must get irq*/
	}
	

    return SVP_DSP_ModInit();
}

static int hi35xx_svp_dsp_remove(struct platform_device *pdev)
{
	HI_U32 i; 
    SVP_DSP_ModExit();

	for (i = 0; i < SVP_DSP_ID_BUTT; i++)    
	{    	
		g_apstSvpDspReg[i] = HI_NULL;   
	}
    return 0;
}

static const struct of_device_id hi35xx_svp_dsp_match[] = {
        { .compatible = "hisilicon,hisi-dsp" },
        {},
};
MODULE_DEVICE_TABLE(of, hi35xx_svp_dsp_match);

static struct platform_driver hi35xx_svp_dsp_driver = {
        .probe          = hi35xx_svp_dsp_probe,
        .remove         = hi35xx_svp_dsp_remove,
        .driver         = {
                .name   = "hi35xx_dsp",
                .of_match_table = hi35xx_svp_dsp_match,
        },
};

osal_module_platform_driver(hi35xx_svp_dsp_driver);

MODULE_LICENSE("Proprietary");

