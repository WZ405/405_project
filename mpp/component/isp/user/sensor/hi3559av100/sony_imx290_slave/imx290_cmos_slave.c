
/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : imx290_cmos_slave.c

  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/11/07
  Description   :
  History       :
  1.Date        : 2013/11/07
    Author      :
    Modification: Created file

******************************************************************************/
#if !defined(__IMX290_CMOS_SLAVE_H_)
#define __IMX290_CMOS_SLAVE_H_

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "hi_comm_sns.h"
#include "hi_comm_video.h"
#include "hi_sns_ctrl.h"
#include "mpi_isp.h"
#include "mpi_ae.h"
#include "mpi_awb.h"

#include "imx290_cmos_slave_ex.h"
#include "imx290_slave_priv.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


#define IMX290_SLAVE_ID 292
#define IMX290_VS_TIME_MAX 0xFFFFFFFF
#define IMX290_SLAVE_INCREASE_LINES (1) /* make real fps less than stand fps because NVR require*/

/****************************************************************************
 * global variables                                                            *
 ****************************************************************************/

const IMX290_VIDEO_MODE_TBL_S g_astImx290ModeTbl[IMX290_MODE_BUTT] =
{
    {74500000, 1103, 1241667, 1125 + IMX290_SLAVE_INCREASE_LINES, 60, "1080P_60FPS_RAW10_SLAVE_2LANE"},  /*Actually 59.94fps*/
};

ISP_SLAVE_SNS_SYNC_S  gstImx290Sync[ISP_MAX_PIPE_NUM];

/* Sensor slave mode, default binding setting : Slave[0]->Pipe[0]&[1]; Slave[1]->Pipe[2]&[3]; Slave[2]->Pipe[4]&[5] */
HI_S32 g_Imx290SlaveBindDev[ISP_MAX_PIPE_NUM] = {0, 0, 1, 1, 2, 2, 3, 3};

ISP_SNS_STATE_S             g_astImx290Slave[ISP_MAX_PIPE_NUM] = {{0}};
static ISP_SNS_STATE_S     *g_apstSnsState[ISP_MAX_PIPE_NUM] =
{
    &g_astImx290Slave[0],
    &g_astImx290Slave[1],
    &g_astImx290Slave[2],
    &g_astImx290Slave[3],
    &g_astImx290Slave[4],
    &g_astImx290Slave[5],
    &g_astImx290Slave[6],
    &g_astImx290Slave[7]
};
ISP_SNS_COMMBUS_U g_aunImx290SlaveBusInfo[ISP_MAX_PIPE_NUM] =
{
    [0] = { .s8I2cDev = 0},
    [1] = { .s8I2cDev = 1},
    [2] = { .s8I2cDev = 2},
    [3] = { .s8I2cDev = 3},
    [4] = { .s8I2cDev = 4},
    [5] = { .s8I2cDev = 5},
    [6] = { .s8I2cDev = 6},
    [7] = { .s8I2cDev = 7}
};

static HI_U32 g_au32InitExposure[ISP_MAX_PIPE_NUM]  = {0};
static HI_U32 g_au32LinesPer500ms[ISP_MAX_PIPE_NUM] = {0};

static HI_U16 g_au16InitWBGain[ISP_MAX_PIPE_NUM][3] = {{0}};
static HI_U16 g_au16SampleRgain[ISP_MAX_PIPE_NUM] = {0};
static HI_U16 g_au16SampleBgain[ISP_MAX_PIPE_NUM] = {0};

extern const unsigned int imx290slave_i2c_addr;
extern unsigned int imx290slave_addr_byte;
extern unsigned int imx290slave_data_byte;

typedef struct hiIMX290_SLAVE_STATE_S
{
    HI_U8       u8Hcg;
    HI_U32      u32BRL;
    HI_U32      u32RHS1_MAX;
    HI_U32      u32RHS2_MAX;
} IMX290_SLAVE_STATE_S;

IMX290_SLAVE_STATE_S g_astimx290slaveState[ISP_MAX_PIPE_NUM] = {{0}};

extern void imx290slave_init(VI_PIPE ViPipe);
extern void imx290slave_exit(VI_PIPE ViPipe);
extern void imx290slave_standby(VI_PIPE ViPipe);
extern void imx290slave_restart(VI_PIPE ViPipe);
extern int imx290slave_write_register(VI_PIPE ViPipe, int addr, int data);
extern int imx290slave_read_register(VI_PIPE ViPipe, int addr);

/****************************************************************************
 * local variables                                                            *
 ****************************************************************************/
#define IMX290_SLAVE_FULL_LINES_MAX  (0x3FFFF)

/*****Imx290Slave Register Address*****/
#define IMX290_SLAVE_SHS1_ADDR (0x3020)
#define IMX290_SLAVE_SHS2_ADDR (0x3024)
#define IMX290_SLAVE_SHS3_ADDR (0x3028)
#define IMX290_SLAVE_GAIN_ADDR (0x3014)
#define IMX290_SLAVE_HCG_ADDR  (0x3009)
#define IMX290_SLAVE_VMAX_ADDR (0x3018)
#define IMX290_SLAVE_HMAX_ADDR (0x301c)
#define IMX290_SLAVE_Y_OUT_SIZE_ADDR (0x3418)

//sensor fps mode

#define IMX290_SLAVE_RES_IS_1080P(w, h)      ((w) <= 1920 && (h) <= 1080)

static HI_S32 cmos_get_ae_default(VI_PIPE ViPipe, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    if (HI_NULL == pstAeSnsDft)
    {
        printf("null pointer when get ae default value!\n");
        return - 1;
    }

    memset(&pstAeSnsDft->stAERouteAttr, 0, sizeof(ISP_AE_ROUTE_S));

    pstAeSnsDft->u32FullLinesStd = g_apstSnsState[ViPipe]->u32FLStd;
    pstAeSnsDft->u32FlickerFreq = 50 * 256;
    pstAeSnsDft->u32FullLinesMax = IMX290_SLAVE_FULL_LINES_MAX;

    pstAeSnsDft->stIntTimeAccu.enAccuType = AE_ACCURACY_LINEAR;
    pstAeSnsDft->stIntTimeAccu.f32Accuracy = 1;
    pstAeSnsDft->stIntTimeAccu.f32Offset = 0;

    pstAeSnsDft->stAgainAccu.enAccuType = AE_ACCURACY_TABLE;
    pstAeSnsDft->stAgainAccu.f32Accuracy = 1;

    pstAeSnsDft->stDgainAccu.enAccuType = AE_ACCURACY_TABLE;
    pstAeSnsDft->stDgainAccu.f32Accuracy = 1;

    pstAeSnsDft->u32ISPDgainShift = 8;
    pstAeSnsDft->u32MinISPDgainTarget = 1 << pstAeSnsDft->u32ISPDgainShift;
    pstAeSnsDft->u32MaxISPDgainTarget = 2 << pstAeSnsDft->u32ISPDgainShift;

    if (g_au32LinesPer500ms[ViPipe] == 0)
    {
        pstAeSnsDft->u32LinesPer500ms = g_apstSnsState[ViPipe]->u32FLStd * 30 / 2;
    }
    else
    {
        pstAeSnsDft->u32LinesPer500ms = g_au32LinesPer500ms[ViPipe];
    }


    pstAeSnsDft->enMaxIrisFNO = ISP_IRIS_F_NO_1_0;
    pstAeSnsDft->enMinIrisFNO = ISP_IRIS_F_NO_32_0;

    pstAeSnsDft->bAERouteExValid = HI_FALSE;
    pstAeSnsDft->stAERouteAttr.u32TotalNum = 0;
    pstAeSnsDft->stAERouteAttrEx.u32TotalNum = 0;

    switch (g_apstSnsState[ViPipe]->enWDRMode)
    {
        default:
        case WDR_MODE_NONE:   /*linear mode*/
            pstAeSnsDft->au8HistThresh[0] = 0xd;
            pstAeSnsDft->au8HistThresh[1] = 0x28;
            pstAeSnsDft->au8HistThresh[2] = 0x60;
            pstAeSnsDft->au8HistThresh[3] = 0x80;

            pstAeSnsDft->u32MaxAgain = 62564;
            pstAeSnsDft->u32MinAgain = 1024;
            pstAeSnsDft->u32MaxAgainTarget = pstAeSnsDft->u32MaxAgain;
            pstAeSnsDft->u32MinAgainTarget = pstAeSnsDft->u32MinAgain;

            pstAeSnsDft->u32MaxDgain = 38577;
            pstAeSnsDft->u32MinDgain = 1024;
            pstAeSnsDft->u32MaxDgainTarget = 20013;
            pstAeSnsDft->u32MinDgainTarget = pstAeSnsDft->u32MinDgain;

            pstAeSnsDft->u8AeCompensation = 0x38;
            pstAeSnsDft->enAeExpMode = AE_EXP_HIGHLIGHT_PRIOR;

            pstAeSnsDft->u32InitExposure = g_au32InitExposure[ViPipe] ? g_au32InitExposure[ViPipe] : 148859;

            pstAeSnsDft->u32MaxIntTime = g_apstSnsState[ViPipe]->u32FLStd - 2;
            pstAeSnsDft->u32MinIntTime = 1;
            pstAeSnsDft->u32MaxIntTimeTarget = 65535;
            pstAeSnsDft->u32MinIntTimeTarget = 1;
            break;
    }

    return 0;
}

/* the function of sensor set fps */
static HI_VOID cmos_fps_set(VI_PIPE ViPipe, HI_FLOAT f32Fps, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    HI_FLOAT f32maxFps = 0;
    HI_U32 u32Lines = 0;

    f32maxFps = g_astImx290ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].f32MaxFps;

    switch (g_apstSnsState[ViPipe]->u8ImgMode)
    {
        case IMX290_2M60FPS_LINER_MODE:
            if ((f32Fps <= f32maxFps) && (f32Fps >= 0.5))
            {
                u32Lines = (g_astImx290ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].u32VertiLines) * f32maxFps / f32Fps;
                gstImx290Sync[ViPipe].u32VsTime = (g_astImx290ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].u32InckPerVs) * (f32maxFps / f32Fps);
            }
            else
            {
                printf("Not support Fps: %f\n", f32Fps);
                return;
            }
            u32Lines = (u32Lines > IMX290_SLAVE_FULL_LINES_MAX) ? IMX290_SLAVE_FULL_LINES_MAX : u32Lines;
            break;

        default:
            return;
            break;
    }

    if (WDR_MODE_NONE == g_apstSnsState[ViPipe]->enWDRMode)
    {
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[5].u32Data = (u32Lines & 0xFF);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[6].u32Data = ((u32Lines & 0xFF00) >> 8);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[7].u32Data = ((u32Lines & 0xF0000) >> 16);
    }

    g_apstSnsState[ViPipe]->u32FLStd = u32Lines;
    g_apstSnsState[ViPipe]->astRegsInfo[0].stSlvSync.u32SlaveVsTime = gstImx290Sync[ViPipe].u32VsTime;

    pstAeSnsDft->f32Fps = f32Fps;
    pstAeSnsDft->u32LinesPer500ms = g_apstSnsState[ViPipe]->u32FLStd * f32Fps / 2;
    pstAeSnsDft->u32FullLinesStd = g_apstSnsState[ViPipe]->u32FLStd;
    pstAeSnsDft->u32MaxIntTime = g_apstSnsState[ViPipe]->u32FLStd - 2;
    g_apstSnsState[ViPipe]->au32FL[0] = g_apstSnsState[ViPipe]->u32FLStd;
    pstAeSnsDft->u32FullLines = g_apstSnsState[ViPipe]->au32FL[0];

    return;

}

static HI_VOID cmos_slow_framerate_set(VI_PIPE ViPipe, HI_U32 u32FullLines,
                                       AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{

    u32FullLines = (u32FullLines > IMX290_SLAVE_FULL_LINES_MAX) ? IMX290_SLAVE_FULL_LINES_MAX : u32FullLines;
    gstImx290Sync[ViPipe].u32VsTime =  MIN((HI_U64)g_astImx290ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].u32InckPerHs * u32FullLines, IMX290_VS_TIME_MAX);
    g_apstSnsState[ViPipe]->au32FL[0] = u32FullLines;
    g_apstSnsState[ViPipe]->astRegsInfo[0].stSlvSync.u32SlaveVsTime = gstImx290Sync[ViPipe].u32VsTime;

    if (WDR_MODE_NONE == g_apstSnsState[ViPipe]->enWDRMode)
    {
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[5].u32Data = (g_apstSnsState[ViPipe]->au32FL[0] & 0xFF);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[6].u32Data = ((g_apstSnsState[ViPipe]->au32FL[0] & 0xFF00) >> 8);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[7].u32Data = ((g_apstSnsState[ViPipe]->au32FL[0] & 0xF0000) >> 16);
    }

    pstAeSnsDft->u32FullLines = g_apstSnsState[ViPipe]->au32FL[0];
    pstAeSnsDft->u32MaxIntTime = g_apstSnsState[ViPipe]->au32FL[0] - 2;

    return;
}

/* while isp notify ae to update sensor regs, ae call these funcs. */
static HI_VOID cmos_inttime_update(VI_PIPE ViPipe, HI_U32 u32IntTime)
{
    HI_U32 u32Value = 0;

    u32Value = g_apstSnsState[ViPipe]->au32FL[0] - u32IntTime - 1;

    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[0].u32Data = (u32Value & 0xFF);
    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[1].u32Data = ((u32Value & 0xFF00) >> 8);
    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[2].u32Data = ((u32Value & 0x30000) >> 16);

    return;

}

static HI_U32 gain_table[262] =
{
    1024, 1059, 1097, 1135, 1175, 1217, 1259, 1304, 1349, 1397, 1446, 1497, 1549, 1604, 1660, 1719, 1779, 1842, 1906,
    1973, 2043, 2048, 2119, 2194, 2271, 2351, 2434, 2519, 2608, 2699, 2794, 2892, 2994, 3099, 3208, 3321, 3438, 3559,
    3684, 3813, 3947, 4086, 4229, 4378, 4532, 4691, 4856, 5027, 5203, 5386, 5576, 5772, 5974, 6184, 6402, 6627, 6860,
    7101, 7350, 7609, 7876, 8153, 8439, 8736, 9043, 9361, 9690, 10030, 10383, 10748, 11125, 11516, 11921, 12340, 12774,
    13222, 13687, 14168, 14666, 15182, 15715, 16267, 16839, 17431, 18043, 18677, 19334, 20013, 20717, 21445, 22198,
    22978, 23786, 24622, 25487, 26383, 27310, 28270, 29263, 30292, 31356, 32458, 33599, 34780, 36002, 37267, 38577,
    39932, 41336, 42788, 44292, 45849, 47460, 49128, 50854, 52641, 54491, 56406, 58388, 60440, 62564, 64763, 67039,
    69395, 71833, 74358, 76971, 79676, 82476, 85374, 88375, 91480, 94695, 98023, 101468, 105034, 108725, 112545,
    116501, 120595, 124833, 129220, 133761, 138461, 143327, 148364, 153578, 158975, 164562, 170345, 176331, 182528,
    188942, 195582, 202455, 209570, 216935, 224558, 232450, 240619, 249074, 257827, 266888, 276267, 285976, 296026,
    306429, 317197, 328344, 339883, 351827, 364191, 376990, 390238, 403952, 418147, 432842, 448053, 463799, 480098,
    496969, 514434, 532512, 551226, 570597, 590649, 611406, 632892, 655133, 678156, 701988, 726657, 752194, 778627,
    805990, 834314, 863634, 893984, 925400, 957921, 991585, 1026431, 1062502, 1099841, 1138491, 1178500, 1219916,
    1262786, 1307163, 1353100, 1400651, 1449872, 1500824, 1553566, 1608162, 1664676, 1723177, 1783733, 1846417,
    1911304, 1978472, 2048000, 2119971, 2194471, 2271590, 2351418, 2434052, 2519590, 2608134, 2699789, 2794666,
    2892876, 2994538, 3099773, 3208706, 3321467, 3438190, 3559016, 3684087, 3813554, 3947571, 4086297, 4229898,
    4378546, 4532417, 4691696, 4856573, 5027243, 5203912, 5386788, 5576092, 5772048, 5974890, 6184861, 6402210,
    6627198, 6860092, 7101170, 7350721, 7609041, 7876439, 8153234
};

static HI_VOID cmos_again_calc_table(VI_PIPE ViPipe, HI_U32 *pu32AgainLin, HI_U32 *pu32AgainDb)
{
    int i;
    if ((HI_NULL == pu32AgainLin) || (HI_NULL == pu32AgainDb))
    {
        printf("null pointer when get ae sensor again info value!\n");
        return;
    }


    if (*pu32AgainLin >= gain_table[120])
    {
        *pu32AgainLin = gain_table[120];
        *pu32AgainDb = 120;
        return ;
    }

    for (i = 1; i < 121; i++)
    {
        if (*pu32AgainLin < gain_table[i])
        {
            *pu32AgainLin = gain_table[i - 1];
            *pu32AgainDb = i - 1;
            break;
        }
    }
    return;
}

static HI_VOID cmos_dgain_calc_table(VI_PIPE ViPipe, HI_U32 *pu32DgainLin, HI_U32 *pu32DgainDb)
{
    int i;

    if ((HI_NULL == pu32DgainLin) || (HI_NULL == pu32DgainDb))
    {
        printf("null pointer when get ae sensor gain info value!\n");
        return;
    }

    if (*pu32DgainLin >= gain_table[106])
    {
        *pu32DgainLin = gain_table[106];
        *pu32DgainDb = 106;
        return ;
    }

    for (i = 1; i < 106; i++)
    {
        if (*pu32DgainLin < gain_table[i])
        {
            *pu32DgainLin = gain_table[i - 1];
            *pu32DgainDb = i - 1;
            break;
        }
    }

    return;
}

static HI_VOID cmos_gains_update(VI_PIPE ViPipe, HI_U32 u32Again, HI_U32 u32Dgain)
{
    HI_U32 u32HCG = g_astimx290slaveState[ViPipe].u8Hcg;
    HI_U32 u32Tmp;

    if (u32Again >= 21)
    {
        u32HCG = u32HCG | 0x10;  // bit[4] HCG  .Reg0x3009[7:0]
        u32Again = u32Again - 21;
    }

    u32Tmp = u32Again + u32Dgain;

    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[3].u32Data = (u32Tmp & 0xFF);
    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[4].u32Data = (u32HCG & 0xFF);

    return;
}

static HI_S32 cmos_init_ae_exp_function(AE_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    memset(pstExpFuncs, 0, sizeof(AE_SENSOR_EXP_FUNC_S));

    pstExpFuncs->pfn_cmos_get_ae_default    = cmos_get_ae_default;
    pstExpFuncs->pfn_cmos_fps_set           = cmos_fps_set;
    pstExpFuncs->pfn_cmos_slow_framerate_set = cmos_slow_framerate_set;
    pstExpFuncs->pfn_cmos_inttime_update    = cmos_inttime_update;
    pstExpFuncs->pfn_cmos_gains_update      = cmos_gains_update;
    pstExpFuncs->pfn_cmos_again_calc_table  = cmos_again_calc_table;
    pstExpFuncs->pfn_cmos_dgain_calc_table  = cmos_dgain_calc_table;

    return 0;
}

/* Rgain and Bgain of the golden sample */
#define GOLDEN_RGAIN 0
#define GOLDEN_BGAIN 0
static HI_S32 cmos_get_awb_default(VI_PIPE ViPipe, AWB_SENSOR_DEFAULT_S *pstAwbSnsDft)
{
    if (HI_NULL == pstAwbSnsDft)
    {
        printf("null pointer when get awb default value!\n");
        return -1;
    }

    memset(pstAwbSnsDft, 0, sizeof(AWB_SENSOR_DEFAULT_S));
    pstAwbSnsDft->u16WbRefTemp = 4900;

    pstAwbSnsDft->au16GainOffset[0] = 0x1C3;
    pstAwbSnsDft->au16GainOffset[1] = 0x100;
    pstAwbSnsDft->au16GainOffset[2] = 0x100;
    pstAwbSnsDft->au16GainOffset[3] = 0x1D4;

    pstAwbSnsDft->as32WbPara[0] = -37;
    pstAwbSnsDft->as32WbPara[1] = 293;
    pstAwbSnsDft->as32WbPara[2] = 0;
    pstAwbSnsDft->as32WbPara[3] = 179537;
    pstAwbSnsDft->as32WbPara[4] = 128;
    pstAwbSnsDft->as32WbPara[5] = -123691;

    pstAwbSnsDft->u16GoldenRgain = GOLDEN_RGAIN;
    pstAwbSnsDft->u16GoldenBgain = GOLDEN_BGAIN;

    switch (g_apstSnsState[ViPipe]->enWDRMode)
    {
        default:
        case WDR_MODE_NONE:
            memcpy(&pstAwbSnsDft->stCcm, &g_stAwbCcm, sizeof(AWB_CCM_S));
            memcpy(&pstAwbSnsDft->stAgcTbl, &g_stAwbAgcTable, sizeof(AWB_AGC_TABLE_S));
            break;
    }

    pstAwbSnsDft->u16InitRgain = g_au16InitWBGain[ViPipe][0];
    pstAwbSnsDft->u16InitGgain = g_au16InitWBGain[ViPipe][1];
    pstAwbSnsDft->u16InitBgain = g_au16InitWBGain[ViPipe][2];
    pstAwbSnsDft->u16SampleRgain = g_au16SampleRgain[ViPipe];
    pstAwbSnsDft->u16SampleBgain = g_au16SampleBgain[ViPipe];

    return 0;
}

static HI_S32 cmos_init_awb_exp_function(AWB_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    memset(pstExpFuncs, 0, sizeof(AWB_SENSOR_EXP_FUNC_S));

    pstExpFuncs->pfn_cmos_get_awb_default = cmos_get_awb_default;

    return 0;
}

static ISP_CMOS_DNG_COLORPARAM_S g_stDngColorParam =
{
    {378, 256, 430},
    {439, 256, 439}
};

static HI_U32 cmos_get_isp_default(VI_PIPE ViPipe, ISP_CMOS_DEFAULT_S *pstDef)
{
    if (HI_NULL == pstDef)
    {
        printf("null pointer when get isp default value!\n");
        return -1;
    }

    memset(pstDef, 0, sizeof(ISP_CMOS_DEFAULT_S));

    memcpy(&pstDef->stLsc, &g_stCmosLsc, sizeof(ISP_CMOS_LSC_S));
    memcpy(&pstDef->stRLsc, &g_stCmosRLsc, sizeof(ISP_CMOS_RLSC_S));
    memcpy(&pstDef->stCa, &g_stIspCA, sizeof(ISP_CMOS_CA_S));
    memcpy(&pstDef->stClut, &g_stIspCLUT, sizeof(ISP_CMOS_CLUT_S));
    memcpy(&pstDef->stIspEdgeMark, &g_stIspEdgeMark, sizeof(ISP_CMOS_ISP_EDGEMARK_S));
    memcpy(&pstDef->stWdr, &g_stIspWDR, sizeof(ISP_CMOS_WDR_S));

    switch (g_apstSnsState[ViPipe]->enWDRMode)
    {
        default:
        case WDR_MODE_NONE:
            memcpy(&pstDef->stDemosaic,   &g_stIspDemosaic, sizeof(ISP_CMOS_DEMOSAIC_S));
            memcpy(&pstDef->stIspSharpen,    &g_stIspYuvSharpen, sizeof(ISP_CMOS_ISP_SHARPEN_S));
            memcpy(&pstDef->stDrc,           &g_stIspDRC, sizeof(ISP_CMOS_DRC_S));
            memcpy(&pstDef->stGamma,         &g_stIspGamma, sizeof(ISP_CMOS_GAMMA_S));
            memcpy(&pstDef->stBayerNr,       &g_stIspBayerNr, sizeof(ISP_CMOS_BAYERNR_S));
            memcpy(&pstDef->stGe,            &g_stIspGe, sizeof(ISP_CMOS_GE_S));
            memcpy(&pstDef->stAntiFalseColor, &g_stIspAntiFalseColor, sizeof(ISP_CMOS_ANTIFALSECOLOR_S));
            memcpy(&pstDef->stDpc,        &g_stCmosDpc,       sizeof(ISP_CMOS_DPC_S));
            memcpy(&pstDef->stLdci,       &g_stIspLdci,       sizeof(ISP_CMOS_LDCI_S));
            memcpy(&pstDef->stNoiseCalibration,    &g_stIspNoiseCalibratio, sizeof(ISP_CMOS_NOISE_CALIBRATION_S));

            break;
    }
    pstDef->stSensorMode.u32SensorID = IMX290_SLAVE_ID;
    pstDef->stSensorMode.u8SensorMode = g_apstSnsState[ViPipe]->u8ImgMode;

    memcpy(&pstDef->stDngColorParam, &g_stDngColorParam, sizeof(ISP_CMOS_DNG_COLORPARAM_S));
    switch (g_apstSnsState[ViPipe]->u8ImgMode)
    {
        default:
        case IMX290_2M60FPS_LINER_MODE:
            pstDef->stSensorMode.stDngRawFormat.u8BitsPerSample = 10;
            pstDef->stSensorMode.stDngRawFormat.u32WhiteLevel = 1023;
            break;
    }
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleH.u32Denominator = 1;
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleH.u32Numerator = 1;
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleV.u32Denominator = 1;
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleV.u32Numerator = 1;
    pstDef->stSensorMode.stDngRawFormat.stCfaRepeatPatternDim.u16RepeatPatternDimRows = 2;
    pstDef->stSensorMode.stDngRawFormat.stCfaRepeatPatternDim.u16RepeatPatternDimCols = 2;
    pstDef->stSensorMode.stDngRawFormat.stBlcRepeatDim.u16BlcRepeatRows = 2;
    pstDef->stSensorMode.stDngRawFormat.stBlcRepeatDim.u16BlcRepeatCols = 2;
    pstDef->stSensorMode.stDngRawFormat.enCfaLayout = CFALAYOUT_TYPE_RECTANGULAR;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPlaneColor[0] = 0;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPlaneColor[1] = 1;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPlaneColor[2] = 2;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[0] = 0;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[1] = 1;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[2] = 1;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[3] = 2;
    pstDef->stSensorMode.bValidDngRawFormat = HI_TRUE;

    return 0;
}

static HI_U32 cmos_get_isp_black_level(VI_PIPE ViPipe, ISP_CMOS_BLACK_LEVEL_S *pstBlackLevel)
{
    HI_S32  i;

    if (HI_NULL == pstBlackLevel)
    {
        printf("null pointer when get isp black level value!\n");
        return -1;
    }

    /* Don't need to update black level when iso change */
    pstBlackLevel->bUpdate = HI_FALSE;

    /* black level of linear mode */
    if (WDR_MODE_NONE == g_apstSnsState[ViPipe]->enWDRMode)
    {
        for (i = 0; i < 4; i++)
        {
            pstBlackLevel->au16BlackLevel[i] = 0xF0;    // 240
        }
    }

    /* black level of DOL mode */
    else
    {
        pstBlackLevel->au16BlackLevel[0] = 0xF0;
        pstBlackLevel->au16BlackLevel[1] = 0xF0;
        pstBlackLevel->au16BlackLevel[2] = 0xF0;
        pstBlackLevel->au16BlackLevel[3] = 0xF0;
    }

    return 0;

}

