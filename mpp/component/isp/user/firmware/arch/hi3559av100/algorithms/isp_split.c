/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_split.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2016/01/16
  Description   :
  History       :
  1.Date        : 2016/01/16
    Author      :
    Modification: Created file

******************************************************************************/

#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_config.h"
#include "isp_proc.h"
#include "isp_math_utils.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */
/***************************************************************************
ModeIn:       0=linear; 1=2~3mux;2=16LOG;3=sensor-build-in
ModeOut:     0=1 channel;1=2 channel;2=3 channel;3=4 chnnel
Inputwidthselect:  0=12bit;  1=14bit;  2=16bit; 3=20bit
****************************************************************************/
static HI_U8 SplitGetBitInWidth(HI_U8 u8InputWidthSel)
{
    switch (u8InputWidthSel)
    {
        case 0:
            return 12;
        case 1:
            return 14;
        case 2:
            return 16;
        case 3:
            return 20;
        default :
            return 12;
    }
}
#if 0
static HI_U8 SplitGetModeOut(HI_U8 u8WDRMode)
{
    switch (u8WDRMode)
    {
        case WDR_MODE_1To1_SPLIT:
            return 0;
        case WDR_MODE_1To2_SPLIT:
            return 1;
        case WDR_MODE_1To3_SPLIT:
            return 2;
        case WDR_MODE_2To3_SPLIT:
            return 2;
        default :
            return 0;
    }
}
#endif
static HI_VOID SplitStaticRegsInitialize(VI_PIPE ViPipe, ISP_SPLIT_STATIC_CFG_S *pstStaticRegCfg, ISP_CMOS_SPLIT_S *pstSnsSplit, HI_U8 u8WDRMode)
{
    HI_U32 _i, _v;
    HI_U32 X0, Y0, X1, Y1, X2, Y2, X3, Y3, X_max, Y_max;

    if (pstSnsSplit->bValid)
    {
        pstStaticRegCfg->u8BitDepthIn = SplitGetBitInWidth(pstSnsSplit->u8InputWidthSel);
        pstStaticRegCfg->u8BitDepthOut = pstSnsSplit->u32BitDepthOut;
        pstStaticRegCfg->u8ModeIn     = pstSnsSplit->u8ModeIn;

        X0       = pstSnsSplit->astSplitPoint[0].u8X;
        Y0       = pstSnsSplit->astSplitPoint[0].u16Y;
        X1       = pstSnsSplit->astSplitPoint[1].u8X;
        Y1       = pstSnsSplit->astSplitPoint[1].u16Y;
        X2       = pstSnsSplit->astSplitPoint[2].u8X;
        Y2       = pstSnsSplit->astSplitPoint[2].u16Y;
        X3       = pstSnsSplit->astSplitPoint[3].u8X;
        Y3       = pstSnsSplit->astSplitPoint[3].u16Y;
        X_max    = pstSnsSplit->astSplitPoint[4].u8X;
        Y_max    = pstSnsSplit->astSplitPoint[4].u16Y;

        for (_i = 0; _i < X0; _i++)
        {
            _v = (((_i * Y0) / DIV_0_TO_1(X0)) >> 0);
            pstStaticRegCfg->au16WdrSplitLut[_i] = _v;
        }

        for (; _i < X1; _i++)
        {
            _v = ((((_i - X0) * (Y1 - Y0)) / DIV_0_TO_1(X1 - X0) + Y0)  >> 0);
            pstStaticRegCfg->au16WdrSplitLut[_i] = _v;
        }

        for (; _i < X2; _i++)
        {
            _v = ((((_i - X1) * (Y2 - Y1)) / DIV_0_TO_1(X2 - X1) + Y1)  >> 0);
            pstStaticRegCfg->au16WdrSplitLut[_i] = _v;
        }

        for (; _i < X3; _i++)
        {
            _v = ((((_i - X2) * (Y3 - Y2)) / DIV_0_TO_1(X3 - X2) + Y2)  >> 0);
            pstStaticRegCfg->au16WdrSplitLut[_i] = _v;
        }

        for (; _i < X_max; _i++)
        {
            _v = (Y_max  >> 0);
            pstStaticRegCfg->au16WdrSplitLut[_i] = _v;
        }
    }
    else
    {
        pstStaticRegCfg->u8BitDepthIn  = 16;
        pstStaticRegCfg->u8BitDepthOut = 16;
        pstStaticRegCfg->u8ModeIn      = 0;
        memset(pstStaticRegCfg->au16WdrSplitLut, 0, 129 * sizeof(HI_U16));
    }

    pstStaticRegCfg->u8ModeOut = pstSnsSplit->u8ModeOut;//SplitGetModeOut(u8WDRMode);
    pstStaticRegCfg->bResh     = HI_TRUE;
}

static HI_VOID SplitRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8     i;
    ISP_CTX_S          *pstIspCtx = HI_NULL;
    ISP_CMOS_DEFAULT_S *pstSnsDft = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        SplitStaticRegsInitialize(ViPipe, &pstRegCfg->stAlgRegCfg[i].stSplitCfg.stStaticRegCfg, &pstSnsDft->stSplit, pstIspCtx->u8SnsWDRMode);

        if (pstSnsDft->stSplit.bValid)
        {
            pstRegCfg->stAlgRegCfg[0].stSplitCfg.bEnable = pstSnsDft->stSplit.bEnable;
        }
        else
        {
            pstRegCfg->stAlgRegCfg[0].stSplitCfg.bEnable = HI_FALSE;
        }
    }

    pstRegCfg->unKey.bit1SplitCfg = 1;

    return;
}

HI_S32 ISP_SplitInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    SplitRegsInitialize(ViPipe, pstRegCfg);

    return HI_SUCCESS;
}

HI_S32 ISP_SplitRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                    HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    return HI_SUCCESS;
}

HI_S32 ISP_SplitCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    switch (u32Cmd)
    {
        default :
            break;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_SplitExit(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);

    for (i = 0; i < pRegCfg->stRegCfg.u8CfgNum; i++)
    {
        pRegCfg->stRegCfg.stAlgRegCfg[i].stSplitCfg.bEnable = HI_FALSE;
    }

    pRegCfg->stRegCfg.unKey.bit1SplitCfg = 1;

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterSplit(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_SPLIT;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_SplitInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_SplitRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_SplitCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_SplitExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


