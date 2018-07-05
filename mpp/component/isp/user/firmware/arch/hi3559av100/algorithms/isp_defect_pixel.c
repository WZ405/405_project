/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_defect_pixel.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/01/16
  Description   :
  History       :
  1.Date        : 2013/01/16
    Author      :
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include "isp_sensor.h"
#include "isp_alg.h"
#include "isp_config.h"
#include "isp_ext_config.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */
#define ISP_DPCC_MODE (35)
#define ISP_DPCC_HOT_MODE  (7)
#define ISP_DPCC_DEAD_MODE (71)
#define ISP_DPCC_HIGHLIGHT_MODE (160)
#define ISP_CALIBRATION_PARAMS_SET (1)
#define ISP_HOT_DEV_THRESH (20)
#define ISP_DEAD_DEV_THRESH (15)

#define ISP_DPCC_PAR_MAX_COUNT (9)//(6)
#define ISP_DPC_SLOPE_GRADE (5)
#define ISP_DPC_SOFT_SLOPE_GRADE (5)
#define ISP_DPC_MAX_BPT_NUM_SBS 8192
#define ISP_DPC_MAX_BPT_NUM_NORMAL 4096

static const HI_S32 g_as32IsoLutLow[16]  = {100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600, 51200, 102400, 204800, 409600, 819200, 1638400, 3276800};
static const HI_U16 g_au16DpcStrength[16]   = {0, 0, 0, 152, 200, 200, 220, 220, 220, 220, 152, 152, 152, 152, 152, 152};
static const HI_U16 g_au16DpcBlendRatio[16] = {0, 0, 0,  0,  0,  0,  0,  0,  0,  0, 50, 50, 50, 50, 50, 50};
static const HI_S32 g_as32DpcLineThr1[27]       =   {0x0808, 0x0808, 0x0808, 0x0808, 0x0808, 0x0808, 0x0808, 0x0808, 0x0808, 0x0808, 0x0808, 0x0808, 0x0707, 0x0606, 0x0505, 0x0404, 0x0404, 0x0404, 0x0404, 0x0303, 0x0202, 0x0202, 0x0202, 0x0101, 0x0101, 0x0101, 0x0101};
static const HI_S32 g_as32DpcLineMadFac1[27]    =   {0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303};
static const HI_S32 g_as32DpcPgFac1[27]         =   {0x0404, 0x0404, 0x0404, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303};
static const HI_S32 g_as32DpcRndThr1[27]        =   {0x0a0a, 0x0a0a, 0x0a0a, 0x0a0a, 0x0808, 0x0808, 0x0808, 0x0808, 0x0808, 0x0808, 0x0808, 0x0808, 0x0707, 0x0606, 0x0505, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404};
static const HI_S32 g_as32DpcRgFac1[27]         =   {0x1f1f, 0x1e1e, 0x1d1d, 0x1d1d, 0x1d1d, 0x1b1b, 0x1919, 0x1717, 0x1515, 0x1313, 0x1111, 0x1010, 0x1010, 0x1010, 0x1010, 0x1010, 0x0d0d, 0x0c0c, 0x0a0a, 0x0a0a, 0x0a0a, 0x0808, 0x0808, 0x0808, 0x0606, 0x0404, 0x0202};
static const HI_S32 g_as32DpcRoLimits1[27]      =   {0x0dfa, 0x0dfa, 0x0efe, 0x0efe, 0x0efe, 0x0efe, 0x0efe, 0x0efe, 0x0efe, 0x0efe, 0x0efe, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff};
static const HI_S32 g_as32DpcRndOffs1[27]       =   {0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff};
static const HI_S32 g_as32SlopeGrade[5]         =   {0, 76, 99, 100, 127};

static const HI_S32 g_s32SoftLineThr[5]     = {0x5454, 0x1818, 0x1212, 0x0a0a, 0x0a0a};
static const HI_S32 g_s32SoftLineMadFac[5]  = {0x1810, 0x1810, 0x1810, 0x1010, 0x0a0a};
static const HI_S32 g_s32SoftSlopeGrade[5]  = {0, 76, 100, 115, 120};


typedef struct hiISP_DEFECT_PIXEL_S
{
    /* Public */
    HI_BOOL bEnable;                // enable dpc module
    HI_BOOL bUsrEn;
    HI_BOOL bStatEn;
    HI_U32  u32DpccMode;
    HI_U32  u32DpccBpData;
    HI_U32  u32DpccBadThresh;
    //static calib
    HI_BOOL bStaCalibrationEn;      // enable static calibration
    HI_U8   u8PixelDetectType;       //0: hot pixel detect; 1: dead pixel detect;
    HI_U8   u8FrameCnt;
    HI_U8   u8StaticDPThresh;
    HI_U8   u8TrialCount;
    HI_U8   u8TrialCntLimit;
    HI_U8   u8CalibStarted;
    HI_U8   u8StaCalibrationParamsSet;
    HI_U8   u8HotDevThresh;
    HI_U8   u8DeadDevThresh;
    HI_U16  u16DpCountMax;
    HI_U16  u16DpCountMin;
    HI_U16  u16BptCalibNum;
    HI_U16  au16BpCalibNum[ISP_STRIPING_MAX_NUM];
    HI_U32  au32BptCalibTable[ISP_STRIPING_MAX_NUM][ISP_DPC_MAX_BPT_NUM_NORMAL];

    //static cor
    HI_BOOL bStaticEnable;
    HI_BOOL bStaicShow;
    HI_BOOL bStaticAttrUpdate;
    HI_U16  u16BptCorNum;
    HI_U32  au32BptCorTable[ISP_DPC_MAX_BPT_NUM_SBS];
    HI_U16  au16Offset[ISP_STRIPING_MAX_NUM];
    HI_U16  au16OffsetForSplit[ISP_STRIPING_MAX_NUM + 1];
    HI_U16  u16Overlap;
    //dynamic cor
    HI_BOOL bDynamicEnable;
    HI_BOOL bDynamicManual;
    HI_BOOL bSupTwinkleEn;
    HI_BOOL bDynaAttrUpdateEn;
    HI_U16  u16BlendRatio;
    HI_U16  u16Strength;
    HI_S8   s8SupTwinkleThr;
    HI_U8   u8SupTwinkleSlope;

    ISP_CMOS_DPC_S stCmosDpc;
} ISP_DEFECT_PIXEL_S;

typedef struct
{
    HI_U32 u32DpccSetUse;
    HI_U32 u32DpccMethodsSet1;
    HI_U32 u32DpccMethodsSet2;
    HI_U32 u32DpccMethodsSet3;
    HI_U32 u32DpccBadThresh;
} ISP_DPC_CFG_S;

static const  ISP_DPC_CFG_S g_stDpcDefCfg[ISP_DPCC_PAR_MAX_COUNT] =
{
    {0x0001 , 0x1F1F, 0x0707, 0x1F1F, 0xff800080}, //0~75
    {0x0003 , 0x1F1F, 0x0707, 0x1F1F, 0xff800080}, //ori set 1  (76)
    {0x0003 , 0x1F1F, 0x0707, 0x1F1F, 0xff800080}, //ori set 2 (99)
    {0x0007 , 0x1D1F, 0x0707, 0x1F1F, 0xff800080}, // set 23(RB set3, G set2) (100)
    {0x0001 , 0x1F1F, 0x0707, 0x1F1F, 0xff800080}, //101 ~127
};

typedef struct
{
    HI_U16 au16DpccLineThr[2][3];
    HI_U16 au16DpccLineMadFac[2][3];
    HI_U16 au16DpccPgFac[2][3];
    HI_U16 au16DpccRndThr[2][3];
    HI_U16 au16DpccRgFac[2][3];
    HI_U16 au16DpccRo[2][3];
    HI_U16 au16DpccRndOffs[2][3];
} ISP_DPCC_DERIVED_PARAM_S;

static const  ISP_DPCC_DERIVED_PARAM_S g_stDpcDerParam[5] =
{
    {{{0x54, 0x21, 0x20}, {0x54, 0x21, 0x20}}, {{0x1B, 0x18, 0x04}, {0x1B, 0x10, 0x04}}, {{0x08, 0x0B, 0x0A}, {0x08, 0x0B, 0x0A}}, {{0x0A, 0x08, 0x08}, {0x0A, 0x08, 0x06}}, {{0x26, 0x08, 0x04}, {0x26, 0x08, 0x04}}, {{0x1, 0x2, 0x2}, {0x1, 0x2, 0x1}}, {{0x2, 0x2, 0x2}, {0x2, 0x2, 0x2}}}, //0
    {{{0x08, 0x21, 0x20}, {0x08, 0x21, 0x20}}, {{0x1B, 0x18, 0x04}, {0x1B, 0x10, 0x04}}, {{0x08, 0x0B, 0x0A}, {0x08, 0x0B, 0x0A}}, {{0x0A, 0x08, 0x08}, {0x0A, 0x08, 0x06}}, {{0x26, 0x08, 0x04}, {0x26, 0x08, 0x04}}, {{0x1, 0x2, 0x2}, {0x1, 0x2, 0x1}}, {{0x1, 0x2, 0x2}, {0x2, 0x2, 0x2}}}, //76
    {{{0x08, 0x10, 0x20}, {0x08, 0x10, 0x20}}, {{0x04, 0x18, 0x04}, {0x04, 0x10, 0x04}}, {{0x08, 0x08, 0x0A}, {0x08, 0x06, 0x0A}}, {{0x0A, 0x08, 0x08}, {0x0A, 0x08, 0x06}}, {{0x20, 0x08, 0x04}, {0x20, 0x08, 0x04}}, {{0x2, 0x3, 0x2}, {0x2, 0x3, 0x1}}, {{0x3, 0x3, 0x3}, {0x3, 0x3, 0x3}}}, //99
    {{{0x08, 0x10, 0x20}, {0x08, 0x10, 0x20}}, {{0x04, 0x18, 0x04}, {0x04, 0x10, 0x04}}, {{0x08, 0x08, 0x0A}, {0x08, 0x06, 0x0A}}, {{0x0A, 0x08, 0x08}, {0x0A, 0x08, 0x06}}, {{0x20, 0x08, 0x04}, {0x20, 0x08, 0x04}}, {{0x2, 0x3, 0x2}, {0x2, 0x3, 0x1}}, {{0x3, 0x3, 0x3}, {0x3, 0x3, 0x3}}}, //100
    {{{0x01, 0x10, 0x20}, {0x01, 0x10, 0x20}}, {{0x03, 0x18, 0x04}, {0x03, 0x10, 0x04}}, {{0x03, 0x08, 0x0A}, {0x03, 0x06, 0x0A}}, {{0x04, 0x08, 0x08}, {0x04, 0x08, 0x06}}, {{0x08, 0x08, 0x04}, {0x08, 0x08, 0x04}}, {{0x2, 0x3, 0x2}, {0x2, 0x3, 0x1}}, {{0x3, 0x3, 0x3}, {0x3, 0x3, 0x3}}}, //127
};

ISP_DEFECT_PIXEL_S g_astDpCtx[ISP_MAX_PIPE_NUM] = {{0}};
#define DP_GET_CTX(dev, pstCtx)   pstCtx = &g_astDpCtx[dev]

HI_U8 DpGetChnNum(HI_U8 u8WDRMode)
{
    if (IS_LINEAR_MODE(u8WDRMode))
    {
        return 1;
    }
    else if (IS_BUILT_IN_WDR_MODE(u8WDRMode))
    {
        return 1;
    }
    else if (IS_2to1_WDR_MODE(u8WDRMode))
    {
        return 2;
    }
    else if (IS_3to1_WDR_MODE(u8WDRMode))
    {
        return 3;
    }
    else if (IS_4to1_WDR_MODE(u8WDRMode))
    {
        return 4;
    }
    else
    {
        /* unknow u8Mode */
        return 1;
    }
}

static HI_VOID ISP_DpEnableCfg(VI_PIPE ViPipe, HI_U8 u8CfgNum, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8     i, j, u8ChnNum;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    u8ChnNum = DpGetChnNum(pstIspCtx->u8SnsWDRMode);

    for (i = 0; i < u8CfgNum; i++)
    {
        for (j = 0; j < 4; j++)
        {
            pstRegCfg->stAlgRegCfg[i].stDpRegCfg.abDpcEn[j] = (j < u8ChnNum) ? (HI_TRUE) : (HI_FALSE);
        }
    }
}

static HI_VOID DpStaticRegsInitialize(ISP_DPC_STATIC_CFG_S *pstStaticRegCfg)
{
    pstStaticRegCfg->u32DpccBptCtrl    = HI_ISP_DPC_DEFAULT_BPT_CTRL;
    pstStaticRegCfg->u32DpccOutputMode = HI_ISP_DPC_DEFAULT_OUTPUT_MODE;
    pstStaticRegCfg->bStaticResh       = HI_TRUE;

    return;
}

static HI_VOID DpUsrRegsInitialize(ISP_DPC_USR_CFG_S *pstUsrRegCfg)
{
    pstUsrRegCfg->stUsrDynaCorRegCfg.bDpccHardThrEn         = HI_ISP_DPC_DEFAULT_HARD_THR_ENABLE;
    pstUsrRegCfg->stUsrDynaCorRegCfg.s8DpccSupTwinkleThrMax = HI_ISP_DPC_DEFAULT_SOFT_THR_MAX;
    pstUsrRegCfg->stUsrDynaCorRegCfg.s8DpccSupTwinkleThrMin = HI_ISP_DPC_DEFAULT_SOFT_THR_MIN;
    pstUsrRegCfg->stUsrDynaCorRegCfg.u16DpccRakeRatio       = HI_ISP_DPC_DEFAULT_SOFT_RAKE_RATIO;
    pstUsrRegCfg->stUsrDynaCorRegCfg.bResh                  = HI_TRUE;

    pstUsrRegCfg->stUsrStaCorRegCfg.u32DpccBptNumber        = HI_ISP_DPC_DEFAULT_BPT_NUMBER;
    pstUsrRegCfg->stUsrStaCorRegCfg.u32UpdateIndex          = 1;
    pstUsrRegCfg->stUsrStaCorRegCfg.bResh                   = HI_FALSE;

    return;
}

static HI_VOID DpDynaRegsInitialize(ISP_DPC_DYNA_CFG_S *pstDynaRegCfg)
{
    pstDynaRegCfg->bResh                = HI_TRUE;
    pstDynaRegCfg->bDpcStatEn           = 0;
    pstDynaRegCfg->u32DpccAlpha         = HI_ISP_DPC_DEFAULT_ALPHA;
    pstDynaRegCfg->u32DpccMode          = HI_ISP_DPC_DEFAULT_MODE;
    pstDynaRegCfg->u32DpccSetUse        = HI_ISP_DPC_DEFAULT_SET_USE;
    pstDynaRegCfg->u32DpccMethodsSet1   = HI_ISP_DPC_DEFAULT_METHODS_SET_1;
    pstDynaRegCfg->u32DpccMethodsSet2   = HI_ISP_DPC_DEFAULT_METHODS_SET_2;
    pstDynaRegCfg->u32DpccMethodsSet3   = HI_ISP_DPC_DEFAULT_METHODS_SET_3;
    pstDynaRegCfg->au16DpccLineThr[0]   = HI_ISP_DPC_DEFAULT_LINE_THRESH_1;
    pstDynaRegCfg->au16DpccLineMadFac[0] = HI_ISP_DPC_DEFAULT_LINE_MAD_FAC_1;
    pstDynaRegCfg->au16DpccPgFac[0]     = HI_ISP_DPC_DEFAULT_PG_FAC_1;
    pstDynaRegCfg->au16DpccRndThr[0]    = HI_ISP_DPC_DEFAULT_RND_THRESH_1;
    pstDynaRegCfg->au16DpccRgFac[0]     = HI_ISP_DPC_DEFAULT_RG_FAC_1;
    pstDynaRegCfg->au16DpccLineThr[1]   = HI_ISP_DPC_DEFAULT_LINE_THRESH_2;
    pstDynaRegCfg->au16DpccLineMadFac[1] = HI_ISP_DPC_DEFAULT_LINE_MAD_FAC_2;
    pstDynaRegCfg->au16DpccPgFac[1]     = HI_ISP_DPC_DEFAULT_PG_FAC_2;
    pstDynaRegCfg->au16DpccRndThr[1]    = HI_ISP_DPC_DEFAULT_RND_THRESH_2;
    pstDynaRegCfg->au16DpccRgFac[1]     = HI_ISP_DPC_DEFAULT_RG_FAC_2;
    pstDynaRegCfg->au16DpccLineThr[2]   = HI_ISP_DPC_DEFAULT_LINE_THRESH_3;
    pstDynaRegCfg->au16DpccLineMadFac[2] = HI_ISP_DPC_DEFAULT_LINE_MAD_FAC_3;
    pstDynaRegCfg->au16DpccPgFac[2]     = HI_ISP_DPC_DEFAULT_PG_FAC_3;
    pstDynaRegCfg->au16DpccRndThr[2]    = HI_ISP_DPC_DEFAULT_RND_THRESH_3;
    pstDynaRegCfg->au16DpccRgFac[2]     = HI_ISP_DPC_DEFAULT_RG_FAC_3;
    pstDynaRegCfg->u32DpccRoLimits      = HI_ISP_DPC_DEFAULT_RO_LIMITS;
    pstDynaRegCfg->u32DpccRndOffs       = HI_ISP_DPC_DEFAULT_RND_OFFS;
    pstDynaRegCfg->u32DpccBadThresh     = HI_ISP_DPC_DEFAULT_BPT_THRESH;

    pstDynaRegCfg->au16DpccLineStdThr[0] = HI_ISP_DPC_DEFAULT_LINE_STD_THR_1;
    pstDynaRegCfg->au16DpccLineStdThr[1] = HI_ISP_DPC_DEFAULT_LINE_STD_THR_2;
    pstDynaRegCfg->au16DpccLineStdThr[2] = HI_ISP_DPC_DEFAULT_LINE_STD_THR_3;
    pstDynaRegCfg->au16DpccLineStdThr[3] = HI_ISP_DPC_DEFAULT_LINE_STD_THR_4;
    pstDynaRegCfg->au16DpccLineStdThr[4] = HI_ISP_DPC_DEFAULT_LINE_STD_THR_5;


    pstDynaRegCfg->au16DpccLineDiffThr[0] = HI_ISP_DPC_DEFAULT_LINE_DIFF_THR_1;
    pstDynaRegCfg->au16DpccLineDiffThr[1] = HI_ISP_DPC_DEFAULT_LINE_DIFF_THR_2;
    pstDynaRegCfg->au16DpccLineDiffThr[2] = HI_ISP_DPC_DEFAULT_LINE_DIFF_THR_3;
    pstDynaRegCfg->au16DpccLineDiffThr[3] = HI_ISP_DPC_DEFAULT_LINE_DIFF_THR_4;
    pstDynaRegCfg->au16DpccLineDiffThr[4] = HI_ISP_DPC_DEFAULT_LINE_DIFF_THR_5;

    pstDynaRegCfg->au16DpccLineAverFac[0] = HI_ISP_DPC_DEFAULT_LINE_AVER_FAC_1;
    pstDynaRegCfg->au16DpccLineAverFac[1] = HI_ISP_DPC_DEFAULT_LINE_AVER_FAC_2;
    pstDynaRegCfg->au16DpccLineAverFac[2] = HI_ISP_DPC_DEFAULT_LINE_AVER_FAC_3;
    pstDynaRegCfg->au16DpccLineAverFac[3] = HI_ISP_DPC_DEFAULT_LINE_AVER_FAC_4;
    pstDynaRegCfg->au16DpccLineAverFac[4] = HI_ISP_DPC_DEFAULT_LINE_AVER_FAC_5;

    pstDynaRegCfg->u16DpccLineKerdiffFac = HI_ISP_DPC_DEFAULT_LINE_KERDIFF_FAC;
    pstDynaRegCfg->u16DpccBlendMode      = HI_ISP_DPC_DEFAULT_BLEND_MODE;
    pstDynaRegCfg->u16DpccBitDepthSel    = HI_ISP_DPC_DEFAULT_BIT_DEPTH_SEL;


    return;
}