static HI_VOID cmos_set_pixel_detect(VI_PIPE ViPipe, HI_BOOL bEnable)
{

    HI_U32 u32FullLines_5Fps, u32MaxIntTime_5Fps;

    /* Detect set 5fps */
    CHECK_RET(HI_MPI_ISP_GetSnsSlaveAttr(ViPipe, &gstImx290Sync[ViPipe]));

    if (WDR_MODE_2To1_LINE == g_apstSnsState[ViPipe]->enWDRMode || WDR_MODE_3To1_LINE == g_apstSnsState[ViPipe]->enWDRMode)
    {
        return;
    }

    else
    {
        if (IMX290_2M60FPS_LINER_MODE == g_apstSnsState[ViPipe]->u8ImgMode)
        {
            gstImx290Sync[ViPipe].u32VsTime = (g_astImx290ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].u32InckPerVs) * g_astImx290ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].f32MaxFps / 5;
            u32FullLines_5Fps = (g_astImx290ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].u32VertiLines) * g_astImx290ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].f32MaxFps / 5;
        }

        else
        {
            return;
        }
    }

    u32MaxIntTime_5Fps = 4;

    if (bEnable) /* setup for ISP pixel calibration mode */
    {
        imx290slave_write_register (ViPipe, IMX290_SLAVE_GAIN_ADDR, 0x00);

        imx290slave_write_register (ViPipe, IMX290_SLAVE_VMAX_ADDR, u32FullLines_5Fps & 0xFF);
        imx290slave_write_register (ViPipe, IMX290_SLAVE_VMAX_ADDR + 1, (u32FullLines_5Fps & 0xFF00) >> 8);
        imx290slave_write_register (ViPipe, IMX290_SLAVE_VMAX_ADDR + 2, (u32FullLines_5Fps & 0xF0000) >> 16);

        imx290slave_write_register (ViPipe, IMX290_SLAVE_SHS1_ADDR, u32MaxIntTime_5Fps & 0xFF);
        imx290slave_write_register (ViPipe, IMX290_SLAVE_SHS1_ADDR + 1,  (u32MaxIntTime_5Fps & 0xFF00) >> 8);
        imx290slave_write_register (ViPipe, IMX290_SLAVE_SHS1_ADDR + 2, (u32MaxIntTime_5Fps & 0xF0000) >> 16);

    }
    else /* setup for ISP 'normal mode' */
    {
        g_apstSnsState[ViPipe]->u32FLStd = (g_apstSnsState[ViPipe]->u32FLStd > 0x1FFFF) ? 0x1FFFF : g_apstSnsState[ViPipe]->u32FLStd;
        gstImx290Sync[ViPipe].u32VsTime = (g_astImx290ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].u32InckPerVs);
        imx290slave_write_register (ViPipe, IMX290_SLAVE_VMAX_ADDR, g_apstSnsState[ViPipe]->u32FLStd & 0xFF);
        imx290slave_write_register (ViPipe, IMX290_SLAVE_VMAX_ADDR + 1, (g_apstSnsState[ViPipe]->u32FLStd & 0xFF00) >> 8);
        imx290slave_write_register (ViPipe, IMX290_SLAVE_VMAX_ADDR + 2, (g_apstSnsState[ViPipe]->u32FLStd & 0xF0000) >> 16);
        g_apstSnsState[ViPipe]->bSyncInit = HI_FALSE;
    }
    CHECK_RET(HI_MPI_ISP_SetSnsSlaveAttr(ViPipe, &gstImx290Sync[ViPipe]));

    return;
}

static HI_S32 cmos_set_wdr_mode(VI_PIPE ViPipe, HI_U8 u8Mode)
{
    g_apstSnsState[ViPipe]->bSyncInit = HI_FALSE;

    switch (u8Mode)
    {
        case WDR_MODE_NONE:
            g_apstSnsState[ViPipe]->enWDRMode = WDR_MODE_NONE;
            g_apstSnsState[ViPipe]->u32FLStd = g_astImx290ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].u32VertiLines;
            g_astimx290slaveState[ViPipe].u8Hcg = 0x1;
            printf("linear mode\n");
            break;

        default:
            printf("NOT support this mode!\n");
            return HI_FAILURE;
            break;
    }

    g_apstSnsState[ViPipe]->au32FL[0] = g_apstSnsState[ViPipe]->u32FLStd;
    g_apstSnsState[ViPipe]->au32FL[1] = g_apstSnsState[ViPipe]->au32FL[0];
    memset(g_apstSnsState[ViPipe]->au32WDRIntTime, 0, sizeof(g_apstSnsState[ViPipe]->au32WDRIntTime));

    return HI_SUCCESS;
}

