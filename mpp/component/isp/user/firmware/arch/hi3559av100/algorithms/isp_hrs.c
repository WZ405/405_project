/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_hrs.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2016/12/21
  Description   :
  History       :
  1.Date        : 2016/12/21
    Modification: Created file

******************************************************************************/

#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_config.h"
#include "isp_ext_config.h"
#include "isp_proc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

HI_S16 as16HRSFilterLut[2][6] =
{
    { -14, 23, 232, 23, -14 , 6},
    {10, -39, 157, 157, -39, 10}
};

static HI_VOID HrsRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8 j;
    ISP_CTX_S    *pstIspCtx = HI_NULL;
    ISP_HRS_STATIC_CFG_S   *pstHrsStaticRegCfg  = &pstRegCfg->stAlgRegCfg[0].stHrsRegCfg.stStaticRegCfg;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstHrsStaticRegCfg->u8RSEnable = (IS_HRS_ON(pstIspCtx->stBlockAttr.enIspRunningMode) ? HI_TRUE : HI_FALSE);
    pstHrsStaticRegCfg->u8Enable   = HI_TRUE;
    pstHrsStaticRegCfg->u16Height  = pstIspCtx->stBlockAttr.stFrameRect.u32Height;
    pstHrsStaticRegCfg->u16Width   = pstIspCtx->stBlockAttr.stFrameRect.u32Width;

    for (j = 0; j < 6; j++)
    {
        pstHrsStaticRegCfg->as16HRSFilterLut0[j] = as16HRSFilterLut[0][j];
        pstHrsStaticRegCfg->as16HRSFilterLut1[j] = as16HRSFilterLut[1][j];
    }

    pstHrsStaticRegCfg->bResh    = HI_TRUE;

    pstRegCfg->unKey.bit1HrsCfg = 1;

    return;
}

HI_S32 ISP_HrsInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    HrsRegsInitialize(ViPipe, pstRegCfg);

    return HI_SUCCESS;
}

HI_S32 ISP_HrsRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                  HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    return HI_SUCCESS;
}

HI_S32 ISP_HrsCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    switch (u32Cmd)
    {
        case ISP_WDR_MODE_SET :

            break;
        case ISP_PROC_WRITE:
            break;
        case ISP_CHANGE_IMAGE_MODE_SET:

            break;
        default :
            break;
    }
    return HI_SUCCESS;
}

HI_S32 ISP_HrsExit(VI_PIPE ViPipe)
{
    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterHrs(VI_PIPE ViPipe)
{
    ISP_CTX_S      *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs   = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_HRS;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_HrsInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_HrsRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_HrsCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_HrsExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


