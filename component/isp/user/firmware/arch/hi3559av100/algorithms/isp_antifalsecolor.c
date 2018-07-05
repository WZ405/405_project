/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_antifalsecolor.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2015/07/21
  Description   :
  History       :
  1.Date        : 2015/07/21
    Author      :
    Modification: Created file

******************************************************************************/

#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_config.h"
#include "isp_ext_config.h"
#include "isp_proc.h"
#include "isp_math_utils.h"
#include <math.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define HI_ANTIFALSECOLOR_BITDEPTH    (12)
#define HI_ISP_ANTIFALSECOLOR_LUT_LEN (16)

static const  HI_U32 au32AntiFalseColorIsoLut[ISP_AUTO_ISO_STRENGTH_NUM] = {100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600, 51200, 102400, 204800, 409600, 819200, 1638400, 3276800};

static const  HI_U8 g_au8AntiFalseColorThreshold[ISP_AUTO_ISO_STRENGTH_NUM] = {8, 8, 8, 8, 7, 7, 7, 6, 6, 6, 5, 4, 3, 2, 1, 0};
static const  HI_U8 g_au8AntiFalseColorStrength[ISP_AUTO_ISO_STRENGTH_NUM]  = {8, 8, 8, 8, 7, 7, 7, 6, 6, 6, 5, 4, 3, 2, 1, 0};

typedef struct hiISP_ANTIFALSECOLOR_MANUAL_S
{
    HI_U8    u8AntiFalseColorThreshold;       /*RW;Range:[0x0,0x20];Format:6.0;Threshold for antifalsecolor */
    HI_U8    u8AntiFalseColorStrength;        /*RW;Range:[0x0,0x1F];Format:5.0;Strength of antifalsecolor */
} ISP_ANTIFALSECOLOR_MANUAL_S;

typedef struct hiISP_ANTIFALSECOLOR_AUTO_S
{
    HI_U8    au8AntiFalseColorThreshold[ISP_AUTO_ISO_STRENGTH_NUM];    /*RW;Range:[0x0,0x20];Format:6.0;Threshold for antifalsecolor */
    HI_U8    au8AntiFalseColorStrength[ISP_AUTO_ISO_STRENGTH_NUM];     /*RW;Range:[0x0,0x1F];Format:5.0;Strength of antifalsecolor */
} ISP_ANTIFALSECOLOR_AUTO_S;

typedef struct hiISP_ANTIFALSECOLOR_S
{
    HI_BOOL bEnable;                    /*RW;Range:[0x0,0x1];Format:1.0; AntiFalseColor Enable*/
    HI_U8   u8WdrMode;
    HI_U8   au8LutAntiFalseColorGrayRatio[HI_ISP_ANTIFALSECOLOR_LUT_LEN];             //u5.0,
    HI_U8   au8LutAntiFalseColorCmaxSel[HI_ISP_ANTIFALSECOLOR_LUT_LEN];               //u4.0,
    HI_U8   au8LutAntiFalseColorDetgSel[HI_ISP_ANTIFALSECOLOR_LUT_LEN];               //u4.0,
    HI_U16  au16AntiFalseColorHfThreshLow[ISP_AUTO_ISO_STRENGTH_NUM];                 //10.0,
    HI_U16  au16AntiFalseColorHfThreshHig[ISP_AUTO_ISO_STRENGTH_NUM];                 //10.0,
    HI_U16  u16AntiFalseColorThr;                                                     //u12.0,

    ISP_OP_TYPE_E enOpType;
    ISP_ANTIFALSECOLOR_AUTO_S stAuto;
    ISP_ANTIFALSECOLOR_MANUAL_S stManual;
} ISP_ANTIFALSECOLOR_S;

ISP_ANTIFALSECOLOR_S g_astAntiFalseColorCtx[ISP_MAX_PIPE_NUM] = {{0}};
#define ANTIFALSECOLOR_GET_CTX(dev, pstCtx)   pstCtx = &g_astAntiFalseColorCtx[dev]

