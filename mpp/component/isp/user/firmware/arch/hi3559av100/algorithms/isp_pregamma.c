/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_pregamma.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2014/08/19
  Description   :
  History       :
  1.Date        : 2014/08/19
    Author      :
    Modification: Created file

******************************************************************************/
#include <math.h>
#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_config.h"
#include "isp_proc.h"
#include "isp_ext_config.h"
#include "hi_math.h"
#include "isp_math_utils.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

static HI_U32 au32PreGAMMA[PREGAMMA_NODE_NUMBER] =
{
    0,   4096,   8192,  12288,  16384,  20480,  24576,  28672,  32768,  36864,  40960,  45056,  49152,  53248,  57344,  61440,  65536,  69632,  73728,  77824,  81920,  86016,  90112,  94208,  98304, 102400,
    106496, 110592, 114688, 118784, 122880, 126976, 131072, 135168, 139264, 143360, 147456, 151552, 155648, 159744, 163840, 167936, 172032, 176128, 180224, 184320, 188416, 192512, 196608, 200704, 204800, 208896,
    212992, 217088, 221184, 225280, 229376, 233472, 237568, 241664, 245760, 249856, 253952, 258048, 262144, 266240, 270336, 274432, 278528, 282624, 286720, 290816, 294912, 299008, 303104, 307200, 311296, 315392,
    319488, 323584, 327680, 331776, 335872, 339968, 344064, 348160, 352256, 356352, 360448, 364544, 368640, 372736, 376832, 380928, 385024, 389120, 393216, 397312, 401408, 405504, 409600, 413696, 417792, 421888,
    425984, 430080, 434176, 438272, 442368, 446464, 450560, 454656, 458752, 462848, 466944, 471040, 475136, 479232, 483328, 487424, 491520, 495616, 499712, 503808, 507904, 512000, 516096, 520192, 524288, 528384,
    532480, 536576, 540672, 544768, 548864, 552960, 557056, 561152, 565248, 569344, 573440, 577536, 581632, 585728, 589824, 593920, 598016, 602112, 606208, 610304, 614400, 618496, 622592, 626688, 630784, 634880,
    638976, 643072, 647168, 651264, 655360, 659456, 663552, 667648, 671744, 675840, 679936, 684032, 688128, 692224, 696320, 700416, 704512, 708608, 712704, 716800, 720896, 724992, 729088, 733184, 737280, 741376,
    745472, 749568, 753664, 757760, 761856, 765952, 770048, 774144, 778240, 782336, 786432, 790528, 794624, 798720, 802816, 806912, 811008, 815104, 819200, 823296, 827392, 831488, 835584, 839680, 843776, 847872,
    851968, 856064, 860160, 864256, 868352, 872448, 876544, 880640, 884736, 888832, 892928, 897024, 901120, 905216, 909312, 913408, 917504, 921600, 925696, 929792, 933888, 937984, 942080, 946176, 950272, 954368,
    958464, 962560, 966656, 970752, 974848, 978944, 983040, 987136, 991232, 995328, 999424, 1003520, 1007616, 1011712, 1015808, 1019904, 1024000, 1028096, 1032192, 1036288, 1040384, 1044480, 1048576
};

typedef struct hiISP_PREGAMMA_S
{
    HI_BOOL bEnable;
    HI_BOOL bLutUpdate;
} ISP_PREGAMMA_S;


ISP_PREGAMMA_S g_astPreGammaCtx[ISP_MAX_PIPE_NUM] = {{0}};
#define PREGAMMA_GET_CTX(dev, pstCtx)   pstCtx = &g_astPreGammaCtx[dev]

static HI_VOID PreGammaRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U16 i;
    HI_U8  u8BlockNum;
    HI_U32 *pau32PreGAMMA = HI_NULL;
    ISP_PREGAMMA_REG_CFG_S *pstPreGammaCfg = HI_NULL;
    ISP_PREGAMMA_S         *pstPreGammaCtx = HI_NULL;
    ISP_CTX_S              *pstIspCtx      = HI_NULL;
    ISP_CMOS_DEFAULT_S     *pstSnsDft      = HI_NULL;

    ISP_SensorGetDefault(ViPipe, &pstSnsDft);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    PREGAMMA_GET_CTX(ViPipe, pstPreGammaCtx);

    pstPreGammaCtx->bEnable    = HI_FALSE;
    pstPreGammaCtx->bLutUpdate = HI_FALSE;

    u8BlockNum = pstIspCtx->stBlockAttr.u8BlockNum;

    /*Read from CMOS*/
    if (pstSnsDft->stPreGamma.bValid)
    {
        /*TODO: Change when cmos is settled!!!!!!*/
        pau32PreGAMMA = pstSnsDft->stPreGamma.au32PreGamma;
    }
    else
    {
        pau32PreGAMMA = au32PreGAMMA;
    }

    for (i = 0 ; i < PREGAMMA_NODE_NUMBER ; i++)
    {
        hi_ext_system_pregamma_lut_write(ViPipe, i, pau32PreGAMMA[i]);
    }

    for (i = 0 ; i < u8BlockNum ; i++)
    {
        pstPreGammaCfg = &pstRegCfg->stAlgRegCfg[i].stPreGammaCfg;

        /*Static*/
        pstPreGammaCfg->stStaticRegCfg.u8BitDepthIn  = 20;
        pstPreGammaCfg->stStaticRegCfg.u8BitDepthOut = 20;
        pstPreGammaCfg->stStaticRegCfg.bStaticResh   = HI_TRUE;

        /*Dynamic*/
        //Enable Gamma
        pstPreGammaCfg->bPreGammaEn                       = HI_FALSE;
        pstPreGammaCfg->stDynaRegCfg.bPreGammaLutUpdateEn = HI_TRUE;
        pstPreGammaCfg->stDynaRegCfg.u32UpdateIndex       = 1;
        memcpy(pstPreGammaCfg->stDynaRegCfg.u32PreGammaLUT, pau32PreGAMMA, PREGAMMA_NODE_NUMBER * sizeof(HI_U32));
    }

    pstRegCfg->unKey.bit1PreGammaCfg = 1;

    return;
}

static HI_VOID PreGammaExtRegsInitialize(VI_PIPE ViPipe)
{
    ISP_CMOS_DEFAULT_S *pstSnsDft = HI_NULL;
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);

    if (pstSnsDft->stLdci.bValid)
    {
        hi_ext_system_pregamma_en_write(ViPipe, pstSnsDft->stPreGamma.bEnable);
    }
    else
    {
        hi_ext_system_pregamma_en_write(ViPipe, HI_FALSE);
    }

    hi_ext_system_pregamma_lut_update_write(ViPipe, HI_FALSE);

    return;
}


HI_S32 ISP_PreGammaInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;
    PreGammaRegsInitialize(ViPipe, pstRegCfg);
    PreGammaExtRegsInitialize(ViPipe);

    return HI_SUCCESS;
}

static HI_VOID ISP_PreGammaWdrModeSet(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    HI_U8  i;
    HI_U32 au32UpdateIdx[ISP_STRIPING_MAX_NUM] = {0};
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        au32UpdateIdx[i] = pstRegCfg->stAlgRegCfg[i].stPreGammaCfg.stDynaRegCfg.u32UpdateIndex;
    }

    ISP_PreGammaInit(ViPipe, pRegCfg);

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        pstRegCfg->stAlgRegCfg[i].stPreGammaCfg.stDynaRegCfg.u32UpdateIndex = au32UpdateIdx[i] + 1;
    }
}

static HI_S32 PreGammaReadExtRegs(VI_PIPE ViPipe)
{
    ISP_PREGAMMA_S *pstPreGammaCtx = HI_NULL;

    PREGAMMA_GET_CTX(ViPipe, pstPreGammaCtx);

    pstPreGammaCtx->bLutUpdate = hi_ext_system_pregamma_lut_update_read(ViPipe);
    hi_ext_system_pregamma_lut_update_write(ViPipe, HI_FALSE);

    return 0;
}

