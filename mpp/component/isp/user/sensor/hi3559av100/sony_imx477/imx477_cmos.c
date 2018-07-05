
/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : imx477_cmos.c

  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/11/07
  Description   :
  History       :
  1.Date        : 2013/11/07
    Author      :
    Modification: Created file

******************************************************************************/
#if !defined(__IMX477_CMOS_H_)
#define __IMX477_CMOS_H_

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "hi_comm_sns.h"
#include "hi_comm_video.h"
#include "hi_sns_ctrl.h"
#include "mpi_isp.h"
#include "mpi_ae.h"
#include "mpi_awb.h"

#include "imx477_slave_priv.h"
#include "imx477_cmos_ex.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define IMX477_ID 477
#define IMX477_FULL_LINES_STD_FPGA_COMP 0
#define IMX477_VS_TIME_MAX 0xFFFFFFFF
#define IMX477_FLL_MAX 0xFFFF

#define IMX477_AGAIN_TBL_RANGE  62
#define IMX477_DGAIN_TBL_RANGE  97

//sensor gain
#define IMX477_AGAIN_21X    (21504)
#define IMX477_AGAIN_MAX    (21845)

#define IMX477_DGAIN_1X (1024)
#define IMX477_DGAIN_8X (8192)
#define IMX477_DGAIN_9X (9216)
#define IMX477_DGAIN_10X    (10240)
#define IMX477_DGAIN_11X    (11264)
#define IMX477_DGAIN_12X    (12288)
#define IMX477_DGAIN_13X    (13312)
#define IMX477_DGAIN_14X    (14336)
#define IMX477_DGAIN_15X    (15360)
#define IMX477_DGAIN_16X    (16384-1)

extern unsigned char imx477_i2c_addr;
extern unsigned int  imx477_addr_byte;
extern unsigned int  imx477_data_byte;

const IMX477_VIDEO_MODE_TBL_S g_astImx477ModeTbl[IMX477_MODE_BUTT] =
{
    {24000000, 229, 800000, 3500, 30,  "4K3K_4CH_12BIT_30FPS"  }, //MODE0
    {24000000, 352, 800800, 2275, 30,  "4K2K_4CH_12BIT_30FPS"  }, //MODE1
    {24000000, 181, 400000, 2215, 60,  "4K2K_4CH_12BIT_60FPS"  }, //MODE2
    {24000000, 143, 480000, 3360, 50,  "3K3K_4CH_10BIT_50FPS"  }, //MODE3
    {24000000, 89, 100088, 1130, 240,  "2K1K_4CH_10BIT_240FPS"  }, //real fps: 239.79
};

ISP_SLAVE_SNS_SYNC_S gstImx477Sync[ISP_MAX_PIPE_NUM];


/****************************************************************************
 * extern function reference                                                *
 ****************************************************************************/

extern void imx477_init(VI_PIPE ViPipe);
extern void imx477_exit(VI_PIPE ViPipe);
extern void imx477_standby(VI_PIPE ViPipe);
extern void imx477_restart(VI_PIPE ViPipe);
extern int imx477_write_register(VI_PIPE ViPipe, int addr, int data);
extern int imx477_read_register(VI_PIPE ViPipe, int addr);

ISP_SNS_STATE_S             g_astImx477[ISP_MAX_PIPE_NUM] = {{0}};
static ISP_SNS_STATE_S     *g_apstSnsState[ISP_MAX_PIPE_NUM] =
{
    &g_astImx477[0],
    &g_astImx477[1],
    &g_astImx477[2],
    &g_astImx477[3],
    &g_astImx477[4],
    &g_astImx477[5],
    &g_astImx477[6],
    &g_astImx477[7]
};

ISP_SNS_COMMBUS_U g_aunImx477BusInfo[ISP_MAX_PIPE_NUM] =
{
    [0] = { .s8I2cDev = 0},
    [1] = { .s8I2cDev = -1},
    [2] = { .s8I2cDev = -1},
    [3] = { .s8I2cDev = -1},
    [4] = { .s8I2cDev = 1},
    [5] = { .s8I2cDev = -1},
    [6] = { .s8I2cDev = -1},
    [7] = { .s8I2cDev = -1}
};

//static HI_U32             gu32_IMX477_DgainVal = 0;
static HI_U32   g_u32Imx477AGain[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = 1024};
static HI_U32   g_u32Imx477DGain[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = 1024};

/*Depart different sensor mode to get CCM/AWB param*/
HI_U8 g_u8SensorImageMode = IMX477_12M30FPS_LINER_MODE;

/* Sensor slave mode, default binding setting : Slave[0]->Pipe[0]&[1]; Slave[1]->Pipe[2]&[3]; Slave[2]->Pipe[4]&[5] */
HI_S32 g_Imx477SlaveBindDev[ISP_MAX_PIPE_NUM] = {0, 0, 1, 1, 2, 2, 3, 3};

static HI_U32 g_au32InitExposure[ISP_MAX_PIPE_NUM]  = {0};
static HI_U32 g_au32LinesPer500ms[ISP_MAX_PIPE_NUM] = {0};
static HI_U16 g_au16InitWBGain[ISP_MAX_PIPE_NUM][3] = {{0}};
static HI_U16 g_au16SampleRgain[ISP_MAX_PIPE_NUM] = {0};
static HI_U16 g_au16SampleBgain[ISP_MAX_PIPE_NUM] = {0};

static HI_S32 cmos_get_ae_default(VI_PIPE ViPipe, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{

    HI_U32 u32Fll;
    HI_FLOAT F32MaxFps;

    if (HI_NULL == pstAeSnsDft)
    {
        printf("null pointer when get ae default value!\n");
        return -1;
    }

    u32Fll = g_astImx477ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].u32VertiLines;
    F32MaxFps = g_astImx477ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].f32MaxFps;
    g_apstSnsState[ViPipe]->u32FLStd = u32Fll;
    pstAeSnsDft->u32FullLinesStd = g_apstSnsState[ViPipe]->u32FLStd;
    pstAeSnsDft->u32FlickerFreq = 50 * 256;

    pstAeSnsDft->stIntTimeAccu.enAccuType = AE_ACCURACY_LINEAR;
    pstAeSnsDft->stIntTimeAccu.f32Accuracy = 1;

    if (g_au32InitExposure[ViPipe] == 0)
    {
        pstAeSnsDft->u32InitExposure = 1000000;
    }
    else
    {
        pstAeSnsDft->u32InitExposure = g_au32InitExposure[ViPipe];
    }

    if (g_au32LinesPer500ms[ViPipe] == 0)
    {
        pstAeSnsDft->u32LinesPer500ms = ((HI_U64)(u32Fll * F32MaxFps)) >> 1;
    }
    else
    {
        pstAeSnsDft->u32LinesPer500ms = g_au32LinesPer500ms[ViPipe];
    }

    switch (g_apstSnsState[ViPipe]->u8ImgMode)
    {
        case IMX477_12M30FPS_LINER_MODE:
        case IMX477_8M30FPS_LINER_MODE:
        case IMX477_8M60FPS_LINER_MODE:
        case IMX477_9M50FPS_LINER_MODE:
        case IMX477_2M240FPS_LINER_MODE:
            pstAeSnsDft->u32MaxIntTime = g_apstSnsState[ViPipe]->u32FLStd - 8;
            pstAeSnsDft->stIntTimeAccu.f32Offset = 0.221;
            break;

        default:
            printf("[%s]:[%d] NOT support this mode!\n", __FUNCTION__, __LINE__);
            break;
    }

    pstAeSnsDft->stAgainAccu.enAccuType = AE_ACCURACY_TABLE;
    pstAeSnsDft->stAgainAccu.f32Accuracy = 0.3;

    pstAeSnsDft->stDgainAccu.enAccuType = AE_ACCURACY_TABLE;
    pstAeSnsDft->stDgainAccu.f32Accuracy = 0.3;

    pstAeSnsDft->u32ISPDgainShift = 8;
    pstAeSnsDft->u32MinISPDgainTarget = 1 << pstAeSnsDft->u32ISPDgainShift;
    pstAeSnsDft->u32MaxISPDgainTarget = 6 << pstAeSnsDft->u32ISPDgainShift;

    memcpy(&pstAeSnsDft->stPirisAttr, &gstPirisAttr, sizeof(ISP_PIRIS_ATTR_S));
    pstAeSnsDft->enMaxIrisFNO = ISP_IRIS_F_NO_1_4;
    pstAeSnsDft->enMinIrisFNO = ISP_IRIS_F_NO_5_6;

    pstAeSnsDft->bAERouteExValid = HI_FALSE;
    pstAeSnsDft->stAERouteAttr.u32TotalNum = 0;
    pstAeSnsDft->stAERouteAttrEx.u32TotalNum = 0;

    switch (g_apstSnsState[ViPipe]->enWDRMode)
    {
        default:
        case WDR_MODE_NONE:
        {
            pstAeSnsDft->au8HistThresh[0] = 0xd;
            pstAeSnsDft->au8HistThresh[1] = 0x28;
            pstAeSnsDft->au8HistThresh[2] = 0x60;
            pstAeSnsDft->au8HistThresh[3] = 0x80;

            pstAeSnsDft->u8AeCompensation = 0x38;

            pstAeSnsDft->u32MinIntTime = 8;
            pstAeSnsDft->u32MaxIntTimeTarget = 65515;
            pstAeSnsDft->u32MinIntTimeTarget = 8;

            pstAeSnsDft->u32MaxAgain = 21845;
            pstAeSnsDft->u32MinAgain = 1024;
            pstAeSnsDft->u32MaxAgainTarget = pstAeSnsDft->u32MaxAgain;
            pstAeSnsDft->u32MinAgainTarget = pstAeSnsDft->u32MinAgain;

            pstAeSnsDft->u32MaxDgain = 16384 - 1; /* if Dgain enable,please set ispdgain bigger than 1*/
            pstAeSnsDft->u32MinDgain = 1024;
            pstAeSnsDft->u32MaxDgainTarget = pstAeSnsDft->u32MaxDgain;
            pstAeSnsDft->u32MinDgainTarget = pstAeSnsDft->u32MinDgain;

            break;
        }
    }

    return 0;
}


static HI_VOID cmos_fps_set(VI_PIPE ViPipe, HI_FLOAT f32Fps, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    HI_FLOAT f32maxFps;
    HI_U32 u32Lines;

    f32maxFps = g_astImx477ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].f32MaxFps;
    u32Lines = (g_astImx477ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].u32VertiLines) * f32maxFps / f32Fps;

    /* SHR was 16bit, so limit full_lines as 0xFFFF */
    if (f32Fps > f32maxFps || u32Lines > FULL_LINES_MAX)
    {
        printf("[%s]:[%d] Not support Fps: %f\n", __FUNCTION__, __LINE__, f32Fps);
        return;
    }

    gstImx477Sync[ViPipe].u32VsTime = (g_astImx477ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].u32InckPerVs) * (f32maxFps / f32Fps);
    g_apstSnsState[ViPipe]->u32FLStd = (g_astImx477ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].u32VertiLines) * (f32maxFps / f32Fps);

    g_apstSnsState[ViPipe]->astRegsInfo[0].stSlvSync.u32SlaveVsTime = gstImx477Sync[ViPipe].u32VsTime;
    g_apstSnsState[ViPipe]->u32FLStd = (g_apstSnsState[ViPipe]->u32FLStd > FULL_LINES_MAX) ? FULL_LINES_MAX : g_apstSnsState[ViPipe]->u32FLStd;

    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[0].u32Data =  (g_apstSnsState[ViPipe]->u32FLStd & 0xFF);
    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[1].u32Data =  (g_apstSnsState[ViPipe]->u32FLStd & 0xFF00) >> 8;

    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[9].u32Data =  (g_apstSnsState[ViPipe]->u32FLStd & 0xFF);
    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[10].u32Data =  (g_apstSnsState[ViPipe]->u32FLStd & 0xFF00) >> 8;

    pstAeSnsDft->f32Fps = f32Fps;
    pstAeSnsDft->u32LinesPer500ms = g_apstSnsState[ViPipe]->u32FLStd * f32Fps / 2;

    switch (g_apstSnsState[ViPipe]->u8ImgMode)
    {
        case IMX477_12M30FPS_LINER_MODE:
        case IMX477_8M30FPS_LINER_MODE:
        case IMX477_8M60FPS_LINER_MODE:
        case IMX477_9M50FPS_LINER_MODE:
        case IMX477_2M240FPS_LINER_MODE:
            pstAeSnsDft->u32MaxIntTime = g_apstSnsState[ViPipe]->u32FLStd - 20;
            break;

        default:
            printf("[%s]:[%d] NOT support this mode!\n", __FUNCTION__, __LINE__);
            return;
            break;
    }

    pstAeSnsDft->u32FullLinesStd = g_apstSnsState[ViPipe]->u32FLStd;
    g_apstSnsState[ViPipe]->au32FL[0] = g_apstSnsState[ViPipe]->u32FLStd;
    pstAeSnsDft->u32FullLines = g_apstSnsState[ViPipe]->au32FL[0];


    return;
}


static HI_VOID cmos_slow_framerate_set(VI_PIPE ViPipe, HI_U32 u32FullLines,
                                       AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    u32FullLines = MIN(u32FullLines, IMX477_FLL_MAX);
    gstImx477Sync[ViPipe].u32VsTime =  MIN((HI_U64)g_astImx477ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].u32InckPerHs * u32FullLines, IMX477_VS_TIME_MAX);
    g_apstSnsState[ViPipe]->au32FL[0] = u32FullLines;
    g_apstSnsState[ViPipe]->astRegsInfo[0].stSlvSync.u32SlaveVsTime = gstImx477Sync[ViPipe].u32VsTime;
    pstAeSnsDft->u32FullLines = g_apstSnsState[ViPipe]->au32FL[0];

    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[0].u32Data =  (g_apstSnsState[ViPipe]->au32FL[0] & 0xFF);
    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[1].u32Data =  (g_apstSnsState[ViPipe]->au32FL[0] & 0xFF00) >> 8;

    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[9].u32Data =  (g_apstSnsState[ViPipe]->au32FL[0] & 0xFF);
    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[10].u32Data =  (g_apstSnsState[ViPipe]->au32FL[0] & 0xFF00) >> 8;

    switch (g_apstSnsState[ViPipe]->u8ImgMode)
    {
        case IMX477_12M30FPS_LINER_MODE:
        case IMX477_8M30FPS_LINER_MODE:
        case IMX477_8M60FPS_LINER_MODE:
        case IMX477_9M50FPS_LINER_MODE:
        case IMX477_2M240FPS_LINER_MODE:
            pstAeSnsDft->u32MaxIntTime = g_apstSnsState[ViPipe]->au32FL[0] - 20;
            break;

        default:
            printf("[%s]:[%d] NOT support this mode!\n", __FUNCTION__, __LINE__);
            return;
            break;
    }

    return;
}

/* while isp notify ae to update sensor regs, ae call these funcs. */
static HI_VOID cmos_inttime_update(VI_PIPE ViPipe, HI_U32 u32IntTime)
{
    g_apstSnsState[ViPipe]->au32FL[0] = u32IntTime;

    // SET CORASE_INTEG_TIME
    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[7].u32Data = LOW_8BITS(g_apstSnsState[ViPipe]->au32FL[0]);
    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[8].u32Data = HIGH_8BITS(g_apstSnsState[ViPipe]->au32FL[0]);

    return;
}

/* Again segmentation = 62 */
static HI_U32 ad_gain_table[IMX477_AGAIN_TBL_RANGE] =
{
    1024, 1040, 1057, 1074, 1092, 1111, 1130, 1150, 1170, 1192,
    1214, 1237, 1260, 1285, 1311, 1337, 1365, 1394, 1425, 1456,
    1489, 1524, 1560, 1598, 1638, 1680, 1725, 1771, 1820, 1872,
    1928, 1986, 2048, 2114, 2185, 2260, 2341, 2427, 2521, 2621,
    2731, 2849, 2979, 3121, 3277, 3449, 3641, 3855, 4096, 4369,
    4681, 5041, 5461, 5958, 6554, 7282, 8192, 9362, 10923, 13107,
    16384, 21845
};

static HI_VOID cmos_again_calc_table(VI_PIPE ViPipe, HI_U32 *pu32AgainLin, HI_U32 *pu32AgainDb)
{
    int i;

    if ((HI_NULL == pu32AgainLin) || (HI_NULL == pu32AgainDb))
    {
        printf("null pointer when get ae sensor gain info  value!\n");
        return;
    }

    if (*pu32AgainLin >= ad_gain_table[IMX477_AGAIN_TBL_RANGE - 1])
    {
        *pu32AgainLin = ad_gain_table[IMX477_AGAIN_TBL_RANGE - 1];
        *pu32AgainDb = IMX477_AGAIN_TBL_RANGE - 1;
        goto calc_table_end;
    }

    for (i = 1; i < IMX477_AGAIN_TBL_RANGE; i++)
    {
        if (*pu32AgainLin < ad_gain_table[i])
        {
            *pu32AgainLin = ad_gain_table[i - 1];
            *pu32AgainDb = i - 1;
            goto calc_table_end;;
        }
    }


calc_table_end:

    g_u32Imx477AGain[ViPipe] = *pu32AgainLin;

    *pu32AgainDb <<= 4;

    return;
}

