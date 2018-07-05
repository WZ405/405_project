/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_demosaic.c
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

#define HI_ISP_DEMOSAIC_LUT_LEN (16)
#define HI_DEMOSAIC_BITDEPTH    (12)
#define HI_ISP_DEMOSAIC_LUT_LENGTH  (17)

#ifndef HI_DEMOSAIC_MAX
#define HI_DEMOSAIC_MAX(a, b) (((a) < (b)) ?  (b) : (a))
#endif

#ifndef HI_DEMOSAIC_MIN
#define HI_DEMOSAIC_MIN(a, b) (((a) > (b)) ?  (b) : (a))
#endif

#define HI_DEMOSAIC_CLIP3(low, high, x) (HI_DEMOSAIC_MAX( HI_DEMOSAIC_MIN((x), high), low))

static const  HI_U32 au32DemosaicIsoLut[ISP_AUTO_ISO_STRENGTH_NUM] = {100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600, 51200, 102400, 204800, 409600, 819200, 1638400, 3276800};
static const  HI_S32 g_as32DemosaicFilterCoef[8][3] = {{ -1, 4, 26}, { -1, 5, 24}, { -1, 6, 22}, { -1, 7, 20}, { -1, 8, 18}, { -1, 9, 16}, {0, 8, 16}, {1, 7, 16}};
static const  HI_U8  g_au8EhcGainLut[HI_ISP_DEMOSAIC_LUT_LENGTH]  = {255, 200, 200, 180, 160, 128, 96, 64, 36, 32, 28, 28, 24, 24, 16, 16, 16};

static const  HI_U8  g_au8DetailSmoothRange[ISP_AUTO_ISO_STRENGTH_NUM] = {2, 2, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 7};
static const  HI_U16 g_au16DetailSmoothStr[ISP_AUTO_ISO_STRENGTH_NUM]  = {256, 256, 256, 256, 256, 256, 256, 256, 256, 224, 224, 200, 200, 128, 128, 128};

typedef struct hiISP_DEMOSAIC_AUTO_S
{
    HI_U8    au8DetailSmoothRange[ISP_AUTO_ISO_STRENGTH_NUM];   /*RW;Range:[0x1,0x8];Format:4.0;   Fine strength for noise suppressing*/
    HI_U16   au16DetailSmoothStr[ISP_AUTO_ISO_STRENGTH_NUM];    /*RW;Range:[0x0,0x100];Format:9.0; Strength of detail smooth*/
} ISP_DEMOSAIC_AUTO_S;

typedef struct hiISP_DEMOSAIC_MANUAL_S
{
    HI_U8    u8DetailSmoothRange;       /*RW;Range:[0x1,0x8];Format:3.0; Range of detail smooth*/
    HI_U16   u16DetailSmoothStr;        /*RW;Range:[0x0,0x100];Format:9.0; Strength of detail smooth*/
} ISP_DEMOSAIC_MANUAL_S;

typedef struct hiISP_DEMOSAIC_S
{
    // Processing Depth
    HI_BOOL bEnable;           //u1.0
    HI_BOOL bVhdmEnable;       //u1.0
    HI_BOOL bNddmEnable;       //u1.0
    HI_BOOL bGFCtrlEnable;     //u1.0
    HI_BOOL bInit;
    HI_BOOL bDemAttrUpdate;

    HI_U8   u8BitDepthPrc;    //u5.0
    HI_U8   u8WdrMode;
    HI_U16  u16NddmStrength;
    HI_U32  au32sigma[HI_ISP_DEMOSAIC_LUT_LENGTH];

    HI_U8   au8EhcGainLut          [HI_ISP_DEMOSAIC_LUT_LENGTH]; //u8.0,    usm gain at each luma
    HI_U8   au8LutAwbGFGainLow     [ISP_AUTO_ISO_STRENGTH_NUM]; //u3.4,
    HI_U8   au8LutAwbGFGainHig     [ISP_AUTO_ISO_STRENGTH_NUM]; //u3.4,
    HI_U8   au8LutAwbGFGainMax     [ISP_AUTO_ISO_STRENGTH_NUM]; //u4.0,
    HI_U8   au8LutBldrCbCr         [ISP_AUTO_ISO_STRENGTH_NUM]; //u5.0,  gf cr-cb strength
    HI_U8   au8LutBldrGFStr        [ISP_AUTO_ISO_STRENGTH_NUM]; //u5.0,  gf r-g-b strength
    HI_U8   au8LutDitherMax        [ISP_AUTO_ISO_STRENGTH_NUM]; //u8.0,
    HI_U8   au8LutClipDeltaGain    [ISP_AUTO_ISO_STRENGTH_NUM]; //u8.0,
    HI_U8   au8LutClipAdjustMax    [ISP_AUTO_ISO_STRENGTH_NUM]; //u8.0,
    HI_U8   au8LutFilterStrIntp    [ISP_AUTO_ISO_STRENGTH_NUM]; //u6.0, [0 16]
    HI_U8   au8LutFilterStrFilt    [ISP_AUTO_ISO_STRENGTH_NUM]; //u6.0, [0 16]
    HI_U8   au8LutBldrGray         [ISP_AUTO_ISO_STRENGTH_NUM]; //u5.0, 0~16
    HI_U8   au8LutCcHFMaxRatio     [ISP_AUTO_ISO_STRENGTH_NUM]; //u5.0, 0~16
    HI_U8   au8LutCcHFMinRatio     [ISP_AUTO_ISO_STRENGTH_NUM]; //u5.0, 0~16
    HI_S8   as8LutFcrGFGain        [ISP_AUTO_ISO_STRENGTH_NUM]; //s3.2, fcr control

    HI_U16  au16LutNddmStrength    [ISP_AUTO_ISO_STRENGTH_NUM]; //u9.0,
    HI_U16  au16LuthfIntpBldLow    [ISP_AUTO_ISO_STRENGTH_NUM]; //u9.0,
    HI_U16  au16LuthfIntpBldHig    [ISP_AUTO_ISO_STRENGTH_NUM]; //u9.0,
    HI_U16  au16LutSatuRThFix      [ISP_AUTO_ISO_STRENGTH_NUM]; //u9.0,
    HI_U16  au16LutSatuRThLow      [ISP_AUTO_ISO_STRENGTH_NUM]; //u9.0,
    HI_U16  au16LutSatuRThHig      [ISP_AUTO_ISO_STRENGTH_NUM]; //u9.0,
    HI_U16  au16LutSatuBThFix      [ISP_AUTO_ISO_STRENGTH_NUM]; //u9.0,
    HI_U16  au16LutSatuBThLow      [ISP_AUTO_ISO_STRENGTH_NUM]; //u9.0,
    HI_U16  au16LutSatuBThHig      [ISP_AUTO_ISO_STRENGTH_NUM]; //u9.0,
    HI_U16  au16LutFcrDetLow       [ISP_AUTO_ISO_STRENGTH_NUM]; //u12.0,  fcr det thresh
    HI_U16  au16LutSharpenLumaStr  [ISP_AUTO_ISO_STRENGTH_NUM]; //u12.0,
    HI_U16  au16LutClipDeltaFiltLow[ISP_AUTO_ISO_STRENGTH_NUM]; //u12.0,
    HI_U16  au16LutClipDeltaFiltHig[ISP_AUTO_ISO_STRENGTH_NUM]; //u12.0,
    HI_U16  au16LutEdgeSmoothLowThr[ISP_AUTO_ISO_STRENGTH_NUM]; //u10.0,
    HI_U16  au16LutEdgeSmoothHigThr[ISP_AUTO_ISO_STRENGTH_NUM]; //u10.0,
    HI_U16  au16LutAntiAliasLowThr [ISP_AUTO_ISO_STRENGTH_NUM]; //u10.0,
    HI_U16  au16LutAntiAliasHigThr [ISP_AUTO_ISO_STRENGTH_NUM]; //u10.0,

    ISP_OP_TYPE_E enOpType;
    ISP_DEMOSAIC_AUTO_S  stAuto;
    ISP_DEMOSAIC_MANUAL_S stManual;
} ISP_DEMOSAIC_S;