HI_S32 PreGammaProcWrite(VI_PIPE ViPipe, ISP_CTRL_PROC_WRITE_S *pstProc)
{
    ISP_CTRL_PROC_WRITE_S stProcTmp;
    ISP_PREGAMMA_S *pstPreGamma = HI_NULL;

    PREGAMMA_GET_CTX(ViPipe, pstPreGamma);

    if ((HI_NULL == pstProc->pcProcBuff)
        || (0 == pstProc->u32BuffLen))
    {
        return HI_FAILURE;
    }

    stProcTmp.pcProcBuff = pstProc->pcProcBuff;
    stProcTmp.u32BuffLen = pstProc->u32BuffLen;

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "-----PreGamma INFO--------------------------------------------------------------\n");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen, "%16s\n", "Enable");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen, "%16u\n", pstPreGamma->bEnable);

    pstProc->u32WriteLen += 1;

    return HI_SUCCESS;
}

static HI_BOOL __inline CheckPreGammaOpen(ISP_PREGAMMA_S *pstPreGamma)
{
    return (HI_TRUE == pstPreGamma->bEnable);
}

HI_S32 ISP_PreGammaRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                       HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_U16 i, j;
    ISP_PREGAMMA_S *pstPreGammaCtx = HI_NULL;
    ISP_REG_CFG_S *pstReg = (ISP_REG_CFG_S *)pRegCfg;
    ISP_PREGAMMA_DYNA_CFG_S   *pstDynaRegCfg = HI_NULL;
    PREGAMMA_GET_CTX(ViPipe, pstPreGammaCtx);

    pstPreGammaCtx->bEnable = hi_ext_system_pregamma_en_read(ViPipe);

    for (i = 0; i < pstReg->u8CfgNum; i++)
    {
        pstReg->stAlgRegCfg[i].stPreGammaCfg.bPreGammaEn = pstPreGammaCtx->bEnable;
    }

    pstReg->unKey.bit1PreGammaCfg = 1;

    /*check hardware setting*/
    if (!CheckPreGammaOpen(pstPreGammaCtx))
    {
        return HI_SUCCESS;
    }

    PreGammaReadExtRegs(ViPipe);

    if (pstPreGammaCtx->bLutUpdate)
    {
        for (i = 0; i < pstReg->u8CfgNum; i++)
        {
            pstDynaRegCfg = &pstReg->stAlgRegCfg[i].stPreGammaCfg.stDynaRegCfg;
            for (j = 0 ; j < PREGAMMA_NODE_NUMBER ; j++)
            {
                pstDynaRegCfg->u32PreGammaLUT[j] = hi_ext_system_pregamma_lut_read(ViPipe, j);
            }

            pstDynaRegCfg->bPreGammaLutUpdateEn = HI_TRUE;
            pstDynaRegCfg->u32UpdateIndex      += 1;
        }
    }

    return HI_SUCCESS;
}

HI_S32 ISP_PreGammaCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);
    switch (u32Cmd)
    {
        case ISP_WDR_MODE_SET :
            ISP_PreGammaWdrModeSet(ViPipe, (HI_VOID *)&pRegCfg->stRegCfg);
            break;
        case ISP_PROC_WRITE:
            PreGammaProcWrite(ViPipe, (ISP_CTRL_PROC_WRITE_S *)pValue);
            break;
        default :
            break;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_PreGammaExit(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    for (i = 0; i < pstIspCtx->stBlockAttr.u8BlockNum; i++)
    {
        isp_pregamma_en_write(ViPipe, i, HI_FALSE);
    }

    return HI_SUCCESS;
}


HI_S32 ISP_AlgRegisterPreGamma(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_PREGAMMA;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_PreGammaInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_PreGammaRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_PreGammaCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_PreGammaExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

