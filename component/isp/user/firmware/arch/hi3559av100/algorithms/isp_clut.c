/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_clut.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       :
  Description   :
  History       :
  1.Date        :
    Author      :
    Modification: Created file

******************************************************************************/

#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_config.h"
#include "isp_ext_config.h"
#include "isp_proc.h"
#include "mpi_sys.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

static const  HI_U16 g_au16GainR = 128;
static const  HI_U16 g_au16GainG = 128;
static const  HI_U16 g_au16GainB = 128;

typedef struct hi_ISP_CLUT_CTX_S
{
    HI_BOOL         bClutLutUpdateEn;
    HI_BOOL         bClutCtrlUpdateEn;
    HI_U32          *pu32VirAddr;
    ISP_CLUT_ATTR_S stClutCtrl;
} ISP_CLUT_CTX_S;

ISP_CLUT_CTX_S g_astClutCtx[ISP_MAX_PIPE_NUM] = {{0}};

#define CLUT_GET_CTX(dev, pstCtx)   pstCtx = &g_astClutCtx[dev]

static HI_VOID ClutExtRegsInitialize(VI_PIPE ViPipe)
{
    ISP_CLUT_CTX_S *pstClutCtx = HI_NULL;
    CLUT_GET_CTX(ViPipe, pstClutCtx);

    hi_ext_system_clut_en_write(ViPipe, pstClutCtx->stClutCtrl.bEnable);
    hi_ext_system_clut_gainR_write(ViPipe, pstClutCtx->stClutCtrl.u32GainR);
    hi_ext_system_clut_gainG_write(ViPipe, pstClutCtx->stClutCtrl.u32GainG);
    hi_ext_system_clut_gainB_write(ViPipe, pstClutCtx->stClutCtrl.u32GainB);
    hi_ext_system_clut_ctrl_update_en_write(ViPipe, HI_FALSE);
    hi_ext_system_clut_lut_update_en_write(ViPipe, HI_FALSE);

    return;
}

static HI_VOID ClutUsrCoefRegsInitialize(VI_PIPE ViPipe, ISP_CLUT_USR_COEF_CFG_S *pstUsrCoefRegCfg)
{
    HI_U32 u32Offset = 0;
    ISP_CLUT_CTX_S  *pstClutCtx  = HI_NULL;
    CLUT_GET_CTX(ViPipe, pstClutCtx);

    memcpy(pstUsrCoefRegCfg->au32lut0, pstClutCtx->pu32VirAddr + u32Offset, HI_ISP_CLUT_LUT0 * sizeof(HI_U32));

    u32Offset += HI_ISP_CLUT_LUT0;
    memcpy(pstUsrCoefRegCfg->au32lut1, pstClutCtx->pu32VirAddr + u32Offset, HI_ISP_CLUT_LUT1 * sizeof(HI_U32));

    u32Offset += HI_ISP_CLUT_LUT1;
    memcpy(pstUsrCoefRegCfg->au32lut2, pstClutCtx->pu32VirAddr + u32Offset, HI_ISP_CLUT_LUT2 * sizeof(HI_U32));

    u32Offset += HI_ISP_CLUT_LUT2;
    memcpy(pstUsrCoefRegCfg->au32lut3, pstClutCtx->pu32VirAddr + u32Offset, HI_ISP_CLUT_LUT3 * sizeof(HI_U32));

    u32Offset += HI_ISP_CLUT_LUT3;
    memcpy(pstUsrCoefRegCfg->au32lut4, pstClutCtx->pu32VirAddr + u32Offset, HI_ISP_CLUT_LUT4 * sizeof(HI_U32));

    u32Offset += HI_ISP_CLUT_LUT4;
    memcpy(pstUsrCoefRegCfg->au32lut5, pstClutCtx->pu32VirAddr + u32Offset, HI_ISP_CLUT_LUT5 * sizeof(HI_U32));

    u32Offset += HI_ISP_CLUT_LUT5;
    memcpy(pstUsrCoefRegCfg->au32lut6, pstClutCtx->pu32VirAddr + u32Offset, HI_ISP_CLUT_LUT6 * sizeof(HI_U32));

    u32Offset += HI_ISP_CLUT_LUT6;
    memcpy(pstUsrCoefRegCfg->au32lut7, pstClutCtx->pu32VirAddr + u32Offset, HI_ISP_CLUT_LUT7 * sizeof(HI_U32));

    pstUsrCoefRegCfg->bResh          = HI_TRUE;
    pstUsrCoefRegCfg->u32UpdateIndex = 1;
}

static HI_VOID ClutUsrCtrlRegsInitialize(VI_PIPE ViPipe, ISP_CLUT_USR_CTRL_CFG_S *pstUsrCtrlRegCfg)
{
    pstUsrCtrlRegCfg->bDemoMode        = HI_FALSE;
    pstUsrCtrlRegCfg->u32GainR         = 128;
    pstUsrCtrlRegCfg->u32GainB         = 128;
    pstUsrCtrlRegCfg->u32GainG         = 128;
    pstUsrCtrlRegCfg->bDemoEnable      = HI_FALSE;
    pstUsrCtrlRegCfg->bResh            = HI_TRUE;

    return;
}
static HI_VOID ClutUsrRegsInitialize(VI_PIPE ViPipe, ISP_CLUT_USR_CFG_S *pstUsrRegCfg)
{
    ClutUsrCoefRegsInitialize(ViPipe, &pstUsrRegCfg->stClutUsrCoefCfg);
    ClutUsrCtrlRegsInitialize(ViPipe, &pstUsrRegCfg->stClutUsrCtrlCfg);

    return;
}

HI_S32 ClutRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8 i;
    ISP_CLUT_CTX_S *pstClutCtx = HI_NULL;
    CLUT_GET_CTX(ViPipe, pstClutCtx);

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        ClutUsrRegsInitialize(ViPipe, &pstRegCfg->stAlgRegCfg[i].stClutCfg.stUsrRegCfg);
        pstRegCfg->stAlgRegCfg[i].stClutCfg.bEnable = pstClutCtx->stClutCtrl.bEnable;
    }

    pstRegCfg->unKey.bit1ClutCfg   = 1;

    return HI_SUCCESS;
}

static HI_S32 ClutReadExtregs(VI_PIPE ViPipe)
{
    ISP_CLUT_CTX_S *pstCLUTCtx = HI_NULL;

    CLUT_GET_CTX(ViPipe, pstCLUTCtx);

    pstCLUTCtx->bClutCtrlUpdateEn = hi_ext_system_clut_ctrl_update_en_read(ViPipe);
    hi_ext_system_clut_ctrl_update_en_write(ViPipe, HI_FALSE);

    if (pstCLUTCtx->bClutCtrlUpdateEn)
    {
        pstCLUTCtx->stClutCtrl.bEnable    = hi_ext_system_clut_en_read(ViPipe);
        pstCLUTCtx->stClutCtrl.u32GainR   = hi_ext_system_clut_gainR_read(ViPipe);
        pstCLUTCtx->stClutCtrl.u32GainG   = hi_ext_system_clut_gainG_read(ViPipe);
        pstCLUTCtx->stClutCtrl.u32GainB   = hi_ext_system_clut_gainB_read(ViPipe);
    }

    pstCLUTCtx->bClutLutUpdateEn = hi_ext_system_clut_lut_update_en_read(ViPipe);
    hi_ext_system_clut_lut_update_en_write(ViPipe, HI_FALSE);

    return 0;
}

static HI_S32 ClutInitialize(VI_PIPE ViPipe)
{
    ISP_CLUT_BUF_S  stClutBuf;
    ISP_CLUT_CTX_S *pstClutCtx = HI_NULL;
    ISP_CMOS_DEFAULT_S *pstSnsDft = HI_NULL;

    CLUT_GET_CTX(ViPipe, pstClutCtx);
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);

    pstClutCtx->pu32VirAddr = HI_NULL;

    if (ioctl(g_as32IspFd[ViPipe], ISP_CLUT_BUF_GET, &stClutBuf.u64PhyAddr))
    {
        ISP_TRACE(HI_DBG_ERR, "FUNC:%s,Get Clut Buffer Err\n", __FUNCTION__);
        return HI_FAILURE;
    }

    stClutBuf.pVirAddr = HI_MPI_SYS_Mmap(stClutBuf.u64PhyAddr, HI_ISP_CLUT_LUT_LENGTH * sizeof(HI_U32));

    if ( !stClutBuf.pVirAddr )
    {
        return HI_FAILURE;
    }

    pstClutCtx->pu32VirAddr = (HI_U32 *)stClutBuf.pVirAddr;

    if (pstSnsDft->stClut.bValid)
    {
        pstClutCtx->stClutCtrl.bEnable  = pstSnsDft->stClut.bEnable;
        pstClutCtx->stClutCtrl.u32GainR = pstSnsDft->stClut.u32GainR;
        pstClutCtx->stClutCtrl.u32GainG = pstSnsDft->stClut.u32GainG;
        pstClutCtx->stClutCtrl.u32GainB = pstSnsDft->stClut.u32GainB;

        memcpy(pstClutCtx->pu32VirAddr, pstSnsDft->stClut.stClutLut.au32lut, HI_ISP_CLUT_LUT_LENGTH * sizeof(HI_U32));
    }
    else
    {
        pstClutCtx->stClutCtrl.bEnable  = HI_FALSE;
        pstClutCtx->stClutCtrl.u32GainR = g_au16GainR;
        pstClutCtx->stClutCtrl.u32GainG = g_au16GainG;
        pstClutCtx->stClutCtrl.u32GainB = g_au16GainB;

        memset(pstClutCtx->pu32VirAddr, 0, HI_ISP_CLUT_LUT_LENGTH * sizeof(HI_U32));
    }

    pstClutCtx->stClutCtrl.bEnable = pstSnsDft->stClut.bEnable;

    return HI_SUCCESS;
}

HI_VOID Isp_Clut_Usr_Coef_Fw(ISP_CLUT_CTX_S *pstCLUTCtx, ISP_CLUT_USR_COEF_CFG_S *pstClutUsrCoefCfg)
{
    HI_U32 u32Offset = 0;

    memcpy(pstClutUsrCoefCfg->au32lut0, pstCLUTCtx->pu32VirAddr + u32Offset, HI_ISP_CLUT_LUT0 * sizeof(HI_U32));

    u32Offset += HI_ISP_CLUT_LUT0;
    memcpy(pstClutUsrCoefCfg->au32lut1, pstCLUTCtx->pu32VirAddr + u32Offset, HI_ISP_CLUT_LUT1 * sizeof(HI_U32));

    u32Offset += HI_ISP_CLUT_LUT1;
    memcpy(pstClutUsrCoefCfg->au32lut2, pstCLUTCtx->pu32VirAddr + u32Offset, HI_ISP_CLUT_LUT2 * sizeof(HI_U32));

    u32Offset += HI_ISP_CLUT_LUT2;
    memcpy(pstClutUsrCoefCfg->au32lut3, pstCLUTCtx->pu32VirAddr + u32Offset, HI_ISP_CLUT_LUT3 * sizeof(HI_U32));

    u32Offset += HI_ISP_CLUT_LUT3;
    memcpy(pstClutUsrCoefCfg->au32lut4, pstCLUTCtx->pu32VirAddr + u32Offset, HI_ISP_CLUT_LUT4 * sizeof(HI_U32));

    u32Offset += HI_ISP_CLUT_LUT4;
    memcpy(pstClutUsrCoefCfg->au32lut5, pstCLUTCtx->pu32VirAddr + u32Offset, HI_ISP_CLUT_LUT5 * sizeof(HI_U32));

    u32Offset += HI_ISP_CLUT_LUT5;
    memcpy(pstClutUsrCoefCfg->au32lut6, pstCLUTCtx->pu32VirAddr + u32Offset, HI_ISP_CLUT_LUT6 * sizeof(HI_U32));

    u32Offset += HI_ISP_CLUT_LUT6;
    memcpy(pstClutUsrCoefCfg->au32lut7, pstCLUTCtx->pu32VirAddr + u32Offset, HI_ISP_CLUT_LUT7 * sizeof(HI_U32));

    pstClutUsrCoefCfg->bResh           = HI_TRUE;
    pstClutUsrCoefCfg->u32UpdateIndex += 1;
}

HI_VOID Isp_Clut_Usr_Ctrl_Fw(ISP_CLUT_ATTR_S *pstClutCtrl, ISP_CLUT_USR_CTRL_CFG_S *pstClutUsrCtrlCfg)
{
    pstClutUsrCtrlCfg->u32GainR       = pstClutCtrl->u32GainR;
    pstClutUsrCtrlCfg->u32GainG       = pstClutCtrl->u32GainG;
    pstClutUsrCtrlCfg->u32GainB       = pstClutCtrl->u32GainB;
    pstClutUsrCtrlCfg->bResh            = HI_TRUE;
}

static HI_BOOL __inline CheckClutOpen(ISP_CLUT_CTX_S *pstClutCtx)
{
    return (HI_TRUE == pstClutCtx->stClutCtrl.bEnable);
}

static HI_S32 ISP_ClutInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    if (HI_FAILURE == ClutInitialize(ViPipe))
    {
        printf("FUNC:%s failed!\n", __FUNCTION__);
        return HI_FAILURE;
    }

    ClutRegsInitialize(ViPipe, pstRegCfg);
    ClutExtRegsInitialize(ViPipe);

    return HI_SUCCESS;
}

static HI_S32 ISP_ClutRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                          HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_U8 i;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_CLUT_CTX_S *pstCLUTCtx = HI_NULL;
    ISP_REG_CFG_S *pstReg = (ISP_REG_CFG_S *)pRegCfg;

    CLUT_GET_CTX(ViPipe, pstCLUTCtx);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    /* calculate every two interrupts */
    if ((0 != pstIspCtx->u32FrameCnt % 2) && (HI_TRUE != pstIspCtx->stLinkage.bSnapState))
    {
        return HI_SUCCESS;
    }

    pstCLUTCtx->stClutCtrl.bEnable = hi_ext_system_clut_en_read(ViPipe);

    for (i = 0; i < pstReg->u8CfgNum; i++)
    {
        pstReg->stAlgRegCfg[i].stClutCfg.bEnable = pstCLUTCtx->stClutCtrl.bEnable;
    }

    pstReg->unKey.bit1ClutCfg = 1;

    /*check hardware setting*/
    if (!CheckClutOpen(pstCLUTCtx))
    {
        return HI_SUCCESS;
    }

    ClutReadExtregs(ViPipe);

    if (pstCLUTCtx->bClutCtrlUpdateEn)
    {
        for (i = 0; i < pstReg->u8CfgNum; i++)
        {
            Isp_Clut_Usr_Ctrl_Fw(&pstCLUTCtx->stClutCtrl, &pstReg->stAlgRegCfg[i].stClutCfg.stUsrRegCfg.stClutUsrCtrlCfg);
        }
    }

    if (pstCLUTCtx->bClutLutUpdateEn)
    {
        for (i = 0; i < pstReg->u8CfgNum; i++)
        {
            if (!pstCLUTCtx->pu32VirAddr)
            {
                return HI_FAILURE;
            }

            Isp_Clut_Usr_Coef_Fw(pstCLUTCtx, &pstReg->stAlgRegCfg[i].stClutCfg.stUsrRegCfg.stClutUsrCoefCfg);
        }
    }

    return HI_SUCCESS;
}
static HI_S32 ISP_ClutCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    switch (u32Cmd)
    {
        default :
            break;
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_ClutExit(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_CTX_S      *pstIspCtx  = HI_NULL;
    ISP_CLUT_CTX_S *pstClutCtx = HI_NULL;

    CLUT_GET_CTX(ViPipe, pstClutCtx);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    for (i = 0; i < pstIspCtx->stBlockAttr.u8BlockNum; i++)
    {
        isp_clut_en_write(ViPipe, i, HI_FALSE);
    }

    HI_MPI_SYS_Munmap((HI_VOID *)pstClutCtx->pu32VirAddr, HI_ISP_CLUT_LUT_LENGTH * sizeof(HI_U32));

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterClut(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_CLUT;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_ClutInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_ClutRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_ClutCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_ClutExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

