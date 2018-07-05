/******************************************************************************
 
  Copyright (C), 2001-2017, Hisilicon Tech. Co., Ltd.
 
 ******************************************************************************
  File Name     : svp_dsp_algo.h
  Version       : Initial Draft
  Author        : 
  Created       : 2017/10/31
  Description   : VDSP algorithm header file
  History       : 
  1.Date        : 2017/10/31
    Authr       : 
    Modification: Created file 
 ******************************************************************************/

#ifndef __SVP_DSP_ALGO_H__
#define __SVP_DSP_ALGO_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

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
HI_S32 SVP_DSP_ALGO_Init(HI_VOID);

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
HI_S32 SVP_DSP_ALGO_DeInit(HI_VOID);

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
HI_S32 SVP_DSP_ALGO_Process(HI_U64 u64IdmaOffset,HI_U32 u32CMD,HI_U32 u32MsgId,HI_U64 u64Body,HI_U32 u32BodyLen);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __SVP_DSP_KERNEL_H__ */
