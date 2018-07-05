/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_hlc.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/01/16
  Description   :
  History       :
  1.Date        : 2017/12/16
    Author      :
    Modification: Created file

******************************************************************************/
#include "isp_config.h"
#include "hi_isp_debug.h"
#include "isp_ext_config.h"
#include "isp_math_utils.h"

#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_proc.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

HI_S32 ISP_HlcInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    return HI_SUCCESS;
}

HI_S32 ISP_HlcRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo, HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    return HI_SUCCESS;
}


HI_S32 ISP_HlcCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    return HI_SUCCESS;
}

HI_S32 ISP_HlcExit(VI_PIPE ViPipe)
{
    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterHlc(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType =  ISP_ALG_EDGEAMRK;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_HlcInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_HlcRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_HlcCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_HlcExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */



