/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_fpn.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/07/24
  Description   :
  History       :
  1.Date        : 2013/07/24
    Author      :
    Modification: Created file

******************************************************************************/

#include "isp_config.h"
#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_ext_config.h"


#include "hi_comm_vi.h"
#include "mpi_vi.h"
#include "vi_ext.h"
#include "isp_proc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/****************************************************************************
 * MACRO DEFINITION                                                         *
 ****************************************************************************/
#define FPN_OVERFLOWTHR                 0x7C0
#define ISP_FPN_MAX_O                   0xFFF
#define FPN_OVERFLOWTHR_OFF             0x3FFF
#define FPN_CHN_NUM                     4
#define ISP_FPN_MODE_CORRECTION             0x0
#define ISP_FPN_MODE_CALIBRATE              0x1
#define ISP_FPN_CLIP(min,max,x)         ( (x)<= (min) ? (min) : ((x)>(max)?(max):(x)) )

//HI_S32 g_s32ViFd = -1;
//static HI_S32 g_s32ViPipeFd[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = -1};

typedef struct hiISP_FPN_S
{
    HI_S32  s32Iso[ISP_MAX_PIPE_NUM];
    HI_U32  u32IspIntCnt;
} ISP_FPN_S;

typedef enum hiISP_SENSOR_BIT_WIDTH_E
{
    ISP_SENSOR_8BIT  = 8,
    ISP_SENSOR_10BIT = 10,
    ISP_SENSOR_12BIT = 12,
    ISP_SENSOR_14BIT = 14,
    ISP_SENSOR_16BIT = 16,
    ISP_SENSOR_32BIT = 32,
    ISP_SENSOR_BUTT
} ISP_SENSOR_BIT_WIDTH_E;

ISP_FPN_S g_astFpnCtx[ISP_MAX_PIPE_NUM] = {0};
#define FPN_GET_CTX(dev, pstFpnCtx)   pstFpnCtx = &g_astFpnCtx[dev]


HI_S32 ISP_CheckFpnConfig(ISP_FPN_TYPE_E enFpnType,
                          const ISP_FPN_FRAME_INFO_S *pstFPNFrmInfo)
{
    COMPRESS_MODE_E enCompressMode;
    PIXEL_FORMAT_E  enPixelFormat;

    if (0 == pstFPNFrmInfo->u32Iso)
    {
        ISP_TRACE(HI_DBG_ERR, "Invalid fpn ISO(%d)!\n", pstFPNFrmInfo->u32Iso);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    if (pstFPNFrmInfo->u32Offset[0] > 0xFFF || pstFPNFrmInfo->u32Offset[1] > 0xFFF)
    {
        ISP_TRACE(HI_DBG_ERR, "Invalid fpn u32Offset0 0x%x, u32Offset1 0x%x, must be in [0, 0xFFF].\n",
                  pstFPNFrmInfo->u32Offset[0], pstFPNFrmInfo->u32Offset[1]);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    enCompressMode  = pstFPNFrmInfo->stFpnFrame.stVFrame.enCompressMode;
    enPixelFormat   = pstFPNFrmInfo->stFpnFrame.stVFrame.enPixelFormat;

    if ((enFpnType != ISP_FPN_TYPE_FRAME) && (enFpnType != ISP_FPN_TYPE_LINE))
    {
        ISP_TRACE(HI_DBG_ERR, "Invalid fpn Mode, should be in {line, frame} mode!\n");
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((enCompressMode != COMPRESS_MODE_NONE) && (enCompressMode != COMPRESS_MODE_LINE))
    {
        ISP_TRACE(HI_DBG_ERR,
                  "fpn compress can only be {none(%d), frame(%d)} mode!\n",
                  COMPRESS_MODE_NONE, COMPRESS_MODE_LINE);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    if (enFpnType == ISP_FPN_TYPE_LINE)
    {
        /* 16bit only in line mode */
        if (PIXEL_FORMAT_RGB_BAYER_16BPP != enPixelFormat)
        {
            ISP_TRACE(HI_DBG_ERR, "only support 16bit fpn in line mode!\n");
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }

        if (COMPRESS_MODE_NONE != enCompressMode)
        {
            ISP_TRACE(HI_DBG_ERR, "fpn compress is not supported in line mode!\n");
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    else
    {
        if ((PIXEL_FORMAT_RGB_BAYER_16BPP != enPixelFormat)
            && (PIXEL_FORMAT_RGB_BAYER_8BPP != enPixelFormat)
            && (PIXEL_FORMAT_RGB_BAYER_10BPP != enPixelFormat)
            && (PIXEL_FORMAT_RGB_BAYER_12BPP != enPixelFormat))
        {
            ISP_TRACE(HI_DBG_ERR,
                      "enPixelFormat invalid(%d), only support Raw8,Raw10,Raw12,Raw16.\n",
                      enPixelFormat);
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }

        if ((COMPRESS_MODE_NONE != enCompressMode)
            && (PIXEL_FORMAT_RGB_BAYER_16BPP == enPixelFormat))
        {
            ISP_TRACE(HI_DBG_ERR, "fpn compress is not supported in 16bit frame mode!\n");
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return HI_SUCCESS;
}

HI_S32 ISP_CheckCalibrateAttr(VI_PIPE ViPipe, ISP_FPN_CALIBRATE_ATTR_S *pstCalibrate)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_FPN_TYPE_E enFpnType;
    const ISP_FPN_FRAME_INFO_S *pstFPNFrmInfo;

    enFpnType      = pstCalibrate->enFpnType;
    pstFPNFrmInfo  = &pstCalibrate->stFpnCaliFrame;
    s32Ret         = ISP_CheckFpnConfig(enFpnType, pstFPNFrmInfo);

    return s32Ret;
}

HI_S32 ISP_CheckCorrectionAttr(VI_PIPE ViPipe, const ISP_FPN_ATTR_S *pstCorrection)
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32Strength;
    ISP_FPN_TYPE_E enFpnType;
    HI_U32 u32CalIso;
    const ISP_FPN_FRAME_INFO_S *pstFPNFrmInfo;

    if (pstCorrection->enOpType >= OP_TYPE_BUTT)
    {
        ISP_TRACE(HI_DBG_ERR, "Invalid fpn op type!\n");
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }
    if ((pstCorrection->bEnable != HI_TRUE)
        && (pstCorrection->bEnable != HI_FALSE))
    {
        ISP_TRACE(HI_DBG_ERR, "Invalid fpn enable, must be {true, false}!\n");
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    if (OP_TYPE_MANUAL == pstCorrection->enOpType)
    {
        u32Strength    = pstCorrection->stManual.u32Strength;
        enFpnType      = pstCorrection->enFpnType;
        pstFPNFrmInfo  = &pstCorrection->stFpnFrmInfo;

        if (u32Strength > 1023)
        {
            ISP_TRACE(HI_DBG_ERR, "Invalid fpn strength, over 1023!\n");
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }
    }
    else
    {
        u32CalIso      = pstCorrection->stFpnFrmInfo.u32Iso;
        if (0 == u32CalIso)
        {

            ISP_TRACE(HI_DBG_ERR, "Invalid fpn ISO(%d)!\n", u32CalIso);
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }
        enFpnType      = pstCorrection->enFpnType;
        pstFPNFrmInfo  = &pstCorrection->stFpnFrmInfo;
    }

    if (pstFPNFrmInfo->u32Offset[0] > 0xFFF || pstFPNFrmInfo->u32Offset[1] > 0xFFF)
    {
        ISP_TRACE(HI_DBG_ERR, "Invalid fpn offset, can't be larger than 0xFFF!\n");
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    s32Ret = ISP_CheckFpnConfig(enFpnType, pstFPNFrmInfo);

    /* to do param check */
    return s32Ret;
}


HI_S32  ISP_SetCalibrateAttr(VI_PIPE ViPipe, ISP_FPN_CALIBRATE_ATTR_S *pstCalibrate)
{
    VI_FPN_ATTR_S stFpnAttr;
    HI_S32 s32Ret = HI_SUCCESS;

    pstCalibrate->stFpnCaliFrame.u32Iso = hi_ext_system_fpn_sensor_iso_read(ViPipe);

    s32Ret = ISP_CheckCalibrateAttr(ViPipe, pstCalibrate);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    stFpnAttr.enFpnWorkMode     = FPN_MODE_CALIBRATE;
    stFpnAttr.enFpnType         = pstCalibrate->enFpnType;

    stFpnAttr.stCalibrate.u32Threshold = pstCalibrate->u32Threshold;
    stFpnAttr.stCalibrate.u32FrameNum = pstCalibrate->u32FrameNum;

    memcpy(&stFpnAttr.stCalibrate.stFpnCaliFrame,
           &pstCalibrate->stFpnCaliFrame,
           sizeof(ISP_FPN_FRAME_INFO_S));

    /* get calibrate result */
    s32Ret = MPI_VI_SetFPNAttr(ViPipe, &stFpnAttr);

    /* get value from temp variable */
    memcpy(&pstCalibrate->stFpnCaliFrame,
           &stFpnAttr.stCalibrate.stFpnCaliFrame,
           sizeof(ISP_FPN_FRAME_INFO_S));

    return s32Ret;
}

inline HI_U32 ISP_Fpn_GetStrength(HI_U32 u32Iso, HI_S32 s32K1, HI_S32 s32K2, HI_S32 s32K3, HI_U32 u32CalibrateIso)
{
    HI_U32 u32Strength = 256;
    HI_U32 u32In;

    u32In = 256 * u32Iso / DIV_0_TO_1(u32CalibrateIso);
    //s32In = 256 * s32Iso /6400;
    //u32Strength = (s32K1 + s32K2*u32In + s32K3*u32In*u32In) >> 8;
    u32Strength = u32In;
    return u32Strength;
}

HI_VOID ISP_SetFpnDarkFrameBitWidth(ISP_FPN_DYNA_CFG_S *pstDynaRegCfg, HI_S32 enFpnType,
                                    HI_S32 enFpnWorkMode,
                                    HI_U32 u32SensorBitWidth,
                                    PIXEL_FORMAT_E enFpnPixFmt)
{
    if (0 == enFpnType) /*frame mode*/
    {
        if (PIXEL_FORMAT_RGB_BAYER_16BPP != enFpnPixFmt)
        {
            if (ISP_FPN_MODE_CORRECTION == enFpnWorkMode)
            {
                pstDynaRegCfg->u32IspFpnShift             = 4;
                pstDynaRegCfg->u32IspFpnFrameCalibShift      = 0;
            }
            else if (ISP_FPN_MODE_CALIBRATE == enFpnWorkMode)
            {
                pstDynaRegCfg->u32IspFpnShift             = 0;
                pstDynaRegCfg->u32IspFpnFrameCalibShift      = 4;
            }

            pstDynaRegCfg->u32IspFpnInShift  = 0;
            pstDynaRegCfg->u32IspFpnOutShift = 0;

            pstDynaRegCfg->u32IspFpnMaxO = 0x3FFF;
        }
        else /* fpn frame 16bit */
        {
            if (ISP_SENSOR_14BIT == u32SensorBitWidth)
            {
                pstDynaRegCfg->u32IspFpnShift             = 0;
                pstDynaRegCfg->u32IspFpnFrameCalibShift = 0;
                pstDynaRegCfg->u32IspFpnInShift          = 2;    //14bit >> 2
                pstDynaRegCfg->u32IspFpnOutShift         = 2;   // 12bit << 2

                pstDynaRegCfg->u32IspFpnMaxO = 0x3FFF;
            }
            else if (ISP_SENSOR_12BIT == u32SensorBitWidth)
            {
                if (ISP_FPN_MODE_CORRECTION == enFpnWorkMode)
                {
                    pstDynaRegCfg->u32IspFpnShift             = 0;
                    pstDynaRegCfg->u32IspFpnFrameCalibShift = 0;
                    pstDynaRegCfg->u32IspFpnInShift          = 2;  //14bit >> 2
                    pstDynaRegCfg->u32IspFpnOutShift         = 2;  //12bit << 2
                }
                else
                {
                    pstDynaRegCfg->u32IspFpnShift             = 0;
                    pstDynaRegCfg->u32IspFpnFrameCalibShift = 0;
                    pstDynaRegCfg->u32IspFpnInShift          = 2;  //14bit >> 2
                    pstDynaRegCfg->u32IspFpnOutShift         = 0;
                }

                pstDynaRegCfg->u32IspFpnMaxO = 0xFFF;
            }
            else if (ISP_SENSOR_10BIT == u32SensorBitWidth)
            {
                if (ISP_FPN_MODE_CORRECTION == enFpnWorkMode)
                {
                    pstDynaRegCfg->u32IspFpnShift             = 0;
                    pstDynaRegCfg->u32IspFpnFrameCalibShift = 0;
                    pstDynaRegCfg->u32IspFpnInShift          = 4;  //14bit >> 4
                    pstDynaRegCfg->u32IspFpnOutShift         = 4;  //10bit << 4
                }
                else
                {
                    pstDynaRegCfg->u32IspFpnShift           = 0;
                    pstDynaRegCfg->u32IspFpnFrameCalibShift = 0;
                    pstDynaRegCfg->u32IspFpnInShift         = 4;  //14bit >> 4
                    pstDynaRegCfg->u32IspFpnOutShift        = 0;
                }

                pstDynaRegCfg->u32IspFpnMaxO = 0x3FF;
            }
        }
    }
    else /*line mode*/
    {
        pstDynaRegCfg->u32IspFpnInShift          = 0;
        pstDynaRegCfg->u32IspFpnInShift          = 0;
        pstDynaRegCfg->u32IspFpnOutShift         = 0;
        pstDynaRegCfg->u32IspFpnFrameCalibShift  = 0;

        pstDynaRegCfg->u32IspFpnMaxO = 0x3FFF;
    }

}

HI_VOID ISP_OfflineCorrection(VI_PIPE ViPipe, ISP_REGCFG_S  *pRegCfg, const ISP_FPN_ATTR_S *pstCorrection, HI_U32 u32Strength)
{
    HI_U8               i = 0;
    HI_U8               j = 0;
    HI_U32              u32Overlap        = 100;  /* OVERLAP */
    HI_S32              s32ActCurLeftWidth;
    HI_S32              s32ActCurRightWidth;
    SIZE_S              stFpnChSize[2];
    ISP_CTX_S           *pstIspCtx        = HI_NULL;


    ISP_GET_CTX(ViPipe, pstIspCtx);
    u32Overlap = pstIspCtx->stBlockAttr.u32OverLap;

    if (ISP_MODE_RUNNING_OFFLINE == pstIspCtx->stBlockAttr.enIspRunningMode)
    {
        stFpnChSize[0].u32Height = pstCorrection->stFpnFrmInfo.stFpnFrame.stVFrame.u32Height;
        stFpnChSize[0].u32Width  = pstCorrection->stFpnFrmInfo.stFpnFrame.stVFrame.u32Width;
    }
    else
    {
        s32ActCurLeftWidth   = (pstCorrection->stFpnFrmInfo.stFpnFrame.stVFrame.u32Width / 2 + 255) & 0xFFFFFF00 ;
        s32ActCurRightWidth  = pstCorrection->stFpnFrmInfo.stFpnFrame.stVFrame.u32Width - s32ActCurLeftWidth ;
        stFpnChSize[0].u32Width  = s32ActCurLeftWidth + u32Overlap;
        stFpnChSize[1].u32Width  = s32ActCurRightWidth + u32Overlap;
        stFpnChSize[0].u32Height = pstCorrection->stFpnFrmInfo.stFpnFrame.stVFrame.u32Height;
        stFpnChSize[1].u32Height = pstCorrection->stFpnFrmInfo.stFpnFrame.stVFrame.u32Height;
    }

    for (i = 0; i < pRegCfg->stRegCfg.u8CfgNum; i++)
    {
        pRegCfg->stRegCfg.stAlgRegCfg[i].stFpnRegCfg.stDynaRegCfg.u32IspFpnLineFrame = pstCorrection->enFpnType;
        pRegCfg->stRegCfg.stAlgRegCfg[i].stFpnRegCfg.stDynaRegCfg.u32IspFpnCalibCorr = ISP_FPN_MODE_CORRECTION;
        pRegCfg->stRegCfg.stAlgRegCfg[i].stFpnRegCfg.stDynaRegCfg.bIspFpnEnable = pstCorrection->bEnable;
        for (j = 0; j < FPN_CHN_NUM; j++)
        {
            pRegCfg->stRegCfg.stAlgRegCfg[i].stFpnRegCfg.stDynaRegCfg.u32IspFpnCorrectEnId[j] = HI_TRUE;
            pRegCfg->stRegCfg.stAlgRegCfg[i].stFpnRegCfg.stDynaRegCfg.u32IspFpnStrength[j]    = u32Strength;
            pRegCfg->stRegCfg.stAlgRegCfg[i].stFpnRegCfg.stDynaRegCfg.u32IspFpnOffset[j]      = pstCorrection->stFpnFrmInfo.u32Offset[i];
        }

        pRegCfg->stRegCfg.stAlgRegCfg[i].stFpnRegCfg.stDynaRegCfg.u32IspFpnMaxO   = ISP_FPN_MAX_O;
        pRegCfg->stRegCfg.stAlgRegCfg[i].stFpnRegCfg.stDynaRegCfg.u32IspFpnOverflowThr = FPN_OVERFLOWTHR_OFF;
        pRegCfg->stRegCfg.stAlgRegCfg[i].stFpnRegCfg.stDynaRegCfg.bIspFpnOffline  = HI_TRUE;
        pRegCfg->stRegCfg.stAlgRegCfg[i].stFpnRegCfg.stDynaRegCfg.u32IspFpnHeight = stFpnChSize[i].u32Height - 1;
        pRegCfg->stRegCfg.stAlgRegCfg[i].stFpnRegCfg.stDynaRegCfg.u32IspFpnWidth  = stFpnChSize[i].u32Width - 1;
        ISP_SetFpnDarkFrameBitWidth(&pRegCfg->stRegCfg.stAlgRegCfg[i].stFpnRegCfg.stDynaRegCfg,
                                    pstCorrection->enFpnType,
                                    ISP_FPN_MODE_CORRECTION,
                                    ISP_SENSOR_12BIT,
                                    pstCorrection->stFpnFrmInfo.stFpnFrame.stVFrame.enPixelFormat);
    }
}

HI_S32 ISP_SetCorrectionAttr(VI_PIPE ViPipe, const ISP_FPN_ATTR_S *pstCorrection)
{
    HI_U8               i = 0;
    HI_U32              u32Iso;
    HI_U32              u32Gain = 0;
    HI_S32              s32Ret = HI_SUCCESS;
    VI_FPN_ATTR_S       stFpnAttr;
    VI_FPN_TYPE_E       enFpnType;
    ISP_CTX_S           *pstIspCtx = HI_NULL;
    const ISP_FPN_FRAME_INFO_S *pstFPNFrmInfo = HI_NULL;
    ISP_REGCFG_S        *pRegCfg = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    REGCFG_GET_CTX(ViPipe, pRegCfg);

    s32Ret = ISP_CheckCorrectionAttr(ViPipe, pstCorrection);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    hi_ext_system_manual_fpn_opmode_write(ViPipe, pstCorrection->enOpType);

    if (OP_TYPE_AUTO == pstCorrection->enOpType)
    {
        u32Iso = hi_ext_system_fpn_sensor_iso_read(ViPipe);
        hi_ext_system_manual_fpn_update_write(ViPipe, 0x1);
        hi_ext_system_manual_fpn_iso_write(ViPipe, pstCorrection->stFpnFrmInfo.u32Iso);

        u32Gain     = ISP_Fpn_GetStrength(u32Iso, 0, 256, 0, pstCorrection->stFpnFrmInfo.u32Iso);
    }
    else
    {
        u32Gain     = pstCorrection->stManual.u32Strength;
    }
    u32Gain         = ISP_FPN_CLIP(0, 1023, u32Gain);

    /* Config FPN WDR mode correction threshold */
    for (i = 0; i < pRegCfg->stRegCfg.u8CfgNum; i++)
    {
        if (IS_WDR_MODE(pstIspCtx->u8PreSnsWDRMode) && (OP_TYPE_AUTO == pstCorrection->enOpType))
        {
            pRegCfg->stRegCfg.stAlgRegCfg[i].stFpnRegCfg.stDynaRegCfg.u32IspFpnOverflowThr = FPN_OVERFLOWTHR;
        }
        else
        {
            pRegCfg->stRegCfg.stAlgRegCfg[i].stFpnRegCfg.stDynaRegCfg.u32IspFpnOverflowThr = FPN_OVERFLOWTHR_OFF;
        }
    }

    /* Offline Mode special handle */
    if (ISP_MODE_RUNNING_OFFLINE == pstIspCtx->stBlockAttr.enIspRunningMode ||
        ISP_MODE_RUNNING_STRIPING == pstIspCtx->stBlockAttr.enIspRunningMode)
    {
        ISP_OfflineCorrection(ViPipe, pRegCfg, pstCorrection, u32Gain);
    }

    enFpnType           = pstCorrection->enFpnType;
    pstFPNFrmInfo       = &pstCorrection->stFpnFrmInfo;
    stFpnAttr.enFpnWorkMode             = FPN_MODE_CORRECTION;
    stFpnAttr.enFpnType                 = enFpnType;
    stFpnAttr.stCorrection.bEnable      = pstCorrection->bEnable;
    stFpnAttr.stCorrection.u32Gain      = u32Gain;
    stFpnAttr.stCorrection.enFpnOpType  = pstCorrection->enOpType;
    hi_ext_system_manual_fpn_strength_write(ViPipe, u32Gain);

    memcpy(&stFpnAttr.stCorrection.stFpnFrmInfo, pstFPNFrmInfo,
           sizeof(ISP_FPN_FRAME_INFO_S));

    s32Ret = MPI_VI_SetFPNAttr(ViPipe, &stFpnAttr);
    if (HI_SUCCESS == s32Ret)
    {
        hi_ext_system_manual_fpn_CorrCfg_write(ViPipe, HI_TRUE);
    }
    else
    {
        hi_ext_system_manual_fpn_CorrCfg_write(ViPipe, HI_FALSE);
    }

    if (ISP_MODE_RUNNING_OFFLINE == pstIspCtx->stBlockAttr.enIspRunningMode ||
        ISP_MODE_RUNNING_STRIPING == pstIspCtx->stBlockAttr.enIspRunningMode)
    {
        isp_fpn_en_write(ViPipe, 0, pstCorrection->bEnable);
        isp_dcg_fpn_sel(ViPipe, 0, pstCorrection->bEnable);
    }

    return s32Ret;
}

HI_S32 ISP_GetCorrectionAttr(VI_PIPE ViPipe, ISP_FPN_ATTR_S *pstCorrection)
{
    HI_U8 u8BlkDev = 0;
    HI_U8 index = 8;
    HI_S32 s32Ret = HI_SUCCESS;
    VI_FPN_ATTR_S stTempViFpnAttr;
    ISP_FPN_FRAME_INFO_S *pstFPNFrmInfo;

    if (0 == isp_ext_system_manual_fpn_CorrCfg_read(ViPipe))
    {
        return HI_ERR_ISP_ATTR_NOT_CFG;
    }

    s32Ret =  MPI_VI_GetFPNAttr(ViPipe, &stTempViFpnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    pstCorrection->bEnable         = stTempViFpnAttr.stCorrection.bEnable;
    pstCorrection->enOpType        = stTempViFpnAttr.stCorrection.enFpnOpType;
    pstCorrection->enFpnType       = stTempViFpnAttr.enFpnType;
    pstCorrection->stManual.u32Strength = hi_ext_system_manual_fpn_strength_read(ViPipe);
    /* auto.strength is equal to fpn gain, high 16 bits: gain */
    pstCorrection->stAuto.u32Strength = (isp_fpn_corr_gainoffset_read(ViPipe, u8BlkDev, index) >> 16);

    /* fpn frame info copy */
    pstFPNFrmInfo = &pstCorrection->stFpnFrmInfo;
    memcpy(pstFPNFrmInfo, &stTempViFpnAttr.stCorrection.stFpnFrmInfo, sizeof(ISP_FPN_FRAME_INFO_S));

    return HI_SUCCESS;
}

static HI_VOID FPNExtRegsDefault(VI_PIPE ViPipe)
{
    hi_ext_system_fpn_sensor_iso_write(ViPipe, HI_EXT_SYSTEM_FPN_SENSOR_ISO_DEFAULT);
    hi_ext_system_manual_fpn_iso_write(ViPipe, HI_EXT_SYSTEM_FPN_MANU_ISO_DEFAULT);

    return;
}

static HI_VOID FPNRegsDefault(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8 i;
    ISP_FPN_S        *pstFpnCtx    = HI_NULL;

    FPN_GET_CTX(ViPipe, pstFpnCtx);
    for (i = 0; i < 4; i++)
    {
        pstFpnCtx->s32Iso[i] = 0;
    }
    pstFpnCtx->u32IspIntCnt = 3;

    hi_ext_system_manual_fpn_CorrCfg_write(ViPipe, HI_EXT_SYSTEM_FPN_MANU_CORRCFG_DEFAULT);
    hi_ext_system_manual_fpn_strength_write(ViPipe, HI_EXT_SYSTEM_FPN_STRENGTH_DEFAULT);
    hi_ext_system_manual_fpn_opmode_write(ViPipe, HI_EXT_SYSTEM_FPN_OPMODE_DEFAULT);
    hi_ext_system_manual_fpn_update_write(ViPipe, HI_EXT_SYSTEM_FPN_MANU_UPDATE_DEFAULT);

    pstRegCfg->unKey.bit1FpnCfg = 1;

    return;
}

static HI_VOID FPNExtRegsInitialize(VI_PIPE ViPipe)
{
    return;
}

static HI_VOID FPNRegsInitialize(VI_PIPE ViPipe)
{
    return;
}

static HI_S32 FPNReadExtregs(VI_PIPE ViPipe)
{
    return 0;
}

HI_S32 FPNUpdateExtRegs(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    hi_ext_system_fpn_sensor_iso_write(ViPipe, pstIspCtx->stLinkage.u32SensorIso);
    //isp_fpn_en_write(ViPipe, i, pstDynaRegCfg->bIspFpnEnable);

    return 0;
}

HI_VOID IspSetStrength(VI_PIPE ViPipe)
{
    ISP_CTX_S        *pstIspCtx                 = HI_NULL;
    HI_U32           u32Iso, u32Gain, Offfset, i;
    HI_U32           u32CalibrateIso;
    HI_U32           u32GainOffset[FPN_CHN_NUM];
    HI_U8            u8FpnOpMode, u8FpnEn;
    HI_U8            u8BlkDev                   = 0; /* BE Num, for SBS mode, BE0 & BE1 is the same FPN config, so just 0 is ok */
    HI_U8            u8ViSBSModeEn              = 0; /* SBS mode ,or not  */
    ISP_FPN_S        *pstFpnCtx                 = HI_NULL;
    ISP_REGCFG_S     *pRegCfg                   = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    FPN_GET_CTX(ViPipe, pstFpnCtx);

    if (ISP_MODE_RUNNING_SIDEBYSIDE == pstIspCtx->stBlockAttr.enIspRunningMode ||
        ISP_MODE_RUNNING_STRIPING == pstIspCtx->stBlockAttr.enIspRunningMode)
    {
        u8ViSBSModeEn = 1;
    }

    /* calculate every 120 interrupts */
    if (0 != pstIspCtx->u32FrameCnt % pstFpnCtx->u32IspIntCnt) /* is ok? */
    {
        return ;
    }

    u8FpnOpMode = hi_ext_system_manual_fpn_opmode_read(ViPipe);
    if (OP_TYPE_MANUAL == u8FpnOpMode)
    {
        return ;
    }

    //u32Iso = pstIspCtx->stLinkage.u32SensorIso;
    u32Iso = hi_ext_system_fpn_sensor_iso_read(ViPipe);
    u8FpnEn = isp_fpn_en_read(ViPipe, u8BlkDev);

    if (u8FpnEn && ((pstFpnCtx->s32Iso[ViPipe] != u32Iso) || hi_ext_system_manual_fpn_update_read(ViPipe)))
    {
        if (hi_ext_system_manual_fpn_update_read(ViPipe))
        {
            hi_ext_system_manual_fpn_update_write(ViPipe, 0);
        }

        u32CalibrateIso = hi_ext_system_manual_fpn_iso_read(ViPipe);
        u32Gain      = ISP_Fpn_GetStrength(u32Iso, 0 , 255, 0, u32CalibrateIso);
        u32Gain      = ISP_FPN_CLIP(0, 1023, u32Gain);
        hi_ext_system_manual_fpn_strength_write(ViPipe, u32Gain);

        for (i = 0; i < FPN_CHN_NUM; i++)
        {
            u32GainOffset[i]  = isp_fpn_corr_gainoffset_read(ViPipe, u8BlkDev, i);
            Offfset           = u32GainOffset[i] & 0xFFF;
            u32GainOffset[i]  = (u32Gain << 16) + Offfset;
            pRegCfg->stRegCfg.stAlgRegCfg[0].stFpnRegCfg.stDynaRegCfg.u32IspFpnOffset[i] = Offfset;
            pRegCfg->stRegCfg.stAlgRegCfg[0].stFpnRegCfg.stDynaRegCfg.u32IspFpnStrength[i] = u32Gain;

            if (u8ViSBSModeEn)
            {
                u32GainOffset[i]  = isp_fpn_corr_gainoffset_read(ViPipe, u8BlkDev + 1, i);
                Offfset           = u32GainOffset[i] & 0xFFF;
                u32GainOffset[i]  = (u32Gain << 16) + Offfset;
                pRegCfg->stRegCfg.stAlgRegCfg[1].stFpnRegCfg.stDynaRegCfg.u32IspFpnOffset[i] = Offfset;
                pRegCfg->stRegCfg.stAlgRegCfg[1].stFpnRegCfg.stDynaRegCfg.u32IspFpnStrength[i] = u32Gain;
            }
        }

        //printf("\nIspSetStrength: u32Iso =%d, 32Strength= %d, u8FpnEn=%d\n",u32Iso, u32Gain, u8FpnEn);
        pstFpnCtx->s32Iso[ViPipe] = u32Iso;
        pRegCfg->stRegCfg.unKey.bit1FpnCfg = 1;
    }
    else
    {
        return ;
    }
}

HI_S32 ISP_FPNInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    FPNRegsDefault(ViPipe, (ISP_REG_CFG_S *)pRegCfg);
    FPNExtRegsDefault(ViPipe);
    FPNReadExtregs(ViPipe);
    FPNRegsInitialize(ViPipe);
    FPNExtRegsInitialize(ViPipe);

    return HI_SUCCESS;
}

HI_S32 ISP_FPNRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                  HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    FPNUpdateExtRegs(ViPipe);
    IspSetStrength(ViPipe);

    return HI_SUCCESS;
}

HI_S32 FpnProcWrite(VI_PIPE ViPipe, ISP_CTRL_PROC_WRITE_S *pstProc)
{
    HI_U8 u8BlkDev = 0;
    ISP_CTRL_PROC_WRITE_S stProcTmp;
    HI_U32 u32Offset;
    HI_U32 u32Strength;

    if ((HI_NULL == pstProc->pcProcBuff)
        || (0 == pstProc->u32BuffLen))
    {
        return HI_FAILURE;
    }

    stProcTmp.pcProcBuff = pstProc->pcProcBuff;
    stProcTmp.u32BuffLen = pstProc->u32BuffLen;

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "-----FPN INFO------------------------------------------------------------------\n");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%8s" "%7s"     "%9s"       "%8s",
                    "En", "OpType", "Strength", "Offset\n");

    u32Offset = isp_fpn_corr_gainoffset_read(ViPipe, u8BlkDev, 0) & 0xfff;
    u32Strength = (isp_fpn_corr_gainoffset_read(ViPipe, u8BlkDev, 0) >> 16) & 0xffff;

    if (isp_fpn_en_read(ViPipe, u8BlkDev))
    {
        ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                        "%8d" "%4d"  "%9d"  "%8d",
                        isp_fpn_en_read(ViPipe, u8BlkDev),
                        hi_ext_system_manual_fpn_opmode_read(ViPipe),
                        u32Strength,
                        u32Offset);
    }
    else
    {
        ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                        "%8d" "%4s"  "%9s"  "%8s\n",
                        isp_fpn_en_read(ViPipe, u8BlkDev),
                        "--",
                        "--",
                        "--");
    }

    pstProc->u32WriteLen += 1;

    return HI_SUCCESS;
}

HI_S32 ISP_FPNCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    switch (u32Cmd)
    {
        case ISP_PROC_WRITE:
            FpnProcWrite(ViPipe, (ISP_CTRL_PROC_WRITE_S *)pValue);
            break;

        default :
            break;
    }
    return HI_SUCCESS;
}

HI_S32 ISP_FPNExit(VI_PIPE ViPipe)
{
    //pstRegCfg->unKey2.bit1FpnCfg = 1;
    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterFPN(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_FPN;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_FPNInit;
    pstAlgs->stAlgFunc.pfn_alg_run = ISP_FPNRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_FPNCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_FPNExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

