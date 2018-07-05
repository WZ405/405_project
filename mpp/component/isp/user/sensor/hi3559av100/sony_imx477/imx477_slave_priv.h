/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : IMX477_slave_priv.h
  Version       : Initial Draft
  Author        : Hisilicon BVT PQ group
  Created       : 2017/05/08
  Description   : this file was private for IMX477 slave mode sensor
  History       :
  1.Date        :
    Author      :
    Modification: Created file
******************************************************************************/
#ifndef __IMX477_SLAVE_PRIV_H_
#define __IMX477_SLAVE_PRIV_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

// usefull macro
#define HIGH_8BITS(x) ((x & 0xff00) >> 8)
#define LOW_8BITS(x)  (x & 0x00ff)
#ifndef MAX
#define MAX(a, b) (((a) < (b)) ?  (b) : (a))
#endif

#ifndef MIN
#define MIN(a, b) (((a) > (b)) ?  (b) : (a))
#endif

#define IMX477_RES_IS_8M(w, h)        ((w == 3840) && (h == 2160))
#define IMX477_RES_IS_12M(w, h)        (((w == 4056) && (h == 3040)) || ((w == 4000) && (h == 3000)))
#define IMX477_RES_IS_2M(w, h)        (((w == 1920) && (h == 1080)) || ((w == 1920) && (h == 1080)))

//#define LENS_TYPE_DUAL_FISH    //Donot remove this define

#define CHECK_RET(express)\
    do{\
        HI_S32 s32Ret;\
        s32Ret = express;\
        if (HI_SUCCESS != s32Ret)\
        {\
            printf("Failed at %s: LINE: %d with %#x!\n", __FUNCTION__, __LINE__, s32Ret);\
        }\
    }while(0)

/*
--------------------------------------------------------------------------------
- Structure For Slave Mode Sensor Using
--------------------------------------------------------------------------------
*/
#define     FULL_LINES_MAX              (0xFFFF)
// registers to control exposure
#define     IMX477_COARSE_INTEG_TIME_L                (0x0203)
#define     IMX477_COARSE_INTEG_TIME_H                (0x0202)
#define     IMX477_ANA_GAIN_GLOBAL_L             (0x0205)
#define     IMX477_ANA_GAIN_GLOBAL_H               (0x0204)
#define     IMX477_DPGA_USE_GLOBAL_GAIN             (0x3FF9)
#define     IMX477_DIG_GAIN_GR_L             (0x020F)
#define     IMX477_DIG_GAIN_GR_H             (0x020E)
#define     IMX477_LINE_LENGTH_PCK_L        (0x341)
#define     IMX477_LINE_LENGTH_PCK_H        (0x340)
#define     IMX477_FRM_LENGTH_CTL      (0x350)
#define     IMX477_PRSH_LENGTH_LINE_L        (0x3F3B)
#define     IMX477_PRSH_LENGTH_LINE_H        (0x3F3A)

typedef struct hiIMX477_SENSOR_REG_S
{

    HI_U16 u16Addr;
    HI_U8  u8Data;
} IMX477_SENSOR_REG_S;

typedef enum
{
    IMX477_12M30FPS_LINER_MODE = 0,  //4000x3000@30fps@12bit
    IMX477_8M30FPS_LINER_MODE = 1,  //3840x2160@30fps@12bit
    IMX477_8M60FPS_LINER_MODE = 2,  //3840x2160@60fps@12bit
    IMX477_9M50FPS_LINER_MODE = 3,  //3000x3000@50fps@10bit
    IMX477_2M240FPS_LINER_MODE = 4, //1920x1080@240fps@10bit
    IMX477_MODE_BUTT

} IMX477_RES_MODE_E;

typedef struct hiIMX477_VIDEO_MODE_TBL_S
{
    HI_U32  u32Inck;
    HI_U32  u32InckPerHs;
    HI_U32  u32InckPerVs;
    HI_U32  u32VertiLines;

    HI_FLOAT  f32MaxFps;
    const char *pszModeName;

} IMX477_VIDEO_MODE_TBL_S;



#endif /* __IMX477_SLAVE_PRIV_H_ */
