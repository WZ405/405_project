/*******************************************************************
                   Copyright 2008 - 2017, Huawei Tech. Co., Ltd.
                             ALL RIGHTS RESERVED

Filename      : svp_dsp_kernel.c
Author        : 
Creation time : 2017/1/18
Description   : DSP algorithm process

Version       : 1.0
********************************************************************/
#include "hi_dsp.h"
#include "svp_dsp_frm.h"
#include "svp_dsp_algo.h"
#include "svp_dsp_proc_algo.h"
#include "svp_dsp_trace.h"

/*****************************************************************************
*   Prototype    : SVP_DSP_ALGO_Init
*   Description:   Algorithm initialization
*   Input        : HI_VOID                              
*
*   Output       : 
*   Return Value : HI_SUCCESS: Success;Error codes: Failure.
*   Spec         : 
*   Calls        : 
*   Called By    : 
*   History:
* 
*       1.  Date         : 2017-03-24
*           Author       : 
*           Modification : Created function
*
*****************************************************************************/
HI_S32 SVP_DSP_ALGO_Init(HI_VOID)
{
    return SVP_DSP_FRM_Init();
}

/*****************************************************************************
*   Prototype    : SVP_DSP_ALGO_DeInit
*   Description:   Algorithm deinitialization
*   Input        : HI_VOID                              
*
*   Output       : 
*   Return Value : HI_SUCCESS: Success;Error codes: Failure.
*   Spec         : 
*   Calls        : 
*   Called By    : 
*   History:
* 
*       1.  Date         : 2017-03-24
*           Author       : 
*           Modification : Created function
*
*****************************************************************************/
HI_S32 SVP_DSP_ALGO_DeInit(HI_VOID)
{
    return SVP_DSP_FRM_Exit();
}

/*****************************************************************************
*   Prototype    : SVP_DSP_ALGO_Process
*   Description  : Process kernel
*   Input        : HI_U64     u64IdmaOffset   IDMA Address offset
*                  HI_U32     u32CMD          CMD                             
*                  HI_U32     u32MsgId        Message Id
*                  HI_U64     u64Body         Body
*                  HI_U32     u32BodyLen      Body Len
*   Output       : 
*   Return Value : HI_SUCCESS: Success;Error codes: Failure.
*   Spec         : 
*   Calls        : 
*   Called By    : 
*   History:
* 
*       1.  Date         : 2017-11-1
*           Author       : 
*           Modification : Created function
*
*****************************************************************************/
HI_S32 SVP_DSP_ALGO_Process(HI_U64 u64IdmaOffset,HI_U32 u32CMD,HI_U32 u32MsgId,HI_U64 u64Body,HI_U32 u32BodyLen)
{
    HI_S32 s32Ret = HI_SUCCESS;

    //HI_TRACE_SVP_DSP(HI_DBG_INFO, "u32CMD %u u32MsgId %u u64Body 0x%llx u32BodyLen %u\n", u32CMD, u32MsgId, u64Body, u32BodyLen);
    /*Add customize cmd in here*/
    switch (u32CMD)
    {
     case SVP_DSP_CMD_ERODE_3X3:
        {
            s32Ret = SVP_DSP_ProcessErode3x3(u64IdmaOffset,u64Body,u32BodyLen);
        }
        break;
     case SVP_DSP_CMD_DILATE_3X3:
        {
            s32Ret = SVP_DSP_ProcessDilate3x3(u64IdmaOffset,u64Body,u32BodyLen);
        }
        break;
#if CONFIG_HI_PHOTO_SUPPORT
#if (0 == DSP_ID)
    case SVP_DSP_CMD_PHOTO_PROC:
        {
            extern HI_S32 SVP_DSP_PhotoProcess(HI_U64 u64IdmaOffset,HI_U64 u64Body,HI_U32 u32BodyLen);
            s32Ret = SVP_DSP_PhotoProcess(u64IdmaOffset,u64Body,u32BodyLen);
        }
        break;
#endif
#endif
     default:
        {
            s32Ret = HI_ERR_SVP_DSP_ILLEGAL_PARAM;
        }
        break;
    }   

    return s32Ret;
}