static HI_VOID  AntiFalseColorInitFwWdr(VI_PIPE ViPipe)
{
    ISP_ANTIFALSECOLOR_S *pstAntiFalseColor = HI_NULL;

    HI_U8   au8LutAntiFalseColorGrayRatio[HI_ISP_ANTIFALSECOLOR_LUT_LEN] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
    HI_U8   au8LutAntiFalseColorCmaxSel[HI_ISP_ANTIFALSECOLOR_LUT_LEN]   = {4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
    HI_U8   au8LutAntiFalseColorDetgSel[HI_ISP_ANTIFALSECOLOR_LUT_LEN]   = {4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    HI_U16  au16AntiFalseColorHfThreshLow[ISP_AUTO_ISO_STRENGTH_NUM]     = {96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96};
    HI_U16  au16AntiFalseColorHfThreshHig[ISP_AUTO_ISO_STRENGTH_NUM]     = {256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256};

    ANTIFALSECOLOR_GET_CTX(ViPipe, pstAntiFalseColor);

    memcpy(pstAntiFalseColor->au8LutAntiFalseColorGrayRatio, au8LutAntiFalseColorGrayRatio,   sizeof(HI_U8)*HI_ISP_ANTIFALSECOLOR_LUT_LEN);
    memcpy(pstAntiFalseColor->au8LutAntiFalseColorCmaxSel, au8LutAntiFalseColorCmaxSel,   sizeof(HI_U8)*HI_ISP_ANTIFALSECOLOR_LUT_LEN);
    memcpy(pstAntiFalseColor->au8LutAntiFalseColorDetgSel,   au8LutAntiFalseColorDetgSel, sizeof(HI_U8)*HI_ISP_ANTIFALSECOLOR_LUT_LEN);
    memcpy(pstAntiFalseColor->au16AntiFalseColorHfThreshLow, au16AntiFalseColorHfThreshLow,   sizeof(HI_U16)*HI_ISP_ANTIFALSECOLOR_LUT_LEN);
    memcpy(pstAntiFalseColor->au16AntiFalseColorHfThreshHig,   au16AntiFalseColorHfThreshHig, sizeof(HI_U16)*HI_ISP_ANTIFALSECOLOR_LUT_LEN);

    return;
}

static HI_VOID  AntiFalseColorInitFwLinear(VI_PIPE ViPipe)
{
    ISP_ANTIFALSECOLOR_S *pstAntiFalseColor = HI_NULL;

    HI_U8   au8LutAntiFalseColorGrayRatio[HI_ISP_ANTIFALSECOLOR_LUT_LEN] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
    HI_U8   au8LutAntiFalseColorCmaxSel[HI_ISP_ANTIFALSECOLOR_LUT_LEN]   = {4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
    HI_U8   au8LutAntiFalseColorDetgSel[HI_ISP_ANTIFALSECOLOR_LUT_LEN]   = {4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    HI_U16  au16AntiFalseColorHfThreshLow[ISP_AUTO_ISO_STRENGTH_NUM]     = {30, 30, 35, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48};
    HI_U16  au16AntiFalseColorHfThreshHig[ISP_AUTO_ISO_STRENGTH_NUM]     = {128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128};

    ANTIFALSECOLOR_GET_CTX(ViPipe, pstAntiFalseColor);

    memcpy(pstAntiFalseColor->au8LutAntiFalseColorGrayRatio, au8LutAntiFalseColorGrayRatio,   sizeof(HI_U8)*HI_ISP_ANTIFALSECOLOR_LUT_LEN);
    memcpy(pstAntiFalseColor->au8LutAntiFalseColorCmaxSel, au8LutAntiFalseColorCmaxSel,   sizeof(HI_U8)*HI_ISP_ANTIFALSECOLOR_LUT_LEN);
    memcpy(pstAntiFalseColor->au8LutAntiFalseColorDetgSel,   au8LutAntiFalseColorDetgSel, sizeof(HI_U8)*HI_ISP_ANTIFALSECOLOR_LUT_LEN);
    memcpy(pstAntiFalseColor->au16AntiFalseColorHfThreshLow, au16AntiFalseColorHfThreshLow,   sizeof(HI_U16)*HI_ISP_ANTIFALSECOLOR_LUT_LEN);
    memcpy(pstAntiFalseColor->au16AntiFalseColorHfThreshHig,   au16AntiFalseColorHfThreshHig, sizeof(HI_U16)*HI_ISP_ANTIFALSECOLOR_LUT_LEN);

    return;
}

static HI_VOID AntiFalseColorExtRegsInitialize(VI_PIPE ViPipe)
{
    HI_U32 i;
    ISP_ANTIFALSECOLOR_S *pstAntiFalseColor = HI_NULL;
    ISP_CMOS_DEFAULT_S *pstSnsDft = HI_NULL;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);
    ANTIFALSECOLOR_GET_CTX(ViPipe, pstAntiFalseColor);

    pstAntiFalseColor->u8WdrMode = pstIspCtx->u8SnsWDRMode;

    if (0 != pstAntiFalseColor->u8WdrMode)
    {
        AntiFalseColorInitFwWdr(ViPipe);
    }
    else
    {
        AntiFalseColorInitFwLinear(ViPipe);
    }

    hi_ext_system_antifalsecolor_enable_write(ViPipe, HI_EXT_SYSTEM_ANTIFALSECOLOR_ENABLE_DEFAULT);
    hi_ext_system_antifalsecolor_manual_mode_write(ViPipe, HI_EXT_SYSTEM_ANTIFALSECOLOR_MANUAL_MODE_DEFAULT);


    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)   //Auto
    {
        pstAntiFalseColor->stAuto.au8AntiFalseColorThreshold[i]    = g_au8AntiFalseColorThreshold[i];
        pstAntiFalseColor->stAuto.au8AntiFalseColorStrength[i]     = g_au8AntiFalseColorStrength[i];
        hi_ext_system_antifalsecolor_auto_threshold_write(ViPipe, i, pstAntiFalseColor->stAuto.au8AntiFalseColorThreshold[i]);
        hi_ext_system_antifalsecolor_auto_strenght_write(ViPipe, i, pstAntiFalseColor->stAuto.au8AntiFalseColorStrength[i]);
    }

    pstAntiFalseColor->stManual.u8AntiFalseColorThreshold    = HI_EXT_SYSTEM_ANTIFALSECOLOR_MANUAL_THRESHOLD_DEFAULT;
    pstAntiFalseColor->stManual.u8AntiFalseColorStrength     = HI_EXT_SYSTEM_ANTIFALSECOLOR_MANUAL_STRENGTH_DEFAULT;

    hi_ext_system_antifalsecolor_manual_threshold_write(ViPipe, pstAntiFalseColor->stManual.u8AntiFalseColorThreshold);
    hi_ext_system_antifalsecolor_manual_strenght_write(ViPipe, pstAntiFalseColor->stManual.u8AntiFalseColorStrength);

    if (pstSnsDft->stAntiFalseColor.bValid)
    {
        hi_ext_system_antifalsecolor_enable_write(ViPipe, pstSnsDft->stAntiFalseColor.bEnable);

        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)   //Auto
        {
            pstAntiFalseColor->stAuto.au8AntiFalseColorThreshold[i]    = pstSnsDft->stAntiFalseColor.au8AntiFalseColorThreshold[i];
            pstAntiFalseColor->stAuto.au8AntiFalseColorStrength[i]     = pstSnsDft->stAntiFalseColor.au8AntiFalseColorStrength[i];
            hi_ext_system_antifalsecolor_auto_threshold_write(ViPipe, i, pstAntiFalseColor->stAuto.au8AntiFalseColorThreshold[i]);
            hi_ext_system_antifalsecolor_auto_strenght_write(ViPipe, i, pstAntiFalseColor->stAuto.au8AntiFalseColorStrength[i]);
        }
    }

    return;
}

static HI_VOID AntiFalseColorRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S  *pstRegCfg)
{
    HI_U32 i;

    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ANTIFALSECOLOR_STATIC_CFG_S *pstAntiFalseColorStaticRegCfg = HI_NULL;
    ISP_ANTIFALSECOLOR_DYNA_CFG_S *pstAntiFalseColorDynaRegCfg = HI_NULL;
    ISP_RECT_S stBlockRect;
    ISP_GET_CTX(ViPipe, pstIspCtx);

    for (i = 0; i < pstIspCtx->stBlockAttr.u8BlockNum; i++)
    {
        pstRegCfg->stAlgRegCfg[i].stAntiFalseColorRegCfg.bFcrEnable = HI_ISP_DEMOSAIC_FCR_EN_DEFAULT;
        pstAntiFalseColorStaticRegCfg = &pstRegCfg->stAlgRegCfg[i].stAntiFalseColorRegCfg.stStaticRegCfg;
        pstAntiFalseColorDynaRegCfg   = &pstRegCfg->stAlgRegCfg[i].stAntiFalseColorRegCfg.stDynaRegCfg;

        //static
        ISP_GetBlockRect(&stBlockRect, &pstIspCtx->stBlockAttr, i);
        pstRegCfg->stAlgRegCfg[i].stAntiFalseColorRegCfg.stStaticRegCfg.bResh = HI_TRUE;
        pstAntiFalseColorStaticRegCfg->u16FcrLimit1 = HI_ISP_DEMOSAIC_FCR_LIMIT1_DEFAULT;
        pstAntiFalseColorStaticRegCfg->u16FcrLimit2 = HI_ISP_DEMOSAIC_FCR_LIMIT2_DEFAULT;
        pstAntiFalseColorStaticRegCfg->u16FcrThr    = HI_ISP_DEMOSAIC_FCR_THR_DEFAULT;

        /*dynamic*/
        pstRegCfg->stAlgRegCfg[i].stAntiFalseColorRegCfg.stDynaRegCfg.bResh = HI_TRUE;
        pstAntiFalseColorDynaRegCfg->u8FcrGain         = HI_ISP_DEMOSAIC_FCR_GAIN_DEFAULT;
        pstAntiFalseColorDynaRegCfg->u8FcrRatio        = HI_ISP_DEMOSAIC_FCR_RATIO_DEFAULT;
        pstAntiFalseColorDynaRegCfg->u8FcrGrayRatio    = HI_ISP_DEMOSAIC_FCR_GRAY_RATIO_DEFAULT;
        pstAntiFalseColorDynaRegCfg->u8FcrCmaxSel      = HI_ISP_DEMOSAIC_FCR_CMAX_SEL_DEFAULT;
        pstAntiFalseColorDynaRegCfg->u8FcrDetgSel      = HI_ISP_DEMOSAIC_FCR_DETG_SEL_DEFAULT;
        pstAntiFalseColorDynaRegCfg->u16FcrHfThreshLow = HI_ISP_DEMOSAIC_FCR_HF_THRESH_LOW_DEFAULT;
        pstAntiFalseColorDynaRegCfg->u16FcrHfThreshHig = HI_ISP_DEMOSAIC_FCR_HF_THRESH_HIGH_DEFAULT;
    }

    pstRegCfg->unKey.bit1FcrCfg = 1;

    return;
}