/* dgain segmentation = 97 */
static HI_U32 dgain_table[IMX477_DGAIN_TBL_RANGE] =
{
    1024, 1034, 1044, 1055, 1066, 1077, 1088, 1099, 1111, 1123,
    1135, 1147, 1160, 1173, 1186, 1200, 1214, 1228, 1242, 1257,
    1273, 1288, 1304, 1321, 1337, 1355, 1372, 1391, 1409, 1429,
    1448, 1469, 1489, 1511, 1533, 1556, 1579, 1603, 1628, 1654,
    1680, 1708, 1736, 1765, 1796, 1827, 1859, 1893, 1928, 1964,
    2001, 2040, 2081, 2123, 2166, 2212, 2260, 2310, 2362, 2416,
    2473, 2533, 2595, 2661, 2731, 2804, 2881, 2962, 3048, 3139,
    3236, 3339, 3449, 3567, 3692, 3827, 3972, 4128, 4297, 4481,
    4681, 4900, 5140, 5405, 5699, 6026, 6394, 6809, 7282, 7825,
    8456, 9198, 10082, 11155, 12483, 14170, (16384 - 1)
};

#if 0
/* dgain segmentation = 61 */
static HI_U32 dgain_table[61] =
{
    1024, 1040, 1057, 1074, 1092, 1111, 1130, 1150, 1170, 1192, 1214, 1237, 1260, 1285, 1311, 1337, 1365, 1394, 1425, 1456, 1489, 1524, 1560, 1598, 1638 ,
    1680, 1725, 1771, 1820, 1872, 1928, 1986, 2048, 2114, 2185, 2260, 2341, 2427, 2521, 2621, 2731, 2849, 2979, 3121, 3277 , 3449, 3641, 3855, 4096, 4369,
    4681, 5041, 5461, 5958, 6554, 7282, 8192, 9362, 10923, 13107, 16384
};
#endif
static HI_VOID cmos_dgain_calc_table(VI_PIPE ViPipe, HI_U32 *pu32DgainLin, HI_U32 *pu32DgainDb)
{
    //int i;
    if ((HI_NULL == pu32DgainLin) || (HI_NULL == pu32DgainDb))
    {
        printf("null pointer when get ae sensor gain info value!\n");
        return;
    }

    if (*pu32DgainLin >= dgain_table[IMX477_DGAIN_TBL_RANGE - 1])
    {
        *pu32DgainLin = dgain_table[IMX477_DGAIN_TBL_RANGE - 1];
        //*pu32DgainDb = IMX477_DGAIN_TBL_RANGE-1;
        //*pu32DgainDb = *pu32DgainLin;
        //return ;
    }
    g_u32Imx477DGain[ViPipe] = *pu32DgainLin;
#if 0
    for (i = 1; i < IMX477_DGAIN_TBL_RANGE; i++)
    {
        if (*pu32DgainLin < dgain_table[i])
        {
            *pu32DgainLin = dgain_table[i - 1];
            //*pu32DgainDb = i - 1;
            break;
        }
    }
#endif
    *pu32DgainDb = *pu32DgainLin;

    return;

}


static HI_VOID cmos_gains_update(VI_PIPE ViPipe, HI_U32 u32Again, HI_U32 u32Dgain)
{
    //Again
    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[2].u32Data = (u32Again & 0xFF);
    g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[3].u32Data = ((u32Again >> 8) & 0x0003);

    //Dgain
    if (u32Dgain % 1024 != 0)
    {
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[4].u32Data = (((u32Dgain % 1024) >> 2) & 0x00FF);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[5].u32Data = ((u32Dgain >> 10 ) & 0xF);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[6].u32Data = 1;
    }
    else
    {
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[4].u32Data = 0xFF;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[5].u32Data = ((u32Dgain >> 10 ) & 0xF) - 1;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[6].u32Data = 1;

    }

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

//#define MT_ABSOLUTE_LS_H_CALIBRATE_CCM_LINEAR_R_R 0x017C
//#define MT_ABSOLUTE_LS_H_CALIBRATE_CCM_LINEAR_R_G 0x806E
//#define MT_ABSOLUTE_LS_H_CALIBRATE_CCM_LINEAR_R_B 0x800E
//#define MT_ABSOLUTE_LS_H_CALIBRATE_CCM_LINEAR_G_R 0x803B
//#define MT_ABSOLUTE_LS_H_CALIBRATE_CCM_LINEAR_G_G 0x018D
//#define MT_ABSOLUTE_LS_H_CALIBRATE_CCM_LINEAR_G_B 0x8052
//#define MT_ABSOLUTE_LS_H_CALIBRATE_CCM_LINEAR_B_R 0x8012
//#define MT_ABSOLUTE_LS_H_CALIBRATE_CCM_LINEAR_B_G 0x8089
//#define MT_ABSOLUTE_LS_H_CALIBRATE_CCM_LINEAR_B_B 0x019B
//#define MT_ABSOLUTE_LS_M_CALIBRATE_CCM_LINEAR_R_R 0x01BD
//#define MT_ABSOLUTE_LS_M_CALIBRATE_CCM_LINEAR_R_G 0x80D3
//#define MT_ABSOLUTE_LS_M_CALIBRATE_CCM_LINEAR_R_B 0x0016
//#define MT_ABSOLUTE_LS_M_CALIBRATE_CCM_LINEAR_G_R 0x8048
//#define MT_ABSOLUTE_LS_M_CALIBRATE_CCM_LINEAR_G_G 0x016F
//#define MT_ABSOLUTE_LS_M_CALIBRATE_CCM_LINEAR_G_B 0x8027
//#define MT_ABSOLUTE_LS_M_CALIBRATE_CCM_LINEAR_B_R 0x8008
//#define MT_ABSOLUTE_LS_M_CALIBRATE_CCM_LINEAR_B_G 0x80AF
//#define MT_ABSOLUTE_LS_M_CALIBRATE_CCM_LINEAR_B_B 0x01B7
//#define MT_ABSOLUTE_LS_L_CALIBRATE_CCM_LINEAR_R_R 0x016A
//#define MT_ABSOLUTE_LS_L_CALIBRATE_CCM_LINEAR_R_G 0x8042
//#define MT_ABSOLUTE_LS_L_CALIBRATE_CCM_LINEAR_R_B 0x8028
//#define MT_ABSOLUTE_LS_L_CALIBRATE_CCM_LINEAR_G_R 0x8067
//#define MT_ABSOLUTE_LS_L_CALIBRATE_CCM_LINEAR_G_G 0x018e
//#define MT_ABSOLUTE_LS_L_CALIBRATE_CCM_LINEAR_G_B 0x8027
//#define MT_ABSOLUTE_LS_L_CALIBRATE_CCM_LINEAR_B_R 0x8037
//#define MT_ABSOLUTE_LS_L_CALIBRATE_CCM_LINEAR_B_G 0x8129
//#define MT_ABSOLUTE_LS_L_CALIBRATE_CCM_LINEAR_B_B 0x0260

//wide lens & FPGA
#ifdef LENS_TYPE_DUAL_FISH
#define CALIBRATE_STATIC_WB_R_GAIN 549
#define CALIBRATE_STATIC_WB_GR_GAIN 0x100
#define CALIBRATE_STATIC_WB_GB_GAIN 0x100
#define CALIBRATE_STATIC_WB_B_GAIN 480

/* Calibration results for Auto WB Planck */
#define CALIBRATE_AWB_P1 81
#define CALIBRATE_AWB_P2 109
#define CALIBRATE_AWB_Q1 (-67)
#define CALIBRATE_AWB_A1 156314
#define CALIBRATE_AWB_B1 128
#define CALIBRATE_AWB_C1 (-100470)
#else
#define CALIBRATE_STATIC_WB_R_GAIN 491
#define CALIBRATE_STATIC_WB_GR_GAIN 0x100
#define CALIBRATE_STATIC_WB_GB_GAIN 0x100
#define CALIBRATE_STATIC_WB_B_GAIN 458

/* Calibration results for Auto WB Planck */
#define CALIBRATE_AWB_P1 (-4)
#define CALIBRATE_AWB_P2 235
#define CALIBRATE_AWB_Q1 (-24)
#define CALIBRATE_AWB_A1 153042
#define CALIBRATE_AWB_B1 128
#define CALIBRATE_AWB_C1 (-101980)
#endif
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
    pstAwbSnsDft->u16WbRefTemp = 5120;

    pstAwbSnsDft->au16GainOffset[0] = CALIBRATE_STATIC_WB_R_GAIN;
    pstAwbSnsDft->au16GainOffset[1] = CALIBRATE_STATIC_WB_GR_GAIN;
    pstAwbSnsDft->au16GainOffset[2] = CALIBRATE_STATIC_WB_GB_GAIN;
    pstAwbSnsDft->au16GainOffset[3] = CALIBRATE_STATIC_WB_B_GAIN;

    pstAwbSnsDft->as32WbPara[0] = CALIBRATE_AWB_P1;
    pstAwbSnsDft->as32WbPara[1] = CALIBRATE_AWB_P2;
    pstAwbSnsDft->as32WbPara[2] = CALIBRATE_AWB_Q1;
    pstAwbSnsDft->as32WbPara[3] = CALIBRATE_AWB_A1;
    pstAwbSnsDft->as32WbPara[4] = CALIBRATE_AWB_B1;
    pstAwbSnsDft->as32WbPara[5] = CALIBRATE_AWB_C1;

    pstAwbSnsDft->u16GoldenRgain = GOLDEN_RGAIN;
    pstAwbSnsDft->u16GoldenBgain = GOLDEN_BGAIN;
#ifdef LENS_TYPE_DUAL_FISH
    memcpy(&pstAwbSnsDft->stCcm, &g_stAwbCcm_FishLens, sizeof(AWB_CCM_S));
#else
    memcpy(&pstAwbSnsDft->stCcm, &g_stAwbCcm_NormalLens, sizeof(AWB_CCM_S));
#endif

    memcpy(&pstAwbSnsDft->stAgcTbl, &g_stAwbAgcTable, sizeof(AWB_AGC_TABLE_S));
    pstAwbSnsDft->u16InitRgain = g_au16InitWBGain[ViPipe][0];
    pstAwbSnsDft->u16InitGgain = g_au16InitWBGain[ViPipe][1];
    pstAwbSnsDft->u16InitBgain = g_au16InitWBGain[ViPipe][2];
    pstAwbSnsDft->u16SampleRgain = g_au16SampleRgain[ViPipe];
    pstAwbSnsDft->u16SampleBgain = g_au16SampleBgain[ViPipe];
    memcpy(&pstAwbSnsDft->stSpecAwbAttrs, &g_SpecAWBFactTbl, sizeof(ISP_SPECAWB_ATTR_S));
    memcpy(&pstAwbSnsDft->stCaaControl, &g_SpecKCAWBCaaTblControl, sizeof(ISP_SPECAWB_CAA_CONTROl_S));
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
    memcpy(&pstDef->stCa, &g_stIspCA, sizeof(ISP_CMOS_CA_S));
    memcpy(&pstDef->stClut, &g_stIspCLUT, sizeof(ISP_CMOS_CLUT_S));
    memcpy(&pstDef->stWdr, &g_stIspWDR, sizeof(ISP_CMOS_WDR_S));
    memcpy(&pstDef->stLsc, &g_stCmosLsc, sizeof(ISP_CMOS_LSC_S));

    switch (g_apstSnsState[ViPipe]->enWDRMode)
    {
        default:
        case WDR_MODE_NONE:
            memcpy(&pstDef->stDemosaic,   &g_stIspDemosaic, sizeof(ISP_CMOS_DEMOSAIC_S));
            memcpy(&pstDef->stDrc,        &g_stIspDRC, sizeof(ISP_CMOS_DRC_S));
            memcpy(&pstDef->stGamma,      &g_stIspGamma, sizeof(ISP_CMOS_GAMMA_S));
            memcpy(&pstDef->stBayerNr,    &g_stIspBayerNr, sizeof(ISP_CMOS_BAYERNR_S));
            memcpy(&pstDef->stIspSharpen, &g_stIspYuvSharpen, sizeof(ISP_CMOS_ISP_SHARPEN_S));
            memcpy(&pstDef->stIspEdgeMark, &g_stIspEdgeMark, sizeof(ISP_CMOS_ISP_EDGEMARK_S));
            memcpy(&pstDef->stGe,         &g_stIspGe, sizeof(ISP_CMOS_GE_S));
            memcpy(&pstDef->stAntiFalseColor, &g_stIspAntiFalseColor, sizeof(ISP_CMOS_ANTIFALSECOLOR_S));
            memcpy(&pstDef->stDpc,        &g_stCmosDpc,       sizeof(ISP_CMOS_DPC_S));
            memcpy(&pstDef->stLdci,       &g_stIspLdci,       sizeof(ISP_CMOS_LDCI_S));
            memcpy(&pstDef->stNoiseCalibration,    &g_stIspNoiseCalibratio, sizeof(ISP_CMOS_NOISE_CALIBRATION_S));
            break;
    }

    pstDef->stSensorMaxResolution.u32MaxWidth  = 4056;
    pstDef->stSensorMaxResolution.u32MaxHeight = 3040;
    pstDef->stSensorMode.u32SensorID = IMX477_ID;
    pstDef->stSensorMode.u8SensorMode = g_apstSnsState[ViPipe]->u8ImgMode;

    memcpy(&pstDef->stDngColorParam, &g_stDngColorParam, sizeof(ISP_CMOS_DNG_COLORPARAM_S));
    switch (g_apstSnsState[ViPipe]->u8ImgMode)
    {
        default:
        case IMX477_8M60FPS_LINER_MODE:
        case IMX477_8M30FPS_LINER_MODE:
        case IMX477_12M30FPS_LINER_MODE:

            pstDef->stSensorMode.stDngRawFormat.u8BitsPerSample = 12;
            pstDef->stSensorMode.stDngRawFormat.u32WhiteLevel = 4095;
            break;

        case IMX477_9M50FPS_LINER_MODE:
        case IMX477_2M240FPS_LINER_MODE:

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
    //HI_S32  i;

    if (HI_NULL == pstBlackLevel)
    {
        printf("null pointer when get isp black level value!\n");
        return -1;
    }

    /* Don't need to update black level when iso change */
    pstBlackLevel->bUpdate = HI_TRUE;

    //for (i=0; i<4; i++)
    //{
    //    pstBlackLevel->au16BlackLevel[i] = 0x100;/*10bit : 0x40*/
    //}

    if (HI_TRUE == pstBlackLevel->bUpdate)
    {
        if (g_u32Imx477AGain[ViPipe] >= IMX477_AGAIN_21X)
        {
            if ((g_u32Imx477DGain[ViPipe] <= IMX477_DGAIN_8X))
            {
                pstBlackLevel->au16BlackLevel[0] = 258;
                pstBlackLevel->au16BlackLevel[1] = 261;
                pstBlackLevel->au16BlackLevel[2] = 261;
                pstBlackLevel->au16BlackLevel[3] = 258;
            }
            if ((g_u32Imx477DGain[ViPipe] > IMX477_DGAIN_8X) && (g_u32Imx477DGain[ViPipe] <= IMX477_DGAIN_9X))
            {
                pstBlackLevel->au16BlackLevel[0] = 261;
                pstBlackLevel->au16BlackLevel[1] = 262;
                pstBlackLevel->au16BlackLevel[2] = 262;
                pstBlackLevel->au16BlackLevel[3] = 261;
            }
            else if ((g_u32Imx477DGain[ViPipe] > IMX477_DGAIN_9X) && (g_u32Imx477DGain[ViPipe] <= IMX477_DGAIN_11X))
            {
                pstBlackLevel->au16BlackLevel[0] = 262;
                pstBlackLevel->au16BlackLevel[1] = 263;
                pstBlackLevel->au16BlackLevel[2] = 263;
                pstBlackLevel->au16BlackLevel[3] = 262;
            }
            else if ((g_u32Imx477DGain[ViPipe] > IMX477_DGAIN_11X) && (g_u32Imx477DGain[ViPipe] <= IMX477_DGAIN_12X))
            {
                pstBlackLevel->au16BlackLevel[0] = 264;
                pstBlackLevel->au16BlackLevel[1] = 265;
                pstBlackLevel->au16BlackLevel[2] = 265;
                pstBlackLevel->au16BlackLevel[3] = 264;
            }
            else if ((g_u32Imx477DGain[ViPipe] > IMX477_DGAIN_12X) && (g_u32Imx477DGain[ViPipe] <= IMX477_DGAIN_13X))
            {
                pstBlackLevel->au16BlackLevel[0] = 269;
                pstBlackLevel->au16BlackLevel[1] = 268;
                pstBlackLevel->au16BlackLevel[2] = 268;
                pstBlackLevel->au16BlackLevel[3] = 269;
            }
            else if ((g_u32Imx477DGain[ViPipe] > IMX477_DGAIN_13X) && (g_u32Imx477DGain[ViPipe] <= IMX477_DGAIN_14X))
            {
                pstBlackLevel->au16BlackLevel[0] = 270;
                pstBlackLevel->au16BlackLevel[1] = 272;
                pstBlackLevel->au16BlackLevel[2] = 272;
                pstBlackLevel->au16BlackLevel[3] = 270;
            }
            else if ((g_u32Imx477DGain[ViPipe] > IMX477_DGAIN_14X) && (g_u32Imx477DGain[ViPipe] <= IMX477_DGAIN_15X))
            {
                pstBlackLevel->au16BlackLevel[0] = 277;
                pstBlackLevel->au16BlackLevel[1] = 275;
                pstBlackLevel->au16BlackLevel[2] = 275;
                pstBlackLevel->au16BlackLevel[3] = 277;
            }
            else if ((g_u32Imx477DGain[ViPipe] > IMX477_DGAIN_15X) && (g_u32Imx477DGain[ViPipe] <= IMX477_DGAIN_16X))
            {
                pstBlackLevel->au16BlackLevel[0] = 282;
                pstBlackLevel->au16BlackLevel[1] = 278;
                pstBlackLevel->au16BlackLevel[2] = 278;
                pstBlackLevel->au16BlackLevel[3] = 282;
            }
        }
        else
        {
            pstBlackLevel->au16BlackLevel[0] = 257;
            pstBlackLevel->au16BlackLevel[1] = 261;
            pstBlackLevel->au16BlackLevel[2] = 261;
            pstBlackLevel->au16BlackLevel[3] = 257;
        }
    }
    else
    {
        pstBlackLevel->au16BlackLevel[0] = 257;
        pstBlackLevel->au16BlackLevel[1] = 261;
        pstBlackLevel->au16BlackLevel[2] = 261;
        pstBlackLevel->au16BlackLevel[3] = 257;
    }

    //printf("black level Again: %d, Dgain: %d\n", g_u32Imx477AGain, g_u32Imx477DGain);
    //printf("pstBlackLevel->au16BlackLevel[0]: %d\n", pstBlackLevel->au16BlackLevel[0]);
    //printf("pstBlackLevel->au16BlackLevel[1]: %d\n", pstBlackLevel->au16BlackLevel[1]);
    //printf("pstBlackLevel->au16BlackLevel[2]: %d\n", pstBlackLevel->au16BlackLevel[2]);
    //printf("pstBlackLevel->au16BlackLevel[3]: %d\n", pstBlackLevel->au16BlackLevel[3]);

    return 0;
}

static HI_VOID cmos_set_pixel_detect(VI_PIPE ViPipe, HI_BOOL bEnable)
{
    if (bEnable)
    {
        /* Detect set 5fps */
        CHECK_RET(HI_MPI_ISP_GetSnsSlaveAttr(ViPipe, &gstImx477Sync[ViPipe]));
    
        gstImx477Sync[ViPipe].u32VsTime =  (HI_U32)((g_astImx477ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].u32InckPerVs) * g_astImx477ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].f32MaxFps / 5);

        imx477_write_register(ViPipe, IMX477_ANA_GAIN_GLOBAL_L, 0x0);
        imx477_write_register(ViPipe, IMX477_ANA_GAIN_GLOBAL_H, 0x0);
        
        imx477_write_register(ViPipe, IMX477_DIG_GAIN_GR_L, 0x0);
        imx477_write_register(ViPipe, IMX477_DIG_GAIN_GR_H, 0xff);
    }
    else /* setup for ISP 'normal mode' */
    {
        gstImx477Sync[ViPipe].u32VsTime = (HI_U32)(g_astImx477ModeTbl[g_apstSnsState[ViPipe]->u8ImgMode].u32InckPerVs);
        g_apstSnsState[ViPipe]->bSyncInit = HI_FALSE;

        printf("VsTime: %d", gstImx477Sync[ViPipe].u32VsTime);
    }

    CHECK_RET(HI_MPI_ISP_SetSnsSlaveAttr(ViPipe, &gstImx477Sync[ViPipe]));

    return;
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
        g_apstSnsState[ViPipe]->astRegsInfo[0].unComBus.s8I2cDev = g_aunImx477BusInfo[ViPipe].s8I2cDev;
        g_apstSnsState[ViPipe]->astRegsInfo[0].u8Cfg2ValidDelayMax = 3;
        g_apstSnsState[ViPipe]->astRegsInfo[0].u32RegNum = 11;

        for (i = 0; i < g_apstSnsState[ViPipe]->astRegsInfo[0].u32RegNum; i++)
        {
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].bUpdate = HI_TRUE;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].u8DevAddr = imx477_i2c_addr;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].u32AddrByteNum = imx477_addr_byte;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].u32DataByteNum = imx477_data_byte;
        }

        /* Slave Sensor VsTime cfg */

        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[0].u32RegAddr = IMX477_LINE_LENGTH_PCK_L;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[0].u8DelayFrmNum = 0;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[1].u32RegAddr = IMX477_LINE_LENGTH_PCK_H;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[1].u8DelayFrmNum = 0;

        // Again related
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[2].u32RegAddr = IMX477_ANA_GAIN_GLOBAL_L;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[2].u8DelayFrmNum = 1;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[3].u32RegAddr = IMX477_ANA_GAIN_GLOBAL_H;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[3].u8DelayFrmNum = 1;

        /* Dgain cfg */
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[4].u32RegAddr = IMX477_DIG_GAIN_GR_L;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[4].u8DelayFrmNum = 1;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[5].u32RegAddr = IMX477_DIG_GAIN_GR_H;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[5].u8DelayFrmNum = 1;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[6].u32RegAddr = IMX477_DPGA_USE_GLOBAL_GAIN;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[6].u8DelayFrmNum = 1;

        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[7].u8DelayFrmNum = 1;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[7].u32RegAddr = IMX477_COARSE_INTEG_TIME_L ;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[8].u8DelayFrmNum = 1;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[8].u32RegAddr = IMX477_COARSE_INTEG_TIME_H ;

        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[9].u32RegAddr = IMX477_PRSH_LENGTH_LINE_L;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[9].u8DelayFrmNum = 0;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[10].u32RegAddr = IMX477_PRSH_LENGTH_LINE_H;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[10].u8DelayFrmNum = 0;


        // Again related

        /* Dgain cfg */
        g_apstSnsState[ViPipe]->astRegsInfo[0].stSlvSync.bUpdate = HI_TRUE;



        g_apstSnsState[ViPipe]->astRegsInfo[0].stSlvSync.u8DelayFrmNum  = 2;
        g_apstSnsState[ViPipe]->astRegsInfo[0].stSlvSync.u32SlaveBindDev = g_Imx477SlaveBindDev[ViPipe];

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

    HI_U8 u8M = 0, u8SensorImageMode = g_apstSnsState[ViPipe]->u8ImgMode;
    HI_U32 u32W, u32H;

    g_apstSnsState[ViPipe]->bSyncInit = HI_FALSE;


    if (HI_NULL == pstSensorImageMode )
    {
        printf("null pointer when set image mode\n");
        return -1;
    }

    u32H = pstSensorImageMode->u16Height;
    u32W = pstSensorImageMode->u16Width;
    u8M = pstSensorImageMode->u8SnsMode;


    if (IMX477_RES_IS_8M(u32W, u32H))
    {
        if (u8M == 0)
        {
            u8SensorImageMode = IMX477_8M60FPS_LINER_MODE;

        }
        else
        {
            u8SensorImageMode = IMX477_8M30FPS_LINER_MODE;
        }
    }
    else if (IMX477_RES_IS_12M(u32W, u32H))
    {
        u8SensorImageMode = IMX477_12M30FPS_LINER_MODE;
    }
    else if (IMX477_RES_IS_2M(u32W, u32H))
    {
        u8SensorImageMode = IMX477_2M240FPS_LINER_MODE;
    }
    else
    {
        u8SensorImageMode = IMX477_9M50FPS_LINER_MODE;
    }

    g_u8SensorImageMode = u8SensorImageMode;

    /* Sensor first init */
    if (HI_FALSE == g_apstSnsState[ViPipe]->bInit)
    {
        g_apstSnsState[ViPipe]->u8ImgMode = u8SensorImageMode;

        return 0;
    }

    /* Switch SensorImageMode */

    if (u8SensorImageMode == g_apstSnsState[ViPipe]->u8ImgMode)
    {
        /* Don't need to switch SensorImageMode */
        return -1;
    }

    g_apstSnsState[ViPipe]->u8ImgMode = u8SensorImageMode;

    return 0;
}