static HI_S32 cmos_get_sns_regs_info(VI_PIPE ViPipe, ISP_SNS_REGS_INFO_S *pstSnsRegsInfo)
{
    HI_S32 i;

    if (HI_NULL == pstSnsRegsInfo)
    {
        printf("null pointer when get sns reg info!\n");
        return -1;
    }

    if ((HI_FALSE == g_apstSnsState[ViPipe]->bSyncInit) || (HI_FALSE == pstSnsRegsInfo->bConfig))
    {
        g_apstSnsState[ViPipe]->astRegsInfo[0].enSnsType = ISP_SNS_I2C_TYPE;
        g_apstSnsState[ViPipe]->astRegsInfo[0].unComBus.s8I2cDev = g_aunImx290SlaveBusInfo[ViPipe].s8I2cDev;
        g_apstSnsState[ViPipe]->astRegsInfo[0].u8Cfg2ValidDelayMax = 2;
        g_apstSnsState[ViPipe]->astRegsInfo[0].u32RegNum = 8;

        for (i = 0; i < g_apstSnsState[ViPipe]->astRegsInfo[0].u32RegNum; i++)
        {
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].bUpdate = HI_TRUE;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].u8DevAddr = imx290slave_i2c_addr;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].u32AddrByteNum = imx290slave_addr_byte;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].u32DataByteNum = imx290slave_data_byte;
        }

        //Linear Mode Regs
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[0].u8DelayFrmNum = 0;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[0].u32RegAddr = IMX290_SLAVE_SHS1_ADDR;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[1].u8DelayFrmNum = 0;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[1].u32RegAddr = IMX290_SLAVE_SHS1_ADDR + 1;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[2].u8DelayFrmNum = 0;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[2].u32RegAddr = IMX290_SLAVE_SHS1_ADDR + 2;

        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[3].u8DelayFrmNum = 0;       //make shutter and gain effective at the same time
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[3].u32RegAddr = IMX290_SLAVE_GAIN_ADDR;  //gain
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[4].u8DelayFrmNum = 1;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[4].u32RegAddr = IMX290_SLAVE_HCG_ADDR;

        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[5].u8DelayFrmNum = 0;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[5].u32RegAddr = IMX290_SLAVE_VMAX_ADDR;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[6].u8DelayFrmNum = 0;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[6].u32RegAddr = IMX290_SLAVE_VMAX_ADDR + 1;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[7].u8DelayFrmNum = 0;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[7].u32RegAddr = IMX290_SLAVE_VMAX_ADDR + 2;

        /* Slave Sensor VsTime cfg */
        g_apstSnsState[ViPipe]->astRegsInfo[0].stSlvSync.bUpdate = HI_TRUE;
        g_apstSnsState[ViPipe]->astRegsInfo[0].stSlvSync.u8DelayFrmNum = 1;
        g_apstSnsState[ViPipe]->astRegsInfo[0].stSlvSync.u32SlaveBindDev = g_Imx290SlaveBindDev[ViPipe];

        g_apstSnsState[ViPipe]->bSyncInit = HI_TRUE;


    }

    else
    {
        for (i = 0; i < g_apstSnsState[ViPipe]->astRegsInfo[0].u32RegNum; i++)
        {
            if (g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].u32Data == g_apstSnsState[ViPipe]->astRegsInfo[1].astI2cData[i].u32Data)
            {
                g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].bUpdate = HI_FALSE;
            }

            else
            {
                g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].bUpdate = HI_TRUE;
            }
        }

        if (g_apstSnsState[ViPipe]->astRegsInfo[0].stSlvSync.u32SlaveVsTime == g_apstSnsState[ViPipe]->astRegsInfo[1].stSlvSync.u32SlaveVsTime)
        {
            g_apstSnsState[ViPipe]->astRegsInfo[0].stSlvSync.bUpdate = HI_FALSE;
        }
        else
        {
            g_apstSnsState[ViPipe]->astRegsInfo[0].stSlvSync.bUpdate = HI_TRUE;
        }
    }

    pstSnsRegsInfo->bConfig = HI_FALSE;
    memcpy(pstSnsRegsInfo, &g_apstSnsState[ViPipe]->astRegsInfo[0], sizeof(ISP_SNS_REGS_INFO_S));
    memcpy(&g_apstSnsState[ViPipe]->astRegsInfo[1], &g_apstSnsState[ViPipe]->astRegsInfo[0], sizeof(ISP_SNS_REGS_INFO_S));

    g_apstSnsState[ViPipe]->au32FL[1] = g_apstSnsState[ViPipe]->au32FL[0];

    return 0;
}

static HI_S32 cmos_set_image_mode(VI_PIPE ViPipe, ISP_CMOS_SENSOR_IMAGE_MODE_S *pstSensorImageMode)
{
    HI_U8 u8SensorImageMode = g_apstSnsState[ViPipe]->u8ImgMode;
    HI_FLOAT f32maxFps = 0;

    g_apstSnsState[ViPipe]->bSyncInit = HI_FALSE;
    f32maxFps = g_astImx290ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].f32MaxFps;

    if (HI_NULL == pstSensorImageMode )
    {
        printf("null pointer when set image mode\n");
        return -1;
    }

    if (pstSensorImageMode->f32Fps <= f32maxFps)
    {
        if (WDR_MODE_NONE == g_apstSnsState[ViPipe]->enWDRMode)
        {
            if (IMX290_SLAVE_RES_IS_1080P(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height))
            {
                u8SensorImageMode = IMX290_2M60FPS_LINER_MODE;
            }
            else
            {
                printf("Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
                       pstSensorImageMode->u16Width,
                       pstSensorImageMode->u16Height,
                       pstSensorImageMode->f32Fps,
                       g_apstSnsState[ViPipe]->enWDRMode);
                return HI_FAILURE;
            }
        }

        else
        {
            printf("Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
                   pstSensorImageMode->u16Width,
                   pstSensorImageMode->u16Height,
                   pstSensorImageMode->f32Fps,
                   g_apstSnsState[ViPipe]->enWDRMode);
            return HI_FAILURE;
        }
    }
    else
    {
    }

    if ((HI_TRUE == g_apstSnsState[ViPipe]->bInit) && (u8SensorImageMode == g_apstSnsState[ViPipe]->u8ImgMode))
    {
        /* Don't need to switch SensorImageMode */
        return -1;
    }

    g_apstSnsState[ViPipe]->u8ImgMode = u8SensorImageMode;

    return 0;
}

static HI_VOID sensor_global_init(VI_PIPE ViPipe)
{
    g_apstSnsState[ViPipe]->bInit = HI_FALSE;
    g_apstSnsState[ViPipe]->bSyncInit = HI_FALSE;
    g_apstSnsState[ViPipe]->u8ImgMode = IMX290_2M60FPS_LINER_MODE;
    g_apstSnsState[ViPipe]->enWDRMode = WDR_MODE_NONE;
    g_apstSnsState[ViPipe]->u32FLStd  = g_astImx290ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].u32VertiLines;
    g_apstSnsState[ViPipe]->au32FL[0] = g_apstSnsState[ViPipe]->u32FLStd;
    g_apstSnsState[ViPipe]->au32FL[1] = g_apstSnsState[ViPipe]->u32FLStd;

    memset(&g_apstSnsState[ViPipe]->astRegsInfo[0], 0, sizeof(ISP_SNS_REGS_INFO_S));
    memset(&g_apstSnsState[ViPipe]->astRegsInfo[1], 0, sizeof(ISP_SNS_REGS_INFO_S));
}

static HI_S32 cmos_init_sensor_exp_function(ISP_SENSOR_EXP_FUNC_S *pstSensorExpFunc)
{
    memset(pstSensorExpFunc, 0, sizeof(ISP_SENSOR_EXP_FUNC_S));

    pstSensorExpFunc->pfn_cmos_sensor_init = imx290slave_init;
    pstSensorExpFunc->pfn_cmos_sensor_exit = imx290slave_exit;
    pstSensorExpFunc->pfn_cmos_sensor_global_init = sensor_global_init;
    pstSensorExpFunc->pfn_cmos_set_image_mode = cmos_set_image_mode;
    pstSensorExpFunc->pfn_cmos_set_wdr_mode = cmos_set_wdr_mode;

    pstSensorExpFunc->pfn_cmos_get_isp_default = cmos_get_isp_default;
    pstSensorExpFunc->pfn_cmos_get_isp_black_level = cmos_get_isp_black_level;
    pstSensorExpFunc->pfn_cmos_set_pixel_detect = cmos_set_pixel_detect;
    pstSensorExpFunc->pfn_cmos_get_sns_reg_info = cmos_get_sns_regs_info;

    return 0;
}

/****************************************************************************
 * callback structure                                                       *
 ****************************************************************************/

static int imx290slave_set_bus_info(VI_PIPE ViPipe, ISP_SNS_COMMBUS_U unSNSBusInfo)
{
    g_aunImx290SlaveBusInfo[ViPipe].s8I2cDev = unSNSBusInfo.s8I2cDev;

    return 0;
}

static int sensor_register_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    HI_S32 s32Ret;
    ISP_SENSOR_REGISTER_S stIspRegister;
    AE_SENSOR_REGISTER_S  stAeRegister;
    AWB_SENSOR_REGISTER_S stAwbRegister;
    ISP_SNS_ATTR_INFO_S   stSnsAttrInfo;

    stSnsAttrInfo.eSensorId = IMX290_SLAVE_ID;

    cmos_init_sensor_exp_function(&stIspRegister.stSnsExp);
    s32Ret = HI_MPI_ISP_SensorRegCallBack(ViPipe, &stSnsAttrInfo, &stIspRegister);

    if (s32Ret)
    {
        printf("sensor register callback function failed!\n");
        return s32Ret;
    }

    cmos_init_ae_exp_function(&stAeRegister.stSnsExp);
    s32Ret = HI_MPI_AE_SensorRegCallBack(ViPipe, pstAeLib, &stSnsAttrInfo, &stAeRegister);

    if (s32Ret)
    {
        printf("sensor register callback function to ae lib failed!\n");
        return s32Ret;
    }

    cmos_init_awb_exp_function(&stAwbRegister.stSnsExp);
    s32Ret = HI_MPI_AWB_SensorRegCallBack(ViPipe, pstAwbLib, &stSnsAttrInfo, &stAwbRegister);

    if (s32Ret)
    {
        printf("sensor register callback function to awb lib failed!\n");
        return s32Ret;
    }

    return 0;
}

static int sensor_unregister_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_ISP_SensorUnRegCallBack(ViPipe, IMX290_SLAVE_ID);
    if (s32Ret)
    {
        printf("sensor unregister callback function failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_AE_SensorUnRegCallBack(ViPipe, pstAeLib, IMX290_SLAVE_ID);
    if (s32Ret)
    {
        printf("sensor unregister callback function to ae lib failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_AWB_SensorUnRegCallBack(ViPipe, pstAwbLib, IMX290_SLAVE_ID);
    if (s32Ret)
    {
        printf("sensor unregister callback function to awb lib failed!\n");
        return s32Ret;
    }

    return 0;
}

static int sensor_set_init(VI_PIPE ViPipe, ISP_INIT_ATTR_S *pstInitAttr)
{
    g_au32InitExposure[ViPipe] = pstInitAttr->u32Exposure;
    g_au32LinesPer500ms[ViPipe] = pstInitAttr->u32LinesPer500ms;
    g_au16InitWBGain[ViPipe][0] = pstInitAttr->u16WBRgain;
    g_au16InitWBGain[ViPipe][1] = pstInitAttr->u16WBGgain;
    g_au16InitWBGain[ViPipe][2] = pstInitAttr->u16WBBgain;
    g_au16SampleRgain[ViPipe] = pstInitAttr->u16SampleRgain;
    g_au16SampleBgain[ViPipe] = pstInitAttr->u16SampleBgain;

    return 0;
}

ISP_SNS_OBJ_S stSnsImx290SlaveObj =
{
    .pfnRegisterCallback    = sensor_register_callback,
    .pfnUnRegisterCallback  = sensor_unregister_callback,
    .pfnStandby             = imx290slave_standby,
    .pfnRestart             = imx290slave_restart,
    .pfnWriteReg            = imx290slave_write_register,
    .pfnReadReg             = imx290slave_read_register,
    .pfnSetBusInfo          = imx290slave_set_bus_info,
    .pfnSetInit             = sensor_set_init
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __IMX290_CMOS_SLAVE_H_ */
