/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_bayernr.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2015/07/21
  Description   :
  History       :
  1.Date        : 2015/07/21
    Author      :
    Modification: Created file

******************************************************************************/
#include <math.h>
#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_config.h"
#include "isp_ext_config.h"
#include "isp_proc.h"
#include "isp_math_utils.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define HI_ISP_BAYERNR_BITDEP (16)
#define HI_ISP_BAYERNR_MAXLMT (11)
#define HI_ISP_NR_LUT_LEN     (16)

/*For Bayernr*/
#define HI_ISP_BAYERNR_EN (1)
#define HI_ISP_BAYERNR_COARSE_STRENGTH_DIVISOR (128)
#define HI_ISP_BNR_LSC_DEFAULT_WIDTH_OFFSET    (0)
#define HI_ISP_BNR_LSC_DEFAULT_WEIGHT          (256)
#define HI_ISP_BNR_LSC_DEFAULT_LIGHT0_INDEX    (0)
#define HI_ISP_BNR_LSC_DEFAULT_LIGHT1_INDEX    (1)

#define HI_BCOM_ALPHA (2)
#define HI_BDEC_ALPHA (4)
#define HI_WDR_EINIT_BLCNR  (64)

#ifndef HI_BAYERNR_MAX
#define HI_BAYERNR_MAX(a, b) (((a) < (b)) ?  (b) : (a))
#endif

#ifndef HI_BAYERNR_MIN
#define HI_BAYERNR_MIN(a, b) (((a) > (b)) ?  (b) : (a))
#endif
#define HI_BAYERNR_CLIP3(low, high, x) (HI_BAYERNR_MAX(HI_BAYERNR_MIN((x), high), low))

static const  HI_U32 g_au32BayernrIsoLut[ISP_AUTO_ISO_STRENGTH_NUM] = {100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600, 51200, 102400, 204800, 409600, 819200, 1638400, 3276800};

static const  HI_U16 g_au16LutCoringRatio[HI_ISP_BAYERNR_LUT_LENGTH] = {60, 60, 60, 60, 65, 65, 65, 65, 70, 70, 70, 70, 70, 70, 70, 70, 80, 80, 80, 85, 85, 85, 90, 90, 90, 95, 95, 95, 100, 100, 100, 100, 100};
static const  HI_U8  g_au8LutFineStr[ISP_AUTO_ISO_STRENGTH_NUM] = {70, 70, 70, 50, 48, 37, 28, 24, 20, 20, 20, 16, 16, 16, 16, 16};
static const  HI_U8  g_au8ChromaStr[BAYER_PATT_NUM][ISP_AUTO_ISO_STRENGTH_NUM] = {{1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3},
    {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3}
};
static const  HI_U16 g_au16LutCoringWgt[ISP_AUTO_ISO_STRENGTH_NUM] = {30, 35, 40, 80, 100, 140, 200, 240, 280, 280, 300, 400, 400, 400, 400, 400};
static const  HI_U16 g_au16CoarseStr[BAYER_PATT_NUM][ISP_AUTO_ISO_STRENGTH_NUM] = {{120, 120, 120, 120, 120, 120, 120, 140, 160, 160, 180, 200, 200, 200, 200, 200},
    {110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110},
    {110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110},
    {120, 120, 120, 120, 120, 120, 120, 140, 160, 160, 180, 200, 200, 200, 200, 200}
};
static const  HI_U8  g_au8WDRFrameStr[WDR_MAX_FRAME_NUM] = {0, 0, 0, 0};

typedef struct hiISP_NR_AUTO_S
{
    HI_U8   au8ChromaStr[BAYER_PATT_NUM][ISP_AUTO_ISO_STRENGTH_NUM]; /*RW;Range:[0x0,0x3];Format:2.0; Strength of chrmoa noise reduction for R/Gr/Gb/B channel*/
    HI_U8   au8FineStr[ISP_AUTO_ISO_STRENGTH_NUM];                   /*RW;Range:[0x0,0x80];Format:8.0; Strength of luma noise reduction*/
    HI_U16  au16CoarseStr[BAYER_PATT_NUM][ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U16  au16CoringWgt[ISP_AUTO_ISO_STRENGTH_NUM];                /*RW;Range:[0x0,0xC80];Format:12.0; Strength of reserving the random noise*/
} ISP_NR_AUTO_S;

typedef struct hiISP_NR_MANUAL_S
{
    HI_U8   au8ChromaStr[BAYER_PATT_NUM]; /*RW;Range:[0x0,0x3];Format:2.0;Strength of Chrmoa noise reduction for R/Gr/Gb/B channel*/
    HI_U8   u8FineStr;                    /*RW;Range:[0x0,0x80];Format:8.0;Strength of Luma noise reduction*/
    HI_U16  au16CoarseStr[BAYER_PATT_NUM];
    HI_U16  u16CoringWgt;                 /*RW;Range:[0x0,0xc80];Format:12.0;Strength of reserving the random noise*/
} ISP_NR_MANUAL_S;

typedef struct hiIISP_NR_WDR_S
{
    HI_U8    au8WDRFrameStr[WDR_MAX_FRAME];  /*RW;Range:[0x0,0x50];Format:7.0; Strength of each frame in wdr mode*/
} ISP_NR_WDR_S;

typedef struct hiISP_BAYERNR_S
{
    HI_BOOL  bInit;
    HI_BOOL  bEnable;
    HI_BOOL  bLowPowerEnable;
    HI_BOOL  bNrLscEnable;
    HI_BOOL  bBnrMonoSensorEn;
    HI_BOOL  bTriSadEn;                         //u1.0,
    HI_BOOL  bLutUpdate;
    HI_BOOL  bSkipEnable;                       //u1.0
    HI_BOOL  bSkipLevel1Enable;                 //u1.0
    HI_BOOL  bSkipLevel2Enable;                 //u1.0
    HI_BOOL  bSkipLevel3Enable;                 //u1.0
    HI_BOOL  bSkipLevel4Enable;                 //u1.0

    HI_U8    u8NrLscRatio;
    HI_U8    u8WdrFramesMerge;
    HI_U8    u8FineStr;
    HI_U16   u16WDRBlcThr;
    HI_U16   u16CoringLow;

    HI_U8    au8JnlmLimitLut[HI_ISP_BAYERNR_LMTLUTNUM]; //u8.0
    HI_U8    au8LutChromaRatio[BAYER_PATT_NUM][ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U8    au8LutWDRChromaRatio[BAYER_PATT_NUM][ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U8    au8LutAmedMode[BAYER_PATT_NUM];          //u1.0
    HI_U16   au16WDRFrameThr[WDR_MAX_FRAME + 2];
    HI_U16   au16LutCoringLow[ISP_AUTO_ISO_STRENGTH_NUM];           //u14.0
    HI_U16   au16LutCoringHig[ISP_AUTO_ISO_STRENGTH_NUM];           //u14.0
    HI_U16   au16LutCoringRatio[HI_ISP_BAYERNR_LUT_LENGTH];
    HI_U16   au16LutB[ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U16   au16CoarseStr[BAYER_PATT_NUM];
    HI_U32   au32JnlmLimitMultGain[2][BAYER_PATT_NUM];  //u25.0
    HI_FLOAT afExpoValues[WDR_MAX_FRAME];
    HI_U32   au32ExpoValues[WDR_MAX_FRAME];
    HI_U32   au32LutCoringHig[ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U32   au32LutCoringRatio[HI_ISP_BAYERNR_LUT_LENGTH];

    ISP_OP_TYPE_E enOpType;
    ISP_NR_AUTO_S stAuto;
    ISP_NR_MANUAL_S stManual;
    ISP_NR_WDR_S  stWDR;
} ISP_BAYERNR_S;

ISP_BAYERNR_S g_astBayernrCtx[ISP_MAX_PIPE_NUM] = {{0}};
#define BAYERNR_GET_CTX(dev, pstCtx)   pstCtx = &g_astBayernrCtx[dev]

static HI_VOID  NrInitFw(VI_PIPE ViPipe)
{
    ISP_BAYERNR_S *pstBayernr = HI_NULL;

    HI_U8   au8LutChromaRatio[BAYER_PATT_NUM][HI_ISP_NR_LUT_LEN] = {{0, 0, 1, 2, 4, 6, 8, 9, 10, 15, 18, 20, 25, 28, 30, 32}, //ChromaRatioR
        {0, 0, 0, 0, 1, 4, 6, 8, 10, 15, 18, 20, 25, 28, 30, 32}, //ChromaRatioGr
        {0, 0, 0, 0, 1, 4, 6, 8, 10, 15, 18, 20, 25, 28, 30, 32}, //ChromaRatioGb
        {0, 0, 1, 2, 4, 6, 8, 9, 10, 15, 18, 20, 25, 28, 30, 32}
    };//ChromaRatioB
    HI_U8   au8LutWDRChromaRatio[BAYER_PATT_NUM][HI_ISP_NR_LUT_LEN] = {{0, 0, 0, 2, 4, 6, 8, 9, 10, 10, 10, 10, 10, 10, 10, 10}, //ChromaRatioR
        {0, 0, 0, 2, 4, 6, 8, 9, 10, 10, 10, 10, 10, 10, 10, 10}, //ChromaRatioGr
        {0, 0, 0, 2, 4, 6, 8, 9, 10, 10, 10, 10, 10, 10, 10, 10}, //ChromaRatioGb
        {0, 0, 0, 2, 4, 6, 8, 9, 10, 10, 10, 10, 10, 10, 10, 10}
    };//ChromaRatioB
    HI_U16  au16LutCoringHig[HI_ISP_NR_LUT_LEN] = {3200, 3200, 3200, 3200, 3200, 3200, 3200, 3200, 3200, 3200, 3200, 3200, 3200, 3200, 3200, 3200};

    BAYERNR_GET_CTX(ViPipe, pstBayernr);

    memcpy(pstBayernr->au8LutChromaRatio,    au8LutChromaRatio,    sizeof(HI_U8)*HI_ISP_NR_LUT_LEN * BAYER_PATT_NUM);
    memcpy(pstBayernr->au8LutWDRChromaRatio, au8LutWDRChromaRatio, sizeof(HI_U8)*HI_ISP_NR_LUT_LEN * BAYER_PATT_NUM);
    memcpy(pstBayernr->au16LutCoringHig,     au16LutCoringHig,     sizeof(HI_U16)*HI_ISP_NR_LUT_LEN);

    return;
}

static HI_VOID BayernrExtRegsInitialize(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_BAYERNR_S *pstBayernr = HI_NULL;
    ISP_CMOS_DEFAULT_S *pstSnsDft = HI_NULL;
    BAYERNR_GET_CTX(ViPipe, pstBayernr);
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);

    NrInitFw(ViPipe);

    hi_ext_system_bayernr_manual_mode_write(ViPipe, HI_EXT_SYSTEM_BAYERNR_MANU_MODE_DEFAULT);
    hi_ext_system_bayernr_enable_write(ViPipe, HI_EXT_SYSTEM_BAYERNR_ENABLE_DEFAULT);
    hi_ext_system_bayernr_low_power_enable_write(ViPipe, HI_EXT_SYSTEM_BAYERNR_LOW_POWER_ENABLE_DEFAULT);
    hi_ext_system_bayernr_lsc_enable_write(ViPipe, HI_EXT_SYSTEM_BAYERNR_LSC_ENABLE_DEFAULT);
    hi_ext_system_bayernr_lsc_nr_ratio_write(ViPipe, HI_EXT_SYSTEM_BAYERNR_LSC_NR_RATIO_DEFAULT);
    hi_ext_system_bayernr_mono_sensor_write(ViPipe, HI_EXT_SYSTEM_BAYERNR_MONO_SENSOR_ENABLE_DEFAULT);

    for (i = 0; i < HI_ISP_BAYERNR_LUT_LENGTH; i++)
    {
        pstBayernr->au16LutCoringRatio[i] = g_au16LutCoringRatio[i];
        hi_ext_system_bayernr_coring_ratio_write(ViPipe, i, pstBayernr->au16LutCoringRatio[i]);
    }


    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        pstBayernr->stAuto.au8FineStr[i]       = g_au8LutFineStr[i];
        pstBayernr->stAuto.au8ChromaStr[0][i]  = g_au8ChromaStr[0][i];
        pstBayernr->stAuto.au8ChromaStr[1][i]  = g_au8ChromaStr[1][i];
        pstBayernr->stAuto.au8ChromaStr[2][i]  = g_au8ChromaStr[2][i];
        pstBayernr->stAuto.au8ChromaStr[3][i]  = g_au8ChromaStr[3][i];
        pstBayernr->stAuto.au16CoarseStr[0][i] = g_au16CoarseStr[0][i];
        pstBayernr->stAuto.au16CoarseStr[1][i] = g_au16CoarseStr[1][i];
        pstBayernr->stAuto.au16CoarseStr[2][i] = g_au16CoarseStr[2][i];
        pstBayernr->stAuto.au16CoarseStr[3][i] = g_au16CoarseStr[3][i];
        pstBayernr->stAuto.au16CoringWgt[i]    = g_au16LutCoringWgt[i];

        hi_ext_system_bayernr_auto_fine_strength_write(ViPipe, i, pstBayernr->stAuto.au8FineStr[i]);
        hi_ext_system_bayernr_auto_chroma_strength_r_write(ViPipe, i, pstBayernr->stAuto.au8ChromaStr[0][i]);    //ChromaStrR
        hi_ext_system_bayernr_auto_chroma_strength_gr_write(ViPipe, i, pstBayernr->stAuto.au8ChromaStr[1][i]);  //ChromaStrGr
        hi_ext_system_bayernr_auto_chroma_strength_gb_write(ViPipe, i, pstBayernr->stAuto.au8ChromaStr[2][i]);  //ChromaStrGb
        hi_ext_system_bayernr_auto_chroma_strength_b_write(ViPipe, i, pstBayernr->stAuto.au8ChromaStr[3][i]);    //ChromaStrB
        hi_ext_system_bayernr_auto_coarse_strength_r_write(ViPipe, i, pstBayernr->stAuto.au16CoarseStr[0][i]);
        hi_ext_system_bayernr_auto_coarse_strength_gr_write(ViPipe, i, pstBayernr->stAuto.au16CoarseStr[1][i]);
        hi_ext_system_bayernr_auto_coarse_strength_gb_write(ViPipe, i, pstBayernr->stAuto.au16CoarseStr[2][i]);
        hi_ext_system_bayernr_auto_coarse_strength_b_write(ViPipe, i, pstBayernr->stAuto.au16CoarseStr[3][i]);
        hi_ext_system_bayernr_auto_coring_weight_write(ViPipe, i, pstBayernr->stAuto.au16CoringWgt[i]);


    }
    for (i = 0; i < WDR_MAX_FRAME_NUM; i++)
    {
        pstBayernr->stWDR.au8WDRFrameStr[i] = g_au8WDRFrameStr[i];
        hi_ext_system_bayernr_wdr_frame_strength_write(ViPipe, i, pstBayernr->stWDR.au8WDRFrameStr[i]);
    }

    //Manual
    pstBayernr->stManual.u8FineStr        = HI_EXT_SYSTEM_BAYERNR_MANU_FINE_STRENGTH_DEFAULT;
    pstBayernr->stManual.au8ChromaStr[0]  = HI_EXT_SYSTEM_BAYERNR_MANU_CHROMA_STRENGTH_DEFAULT;
    pstBayernr->stManual.au8ChromaStr[1]  = HI_EXT_SYSTEM_BAYERNR_MANU_CHROMA_STRENGTH_DEFAULT;
    pstBayernr->stManual.au8ChromaStr[2]  = HI_EXT_SYSTEM_BAYERNR_MANU_CHROMA_STRENGTH_DEFAULT;
    pstBayernr->stManual.au8ChromaStr[3]  = HI_EXT_SYSTEM_BAYERNR_MANU_CHROMA_STRENGTH_DEFAULT;
    pstBayernr->stManual.au16CoarseStr[0] = HI_EXT_SYSTEM_BAYERNR_MANU_COARSE_STRENGTH_DEFAULT;
    pstBayernr->stManual.au16CoarseStr[1] = HI_EXT_SYSTEM_BAYERNR_MANU_COARSE_STRENGTH_DEFAULT;
    pstBayernr->stManual.au16CoarseStr[2] = HI_EXT_SYSTEM_BAYERNR_MANU_COARSE_STRENGTH_DEFAULT;
    pstBayernr->stManual.au16CoarseStr[3] = HI_EXT_SYSTEM_BAYERNR_MANU_COARSE_STRENGTH_DEFAULT;
    pstBayernr->stManual.u16CoringWgt     = HI_EXT_SYSTEM_BAYERNR_MANU_CORING_WEIGHT_DEFAULT;

    hi_ext_system_bayernr_manual_fine_strength_write(ViPipe, pstBayernr->stManual.u8FineStr);
    hi_ext_system_bayernr_manual_chroma_strength_write(ViPipe, 0, pstBayernr->stManual.au8ChromaStr[0]);
    hi_ext_system_bayernr_manual_chroma_strength_write(ViPipe, 1, pstBayernr->stManual.au8ChromaStr[1]);
    hi_ext_system_bayernr_manual_chroma_strength_write(ViPipe, 2, pstBayernr->stManual.au8ChromaStr[2]);
    hi_ext_system_bayernr_manual_chroma_strength_write(ViPipe, 3, pstBayernr->stManual.au8ChromaStr[3]);
    hi_ext_system_bayernr_manual_coarse_strength_write(ViPipe, 0, pstBayernr->stManual.au16CoarseStr[0]);
    hi_ext_system_bayernr_manual_coarse_strength_write(ViPipe, 1, pstBayernr->stManual.au16CoarseStr[1]);
    hi_ext_system_bayernr_manual_coarse_strength_write(ViPipe, 2, pstBayernr->stManual.au16CoarseStr[2]);
    hi_ext_system_bayernr_manual_coarse_strength_write(ViPipe, 3, pstBayernr->stManual.au16CoarseStr[3]);
    hi_ext_system_bayernr_manual_coring_weight_write(ViPipe, pstBayernr->stManual.u16CoringWgt);

    if (pstSnsDft->stBayerNr.bValid)
    {
        hi_ext_system_bayernr_enable_write(ViPipe, pstSnsDft->stBayerNr.bEnable);
        hi_ext_system_bayernr_low_power_enable_write(ViPipe, pstSnsDft->stBayerNr.bLowPowerEnable);
        hi_ext_system_bayernr_lsc_enable_write(ViPipe, pstSnsDft->stBayerNr.bNrLscEnable);
        hi_ext_system_bayernr_lsc_nr_ratio_write(ViPipe, pstSnsDft->stBayerNr.u8NrLscRatio);
        hi_ext_system_bayernr_mono_sensor_write(ViPipe, pstSnsDft->stBayerNr.bBnrMonoSensorEn);

        for (i = 0; i < HI_ISP_BAYERNR_LUT_LENGTH; i++)
        {
            pstBayernr->au16LutCoringRatio[i] = pstSnsDft->stBayerNr.au16LutCoringRatio[i];
            hi_ext_system_bayernr_coring_ratio_write(ViPipe, i, pstBayernr->au16LutCoringRatio[i]);
        }

        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)   //Auto
        {
            pstBayernr->stAuto.au8FineStr[i]       = pstSnsDft->stBayerNr.au8LutFineStr[i];
            pstBayernr->stAuto.au8ChromaStr[0][i]  = pstSnsDft->stBayerNr.au8ChromaStr[0][i];
            pstBayernr->stAuto.au8ChromaStr[1][i]  = pstSnsDft->stBayerNr.au8ChromaStr[1][i];
            pstBayernr->stAuto.au8ChromaStr[2][i]  = pstSnsDft->stBayerNr.au8ChromaStr[2][i];
            pstBayernr->stAuto.au8ChromaStr[3][i]  = pstSnsDft->stBayerNr.au8ChromaStr[3][i];
            pstBayernr->stAuto.au16CoarseStr[0][i] = pstSnsDft->stBayerNr.au16CoarseStr[0][i];
            pstBayernr->stAuto.au16CoarseStr[1][i] = pstSnsDft->stBayerNr.au16CoarseStr[1][i];
            pstBayernr->stAuto.au16CoarseStr[2][i] = pstSnsDft->stBayerNr.au16CoarseStr[2][i];
            pstBayernr->stAuto.au16CoarseStr[3][i] = pstSnsDft->stBayerNr.au16CoarseStr[3][i];
            pstBayernr->stAuto.au16CoringWgt[i]    = pstSnsDft->stBayerNr.au16LutCoringWgt[i];

            hi_ext_system_bayernr_auto_fine_strength_write(ViPipe, i, pstBayernr->stAuto.au8FineStr[i]);
            hi_ext_system_bayernr_auto_chroma_strength_r_write(ViPipe, i, pstBayernr->stAuto.au8ChromaStr[0][i]);   //ChromaStrR
            hi_ext_system_bayernr_auto_chroma_strength_gr_write(ViPipe, i, pstBayernr->stAuto.au8ChromaStr[1][i]);  //ChromaStrGr
            hi_ext_system_bayernr_auto_chroma_strength_gb_write(ViPipe, i, pstBayernr->stAuto.au8ChromaStr[2][i]);  //ChromaStrGb
            hi_ext_system_bayernr_auto_chroma_strength_b_write(ViPipe, i, pstBayernr->stAuto.au8ChromaStr[3][i]);   //ChromaStrB
            hi_ext_system_bayernr_auto_coarse_strength_r_write(ViPipe, i, pstBayernr->stAuto.au16CoarseStr[0][i]);
            hi_ext_system_bayernr_auto_coarse_strength_gr_write(ViPipe, i, pstBayernr->stAuto.au16CoarseStr[1][i]);
            hi_ext_system_bayernr_auto_coarse_strength_gb_write(ViPipe, i, pstBayernr->stAuto.au16CoarseStr[2][i]);
            hi_ext_system_bayernr_auto_coarse_strength_b_write(ViPipe, i, pstBayernr->stAuto.au16CoarseStr[3][i]);
            hi_ext_system_bayernr_auto_coring_weight_write(ViPipe, i, pstBayernr->stAuto.au16CoringWgt[i]);
        }

        for (i = 0; i < WDR_MAX_FRAME_NUM; i++)
        {
            pstBayernr->stWDR.au8WDRFrameStr[i] = pstSnsDft->stBayerNr.au8WDRFrameStr[i];
            hi_ext_system_bayernr_wdr_frame_strength_write(ViPipe, i, pstBayernr->stWDR.au8WDRFrameStr[i]);
        }
    }

    pstBayernr->bInit = HI_TRUE;

    return;
}

static HI_VOID BayernrStaticRegsInitialize(VI_PIPE ViPipe, ISP_BAYERNR_STATIC_CFG_S *pstBayernrStaticRegCfg, HI_U8 i)
{
    ISP_CMOS_BLACK_LEVEL_S *pstSnsBlackLevel = HI_NULL;
    ISP_SensorGetBlc(ViPipe, &pstSnsBlackLevel);

    pstBayernrStaticRegCfg->u16RLmtBlc          = pstSnsBlackLevel->au16BlackLevel[0] >> 4;
    pstBayernrStaticRegCfg->bBnrDetailEnhanceEn = HI_ISP_BNR_DEFAULT_DE_ENABLE;
    pstBayernrStaticRegCfg->bSkipEnable         = HI_ISP_BNR_DEFAULT_SKIP_ENABLE;
    pstBayernrStaticRegCfg->bSkipLevel1Enable   = HI_ISP_BNR_DEFAULT_SKIP_LEV1_ENABLE;
    pstBayernrStaticRegCfg->bSkipLevel2Enable   = HI_ISP_BNR_DEFAULT_SKIP_LEV2_ENABLE;
    pstBayernrStaticRegCfg->bSkipLevel3Enable   = HI_ISP_BNR_DEFAULT_SKIP_LEV3_ENABLE;
    pstBayernrStaticRegCfg->u8JnlmSel           = HI_ISP_BNR_DEFAULT_JNLM_SEL;
    pstBayernrStaticRegCfg->u8BnrDePosClip      = HI_ISP_BNR_DEFAULT_DE_POS_CLIP;
    pstBayernrStaticRegCfg->u8BnrDeNegClip      = HI_ISP_BNR_DEFAULT_DE_NEG_CLIP;
    pstBayernrStaticRegCfg->u8WtiSvalThr        = HI_ISP_BNR_DEFAULT_WTI_SVAL_THR;
    pstBayernrStaticRegCfg->u8WtiCoefMid        = HI_ISP_BNR_DEFAULT_WTI_MID_COEF;
    pstBayernrStaticRegCfg->u8WtiDvalThr        = HI_ISP_BNR_DEFAULT_WTI_DVAL_THR;
    pstBayernrStaticRegCfg->s16WtiDenomOffset   = HI_ISP_BNR_DEFAULT_WTI_DENOM_OFFSET;
    pstBayernrStaticRegCfg->u16WtiCoefMax       = HI_ISP_BNR_DEFAULT_WTI_MAX_COEF;
    pstBayernrStaticRegCfg->u16JnlmMaxWtCoef    = HI_ISP_BNR_DEFAULT_JNLM_MAX_WT_COEF;
    pstBayernrStaticRegCfg->u16BnrDeBlcValue    = HI_ISP_BNR_DEFAULT_DE_BLC_VALUE;

    pstBayernrStaticRegCfg->bResh = HI_TRUE;

    return;
}

static HI_VOID BayernrDynaRegsInitialize(VI_PIPE ViPipe, ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaRegCfg, ISP_CTX_S *pstIspCtx)
{
    HI_U8  u8WDRMode;
    HI_U16 j;

    u8WDRMode = pstIspCtx->u8SnsWDRMode;

    ISP_BAYERNR_S *pstBayernr = HI_NULL;
    BAYERNR_GET_CTX(ViPipe, pstBayernr);

    pstBayernrDynaRegCfg->bMedcEnable           = HI_TRUE;
    pstBayernrDynaRegCfg->bTriSadEn             = HI_ISP_BNR_DEFAULT_TRISAD_ENABLE;
    pstBayernrDynaRegCfg->bSkipLevel4Enable     = HI_ISP_BNR_DEFAULT_SKIP_LEV4_ENABLE;
    pstBayernrDynaRegCfg->au8BnrCRatio[0]       = HI_ISP_BNR_DEFAULT_C_RATIO_R;
    pstBayernrDynaRegCfg->au8BnrCRatio[1]       = HI_ISP_BNR_DEFAULT_C_RATIO_GR;
    pstBayernrDynaRegCfg->au8BnrCRatio[2]       = HI_ISP_BNR_DEFAULT_C_RATIO_GB;
    pstBayernrDynaRegCfg->au8BnrCRatio[3]       = HI_ISP_BNR_DEFAULT_C_RATIO_B;
    pstBayernrDynaRegCfg->au8AmedMode[0]        = HI_ISP_BNR_DEFAULT_AMED_MODE_R;
    pstBayernrDynaRegCfg->au8AmedMode[1]        = HI_ISP_BNR_DEFAULT_AMED_MODE_GR;
    pstBayernrDynaRegCfg->au8AmedMode[2]        = HI_ISP_BNR_DEFAULT_AMED_MODE_GB;
    pstBayernrDynaRegCfg->au8AmedMode[3]        = HI_ISP_BNR_DEFAULT_AMED_MODE_B;
    pstBayernrDynaRegCfg->au8AmedLevel[0]       = HI_ISP_BNR_DEFAULT_AMED_LEVEL_R;
    pstBayernrDynaRegCfg->au8AmedLevel[1]       = HI_ISP_BNR_DEFAULT_AMED_LEVEL_GR;
    pstBayernrDynaRegCfg->au8AmedLevel[2]       = HI_ISP_BNR_DEFAULT_AMED_LEVEL_GB;
    pstBayernrDynaRegCfg->au8AmedLevel[3]       = HI_ISP_BNR_DEFAULT_AMED_LEVEL_B;
    pstBayernrDynaRegCfg->u16LmtNpThresh        = HI_ISP_BNR_DEFAULT_NP_THRESH;
    pstBayernrDynaRegCfg->u8JnlmGain            = HI_ISP_BNR_DEFAULT_JNLM_GAIN;
    pstBayernrDynaRegCfg->u16JnlmCoringHig      = HI_ISP_BNR_DEFAULT_JNLM_CORING_HIGH;
    pstBayernrDynaRegCfg->u16RLmtRgain          = HI_ISP_BNR_DEFAULT_RLMT_RGAIN;
    pstBayernrDynaRegCfg->u16RLmtBgain          = HI_ISP_BNR_DEFAULT_RLMT_BGAIN;

    for (j = 0; j < 129; j++)
    {
        pstBayernrDynaRegCfg->au8JnlmLimitLut[j] = 0;
    }
    for (j = 0; j < HI_ISP_BAYERNR_LUT_LENGTH; j++)
    {
        pstBayernrDynaRegCfg->au16JnlmCoringLowLUT[j] = 0;
    }

    for (j = 0; j < BAYER_PATT_NUM; j++)
    {
        pstBayernrDynaRegCfg->au32JnlmLimitMultGain[0][j] = 0;
        pstBayernrDynaRegCfg->au32JnlmLimitMultGain[1][j] = 0;
    }
    pstBayernrDynaRegCfg->bBnrLutUpdateEn = HI_TRUE;

    if (IS_2to1_WDR_MODE(u8WDRMode))
    {
        pstBayernr->u8WdrFramesMerge = 2;
        pstBayernrDynaRegCfg->u8JnlmSymCoef = HI_ISP_BNR_DEFAULT_JNLM_SYMCOEF_WDR;
    }
    else if (IS_3to1_WDR_MODE(u8WDRMode))
    {
        pstBayernr->u8WdrFramesMerge = 3;
        pstBayernrDynaRegCfg->u8JnlmSymCoef = HI_ISP_BNR_DEFAULT_JNLM_SYMCOEF_WDR;
    }
    else if (IS_4to1_WDR_MODE(u8WDRMode))
    {
        pstBayernr->u8WdrFramesMerge = 4;
        pstBayernrDynaRegCfg->u8JnlmSymCoef = HI_ISP_BNR_DEFAULT_JNLM_SYMCOEF_WDR;
    }
    else
    {
        pstBayernr->u8WdrFramesMerge = 1;
        pstBayernrDynaRegCfg->u8JnlmSymCoef = HI_ISP_BNR_DEFAULT_JNLM_SYMCOEF_LINEAR;
    }

    pstBayernrDynaRegCfg->bResh = HI_TRUE;

    return;
}

static HI_VOID BayernrUsrRegsInitialize(ISP_BAYERNR_USR_CFG_S *pstBayernrUsrRegCfg, ISP_CMOS_DEFAULT_S *pstSnsDft)
{
    pstBayernrUsrRegCfg->bBnrLscEn         = pstSnsDft->stBayerNr.bNrLscEnable;
    pstBayernrUsrRegCfg->bBnrMonoSensorEn  = pstSnsDft->stBayerNr.bBnrMonoSensorEn;
    pstBayernrUsrRegCfg->u8BnrLscRatio     = pstSnsDft->stBayerNr.u8NrLscRatio;
    pstBayernrUsrRegCfg->bResh             = HI_TRUE;

    return;
}

static HI_VOID BayernrRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8  i;
    HI_U8  u8BlockNum;
    ISP_CTX_S   *pstIspCtx = HI_NULL;
    ISP_CMOS_DEFAULT_S     *pstSnsDft        = HI_NULL;
    ISP_CMOS_BLACK_LEVEL_S *pstSnsBlackLevel = HI_NULL;
    ISP_BAYERNR_STATIC_CFG_S *pstBayernrStaticRegCfg = HI_NULL;
    ISP_BAYERNR_DYNA_CFG_S   *pstBayernrDynaRegCfg   = HI_NULL;
    ISP_BAYERNR_USR_CFG_S    *pstBayernrUsrRegCfg    = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);
    ISP_SensorGetBlc(ViPipe, &pstSnsBlackLevel);

    u8BlockNum = pstIspCtx->stBlockAttr.u8BlockNum;

    for (i = 0; i < u8BlockNum; i++)
    {

        pstBayernrStaticRegCfg = &pstRegCfg->stAlgRegCfg[i].stBnrRegCfg.stStaticRegCfg;
        pstBayernrDynaRegCfg   = &pstRegCfg->stAlgRegCfg[i].stBnrRegCfg.stDynaRegCfg;
        pstBayernrUsrRegCfg    = &pstRegCfg->stAlgRegCfg[i].stBnrRegCfg.stUsrRegCfg;

        pstRegCfg->stAlgRegCfg[i].stBnrRegCfg.bBnrEnable = HI_TRUE;
        BayernrStaticRegsInitialize(ViPipe, pstBayernrStaticRegCfg, i);
        BayernrDynaRegsInitialize(ViPipe, pstBayernrDynaRegCfg, pstIspCtx);
        BayernrUsrRegsInitialize(pstBayernrUsrRegCfg, pstSnsDft);
    }

    pstRegCfg->unKey.bit1BayernrCfg = 1;

    return;
}