static HI_VOID sensor_global_init(VI_PIPE ViPipe)
{
    g_apstSnsState[ViPipe]->enWDRMode = WDR_MODE_NONE;
    g_apstSnsState[ViPipe]->bInit = HI_FALSE;
    g_apstSnsState[ViPipe]->bSyncInit = HI_FALSE;

    g_apstSnsState[ViPipe]->u32FLStd = 3500 + IMX477_FULL_LINES_STD_FPGA_COMP;
    g_apstSnsState[ViPipe]->u8ImgMode = IMX477_12M30FPS_LINER_MODE;
    g_apstSnsState[ViPipe]->au32FL[0] = g_apstSnsState[ViPipe]->u32FLStd;
    g_apstSnsState[ViPipe]->au32FL[1] = g_apstSnsState[ViPipe]->u32FLStd;

    memset(&g_apstSnsState[ViPipe]->astRegsInfo[0], 0, sizeof(ISP_SNS_REGS_INFO_S));
    memset(&g_apstSnsState[ViPipe]->astRegsInfo[1], 0, sizeof(ISP_SNS_REGS_INFO_S));
}

static HI_S32 cmos_set_wdr_mode(VI_PIPE ViPipe, HI_U8 u8Mode)
{
    return HI_SUCCESS;
}

static HI_S32 cmos_init_sensor_exp_function(ISP_SENSOR_EXP_FUNC_S *pstSensorExpFunc)
{
    memset(pstSensorExpFunc, 0, sizeof(ISP_SENSOR_EXP_FUNC_S));

    pstSensorExpFunc->pfn_cmos_sensor_init                  = imx477_init;
    pstSensorExpFunc->pfn_cmos_sensor_exit                  = imx477_exit;
    pstSensorExpFunc->pfn_cmos_sensor_global_init           = sensor_global_init;
    pstSensorExpFunc->pfn_cmos_set_image_mode               = cmos_set_image_mode;
    pstSensorExpFunc->pfn_cmos_set_wdr_mode                 = cmos_set_wdr_mode;
    pstSensorExpFunc->pfn_cmos_get_isp_default              = cmos_get_isp_default;
    pstSensorExpFunc->pfn_cmos_get_isp_black_level          = cmos_get_isp_black_level;
    pstSensorExpFunc->pfn_cmos_set_pixel_detect             = cmos_set_pixel_detect;
    pstSensorExpFunc->pfn_cmos_get_sns_reg_info             = cmos_get_sns_regs_info;

    return 0;
}

static int imx477_set_bus_info(VI_PIPE ViPipe, ISP_SNS_COMMBUS_U unSNSBusInfo)
{
    g_aunImx477BusInfo[ViPipe].s8I2cDev = unSNSBusInfo.s8I2cDev;

    return 0;
}

/****************************************************************************
 * callback structure                                                       *
 ****************************************************************************/

static int sensor_register_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    HI_S32 s32Ret;
    ISP_SENSOR_REGISTER_S stIspRegister;
    AE_SENSOR_REGISTER_S  stAeRegister;
    AWB_SENSOR_REGISTER_S stAwbRegister;
    ISP_SNS_ATTR_INFO_S   stSnsAttrInfo;

    stSnsAttrInfo.eSensorId = IMX477_ID;

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

    s32Ret = HI_MPI_ISP_SensorUnRegCallBack(ViPipe, IMX477_ID);
    if (s32Ret)
    {
        printf("sensor unregister callback function failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_AE_SensorUnRegCallBack(ViPipe, pstAeLib, IMX477_ID);
    if (s32Ret)
    {
        printf("sensor unregister callback function to ae lib failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_AWB_SensorUnRegCallBack(ViPipe, pstAwbLib, IMX477_ID);
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

ISP_SNS_OBJ_S stSnsImx477Obj =
{
    .pfnRegisterCallback    = sensor_register_callback,
    .pfnUnRegisterCallback  = sensor_unregister_callback,
    .pfnStandby             = imx477_standby,
    .pfnRestart             = imx477_restart,
    .pfnWriteReg            = imx477_write_register,
    .pfnReadReg             = imx477_read_register,
    .pfnSetBusInfo          = imx477_set_bus_info,
    .pfnSetInit             = sensor_set_init
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif

//
