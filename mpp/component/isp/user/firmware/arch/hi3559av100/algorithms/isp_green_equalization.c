/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_green_equalization.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/04/24
  Description   :
  History       :
  1.Date        : 2016/09/20
    Modification: Created file

******************************************************************************/
#include <math.h>
#include "isp_alg.h"
#include "isp_config.h"
#include "isp_ext_config.h"
#include "isp_sensor.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define HI_ISP_GE_MAX_BLK_NUM_X (17)
#define HI_ISP_GE_MAX_BLK_NUM_Y (15)
#define HI_MINIISP_BITDEPTH     (14)
#define ISP_GE_SLOPE_DEFAULT (9)
#define ISP_GE_SENSI_SLOPE_DEFAULT (9)
#define ISP_GE_SENSI_THR_DEFAULT (300)

static const  HI_U16 g_au16Threshold[ISP_AUTO_ISO_STRENGTH_NUM] = { 300, 300, 300, 300, 310, 310, 310,  310,  320, 320, 320, 320, 330, 330, 330, 330};
static const  HI_U16 g_au16Strength[ISP_AUTO_ISO_STRENGTH_NUM]  = { 128, 128, 128, 128, 129, 129, 129,  129,  130, 130, 130, 130, 131, 131, 131, 131};
static const  HI_U16 g_au16NpOffset[ISP_AUTO_ISO_STRENGTH_NUM]  = {1024, 1024, 1024, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048};
static const  HI_U32 g_au32GeIsoLut[ISP_AUTO_ISO_STRENGTH_NUM]  = { 100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600, 51200, 102400, 204800, 409600, 819200, 1638400, 3276800};

typedef struct hiISP_GE_S
{
    HI_BOOL bEnable;
    HI_BOOL bCoefUpdateEn;
    HI_U8   u8GeChnNum;

    HI_S32  grgb_w;
    HI_S32  grgb_h;
    HI_U32  bitDepth;
    ISP_CMOS_GE_S stCmosGe;
} ISP_GE_S;

ISP_GE_S g_astGeCtx[ISP_MAX_PIPE_NUM] = {{0}};
#define GE_GET_CTX(dev, pstCtx)   pstCtx = &g_astGeCtx[dev]

static HI_VOID GeInitialize(VI_PIPE ViPipe)
{
    ISP_GE_S   *pstGe = HI_NULL;
    ISP_CMOS_DEFAULT_S  *pstSnsDft = HI_NULL;

    GE_GET_CTX(ViPipe, pstGe);
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);

    if (pstSnsDft->stGe.bValid)
    {
        memcpy(&pstGe->stCmosGe, &pstSnsDft->stGe, sizeof(ISP_CMOS_GE_S));
    }
    else
    {
        pstGe->stCmosGe.bEnable      = HI_TRUE;
        pstGe->stCmosGe.u8Slope      = ISP_GE_SLOPE_DEFAULT;
        pstGe->stCmosGe.u8SensiSlope = ISP_GE_SENSI_SLOPE_DEFAULT;
        pstGe->stCmosGe.u16SensiThr  = ISP_GE_SENSI_THR_DEFAULT;
        memcpy(pstGe->stCmosGe.au16Strength, g_au16Strength, ISP_AUTO_ISO_STRENGTH_NUM * sizeof(HI_U16));
        memcpy(pstGe->stCmosGe.au16Threshold, g_au16Threshold, ISP_AUTO_ISO_STRENGTH_NUM * sizeof(HI_U16));
        memcpy(pstGe->stCmosGe.au16NpOffset, g_au16NpOffset, ISP_AUTO_ISO_STRENGTH_NUM * sizeof(HI_U16));
    }

    pstGe->bEnable = pstGe->stCmosGe.bEnable;

    pstGe->grgb_w     = HI_ISP_GE_MAX_BLK_NUM_X;
    pstGe->grgb_h     = HI_ISP_GE_MAX_BLK_NUM_Y;

    pstGe->bitDepth   = HI_MINIISP_BITDEPTH;
}