static HI_S32 BayernrReadExtregs(VI_PIPE ViPipe)
{
    HI_U8 i;
    HI_U16 u16BlackLevel;
    HI_U32 au32ExpRatio[3] = {0};
    ISP_BAYERNR_S *pstBayernr = HI_NULL;
    ISP_CTX_S     *pstIspCtx  = HI_NULL;
    BAYERNR_GET_CTX(ViPipe, pstBayernr);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstBayernr->enOpType           = hi_ext_system_bayernr_manual_mode_read(ViPipe);
    pstBayernr->bLowPowerEnable    = hi_ext_system_bayernr_low_power_enable_read(ViPipe);
    pstBayernr->bNrLscEnable       = hi_ext_system_bayernr_lsc_enable_read(ViPipe);
    pstBayernr->bBnrMonoSensorEn   = hi_ext_system_bayernr_mono_sensor_read(ViPipe);
    pstBayernr->u8NrLscRatio       = hi_ext_system_bayernr_lsc_nr_ratio_read(ViPipe);
    pstBayernr->au16WDRFrameThr[0] = hi_ext_system_wdr_longthr_read(ViPipe);
    pstBayernr->au16WDRFrameThr[1] = hi_ext_system_wdr_shortthr_read(ViPipe);

    u16BlackLevel = hi_ext_system_black_level_00_read(ViPipe);
    memcpy(au32ExpRatio, pstIspCtx->stLinkage.au32ExpRatio, sizeof(au32ExpRatio));

    switch (pstBayernr->u8WdrFramesMerge)
    {
        case 2:
            pstBayernr->au32ExpoValues[0] = 64;
            pstBayernr->au32ExpoValues[1] = (HI_U32)au32ExpRatio[0];
            pstBayernr->u16WDRBlcThr      = u16BlackLevel << 2;
            break;
        case 3:
            pstBayernr->au32ExpoValues[0] = 64;
            pstBayernr->au32ExpoValues[1] = (HI_U32)au32ExpRatio[0];
            pstBayernr->au32ExpoValues[2] = (HI_U32)(au32ExpRatio[0] * au32ExpRatio[1]);
            pstBayernr->u16WDRBlcThr      = u16BlackLevel << 2;
            break;
        case 4:
            pstBayernr->au32ExpoValues[0] = 64;
            pstBayernr->au32ExpoValues[1] = (HI_U32)au32ExpRatio[0];
            pstBayernr->au32ExpoValues[2] = (HI_U32)(au32ExpRatio[0] * au32ExpRatio[1]);
            pstBayernr->au32ExpoValues[3] = (HI_U32)(au32ExpRatio[0] * au32ExpRatio[1] * au32ExpRatio[2]);
            pstBayernr->u16WDRBlcThr      = u16BlackLevel << 2;
            break;
        default:
            pstBayernr->u16WDRBlcThr      = 0;
            break;
    }

    for (i = 0; i < HI_ISP_BAYERNR_LUT_LENGTH; i++)
    {
        pstBayernr->au16LutCoringRatio[i] = hi_ext_system_bayernr_coring_ratio_read(ViPipe, i);
    }

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        pstBayernr->stAuto.au8FineStr[i]       = hi_ext_system_bayernr_auto_fine_strength_read(ViPipe, i);
        pstBayernr->stAuto.au16CoringWgt[i]    = hi_ext_system_bayernr_auto_coring_weight_read(ViPipe, i);
        pstBayernr->stAuto.au8ChromaStr[0][i]  = hi_ext_system_bayernr_auto_chroma_strength_r_read(ViPipe, i);
        pstBayernr->stAuto.au8ChromaStr[1][i]  = hi_ext_system_bayernr_auto_chroma_strength_gr_read(ViPipe, i);
        pstBayernr->stAuto.au8ChromaStr[2][i]  = hi_ext_system_bayernr_auto_chroma_strength_gb_read(ViPipe, i);
        pstBayernr->stAuto.au8ChromaStr[3][i]  = hi_ext_system_bayernr_auto_chroma_strength_b_read(ViPipe, i);
        pstBayernr->stAuto.au16CoarseStr[0][i] = hi_ext_system_bayernr_auto_coarse_strength_r_read(ViPipe, i);
        pstBayernr->stAuto.au16CoarseStr[1][i] = hi_ext_system_bayernr_auto_coarse_strength_gr_read(ViPipe, i);
        pstBayernr->stAuto.au16CoarseStr[2][i] = hi_ext_system_bayernr_auto_coarse_strength_gb_read(ViPipe, i);
        pstBayernr->stAuto.au16CoarseStr[3][i] = hi_ext_system_bayernr_auto_coarse_strength_b_read(ViPipe, i);
    }

    pstBayernr->stManual.u8FineStr        = hi_ext_system_bayernr_manual_fine_strength_read(ViPipe);
    pstBayernr->stManual.u16CoringWgt     = hi_ext_system_bayernr_manual_coring_weight_read(ViPipe);
    pstBayernr->stManual.au8ChromaStr[0]  = hi_ext_system_bayernr_manual_chroma_strength_read(ViPipe, 0);
    pstBayernr->stManual.au8ChromaStr[1]  = hi_ext_system_bayernr_manual_chroma_strength_read(ViPipe, 1);
    pstBayernr->stManual.au8ChromaStr[2]  = hi_ext_system_bayernr_manual_chroma_strength_read(ViPipe, 2);
    pstBayernr->stManual.au8ChromaStr[3]  = hi_ext_system_bayernr_manual_chroma_strength_read(ViPipe, 3);
    pstBayernr->stManual.au16CoarseStr[0] = hi_ext_system_bayernr_manual_coarse_strength_read(ViPipe, 0);
    pstBayernr->stManual.au16CoarseStr[1] = hi_ext_system_bayernr_manual_coarse_strength_read(ViPipe, 1);
    pstBayernr->stManual.au16CoarseStr[2] = hi_ext_system_bayernr_manual_coarse_strength_read(ViPipe, 2);
    pstBayernr->stManual.au16CoarseStr[3] = hi_ext_system_bayernr_manual_coarse_strength_read(ViPipe, 3);

    for (i = 0; i < WDR_MAX_FRAME; i++)
    {
        pstBayernr->stWDR.au8WDRFrameStr[i] =  hi_ext_system_bayernr_wdr_frame_strength_read(ViPipe, i);
    }

    return 0;
}