static HI_VOID DpRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8     i;

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        DpStaticRegsInitialize(&pstRegCfg->stAlgRegCfg[i].stDpRegCfg.stStaticRegCfg);
        DpDynaRegsInitialize(&pstRegCfg->stAlgRegCfg[i].stDpRegCfg.stDynaRegCfg);
        DpUsrRegsInitialize(&pstRegCfg->stAlgRegCfg[i].stDpRegCfg.stUsrRegCfg);
    }

    ISP_DpEnableCfg(ViPipe, pstRegCfg->u8CfgNum, pstRegCfg);

    pstRegCfg->unKey.bit1DpCfg = 1;

    return;
}

static HI_VOID DpExtRegsInitialize(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_DEFECT_PIXEL_S *pstDp     = HI_NULL;

    DP_GET_CTX(ViPipe,  pstDp);

    //dynamic attr
    for (i = 0; i < 16; i++)
    {
        hi_ext_system_dpc_dynamic_strength_table_write(ViPipe, i, pstDp->stCmosDpc.au16Strength[i]);
        hi_ext_system_dpc_dynamic_blend_ratio_table_write(ViPipe, i, pstDp->stCmosDpc.au16BlendRatio[i]);
    }

    hi_ext_system_dpc_dynamic_cor_enable_write(ViPipe, pstDp->bEnable);
    hi_ext_system_dpc_manual_mode_write(ViPipe, HI_EXT_SYSTEM_DPC_MANU_MODE_DEFAULT);
    hi_ext_system_dpc_dynamic_manual_enable_write(ViPipe, HI_EXT_SYSTEM_DPC_DYNAMIC_MANUAL_ENABLE_DEFAULT);
    hi_ext_system_dpc_dynamic_strength_write(ViPipe, HI_EXT_SYSTEM_DPC_DYNAMIC_STRENGTH_DEFAULT);
    hi_ext_system_dpc_dynamic_blend_ratio_write(ViPipe, HI_EXT_SYSTEM_DPC_DYNAMIC_BLEND_RATIO_DEFAULT);
    hi_ext_system_dpc_suppress_twinkle_enable_write(ViPipe, HI_EXT_SYSTEM_DPC_SUPPRESS_TWINKLE_ENABLE_DEFAULT);
    hi_ext_system_dpc_suppress_twinkle_thr_write(ViPipe, HI_EXT_SYSTEM_DPC_SUPPRESS_TWINKLE_THR_DEFAULT);
    hi_ext_system_dpc_suppress_twinkle_slope_write(ViPipe, HI_EXT_SYSTEM_DPC_SUPPRESS_TWINKLE_SLOPE_DEFAULT);
    hi_ext_system_dpc_dynamic_attr_update_write(ViPipe, HI_TRUE);

    //static calib
    hi_ext_system_dpc_static_calib_enable_write(ViPipe, HI_EXT_SYSTEM_DPC_STATIC_CALIB_ENABLE_DEFAULT);
    hi_ext_system_dpc_count_max_write(ViPipe, HI_EXT_SYSTEM_DPC_COUNT_MAX_DEFAULT);
    hi_ext_system_dpc_count_min_write(ViPipe, HI_EXT_SYSTEM_DPC_COUNT_MIN_DEFAULT);
    hi_ext_system_dpc_start_thresh_write(ViPipe, HI_EXT_SYSTEM_DPC_START_THRESH_DEFAULT);
    hi_ext_system_dpc_trigger_status_write(ViPipe, HI_EXT_SYSTEM_DPC_TRIGGER_STATUS_DEFAULT);
    hi_ext_system_dpc_trigger_time_write(ViPipe, HI_EXT_SYSTEM_DPC_TRIGGER_TIME_DEFAULT);
    hi_ext_system_dpc_static_defect_type_write(ViPipe, HI_EXT_SYSTEM_DPC_STATIC_DEFECT_TYPE_DEFAULT);
    hi_ext_system_dpc_finish_thresh_write(ViPipe, HI_EXT_SYSTEM_DPC_START_THRESH_DEFAULT);
    hi_ext_system_dpc_bpt_calib_number_write(ViPipe, HI_EXT_SYSTEM_DPC_BPT_CALIB_NUMBER_DEFAULT);
    //static attr
    hi_ext_system_dpc_bpt_cor_number_write(ViPipe, HI_EXT_SYSTEM_DPC_BPT_COR_NUMBER_DEFAULT );
    hi_ext_system_dpc_static_cor_enable_write(ViPipe, pstDp->bEnable);
    hi_ext_system_dpc_static_dp_show_write(ViPipe, HI_EXT_SYSTEM_DPC_STATIC_DP_SHOW_DEFAULT);
    hi_ext_system_dpc_static_attr_update_write(ViPipe, HI_TRUE);

    //debug
    hi_ext_system_dpc_alpha0_rb_write(ViPipe, HI_EXT_SYSTEM_DPC_DYNAMIC_ALPHA0_RB_DEFAULT);
    hi_ext_system_dpc_alpha0_g_write(ViPipe,  HI_EXT_SYSTEM_DPC_DYNAMIC_ALPHA0_G_DEFAULT);

    return;
}

static HI_S32 DpcImageSize(VI_PIPE ViPipe, ISP_DEFECT_PIXEL_S *pstDp, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8      i;
    ISP_RECT_S stBlockRect;
    ISP_CTX_S  *pstIspCtx  = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstDp->u16Overlap = pstIspCtx->stBlockAttr.u32OverLap;

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        ISP_GetBlockRect(&stBlockRect, &pstIspCtx->stBlockAttr, i);

        pstDp->au16Offset[i] = stBlockRect.s32X;
        pstDp->au16OffsetForSplit[i] = (i == 0) ? 0 : (pstDp->au16Offset[i] + pstDp->u16Overlap - 2);
    }

    pstDp->au16OffsetForSplit[pstRegCfg->u8CfgNum] = pstIspCtx->stBlockAttr.stFrameRect.u32Width;

    return HI_SUCCESS;
}

static HI_S32 DpInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    ISP_DEFECT_PIXEL_S  *pstDp      = HI_NULL;
    ISP_CTX_S           *pstIspCtx  = HI_NULL;
    ISP_CMOS_DEFAULT_S  *pstSnsDft  = HI_NULL;

    DP_GET_CTX(ViPipe, pstDp);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);

    if ( DYNAMIC_RANGE_XDR == pstIspCtx->stHdrAttr.enDynamicRange)
    {
        pstDp->bEnable = HI_FALSE;
    }
    else
    {
        pstDp->bEnable = HI_TRUE;
    }

    if (pstSnsDft->stDpc.bValid)
    {
        memcpy(&pstDp->stCmosDpc, &pstSnsDft->stDpc, sizeof(ISP_CMOS_DPC_S));
    }
    else
    {
        memcpy(pstDp->stCmosDpc.au16Strength,  g_au16DpcStrength,  ISP_AUTO_ISO_STRENGTH_NUM * sizeof(HI_U16));
        memcpy(pstDp->stCmosDpc.au16BlendRatio, g_au16DpcBlendRatio, ISP_AUTO_ISO_STRENGTH_NUM * sizeof(HI_U16));
    }

    pstDp->u8TrialCount     = 0;
    pstDp->u8CalibStarted   = 0;
    pstDp->bStatEn          = 0;
    pstDp->u8HotDevThresh   = ISP_HOT_DEV_THRESH;
    pstDp->u8DeadDevThresh  = ISP_DEAD_DEV_THRESH;
    pstDp->u8FrameCnt       = 0;
    pstDp->u32DpccBadThresh = 0xff800080;
    pstDp->u8StaCalibrationParamsSet = ISP_CALIBRATION_PARAMS_SET;

    memset(pstDp->au32BptCalibTable, 0, ISP_STRIPING_MAX_NUM * ISP_DPC_MAX_BPT_NUM_NORMAL * sizeof(HI_U32));

    DpcImageSize(ViPipe, pstDp, pstRegCfg);

    return HI_SUCCESS;
}

static HI_S32 DpEnter(VI_PIPE ViPipe, HI_U8 u8BlkIdx, ISP_DEFECT_PIXEL_S *pstDp, ISP_ALG_REG_CFG_S  *pstAlgRegCfg)
{
    ISP_CMOS_BLACK_LEVEL_S  *pstSnsBlackLevel = HI_NULL;

    ISP_SensorGetBlc(ViPipe, &pstSnsBlackLevel);
    ISP_SensorSetPixelDetect(ViPipe, HI_TRUE);

    /*Should bypass digital gain when enter defect pixel calibration*/

    pstAlgRegCfg->stDgRegCfg.bDgEn                        = HI_FALSE;

    pstDp->u8StaticDPThresh = hi_ext_system_dpc_start_thresh_read(ViPipe);
    pstDp->u8CalibStarted   = 1;

    return HI_SUCCESS;
}

static HI_S32 DpExit(VI_PIPE ViPipe, HI_U8 u8BlkIdx, ISP_DEFECT_PIXEL_S *pstDp, ISP_REG_CFG_S *pstRegCfg)
{
    DpInitialize(ViPipe, pstRegCfg);

    ISP_SensorSetPixelDetect(ViPipe, HI_FALSE);
    pstDp->u8CalibStarted = 0;

    /*Not bypass digital gain when finish defect pixel calibration*/
    //isp_dg_en_write(ViPipe, u8BlkIdx, 1);
    //isp_wb_en_in_write(ViPipe, u8BlkIdx, 0);

    return HI_SUCCESS;
}

static HI_S32 DpReadStaticCalibExtregs(VI_PIPE ViPipe)
{
    ISP_DEFECT_PIXEL_S *pstDp = HI_NULL;

    DP_GET_CTX(ViPipe, pstDp);

    pstDp->u8PixelDetectType = hi_ext_system_dpc_static_defect_type_read(ViPipe);
    pstDp->u8TrialCntLimit   = (HI_U8)(hi_ext_system_dpc_trigger_time_read(ViPipe) >> 3);
    pstDp->u16DpCountMax     = hi_ext_system_dpc_count_max_read(ViPipe);
    pstDp->u16DpCountMin     = hi_ext_system_dpc_count_min_read(ViPipe);

    return HI_SUCCESS;
}

HI_U16 Dpc_Read_Calib_Num(VI_PIPE ViPipe, HI_U8 u8BlkNum, ISP_DEFECT_PIXEL_S *pstDp)
{
    HI_U8  i;
    HI_U16 u16BadPixelsCount = 0, j;
    HI_U32 u32BptValue;


    for (i = 0; i < u8BlkNum; i++)
    {
        pstDp->au16BpCalibNum[i] = isp_dpc_bpt_calib_number_read(ViPipe, i);
    }

    for (i = 0; i < u8BlkNum - 1; i++)
    {
        isp_dpc_bpt_raddr_write(ViPipe, i, 0);
        for (j = 0; j < pstDp->au16BpCalibNum[i]; j++)
        {
            u32BptValue = isp_dpc_bpt_rdata_read(ViPipe, i);
            if (((u32BptValue & 0x1FFF) + pstDp->au16Offset[i]) < pstDp->au16Offset[i + 1])
            {
                u16BadPixelsCount++;
            }
        }
    }

    u16BadPixelsCount += pstDp->au16BpCalibNum[u8BlkNum - 1];

    return u16BadPixelsCount;
}

HI_VOID Dpc_Calib_TimeOut(VI_PIPE ViPipe, ISP_DEFECT_PIXEL_S *pstDp)
{
    printf("BAD PIXEL CALIBRATION TIME OUT  0x%x\n", pstDp->u8TrialCntLimit);
    pstDp->bStaCalibrationEn = HI_FALSE;
    hi_ext_system_dpc_static_calib_enable_write(ViPipe, HI_FALSE);
    hi_ext_system_dpc_finish_thresh_write(ViPipe, pstDp->u8StaticDPThresh);
    hi_ext_system_dpc_trigger_status_write(ViPipe, 0x2);
}

HI_VOID Dpc_Calib_Max(HI_U16 u16BadPixelsCount, ISP_DEFECT_PIXEL_S *pstDp)
{
    printf("BAD_PIXEL_COUNT_UPPER_LIMIT 0x%x, 0x%x\n", pstDp->u8StaticDPThresh, u16BadPixelsCount );
    pstDp->u8FrameCnt = 2;
    //pstDp->u8StaticDPThresh++;
    pstDp->u8TrialCount ++;
}

HI_VOID Dpc_Calib_Min(HI_U16 u16BadPixelsCount, ISP_DEFECT_PIXEL_S *pstDp)
{
    printf("BAD_PIXEL_COUNT_LOWER_LIMIT 0x%x, 0x%x\n", pstDp->u8StaticDPThresh, u16BadPixelsCount);
    pstDp->u8FrameCnt = 2;

    pstDp->u8TrialCount ++;
}

HI_S32 MergingDpCalibLut(VI_PIPE ViPipe, ISP_DEFECT_PIXEL_S  *pstDp, HI_U8 ChnNum)
{
    HI_U16 i = 0, j = 0;
    HI_U16 u16BpNum = 0;
    HI_U32 *pu32BpTable = HI_NULL;

    if (ChnNum == 1)
    {
        hi_ext_system_dpc_bpt_calib_number_write(ViPipe, pstDp->au16BpCalibNum[0]);
        for ( i = 0; i < pstDp->au16BpCalibNum[0]; i++)
        {
            hi_ext_system_dpc_calib_bpt_write(ViPipe, i, pstDp->au32BptCalibTable[0][i]);
        }
        return HI_SUCCESS;
    }

    pu32BpTable = (HI_U32 *)malloc(8192 * sizeof(HI_U32));

    if (!pu32BpTable)
    {
        return HI_FAILURE;
    }

    while ((i <  pstDp->au16BpCalibNum[0]) && (j < pstDp->au16BpCalibNum[1]))
    {
        if (pstDp->au32BptCalibTable[0][i] > (pstDp->au32BptCalibTable[1][j] + pstDp->au16Offset[1]))
        {
            pu32BpTable[u16BpNum++] = pstDp->au32BptCalibTable[1][j++] + pstDp->au16Offset[1];
        }
        else if (pstDp->au32BptCalibTable[0][i] < (pstDp->au32BptCalibTable[1][j] + pstDp->au16Offset[1]))
        {
            pu32BpTable[u16BpNum++] = pstDp->au32BptCalibTable[0][i++];
        }
        else
        {
            pu32BpTable[u16BpNum++] = pstDp->au32BptCalibTable[0][i];
            i++;
            j++;
        }
    }

    if (i >=  pstDp->au16BpCalibNum[0])
    {
        while (j < pstDp->au16BpCalibNum[1])
        {
            pu32BpTable[u16BpNum++] = pstDp->au32BptCalibTable[1][j++] + pstDp->au16Offset[1];
        }
    }
    if (j >=  pstDp->au16BpCalibNum[1])
    {
        while (i < pstDp->au16BpCalibNum[0])
        {
            pu32BpTable[u16BpNum++] = pstDp->au32BptCalibTable[0][i++];
        }
    }

    hi_ext_system_dpc_bpt_calib_number_write(ViPipe, u16BpNum);
    for ( i = 0; i < u16BpNum; i++)
    {
        hi_ext_system_dpc_calib_bpt_write(ViPipe, i, pu32BpTable[i]);
    }

    if (pu32BpTable)
    {
        free(pu32BpTable);
    }

    return HI_SUCCESS;
}