static HI_VOID GeExtRegsInitialize(VI_PIPE ViPipe)
{
    HI_U32 i;
    ISP_GE_S *pstGe = HI_NULL;

    GE_GET_CTX(ViPipe, pstGe);

    /* initial ext register of Ge */
    hi_ext_system_ge_enable_write(ViPipe, pstGe->bEnable);
    hi_ext_system_ge_slope_write(ViPipe, pstGe->stCmosGe.u8Slope);
    hi_ext_system_ge_sensitivity_write(ViPipe, pstGe->stCmosGe.u8SensiSlope);
    hi_ext_system_ge_sensithreshold_write(ViPipe, pstGe->stCmosGe.u16SensiThr);
    hi_ext_system_ge_coef_update_en_write(ViPipe, HI_TRUE);

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        hi_ext_system_ge_strength_write(ViPipe, i, pstGe->stCmosGe.au16Strength[i]);
        hi_ext_system_ge_npoffset_write(ViPipe, i, pstGe->stCmosGe.au16NpOffset[i]);
        hi_ext_system_ge_threshold_write(ViPipe, i, pstGe->stCmosGe.au16Threshold[i]);
    }

    return;
}

HI_U8 GeGetChnNum(HI_U8 u8WDRMode)
{
    HI_U8 u8ChnNum;

    if (IS_LINEAR_MODE(u8WDRMode))
    {
        u8ChnNum = 1;
    }
    else if (IS_BUILT_IN_WDR_MODE(u8WDRMode))
    {
        u8ChnNum = 1;
    }
    else if (IS_2to1_WDR_MODE(u8WDRMode))
    {
        u8ChnNum = 2;
    }
    else if (IS_3to1_WDR_MODE(u8WDRMode))
    {
        u8ChnNum = 3;
    }
    else if (IS_4to1_WDR_MODE(u8WDRMode))
    {
        u8ChnNum = 4;
    }
    else
    {
        /* unknow u8Mode */
        u8ChnNum = 1;
    }

    return u8ChnNum;
}

static HI_VOID GeStaticRegsInitialize(VI_PIPE ViPipe, ISP_GE_STATIC_CFG_S *pstGeStaticRegCfg)
{
    ISP_GE_S   *pstGe = HI_NULL;

    GE_GET_CTX(ViPipe, pstGe);

    pstGeStaticRegCfg->bGeGrGbEn           = HI_TRUE;
    pstGeStaticRegCfg->bGeGrEn             = HI_FALSE;
    pstGeStaticRegCfg->bGeGbEn             = HI_FALSE;

    pstGeStaticRegCfg->u8GeNumV    = pstGe->grgb_h;

    pstGeStaticRegCfg->bStaticResh = HI_TRUE;
}

static HI_VOID GeImageSize(VI_PIPE ViPipe, HI_U8 i, ISP_GE_USR_CFG_S *pstUsrRegCfg, ISP_CTX_S *pstIspCtx)
{
    HI_U8      u8BlkNum = pstIspCtx->stBlockAttr.u8BlockNum;
    HI_U16     u16Overlap;
    ISP_RECT_S stBlockRect;
    ISP_GE_S   *pstGe = HI_NULL;

    GE_GET_CTX(ViPipe, pstGe);

    u16Overlap = pstIspCtx->stBlockAttr.u32OverLap;
    if (i < (pstGe->grgb_w % DIV_0_TO_1(u8BlkNum)))
    {
        pstUsrRegCfg->u8GeNumH = pstGe->grgb_w / DIV_0_TO_1(u8BlkNum) + 1;
    }
    else
    {
        pstUsrRegCfg->u8GeNumH = pstGe->grgb_w / DIV_0_TO_1(u8BlkNum);
    }

    ISP_GetBlockRect(&stBlockRect, &pstIspCtx->stBlockAttr, i);

    pstUsrRegCfg->u16GeCropPosY      = 0;
    pstUsrRegCfg->u16GeCropOutHeight = stBlockRect.u32Height;

    if (i == 0)
    {
        pstUsrRegCfg->u16GeCropPosX      = 0;
        pstUsrRegCfg->u16GeCropOutWidth  = stBlockRect.u32Width - u16Overlap;
    }
    else if (i == (u8BlkNum - 1))
    {
        pstUsrRegCfg->u16GeCropPosX      = u16Overlap;
        pstUsrRegCfg->u16GeCropOutWidth  = stBlockRect.u32Width - u16Overlap;
    }
    else
    {
        pstUsrRegCfg->u16GeCropPosX      = u16Overlap;
        pstUsrRegCfg->u16GeCropOutWidth  = stBlockRect.u32Width - (u16Overlap << 1);
    }
}

static HI_VOID GeUsrRegsInitialize(VI_PIPE ViPipe, HI_U8 i, HI_U8 u8ChnNum, ISP_GE_USR_CFG_S *pstGeUsrRegCfg, ISP_CTX_S *pstIspCtx)
{
    HI_U8 j;

    for (j = 0; j < u8ChnNum; j++)
    {
        pstGeUsrRegCfg->au16GeCtTh2[j]   = HI_ISP_GE_SENSITHR_DEFAULT;
        pstGeUsrRegCfg->au8GeCtSlope1[j] = HI_ISP_GE_SENSISLOPE_DEFAULT;
        pstGeUsrRegCfg->au8GeCtSlope2[j] = HI_ISP_GE_SLOPE_DEFAULT;
    }

    GeImageSize(ViPipe, i, pstGeUsrRegCfg, pstIspCtx);
    pstGeUsrRegCfg->bResh = HI_TRUE;
}

static HI_VOID GeDynaRegsInitialize(HI_U8 u8ChnNum, ISP_GE_DYNA_CFG_S *pstGeDynaRegCfg)
{
    HI_U8 i;

    for (i = 0; i < u8ChnNum; i++)
    {
        pstGeDynaRegCfg->au16GeCtTh1[i] = HI_ISP_GE_NPOFFSET_DEFAULT;
        pstGeDynaRegCfg->au16GeCtTh3[i] = HI_ISP_GE_THRESHOLD_DEFAULT;
    }
    pstGeDynaRegCfg->u16GeStrength = HI_ISP_GE_STRENGTH_DEFAULT;
    pstGeDynaRegCfg->bResh         = HI_TRUE;
}

static HI_VOID GeRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8      i, j;
    ISP_CTX_S  *pstIspCtx    = HI_NULL;
    ISP_GE_S   *pstGe = HI_NULL;
    HI_U8       u8GeChnNum;

    GE_GET_CTX(ViPipe, pstGe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    u8GeChnNum = GeGetChnNum(pstIspCtx->u8SnsWDRMode);
    pstGe->u8GeChnNum = u8GeChnNum;

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        GeStaticRegsInitialize(ViPipe, &pstRegCfg->stAlgRegCfg[i].stGeRegCfg.stStaticRegCfg);
        GeUsrRegsInitialize(ViPipe, i, u8GeChnNum, &pstRegCfg->stAlgRegCfg[i].stGeRegCfg.stUsrRegCfg, pstIspCtx);
        GeDynaRegsInitialize(u8GeChnNum, &pstRegCfg->stAlgRegCfg[i].stGeRegCfg.stDynaRegCfg);
        pstRegCfg->stAlgRegCfg[i].stGeRegCfg.u8ChnNum = u8GeChnNum;

        for (j = 0; j < u8GeChnNum; j++)
        {
            pstRegCfg->stAlgRegCfg[i].stGeRegCfg.abGeEn[j] = pstGe->bEnable;
        }

        for (j = u8GeChnNum; j < 4; j++)
        {
            pstRegCfg->stAlgRegCfg[i].stGeRegCfg.abGeEn[j] = HI_FALSE;
        }

    }

    pstRegCfg->unKey.bit1GeCfg = 1;

    return;
}

static HI_S32 _linearinter (HI_S32 v, HI_S32 x0, HI_S32 x1, HI_S32 y0, HI_S32 y1)
{
    if (v <= x0)
    {
        return y0;
    }
    if (v >= x1)
    {
        return y1;
    }

    if (y1 >= y0)
    {
        return  (y0 + (y1 - y0) * (v - x0) / DIV_0_TO_1(x1 - x0)); // 0 protect
    }
    else
    {
        return  (y1 + (y0 - y1) * (x1 - v) / DIV_0_TO_1(x1 - x0)); // 0 protect
    }
}

static HI_U8 GeGetIsoIndex(HI_U32 u32Iso)
{
    HI_U8 u8Index;

    for (u8Index = 0; u8Index < ISP_AUTO_ISO_STRENGTH_NUM - 1; u8Index++)
    {
        if (u32Iso <= g_au32GeIsoLut[u8Index])
        {
            break;
        }
    }

    return u8Index;
}

static HI_U16 GeGetStrength(HI_U32 u32Iso, ISP_GE_S *pstGe)
{
    HI_U8 u8Index = GeGetIsoIndex(u32Iso);

    if (0 == u8Index
        || (ISP_AUTO_ISO_STRENGTH_NUM - 1) == u8Index)
    {
        return pstGe->stCmosGe.au16Strength[u8Index];
    }

    return (HI_U16)_linearinter(u32Iso, g_au32GeIsoLut[u8Index - 1], g_au32GeIsoLut[u8Index], pstGe->stCmosGe.au16Strength[u8Index - 1], pstGe->stCmosGe.au16Strength[u8Index]);
}

static HI_U16 GeGetNpOffset(HI_U32 u32Iso, ISP_GE_S *pstGe)
{
    HI_U8 u8Index = GeGetIsoIndex(u32Iso);

    if (0 == u8Index
        || (ISP_AUTO_ISO_STRENGTH_NUM - 1) == u8Index)
    {
        return pstGe->stCmosGe.au16NpOffset[u8Index];
    }

    return (HI_U16)_linearinter(u32Iso, g_au32GeIsoLut[u8Index - 1], g_au32GeIsoLut[u8Index], pstGe->stCmosGe.au16NpOffset[u8Index - 1], pstGe->stCmosGe.au16NpOffset[u8Index]);
}

static HI_U16 GeGetThreshold(HI_U32 u32Iso, ISP_GE_S *pstGe)
{
    HI_U8 u8Index = GeGetIsoIndex(u32Iso);

    if (0 == u8Index
        || (ISP_AUTO_ISO_STRENGTH_NUM - 1) == u8Index)
    {
        return pstGe->stCmosGe.au16Threshold[u8Index];
    }

    return (HI_U16)_linearinter(u32Iso, g_au32GeIsoLut[u8Index - 1], g_au32GeIsoLut[u8Index], pstGe->stCmosGe.au16Threshold[u8Index - 1], pstGe->stCmosGe.au16Threshold[u8Index]);
}

static HI_S32 GeReadExtregs(VI_PIPE ViPipe)
{
    HI_U32 i;
    ISP_GE_S *pstGe = HI_NULL;

    GE_GET_CTX(ViPipe, pstGe);

    /* read ext register of Ge */
    pstGe->bCoefUpdateEn         = hi_ext_system_ge_coef_update_en_read(ViPipe);
    hi_ext_system_ge_coef_update_en_write(ViPipe, HI_FALSE);

    if (pstGe->bCoefUpdateEn)
    {
        pstGe->stCmosGe.u8Slope      = hi_ext_system_ge_slope_read(ViPipe);
        pstGe->stCmosGe.u8SensiSlope = hi_ext_system_ge_sensitivity_read(ViPipe);
        pstGe->stCmosGe.u16SensiThr  = hi_ext_system_ge_sensithreshold_read(ViPipe);

        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
        {
            pstGe->stCmosGe.au16Strength[i] = hi_ext_system_ge_strength_read (ViPipe, i);
            pstGe->stCmosGe.au16NpOffset[i] = hi_ext_system_ge_npoffset_read (ViPipe, i);
            pstGe->stCmosGe.au16Threshold[i] = hi_ext_system_ge_threshold_read(ViPipe, i);
        }
    }

    return HI_SUCCESS;
}

HI_VOID Ge_Usr_Fw(ISP_GE_S *pstGe, ISP_GE_USR_CFG_S *pstGeUsrRegCfg)
{
    HI_U8 j;

    for (j = 0; j < pstGe->u8GeChnNum; j++)
    {
        pstGeUsrRegCfg->au16GeCtTh2[j]   = MIN2(pstGe->stCmosGe.u16SensiThr, (1 << pstGe->bitDepth));
        pstGeUsrRegCfg->au8GeCtSlope1[j] = MIN2(pstGe->stCmosGe.u8SensiSlope, pstGe->bitDepth);
        pstGeUsrRegCfg->au8GeCtSlope2[j] = MIN2(pstGe->stCmosGe.u8Slope, pstGe->bitDepth);
    }

    pstGeUsrRegCfg->bResh    = HI_TRUE;
}

HI_VOID Ge_Dyna_Fw(HI_U32 u32Iso, ISP_GE_S *pstGe, ISP_GE_DYNA_CFG_S *pstGeDynaRegCfg)
{
    HI_U8 i;

    for (i = 0; i < pstGe->u8GeChnNum; i++)
    {
        pstGeDynaRegCfg->au16GeCtTh3[i] = CLIP3((HI_S32)GeGetThreshold(u32Iso, pstGe), 0, (1 << pstGe->bitDepth));
        pstGeDynaRegCfg->au16GeCtTh1[i] = GeGetNpOffset(u32Iso, pstGe);
    }

    pstGeDynaRegCfg->u16GeStrength  = GeGetStrength(u32Iso, pstGe);
    pstGeDynaRegCfg->bResh          = HI_TRUE;
}

static HI_BOOL __inline CheckGeOpen(ISP_GE_S *pstGe)
{
    return (HI_TRUE == pstGe->bEnable);
}

HI_VOID Ge_Bypass(ISP_REG_CFG_S *pstReg, ISP_GE_S *pstGe)
{
    HI_U8 i, j;

    for (i = 0; i < pstReg->u8CfgNum; i++)
    {
        for (j = 0; j < pstGe->u8GeChnNum; j++)
        {
            pstReg->stAlgRegCfg[i].stGeRegCfg.abGeEn[j] = HI_FALSE;
        }
    }

    pstReg->unKey.bit1GeCfg = 1;
}

static __inline HI_S32 GeImageResWrite(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8 i;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    for (i = 0; i < pstIspCtx->stBlockAttr.u8BlockNum; i++)
    {
        GeImageSize(ViPipe, i, &pstRegCfg->stAlgRegCfg[i].stGeRegCfg.stUsrRegCfg, pstIspCtx);
        pstRegCfg->stAlgRegCfg[i].stGCacRegCfg.stUsrRegCfg.bResh = HI_TRUE;
    }

    pstRegCfg->unKey.bit1GeCfg = 1;

    return HI_SUCCESS;
}

HI_S32 ISP_GeInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    GeInitialize(ViPipe);
    GeRegsInitialize(ViPipe, pstRegCfg);
    GeExtRegsInitialize(ViPipe);

    return HI_SUCCESS;
}

HI_S32 ISP_GeRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                 HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_U8 i, j;
    ISP_GE_S  *pstGe = HI_NULL;
    ISP_CTX_S *pstIspCtx  = HI_NULL;
    ISP_REG_CFG_S *pstReg = (ISP_REG_CFG_S *)pRegCfg;

    GE_GET_CTX(ViPipe, pstGe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    /* calculate every two interrupts */

    if ((0 != pstIspCtx->u32FrameCnt % 2) && (HI_TRUE != pstIspCtx->stLinkage.bSnapState))
    {
        return HI_SUCCESS;
    }

    if (pstIspCtx->stLinkage.bDefectPixel)
    {
        Ge_Bypass(pstReg, pstGe);
        return HI_SUCCESS;
    }

    pstGe->bEnable = hi_ext_system_ge_enable_read(ViPipe);

    for (i = 0; i < pstReg->u8CfgNum; i++)
    {
        for (j = 0; j < pstGe->u8GeChnNum; j++)
        {
            pstReg->stAlgRegCfg[i].stGeRegCfg.abGeEn[j] = pstGe->bEnable;
        }

        for (j = pstGe->u8GeChnNum; j < 4; j++)
        {
            pstReg->stAlgRegCfg[i].stGeRegCfg.abGeEn[j] = HI_FALSE;
        }
    }

    pstReg->unKey.bit1GeCfg = 1;

    /*check hardware setting*/
    if (!CheckGeOpen(pstGe))
    {
        return HI_SUCCESS;
    }

    GeReadExtregs(ViPipe);

    if (pstGe->bCoefUpdateEn)
    {
        for (i = 0; i < pstReg->u8CfgNum; i++)
        {
            Ge_Usr_Fw(pstGe, &pstReg->stAlgRegCfg[i].stGeRegCfg.stUsrRegCfg);
        }
    }

    for (i = 0; i < pstReg->u8CfgNum; i++)
    {
        Ge_Dyna_Fw(pstIspCtx->stLinkage.u32Iso, pstGe, &pstReg->stAlgRegCfg[i].stGeRegCfg.stDynaRegCfg);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_GeCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);

    switch (u32Cmd)
    {
        case ISP_WDR_MODE_SET :
            ISP_GeInit(ViPipe, (HI_VOID *)&pRegCfg->stRegCfg);
            break;
        case ISP_CHANGE_IMAGE_MODE_SET:
            GeImageResWrite(ViPipe, &pRegCfg->stRegCfg);
            break;
        default :
            break;
    }
    return HI_SUCCESS;
}

HI_S32 ISP_GeExit(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);

    for (i = 0; i < pRegCfg->stRegCfg.u8CfgNum; i++)
    {
        pRegCfg->stRegCfg.stAlgRegCfg[i].stGeRegCfg.abGeEn[0] = HI_FALSE;
        pRegCfg->stRegCfg.stAlgRegCfg[i].stGeRegCfg.abGeEn[1] = HI_FALSE;
        pRegCfg->stRegCfg.stAlgRegCfg[i].stGeRegCfg.abGeEn[2] = HI_FALSE;
        pRegCfg->stRegCfg.stAlgRegCfg[i].stGeRegCfg.abGeEn[3] = HI_FALSE;
    }

    pRegCfg->stRegCfg.unKey.bit1GeCfg = 1;

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterGe(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_GE;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_GeInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_GeRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_GeCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_GeExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */



