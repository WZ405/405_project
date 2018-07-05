
/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_block.c

  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/11/07
  Description   :
  History       :
  1.Date        : 2013/11/07
    Author      :
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include "isp_block.h"
#include "mkp_isp.h"
#include "hi_isp_debug.h"
#include "isp_main.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#if 0
static HI_U32 ISP_GetOverlap(VI_PIPE ViPipe, ISP_RUNNING_MODE_E enRunningMode)
{
    switch (enRunningMode)
    {
        case ISP_MODE_RUNNING_STRIPING:
        case ISP_MODE_RUNNING_SIDEBYSIDE:
            return ISP_STRIPING_OVERLAP_DEFAULT;
        default:
            return 0;
    }
}

static HI_U8 ISP_GetBlockNum(VI_PIPE ViPipe, ISP_RUNNING_MODE_E enRunningMode)
{
    switch (enRunningMode)
    {
        case ISP_MODE_RUNNING_STRIPING:
        case ISP_MODE_RUNNING_SIDEBYSIDE:
            return ISP_DEFAULT_BLOCK_NUM;
        default:
            return ISP_NORMAL_BLOCK_NUM;
    }
}
#endif
HI_S32 ISP_GetWorkMode(VI_PIPE ViPipe, ISP_WORKING_MODE_S *pstIspWorkMode)
{
    return HI_SUCCESS;
}

static HI_S32 ISP_GetBlockHwAttr(VI_PIPE ViPipe, ISP_BLOCK_ATTR_S *pstBlock)
{
    HI_S32 s32Ret = -1;
    ISP_BLOCK_ATTR_S stBlkAttr;

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_WORK_MODE_INIT, &stBlkAttr);
    if (s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "s32Ret:%d!\n", s32Ret);
        return s32Ret;
    }

    ISP_CHECK_RUNNING_MODE(stBlkAttr.enIspRunningMode);
    ISP_CHECK_BLOCK_NUM(stBlkAttr.u8BlockNum);

    if (((ISP_MODE_RUNNING_OFFLINE == stBlkAttr.enIspRunningMode) || (ISP_MODE_RUNNING_ONLINE == stBlkAttr.enIspRunningMode)) && (ISP_NORMAL_BLOCK_NUM != stBlkAttr.u8BlockNum))
    {
        ISP_TRACE(HI_DBG_ERR, "ViPipe :%d,When enIspRunningMode = %d,u8BlockNum's value should be equal to %d!\n", ViPipe, stBlkAttr.enIspRunningMode, ISP_NORMAL_BLOCK_NUM);
        return HI_FAILURE;
    }
    else if ((ISP_MODE_RUNNING_SIDEBYSIDE == stBlkAttr.enIspRunningMode) && (ISP_SBS_BLOCK_NUM != stBlkAttr.u8BlockNum))
    {
        ISP_TRACE(HI_DBG_ERR, "ViPipe :%d,When enIspRunningMode = %d,u8BlockNum's value should be equal to %d!\n", ViPipe, stBlkAttr.enIspRunningMode, ISP_SBS_BLOCK_NUM);
        return HI_FAILURE;
    }
    else if ((ISP_MODE_RUNNING_STRIPING == stBlkAttr.enIspRunningMode) && (stBlkAttr.u8BlockNum < 2))
    {
        ISP_TRACE(HI_DBG_ERR, "ViPipe :%d,When enIspRunningMode = %d,u8BlockNum's value should not be less than %d!\n", ViPipe, stBlkAttr.enIspRunningMode, 2);
        return HI_FAILURE;
    }

    pstBlock->enIspRunningMode = stBlkAttr.enIspRunningMode;
    pstBlock->u8BlockNum       = stBlkAttr.u8BlockNum;
    pstBlock->u32OverLap       = stBlkAttr.u32OverLap;

    memcpy(pstBlock->astBlockRect, stBlkAttr.astBlockRect, sizeof(RECT_S) * ISP_STRIPING_MAX_NUM);

    return HI_SUCCESS;
}

HI_S32 ISP_BlockInit(VI_PIPE ViPipe, ISP_BLOCK_ATTR_S *pstBlock)
{
    HI_S32 s32Ret = -1;
    ISP_CHECK_OPEN(ViPipe);

    s32Ret = ISP_GetBlockHwAttr(ViPipe, pstBlock);
    if (s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "get isp block HW attr failed!\n");
        return s32Ret;
    }

    pstBlock->u8PreBlockNum    = pstBlock->u8BlockNum;

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_PRE_BLK_NUM_UPDATE, &pstBlock->u8PreBlockNum);
    if (s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "s32Ret:%d!\n", s32Ret);
        return s32Ret;
    }

    pstBlock->stFrameRect.s32X = 0;
    pstBlock->stFrameRect.s32Y = 0;
    pstBlock->stFrameRect.u32Width  = 0;
    pstBlock->stFrameRect.u32Height = 0;

    return HI_SUCCESS;
}

HI_S32 ISP_BlockUpdate(VI_PIPE ViPipe, ISP_BLOCK_ATTR_S *pstBlock)
{
    HI_S32 s32Ret = -1;
    ISP_CHECK_OPEN(ViPipe);

    s32Ret = ISP_GetBlockHwAttr(ViPipe, pstBlock);
    if (s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "get isp block HW attr failed!\n");
        return s32Ret;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_BlockExit(VI_PIPE ViPipe)
{
    HI_S32 s32Ret = -1;
    ISP_CHECK_OPEN(ViPipe);

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_WORK_MODE_EXIT);
    if (s32Ret)
    {
        return s32Ret;
    }

    return HI_SUCCESS;
}

HI_U32 ISP_GetBlockRect(ISP_RECT_S *pstBlockRect, ISP_BLOCK_ATTR_S *pstBlock, HI_U8 u8BlockId)
{
    ISP_CHECK_BLOCK_ID(u8BlockId);

    if (1 == pstBlock->u8BlockNum)
    {
        pstBlockRect->s32X      = 0;
        pstBlockRect->s32Y      = 0;
        pstBlockRect->u32Width  = pstBlock->stFrameRect.u32Width;
        pstBlockRect->u32Height = pstBlock->stFrameRect.u32Height;

        return HI_SUCCESS;
    }

    pstBlockRect->s32X     = pstBlock->astBlockRect[u8BlockId].s32X;
    pstBlockRect->s32Y     = pstBlock->astBlockRect[u8BlockId].s32Y;
    pstBlockRect->u32Width = pstBlock->astBlockRect[u8BlockId].u32Width;
    pstBlockRect->u32Height = pstBlock->astBlockRect[u8BlockId].u32Height;

    return HI_SUCCESS;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