HI_VOID Dpc_Calib_Success(VI_PIPE ViPipe, HI_U8 u8BlkNum, HI_U16 u16BadPixelsCount, ISP_DEFECT_PIXEL_S *pstDp)
{
    HI_U8  i;
    HI_U16 j;

    printf("trial: 0x%x, findshed: 0x%x\n", pstDp->u8TrialCount, u16BadPixelsCount);

    for (i = 0; i < u8BlkNum; i++)
    {
        isp_dpc_bpt_raddr_write(ViPipe, i, 0);
        for (j = 0; j < pstDp->au16BpCalibNum[i]; j++)
        {
            pstDp->au32BptCalibTable[i][j] = isp_dpc_bpt_rdata_read(ViPipe, i);
        }
    }

    MergingDpCalibLut(ViPipe, pstDp, u8BlkNum);

    pstDp->u32DpccMode = (ISP_DPCC_MODE & 0x3dd) + (pstDp->bStaticEnable << 5) + (pstDp->bDynamicEnable << 1);
    pstDp->bStatEn     = 0;
    pstDp->bStaCalibrationEn = HI_FALSE;
    hi_ext_system_dpc_static_calib_enable_write(ViPipe, HI_FALSE);
    hi_ext_system_dpc_finish_thresh_write(ViPipe, pstDp->u8StaticDPThresh);
    hi_ext_system_dpc_trigger_status_write(ViPipe, 0x1);
}

HI_VOID Dpc_Hot_Calib(VI_PIPE ViPipe, ISP_DEFECT_PIXEL_S  *pstDp, HI_U8 u8BlkNum, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8  i;
    HI_U16 u16BadPixelsCount;

    if (pstDp->u8FrameCnt < 9)
    {
        if (pstDp->u8FrameCnt == 1)
        {
            hi_ext_system_dpc_trigger_status_write(ViPipe, ISP_STATE_INIT);
            for ( i = 0; i < u8BlkNum; i++)
            {
                DpEnter(ViPipe, i, pstDp, &pstRegCfg->stAlgRegCfg[i]);
            }
        }

        pstDp->u8FrameCnt++;

        if (pstDp->u8FrameCnt == 3)
        {
            pstDp->u32DpccBadThresh = (pstDp->u8StaticDPThresh << 24) + (((50 + 0x80 * pstDp->u8HotDevThresh) / 100) << 16) + 0x00000080;
            pstDp->u32DpccMode      = (ISP_DPCC_HOT_MODE & 0x3dd) + (pstDp->bStaticEnable << 5) + (pstDp->bDynamicEnable << 1);
            pstDp->bStatEn          = 1;
        }

        if (pstDp->u8FrameCnt == 5)
        {
            pstDp->u32DpccMode = (ISP_DPCC_MODE & 0x3dd) + (pstDp->bStaticEnable << 5) + (pstDp->bDynamicEnable << 1);
            pstDp->bStatEn     = 0;
        }

        if (pstDp->u8FrameCnt == 7)
        {
            u16BadPixelsCount = Dpc_Read_Calib_Num(ViPipe, u8BlkNum, pstDp);

            if (pstDp->u8TrialCount >= pstDp->u8TrialCntLimit)//TIMEOUT
            {
                Dpc_Calib_TimeOut(ViPipe, pstDp);

                for (i = 0; i < u8BlkNum; i++)
                {
                    DpExit(ViPipe, i, pstDp, pstRegCfg);
                }
            }
            else if (u16BadPixelsCount > pstDp->u16DpCountMax)
            {
                Dpc_Calib_Max(u16BadPixelsCount, pstDp);
                pstDp->u8StaticDPThresh++;
            }
            else if (u16BadPixelsCount < pstDp->u16DpCountMin)
            {
                Dpc_Calib_Min(u16BadPixelsCount, pstDp);
                if (pstDp->u8StaticDPThresh == 1 )
                {
                    pstDp->u8TrialCount = pstDp->u8TrialCntLimit;
                }

                pstDp->u8StaticDPThresh--;
            }
            else//SUCCESS
            {
                Dpc_Calib_Success(ViPipe, u8BlkNum, u16BadPixelsCount, pstDp);
                for (i = 0; i < u8BlkNum; i++)
                {
                    DpExit(ViPipe, i, pstDp, pstRegCfg);
                }
            }
        }
    }
}

HI_VOID Dpc_Dark_Calib(VI_PIPE ViPipe, ISP_DEFECT_PIXEL_S  *pstDp, HI_U8 u8BlkNum, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U16 u16BadPixelsCount;

    if (pstDp->u8FrameCnt < 9)
    {
        if (pstDp->u8FrameCnt == 1)
        {
            hi_ext_system_dpc_trigger_status_write(ViPipe, ISP_STATE_INIT);
            pstDp->u8CalibStarted = 1;
            pstDp->u8StaticDPThresh = hi_ext_system_dpc_start_thresh_read(ViPipe);
        }

        pstDp->u8FrameCnt++;

        if (pstDp->u8FrameCnt == 3)
        {
            pstDp->u32DpccBadThresh = 0xFF800000 + (pstDp->u8StaticDPThresh << 8) + ((0x80 * pstDp->u8DeadDevThresh) / 100);
            pstDp->u32DpccMode      = (ISP_DPCC_DEAD_MODE & 0x3dd) + (pstDp->bStaticEnable << 5) + (pstDp->bDynamicEnable << 1);
            pstDp->bStatEn          = 1;
        }

        if (pstDp->u8FrameCnt == 5)
        {
            pstDp->u32DpccMode = (ISP_DPCC_MODE & 0x3dd) + (pstDp->bStaticEnable << 5) + (pstDp->bDynamicEnable << 1);
            pstDp->bStatEn     = 0;
        }

        if (pstDp->u8FrameCnt == 7)
        {
            u16BadPixelsCount = Dpc_Read_Calib_Num(ViPipe, u8BlkNum, pstDp);

            if (pstDp->u8TrialCount >= pstDp->u8TrialCntLimit)
            {
                Dpc_Calib_TimeOut(ViPipe, pstDp);
                DpInitialize(ViPipe, pstRegCfg);
                pstDp->u8CalibStarted = 0;
            }
            else if (u16BadPixelsCount > pstDp->u16DpCountMax)
            {
                Dpc_Calib_Max(u16BadPixelsCount, pstDp);
                pstDp->u8StaticDPThresh--;
            }
            else if (u16BadPixelsCount < pstDp->u16DpCountMin)
            {
                Dpc_Calib_Min(u16BadPixelsCount, pstDp);
                if (pstDp->u8StaticDPThresh == 255 )
                {
                    pstDp->u8TrialCount = pstDp->u8TrialCntLimit;
                }
                pstDp->u8StaticDPThresh++;
            }
            else
            {
                Dpc_Calib_Success(ViPipe, u8BlkNum, u16BadPixelsCount, pstDp);
                DpInitialize(ViPipe, pstRegCfg);

                pstDp->u8CalibStarted = 0;
            }
        }
    }
}

HI_VOID ISP_Dpc_StaticCalibration(VI_PIPE ViPipe, ISP_DEFECT_PIXEL_S  *pstDp, HI_U8 u8BlkNum, ISP_REG_CFG_S *pstRegCfg)
{
    if (0 == pstDp->u8PixelDetectType)
    {
        Dpc_Hot_Calib(ViPipe, pstDp, u8BlkNum, pstRegCfg);
    }
    else if (1 == pstDp->u8PixelDetectType)
    {
        Dpc_Dark_Calib(ViPipe, pstDp, u8BlkNum, pstRegCfg);
    }
    else
    {
        printf("invalid static defect pixel detect type!\n");
    }

    return ;
}

HI_S32 ISP_Dpc_Calib_Mode(VI_PIPE ViPipe, ISP_DEFECT_PIXEL_S *pstDp, ISP_REG_CFG_S *pstRegCfg, ISP_CTX_S *pstIspCtx)
{
    HI_U8 i;

    DpReadStaticCalibExtregs(ViPipe);
    pstIspCtx->stLinkage.bDefectPixel = HI_TRUE;
    ISP_Dpc_StaticCalibration(ViPipe, pstDp, pstRegCfg->u8CfgNum, pstRegCfg);

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        pstRegCfg->stAlgRegCfg[i].stDpRegCfg.stDynaRegCfg.u32DpccBadThresh  = pstDp->u32DpccBadThresh;
        pstRegCfg->stAlgRegCfg[i].stDpRegCfg.stDynaRegCfg.u32DpccMode       = pstDp->u32DpccMode;
        pstRegCfg->stAlgRegCfg[i].stDpRegCfg.stDynaRegCfg.bDpcStatEn        = pstDp->bStatEn;
        pstRegCfg->stAlgRegCfg[i].stDpRegCfg.stDynaRegCfg.bResh             = HI_TRUE;
    }

    return HI_SUCCESS;
}