static HI_S32 AntiFalseColorReadExtregs(VI_PIPE ViPipe)
{
    HI_U32 i;
    ISP_ANTIFALSECOLOR_S *pstAntiFalseColor = HI_NULL;
    ANTIFALSECOLOR_GET_CTX(ViPipe, pstAntiFalseColor);

    pstAntiFalseColor->enOpType   = hi_ext_system_antifalsecolor_manual_mode_read(ViPipe);

    if (OP_TYPE_AUTO == pstAntiFalseColor->enOpType)
    {
        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
        {
            pstAntiFalseColor->stAuto.au8AntiFalseColorThreshold[i]    = hi_ext_system_antifalsecolor_auto_threshold_read(ViPipe, i);
            pstAntiFalseColor->stAuto.au8AntiFalseColorStrength[i]     = hi_ext_system_antifalsecolor_auto_strenght_read(ViPipe, i);
        }
    }
    else if (OP_TYPE_MANUAL == pstAntiFalseColor->enOpType)
    {
        pstAntiFalseColor->stManual.u8AntiFalseColorThreshold    = hi_ext_system_antifalsecolor_manual_threshold_read(ViPipe);
        pstAntiFalseColor->stManual.u8AntiFalseColorStrength     = hi_ext_system_antifalsecolor_manual_strenght_read(ViPipe);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_AntiFalseColorInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S  *pstRegCfg   = (ISP_REG_CFG_S *)pRegCfg;

    AntiFalseColorRegsInitialize(ViPipe, pstRegCfg);
    AntiFalseColorExtRegsInitialize(ViPipe);

    return HI_SUCCESS;
}

static HI_BOOL __inline CheckAntiFalseColorOpen(ISP_ANTIFALSECOLOR_S *pstAntiFalseColor)
{
    return (HI_TRUE == pstAntiFalseColor->bEnable);
}

static HI_U8 AntiFalseColorGetIsoIndex(HI_U32 u32Iso)
{
    HI_U8 u8Index;

    for (u8Index = 1; u8Index < HI_ISP_ANTIFALSECOLOR_LUT_LEN - 1; u8Index++)
    {
        if (u32Iso <= au32AntiFalseColorIsoLut[u8Index])
        {
            break;
        }
    }

    return u8Index;
}

HI_U32 AntiFalseColorGetValueFromLut (HI_U32 u32IsoLevel , HI_S32 s32Y2, HI_S32 s32Y1, HI_S32 s32X2, HI_S32 s32X1, HI_S32 s32Iso)
{
    HI_U32 u32Offset;

    if (s32X1 == s32X2)
    {
        u32Offset = s32Y2;
    }
    else if (s32Y1 <= s32Y2)
    {
        u32Offset = s32Y1 + (ABS(s32Y2 - s32Y1) * ABS(s32Iso - s32X1) + ABS((s32X2 - s32X1) / 2)) / ABS((s32X2 - s32X1));
    }
    else if (s32Y1 > s32Y2)
    {
        u32Offset = s32Y1 - (ABS(s32Y2 - s32Y1) * ABS(s32Iso - s32X1) + ABS((s32X2 - s32X1) / 2)) / ABS((s32X2 - s32X1));
    }

    return u32Offset;
}

HI_S32 AntiFalseColorCfg(ISP_ANTIFALSECOLOR_DYNA_CFG_S *pstAntiFalseColorDynaCfg, ISP_ANTIFALSECOLOR_S *pstAntiFalseColor, HI_U32 u32IsoLevel, HI_U32 u32ISO2, HI_U32 u32ISO1, HI_U32 u32Iso)
{
    HI_S32 s32Y1, s32Y2;

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstAntiFalseColor->au8LutAntiFalseColorGrayRatio[u32IsoLevel - 1] : (HI_S32)pstAntiFalseColor->au8LutAntiFalseColorGrayRatio[0];
    s32Y2 =                 (HI_S32)pstAntiFalseColor->au8LutAntiFalseColorGrayRatio[u32IsoLevel];
    pstAntiFalseColorDynaCfg->u8FcrGrayRatio  = (HI_U8)AntiFalseColorGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstAntiFalseColor->au8LutAntiFalseColorCmaxSel[u32IsoLevel - 1] : (HI_S32)pstAntiFalseColor->au8LutAntiFalseColorCmaxSel[0];
    s32Y2 =                 (HI_S32)pstAntiFalseColor->au8LutAntiFalseColorCmaxSel[u32IsoLevel];
    pstAntiFalseColorDynaCfg->u8FcrCmaxSel = (HI_U8)AntiFalseColorGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstAntiFalseColor->au8LutAntiFalseColorDetgSel[u32IsoLevel - 1] : (HI_S32)pstAntiFalseColor->au8LutAntiFalseColorDetgSel[0];
    s32Y2 =                 (HI_S32)pstAntiFalseColor->au8LutAntiFalseColorDetgSel[u32IsoLevel];
    pstAntiFalseColorDynaCfg->u8FcrDetgSel = (HI_U8)AntiFalseColorGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstAntiFalseColor->au16AntiFalseColorHfThreshLow[u32IsoLevel - 1] : (HI_S32)pstAntiFalseColor->au16AntiFalseColorHfThreshLow[0];
    s32Y2 =                 (HI_S32)pstAntiFalseColor->au16AntiFalseColorHfThreshLow[u32IsoLevel];
    pstAntiFalseColorDynaCfg->u16FcrHfThreshLow = (HI_U16)AntiFalseColorGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstAntiFalseColor->au16AntiFalseColorHfThreshHig[u32IsoLevel - 1] : (HI_S32)pstAntiFalseColor->au16AntiFalseColorHfThreshHig[0];
    s32Y2 =                 (HI_S32)pstAntiFalseColor->au16AntiFalseColorHfThreshHig[u32IsoLevel];
    pstAntiFalseColorDynaCfg->u16FcrHfThreshHig = (HI_U16)AntiFalseColorGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    pstAntiFalseColorDynaCfg->bResh = HI_TRUE;

    return HI_SUCCESS;
}

HI_S32 ISP_AntiFalseColor_Fw(HI_U32 u32Iso, VI_PIPE ViPipe, HI_U8 u8CurBlk, ISP_REG_CFG_S *pstReg)
{
    ISP_ANTIFALSECOLOR_DYNA_CFG_S *pstAntiFalseColorDynaCfg = &pstReg->stAlgRegCfg[u8CurBlk].stAntiFalseColorRegCfg.stDynaRegCfg;
    ISP_ANTIFALSECOLOR_S *pstAntiFalseColor = HI_NULL;
    ANTIFALSECOLOR_GET_CTX(ViPipe, pstAntiFalseColor);

    HI_U32 u32IsoLevel;
    HI_U32 u32ISO1 = 0;
    HI_U32 u32ISO2 = 0;
    HI_S32 s32Y1, s32Y2;

    if (u32Iso > au32AntiFalseColorIsoLut[HI_ISP_ANTIFALSECOLOR_LUT_LEN - 1])
    {
        u32IsoLevel = HI_ISP_ANTIFALSECOLOR_LUT_LEN - 1;
        u32ISO1 = au32AntiFalseColorIsoLut[HI_ISP_ANTIFALSECOLOR_LUT_LEN - 1];
        u32ISO2 = au32AntiFalseColorIsoLut[HI_ISP_ANTIFALSECOLOR_LUT_LEN - 1];
    }
    else if (u32Iso <= au32AntiFalseColorIsoLut[0])
    {
        u32IsoLevel = 0;
        u32ISO1 = 0;
        u32ISO2 = au32AntiFalseColorIsoLut[0];
    }
    else
    {
        u32IsoLevel = AntiFalseColorGetIsoIndex(u32Iso);
        u32ISO1 = au32AntiFalseColorIsoLut[u32IsoLevel - 1];
        u32ISO2 = au32AntiFalseColorIsoLut[u32IsoLevel];
    }

    AntiFalseColorCfg(pstAntiFalseColorDynaCfg, pstAntiFalseColor, u32IsoLevel, u32ISO2, u32ISO1, u32Iso);

    if (OP_TYPE_AUTO == pstAntiFalseColor->enOpType)
    {
        s32Y1 = (u32IsoLevel) ? (HI_S32)pstAntiFalseColor->stAuto.au8AntiFalseColorThreshold[u32IsoLevel - 1] : (HI_S32)pstAntiFalseColor->stAuto.au8AntiFalseColorThreshold[0];
        s32Y2 =                 (HI_S32)pstAntiFalseColor->stAuto.au8AntiFalseColorThreshold[u32IsoLevel];
        pstAntiFalseColorDynaCfg->u8FcrRatio = (HI_U8)AntiFalseColorGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstAntiFalseColor->stAuto.au8AntiFalseColorStrength[u32IsoLevel - 1] : (HI_S32)pstAntiFalseColor->stAuto.au8AntiFalseColorStrength[0];
        s32Y2 =                 (HI_S32)pstAntiFalseColor->stAuto.au8AntiFalseColorStrength[u32IsoLevel];
        pstAntiFalseColorDynaCfg->u8FcrGain = (HI_U8)AntiFalseColorGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    }
    else if (OP_TYPE_MANUAL == pstAntiFalseColor->enOpType)
    {
        pstAntiFalseColorDynaCfg->u8FcrRatio        = pstAntiFalseColor->stManual.u8AntiFalseColorThreshold;
        pstAntiFalseColorDynaCfg->u8FcrGain         = pstAntiFalseColor->stManual.u8AntiFalseColorStrength;
    }

    pstAntiFalseColorDynaCfg->bResh = HI_TRUE;

    return HI_SUCCESS;
}

//static __inline HI_S32 FcrImageResWrite(VI_PIPE ViPipe, ISP_CMOS_SENSOR_IMAGE_MODE_S *pstRes)
//{
//    HI_U16 i;
//    ISP_CTX_S *pstIspCtx  = HI_NULL;
//
//    ISP_GET_CTX(ViPipe, pstIspCtx);
//
//  for(i = 0; i < pstIspCtx->stBlockAttr.u8BlockNum; i++)
//  {
//
//  }
//
//    return HI_SUCCESS;
//}

HI_S32 ISP_AntiFalseColorRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo, HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_U8 i;
    ISP_REG_CFG_S *pstReg = (ISP_REG_CFG_S *)pRegCfg;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ANTIFALSECOLOR_S *pstAntiFalseColor = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    ANTIFALSECOLOR_GET_CTX(ViPipe, pstAntiFalseColor);

    /* calculate every two interrupts */
    if ((0 != pstIspCtx->u32FrameCnt % 2) && (HI_TRUE != pstIspCtx->stLinkage.bSnapState))
    {
        return HI_SUCCESS;
    }

    pstAntiFalseColor->bEnable = hi_ext_system_antifalsecolor_enable_read(ViPipe);

    for (i = 0; i < pstReg->u8CfgNum; i++)
    {
        pstReg->stAlgRegCfg[i].stAntiFalseColorRegCfg.bFcrEnable = pstAntiFalseColor->bEnable;
    }

    pstReg->unKey.bit1FcrCfg = 1;

    /*check hardware setting*/
    if (!CheckAntiFalseColorOpen(pstAntiFalseColor))
    {
        return HI_SUCCESS;
    }

    AntiFalseColorReadExtregs(ViPipe);

    for (i = 0; i < pstReg->u8CfgNum; i++)
    {
        ISP_AntiFalseColor_Fw(pstIspCtx->stLinkage.u32Iso, ViPipe, i , pstReg);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_AntiFalseColorCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    switch (u32Cmd)
    {
        case ISP_WDR_MODE_SET :
            //ANTIFALSECOLOR_GET_CTX(ViPipe, pstFcr);
            //ISP_FcrInit(ViPipe);
            break;
        case ISP_PROC_WRITE:
            //FcrProcWrite(ViPipe, (ISP_CTRL_PROC_WRITE_S *)pValue);
            break;
        case ISP_CHANGE_IMAGE_MODE_SET:
            //FcrImageResWrite(ViPipe, (ISP_CMOS_SENSOR_IMAGE_MODE_S *)pValue);

        default :
            break;
    }
    return HI_SUCCESS;
}

HI_S32 ISP_AntiFalseColorExit(VI_PIPE ViPipe)
{
    HI_U16 i;
    ISP_CTX_S *pstIspCtx  = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    for (i = 0; i < pstIspCtx->stBlockAttr.u8BlockNum; i++)
    {
        isp_demosaic_fcr_en_write(ViPipe, i, HI_FALSE);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterFcr(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_ANTIFALSECOLOR;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_AntiFalseColorInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_AntiFalseColorRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_AntiFalseColorCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_AntiFalseColorExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

