/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : imx290_slave_priv.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2015/06/18
  Description   : this file was private for imx290 slave mode sensor
  History       :
  1.Date        :
    Author      :
    Modification: Created file
******************************************************************************/
#ifndef __IMX290_SLAVE_PRIV_H_
#define __IMX290_SLAVE_PRIV_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifndef MIN
#define MIN(a, b) (((a) > (b)) ?  (b) : (a))
#endif


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

typedef enum
{
    IMX290_2M60FPS_LINER_MODE = 0,
    IMX290_MODE_BUTT
} IMX290_RES_MODE_E;


typedef struct hiIMX290_VIDEO_MODE_TBL_S
{
    HI_U32  u32Inck;
    HI_U32  u32InckPerHs;
    HI_U32  u32InckPerVs;
    HI_U32  u32VertiLines;
    HI_FLOAT  f32MaxFps;
    const char *pszModeName;

} IMX290_VIDEO_MODE_TBL_S;

#endif /* __IMX290_SLAVE_PRIV_H_ */