static HI_S32 DpReadExtregs(VI_PIPE ViPipe)
{
    HI_U16 i;
    ISP_DEFECT_PIXEL_S *pstDp = HI_NULL;

    DP_GET_CTX(ViPipe, pstDp);

    pstDp->bStaticAttrUpdate    = hi_ext_system_dpc_static_attr_update_read(ViPipe);
    hi_ext_system_dpc_static_attr_update_write(ViPipe, HI_FALSE);

    if (pstDp->bStaticAttrUpdate)
    {

        pstDp->u16BptCorNum         = hi_ext_system_dpc_bpt_cor_number_read(ViPipe);
        pstDp->bStaicShow           = hi_ext_system_dpc_static_dp_show_read(ViPipe);

        for (i = 0; i < ISP_DPC_MAX_BPT_NUM_SBS; i++)
        {
            if (i < pstDp->u16BptCorNum)
            {
                pstDp->au32BptCorTable[i] = hi_ext_system_dpc_cor_bpt_read(ViPipe, i);
            }
            else
            {
                pstDp->au32BptCorTable[i] = 0;
            }
        }
    }

    pstDp->bDynaAttrUpdateEn = hi_ext_system_dpc_dynamic_attr_update_read(ViPipe);
    hi_ext_system_dpc_dynamic_attr_update_write(ViPipe, HI_FALSE);

    if (pstDp->bDynaAttrUpdateEn)
    {
        pstDp->bDynamicManual       = hi_ext_system_dpc_dynamic_manual_enable_read(ViPipe);

        for (i = 0; i < 16; i++)
        {
            pstDp->stCmosDpc.au16Strength[i]    = hi_ext_system_dpc_dynamic_strength_table_read(ViPipe, i);
            pstDp->stCmosDpc.au16BlendRatio[i]  = hi_ext_system_dpc_dynamic_blend_ratio_table_read(ViPipe, i);
        }

        pstDp->u16BlendRatio        = hi_ext_system_dpc_dynamic_blend_ratio_read(ViPipe);
        pstDp->u16Strength          = hi_ext_system_dpc_dynamic_strength_read(ViPipe);
        pstDp->bSupTwinkleEn        = hi_ext_system_dpc_suppress_twinkle_enable_read(ViPipe);
        pstDp->s8SupTwinkleThr      = hi_ext_system_dpc_suppress_twinkle_thr_read(ViPipe);
        pstDp->u8SupTwinkleSlope    = hi_ext_system_dpc_suppress_twinkle_slope_read(ViPipe);
    }

    return HI_SUCCESS;
}

HI_S32  SplitDpCorLut(ISP_DEFECT_PIXEL_S  *pstDp, ISP_REG_CFG_S *pstRegCfg, HI_U8 u8BlkNum)
{
    HI_S8  j;
    HI_U16 au16BptNum[ISP_STRIPING_MAX_NUM] = {0};
    HI_U16 i;
    HI_U16 u16XValue;
    for (j = 0; j < u8BlkNum; j++)
    {
        memset(pstRegCfg->stAlgRegCfg[j].stDpRegCfg.stUsrRegCfg.stUsrStaCorRegCfg.au32DpccBpTable, 0, 4096 * sizeof(HI_U32));
    }

    for (j = (HI_S8)u8BlkNum - 1; j >= 0; j--)
    {
        for (i = 0; i < pstDp->u16BptCorNum; i++)
        {
            u16XValue = pstDp->au32BptCorTable[i] & 0x1FFF;
            if ((u16XValue >= (pstDp->au16OffsetForSplit[j])) && (u16XValue < pstDp->au16OffsetForSplit[j + 1]))
            {
                pstRegCfg->stAlgRegCfg[j].stDpRegCfg.stUsrRegCfg.stUsrStaCorRegCfg.au32DpccBpTable[au16BptNum[j]] = pstDp->au32BptCorTable[i] - pstDp->au16Offset[j];
                au16BptNum[j]++;

                if (au16BptNum[j] >= 4096)
                {
                    break;
                }
            }
        }
    }

    for (j = 0; j < (HI_S8)u8BlkNum; j++)
    {
        pstRegCfg->stAlgRegCfg[j].stDpRegCfg.stUsrRegCfg.stUsrStaCorRegCfg.u32DpccBptNumber = au16BptNum[j];
        pstRegCfg->stAlgRegCfg[j].stDpRegCfg.stUsrRegCfg.stUsrStaCorRegCfg.bResh            = HI_TRUE;
        pstRegCfg->stAlgRegCfg[j].stDpRegCfg.stUsrRegCfg.stUsrStaCorRegCfg.u32UpdateIndex  += 1;
    }

    return HI_SUCCESS;
}

static HI_S32 CalcRakeRatio(HI_S32 x0, HI_S32 y0, HI_S32 x1, HI_S32 y1, HI_S32 shift)
{
    if ( x0 == x1 )
    {
        return 0;
    }
    else
    {
        return ( (y1 - y0) << shift ) / DIV_0_TO_1( x1 - x0);
    }
}

HI_S32 ISP_Dpc_Usr_Cfg(VI_PIPE ViPipe, ISP_DEFECT_PIXEL_S *pstDp, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8 i;
    ISP_DPC_USR_DYNA_COR_CFG_S *pstUsrDynaCorRegCfg;

    if (pstDp->bStaticAttrUpdate)
    {
        if (pstDp->bStaicShow || pstDp->bStaticEnable)
        {
            SplitDpCorLut(pstDp, pstRegCfg, pstRegCfg->u8CfgNum);
        }
    }

    if (pstDp->bDynaAttrUpdateEn)
    {
        for (i = 0; i < pstRegCfg->u8CfgNum; i++)
        {
            pstUsrDynaCorRegCfg = &pstRegCfg->stAlgRegCfg[i].stDpRegCfg.stUsrRegCfg.stUsrDynaCorRegCfg;

            pstUsrDynaCorRegCfg->bDpccHardThrEn         = pstDp->bSupTwinkleEn ? (HI_FALSE) : (HI_TRUE);
            pstUsrDynaCorRegCfg->s8DpccSupTwinkleThrMax = CLIP3(pstDp->s8SupTwinkleThr, -128, 127);
            pstUsrDynaCorRegCfg->s8DpccSupTwinkleThrMin = CLIP3(pstUsrDynaCorRegCfg->s8DpccSupTwinkleThrMax - pstDp->u8SupTwinkleSlope, -128, 127);
            pstUsrDynaCorRegCfg->u16DpccRakeRatio       = CalcRakeRatio(pstUsrDynaCorRegCfg->s8DpccSupTwinkleThrMin, 0, pstUsrDynaCorRegCfg->s8DpccSupTwinkleThrMax, 128, 2);
            pstUsrDynaCorRegCfg->bResh                  = HI_TRUE;
        }
    }

    return HI_SUCCESS;
}

