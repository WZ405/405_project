/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_awb.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/01/05
  Description   :
  History       :
  1.Date        : 2013/01/05
    Author      :
    Modification: Created file

******************************************************************************/

#include <stdio.h>
#include "isp_alg.h"
#include "isp_ext_config.h"
#include "isp_config.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef MIN
#define MIN(a, b) (((a) > (b)) ?  (b) : (a))
#endif

/* Convert to direct presentation (used in the ISP) */
static HI_U16 AwbComplementToDirect(HI_S16 s16Value)
{
    HI_U16 u16Result;

    if (s16Value >= 0)
    {
        u16Result = s16Value;
    }
    else
    {
        u16Result = -s16Value;
        u16Result |= (1 << 15);
    }

    return u16Result;
}

/* Convert to complement presentation (used in the firmware for calculations) */
static HI_S16 AwbDirectToComplement(HI_U16 u16Value)
{
    HI_S16 s16Result;

    s16Result = u16Value & (~(1 << 15));

    if (u16Value &  (1 << 15))
    {
        s16Result = - s16Result;
    }

    return s16Result;
}

/* Multiply the two matrixes a1[dim1 x dim2] a2[dim2 x dim3] */
static HI_VOID AwbMatrixMultiply(HI_S16 *ps16Matrix1, HI_S16 *ps16Matrix2,
                                 HI_S16 *ps16Result, HI_S32 s32Dim1, HI_S32 s32Dim2, HI_S32 s32Dim3)
{
    HI_S32 i, j, k;
    HI_S32 s32Temp;

    for (i = 0; i < s32Dim1; ++i)
    {
        for (j = 0; j < s32Dim3; ++j)
        {
            s32Temp = 0;

            for (k = 0; k < s32Dim2; ++k)
            {
                s32Temp += (((HI_S32)ps16Matrix1[i * s32Dim2 + k] * ps16Matrix2[k * s32Dim3 + j]));
            }

            ps16Result[i * s32Dim3 + j] = (HI_S16)((s32Temp + 128) >> 8);
        }
    }

    for (i = 0; i < s32Dim1; ++i)
    {
        s32Temp = 0;

        for (j = 0; j < s32Dim3; ++j)
        {
            s32Temp += (HI_S32)ps16Result[i * s32Dim3 + j];
        }

        if (0x0 != s32Temp)
        {
            for (j = 0; j < s32Dim3; ++j)
            {
                ps16Result[i * s32Dim3 + j] = (HI_S16)(ps16Result[i * s32Dim3 + j] * 0x100 / DIV_0_TO_1(s32Temp));
            }
        }
    }

    for (i = 0; i < s32Dim1; ++i)
    {
        s32Temp = 0;

        for (j = 0; j < s32Dim3; ++j)
        {
            s32Temp += (HI_S16)ps16Result[i * s32Dim3 + j];
        }

        if (0x100 != s32Temp)
        {
            ps16Result[i * s32Dim3 + i] += (0x100 - s32Temp);
        }
    }

    return;
}

static HI_VOID AwbResRegsDefault(VI_PIPE ViPipe, HI_U8 i, HI_U8 u8BlockNum, ISP_AWB_REG_DYN_CFG_S  *pstAwbRegDynCfg)
{
    HI_U16 u16Overlap;
    HI_U32 u32Width, u32Height;
    ISP_RECT_S stBlockRect;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    //BEDYNAMIC
    ISP_GetBlockRect(&stBlockRect, &pstIspCtx->stBlockAttr, i);
    pstAwbRegDynCfg->u32BEWidth = stBlockRect.u32Width;
    pstAwbRegDynCfg->u32BEHeight = stBlockRect.u32Height;

    u16Overlap = pstIspCtx->stBlockAttr.u32OverLap;

    //awb overlap configs
    if (i == 0)
    {
        if (u8BlockNum > 1)
        {
            pstAwbRegDynCfg->u32BECropPosX = 0;
            pstAwbRegDynCfg->u32BECropPosY = 0;
            pstAwbRegDynCfg->u32BECropOutWidth = pstAwbRegDynCfg->u32BEWidth - u16Overlap;
            pstAwbRegDynCfg->u32BECropOutHeight = pstAwbRegDynCfg->u32BEHeight;
        }
        else
        {
            u32Width = hi_ext_sync_total_width_read(ViPipe);
            u32Height = hi_ext_sync_total_height_read(ViPipe);
            pstAwbRegDynCfg->u32BECropPosX = 0;
            pstAwbRegDynCfg->u32BECropPosY = 0;
            pstAwbRegDynCfg->u32BECropOutHeight = u32Height;
            pstAwbRegDynCfg->u32BECropOutWidth = u32Width;
        }
    }
    else if (i == (u8BlockNum - 1))
    {
        pstAwbRegDynCfg->u32BECropPosX = u16Overlap;
        pstAwbRegDynCfg->u32BECropPosY = 0;
        pstAwbRegDynCfg->u32BECropOutWidth  = pstAwbRegDynCfg->u32BEWidth - u16Overlap;
        pstAwbRegDynCfg->u32BECropOutHeight = pstAwbRegDynCfg->u32BEHeight;
    }
    else
    {
        pstAwbRegDynCfg->u32BECropPosX = u16Overlap;
        pstAwbRegDynCfg->u32BECropPosY = 0;
        pstAwbRegDynCfg->u32BECropOutWidth = pstAwbRegDynCfg->u32BEWidth - (u16Overlap << 1);
        pstAwbRegDynCfg->u32BECropOutHeight = pstAwbRegDynCfg->u32BEHeight;
    }
}