static HI_S32 BayernrReadProMode(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_BAYERNR_S *pstBayernr = HI_NULL;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    HI_U8 u8Index = 0;
    BAYERNR_GET_CTX(ViPipe, pstBayernr);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    if (HI_TRUE == pstIspCtx->stProNrParamCtrl.pstProNrParam->bEnable)
    {
        u8Index = pstIspCtx->stLinkage.u8ProIndex;
        if (u8Index < 1)
        {
            return HI_SUCCESS;
        }
        u8Index -= 1;
    }
    else
    {
        return HI_SUCCESS;
    }
    pstBayernr->enOpType = OP_TYPE_AUTO;
    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        pstBayernr->stAuto.au8FineStr[i]       = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au8FineStr[i];
        pstBayernr->stAuto.au16CoringWgt[i]    = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au16CoringWgt[i];
        pstBayernr->stAuto.au8ChromaStr[0][i]  = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au8ChromaStr[0][i];
        pstBayernr->stAuto.au8ChromaStr[1][i]  = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au8ChromaStr[1][i];
        pstBayernr->stAuto.au8ChromaStr[2][i]  = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au8ChromaStr[2][i];
        pstBayernr->stAuto.au8ChromaStr[3][i]  = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au8ChromaStr[3][i];
        pstBayernr->stAuto.au16CoarseStr[0][i] = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au16CoarseStr[0][i];
        pstBayernr->stAuto.au16CoarseStr[1][i] = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au16CoarseStr[1][i];
        pstBayernr->stAuto.au16CoarseStr[2][i] = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au16CoarseStr[2][i];
        pstBayernr->stAuto.au16CoarseStr[3][i] = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au16CoarseStr[3][i];
    }
    return 0;
}
HI_S32 BayernrProcWrite(VI_PIPE ViPipe, ISP_CTRL_PROC_WRITE_S *pstProc)
{
    ISP_CTRL_PROC_WRITE_S stProcTmp;
    ISP_BAYERNR_S *pstBayernr = HI_NULL;

    BAYERNR_GET_CTX(ViPipe, pstBayernr);

    if ((HI_NULL == pstProc->pcProcBuff)
        || (0 == pstProc->u32BuffLen))
    {
        return HI_FAILURE;
    }

    stProcTmp.pcProcBuff = pstProc->pcProcBuff;
    stProcTmp.u32BuffLen = pstProc->u32BuffLen;

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "-----BAYERNR INFO----------------------------------------------------------------------------\n");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s"      "%16s"         "%16s"        "%16s"      "%16s"       "%16s"        "%16s\n",
                    "Enable",  "NrLscEnable", "NrLscRatio",  "CoarseStr0", "CoarseStr1", "CoarseStr2", "CoarseStr3" );

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16u"   "%16u"   "%16u"   "%16u"   "%16u"  "%16u"   "%16u\n",
                    pstBayernr->bEnable,
                    (HI_U16)pstBayernr->bNrLscEnable,
                    (HI_U16)pstBayernr->u8NrLscRatio,
                    (HI_U16)pstBayernr->au16CoarseStr[0],
                    (HI_U16)pstBayernr->au16CoarseStr[1],
                    (HI_U16)pstBayernr->au16CoarseStr[2],
                    (HI_U16)pstBayernr->au16CoarseStr[3]
                   );

    pstProc->u32WriteLen += 1;

    return HI_SUCCESS;
}


HI_S32 ISP_BayernrInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;
    BayernrRegsInitialize(ViPipe, pstRegCfg);
    BayernrExtRegsInitialize(ViPipe);
    return HI_SUCCESS;
}

HI_U32 NRGetValueFromLut (HI_U32 u32IsoLevel , HI_S32 s32Y2, HI_S32 s32Y1, HI_S32 s32X2, HI_S32 s32X1, HI_S32 s32Iso)
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

HI_U16 NRGetValueFromLut_fix(HI_U32 x, HI_U32 *pLutX, HI_U16 *pLutY, HI_U32 length)
{
    HI_S32 j;

    if (x <= pLutX[0])
    {
        return pLutY[0];
    }
    for (j = 1; j < length; j++)
    {
        if (x <= pLutX[j])
        {
            if (pLutY[j] < pLutY[j - 1])
            {
                return (HI_U16)(pLutY[j - 1] - (pLutY[j - 1] - pLutY[j]) * (HI_U16)(x - pLutX[j - 1]) / DIV_0_TO_1((HI_U16)(pLutX[j] - pLutX[j - 1])));
            }
            else
            {
                return (HI_U16)(pLutY[j - 1] + (pLutY[j] - pLutY[j - 1]) * (HI_U16)(x - pLutX[j - 1]) / DIV_0_TO_1((HI_U16)(pLutX[j] - pLutX[j - 1])));
            }
        }
    }
    return pLutY[length - 1];
}

#define  EPS (0.000001f)
#define  COL_ISO      0
#define  COL_K        1
#define  COL_B        2

static HI_FLOAT Bayernr_getKfromNoiseLut(HI_FLOAT (*pRecord)[3], HI_U16 recordNum, HI_S32 iso)
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

static HI_FLOAT Bayernr_getBfromNoiseLut(HI_FLOAT (*pRecord)[3], HI_U16 recordNum, HI_S32 iso)
{
    HI_S32 i = 0;
    HI_FLOAT  y_diff = 0, x_diff = 0;
    HI_FLOAT b = 0.0f;

    // record: iso - b
    if (iso <= pRecord[0][COL_ISO])
    {
        b = pRecord[0][COL_B];
        //return b;
    }

    if (iso >= pRecord[recordNum - 1][COL_ISO])
    {
        b = pRecord[recordNum - 1][COL_B];
        //return b;
    }

    for (i = 0; i < recordNum - 1; i++)
    {
        if (iso >= pRecord[i][COL_ISO] && iso <= pRecord[i + 1][COL_ISO])
        {
            x_diff = pRecord[i + 1][COL_ISO] - pRecord[i][COL_ISO];  // iso diff
            y_diff = pRecord[i + 1][COL_B]  - pRecord[i][COL_B];     // k diff
            b = pRecord[i][COL_B] + y_diff * (iso - pRecord[i][COL_ISO]) / DIV_0_TO_1(x_diff + EPS);
            //return b;
        }
    }
    return b;
}


static __inline HI_U16 BayernrOffsetCalculate(
    const HI_U16 u16Y2,
    const HI_U16 u16Y1,
    const HI_U32 u32X2,
    const HI_U32 u32X1,
    const HI_U32 u32Iso)
{
    HI_U32 u32Offset;
    if (u32X1 == u32X2)
    {
        u32Offset = u16Y2;
    }
    else if (u16Y1 <= u16Y2)
    {
        u32Offset = u16Y1 + (ABS(u16Y2 - u16Y1) * (u32Iso - u32X1) + (u32X2 - u32X1) / 2) / (u32X2 - u32X1);
    }
    else if (u16Y1 > u16Y2)
    {
        u32Offset = u16Y1 - (ABS(u16Y2 - u16Y1) * (u32Iso - u32X1) + (u32X2 - u32X1) / 2) / (u32X2 - u32X1);
    }

    return (HI_U16)u32Offset;
}

static HI_U32 NRGetIsoIndex(HI_U32 u32Iso)
{
    HI_U32 u32Index;

    for (u32Index = 1; u32Index < HI_ISP_NR_LUT_LEN - 1; u32Index++)
    {
        if (u32Iso <= g_au32BayernrIsoLut[u32Index])
        {
            break;
        }
    }
    return u32Index;
}

HI_S32 NRCfg(ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg, ISP_BAYERNR_S *pstBayernr, HI_U32 u32IsoLevel, HI_U32 u32ISO2, HI_U32 u32ISO1, HI_U32 u32Iso)
{
    HI_U8  u8MaxCRaio, u8MaxCRaio01, u8MaxCRaio23;
    HI_S32 s32Y1, s32Y2;

    if (pstBayernr->u8WdrFramesMerge > 1)
    {
        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->au8LutWDRChromaRatio[BAYER_RGGB][u32IsoLevel - 1] : (HI_S32)pstBayernr->au8LutWDRChromaRatio[BAYER_RGGB][0];
        s32Y2 =                 (HI_S32)pstBayernr->au8LutWDRChromaRatio[BAYER_RGGB][u32IsoLevel];
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_RGGB]  = (HI_U8)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->au8LutWDRChromaRatio[BAYER_GRBG][u32IsoLevel - 1] : (HI_S32)pstBayernr->au8LutWDRChromaRatio[BAYER_GRBG][0];
        s32Y2 =                 (HI_S32)pstBayernr->au8LutWDRChromaRatio[BAYER_GRBG][u32IsoLevel];
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_GRBG]  = (HI_U8)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->au8LutWDRChromaRatio[BAYER_GBRG][u32IsoLevel - 1] : (HI_S32)pstBayernr->au8LutWDRChromaRatio[BAYER_GBRG][0];
        s32Y2 =                 (HI_S32)pstBayernr->au8LutWDRChromaRatio[BAYER_GBRG][u32IsoLevel];
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_GBRG]  = (HI_U8)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->au8LutWDRChromaRatio[BAYER_BGGR][u32IsoLevel - 1] : (HI_S32)pstBayernr->au8LutWDRChromaRatio[BAYER_BGGR][0];
        s32Y2 =                 (HI_S32)pstBayernr->au8LutWDRChromaRatio[BAYER_BGGR][u32IsoLevel];
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_BGGR]  = (HI_U8)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);
    }
    else
    {
        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->au8LutChromaRatio[BAYER_RGGB][u32IsoLevel - 1] : (HI_S32)pstBayernr->au8LutChromaRatio[BAYER_RGGB][0];
        s32Y2 =                 (HI_S32)pstBayernr->au8LutChromaRatio[BAYER_RGGB][u32IsoLevel];
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_RGGB]  = (HI_U8)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->au8LutChromaRatio[BAYER_GRBG][u32IsoLevel - 1] : (HI_S32)pstBayernr->au8LutChromaRatio[BAYER_GRBG][0];
        s32Y2 =                 (HI_S32)pstBayernr->au8LutChromaRatio[BAYER_GRBG][u32IsoLevel];
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_GRBG]  = (HI_U8)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->au8LutChromaRatio[BAYER_GBRG][u32IsoLevel - 1] : (HI_S32)pstBayernr->au8LutChromaRatio[BAYER_GBRG][0];
        s32Y2 =                 (HI_S32)pstBayernr->au8LutChromaRatio[BAYER_GBRG][u32IsoLevel];
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_GBRG]  = (HI_U8)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->au8LutChromaRatio[BAYER_BGGR][u32IsoLevel - 1] : (HI_S32)pstBayernr->au8LutChromaRatio[BAYER_BGGR][0];
        s32Y2 =                 (HI_S32)pstBayernr->au8LutChromaRatio[BAYER_BGGR][u32IsoLevel];
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_BGGR]  = (HI_U8)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);
    }

    s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->au16LutCoringHig[u32IsoLevel - 1] : (HI_S32)pstBayernr->au16LutCoringHig[0];
    s32Y2 =                 (HI_S32)pstBayernr->au16LutCoringHig[u32IsoLevel];
    pstBayernrDynaCfg->u16JnlmCoringHig  = (HI_U16)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);
    pstBayernrDynaCfg->u16JnlmCoringHig  = (HI_U16)(256 * ((HI_FLOAT)pstBayernrDynaCfg->u16JnlmCoringHig / (HI_FLOAT)HI_ISP_BAYERNR_STRENGTH_DIVISOR));

    pstBayernrDynaCfg->au8AmedMode[BAYER_RGGB] = (u32Iso < 5000) ? 0 : 1;
    pstBayernrDynaCfg->au8AmedMode[BAYER_GRBG] = 0;
    pstBayernrDynaCfg->au8AmedMode[BAYER_GBRG] = 0;
    pstBayernrDynaCfg->au8AmedMode[BAYER_BGGR] = (u32Iso < 5000) ? 0 : 1;

    u8MaxCRaio01 = MAX2(pstBayernrDynaCfg->au8BnrCRatio[BAYER_RGGB], pstBayernrDynaCfg->au8BnrCRatio[BAYER_GRBG]);
    u8MaxCRaio23 = MAX2(pstBayernrDynaCfg->au8BnrCRatio[BAYER_GBRG], pstBayernrDynaCfg->au8BnrCRatio[BAYER_BGGR]);
    u8MaxCRaio   = MAX2(u8MaxCRaio01, u8MaxCRaio23);

    if (u8MaxCRaio <= 4)
    {
        pstBayernrDynaCfg->bMedcEnable = HI_FALSE;
    }
    else
    {
        pstBayernrDynaCfg->bMedcEnable = HI_TRUE;
    }

    if (u8MaxCRaio >= 20)
    {
        pstBayernrDynaCfg->bTriSadEn   = HI_TRUE;
    }

    return HI_SUCCESS;
}

HI_S32 NRExtCfg( VI_PIPE ViPipe,  ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg, ISP_BAYERNR_S *pstBayernr, HI_U32 u32IsoLevel, HI_U32 u32ISO2, HI_U32 u32ISO1, HI_U32 u32Iso)
{
    HI_U8  u8sSadFac = 25;
    HI_U16 u16JnlmScale = 49;
    HI_U16 u16JnlmShotScale;
    HI_U16 au16LmtStrength[4] = {0};
    HI_U32 i = 0;
    HI_S32 s32Y1, s32Y2;
    HI_U32 u32CoringLow = 1;
    HI_U16 u16ShotCoef = 2;

    if (OP_TYPE_AUTO == pstBayernr->enOpType)
    {
        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->stAuto.au16CoringWgt[u32IsoLevel - 1] : (HI_S32)pstBayernr->stAuto.au16CoringWgt[0];
        s32Y2 =                 (HI_S32)pstBayernr->stAuto.au16CoringWgt[u32IsoLevel];
        pstBayernr->u16CoringLow  = (HI_U16)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);
        u32CoringLow  = 256 * (HI_U32)pstBayernr->u16CoringLow;

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->stAuto.au8FineStr[u32IsoLevel - 1] : (HI_S32)pstBayernr->stAuto.au8FineStr[0];
        s32Y2 =                 (HI_S32)pstBayernr->stAuto.au8FineStr[u32IsoLevel];
        pstBayernrDynaCfg->u8JnlmGain  = (HI_U8)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);
        pstBayernr->u8FineStr = pstBayernrDynaCfg->u8JnlmGain;

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->stAuto.au8ChromaStr[BAYER_RGGB][u32IsoLevel - 1] : (HI_S32)pstBayernr->stAuto.au8ChromaStr[BAYER_RGGB][0];
        s32Y2 =                 (HI_S32)pstBayernr->stAuto.au8ChromaStr[BAYER_RGGB][u32IsoLevel];
        pstBayernrDynaCfg->au8AmedLevel[BAYER_RGGB]  = (HI_U8)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->stAuto.au8ChromaStr[BAYER_GRBG][u32IsoLevel - 1] : (HI_S32)pstBayernr->stAuto.au8ChromaStr[BAYER_GRBG][0];
        s32Y2 =                 (HI_S32)pstBayernr->stAuto.au8ChromaStr[BAYER_GRBG][u32IsoLevel];
        pstBayernrDynaCfg->au8AmedLevel[BAYER_GRBG]  = (HI_U8)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->stAuto.au8ChromaStr[BAYER_GBRG][u32IsoLevel - 1] : (HI_S32)pstBayernr->stAuto.au8ChromaStr[BAYER_GBRG][0];
        s32Y2 =                 (HI_S32)pstBayernr->stAuto.au8ChromaStr[BAYER_GBRG][u32IsoLevel];
        pstBayernrDynaCfg->au8AmedLevel[BAYER_GBRG]  = (HI_U8)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->stAuto.au8ChromaStr[BAYER_BGGR][u32IsoLevel - 1] : (HI_S32)pstBayernr->stAuto.au8ChromaStr[BAYER_BGGR][0];
        s32Y2 =                 (HI_S32)pstBayernr->stAuto.au8ChromaStr[BAYER_BGGR][u32IsoLevel];
        pstBayernrDynaCfg->au8AmedLevel[BAYER_BGGR]  = (HI_U8)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->stAuto.au16CoarseStr[BAYER_RGGB][u32IsoLevel - 1] : (HI_S32)pstBayernr->stAuto.au16CoarseStr[BAYER_RGGB][0];
        s32Y2 =                 (HI_S32)pstBayernr->stAuto.au16CoarseStr[BAYER_RGGB][u32IsoLevel];
        au16LmtStrength[BAYER_RGGB]  = (HI_U16)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->stAuto.au16CoarseStr[BAYER_GRBG][u32IsoLevel - 1] : (HI_S32)pstBayernr->stAuto.au16CoarseStr[BAYER_GRBG][0];
        s32Y2 =                 (HI_S32)pstBayernr->stAuto.au16CoarseStr[BAYER_GRBG][u32IsoLevel];
        au16LmtStrength[BAYER_GRBG]  = (HI_U16)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->stAuto.au16CoarseStr[BAYER_GBRG][u32IsoLevel - 1] : (HI_S32)pstBayernr->stAuto.au16CoarseStr[BAYER_GBRG][0];
        s32Y2 =                 (HI_S32)pstBayernr->stAuto.au16CoarseStr[BAYER_GBRG][u32IsoLevel];
        au16LmtStrength[BAYER_GBRG]  = (HI_U16)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);

        s32Y1 = (u32IsoLevel) ? (HI_S32)pstBayernr->stAuto.au16CoarseStr[BAYER_BGGR][u32IsoLevel - 1] : (HI_S32)pstBayernr->stAuto.au16CoarseStr[BAYER_BGGR][0];
        s32Y2 =                 (HI_S32)pstBayernr->stAuto.au16CoarseStr[BAYER_BGGR][u32IsoLevel];
        au16LmtStrength[BAYER_BGGR]  = (HI_U16)NRGetValueFromLut(u32IsoLevel, s32Y2, s32Y1, u32ISO2, u32ISO1, u32Iso);
    }
    else if (OP_TYPE_MANUAL == pstBayernr->enOpType)
    {
        pstBayernr->u16CoringLow  = pstBayernr->stManual.u16CoringWgt;
        u32CoringLow  = 256 * (HI_U32)pstBayernr->u16CoringLow;
        pstBayernrDynaCfg->u8JnlmGain                = pstBayernr->stManual.u8FineStr;
        pstBayernrDynaCfg->au8AmedLevel[BAYER_RGGB]  = pstBayernr->stManual.au8ChromaStr[BAYER_RGGB];
        pstBayernrDynaCfg->au8AmedLevel[BAYER_GRBG]  = pstBayernr->stManual.au8ChromaStr[BAYER_GRBG];
        pstBayernrDynaCfg->au8AmedLevel[BAYER_GBRG]  = pstBayernr->stManual.au8ChromaStr[BAYER_GBRG];
        pstBayernrDynaCfg->au8AmedLevel[BAYER_BGGR]  = pstBayernr->stManual.au8ChromaStr[BAYER_BGGR];

        au16LmtStrength[BAYER_RGGB] = pstBayernr->stManual.au16CoarseStr[BAYER_RGGB];
        au16LmtStrength[BAYER_GRBG] = pstBayernr->stManual.au16CoarseStr[BAYER_GRBG];
        au16LmtStrength[BAYER_GBRG] = pstBayernr->stManual.au16CoarseStr[BAYER_GBRG];
        au16LmtStrength[BAYER_BGGR] = pstBayernr->stManual.au16CoarseStr[BAYER_BGGR];
    }


    hi_ext_system_bayernr_actual_coring_weight_write(ViPipe, pstBayernr->u16CoringLow);
    hi_ext_system_bayernr_actual_fine_strength_write(ViPipe, pstBayernrDynaCfg->u8JnlmGain);
    hi_ext_system_bayernr_actual_nr_lsc_ratio_write(ViPipe, pstBayernr->u8NrLscRatio);

    for (i = 0; i < 4; i++)
    {
        hi_ext_system_bayernr_actual_coarse_strength_write(ViPipe, i, au16LmtStrength[i]);
        hi_ext_system_bayernr_actual_chroma_strength_write(ViPipe, i, pstBayernrDynaCfg->au8AmedLevel[i]);
        hi_ext_system_bayernr_actual_wdr_frame_strength_write(ViPipe, i, pstBayernr->stWDR.au8WDRFrameStr[i]);
    }

    if (HI_TRUE == pstBayernr->bLowPowerEnable)
    {
        pstBayernrDynaCfg->bSkipLevel4Enable = HI_TRUE;
    }

    for (i = 0; i < HI_ISP_BAYERNR_LUT_LENGTH; i++)
    {
        pstBayernrDynaCfg->au16JnlmCoringLowLUT[i] = (HI_U16)(pstBayernr->au16LutCoringRatio[i] * u32CoringLow / HI_ISP_BAYERNR_CORINGLOW_STRENGTH_DIVISOR);
        pstBayernrDynaCfg->au16JnlmCoringLowLUT[i] = HI_BAYERNR_MIN(16383, pstBayernrDynaCfg->au16JnlmCoringLowLUT[i]);
    }

    u16JnlmShotScale   = 128 + HI_BAYERNR_CLIP3(0, 255, (HI_S32)(u16JnlmScale * u16ShotCoef));
    u16JnlmScale       = u16JnlmScale + 128;

    for (i = 0; i < BAYER_PATT_NUM; i++)
    {
        pstBayernrDynaCfg->au32JnlmLimitMultGain[0][i] = (pstBayernrDynaCfg->u16LmtNpThresh * au16LmtStrength[i] * u8sSadFac) >> 7;
        pstBayernrDynaCfg->au32JnlmLimitMultGain[1][i] = pstBayernrDynaCfg->au32JnlmLimitMultGain[0][i];
        pstBayernrDynaCfg->au32JnlmLimitMultGain[0][i] = (pstBayernrDynaCfg->au32JnlmLimitMultGain[0][i] * u16JnlmScale) >> 7;
        pstBayernrDynaCfg->au32JnlmLimitMultGain[1][i] = (pstBayernrDynaCfg->au32JnlmLimitMultGain[1][i] * u16JnlmShotScale) >> 7;
    }

    return HI_SUCCESS;
}

HI_S32 NRLimitLut(VI_PIPE ViPipe, ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg, ISP_BAYERNR_S *pstBayernr, HI_U32 u32Iso, HI_U32 u32ISO1, HI_U32 u32ISO2, HI_U32 u32IsoLevel)
{
    HI_U16 u16BlackLevel, str;
    HI_U32 u32LmtNpThresh;
    HI_U32 i = 0, n = 0;
    HI_U16 u16BitMask = ((1 << (HI_ISP_BAYERNR_BITDEP - 1)) - 1);
    HI_U32 lutN[2] = {16, 45};
    HI_U32 k = 0, b = 0;
    HI_U32 sigma = 0, sigma_max = 0;
    HI_U16 DarkStrength = 230;   //1.8f*128
    HI_U16 lutStr[2] = {96, 128};  //{0.75f, 1.0f}*128
    HI_FLOAT fCalibrationCoef = 0.0f;

    ISP_CMOS_DEFAULT_S *pstSnsDft = HI_NULL;
    ISP_CMOS_BLACK_LEVEL_S *pstSnsBlackLevel = HI_NULL;
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);
    ISP_SensorGetBlc(ViPipe, &pstSnsBlackLevel);

    fCalibrationCoef = Bayernr_getKfromNoiseLut(pstSnsDft->stNoiseCalibration.afCalibrationCoef, pstSnsDft->stNoiseCalibration.u16CalibrationLutNum, u32Iso);
    k     = (HI_U32)(fCalibrationCoef * ISP_BITFIX(14));
    fCalibrationCoef = Bayernr_getBfromNoiseLut(pstSnsDft->stNoiseCalibration.afCalibrationCoef, pstSnsDft->stNoiseCalibration.u16CalibrationLutNum, u32Iso);
    b     = (HI_U32)(fCalibrationCoef * ISP_BITFIX(14));

    u16BlackLevel = pstSnsBlackLevel->au16BlackLevel[0] >> 4;

    sigma_max = (HI_U32)(HI_BAYERNR_MAX((HI_S32)(k * (HI_S32)(255 - u16BlackLevel) + (HI_S32)b), 0));
    sigma_max = (HI_U32)Sqrt32(sigma_max);

    u32LmtNpThresh = (HI_U32)(sigma_max * (1 << (HI_ISP_BAYERNR_BITDEP - 8 - 7))); //sad win size, move to hw
    pstBayernrDynaCfg->u16LmtNpThresh = (HI_U16)((u32LmtNpThresh > u16BitMask) ? u16BitMask : u32LmtNpThresh);

    lutStr[0] = DarkStrength;

    for (i = 0; i < HI_ISP_BAYERNR_LMTLUTNUM; i++)
    {
        sigma = (HI_U32)HI_BAYERNR_MAX(((HI_S32)(k * (i * 255 - 128 * u16BlackLevel)) / (HI_S32)128) + (HI_S32)b, 0);
        sigma = (HI_U32)Sqrt32(sigma);
        str = NRGetValueFromLut_fix(2 * i, lutN, lutStr, 2);
        sigma = sigma * str;

        pstBayernrDynaCfg->au8JnlmLimitLut[i] = (HI_U8)((sigma + sigma_max / 2) / DIV_0_TO_1(sigma_max));
    }

    //copy the first non-zero value to its left side
    for (i = 0; i < HI_ISP_BAYERNR_LMTLUTNUM; i++)
    {
        if (pstBayernrDynaCfg->au8JnlmLimitLut[i] > 0)
        {
            n = i;
            break;
        }
    }

    for (i = 0; i < n; i++)
    {
        pstBayernrDynaCfg->au8JnlmLimitLut[i] = pstBayernrDynaCfg->au8JnlmLimitLut[n];
    }

    return HI_SUCCESS;
}

HI_S32 hiisp_bayernr_fw(HI_U32 u32Iso, VI_PIPE ViPipe, HI_U8 u8CurBlk, ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg, ISP_BAYERNR_USR_CFG_S *pstBayernrUsrCfg)
{
    HI_U32 i = 0;
    HI_U32 u32IsoLevel;
    HI_U32 u32ISO1 = 0, u32ISO2 = 0;

    ISP_BAYERNR_S      *pstBayernr = HI_NULL;
    ISP_CMOS_DEFAULT_S *pstSnsDft  = HI_NULL;
    ISP_CTX_S          *pstIspCtx  = HI_NULL;

    ISP_SensorGetDefault(ViPipe, &pstSnsDft);
    BAYERNR_GET_CTX(ViPipe, pstBayernr);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstBayernrUsrCfg->bBnrMonoSensorEn    = pstBayernr->bBnrMonoSensorEn;     //MonoSensor, waiting to get
    pstBayernrUsrCfg->bBnrLscEn           = pstBayernr->bNrLscEnable;
    pstBayernrUsrCfg->u8BnrLscRatio       = pstBayernr->u8NrLscRatio;

    if (u32Iso > g_au32BayernrIsoLut[HI_ISP_NR_LUT_LEN - 1])
    {
        u32IsoLevel = HI_ISP_NR_LUT_LEN - 1;
        u32ISO1 = g_au32BayernrIsoLut[HI_ISP_NR_LUT_LEN - 1];
        u32ISO2 = g_au32BayernrIsoLut[HI_ISP_NR_LUT_LEN - 1];
    }
    else if (u32Iso <= g_au32BayernrIsoLut[0])
    {
        u32IsoLevel = 0;
        u32ISO1     = 0;
        u32ISO2     = g_au32BayernrIsoLut[0];
    }
    else
    {
        u32IsoLevel = NRGetIsoIndex(u32Iso);
        u32ISO1     = g_au32BayernrIsoLut[u32IsoLevel - 1];
        u32ISO2     = g_au32BayernrIsoLut[u32IsoLevel];
    }

    NRLimitLut(ViPipe, pstBayernrDynaCfg, pstBayernr, u32Iso, u32ISO1, u32ISO2, u32IsoLevel);
    NRCfg(pstBayernrDynaCfg, pstBayernr, u32IsoLevel, u32ISO2, u32ISO1, u32Iso);
    NRExtCfg(ViPipe,  pstBayernrDynaCfg, pstBayernr, u32IsoLevel, u32ISO2, u32ISO1, u32Iso);

    if (pstBayernrUsrCfg->bBnrMonoSensorEn == HI_TRUE)
    {
        for (i = 0; i < BAYER_PATT_NUM; i++)
        {
            pstBayernrDynaCfg->au8BnrCRatio[i] = 0;
            pstBayernrDynaCfg->au8AmedLevel[i] = 0;
            pstBayernrDynaCfg->bMedcEnable     = HI_FALSE;
        }
    }

    pstBayernrDynaCfg->bBnrLutUpdateEn = HI_TRUE;
    pstBayernrDynaCfg->u16RLmtRgain    = pstIspCtx->stLinkage.au32WhiteBalanceGain[0] >> 8;
    pstBayernrDynaCfg->u16RLmtBgain    = pstIspCtx->stLinkage.au32WhiteBalanceGain[3] >> 8;

    pstBayernrDynaCfg->bResh = HI_TRUE;
    pstBayernrUsrCfg->bResh  = HI_TRUE;

    return  HI_SUCCESS;
}

// WDR FW: ADJ_C(2) + ADJ_D(4) = 6
#define  ADJ_C  2
#define  ADJ_D  4

HI_U16 BCOM(HI_U64 x)
{
    HI_U64 out = (x << 22) / DIV_0_TO_1((x << 6) + (((1 << 20) - x) << ADJ_C));
    return (HI_U16)out;
}

// 16bit -> 20bit
HI_U32 BDEC(HI_U64 y)
{
    HI_U64 out = (y << 26) / DIV_0_TO_1((y << 6) + (((1 << 16) - y) << (ADJ_D + 6)));
    return (HI_U32)out;
}

HI_S32 NRLimitLut_WDR2to1(ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg, ISP_BAYERNR_S *pstBayernr, HI_U32 k, HI_U32 cprClip)
{
    HI_U32 i;
    HI_U16 u16BitMask = ((1 << (HI_ISP_BAYERNR_BITDEP - 1)) - 1);
    HI_U32 au32WDRFrameThr[WDR_MAX_FRAME + 2];
    HI_U32 u32WDR_PxValue , u32n8b_cur_axs;
    HI_U32 u32LmtNpThresh;
    HI_U32 sigma = 0, sigma_max = 0;
    HI_U32 WDR_JNLM_Limit_LUT[HI_ISP_BAYERNR_LMTLUTNUM];
    HI_U32 n8b_pre_pos, n8b_pre_axs;

    pstBayernr->au32ExpoValues[1] = (0 == pstBayernr->au32ExpoValues[1]) ? 64 : pstBayernr->au32ExpoValues[1];

    //intensity threshold
    au32WDRFrameThr[0] = ((HI_U32)pstBayernr->au16WDRFrameThr[0] << 12) / pstBayernr->au32ExpoValues[1] + (HI_U32)pstBayernr->u16WDRBlcThr;
    au32WDRFrameThr[1] = ((HI_U32)pstBayernr->au16WDRFrameThr[1] << 12) / pstBayernr->au32ExpoValues[1] + (HI_U32)pstBayernr->u16WDRBlcThr;

    pstBayernr->au16WDRFrameThr[0]  = BCOM((HI_U64)au32WDRFrameThr[0]);
    pstBayernr->au16WDRFrameThr[1]  = BCOM((HI_U64)au32WDRFrameThr[1]);

    for (i = 0; i < HI_ISP_BAYERNR_LMTLUTNUM; i++)
    {
        u32WDR_PxValue = i * 512;
        u32n8b_cur_axs = i * 2;
        n8b_pre_pos = HI_BAYERNR_MAX((HI_S32)BDEC((HI_U64)u32n8b_cur_axs << 8) - (HI_S32)pstBayernr->u16WDRBlcThr, 0);
        n8b_pre_axs = BDEC((HI_U64)u32n8b_cur_axs << 8);

        sigma = k * (n8b_pre_pos + HI_WDR_EINIT_BLCNR * ISP_BITFIX(12));
        sigma = (HI_U32)Sqrt32(sigma);
        sigma = (HI_U32)((HI_U64)sigma * u32n8b_cur_axs * ISP_BITFIX(12) / DIV_0_TO_1(HI_BAYERNR_MAX(n8b_pre_axs, cprClip)));

        if (u32WDR_PxValue <= pstBayernr->au16WDRFrameThr[0])
        {
            //long frame strength
            sigma /= DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[1]));
            sigma  = sigma * pstBayernr->stWDR.au8WDRFrameStr[1] / 16;
        }
        else if (u32WDR_PxValue > pstBayernr->au16WDRFrameThr[0] && u32WDR_PxValue < pstBayernr->au16WDRFrameThr[1])
        {
            HI_U32   sgmFS = sigma / DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[0]));
            HI_U32   sgmFL = sigma / DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[1]));
            HI_U32   bldr  = (u32WDR_PxValue - (HI_U32)pstBayernr->au16WDRFrameThr[0]) *  ISP_BITFIX(8) / HI_BAYERNR_MAX((pstBayernr->au16WDRFrameThr[1] - pstBayernr->au16WDRFrameThr[0]), 1);

            sgmFS = sgmFS * pstBayernr->stWDR.au8WDRFrameStr[0] / 16;
            sgmFL = sgmFL * pstBayernr->stWDR.au8WDRFrameStr[1] / 16;
            sigma = (sgmFS * bldr + sgmFL * (256 - bldr)) / 256;
        }
        else
        {
            //short frame strength
            sigma /= DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[0]));
            sigma  = sigma * pstBayernr->stWDR.au8WDRFrameStr[0] / 16;
        }
        WDR_JNLM_Limit_LUT[i] = sigma;
        sigma_max = (sigma_max < sigma) ? sigma : sigma_max;
    }

    for (i = 0; i < HI_ISP_BAYERNR_LMTLUTNUM; i++)
    {
        pstBayernrDynaCfg->au8JnlmLimitLut[i] = (HI_U8)((WDR_JNLM_Limit_LUT[i] * 128 + sigma_max / 2) / DIV_0_TO_1(sigma_max));
    }

    u32LmtNpThresh = (HI_U32)(sigma_max * (1 << (HI_ISP_BAYERNR_BITDEP - 8 - 3)) / ISP_BITFIX(10)); //sad win size, move to hw
    pstBayernrDynaCfg->u16LmtNpThresh = (u32LmtNpThresh > u16BitMask) ? u16BitMask : u32LmtNpThresh;

    return HI_SUCCESS;
}

HI_S32 NRLimitLut_WDR3to1(ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg, ISP_BAYERNR_S *pstBayernr, HI_U32 k, HI_U32 cprClip)
{
    HI_U8  i;
    HI_U16 u16BitMask = ((1 << (HI_ISP_BAYERNR_BITDEP - 1)) - 1);
    HI_U32 au32WDRFrameThr[WDR_MAX_FRAME + 2];
    HI_U32 u32WDR_PxValue , u32n8b_cur_axs;
    HI_U32 u32LmtNpThresh;

    HI_U32 sigma = 0, sigma_max = 0;
    HI_U32 WDR_JNLM_Limit_LUT[HI_ISP_BAYERNR_LMTLUTNUM];
    HI_U32 n8b_pre_pos, n8b_pre_axs;

    pstBayernr->au32ExpoValues[1] = (0 == pstBayernr->au32ExpoValues[1]) ? 64 : pstBayernr->au32ExpoValues[1];
    pstBayernr->au32ExpoValues[2] = (0 == pstBayernr->au32ExpoValues[2]) ? 64 : pstBayernr->au32ExpoValues[2];

    //intensity threshold
    au32WDRFrameThr[0] = (HI_U32)((pstBayernr->au16WDRFrameThr[0] << 12) / pstBayernr->au32ExpoValues[2]) + pstBayernr->u16WDRBlcThr;
    au32WDRFrameThr[1] = (HI_U32)((pstBayernr->au16WDRFrameThr[1] << 12) / pstBayernr->au32ExpoValues[2]) + pstBayernr->u16WDRBlcThr;
    au32WDRFrameThr[2] = (HI_U32)((pstBayernr->au16WDRFrameThr[0] << 12) / pstBayernr->au32ExpoValues[1]) + pstBayernr->u16WDRBlcThr;
    au32WDRFrameThr[3] = (HI_U32)((pstBayernr->au16WDRFrameThr[1] << 12) / pstBayernr->au32ExpoValues[1]) + pstBayernr->u16WDRBlcThr;

    pstBayernr->au16WDRFrameThr[0]  = BCOM((HI_U64)au32WDRFrameThr[0]);
    pstBayernr->au16WDRFrameThr[1]  = BCOM((HI_U64)au32WDRFrameThr[1]);
    pstBayernr->au16WDRFrameThr[2]  = BCOM((HI_U64)au32WDRFrameThr[2]);
    pstBayernr->au16WDRFrameThr[3]  = BCOM((HI_U64)au32WDRFrameThr[3]);

    for (i = 0; i < HI_ISP_BAYERNR_LMTLUTNUM; i++)
    {
        u32WDR_PxValue = i * 512;
        u32n8b_cur_axs = i * 2;

        n8b_pre_pos = HI_BAYERNR_MAX((HI_S32)BDEC((HI_U64)u32n8b_cur_axs << 8) - (HI_S32)pstBayernr->u16WDRBlcThr, 0);
        n8b_pre_axs = HI_BAYERNR_MAX((HI_S32)BDEC((HI_U64)u32n8b_cur_axs << 8), 0);

        sigma = k * (n8b_pre_pos + HI_WDR_EINIT_BLCNR * ISP_BITFIX(12));
        sigma = (HI_U32)Sqrt32(sigma);
        sigma = (HI_U32)((HI_U64)sigma * u32n8b_cur_axs * ISP_BITFIX(12) / DIV_0_TO_1(HI_BAYERNR_MAX(n8b_pre_axs, cprClip)));

        if (u32WDR_PxValue <= pstBayernr->au16WDRFrameThr[0])
        {
            //long frame strength
            sigma /= DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[2]));
            sigma  = sigma * pstBayernr->stWDR.au8WDRFrameStr[2] / 16;
        }
        else if (u32WDR_PxValue > pstBayernr->au16WDRFrameThr[0] && u32WDR_PxValue < pstBayernr->au16WDRFrameThr[1])
        {
            HI_U32   sgmFS = sigma / DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[1]));
            HI_U32   sgmFL = sigma / DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[2]));
            HI_U32   bldr  = (u32WDR_PxValue - (HI_U32)pstBayernr->au16WDRFrameThr[0]) *  ISP_BITFIX(8) / HI_BAYERNR_MAX(pstBayernr->au16WDRFrameThr[1] - pstBayernr->au16WDRFrameThr[0], 1);

            sgmFS = sgmFS * pstBayernr->stWDR.au8WDRFrameStr[1] / 16;
            sgmFL = sgmFL * pstBayernr->stWDR.au8WDRFrameStr[2] / 16;
            sigma = (sgmFS * bldr + sgmFL * (256 - bldr)) / 256;
        }
        else if (u32WDR_PxValue > pstBayernr->au16WDRFrameThr[1] && u32WDR_PxValue < pstBayernr->au16WDRFrameThr[2])
        {
            //medium frame strength
            sigma /= DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[1]));
            sigma  = sigma * pstBayernr->stWDR.au8WDRFrameStr[1] / 16;
        }
        else if (u32WDR_PxValue > pstBayernr->au16WDRFrameThr[2] && u32WDR_PxValue < pstBayernr->au16WDRFrameThr[3])
        {
            HI_U32   sgmFS = sigma / DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[0]));
            HI_U32   sgmFL = sigma / DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[1]));
            HI_U32   bldr  = (u32WDR_PxValue - (HI_U32)pstBayernr->au16WDRFrameThr[2]) *  ISP_BITFIX(8) / HI_BAYERNR_MAX(pstBayernr->au16WDRFrameThr[3] - pstBayernr->au16WDRFrameThr[2], 1);

            sgmFS = sgmFS * pstBayernr->stWDR.au8WDRFrameStr[0] / 16;
            sgmFL = sgmFL * pstBayernr->stWDR.au8WDRFrameStr[1] / 16;
            sigma = (sgmFS * bldr + sgmFL * (256 - bldr)) / 256;
        }
        else
        {
            //short frame strength
            sigma /= DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[0]));
            sigma  = sigma * pstBayernr->stWDR.au8WDRFrameStr[0] / 16;
        }

        WDR_JNLM_Limit_LUT[i] = sigma;
        sigma_max = (sigma_max < sigma) ? sigma : sigma_max;
    }

    for (i = 0; i <= 128; i++)
    {
        pstBayernrDynaCfg->au8JnlmLimitLut[i] = (HI_U8)((WDR_JNLM_Limit_LUT[i] * 128 + sigma_max / 2) / DIV_0_TO_1(sigma_max));
    }

    u32LmtNpThresh = (HI_U32)(sigma_max * (1 << (HI_ISP_BAYERNR_BITDEP - 8 - 3)) / ISP_BITFIX(10)); //sad win size, move to hw
    pstBayernrDynaCfg->u16LmtNpThresh = (u32LmtNpThresh > u16BitMask) ? u16BitMask : u32LmtNpThresh;

    return HI_SUCCESS;
}

HI_S32 NRLimitLut_WDR4to1(ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg, ISP_BAYERNR_S *pstBayernr, HI_U32 k, HI_U32 cprClip)
{
    HI_U8  i;
    HI_U16 u16BitMask = ((1 << (HI_ISP_BAYERNR_BITDEP - 1)) - 1);
    HI_U32 au32WDRFrameThr[WDR_MAX_FRAME + 2];
    HI_U32 u32WDR_PxValue, u32n8b_cur_axs;
    HI_U32 u32LmtNpThresh;

    HI_U32 sigma = 0, sigma_max = 0;
    HI_U32 WDR_JNLM_Limit_LUT[HI_ISP_BAYERNR_LMTLUTNUM];
    HI_U32 n8b_pre_pos, n8b_pre_axs;

    pstBayernr->au32ExpoValues[1] = (0 == pstBayernr->au32ExpoValues[1]) ? 64 : pstBayernr->au32ExpoValues[1];
    pstBayernr->au32ExpoValues[2] = (0 == pstBayernr->au32ExpoValues[2]) ? 64 : pstBayernr->au32ExpoValues[2];
    pstBayernr->au32ExpoValues[3] = (0 == pstBayernr->au32ExpoValues[3]) ? 64 : pstBayernr->au32ExpoValues[3];

    // intensity threshold
    au32WDRFrameThr[0] = (HI_U32)((pstBayernr->au16WDRFrameThr[0] << 12) / pstBayernr->au32ExpoValues[3]) + pstBayernr->u16WDRBlcThr;
    au32WDRFrameThr[1] = (HI_U32)((pstBayernr->au16WDRFrameThr[1] << 12) / pstBayernr->au32ExpoValues[3]) + pstBayernr->u16WDRBlcThr;
    au32WDRFrameThr[2] = (HI_U32)((pstBayernr->au16WDRFrameThr[0] << 12) / pstBayernr->au32ExpoValues[2]) + pstBayernr->u16WDRBlcThr;
    au32WDRFrameThr[3] = (HI_U32)((pstBayernr->au16WDRFrameThr[1] << 12) / pstBayernr->au32ExpoValues[2]) + pstBayernr->u16WDRBlcThr;
    au32WDRFrameThr[4] = (HI_U32)((pstBayernr->au16WDRFrameThr[0] << 12) / pstBayernr->au32ExpoValues[1]) + pstBayernr->u16WDRBlcThr;
    au32WDRFrameThr[5] = (HI_U32)((pstBayernr->au16WDRFrameThr[1] << 12) / pstBayernr->au32ExpoValues[1]) + pstBayernr->u16WDRBlcThr;

    pstBayernr->au16WDRFrameThr[0]  = BCOM((HI_U64)au32WDRFrameThr[0]);
    pstBayernr->au16WDRFrameThr[1]  = BCOM((HI_U64)au32WDRFrameThr[1]);
    pstBayernr->au16WDRFrameThr[2]  = BCOM((HI_U64)au32WDRFrameThr[2]);
    pstBayernr->au16WDRFrameThr[3]  = BCOM((HI_U64)au32WDRFrameThr[3]);
    pstBayernr->au16WDRFrameThr[4]  = BCOM((HI_U64)au32WDRFrameThr[4]);
    pstBayernr->au16WDRFrameThr[5]  = BCOM((HI_U64)au32WDRFrameThr[5]);

    for (i = 0; i < HI_ISP_BAYERNR_LMTLUTNUM; i++)
    {
        u32WDR_PxValue = i * 512;
        u32n8b_cur_axs = i * 2;
        n8b_pre_pos = HI_BAYERNR_MAX((HI_S32)BDEC((HI_U64)u32n8b_cur_axs << 8) - (HI_S32)pstBayernr->u16WDRBlcThr, 0);
        n8b_pre_axs = HI_BAYERNR_MAX((HI_S32)BDEC((HI_U64)u32n8b_cur_axs << 8), 0);

        sigma = k * (n8b_pre_pos + HI_WDR_EINIT_BLCNR * ISP_BITFIX(12));
        sigma = (HI_U32)Sqrt32(sigma);
        sigma = (HI_U32)((HI_U64)sigma * u32n8b_cur_axs * ISP_BITFIX(12) / DIV_0_TO_1(HI_BAYERNR_MAX(n8b_pre_axs, cprClip)));

        if (u32WDR_PxValue <= pstBayernr->au16WDRFrameThr[0])
        {
            //long frame strength
            sigma /= DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[3]));
            sigma  = sigma * pstBayernr->stWDR.au8WDRFrameStr[3] / 16;
        }
        else if (u32WDR_PxValue > pstBayernr->au16WDRFrameThr[0] && u32WDR_PxValue < pstBayernr->au16WDRFrameThr[1])
        {
            HI_U32   sgmFS = sigma / DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[2]));
            HI_U32   sgmFL = sigma / DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[3]));
            HI_U32   bldr  = (u32WDR_PxValue - (HI_U32)pstBayernr->au16WDRFrameThr[0]) *  ISP_BITFIX(8) / HI_BAYERNR_MAX(pstBayernr->au16WDRFrameThr[1] - pstBayernr->au16WDRFrameThr[0], 1);

            sgmFS = sgmFS * pstBayernr->stWDR.au8WDRFrameStr[2] / 16;
            sgmFL = sgmFL * pstBayernr->stWDR.au8WDRFrameStr[3] / 16;
            sigma = (sgmFS * bldr + sgmFL * (256 - bldr)) / 256;
        }
        else if (u32WDR_PxValue > pstBayernr->au16WDRFrameThr[1] && u32WDR_PxValue < pstBayernr->au16WDRFrameThr[2])
        {
            //medium frame strength
            sigma /= DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[2]));
            sigma  = sigma * pstBayernr->stWDR.au8WDRFrameStr[2] / 16;
        }
        else if (u32WDR_PxValue > pstBayernr->au16WDRFrameThr[2] && u32WDR_PxValue < pstBayernr->au16WDRFrameThr[3])
        {
            HI_U32   sgmFS = sigma / DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[1]));
            HI_U32   sgmFL = sigma / DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[2]));
            HI_U32   bldr  = (u32WDR_PxValue - (HI_U32)pstBayernr->au16WDRFrameThr[2]) *  ISP_BITFIX(8) / HI_BAYERNR_MAX(pstBayernr->au16WDRFrameThr[3] - pstBayernr->au16WDRFrameThr[2], 1);

            sgmFS = sgmFS * pstBayernr->stWDR.au8WDRFrameStr[1] / 16;
            sgmFL = sgmFL * pstBayernr->stWDR.au8WDRFrameStr[2] / 16;
            sigma = (sgmFS * bldr + sgmFL * (256 - bldr)) / 256;
        }
        else if (u32WDR_PxValue > pstBayernr->au16WDRFrameThr[3] && u32WDR_PxValue < pstBayernr->au16WDRFrameThr[4])
        {
            //short frame strength
            sigma /= DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[1]));
            sigma  = sigma * pstBayernr->stWDR.au8WDRFrameStr[1] / 16;
        }
        else if (u32WDR_PxValue > pstBayernr->au16WDRFrameThr[4] && u32WDR_PxValue < pstBayernr->au16WDRFrameThr[5])
        {
            HI_U32   sgmFS = sigma / DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[0]));
            HI_U32   sgmFL = sigma / DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[1]));
            HI_U32   bldr  = (u32WDR_PxValue - (HI_U32)pstBayernr->au16WDRFrameThr[4]) *  ISP_BITFIX(8) / HI_BAYERNR_MAX(pstBayernr->au16WDRFrameThr[5] - pstBayernr->au16WDRFrameThr[4], 1);

            sgmFS = sgmFS * pstBayernr->stWDR.au8WDRFrameStr[0] / 16;
            sgmFL = sgmFL * pstBayernr->stWDR.au8WDRFrameStr[1] / 16;
            sigma = (sgmFS * bldr + sgmFL * (256 - bldr)) / 256;
        }
        else
        {
            //short short frame strength
            sigma /= DIV_0_TO_1((HI_U32)Sqrt32(pstBayernr->au32ExpoValues[0]));
            sigma  = sigma * pstBayernr->stWDR.au8WDRFrameStr[0] / 16;
        }

        WDR_JNLM_Limit_LUT[i] = sigma;
        sigma_max = (sigma_max < sigma) ? sigma : sigma_max;
    }

    for (i = 0; i <= 128; i++)
    {
        pstBayernrDynaCfg->au8JnlmLimitLut[i] = (HI_U8)((WDR_JNLM_Limit_LUT[i] * 128 + sigma_max / 2) / DIV_0_TO_1(sigma_max));
    }

    u32LmtNpThresh = (HI_U32)(sigma_max * (1 << (HI_ISP_BAYERNR_BITDEP - 8 - 3)) / ISP_BITFIX(10)); //sad win size, move to hw
    pstBayernrDynaCfg->u16LmtNpThresh = (u32LmtNpThresh > u16BitMask) ? u16BitMask : u32LmtNpThresh;

    return HI_SUCCESS;
}

HI_S32 hiisp_bayernr_fw_wdr(HI_U32 u32Iso, VI_PIPE ViPipe, ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg, ISP_BAYERNR_USR_CFG_S *pstBayernrUsrCfg)
{
    HI_U8  i;
    HI_U32 u32IsoLevel;
    HI_U32 u32ISO1 = 0, u32ISO2 = 0;
    HI_U32 k, cprClip;
    HI_FLOAT fCalibrationCoef = 0.0f;

    ISP_BAYERNR_S      *pstBayernr = HI_NULL;
    ISP_CMOS_DEFAULT_S *pstSnsDft  = HI_NULL;
    ISP_CTX_S          *pstIspCtx  = HI_NULL;
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);
    BAYERNR_GET_CTX(ViPipe, pstBayernr);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstBayernrUsrCfg->bBnrMonoSensorEn    = pstSnsDft->stBayerNr.bBnrMonoSensorEn;     //MonoSensor, waiting to get
    pstBayernrUsrCfg->bBnrLscEn           = pstBayernr->bNrLscEnable;
    pstBayernrUsrCfg->u8BnrLscRatio       = pstBayernr->u8NrLscRatio;

    // Noise LUT
    fCalibrationCoef = Bayernr_getKfromNoiseLut(pstSnsDft->stNoiseCalibration.afCalibrationCoef, pstSnsDft->stNoiseCalibration.u16CalibrationLutNum, u32Iso);
    k     = (HI_U32)(fCalibrationCoef * ISP_BITFIX(14));
    cprClip = ISP_BITFIX(12) / ISP_BITFIX(ADJ_D); //the slope of the beginning of Compression curve

    switch (pstBayernr->u8WdrFramesMerge)
    {
        case 2:  //WDR mode: 2 to 1
            NRLimitLut_WDR2to1(pstBayernrDynaCfg, pstBayernr, k, cprClip);
            break;

        case 3:  //WDR mode: 3 to 1
            NRLimitLut_WDR3to1(pstBayernrDynaCfg, pstBayernr, k, cprClip);
            break;

        case 4:  //WDR mode: 4 to 1
            NRLimitLut_WDR4to1(pstBayernrDynaCfg, pstBayernr, k, cprClip);
            break;
        default:
            break;
    }

    if (0 == HI_WDR_EINIT_BLCNR)
    {
        pstBayernrDynaCfg->au8JnlmLimitLut[0] = pstBayernrDynaCfg->au8JnlmLimitLut[3];
        pstBayernrDynaCfg->au8JnlmLimitLut[1] = pstBayernrDynaCfg->au8JnlmLimitLut[3];
        pstBayernrDynaCfg->au8JnlmLimitLut[2] = pstBayernrDynaCfg->au8JnlmLimitLut[3];
    }

    if (u32Iso > g_au32BayernrIsoLut[HI_ISP_NR_LUT_LEN - 1])
    {
        u32IsoLevel = HI_ISP_NR_LUT_LEN - 1;
        u32ISO1 = g_au32BayernrIsoLut[HI_ISP_NR_LUT_LEN - 1];
        u32ISO2 = g_au32BayernrIsoLut[HI_ISP_NR_LUT_LEN - 1];
    }
    else if (u32Iso <= g_au32BayernrIsoLut[0])
    {
        u32IsoLevel = 0;
        u32ISO1 = 0;
        u32ISO2 = g_au32BayernrIsoLut[0];
    }
    else
    {
        u32IsoLevel = NRGetIsoIndex(u32Iso);
        u32ISO1 = g_au32BayernrIsoLut[u32IsoLevel - 1];
        u32ISO2 = g_au32BayernrIsoLut[u32IsoLevel];
    }

    NRCfg(pstBayernrDynaCfg, pstBayernr, u32IsoLevel, u32ISO2, u32ISO1, u32Iso);
    NRExtCfg(ViPipe, pstBayernrDynaCfg, pstBayernr, u32IsoLevel, u32ISO2, u32ISO1, u32Iso);

    if (pstBayernrUsrCfg->bBnrMonoSensorEn == 1)
    {
        for (i = 0; i < BAYER_PATT_NUM; i++)
        {
            pstBayernrDynaCfg->bMedcEnable     = HI_FALSE;
            pstBayernrDynaCfg->au8BnrCRatio[i] = 0;
            pstBayernrDynaCfg->au8AmedLevel[i] = 0;
        }
    }

    pstBayernrDynaCfg->bBnrLutUpdateEn = HI_TRUE;
    pstBayernrDynaCfg->u16RLmtRgain = pstIspCtx->stLinkage.au32WhiteBalanceGain[0];
    pstBayernrDynaCfg->u16RLmtBgain = pstIspCtx->stLinkage.au32WhiteBalanceGain[3];

    pstBayernrDynaCfg->bResh = HI_TRUE;
    pstBayernrUsrCfg->bResh  = HI_TRUE;

    return HI_SUCCESS;
}

static HI_BOOL __inline CheckBnrOpen(ISP_BAYERNR_S *pstBayernr)
{
    return (HI_TRUE == pstBayernr->bEnable);
}

HI_S32 ISP_BayernrRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                      HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_U8 i;
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_BAYERNR_S *pstBayernr = HI_NULL;

    BAYERNR_GET_CTX(ViPipe, pstBayernr);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    /* calculate every two interrupts */
    if (!pstBayernr->bInit)
    {
        return HI_SUCCESS;
    }

    if ((0 != pstIspCtx->u32FrameCnt % 2) && (HI_TRUE != pstIspCtx->stLinkage.bSnapState) && (pstIspCtx->stLinkage.u8ProIndex < 1))
    {
        return HI_SUCCESS;
    }

    pstBayernr->bEnable = hi_ext_system_bayernr_enable_read(ViPipe);

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        pstRegCfg->stAlgRegCfg[i].stBnrRegCfg.bBnrEnable = pstBayernr->bEnable;
    }

    pstRegCfg->unKey.bit1BayernrCfg = 1;

    /*check hardware setting*/
    if (!CheckBnrOpen(pstBayernr))
    {
        return HI_SUCCESS;
    }

    BayernrReadExtregs(ViPipe);
    BayernrReadProMode(ViPipe);

    for ( i = 0; i < pstIspCtx->stBlockAttr.u8BlockNum; i++)
    {
        if (pstBayernr->u8WdrFramesMerge > 1)
        {
            hiisp_bayernr_fw_wdr(pstIspCtx->stLinkage.u32Iso, ViPipe, &pstRegCfg->stAlgRegCfg[i].stBnrRegCfg.stDynaRegCfg, &pstRegCfg->stAlgRegCfg[i].stBnrRegCfg.stUsrRegCfg);
        }
        else
        {
            hiisp_bayernr_fw(pstIspCtx->stLinkage.u32Iso, ViPipe, i, &pstRegCfg->stAlgRegCfg[i].stBnrRegCfg.stDynaRegCfg, &pstRegCfg->stAlgRegCfg[i].stBnrRegCfg.stUsrRegCfg);
        }
    }

    return HI_SUCCESS;
}


static __inline HI_S32 BayerNrImageResWrite(VI_PIPE ViPipe, ISP_CMOS_SENSOR_IMAGE_MODE_S *pstRes)
{
    return HI_SUCCESS;
}


HI_S32 ISP_BayernrCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    ISP_BAYERNR_S *pstBayernr = HI_NULL;
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);

    switch (u32Cmd)
    {
        case ISP_WDR_MODE_SET :
            BAYERNR_GET_CTX(ViPipe, pstBayernr);
            pstBayernr->bInit = HI_FALSE;
            ISP_BayernrInit(ViPipe, (HI_VOID *)&pRegCfg->stRegCfg);
            break;
        case ISP_CHANGE_IMAGE_MODE_SET:
            break;
        case ISP_PROC_WRITE:
            BayernrProcWrite(ViPipe, (ISP_CTRL_PROC_WRITE_S *)pValue);
            break;
        default :
            break;
    }
    return HI_SUCCESS;
}

HI_S32 ISP_BayernrExit(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_GET_CTX(ViPipe, pstIspCtx);

    for (i = 0; i < pstIspCtx->stBlockAttr.u8BlockNum; i++)
    {
        isp_bnr_en_write(ViPipe, i, HI_FALSE);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterBayernr(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_BAYERNR;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_BayernrInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_BayernrRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_BayernrCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_BayernrExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