HI_S32 ISP_Dpc_Show_Mode(VI_PIPE ViPipe, ISP_DEFECT_PIXEL_S *pstDp, ISP_REG_CFG_S *pstRegCfg, ISP_CTX_S *pstIspCtx)
{
    HI_U8 i;

    pstIspCtx->stLinkage.bDefectPixel = HI_FALSE;

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        pstRegCfg->stAlgRegCfg[i].stDpRegCfg.stDynaRegCfg.u32DpccMode = ISP_DPCC_HIGHLIGHT_MODE;
        pstRegCfg->stAlgRegCfg[i].stDpRegCfg.stDynaRegCfg.bResh       = HI_TRUE;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DPC_Linear_Interpol(HI_S32 xm, HI_S32 x0, HI_S32 y0, HI_S32 x1, HI_S32 y1)
{
    HI_S32 ym;

    if ( xm <= x0 )
    {
        return y0;
    }
    if ( xm >= x1 )
    {
        return y1;
    }

    ym = (y1 - y0) * (xm - x0) / DIV_0_TO_1(x1 - x0) + y0;
    return ym;
}
static HI_VOID soft_inter(ISP_DPC_DYNA_CFG_S *pstDpcHwCfg, HI_U32 u32DpccStat)
{
    HI_S32 s32StatIdxUp, s32StatIdxLow;
    HI_S32 s32StatUpper, s32StatLower;
    HI_U32 u32DpccLineThrRb1, u32DpccLineThrG1, u32DpccLineMadFacRb1, u32DpccLineMadFacG1;
    HI_S32 i;

    s32StatIdxUp = ISP_DPC_SOFT_SLOPE_GRADE - 1;
    for (i = 0; i < ISP_DPC_SOFT_SLOPE_GRADE; i++)
    {
        if ((HI_S32)u32DpccStat < g_s32SoftSlopeGrade[i])
        {
            s32StatIdxUp = i;
            break;
        }
    }
    s32StatIdxLow = MAX2(s32StatIdxUp - 1, 0);

    s32StatUpper = g_s32SoftSlopeGrade[s32StatIdxUp];
    s32StatLower = g_s32SoftSlopeGrade[s32StatIdxLow];

    u32DpccLineThrRb1 = (HI_U32)ISP_DPC_Linear_Interpol(u32DpccStat,
                        s32StatLower, (g_s32SoftLineThr[s32StatIdxLow] & 0xFF00) >> 8,
                        s32StatUpper, (g_s32SoftLineThr[s32StatIdxUp] & 0xFF00) >> 8);
    u32DpccLineThrG1  = (HI_U32)ISP_DPC_Linear_Interpol(u32DpccStat,
                        s32StatLower, g_s32SoftLineThr[s32StatIdxLow] & 0xFF,
                        s32StatUpper, g_s32SoftLineThr[s32StatIdxUp] & 0xFF);

    u32DpccLineMadFacRb1 = (HI_U32)ISP_DPC_Linear_Interpol(u32DpccStat,
                           s32StatLower, (g_s32SoftLineMadFac[s32StatIdxLow] & 0xFF00) >> 8,
                           s32StatUpper, (g_s32SoftLineMadFac[s32StatIdxUp] & 0xFF00) >> 8);
    u32DpccLineMadFacG1 = (HI_U32)ISP_DPC_Linear_Interpol(u32DpccStat,
                          s32StatLower, g_s32SoftLineMadFac[s32StatIdxLow] & 0xFF,
                          s32StatUpper, g_s32SoftLineMadFac[s32StatIdxUp] & 0xFF);
    pstDpcHwCfg->au16DpccLineThr[0]    = ((u32DpccLineThrRb1 & 0xFF) << 8) + (u32DpccLineThrG1 & 0xFF);
    pstDpcHwCfg->au16DpccLineMadFac[0] = ((u32DpccLineMadFacRb1 & 0x3F) << 8) + (u32DpccLineMadFacG1 & 0x3F);
}

static HI_VOID set_dpcc_parameters_inter(ISP_DPC_DYNA_CFG_S *pstIspDpccHwCfg, HI_U32 u32DpccStat)
{
    HI_S32 i, j;
    HI_S32 s32StatUpper, s32StatLower;
    HI_S32 s32StatIdxUp, s32StatIdxLow;
    ISP_DPCC_DERIVED_PARAM_S stDpcDerParam;

    s32StatIdxUp = ISP_DPC_SLOPE_GRADE - 1;
    for (i = 0; i < ISP_DPC_SLOPE_GRADE; i++)
    {
        if ((HI_S32)u32DpccStat < g_as32SlopeGrade[i])
        {
            s32StatIdxUp = i;
            break;
        }
    }
    s32StatIdxLow = MAX2(s32StatIdxUp - 1, 0);

    s32StatUpper = g_as32SlopeGrade[s32StatIdxUp];
    s32StatLower = g_as32SlopeGrade[s32StatIdxLow];

    pstIspDpccHwCfg->u32DpccSetUse      = g_stDpcDefCfg[s32StatIdxLow].u32DpccSetUse;
    pstIspDpccHwCfg->u32DpccMethodsSet1 = g_stDpcDefCfg[s32StatIdxLow].u32DpccMethodsSet1;
    pstIspDpccHwCfg->u32DpccMethodsSet2 = g_stDpcDefCfg[s32StatIdxLow].u32DpccMethodsSet2;
    pstIspDpccHwCfg->u32DpccMethodsSet3 = g_stDpcDefCfg[s32StatIdxLow].u32DpccMethodsSet3;
    pstIspDpccHwCfg->u32DpccBadThresh   = g_stDpcDefCfg[s32StatIdxLow].u32DpccBadThresh;

    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 3; j++)
        {
            stDpcDerParam.au16DpccLineThr[i][j]    = (HI_U16)ISP_DPC_Linear_Interpol(u32DpccStat,
                    s32StatLower, g_stDpcDerParam[s32StatIdxLow].au16DpccLineThr[i][j],
                    s32StatUpper, g_stDpcDerParam[s32StatIdxUp].au16DpccLineThr[i][j]);
            stDpcDerParam.au16DpccLineMadFac[i][j] = (HI_U16)ISP_DPC_Linear_Interpol(u32DpccStat,
                    s32StatLower, g_stDpcDerParam[s32StatIdxLow].au16DpccLineMadFac[i][j],
                    s32StatUpper, g_stDpcDerParam[s32StatIdxUp].au16DpccLineMadFac[i][j]);
            stDpcDerParam.au16DpccPgFac[i][j]      = (HI_U16)ISP_DPC_Linear_Interpol(u32DpccStat,
                    s32StatLower, g_stDpcDerParam[s32StatIdxLow].au16DpccPgFac[i][j],
                    s32StatUpper, g_stDpcDerParam[s32StatIdxUp].au16DpccPgFac[i][j]);
            stDpcDerParam.au16DpccRgFac[i][j]      = (HI_U16)ISP_DPC_Linear_Interpol(u32DpccStat,
                    s32StatLower, g_stDpcDerParam[s32StatIdxLow].au16DpccRgFac[i][j],
                    s32StatUpper, g_stDpcDerParam[s32StatIdxUp].au16DpccRgFac[i][j]);
            stDpcDerParam.au16DpccRndThr[i][j]     = (HI_U16)ISP_DPC_Linear_Interpol(u32DpccStat,
                    s32StatLower, g_stDpcDerParam[s32StatIdxLow].au16DpccRndThr[i][j],
                    s32StatUpper, g_stDpcDerParam[s32StatIdxUp].au16DpccRndThr[i][j]);
            stDpcDerParam.au16DpccRndOffs[i][j]    = (HI_U16)ISP_DPC_Linear_Interpol(u32DpccStat,
                    s32StatLower, g_stDpcDerParam[s32StatIdxLow].au16DpccRndOffs[i][j],
                    s32StatUpper, g_stDpcDerParam[s32StatIdxUp].au16DpccRndOffs[i][j]);
            stDpcDerParam.au16DpccRo[i][j]         = (HI_U16)ISP_DPC_Linear_Interpol(u32DpccStat,
                    s32StatLower, g_stDpcDerParam[s32StatIdxLow].au16DpccRo[i][j],
                    s32StatUpper, g_stDpcDerParam[s32StatIdxUp].au16DpccRo[i][j]);
        }
    }

    for (j = 0; j < 3; j++)
    {
        pstIspDpccHwCfg->au16DpccLineThr[j]    = ((stDpcDerParam.au16DpccLineThr[0][j]   & 0xFF) << 8) + (stDpcDerParam.au16DpccLineThr[1][j]   & 0xFF);
        pstIspDpccHwCfg->au16DpccLineMadFac[j] = ((stDpcDerParam.au16DpccLineMadFac[0][j] & 0x3F) << 8) + (stDpcDerParam.au16DpccLineMadFac[1][j] & 0x3F);
        pstIspDpccHwCfg->au16DpccPgFac[j]      = ((stDpcDerParam.au16DpccPgFac[0][j]     & 0x3F) << 8) + (stDpcDerParam.au16DpccPgFac[1][j]     & 0x3F);
        pstIspDpccHwCfg->au16DpccRndThr[j]     = ((stDpcDerParam.au16DpccRndThr[0][j]    & 0xFF) << 8) + (stDpcDerParam.au16DpccRndThr[1][j]    & 0xFF);
        pstIspDpccHwCfg->au16DpccRgFac[j]      = ((stDpcDerParam.au16DpccRgFac[0][j]     & 0x3F) << 8) + (stDpcDerParam.au16DpccRgFac[1][j]     & 0x3F);
    }

    pstIspDpccHwCfg->u32DpccRoLimits    = ((stDpcDerParam.au16DpccRo[0][2] & 0x3) << 10) + ((stDpcDerParam.au16DpccRo[1][2] & 0x3) << 8) + ((stDpcDerParam.au16DpccRo[0][1] & 0x3) << 6) +
                                          ((stDpcDerParam.au16DpccRo[1][1] & 0x3) << 4) + ((stDpcDerParam.au16DpccRo[0][0] & 0x3) << 2) + (stDpcDerParam.au16DpccRo[1][0] & 0x3);

    pstIspDpccHwCfg->u32DpccRndOffs     = ((stDpcDerParam.au16DpccRndOffs[0][2] & 0x3) << 10) + ((stDpcDerParam.au16DpccRndOffs[1][2] & 0x3) << 8) + ((stDpcDerParam.au16DpccRndOffs[0][1] & 0x3) << 6) +
                                          ((stDpcDerParam.au16DpccRndOffs[1][1] & 0x3) << 4) + ((stDpcDerParam.au16DpccRndOffs[0][0] & 0x3) << 2) + (stDpcDerParam.au16DpccRndOffs[1][0] & 0x3);

    return;
}

HI_S32 ISP_Dynamic_set(HI_S32 s32Iso, ISP_DPC_DYNA_CFG_S *pstDpcHwCfg, ISP_DEFECT_PIXEL_S *pstDpcFwCfg)
{
    HI_U8  i = 0;
    HI_U8  u8Alpha0RB = 0;
    HI_U8  u8Alpha0G  = 0;  //the blend ratio of 5 & 9
    HI_U8  u8Alpha1RB = 0;
    HI_U8  u8Alpha1G  = 0;  //the blend ratio of input data and filtered result
    HI_U16 u16BlendRatio = 0x0;
    HI_U32 u32DpccStat   = 0;
    HI_U32 u32DpccMode   = pstDpcFwCfg->u32DpccMode;
    HI_S32 s32IsoIndexUpper, s32IsoIndexLower;
    ISP_CMOS_DPC_S *pstDpc = &pstDpcFwCfg->stCmosDpc;

    if (pstDpcFwCfg->bDynamicManual)
    {
        u32DpccStat = pstDpcFwCfg->u16Strength;
        u16BlendRatio = pstDpcFwCfg->u16BlendRatio;
    }
    else
    {
        s32IsoIndexUpper = ISP_AUTO_ISO_STRENGTH_NUM - 1;

        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
        {
            if (s32Iso <= g_as32IsoLutLow[i])
            {
                s32IsoIndexUpper = i;
                break;
            }
        }

        s32IsoIndexLower = MAX2(s32IsoIndexUpper - 1, 0);

        u32DpccStat     = (HI_U32)ISP_DPC_Linear_Interpol(s32Iso,
                          g_as32IsoLutLow[s32IsoIndexLower], (HI_S32)pstDpc->au16Strength[s32IsoIndexLower],
                          g_as32IsoLutLow[s32IsoIndexUpper], (HI_S32)pstDpc->au16Strength[s32IsoIndexUpper]);
        u16BlendRatio   = (HI_U16)ISP_DPC_Linear_Interpol(s32Iso,
                          g_as32IsoLutLow[s32IsoIndexLower], (HI_S32)pstDpc->au16BlendRatio[s32IsoIndexLower],
                          g_as32IsoLutLow[s32IsoIndexUpper], (HI_S32)pstDpc->au16BlendRatio[s32IsoIndexUpper]);
    }

    u32DpccStat >>= 1;
    set_dpcc_parameters_inter(pstDpcHwCfg, u32DpccStat);
    if (u32DpccStat == 0)
    {
        u32DpccMode &= 0xFFFFFFFC;
    }
    else if (u32DpccStat > 100)
    {
        pstDpcHwCfg->u32DpccSetUse        = 0x1;
        pstDpcHwCfg->u32DpccMethodsSet1   = 0x1f1f;
        pstDpcHwCfg->au16DpccLineThr[0]   = g_as32DpcLineThr1[u32DpccStat - 101];
        pstDpcHwCfg->au16DpccLineMadFac[0] = g_as32DpcLineMadFac1[u32DpccStat - 101];
        pstDpcHwCfg->au16DpccPgFac[0]     = g_as32DpcPgFac1[u32DpccStat - 101];
        pstDpcHwCfg->au16DpccRndThr[0]    = g_as32DpcRndThr1[u32DpccStat - 101];
        pstDpcHwCfg->au16DpccRgFac[0]     = g_as32DpcRgFac1[u32DpccStat - 101];
        pstDpcHwCfg->u32DpccRoLimits      = g_as32DpcRoLimits1[u32DpccStat - 101];
        pstDpcHwCfg->u32DpccRndOffs       = g_as32DpcRndOffs1[u32DpccStat - 101];
    }

    if (pstDpcFwCfg->bSupTwinkleEn)
    {
        if ((u32DpccStat == 0) || !((pstDpcFwCfg->u32DpccMode & 0x2) >> 1))
        {
            pstDpcFwCfg->bSupTwinkleEn = 0;
        }
        else
        {
            soft_inter(pstDpcHwCfg, u32DpccStat);
        }
    }

    if (!((u32DpccMode & 0x2) >> 1))
    {
        u16BlendRatio = 0;
    }
    u8Alpha0RB = (u16BlendRatio > 0x80) ? (u16BlendRatio - 0x80) : 0x0;
    u8Alpha1RB = (u16BlendRatio > 0x80) ? 0x80 : u16BlendRatio;
    pstDpcHwCfg->u32DpccAlpha = (u8Alpha0RB << 24) + (u8Alpha0G << 16) + (u8Alpha1RB << 8) + u8Alpha1G;
    pstDpcHwCfg->u32DpccMode  = u32DpccMode;

    return HI_SUCCESS;
}

HI_S32 ISP_Dpc_Normal_Mode(VI_PIPE ViPipe, ISP_DEFECT_PIXEL_S *pstDp, ISP_REG_CFG_S *pstRegCfg, ISP_CTX_S *pstIspCtx)
{
    HI_U8 i;

    pstIspCtx->stLinkage.bDefectPixel = HI_FALSE;

    pstDp->u32DpccMode = (ISP_DPCC_MODE & 0x3dd) + (pstDp->bStaticEnable << 5) + (pstDp->bDynamicEnable << 1);

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        ISP_Dynamic_set((HI_S32)pstIspCtx->stLinkage.u32Iso, &pstRegCfg->stAlgRegCfg[i].stDpRegCfg.stDynaRegCfg, pstDp);
        pstRegCfg->stAlgRegCfg[i].stDpRegCfg.stDynaRegCfg.bResh = HI_TRUE;
    }

    return HI_SUCCESS;
}

static HI_VOID ISP_DpWdrModeSet(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg    = (ISP_REG_CFG_S *)pRegCfg;

    ISP_DpEnableCfg(ViPipe, pstRegCfg->u8CfgNum, pstRegCfg);

    pstRegCfg->unKey.bit1DpCfg = 1;
}

HI_S32 ISP_DpInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg    = (ISP_REG_CFG_S *)pRegCfg;

    DpInitialize(ViPipe, pstRegCfg);
    DpRegsInitialize(ViPipe, pstRegCfg);
    DpExtRegsInitialize(ViPipe);

    return HI_SUCCESS;
}

HI_S32 ISP_DpRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                 HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_U8 i;
    ISP_CTX_S           *pstIspCtx  = HI_NULL;
    ISP_DEFECT_PIXEL_S  *pstDp      = HI_NULL;
    ISP_REG_CFG_S       *pstRegCfg  = (ISP_REG_CFG_S *)pRegCfg;

    DP_GET_CTX(ViPipe, pstDp);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstRegCfg->unKey.bit1DpCfg = 1;

    pstDp->bStaCalibrationEn = hi_ext_system_dpc_static_calib_enable_read(ViPipe);
    pstDp->bDynamicEnable    = hi_ext_system_dpc_dynamic_cor_enable_read(ViPipe);
    pstDp->bStaticEnable     = hi_ext_system_dpc_static_cor_enable_read(ViPipe);

    if ((!pstDp->bStaCalibrationEn) && (pstDp->u8CalibStarted == 1))
    {
        for (i = 0; i < pstRegCfg->u8CfgNum; i++)
        {
            DpExit(ViPipe, i, pstDp, pstRegCfg);
        }
    }

    if (pstDp->bStaCalibrationEn)//calibration mode
    {
        if ((pstIspCtx->stBlockAttr.enIspRunningMode == ISP_MODE_RUNNING_OFFLINE) \
            || (pstIspCtx->stBlockAttr.enIspRunningMode == ISP_MODE_RUNNING_STRIPING))
        {
            return HI_FAILURE;
        }

        ISP_Dpc_Calib_Mode(ViPipe, pstDp, pstRegCfg, pstIspCtx);

        return HI_SUCCESS;
    }

    DpReadExtregs(ViPipe);

    if (pstDp->bStaicShow) //highlight static defect pixels mode
    {
        ISP_Dpc_Show_Mode(ViPipe, pstDp, pstRegCfg, pstIspCtx);
    }
    else//normal detection and correction mode
    {
        ISP_Dpc_Normal_Mode(ViPipe, pstDp, pstRegCfg, pstIspCtx);
    }

    ISP_Dpc_Usr_Cfg(ViPipe, pstDp, pstRegCfg);

    return HI_SUCCESS;
}

HI_S32 ISP_DpCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    ISP_REGCFG_S        *pRegCfg = HI_NULL;
    ISP_DEFECT_PIXEL_S  *pstDp   = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);
    DP_GET_CTX(ViPipe, pstDp);

    switch (u32Cmd)
    {
        case ISP_WDR_MODE_SET:
            ISP_DpWdrModeSet(ViPipe, (HI_VOID *)&pRegCfg->stRegCfg);
            break;
        case ISP_CHANGE_IMAGE_MODE_SET:
            DpcImageSize(ViPipe, pstDp, &pRegCfg->stRegCfg);
            break;
        default :
            break;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DpExit(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);

    for (i = 0; i < pRegCfg->stRegCfg.u8CfgNum; i++)
    {
        pRegCfg->stRegCfg.stAlgRegCfg[i].stDpRegCfg.abDpcEn[0] = HI_FALSE;
        pRegCfg->stRegCfg.stAlgRegCfg[i].stDpRegCfg.abDpcEn[1] = HI_FALSE;
        pRegCfg->stRegCfg.stAlgRegCfg[i].stDpRegCfg.abDpcEn[2] = HI_FALSE;
        pRegCfg->stRegCfg.stAlgRegCfg[i].stDpRegCfg.abDpcEn[3] = HI_FALSE;
    }

    pRegCfg->stRegCfg.unKey.bit1DpCfg = 1;

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterDp(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_DP;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_DpInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_DpRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_DpCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_DpExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