ISP_DEMOSAIC_S g_astDemosaicCtx[ISP_MAX_PIPE_NUM] = {{0}};
#define DEMOSAIC_GET_CTX(dev, pstCtx)   pstCtx = &g_astDemosaicCtx[dev]

static HI_VOID  DemosaicInitFwLinear(VI_PIPE ViPipe)
{
    ISP_DEMOSAIC_S *pstDemosaic = HI_NULL;

    HI_U8   au8LutBldrCbCr[HI_ISP_DEMOSAIC_LUT_LEN]           = {16, 16, 8, 6, 6, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    HI_U8   au8LutDitherMax[HI_ISP_DEMOSAIC_LUT_LEN]          = {3, 3, 3, 5, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};
    HI_U8   au8LutFilterStrIntp[HI_ISP_DEMOSAIC_LUT_LEN]      = {8, 8, 8, 8, 8, 8, 8, 10, 10, 10, 10, 10, 10, 10, 10, 10};
    HI_U8   au8LutAwbGFGainLow[HI_ISP_DEMOSAIC_LUT_LEN]       = {32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32 };
    HI_U8   au8LutAwbGFGainHig[HI_ISP_DEMOSAIC_LUT_LEN]       = {64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64 };
    HI_U8   au8LutAwbGFGainMax[HI_ISP_DEMOSAIC_LUT_LEN]       = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
    HI_U8   au8LutBldrGray[HI_ISP_DEMOSAIC_LUT_LEN]           = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    HI_U8   au8LutCcHFMaxRatio[HI_ISP_DEMOSAIC_LUT_LEN]       = {8, 8, 8, 8, 10, 10, 10, 12, 12, 14, 14, 16, 16, 16, 16, 16};
    HI_U8   au8LutCcHFMinRatio[HI_ISP_DEMOSAIC_LUT_LEN]       = {0, 0, 0, 0, 0, 0, 2, 2, 4, 4, 4, 8, 10, 10, 12, 14};
    HI_S8   as8LutFcrGFGain[HI_ISP_DEMOSAIC_LUT_LEN]          = { -15, -15, 12, 10, 8, 6, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    HI_U16  au16LutFcrDetLow[HI_ISP_DEMOSAIC_LUT_LEN]         = {120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120};
    HI_U16  au16LutSatuRThLow[HI_ISP_DEMOSAIC_LUT_LEN]        = {110, 110, 110, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130};
    HI_U16  au16LutSatuRThHig[HI_ISP_DEMOSAIC_LUT_LEN]        = {200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200};
    HI_U16  au16LutSatuRThFix[HI_ISP_DEMOSAIC_LUT_LEN]        = {180, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92};
    HI_U16  au16LutSatuBThLow[HI_ISP_DEMOSAIC_LUT_LEN]        = {110, 110, 110, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130};
    HI_U16  au16LutSatuBThHig[HI_ISP_DEMOSAIC_LUT_LEN]        = {200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200};
    HI_U16  au16LutSatuBThFix[HI_ISP_DEMOSAIC_LUT_LEN]        = {100, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92};
    HI_U16  au16LutNddmStrength[ISP_AUTO_ISO_STRENGTH_NUM]    = {128, 128, 128, 192, 192, 224, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240};
    HI_U16  au16LutSharpenLumaStr[ISP_AUTO_ISO_STRENGTH_NUM]  = {12 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256};
    HI_U16  au16LutClipDeltaFiltLow[HI_ISP_DEMOSAIC_LUT_LEN]  = {10, 10, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20};
    HI_U16  au16LutClipDeltaFiltHig[HI_ISP_DEMOSAIC_LUT_LEN]  = {40, 40, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80};
    HI_U16  au16LutEdgeSmoothLowThr[ISP_AUTO_ISO_STRENGTH_NUM] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
    HI_U16  au16LutEdgeSmoothHigThr[ISP_AUTO_ISO_STRENGTH_NUM] = {48, 48, 48, 48, 48, 48, 48, 48, 43, 43, 41, 36, 36, 32, 32, 32};
    HI_U16  au16LutAntiAliasLowThr[ISP_AUTO_ISO_STRENGTH_NUM] = {152, 152, 152, 152, 256, 256, 974, 974, 974, 974, 974, 974, 974, 974, 974, 974};
    HI_U16  au16LutAntiAliasHigThr[ISP_AUTO_ISO_STRENGTH_NUM] = {208, 208, 261, 261, 261, 261, 975, 975, 975, 975, 975, 974, 974, 974, 974, 974};

    DEMOSAIC_GET_CTX(ViPipe, pstDemosaic);

    memcpy(pstDemosaic->au8LutBldrCbCr,          au8LutBldrCbCr,          sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au8LutDitherMax,         au8LutDitherMax,         sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au8LutFilterStrIntp,     au8LutFilterStrIntp,     sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->as8LutFcrGFGain,         as8LutFcrGFGain,         sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au8LutAwbGFGainLow,      au8LutAwbGFGainLow,      sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au8LutAwbGFGainHig,      au8LutAwbGFGainHig,      sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au8LutAwbGFGainMax,      au8LutAwbGFGainMax,      sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au8LutBldrGray,          au8LutBldrGray,          sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au8LutCcHFMaxRatio,      au8LutCcHFMaxRatio,      sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au8LutCcHFMinRatio,      au8LutCcHFMinRatio,      sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutNddmStrength,     au16LutNddmStrength,     sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutFcrDetLow,        au16LutFcrDetLow,        sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutSatuRThLow,       au16LutSatuRThLow,       sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutSatuRThHig,       au16LutSatuRThHig,       sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutSatuRThFix,       au16LutSatuRThFix,       sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutSatuBThLow,       au16LutSatuBThLow,       sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutSatuBThHig,       au16LutSatuBThHig,       sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutSatuBThFix,       au16LutSatuBThFix,       sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutClipDeltaFiltLow, au16LutClipDeltaFiltLow, sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutClipDeltaFiltHig, au16LutClipDeltaFiltHig, sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutSharpenLumaStr,   au16LutSharpenLumaStr,   sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutEdgeSmoothLowThr, au16LutEdgeSmoothLowThr, sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutEdgeSmoothHigThr, au16LutEdgeSmoothHigThr, sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutAntiAliasLowThr,  au16LutAntiAliasLowThr,  sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutAntiAliasHigThr,  au16LutAntiAliasHigThr,  sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    return;
}

static HI_VOID  DemosaicInitFwWdr(VI_PIPE ViPipe)
{
    ISP_DEMOSAIC_S *pstDemosaic = HI_NULL;

    HI_U8   au8LutBldrCbCr[HI_ISP_DEMOSAIC_LUT_LEN]           = {6, 6, 6, 4, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    HI_U8   au8LutDitherMax[HI_ISP_DEMOSAIC_LUT_LEN]          = {0, 1, 2, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};
    HI_U8   au8LutFilterStrIntp[HI_ISP_DEMOSAIC_LUT_LEN]      = {8, 8, 8, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
    HI_U8   au8LutAwbGFGainLow[HI_ISP_DEMOSAIC_LUT_LEN]       = {32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32 };
    HI_U8   au8LutAwbGFGainHig[HI_ISP_DEMOSAIC_LUT_LEN]       = {64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64 };
    HI_U8   au8LutAwbGFGainMax[HI_ISP_DEMOSAIC_LUT_LEN]       = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };
    HI_U8   au8LutCcHFMaxRatio[HI_ISP_DEMOSAIC_LUT_LEN]       = {8, 8, 8, 8, 10, 10, 10, 12, 12, 14, 14, 16, 16, 16, 16, 16};
    HI_U8   au8LutCcHFMinRatio[HI_ISP_DEMOSAIC_LUT_LEN]       = {0, 0, 0, 0, 0, 0, 2, 2, 4, 4, 4, 8, 10, 10, 12, 14};
    HI_U8   au8LutBldrGray[HI_ISP_DEMOSAIC_LUT_LEN]           = {16, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    HI_S8   as8LutFcrGFGain[HI_ISP_DEMOSAIC_LUT_LEN]          = {12, 12, 12, 10, 8, 6, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    HI_U16  au16LutFcrDetLow[HI_ISP_DEMOSAIC_LUT_LEN]         = {150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150};
    HI_U16  au16LutSatuRThLow[HI_ISP_DEMOSAIC_LUT_LEN]        = {150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150};
    HI_U16  au16LutSatuRThHig[HI_ISP_DEMOSAIC_LUT_LEN]        = {200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200};
    HI_U16  au16LutSatuRThFix[HI_ISP_DEMOSAIC_LUT_LEN]        = {92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92};
    HI_U16  au16LutSatuBThLow[HI_ISP_DEMOSAIC_LUT_LEN]        = {110, 110, 110, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130};
    HI_U16  au16LutSatuBThHig[HI_ISP_DEMOSAIC_LUT_LEN]        = {200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200};
    HI_U16  au16LutSatuBThFix[HI_ISP_DEMOSAIC_LUT_LEN]        = {100, 100, 100, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92};
    HI_U16  au16LutNddmStrength[ISP_AUTO_ISO_STRENGTH_NUM]    = {32, 32, 24, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
    HI_U16  au16LutSharpenLumaStr[HI_ISP_DEMOSAIC_LUT_LEN]    = {0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256, 0 + 256}; //[-256, 256]
    HI_U16  au16LutClipDeltaFiltLow[HI_ISP_DEMOSAIC_LUT_LEN]  = {10, 10, 10, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20};
    HI_U16  au16LutClipDeltaFiltHig[HI_ISP_DEMOSAIC_LUT_LEN]  = {40, 40, 40, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80};
    HI_U16  au16LutEdgeSmoothLowThr[ISP_AUTO_ISO_STRENGTH_NUM] = {16, 16, 16, 16, 16, 16, 1022, 1022, 1022, 1022, 1022, 1022, 1022, 1022, 1022, 1022};
    HI_U16  au16LutEdgeSmoothHigThr[ISP_AUTO_ISO_STRENGTH_NUM] = {48, 48, 48, 48, 48, 48, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023};
    HI_U16  au16LutAntiAliasLowThr[ISP_AUTO_ISO_STRENGTH_NUM] = {152, 152, 152, 152, 256, 256, 974, 974, 974, 974, 974, 974, 974, 974, 974, 974};
    HI_U16  au16LutAntiAliasHigThr[ISP_AUTO_ISO_STRENGTH_NUM] = {208, 208, 261, 261, 261, 261, 975, 975, 975, 975, 975, 974, 974, 974, 974, 974};

    DEMOSAIC_GET_CTX(ViPipe, pstDemosaic);

    memcpy(pstDemosaic->au8LutBldrCbCr,          au8LutBldrCbCr,          sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au8LutDitherMax,         au8LutDitherMax,         sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au8LutFilterStrIntp,     au8LutFilterStrIntp,     sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->as8LutFcrGFGain,         as8LutFcrGFGain,         sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au8LutCcHFMaxRatio,      au8LutCcHFMaxRatio,      sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au8LutCcHFMinRatio,      au8LutCcHFMinRatio,      sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au8LutAwbGFGainLow,      au8LutAwbGFGainLow,      sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au8LutAwbGFGainHig,      au8LutAwbGFGainHig,      sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au8LutAwbGFGainMax,      au8LutAwbGFGainMax,      sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au8LutBldrGray,          au8LutBldrGray,          sizeof(HI_U8)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutNddmStrength,     au16LutNddmStrength,     sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutFcrDetLow,        au16LutFcrDetLow,        sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutSatuRThLow,       au16LutSatuRThLow,       sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutSatuRThHig,       au16LutSatuRThHig,       sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutSatuBThLow,       au16LutSatuBThLow,       sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutSatuBThHig,       au16LutSatuBThHig,       sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutSatuRThFix,       au16LutSatuRThFix,       sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutSatuBThFix,       au16LutSatuBThFix,       sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutClipDeltaFiltLow, au16LutClipDeltaFiltLow, sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutClipDeltaFiltHig, au16LutClipDeltaFiltHig, sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutSharpenLumaStr,   au16LutSharpenLumaStr,   sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutEdgeSmoothLowThr, au16LutEdgeSmoothLowThr, sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutEdgeSmoothHigThr, au16LutEdgeSmoothHigThr, sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutAntiAliasLowThr,  au16LutAntiAliasLowThr,  sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);
    memcpy(pstDemosaic->au16LutAntiAliasHigThr,  au16LutAntiAliasHigThr,  sizeof(HI_U16)*HI_ISP_DEMOSAIC_LUT_LEN);

    return;
}

static HI_VOID DemosaicStaticRegsInitialize(VI_PIPE ViPipe, ISP_DEMOSAIC_STATIC_CFG_S *pstDemosaicStaticRegCfg, HI_U32 i)
{
    pstDemosaicStaticRegCfg->bFcsEnable           = HI_ISP_DEMOSAIC_FCS_EN_DEFAULT;
    pstDemosaicStaticRegCfg->bAHDEnable           = HI_ISP_DEMOSAIC_AND_EN_DEFAULT;
    pstDemosaicStaticRegCfg->bDeFakeEnable        = HI_ISP_DEMOSAIC_DE_FAKE_EN_DEFAULT;
    pstDemosaicStaticRegCfg->bAlphaFilter         = HI_ISP_NDDM_ALPHA_FILTER_EN_DEFAULT;
    pstDemosaicStaticRegCfg->bAptFltEn            = HI_ISP_NDDM_ADT_FLT_EN_DEFAULT;
    pstDemosaicStaticRegCfg->u8GClipBitSft        = HI_ISP_DEMOSAIC_G_CLIP_SFT_BIT_DEFAULT;
    pstDemosaicStaticRegCfg->u8hvBlendLimit1      = HI_ISP_DEMOSAIC_BLENDLIMIT1_DEFAULT;
    pstDemosaicStaticRegCfg->u8hvBlendLimit2      = HI_ISP_DEMOSAIC_BLENDLIMIT2_DEFAULT;
    pstDemosaicStaticRegCfg->u8hvColorRatio       = HI_ISP_DEMOSAIC_HV_RATIO_DEFAULT;
    pstDemosaicStaticRegCfg->u8hvSelection        = HI_ISP_DEMOSAIC_HV_SEL_DEFAULT;
    pstDemosaicStaticRegCfg->u8BldrGrGb           = HI_ISP_NDDM_BLDR_GRGB_DEFAULT;
    pstDemosaicStaticRegCfg->u8FcrScale           = HI_ISP_NDDM_FCR_SCALE_DEFAULT;
    pstDemosaicStaticRegCfg->u8ClipDeltaGain      = HI_ISP_NDDM_CLIP_DELTA_GAIN_DEFAULT;
    pstDemosaicStaticRegCfg->u8ClipAdjustMax      = HI_ISP_NDDM_CLIP_ADJUST_MAX_DEFAULT;
    pstDemosaicStaticRegCfg->u8ShtCtrlGain        = HI_ISP_NDDM_SHT_CTRL_GAIN_DEFAULT;
    pstDemosaicStaticRegCfg->u8MultiMF            = HI_ISP_NDDM_MULTI_MF_DEFAULT;
    pstDemosaicStaticRegCfg->u8MultiMFRed         = HI_ISP_NDDM_MULTI_MF_R_DEFAULT;
    pstDemosaicStaticRegCfg->u8MultiMFBlue        = HI_ISP_NDDM_MULTI_MF_B_DEFAULT;
    pstDemosaicStaticRegCfg->u8DitherMask         = HI_ISP_NDDM_DITH_MASK_DEFAULT;
    pstDemosaicStaticRegCfg->u8DitherRatio        = HI_ISP_NDDM_DITH_RATIO_DEFAULT;
    pstDemosaicStaticRegCfg->u8BldrGFStr          = HI_ISP_NDDM_BLDR_GF_STR_DEFAULT;
    pstDemosaicStaticRegCfg->u8CxVarMaxRate       = HI_ISP_DEMOSAIC_CX_VAR_MAX_RATE_DEFAULT;
    pstDemosaicStaticRegCfg->u8CxVarMinRate       = HI_ISP_DEMOSAIC_CX_VAR_MIN_RATE_DEFAULT;
    pstDemosaicStaticRegCfg->u16hvBlendRatio1     = HI_ISP_DEMOSAIC_BLENDRATIO1_DEFAULT;
    pstDemosaicStaticRegCfg->u16hvBlendRatio2     = HI_ISP_DEMOSAIC_BLENDRATIO2_DEFAULT;
    pstDemosaicStaticRegCfg->u16AhdPart1          = HI_ISP_DEMOSAIC_AHDPART1_DEFAULT;
    pstDemosaicStaticRegCfg->u16AhdPart2          = HI_ISP_DEMOSAIC_AHDPART2_DEFAULT;
    pstDemosaicStaticRegCfg->u16GFThLow           = HI_ISP_NDDM_GF_TH_LOW_DEFAULT;
    pstDemosaicStaticRegCfg->u16GFThHig           = HI_ISP_NDDM_GF_TH_HIGH_DEFAULT;
    pstDemosaicStaticRegCfg->u16ClipUSM           = HI_ISP_NDDM_CLIP_USM_DEFAULT;
    pstDemosaicStaticRegCfg->u16SatuThLow         = HI_ISP_NDDM_SATU_TH_LOW_DEFAULT;
    pstDemosaicStaticRegCfg->u16SatuThHig         = HI_ISP_NDDM_SATU_TH_HIGH_DEFAULT;
    pstDemosaicStaticRegCfg->u16SatuThFix         = HI_ISP_NDDM_SATU_TH_FIX_DEFAULT;
    pstDemosaicStaticRegCfg->u16GrayThLow         = HI_ISP_NDDM_GRAY_TH_LOW_DEFAULT;
    pstDemosaicStaticRegCfg->u16GrayThHig         = HI_ISP_NDDM_GRAY_TH_HIGH_DEFAULT;
    pstDemosaicStaticRegCfg->u16GrayThFixLow      = HI_ISP_NDDM_GRAY_TH_FIX_LOW_DEFAULT;
    pstDemosaicStaticRegCfg->u16GrayThFixHig      = HI_ISP_NDDM_GRAY_TH_FIX_HIGH_DEFAULT;
    pstDemosaicStaticRegCfg->u16FcrLimitLow       = HI_ISP_NDDM_FCR_LIMIT_LOW_DEFAULT;
    pstDemosaicStaticRegCfg->u16FcrLimitHigh      = HI_ISP_NDDM_FCR_LIMIT_HIGH_DEFAULT;
    pstDemosaicStaticRegCfg->u16ShtCtrlTh         = HI_ISP_NDDM_SHT_CTRL_TH_DEFAULT;
    pstDemosaicStaticRegCfg->u16ClipRUdSht        = HI_ISP_NDDM_CLIP_R_UD_SHT_DEFAULT;
    pstDemosaicStaticRegCfg->u16ClipROvSht        = HI_ISP_NDDM_CLIP_R_OV_SHT_DEFAULT;
    pstDemosaicStaticRegCfg->u16ClipBUdSht        = HI_ISP_NDDM_CLIP_B_UD_SHT_DEFAULT;
    pstDemosaicStaticRegCfg->u16ClipBOvSht        = HI_ISP_NDDM_CLIP_B_OV_SHT_DEFAULT;
    pstDemosaicStaticRegCfg->u16CbCrAvgThr        = HI_ISP_DEMOSAIC_CBCR_AVG_THLD_DEFAULT;

    pstDemosaicStaticRegCfg->bResh = HI_TRUE;

    return;
}

static HI_VOID DemosaicDynaRegsInitialize(ISP_DEMOSAIC_DYNA_CFG_S *pstDemosaicDynaRegCfg)
{
    HI_U32 n;

    pstDemosaicDynaRegCfg->u8Lpff0              = HI_ISP_DEMOSAIC_LPF_F0_DEFAULT;
    pstDemosaicDynaRegCfg->u8Lpff1              = HI_ISP_DEMOSAIC_LPF_F1_DEFAULT;
    pstDemosaicDynaRegCfg->u8Lpff2              = HI_ISP_DEMOSAIC_LPF_F2_DEFAULT;
    pstDemosaicDynaRegCfg->u8Lpff3              = HI_ISP_DEMOSAIC_LPF_F3_DEFAULT;
    pstDemosaicDynaRegCfg->u8CcHFMaxRatio       = HI_ISP_DEMOSAIC_CC_HF_MAX_RATIO_DEFAULT;
    pstDemosaicDynaRegCfg->u8CcHFMinRatio       = HI_ISP_DEMOSAIC_CC_HF_MIN_RATIO_DEFAULT;
    pstDemosaicDynaRegCfg->u32hfIntpRatio       = HI_ISP_DEMOSAIC_INTERP_RATIO1_DEFAULT;
    pstDemosaicDynaRegCfg->u32hfIntpRatio1      = HI_ISP_DEMOSAIC_INTERP_RATIO2_DEFAULT;
    pstDemosaicDynaRegCfg->u16hfIntpBldLow      = HI_ISP_DEMOSAIC_HF_INTP_BLD_LOW_DEFAULT;
    pstDemosaicDynaRegCfg->u16hfIntpBldHig      = HI_ISP_DEMOSAIC_HF_INTP_BLD_HIGH_DEFAULT;
    pstDemosaicDynaRegCfg->u16hfIntpThLow       = HI_ISP_DEMOSAIC_HF_INTP_TH_LOW_DEFAULT;
    pstDemosaicDynaRegCfg->u16hfIntpThHig       = HI_ISP_DEMOSAIC_HF_INTP_TH_HIGH_DEFAULT;
    pstDemosaicDynaRegCfg->u16hfIntpThLow1      = HI_ISP_DEMOSAIC_HF_INTP_TH_LOW1_DEFAULT;
    pstDemosaicDynaRegCfg->u16hfIntpThHig1      = HI_ISP_DEMOSAIC_HF_INTP_TH_HIGH1_DEFAULT;
    pstDemosaicDynaRegCfg->u8BldrCbCr           = HI_ISP_NDDM_BLDR_CBCR_DEFAULT;
    pstDemosaicDynaRegCfg->u8AwbGFGainLow       = HI_ISP_NDDM_AWB_GF_GN_LOW_DEFAULT;
    pstDemosaicDynaRegCfg->u8AwbGFGainHig       = HI_ISP_NDDM_AWB_GF_GN_HIGH_DEFAULT;
    pstDemosaicDynaRegCfg->u8AwbGFGainMax       = HI_ISP_NDDM_AWB_GF_GN_MAX_DEFAULT;
    pstDemosaicDynaRegCfg->u8DitherMax          = HI_ISP_NDDM_DITH_MAX_DEFAULT;
    pstDemosaicDynaRegCfg->u16FcrDetLow         = HI_ISP_NDDM_FCR_DET_LOW_DEFAULT;
    pstDemosaicDynaRegCfg->u8FcrGFGain          = HI_ISP_NDDM_FCR_GF_GAIN_DEFAULT;

    pstDemosaicDynaRegCfg->u8FilterStrIntp      = HI_ISP_NDDM_FILTER_STR_INTP_DEFAULT;
    pstDemosaicDynaRegCfg->u16ClipDeltaIntpLow  = HI_ISP_NDDM_CLIP_DELTA_INTP_LOW_DEFAULT;
    pstDemosaicDynaRegCfg->u16ClipDeltaIntpHigh = HI_ISP_NDDM_CLIP_DELTA_INTP_HIGH_DEFAULT;
    pstDemosaicDynaRegCfg->u8FilterStrFilt      = HI_ISP_NDDM_FILTER_STR_FILT_DEFAULT;
    pstDemosaicDynaRegCfg->u16ClipDeltaFiltLow  = HI_ISP_NDDM_CLIP_DELTA_FILT_LOW_DEFAULT;
    pstDemosaicDynaRegCfg->u16ClipDeltaFiltHigh = HI_ISP_NDDM_CLIP_DELTA_FILT_HIGH_DEFAULT;
    pstDemosaicDynaRegCfg->u8BldrGray           = HI_ISP_NDDM_BLDR_GRAY_DEFAULT;
    pstDemosaicDynaRegCfg->u16SatuRThFix        = HI_ISP_NDDM_SATU_R_TH_FIX_DEFAULT;
    pstDemosaicDynaRegCfg->u16SatuRThLow        = HI_ISP_NDDM_SATU_R_TH_LOW_DEFAULT;
    pstDemosaicDynaRegCfg->u16SatuRThHig        = HI_ISP_NDDM_SATU_R_TH_HIGH_DEFAULT;
    pstDemosaicDynaRegCfg->u16SatuBThFix        = HI_ISP_NDDM_SATU_B_TH_FIX_DEFAULT;
    pstDemosaicDynaRegCfg->u16SatuBThLow        = HI_ISP_NDDM_SATU_B_TH_LOW_DEFAULT;
    pstDemosaicDynaRegCfg->u16SatuBThHig        = HI_ISP_NDDM_SATU_B_TH_HIGH_DEFAULT;
    pstDemosaicDynaRegCfg->s16SatuFixEhcY       = HI_ISP_NDDM_SATU_FIX_EHCY_DEFAULT;

    for (n = 0; n < HI_ISP_DEMOSAIC_LUT_LENGTH; n++)
    {
        pstDemosaicDynaRegCfg->au8EhcGainLut[n] = g_au8EhcGainLut[n];
        pstDemosaicDynaRegCfg->au16GFBlurLut[n] = 0;
    }

    pstDemosaicDynaRegCfg->bUpdateGF            = HI_TRUE;
    pstDemosaicDynaRegCfg->bUpdateUsm           = HI_TRUE;
    pstDemosaicDynaRegCfg->bResh                = HI_TRUE;

    return;
}

static HI_VOID DemosaicRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S  *pstRegCfg)
{
    HI_U32 i;

    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_DEMOSAIC_STATIC_CFG_S *pstDemosaicStaticRegCfg = HI_NULL;
    ISP_DEMOSAIC_DYNA_CFG_S *pstDemosaicDynaRegCfg = HI_NULL;
    ISP_GET_CTX(ViPipe, pstIspCtx);

    for (i = 0; i < pstIspCtx->stBlockAttr.u8BlockNum; i++)
    {
        pstDemosaicStaticRegCfg = &pstRegCfg->stAlgRegCfg[i].stDemRegCfg.stStaticRegCfg;
        pstDemosaicDynaRegCfg   = &pstRegCfg->stAlgRegCfg[i].stDemRegCfg.stDynaRegCfg;
        pstRegCfg->stAlgRegCfg[i].stDemRegCfg.bVhdmEnable = HI_TRUE;
        pstRegCfg->stAlgRegCfg[i].stDemRegCfg.bNddmEnable = HI_TRUE;
        DemosaicStaticRegsInitialize(ViPipe, pstDemosaicStaticRegCfg, i);
        DemosaicDynaRegsInitialize(pstDemosaicDynaRegCfg);
    }

    pstRegCfg->unKey.bit1DemCfg = 1;

    return;
}

static HI_VOID DemosaicExtRegsInitialize(VI_PIPE ViPipe)
{
    HI_U8 i;
    HI_FLOAT n_cur, n_pre, n_fct;
    HI_FLOAT drc_compensate = 0.6f;
    HI_FLOAT afsigma[HI_ISP_DEMOSAIC_LUT_LENGTH];
    ISP_DEMOSAIC_S *pstDemosaic = HI_NULL;

    ISP_CMOS_DEFAULT_S *pstSnsDft = HI_NULL;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);
    DEMOSAIC_GET_CTX(ViPipe, pstDemosaic);

    pstDemosaic->u8WdrMode = pstIspCtx->u8SnsWDRMode;
    pstDemosaic->u8BitDepthPrc = HI_DEMOSAIC_BITDEPTH;

    if (0 != pstDemosaic->u8WdrMode)
    {
        DemosaicInitFwWdr(ViPipe);

        for (i = 0; i < HI_ISP_DEMOSAIC_LUT_LENGTH; i++)
        {
            n_cur = (HI_FLOAT)(i * 16);
            n_pre = (HI_FLOAT)(256.0 * pow(n_cur / 256.0, 1.0f / drc_compensate)) + 0.5f;
            n_fct = (HI_FLOAT)(ISP_SQR(n_cur / n_pre));

            afsigma[i]  =  n_cur * n_fct;
            pstDemosaic->au32sigma[i] = (HI_U32)(afsigma[i] * ISP_BITFIX(10)) ;
        }
    }
    else
    {
        DemosaicInitFwLinear(ViPipe);

        for (i = 0; i < HI_ISP_DEMOSAIC_LUT_LENGTH; i++)
        {
            pstDemosaic->au32sigma[i]  = (HI_U32)(i * 16 * ISP_BITFIX(10));
        }
    }

    hi_ext_system_demosaic_enable_write(ViPipe, pstSnsDft->stDemosaic.bEnable);
    hi_ext_system_demosaic_manual_mode_write(ViPipe, HI_EXT_SYSTEM_DEMOSAIC_MANUAL_MODE_DEFAULT);

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)   //Auto
    {
        pstDemosaic->stAuto.au8DetailSmoothRange[i] = g_au8DetailSmoothRange[i];
        pstDemosaic->stAuto.au16DetailSmoothStr[i]  = g_au16DetailSmoothStr[i];
        hi_ext_system_demosaic_auto_detail_smooth_range_write(ViPipe, i, pstDemosaic->stAuto.au8DetailSmoothRange[i]);
        hi_ext_system_demosaic_auto_detail_smooth_strength_write(ViPipe, i, pstDemosaic->stAuto.au16DetailSmoothStr[i]);
    }

    if (IS_WDR_MODE(pstIspCtx->u8SnsWDRMode))      //Manual:WDR mode
    {
        pstDemosaic->stManual.u8DetailSmoothRange = HI_EXT_SYSTEM_DEMOSAIC_MANUAL_DETAIL_SMOOTH_RANGE_WDR_DEFAULT;
        pstDemosaic->stManual.u16DetailSmoothStr  = HI_EXT_SYSTEM_DEMOSAIC_MANUAL_DETAIL_SMOOTH_STRENGTH_WDR_DEFAULT;
    }
    else       //Manual:Linear Mode
    {
        pstDemosaic->stManual.u8DetailSmoothRange = HI_EXT_SYSTEM_DEMOSAIC_MANUAL_DETAIL_SMOOTH_RANGE_LINEAR_DEFAULT;
        pstDemosaic->stManual.u16DetailSmoothStr  = HI_EXT_SYSTEM_DEMOSAIC_MANUAL_DETAIL_SMOOTH_STRENGTH_LINEAR_DEFAULT;
    }
    hi_ext_system_demosaic_manual_detail_smooth_range_write(ViPipe, pstDemosaic->stManual.u8DetailSmoothRange);
    hi_ext_system_demosaic_manual_detail_smooth_strength_write(ViPipe, pstDemosaic->stManual.u16DetailSmoothStr);

    if (pstSnsDft->stDemosaic.bValid)
    {
        hi_ext_system_demosaic_enable_write(ViPipe, pstSnsDft->stDemosaic.bEnable);

        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)   //Auto
        {
            pstDemosaic->stAuto.au8DetailSmoothRange[i]   = pstSnsDft->stDemosaic.au8DetailSmoothRange[i];
            pstDemosaic->stAuto.au16DetailSmoothStr[i]    = pstSnsDft->stDemosaic.au16DetailSmoothStr[i];
            hi_ext_system_demosaic_auto_detail_smooth_range_write(ViPipe, i, pstDemosaic->stAuto.au8DetailSmoothRange[i]);
            hi_ext_system_demosaic_auto_detail_smooth_strength_write(ViPipe, i, pstDemosaic->stAuto.au16DetailSmoothStr[i]);
        }
    }

    return;
}

static HI_S32 DemosaicReadExtregs(VI_PIPE ViPipe)
{
    ISP_DEMOSAIC_S *pstDemosaic = HI_NULL;
    HI_U8 i;
    DEMOSAIC_GET_CTX(ViPipe, pstDemosaic);

    pstDemosaic->bDemAttrUpdate = hi_ext_system_demosaic_attr_update_en_read(ViPipe);

    hi_ext_system_demosaic_attr_update_en_write(ViPipe, HI_FALSE);

    if (pstDemosaic->bDemAttrUpdate)
    {
        pstDemosaic->enOpType        = hi_ext_system_demosaic_manual_mode_read(ViPipe);

        if (OP_TYPE_AUTO == pstDemosaic->enOpType)
        {
            for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
            {
                pstDemosaic->stAuto.au8DetailSmoothRange[i] = hi_ext_system_demosaic_auto_detail_smooth_range_read(ViPipe, i);
                pstDemosaic->stAuto.au16DetailSmoothStr[i]  = hi_ext_system_demosaic_auto_detail_smooth_strength_read(ViPipe, i);
            }
        }
        else if (OP_TYPE_MANUAL == pstDemosaic->enOpType)
        {
            pstDemosaic->stManual.u8DetailSmoothRange = hi_ext_system_demosaic_manual_detail_smooth_range_read(ViPipe);
            pstDemosaic->stManual.u16DetailSmoothStr  = hi_ext_system_demosaic_manual_detail_smooth_strength_read(ViPipe);
        }
    }
    return 0;
}

HI_S32 DemosaicProcWrite(VI_PIPE ViPipe, ISP_CTRL_PROC_WRITE_S *pstProc)
{
    ISP_CTRL_PROC_WRITE_S stProcTmp;

    ISP_DEMOSAIC_S *pstDemosaic = HI_NULL;
    DEMOSAIC_GET_CTX(ViPipe, pstDemosaic);

    if ((HI_NULL == pstProc->pcProcBuff)
        || (0 == pstProc->u32BuffLen))
    {
        return HI_FAILURE;
    }

    stProcTmp.pcProcBuff = pstProc->pcProcBuff;
    stProcTmp.u32BuffLen = pstProc->u32BuffLen;

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "-----DEMOSAIC INFO-------------------------------------------------------------\n");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s\n",
                    "Enable");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16u\n",
                    pstDemosaic->bEnable
                   );

    pstProc->u32WriteLen += 1;

    return HI_SUCCESS;
}


HI_S32 ISP_DemosaicInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_DEMOSAIC_S *pstDemosaic = HI_NULL;
    ISP_REG_CFG_S  *pstRegCfg   = (ISP_REG_CFG_S *)pRegCfg;

    DEMOSAIC_GET_CTX(ViPipe, pstDemosaic);

    DemosaicRegsInitialize(ViPipe, pstRegCfg);
    DemosaicExtRegsInitialize(ViPipe);

    pstDemosaic->bInit = HI_TRUE;
    return HI_SUCCESS;
}

static HI_U32 DemosaicGetIsoIndex(HI_U32 u32Iso)
{
    HI_U32 u32Index;

    for (u32Index = 1; u32Index < HI_ISP_DEMOSAIC_LUT_LEN - 1; u32Index++)
    {
        if (u32Iso <= au32DemosaicIsoLut[u32Index])
        {
            break;
        }
    }

    return u32Index;
}

HI_U32 DemosaicGetValueFromLut (HI_U32 u32IsoLevel , HI_S32 s32Y2, HI_S32 s32Y1, HI_S32 s32X2, HI_S32 s32X1, HI_S32 s32Iso)
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

#define  EPS (0.000001f)
#define  COL_ISO      0
#define  COL_K        1

static HI_FLOAT getAlphafromNoiseLut(HI_FLOAT (*pRecord)[3], HI_U16 recordNum, HI_S32 iso)
{
    HI_S32 i = 0;
    HI_FLOAT  y_diff = 0, x_diff = 0;
    HI_FLOAT k = 0.0f;

    // record: iso - k
    if (iso <= pRecord[0][COL_ISO])
    {
        k = pRecord[0][COL_K];
        //return k;
    }

    if (iso >= pRecord[recordNum - 1][COL_ISO])
    {
        k = pRecord[recordNum - 1][COL_K];
        //return k;
    }

    for (i = 0; i < recordNum - 1; i++)
    {
        if (iso >= pRecord[i][COL_ISO] && iso <= pRecord[i + 1][COL_ISO])
        {
            x_diff = pRecord[i + 1][COL_ISO] - pRecord[i][COL_ISO];  // iso diff
            y_diff = pRecord[i + 1][COL_K]  - pRecord[i][COL_K];     // k diff
            k = pRecord[i][COL_K] + y_diff * (iso - pRecord[i][COL_ISO]) / DIV_0_TO_1(x_diff + EPS);
            //return k;
        }
    }

    return k;
}

HI_S32 Demosaic_GFBlurLut(ISP_DEMOSAIC_S *pstDemosaic, VI_PIPE ViPipe, ISP_DEMOSAIC_DYNA_CFG_S *pstDmCfg, HI_U16 u16NddmStrength, HI_U32 u32Iso)
{
    HI_U8  n = 0;
    HI_U32 alpha, sigma;
    HI_U64 u64sigma;
    ISP_CMOS_DEFAULT_S *pstSnsDft = HI_NULL;
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);
    HI_S32   bitScale   =  ISP_BITFIX(pstDemosaic->u8BitDepthPrc - 8);
    HI_FLOAT fCalibrationCoef = 0.0f;

    fCalibrationCoef = getAlphafromNoiseLut(pstSnsDft->stNoiseCalibration.afCalibrationCoef, pstSnsDft->stNoiseCalibration.u16CalibrationLutNum, u32Iso);
    alpha = (HI_U32)(fCalibrationCoef * ISP_BITFIX(10));

    for (n = 0; n < HI_ISP_DEMOSAIC_LUT_LENGTH; n++)
    {
        u64sigma  = (HI_U64)pstDemosaic->au32sigma[n] * alpha;
        u64sigma *= u16NddmStrength;
        sigma  = (HI_U32)(u64sigma >> 14);
        sigma  = (HI_U32)Sqrt32(sigma);
        sigma = (sigma * bitScale) >> 5;
        pstDmCfg->au16GFBlurLut[n] = HI_DEMOSAIC_MIN(sigma, ISP_BITMASK(pstDemosaic->u8BitDepthPrc));
    }

    pstDmCfg->au16GFBlurLut[0] = pstDmCfg->au16GFBlurLut[3];
    pstDmCfg->au16GFBlurLut[1] = pstDmCfg->au16GFBlurLut[3];
    pstDmCfg->au16GFBlurLut[2] = pstDmCfg->au16GFBlurLut[3];
    pstDmCfg->bUpdateGF       = HI_TRUE;

    return HI_SUCCESS;
}

HI_S32 Demosaic_HFIntp(ISP_DEMOSAIC_DYNA_CFG_S *pstDmCfg)
{
    HI_U16 u16hfIntpBldLow = pstDmCfg->u16hfIntpBldLow;
    HI_U16 u16hfIntpBldHig = pstDmCfg->u16hfIntpBldHig;

    if (pstDmCfg->u16hfIntpThHig == pstDmCfg->u16hfIntpThLow)
    {
        pstDmCfg->u16hfIntpThHig = pstDmCfg->u16hfIntpThLow + 1;
    }

    pstDmCfg->u32hfIntpRatio = (u16hfIntpBldHig - u16hfIntpBldLow) * 128 / HI_DEMOSAIC_MAX((pstDmCfg->u16hfIntpThHig - pstDmCfg->u16hfIntpThLow), 1);
    pstDmCfg->u16hfIntpThLow1 = HI_DEMOSAIC_MIN(pstDmCfg->u16hfIntpThHig + pstDmCfg->u16hfIntpThLow1, 1022);;

    if (pstDmCfg->u16hfIntpThHig1 == pstDmCfg->u16hfIntpThLow1)
    {
        pstDmCfg->u16hfIntpThHig1 = pstDmCfg->u16hfIntpThLow1 + 1;
    }

    pstDmCfg->u32hfIntpRatio1 = (u16hfIntpBldHig - u16hfIntpBldLow) * 128 / HI_DEMOSAIC_MAX((pstDmCfg->u16hfIntpThHig1 - pstDmCfg->u16hfIntpThLow1), 1);

    return HI_SUCCESS;
}

HI_S32 DemosaicCfg(ISP_DEMOSAIC_DYNA_CFG_S *pstDmCfg, ISP_DEMOSAIC_S *pstDemosaic, HI_U32 u32IsoLevel, HI_U32 u32ISO2, HI_U32 u32ISO1, HI_U32 u32Iso)
{
    HI_S32 s32Y1, s32Y2;

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au8LutBldrCbCr[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au8LutBldrCbCr[0];
    s32Y2 =               (HI_S32)pstDemosaic->au8LutBldrCbCr[u32IsoLevel];
    pstDmCfg->u8BldrCbCr      = (HI_U8)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au8LutDitherMax[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au8LutDitherMax[0];
    s32Y2 =               (HI_S32)pstDemosaic->au8LutDitherMax[u32IsoLevel];
    pstDmCfg->u8DitherMax      = (HI_U8)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au8LutAwbGFGainLow[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au8LutAwbGFGainLow[0];
    s32Y2 =               (HI_S32)pstDemosaic->au8LutAwbGFGainLow[u32IsoLevel];
    pstDmCfg->u8AwbGFGainLow = (HI_U8)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au8LutAwbGFGainHig[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au8LutAwbGFGainHig[0];
    s32Y2 =               (HI_S32)pstDemosaic->au8LutAwbGFGainHig[u32IsoLevel];
    pstDmCfg->u8AwbGFGainHig = (HI_U8)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au8LutAwbGFGainMax[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au8LutAwbGFGainMax[0];
    s32Y2 =               (HI_S32)pstDemosaic->au8LutAwbGFGainMax[u32IsoLevel];
    pstDmCfg->u8AwbGFGainMax = (HI_U8)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au16LutFcrDetLow[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au16LutFcrDetLow[0];
    s32Y2 =               (HI_S32)pstDemosaic->au16LutFcrDetLow[u32IsoLevel];
    pstDmCfg->u16FcrDetLow     = (HI_U16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->as8LutFcrGFGain[u32IsoLevel - 1] : (HI_S32)pstDemosaic->as8LutFcrGFGain[0];
    s32Y2 =               (HI_S32)pstDemosaic->as8LutFcrGFGain[u32IsoLevel];
    pstDmCfg->u8FcrGFGain = (HI_U8)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au16LutNddmStrength[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au16LutNddmStrength[0];
    s32Y2 =                 (HI_S32)pstDemosaic->au16LutNddmStrength[u32IsoLevel];
    pstDemosaic->u16NddmStrength = (HI_U16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au16LutSatuRThFix[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au16LutSatuRThFix[0];
    s32Y2 =               (HI_S32)pstDemosaic->au16LutSatuRThFix[u32IsoLevel];
    pstDmCfg->u16SatuRThFix    = (HI_U16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au16LutSatuRThLow[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au16LutSatuRThLow[0];
    s32Y2 =               (HI_S32)pstDemosaic->au16LutSatuRThLow[u32IsoLevel];
    pstDmCfg->u16SatuRThLow    = (HI_U16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au16LutSatuRThHig[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au16LutSatuRThHig[0];
    s32Y2 =               (HI_S32)pstDemosaic->au16LutSatuRThHig[u32IsoLevel];
    pstDmCfg->u16SatuRThHig    = (HI_U16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au16LutSatuBThFix[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au16LutSatuBThFix[0];
    s32Y2 =               (HI_S32)pstDemosaic->au16LutSatuBThFix[u32IsoLevel];
    pstDmCfg->u16SatuBThFix    = (HI_U16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au16LutSatuBThLow[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au16LutSatuBThLow[0];
    s32Y2 =               (HI_S32)pstDemosaic->au16LutSatuBThLow[u32IsoLevel];
    pstDmCfg->u16SatuBThLow    = (HI_U16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au16LutSatuBThHig[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au16LutSatuBThHig[0];
    s32Y2 =               (HI_S32)pstDemosaic->au16LutSatuBThHig[u32IsoLevel];
    pstDmCfg->u16SatuBThHig    = (HI_U16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au8LutFilterStrIntp[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au8LutFilterStrIntp[0];
    s32Y2 =               (HI_S32)pstDemosaic->au8LutFilterStrIntp[u32IsoLevel];
    pstDmCfg->u8FilterStrIntp  = (HI_U8)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);
    pstDmCfg->u8FilterStrFilt  = pstDmCfg->u8FilterStrIntp;

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au16LutSharpenLumaStr[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au16LutSharpenLumaStr[0];
    s32Y2 =               (HI_S32)pstDemosaic->au16LutSharpenLumaStr[u32IsoLevel];
    pstDmCfg->s16SatuFixEhcY  = (HI_S16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);
    pstDmCfg->s16SatuFixEhcY  -= 256;

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au16LutClipDeltaFiltLow[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au16LutClipDeltaFiltLow[0];
    s32Y2 =               (HI_S32)pstDemosaic->au16LutClipDeltaFiltLow[u32IsoLevel];
    pstDmCfg->u16ClipDeltaFiltLow   = (HI_U16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);
    pstDmCfg->u16ClipDeltaIntpLow   = pstDmCfg->u16ClipDeltaFiltLow;

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au16LutClipDeltaFiltHig[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au16LutClipDeltaFiltHig[0];
    s32Y2 =               (HI_S32)pstDemosaic->au16LutClipDeltaFiltHig[u32IsoLevel];
    pstDmCfg->u16ClipDeltaFiltHigh  = (HI_U16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);
    pstDmCfg->u16ClipDeltaIntpHigh  = pstDmCfg->u16ClipDeltaFiltHigh;

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au8LutBldrGray[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au8LutBldrGray[0];
    s32Y2 =               (HI_S32)pstDemosaic->au8LutBldrGray[u32IsoLevel];
    pstDmCfg->u8BldrGray   = (HI_U8)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au16LutEdgeSmoothLowThr[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au16LutEdgeSmoothLowThr[0];
    s32Y2 =               (HI_S32)pstDemosaic->au16LutEdgeSmoothLowThr[u32IsoLevel];
    pstDmCfg->u16hfIntpThLow  = (HI_U16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);
    pstDmCfg->u16hfIntpThLow = HI_DEMOSAIC_MIN(pstDmCfg->u16hfIntpThLow, 1022);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au16LutEdgeSmoothHigThr[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au16LutEdgeSmoothHigThr[0];
    s32Y2 =               (HI_S32)pstDemosaic->au16LutEdgeSmoothHigThr[u32IsoLevel];
    pstDmCfg->u16hfIntpThHig   = (HI_U16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);
    pstDmCfg->u16hfIntpThHig = HI_DEMOSAIC_MIN(pstDmCfg->u16hfIntpThHig, 1023);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au16LutAntiAliasLowThr[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au16LutAntiAliasLowThr[0];
    s32Y2 =                 (HI_S32)pstDemosaic->au16LutAntiAliasLowThr[u32IsoLevel];
    pstDmCfg->u16hfIntpThLow1 = (HI_U16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);
    pstDmCfg->u16hfIntpThLow1 = HI_DEMOSAIC_MIN(pstDmCfg->u16hfIntpThLow1, 0x3ce);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au16LutAntiAliasHigThr[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au16LutAntiAliasHigThr[0];
    s32Y2 =                 (HI_S32)pstDemosaic->au16LutAntiAliasHigThr[u32IsoLevel];
    pstDmCfg->u16hfIntpThHig1 = (HI_U16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);
    pstDmCfg->u16hfIntpThHig1 = HI_DEMOSAIC_MIN(pstDmCfg->u16hfIntpThHig1, 0x3cf);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au8LutCcHFMaxRatio[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au8LutCcHFMaxRatio[0];
    s32Y2 =                 (HI_S32)pstDemosaic->au8LutCcHFMaxRatio[u32IsoLevel];
    pstDmCfg->u8CcHFMaxRatio   = (HI_U8)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->au8LutCcHFMinRatio[u32IsoLevel - 1] : (HI_S32)pstDemosaic->au8LutCcHFMinRatio[0];
    s32Y2 =               (HI_S32)pstDemosaic->au8LutCcHFMinRatio[u32IsoLevel];
    pstDmCfg->u8CcHFMinRatio   = (HI_U16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

    return HI_SUCCESS;
}

HI_S32 ISP_DemosaicFw(HI_U32 u32Iso, VI_PIPE ViPipe, HI_U8 u8CurBlk, ISP_REG_CFG_S *pstReg)
{
    ISP_DEMOSAIC_DYNA_CFG_S *pstDmCfg = &pstReg->stAlgRegCfg[u8CurBlk].stDemRegCfg.stDynaRegCfg;
    ISP_DEMOSAIC_S *pstDemosaic = HI_NULL;
    DEMOSAIC_GET_CTX(ViPipe, pstDemosaic);

    HI_U8  u8FilterCoefIndex = 0;
    HI_U32 u32IsoLevel;
    HI_U32 u32ISO1 = 0;
    HI_U32 u32ISO2 = 0;
    HI_S32 s32Y1, s32Y2;

    if (u32Iso > au32DemosaicIsoLut[HI_ISP_DEMOSAIC_LUT_LEN - 1])
    {
        u32IsoLevel = HI_ISP_DEMOSAIC_LUT_LEN - 1;
        u32ISO1 = au32DemosaicIsoLut[HI_ISP_DEMOSAIC_LUT_LEN - 1];
        u32ISO2 = au32DemosaicIsoLut[HI_ISP_DEMOSAIC_LUT_LEN - 1];
    }
    else if (u32Iso <= au32DemosaicIsoLut[0])
    {
        u32IsoLevel = 0;
        u32ISO1 = 0;
        u32ISO2 = au32DemosaicIsoLut[0];
    }
    else
    {
        u32IsoLevel = DemosaicGetIsoIndex(u32Iso);
        u32ISO1 = au32DemosaicIsoLut[u32IsoLevel - 1];
        u32ISO2 = au32DemosaicIsoLut[u32IsoLevel];
    }

    if (OP_TYPE_AUTO == pstDemosaic->enOpType)
    {
        s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->stAuto.au8DetailSmoothRange[u32IsoLevel - 1] : (HI_S32)pstDemosaic->stAuto.au8DetailSmoothRange[0];
        s32Y2 =                 (HI_S32)pstDemosaic->stAuto.au8DetailSmoothRange[u32IsoLevel];
        u8FilterCoefIndex = (HI_U8)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstDemosaic->stAuto.au16DetailSmoothStr[u32IsoLevel - 1] : (HI_S32)pstDemosaic->stAuto.au16DetailSmoothStr[0];
        s32Y2 =                 (HI_S32)pstDemosaic->stAuto.au16DetailSmoothStr[u32IsoLevel];
        pstDmCfg->u16hfIntpBldLow = (HI_U16)DemosaicGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);
        pstDmCfg->u16hfIntpBldLow = HI_DEMOSAIC_MAX((256 - pstDmCfg->u16hfIntpBldLow), 0);
        pstDmCfg->u16hfIntpBldHig = pstDmCfg->u16hfIntpBldLow;
    }
    else if (OP_TYPE_MANUAL == pstDemosaic->enOpType)
    {
        u8FilterCoefIndex         = pstDemosaic->stManual.u8DetailSmoothRange;
        pstDmCfg->u16hfIntpBldLow = HI_DEMOSAIC_MAX((256 - pstDemosaic->stManual.u16DetailSmoothStr), 0);
        pstDmCfg->u16hfIntpBldHig = pstDmCfg->u16hfIntpBldLow;
    }

    DemosaicCfg(pstDmCfg, pstDemosaic, u32IsoLevel, u32ISO2, u32ISO1, u32Iso);
    Demosaic_HFIntp(pstDmCfg);
    Demosaic_GFBlurLut(pstDemosaic, ViPipe, pstDmCfg, pstDemosaic->u16NddmStrength, u32Iso);

    if (u8FilterCoefIndex < 1)
    {
        u8FilterCoefIndex = 1;
    }

    pstDmCfg->u8Lpff0 = g_as32DemosaicFilterCoef[u8FilterCoefIndex - 1][0];
    pstDmCfg->u8Lpff1 = g_as32DemosaicFilterCoef[u8FilterCoefIndex - 1][1];
    pstDmCfg->u8Lpff2 = g_as32DemosaicFilterCoef[u8FilterCoefIndex - 1][2];

    pstDmCfg->bResh = HI_TRUE;

    return  HI_SUCCESS;
}

static HI_BOOL __inline CheckDemosaicOpen(ISP_DEMOSAIC_S *pstDemosaic)
{
    return (HI_TRUE == pstDemosaic->bEnable);
}

HI_S32 ISP_DemosaicRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo, HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_U8  i;
    ISP_REG_CFG_S *pstReg = (ISP_REG_CFG_S *)pRegCfg;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_DEMOSAIC_S *pstDemosaic = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    DEMOSAIC_GET_CTX(ViPipe, pstDemosaic);

    /* calculate every two interrupts */
    if (!pstDemosaic->bInit)
    {
        return HI_SUCCESS;
    }

    if ((0 != pstIspCtx->u32FrameCnt % 2) && (HI_TRUE != pstIspCtx->stLinkage.bSnapState))
    {
        return HI_SUCCESS;
    }

    pstDemosaic->bEnable = hi_ext_system_demosaic_enable_read(ViPipe);

    for (i = 0; i < pstReg->u8CfgNum; i++)
    {
        pstReg->stAlgRegCfg[i].stDemRegCfg.bVhdmEnable = pstDemosaic->bEnable;
        pstReg->stAlgRegCfg[i].stDemRegCfg.bNddmEnable = pstDemosaic->bEnable;
    }

    pstReg->unKey.bit1DemCfg = 1;

    /*check hardware setting*/
    if (!CheckDemosaicOpen(pstDemosaic))
    {
        return HI_SUCCESS;
    }

    DemosaicReadExtregs(ViPipe);

    for (i = 0; i < pstIspCtx->stBlockAttr.u8BlockNum; i++)
    {
        ISP_DemosaicFw(pstIspCtx->stLinkage.u32Iso, ViPipe, i, pstReg);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DemosaicCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    ISP_DEMOSAIC_S *pstDemosaic = HI_NULL;
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);
    switch (u32Cmd)
    {
        case ISP_WDR_MODE_SET :
            DEMOSAIC_GET_CTX(ViPipe, pstDemosaic);
            pstDemosaic->bInit = HI_FALSE;
            ISP_DemosaicInit(ViPipe, (HI_VOID *)&pRegCfg->stRegCfg);
            break;

        case ISP_PROC_WRITE:
            DemosaicProcWrite(ViPipe, (ISP_CTRL_PROC_WRITE_S *)pValue);
            break;

        default :
            break;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DemosaicExit(VI_PIPE ViPipe)
{
    HI_U16 i;
    ISP_CTX_S *pstIspCtx  = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    for (i = 0; i < pstIspCtx->stBlockAttr.u8BlockNum; i++)
    {
        isp_dmnr_vhdm_en_write(ViPipe, i, HI_FALSE);
        isp_demosaic_ahd_en_write(ViPipe, i, HI_FALSE);
        isp_dmnr_nddm_en_write(ViPipe, i, HI_FALSE);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterDemosaic(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_DEMOSAIC;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_DemosaicInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_DemosaicRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_DemosaicCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_DemosaicExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */



