/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_rlsc.c
  Version       : Initial Draft
  Author        :
  Created       : 2016/08/17
  Last Modified :
  Description   : Radial Lens Shading Correction Algorithms
  Function List :
  History       :
  1.Date        : 2016/08/17
    Author      :
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include "isp_alg.h"
#include "isp_ext_config.h"
#include "isp_config.h"
#include "isp_sensor.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/
static const  HI_U16  g_au16GainStrDef[14] = {32768, 16384, 8192, 4096, 2048, 1024, 512, 0, 0, 0, 0, 0, 0, 0};

typedef struct hiISP_RLSC_CALI_S
{
    HI_U16 u16WBRGain;
    HI_U16 u16WBBGain;

    HI_U16 au16RGain[HI_ISP_RLSC_POINTS];
    HI_U16 au16GrGain[HI_ISP_RLSC_POINTS];
    HI_U16 au16GbGain[HI_ISP_RLSC_POINTS];
    HI_U16 au16BGain[HI_ISP_RLSC_POINTS];
} ISP_RLSC_CALI_S;

typedef struct hiISP_RLSC
{
    // Enable/Disable, ext reg: yes, PQTools: yes:
    HI_BOOL bLscEnable;         // General rlsc enable
    HI_BOOL bRLscFuncEn;
    HI_BOOL bRadialCropEn;      // Radial Crop Function enable
    OPERATION_MODE_E enLightMode; // Switch to select auto/manual mode

    //Manual related value, ext reg: yes, PQTools: yes:
    HI_U16 u16BlendRatio;         //Manual mode, light blending weight
    HI_U8  u8LightType1;   //Manual mode, light source 1 info
    HI_U8  u8LightType2;   //Manual mode, light source 2 info
    HI_U8  u8RadialScale;     //Select Radial Scale mode
    //Correction control coefficients, ext reg: yes, PQTools: yes:
    HI_U32 u32ValidRadius;    //square Valid radius, Enable when Radial crop is on
    HI_U16 u16RadialStrength; //correction strength

    // Update Booleans, ext reg: yes, PQTools: no:
    HI_BOOL bLscCoefUpdate;    //Coefficient update flag
    HI_BOOL bLutUpdate;        //LUT update flag

    // Center point info, ext reg: yes, PQTools: yes,
    HI_U16  u16CenterRX;
    HI_U16  u16CenterRY;
    HI_U16  u16CenterGrX;
    HI_U16  u16CenterGrY;
    HI_U16  u16CenterGbX;
    HI_U16  u16CenterGbY;
    HI_U16  u16CenterBX;
    HI_U16  u16CenterBY;

    HI_U16  u16OffCenterR;
    HI_U16  u16OffCenterGr;
    HI_U16  u16OffCenterGb;
    HI_U16  u16OffCenterB;

    //White balance info, ext reg:no, PQTools: no
    HI_U16  u16WbRGain;
    HI_U16  u16WbBGain;

    ISP_RLSC_CALI_S stRLscCaliResult[3];

} ISP_RLSC_S;

ISP_RLSC_S g_astRLscCtx[ISP_MAX_PIPE_NUM] = {{0}};
#define RLSC_GET_CTX(dev, pstCtx)   pstCtx = &g_astRLscCtx[dev]

/**
 * [RLscCalcWeight:]
 * [Used to calculate LUT weight when in auto mode]
 * @param  pstRLsc   [Input]  Radial LSC structure, contains original LUT gain(three)
 * @return u16BlendRatio [Output]
 */
static HI_VOID RLscCalcWeight(ISP_RLSC_S *pstRLsc, HI_U16 u16BlendRatio, HI_U8 u8LscLightInfo1, HI_U8 u8LscLightInfo2)
{
    HI_U8  i;
    HI_U16 x11, y11;
    HI_U16 u16WbRGain, u16WbBGain;

    HI_U32 u32MinD1, u32MinD2;
    HI_U32 d[3];

    //Using input white balance info to calculate weight
    // Parameter Declaration Ends

    u16WbRGain = pstRLsc->u16WbRGain;
    u16WbBGain = pstRLsc->u16WbBGain;

    u8LscLightInfo1 = 0;
    u8LscLightInfo2 = u8LscLightInfo1;
    u32MinD1 = 1 << 31;
    u32MinD2 = 1 << 31;

    for (i = 0; i < 3; i++)
    {
        x11  = u16WbRGain - pstRLsc->stRLscCaliResult[i].u16WBRGain;
        y11  = u16WbBGain - pstRLsc->stRLscCaliResult[i].u16WBBGain;
        d[i] = x11 * x11 + y11 * y11;

        if (d[i] < u32MinD1)
        {
            u32MinD1 = d[i];
            u8LscLightInfo1 = i;
        }
    }

    d[u8LscLightInfo1] = u32MinD2;

    for (i = 0; i < 3; i++)
    {
        if (d[i] < u32MinD2)
        {
            u32MinD2 = d[i];
            u8LscLightInfo2 = i;
        }
    }

    u16BlendRatio = (HI_U16)(u32MinD2 * 256 / DIV_0_TO_1(u32MinD1 + u32MinD2));

    return;
}

static HI_VOID AdjustCenterPoint(VI_PIPE ViPipe, ISP_RLSC_S *pstRLsc)
{
    //HI_U8 u8BayerFormat;
    ISP_CTX_S *pstIspCtx;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    switch ( pstIspCtx->enBayer )
    {
        case BAYER_RGGB :
            pstRLsc->u16CenterGrX += 1;
            pstRLsc->u16CenterGbY += 1;
            pstRLsc->u16CenterBX += 1;
            pstRLsc->u16CenterBY += 1;
            break;

        case BAYER_GRBG :
            pstRLsc->u16CenterRX += 1;
            pstRLsc->u16CenterBY += 1;
            pstRLsc->u16CenterGbX += 1;
            pstRLsc->u16CenterGbY += 1;
            break;

        case BAYER_GBRG :
            pstRLsc->u16CenterBX += 1;
            pstRLsc->u16CenterRY += 1;
            pstRLsc->u16CenterGrX += 1;
            pstRLsc->u16CenterGrY += 1;
            break;

        case BAYER_BGGR :
            pstRLsc->u16CenterGbX += 1;
            pstRLsc->u16CenterGrY += 1;
            pstRLsc->u16CenterRX += 1;
            pstRLsc->u16CenterRY += 1;
            break;

        default:
            printf("Invalid Bayer pattern in function: %s !!\n", __FUNCTION__);
    }

    return;
}

static HI_VOID CalcRadius(VI_PIPE ViPipe, ISP_RLSC_S *pstRLsc)
{
    HI_U8  i;
    HI_U32 u32Radius;
    HI_U16 u16OffCenter[4];
    HI_U16 u16MaxOffCenter = 0;
    HI_U32 temp = (1 << 31);

    u16OffCenter[0] = pstRLsc->u16OffCenterR;
    u16OffCenter[1] = pstRLsc->u16OffCenterGr;
    u16OffCenter[2] = pstRLsc->u16OffCenterGb;
    u16OffCenter[3] = pstRLsc->u16OffCenterB;

    for (i = 0; i < 4; i++)
    {
        if (u16OffCenter[i] >= u16MaxOffCenter)
        {
            u16MaxOffCenter = u16OffCenter[i];
        }
    }
    u32Radius = temp / (HI_U32)DIV_0_TO_1(u16MaxOffCenter);
    pstRLsc->u32ValidRadius = u32Radius;

    return;
}

/**
 * [RLscGetGainLut:]
 * [Used to calculate actual LUT gain value]
 * @param  ViPipe       [Input]  Vi Pipe No.
 * @param  pstRLsc      [Input]  Radial LSC structure, contains original LUT gain(three)
 * @param  pstUsrRegCfg [Output] User Register config, need to update its LUT gain value
 * @return              [Void]   No return value
 */
static HI_VOID RLscGetGainLut(VI_PIPE ViPipe, ISP_RLSC_S *pstRLsc, ISP_RLSC_USR_CFG_S *pstUsrRegCfg)
{
    HI_U16 i;
    HI_U16 u16BlendRatio   = 0;
    HI_U8  u8LscLightInfo1 = 0;
    HI_U8  u8LscLightInfo2 = 0;

    HI_U32 au32RGain[HI_ISP_RLSC_POINTS + 1];
    HI_U32 au32GrGain[HI_ISP_RLSC_POINTS + 1];
    HI_U32 au32GbGain[HI_ISP_RLSC_POINTS + 1];
    HI_U32 au32BGain[HI_ISP_RLSC_POINTS + 1];

    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    //Get Weight, lightinfo1 and lightinfo2
    if ( OPERATION_MODE_MANUAL == pstRLsc->enLightMode )
    {
        u16BlendRatio = pstRLsc->u16BlendRatio;
        u8LscLightInfo1 = pstRLsc->u8LightType1;
        u8LscLightInfo2 = pstRLsc->u8LightType2;
    }
    else
    {
        RLscCalcWeight(pstRLsc, u16BlendRatio, u8LscLightInfo1, u8LscLightInfo2);
    }

    //Use weight, lightinfo1 and lightinfo2 to calculate actural gain value
    for ( i = 0 ; i < HI_ISP_RLSC_POINTS ; i++ )
    {
        au32RGain[i]  = ((pstRLsc->stRLscCaliResult[u8LscLightInfo1].au16RGain[i]) * u16BlendRatio + (pstRLsc->stRLscCaliResult[u8LscLightInfo2].au16RGain[i]) * (256 - u16BlendRatio)) >> HI_ISP_RLSC_WEIGHT_Q_BITS;
        au32GrGain[i] = ((pstRLsc->stRLscCaliResult[u8LscLightInfo1].au16GrGain[i]) * u16BlendRatio + (pstRLsc->stRLscCaliResult[u8LscLightInfo2].au16GrGain[i]) * (256 - u16BlendRatio)) >> HI_ISP_RLSC_WEIGHT_Q_BITS;
        au32GbGain[i] = ((pstRLsc->stRLscCaliResult[u8LscLightInfo1].au16GbGain[i]) * u16BlendRatio + (pstRLsc->stRLscCaliResult[u8LscLightInfo2].au16GbGain[i]) * (256 - u16BlendRatio)) >> HI_ISP_RLSC_WEIGHT_Q_BITS;
        au32BGain[i]  = ((pstRLsc->stRLscCaliResult[u8LscLightInfo1].au16BGain[i]) * u16BlendRatio + (pstRLsc->stRLscCaliResult[u8LscLightInfo2].au16BGain[i]) * (256 - u16BlendRatio)) >> HI_ISP_RLSC_WEIGHT_Q_BITS;
    }
    //Copy the last node
    au32RGain[HI_ISP_RLSC_POINTS]  = au32RGain[HI_ISP_RLSC_POINTS - 1];
    au32GrGain[HI_ISP_RLSC_POINTS] = au32GrGain[HI_ISP_RLSC_POINTS - 1];
    au32GbGain[HI_ISP_RLSC_POINTS] = au32GbGain[HI_ISP_RLSC_POINTS - 1];
    au32BGain[HI_ISP_RLSC_POINTS]  = au32BGain[HI_ISP_RLSC_POINTS - 1];
    //allocate to this right position according to rggb pattern:
    switch ( pstIspCtx->enBayer )
    {
        case BAYER_RGGB :
            memcpy(pstUsrRegCfg->au32Lut0Chn0, au32RGain, (HI_ISP_RLSC_POINTS + 1)*sizeof(HI_U32));
            memcpy(pstUsrRegCfg->au32Lut0Chn1, au32GrGain, (HI_ISP_RLSC_POINTS + 1)*sizeof(HI_U32));
            memcpy(pstUsrRegCfg->au32Lut1Chn2, au32GbGain, (HI_ISP_RLSC_POINTS + 1)*sizeof(HI_U32));
            memcpy(pstUsrRegCfg->au32Lut1Chn3, au32BGain, (HI_ISP_RLSC_POINTS + 1)*sizeof(HI_U32));
            break;

        case BAYER_GRBG :
            memcpy(pstUsrRegCfg->au32Lut0Chn0, au32GrGain, (HI_ISP_RLSC_POINTS + 1)*sizeof(HI_U32));
            memcpy(pstUsrRegCfg->au32Lut0Chn1, au32RGain, (HI_ISP_RLSC_POINTS + 1)*sizeof(HI_U32));
            memcpy(pstUsrRegCfg->au32Lut1Chn2, au32BGain, (HI_ISP_RLSC_POINTS + 1)*sizeof(HI_U32));
            memcpy(pstUsrRegCfg->au32Lut1Chn3, au32GbGain, (HI_ISP_RLSC_POINTS + 1)*sizeof(HI_U32));
            break;

        case BAYER_GBRG :
            memcpy(pstUsrRegCfg->au32Lut0Chn0, au32GbGain, (HI_ISP_RLSC_POINTS + 1)*sizeof(HI_U32));
            memcpy(pstUsrRegCfg->au32Lut0Chn1, au32BGain, (HI_ISP_RLSC_POINTS + 1)*sizeof(HI_U32));
            memcpy(pstUsrRegCfg->au32Lut1Chn2, au32RGain, (HI_ISP_RLSC_POINTS + 1)*sizeof(HI_U32));
            memcpy(pstUsrRegCfg->au32Lut1Chn3, au32GrGain, (HI_ISP_RLSC_POINTS + 1)*sizeof(HI_U32));
            break;

        case BAYER_BGGR :
            memcpy(pstUsrRegCfg->au32Lut0Chn0, au32BGain, (HI_ISP_RLSC_POINTS + 1)*sizeof(HI_U32));
            memcpy(pstUsrRegCfg->au32Lut0Chn1, au32GbGain, (HI_ISP_RLSC_POINTS + 1)*sizeof(HI_U32));
            memcpy(pstUsrRegCfg->au32Lut1Chn2, au32GrGain, (HI_ISP_RLSC_POINTS + 1)*sizeof(HI_U32));
            memcpy(pstUsrRegCfg->au32Lut1Chn3, au32RGain, (HI_ISP_RLSC_POINTS + 1)*sizeof(HI_U32));
            break;

        default:
            printf("Invalid Bayer pattern in function: %s !!\n", __FUNCTION__);
    }

    return;
}

static HI_VOID RLscGetCaliRadiusInfo(ISP_RLSC_S *pstRLsc, ISP_RLSC_USR_CFG_S *pstUsrRegCfg)
{

    pstUsrRegCfg->u16CenterRX  = pstRLsc->u16CenterRX;
    pstUsrRegCfg->u16CenterRY  = pstRLsc->u16CenterRY;
    pstUsrRegCfg->u16CenterGrX = pstRLsc->u16CenterGrX;
    pstUsrRegCfg->u16CenterGrY = pstRLsc->u16CenterGrY;
    pstUsrRegCfg->u16CenterGbX = pstRLsc->u16CenterGbX;
    pstUsrRegCfg->u16CenterGbY = pstRLsc->u16CenterGbY;
    pstUsrRegCfg->u16CenterBX  = pstRLsc->u16CenterBX;
    pstUsrRegCfg->u16CenterBY  = pstRLsc->u16CenterBY;

    pstUsrRegCfg->u16OffCenterR  = pstRLsc->u16OffCenterR;
    pstUsrRegCfg->u16OffCenterGr = pstRLsc->u16OffCenterGr;
    pstUsrRegCfg->u16OffCenterGb = pstRLsc->u16OffCenterGb;
    pstUsrRegCfg->u16OffCenterB  = pstRLsc->u16OffCenterB;

    pstUsrRegCfg->u32ValidRadius = pstRLsc->u32ValidRadius;
}


static HI_VOID RLscStaticRegsInitialize(VI_PIPE ViPipe, ISP_RLSC_STATIC_CFG_S *pstStaticRegCfg)
{
    pstStaticRegCfg->u16NodeNum = HI_ISP_RLSC_POINTS;

    pstStaticRegCfg->bStaticResh = HI_TRUE;

    return;
}

