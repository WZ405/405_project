/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_alg.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/04/23
  Description   :
  History       :
  1.Date        : 2013/04/23
    Author      :
    Modification: Created file

******************************************************************************/
#ifndef __ISP_AF_H__
#define __ISP_AF_H__

#include <stdlib.h>
#include <string.h>
#include "hi_common.h"
#include "hi_comm_isp.h"
#include "isp_main.h"
#include "hi_math.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

HI_S32 ISP_AlgRegisterAe(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterAwb(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterAf(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterDp(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterGe(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterFrameWDR(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterBayernr(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterFlicker(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterHrs(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterDg(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterFeLsc(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterLsc(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterRLsc(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterRc(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterDrc(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterDehaze(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterLCac(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterGCac(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterSplit(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterDemosaic(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterFcr(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterGamma(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterCsc(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterSharpen(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterEdgeMark(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterMcds(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterDci(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterLdci(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterCa(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterFPN(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterPreGamma(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterBlc(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterFeLogLUT(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterLogLUT(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterClut(VI_PIPE ViPipe);
HI_S32 ISP_AlgRegisterHlc(VI_PIPE ViPipe);

/* find the specified library */
static inline HI_S32 ISP_FindLib(const ISP_LIB_NODE_S *astLibs,
                                 const ALG_LIB_S *pstLib)
{
    HI_S32  i;

    for (i = 0; i < MAX_REGISTER_ALG_LIB_NUM; i++)
    {
        /* can't use memcmp, if user not fill the string. */
        if ((0 == strcmp(astLibs[i].stAlgLib.acLibName, pstLib->acLibName))
            && (astLibs[i].stAlgLib.s32Id == pstLib->s32Id))
        {
            return i;
        }
    }

    return -1;
}

/* search the empty pos of libs */
static inline ISP_LIB_NODE_S *ISP_SearchLib(ISP_LIB_NODE_S *astLibs)
{
    HI_S32 i;

    for (i = 0; i < MAX_REGISTER_ALG_LIB_NUM; i++)
    {
        if (!astLibs[i].bUsed)
        {
            return &astLibs[i];
        }
    }

    return HI_NULL;
}


/* search the empty pos of algs */
static inline ISP_ALG_NODE_S *ISP_SearchAlg(ISP_ALG_NODE_S *astAlgs)
{
    HI_S32 i;

    for (i = 0; i < ISP_MAX_ALGS_NUM; i++)
    {
        if (!astAlgs[i].bUsed)
        {
            return &astAlgs[i];
        }
    }

    return HI_NULL;
}

static inline HI_VOID ISP_AlgsInit(const ISP_ALG_NODE_S *astAlgs, VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    HI_S32 i;

    for (i = 0; i < ISP_MAX_ALGS_NUM; i++)
    {
        if (astAlgs[i].bUsed)
        {
            if (HI_NULL != astAlgs[i].stAlgFunc.pfn_alg_init)
            {
                astAlgs[i].stAlgFunc.pfn_alg_init(ViPipe, pRegCfg);
            }
        }
    }

    return;
}

static inline HI_VOID ISP_AlgsRun(const ISP_ALG_NODE_S *astAlgs, VI_PIPE ViPipe,
                                  const HI_VOID *pStatInfo, HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_S32 i;

    for (i = 0; i < ISP_MAX_ALGS_NUM; i++)
    {
        if (astAlgs[i].bUsed)
        {
            if (HI_NULL != astAlgs[i].stAlgFunc.pfn_alg_run)
            {
                astAlgs[i].stAlgFunc.pfn_alg_run(ViPipe, pStatInfo, pRegCfg, s32Rsv);
            }
        }
    }

    return;
}

static inline HI_VOID ISP_AlgsCtrl(const ISP_ALG_NODE_S *astAlgs,
                                   VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    HI_S32 i;

    for (i = 0; i < ISP_MAX_ALGS_NUM; i++)
    {
        if (astAlgs[i].bUsed)
        {
            if (HI_NULL != astAlgs[i].stAlgFunc.pfn_alg_ctrl)
            {
                astAlgs[i].stAlgFunc.pfn_alg_ctrl(ViPipe, u32Cmd, pValue);
            }
        }
    }

    return;
}

static inline HI_VOID ISP_AlgsExit(const ISP_ALG_NODE_S *astAlgs, VI_PIPE ViPipe)
{
    HI_S32 i;

    for (i = 0; i < ISP_MAX_ALGS_NUM; i++)
    {
        if (astAlgs[i].bUsed)
        {
            if (HI_NULL != astAlgs[i].stAlgFunc.pfn_alg_exit)
            {
                astAlgs[i].stAlgFunc.pfn_alg_exit(ViPipe);
            }
        }
    }

    return;
}

static inline HI_VOID ISP_AlgsRegister(VI_PIPE ViPipe)
{
    ISP_AlgRegisterAe(ViPipe);
    ISP_AlgRegisterAwb(ViPipe);
    ISP_AlgRegisterAf(ViPipe);
    ISP_AlgRegisterDp(ViPipe);
    ISP_AlgRegisterGe(ViPipe);
    ISP_AlgRegisterFrameWDR(ViPipe);
    ISP_AlgRegisterBlc(ViPipe);
    ISP_AlgRegisterBayernr(ViPipe);
    ISP_AlgRegisterSplit(ViPipe);
    ISP_AlgRegisterFlicker(ViPipe);
    ISP_AlgRegisterDg(ViPipe);
    ISP_AlgRegisterHrs(ViPipe);
    ISP_AlgRegisterFeLsc(ViPipe);
    ISP_AlgRegisterLsc(ViPipe);
    ISP_AlgRegisterRLsc(ViPipe);
    ISP_AlgRegisterRc(ViPipe);
    ISP_AlgRegisterDrc(ViPipe);
    ISP_AlgRegisterDehaze(ViPipe);
    ISP_AlgRegisterLCac(ViPipe);
    ISP_AlgRegisterGCac(ViPipe);
    ISP_AlgRegisterDemosaic(ViPipe);
    ISP_AlgRegisterFcr(ViPipe);
    ISP_AlgRegisterGamma(ViPipe);
    ISP_AlgRegisterCsc(ViPipe);
    ISP_AlgRegisterCa(ViPipe);
    ISP_AlgRegisterFPN(ViPipe);
    ISP_AlgRegisterSharpen(ViPipe);
    ISP_AlgRegisterEdgeMark(ViPipe);
    ISP_AlgRegisterMcds(ViPipe);
    ISP_AlgRegisterLdci(ViPipe);
    ISP_AlgRegisterPreGamma(ViPipe);
    ISP_AlgRegisterFeLogLUT(ViPipe);
    ISP_AlgRegisterLogLUT(ViPipe);
    ISP_AlgRegisterClut(ViPipe);
    ISP_AlgRegisterHlc(ViPipe);

    return;
}

/* resolve problem:
isp error: Null Pointer in ISP_AlgRegisterxxx when rerun isp_init without app exit */
static inline HI_VOID ISP_AlgsUnRegister(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    HI_S32 i;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    for (i = 0; i < ISP_MAX_ALGS_NUM; i++)
    {
        if (pstIspCtx->astAlgs[i].bUsed)
        {
            pstIspCtx->astAlgs[i].bUsed = HI_FALSE;
        }
    }
}

/* resolve problem:
isp error: HI_MPI_XX_Register failed when rerun 3a lib register without app exit */
static inline HI_VOID ISP_LibsUnRegister(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    HI_S32 i;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    for (i = 0; i < MAX_REGISTER_ALG_LIB_NUM; i++)
    {
        pstIspCtx->stAeLibInfo.astLibs[i].bUsed  = HI_FALSE;
        pstIspCtx->stAwbLibInfo.astLibs[i].bUsed = HI_FALSE;
        pstIspCtx->stAfLibInfo.astLibs[i].bUsed  = HI_FALSE;
    }
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif
