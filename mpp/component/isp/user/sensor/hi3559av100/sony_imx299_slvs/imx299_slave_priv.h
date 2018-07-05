/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : imx299_slave_priv.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2015/06/18
  Description   : this file was private for imx226 slave mode sensor
  History       :
  1.Date        :
    Author      :
    Modification: Created file
******************************************************************************/
#ifndef __IMX299_SLAVE_PRIV_H_
#define __IMX299_SLAVE_PRIV_H_

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

#define IMX299_RES_IS_4K30(w, h)    (w <= 3840 && h <= 2160)

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

// difference registers in various mode
#define     IMX299_VIDEO_MODE_REG_NUM   (0x0004)

typedef struct hiIMX299_SENSOR_REG_S
{

    HI_U16 u16Addr;
    HI_U8  u8Data;
} IMX299_SENSOR_REG_S;

typedef enum
{
    IMX299_4K120FPS_MODE = 0,
    IMX299_4K60FPS_WDR_MODE,
    IMX299_MODE_BUTT

} IMX299_RES_MODE_E;

typedef struct hiIMX299_VIDEO_MODE_TBL_S
{
    HI_U32  u32Inck;
    HI_U32  u32InckPerHs;
    HI_U32  u32InckPerVs;
    HI_U32  u32VertiLines;
    HI_U32  u32MaxFps;
    const char *pszModeName;

} IMX299_VIDEO_MODE_TBL_S;


#endif /* __IMX299_SLAVE_PRIV_H_ */