static HI_VOID RLscUsrRegsInitialize(HI_U8 u8CurBlk, VI_PIPE ViPipe, ISP_RLSC_USR_CFG_S *pstUsrRegCfg)
{
    ISP_CTX_S  *pstIspCtx;
    ISP_RLSC_S *pstRLsc    = HI_NULL;
    ISP_RECT_S stBlockRect;

    RLSC_GET_CTX(ViPipe, pstRLsc);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    ISP_GetBlockRect(&stBlockRect, &pstIspCtx->stBlockAttr, u8CurBlk);
    pstUsrRegCfg->u16WidthOffset = ABS(stBlockRect.s32X);//TODO: Fill in the exact bias value.!!!important in SBS mode
    pstUsrRegCfg->u16GainStr  = pstRLsc->u16RadialStrength;
    pstUsrRegCfg->u8GainScale = pstRLsc->u8RadialScale;

    RLscGetCaliRadiusInfo(pstRLsc, pstUsrRegCfg);

    RLscGetGainLut(ViPipe, pstRLsc, pstUsrRegCfg);

    pstUsrRegCfg->u32UpdateIndex = 1;

    pstUsrRegCfg->bLutUpdate     = HI_TRUE;
    pstUsrRegCfg->bCoefUpdate    = HI_TRUE;
    pstUsrRegCfg->bRadialCropEn  = HI_TRUE;
    pstUsrRegCfg->bRLscFuncEn    = pstRLsc->bRLscFuncEn;
    pstUsrRegCfg->bUsrResh       = HI_TRUE;

    return;
}

/**
 * [RLscRegsInitialize description]
 * @param  ViPipe    [Input]  ViPipe Number
 * @param  pstRegCfg [Output] Register config
 * @return           [Void]   No return value
 */
static HI_VOID RLscRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_S32 i;
    ISP_RLSC_S *pstRLsc    = HI_NULL;

    RLSC_GET_CTX(ViPipe, pstRLsc);

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        RLscStaticRegsInitialize(ViPipe, &pstRegCfg->stAlgRegCfg[i].stRLscRegCfg.stStaticRegCfg);
        RLscUsrRegsInitialize(i, ViPipe, &pstRegCfg->stAlgRegCfg[i].stRLscRegCfg.stUsrRegCfg);

        pstRegCfg->stAlgRegCfg[i].stRLscRegCfg.bRLscEn = pstRLsc->bLscEnable;
    }

    pstRegCfg->unKey.bit1RLscCfg = 1;

    return;
}

/**
 * [RLscExtRegsInitialize: Initialize ext registers, which is mainly used by tools]
 * @param  ViPipe    [Input]  ViPipe Number
 * @return           [Void]   No return value
 */
static HI_VOID RLscExtRegsInitialize(VI_PIPE ViPipe)
{
    HI_U16 i;
    ISP_RLSC_S  *pstRLsc    = HI_NULL;

    RLSC_GET_CTX(ViPipe, pstRLsc);

    hi_ext_system_isp_radial_shading_enable_write(ViPipe, pstRLsc->bLscEnable);

    hi_ext_system_isp_radial_shading_coefupdate_write(ViPipe, HI_FALSE);
    hi_ext_system_isp_radial_shading_lutupdate_write(ViPipe, HI_FALSE);

    //Coef update
    hi_ext_system_isp_radial_shading_strength_write(ViPipe, pstRLsc->u16RadialStrength);

    //Lut update
    hi_ext_system_isp_radial_shading_lightmode_write(ViPipe, pstRLsc->enLightMode);
    hi_ext_system_isp_radial_shading_blendratio_write(ViPipe, pstRLsc->u16BlendRatio);
    hi_ext_system_isp_radial_shading_scale_write(ViPipe, pstRLsc->u8RadialScale);

    // light info, manual mode
    hi_ext_system_isp_radial_shading_lightinfo_write(ViPipe, 0, pstRLsc->u8LightType1);
    hi_ext_system_isp_radial_shading_lightinfo_write(ViPipe, 1, pstRLsc->u8LightType2);

    hi_ext_system_isp_radial_shading_centerrx_write(ViPipe, pstRLsc->u16CenterRX);
    hi_ext_system_isp_radial_shading_centerry_write(ViPipe, pstRLsc->u16CenterRY);
    hi_ext_system_isp_radial_shading_centergrx_write(ViPipe, pstRLsc->u16CenterGrX);
    hi_ext_system_isp_radial_shading_centergry_write(ViPipe, pstRLsc->u16CenterGrY);
    hi_ext_system_isp_radial_shading_centergbx_write(ViPipe, pstRLsc->u16CenterGbX);
    hi_ext_system_isp_radial_shading_centergby_write(ViPipe, pstRLsc->u16CenterGbY);
    hi_ext_system_isp_radial_shading_centerbx_write(ViPipe, pstRLsc->u16CenterBX);
    hi_ext_system_isp_radial_shading_centerby_write(ViPipe, pstRLsc->u16CenterBY);
    hi_ext_system_isp_radial_shading_offcenterr_write(ViPipe, pstRLsc->u16OffCenterR);
    hi_ext_system_isp_radial_shading_offcentergr_write(ViPipe, pstRLsc->u16OffCenterGr);
    hi_ext_system_isp_radial_shading_offcentergb_write(ViPipe, pstRLsc->u16OffCenterGb);
    hi_ext_system_isp_radial_shading_offcenterb_write(ViPipe, pstRLsc->u16OffCenterB);

    for ( i = 0 ; i < HI_ISP_RLSC_POINTS ; i++ )
    {
        hi_ext_system_isp_radial_shading_r_gain0_write(ViPipe, i, pstRLsc->stRLscCaliResult[0].au16RGain[i]);
        hi_ext_system_isp_radial_shading_r_gain1_write(ViPipe, i, pstRLsc->stRLscCaliResult[1].au16RGain[i]);
        hi_ext_system_isp_radial_shading_r_gain2_write(ViPipe, i, pstRLsc->stRLscCaliResult[2].au16RGain[i]);

        hi_ext_system_isp_radial_shading_gr_gain0_write(ViPipe, i, pstRLsc->stRLscCaliResult[0].au16GrGain[i]);
        hi_ext_system_isp_radial_shading_gr_gain1_write(ViPipe, i, pstRLsc->stRLscCaliResult[1].au16GrGain[i]);
        hi_ext_system_isp_radial_shading_gr_gain2_write(ViPipe, i, pstRLsc->stRLscCaliResult[2].au16GrGain[i]);

        hi_ext_system_isp_radial_shading_gb_gain0_write(ViPipe, i, pstRLsc->stRLscCaliResult[0].au16GbGain[i]);
        hi_ext_system_isp_radial_shading_gb_gain1_write(ViPipe, i, pstRLsc->stRLscCaliResult[1].au16GbGain[i]);
        hi_ext_system_isp_radial_shading_gb_gain2_write(ViPipe, i, pstRLsc->stRLscCaliResult[2].au16GbGain[i]);

        hi_ext_system_isp_radial_shading_b_gain0_write(ViPipe, i, pstRLsc->stRLscCaliResult[0].au16BGain[i]);
        hi_ext_system_isp_radial_shading_b_gain1_write(ViPipe, i, pstRLsc->stRLscCaliResult[1].au16BGain[i]);
        hi_ext_system_isp_radial_shading_b_gain2_write(ViPipe, i, pstRLsc->stRLscCaliResult[2].au16BGain[i]);
    }

    return;
}

static HI_VOID RLscReadExtRegs(VI_PIPE ViPipe)
{
    HI_U16  i;
    HI_U16  u16WbRGain, u16WbGGain, u16WbBGain;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_RLSC_S *pstRLsc      = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    RLSC_GET_CTX(ViPipe, pstRLsc);

    u16WbRGain = pstIspCtx->stLinkage.au32WhiteBalanceGain[0];
    u16WbGGain = (pstIspCtx->stLinkage.au32WhiteBalanceGain[1] + pstIspCtx->stLinkage.au32WhiteBalanceGain[2]) >> 1;
    u16WbBGain = pstIspCtx->stLinkage.au32WhiteBalanceGain[3];
    pstRLsc->u16WbRGain = (HI_U16)(((HI_U32)u16WbRGain * 256) / DIV_0_TO_1(u16WbGGain));
    pstRLsc->u16WbBGain = (HI_U16)(((HI_U32)u16WbBGain * 256) / DIV_0_TO_1(u16WbGGain));

    //Read Coef udpate, then re-config to HI_FALSE
    pstRLsc->bLscCoefUpdate = hi_ext_system_isp_radial_shading_coefupdate_read(ViPipe);
    hi_ext_system_isp_radial_shading_coefupdate_write(ViPipe, HI_FALSE);

    if (pstRLsc->bLscCoefUpdate)
    {
        //pstRLsc->bRLscFuncEn       = hi_ext_system_isp_radial_shading_lsc_enable_read(ViPipe);
        pstRLsc->u16RadialStrength = hi_ext_system_isp_radial_shading_strength_read(ViPipe);
    }

    pstRLsc->bLutUpdate = hi_ext_system_isp_radial_shading_lutupdate_read(ViPipe);
    hi_ext_system_isp_radial_shading_lutupdate_write(ViPipe, HI_FALSE);

    if (pstRLsc->bLutUpdate)
    {

        pstRLsc->enLightMode   = hi_ext_system_isp_radial_shading_lightmode_read(ViPipe);
        pstRLsc->u16BlendRatio = hi_ext_system_isp_radial_shading_blendratio_read(ViPipe);
        pstRLsc->u8LightType1  = hi_ext_system_isp_radial_shading_lightinfo_read(ViPipe, 0);
        pstRLsc->u8LightType2  = hi_ext_system_isp_radial_shading_lightinfo_read(ViPipe, 1);

        pstRLsc->u8RadialScale = hi_ext_system_isp_radial_shading_scale_read(ViPipe);

        pstRLsc->u16CenterRX   = hi_ext_system_isp_radial_shading_centerrx_read(ViPipe);
        pstRLsc->u16CenterRY   = hi_ext_system_isp_radial_shading_centerry_read(ViPipe);
        pstRLsc->u16OffCenterR = hi_ext_system_isp_radial_shading_offcenterr_read(ViPipe);

        pstRLsc->u16CenterGrX  = hi_ext_system_isp_radial_shading_centergrx_read(ViPipe);
        pstRLsc->u16CenterGrY  = hi_ext_system_isp_radial_shading_centergry_read(ViPipe);
        pstRLsc->u16OffCenterGr = hi_ext_system_isp_radial_shading_offcentergr_read(ViPipe);

        pstRLsc->u16CenterGbX  = hi_ext_system_isp_radial_shading_centergbx_read(ViPipe);
        pstRLsc->u16CenterGbY  = hi_ext_system_isp_radial_shading_centergby_read(ViPipe);
        pstRLsc->u16OffCenterGb = hi_ext_system_isp_radial_shading_offcentergb_read(ViPipe);

        pstRLsc->u16CenterBX   = hi_ext_system_isp_radial_shading_centerbx_read(ViPipe);
        pstRLsc->u16CenterBY   = hi_ext_system_isp_radial_shading_centerby_read(ViPipe);
        pstRLsc->u16OffCenterB = hi_ext_system_isp_radial_shading_offcenterb_read(ViPipe);

        for ( i = 0; i < HI_ISP_RLSC_POINTS ; i++)
        {
            pstRLsc->stRLscCaliResult[0].au16RGain[i]  = hi_ext_system_isp_radial_shading_r_gain0_read(ViPipe, i);
            pstRLsc->stRLscCaliResult[1].au16RGain[i]  = hi_ext_system_isp_radial_shading_r_gain1_read(ViPipe, i);
            pstRLsc->stRLscCaliResult[2].au16RGain[i]  = hi_ext_system_isp_radial_shading_r_gain2_read(ViPipe, i);

            pstRLsc->stRLscCaliResult[0].au16GrGain[i] = hi_ext_system_isp_radial_shading_gr_gain0_read(ViPipe, i);
            pstRLsc->stRLscCaliResult[1].au16GrGain[i] = hi_ext_system_isp_radial_shading_gr_gain1_read(ViPipe, i);
            pstRLsc->stRLscCaliResult[2].au16GrGain[i] = hi_ext_system_isp_radial_shading_gr_gain2_read(ViPipe, i);

            pstRLsc->stRLscCaliResult[0].au16GbGain[i] = hi_ext_system_isp_radial_shading_gb_gain0_read(ViPipe, i);
            pstRLsc->stRLscCaliResult[1].au16GbGain[i] = hi_ext_system_isp_radial_shading_gb_gain1_read(ViPipe, i);
            pstRLsc->stRLscCaliResult[2].au16GbGain[i] = hi_ext_system_isp_radial_shading_gb_gain2_read(ViPipe, i);

            pstRLsc->stRLscCaliResult[0].au16BGain[i]  = hi_ext_system_isp_radial_shading_b_gain0_read(ViPipe, i);
            pstRLsc->stRLscCaliResult[1].au16BGain[i]  = hi_ext_system_isp_radial_shading_b_gain1_read(ViPipe, i);
            pstRLsc->stRLscCaliResult[2].au16BGain[i]  = hi_ext_system_isp_radial_shading_b_gain2_read(ViPipe, i);
        }
    }

    //AdjustCenterPoint(ViPipe, pstRLsc);
    CalcRadius(ViPipe, pstRLsc);
    return;
}

/**
 * [RLscInitialize description]
 * @param  ViPipe    [Input ]  ViPipe Num
 * @param  pstRegCfg [Output]  ISP Register Config
 * @return           [Void]
 */
static HI_VOID RLscInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U16     i, j;
    HI_U32     u32DefGain;
    HI_U32     u32Width, u32Height;
    HI_U32     u32OffCenter;
    HI_U32     temp = (1 << 31);
    ISP_RLSC_S *pstRLsc    = HI_NULL;
    ISP_CTX_S  *pstIspCtx = HI_NULL;
    ISP_CMOS_DEFAULT_S *pstSnsDft  = HI_NULL;
    ISP_CMOS_RLSC_S    *pstCmosLsc = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    RLSC_GET_CTX(ViPipe, pstRLsc);
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);

    pstRLsc->bRLscFuncEn   = HI_TRUE;
    pstRLsc->bRadialCropEn = HI_TRUE; // Initialize as enable
    pstRLsc->enLightMode   = OPERATION_MODE_AUTO; //Initialize as AUTO mode
    pstRLsc->u16RadialStrength = HI_ISP_RLSC_DEFAULT_RADIAL_STR;

    pstRLsc->u16BlendRatio = HI_ISP_RLSC_DEFAULT_MANUAL_WEIGHT;
    pstRLsc->u8LightType1  = HI_ISP_RLSC_DEFAULT_LIGHT;
    pstRLsc->u8LightType2  = HI_ISP_RLSC_DEFAULT_LIGHT;

    pstRLsc->u16WbRGain = HI_ISP_RLSC_DEFAULT_WBGAIN;
    pstRLsc->u16WbBGain = HI_ISP_RLSC_DEFAULT_WBGAIN;

    pstCmosLsc = &pstSnsDft->stRLsc;

    u32Width  = hi_ext_sync_total_width_read(ViPipe);
    u32Height = hi_ext_sync_total_height_read(ViPipe);

    if (pstSnsDft->stRLsc.bValid)
    {
        pstRLsc->u8RadialScale = pstCmosLsc->u8Scale;

        pstRLsc->u16CenterRX  = pstCmosLsc->u16CenterRX;
        pstRLsc->u16CenterRY  = pstCmosLsc->u16CenterRY;
        pstRLsc->u16CenterGrX = pstCmosLsc->u16CenterGrX;
        pstRLsc->u16CenterGrY = pstCmosLsc->u16CenterGrY;
        pstRLsc->u16CenterGbX = pstCmosLsc->u16CenterGbX;
        pstRLsc->u16CenterGbY = pstCmosLsc->u16CenterGbY;
        pstRLsc->u16CenterBX  = pstCmosLsc->u16CenterBX;
        pstRLsc->u16CenterBY  = pstCmosLsc->u16CenterBY;

        pstRLsc->u16OffCenterR  = pstCmosLsc->u16OffCenterR;
        pstRLsc->u16OffCenterGr = pstCmosLsc->u16OffCenterGr;
        pstRLsc->u16OffCenterGb = pstCmosLsc->u16OffCenterGb;
        pstRLsc->u16OffCenterB  = pstCmosLsc->u16OffCenterB;

        CalcRadius(ViPipe, pstRLsc);

        for ( j = 0 ; j < 3 ; j++ )
        {
            //Initialize White Balance gain
            pstRLsc->stRLscCaliResult[j].u16WBRGain = pstCmosLsc->stLscCalibTable[j].u16WBRGain;
            pstRLsc->stRLscCaliResult[j].u16WBBGain = pstCmosLsc->stLscCalibTable[j].u16WBBGain;

            //Initialize
            for (i = 0; i < HI_ISP_RLSC_POINTS ; i++)
            {
                pstRLsc->stRLscCaliResult[j].au16RGain[i]  = pstCmosLsc->stLscCalibTable[j].au16R_Gain[i];
                pstRLsc->stRLscCaliResult[j].au16GrGain[i] = pstCmosLsc->stLscCalibTable[j].au16Gr_Gain[i];
                pstRLsc->stRLscCaliResult[j].au16GbGain[i] = pstCmosLsc->stLscCalibTable[j].au16Gb_Gain[i];
                pstRLsc->stRLscCaliResult[j].au16BGain[i]  = pstCmosLsc->stLscCalibTable[j].au16B_Gain[i];
            }
        }
    }
    else
    {
        // With no cmos
        pstRLsc->u8RadialScale = HI_ISP_RLSC_DEFAULT_SCALE;//default scale: 4.12

        pstRLsc->u16CenterRX  = u32Width  / 2;
        pstRLsc->u16CenterRY  = u32Height / 2;
        pstRLsc->u16CenterGrX = u32Width  / 2;
        pstRLsc->u16CenterGrY = u32Height / 2;
        pstRLsc->u16CenterGbX = u32Width  / 2;
        pstRLsc->u16CenterGbY = u32Height / 2;
        pstRLsc->u16CenterBX  = u32Width  / 2;
        pstRLsc->u16CenterBY  = u32Height / 2;

        u32OffCenter = temp / DIV_0_TO_1(pstRLsc->u16CenterRX * pstRLsc->u16CenterRX + pstRLsc->u16CenterRY * pstRLsc->u16CenterRY);

        pstRLsc->u16OffCenterR  = u32OffCenter;
        pstRLsc->u16OffCenterGr = u32OffCenter;
        pstRLsc->u16OffCenterGb = u32OffCenter;
        pstRLsc->u16OffCenterB  = u32OffCenter;

        AdjustCenterPoint(ViPipe, pstRLsc);
        CalcRadius(ViPipe, pstRLsc);

        u32DefGain = g_au16GainStrDef[pstRLsc->u8RadialScale];

        for ( j = 0 ; j < 3 ; j++ )
        {
            pstRLsc->stRLscCaliResult[j].u16WBRGain = HI_ISP_RLSC_DEFAULT_WBGAIN;
            pstRLsc->stRLscCaliResult[j].u16WBBGain = HI_ISP_RLSC_DEFAULT_WBGAIN;

            for (i = 0; i < HI_ISP_RLSC_POINTS; i++)
            {
                pstRLsc->stRLscCaliResult[j].au16RGain[i]  = u32DefGain;
                pstRLsc->stRLscCaliResult[j].au16GrGain[i] = u32DefGain;
                pstRLsc->stRLscCaliResult[j].au16GbGain[i] = u32DefGain;
                pstRLsc->stRLscCaliResult[j].au16BGain[i]  = u32DefGain;
            }
        }
    }

    pstRLsc->bLutUpdate     = HI_TRUE;
    pstRLsc->bLscCoefUpdate = HI_TRUE;

    if ( DYNAMIC_RANGE_XDR == pstIspCtx->stHdrAttr.enDynamicRange )
    {
        pstRLsc->bLscEnable = HI_FALSE;
    }
    else
    {
        pstRLsc->bLscEnable = HI_FALSE;
    }

    return;
}

static HI_VOID RLsc_Usr_Fw(VI_PIPE ViPipe, HI_U8 u8CurBlk, ISP_RLSC_S *pstRLsc, ISP_RLSC_USR_CFG_S *pstUsrRegCfg)
{
    RLscGetGainLut(ViPipe, pstRLsc, pstUsrRegCfg);

    pstUsrRegCfg->u8GainScale     = pstRLsc->u8RadialScale;
    pstUsrRegCfg->bLutUpdate      = HI_TRUE;

    pstUsrRegCfg->u16CenterRX     = pstRLsc->u16CenterRX;
    pstUsrRegCfg->u16CenterRY     = pstRLsc->u16CenterRY;
    pstUsrRegCfg->u16OffCenterR   = pstRLsc->u16OffCenterR;

    pstUsrRegCfg->u16CenterGrX    = pstRLsc->u16CenterGrX;
    pstUsrRegCfg->u16CenterGrY    = pstRLsc->u16CenterGrY;
    pstUsrRegCfg->u16OffCenterGr  = pstRLsc->u16OffCenterGr;

    pstUsrRegCfg->u16CenterGbX    = pstRLsc->u16CenterGbX;
    pstUsrRegCfg->u16CenterGbY    = pstRLsc->u16CenterGbY;
    pstUsrRegCfg->u16OffCenterGb  = pstRLsc->u16OffCenterGb;

    pstUsrRegCfg->u16CenterBX     = pstRLsc->u16CenterBX;
    pstUsrRegCfg->u16CenterBY     = pstRLsc->u16CenterBY;
    pstUsrRegCfg->u16OffCenterB   = pstRLsc->u16OffCenterB;

    pstUsrRegCfg->u32ValidRadius  = pstRLsc->u32ValidRadius;

    pstUsrRegCfg->u32UpdateIndex += 1;
    return;
}


static HI_S32 RLscImageSize(HI_U8 i, ISP_RLSC_USR_CFG_S *pstRLscUsrRegCfg, ISP_BLOCK_ATTR_S *pstBlockAttr)
{
    ISP_RECT_S stBlockRect;

    ISP_GetBlockRect(&stBlockRect, pstBlockAttr, i);
    pstRLscUsrRegCfg->u16WidthOffset = ABS(stBlockRect.s32X);

    return HI_SUCCESS;
}

static __inline HI_S32 RLscImageResWrite(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8 i;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    for (i = 0; i < pstIspCtx->stBlockAttr.u8BlockNum; i++)
    {
        RLscImageSize(i, &pstRegCfg->stAlgRegCfg[i].stRLscRegCfg.stUsrRegCfg, &pstIspCtx->stBlockAttr);
        pstRegCfg->stAlgRegCfg[i].stGCacRegCfg.stUsrRegCfg.u32UpdateIndex += 1;
        pstRegCfg->stAlgRegCfg[i].stGCacRegCfg.stUsrRegCfg.bResh           = HI_TRUE;
    }

    pstRegCfg->unKey.bit1RLscCfg = 1;

    return HI_SUCCESS;
}

static HI_BOOL __inline CheckRLscOpen(ISP_RLSC_S *pstRLsc)
{
    return (HI_TRUE == pstRLsc->bLscEnable);
}

HI_S32 ISP_RLscInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    RLscInitialize(ViPipe, pstRegCfg);
    RLscRegsInitialize(ViPipe, pstRegCfg);
    RLscExtRegsInitialize(ViPipe);

    return HI_SUCCESS;
}

HI_S32 ISP_RLscRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                   HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_S32 i;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_RLSC_S *pstRLsc = HI_NULL;
    ISP_REG_CFG_S *pstRegCfg  = (ISP_REG_CFG_S *)pRegCfg;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    RLSC_GET_CTX(ViPipe, pstRLsc);

    if (pstIspCtx->stLinkage.bDefectPixel)
    {
        return HI_SUCCESS;
    }

    pstRLsc->bLscEnable = hi_ext_system_isp_radial_shading_enable_read(ViPipe);

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        pstRegCfg->stAlgRegCfg[i].stRLscRegCfg.bRLscEn = pstRLsc->bLscEnable;
    }

    pstRegCfg->unKey.bit1RLscCfg = 1;

    /*check hardware setting*/
    if (!CheckRLscOpen(pstRLsc))
    {
        return HI_SUCCESS;
    }

    RLscReadExtRegs(ViPipe);

    if (pstRLsc->bLscCoefUpdate)
    {
        for (i = 0; i < pstRegCfg->u8CfgNum; i++)
        {
            pstRegCfg->stAlgRegCfg[i].stRLscRegCfg.stUsrRegCfg.bCoefUpdate    = HI_TRUE;
            pstRegCfg->stAlgRegCfg[i].stRLscRegCfg.stUsrRegCfg.bRLscFuncEn    = pstRLsc->bRLscFuncEn;
            pstRegCfg->stAlgRegCfg[i].stRLscRegCfg.stUsrRegCfg.bRadialCropEn  = pstRLsc->bRadialCropEn;
            //pstRegCfg->stAlgRegCfg[i].stRLscRegCfg.stUsrRegCfg.u32ValidRadius = pstRLsc->u32ValidRadius;
            pstRegCfg->stAlgRegCfg[i].stRLscRegCfg.stUsrRegCfg.u16GainStr     = pstRLsc->u16RadialStrength;
        }
    }

    if (pstRLsc->bLutUpdate)
    {
        for ( i = 0; i < pstRegCfg->u8CfgNum; i++)
        {
            RLsc_Usr_Fw(ViPipe, i, pstRLsc, &pstRegCfg->stAlgRegCfg[i].stRLscRegCfg.stUsrRegCfg);
        }
    }

    return HI_SUCCESS;
}

HI_S32 ISP_RLscCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);

    switch (u32Cmd)
    {
        case ISP_CHANGE_IMAGE_MODE_SET :
            RLscImageResWrite(ViPipe, &pRegCfg->stRegCfg);
            break;
        default :
            break;
    }
    return HI_SUCCESS;
}

HI_S32 ISP_RLscExit(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);

    for (i = 0; i < pRegCfg->stRegCfg.u8CfgNum; i++)
    {
        pRegCfg->stRegCfg.stAlgRegCfg[i].stRLscRegCfg.bRLscEn = HI_FALSE;
    }

    pRegCfg->stRegCfg.unKey.bit1RLscCfg = 1;

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterRLsc(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_RLSC;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_RLscInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_RLscRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_RLscCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_RLscExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