static HI_VOID AwbImageModeSet(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8 i;

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        AwbResRegsDefault(ViPipe, i, pstRegCfg->u8CfgNum, &pstRegCfg->stAlgRegCfg[i].stAwbRegCfg.stAwbRegDynCfg);
    }

    pstRegCfg->unKey.bit1AwbDynCfg = 1;
}

static HI_VOID AwbRegsDefault(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg, HI_U8 u8AWBZoneRow, HI_U8 u8AWBZoneCol, HI_U8 u8AWBZoneBin)
{
    HI_U8 i = 0;
    HI_U8 u8BlockNum = 0;
    ISP_AWB_REG_DYN_CFG_S  *pstAwbRegDynCfg = HI_NULL;
    ISP_AWB_REG_STA_CFG_S  *pstAwbRegStaCfg = HI_NULL;
    ISP_AWB_REG_USR_CFG_S  *pstAwbRegUsrCfg = HI_NULL;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    u8BlockNum = pstIspCtx->stBlockAttr.u8BlockNum;

    pstAwbRegDynCfg = &pstRegCfg->stAlgRegCfg[0].stAwbRegCfg.stAwbRegDynCfg;//dynamic
    pstAwbRegStaCfg = &pstRegCfg->stAlgRegCfg[0].stAwbRegCfg.stAwbRegStaCfg;//static
    pstAwbRegUsrCfg = &pstRegCfg->stAlgRegCfg[0].stAwbRegCfg.stAwbRegUsrCfg;//user

    //FE DYNAMIC
    pstAwbRegDynCfg->au32FEWhiteBalanceGain[0] = 0x1ED;
    pstAwbRegDynCfg->au32FEWhiteBalanceGain[1] = 0x100;
    pstAwbRegDynCfg->au32FEWhiteBalanceGain[2] = 0x100;
    pstAwbRegDynCfg->au32FEWhiteBalanceGain[3] = 0x1AB;
    pstAwbRegDynCfg->u8FEWbWorkEn = HI_TRUE;

    //FE STATIC
    pstAwbRegStaCfg->bFEAwbStaCfg   = 1;
    pstAwbRegStaCfg->u8FEWbBlcInEn  = HI_FALSE;
    pstAwbRegStaCfg->u8FEWbBlcOutEn = HI_FALSE;

    pstAwbRegStaCfg->u32FEClipValue = 0xFFFFF;
    pstAwbRegStaCfg->u16FEWbBlcOffset1 = 0x0;
    pstAwbRegStaCfg->u16FEWbBlcOffset2 = 0x0;
    pstAwbRegStaCfg->u16FEWbBlcOffset3 = 0x0;
    pstAwbRegStaCfg->u16FEWbBlcOffset4 = 0x0;

    pstRegCfg->unKey.bit1AwbDynCfg = 1;

    for (i = 0; i < u8BlockNum; i++)
    {
        pstAwbRegStaCfg = &(pstRegCfg->stAlgRegCfg[i].stAwbRegCfg.stAwbRegStaCfg);//static
        pstAwbRegDynCfg = &(pstRegCfg->stAlgRegCfg[i].stAwbRegCfg.stAwbRegDynCfg);//dynamic
        pstAwbRegUsrCfg = &(pstRegCfg->stAlgRegCfg[i].stAwbRegCfg.stAwbRegUsrCfg);//user

        //BE STATIC
        pstAwbRegStaCfg->bBEAwbStaCfg   = 1;
        pstAwbRegStaCfg->u8BEAwbBitmove = 0x0;
        pstAwbRegStaCfg->u8BEWbWorkEn   = HI_TRUE;
        pstAwbRegStaCfg->u8BEAwbWorkEn  = HI_TRUE;
        pstAwbRegStaCfg->u8BEWbBlcInEn  = HI_FALSE;
        pstAwbRegStaCfg->u8BEWbBlcOutEn = HI_FALSE;

        pstAwbRegStaCfg->u32BEAwbStatRaddr = 0x000;
        pstAwbRegStaCfg->u8BECcColortoneEn = 0x0;

        pstAwbRegStaCfg->u16BEWbBlcOffset1 = 0x0;
        pstAwbRegStaCfg->u16BEWbBlcOffset2 = 0x0;
        pstAwbRegStaCfg->u16BEWbBlcOffset3 = 0x0;
        pstAwbRegStaCfg->u16BEWbBlcOffset4 = 0x0;

        pstAwbRegStaCfg->u32BETopK = 0x0;
        pstAwbRegStaCfg->u32BETopB = 0xFFFFF;
        pstAwbRegStaCfg->u32BEBotK = 0x0;
        pstAwbRegStaCfg->u32BEBotB = 0x0;

        pstAwbRegStaCfg->u32BECcInDc0  = 0x0;
        pstAwbRegStaCfg->u32BECcInDc1  = 0x0;
        pstAwbRegStaCfg->u32BECcInDc2  = 0x0;
        pstAwbRegStaCfg->u32BECcOutDc0 = 0x0;
        pstAwbRegStaCfg->u32BECcOutDc1 = 0x0;
        pstAwbRegStaCfg->u32BECcOutDc2 = 0x0;
        pstAwbRegStaCfg->u32BEWbClipValue = 0xFFFFF;
        pstAwbRegStaCfg->u16BEAwbOffsetComp = HI_ISP_AWB_OFFSET_COMP_DEF;

        AwbResRegsDefault(ViPipe, i, u8BlockNum, pstAwbRegDynCfg);

        if (i < u8AWBZoneCol % DIV_0_TO_1(u8BlockNum))
        {
            pstAwbRegUsrCfg->u16BEZoneCol = u8AWBZoneCol / DIV_0_TO_1(u8BlockNum) + 1;
        }
        else
        {
            pstAwbRegUsrCfg->u16BEZoneCol = u8AWBZoneCol / DIV_0_TO_1(u8BlockNum);
        }

        pstAwbRegDynCfg->au16BEColorMatrix[0] = 0x100;
        pstAwbRegDynCfg->au16BEColorMatrix[1] = 0x0;
        pstAwbRegDynCfg->au16BEColorMatrix[2] = 0x0;
        pstAwbRegDynCfg->au16BEColorMatrix[3] = 0x0;
        pstAwbRegDynCfg->au16BEColorMatrix[4] = 0x100;
        pstAwbRegDynCfg->au16BEColorMatrix[5] = 0x0;
        pstAwbRegDynCfg->au16BEColorMatrix[6] = 0x0;
        pstAwbRegDynCfg->au16BEColorMatrix[7] = 0x0;
        pstAwbRegDynCfg->au16BEColorMatrix[8] = 0x100;
        pstAwbRegDynCfg->au32BEWhiteBalanceGain[0] = 0x1ED;
        pstAwbRegDynCfg->au32BEWhiteBalanceGain[1] = 0x100;
        pstAwbRegDynCfg->au32BEWhiteBalanceGain[2] = 0x100;
        pstAwbRegDynCfg->au32BEWhiteBalanceGain[3] = 0x1AB;

        pstAwbRegDynCfg->u16BEMeteringWhiteLevelAwb = 0xffff;
        pstAwbRegDynCfg->u16BEMeteringBlackLevelAwb = 0x0;
        pstAwbRegDynCfg->u16BEMeteringCrRefMaxAwb = 0x180;
        pstAwbRegDynCfg->u16BEMeteringCrRefMinAwb = 0x40;
        pstAwbRegDynCfg->u16BEMeteringCbRefMaxAwb = 0x180;
        pstAwbRegDynCfg->u16BEMeteringCbRefMinAwb = 0x40;

        pstAwbRegDynCfg->u8BEWbWorkEn = HI_TRUE;
        pstAwbRegDynCfg->u8BECcEn = HI_TRUE;

        pstAwbRegDynCfg->u16BECcColortoneEn = HI_ISP_CCM_COLORTONE_EN_DEFAULT;
        pstAwbRegDynCfg->u16BECcRGain = HI_ISP_CCM_COLORTONE_RGAIN_DEFAULT;
        pstAwbRegDynCfg->u16BECcGGain = HI_ISP_CCM_COLORTONE_GGAIN_DEFAULT;
        pstAwbRegDynCfg->u16BECcBGain = HI_ISP_CCM_COLORTONE_GGAIN_DEFAULT;

        pstAwbRegUsrCfg->u16BEZoneRow = u8AWBZoneRow;
        pstAwbRegUsrCfg->u16BEZoneBin = u8AWBZoneBin;
        pstAwbRegUsrCfg->u16BEMeteringBinHist0 = 0xffff;
        pstAwbRegUsrCfg->u16BEMeteringBinHist1 = 0xffff;
        pstAwbRegUsrCfg->u16BEMeteringBinHist2 = 0xffff;
        pstAwbRegUsrCfg->u16BEMeteringBinHist3 = 0xffff;
        pstAwbRegUsrCfg->enBEAWBSwitch = ISP_AWB_AFTER_DG;
        pstAwbRegUsrCfg->bResh = HI_TRUE;
        pstAwbRegUsrCfg->u32UpdateIndex = 1;

        hi_ext_system_cc_enable_write(ViPipe, pstAwbRegDynCfg->u8BECcEn);
        hi_ext_system_awb_gain_enable_write(ViPipe, pstAwbRegDynCfg->u8BEWbWorkEn);
        hi_ext_system_awb_white_level_write(ViPipe, pstAwbRegDynCfg->u16BEMeteringWhiteLevelAwb);
        hi_ext_system_awb_black_level_write(ViPipe, pstAwbRegDynCfg->u16BEMeteringBlackLevelAwb);
        hi_ext_system_awb_cr_ref_max_write(ViPipe, pstAwbRegDynCfg->u16BEMeteringCrRefMaxAwb);
        hi_ext_system_awb_cr_ref_min_write(ViPipe, pstAwbRegDynCfg->u16BEMeteringCrRefMinAwb);
        hi_ext_system_awb_cb_ref_max_write(ViPipe, pstAwbRegDynCfg->u16BEMeteringCbRefMaxAwb);
        hi_ext_system_awb_cb_ref_min_write(ViPipe, pstAwbRegDynCfg->u16BEMeteringCbRefMinAwb);
        hi_ext_system_cc_colortone_rgain_write(ViPipe, pstAwbRegDynCfg->u16BECcRGain);
        hi_ext_system_cc_colortone_ggain_write(ViPipe, pstAwbRegDynCfg->u16BECcGGain);
        hi_ext_system_cc_colortone_bgain_write(ViPipe, pstAwbRegDynCfg->u16BECcBGain);

        //User
        hi_ext_system_wb_statistics_mpi_update_en_write(ViPipe, pstAwbRegUsrCfg->bResh);
        hi_ext_system_awb_switch_write(ViPipe, pstAwbRegUsrCfg->enBEAWBSwitch);
        hi_ext_system_awb_hnum_write(ViPipe, u8AWBZoneCol);//the col num of the whole picture
        hi_ext_system_awb_vnum_write(ViPipe, pstAwbRegUsrCfg->u16BEZoneRow);
        hi_ext_system_awb_zone_bin_write(ViPipe, pstAwbRegUsrCfg->u16BEZoneBin);
        hi_ext_system_awb_hist_bin_thresh0_write(ViPipe, pstAwbRegUsrCfg->u16BEMeteringBinHist0);
        hi_ext_system_awb_hist_bin_thresh1_write(ViPipe, pstAwbRegUsrCfg->u16BEMeteringBinHist1);
        hi_ext_system_awb_hist_bin_thresh2_write(ViPipe, pstAwbRegUsrCfg->u16BEMeteringBinHist2);
        hi_ext_system_awb_hist_bin_thresh3_write(ViPipe, pstAwbRegUsrCfg->u16BEMeteringBinHist3);
    }

    return;
}

HI_S32 AwbCfgReg(VI_PIPE ViPipe, ISP_AWB_RESULT_S *pstAwbResult, HI_U8 u8WDRMode,
                 HI_U32 u32IspDgain, HI_U32 u32IspDgainShift, ISP_REG_CFG_S *pstRegCfg/*, HI_U8 u8BlockNum*/)
{
    HI_BOOL bUsrResh;
    HI_S32 i, k;
    HI_U32  au32WbGain[4];
    HI_U8 u8BlkNum;
    ISP_AWB_REG_DYN_CFG_S  *pstAwbRegDynCfg = HI_NULL;
    ISP_AWB_REG_USR_CFG_S  *pstAwbRegUsrCfg = HI_NULL;
    u8BlkNum = ((ISP_REG_CFG_S *)pstRegCfg)->u8CfgNum;

    bUsrResh = hi_ext_system_wb_statistics_mpi_update_en_read(ViPipe);
    hi_ext_system_wb_statistics_mpi_update_en_write(ViPipe, HI_FALSE);

    for (k = 0; k < u8BlkNum; k++)
    {

        pstAwbRegDynCfg = &(pstRegCfg->stAlgRegCfg[k].stAwbRegCfg.stAwbRegDynCfg);//dynamic
        pstAwbRegUsrCfg = &(pstRegCfg->stAlgRegCfg[k].stAwbRegCfg.stAwbRegUsrCfg);//user

        if (pstAwbResult->stRawStatAttr.bStatCfgUpdate)//CbCr
        {
            hi_ext_system_awb_cr_ref_max_write(ViPipe, MIN(pstAwbResult->stRawStatAttr.u16MeteringCrRefMaxAwb, 0xFFF));
            hi_ext_system_awb_cr_ref_min_write(ViPipe, MIN(pstAwbResult->stRawStatAttr.u16MeteringCrRefMinAwb, 0xFFF));
            hi_ext_system_awb_cb_ref_max_write(ViPipe, MIN(pstAwbResult->stRawStatAttr.u16MeteringCbRefMaxAwb, 0xFFF));
            hi_ext_system_awb_cb_ref_min_write(ViPipe, MIN(pstAwbResult->stRawStatAttr.u16MeteringCbRefMinAwb, 0xFFF));
            hi_ext_system_awb_white_level_write(ViPipe, MIN(pstAwbResult->stRawStatAttr.u16MeteringWhiteLevelAwb, 0xFFFF));
        }

        pstAwbRegDynCfg->u16BEMeteringCrRefMaxAwb = hi_ext_system_awb_cr_ref_max_read(ViPipe);
        pstAwbRegDynCfg->u16BEMeteringCrRefMinAwb = hi_ext_system_awb_cr_ref_min_read(ViPipe);
        pstAwbRegDynCfg->u16BEMeteringCbRefMaxAwb = hi_ext_system_awb_cb_ref_max_read(ViPipe);
        pstAwbRegDynCfg->u16BEMeteringCbRefMinAwb = hi_ext_system_awb_cb_ref_min_read(ViPipe);
        pstAwbRegDynCfg->u16BEMeteringWhiteLevelAwb = hi_ext_system_awb_white_level_read(ViPipe);
        pstAwbRegDynCfg->u16BEMeteringBlackLevelAwb = hi_ext_system_awb_black_level_read(ViPipe);

        for (i = 0; i < 9; i++)
        {
            pstAwbRegDynCfg->au16BEColorMatrix[i] = pstAwbResult->au16ColorMatrix[i];
        }

        for (i = 0; i < 4; i++)
        {
            au32WbGain[i] = pstAwbResult->au32WhiteBalanceGain[i];
            au32WbGain[i] = (au32WbGain[i] + 128) >> 8;
            au32WbGain[i] = (au32WbGain[i] > 0xFFF) ? 0xFFF : au32WbGain[i];
            pstAwbRegDynCfg->au32BEWhiteBalanceGain[i] = (HI_U16)au32WbGain[i];
        }

        if (bUsrResh == HI_TRUE)
        {
            HI_U8 u8AWBZoneCol = hi_ext_system_awb_hnum_read(ViPipe);
            if (i < u8AWBZoneCol % DIV_0_TO_1(u8BlkNum))
            {
                pstAwbRegUsrCfg->u16BEZoneCol = u8AWBZoneCol / DIV_0_TO_1(u8BlkNum) + 1;
            }
            else
            {
                pstAwbRegUsrCfg->u16BEZoneCol = u8AWBZoneCol / DIV_0_TO_1(u8BlkNum);
            }
            pstAwbRegUsrCfg->u32UpdateIndex += 1;

            pstAwbRegUsrCfg->enBEAWBSwitch = hi_ext_system_awb_switch_read(ViPipe);
            pstAwbRegUsrCfg->u16BEZoneRow  = hi_ext_system_awb_vnum_read(ViPipe);
            pstAwbRegUsrCfg->u16BEZoneBin  = hi_ext_system_awb_zone_bin_read(ViPipe);

            pstAwbRegUsrCfg->u16BEMeteringBinHist0 = hi_ext_system_awb_hist_bin_thresh0_read(ViPipe);
            pstAwbRegUsrCfg->u16BEMeteringBinHist1 = hi_ext_system_awb_hist_bin_thresh1_read(ViPipe);
            pstAwbRegUsrCfg->u16BEMeteringBinHist2 = hi_ext_system_awb_hist_bin_thresh2_read(ViPipe);
            pstAwbRegUsrCfg->u16BEMeteringBinHist3 = hi_ext_system_awb_hist_bin_thresh3_read(ViPipe);
            pstAwbRegUsrCfg->bResh                 = HI_TRUE;

        }

        pstAwbRegDynCfg->u8BECcEn = hi_ext_system_cc_enable_read(ViPipe);
        pstAwbRegDynCfg->u16BECcBGain = hi_ext_system_cc_colortone_bgain_read(ViPipe);
        pstAwbRegDynCfg->u16BECcGGain = hi_ext_system_cc_colortone_ggain_read(ViPipe);
        pstAwbRegDynCfg->u16BECcRGain = hi_ext_system_cc_colortone_rgain_read(ViPipe);

        pstAwbRegDynCfg->u8BEWbWorkEn = hi_ext_system_awb_gain_enable_read(ViPipe);

        pstAwbRegDynCfg->u8FEWbWorkEn = hi_ext_system_awb_gain_enable_read(ViPipe);
    }


    //FE
    for (i = 0; i < 4; i++)
    {
        au32WbGain[i] = pstAwbResult->au32WhiteBalanceGain[i];
        au32WbGain[i] = (au32WbGain[i] + 128) >> 8;
        au32WbGain[i] = (au32WbGain[i] > 0xFFF) ? 0xFFF : au32WbGain[i];
        pstRegCfg->stAlgRegCfg[0].stAwbRegCfg.stAwbRegDynCfg.au32FEWhiteBalanceGain[i] = (HI_U16)au32WbGain[i];
    }

    pstRegCfg->unKey.bit1AwbDynCfg = 1;

    return HI_SUCCESS;
}
HI_S32 ISP_AwbCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue);
HI_S32 ISP_AwbInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    HI_S32 i;
    ISP_AWB_PARAM_S stAwbParam;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_LIB_NODE_S *pstLib = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    stAwbParam.SensorId = pstIspCtx->stBindAttr.SensorId;
    stAwbParam.u8WDRMode = pstIspCtx->u8SnsWDRMode;
    memcpy(&stAwbParam.stStitchAttr, &pstIspCtx->stStitchAttr, sizeof(ISP_STITCH_ATTR_S));

    if (1 == stAwbParam.stStitchAttr.bStitchEnable)
    {
        HI_U32 u32BlockWidth = AWB_ZONE_ORIG_COLUMN;

        //if ((stAwbParam.stStitchAttr.u8StitchPipeNum > 4) && (stAwbParam.stStitchAttr.u8StitchPipeNum <= 8))
        //{
        //    u32BlockWidth = AWB_ZONE_ORIG_COLUMN * 4 / DIV_0_TO_1(stAwbParam.stStitchAttr.u8StitchPipeNum);
        //}

        AwbRegsDefault(ViPipe, (ISP_REG_CFG_S *)pRegCfg, AWB_ZONE_ORIG_ROW, u32BlockWidth, 1);
        stAwbParam.u8AWBZoneRow = (HI_U8)(AWB_ZONE_ORIG_ROW);
        stAwbParam.u8AWBZoneCol = (HI_U8)(u32BlockWidth * stAwbParam.stStitchAttr.u8StitchPipeNum);
        stAwbParam.u8AWBZoneBin = 1;
    }
    else
    {
        AwbRegsDefault(ViPipe, (ISP_REG_CFG_S *)pRegCfg, AWB_ZONE_ORIG_ROW, AWB_ZONE_ORIG_COLUMN, 1);

        stAwbParam.u8AWBZoneRow = AWB_ZONE_ORIG_ROW;
        stAwbParam.u8AWBZoneCol = AWB_ZONE_ORIG_COLUMN;
        stAwbParam.u8AWBZoneBin = 1;
    }
    stAwbParam.u16AWBWidth  = hi_ext_sync_total_width_read(ViPipe);
    stAwbParam.u16AWBHeight = hi_ext_sync_total_height_read(ViPipe);

    for (i = 0; i < MAX_REGISTER_ALG_LIB_NUM; i++)
    {
        if (pstIspCtx->stAwbLibInfo.astLibs[i].bUsed)
        {
            pstLib = &pstIspCtx->stAwbLibInfo.astLibs[i];

            if (HI_NULL != pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_init)
            {
                pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_init(
                    pstLib->stAlgLib.s32Id, &stAwbParam);
            }
        }
    }

    return HI_SUCCESS;
}

HI_S32 ISP_AwbRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                  HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_S32 i, s32Ret = HI_FAILURE;
    ISP_AWB_INFO_S   stAwbInfo = {0};
    ISP_AWB_RESULT_S stAwbResult = {{0}};
    ISP_CTX_S       *pstIspCtx   = HI_NULL;
    ISP_LIB_NODE_S  *pstLib      = HI_NULL;
    HI_BOOL bLoadCCM;
    HI_U32 u32DiffGain;
    HI_S8 s8StitchMainPipe;
    AWB_CCM_CONFIG_S stCCMConf, stCCMConfDef;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    pstLib = &pstIspCtx->stAwbLibInfo.astLibs[pstIspCtx->stAwbLibInfo.u32ActiveLib];

    if (pstIspCtx->stLinkage.bDefectPixel)
    {
        return HI_SUCCESS;
    }

    bLoadCCM = pstIspCtx->stLinkage.bLoadCCM;

    if (HI_TRUE == pstIspCtx->stLinkage.bSnapState)
    {
        if (HI_TRUE == bLoadCCM)
        {
            return HI_SUCCESS;
        }
        else
        {
            pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_ctrl(pstLib->stAlgLib.s32Id,
                    AWB_CCM_CONFIG_GET, (HI_VOID *)&stCCMConfDef);
            stCCMConf.bAWBBypassEn = HI_TRUE;
            stCCMConf.bManualTempEn = HI_TRUE;
            stCCMConf.u32ManualTempValue = pstIspCtx->stLinkage.u32ColorTemp;
            stCCMConf.u16CCMSpeed = 0xfff;

            stCCMConf.bManualSatEn = stCCMConfDef.bManualSatEn;
            stCCMConf.u32ManualSatValue = stCCMConfDef.u32ManualSatValue;
            pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_ctrl(pstLib->stAlgLib.s32Id,
                    AWB_CCM_CONFIG_SET, (HI_VOID *)&stCCMConf);
        }
    }
    else
    {
        if (HI_FALSE == pstIspCtx->stLinkage.bStatReady)
        {
            return HI_SUCCESS;
        }
    }

    if ((IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
         || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))\
        && (ISP_SNAP_PICTURE == pstIspCtx->stLinkage.enSnapPipeMode))
    {
        return HI_SUCCESS;
    }

    /* get statistics */
    stAwbInfo.u32FrameCnt = pstIspCtx->u32FrameCnt;

    /* linkage with the iso of ae */
    for (i = 0; i < MAX_REGISTER_ALG_LIB_NUM; i++)
    {
        if (pstIspCtx->stAwbLibInfo.astLibs[i].bUsed)
        {
            pstLib = &pstIspCtx->stAwbLibInfo.astLibs[i];

            if (HI_NULL != pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_ctrl)
            {
                pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_ctrl(pstLib->stAlgLib.s32Id,
                        ISP_AWB_ISO_SET, (HI_VOID *)&pstIspCtx->stLinkage.u32Iso);
                pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_ctrl(pstLib->stAlgLib.s32Id,
                        ISP_AWB_INTTIME_SET, (HI_VOID *)&pstIspCtx->stLinkage.u32Inttime);
            }
        }
    }

    pstLib = &pstIspCtx->stAwbLibInfo.astLibs[pstIspCtx->stAwbLibInfo.u32ActiveLib];
    stAwbInfo.pstAwbStat1 = &((ISP_STAT_S *)pStatInfo)->stAwbStat1;
    stAwbInfo.stAwbStat2.pau16ZoneAvgR  = (((ISP_STAT_S *)pStatInfo)->stAwbStat2.au16MeteringMemArrayAvgR);
    stAwbInfo.stAwbStat2.pau16ZoneAvgG  = (((ISP_STAT_S *)pStatInfo)->stAwbStat2.au16MeteringMemArrayAvgG);
    stAwbInfo.stAwbStat2.pau16ZoneAvgB  = (((ISP_STAT_S *)pStatInfo)->stAwbStat2.au16MeteringMemArrayAvgB);
    stAwbInfo.stAwbStat2.pau16ZoneCount = (((ISP_STAT_S *)pStatInfo)->stAwbStat2.au16MeteringMemArrayCountAll);

    if (HI_TRUE == pstIspCtx->stStitchAttr.bStitchEnable)
    {
        s8StitchMainPipe = pstIspCtx->stStitchAttr.as8StitchBindId[0];

        if (IS_STITCH_MAIN_PIPE(ViPipe, s8StitchMainPipe))
        {
            stAwbInfo.stAwbStat2.pau16ZoneAvgR  = (((ISP_STAT_S *)pStatInfo)->stStitchStat.stAwbStat2.au16MeteringMemArrayAvgR);
            stAwbInfo.stAwbStat2.pau16ZoneAvgG  = (((ISP_STAT_S *)pStatInfo)->stStitchStat.stAwbStat2.au16MeteringMemArrayAvgG);
            stAwbInfo.stAwbStat2.pau16ZoneAvgB  = (((ISP_STAT_S *)pStatInfo)->stStitchStat.stAwbStat2.au16MeteringMemArrayAvgB);
            stAwbInfo.stAwbStat2.pau16ZoneCount = (((ISP_STAT_S *)pStatInfo)->stStitchStat.stAwbStat2.au16MeteringMemArrayCountAll);

            if (HI_NULL != pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_run)
            {
                s32Ret = pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_run(
                             pstLib->stAlgLib.s32Id, &stAwbInfo, &stAwbResult, 0);

                if (HI_SUCCESS != s32Ret)
                {
                    printf("WARNING!! run awb lib err 0x%x!\n", s32Ret);
                }
            }

            memcpy(&pstIspCtx->stAwbResult, &stAwbResult, sizeof(ISP_AWB_RESULT_S));
        }
        else
        {
            memcpy(&stAwbResult, &g_astIspCtx[s8StitchMainPipe].stAwbResult, sizeof(ISP_AWB_RESULT_S));
        }
    }
    else
    {
        if (HI_NULL != pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_run)
        {
            s32Ret = pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_run(
                         pstLib->stAlgLib.s32Id, &stAwbInfo, &stAwbResult, 0);

            if (HI_SUCCESS != s32Ret)
            {
                printf("WARNING!! run awb lib err 0x%x!\n", s32Ret);
            }
        }
    }

    pstIspCtx->stLinkage.u32ColorTemp = stAwbResult.u32ColorTemp;
    pstIspCtx->stLinkage.u8Saturation = stAwbResult.u8Saturation;
    pstIspCtx->stLinkage.au32WhiteBalanceGain[0] = stAwbResult.au32WhiteBalanceGain[0];
    pstIspCtx->stLinkage.au32WhiteBalanceGain[1] = stAwbResult.au32WhiteBalanceGain[1];
    pstIspCtx->stLinkage.au32WhiteBalanceGain[2] = stAwbResult.au32WhiteBalanceGain[2];
    pstIspCtx->stLinkage.au32WhiteBalanceGain[3] = stAwbResult.au32WhiteBalanceGain[3];

    for (i = 0; i < CCM_MATRIX_SIZE; i++)
    {
        pstIspCtx->stLinkage.au16CCM[i] = stAwbResult.au16ColorMatrix[i];
    }

    {
        pstIspCtx->stAttachInfoCtrl.pstAttachInfo->stIspHdr.u32ColorTemp = pstIspCtx->stLinkage.u32ColorTemp;
        memcpy(pstIspCtx->stAttachInfoCtrl.pstAttachInfo->stIspHdr.au16CCM, pstIspCtx->stLinkage.au16CCM, CCM_MATRIX_SIZE * sizeof(HI_U16));
        pstIspCtx->stAttachInfoCtrl.pstAttachInfo->stIspHdr.u8Saturation = pstIspCtx->stLinkage.u8Saturation;
        pstIspCtx->stAttachInfoCtrl.pstAttachInfo->u32ISO = pstIspCtx->stLinkage.u32Iso ;
        pstIspCtx->stAttachInfoCtrl.pstAttachInfo->u8SnsWDRMode = pstIspCtx->u8SnsWDRMode;

    }

    if (HI_TRUE == pstIspCtx->stStitchAttr.bStitchEnable)
    {
        HI_S16 as16CalcCCM[CCM_MATRIX_SIZE] = {0};
        HI_S16 as16DiffCCM[CCM_MATRIX_SIZE] = {0};
        HI_S16 as16ResuCCM[CCM_MATRIX_SIZE] = {0};

        /* TODO: Multi-Pipe different config WBgain/CCM */
        for (i = 0; i < ISP_BAYER_CHN_NUM; i++)
        {
            u32DiffGain = hi_ext_system_isp_pipe_diff_gain_read(ViPipe, i);
            stAwbResult.au32WhiteBalanceGain[i] = (stAwbResult.au32WhiteBalanceGain[i] * u32DiffGain) >> 8;
        }

        for (i = 0; i < CCM_MATRIX_SIZE; i++)
        {
            as16CalcCCM[i] = AwbDirectToComplement(stAwbResult.au16ColorMatrix[i]);
            as16DiffCCM[i] = hi_ext_system_isp_pipe_diff_ccm_read(ViPipe, i);
            as16DiffCCM[i] = AwbDirectToComplement(as16DiffCCM[i]);
        }

        AwbMatrixMultiply(as16CalcCCM, as16DiffCCM, as16ResuCCM, 3, 3, 3);

        for (i = 0; i < CCM_MATRIX_SIZE; i++)
        {
            stAwbResult.au16ColorMatrix[i] = AwbComplementToDirect(as16ResuCCM[i]);
        }
    }

    //for (i = 0; i < ((ISP_REG_CFG_S *)pRegCfg)->u8CfgNum; i++)

    AwbCfgReg(ViPipe, &stAwbResult, pstIspCtx->u8SnsWDRMode, pstIspCtx->stLinkage.u32IspDgain,
              pstIspCtx->stLinkage.u32IspDgainShift, (ISP_REG_CFG_S *)pRegCfg);

    if (HI_TRUE == pstIspCtx->stLinkage.bSnapState)
    {
        pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_ctrl(pstLib->stAlgLib.s32Id,
                AWB_CCM_CONFIG_SET, (HI_VOID *)&stCCMConfDef);
    }

    return s32Ret;
}

HI_S32 ISP_AwbCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    HI_S32  i, s32Ret = HI_FAILURE;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_LIB_NODE_S *pstLib = HI_NULL;
    ISP_REGCFG_S  *pstRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pstRegCfg);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstLib = &pstIspCtx->stAwbLibInfo.astLibs[pstIspCtx->stAwbLibInfo.u32ActiveLib];

    if (ISP_PROC_WRITE == u32Cmd)
    {
        if (pstLib->bUsed)
        {
            if (HI_NULL != pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_ctrl)
            {
                s32Ret = pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_ctrl(
                             pstLib->stAlgLib.s32Id, u32Cmd, pValue);
            }
        }
    }
    else
    {
        for (i = 0; i < MAX_REGISTER_ALG_LIB_NUM; i++)
        {
            if (pstIspCtx->stAwbLibInfo.astLibs[i].bUsed)
            {
                pstLib = &pstIspCtx->stAwbLibInfo.astLibs[i];
                if (HI_NULL != pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_ctrl)
                {
                    s32Ret = pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_ctrl(
                                 pstLib->stAlgLib.s32Id, u32Cmd, pValue);
                }
            }
        }
    }

    if (ISP_CHANGE_IMAGE_MODE_SET == u32Cmd)
    {
        AwbImageModeSet(ViPipe, &pstRegCfg->stRegCfg);
    }

    return s32Ret;
}

HI_S32 ISP_AwbExit(VI_PIPE ViPipe)
{
    HI_S32 i;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_LIB_NODE_S *pstLib = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    for (i = 0; i < MAX_REGISTER_ALG_LIB_NUM; i++)
    {
        if (pstIspCtx->stAwbLibInfo.astLibs[i].bUsed)
        {
            pstLib = &pstIspCtx->stAwbLibInfo.astLibs[i];

            if (HI_NULL != pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_exit)
            {
                pstLib->stAwbRegsiter.stAwbExpFunc.pfn_awb_exit(
                    pstLib->stAlgLib.s32Id);
            }
        }
    }

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterAwb(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_AWB;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_AwbInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_AwbRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_AwbCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_AwbExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

