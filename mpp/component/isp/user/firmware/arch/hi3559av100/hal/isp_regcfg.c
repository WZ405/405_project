/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_regcfg.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/05/07
  Description   :
  History       :
  1.Date        : 2017/01/12
    Author      :
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include "mkp_isp.h"
#include "isp_regcfg.h"
#include "isp_config.h"
#include "isp_lut_config.h"
#include "isp_ext_config.h"
#include "isp_main.h"
#include "mpi_sys.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


ISP_BE_BUF_S g_astBeBufCtx[ISP_MAX_PIPE_NUM] = {{0}};
ISP_REGCFG_S g_astRegCfgCtx[ISP_MAX_PIPE_NUM] = {{0}};

//#define REGCFG_GET_CTX(dev, pstCtx)   pstCtx = &g_astRegCfgCtx[dev]
#define BE_REG_GET_CTX(dev, pstCtx)   pstCtx = &g_astBeBufCtx[dev]

extern HI_S32 g_as32IspFd[ISP_MAX_PIPE_NUM];

HI_S32 ISP_ClutBufInit(VI_PIPE ViPipe)
{
    ISP_CHECK_PIPE(ViPipe);

    if (HI_SUCCESS != ioctl(g_as32IspFd[ViPipe], ISP_CLUT_BUF_INIT))
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] clut buffer init failed\n", ViPipe);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_ClutBufExit(VI_PIPE ViPipe)
{
    ISP_CHECK_PIPE(ViPipe);

    if (HI_SUCCESS != ioctl(g_as32IspFd[ViPipe], ISP_CLUT_BUF_EXIT))
    {
        ISP_TRACE(HI_DBG_ERR, "exit clut bufs failed\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_SpecAwbBufInit(VI_PIPE ViPipe)
{
    ISP_CHECK_PIPE(ViPipe);

    if (HI_SUCCESS != ioctl(g_as32IspFd[ViPipe], ISP_SPECAWB_BUF_INIT))
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] specawb buffer init failed\n", ViPipe);
        return HI_ERR_ISP_MEM_NOT_INIT;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_SpecAwbBufExit(VI_PIPE ViPipe)
{
    ISP_CHECK_PIPE(ViPipe);

    if (HI_SUCCESS != ioctl(g_as32IspFd[ViPipe], ISP_SPECAWB_BUF_EXIT))
    {
        ISP_TRACE(HI_DBG_ERR, "exit specawb bufs failed\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}
HI_S32 ISP_SttBufInit(VI_PIPE ViPipe)
{
    ISP_CHECK_PIPE(ViPipe);

    if (HI_SUCCESS != ioctl(g_as32IspFd[ViPipe], ISP_STT_BUF_INIT))
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] stt buffer init failed\n", ViPipe);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_SttBufExit(VI_PIPE ViPipe)
{
    ISP_CHECK_PIPE(ViPipe);

    if (HI_SUCCESS != ioctl(g_as32IspFd[ViPipe], ISP_STT_BUF_EXIT))
    {
        ISP_TRACE(HI_DBG_ERR, "exit stt bufs failed\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_SttAddrInit(VI_PIPE ViPipe)
{
    return HI_SUCCESS;
}

HI_S32 ISP_CfgBeBufInit(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    ISP_BE_BUF_S *pstBeBuf = HI_NULL;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    HI_U32 u32BeBufSize;

    ISP_CHECK_PIPE(ViPipe);
    BE_REG_GET_CTX(ViPipe, pstBeBuf);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode) || \
        IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
    {
        return HI_SUCCESS;
    }

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_BE_CFG_BUF_INIT, &pstBeBuf->u64BePhyAddr);

    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "Pipe:%d init be config bufs failed %x!\n", ViPipe, s32Ret);
        return s32Ret;
    }

    pstBeBuf->pBeVirtAddr = HI_MPI_SYS_MmapCache(pstBeBuf->u64BePhyAddr, sizeof(ISP_BE_WO_REG_CFG_S) * MAX_ISP_BE_BUF_NUM);

    if (HI_NULL == pstBeBuf->pBeVirtAddr)
    {
        ISP_TRACE(HI_DBG_ERR, "Pipe:%d init be config bufs failed!\n", ViPipe);
        s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_BE_CFG_BUF_EXIT);

        if (HI_SUCCESS != s32Ret)
        {
            ISP_TRACE(HI_DBG_ERR, "Pipe:%d exit be config bufs failed %x!\n", ViPipe, s32Ret);
            return s32Ret;
        }

        return HI_ERR_ISP_NOMEM;
    }

    pstBeBuf->stBeWoCfgBuf.u64PhyAddr = pstBeBuf->u64BePhyAddr;

    /* Get be buffer start address & size */
    u32BeBufSize = sizeof(ISP_BE_WO_REG_CFG_S) * MAX_ISP_BE_BUF_NUM;
    hi_ext_system_be_buffer_address_high_write(ViPipe, (pstBeBuf->u64BePhyAddr >> 32));
    hi_ext_system_be_buffer_address_low_write(ViPipe, (pstBeBuf->u64BePhyAddr & 0xFFFFFFFF));
    hi_ext_system_be_buffer_size_write(ViPipe, u32BeBufSize);

    return HI_SUCCESS;
}


HI_S32 ISP_UpdateBeBufAddr(VI_PIPE ViPipe, HI_VOID *pVirtAddr)
{
    HI_U16 i;
    HI_U64 u64BufSize = 0;
    ISP_RUNNING_MODE_E enIspRuningMode;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    enIspRuningMode = pstIspCtx->stBlockAttr.enIspRunningMode;
    u64BufSize = sizeof(ISP_BE_WO_REG_CFG_S) / ISP_STRIPING_MAX_NUM;

    switch ( enIspRuningMode )
    {
        case ISP_MODE_RUNNING_STRIPING :
            for (i = 0; i < ISP_STRIPING_MAX_NUM; i++)
            {
                pstIspCtx->pIspBeVirtAddr[i]  = (HI_VOID *)((HI_U8 *)pVirtAddr + i * u64BufSize);
                pstIspCtx->pViProcVirtAddr[i] = (HI_VOID *)((HI_U8 *)pstIspCtx->pIspBeVirtAddr[i] + VIPROC_OFFLINE_OFFSET);
            }

            break;

        case ISP_MODE_RUNNING_OFFLINE :
            for (i = 0; i < ISP_STRIPING_MAX_NUM; i++)
            {
                if (0 == i)
                {
                    pstIspCtx->pIspBeVirtAddr[i]  = pVirtAddr;
                    pstIspCtx->pViProcVirtAddr[i] = (HI_VOID *)((HI_U8 *)pVirtAddr + VIPROC_OFFLINE_OFFSET);
                }
                else
                {
                    pstIspCtx->pIspBeVirtAddr[i]  = HI_NULL;
                    pstIspCtx->pViProcVirtAddr[i] = HI_NULL;
                }
            }

            break;

        default :
            return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_CfgBeBufMmap(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    HI_U64 u64BePhyAddr;
    ISP_BE_BUF_S *pstBeBuf = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    BE_REG_GET_CTX(ViPipe, pstBeBuf);

    u64BePhyAddr = pstBeBuf->stBeWoCfgBuf.u64PhyAddr;
    hi_ext_system_be_free_buffer_high_addr_write(ViPipe, (u64BePhyAddr >> 32));
    hi_ext_system_be_free_buffer_low_addr_write(ViPipe, (u64BePhyAddr & 0xFFFFFFFF));

    if (HI_NULL != pstBeBuf->pBeVirtAddr)
    {
        pstBeBuf->stBeWoCfgBuf.pVirAddr = pstBeBuf->pBeVirtAddr + \
                                          (pstBeBuf->stBeWoCfgBuf.u64PhyAddr - pstBeBuf->u64BePhyAddr);
    }
    else
    {
        pstBeBuf->stBeWoCfgBuf.pVirAddr = HI_NULL;
    }

    if (HI_NULL == pstBeBuf->stBeWoCfgBuf.pVirAddr)
    {
        return HI_FAILURE;
    }

    s32Ret = ISP_UpdateBeBufAddr(ViPipe, pstBeBuf->stBeWoCfgBuf.pVirAddr);

    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "Pipe:%d isp update BE bufs failed %x!\n", ViPipe, s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_GetBeBufFirst(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    ISP_BE_BUF_S *pstBeBuf = HI_NULL;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    BE_REG_GET_CTX(ViPipe, pstBeBuf);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode) || \
        IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
    {
        return HI_SUCCESS;
    }

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_GET_BE_BUF_FIRST, &pstBeBuf->stBeWoCfgBuf.u64PhyAddr);

    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "Pipe:%d Get be free bufs failed %x!\n", ViPipe, s32Ret);
        return s32Ret;
    }

    s32Ret = ISP_CfgBeBufMmap(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "Pipe:%d ISP_CfgBeBufMmap failed %x!\n", ViPipe, s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_GetBeFreeBuf(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    ISP_BE_BUF_S *pstBeBuf = HI_NULL;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    BE_REG_GET_CTX(ViPipe, pstBeBuf);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode) || \
        IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
    {
        return HI_SUCCESS;
    }

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_BE_FREE_BUF_GET, &pstBeBuf->stBeWoCfgBuf);

    if (HI_SUCCESS != s32Ret)
    {
        //ISP_TRACE(HI_DBG_ERR, "Pipe:%d ISP_GetBeFreeBuf failed %x!\n", ViPipe, s32Ret);
        return s32Ret;
    }

    s32Ret = ISP_CfgBeBufMmap(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "Pipe:%d ISP_CfgBeBufMmap failed %x!\n", ViPipe, s32Ret);
        return HI_FAILURE;
    }


    return HI_SUCCESS;
}

HI_S32 ISP_GetBeLastBuf(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    ISP_BE_BUF_S *pstBeBuf  = HI_NULL;
    ISP_CTX_S    *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    BE_REG_GET_CTX(ViPipe, pstBeBuf);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode) || \
        IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
    {
        return HI_SUCCESS;
    }

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_BE_LAST_BUF_GET, &pstBeBuf->stBeWoCfgBuf.u64PhyAddr);

    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "Pipe:%d Get be busy bufs failed %x!\n", ViPipe, s32Ret);
        return s32Ret;
    }

    s32Ret = ISP_CfgBeBufMmap(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "Pipe:%d ISP_CfgBeBufMmap failed %x!\n", ViPipe, s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_CfgBeBufExit(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    ISP_BE_BUF_S *pstBeBuf = HI_NULL;
    ISP_CTX_S    *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    BE_REG_GET_CTX(ViPipe, pstBeBuf);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode) || \
        IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
    {
        return HI_SUCCESS;
    }

    if (HI_NULL != pstBeBuf->pBeVirtAddr)
    {
        HI_MPI_SYS_Munmap(pstBeBuf->pBeVirtAddr, sizeof(ISP_BE_WO_REG_CFG_S) * MAX_ISP_BE_BUF_NUM);
        pstBeBuf->pBeVirtAddr = HI_NULL;
    }

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_BE_CFG_BUF_EXIT);

    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "Pipe:%d exit be config bufs failed %x!\n", ViPipe, s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_CfgBeBufCtl(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    ISP_BE_BUF_S *pstBeBuf = HI_NULL;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    BE_REG_GET_CTX(ViPipe, pstBeBuf);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode) || \
        IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
    {
        return HI_SUCCESS;
    }

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_BE_CFG_BUF_CTL, &pstBeBuf->stBeWoCfgBuf);

    if (s32Ret)
    {
        return s32Ret;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_SetCfgBeBufState(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode) || \
        IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
    {
        return HI_SUCCESS;
    }

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_BE_CFG_BUF_RUNNING);
    if (s32Ret)
    {
        return s32Ret;
    }

    return HI_SUCCESS;
}

/* init isp be cfgs all buffer */
HI_S32 ISP_AllCfgsBeBufInit(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode) || \
        IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
    {
        return HI_SUCCESS;
    }

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_BE_All_BUF_INIT);

    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] init be all bufs Failed with ec %#x!\n", ViPipe, s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

static HI_S8 ISP_GetBlockIdByPipe(VI_PIPE ViPipe)
{
    HI_S8 s8BlockId = 0;

    switch (ViPipe)
    {
        case ISP_BE0_PIPE_ID :
            s8BlockId = 0;
            break;

        case ISP_BE1_PIPE_ID :
            s8BlockId = 1;
            break;

        default:
            return HI_FAILURE;
    }

    return s8BlockId;
}

HI_S32 ISP_BeVregAddrInit(VI_PIPE ViPipe)
{
    HI_U8  k = 0;
    HI_S8  s8BlkDev = 0;
    HI_U8  u8BlockId = 0;
    HI_U64 u64BufSize = 0;
    ISP_RUNNING_MODE_E enIspRuningMode;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_BE_BUF_S *pstBeBuf = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    BE_REG_GET_CTX(ViPipe, pstBeBuf);

    enIspRuningMode = pstIspCtx->stBlockAttr.enIspRunningMode;
    u64BufSize      = sizeof(ISP_BE_WO_REG_CFG_S) / ISP_STRIPING_MAX_NUM;

    switch ( enIspRuningMode )
    {
        case ISP_MODE_RUNNING_ONLINE :
            s8BlkDev = ISP_GetBlockIdByPipe(ViPipe);

            if (-1 == s8BlkDev)
            {
                ISP_TRACE(HI_DBG_ERR, "ISP[%d] init Online Mode Pipe Err!\n", ViPipe);
                return HI_FAILURE;
            }

            u8BlockId = (HI_U8)s8BlkDev;

            for (k = 0; k < ISP_STRIPING_MAX_NUM; k++)
            {
                if (0 == k)
                {
                    pstIspCtx->pIspBeVirtAddr[k]  = VReg_GetVirtAddrBase(ISP_BE_REG_BASE(u8BlockId));
                    pstIspCtx->pViProcVirtAddr[k] = VReg_GetVirtAddrBase(ISP_VIPROC_REG_BASE(u8BlockId));
                }
                else
                {
                    pstIspCtx->pIspBeVirtAddr[k]  = HI_NULL;
                    pstIspCtx->pViProcVirtAddr[k] = HI_NULL;
                }
            }

            break;

        case ISP_MODE_RUNNING_OFFLINE :
            for (k = 0; k < ISP_STRIPING_MAX_NUM; k++)
            {
                if (0 == k)
                {
                    pstIspCtx->pIspBeVirtAddr[k]  = pstBeBuf->stBeWoCfgBuf.pVirAddr;
                    pstIspCtx->pViProcVirtAddr[k] = (HI_VOID *)((HI_U8 *)pstBeBuf->stBeWoCfgBuf.pVirAddr + VIPROC_OFFLINE_OFFSET);
                }
                else
                {
                    pstIspCtx->pIspBeVirtAddr[k]  = HI_NULL;
                    pstIspCtx->pViProcVirtAddr[k] = HI_NULL;
                }
            }

            break;

        case ISP_MODE_RUNNING_SIDEBYSIDE :
            for (k = 0; k < ISP_STRIPING_MAX_NUM; k++)
            {
                if (k < ISP_MAX_BE_NUM)
                {
                    pstIspCtx->pIspBeVirtAddr[k]  = VReg_GetVirtAddrBase(ISP_BE_REG_BASE(k));
                    pstIspCtx->pViProcVirtAddr[k] = VReg_GetVirtAddrBase(ISP_VIPROC_REG_BASE(k));
                }
                else
                {
                    pstIspCtx->pIspBeVirtAddr[k]  = HI_NULL;
                    pstIspCtx->pViProcVirtAddr[k] = HI_NULL;
                }
            }

            break;

        case ISP_MODE_RUNNING_STRIPING :
            for (k = 0; k < ISP_STRIPING_MAX_NUM; k++)
            {
                pstIspCtx->pIspBeVirtAddr[k]  = (HI_VOID *)((HI_U8 *)pstBeBuf->stBeWoCfgBuf.pVirAddr + k * u64BufSize);
                pstIspCtx->pViProcVirtAddr[k] = (HI_VOID *)((HI_U8 *)pstIspCtx->pIspBeVirtAddr[k] + VIPROC_OFFLINE_OFFSET);
            }

            break;

        default:
            ISP_TRACE(HI_DBG_ERR, "ISP[%d] init Running Mode Err!\n", ViPipe);
            return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_VOID *ISP_VRegCfgBufAddr(VI_PIPE ViPipe, BLK_DEV BlkDev)
{
    HI_U64 u64Size = 0;
    HI_U64 u64PhyAddrHigh;
    HI_U64 u64PhyAddrTemp;
    ISP_BE_BUF_S *pstBeBuf = HI_NULL;

    BE_REG_GET_CTX(ViPipe, pstBeBuf);

    u64Size = sizeof(ISP_BE_WO_REG_CFG_S) / ISP_STRIPING_MAX_NUM;//>> 1;

    if (HI_NULL != pstBeBuf->stBeWoCfgBuf.pVirAddr)
    {
        return ((HI_U8 *)pstBeBuf->stBeWoCfgBuf.pVirAddr + BlkDev * u64Size);
    }

    u64PhyAddrHigh = (HI_U64)hi_ext_system_be_free_buffer_high_addr_read(ViPipe);
    u64PhyAddrTemp = (HI_U64)hi_ext_system_be_free_buffer_low_addr_read(ViPipe);
    u64PhyAddrTemp |= (u64PhyAddrHigh << 32);

    pstBeBuf->stBeWoCfgBuf.u64PhyAddr = u64PhyAddrTemp;
    pstBeBuf->stBeWoCfgBuf.pVirAddr = HI_MPI_SYS_MmapCache(pstBeBuf->stBeWoCfgBuf.u64PhyAddr, sizeof(ISP_BE_WO_REG_CFG_S));

    return ((HI_U8 *)pstBeBuf->stBeWoCfgBuf.pVirAddr + BlkDev * u64Size);
}

HI_S32 ISP_GetBeVregCfgAddr(VI_PIPE ViPipe, HI_VOID *pVirtAddr[])
{
    HI_U8  k = 0;
    HI_S8  s8BlkDev = 0;
    HI_U8  u8BlockId = 0;
    HI_S32 s32Ret;
    ISP_WORKING_MODE_S stIspWorkMode;
    ISP_CHECK_PIPE(ViPipe);

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_WORK_MODE_GET, &stIspWorkMode);

    if (s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "get isp work mode failed!\n");
        return s32Ret;
    }

    switch ( stIspWorkMode.enIspRunningMode )
    {
        case ISP_MODE_RUNNING_ONLINE :
            s8BlkDev = ISP_GetBlockIdByPipe(ViPipe);

            if (-1 == s8BlkDev)
            {
                ISP_TRACE(HI_DBG_ERR, "ISP[%d] Online Mode Pipe Err!\n", ViPipe);
                return HI_FAILURE;
            }

            u8BlockId = (HI_U8)s8BlkDev;

            for (k = 0; k < ISP_STRIPING_MAX_NUM; k++)
            {
                if (0 == k)
                {
                    pVirtAddr[k] = VReg_GetVirtAddrBase(ISP_BE_REG_BASE(u8BlockId));
                }
                else
                {
                    pVirtAddr[k] = HI_NULL;
                }
            }

            break;

        case ISP_MODE_RUNNING_OFFLINE :
            for (k = 0; k < ISP_STRIPING_MAX_NUM; k++)
            {
                if (0 == k)
                {
                    pVirtAddr[k] = ISP_VRegCfgBufAddr(ViPipe, (BLK_DEV)k);
                }
                else
                {
                    pVirtAddr[k] = HI_NULL;
                }
            }

            break;

        case ISP_MODE_RUNNING_SIDEBYSIDE :
            for (k = 0; k < ISP_STRIPING_MAX_NUM; k++)
            {
                if (k < ISP_MAX_BE_NUM)
                {
                    pVirtAddr[k] = VReg_GetVirtAddrBase(ISP_BE_REG_BASE(k));
                }
                else
                {
                    pVirtAddr[k] = HI_NULL;
                }
            }

            break;

        case ISP_MODE_RUNNING_STRIPING :
            for (k = 0; k < ISP_STRIPING_MAX_NUM; k++)
            {
                pVirtAddr[k] = ISP_VRegCfgBufAddr(ViPipe, (BLK_DEV)k);
            }

            break;

        default:
            ISP_TRACE(HI_DBG_ERR, "ISP[%d] GetBe Running Mode Err!\n", ViPipe);
            return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_GetViProcCfgAddr(VI_PIPE ViPipe, HI_VOID *pVirtAddr[])
{
    HI_U8  k = 0;
    HI_S8  s8BlkDev = 0;
    HI_U8  u8BlockId = 0;
    HI_S32 s32Ret;
    HI_VOID *pBeVirtAddr;
    ISP_WORKING_MODE_S stIspWorkMode;
    ISP_CHECK_PIPE(ViPipe);

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_WORK_MODE_GET, &stIspWorkMode);

    if (s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "get isp work mode failed!\n");
        return s32Ret;
    }

    switch ( stIspWorkMode.enIspRunningMode )
    {
        case ISP_MODE_RUNNING_ONLINE :
            s8BlkDev = ISP_GetBlockIdByPipe(ViPipe);

            if (-1 == s8BlkDev)
            {
                ISP_TRACE(HI_DBG_ERR, "ISP[%d] Online Mode Pipe Err!\n", ViPipe);
                return HI_FAILURE;
            }

            u8BlockId = (HI_U8)s8BlkDev;

            for (k = 0; k < ISP_STRIPING_MAX_NUM; k++)
            {
                if (0 == k)
                {
                    pVirtAddr[k] = VReg_GetVirtAddrBase(ISP_VIPROC_REG_BASE(u8BlockId));
                }
                else
                {
                    pVirtAddr[k] = HI_NULL;
                }
            }

            break;

        case ISP_MODE_RUNNING_OFFLINE :
            for (k = 0; k < ISP_STRIPING_MAX_NUM; k++)
            {
                if (0 == k)
                {
                    pBeVirtAddr = ISP_VRegCfgBufAddr(ViPipe, (BLK_DEV)k);
                    pVirtAddr[k] = (HI_VOID * )((HI_U8 *)pBeVirtAddr + VIPROC_OFFLINE_OFFSET);
                }
                else
                {
                    pVirtAddr[k] = HI_NULL;
                }
            }

            break;

        case ISP_MODE_RUNNING_SIDEBYSIDE :
            for (k = 0; k < ISP_STRIPING_MAX_NUM; k++)
            {
                if (k < ISP_MAX_BE_NUM)
                {
                    pVirtAddr[k] = VReg_GetVirtAddrBase(ISP_VIPROC_REG_BASE(k));
                }
                else
                {
                    pVirtAddr[k] = HI_NULL;
                }
            }

            break;

        case ISP_MODE_RUNNING_STRIPING :
            for (k = 0; k < ISP_STRIPING_MAX_NUM; k++)
            {
                pBeVirtAddr = ISP_VRegCfgBufAddr(ViPipe, (BLK_DEV)k);
                pVirtAddr[k] = (HI_VOID * )((HI_U8 *)pBeVirtAddr + VIPROC_OFFLINE_OFFSET);
            }

            break;

        default:
            ISP_TRACE(HI_DBG_ERR, "ISP[%d] GetBe Running Mode Err!\n", ViPipe);
            return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_VOID *ISP_GetFeVirAddr(VI_PIPE ViPipe)
{
    ISP_CHECK_FE_PIPE(ViPipe);

    return VReg_GetVirtAddrBase(ISP_FE_REG_BASE(ViPipe));
}

HI_VOID *ISP_GetBeVirAddr(VI_PIPE ViPipe, BLK_DEV BlkDev)
{
    HI_U32 s32Ret;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    HI_VOID *pVirtAddr[ISP_STRIPING_MAX_NUM] = {HI_NULL};

    ISP_CHECK_FE_PIPE(ViPipe);
    ISP_CHECK_BE_DEV(BlkDev);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (pstIspCtx->pIspBeVirtAddr[BlkDev])
    {
        return pstIspCtx->pIspBeVirtAddr[BlkDev];
    }

    s32Ret = ISP_GetBeVregCfgAddr(ViPipe, pVirtAddr);

    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] Get Be CfgAddr Failed!\n", ViPipe);
        return HI_NULL;
    }

    ISP_CHECK_NULLPTR(pVirtAddr[BlkDev]);

    return pVirtAddr[BlkDev];
}

HI_VOID *ISP_VIPROC_GetVirAddr(VI_PIPE ViPipe, BLK_DEV BlkDev)
{
    HI_U32 s32Ret;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    HI_VOID *pVirtAddr[ISP_STRIPING_MAX_NUM] = {[0 ... (ISP_STRIPING_MAX_NUM - 1)] = HI_NULL};

    ISP_CHECK_FE_PIPE(ViPipe);
    ISP_CHECK_BE_DEV(BlkDev);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (pstIspCtx->pViProcVirtAddr[BlkDev])
    {
        return pstIspCtx->pViProcVirtAddr[BlkDev];
    }

    s32Ret = ISP_GetViProcCfgAddr(ViPipe, pVirtAddr);

    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] Get viproc CfgAddr Failed!\n", ViPipe);
        return HI_NULL;
    }

    ISP_CHECK_NULLPTR(pVirtAddr[BlkDev]);

    return pVirtAddr[BlkDev];
}

static HI_S32  ISP_FeHrsRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo)
{
    ISP_HRS_STATIC_CFG_S   *pstHrsStaticRegCfg  = HI_NULL;

    pstHrsStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[0].stHrsRegCfg.stStaticRegCfg;

    if (pstRegCfgInfo->unKey.bit1HrsCfg)
    {
        if (pstHrsStaticRegCfg->bResh)
        {
            isp_fe_hrs_en_write(0, pstHrsStaticRegCfg->u8Enable);
            isp_hrs_ds_en_write(0, pstHrsStaticRegCfg->u8RSEnable);
            isp_hrs_filterlut0_0_write(0, pstHrsStaticRegCfg->as16HRSFilterLut0[0]);
            isp_hrs_filterlut1_0_write(0, pstHrsStaticRegCfg->as16HRSFilterLut0[1]);
            isp_hrs_filterlut2_0_write(0, pstHrsStaticRegCfg->as16HRSFilterLut0[2]);
            isp_hrs_filterlut3_0_write(0, pstHrsStaticRegCfg->as16HRSFilterLut0[3]);
            isp_hrs_filterlut4_0_write(0, pstHrsStaticRegCfg->as16HRSFilterLut0[4]);
            isp_hrs_filterlut5_0_write(0, pstHrsStaticRegCfg->as16HRSFilterLut0[5]);
            isp_hrs_filterlut0_1_write(0, pstHrsStaticRegCfg->as16HRSFilterLut1[0]);
            isp_hrs_filterlut1_1_write(0, pstHrsStaticRegCfg->as16HRSFilterLut1[1]);
            isp_hrs_filterlut2_1_write(0, pstHrsStaticRegCfg->as16HRSFilterLut1[2]);
            isp_hrs_filterlut3_1_write(0, pstHrsStaticRegCfg->as16HRSFilterLut1[3]);
            isp_hrs_filterlut4_1_write(0, pstHrsStaticRegCfg->as16HRSFilterLut1[4]);
            isp_hrs_filterlut5_1_write(0, pstHrsStaticRegCfg->as16HRSFilterLut1[5]);

            pstHrsStaticRegCfg->bResh = HI_FALSE;
        }

        pstRegCfgInfo->unKey.bit1HrsCfg = 0;
    }

    return HI_SUCCESS;
}

static HI_VOID ISP_FeModifyLscXGridSize(HI_U32 u32Width, HI_U16 *pu16DeltaX)
{
    HI_U8  i;
    HI_U8  u8HalfGridSize  = (HI_ISP_LSC_GRID_COL - 1) / 2;
    HI_U16 u16SumGridWidth = 0;
    HI_U16 u16diff = 0;
    HI_U16 au16DeltaX[HI_ISP_LSC_GRID_COL - 1] = {0};
    HI_U16 au16InvX[HI_ISP_LSC_GRID_COL - 1]   = {0};

    for (i = 0; i < u8HalfGridSize; i++)
    {
        au16DeltaX[i]    = (pu16DeltaX[i] + 1) >> 1;
        u16SumGridWidth += au16DeltaX[i];
    }

    if (u16SumGridWidth > (u32Width >> 2))
    {
        u16diff = u16SumGridWidth - (u32Width >> 2);

        for (i = 1; i <= u16diff; i++)
        {
            au16DeltaX[u8HalfGridSize - i] = au16DeltaX[u8HalfGridSize - i] - 1;
        }
    }

    for (i = 0; i < HI_ISP_LSC_GRID_COL - 1; i++)
    {
        au16InvX[i] = (0 == au16DeltaX[i]) ? (0) : ((4096 * 1024 / au16DeltaX[i] + 512) >> 10);
        isp_lsc1_winX_info_write(0, i, au16DeltaX[i], au16InvX[i]);
    }

    return;
}

static HI_S32 ISP_FeLscRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo)
{
    HI_BOOL bLutUpdate = HI_FALSE;
    HI_U16  i, j;
    HI_U16  u16Width;
    VI_PIPE ViPipeBind;
    ISP_FE_LSC_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;
    ISP_FE_LSC_USR_CFG_S    *pstUsrRegCfg    = HI_NULL;
    ISP_CTX_S *pstIspCtx   = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[0].stFeLscRegCfg.stStaticRegCfg;
    pstUsrRegCfg = &pstRegCfgInfo->stAlgRegCfg[0].stFeLscRegCfg.stUsrRegCfg;

    if (pstIspCtx->stWdrAttr.bMastPipe)
    {
        for (i = 0; i < pstIspCtx->stWdrAttr.stDevBindPipe.u32Num; i++)
        {
            ViPipeBind = pstIspCtx->stWdrAttr.stDevBindPipe.PipeId[i];
            ISP_CHECK_PIPE(ViPipeBind);

            if (pstRegCfgInfo->unKey.bit1FeLscCfg)
            {
                isp_fe_lsc1_en_write(ViPipeBind, pstRegCfgInfo->stAlgRegCfg[0].stFeLscRegCfg.bLscEn);

                u16Width = IS_HRS_ON(pstIspCtx->stBlockAttr.enIspRunningMode) ? (pstIspCtx->stBlockAttr.stFrameRect.u32Width >> 1) : pstIspCtx->stBlockAttr.stFrameRect.u32Width;

                if (pstStaticRegCfg->bResh)
                {
                    isp_lsc1_numh_write(ViPipeBind, pstStaticRegCfg->u8WinNumH);
                    isp_lsc1_numv_write(ViPipeBind, pstStaticRegCfg->u8WinNumV);
                    isp_lsc1_width_offset_write(ViPipeBind, pstStaticRegCfg->u16WidthOffset);
                }

                if (pstUsrRegCfg->bResh)
                {
                    isp_lsc1_mesh_scale_write(ViPipeBind, pstUsrRegCfg->u8MeshScale);
                    isp_lsc1_mesh_str_write(ViPipeBind, pstUsrRegCfg->u16MeshStr);
                    isp_lsc1_mesh_weight_write(ViPipeBind, pstUsrRegCfg->u16Weight);

                    if (pstUsrRegCfg->bLutUpdate)
                    {
                        for (j = 0; j < (HI_ISP_LSC_GRID_ROW - 1) / 2; j++)
                        {
                            isp_lsc1_winY_info_write(ViPipeBind, j, pstUsrRegCfg->au16DeltaY[j], pstUsrRegCfg->au16InvY[j]);
                        }

                        if (IS_HRS_ON(pstIspCtx->stBlockAttr.enIspRunningMode))
                        {
                            ISP_FeModifyLscXGridSize(u16Width, pstUsrRegCfg->au16DeltaX);
                        }
                        else
                        {
                            for (j = 0; j < (HI_ISP_LSC_GRID_COL - 1); j++)
                            {
                                isp_lsc1_winX_info_write(ViPipeBind, j, pstUsrRegCfg->au16DeltaX[j], pstUsrRegCfg->au16InvX[j]);
                            }
                        }

                        isp_lsc1_rgain_waddr_write(ViPipeBind, 0);

                        for (j = 0; j < HI_ISP_LSC_GRID_POINTS; j++)
                        {
                            isp_lsc1_rgain_wdata_write(ViPipeBind, pstUsrRegCfg->au32RGain[j]);
                        }

                        isp_lsc1_grgain_waddr_write(ViPipeBind, 0);

                        for (j = 0; j < HI_ISP_LSC_GRID_POINTS; j++)
                        {
                            isp_lsc1_grgain_wdata_write(ViPipeBind, pstUsrRegCfg->au32GrGain[j]);
                        }

                        isp_lsc1_gbgain_waddr_write(ViPipeBind, 0);

                        for (j = 0; j < HI_ISP_LSC_GRID_POINTS; j++)
                        {
                            isp_lsc1_gbgain_wdata_write(ViPipeBind, pstUsrRegCfg->au32GbGain[j]);
                        }

                        isp_lsc1_bgain_waddr_write(ViPipeBind, 0);

                        for (j = 0; j < HI_ISP_LSC_GRID_POINTS; j++)
                        {
                            isp_lsc1_bgain_wdata_write(ViPipeBind, pstUsrRegCfg->au32BGain[j]);
                        }

                        bLutUpdate = HI_TRUE;
                        //isp_lsc1_lut_update_write(ViPipeBind, HI_TRUE);
                    }
                }
            }
        }

        pstUsrRegCfg->bLutUpdate = HI_FALSE;
        pstStaticRegCfg->bResh   = HI_FALSE;
        pstUsrRegCfg->bResh      = HI_FALSE;
        pstRegCfgInfo->stAlgRegCfg[0].stFeLutUpdateCfg.bLsc1LutUpdate = bLutUpdate;
        pstRegCfgInfo->unKey.bit1FeLscCfg = 0;
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_FeDgRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo)
{
    HI_U16 i;
    VI_PIPE ViPipeBind;
    ISP_DG_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_CTX_S *pstIspCtx   = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    pstDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[0].stFeDgRegCfg.stDynaRegCfg;

    if (pstIspCtx->stWdrAttr.bMastPipe)
    {
        for (i = 0; i < pstIspCtx->stWdrAttr.stDevBindPipe.u32Num; i++)
        {
            ViPipeBind = pstIspCtx->stWdrAttr.stDevBindPipe.PipeId[i];
            ISP_CHECK_PIPE(ViPipeBind);

            if (pstRegCfgInfo->unKey.bit1FeDgCfg)
            {
                isp_fe_dg2_en_write(ViPipeBind, pstRegCfgInfo->stAlgRegCfg[0].stFeDgRegCfg.bDgEn);

                //dynamic
                if (pstDynaRegCfg->bResh)
                {
                    isp_dg2_rgain_write(ViPipeBind, pstDynaRegCfg->u16GainR);
                    isp_dg2_grgain_write(ViPipeBind, pstDynaRegCfg->u16GainGR);
                    isp_dg2_gbgain_write(ViPipeBind, pstDynaRegCfg->u16GainGB);
                    isp_dg2_bgain_write(ViPipeBind, pstDynaRegCfg->u16GainB);
                    isp_dg2_clip_value_write(ViPipeBind, pstDynaRegCfg->u32ClipValue);
                }
            }
        }

        pstDynaRegCfg->bResh = HI_FALSE;
        pstRegCfgInfo->unKey.bit1FeDgCfg = 0;
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_FeLogLUTRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo)
{
    HI_U16  i, j;
    VI_PIPE ViPipeBind;
    HI_U8 u8FeLogLutEn[ISP_MAX_PIPE_NUM] = {ISP_PIPE_FELOGLUT_MODULE_ENABLE};
    ISP_CTX_S                  *pstIspCtx       = HI_NULL;
    ISP_FE_LOGLUT_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_FE_LOGLUT_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[0].stFeLogLUTRegCfg.stStaticRegCfg;
    pstDynaRegCfg   = &pstRegCfgInfo->stAlgRegCfg[0].stFeLogLUTRegCfg.stDynaRegCfg;

    if (pstIspCtx->stWdrAttr.bMastPipe)
    {
        for (i = 0; i < pstIspCtx->stWdrAttr.stDevBindPipe.u32Num; i++)
        {
            ViPipeBind = pstIspCtx->stWdrAttr.stDevBindPipe.PipeId[i];

            if (!u8FeLogLutEn[ViPipeBind])
            {
                continue;
            }

            ISP_CHECK_PIPE(ViPipeBind);

            if (pstRegCfgInfo->unKey.bit1FeLogLUTCfg)
            {
                isp_fe_loglut1_en_write(ViPipeBind, pstDynaRegCfg->bEnable);

                if (pstStaticRegCfg->bStaticResh)
                {
                    /*static*/
                    isp_loglut1_bitw_out_write(ViPipeBind, pstStaticRegCfg->u8BitDepthOut);

                    isp_loglut1_lut_waddr_write(ViPipeBind, 0);

                    for (j = 0; j < PRE_LOG_LUT_SIZE; j++)
                    {
                        isp_loglut1_lut_wdata_write(ViPipeBind, pstStaticRegCfg->au32LogLUT[j]);
                    }
                }
            }

        }

        pstStaticRegCfg->bStaticResh          = HI_FALSE;
        pstRegCfgInfo->unKey.bit1FeLogLUTCfg = 0;
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_FeBlcRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo)
{
    HI_U16 i;
    VI_PIPE ViPipeBind;
    ISP_FE_BLC_CFG_S *pstFeBlcCfg = HI_NULL;

    ISP_CTX_S *pstIspCtx   = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    pstFeBlcCfg = &pstRegCfgInfo->stAlgRegCfg[0].stFeBlcCfg;

    if (pstIspCtx->stWdrAttr.bMastPipe)
    {
        for (i = 0; i < pstIspCtx->stWdrAttr.stDevBindPipe.u32Num; i++)
        {
            ViPipeBind = pstIspCtx->stWdrAttr.stDevBindPipe.PipeId[i];
            ISP_CHECK_PIPE(ViPipeBind);

            if (pstRegCfgInfo->unKey.bit1FeBlcCfg)
            {
                if (pstFeBlcCfg->bReshStatic)
                {
                    /*Fe Dg*/
                    isp_dg2_en_in_write(ViPipeBind, pstFeBlcCfg->stFeDgBlc.stStaticRegCfg.bBlcIn);
                    isp_dg2_en_out_write(ViPipeBind, pstFeBlcCfg->stFeDgBlc.stStaticRegCfg.bBlcOut);
                    /*Fe Wb*/
                    isp_wb1_en_in_write(ViPipeBind, pstFeBlcCfg->stFeWbBlc.stStaticRegCfg.bBlcIn);
                    isp_wb1_en_out_write(ViPipeBind, pstFeBlcCfg->stFeWbBlc.stStaticRegCfg.bBlcOut);
                    /*Fe Ae*/
                    isp_ae1_blc_en_write(ViPipeBind, pstFeBlcCfg->stFeAeBlc.stStaticRegCfg.bBlcIn);
                    /*Fe LSC*/
                    isp_lsc1_blc_in_en_write(ViPipeBind, pstFeBlcCfg->stFeLscBlc.stStaticRegCfg.bBlcIn);
                    isp_lsc1_blc_out_en_write(ViPipeBind, pstFeBlcCfg->stFeLscBlc.stStaticRegCfg.bBlcOut);
                    /*Fe RC*/
                    isp_rc_blc_in_en_write(ViPipeBind, pstFeBlcCfg->stRcBlc.stStaticRegCfg.bBlcIn);
                    isp_rc_blc_out_en_write(ViPipeBind, pstFeBlcCfg->stRcBlc.stStaticRegCfg.bBlcOut);
                }

                if (pstFeBlcCfg->bReshDyna)
                {
                    /*Fe Dg*/
                    isp_dg2_ofsr_write(ViPipeBind, pstFeBlcCfg->stFeDgBlc.stUsrRegCfg.au16Blc[0]);
                    isp_dg2_ofsgr_write(ViPipeBind, pstFeBlcCfg->stFeDgBlc.stUsrRegCfg.au16Blc[1]);
                    isp_dg2_ofsgb_write(ViPipeBind, pstFeBlcCfg->stFeDgBlc.stUsrRegCfg.au16Blc[2]);
                    isp_dg2_ofsb_write(ViPipeBind, pstFeBlcCfg->stFeDgBlc.stUsrRegCfg.au16Blc[3]);

                    /*Fe WB*/
                    isp_wb1_ofsr_write(ViPipeBind, pstFeBlcCfg->stFeWbBlc.stUsrRegCfg.au16Blc[0]);
                    isp_wb1_ofsgr_write(ViPipeBind, pstFeBlcCfg->stFeWbBlc.stUsrRegCfg.au16Blc[1]);
                    isp_wb1_ofsgb_write(ViPipeBind, pstFeBlcCfg->stFeWbBlc.stUsrRegCfg.au16Blc[2]);
                    isp_wb1_ofsb_write(ViPipeBind, pstFeBlcCfg->stFeWbBlc.stUsrRegCfg.au16Blc[3]);

                    /*Fe AE*/
                    isp_ae1_offset_r_write(ViPipeBind,  pstFeBlcCfg->stFeAeBlc.stUsrRegCfg.au16Blc[0]);
                    isp_ae1_offset_gr_write(ViPipeBind, pstFeBlcCfg->stFeAeBlc.stUsrRegCfg.au16Blc[1]);
                    isp_ae1_offset_gb_write(ViPipeBind, pstFeBlcCfg->stFeAeBlc.stUsrRegCfg.au16Blc[2]);
                    isp_ae1_offset_b_write(ViPipeBind,  pstFeBlcCfg->stFeAeBlc.stUsrRegCfg.au16Blc[3]);
                    /*Fe LSC*/
                    isp_lsc1_blc_r_write(ViPipeBind, pstFeBlcCfg->stFeLscBlc.stUsrRegCfg.au16Blc[0]);
                    isp_lsc1_blc_gr_write(ViPipeBind, pstFeBlcCfg->stFeLscBlc.stUsrRegCfg.au16Blc[1]);
                    isp_lsc1_blc_gb_write(ViPipeBind, pstFeBlcCfg->stFeLscBlc.stUsrRegCfg.au16Blc[2]);
                    isp_lsc1_blc_b_write(ViPipeBind, pstFeBlcCfg->stFeLscBlc.stUsrRegCfg.au16Blc[3]);
                    /*Fe Rc*/
                    isp_rc_blc_r_write(ViPipeBind, pstFeBlcCfg->stRcBlc.stUsrRegCfg.au16Blc[0]);
                    isp_rc_blc_gr_write(ViPipeBind, pstFeBlcCfg->stRcBlc.stUsrRegCfg.au16Blc[1]);
                    isp_rc_blc_gb_write(ViPipeBind, pstFeBlcCfg->stRcBlc.stUsrRegCfg.au16Blc[2]);
                    isp_rc_blc_b_write(ViPipeBind, pstFeBlcCfg->stRcBlc.stUsrRegCfg.au16Blc[3]);

                    /*Fe LogLUT*/
                    isp_loglut1_offset_r_write(ViPipeBind, pstFeBlcCfg->stFeLogLUTBlc.stUsrRegCfg.au16Blc[0]);
                    isp_loglut1_offset_gr_write(ViPipeBind, pstFeBlcCfg->stFeLogLUTBlc.stUsrRegCfg.au16Blc[1]);
                    isp_loglut1_offset_gb_write(ViPipeBind, pstFeBlcCfg->stFeLogLUTBlc.stUsrRegCfg.au16Blc[2]);
                    isp_loglut1_offset_b_write(ViPipeBind, pstFeBlcCfg->stFeLogLUTBlc.stUsrRegCfg.au16Blc[3]);
                }
            }
        }

        pstFeBlcCfg->bReshStatic = HI_FALSE;
        pstFeBlcCfg->bReshDyna   = HI_FALSE;
        pstRegCfgInfo->unKey.bit1FeBlcCfg = 0;
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_FeAeRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo)
{
    HI_BOOL bLutUpdate = HI_FALSE;
    HI_U16  i, j, k;
    HI_U16  u16CropWidth;
    HI_U32  u32TableWeightTmp = 0;
    HI_U32  u32CombinWeight = 0;
    HI_U32  u32CombinWeightNum = 0;
    VI_PIPE ViPipeBind;
    ISP_AE_STATIC_CFG_S *pstStaticRegFeCfg = HI_NULL;
    ISP_AE_DYNA_CFG_S   *pstDynaRegFeCfg   = HI_NULL;
    ISP_CTX_S *pstIspCtx   = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (pstIspCtx->stWdrAttr.bMastPipe && pstRegCfgInfo->unKey.bit1AeCfg1)
    {
        for (i = 0; i < pstIspCtx->stWdrAttr.stDevBindPipe.u32Num; i++)
        {
            ViPipeBind = pstIspCtx->stWdrAttr.stDevBindPipe.PipeId[i];
            ISP_CHECK_PIPE(ViPipeBind);
            //ae fe static
            pstStaticRegFeCfg = &pstRegCfgInfo->stAlgRegCfg[0].stAeRegCfg.stStaticRegCfg;
            pstDynaRegFeCfg   = &pstRegCfgInfo->stAlgRegCfg[0].stAeRegCfg.stDynaRegCfg;
            isp_fe_ae1_en_write(ViPipeBind, pstStaticRegFeCfg->u8FEEnable);
            isp_ae1_crop_pos_x_write(ViPipeBind, pstStaticRegFeCfg->u16FECropPosX);
            isp_ae1_crop_pos_y_write(ViPipeBind, pstStaticRegFeCfg->u16FECropPosY);
            u16CropWidth = IS_HRS_ON(pstIspCtx->stBlockAttr.enIspRunningMode) ? (pstStaticRegFeCfg->u16FECropOutWidth >> 1) : pstStaticRegFeCfg->u16FECropOutWidth;
            isp_ae1_crop_out_width_write(ViPipeBind, u16CropWidth - 1);
            isp_ae1_crop_out_height_write(ViPipeBind, pstStaticRegFeCfg->u16FECropOutHeight - 1);

            //ae fe dynamic
            isp_ae1_hnum_write(ViPipeBind, pstDynaRegFeCfg->u8FEWeightTableWidth);
            isp_ae1_vnum_write(ViPipeBind, pstDynaRegFeCfg->u8FEWeightTableHeight);
            isp_ae1_skip_x_write(ViPipeBind, pstDynaRegFeCfg->u8FEHistSkipX);
            isp_ae1_offset_x_write(ViPipeBind, pstDynaRegFeCfg->u8FEHistOffsetX);
            isp_ae1_skip_y_write(ViPipeBind, pstDynaRegFeCfg->u8FEHistSkipY);
            isp_ae1_offset_y_write(ViPipeBind, pstDynaRegFeCfg->u8FEHistOffsetY);
            isp_ae1_bitmove_write(ViPipeBind, pstDynaRegFeCfg->u8FEBitMove);
            isp_ae1_hist_gamma_mode_write(ViPipeBind, pstDynaRegFeCfg->u8FEHistGammaMode);
            isp_ae1_aver_gamma_mode_write(ViPipeBind, pstDynaRegFeCfg->u8FEAverGammaMode);
            isp_ae1_gamma_limit_write(ViPipeBind, pstDynaRegFeCfg->u8FEGammaLimit);
            isp_ae1_fourplanemode_write(ViPipeBind, pstDynaRegFeCfg->u8FEFourPlaneMode);

            u32CombinWeight = 0;
            u32CombinWeightNum = 0;

            isp_ae1_wei_waddr_write(ViPipeBind, 0);

            for (j = 0; j < AE_ZONE_ROW; j++)
            {
                for (k = 0; k < AE_ZONE_COLUMN; k++)
                {
                    u32TableWeightTmp = (HI_U32)pstDynaRegFeCfg->au8FEWeightTable[j][k];
                    u32CombinWeight |= (u32TableWeightTmp << (8 * u32CombinWeightNum));
                    u32CombinWeightNum++;

                    if (u32CombinWeightNum == HI_ISP_AE_WEI_COMBIN_COUNT)
                    {
                        isp_ae1_wei_wdata_write(ViPipeBind, u32CombinWeight);
                        u32CombinWeightNum = 0;
                        u32CombinWeight = 0;
                    }
                }
            }

            if ((u32CombinWeightNum != HI_ISP_AE_WEI_COMBIN_COUNT) && (u32CombinWeightNum != 0))
            {
                isp_ae1_wei_wdata_write(ViPipeBind, u32CombinWeight);
            }

            bLutUpdate = pstDynaRegFeCfg->u8FEWightTableUpdate;
            //isp_ae1_lut_update_write(ViPipeBind, pstDynaRegFeCfg->u8FEWightTableUpdate);
        }
    }

    pstRegCfgInfo->stAlgRegCfg[0].stFeLutUpdateCfg.bAe1LutUpdate = bLutUpdate;
    return HI_SUCCESS;
}


static HI_S32 ISP_AeRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL bLutUpdate = HI_FALSE;
    HI_U16  j, k, m;
    //HI_U32 u32TableWeightTmp = 0;
    HI_U32  u32CombinWeight = 0;
    HI_U32  u32CombinWeightNum = 0;
    ISP_AE_STATIC_CFG_S *pstStaticRegBeCfg = HI_NULL;
    ISP_AE_DYNA_CFG_S   *pstDynaRegBeCfg = HI_NULL;
    ISP_MG_STATIC_CFG_S *pstMgStaticRegCfg = HI_NULL;
    ISP_MG_DYNA_CFG_S   *pstMgDynaRegCfg = HI_NULL;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    HI_U32 au32CombineWgt[64] = {0};

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (pstRegCfgInfo->unKey.bit1AeCfg1)
    {
        //ae be static
        pstStaticRegBeCfg = &pstRegCfgInfo->stAlgRegCfg[i].stAeRegCfg.stStaticRegCfg;
        pstDynaRegBeCfg   = &pstRegCfgInfo->stAlgRegCfg[i].stAeRegCfg.stDynaRegCfg;

        isp_ae_en_write(ViPipe, i, pstStaticRegBeCfg->u8BEEnable);
        isp_ae_crop_pos_x_write(ViPipe, i, pstStaticRegBeCfg->u16BECropPosX);
        isp_ae_crop_pos_y_write(ViPipe, i, pstStaticRegBeCfg->u16BECropPosY);
        isp_ae_crop_out_width_write(ViPipe, i, pstStaticRegBeCfg->u16BECropOutWidth - 1);
        isp_ae_crop_out_height_write(ViPipe, i, pstStaticRegBeCfg->u16BECropOutHeight - 1);
        //ae be dynamic
        isp_ae_sel_write(ViPipe, i, pstDynaRegBeCfg->u8BEAESel);
        isp_ae_hnum_write(ViPipe, i, pstDynaRegBeCfg->u8BEWeightTableWidth);
        isp_ae_vnum_write(ViPipe, i, pstDynaRegBeCfg->u8BEWeightTableHeight);
        isp_ae_skip_x_write(ViPipe, i, pstDynaRegBeCfg->u8BEHistSkipX);
        isp_ae_offset_x_write(ViPipe, i, pstDynaRegBeCfg->u8BEHistOffsetX);
        isp_ae_skip_y_write(ViPipe, i, pstDynaRegBeCfg->u8BEHistSkipY);
        isp_ae_offset_y_write(ViPipe, i, pstDynaRegBeCfg->u8BEHistOffsetY);
        isp_ae_bitmove_write(ViPipe, i, pstDynaRegBeCfg->u8BEBitMove);
        isp_ae_hist_gamma_mode_write(ViPipe, i, pstDynaRegBeCfg->u8BEHistGammaMode);
        isp_ae_aver_gamma_mode_write(ViPipe, i, pstDynaRegBeCfg->u8BEAverGammaMode);
        isp_ae_gamma_limit_write(ViPipe, i, pstDynaRegBeCfg->u8BEGammaLimit);
        isp_ae_four_plane_mode_write(ViPipe, i, pstDynaRegBeCfg->u8BEFourPlaneMode);

        m = 0;
        u32CombinWeight = 0;
        u32CombinWeightNum = 0;

        for (j = 0; j < pstDynaRegBeCfg->u8BEWeightTableHeight; j++)
        {
            for (k = 0; k < pstDynaRegBeCfg->u8BEWeightTableWidth; k++)
            {
                u32CombinWeight |= (pstDynaRegBeCfg->au8BEWeightTable[j][k] << (8 * u32CombinWeightNum));
                u32CombinWeightNum++;

                if (u32CombinWeightNum == HI_ISP_AE_WEI_COMBIN_COUNT)
                {
                    //isp_ae_wei_wdata_write(ViPipe, i, u32CombinWeight);
                    au32CombineWgt[m++] = u32CombinWeight;
                    u32CombinWeightNum = 0;
                    u32CombinWeight = 0;
                }
            }
        }

        if (u32CombinWeightNum != HI_ISP_AE_WEI_COMBIN_COUNT
            && u32CombinWeightNum != 0)
        {
            //isp_ae_wei_wdata_write(ViPipe, i, u32CombinWeight);
            au32CombineWgt[m++] = u32CombinWeight;
        }

        if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
            || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
        {
            isp_ae_wei_waddr_write(ViPipe, i, 0);

            for (m = 0; m < 64; m++)
            {
                isp_ae_wei_wdata_write(ViPipe, i, au32CombineWgt[m]);
            }
        }
        else
        {
            isp_ae_weight_write(ViPipe, i, au32CombineWgt);
        }

        //isp_ae_lut_update_write(ViPipe, i, pstDynaRegBeCfg->u8BEWightTableUpdate);
        bLutUpdate = pstDynaRegBeCfg->u8BEWightTableUpdate;

        /*mg static*/
        pstMgStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stMgRegCfg.stStaticRegCfg;
        pstMgDynaRegCfg   = &pstRegCfgInfo->stAlgRegCfg[i].stMgRegCfg.stDynaRegCfg;

        isp_la_en_write(ViPipe, i, pstMgStaticRegCfg->u8Enable);
        isp_la_crop_pos_x_write(ViPipe, i, pstMgStaticRegCfg->u16CropPosX);
        isp_la_crop_pos_y_write(ViPipe, i, pstMgStaticRegCfg->u16CropPosY);
        isp_la_crop_out_width_write(ViPipe, i, pstMgStaticRegCfg->u16CropOutWidth - 1);
        isp_la_crop_out_height_write(ViPipe, i, pstMgStaticRegCfg->u16CropOutHeight - 1);

        /*mg dynamic*/
        isp_la_hnum_write(ViPipe, i, pstMgDynaRegCfg->u8ZoneWidth);
        isp_la_vnum_write(ViPipe, i, pstMgDynaRegCfg->u8ZoneHeight);
        isp_la_bitmove_write(ViPipe, i, pstMgDynaRegCfg->u8BitMove);
        isp_la_gamma_en_write(ViPipe, i, pstMgDynaRegCfg->u8GammaMode);
        isp_la_gamma_limit_write(ViPipe, i, pstMgDynaRegCfg->u8GammaLimit);
    }

    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bAeLutUpdate = bLutUpdate;

    return HI_SUCCESS;
}


static HI_S32 ISP_FeAfRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo)
{
    HI_U32 i;
    VI_PIPE ViPipeBind;
    ISP_AF_REG_CFG_S *pstAfRegFeCfg = HI_NULL;
    ISP_CTX_S *pstIspCtx   = HI_NULL;
    HI_U8 u8AfEnbale[ISP_MAX_PIPE_NUM] = {ISP_PIPE_FEAF_MODULE_ENABLE};
    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (pstIspCtx->stWdrAttr.bMastPipe && pstRegCfgInfo->unKey.bit1AfFeCfg)
    {
        for (i = 0; i < pstIspCtx->stWdrAttr.stDevBindPipe.u32Num; i++)
        {
            ViPipeBind = pstIspCtx->stWdrAttr.stDevBindPipe.PipeId[i];

            if (!u8AfEnbale[ViPipeBind])
            {
                continue;
            }

            ISP_CHECK_PIPE(ViPipeBind);
            pstAfRegFeCfg    = &pstRegCfgInfo->stAlgRegCfg[0].stFEAfRegCfg;
            isp_af1_bayer_mode_write(ViPipeBind, pstAfRegFeCfg->u8BayerMode);
            isp_af1_lpf_en_write(ViPipeBind, pstAfRegFeCfg->bLpfEnable);
            isp_af1_fir0_lpf_en_write(ViPipeBind, pstAfRegFeCfg->bFir0LpfEnable);
            isp_af1_fir1_lpf_en_write(ViPipeBind, pstAfRegFeCfg->bFir1LpfEnable);
            isp_af1_iir0_ds_en_write(ViPipeBind, pstAfRegFeCfg->bIir0DsEnable);
            isp_af1_iir1_ds_en_write(ViPipeBind, pstAfRegFeCfg->bIir1DsEnable);
            isp_af1_iir_delay0_write(ViPipeBind,  pstAfRegFeCfg->u8Iir0Shift);
            isp_af1_iir_delay1_write(ViPipeBind,  pstAfRegFeCfg->u8Iir1Shift);
            isp_af1_iirplg_0_write(ViPipeBind, pstAfRegFeCfg->u8IirPlgGroup0);
            isp_af1_iirpls_0_write(ViPipeBind, pstAfRegFeCfg->u8IirPlsGroup0);
            isp_af1_iirplg_1_write(ViPipeBind, pstAfRegFeCfg->u8IirPlgGroup1);
            isp_af1_iirpls_1_write(ViPipeBind, pstAfRegFeCfg->u8IirPlsGroup1);

            isp_fe_af1_en_write(ViPipeBind, pstAfRegFeCfg->bAfEnable);
            isp_af1_iir0_en0_write(ViPipeBind, pstAfRegFeCfg->bIir0Enable0);
            isp_af1_iir0_en1_write(ViPipeBind, pstAfRegFeCfg->bIir0Enable1);
            isp_af1_iir0_en2_write(ViPipeBind, pstAfRegFeCfg->bIir0Enable2);
            isp_af1_iir1_en0_write(ViPipeBind, pstAfRegFeCfg->bIir1Enable0);
            isp_af1_iir1_en1_write(ViPipeBind, pstAfRegFeCfg->bIir1Enable1);
            isp_af1_iir1_en2_write(ViPipeBind, pstAfRegFeCfg->bIir1Enable2);
            isp_af1_peak_mode_write(ViPipeBind, pstAfRegFeCfg->enPeakMode);
            isp_af1_squ_mode_write(ViPipeBind, pstAfRegFeCfg->enSquMode);
            isp_af1_hnum_write(ViPipeBind, pstAfRegFeCfg->u16WindowHnum);
            isp_af1_vnum_write(ViPipeBind, pstAfRegFeCfg->u16WindowVnum);

            isp_af1_iirg0_0_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16IirGain0Group0);
            isp_af1_iirg0_1_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16IirGain0Group1);

            isp_af1_iirg1_0_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16IirGain1Group0);
            isp_af1_iirg1_1_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16IirGain1Group1);

            isp_af1_iirg2_0_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16IirGain2Group0);
            isp_af1_iirg2_1_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16IirGain2Group1);

            isp_af1_iirg3_0_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16IirGain3Group0);
            isp_af1_iirg3_1_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16IirGain3Group1);

            isp_af1_iirg4_0_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16IirGain4Group0);
            isp_af1_iirg4_1_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16IirGain4Group1);

            isp_af1_iirg5_0_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16IirGain5Group0);
            isp_af1_iirg5_1_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16IirGain5Group1);

            isp_af1_iirg6_0_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16IirGain6Group0);
            isp_af1_iirg6_1_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16IirGain6Group1);

            isp_af1_iirshift0_0_write(ViPipeBind, pstAfRegFeCfg->u16Iir0ShiftGroup0);
            isp_af1_iirshift0_1_write(ViPipeBind, pstAfRegFeCfg->u16Iir1ShiftGroup0);
            isp_af1_iirshift0_2_write(ViPipeBind, pstAfRegFeCfg->u16Iir2ShiftGroup0);
            isp_af1_iirshift0_3_write(ViPipeBind, pstAfRegFeCfg->u16Iir3ShiftGroup0);
            isp_af1_iirshift1_0_write(ViPipeBind, pstAfRegFeCfg->u16Iir0ShiftGroup1);
            isp_af1_iirshift1_1_write(ViPipeBind, pstAfRegFeCfg->u16Iir1ShiftGroup1);
            isp_af1_iirshift1_2_write(ViPipeBind, pstAfRegFeCfg->u16Iir2ShiftGroup1);
            isp_af1_iirshift1_3_write(ViPipeBind, pstAfRegFeCfg->u16Iir3ShiftGroup1);

            isp_af1_firh0_0_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16FirHGain0Group0);
            isp_af1_firh0_1_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16FirHGain0Group1);

            isp_af1_firh1_0_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16FirHGain1Group0);
            isp_af1_firh1_1_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16FirHGain1Group1);

            isp_af1_firh2_0_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16FirHGain2Group0);
            isp_af1_firh2_1_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16FirHGain2Group1);

            isp_af1_firh3_0_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16FirHGain3Group0);
            isp_af1_firh3_1_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16FirHGain3Group1);

            isp_af1_firh4_0_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16FirHGain4Group0);
            isp_af1_firh4_1_write(ViPipeBind, (HI_U32)pstAfRegFeCfg->s16FirHGain4Group1);

            /* AF FE crop */
            isp_af1_crop_en_write(ViPipeBind, pstAfRegFeCfg->bCropEnable);
            isp_af1_pos_x_write(ViPipeBind, pstAfRegFeCfg->u16CropPosX);
            isp_af1_pos_y_write(ViPipeBind, pstAfRegFeCfg->u16CropPosY);
            isp_af1_crop_hsize_write(ViPipeBind, pstAfRegFeCfg->u16CropHsize);
            isp_af1_crop_vsize_write(ViPipeBind, pstAfRegFeCfg->u16CropVsize);

            /* AF FE raw cfg */
            isp_af1_raw_mode_write(ViPipeBind, pstAfRegFeCfg->bRawMode);
            isp_af1_gain_lmt_write(ViPipeBind, pstAfRegFeCfg->u8GainLimit);
            isp_af1_gamma_write(ViPipeBind, pstAfRegFeCfg->u8Gamma);
            isp_af1_bayer_mode_write(ViPipeBind, pstAfRegFeCfg->u8BayerMode);
            isp_af1_offset_en_write(ViPipeBind, pstAfRegFeCfg->bOffsetEnable);
            isp_af1_offset_gr_write(ViPipeBind, pstAfRegFeCfg->u16OffsetGr);
            isp_af1_offset_gb_write(ViPipeBind, pstAfRegFeCfg->u16OffsetGb);

            /* AF FE pre median filter */
            isp_af1_mean_en_write(ViPipeBind, pstAfRegFeCfg->bMeanEnable);
            isp_af1_mean_thres_write(ViPipeBind, 0xffff - pstAfRegFeCfg->u16MeanThres);

            /* level depend gain */
            isp_af1_iir0_ldg_en_write(ViPipeBind, pstAfRegFeCfg->bIir0LdgEnable);
            isp_af1_iir_thre0_l_write(ViPipeBind, pstAfRegFeCfg->u16IirThre0Low);
            isp_af1_iir_thre0_h_write(ViPipeBind, pstAfRegFeCfg->u16IirThre0High);
            isp_af1_iir_slope0_l_write(ViPipeBind, pstAfRegFeCfg->u16IirSlope0Low);
            isp_af1_iir_slope0_h_write(ViPipeBind, pstAfRegFeCfg->u16IirSlope0High);
            isp_af1_iir_gain0_l_write(ViPipeBind, pstAfRegFeCfg->u16IirGain0Low);
            isp_af1_iir_gain0_h_write(ViPipeBind, pstAfRegFeCfg->u16IirGain0High);

            isp_af1_iir1_ldg_en_write(ViPipeBind, pstAfRegFeCfg->bIir1LdgEnable);
            isp_af1_iir_thre1_l_write(ViPipeBind, pstAfRegFeCfg->u16IirThre1Low);
            isp_af1_iir_thre1_h_write(ViPipeBind, pstAfRegFeCfg->u16IirThre1High);
            isp_af1_iir_slope1_l_write(ViPipeBind, pstAfRegFeCfg->u16IirSlope1Low);
            isp_af1_iir_slope1_h_write(ViPipeBind, pstAfRegFeCfg->u16IirSlope1High);
            isp_af1_iir_gain1_l_write(ViPipeBind, pstAfRegFeCfg->u16IirGain1Low);
            isp_af1_iir_gain1_h_write(ViPipeBind, pstAfRegFeCfg->u16IirGain1High);

            isp_af1_fir0_ldg_en_write(ViPipeBind, pstAfRegFeCfg->bFir0LdgEnable);
            isp_af1_fir_thre0_l_write(ViPipeBind, pstAfRegFeCfg->u16FirThre0Low);
            isp_af1_fir_thre0_h_write(ViPipeBind, pstAfRegFeCfg->u16FirThre0High);
            isp_af1_fir_slope0_l_write(ViPipeBind, pstAfRegFeCfg->u16FirSlope0Low);
            isp_af1_fir_slope0_h_write(ViPipeBind, pstAfRegFeCfg->u16FirSlope0High);
            isp_af1_fir_gain0_l_write(ViPipeBind, pstAfRegFeCfg->u16FirGain0Low);
            isp_af1_fir_gain0_h_write(ViPipeBind, pstAfRegFeCfg->u16FirGain0High);

            isp_af1_fir1_ldg_en_write(ViPipeBind, pstAfRegFeCfg->bFir1LdgEnable);
            isp_af1_fir_thre1_l_write(ViPipeBind, pstAfRegFeCfg->u16FirThre1Low);
            isp_af1_fir_thre1_h_write(ViPipeBind, pstAfRegFeCfg->u16FirThre1High);
            isp_af1_fir_slope1_l_write(ViPipeBind, pstAfRegFeCfg->u16FirSlope1Low);
            isp_af1_fir_slope1_h_write(ViPipeBind, pstAfRegFeCfg->u16FirSlope1High);
            isp_af1_fir_gain1_l_write(ViPipeBind, pstAfRegFeCfg->u16FirGain1Low);
            isp_af1_fir_gain1_h_write(ViPipeBind, pstAfRegFeCfg->u16FirGain1High);

            /* AF FE coring */
            isp_af1_iir_thre0_c_write(ViPipeBind, pstAfRegFeCfg->u16IirThre0Coring);
            isp_af1_iir_slope0_c_write(ViPipeBind, pstAfRegFeCfg->u16IirSlope0Coring);
            isp_af1_iir_peak0_c_write(ViPipeBind, pstAfRegFeCfg->u16IirPeak0Coring);

            isp_af1_iir_thre1_c_write(ViPipeBind, pstAfRegFeCfg->u16IirThre1Coring);
            isp_af1_iir_slope1_c_write(ViPipeBind, pstAfRegFeCfg->u16IirSlope1Coring);
            isp_af1_iir_peak1_c_write(ViPipeBind, pstAfRegFeCfg->u16IirPeak1Coring);

            isp_af1_fir_thre0_c_write(ViPipeBind, pstAfRegFeCfg->u16FirThre0Coring);
            isp_af1_fir_slope0_c_write(ViPipeBind, pstAfRegFeCfg->u16FirSlope0Coring);
            isp_af1_fir_peak0_c_write(ViPipeBind, pstAfRegFeCfg->u16FirPeak0Coring);

            isp_af1_fir_thre1_c_write(ViPipeBind, pstAfRegFeCfg->u16FirThre1Coring);
            isp_af1_fir_slope1_c_write(ViPipeBind, pstAfRegFeCfg->u16IirSlope1Coring);
            isp_af1_fir_peak1_c_write(ViPipeBind, pstAfRegFeCfg->u16FirPeak1Coring);

            /* high luma counter */
            isp_af1_hilight_write(ViPipeBind, pstAfRegFeCfg->u8HilighThre);

            /* AF output shift */
            isp_af1_acc_shift0_h_write(ViPipeBind, pstAfRegFeCfg->u16AccShift0H );
            isp_af1_acc_shift1_h_write(ViPipeBind, pstAfRegFeCfg->u16AccShift1H );
            isp_af1_acc_shift0_v_write(ViPipeBind, pstAfRegFeCfg->u16AccShift0V );
            isp_af1_acc_shift1_v_write(ViPipeBind, pstAfRegFeCfg->u16AccShift1V );
            isp_af1_acc_shift_y_write(ViPipeBind, pstAfRegFeCfg->u16AccShiftY );
            isp_af1_cnt_shift_y_write(ViPipeBind, pstAfRegFeCfg->u16ShiftCountY);
            isp_af1_cnt_shift0_h_write(ViPipeBind, 0x0);
            isp_af1_cnt_shift1_h_write(ViPipeBind, 0x0);
            isp_af1_cnt_shift0_v_write(ViPipeBind, ISP_AF_CNT_SHIFT0_V_DEFAULT);
            isp_af1_cnt_shift1_v_write(ViPipeBind, 0x0);
        }

        pstRegCfgInfo->unKey.bit1AfFeCfg = 0;
    }


    return HI_SUCCESS;
}

static HI_S32 ISP_AfRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL  bIsOfflineMode;
    HI_BOOL  bUsrResh  = HI_FALSE;
    HI_BOOL  bIdxResh  = HI_FALSE;
    ISP_AF_REG_CFG_S *pstAfRegBeCfg = HI_NULL;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAfRegBeCfg  = &pstRegCfgInfo->stAlgRegCfg[i].stBEAfRegCfg;
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    bIdxResh = (isp_af_update_index_read(ViPipe, i) != pstAfRegBeCfg->u32UpdateIndex);
    bUsrResh = (bIsOfflineMode) ? (pstRegCfgInfo->unKey.bit1AfBeCfg & bIdxResh) : (pstRegCfgInfo->unKey.bit1AfBeCfg);

    if (bUsrResh)
    {
        isp_af_update_index_write(ViPipe, i, pstAfRegBeCfg->u32UpdateIndex);
        isp_af_bayer_mode_write(ViPipe, i, pstAfRegBeCfg->u8BayerMode);
        isp_af_lpf_en_write(ViPipe, i, pstAfRegBeCfg->bLpfEnable);
        isp_af_fir0_lpf_en_write(ViPipe, i, pstAfRegBeCfg->bFir0LpfEnable);
        isp_af_fir1_lpf_en_write(ViPipe, i, pstAfRegBeCfg->bFir1LpfEnable);
        isp_af_iir0_ds_en_write(ViPipe, i, pstAfRegBeCfg->bIir0DsEnable);
        isp_af_iir1_ds_en_write(ViPipe, i, pstAfRegBeCfg->bIir1DsEnable);
        isp_af_iir_delay0_write(ViPipe, i, pstAfRegBeCfg->u8Iir0Shift);
        isp_af_iir_delay1_write(ViPipe, i, pstAfRegBeCfg->u8Iir1Shift);
        isp_af_iirplg_0_write(ViPipe, i, pstAfRegBeCfg->u8IirPlgGroup0);
        isp_af_iirpls_0_write(ViPipe, i, pstAfRegBeCfg->u8IirPlsGroup0);
        isp_af_iirplg_1_write(ViPipe, i, pstAfRegBeCfg->u8IirPlgGroup1);
        isp_af_iirpls_1_write(ViPipe, i, pstAfRegBeCfg->u8IirPlsGroup1);

        isp_af_en_write(ViPipe, i, pstAfRegBeCfg->bAfEnable);
        isp_af_iir0_en0_write(ViPipe, i, pstAfRegBeCfg->bIir0Enable0);
        isp_af_iir0_en1_write(ViPipe, i, pstAfRegBeCfg->bIir0Enable1);
        isp_af_iir0_en2_write(ViPipe, i, pstAfRegBeCfg->bIir0Enable2);
        isp_af_iir1_en0_write(ViPipe, i, pstAfRegBeCfg->bIir1Enable0);
        isp_af_iir1_en1_write(ViPipe, i, pstAfRegBeCfg->bIir1Enable1);
        isp_af_iir1_en2_write(ViPipe, i, pstAfRegBeCfg->bIir1Enable2);
        isp_af_peak_mode_write(ViPipe, i, pstAfRegBeCfg->enPeakMode);
        isp_af_squ_mode_write(ViPipe, i, pstAfRegBeCfg->enSquMode);
        isp_af_hnum_write(ViPipe, i, pstAfRegBeCfg->u16WindowHnum);
        isp_af_vnum_write(ViPipe, i, pstAfRegBeCfg->u16WindowVnum);

        isp_af_iirg0_0_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16IirGain0Group0);
        isp_af_iirg0_1_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16IirGain0Group1);

        isp_af_iirg1_0_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16IirGain1Group0);
        isp_af_iirg1_1_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16IirGain1Group1);

        isp_af_iirg2_0_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16IirGain2Group0);
        isp_af_iirg2_1_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16IirGain2Group1);

        isp_af_iirg3_0_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16IirGain3Group0);
        isp_af_iirg3_1_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16IirGain3Group1);

        isp_af_iirg4_0_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16IirGain4Group0);
        isp_af_iirg4_1_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16IirGain4Group1);

        isp_af_iirg5_0_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16IirGain5Group0);
        isp_af_iirg5_1_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16IirGain5Group1);

        isp_af_iirg6_0_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16IirGain6Group0);
        isp_af_iirg6_1_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16IirGain6Group1);

        isp_af_iirshift0_0_write(ViPipe, i, pstAfRegBeCfg->u16Iir0ShiftGroup0);
        isp_af_iirshift0_1_write(ViPipe, i, pstAfRegBeCfg->u16Iir1ShiftGroup0);
        isp_af_iirshift0_2_write(ViPipe, i, pstAfRegBeCfg->u16Iir2ShiftGroup0);
        isp_af_iirshift0_3_write(ViPipe, i, pstAfRegBeCfg->u16Iir3ShiftGroup0);
        isp_af_iirshift1_0_write(ViPipe, i, pstAfRegBeCfg->u16Iir0ShiftGroup1);
        isp_af_iirshift1_1_write(ViPipe, i, pstAfRegBeCfg->u16Iir1ShiftGroup1);
        isp_af_iirshift1_2_write(ViPipe, i, pstAfRegBeCfg->u16Iir2ShiftGroup1);
        isp_af_iirshift1_3_write(ViPipe, i, pstAfRegBeCfg->u16Iir3ShiftGroup1);

        isp_af_firh0_0_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16FirHGain0Group0);
        isp_af_firh0_1_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16FirHGain0Group1);

        isp_af_firh1_0_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16FirHGain1Group0);
        isp_af_firh1_1_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16FirHGain1Group1);

        isp_af_firh2_0_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16FirHGain2Group0);
        isp_af_firh2_1_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16FirHGain2Group1);

        isp_af_firh3_0_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16FirHGain3Group0);
        isp_af_firh3_1_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16FirHGain3Group1);

        isp_af_firh4_0_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16FirHGain4Group0);
        isp_af_firh4_1_write(ViPipe, i, (HI_U32)pstAfRegBeCfg->s16FirHGain4Group1);

        /* AF BE crop */
        isp_af_crop_en_write(ViPipe, i, pstAfRegBeCfg->bCropEnable);
        isp_af_pos_x_write(ViPipe, i, pstAfRegBeCfg->u16CropPosX);
        isp_af_pos_y_write(ViPipe, i, pstAfRegBeCfg->u16CropPosY);
        isp_af_crop_hsize_write(ViPipe, i, pstAfRegBeCfg->u16CropHsize);
        isp_af_crop_vsize_write(ViPipe, i, pstAfRegBeCfg->u16CropVsize);

        /* AF BE raw cfg */
        isp_af_sel_write(ViPipe, i, pstAfRegBeCfg->u8AfPosSel);
        isp_af_raw_mode_write(ViPipe, i, pstAfRegBeCfg->bRawMode);
        isp_af_gain_lmt_write(ViPipe, i, pstAfRegBeCfg->u8GainLimit);
        isp_af_gamma_write(ViPipe, i, pstAfRegBeCfg->u8Gamma);
        isp_af_bayer_mode_write(ViPipe, i, pstAfRegBeCfg->u8BayerMode);
        isp_af_offset_en_write(ViPipe, i, pstAfRegBeCfg->bOffsetEnable);
        isp_af_offset_gr_write(ViPipe, i, pstAfRegBeCfg->u16OffsetGr);
        isp_af_offset_gb_write(ViPipe, i, pstAfRegBeCfg->u16OffsetGb);

        /* AF BE pre median filter */
        isp_af_mean_en_write(ViPipe, i, pstAfRegBeCfg->bMeanEnable);
        isp_af_mean_thres_write(ViPipe, i, 0xffff - pstAfRegBeCfg->u16MeanThres);

        /* level depend gain */
        isp_af_iir0_ldg_en_write(ViPipe, i, pstAfRegBeCfg->bIir0LdgEnable);
        isp_af_iir_thre0_l_write      (ViPipe, i, pstAfRegBeCfg->u16IirThre0Low    );
        isp_af_iir_thre0_h_write      (ViPipe, i, pstAfRegBeCfg->u16IirThre0High   );
        isp_af_iir_slope0_l_write     (ViPipe, i, pstAfRegBeCfg->u16IirSlope0Low   );
        isp_af_iir_slope0_h_write     (ViPipe, i, pstAfRegBeCfg->u16IirSlope0High  );
        isp_af_iir_gain0_l_write      (ViPipe, i, pstAfRegBeCfg->u16IirGain0Low    );
        isp_af_iir_gain0_h_write      (ViPipe, i, pstAfRegBeCfg->u16IirGain0High   );

        isp_af_iir1_ldg_en_write(ViPipe, i, pstAfRegBeCfg->bIir1LdgEnable);
        isp_af_iir_thre1_l_write      (ViPipe, i, pstAfRegBeCfg->u16IirThre1Low    );
        isp_af_iir_thre1_h_write      (ViPipe, i, pstAfRegBeCfg->u16IirThre1High   );
        isp_af_iir_slope1_l_write     (ViPipe, i, pstAfRegBeCfg->u16IirSlope1Low   );
        isp_af_iir_slope1_h_write     (ViPipe, i, pstAfRegBeCfg->u16IirSlope1High  );
        isp_af_iir_gain1_l_write      (ViPipe, i, pstAfRegBeCfg->u16IirGain1Low  );
        isp_af_iir_gain1_h_write      (ViPipe, i, pstAfRegBeCfg->u16IirGain1High );

        isp_af_fir0_ldg_en_write(ViPipe, i, pstAfRegBeCfg->bFir0LdgEnable);
        isp_af_fir_thre0_l_write      (ViPipe, i, pstAfRegBeCfg->u16FirThre0Low    );
        isp_af_fir_thre0_h_write      (ViPipe, i, pstAfRegBeCfg->u16FirThre0High   );
        isp_af_fir_slope0_l_write     (ViPipe, i, pstAfRegBeCfg->u16FirSlope0Low    );
        isp_af_fir_slope0_h_write     (ViPipe, i, pstAfRegBeCfg->u16FirSlope0High   );
        isp_af_fir_gain0_l_write      (ViPipe, i, pstAfRegBeCfg->u16FirGain0Low     );
        isp_af_fir_gain0_h_write      (ViPipe, i, pstAfRegBeCfg->u16FirGain0High    );

        isp_af_fir1_ldg_en_write(ViPipe, i, pstAfRegBeCfg->bFir1LdgEnable);
        isp_af_fir_thre1_l_write        (ViPipe, i, pstAfRegBeCfg->u16FirThre1Low    );
        isp_af_fir_thre1_h_write        (ViPipe, i, pstAfRegBeCfg->u16FirThre1High   );
        isp_af_fir_slope1_l_write     (ViPipe, i, pstAfRegBeCfg->u16FirSlope1Low   );
        isp_af_fir_slope1_h_write     (ViPipe, i, pstAfRegBeCfg->u16FirSlope1High  );
        isp_af_fir_gain1_l_write      (ViPipe, i, pstAfRegBeCfg->u16FirGain1Low  );
        isp_af_fir_gain1_h_write      (ViPipe, i, pstAfRegBeCfg->u16FirGain1High );

        /* AF BE coring */
        isp_af_iir_thre0_c_write     (ViPipe, i, pstAfRegBeCfg->u16IirThre0Coring   );
        isp_af_iir_slope0_c_write    (ViPipe, i, pstAfRegBeCfg->u16IirSlope0Coring  );
        isp_af_iir_peak0_c_write     (ViPipe, i, pstAfRegBeCfg->u16IirPeak0Coring  );

        isp_af_iir_thre1_c_write     (ViPipe, i, pstAfRegBeCfg->u16IirThre1Coring   );
        isp_af_iir_slope1_c_write    (ViPipe, i, pstAfRegBeCfg->u16IirSlope1Coring  );
        isp_af_iir_peak1_c_write     (ViPipe, i, pstAfRegBeCfg->u16IirPeak1Coring  );

        isp_af_fir_thre0_c_write     (ViPipe, i, pstAfRegBeCfg->u16FirThre0Coring   );
        isp_af_fir_slope0_c_write    (ViPipe, i, pstAfRegBeCfg->u16FirSlope0Coring  );
        isp_af_fir_peak0_c_write     (ViPipe, i, pstAfRegBeCfg->u16FirPeak0Coring  );

        isp_af_fir_thre1_c_write     (ViPipe, i, pstAfRegBeCfg->u16FirThre1Coring   );
        isp_af_fir_slope1_c_write    (ViPipe, i, pstAfRegBeCfg->u16IirSlope1Coring  );
        isp_af_fir_peak1_c_write     (ViPipe, i, pstAfRegBeCfg->u16FirPeak1Coring  );

        /* high luma counter */
        isp_af_hilight_write(ViPipe, i, pstAfRegBeCfg->u8HilighThre );

        /* AF output shift */
        isp_af_acc_shift0_h_write(ViPipe, i, pstAfRegBeCfg->u16AccShift0H );
        isp_af_acc_shift1_h_write(ViPipe, i, pstAfRegBeCfg->u16AccShift1H );
        isp_af_acc_shift0_v_write(ViPipe, i, pstAfRegBeCfg->u16AccShift0V );
        isp_af_acc_shift1_v_write(ViPipe, i, pstAfRegBeCfg->u16AccShift1V );
        isp_af_acc_shift_y_write(ViPipe, i, pstAfRegBeCfg->u16AccShiftY );
        isp_af_cnt_shift_y_write(ViPipe, i, pstAfRegBeCfg->u16ShiftCountY);
        isp_af_cnt_shift0_v_write(ViPipe, i, ISP_AF_CNT_SHIFT0_V_DEFAULT);
        isp_af_cnt_shift0_h_write(ViPipe, i, 0x0);
        isp_af_cnt_shift1_h_write(ViPipe, i, 0x0);
        isp_af_cnt_shift1_v_write(ViPipe, i, 0x0);

        pstRegCfgInfo->unKey.bit1AfBeCfg = bIsOfflineMode;
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_FeAwbRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo)
{
    HI_U32 i;
    VI_PIPE ViPipeBind;
    ISP_AWB_REG_DYN_CFG_S  *pstAwbRegDynCfg = HI_NULL;
    ISP_AWB_REG_STA_CFG_S  *pstAwbRegStaCfg = HI_NULL;

    ISP_CTX_S *pstIspCtx   = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (pstIspCtx->stWdrAttr.bMastPipe)
    {
        for (i = 0; i < pstIspCtx->stWdrAttr.stDevBindPipe.u32Num; i++)
        {
            ViPipeBind = pstIspCtx->stWdrAttr.stDevBindPipe.PipeId[i];
            ISP_CHECK_PIPE(ViPipeBind);

            if (HI_TRUE == pstIspCtx->stLinkage.bSnapState)
            {
                isp_wb1_rgain_write(ViPipeBind, pstIspCtx->stSnapIspInfo.stIspCfgInfo.au32WhiteBalanceGain[0]);
                isp_wb1_grgain_write(ViPipeBind, pstIspCtx->stSnapIspInfo.stIspCfgInfo.au32WhiteBalanceGain[1]);
                isp_wb1_gbgain_write(ViPipeBind, pstIspCtx->stSnapIspInfo.stIspCfgInfo.au32WhiteBalanceGain[2]);
                isp_wb1_bgain_write(ViPipeBind, pstIspCtx->stSnapIspInfo.stIspCfgInfo.au32WhiteBalanceGain[3]);
            }

            if (pstRegCfgInfo->unKey.bit1AwbDynCfg)
            {
                pstAwbRegDynCfg = &pstRegCfgInfo->stAlgRegCfg[0].stAwbRegCfg.stAwbRegDynCfg;
                isp_wb1_rgain_write(ViPipeBind, pstAwbRegDynCfg->au32FEWhiteBalanceGain[0]);
                isp_wb1_grgain_write(ViPipeBind, pstAwbRegDynCfg->au32FEWhiteBalanceGain[1]);
                isp_wb1_gbgain_write(ViPipeBind, pstAwbRegDynCfg->au32FEWhiteBalanceGain[2]);
                isp_wb1_bgain_write(ViPipeBind, pstAwbRegDynCfg->au32FEWhiteBalanceGain[3]);
                isp_fe_wb1_en_write(ViPipeBind, pstAwbRegDynCfg->u8FEWbWorkEn);
            }

            pstAwbRegStaCfg = &pstRegCfgInfo->stAlgRegCfg[0].stAwbRegCfg.stAwbRegStaCfg;

            if (pstAwbRegStaCfg->bFEAwbStaCfg)
            {
                pstAwbRegStaCfg = &pstRegCfgInfo->stAlgRegCfg[0].stAwbRegCfg.stAwbRegStaCfg;
                isp_wb1_clip_value_write(ViPipeBind, pstAwbRegStaCfg->u32FEClipValue);
            }
        }
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_AwbCCSet(VI_PIPE ViPipe, HI_U16 *pu16BeCC, HI_U8 i)
{
    isp_cc_coef00_write(ViPipe, i, CCM_CONVERT(CCM_CONVERT_PRE(pu16BeCC[0])));
    isp_cc_coef01_write(ViPipe, i, CCM_CONVERT(CCM_CONVERT_PRE(pu16BeCC[1])));
    isp_cc_coef02_write(ViPipe, i, CCM_CONVERT(CCM_CONVERT_PRE(pu16BeCC[2])));
    isp_cc_coef10_write(ViPipe, i, CCM_CONVERT(CCM_CONVERT_PRE(pu16BeCC[3])));
    isp_cc_coef11_write(ViPipe, i, CCM_CONVERT(CCM_CONVERT_PRE(pu16BeCC[4])));
    isp_cc_coef12_write(ViPipe, i, CCM_CONVERT(CCM_CONVERT_PRE(pu16BeCC[5])));
    isp_cc_coef20_write(ViPipe, i, CCM_CONVERT(CCM_CONVERT_PRE(pu16BeCC[6])));
    isp_cc_coef21_write(ViPipe, i, CCM_CONVERT(CCM_CONVERT_PRE(pu16BeCC[7])));
    isp_cc_coef22_write(ViPipe, i, CCM_CONVERT(CCM_CONVERT_PRE(pu16BeCC[8])));

    return HI_SUCCESS;
}

static HI_S32 ISP_AwbGainSet(VI_PIPE ViPipe, HI_U32 *pu32BeWBGain, HI_U8 i)
{
    isp_wb_rgain_write(ViPipe, i, pu32BeWBGain[0]);
    isp_wb_grgain_write(ViPipe, i, pu32BeWBGain[1]);
    isp_wb_gbgain_write(ViPipe, i, pu32BeWBGain[2]);
    isp_wb_bgain_write(ViPipe, i, pu32BeWBGain[3]);

    return HI_SUCCESS;
}

static HI_S32 ISP_StitchAwbSyncCfg(VI_PIPE ViPipe, ISP_AWB_REG_DYN_CFG_S *pstAwbRegDynCfg, HI_U8 i)
{
    HI_S32 k;
    VI_PIPE ViPipeS;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (HI_TRUE == pstIspCtx->stIspParaRec.bInit)
    {
        if (HI_TRUE == pstIspCtx->stStitchAttr.bMainPipe)
        {
            for (k = 0; k < pstIspCtx->stStitchAttr.u8StitchPipeNum; k++)
            {
                ViPipeS = pstIspCtx->stStitchAttr.as8StitchBindId[k];
                ISP_CHECK_PIPE(ViPipeS);

                ISP_AwbCCSet(ViPipeS, pstAwbRegDynCfg->au16BEColorMatrix, i);
                ISP_AwbGainSet(ViPipeS, pstAwbRegDynCfg->au32BEWhiteBalanceGain, i);
            }
        }
    }
    else
    {
        ISP_AwbCCSet(ViPipe, pstAwbRegDynCfg->au16BEColorMatrix, i);
        ISP_AwbGainSet(ViPipe, pstAwbRegDynCfg->au32BEWhiteBalanceGain, i);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_AwbRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    ISP_AWB_REG_DYN_CFG_S  *pstAwbRegDynCfg = HI_NULL;
    ISP_AWB_REG_STA_CFG_S  *pstAwbRegStaCfg = HI_NULL;
    ISP_AWB_REG_USR_CFG_S  *pstAwbRegUsrCfg = HI_NULL;
    HI_BOOL  bIsOfflineMode;
    ISP_CTX_S *pstIspCtx   = HI_NULL;
    HI_BOOL bIdxResh, bUsrResh;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAwbRegDynCfg = &pstRegCfgInfo->stAlgRegCfg[i].stAwbRegCfg.stAwbRegDynCfg;
    pstAwbRegStaCfg = &pstRegCfgInfo->stAlgRegCfg[i].stAwbRegCfg.stAwbRegStaCfg;
    pstAwbRegUsrCfg = &pstRegCfgInfo->stAlgRegCfg[i].stAwbRegCfg.stAwbRegUsrCfg;

    if (HI_TRUE == pstIspCtx->stLinkage.bSnapState)
    {
        if (HI_TRUE == pstIspCtx->stLinkage.bLoadCCM)
        {
            ISP_AwbCCSet(ViPipe, pstIspCtx->stSnapIspInfo.stIspCfgInfo.au16CapCCM, i);
        }
        else
        {
            ISP_AwbCCSet(ViPipe, pstAwbRegDynCfg->au16BEColorMatrix, i);
        }

        ISP_AwbGainSet(ViPipe, pstIspCtx->stSnapIspInfo.stIspCfgInfo.au32WhiteBalanceGain, i);
    }

    if (pstRegCfgInfo->unKey.bit1AwbDynCfg)
    {
        if (HI_TRUE != pstIspCtx->stLinkage.bSnapState)
        {
            if (ISP_SNAP_PICTURE != pstIspCtx->stLinkage.enSnapPipeMode)
            {
                if (HI_TRUE == pstIspCtx->stStitchAttr.bStitchEnable)
                {
                    ISP_StitchAwbSyncCfg(ViPipe, pstAwbRegDynCfg, i);
                }
                else
                {
                    ISP_AwbCCSet(ViPipe, pstAwbRegDynCfg->au16BEColorMatrix, i);
                    ISP_AwbGainSet(ViPipe, pstAwbRegDynCfg->au32BEWhiteBalanceGain, i);
                }
            }
        }

        if ((IS_ONLINE_MODE(pstIspCtx->stLinkage.enPictureRunningMode)\
             || IS_SIDEBYSIDE_MODE(pstIspCtx->stLinkage.enPictureRunningMode))\
            && (ISP_SNAP_PREVIEW == pstIspCtx->stLinkage.enSnapPipeMode))
        {
            ISP_CHECK_PIPE(pstIspCtx->stLinkage.s32PicturePipeId);
            ISP_AwbCCSet(pstIspCtx->stLinkage.s32PicturePipeId, pstAwbRegDynCfg->au16BEColorMatrix, i);
            ISP_AwbGainSet(pstIspCtx->stLinkage.s32PicturePipeId, pstAwbRegDynCfg->au32BEWhiteBalanceGain, i);
        }

        isp_awb_threshold_max_write(ViPipe, i, pstAwbRegDynCfg->u16BEMeteringWhiteLevelAwb);
        isp_awb_threshold_min_write(ViPipe, i, pstAwbRegDynCfg->u16BEMeteringBlackLevelAwb);
        isp_awb_cr_ref_max_write(ViPipe, i, pstAwbRegDynCfg->u16BEMeteringCrRefMaxAwb);
        isp_awb_cr_ref_min_write(ViPipe, i, pstAwbRegDynCfg->u16BEMeteringCrRefMinAwb);
        isp_awb_cb_ref_max_write(ViPipe, i, pstAwbRegDynCfg->u16BEMeteringCbRefMaxAwb);
        isp_awb_cb_ref_min_write(ViPipe, i, pstAwbRegDynCfg->u16BEMeteringCbRefMinAwb);

        isp_wb_en_write(ViPipe, i, pstAwbRegDynCfg->u8BEWbWorkEn);
        isp_cc_en_write(ViPipe, i, pstAwbRegDynCfg->u8BECcEn);

        isp_cc_colortone_en_write(ViPipe, i, pstAwbRegDynCfg->u16BECcColortoneEn);
        isp_cc_r_gain_write(ViPipe, i, pstAwbRegDynCfg->u16BECcRGain);
        isp_cc_g_gain_write(ViPipe, i, pstAwbRegDynCfg->u16BECcGGain);
        isp_cc_b_gain_write(ViPipe, i, pstAwbRegDynCfg->u16BECcBGain);

        isp_cc_prot_en_write(ViPipe, i, HI_ISP_CCM_PROT_EN_DEFAULT);

        isp_awb_crop_pos_x_write(ViPipe, i, pstAwbRegDynCfg->u32BECropPosX);
        isp_awb_crop_pos_y_write(ViPipe, i, pstAwbRegDynCfg->u32BECropPosY);
        isp_awb_crop_out_width_write(ViPipe, i, pstAwbRegDynCfg->u32BECropOutWidth - 1);
        isp_awb_crop_out_height_write(ViPipe, i, pstAwbRegDynCfg->u32BECropOutHeight - 1);
    }

    if (pstAwbRegStaCfg->bBEAwbStaCfg)
    {
        isp_awb_bitmove_write(ViPipe, i, pstAwbRegStaCfg->u8BEAwbBitmove);
        isp_awb_en_write(ViPipe, i, pstAwbRegStaCfg->u8BEAwbWorkEn);

        isp_awb_stat_raddr_write(ViPipe, i, pstAwbRegStaCfg->u32BEAwbStatRaddr);

        isp_cc_prot_en_write(ViPipe, i, HI_ISP_CCM_PROT_EN_DEFAULT);
        isp_cc_data_cc_thd0_write(ViPipe, i, HI_ISP_CCM_CC_THD0_DEFAULT);
        isp_cc_data_cc_thd1_write(ViPipe, i, HI_ISP_CCM_CC_THD1_DEFAULT);
        isp_cc_cc_prot_ratio_write(ViPipe, i, HI_ISP_CCM_CC_PROT_RATIO_DEFAULT);
        isp_cc_data_rr_thd0_write(ViPipe, i, HI_ISP_CCM_RR_THD0_DEFAULT);
        isp_cc_data_rr_thd1_write(ViPipe, i, HI_ISP_CCM_RR_THD1_DEFAULT);
        isp_cc_data_gg_thd0_write(ViPipe, i, HI_ISP_CCM_GG_THD0_DEFAULT);
        isp_cc_data_gg_thd1_write(ViPipe, i, HI_ISP_CCM_GG_THD1_DEFAULT);
        isp_cc_data_bb_thd0_write(ViPipe, i, HI_ISP_CCM_BB_THD0_DEFAULT);
        isp_cc_data_bb_thd1_write(ViPipe, i, HI_ISP_CCM_BB_THD1_DEFAULT);
        isp_cc_max_rgb_thd_write(ViPipe, i, HI_ISP_CCM_MAX_RGB_DEFAULT);
        isp_cc_rgb_prot_ratio_write(ViPipe, i, HI_ISP_CCM_RGB_PROT_RATIO_DEFAULT);

        isp_cc_in_dc0_write(ViPipe, i, pstAwbRegStaCfg->u32BECcInDc0);
        isp_cc_in_dc1_write(ViPipe, i, pstAwbRegStaCfg->u32BECcInDc1);
        isp_cc_in_dc2_write(ViPipe, i, pstAwbRegStaCfg->u32BECcInDc2);
        isp_cc_out_dc0_write(ViPipe, i, pstAwbRegStaCfg->u32BECcOutDc0);
        isp_cc_out_dc1_write(ViPipe, i, pstAwbRegStaCfg->u32BECcOutDc1);
        isp_cc_out_dc2_write(ViPipe, i, pstAwbRegStaCfg->u32BECcOutDc2);
        isp_wb_clip_value_write(ViPipe, i, pstAwbRegStaCfg->u32BEWbClipValue);
        isp_awb_offset_comp_write(ViPipe, i, pstAwbRegStaCfg->u16BEAwbOffsetComp);
        isp_awb_hist_weight_b_write(ViPipe, i, HI_ISP_AWB_HIST_WEIGHT_B_DEFAULT);
        isp_awb_hist_weight_g_write(ViPipe, i, HI_ISP_AWB_HIST_WEIGHT_G_DEFAULT);
        isp_awb_hist_weight_r_write(ViPipe, i, HI_ISP_AWB_HIST_WEIGHT_R_DEFAULT);

        pstAwbRegStaCfg->bBEAwbStaCfg = 0;
    }

    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode) || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    bIdxResh = (isp_awb_update_index_read(ViPipe, i) != pstAwbRegUsrCfg->u32UpdateIndex);
    bUsrResh = (bIsOfflineMode) ? (pstAwbRegUsrCfg->bResh & bIdxResh) : (pstAwbRegUsrCfg->bResh);

    if (bUsrResh)
    {
        isp_awb_update_index_write(ViPipe, i, pstAwbRegUsrCfg->u32UpdateIndex);

        isp_awb_sel_write(ViPipe, i, pstAwbRegUsrCfg->enBEAWBSwitch);
        //isp_awb_hnum_write(ViPipe, i, pstAwbRegUsrCfg->u16BEZoneCol);
        //isp_awb_vnum_write(ViPipe, i, pstAwbRegUsrCfg->u16BEZoneRow);
        //isp_awb_bin_num_write(ViPipe, i, pstAwbRegUsrCfg->u16BEZoneBin);
        isp_awb_hist_bin0_write(ViPipe, i, pstAwbRegUsrCfg->u16BEMeteringBinHist0);
        isp_awb_hist_bin1_write(ViPipe, i, pstAwbRegUsrCfg->u16BEMeteringBinHist1);
        isp_awb_hist_bin2_write(ViPipe, i, pstAwbRegUsrCfg->u16BEMeteringBinHist2);
        isp_awb_hist_bin3_write(ViPipe, i, pstAwbRegUsrCfg->u16BEMeteringBinHist3);
        pstAwbRegUsrCfg->bResh = bIsOfflineMode;  //if online mode, bResh=0; if offline mode, bResh=1; but only index != will resh
    }
    isp_awb_hnum_write(ViPipe, i, pstAwbRegUsrCfg->u16BEZoneCol);
    isp_awb_vnum_write(ViPipe, i, pstAwbRegUsrCfg->u16BEZoneRow);
    isp_awb_bin_num_write(ViPipe, i, pstAwbRegUsrCfg->u16BEZoneBin);

    return HI_SUCCESS;
}

static HI_S32 ISP_SharpenRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL  bIsOfflineMode;
    HI_BOOL  bUsrResh   = HI_FALSE;
    HI_BOOL  bIdxResh   = HI_FALSE;
    HI_BOOL  bLutUpdate = HI_FALSE;
    HI_U8    u8BlkNum   = pstRegCfgInfo->u8CfgNum;
    HI_U16   j;

    ISP_SHARPEN_DEFAULT_DYNA_REG_CFG_S *pstSharpenDefaultDynaRegCfg = HI_NULL;
    ISP_SHARPEN_MPI_DYNA_REG_CFG_S     *pstSharpenMpiDynaRegCfg     = HI_NULL;
    ISP_SHARPEN_STATIC_REG_CFG_S       *pstSharpenStaticRegCfg      = HI_NULL;
    ISP_CTX_S                          *pstIspCtx                   = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);


    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1SharpenCfg)
    {
        isp_sharpen_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stSharpenRegCfg.bEnable);

        pstSharpenStaticRegCfg      = &pstRegCfgInfo->stAlgRegCfg[i].stSharpenRegCfg.stStaticRegCfg;
        pstSharpenDefaultDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stSharpenRegCfg.stDynaRegCfg.stDefaultDynaRegCfg;
        pstSharpenMpiDynaRegCfg     = &pstRegCfgInfo->stAlgRegCfg[i].stSharpenRegCfg.stDynaRegCfg.stMpiDynaRegCfg;

        if (pstSharpenStaticRegCfg->bStaticResh)
        {
            isp_sharpen_mfthdseld_write (ViPipe, i, pstSharpenStaticRegCfg->u8mfThdSelD);
            isp_sharpen_hfthdseld_write (ViPipe, i, pstSharpenStaticRegCfg->u8hfThdSelD);
            isp_sharpen_dirvarscale_write(ViPipe, i, pstSharpenStaticRegCfg->u8dirVarScale);
            isp_sharpen_benchrsad_write (ViPipe, i, pstSharpenStaticRegCfg->u8bEnChrSad);
            isp_sharpen_dirrly0_write(ViPipe, i, pstSharpenStaticRegCfg->u8dirRly[0]);
            isp_sharpen_dirrly1_write(ViPipe, i, pstSharpenStaticRegCfg->u8dirRly[1]);
            isp_sharpen_omaxchg_write  (ViPipe, i, pstSharpenStaticRegCfg->u16oMaxChg);
            isp_sharpen_umaxchg_write  (ViPipe, i, pstSharpenStaticRegCfg->u16uMaxChg);
            isp_sharpen_shtvarsft_write      (ViPipe, i, pstSharpenStaticRegCfg->u8shtVarSft);
            isp_sharpen_oshtvarthd0_write(ViPipe, i, pstSharpenStaticRegCfg->u8oshtVarThd0);
            isp_sharpen_ushtvarthd0_write(ViPipe, i, pstSharpenStaticRegCfg->u8ushtVarThd0);
            isp_sharpen_oshtvardiffwgt0_write(ViPipe, i, pstSharpenStaticRegCfg->u8oshtVarDiffWgt0);
            isp_sharpen_ushtvardiffwgt0_write(ViPipe, i, pstSharpenStaticRegCfg->u8ushtVarDiffWgt0);
            isp_sharpen_oshtvarwgt1_write(ViPipe, i, pstSharpenStaticRegCfg->u8oshtVarWgt1);
            isp_sharpen_ushtvarwgt1_write(ViPipe, i, pstSharpenStaticRegCfg->u8ushtVarWgt1);
            isp_sharpen_lmtmf0_write(ViPipe, i, pstSharpenStaticRegCfg->u8lmtMF[0]);
            isp_sharpen_lmthf0_write(ViPipe, i, pstSharpenStaticRegCfg->u8lmtHF[0]);
            isp_sharpen_lmtmf1_write(ViPipe, i, pstSharpenStaticRegCfg->u8lmtMF[1]);
            isp_sharpen_lmthf1_write(ViPipe, i, pstSharpenStaticRegCfg->u8lmtHF[1]);
            isp_sharpen_lmtmf2_write(ViPipe, i, pstSharpenStaticRegCfg->u8lmtMF[2]);
            isp_sharpen_lmthf2_write(ViPipe, i, pstSharpenStaticRegCfg->u8lmtHF[2]);
            isp_sharpen_lmtmf3_write(ViPipe, i, pstSharpenStaticRegCfg->u8lmtMF[3]);
            isp_sharpen_lmthf3_write(ViPipe, i, pstSharpenStaticRegCfg->u8lmtHF[3]);
            isp_sharpen_lmtmf4_write(ViPipe, i, pstSharpenStaticRegCfg->u8lmtMF[4]);
            isp_sharpen_lmthf4_write(ViPipe, i, pstSharpenStaticRegCfg->u8lmtHF[4]);
            isp_sharpen_lmtmf5_write(ViPipe, i, pstSharpenStaticRegCfg->u8lmtMF[5]);
            isp_sharpen_lmthf5_write(ViPipe, i, pstSharpenStaticRegCfg->u8lmtHF[5]);
            isp_sharpen_lmtmf6_write(ViPipe, i, pstSharpenStaticRegCfg->u8lmtMF[6]);
            isp_sharpen_lmthf6_write(ViPipe, i, pstSharpenStaticRegCfg->u8lmtHF[6]);
            isp_sharpen_lmtmf7_write(ViPipe, i, pstSharpenStaticRegCfg->u8lmtMF[7]);
            isp_sharpen_lmthf7_write(ViPipe, i, pstSharpenStaticRegCfg->u8lmtHF[7]);

            isp_sharpen_skinsrcsel_write(ViPipe, i, pstSharpenStaticRegCfg->u8skinSrcSel);
            isp_sharpen_skinmaxu_write(ViPipe, i, pstSharpenStaticRegCfg->u16skinMaxU);
            isp_sharpen_skinminu_write(ViPipe, i, pstSharpenStaticRegCfg->u16skinMinU);
            isp_sharpen_skinmaxv_write(ViPipe, i, pstSharpenStaticRegCfg->u16skinMaxV);
            isp_sharpen_skinminv_write(ViPipe, i, pstSharpenStaticRegCfg->u16skinMinV);
            isp_sharpen_skinedgesft_write(ViPipe, i, pstSharpenStaticRegCfg->u8skinEdgeSft);

            isp_sharpen_skincntthd0_write  (ViPipe, i, pstSharpenStaticRegCfg->u8skinCntThd[0]);
            isp_sharpen_skinedgethd0_write (ViPipe, i, pstSharpenStaticRegCfg->u8skinEdgeThd[0]);
            isp_sharpen_skinaccumthd0_write(ViPipe, i, pstSharpenStaticRegCfg->u16skinAccumThd[0]);
            isp_sharpen_skinaccumwgt0_write(ViPipe, i, pstSharpenStaticRegCfg->u8skinAccumWgt[0]);

            isp_sharpen_skincntthd1_write  (ViPipe, i, pstSharpenStaticRegCfg->u8skinCntThd[1]);
            isp_sharpen_skinedgethd1_write (ViPipe, i, pstSharpenStaticRegCfg->u8skinEdgeThd[1]);
            isp_sharpen_skinaccumthd1_write(ViPipe, i, pstSharpenStaticRegCfg->u16skinAccumThd[1]);
            isp_sharpen_skinaccumwgt1_write(ViPipe, i, pstSharpenStaticRegCfg->u8skinAccumWgt[1]);

            isp_sharpen_chrrvarsft_write  (ViPipe, i, pstSharpenStaticRegCfg->u8chrRVarSft);
            isp_sharpen_chrrvarscale_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrRVarScale);
            isp_sharpen_chrrscale_write   (ViPipe, i, pstSharpenStaticRegCfg->u16chrRScale);
            isp_sharpen_chrgvarsft_write  (ViPipe, i, pstSharpenStaticRegCfg->u8chrGVarSft);
            isp_sharpen_chrgvarscale_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrGVarScale);
            isp_sharpen_chrgscale_write   (ViPipe, i, pstSharpenStaticRegCfg->u16chrGScale);
            isp_sharpen_chrbvarsft_write  (ViPipe, i, pstSharpenStaticRegCfg->u8chrBVarSft);
            isp_sharpen_chrbvarscale_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrBVarScale);
            isp_sharpen_chrbscale_write   (ViPipe, i, pstSharpenStaticRegCfg->u16chrBScale);

            isp_sharpen_chrrori0_write (ViPipe, i, pstSharpenStaticRegCfg->u8chrROri[0]);
            isp_sharpen_chrrthd0_write (ViPipe, i, pstSharpenStaticRegCfg->u8chrRThd[0]);
            isp_sharpen_chrgori0_write (ViPipe, i, pstSharpenStaticRegCfg->u8chrGOri[0]);
            isp_sharpen_chrgthd0_write (ViPipe, i, pstSharpenStaticRegCfg->u8chrGThd[0]);
            isp_sharpen_chrbori0_write (ViPipe, i, pstSharpenStaticRegCfg->u8chrBOri[0]);
            isp_sharpen_chrbthd0_write (ViPipe, i, pstSharpenStaticRegCfg->u8chrBThd[0]);

            isp_sharpen_chrrori1_write (ViPipe, i, pstSharpenStaticRegCfg->u8chrROri[1]);
            isp_sharpen_chrrthd1_write (ViPipe, i, pstSharpenStaticRegCfg->u8chrRThd[1]);
            isp_sharpen_chrrgain1_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrRGain1);
            isp_sharpen_chrgori1_write (ViPipe, i, pstSharpenStaticRegCfg->u8chrGOri[1]);
            isp_sharpen_chrgthd1_write (ViPipe, i, pstSharpenStaticRegCfg->u8chrGThd[1]);
            isp_sharpen_chrggain1_write(ViPipe, i, pstSharpenStaticRegCfg->u16chrGGain1);
            isp_sharpen_chrbori1_write (ViPipe, i, pstSharpenStaticRegCfg->u8chrBOri[1]);
            isp_sharpen_chrbthd1_write (ViPipe, i, pstSharpenStaticRegCfg->u8chrBThd[1]);
            isp_sharpen_chrbgain1_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrBGain1);

            isp_sharpen_dirrt0_write (ViPipe, i, pstSharpenStaticRegCfg->u8dirRt[0]);
            isp_sharpen_dirrt1_write (ViPipe, i, pstSharpenStaticRegCfg->u8dirRt[1]);
            isp_sharpen_shtnoisemax_write(ViPipe, i, pstSharpenStaticRegCfg->u8shtNoiseMax);
            isp_sharpen_shtnoisemin_write(ViPipe, i, pstSharpenStaticRegCfg->u8shtNoiseMin);
            isp_sharpen_omaxgain_write  (ViPipe, i, pstSharpenStaticRegCfg->u16oMaxGain);
            isp_sharpen_umaxgain_write  (ViPipe, i, pstSharpenStaticRegCfg->u16uMaxGain);

            isp_sharpen_chrrsft0_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrRSft[0]);
            isp_sharpen_chrgsft0_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrGSft[0]);
            isp_sharpen_chrbsft0_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrBSft[0]);
            isp_sharpen_chrrsft1_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrRSft[1]);
            isp_sharpen_chrgsft1_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrGSft[1]);
            isp_sharpen_chrbsft1_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrBSft[1]);
            isp_sharpen_chrrsft2_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrRSft[2]);
            isp_sharpen_chrgsft2_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrGSft[2]);
            isp_sharpen_chrbsft2_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrBSft[2]);
            isp_sharpen_chrrsft3_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrRSft[3]);
            isp_sharpen_chrgsft3_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrGSft[3]);
            isp_sharpen_chrbsft3_write(ViPipe, i, pstSharpenStaticRegCfg->u8chrBSft[3]);
            isp_sharpen_lumasrcsel_write (ViPipe, i, pstSharpenStaticRegCfg->u8lumaSrcSel);
            isp_sharpen_skincntmul_write(ViPipe, i, pstSharpenStaticRegCfg->u8skinCntMul);
            isp_sharpen_skinaccummul_write(ViPipe, i, pstSharpenStaticRegCfg->s16skinAccumMul);

            isp_sharpen_ben8dir_sel_write(ViPipe, i, pstSharpenStaticRegCfg->bEnShp8Dir);
            isp_sharpen_benshplowpow_write(ViPipe, i, pstSharpenStaticRegCfg->bEnShpLowPow);
            isp_sharpen_hfgain_sft_write(ViPipe, i, pstSharpenStaticRegCfg->u8hfGainSft);
            isp_sharpen_mfgain_sft_write(ViPipe, i, pstSharpenStaticRegCfg->u8mfGainSft);
            isp_sharpen_lpf_sel_write(ViPipe, i, pstSharpenStaticRegCfg->u8lpfSel);
            isp_sharpen_hsf_sel_write(ViPipe, i, pstSharpenStaticRegCfg->u8hsfSel);
            isp_sharpen_benshtvar_sel_write(ViPipe, i, pstSharpenStaticRegCfg->u8shtVarSel);
            isp_sharpen_shtvar5x5_sft_write(ViPipe, i, pstSharpenStaticRegCfg->u8shtVar5x5Sft);
            isp_sharpen_detailthd_sel_write(ViPipe, i, pstSharpenStaticRegCfg->u8detailThdSel);
            isp_sharpen_dtl_thdsft_write(ViPipe, i, pstSharpenStaticRegCfg->u8detailThdSft);
            isp_sharpen_osht_dtl_thd0_write(ViPipe, i, pstSharpenStaticRegCfg->u8detailOshtThr[0]);
            isp_sharpen_osht_dtl_thd1_write(ViPipe, i, pstSharpenStaticRegCfg->u8detailOshtThr[1]);
            isp_sharpen_usht_dtl_thd0_write(ViPipe, i, pstSharpenStaticRegCfg->u8detailUshtThr[0]);
            isp_sharpen_usht_dtl_thd1_write(ViPipe, i, pstSharpenStaticRegCfg->u8detailUshtThr[1]);

            pstSharpenStaticRegCfg->bStaticResh = HI_FALSE;
        }

        if (pstSharpenDefaultDynaRegCfg->bResh)
        {
            isp_sharpen_mfthdsftd_write (ViPipe, i, pstSharpenDefaultDynaRegCfg->u8mfThdSftD);
            isp_sharpen_mfthdselud_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->u8mfThdSelUD);
            isp_sharpen_mfthdsftud_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->u8mfThdSftUD);
            isp_sharpen_hfthdsftd_write (ViPipe, i, pstSharpenDefaultDynaRegCfg->u8hfThdSftD);
            isp_sharpen_hfthdselud_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->u8hfThdSelUD);
            isp_sharpen_hfthdsftud_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->u8hfThdSftUD);
            isp_sharpen_dirvarsft_write  (ViPipe, i, pstSharpenDefaultDynaRegCfg->u8dirVarSft);
            isp_sharpen_oshtvarwgt0_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->u8oshtVarWgt0);
            isp_sharpen_ushtvarwgt0_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->u8ushtVarWgt0);
            isp_sharpen_oshtvardiffthd0_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->u8oshtVarDiffThd[0]);
            isp_sharpen_ushtvardiffthd0_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->u8ushtVarDiffThd[0]);
            isp_sharpen_selpixwgt_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->u8selPixWgt);
            isp_sharpen_oshtvardiffthd1_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->u8oshtVarDiffThd[1]);
            isp_sharpen_oshtvardiffwgt1_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->u8oshtVarDiffWgt1);
            isp_sharpen_ushtvardiffthd1_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->u8ushtVarDiffThd[1]);
            isp_sharpen_ushtvardiffwgt1_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->u8ushtVarDiffWgt1);
            isp_sharpen_oshtvardiffmul_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->s16oshtVarDiffMul);
            isp_sharpen_ushtvardiffmul_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->s16ushtVarDiffMul);
            isp_sharpen_chrgmul_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->s16chrGMul);
            isp_sharpen_chrggain0_write(ViPipe, i, pstSharpenDefaultDynaRegCfg->u16chrGGain0);

            pstSharpenDefaultDynaRegCfg->bResh = bIsOfflineMode;
        }

        bIdxResh = (isp_sharpen_update_index_read(ViPipe, i) != pstSharpenMpiDynaRegCfg->u32UpdateIndex);
        bUsrResh = (bIsOfflineMode) ? (pstSharpenMpiDynaRegCfg->bResh & bIdxResh) : (pstSharpenMpiDynaRegCfg->bResh);

        if (bUsrResh)
        {
            isp_sharpen_update_index_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u32UpdateIndex);

            if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                isp_sharpen_mfgaind_waddr_write(ViPipe, i, 0);

                for (j = 0; j < SHRP_GAIN_LUT_SIZE ; j++)
                {
                    isp_sharpen_mfgaind_wdata_write (ViPipe, i, pstSharpenMpiDynaRegCfg->u16mfGainD[j]);
                }

                isp_sharpen_mfgainud_waddr_write(ViPipe, i, 0);

                for (j = 0; j < SHRP_GAIN_LUT_SIZE ; j++)
                {
                    isp_sharpen_mfgainud_wdata_write (ViPipe, i, pstSharpenMpiDynaRegCfg->u16mfGainUD[j]);
                }

                isp_sharpen_hfgaind_waddr_write(ViPipe, i, 0);

                for (j = 0; j < SHRP_GAIN_LUT_SIZE ; j++)
                {
                    isp_sharpen_hfgaind_wdata_write (ViPipe, i, pstSharpenMpiDynaRegCfg->u16hfGainD[j]);
                }

                isp_sharpen_hfgainud_waddr_write(ViPipe, i, 0);

                for (j = 0; j < SHRP_GAIN_LUT_SIZE ; j++)
                {
                    isp_sharpen_hfgainud_wdata_write (ViPipe, i, pstSharpenMpiDynaRegCfg->u16hfGainUD[j]);
                }
            }
            else
            {
                isp_sharpen_mfgaind_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u16mfGainD);
                isp_sharpen_mfgainud_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u16mfGainUD);
                isp_sharpen_hfgaind_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u16hfGainD);
                isp_sharpen_hfgainud_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u16hfGainUD);
            }

            //isp_sharpen_lut_update_write(ViPipe, i, 1);
            bLutUpdate = HI_TRUE;

            isp_sharpen_bendetailctrl_write  (ViPipe, i, pstSharpenMpiDynaRegCfg->bEnDetailCtrl);
            isp_sharpen_osht_dtl_wgt_write  (ViPipe, i, pstSharpenMpiDynaRegCfg->u8detailOshtAmt);
            isp_sharpen_usht_dtl_wgt_write  (ViPipe, i, pstSharpenMpiDynaRegCfg->u8detailUshtAmt);
            isp_sharpen_detl_oshtmul_write(ViPipe, i, pstSharpenMpiDynaRegCfg->s16detailOshtMul);
            isp_sharpen_detl_ushtmul_write(ViPipe, i, pstSharpenMpiDynaRegCfg->s16detailUshtMul);
            isp_sharpen_oshtamt_write  (ViPipe, i, pstSharpenMpiDynaRegCfg->u8oshtAmt);
            isp_sharpen_ushtamt_write  (ViPipe, i, pstSharpenMpiDynaRegCfg->u8ushtAmt);
            isp_sharpen_benshtctrlbyvar_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u8bEnShtCtrlByVar);
            isp_sharpen_shtbldrt_write   (ViPipe, i, pstSharpenMpiDynaRegCfg->u8shtBldRt);
            isp_sharpen_oshtvarthd1_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u8oshtVarThd1);
            isp_sharpen_ushtvarthd1_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u8ushtVarThd1);
            isp_sharpen_benlumactrl_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u8bEnLumaCtrl);
            isp_sharpen_lumawgt0_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[0]);
            isp_sharpen_lumawgt1_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[1]);
            isp_sharpen_lumawgt2_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[2]);
            isp_sharpen_lumawgt3_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[3]);
            isp_sharpen_lumawgt4_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[4]);
            isp_sharpen_lumawgt5_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[5]);
            isp_sharpen_lumawgt6_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[6]);
            isp_sharpen_lumawgt7_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[7]);
            isp_sharpen_lumawgt8_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[8]);
            isp_sharpen_lumawgt9_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[9]);
            isp_sharpen_lumawgt10_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[10]);
            isp_sharpen_lumawgt11_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[11]);
            isp_sharpen_lumawgt12_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[12]);
            isp_sharpen_lumawgt13_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[13]);
            isp_sharpen_lumawgt14_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[14]);
            isp_sharpen_lumawgt15_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[15]);
            isp_sharpen_lumawgt16_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[16]);
            isp_sharpen_lumawgt17_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[17]);
            isp_sharpen_lumawgt18_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[18]);
            isp_sharpen_lumawgt19_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[19]);
            isp_sharpen_lumawgt20_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[20]);
            isp_sharpen_lumawgt21_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[21]);
            isp_sharpen_lumawgt22_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[22]);
            isp_sharpen_lumawgt23_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[23]);
            isp_sharpen_lumawgt24_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[24]);
            isp_sharpen_lumawgt25_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[25]);
            isp_sharpen_lumawgt26_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[26]);
            isp_sharpen_lumawgt27_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[27]);
            isp_sharpen_lumawgt28_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[28]);
            isp_sharpen_lumawgt29_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[29]);
            isp_sharpen_lumawgt30_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[30]);
            isp_sharpen_lumawgt31_write(ViPipe, i, pstSharpenMpiDynaRegCfg->au8LumaWgt[31]);
            isp_sharpen_oshtvarmul_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u16oshtVarMul);
            isp_sharpen_ushtvarmul_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u16ushtVarMul);
            isp_sharpen_benchrctrl_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u8bEnChrCtrl);
            isp_sharpen_chrrgain0_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u8chrRGain0);
            isp_sharpen_chrbgain0_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u8chrBGain0);
            isp_sharpen_chrrmul_write(ViPipe, i, pstSharpenMpiDynaRegCfg->s16chrRMul);
            isp_sharpen_chrbmul_write(ViPipe, i, pstSharpenMpiDynaRegCfg->s16chrBMul);
            isp_sharpen_benskinctrl_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u8bEnSkinCtrl);
            isp_sharpen_skinedgewgt0_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u8skinEdgeWgt[0]);
            isp_sharpen_skinedgewgt1_write(ViPipe, i, pstSharpenMpiDynaRegCfg->u8skinEdgeWgt[1]);
            isp_sharpen_skinedgemul_write(ViPipe, i, pstSharpenMpiDynaRegCfg->s16skinEdgeMul);
            isp_sharpen_dirdiffsft_write (ViPipe, i, pstSharpenMpiDynaRegCfg->u8dirDiffSft);

            pstSharpenMpiDynaRegCfg->bResh = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1SharpenCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bSharpenLutUpdate = bLutUpdate;

    return HI_SUCCESS;
}


static HI_S32 ISP_EdgeMarkRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL  bIsOfflineMode;
    HI_U8    u8BlkNum = pstRegCfgInfo->u8CfgNum;

    ISP_CTX_S *pstIspCtx  = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1EdgeMarkCfg)
    {
        isp_sharpen_benmarkedge_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stEdgeMarkRegCfg.bEnable);
        isp_sharpen_mark_thdsft_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stEdgeMarkRegCfg.u8markEdgeSft);
        isp_sharpen_mark_udata_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stEdgeMarkRegCfg.u16uMarkValue);
        isp_sharpen_mark_vdata_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stEdgeMarkRegCfg.u16vMarkValue);
        isp_sharpen_mark_thd_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stEdgeMarkRegCfg.u8MarkEdgeThd);
    }

    pstRegCfgInfo->unKey.bit1EdgeMarkCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    return HI_SUCCESS;
}

static HI_S32 ISP_DemRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL bIsOfflineMode;
    HI_BOOL bGfLutUpdate  = HI_FALSE;
    HI_BOOL bUsmLutUpdate = HI_FALSE;
    HI_U8   u8BlkNum      = pstRegCfgInfo->u8CfgNum;
    HI_U16  j;

    ISP_DEMOSAIC_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;
    ISP_DEMOSAIC_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_CTX_S                 *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1DemCfg)
    {
        isp_dmnr_vhdm_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stDemRegCfg.bVhdmEnable);
        isp_dmnr_nddm_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stDemRegCfg.bNddmEnable);

        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stDemRegCfg.stStaticRegCfg;
        pstDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stDemRegCfg.stDynaRegCfg;

        if (pstStaticRegCfg->bResh)  /*static*/
        {
            isp_demosaic_fcs_en_write(ViPipe, i, pstStaticRegCfg->bFcsEnable);
            isp_nddm_apt_interp_en_write(ViPipe, i, pstStaticRegCfg->bAptFltEn);
            isp_nddm_alpha_filter_en_write(ViPipe, i, pstStaticRegCfg->bAlphaFilter);
            isp_demosaic_ahd_en_write(ViPipe, i, pstStaticRegCfg->bAHDEnable);
            isp_demosaic_de_fake_en_write(ViPipe, i, pstStaticRegCfg->bDeFakeEnable);
            isp_demosaic_bld_limit1_write(ViPipe, i, pstStaticRegCfg->u8hvBlendLimit1);
            isp_demosaic_bld_limit2_write(ViPipe, i, pstStaticRegCfg->u8hvBlendLimit2);
            isp_demosaic_bld_ratio1_write(ViPipe, i, pstStaticRegCfg->u16hvBlendRatio1);
            isp_demosaic_bld_ratio2_write(ViPipe, i, pstStaticRegCfg->u16hvBlendRatio2);
            isp_demosaic_ahd_par1_write(ViPipe, i, pstStaticRegCfg->u16AhdPart1);
            isp_demosaic_ahd_par2_write(ViPipe, i, pstStaticRegCfg->u16AhdPart2);
            isp_demosaic_cx_var_max_rate_write(ViPipe, i, pstStaticRegCfg->u8CxVarMaxRate);
            isp_demosaic_cx_var_min_rate_write(ViPipe, i, pstStaticRegCfg->u8CxVarMinRate);
            isp_demosaic_g_clip_sft_bit_write(ViPipe, i, pstStaticRegCfg->u8GClipBitSft);
            isp_demosaic_hv_ratio_write(ViPipe, i, pstStaticRegCfg->u8hvColorRatio);
            isp_demosaic_hv_sel_write(ViPipe, i, pstStaticRegCfg->u8hvSelection);
            isp_demosaic_cbcr_avg_thld_write(ViPipe, i, pstStaticRegCfg->u16CbCrAvgThr);
            isp_nddm_bldr_gr_gb_write(ViPipe, i, pstStaticRegCfg->u8BldrGrGb);
            isp_nddm_multi_mf_write(ViPipe, i, pstStaticRegCfg->u8MultiMF);
            isp_nddm_multi_mf_r_write(ViPipe, i, pstStaticRegCfg->u8MultiMFRed);
            isp_nddm_multi_mf_b_write(ViPipe, i, pstStaticRegCfg->u8MultiMFBlue);
            isp_nddm_dith_mask_write(ViPipe, i, pstStaticRegCfg->u8DitherMask);
            isp_nddm_dith_ratio_write(ViPipe, i, pstStaticRegCfg->u8DitherRatio);
            isp_nddm_sht_ctrl_gain_write(ViPipe, i, pstStaticRegCfg->u8ShtCtrlGain);
            isp_nddm_clip_delta_gain_write(ViPipe, i, pstStaticRegCfg->u8ClipDeltaGain);
            isp_nddm_clip_adjust_max_write(ViPipe, i, pstStaticRegCfg->u8ClipAdjustMax);
            isp_nddm_scale_write(ViPipe, i, pstStaticRegCfg->u8FcrScale);
            isp_nddm_bldr_gf_str_write(ViPipe, i, pstStaticRegCfg->u8BldrGFStr);
            isp_nddm_gf_th_low_write(ViPipe, i, pstStaticRegCfg->u16GFThLow);
            isp_nddm_gf_th_high_write(ViPipe, i, pstStaticRegCfg->u16GFThHig);
            isp_nddm_clip_usm_write(ViPipe, i, pstStaticRegCfg->u16ClipUSM);
            isp_nddm_satu_th_fix_write(ViPipe, i, pstStaticRegCfg->u16SatuThFix);
            isp_nddm_satu_th_low_write(ViPipe, i, pstStaticRegCfg->u16SatuThLow);
            isp_nddm_satu_th_high_write(ViPipe, i, pstStaticRegCfg->u16SatuThHig);
            isp_nddm_gray_th_low_write(ViPipe, i, pstStaticRegCfg->u16GrayThLow);
            isp_nddm_gray_th_high_write(ViPipe, i, pstStaticRegCfg->u16GrayThHig);
            isp_nddm_gray_th_fix_write(ViPipe, i, pstStaticRegCfg->u16GrayThFixLow);
            isp_nddm_gray_th_fix2_write(ViPipe, i, pstStaticRegCfg->u16GrayThFixHig);
            isp_nddm_fcr_limit_low_write(ViPipe, i, pstStaticRegCfg->u16FcrLimitLow);
            isp_nddm_fcr_limit_high_write(ViPipe, i, pstStaticRegCfg->u16FcrLimitHigh);
            isp_nddm_sht_ctrl_th_write(ViPipe, i, pstStaticRegCfg->u16ShtCtrlTh);
            isp_nddm_clip_r_ud_sht_write(ViPipe, i, pstStaticRegCfg->u16ClipRUdSht);
            isp_nddm_clip_r_ov_sht_write(ViPipe, i, pstStaticRegCfg->u16ClipROvSht);
            isp_nddm_clip_b_ud_sht_write(ViPipe, i, pstStaticRegCfg->u16ClipBUdSht);
            isp_nddm_clip_b_ov_sht_write(ViPipe, i, pstStaticRegCfg->u16ClipBOvSht);

            pstRegCfgInfo->stAlgRegCfg[i].stDemRegCfg.stStaticRegCfg.bResh = HI_FALSE;
        }

        if (pstDynaRegCfg->bResh)   /*dynamic*/
        {
            isp_demosaic_lpf_f0_write(ViPipe, i, pstDynaRegCfg->u8Lpff0);
            isp_demosaic_lpf_f1_write(ViPipe, i, pstDynaRegCfg->u8Lpff1);
            isp_demosaic_lpf_f2_write(ViPipe, i, pstDynaRegCfg->u8Lpff2);
            isp_demosaic_lpf_f3_write(ViPipe, i, pstDynaRegCfg->u8Lpff3);
            isp_demosaic_cc_hf_max_ratio_write(ViPipe, i, pstDynaRegCfg->u8CcHFMaxRatio);
            isp_demosaic_cc_hf_min_ratio_write(ViPipe, i, pstDynaRegCfg->u8CcHFMinRatio);
            isp_demosaic_interp_ratio1_write(ViPipe, i, pstDynaRegCfg->u32hfIntpRatio);
            isp_demosaic_interp_ratio2_write(ViPipe, i, pstDynaRegCfg->u32hfIntpRatio1);
            isp_demosaic_hf_intp_bld_low_write(ViPipe, i, pstDynaRegCfg->u16hfIntpBldLow);
            isp_demosaic_hf_intp_bld_high_write(ViPipe, i, pstDynaRegCfg->u16hfIntpBldHig);
            isp_demosaic_hf_intp_th_low_write(ViPipe, i, pstDynaRegCfg->u16hfIntpThLow);
            isp_demosaic_hf_intp_th_high_write(ViPipe, i, pstDynaRegCfg->u16hfIntpThHig);
            isp_demosaic_hf_intp_th_low1_write(ViPipe, i, pstDynaRegCfg->u16hfIntpThLow1);
            isp_demosaic_hf_intp_th_high1_write(ViPipe, i, pstDynaRegCfg->u16hfIntpThHig1);
            isp_nddm_bldr_cbcr_write(ViPipe, i, pstDynaRegCfg->u8BldrCbCr);
            isp_nddm_awb_gf_gn_low_write(ViPipe, i, pstDynaRegCfg->u8AwbGFGainLow);
            isp_nddm_awb_gf_gn_high_write(ViPipe, i, pstDynaRegCfg->u8AwbGFGainHig);
            isp_nddm_awb_gf_gn_max_write(ViPipe, i, pstDynaRegCfg->u8AwbGFGainMax);
            isp_nddm_dith_max_write(ViPipe, i, pstDynaRegCfg->u8DitherMax);
            isp_nddm_fcr_det_low_write(ViPipe, i, pstDynaRegCfg->u16FcrDetLow);
            isp_nddm_fcr_gf_gain_write(ViPipe, i, pstDynaRegCfg->u8FcrGFGain);
            isp_nddm_filter_str_intp_write(ViPipe, i, pstDynaRegCfg->u8FilterStrIntp);
            isp_nddm_clip_delta_intp_low_write(ViPipe, i, pstDynaRegCfg->u16ClipDeltaIntpLow);
            isp_nddm_clip_delta_intp_high_write(ViPipe, i, pstDynaRegCfg->u16ClipDeltaIntpHigh);
            isp_nddm_filter_str_filt_write(ViPipe, i, pstDynaRegCfg->u8FilterStrFilt);
            isp_nddm_clip_delta_filt_low_write(ViPipe, i, pstDynaRegCfg->u16ClipDeltaFiltLow);
            isp_nddm_clip_delta_filt_high_write(ViPipe, i, pstDynaRegCfg->u16ClipDeltaFiltHigh);
            isp_nddm_bldr_gray_write(ViPipe, i, pstDynaRegCfg->u8BldrGray);
            isp_nddm_satu_r_th_fix_write(ViPipe, i, pstDynaRegCfg->u16SatuRThFix);
            isp_nddm_satu_r_th_low_write(ViPipe, i, pstDynaRegCfg->u16SatuRThLow);
            isp_nddm_satu_r_th_high_write(ViPipe, i, pstDynaRegCfg->u16SatuRThHig);
            isp_nddm_satu_b_th_fix_write(ViPipe, i, pstDynaRegCfg->u16SatuBThFix);
            isp_nddm_satu_b_th_low_write(ViPipe, i, pstDynaRegCfg->u16SatuBThLow);
            isp_nddm_satu_b_th_high_write(ViPipe, i, pstDynaRegCfg->u16SatuBThHig);
            isp_nddm_satu_fix_ehcy_write(ViPipe, i, pstDynaRegCfg->s16SatuFixEhcY);

            if (pstDynaRegCfg->bUpdateUsm)
            {
                if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                    || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
                {
                    isp_nddm_usm_lut_waddr_write(ViPipe, i, 0);

                    for (j = 0; j < HI_ISP_DEMOSAIC_LUT_LENGTH; j++)
                    {
                        isp_nddm_usm_lut_wdata_write(ViPipe, i, pstDynaRegCfg->au8EhcGainLut[j]);
                    }
                }
                else
                {
                    isp_nddm_usmlut_write(ViPipe, i, pstDynaRegCfg->au8EhcGainLut);
                }

                //isp_nddm_usm_lut_update_write(ViPipe, i, pstDynaRegCfg->bUpdateUsm);
                bUsmLutUpdate = pstDynaRegCfg->bUpdateUsm;
                pstDynaRegCfg->bUpdateUsm = bIsOfflineMode;
            }

            if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                isp_nddm_gf_lut_waddr_write(ViPipe, i, 0);

                for (j = 0; j < HI_ISP_DEMOSAIC_LUT_LENGTH; j++)
                {
                    isp_nddm_gf_lut_wdata_write(ViPipe, i, pstDynaRegCfg->au16GFBlurLut[j]);
                }
            }
            else
            {
                isp_nddm_gflut_write(ViPipe, i, pstDynaRegCfg->au16GFBlurLut);
            }

            //isp_nddm_gf_lut_update_write(ViPipe, i, pstDynaRegCfg->bUpdateGF);
            bGfLutUpdate         = pstDynaRegCfg->bUpdateGF;
            pstDynaRegCfg->bResh = bIsOfflineMode;

        }

        pstRegCfgInfo->unKey.bit1DemCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bNddmUsmLutUpdate = bUsmLutUpdate;
    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bNddmGfLutUpdate  = bGfLutUpdate;

    return HI_SUCCESS;
}

static HI_S32 ISP_FpnRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL bIsOfflineMode;
    HI_U8 u8BlkNum = pstRegCfgInfo->u8CfgNum;
    //    ISP_FPN_STATIC_CFG_S* pstStaticRegCfg = HI_NULL;
    ISP_FPN_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_CTX_S            *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));


    if (pstRegCfgInfo->unKey.bit1FpnCfg)
    {
        /*dynamic*/
        pstDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stFpnRegCfg.stDynaRegCfg;

        if (bIsOfflineMode)
        {
            isp_fpn_mode_write(ViPipe, i, pstDynaRegCfg->u32IspFpnCalibCorr);
            isp_dcg_fpn_sel(ViPipe, i, pstDynaRegCfg->bIspFpnEnable);
            isp_fpn_en_write(ViPipe, i, pstDynaRegCfg->bIspFpnEnable);

            isp_fpn_line_frame_write(ViPipe, i, pstDynaRegCfg->u32IspFpnLineFrame);
            isp_fpn_correct0_en_write(ViPipe, i, pstDynaRegCfg->u32IspFpnCorrectEnId[0]);
            isp_fpn_correct1_en_write(ViPipe, i, pstDynaRegCfg->u32IspFpnCorrectEnId[1]);
            isp_fpn_correct2_en_write(ViPipe, i, pstDynaRegCfg->u32IspFpnCorrectEnId[2]);
            isp_fpn_correct3_en_write(ViPipe, i, pstDynaRegCfg->u32IspFpnCorrectEnId[3]);
            isp_fpn_frame_calib_shift_write(ViPipe, i, pstDynaRegCfg->u32IspFpnFrameCalibShift);
            isp_fpn_out_shift_write(ViPipe, i, pstDynaRegCfg->u32IspFpnOutShift);
            isp_fpn_in_shift_write(ViPipe, i, pstDynaRegCfg->u32IspFpnInShift);
            isp_fpn_shift_write(ViPipe, i, pstDynaRegCfg->u32IspFpnShift);
            isp_fpn_offset0_write(ViPipe, i, pstDynaRegCfg->u32IspFpnOffset[0]);
            isp_fpn_offset1_write(ViPipe, i, pstDynaRegCfg->u32IspFpnOffset[1]);
            isp_fpn_offset2_write(ViPipe, i, pstDynaRegCfg->u32IspFpnOffset[2]);
            isp_fpn_offset3_write(ViPipe, i, pstDynaRegCfg->u32IspFpnOffset[3]);
            isp_fpn_strength0_write(ViPipe, i, pstDynaRegCfg->u32IspFpnStrength[0]);
            isp_fpn_strength1_write(ViPipe, i, pstDynaRegCfg->u32IspFpnStrength[1]);
            isp_fpn_strength2_write(ViPipe, i, pstDynaRegCfg->u32IspFpnStrength[2]);
            isp_fpn_strength3_write(ViPipe, i, pstDynaRegCfg->u32IspFpnStrength[3]);
            isp_fpn_max_o_write(ViPipe, i, pstDynaRegCfg->u32IspFpnMaxO);
            isp_fpn_overflowthr_write(ViPipe, i, pstDynaRegCfg->u32IspFpnOverflowThr);
            isp_fpn_line_raddr_write(ViPipe, i, pstDynaRegCfg->u32IspFpnLineRaddr);
            isp_fpn_line_waddr_write(ViPipe, i, pstDynaRegCfg->u32IspFpnLineWaddr);
        }
        else
        {
            isp_fpn_strength0_write(ViPipe, i, pstDynaRegCfg->u32IspFpnStrength[0]);
            isp_fpn_strength1_write(ViPipe, i, pstDynaRegCfg->u32IspFpnStrength[1]);
            isp_fpn_strength2_write(ViPipe, i, pstDynaRegCfg->u32IspFpnStrength[2]);
            isp_fpn_strength3_write(ViPipe, i, pstDynaRegCfg->u32IspFpnStrength[3]);
        }

        pstRegCfgInfo->unKey.bit1FpnCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_LdciRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL bIsOfflineMode;
    HI_BOOL bLdciDrcLutUpdate  = HI_FALSE;
    HI_BOOL bLdciCalcLutUpdate = HI_FALSE;
    HI_U8   u8BlkNum          = pstRegCfgInfo->u8CfgNum;
    HI_U16  j;
    ISP_LDCI_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;
    ISP_LDCI_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_CTX_S             *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1LdciCfg )
    {

        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stLdciRegCfg.stStaticRegCfg;
        pstDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stLdciRegCfg.stDynaRegCfg;

        if (pstStaticRegCfg->bStaticResh)
        {
            /*static*/
            isp_ldci_luma_sel_write(ViPipe, i, pstStaticRegCfg->u32CalcLumaSel);
            isp_ldci_bende_write(ViPipe, i, pstStaticRegCfg->bDeEnable);
            isp_ldci_deref_write(ViPipe, i, pstStaticRegCfg->bCalcDeRef);
            isp_ldci_deh_lpfsft_write(ViPipe, i, pstStaticRegCfg->u32DehLpfSft);
            isp_ldci_coefh0_write(ViPipe, i, pstStaticRegCfg->u32DehLpfCoefH[0]);
            isp_ldci_coefh1_write(ViPipe, i, pstStaticRegCfg->u32DehLpfCoefH[1]);
            isp_ldci_coefh2_write(ViPipe, i, pstStaticRegCfg->u32DehLpfCoefH[2]);
            isp_ldci_coefv0_write(ViPipe, i, pstStaticRegCfg->u32DehLpfCoefV[0]);
            isp_ldci_coefv1_write(ViPipe, i, pstStaticRegCfg->u32DehLpfCoefV[1]);
            isp_ldci_calc_drcen_write(ViPipe, i, pstStaticRegCfg->bCalcDrcEnable);
            isp_ldci_pflmt_en_write(ViPipe, i, pstStaticRegCfg->bCalcPfLmtEnable);
            isp_ldci_pfori0_write(ViPipe, i, pstStaticRegCfg->u32CalcPfOri[0]);
            isp_ldci_pfori1_write(ViPipe, i, pstStaticRegCfg->u32CalcPfOri[1]);
            isp_ldci_pfsft0_write(ViPipe, i, pstStaticRegCfg->u32CalcPfSft[0]);
            isp_ldci_pfsft1_write(ViPipe, i, pstStaticRegCfg->u32CalcPfSft[1]);
            isp_ldci_pfsft2_write(ViPipe, i, pstStaticRegCfg->u32CalcPfSft[2]);
            isp_ldci_pfsft3_write(ViPipe, i, pstStaticRegCfg->u32CalcPfSft[3]);
            isp_ldci_pfthd0_write(ViPipe, i, pstStaticRegCfg->u32CalcPfThd[0]);
            isp_ldci_pfthd1_write(ViPipe, i, pstStaticRegCfg->u32CalcPfThd[1]);
            isp_ldci_pfrly0_write(ViPipe, i, pstStaticRegCfg->u32CalcPfRly[0]);
            isp_ldci_pfrly1_write(ViPipe, i, pstStaticRegCfg->u32CalcPfRly[1]);
            isp_ldci_pfmul_write(ViPipe, i, pstStaticRegCfg->u32CalcPfmul);
            isp_ldci_lpfsft_write(ViPipe, i, pstStaticRegCfg->u32LpfSft);
            isp_ldci_stat_drcen_write(ViPipe, i, pstStaticRegCfg->bStatDrcEnable);
            isp_ldci_chrposdamp_write(ViPipe, i, pstStaticRegCfg->u32ChrPosDamp);
            isp_ldci_chrnegdamp_write(ViPipe, i, pstStaticRegCfg->u32ChrNegDamp);
            isp_ldci_glb_hewgt_write(ViPipe, i, pstStaticRegCfg->u32GlobalHeWgt);

            if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                isp_ldci_poply_waddr_write(ViPipe, i, 0);
                isp_ldci_plyq01_waddr_write(ViPipe, i, 0);
                isp_ldci_plyq23_waddr_write(ViPipe, i, 0);
                isp_ldci_drc_waddr_write(ViPipe, i, 0);

                for (j = 0; j < 65; j++)
                {
                    isp_ldci_poply_wdata_write(ViPipe, i, pstStaticRegCfg->s32PolyP1[j], pstStaticRegCfg->s32StatPolyP2[j], pstStaticRegCfg->s32StatPolyP3[j]);
                    isp_ldci_plyq01_wdata_write(ViPipe, i, pstStaticRegCfg->s32PolyQ0[j], pstStaticRegCfg->s32PolyQ1[j]);
                    isp_ldci_plyq23_wdata_write(ViPipe, i, pstStaticRegCfg->s32PolyQ2[j], pstStaticRegCfg->s32PolyQ3[j]);
                    isp_ldci_drc_wdata_write(ViPipe, i, pstStaticRegCfg->u32CalcDynRngCmpLut[j], pstStaticRegCfg->u32StatDynRngCmpLut[j]);
                }
            }
            else
            {
                isp_ldci_poply1_wlut_write(ViPipe, i, pstStaticRegCfg->s32PolyP1);
                isp_ldci_poply2_wlut_write(ViPipe, i, pstStaticRegCfg->s32StatPolyP2);
                isp_ldci_poply3_wlut_write(ViPipe, i, pstStaticRegCfg->s32StatPolyP3);
                isp_ldci_plyq0_wlut_write(ViPipe, i, pstStaticRegCfg->s32PolyQ0);
                isp_ldci_plyq1_wlut_write(ViPipe, i, pstStaticRegCfg->s32PolyQ1);
                isp_ldci_plyq2_wlut_write(ViPipe, i, pstStaticRegCfg->s32PolyQ2);
                isp_ldci_plyq3_wlut_write(ViPipe, i, pstStaticRegCfg->s32PolyQ3);
                isp_ldci_statdrc_wlut_write(ViPipe, i, pstStaticRegCfg->u32StatDynRngCmpLut);
                isp_ldci_calcdrc_wlut_write(ViPipe, i, pstStaticRegCfg->u32CalcDynRngCmpLut);

                isp_ldci_usmpos_wlut_write(ViPipe, i, pstDynaRegCfg->u32UsmPosLut);
                isp_ldci_usmneg_wlut_write(ViPipe, i, pstDynaRegCfg->u32UsmNegLut);
                isp_ldci_delut_wlut_write(ViPipe, i, pstDynaRegCfg->u32DeLut);
                isp_ldci_cgain_wlut_write(ViPipe, i, pstDynaRegCfg->u32ColorGainLut);
            }

            //isp_ldci_drc_lut_update_write(ViPipe, i, HI_TRUE);
            bLdciDrcLutUpdate            = HI_TRUE;
            pstStaticRegCfg->bStaticResh = HI_FALSE;
        }

        /*dynamic*/
        isp_ldci_en_write(ViPipe, i, pstDynaRegCfg->bEnable);
        isp_ldci_wrstat_en_write(ViPipe, i, pstDynaRegCfg->bEnable);
        isp_ldci_rdstat_en_write(ViPipe, i, pstDynaRegCfg->bEnable);
        isp_ldci_calc_en_write(ViPipe, i, pstDynaRegCfg->bCalcEnable);
        isp_ldci_blc_ctrl_write(ViPipe, i, pstDynaRegCfg->u32CalcBlcCtrl);
        isp_ldci_lpfcoef0_write(ViPipe, i, pstDynaRegCfg->u32LpfCoef[0]);
        isp_ldci_lpfcoef1_write(ViPipe, i, pstDynaRegCfg->u32LpfCoef[1]);
        isp_ldci_lpfcoef2_write(ViPipe, i, pstDynaRegCfg->u32LpfCoef[2]);
        isp_ldci_lpfcoef3_write(ViPipe, i, pstDynaRegCfg->u32LpfCoef[3]);
        isp_ldci_lpfcoef4_write(ViPipe, i, pstDynaRegCfg->u32LpfCoef[4]);
        isp_ldci_lpfcoef5_write(ViPipe, i, pstDynaRegCfg->u32LpfCoef[5]);
        isp_ldci_lpfcoef6_write(ViPipe, i, pstDynaRegCfg->u32LpfCoef[6]);
        isp_ldci_lpfcoef7_write(ViPipe, i, pstDynaRegCfg->u32LpfCoef[7]);
        isp_ldci_lpfcoef8_write(ViPipe, i, pstDynaRegCfg->u32LpfCoef[8]);
        isp_ldci_calc_map_offsetx_write(ViPipe, i, pstDynaRegCfg->u32CalcMapOffsetX);
        isp_ldci_smlmapstride_write(ViPipe, i, pstDynaRegCfg->u32CalcSmlMapStride);
        isp_ldci_smlmapheight_write(ViPipe, i, pstDynaRegCfg->u32CalcSmlMapHeight);
        isp_ldci_total_zone_write(ViPipe, i, pstDynaRegCfg->u32CalcTotalZone);
        isp_ldci_scalex_write(ViPipe, i, pstDynaRegCfg->u32CalcScaleX);
        isp_ldci_scaley_write(ViPipe, i, pstDynaRegCfg->u32CalcScaleY);
        isp_ldci_stat_smlmapwidth_write(ViPipe, i, pstDynaRegCfg->u32StatSmlMapWidth);
        isp_ldci_stat_smlmapheight_write(ViPipe, i, pstDynaRegCfg->u32StatSmlMapHeight);
        isp_ldci_stat_total_zone_write(ViPipe, i, pstDynaRegCfg->u32StatTotalZone);
        isp_ldci_blk_smlmapwidth0_write(ViPipe, i, pstDynaRegCfg->u32BlkSmlMapWidth[0]);
        isp_ldci_blk_smlmapwidth1_write(ViPipe, i, pstDynaRegCfg->u32BlkSmlMapWidth[1]);
        isp_ldci_blk_smlmapwidth2_write(ViPipe, i, pstDynaRegCfg->u32BlkSmlMapWidth[2]);
        isp_ldci_hstart_write(ViPipe, i, pstDynaRegCfg->u32StatHStart);
        isp_ldci_hend_write(ViPipe, i, pstDynaRegCfg->u32StatHEnd);
        isp_ldci_vstart_write(ViPipe, i, pstDynaRegCfg->u32StatVStart);
        isp_ldci_vend_write(ViPipe, i, pstDynaRegCfg->u32StatVEnd);

        if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
            || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
        {
            isp_ldci_de_usm_waddr_write(ViPipe, i, 0);

            for (j = 0; j < LDCI_DE_USM_LUT_SIZE; j++)
            {
                isp_ldci_de_usm_wdata_write(ViPipe, i, pstDynaRegCfg->u32UsmPosLut[j], pstDynaRegCfg->u32UsmNegLut[j], pstDynaRegCfg->u32DeLut[j]);
            }

            isp_ldci_cgain_waddr_write(ViPipe, i, 0);

            for (j = 0; j < LDCI_COLOR_GAIN_LUT_SIZE; j++)
            {
                isp_ldci_cgain_wdata_write(ViPipe, i, pstDynaRegCfg->u32ColorGainLut[j]);
            }

            isp_ldci_he_waddr_write(ViPipe, i, 0);

            for (j = 0; j < LDCI_HE_LUT_SIZE; j++)
            {
                isp_ldci_he_wdata_write(ViPipe, i, pstDynaRegCfg->u32HePosLut[j], pstDynaRegCfg->u32HeNegLut[j]);
            }
        }
        else
        {
            isp_ldci_hepos_wlut_write(ViPipe, i, pstDynaRegCfg->u32HePosLut);
            isp_ldci_heneg_wlut_write(ViPipe, i, pstDynaRegCfg->u32HeNegLut);
        }

        //isp_ldci_calc_lut_update_write(ViPipe, i, pstDynaRegCfg->bCalcLutUpdate);
        bLdciCalcLutUpdate = pstDynaRegCfg->bCalcLutUpdate;
        pstRegCfgInfo->unKey.bit1LdciCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bLdciCalcLutUpdate = bLdciCalcLutUpdate;
    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bLdciDrcLutUpdate  = bLdciDrcLutUpdate;

    return HI_SUCCESS;
}

static HI_S32 ISP_LogLUTRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_U32 j;
    HI_U8 u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_BOOL  bIsOfflineMode;
    ISP_LOGLUT_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_LOGLUT_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;
    ISP_CTX_S               *pstIspCtx       = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1LogLUTCfg)
    {
        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stLogLUTRegCfg.stStaticRegCfg;
        pstDynaRegCfg   = &pstRegCfgInfo->stAlgRegCfg[i].stLogLUTRegCfg.stDynaRegCfg;

        if (pstStaticRegCfg->bStaticResh)
        {
            /*static*/
            isp_loglut_bitw_out_write(ViPipe, i, pstStaticRegCfg->u8BitDepthOut);

            if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                isp_loglut_lut_waddr_write(ViPipe, i, 0);

                for (j = 0; j < LOG_LUT_SIZE; j++)
                {
                    isp_loglut_lut_wdata_write(ViPipe, i, pstStaticRegCfg->au32LogLUT[j]);
                }
            }
            else
            {
                isp_loglut_lut_write(ViPipe, i, pstStaticRegCfg->au32LogLUT);
            }

            pstStaticRegCfg->bStaticResh = HI_FALSE;
        }

        /*dynamic*/

        isp_loglut_en_write(ViPipe, i, pstDynaRegCfg->bEnable);

        pstRegCfgInfo->unKey.bit1LogLUTCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    return HI_SUCCESS;
}


static HI_S32 ISP_LcacRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_U8 u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_BOOL  bIsOfflineMode;
    ISP_LOCAL_CAC_USR_CFG_S    *pstUsrRegCfg    = HI_NULL;
    ISP_LOCAL_CAC_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_LOCAL_CAC_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;
    ISP_CTX_S                  *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1LocalCacCfg)
    {
        isp_demosaic_local_cac_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stLCacRegCfg.bLocalCacEn);

        /*static*/
        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stLCacRegCfg.stStaticRegCfg;

        if (pstStaticRegCfg->bStaticResh)
        {
            isp_nddm_cac_blend_en_write(ViPipe, i, pstStaticRegCfg->bNddmCacBlendEn);
            isp_nddm_cac_blend_rate_write(ViPipe, i, pstStaticRegCfg->u16NddmCacBlendRate);
            isp_demosaic_r_counter_thr_write(ViPipe, i, pstStaticRegCfg->u8RCounterThr);
            isp_demosaic_g_counter_thr_write(ViPipe, i, pstStaticRegCfg->u8GCounterThr);
            isp_demosaic_b_counter_thr_write(ViPipe, i, pstStaticRegCfg->u8BCounterThr);
            isp_demosaic_satu_thr_write(ViPipe, i, pstStaticRegCfg->u16SatuThr);
            isp_demosaic_fake_cr_var_thr_high_write(ViPipe, i, pstStaticRegCfg->u16FakeCrVarThrHigh);
            isp_demosaic_fake_cr_var_thr_low_write(ViPipe, i, pstStaticRegCfg->u16FakeCrVarThrLow);
            pstStaticRegCfg->bStaticResh = HI_FALSE;
        }

        /* Usr */
        pstUsrRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stLCacRegCfg.stUsrRegCfg;

        if (pstUsrRegCfg->bResh)
        {
            isp_demosaic_purple_var_thr_write(ViPipe, i, pstUsrRegCfg->u16VarThr);
            isp_demosaic_cb_thr_write(ViPipe, i, pstUsrRegCfg->u16CbThr);
            //isp_demosaic_luma_thr_write(ViPipe, i, pstUsrRegCfg->u16LumaThr);
            isp_demosaic_cac_luma_high_cnt_thr_write(ViPipe, i, pstUsrRegCfg->u8LumaHighCntThr);
            isp_demosaic_cac_cb_cnt_low_thr_write(ViPipe, i, pstUsrRegCfg->u8CbCntLowThr);
            isp_demosaic_cac_cb_cnt_high_thr_write(ViPipe, i, pstUsrRegCfg->u8CbCntHighThr);
            isp_demosaci_cac_bld_avg_cur_write(ViPipe, i, pstUsrRegCfg->u8BldAvgCur);
            isp_demosaic_cbcr_ratio_high_limit_write(ViPipe, i, pstUsrRegCfg->s16CbCrRatioLmtHigh);
            isp_demosaic_cbcr_ratio_low_limit_write(ViPipe, i, pstUsrRegCfg->s16CbCrRatioLmtLow);

            pstUsrRegCfg->bResh = bIsOfflineMode;
        }

        /*dynamic*/
        pstDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stLCacRegCfg.stDynaRegCfg;

        if (pstDynaRegCfg->bResh)
        {
            isp_demosaic_r_luma_thr_write(ViPipe, i, pstDynaRegCfg->u16RLumaThr);
            isp_demosaic_g_luma_thr_write(ViPipe, i, pstDynaRegCfg->u16GLumaThr);
            isp_demosaic_b_luma_thr_write(ViPipe, i, pstDynaRegCfg->u16BLumaThr);
            isp_demosaic_luma_thr_write  (ViPipe, i, pstDynaRegCfg->u16LumaThr);
            isp_demosaic_depurplectr_cr_write(ViPipe, i, pstDynaRegCfg->u8DePurpleCtrCr);
            isp_demosaic_depurplectr_cb_write(ViPipe, i, pstDynaRegCfg->u8DePurpleCtrCb);
            pstDynaRegCfg->bResh = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1LocalCacCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_FcrRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_U8 u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_BOOL  bIsOfflineMode;
    ISP_ANTIFALSECOLOR_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_ANTIFALSECOLOR_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;
    ISP_CTX_S                       *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1FcrCfg)
    {
        isp_demosaic_fcr_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stAntiFalseColorRegCfg.bFcrEnable);

        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stAntiFalseColorRegCfg.stStaticRegCfg;
        pstDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stAntiFalseColorRegCfg.stDynaRegCfg;

        /*static*/
        if (pstStaticRegCfg->bResh )
        {
            isp_demosaic_fcr_limit1_write(ViPipe, i, pstStaticRegCfg->u16FcrLimit1);
            isp_demosaic_fcr_limit2_write(ViPipe, i, pstStaticRegCfg->u16FcrLimit2);
            isp_demosaic_fcr_thr_write(ViPipe, i, pstStaticRegCfg->u16FcrThr);
            pstStaticRegCfg->bResh = HI_FALSE;
        }

        /*dynamic*/
        if (pstDynaRegCfg->bResh)
        {
            isp_demosaic_fcr_gain_write(ViPipe, i, pstDynaRegCfg->u8FcrGain);
            isp_demosaic_fcr_ratio_write(ViPipe, i, pstDynaRegCfg->u8FcrRatio);
            isp_demosaic_fcr_gray_ratio_write(ViPipe, i, pstDynaRegCfg->u8FcrGrayRatio);
            isp_demosaic_fcr_cmax_sel_write(ViPipe, i, pstDynaRegCfg->u8FcrCmaxSel);
            isp_demosaic_fcr_detg_sel_write(ViPipe, i, pstDynaRegCfg->u8FcrDetgSel);
            isp_demosaic_fcr_thresh1_write(ViPipe, i, pstDynaRegCfg->u16FcrHfThreshLow);
            isp_demosaic_fcr_thresh2_write(ViPipe, i, pstDynaRegCfg->u16FcrHfThreshHig);
            pstDynaRegCfg->bResh = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1FcrCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_GcacRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL  bIsOfflineMode;
    HI_BOOL  bUsrResh  = HI_FALSE;
    HI_BOOL  bIdxResh  = HI_FALSE;
    HI_U8 u8BlkNum = pstRegCfgInfo->u8CfgNum;
    ISP_GLOBAL_CAC_USR_CFG_S    *pstUsrRegCfg    = HI_NULL;
    ISP_GLOBAL_CAC_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;
    ISP_CTX_S                   *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1GlobalCacCfg)
    {
        isp_gcac_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stGCacRegCfg.bGlobalCacEn);

        /*static*/
        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stGCacRegCfg.stStaticRegCfg;

        if (pstStaticRegCfg->bStaticResh)
        {
            isp_gcac_ver_filt_en_write(ViPipe, i, pstStaticRegCfg->bGCacVerFilEn);
            isp_demosaic_gcac_blend_select_write(ViPipe, i, pstStaticRegCfg->u8GCacBlendSel);
            isp_gcac_chrv_mode_write(ViPipe, i, pstStaticRegCfg->u8GcacChrVerMode);
            isp_gcac_clip_mode_v_write(ViPipe, i, pstStaticRegCfg->u8GcacClipModeVer);
            isp_gcac_clip_mode_h_write(ViPipe, i, pstStaticRegCfg->u8GcacClipModeHor);
            pstStaticRegCfg->bStaticResh = HI_FALSE;
        }

        /*usr*/
        pstUsrRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stGCacRegCfg.stUsrRegCfg;
        bIdxResh = (isp_gcac_update_index_read(ViPipe, i) != pstUsrRegCfg->u32UpdateIndex);
        bUsrResh = (bIsOfflineMode) ? (pstUsrRegCfg->bResh & bIdxResh) : (pstUsrRegCfg->bResh);

        if (bUsrResh)
        {
            isp_gcac_update_index_write(ViPipe, i, pstUsrRegCfg->u32UpdateIndex);
            isp_gcac_red_a_write(ViPipe, i, pstUsrRegCfg->s16ParamRedA);
            isp_gcac_red_b_write(ViPipe, i, pstUsrRegCfg->s16ParamRedB);
            isp_gcac_red_c_write(ViPipe, i, pstUsrRegCfg->s16ParamRedC);
            isp_gcac_blue_a_write(ViPipe, i, pstUsrRegCfg->s16ParamBlueA);
            isp_gcac_blue_b_write(ViPipe, i, pstUsrRegCfg->s16ParamBlueB);
            isp_gcac_blue_c_write(ViPipe, i, pstUsrRegCfg->s16ParamBlueC);
            isp_gcac_ns_y_write(ViPipe, i, pstUsrRegCfg->u8VerNormShift);
            isp_gcac_nf_y_write(ViPipe, i, pstUsrRegCfg->u8VerNormFactor);
            isp_gcac_ns_x_write(ViPipe, i, pstUsrRegCfg->u8HorNormShift);
            isp_gcac_nf_x_write(ViPipe, i, pstUsrRegCfg->u8HorNormFactor);
            isp_gcac_cnt_start_h_write(ViPipe, i, pstUsrRegCfg->u16CenterCoorHor);
            isp_gcac_cnt_start_v_write(ViPipe, i, pstUsrRegCfg->u16CenterCoorVer);
            isp_demosaic_varthr_for_blend_write(ViPipe, i, pstUsrRegCfg->u16CorVarThr);
            isp_gcac_cor_start_h_write(ViPipe, i, pstUsrRegCfg->u16StartCoorHor);
            isp_gcac_cor_start_v_write(ViPipe, i, pstUsrRegCfg->u16StartCoorVer);
            pstUsrRegCfg->bResh = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1GlobalCacCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    return HI_SUCCESS;
}


static HI_S32 ISP_DpcRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL  bUsrResh   = HI_FALSE;
    HI_BOOL  bIdxResh   = HI_FALSE;
    HI_BOOL  bLutUpdate = HI_FALSE;
    HI_BOOL  bIsOfflineMode;
    HI_U8 u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_U16 j;
    ISP_DPC_STATIC_CFG_S    *pstStaticRegCfg = HI_NULL;
    ISP_DPC_DYNA_CFG_S      *pstDynaRegCfg   = HI_NULL;
    ISP_DPC_USR_CFG_S       *pstUsrRegCfg    = HI_NULL;
    ISP_CTX_S               *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1DpCfg)
    {
        isp_dpc_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stDpRegCfg.abDpcEn[0]);
        isp_dpc_dpc_en1_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stDpRegCfg.abDpcEn[1]);
        isp_dpc_dpc_en2_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stDpRegCfg.abDpcEn[2]);
        isp_dpc_dpc_en3_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stDpRegCfg.abDpcEn[3]);

        /*static*/
        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stDpRegCfg.stStaticRegCfg;

        if (pstStaticRegCfg->bStaticResh)
        {
            isp_dpc_output_mode_write(ViPipe, i, pstStaticRegCfg->u32DpccOutputMode);
            isp_dpc_bpt_ctrl_write(ViPipe, i, pstStaticRegCfg->u32DpccBptCtrl);

            pstStaticRegCfg->bStaticResh = HI_FALSE;
        }

        /*usr*/
        pstUsrRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stDpRegCfg.stUsrRegCfg;

        if (pstUsrRegCfg->stUsrDynaCorRegCfg.bResh)
        {
            isp_dpc_ex_soft_thr_max_write(ViPipe, i, pstUsrRegCfg->stUsrDynaCorRegCfg.s8DpccSupTwinkleThrMax);
            isp_dpc_ex_soft_thr_min_write(ViPipe, i, pstUsrRegCfg->stUsrDynaCorRegCfg.s8DpccSupTwinkleThrMin);
            isp_dpc_ex_hard_thr_en_write(ViPipe, i, pstUsrRegCfg->stUsrDynaCorRegCfg.bDpccHardThrEn);
            isp_dpc_ex_rake_ratio_write(ViPipe, i, pstUsrRegCfg->stUsrDynaCorRegCfg.u16DpccRakeRatio);
            pstUsrRegCfg->stUsrDynaCorRegCfg.bResh = bIsOfflineMode;
        }

        bIdxResh = (isp_dpc_update_index_read(ViPipe, i) != pstUsrRegCfg->stUsrStaCorRegCfg.u32UpdateIndex);
        bUsrResh = (bIsOfflineMode) ? (pstUsrRegCfg->stUsrStaCorRegCfg.bResh & bIdxResh) : (pstUsrRegCfg->stUsrStaCorRegCfg.bResh);

        if (bUsrResh)
        {
            isp_dpc_update_index_write(ViPipe, i, pstUsrRegCfg->stUsrStaCorRegCfg.u32UpdateIndex);
            isp_dpc_bpt_number_write(ViPipe, i, pstUsrRegCfg->stUsrStaCorRegCfg.u32DpccBptNumber);

            if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                isp_dpc_bpt_waddr_write(ViPipe, i, 0);

                for (j = 0; j < STATIC_DP_COUNT_NORMAL; j++)
                {
                    if (j < pstUsrRegCfg->stUsrStaCorRegCfg.u32DpccBptNumber)
                    {
                        isp_dpc_bpt_wdata_write(ViPipe, i, pstUsrRegCfg->stUsrStaCorRegCfg.au32DpccBpTable[j]);
                    }
                    else
                    {
                        isp_dpc_bpt_wdata_write(ViPipe, i, 0);
                    }
                }
            }
            else
            {
                isp_dpc_bpt_wlut_write(ViPipe, i, pstUsrRegCfg->stUsrStaCorRegCfg.au32DpccBpTable);
            }

            //isp_dpc_ex_lut_update_write(ViPipe, i, HI_TRUE);
            bLutUpdate = HI_TRUE;
            pstUsrRegCfg->stUsrStaCorRegCfg.bResh = bIsOfflineMode;
        }

        /*dynamic*/
        pstDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stDpRegCfg.stDynaRegCfg;

        if (pstDynaRegCfg->bResh)
        {
            isp_dpc_stat_en_write(ViPipe, i, pstDynaRegCfg->bDpcStatEn);
            isp_dpc_blend_write(ViPipe, i, pstDynaRegCfg->u32DpccAlpha);
            isp_dpc_mode_write(ViPipe, i, pstDynaRegCfg->u32DpccMode);
            isp_dpc_set_use_write(ViPipe, i, pstDynaRegCfg->u32DpccSetUse);
            isp_dpc_methods_set_1_write(ViPipe, i, pstDynaRegCfg->u32DpccMethodsSet1);
            isp_dpc_methods_set_2_write(ViPipe, i, pstDynaRegCfg->u32DpccMethodsSet2);
            isp_dpc_methods_set_3_write(ViPipe, i, pstDynaRegCfg->u32DpccMethodsSet3);
            isp_dpc_line_thresh_1_write(ViPipe, i, pstDynaRegCfg->au16DpccLineThr[0]);
            isp_dpc_line_mad_fac_1_write(ViPipe, i, pstDynaRegCfg->au16DpccLineMadFac[0]);
            isp_dpc_pg_fac_1_write(ViPipe, i, pstDynaRegCfg->au16DpccPgFac[0]);
            isp_dpc_rnd_thresh_1_write(ViPipe, i, pstDynaRegCfg->au16DpccRndThr[0]);
            isp_dpc_rg_fac_1_write(ViPipe, i, pstDynaRegCfg->au16DpccRgFac[0]);
            isp_dpc_line_thresh_2_write(ViPipe, i, pstDynaRegCfg->au16DpccLineThr[1]);
            isp_dpc_line_mad_fac_2_write(ViPipe, i, pstDynaRegCfg->au16DpccLineMadFac[1]);
            isp_dpc_pg_fac_2_write(ViPipe, i, pstDynaRegCfg->au16DpccPgFac[1]);
            isp_dpc_rnd_thresh_2_write(ViPipe, i, pstDynaRegCfg->au16DpccRndThr[1]);
            isp_dpc_rg_fac_2_write(ViPipe, i, pstDynaRegCfg->au16DpccRgFac[1]);
            isp_dpc_line_thresh_3_write(ViPipe, i, pstDynaRegCfg->au16DpccLineThr[2]);
            isp_dpc_line_mad_fac_3_write(ViPipe, i, pstDynaRegCfg->au16DpccLineMadFac[2]);
            isp_dpc_pg_fac_3_write(ViPipe, i, pstDynaRegCfg->au16DpccPgFac[2]);
            isp_dpc_rnd_thresh_3_write(ViPipe, i, pstDynaRegCfg->au16DpccRndThr[2]);
            isp_dpc_rg_fac_3_write(ViPipe, i, pstDynaRegCfg->au16DpccRgFac[2]);
            isp_dpc_ro_limits_write(ViPipe, i, pstDynaRegCfg->u32DpccRoLimits);
            isp_dpc_rnd_offs_write(ViPipe, i, pstDynaRegCfg->u32DpccRndOffs);
            isp_dpc_bpt_thresh_write(ViPipe, i, pstDynaRegCfg->u32DpccBadThresh);

            isp_dpc_line_std_thr_1_write(ViPipe, i, pstDynaRegCfg->au16DpccLineStdThr[0]);
            isp_dpc_line_std_thr_2_write(ViPipe, i, pstDynaRegCfg->au16DpccLineStdThr[1]);
            isp_dpc_line_std_thr_3_write(ViPipe, i, pstDynaRegCfg->au16DpccLineStdThr[2]);
            isp_dpc_line_std_thr_4_write(ViPipe, i, pstDynaRegCfg->au16DpccLineStdThr[3]);
            isp_dpc_line_std_thr_5_write(ViPipe, i, pstDynaRegCfg->au16DpccLineStdThr[4]);


            isp_dpc_line_diff_thr_1_write(ViPipe, i, pstDynaRegCfg->au16DpccLineDiffThr[0]);
            isp_dpc_line_diff_thr_2_write(ViPipe, i, pstDynaRegCfg->au16DpccLineDiffThr[1]);
            isp_dpc_line_diff_thr_3_write(ViPipe, i, pstDynaRegCfg->au16DpccLineDiffThr[2]);
            isp_dpc_line_diff_thr_4_write(ViPipe, i, pstDynaRegCfg->au16DpccLineDiffThr[3]);
            isp_dpc_line_diff_thr_5_write(ViPipe, i, pstDynaRegCfg->au16DpccLineDiffThr[4]);

            isp_dpc_line_aver_fac_1_write(ViPipe, i, pstDynaRegCfg->au16DpccLineAverFac[0]);
            isp_dpc_line_aver_fac_2_write(ViPipe, i, pstDynaRegCfg->au16DpccLineAverFac[1]);
            isp_dpc_line_aver_fac_3_write(ViPipe, i, pstDynaRegCfg->au16DpccLineAverFac[2]);
            isp_dpc_line_aver_fac_4_write(ViPipe, i, pstDynaRegCfg->au16DpccLineAverFac[3]);
            isp_dpc_line_aver_fac_5_write(ViPipe, i, pstDynaRegCfg->au16DpccLineAverFac[4]);

            isp_dpc_line_kerdiff_fac_write(ViPipe, i, pstDynaRegCfg->u16DpccLineKerdiffFac);
            isp_dpc_blend_mode_write(ViPipe, i, pstDynaRegCfg->u16DpccBlendMode);
            isp_dpc_bit_depth_sel_write(ViPipe, i, pstDynaRegCfg->u16DpccBitDepthSel);

            pstDynaRegCfg->bResh = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1DpCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bDpcLutUpdate = bLutUpdate;

    return HI_SUCCESS;
}

static HI_S32 ISP_GeRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_U16 j;
    HI_U8 u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_BOOL  bIsOfflineMode;
    ISP_GE_STATIC_CFG_S     *pstStaticRegCfg = HI_NULL;
    ISP_GE_DYNA_CFG_S       *pstDynaRegCfg   = HI_NULL;
    ISP_GE_USR_CFG_S       *pstUsrRegCfg    = HI_NULL;
    ISP_CTX_S               *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1GeCfg)
    {
        isp_ge_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stGeRegCfg.abGeEn[0]);
        isp_ge_ge1_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stGeRegCfg.abGeEn[1]);
        isp_ge_ge2_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stGeRegCfg.abGeEn[2]);
        isp_ge_ge3_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stGeRegCfg.abGeEn[3]);

        /*static*/
        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stGeRegCfg.stStaticRegCfg;

        if (pstStaticRegCfg->bStaticResh )
        {
            isp_ge_vnum_write(ViPipe, i, pstStaticRegCfg->u8GeNumV);
            isp_ge_gr_en_write(ViPipe, i, pstStaticRegCfg->bGeGrEn);
            isp_ge_gb_en_write(ViPipe, i, pstStaticRegCfg->bGeGbEn);
            isp_ge_gr_gb_en_write(ViPipe, i, pstStaticRegCfg->bGeGrGbEn);
            isp_ge_bit_depth_sel_write(ViPipe, i, HI_ISP_GE_BIT_DEPTH_DEFAULT);

            pstStaticRegCfg->bStaticResh = HI_FALSE;
        }

        /*usr*/
        pstUsrRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stGeRegCfg.stUsrRegCfg;

        if (pstUsrRegCfg->bResh)
        {
            for (j = 0; j < pstRegCfgInfo->stAlgRegCfg[i].stGeRegCfg.u8ChnNum; j++)
            {
                isp_ge_ct_th2_write(ViPipe, i, j, pstUsrRegCfg->au16GeCtTh2[j]);
                isp_ge_ct_slope1_write(ViPipe, i, j, pstUsrRegCfg->au8GeCtSlope1[j]);
                isp_ge_ct_slope2_write(ViPipe, i, j, pstUsrRegCfg->au8GeCtSlope2[j]);
            }

            isp_ge_hnum_write(ViPipe, i, pstUsrRegCfg->u8GeNumH);
            isp_ge_crop_pos_x_write(ViPipe, i, pstUsrRegCfg->u16GeCropPosX);
            isp_ge_crop_pos_y_write(ViPipe, i, pstUsrRegCfg->u16GeCropPosY);
            isp_ge_crop_out_width_write(ViPipe, i, pstUsrRegCfg->u16GeCropOutWidth - 1);
            isp_ge_crop_out_height_write(ViPipe, i, pstUsrRegCfg->u16GeCropOutHeight - 1);
            pstUsrRegCfg->bResh = bIsOfflineMode;
        }

        /*dynamic*/
        pstDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stGeRegCfg.stDynaRegCfg;

        if (pstDynaRegCfg->bResh)
        {
            for (j = 0; j < pstRegCfgInfo->stAlgRegCfg[i].stGeRegCfg.u8ChnNum; j++)
            {
                isp_ge_ct_th1_write(ViPipe, i, j, pstDynaRegCfg->au16GeCtTh1[j]);
                isp_ge_ct_th3_write(ViPipe, i, j, pstDynaRegCfg->au16GeCtTh3[j]);
            }

            isp_ge_strength_write(ViPipe, i, pstDynaRegCfg->u16GeStrength);
            pstDynaRegCfg->bResh = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1GeCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_LscRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL  bIsOfflineMode;
    HI_BOOL  bUsrResh   = HI_FALSE;
    HI_BOOL  bIdxResh   = HI_FALSE;
    HI_BOOL  bLutUpdate = HI_FALSE;
    HI_U8    u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_U16   j;
    ISP_LSC_USR_CFG_S       *pstUsrRegCfg    = HI_NULL;
    ISP_LSC_STATIC_CFG_S    *pstStaticRegCfg = HI_NULL;
    ISP_CTX_S               *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1LscCfg)
    {
        isp_lsc_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stLscRegCfg.bLscEn);

        /*static*/
        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stLscRegCfg.stStaticRegCfg;

        if (pstStaticRegCfg->bStaticResh)
        {
            isp_lsc_numh_write(ViPipe, i, pstStaticRegCfg->u8WinNumH);
            isp_lsc_numv_write(ViPipe, i, pstStaticRegCfg->u8WinNumV);

            isp_bnr_lsc_numh_write(ViPipe, i, pstStaticRegCfg->u8WinNumH);
            isp_bnr_lsc_numv_write(ViPipe, i, pstStaticRegCfg->u8WinNumV);

            pstStaticRegCfg->bStaticResh = HI_FALSE;
        }

        /*usr*/
        pstUsrRegCfg   = &pstRegCfgInfo->stAlgRegCfg[i].stLscRegCfg.stUsrRegCfg;

        if (pstUsrRegCfg->bCoefUpdate)
        {
            isp_lsc_mesh_str_write(ViPipe, i, pstUsrRegCfg->u16MeshStr);
            isp_lsc_mesh_weight_write(ViPipe, i, pstUsrRegCfg->u16Weight);
            pstUsrRegCfg->bCoefUpdate = bIsOfflineMode;
        }

        bIdxResh = (isp_lsc_update_index_read(ViPipe, i) != pstUsrRegCfg->u32UpdateIndex);
        bUsrResh = (bIsOfflineMode) ? (pstUsrRegCfg->bLutUpdate & bIdxResh) : (pstUsrRegCfg->bLutUpdate);

        if (bUsrResh)
        {
            isp_lsc_update_index_write(ViPipe, i, pstUsrRegCfg->u32UpdateIndex);
            isp_lsc_mesh_scale_write(ViPipe, i, pstUsrRegCfg->u8MeshScale);
            isp_lsc_width_offset_write(ViPipe, i, pstUsrRegCfg->u16WidthOffset);

            isp_bnr_lsc_meshscale_write(ViPipe, i, pstUsrRegCfg->u8MeshScale);
            isp_bnr_lsc_mesh_weight_write(ViPipe, i, pstUsrRegCfg->u16Weight);
            isp_bnr_lsc_width_offset_write(ViPipe, i, pstUsrRegCfg->u16WidthOffset);

            for (j = 0; j < (HI_ISP_LSC_GRID_ROW - 1) / 2; j++)
            {
                isp_lsc_winY_info_write(ViPipe, i, j, pstUsrRegCfg->au16DeltaY[j], pstUsrRegCfg->au16InvY[j]);
                isp_bnr_lsc_winY_info_write(ViPipe, i, j, pstUsrRegCfg->au16DeltaY[j], pstUsrRegCfg->au16InvY[j]);
            }

            for (j = 0; j < (HI_ISP_LSC_GRID_COL - 1); j++)
            {
                isp_lsc_winX_info_write(ViPipe, i, j, pstUsrRegCfg->au16DeltaX[j], pstUsrRegCfg->au16InvX[j]);
                isp_bnr_lsc_winX_info_write(ViPipe, i, j, pstUsrRegCfg->au16DeltaX[j], pstUsrRegCfg->au16InvX[j]);
            }

            if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                isp_lsc_r_gain_waddr_write(ViPipe, i, 0);
                isp_bnr_lsc_rgain_waddr_write(ViPipe, i, 0);

                for (j = 0; j < HI_ISP_LSC_GRID_POINTS; j++)
                {
                    isp_lsc_r_gain_wdata_write(ViPipe, i, pstUsrRegCfg->au32RGain[j]);
                    isp_bnr_lsc_rgain_wdata_write(ViPipe, i, pstUsrRegCfg->au32RGain[j]);
                }

                isp_lsc_gr_gain_waddr_write(ViPipe, i, 0);
                isp_bnr_lsc_grgain_waddr_write(ViPipe, i, 0);

                for (j = 0; j < HI_ISP_LSC_GRID_POINTS; j++)
                {
                    isp_lsc_gr_gain_wdata_write(ViPipe, i, pstUsrRegCfg->au32GrGain[j]);
                    isp_bnr_lsc_grgain_wdata_write(ViPipe, i, pstUsrRegCfg->au32GrGain[j]);
                }

                isp_lsc_gb_gain_waddr_write(ViPipe, i, 0);
                isp_bnr_lsc_gbgain_waddr_write(ViPipe, i, 0);

                for (j = 0; j < HI_ISP_LSC_GRID_POINTS; j++)
                {
                    isp_lsc_gb_gain_wdata_write(ViPipe, i, pstUsrRegCfg->au32GbGain[j]);
                    isp_bnr_lsc_gbgain_wdata_write(ViPipe, i, pstUsrRegCfg->au32GbGain[j]);
                }

                isp_lsc_b_gain_waddr_write(ViPipe, i, 0);
                isp_bnr_lsc_bgain_waddr_write(ViPipe, i, 0);

                for (j = 0; j < HI_ISP_LSC_GRID_POINTS; j++)
                {
                    isp_lsc_b_gain_wdata_write(ViPipe, i, pstUsrRegCfg->au32BGain[j]);
                    isp_bnr_lsc_bgain_wdata_write(ViPipe, i, pstUsrRegCfg->au32BGain[j]);
                }
            }
            else
            {
                isp_lsc_rgain_write(ViPipe,  i, pstUsrRegCfg->au32RGain);
                isp_lsc_grgain_write(ViPipe, i, pstUsrRegCfg->au32GrGain);
                isp_lsc_gbgain_write(ViPipe, i, pstUsrRegCfg->au32GbGain);
                isp_lsc_bgain_write(ViPipe,  i, pstUsrRegCfg->au32BGain);

                isp_bnr_lsc_rgain_write(ViPipe,  i, pstUsrRegCfg->au32RGain);
                isp_bnr_lsc_grgain_write(ViPipe, i, pstUsrRegCfg->au32GrGain);
                isp_bnr_lsc_gbgain_write(ViPipe, i, pstUsrRegCfg->au32GbGain);
                isp_bnr_lsc_bgain_write(ViPipe,  i, pstUsrRegCfg->au32GbGain);
            }

            //if (u8BlkNum == (i + 1))
            //{
            //for (j = 0; j < u8BlkNum; j++)
            //{
            //isp_lsc_lut_update_write(ViPipe, j, HI_TRUE);
            //isp_bnr_lsc_lut_update_write(ViPipe, j, HI_TRUE);
            //}
            //}
            bLutUpdate = HI_TRUE;
            pstUsrRegCfg->bLutUpdate = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1LscCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bLscLutUpdate    = bLutUpdate;
    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bBnrLscLutUpdate = bLutUpdate;

    return HI_SUCCESS;
}

static HI_S32 ISP_RLscRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL  bIsOfflineMode;
    HI_BOOL  bUsrResh   = HI_FALSE;
    HI_BOOL  bIdxResh   = HI_FALSE;
    HI_BOOL  bLutUpdate = HI_FALSE;
    HI_U8    u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_U16   j;
    HI_U32   u32LUT0Data, u32LUT1Data;
    ISP_RLSC_USR_CFG_S      *pstUsrRegCfg    = HI_NULL;
    ISP_RLSC_STATIC_CFG_S   *pstStaticRegCfg = HI_NULL;
    ISP_CTX_S               *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1RLscCfg)
    {
        isp_rlsc_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stRLscRegCfg.bRLscEn);

        /*static*/
        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stRLscRegCfg.stStaticRegCfg;

        if (pstStaticRegCfg->bStaticResh)
        {
            isp_rlsc_nodenum_write(ViPipe, i, pstStaticRegCfg->u16NodeNum);

            pstStaticRegCfg->bStaticResh = HI_FALSE;
        }

        /*usr*/
        pstUsrRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stRLscRegCfg.stUsrRegCfg;

        isp_rlsc_rx_write(ViPipe, i, pstUsrRegCfg->u16CenterRX);
        isp_rlsc_ry_write(ViPipe, i, pstUsrRegCfg->u16CenterRY);
        isp_rlsc_grx_write(ViPipe, i, pstUsrRegCfg->u16CenterGrX);
        isp_rlsc_gry_write(ViPipe, i, pstUsrRegCfg->u16CenterGrY);
        isp_rlsc_gbx_write(ViPipe, i, pstUsrRegCfg->u16CenterGbX);
        isp_rlsc_gby_write(ViPipe, i, pstUsrRegCfg->u16CenterGbY);
        isp_rlsc_bx_write(ViPipe, i, pstUsrRegCfg->u16CenterBX);
        isp_rlsc_by_write(ViPipe, i, pstUsrRegCfg->u16CenterBY);

        isp_rlsc_offsetcenterr_write(ViPipe, i, pstUsrRegCfg->u16OffCenterR);
        isp_rlsc_offsetcentergr_write(ViPipe, i, pstUsrRegCfg->u16OffCenterGr);
        isp_rlsc_offsetcentergb_write(ViPipe, i, pstUsrRegCfg->u16OffCenterGb);
        isp_rlsc_offsetcenterb_write(ViPipe, i, pstUsrRegCfg->u16OffCenterB);

        isp_rlsc_validradius_write(ViPipe, i, pstUsrRegCfg->u32ValidRadius);

        if (pstUsrRegCfg->bCoefUpdate)
        {
            isp_rlsc_lsc_en_write(ViPipe, i, pstUsrRegCfg->bRLscFuncEn);
            isp_rlsc_crop_en_write(ViPipe, i, pstUsrRegCfg->bRadialCropEn);
            isp_rlsc_gainstr_write(ViPipe, i, pstUsrRegCfg->u16GainStr);

            pstUsrRegCfg->bCoefUpdate = bIsOfflineMode;
        }

        bIdxResh = (isp_rlsc_update_index_read(ViPipe, i) != pstUsrRegCfg->u32UpdateIndex);
        bUsrResh = (bIsOfflineMode) ? (pstUsrRegCfg->bLutUpdate & bIdxResh) : (pstUsrRegCfg->bLutUpdate);

        if (bUsrResh)
        {
            isp_rlsc_update_index_write(ViPipe, i, pstUsrRegCfg->u32UpdateIndex);
            isp_rlsc_gainscale_write(ViPipe, i, pstUsrRegCfg->u8GainScale);
            isp_rlsc_widthoffset_write(ViPipe, i, pstUsrRegCfg->u16WidthOffset);

            if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {

                isp_rlsc_lut0_waddr_write(ViPipe, i, 0);

                for (j = 0; j < HI_ISP_RLSC_POINTS + 1; j++)
                {
                    u32LUT0Data = ((pstUsrRegCfg->au32Lut0Chn1[j] & 0xffff) << 16) + pstUsrRegCfg->au32Lut0Chn0[j];
                    isp_rlsc_lut0_wdata_write(ViPipe, i, u32LUT0Data);
                }

                isp_rlsc_lut1_waddr_write(ViPipe, i, 0);

                for (j = 0; j < HI_ISP_RLSC_POINTS + 1; j++)
                {
                    u32LUT1Data = ((pstUsrRegCfg->au32Lut1Chn3[j] & 0xffff) << 16) + pstUsrRegCfg->au32Lut1Chn2[j];
                    isp_rlsc_lut1_wdata_write(ViPipe, i, u32LUT1Data);
                }
            }
            else
            {
                isp_rlsc_lut0_ch0_write(ViPipe, i, pstUsrRegCfg->au32Lut0Chn0);
                isp_rlsc_lut0_ch1_write(ViPipe, i, pstUsrRegCfg->au32Lut0Chn1);
                isp_rlsc_lut1_ch2_write(ViPipe, i, pstUsrRegCfg->au32Lut1Chn2);
                isp_rlsc_lut1_ch3_write(ViPipe, i, pstUsrRegCfg->au32Lut1Chn3);
            }

            //if (u8BlkNum == (i + 1))
            //{
            //for (j = 0; j < u8BlkNum; j++)
            //{
            //isp_rlsc_lut_update_write(ViPipe, j, HI_TRUE);
            //}
            //}

            bLutUpdate = HI_TRUE;
            pstUsrRegCfg->bLutUpdate = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1RLscCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bRlscLutUpdate = bLutUpdate;

    return HI_SUCCESS;
}

static HI_S32 ISP_GammaRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL  bIsOfflineMode;
    HI_BOOL  bUsrResh   = HI_FALSE;
    HI_BOOL  bIdxResh   = HI_FALSE;
    HI_BOOL  bLutUpdate = HI_FALSE;
    HI_U8    u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_U16   j;
    ISP_GAMMA_USR_CFG_S    *pstUsrRegCfg    = HI_NULL;
    ISP_CTX_S              *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1GammaCfg)
    {
        isp_gamma_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stGammaCfg.bGammaEn);

        pstUsrRegCfg    = &pstRegCfgInfo->stAlgRegCfg[i].stGammaCfg.stUsrRegCfg;

        bIdxResh = (isp_gamma_update_index_read(ViPipe, i) != pstUsrRegCfg->u32UpdateIndex);
        bUsrResh = (bIsOfflineMode) ? (pstUsrRegCfg->bGammaLutUpdateEn & bIdxResh) : (pstUsrRegCfg->bGammaLutUpdateEn);

        //LUT update
        if (bUsrResh)
        {
            isp_gamma_update_index_write(ViPipe, i, pstUsrRegCfg->u32UpdateIndex);

            if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                isp_gamma_lut_waddr_write(ViPipe, i, 0);

                for ( j = 0 ; j < GAMMA_NODE_NUMBER ; j++ )
                {
                    isp_gamma_lut_wdata_write(ViPipe, i, (pstUsrRegCfg->au16GammaLUT[j]) << 2);
                }
            }
            else
            {
                isp_gamma_lut_write(ViPipe, i, pstUsrRegCfg->au16GammaLUT);
            }

            //isp_gamma_lut_update_write(ViPipe, i, HI_TRUE);
            bLutUpdate = HI_TRUE;

            pstUsrRegCfg->bGammaLutUpdateEn = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1GammaCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bGammaLutUpdate = bLutUpdate;

    return HI_SUCCESS;
}
static HI_S32 ISP_CscRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_U8 u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_BOOL  bIsOfflineMode;
    ISP_CSC_DYNA_CFG_S    *pstDynaRegCfg   = HI_NULL;
    ISP_CTX_S             *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if ( pstRegCfgInfo->unKey.bit1CscCfg)
    {
        pstDynaRegCfg   = &pstRegCfgInfo->stAlgRegCfg[i].stCscCfg.stDynaRegCfg;

        /*General*/
        isp_csc_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stCscCfg.bEnable);

        /*Dynamic*/
        if (pstDynaRegCfg->bResh)
        {
            isp_csc_coef00_write(ViPipe, i, pstDynaRegCfg->s16CscCoef00);
            isp_csc_coef01_write(ViPipe, i, pstDynaRegCfg->s16CscCoef01);
            isp_csc_coef02_write(ViPipe, i, pstDynaRegCfg->s16CscCoef02);
            isp_csc_coef10_write(ViPipe, i, pstDynaRegCfg->s16CscCoef10);
            isp_csc_coef11_write(ViPipe, i, pstDynaRegCfg->s16CscCoef11);
            isp_csc_coef12_write(ViPipe, i, pstDynaRegCfg->s16CscCoef12);
            isp_csc_coef20_write(ViPipe, i, pstDynaRegCfg->s16CscCoef20);
            isp_csc_coef21_write(ViPipe, i, pstDynaRegCfg->s16CscCoef21);
            isp_csc_coef22_write(ViPipe, i, pstDynaRegCfg->s16CscCoef22);

            isp_csc_in_dc0_write(ViPipe, i, pstDynaRegCfg->s16CscInDC0); //10bit, left shift 2 bits
            isp_csc_in_dc1_write(ViPipe, i, pstDynaRegCfg->s16CscInDC1); //10bit, left shift 2 bits
            isp_csc_in_dc2_write(ViPipe, i, pstDynaRegCfg->s16CscInDC2); //10bit, left shift 2 bits

            isp_csc_out_dc0_write(ViPipe, i, pstDynaRegCfg->s16CscOutDC0); //10bit, left shift 2 bits
            isp_csc_out_dc1_write(ViPipe, i, pstDynaRegCfg->s16CscOutDC1); //10bit, left shift 2 bits
            isp_csc_out_dc2_write(ViPipe, i, pstDynaRegCfg->s16CscOutDC2); //10bit, left shift 2 bits

            pstDynaRegCfg->bResh = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1CscCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_CaRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL  bIsOfflineMode;
    HI_BOOL  bUsrResh   = HI_FALSE;
    HI_BOOL  bIdxResh   = HI_FALSE;
    HI_BOOL  bLutUpdate = HI_FALSE;
    HI_U8    u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_U16   j;
    ISP_CA_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;
    ISP_CA_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_CA_USR_CFG_S    *pstUsrRegCfg    = HI_NULL;
    ISP_CTX_S           *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1CaCfg)
    {
        isp_ca_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stCaRegCfg.bCaEn);

        /*usr*/
        pstUsrRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stCaRegCfg.stUsrRegCfg;
        bIdxResh = (isp_ca_update_index_read(ViPipe, i) != pstUsrRegCfg->u32UpdateIndex);
        bUsrResh = (bIsOfflineMode) ? (pstUsrRegCfg->bResh & bIdxResh) : (pstUsrRegCfg->bResh);

        if (bUsrResh)
        {
            isp_ca_update_index_write(ViPipe, i, pstUsrRegCfg->u32UpdateIndex);
            isp_ca_cp_en_write(ViPipe, i, pstUsrRegCfg->bCaCpEn);
            isp_ca_lumath_high_write(ViPipe, i, pstUsrRegCfg->u16CaLumaThrHigh);
            isp_ca_lumaratio_high_write(ViPipe, i, pstUsrRegCfg->u16CaLumaRatioHigh);

            //if (pstUsrRegCfg->bCaLutUpdateEn)
            //{
            if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                isp_ca_lut_waddr_write(ViPipe, i, 0);
                for (j = 0; j < HI_ISP_CA_YRATIO_LUT_LENGTH; j++)
                {
                    isp_ca_lut_wdata_write(ViPipe, i, pstUsrRegCfg->au32YRatioLUT[j]);
                }
            }
            else
            {
                isp_ca_lut_write(ViPipe, i, pstUsrRegCfg->au32YRatioLUT);
            }

            //isp_ca_lut_update_write(ViPipe, i, HI_TRUE);
            bLutUpdate = HI_TRUE;
            //}
            pstUsrRegCfg->bResh = bIsOfflineMode;
        }

        /*dynamic*/
        pstDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stCaRegCfg.stDynaRegCfg;

        if (pstDynaRegCfg->bResh)
        {
            isp_ca_isoratio_write(ViPipe, i, pstDynaRegCfg->u16CaISORatio);
            pstDynaRegCfg->bResh = bIsOfflineMode;
        }

        /*static*/
        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stCaRegCfg.stStaticRegCfg;

        if (pstStaticRegCfg->bStaticResh)
        {
            isp_ca_llhcproc_en_write(ViPipe, i, pstStaticRegCfg->bCaLlhcProcEn);
            isp_ca_skinproc_en_write(ViPipe, i, pstStaticRegCfg->bCaSkinProcEn);
            isp_ca_satadj_en_write(ViPipe, i, pstStaticRegCfg->bCaSatuAdjEn);
            isp_ca_lumath_low_write(ViPipe, i, pstStaticRegCfg->u16CaLumaThrLow);
            isp_ca_darkchromath_low_write(ViPipe, i, pstStaticRegCfg->u16CaDarkChromaThrLow);
            isp_ca_darkchromath_high_write(ViPipe, i, pstStaticRegCfg->u16CaDarkChromaThrHigh);
            isp_ca_sdarkchromath_low_write(ViPipe, i, pstStaticRegCfg->u16CaSDarkChromaThrLow);
            isp_ca_sdarkchromath_high_write(ViPipe, i, pstStaticRegCfg->u16CaSDarkChromaThrHigh);
            isp_ca_lumaratio_low_write(ViPipe, i, pstStaticRegCfg->u16CaLumaRatioLow);
            isp_ca_yuv2rgb_coef00_write(ViPipe, i, pstStaticRegCfg->s16CaYuv2RgbCoef00);
            isp_ca_yuv2rgb_coef01_write(ViPipe, i, pstStaticRegCfg->s16CaYuv2RgbCoef01);
            isp_ca_yuv2rgb_coef02_write(ViPipe, i, pstStaticRegCfg->s16CaYuv2RgbCoef02);
            isp_ca_yuv2rgb_coef10_write(ViPipe, i, pstStaticRegCfg->s16CaYuv2RgbCoef10);
            isp_ca_yuv2rgb_coef11_write(ViPipe, i, pstStaticRegCfg->s16CaYuv2RgbCoef11);
            isp_ca_yuv2rgb_coef12_write(ViPipe, i, pstStaticRegCfg->s16CaYuv2RgbCoef12);
            isp_ca_yuv2rgb_coef20_write(ViPipe, i, pstStaticRegCfg->s16CaYuv2RgbCoef20);
            isp_ca_yuv2rgb_coef21_write(ViPipe, i, pstStaticRegCfg->s16CaYuv2RgbCoef21);
            isp_ca_yuv2rgb_coef22_write(ViPipe, i, pstStaticRegCfg->s16CaYuv2RgbCoef22);
            isp_ca_yuv2rgb_indc0_write(ViPipe, i, pstStaticRegCfg->s16CaYuv2RgbInDc0);
            isp_ca_yuv2rgb_indc1_write(ViPipe, i, pstStaticRegCfg->s16CaYuv2RgbInDc1);
            isp_ca_yuv2rgb_indc2_write(ViPipe, i, pstStaticRegCfg->s16CaYuv2RgbInDc2);
            isp_ca_yuv2rgb_outdc0_write(ViPipe, i, pstStaticRegCfg->s16CaYuv2RgbOutDc0);
            isp_ca_yuv2rgb_outdc1_write(ViPipe, i, pstStaticRegCfg->s16CaYuv2RgbOutDc1);
            isp_ca_yuv2rgb_outdc2_write(ViPipe, i, pstStaticRegCfg->s16CaYuv2RgbOutDc2);
            isp_ca_skinlluma_umin_write(ViPipe, i, pstStaticRegCfg->u16CaSkinLowLumaMinU);
            isp_ca_skinlluma_umax_write(ViPipe, i, pstStaticRegCfg->u16CaSkinLowLumaMaxU);
            isp_ca_skinlluma_uymin_write(ViPipe, i, pstStaticRegCfg->u16CaSkinLowLumaMinUy);
            isp_ca_skinlluma_uymax_write(ViPipe, i, pstStaticRegCfg->u16CaSkinLowLumaMaxUy);
            isp_ca_skinhluma_umin_write(ViPipe, i, pstStaticRegCfg->u16CaSkinHighLumaMinU);
            isp_ca_skinhluma_umax_write(ViPipe, i, pstStaticRegCfg->u16CaSkinHighLumaMaxU);
            isp_ca_skinhluma_uymin_write(ViPipe, i, pstStaticRegCfg->u16CaSkinHighLumaMinUy);
            isp_ca_skinhluma_uymax_write(ViPipe, i, pstStaticRegCfg->u16CaSkinHighLumaMaxUy);
            isp_ca_skinlluma_vmin_write(ViPipe, i, pstStaticRegCfg->u16CaSkinLowLumaMinV);
            isp_ca_skinlluma_vmax_write(ViPipe, i, pstStaticRegCfg->u16CaSkinLowLumaMaxV);
            isp_ca_skinlluma_vymin_write(ViPipe, i, pstStaticRegCfg->u16CaSkinLowLumaMinVy);
            isp_ca_skinlluma_vymax_write(ViPipe, i, pstStaticRegCfg->u16CaSkinLowLumaMaxVy);
            isp_ca_skinhluma_vmin_write(ViPipe, i, pstStaticRegCfg->u16CaSkinHighLumaMinV);
            isp_ca_skinhluma_vmax_write(ViPipe, i, pstStaticRegCfg->u16CaSkinHighLumaMaxV);
            isp_ca_skinhluma_vymin_write(ViPipe, i, pstStaticRegCfg->u16CaSkinHighLumaMinVy);
            isp_ca_skinhluma_vymax_write(ViPipe, i, pstStaticRegCfg->u16CaSkinHighLumaMaxVy);
            isp_ca_skin_uvdiff_write(ViPipe, i, pstStaticRegCfg->s16CaSkinUvDiff);
            isp_ca_skinratioth_low_write(ViPipe, i, pstStaticRegCfg->u16CaSkinRatioThrLow);
            isp_ca_skinratioth_mid_write(ViPipe, i, pstStaticRegCfg->u16CaSkinRatioThrMid);
            isp_ca_skinratioth_high_write(ViPipe, i, pstStaticRegCfg->u16CaSkinRatioThrHigh);

            pstStaticRegCfg->bStaticResh = HI_FALSE;
        }

        pstRegCfgInfo->unKey.bit1CaCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bCaLutUpdate = bLutUpdate;

    return HI_SUCCESS;
}

static HI_S32 ISP_McdsRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_U8 u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_BOOL  bIsOfflineMode;
    ISP_MCDS_DYNA_REG_CFG_S    *pstDynaRegCfg   = HI_NULL;
    ISP_MCDS_STATIC_REG_CFG_S  *pstStaticRegCfg = HI_NULL;
    ISP_CTX_S                  *pstIspCtx           = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1McdsCfg)
    {
        isp_mcds_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stMcdsRegCfg.bMCDSen);

        pstDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stMcdsRegCfg.stDynaRegCfg;
        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stMcdsRegCfg.stStaticRegCfg;

        if (pstStaticRegCfg->bStaticResh)
        {
            isp_mcds_hcds_en_write(ViPipe, i, pstStaticRegCfg->bHcdsEn);
            isp_mcds_vcds_en_write(ViPipe, i, pstStaticRegCfg->bVcdsEn);
            isp_mcds_coefh0_write(ViPipe, i, pstStaticRegCfg->s32HCoef0);
            isp_mcds_coefh1_write(ViPipe, i, pstStaticRegCfg->s32HCoef1);
            isp_mcds_coefh2_write(ViPipe, i, pstStaticRegCfg->s32HCoef2);
            isp_mcds_coefh3_write(ViPipe, i, pstStaticRegCfg->s32HCoef3);
            isp_mcds_coefh4_write(ViPipe, i, pstStaticRegCfg->s32HCoef4);
            isp_mcds_coefh5_write(ViPipe, i, pstStaticRegCfg->s32HCoef5);
            isp_mcds_coefh6_write(ViPipe, i, pstStaticRegCfg->s32HCoef6);
            isp_mcds_coefh7_write(ViPipe, i, pstStaticRegCfg->s32HCoef7);
            isp_mcds_coefv0_write(ViPipe, i, pstStaticRegCfg->s32VCoef0);
            isp_mcds_coefv1_write(ViPipe, i, pstStaticRegCfg->s32VCoef1);
            isp_mcds_limit_write(ViPipe, i, pstStaticRegCfg->u16CoringLimit);
            isp_mcds_midf_bldr_write(ViPipe, i, pstStaticRegCfg->u8MidfBldr);
            pstStaticRegCfg->bStaticResh = 0;
        }

        if (pstDynaRegCfg->bDynaResh)
        {
            isp_mcds_midf_en_write(ViPipe, i, pstDynaRegCfg->bMidfEn);
            pstDynaRegCfg->bDynaResh = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1McdsCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_WdrRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL  bIsOfflineMode;
    HI_BOOL  bUsrResh   = HI_FALSE;
    HI_BOOL  bIdxResh   = HI_FALSE;
    HI_BOOL  bLutUpdate = HI_FALSE;
    HI_U8    u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_U16   j;
    ISP_FSWDR_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;
    ISP_FSWDR_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_FSWDR_USR_CFG_S    *pstUsrRegCfg    = HI_NULL;
    ISP_CTX_S              *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1FsWdrCfg)
    {
        isp_wdr_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stWdrRegCfg.bWDREn);

        /*static*/
        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stWdrRegCfg.stStaticRegCfg;

        if (pstStaticRegCfg->bResh)
        {
            isp_wdr_bitdepth_invalid_write(ViPipe, i, pstStaticRegCfg->u8BitDepthInvalid);
            isp_wdr_grayscale_mode_write(ViPipe, i, pstStaticRegCfg->bGrayScaleMode);
            isp_wdr_bsaveblc_write(ViPipe, i, pstStaticRegCfg->bSaveBLC);
            isp_wdr_mask_similar_thr_write(ViPipe, i, pstStaticRegCfg->u8MaskSimilarThr);
            isp_wdr_mask_similar_cnt_write(ViPipe, i, pstStaticRegCfg->u8MaskSimilarCnt);
            isp_wdr_expovalue0_write(ViPipe, i, pstStaticRegCfg->au16ExpoValue[0]);
            isp_wdr_expovalue1_write(ViPipe, i, pstStaticRegCfg->au16ExpoValue[1]);
            isp_wdr_expovalue2_write(ViPipe, i, pstStaticRegCfg->au16ExpoValue[2]);
            isp_wdr_expovalue3_write(ViPipe, i, pstStaticRegCfg->au16ExpoValue[3]);
            isp_wdr_maxratio_write(ViPipe, i, pstStaticRegCfg->u32MaxRatio);
            isp_wdr_dftwgt_fl_write(ViPipe, i, pstStaticRegCfg->u16dftWgtFL);
            isp_wdr_blc_comp0_write(ViPipe, i, pstStaticRegCfg->au32BlcComp[0]);
            isp_wdr_blc_comp1_write(ViPipe, i, pstStaticRegCfg->au32BlcComp[1]);
            isp_wdr_blc_comp2_write(ViPipe, i, pstStaticRegCfg->au32BlcComp[2]);
            isp_wdr_exporratio0_write(ViPipe, i, pstStaticRegCfg->au16ExpoRRatio[0]);
            isp_wdr_exporratio1_write(ViPipe, i, pstStaticRegCfg->au16ExpoRRatio[1]);
            isp_wdr_exporratio2_write(ViPipe, i, pstStaticRegCfg->au16ExpoRRatio[2]);

            isp_wdr_bldrlhfidx_write(ViPipe, i, pstStaticRegCfg->u8bldrLHFIdx);
            isp_wdr_fusion_rlow_thr_write(ViPipe, i, pstStaticRegCfg->u16FusionRLowThr);
            isp_wdr_fusion_rhig_thr_write(ViPipe, i, pstStaticRegCfg->u16FusionRHigThr);
            isp_wdr_bnr_nosmode_write(ViPipe, i, pstStaticRegCfg->bNrNosMode);
            isp_wdr_gsigma_gain2_write(ViPipe, i, pstStaticRegCfg->u8Gsigma_gain2);
            isp_wdr_gsigma_gain3_write(ViPipe, i, pstStaticRegCfg->u8Gsigma_gain3);
            isp_wdr_csigma_gain2_write(ViPipe, i, pstStaticRegCfg->u8Csigma_gain2);
            isp_wdr_csigma_gain3_write(ViPipe, i, pstStaticRegCfg->u8Csigma_gain3);
            isp_wdr_saturate_thr_write(ViPipe, i, pstStaticRegCfg->u16SaturateThr);
            pstStaticRegCfg->bResh = HI_FALSE;
        }

        /*usr*/
        pstUsrRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stWdrRegCfg.stUsrRegCfg;
        bIdxResh = (isp_wdr_update_index_read(ViPipe, i) != pstUsrRegCfg->u32UpdateIndex);
        bUsrResh = (bIsOfflineMode) ? (pstUsrRegCfg->bResh & bIdxResh) : (pstUsrRegCfg->bResh);

        if (bUsrResh)
        {
            isp_wdr_update_index_write(ViPipe, i, pstUsrRegCfg->u32UpdateIndex);
            isp_wdr_bnr_write(ViPipe, i, pstUsrRegCfg->bWDRBnr);
            isp_wdr_shortexpo_chk_write(ViPipe, i, pstUsrRegCfg->bShortExpoChk);
            isp_wdr_fusionmode_write(ViPipe, i, pstUsrRegCfg->bFusionMode);
            isp_wdr_fusion_bnr_write(ViPipe, i, pstUsrRegCfg->bFusionBnr);
            isp_wdr_mdtlbld_write(ViPipe, i, pstUsrRegCfg->u8MdtLBld);
            isp_wdr_mdt_full_thr_write(ViPipe, i, pstUsrRegCfg->u8MdtFullThr);
            isp_wdr_bnr_fullmdt_thr_write(ViPipe, i, pstUsrRegCfg->u8BnrFullMdtThr);
            isp_wdr_2dnr_weightg0_write(ViPipe, i, pstUsrRegCfg->u8BnrWgtG0);
            isp_wdr_2dnr_weightg1_write(ViPipe, i, pstUsrRegCfg->u8BnrWgtG1);
            isp_wdr_2dnr_weightg2_write(ViPipe, i, pstUsrRegCfg->u8BnrWgtG2);
            isp_wdr_2dnr_weightc0_write(ViPipe, i, pstUsrRegCfg->u8BnrWgtC0);
            isp_wdr_2dnr_weightc1_write(ViPipe, i, pstUsrRegCfg->u8BnrWgtC1);
            isp_wdr_2dnr_weightc2_write(ViPipe, i, pstUsrRegCfg->u8BnrWgtC2);
            isp_wdr_fullmdt_sigwgt_write(ViPipe, i, pstUsrRegCfg->u8FullMotSigWgt);
            isp_wdr_mdt_still_thr_write(ViPipe, i, pstUsrRegCfg->u8MdtStillThr);
            isp_wdr_fullmdt_sigwgt_write(ViPipe, i, pstUsrRegCfg->u8FullMotSigWgt);
            isp_wdr_fusion_f0_thr_write(ViPipe, i, pstUsrRegCfg->au16FusionThr[0]);
            isp_wdr_fusion_f1_thr_write(ViPipe, i, pstUsrRegCfg->au16FusionThr[1]);
            isp_wdr_fusion_f2_thr_write(ViPipe, i, pstUsrRegCfg->au16FusionThr[2]);
            isp_wdr_fusion_f3_thr_write(ViPipe, i, pstUsrRegCfg->au16FusionThr[3]);
            isp_wdr_gsigma_gain1_write(ViPipe, i, pstUsrRegCfg->u8Gsigma_gain1);
            isp_wdr_csigma_gain1_write(ViPipe, i, pstUsrRegCfg->u8Csigma_gain1);

            pstUsrRegCfg->bResh = bIsOfflineMode;
        }

        /*dynamic*/
        pstDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stWdrRegCfg.stDynaRegCfg;

        if (pstDynaRegCfg->bResh)
        {
            //isp_wdr_mdt_en_write(ViPipe, i, pstDynaRegCfg->bWDRMdtEn);
            isp_wdr_sqrt_again_g_write(ViPipe, i, pstDynaRegCfg->u8SqrtAgainG);
            isp_wdr_sqrt_dgain_g_write(ViPipe, i, pstDynaRegCfg->u8SqrtDgainG);
            isp_wdr_mdt_nosfloor_write(ViPipe, i, pstDynaRegCfg->u8MdtNosFloor);
            isp_wdr_mdthr_low_gain_write(ViPipe, i, pstDynaRegCfg->u8MdThrLowGain);
            isp_wdr_mdthr_hig_gain_write(ViPipe, i, pstDynaRegCfg->u8MdThrHigGain);
            isp_wdr_tnosfloor_write(ViPipe, i, pstDynaRegCfg->u16TNosFloor);
            //isp_wdr_short_thr_write(ViPipe, i, pstDynaRegCfg->u16ShortThr);
            //isp_wdr_long_thr_write(ViPipe, i, pstDynaRegCfg->u16LongThr);
            isp_wdr_f0_still_thr_write(ViPipe, i, pstDynaRegCfg->au16StillThr[0]);
            isp_wdr_f1_still_thr_write(ViPipe, i, pstDynaRegCfg->au16StillThr[1]);
            isp_wdr_f2_still_thr_write(ViPipe, i, pstDynaRegCfg->au16StillThr[2]);
            isp_wdr_nosfloor_r_write(ViPipe, i, pstDynaRegCfg->u16NosFloorR);
            isp_wdr_nosfloor_g_write(ViPipe, i, pstDynaRegCfg->u16NosFloorG);
            isp_wdr_nosfloor_b_write(ViPipe, i, pstDynaRegCfg->u16NosFloorB);

            isp_wdr_modelcoef_rgain_write(ViPipe, i, pstDynaRegCfg->u16ModelCoefRgain);
            isp_wdr_modelcoef_ggain_write(ViPipe, i, pstDynaRegCfg->u16ModelCoefGgain);
            isp_wdr_modelcoef_bgain_write(ViPipe, i, pstDynaRegCfg->u16ModelCoefBgain);

            isp_wdr_erosion_en_write(ViPipe, i, pstDynaRegCfg->bErosionEn);

            isp_wdr_nos_np_thr_write(ViPipe, i, pstDynaRegCfg->u16NosNpThr);

            isp_flicker_en_write(ViPipe, i, pstDynaRegCfg->bFlickEn);
            isp_bcom_en_write(ViPipe, i, pstDynaRegCfg->bBcomEn);
            isp_bdec_en_write(ViPipe, i, pstDynaRegCfg->bBdecEn);
            isp_bcom_alpha_write(ViPipe, i, pstDynaRegCfg->u8bcom_alpha);
            isp_bdec_alpha_write(ViPipe, i, pstDynaRegCfg->u8bdec_alpha);
            isp_wdr_mergeframe_write(ViPipe, i, pstDynaRegCfg->u8FrmMerge);

            if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                isp_wdr_noslut129x8_waddr_write(ViPipe, i, 0);

                for (j = 0; j < NLUT_LENGTH; j++)
                {
                    isp_wdr_noslut129x8_wdata_write(ViPipe, i, pstDynaRegCfg->as32BnrNosMDTLut[j]);
                }

                //isp_wdr_129x8_lut_update_write(ViPipe, i, pstDynaRegCfg->bUpdateNosLut);
                bLutUpdate = pstDynaRegCfg->bUpdateNosLut;
            }
            else
            {
                isp_wdr_noslut129x8_write(ViPipe, i, pstDynaRegCfg->as32BnrNosMDTLut);
                //isp_wdr_129x8_lut_update_write(ViPipe, i, HI_TRUE);
                bLutUpdate = HI_TRUE;
            }

            pstDynaRegCfg->bResh = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1FsWdrCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bWdrLutUpdate = bLutUpdate;

    return HI_SUCCESS;
}

static HI_S32 ISP_DrcRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL  bIsOfflineMode;
    HI_BOOL  bUsrResh  = HI_FALSE;
    HI_BOOL  bIdxResh  = HI_FALSE;
    HI_U8    u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_U16   j;
    ISP_DRC_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;
    ISP_DRC_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_DRC_USR_CFG_S    *pstUsrRegCfg    = HI_NULL;
    ISP_CTX_S            *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if ( pstRegCfgInfo->unKey.bit1DrcCfg)
    {
        isp_drc_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stDrcRegCfg.bDrcEn);

        if (HI_TRUE == pstRegCfgInfo->stAlgRegCfg[i].stDrcRegCfg.bDrcEn)
        {
            isp_drc_dither_en_write(ViPipe, i, HI_FALSE);
        }
        else
        {
            isp_drc_dither_en_write(ViPipe, i, !(DYNAMIC_RANGE_XDR == pstIspCtx->stHdrAttr.enDynamicRange));
        }

        /*Static*/
        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stDrcRegCfg.stStaticRegCfg;

        if ( pstStaticRegCfg->bStaticResh )
        {
            isp_drc_outbits_write(ViPipe, i, pstStaticRegCfg->u8BitWidthOut);

            isp_drc_wrstat_en_write(ViPipe, i, pstStaticRegCfg->bWrstatEn);
            isp_drc_rdstat_en_write(ViPipe, i, pstStaticRegCfg->bRdstatEn);
            isp_drc_vbiflt_en_write(ViPipe, i, pstStaticRegCfg->bVbifltEn);

            isp_drc_bin_write(ViPipe, i, pstStaticRegCfg->u8BinNumZ);

            /* Static Registers*/
            isp_drc_local_edge_lmt_write(ViPipe, i, pstStaticRegCfg->u8LocalEdgeLmt);

            isp_drc_r_wgt_write(ViPipe, i, pstStaticRegCfg->u8RWgt);
            isp_drc_g_wgt_write(ViPipe, i, pstStaticRegCfg->u8GWgt);
            isp_drc_b_wgt_write(ViPipe, i, pstStaticRegCfg->u8BWgt);

            isp_drc_cc_lin_pow_write(ViPipe, i, pstStaticRegCfg->u8ColorControlMode);
            isp_drc_cc_lut_ctrl_write(ViPipe, i, pstStaticRegCfg->u8ColorControlLUTCtrl);
            isp_drc_cc_global_corr_write(ViPipe, i, pstStaticRegCfg->u16GlobalColorCorr);

            isp_drc_wgt_box_tri_sel_write(ViPipe, i, pstStaticRegCfg->bWgtBoxTriSel);
            isp_drc_color_corr_en_write(ViPipe, i, pstStaticRegCfg->bColorCorrEnable);
            isp_drc_detail_boost_en_write(ViPipe, i, pstStaticRegCfg->bDetailBoostEnable);
            isp_drc_pdw_sum_en_write(ViPipe, i, pstStaticRegCfg->bPdwSumEnable);

            //Purple Fringe Detection & Reduction
            isp_drc_rg_ctr_write(ViPipe, i, pstStaticRegCfg->u8PFRRGCtr);
            isp_drc_rg_wid_write(ViPipe, i, pstStaticRegCfg->u8PFRRGWid);
            isp_drc_rg_slo_write(ViPipe, i, pstStaticRegCfg->u8PFRRGSlo);

            isp_drc_bg_thr_write(ViPipe, i, pstStaticRegCfg->u8PFRBGThr);
            isp_drc_bg_slo_write(ViPipe, i, pstStaticRegCfg->u8PFRBGSlo);

            //ISP-FE DRCS
            isp_fe_drcs_en_write(0, pstStaticRegCfg->bDrcsEn);
            isp_drcs_vbiflt_en_write(0, pstStaticRegCfg->bDrcsVbiFltEn);
            isp_drcs_wrstat_en_write(0, pstStaticRegCfg->bDrcsWrtStatEn);
            isp_drcs_bin_write(0, pstStaticRegCfg->u8BinNumZ);

            isp_drcs_r_wgt_write(0, pstStaticRegCfg->u8RWgt);
            isp_drcs_g_wgt_write(0, pstStaticRegCfg->u8GWgt);
            isp_drcs_b_wgt_write(0, pstStaticRegCfg->u8BWgt);
            isp_drcs_wgt_box_tri_sel_write(0, pstStaticRegCfg->bWgtBoxTriSel);

            pstStaticRegCfg->bStaticResh = HI_FALSE;
        }

        /*User*/
        pstUsrRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stDrcRegCfg.stUsrRegCfg;
        bIdxResh = (isp_drc_update_index_read(ViPipe, i) != pstUsrRegCfg->u32UpdateIndex);
        bUsrResh = (bIsOfflineMode) ? (pstUsrRegCfg->bUsrResh & bIdxResh) : (pstUsrRegCfg->bUsrResh);

        if (bUsrResh)
        {
            isp_drc_update_index_write(ViPipe, i, pstUsrRegCfg->u32UpdateIndex);

            isp_drc_sft1_y_write(ViPipe, i, pstUsrRegCfg->u8YSFT1);
            isp_drc_val1_y_write(ViPipe, i, pstUsrRegCfg->u8YVAL1);
            isp_drc_sft2_y_write(ViPipe, i, pstUsrRegCfg->u8YSFT2);
            isp_drc_val2_y_write(ViPipe, i, pstUsrRegCfg->u8YVAL2);

            isp_drc_sft1_c_write(ViPipe, i, pstUsrRegCfg->u8CSFT1);
            isp_drc_val1_c_write(ViPipe, i, pstUsrRegCfg->u8CVAL1);
            isp_drc_sft2_c_write(ViPipe, i, pstUsrRegCfg->u8CSFT2);
            isp_drc_val2_c_write(ViPipe, i, pstUsrRegCfg->u8CVAL2);

            isp_drc_sft_write(ViPipe, i, pstUsrRegCfg->u8SFT);
            isp_drc_val_write(ViPipe, i, pstUsrRegCfg->u8VAL);

            isp_drc_var_spa_coarse_write(ViPipe, i, pstUsrRegCfg->u8VarSpaCoarse);
            isp_drc_var_spa_medium_write(ViPipe, i, pstUsrRegCfg->u8VarSpaMedium);
            isp_drc_var_spa_fine_write(ViPipe, i, pstUsrRegCfg->u8VarSpaFine);

            isp_drc_var_rng_coarse_write(ViPipe, i, pstUsrRegCfg->u8VarRngCoarse);
            isp_drc_var_rng_medium_write(ViPipe, i, pstUsrRegCfg->u8VarRngMedium);
            isp_drc_var_rng_fine_write(ViPipe, i, pstUsrRegCfg->u8VarRngFine);

            isp_drc_grad_rev_shift_write(ViPipe, i, pstUsrRegCfg->u8GradShift);
            isp_drc_grad_rev_slope_write(ViPipe, i, pstUsrRegCfg->u8GradSlope);
            isp_drc_grad_rev_max_write(ViPipe, i, pstUsrRegCfg->u8GradMax);
            isp_drc_grad_rev_thres_write(ViPipe, i, pstUsrRegCfg->u8GradThr);

            isp_drc_high_slo_write(ViPipe, i, pstUsrRegCfg->u8PFRHighSlo);
            isp_drc_low_slo_write(ViPipe, i, pstUsrRegCfg->u8PFRLowSlo );
            isp_drc_low_thr_write(ViPipe, i, pstUsrRegCfg->u8PFRLowThr );

            isp_drc_gain_clip_knee_write(ViPipe, i, pstUsrRegCfg->u8GainClipKnee);
            isp_drc_gain_clip_step_write(ViPipe, i, pstUsrRegCfg->u8GainClipStep);

            isp_drc_mixing_coring_write(ViPipe, i, pstUsrRegCfg->u8MixingCoring);
            isp_drc_dark_min_write(ViPipe, i, pstUsrRegCfg->u8MixingDarkMin);
            isp_drc_dark_max_write(ViPipe, i, pstUsrRegCfg->u8MixingDarkMax);
            isp_drc_dark_thr_write(ViPipe, i, pstUsrRegCfg->u8MixingDarkThr);
            isp_drc_dark_slo_write(ViPipe, i, pstUsrRegCfg->s8MixingDarkSlo);

            isp_drc_bright_min_write(ViPipe, i, pstUsrRegCfg->u8MixingBrightMin);
            isp_drc_bright_max_write(ViPipe, i, pstUsrRegCfg->u8MixingBrightMax);
            isp_drc_bright_thr_write(ViPipe, i, pstUsrRegCfg->u8MixingBrightThr);
            isp_drc_bright_slo_write(ViPipe, i, pstUsrRegCfg->s8MixingBrightSlo);

            isp_drc_detail_coring_write(ViPipe, i, pstUsrRegCfg->u8DetailCoring);
            isp_drc_dark_step_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkStep);
            isp_drc_bright_step_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightStep);

            isp_drc_detail_dark_min_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkMin);
            isp_drc_detail_dark_max_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkMax);
            isp_drc_detail_dark_thr_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkThr);
            isp_drc_detail_dark_slo_write(ViPipe, i, pstUsrRegCfg->s8DetailDarkSlo);

            isp_drc_detail_bright_min_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightMin);
            isp_drc_detail_bright_max_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightMax);
            isp_drc_detail_bright_thr_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightThr);
            isp_drc_detail_bright_slo_write(ViPipe, i, pstUsrRegCfg->s8DetailBrightSlo);

            //dark & bright curve write
            isp_drc_detail_dark_curve0_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkCurve[0]);
            isp_drc_detail_dark_curve1_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkCurve[1]);
            isp_drc_detail_dark_curve2_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkCurve[2]);
            isp_drc_detail_dark_curve3_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkCurve[3]);
            isp_drc_detail_dark_curve4_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkCurve[4]);
            isp_drc_detail_dark_curve5_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkCurve[5]);
            isp_drc_detail_dark_curve6_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkCurve[6]);
            isp_drc_detail_dark_curve7_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkCurve[7]);
            isp_drc_detail_dark_curve8_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkCurve[8]);
            isp_drc_detail_dark_curve9_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkCurve[9]);
            isp_drc_detail_dark_curve10_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkCurve[10]);
            isp_drc_detail_dark_curve11_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkCurve[11]);
            isp_drc_detail_dark_curve12_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkCurve[12]);
            isp_drc_detail_dark_curve13_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkCurve[13]);
            isp_drc_detail_dark_curve14_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkCurve[14]);
            isp_drc_detail_dark_curve15_write(ViPipe, i, pstUsrRegCfg->u8DetailDarkCurve[15]);

            isp_drc_detail_bright_curve0_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightCurve[0]);
            isp_drc_detail_bright_curve1_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightCurve[1]);
            isp_drc_detail_bright_curve2_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightCurve[2]);
            isp_drc_detail_bright_curve3_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightCurve[3]);
            isp_drc_detail_bright_curve4_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightCurve[4]);
            isp_drc_detail_bright_curve5_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightCurve[5]);
            isp_drc_detail_bright_curve6_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightCurve[6]);
            isp_drc_detail_bright_curve7_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightCurve[7]);
            isp_drc_detail_bright_curve8_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightCurve[8]);
            isp_drc_detail_bright_curve9_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightCurve[9]);
            isp_drc_detail_bright_curve10_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightCurve[10]);
            isp_drc_detail_bright_curve11_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightCurve[11]);
            isp_drc_detail_bright_curve12_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightCurve[12]);
            isp_drc_detail_bright_curve13_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightCurve[13]);
            isp_drc_detail_bright_curve14_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightCurve[14]);
            isp_drc_detail_bright_curve15_write(ViPipe, i, pstUsrRegCfg->u8DetailBrightCurve[15]);


            isp_drc_cubic_sel_write(ViPipe, i, pstUsrRegCfg->bCubicCurveSel);

            isp_drc_cubic_thresx01_write(ViPipe, i, pstUsrRegCfg->u16CubicThres01);
            isp_drc_cubic_thresx10_write(ViPipe, i, pstUsrRegCfg->u16CubicThres10);
            isp_drc_cubic_thresx11_write(ViPipe, i, pstUsrRegCfg->u16CubicThres11);

            isp_drc_cubic_coef00_aexp_write(ViPipe, i, pstUsrRegCfg->u8CubicCoef00AExp);
            isp_drc_cubic_coef00_a_write(ViPipe, i, pstUsrRegCfg->u32CubicCoef00A);
            isp_drc_cubic_coef01_bexp_write(ViPipe, i, pstUsrRegCfg->u8CubicCoef01BExp);
            isp_drc_cubic_coef01_b_write(ViPipe, i, pstUsrRegCfg->u32CubicCoef01B);
            isp_drc_cubic_coef02_cexp_write(ViPipe, i, pstUsrRegCfg->u8CubicCoef02CExp);
            isp_drc_cubic_coef02_c_write(ViPipe, i, pstUsrRegCfg->u32CubicCoef02C);
            isp_drc_cubic_coef03_d_write(ViPipe, i, pstUsrRegCfg->u32CubicCoef03D);

            isp_drc_cubic_coef10_aexp_write(ViPipe, i, pstUsrRegCfg->u8CubicCoef10AExp);
            isp_drc_cubic_coef10_a_write(ViPipe, i, pstUsrRegCfg->u32CubicCoef10A);
            isp_drc_cubic_coef11_bexp_write(ViPipe, i, pstUsrRegCfg->u8CubicCoef11BExp);
            isp_drc_cubic_coef11_b_write(ViPipe, i, pstUsrRegCfg->u32CubicCoef11B);
            isp_drc_cubic_coef12_cexp_write(ViPipe, i, pstUsrRegCfg->u8CubicCoef12CExp);
            isp_drc_cubic_coef12_c_write(ViPipe, i, pstUsrRegCfg->u32CubicCoef12C);
            isp_drc_cubic_coef13_d_write(ViPipe, i, pstUsrRegCfg->u32CubicCoef13D);

            isp_drc_cubic_coef20_aexp_write(ViPipe, i, pstUsrRegCfg->u8CubicCoef20AExp);
            isp_drc_cubic_coef20_a_write(ViPipe, i, pstUsrRegCfg->u32CubicCoef20A);
            isp_drc_cubic_coef21_bexp_write(ViPipe, i, pstUsrRegCfg->u8CubicCoef21BExp);
            isp_drc_cubic_coef21_b_write(ViPipe, i, pstUsrRegCfg->u32CubicCoef21B);
            isp_drc_cubic_coef22_cexp_write(ViPipe, i, pstUsrRegCfg->u8CubicCoef22CExp);
            isp_drc_cubic_coef22_c_write(ViPipe, i, pstUsrRegCfg->u32CubicCoef22C);
            isp_drc_cubic_coef23_d_write(ViPipe, i, pstUsrRegCfg->u32CubicCoef23D);

            isp_drc_cubic_coef30_aexp_write(ViPipe, i, pstUsrRegCfg->u8CubicCoef30AExp);
            isp_drc_cubic_coef30_a_write(ViPipe, i, pstUsrRegCfg->u32CubicCoef30A);
            isp_drc_cubic_coef31_bexp_write(ViPipe, i, pstUsrRegCfg->u8CubicCoef31BExp);
            isp_drc_cubic_coef31_b_write(ViPipe, i, pstUsrRegCfg->u32CubicCoef31B);
            isp_drc_cubic_coef32_cexp_write(ViPipe, i, pstUsrRegCfg->u8CubicCoef32CExp);
            isp_drc_cubic_coef32_c_write(ViPipe, i, pstUsrRegCfg->u32CubicCoef32C);
            isp_drc_cubic_coef33_d_write(ViPipe, i, pstUsrRegCfg->u32CubicCoef33D);

            /* Spatial/range filtering coefficients */
            isp_drc_flt_spa_fine_write(ViPipe, i, pstUsrRegCfg->u8FltSpaFine);
            isp_drc_flt_spa_medium_write(ViPipe, i, pstUsrRegCfg->u8FltSpaMedium);
            isp_drc_flt_spa_coarse_write(ViPipe, i, pstUsrRegCfg->u8FltSpaCoarse);
            isp_drc_flt_rng_fine_write(ViPipe, i, pstUsrRegCfg->u8FltRngFine);
            isp_drc_flt_rng_medium_write(ViPipe, i, pstUsrRegCfg->u8FltRngMedium);
            isp_drc_flt_rng_coarse_write(ViPipe, i, pstUsrRegCfg->u8FltRngCoarse);

            /* Adaptive range filtering parameters */
            isp_drc_fr_ada_max_write(ViPipe, i, pstUsrRegCfg->u8FltRngAdaMax);
            isp_drc_dis_offset_coef_write(ViPipe, i, pstUsrRegCfg->u8DisOffsetCoef);
            isp_drc_thr_coef_low_write(ViPipe, i, pstUsrRegCfg->u8DisThrCoefLow);
            isp_drc_thr_coef_high_write(ViPipe, i, pstUsrRegCfg->u8DisThrCoefHigh);

            isp_drc_detail_sub_factor_write(ViPipe, i, pstUsrRegCfg->s8DetailSubFactor);


            isp_drc_bin_mix_factor_coarse_0_write(ViPipe, i, pstUsrRegCfg->au8BinMixCoarse[0]);
            isp_drc_bin_mix_factor_coarse_1_write(ViPipe, i, pstUsrRegCfg->au8BinMixCoarse[1]);
            isp_drc_bin_mix_factor_coarse_2_write(ViPipe, i, pstUsrRegCfg->au8BinMixCoarse[2]);
            isp_drc_bin_mix_factor_coarse_3_write(ViPipe, i, pstUsrRegCfg->au8BinMixCoarse[3]);
            isp_drc_bin_mix_factor_coarse_4_write(ViPipe, i, pstUsrRegCfg->au8BinMixCoarse[4]);
            isp_drc_bin_mix_factor_coarse_5_write(ViPipe, i, pstUsrRegCfg->au8BinMixCoarse[5]);
            isp_drc_bin_mix_factor_coarse_6_write(ViPipe, i, pstUsrRegCfg->au8BinMixCoarse[6]);
            isp_drc_bin_mix_factor_coarse_7_write(ViPipe, i, pstUsrRegCfg->au8BinMixCoarse[7]);

            isp_drc_bin_mix_factor_medium_0_write(ViPipe, i, pstUsrRegCfg->au8BinMixMedium[0]);
            isp_drc_bin_mix_factor_medium_1_write(ViPipe, i, pstUsrRegCfg->au8BinMixMedium[1]);
            isp_drc_bin_mix_factor_medium_2_write(ViPipe, i, pstUsrRegCfg->au8BinMixMedium[2]);
            isp_drc_bin_mix_factor_medium_3_write(ViPipe, i, pstUsrRegCfg->au8BinMixMedium[3]);
            isp_drc_bin_mix_factor_medium_4_write(ViPipe, i, pstUsrRegCfg->au8BinMixMedium[4]);
            isp_drc_bin_mix_factor_medium_5_write(ViPipe, i, pstUsrRegCfg->au8BinMixMedium[5]);
            isp_drc_bin_mix_factor_medium_6_write(ViPipe, i, pstUsrRegCfg->au8BinMixMedium[6]);
            isp_drc_bin_mix_factor_medium_7_write(ViPipe, i, pstUsrRegCfg->au8BinMixMedium[7]);

            // isp_drc_shp_exp_write(ViPipe, i, pstUsrRegCfg->u8ShpExp);
            // isp_drc_shp_log_write(ViPipe, i, pstUsrRegCfg->u8ShpLog);
            // isp_drc_div_denom_log_write(ViPipe, i, pstUsrRegCfg->u32DivDenomLog);
            // isp_drc_denom_exp_write(ViPipe, i, pstUsrRegCfg->u32DenomExp);

            if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                isp_drc_cclut_waddr_write(ViPipe, i, 0);

                for ( j = 0 ; j < HI_ISP_DRC_CC_NODE_NUM; j++ )
                {
                    isp_drc_cclut_wdata_write(ViPipe, i, pstUsrRegCfg->u16CCLUT[j]);
                }
            }
            else
            {
                isp_drc_cclut_write(ViPipe, i, pstUsrRegCfg->u16CCLUT);
            }

            //ISP-FE DRCS
            isp_drcs_var_spa_coarse_write(0, pstUsrRegCfg->u8VarSpaCoarse);
            isp_drcs_var_spa_medium_write(0, pstUsrRegCfg->u8VarSpaMedium);
            isp_drcs_var_spa_fine_write(0, pstUsrRegCfg->u8VarSpaFine);

            isp_drcs_var_rng_coarse_write(0, pstUsrRegCfg->u8VarRngCoarse);
            isp_drcs_var_rng_medium_write(0, pstUsrRegCfg->u8VarRngMedium);
            isp_drcs_var_rng_fine_write(0, pstUsrRegCfg->u8VarRngFine);

            isp_drcs_fr_ada_max_write(0, pstUsrRegCfg->u8FltRngAdaMax);
            isp_drcs_dis_offset_coef_write(0, pstUsrRegCfg->u8DisOffsetCoef);
            isp_drcs_thr_coef_low_write(0, pstUsrRegCfg->u8DisThrCoefLow);
            isp_drcs_thr_coef_high_write(0, pstUsrRegCfg->u8DisThrCoefHigh);

            isp_drcs_bin_mix_factor_coarse_0_write(0, pstUsrRegCfg->au8BinMixCoarse[0]);
            isp_drcs_bin_mix_factor_coarse_1_write(0, pstUsrRegCfg->au8BinMixCoarse[1]);
            isp_drcs_bin_mix_factor_coarse_2_write(0, pstUsrRegCfg->au8BinMixCoarse[2]);
            isp_drcs_bin_mix_factor_coarse_3_write(0, pstUsrRegCfg->au8BinMixCoarse[3]);
            isp_drcs_bin_mix_factor_coarse_4_write(0, pstUsrRegCfg->au8BinMixCoarse[4]);
            isp_drcs_bin_mix_factor_coarse_5_write(0, pstUsrRegCfg->au8BinMixCoarse[5]);
            isp_drcs_bin_mix_factor_coarse_6_write(0, pstUsrRegCfg->au8BinMixCoarse[6]);
            isp_drcs_bin_mix_factor_coarse_7_write(0, pstUsrRegCfg->au8BinMixCoarse[7]);

            isp_drcs_bin_mix_factor_medium_0_write(0, pstUsrRegCfg->au8BinMixMedium[0]);
            isp_drcs_bin_mix_factor_medium_1_write(0, pstUsrRegCfg->au8BinMixMedium[1]);
            isp_drcs_bin_mix_factor_medium_2_write(0, pstUsrRegCfg->au8BinMixMedium[2]);
            isp_drcs_bin_mix_factor_medium_3_write(0, pstUsrRegCfg->au8BinMixMedium[3]);
            isp_drcs_bin_mix_factor_medium_4_write(0, pstUsrRegCfg->au8BinMixMedium[4]);
            isp_drcs_bin_mix_factor_medium_5_write(0, pstUsrRegCfg->au8BinMixMedium[5]);
            isp_drcs_bin_mix_factor_medium_6_write(0, pstUsrRegCfg->au8BinMixMedium[6]);
            isp_drcs_bin_mix_factor_medium_7_write(0, pstUsrRegCfg->au8BinMixMedium[7]);

            isp_drcs_flt_spa_fine_write(0, pstUsrRegCfg->u8FltSpaFine);
            isp_drcs_flt_spa_medium_write(0, pstUsrRegCfg->u8FltSpaMedium);
            isp_drcs_flt_spa_coarse_write(0, pstUsrRegCfg->u8FltSpaCoarse);
            isp_drcs_flt_rng_fine_write(0, pstUsrRegCfg->u8FltRngFine);
            isp_drcs_flt_rng_medium_write(0, pstUsrRegCfg->u8FltRngMedium);
            isp_drcs_flt_rng_coarse_write(0, pstUsrRegCfg->u8FltRngCoarse);

            isp_drcs_shp_exp_write(0, pstUsrRegCfg->u8ShpExp);
            isp_drcs_shp_log_write(0, pstUsrRegCfg->u8ShpLog);
            isp_drcs_div_denom_log_write(0, pstUsrRegCfg->u32DivDenomLog);
            isp_drcs_denom_exp_write(0, pstUsrRegCfg->u32DenomExp);

            pstUsrRegCfg->bUsrResh = bIsOfflineMode;
        }

        /*Dynamic*/
        pstDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stDrcRegCfg.stDynaRegCfg;

        if (pstDynaRegCfg->bDynaResh)
        {
            isp_drc_high_thr_write(ViPipe, i, pstDynaRegCfg->u8PFRHighThr);

            if (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                isp_drc_wrstat_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stDrcRegCfg.bDrcEn);
                isp_drc_rdstat_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stDrcRegCfg.bDrcEn);
            }
            else if (IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                isp_fe_drcs_en_write(0, pstRegCfgInfo->stAlgRegCfg[i].stDrcRegCfg.bDrcEn);
            }

            if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                // LutUpdateMode: 0 - update none, 1 - update Lut0, 2 - update Lut1, 3 - update both
                if (pstDynaRegCfg->u8LutUpdateMode & 0x1) // 1 or 3
                {
                    isp_drc_tmlut0_waddr_write(ViPipe, i, 0);
                    for (j = 0; j < HI_ISP_DRC_TM_NODE_NUM; j++)
                    {
                        isp_drc_tmlut0_wdata_write(ViPipe, i, (((HI_U32)pstDynaRegCfg->au16ToneMappingValue0[j]) << 12) | (pstDynaRegCfg->au16ToneMappingDiff0[j]));
                    }
                }

                if (pstDynaRegCfg->u8LutUpdateMode & 0x2) // 2 or 3
                {
                    isp_drc_tmlut1_waddr_write(ViPipe, i, 0);
                    for (j = 0; j < HI_ISP_DRC_TM_NODE_NUM; j++)
                    {
                        isp_drc_tmlut1_wdata_write(ViPipe, i, (((HI_U32)pstDynaRegCfg->au16ToneMappingValue1[j]) << 12) | (pstDynaRegCfg->au16ToneMappingDiff1[j]));
                    }
                }
            }
            else
            {
                isp_drc_tmlut0_value_write(ViPipe, i, pstDynaRegCfg->au16ToneMappingValue0);
                isp_drc_tmlut0_diff_write(ViPipe, i, pstDynaRegCfg->au16ToneMappingDiff0);
                isp_drc_tmlut1_value_write(ViPipe, i, pstDynaRegCfg->au16ToneMappingValue1);
                isp_drc_tmlut1_diff_write(ViPipe, i, pstDynaRegCfg->au16ToneMappingDiff1);
            }

            isp_drc_strength_write(ViPipe, i, pstDynaRegCfg->u16Strength);
            isp_drc_lut_mix_ctrl_write(ViPipe, i, pstDynaRegCfg->u16LutMixCtrl);

            if (pstDynaRegCfg->bImgSizeChanged)
            {
                isp_drc_big_x_init_write(ViPipe, i, pstDynaRegCfg->u8BigXInit);
                isp_drc_idx_x_init_write(ViPipe, i, pstDynaRegCfg->u8IdxXInit);
                isp_drc_cnt_x_init_write(ViPipe, i, pstDynaRegCfg->u16CntXInit);
                isp_drc_acc_x_init_write(ViPipe, i, pstDynaRegCfg->u16AccXInit);
                isp_drc_blk_wgt_init_write(ViPipe, i, pstDynaRegCfg->u16WgtXInit); // drc v4.0
                isp_drc_total_width_write(ViPipe, i, pstDynaRegCfg->u16TotalWidth - 1);
                isp_drc_stat_width_write(ViPipe, i, pstDynaRegCfg->u16StatWidth - 1);

                isp_drc_hnum_write(ViPipe, i, pstDynaRegCfg->u8BlockHNum);
                isp_drc_vnum_write(ViPipe, i, pstDynaRegCfg->u8BlockVNum);

                isp_drc_zone_hsize_write(ViPipe, i, pstDynaRegCfg->u16BlockHSize - 1);
                isp_drc_zone_vsize_write(ViPipe, i, pstDynaRegCfg->u16BlockVSize - 1);
                isp_drc_chk_x_write(ViPipe, i, pstDynaRegCfg->u8BlockChkX);
                isp_drc_chk_y_write(ViPipe, i, pstDynaRegCfg->u8BlockChkY);

                isp_drc_div_x0_write(ViPipe, i, pstDynaRegCfg->u16DivX0);
                isp_drc_div_x1_write(ViPipe, i, pstDynaRegCfg->u16DivX1);
                isp_drc_div_y0_write(ViPipe, i, pstDynaRegCfg->u16DivY0);
                isp_drc_div_y1_write(ViPipe, i, pstDynaRegCfg->u16DivY1);

                isp_drc_bin_scale_write(ViPipe, i, pstDynaRegCfg->u8BinScale);

                // DRCS configuration
                isp_drcs_total_width_write(0, pstDynaRegCfg->u16DrcsTotalWidth - 1);
                isp_drcs_stat_width_write(0, pstDynaRegCfg->u16DrcsStatWidth - 1);

                isp_drcs_big_x_init_write(0, pstDynaRegCfg->u8DrcsBigXInit);
                isp_drcs_idx_x_init_write(0, pstDynaRegCfg->u8DrcsIdxXInit);
                isp_drcs_cnt_x_init_write(0, pstDynaRegCfg->u16DrcsCntXInit);
                isp_drcs_acc_x_init_write(0, pstDynaRegCfg->u16DrcsAccXInit);
                isp_drcs_blk_wgt_init_write(0, pstDynaRegCfg->u16DrcsWgtXInit); // drc v4.0

                isp_drcs_bin_scale_write(0, pstDynaRegCfg->u8BinScale);
                isp_drcs_vnum_write(0, pstDynaRegCfg->u8BlockVNum);
                isp_drcs_hnum_write(0, pstDynaRegCfg->u8BlockHNum);
                isp_drcs_zone_vsize_write(0, pstDynaRegCfg->u16BlockVSize - 1);
                isp_drcs_zone_hsize_write(0, pstDynaRegCfg->u16BlockHSize - 1);
                isp_drcs_chk_x_write(0, pstDynaRegCfg->u8BlockChkX);
                isp_drcs_chk_y_write(0, pstDynaRegCfg->u8BlockChkY);

                pstDynaRegCfg->bImgSizeChanged = bIsOfflineMode;
            }

            pstDynaRegCfg->bDynaResh = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1DrcCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_DehazeRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL  bDehazeLutUpdate  = HI_FALSE;
    HI_BOOL  bPreStatLutUpdate = HI_FALSE;
    HI_U8    u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_U16   j;
    HI_U16   u16BlkNum;
    HI_BOOL  bIsOfflineMode;
    ISP_DEHAZE_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;
    ISP_DEHAZE_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_CTX_S               *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if ( pstRegCfgInfo->unKey.bit1DehazeCfg)
    {
        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stDehazeRegCfg.stStaticRegCfg;
        pstDynaRegCfg   = &pstRegCfgInfo->stAlgRegCfg[i].stDehazeRegCfg.stDynaRegCfg;

        u16BlkNum = ((pstStaticRegCfg->u8Dchnum + 1) * (pstStaticRegCfg->u8Dcvnum + 1) + 1) / 2;
        isp_dehaze_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stDehazeRegCfg.bDehazeEn);

        /*Static Registers*/

        if (pstStaticRegCfg->bResh)
        {
            isp_dehaze_max_mode_write(ViPipe, i, pstStaticRegCfg->u8MaxMode);
            isp_dehaze_thld_write(ViPipe, i, pstStaticRegCfg->u16DehazeThld);
            isp_dehaze_blthld_write(ViPipe, i, pstStaticRegCfg->u16DehazeBlthld);
            isp_dehaze_neg_mode_write(ViPipe, i, pstStaticRegCfg->u8DehazeNegMode);
            isp_dehaze_block_sum_write(ViPipe, i, pstStaticRegCfg->u16BlockSum);
            isp_dehaze_dc_numh_write(ViPipe, i, pstStaticRegCfg->u8Dchnum);
            isp_dehaze_dc_numv_write(ViPipe, i, pstStaticRegCfg->u8Dcvnum);
            pstStaticRegCfg->bResh = HI_FALSE;
        }

        /*Dynamic Regs*/
        if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
            || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
        {
            if (pstDynaRegCfg->u32LutUpdate)
            {
                isp_dehaze_lut_waddr_write(ViPipe, i, 0);

                for (j = 0; j < 256; j++)
                {
                    isp_dehaze_lut_wdata_write(ViPipe, i, pstDynaRegCfg->u8Lut[j]);
                }
            }

            isp_dehaze_prestat_waddr_write(ViPipe, i, 0);

            for (j = 0; j < u16BlkNum; j++)
            {
                isp_dehaze_prestat_wdata_write(ViPipe, i, pstDynaRegCfg->prestat[j]);
            }

            //isp_dehaze_str_lut_update_write(ViPipe, i, pstDynaRegCfg->u32LutUpdate);
            bDehazeLutUpdate = pstDynaRegCfg->u32LutUpdate;
        }
        else
        {
            isp_dehaze_dehaze_lut_write(ViPipe, i, pstDynaRegCfg->u8Lut);
            isp_dehaze_prestat_write(ViPipe, i, pstDynaRegCfg->prestat);
            bDehazeLutUpdate = HI_TRUE;
            //isp_dehaze_str_lut_update_write(ViPipe, i, HI_TRUE);
        }

        //isp_dehaze_str_lut_update_write(ViPipe, i, pstDynaRegCfg->u32LutUpdate);
        //isp_dehaze_pre_lut_update_write(ViPipe, i, pstDynaRegCfg->u32Update);
        bPreStatLutUpdate = pstDynaRegCfg->u32Update;
        isp_dehaze_air_r_write(ViPipe, i, pstDynaRegCfg->u16AirR);
        isp_dehaze_air_g_write(ViPipe, i, pstDynaRegCfg->u16AirG);
        isp_dehaze_air_b_write(ViPipe, i, pstDynaRegCfg->u16AirB);
        isp_dehaze_gstrth_write(ViPipe, i, pstDynaRegCfg->u8Strength);

        isp_dehaze_block_sizeh_write(ViPipe, i, pstDynaRegCfg->u16Blockhsize);
        isp_dehaze_block_sizev_write(ViPipe, i, pstDynaRegCfg->u16Blockvsize);
        isp_dehaze_phase_x_write(ViPipe, i, pstDynaRegCfg->u32phasex);
        isp_dehaze_phase_y_write(ViPipe, i, pstDynaRegCfg->u32phasey);

        pstRegCfgInfo->unKey.bit1DehazeCfg  = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bDehazeLutUpdate        = bDehazeLutUpdate;
    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bDehazePreStatLutUpdate = bPreStatLutUpdate;

    return HI_SUCCESS;
}

static HI_S32 ISP_BayerNrRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_U16   j;
    HI_U8    u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_BOOL  bIsOfflineMode;
    HI_BOOL  bLutUpdate = HI_FALSE;
    ISP_BAYERNR_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;
    ISP_BAYERNR_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_BAYERNR_USR_CFG_S    *pstUsrRegCfg    = HI_NULL;
    ISP_CTX_S                *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1BayernrCfg)
    {
        isp_bnr_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stBnrRegCfg.bBnrEnable);

        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stBnrRegCfg.stStaticRegCfg;
        pstDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stBnrRegCfg.stDynaRegCfg;
        pstUsrRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stBnrRegCfg.stUsrRegCfg;

        if (pstStaticRegCfg->bResh)         /*satic*/
        {
            isp_bnr_de_enable_write(ViPipe, i, pstStaticRegCfg->bBnrDetailEnhanceEn);
            isp_bnr_skip_enable_write(ViPipe, i, pstStaticRegCfg->bSkipEnable);
            isp_bnr_skip_lev1_enable_write(ViPipe, i, pstStaticRegCfg->bSkipLevel1Enable);
            isp_bnr_skip_lev2_enable_write(ViPipe, i, pstStaticRegCfg->bSkipLevel2Enable);
            isp_bnr_skip_lev3_enable_write(ViPipe, i, pstStaticRegCfg->bSkipLevel3Enable);
            isp_bnr_jnlm_sel_write(ViPipe, i, pstStaticRegCfg->u8JnlmSel);
            isp_bnr_de_posclip_write(ViPipe, i, pstStaticRegCfg->u8BnrDePosClip);
            isp_bnr_de_negclip_write(ViPipe, i, pstStaticRegCfg->u8BnrDeNegClip);
            isp_bnr_de_blcvalue_write(ViPipe, i, pstStaticRegCfg->u16BnrDeBlcValue);
            isp_bnr_rlmt_blc_write(ViPipe, i, pstStaticRegCfg->u16RLmtBlc);
            isp_bnr_jnlm_maxwtcoef_write(ViPipe, i, pstStaticRegCfg->u16JnlmMaxWtCoef);
            isp_bnr_wti_midcoef_write(ViPipe, i, pstStaticRegCfg->u8WtiCoefMid);
            isp_bnr_wti_dval_th_write(ViPipe, i, pstStaticRegCfg->u8WtiDvalThr);
            isp_bnr_wti_sval_th_write(ViPipe, i, pstStaticRegCfg->u8WtiSvalThr);
            isp_bnr_wti_denom_offset_write(ViPipe, i, pstStaticRegCfg->s16WtiDenomOffset);
            isp_bnr_wti_maxcoef_write(ViPipe, i, pstStaticRegCfg->u16WtiCoefMax);
            pstStaticRegCfg->bResh = HI_FALSE;
        }

        if (pstDynaRegCfg->bResh)
        {
            isp_bnr_medc_enable_write(ViPipe, i, pstDynaRegCfg->bMedcEnable);
            isp_bnr_trisad_enable_write(ViPipe, i, pstDynaRegCfg->bTriSadEn);
            isp_bnr_skip_lev4_enable_write(ViPipe, i, pstDynaRegCfg->bSkipLevel4Enable);
            isp_bnr_ratio_r_write(ViPipe, i, pstDynaRegCfg->au8BnrCRatio[0]);
            isp_bnr_ratio_gr_write(ViPipe, i, pstDynaRegCfg->au8BnrCRatio[1]);
            isp_bnr_ratio_gb_write(ViPipe, i, pstDynaRegCfg->au8BnrCRatio[2]);
            isp_bnr_ratio_b_write(ViPipe, i, pstDynaRegCfg->au8BnrCRatio[3]);
            isp_bnr_amed_mode_r_write(ViPipe, i, pstDynaRegCfg->au8AmedMode[0]);
            isp_bnr_amed_mode_gr_write(ViPipe, i, pstDynaRegCfg->au8AmedMode[1]);
            isp_bnr_amed_mode_gb_write(ViPipe, i, pstDynaRegCfg->au8AmedMode[2]);
            isp_bnr_amed_mode_b_write(ViPipe, i, pstDynaRegCfg->au8AmedMode[3]);
            isp_bnr_amed_lev_r_write(ViPipe, i, pstDynaRegCfg->au8AmedLevel[0]);
            isp_bnr_amed_lev_gr_write(ViPipe, i, pstDynaRegCfg->au8AmedLevel[1]);
            isp_bnr_amed_lev_gb_write(ViPipe, i, pstDynaRegCfg->au8AmedLevel[2]);
            isp_bnr_amed_lev_b_write(ViPipe, i, pstDynaRegCfg->au8AmedLevel[3]);
            isp_bnr_jnlm_symcoef_write(ViPipe, i, pstDynaRegCfg->u8JnlmSymCoef);
            isp_bnr_jnlm_gain_write(ViPipe, i, pstDynaRegCfg->u8JnlmGain);
            isp_bnr_lmt_npthresh_write(ViPipe, i, pstDynaRegCfg->u16LmtNpThresh);
            isp_bnr_jnlm_coringhig_write(ViPipe, i, pstDynaRegCfg->u16JnlmCoringHig);
            isp_bnr_rlmt_rgain_write(ViPipe, i, pstDynaRegCfg->u16RLmtRgain);
            isp_bnr_rlmt_bgain_write(ViPipe, i, pstDynaRegCfg->u16RLmtBgain);

            if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                isp_bnr_lmt_odd_waddr_write(ViPipe, i, 0);

                for (j = 1; j < 129; j += 2)
                {
                    isp_bnr_lmt_odd_wdata_write(ViPipe, i, pstDynaRegCfg->au8JnlmLimitLut[j]);
                }

                isp_bnr_lmt_even_waddr_write(ViPipe, i, 0);

                for (j = 0; j < 129; j += 2)
                {
                    isp_bnr_lmt_even_wdata_write(ViPipe, i, pstDynaRegCfg->au8JnlmLimitLut[j]);
                }

                isp_bnr_cor_odd_waddr_write(ViPipe, i, 0);

                for (j = 1; j < 33; j += 2)
                {
                    isp_bnr_cor_odd_wdata_write(ViPipe, i, pstDynaRegCfg->au16JnlmCoringLowLUT[j]);
                }

                isp_bnr_cor_even_waddr_write(ViPipe, i, 0);

                for (j = 0; j < 33; j += 2)
                {
                    isp_bnr_cor_even_wdata_write(ViPipe, i, pstDynaRegCfg->au16JnlmCoringLowLUT[j]);
                }
            }
            else
            {
                isp_bnr_lmt_even_write(ViPipe, i, pstDynaRegCfg->au8JnlmLimitLut);
                isp_bnr_lmt_odd_write(ViPipe, i, pstDynaRegCfg->au8JnlmLimitLut);
                isp_bnr_cor_even_write(ViPipe, i, pstDynaRegCfg->au16JnlmCoringLowLUT);
                isp_bnr_cor_odd_write(ViPipe, i, pstDynaRegCfg->au16JnlmCoringLowLUT);
            }

            isp_bnr_jnlmgain_r0_write(ViPipe, i, pstDynaRegCfg->au32JnlmLimitMultGain[0][BAYER_RGGB]);
            isp_bnr_jnlmgain_gr0_write(ViPipe, i, pstDynaRegCfg->au32JnlmLimitMultGain[0][BAYER_GRBG]);
            isp_bnr_jnlmgain_gb0_write(ViPipe, i, pstDynaRegCfg->au32JnlmLimitMultGain[0][BAYER_GBRG]);
            isp_bnr_jnlmgain_b0_write(ViPipe, i, pstDynaRegCfg->au32JnlmLimitMultGain[0][BAYER_BGGR]);
            isp_bnr_jnlmgain_r1_write(ViPipe, i, pstDynaRegCfg->au32JnlmLimitMultGain[1][BAYER_RGGB]);
            isp_bnr_jnlmgain_gr1_write(ViPipe, i, pstDynaRegCfg->au32JnlmLimitMultGain[1][BAYER_GRBG]);
            isp_bnr_jnlmgain_gb1_write(ViPipe, i, pstDynaRegCfg->au32JnlmLimitMultGain[1][BAYER_GBRG]);
            isp_bnr_jnlmgain_b1_write(ViPipe, i, pstDynaRegCfg->au32JnlmLimitMultGain[1][BAYER_BGGR]);
            //isp_bnr_lut_update_write(ViPipe, i, HI_TRUE);

            bLutUpdate = HI_TRUE;
            pstDynaRegCfg->bResh = bIsOfflineMode;
        }

        if (pstUsrRegCfg->bResh)
        {
            isp_bnr_mono_sensor_write(ViPipe, i, pstUsrRegCfg->bBnrMonoSensorEn);
            isp_bnr_lsc_en_write(ViPipe, i, pstUsrRegCfg->bBnrLscEn);
            isp_bnr_lsc_nrratio_write(ViPipe, i, pstUsrRegCfg->u8BnrLscRatio);

            pstUsrRegCfg->bResh = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1BayernrCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bBnrLutUpdate = bLutUpdate;

    return HI_SUCCESS;
}

static HI_S32 ISP_DgRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_U8  u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_BOOL  bIsOfflineMode;
    ISP_DG_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;
    ISP_DG_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_CTX_S           *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1DgCfg)
    {
        isp_dg_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stDgRegCfg.bDgEn);

        //static
        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stDgRegCfg.stStaticRegCfg;

        if (pstStaticRegCfg->bResh)
        {
            isp_dg_rgain_write(ViPipe, i, 0x100);
            isp_dg_grgain_write(ViPipe, i, 0x100);
            isp_dg_gbgain_write(ViPipe, i, 0x100);
            isp_dg_bgain_write(ViPipe, i, 0x100);
            pstStaticRegCfg->bResh = HI_FALSE;
        }

        //dynamic
        pstDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stDgRegCfg.stDynaRegCfg;

        if (pstDynaRegCfg->bResh)
        {
            isp_dg_clip_value_write(ViPipe, i, pstDynaRegCfg->u32ClipValue);
            pstDynaRegCfg->bResh = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1DgCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_4DgRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_U8    u8BlkNum = pstRegCfgInfo->u8CfgNum;
    HI_BOOL  bIsOfflineMode;
    ISP_4DG_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;
    ISP_4DG_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_CTX_S            *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1WDRDgCfg)
    {
        isp_4dg_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].st4DgRegCfg.bEnable);

        //static
        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].st4DgRegCfg.stStaticRegCfg;

        if (pstStaticRegCfg->bResh)
        {
            isp_4dg0_rgain_write(ViPipe, i, pstStaticRegCfg->u16GainR0);
            isp_4dg0_grgain_write(ViPipe, i, pstStaticRegCfg->u16GainGR0);
            isp_4dg0_gbgain_write(ViPipe, i, pstStaticRegCfg->u16GainGB0);
            isp_4dg0_bgain_write(ViPipe, i, pstStaticRegCfg->u16GainB0);
            isp_4dg1_rgain_write(ViPipe, i, pstStaticRegCfg->u16GainR1);
            isp_4dg1_grgain_write(ViPipe, i, pstStaticRegCfg->u16GainGR1);
            isp_4dg1_gbgain_write(ViPipe, i, pstStaticRegCfg->u16GainGB1);
            isp_4dg1_bgain_write(ViPipe, i, pstStaticRegCfg->u16GainB1);
            isp_4dg2_rgain_write(ViPipe, i, pstStaticRegCfg->u16GainR2);
            isp_4dg2_grgain_write(ViPipe, i, pstStaticRegCfg->u16GainGR2);
            isp_4dg2_gbgain_write(ViPipe, i, pstStaticRegCfg->u16GainGB2);
            isp_4dg2_bgain_write(ViPipe, i, pstStaticRegCfg->u16GainB2);
            isp_4dg3_rgain_write(ViPipe, i, pstStaticRegCfg->u16GainR3);
            isp_4dg3_grgain_write(ViPipe, i, pstStaticRegCfg->u16GainGR3);
            isp_4dg3_gbgain_write(ViPipe, i, pstStaticRegCfg->u16GainGB3);
            isp_4dg3_bgain_write(ViPipe, i, pstStaticRegCfg->u16GainB3);
            pstStaticRegCfg->bResh = HI_FALSE;
        }

        //dynamic
        pstDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].st4DgRegCfg.stDynaRegCfg;

        if (pstDynaRegCfg->bResh)
        {
            isp_4dg0_clip_value_write(ViPipe, i, pstDynaRegCfg->u32ClipValue0);
            isp_4dg1_clip_value_write(ViPipe, i, pstDynaRegCfg->u32ClipValue1);
            isp_4dg2_clip_value_write(ViPipe, i, pstDynaRegCfg->u32ClipValue2);
            isp_4dg3_clip_value_write(ViPipe, i, pstDynaRegCfg->u32ClipValue3);

            pstDynaRegCfg->bResh = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1WDRDgCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_FeRcRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo)
{
    HI_U16 i;
    VI_PIPE ViPipeBind;
    ISP_RC_USR_CFG_S    *pstUsrRegCfg    = HI_NULL;
    ISP_CTX_S *pstIspCtx   = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstUsrRegCfg = &pstRegCfgInfo->stAlgRegCfg[0].stRcRegCfg.stUsrRegCfg;

    if (pstIspCtx->stWdrAttr.bMastPipe)
    {
        for (i = 0; i < pstIspCtx->stWdrAttr.stDevBindPipe.u32Num; i++)
        {
            ViPipeBind = pstIspCtx->stWdrAttr.stDevBindPipe.PipeId[i];
            ISP_CHECK_PIPE(ViPipeBind);

            if (pstRegCfgInfo->unKey.bit1RcCfg)
            {
                isp_fe_rc_en_write(ViPipeBind, pstRegCfgInfo->stAlgRegCfg[0].stRcRegCfg.bRcEn);

                if (pstUsrRegCfg->bResh)
                {
                    isp_rc_sqradius_write(ViPipeBind, pstUsrRegCfg->u32SquareRadius);
                    isp_rc_cenhor_coor_write(ViPipeBind, pstUsrRegCfg->u16CenterHorCoor);
                    isp_rc_cenver_coor_write(ViPipeBind, pstUsrRegCfg->u16CenterVerCoor);
                }
            }
        }

        pstUsrRegCfg->bResh            = HI_FALSE;
        pstRegCfgInfo->unKey.bit1RcCfg = 0;
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_FlickRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL bIsOfflineMode;
    HI_U8   u8BlkNum = pstRegCfgInfo->u8CfgNum;
    ISP_FLICKER_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_CTX_S                *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1FlickerCfg)
    {
        pstDynaRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stFlickerRegCfg.stDynaRegCfg;

        if (pstDynaRegCfg->bResh)
        {
            isp_flick_mergeframe_write(ViPipe, i, pstDynaRegCfg->u8MergeFrm);
            isp_flick_overth_write(ViPipe, i, pstDynaRegCfg->u16OverThr);
            isp_flick_gravg_pre_write(ViPipe, i, pstDynaRegCfg->s16GrAvgPre);
            isp_flick_gbavg_pre_write(ViPipe, i, pstDynaRegCfg->s16GbAvgPre);
            isp_flick_overcountth_write(ViPipe, i, pstDynaRegCfg->u32OverCntThr);
            isp_flick_countover_pre_write(ViPipe, i, pstDynaRegCfg->u32CntOverPre);
            pstDynaRegCfg->bResh = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1FlickerCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_PreGammaRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL bIsOfflineMode;
    HI_BOOL bUsrResh  = HI_FALSE;
    HI_BOOL bIdxResh  = HI_FALSE;
    HI_U8   u8BlkNum  = pstRegCfgInfo->u8CfgNum;
    HI_U16  j;
    ISP_PREGAMMA_DYNA_CFG_S   *pstDynaRegCfg   = HI_NULL;
    ISP_PREGAMMA_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;
    ISP_CTX_S                 *pstIspCtx       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1PreGammaCfg)
    {
        isp_pregamma_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stPreGammaCfg.bPreGammaEn);

        pstDynaRegCfg   = &pstRegCfgInfo->stAlgRegCfg[i].stPreGammaCfg.stDynaRegCfg;
        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stPreGammaCfg.stStaticRegCfg;

        //Enable PreGamma
        if (pstStaticRegCfg->bStaticResh)
        {
            isp_pregamma_bitw_out_write(ViPipe, i, pstStaticRegCfg->u8BitDepthOut);
            isp_pregamma_bitw_in_write(ViPipe, i, pstStaticRegCfg->u8BitDepthIn);

            pstStaticRegCfg->bStaticResh = HI_FALSE;
        }

        bIdxResh = (isp_pregamma_update_index_read(ViPipe, i) != pstDynaRegCfg->u32UpdateIndex);
        bUsrResh = (bIsOfflineMode) ? (pstDynaRegCfg->bPreGammaLutUpdateEn & bIdxResh) : (pstDynaRegCfg->bPreGammaLutUpdateEn);

        //LUT update
        if (bUsrResh)
        {
            isp_pregamma_update_index_write(ViPipe, i, pstDynaRegCfg->u32UpdateIndex);

            if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                isp_pregamma_lut_waddr_write(ViPipe, i, 0);

                for ( j = 0 ; j < PREGAMMA_NODE_NUMBER ; j++ )
                {
                    isp_pregamma_lut_wdata_write(ViPipe, i, (pstDynaRegCfg->u32PreGammaLUT[j]));
                }
            }
            else
            {
                isp_pregamma_lut_write(ViPipe, i, pstDynaRegCfg->u32PreGammaLUT);
            }

            pstDynaRegCfg->bPreGammaLutUpdateEn = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1PreGammaCfg  = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_BeBlcRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL bIsOfflineMode;
    HI_U8 u8BlkNum = pstRegCfgInfo->u8CfgNum;
    ISP_BE_BLC_CFG_S *pstBeBlcCfg = HI_NULL;
    ISP_CTX_S        *pstIspCtx   = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1BeBlcCfg)
    {
        pstBeBlcCfg = &pstRegCfgInfo->stAlgRegCfg[i].stBeBlcCfg;

        if (pstBeBlcCfg->bReshStatic)
        {
            /*4Dg*/
            isp_4dg_en_in_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[0].stStaticRegCfg.bBlcIn);
            isp_4dg_en_out_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[0].stStaticRegCfg.bBlcOut);

            /*WDR*/
            isp_wdr_bsaveblc_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[0].stStaticRegCfg.bBlcOut);
            /*rlsc*/
            isp_rlsc_blcoffsetin_en_write(ViPipe, i, pstBeBlcCfg->stRLscBlc.stStaticRegCfg.bBlcIn);
            isp_rlsc_blcoffsetout_en_write(ViPipe, i, pstBeBlcCfg->stRLscBlc.stStaticRegCfg.bBlcOut);
            /*lsc*/
            isp_lsc_blc_in_en_write(ViPipe, i, pstBeBlcCfg->stLscBlc.stStaticRegCfg.bBlcIn);
            isp_lsc_blc_out_en_write(ViPipe, i, pstBeBlcCfg->stLscBlc.stStaticRegCfg.bBlcOut);
            /*Dg*/
            isp_dg_en_in_write(ViPipe, i, pstBeBlcCfg->stDgBlc.stStaticRegCfg.bBlcIn);
            isp_dg_en_out_write(ViPipe, i, pstBeBlcCfg->stDgBlc.stStaticRegCfg.bBlcOut);
            /*AE*/
            isp_ae_blc_en_write(ViPipe, i, pstBeBlcCfg->stAeBlc.stStaticRegCfg.bBlcIn);
            /*MG*/
            isp_la_blc_en_write(ViPipe, i, pstBeBlcCfg->stMgBlc.stStaticRegCfg.bBlcIn);
            /*WB*/
            isp_wb_en_in_write(ViPipe, i, pstBeBlcCfg->stWbBlc.stStaticRegCfg.bBlcIn);
            isp_wb_en_out_write(ViPipe, i, pstBeBlcCfg->stWbBlc.stStaticRegCfg.bBlcOut);

            pstBeBlcCfg->bReshStatic = HI_FALSE;
        }

        if (pstBeBlcCfg->bReshDyna)
        {
            /*4Dg*/
            isp_4dg0_ofsr_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[0].stUsrRegCfg.au16Blc[0]);
            isp_4dg0_ofsgr_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[0].stUsrRegCfg.au16Blc[1]);
            isp_4dg0_ofsgb_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[0].stUsrRegCfg.au16Blc[2]);
            isp_4dg0_ofsb_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[0].stUsrRegCfg.au16Blc[3]);

            isp_4dg1_ofsr_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[1].stUsrRegCfg.au16Blc[0]);
            isp_4dg1_ofsgr_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[1].stUsrRegCfg.au16Blc[1]);
            isp_4dg1_ofsgb_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[1].stUsrRegCfg.au16Blc[2]);
            isp_4dg1_ofsb_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[1].stUsrRegCfg.au16Blc[3]);

            isp_4dg2_ofsr_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[2].stUsrRegCfg.au16Blc[0]);
            isp_4dg2_ofsgr_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[2].stUsrRegCfg.au16Blc[1]);
            isp_4dg2_ofsgb_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[2].stUsrRegCfg.au16Blc[2]);
            isp_4dg2_ofsb_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[2].stUsrRegCfg.au16Blc[3]);

            isp_4dg3_ofsr_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[3].stUsrRegCfg.au16Blc[0]);
            isp_4dg3_ofsgr_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[3].stUsrRegCfg.au16Blc[1]);
            isp_4dg3_ofsgb_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[3].stUsrRegCfg.au16Blc[2]);
            isp_4dg3_ofsb_write(ViPipe, i, pstBeBlcCfg->st4DgBlc[3].stUsrRegCfg.au16Blc[3]);
            /*WDR*/
            isp_wdr_outblc_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[0].stUsrRegCfg.u16OutBlc);
            isp_wdr_f0_inblc_r_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[0].stUsrRegCfg.au16Blc[0]);
            isp_wdr_f0_inblc_gr_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[0].stUsrRegCfg.au16Blc[1]);
            isp_wdr_f0_inblc_gb_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[0].stUsrRegCfg.au16Blc[2]);
            isp_wdr_f0_inblc_b_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[0].stUsrRegCfg.au16Blc[3]);

            isp_wdr_f1_inblc_r_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[1].stUsrRegCfg.au16Blc[0]);
            isp_wdr_f1_inblc_gr_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[1].stUsrRegCfg.au16Blc[1]);
            isp_wdr_f1_inblc_gb_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[1].stUsrRegCfg.au16Blc[2]);
            isp_wdr_f1_inblc_b_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[1].stUsrRegCfg.au16Blc[3]);

            isp_wdr_f2_inblc_r_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[2].stUsrRegCfg.au16Blc[0]);
            isp_wdr_f2_inblc_gr_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[2].stUsrRegCfg.au16Blc[1]);
            isp_wdr_f2_inblc_gb_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[2].stUsrRegCfg.au16Blc[2]);
            isp_wdr_f2_inblc_b_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[2].stUsrRegCfg.au16Blc[3]);

            isp_wdr_f3_inblc_r_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[3].stUsrRegCfg.au16Blc[0]);
            isp_wdr_f3_inblc_gr_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[3].stUsrRegCfg.au16Blc[1]);
            isp_wdr_f3_inblc_gb_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[3].stUsrRegCfg.au16Blc[2]);
            isp_wdr_f3_inblc_b_write(ViPipe, i, pstBeBlcCfg->stWdrBlc[3].stUsrRegCfg.au16Blc[3]);

            /*flicker*/
            isp_flick_f0_inblc_r_write(ViPipe, i, pstBeBlcCfg->stFlickerBlc[0].stUsrRegCfg.au16Blc[0]);
            isp_flick_f0_inblc_gr_write(ViPipe, i, pstBeBlcCfg->stFlickerBlc[0].stUsrRegCfg.au16Blc[1]);
            isp_flick_f0_inblc_gb_write(ViPipe, i, pstBeBlcCfg->stFlickerBlc[0].stUsrRegCfg.au16Blc[2]);
            isp_flick_f0_inblc_b_write(ViPipe, i, pstBeBlcCfg->stFlickerBlc[0].stUsrRegCfg.au16Blc[3]);

            isp_flick_f1_inblc_r_write(ViPipe, i, pstBeBlcCfg->stFlickerBlc[1].stUsrRegCfg.au16Blc[0]);
            isp_flick_f1_inblc_gr_write(ViPipe, i, pstBeBlcCfg->stFlickerBlc[1].stUsrRegCfg.au16Blc[1]);
            isp_flick_f1_inblc_gb_write(ViPipe, i, pstBeBlcCfg->stFlickerBlc[1].stUsrRegCfg.au16Blc[2]);
            isp_flick_f1_inblc_b_write(ViPipe, i, pstBeBlcCfg->stFlickerBlc[1].stUsrRegCfg.au16Blc[3]);

            isp_flick_f2_inblc_r_write(ViPipe, i, pstBeBlcCfg->stFlickerBlc[2].stUsrRegCfg.au16Blc[0]);
            isp_flick_f2_inblc_gr_write(ViPipe, i, pstBeBlcCfg->stFlickerBlc[2].stUsrRegCfg.au16Blc[1]);
            isp_flick_f2_inblc_gb_write(ViPipe, i, pstBeBlcCfg->stFlickerBlc[2].stUsrRegCfg.au16Blc[2]);
            isp_flick_f2_inblc_b_write(ViPipe, i, pstBeBlcCfg->stFlickerBlc[2].stUsrRegCfg.au16Blc[3]);

            isp_flick_f3_inblc_r_write(ViPipe, i, pstBeBlcCfg->stFlickerBlc[3].stUsrRegCfg.au16Blc[0]);
            isp_flick_f3_inblc_gr_write(ViPipe, i, pstBeBlcCfg->stFlickerBlc[3].stUsrRegCfg.au16Blc[1]);
            isp_flick_f3_inblc_gb_write(ViPipe, i, pstBeBlcCfg->stFlickerBlc[3].stUsrRegCfg.au16Blc[2]);
            isp_flick_f3_inblc_b_write(ViPipe, i, pstBeBlcCfg->stFlickerBlc[3].stUsrRegCfg.au16Blc[3]);

            /*pregamma*/
            isp_pregamma_offset_r_write(ViPipe, i, pstBeBlcCfg->stPreGammaBlc.stUsrRegCfg.au16Blc[0]);
            isp_pregamma_offset_gr_write(ViPipe, i, pstBeBlcCfg->stPreGammaBlc.stUsrRegCfg.au16Blc[1]);
            isp_pregamma_offset_b_write(ViPipe, i, pstBeBlcCfg->stPreGammaBlc.stUsrRegCfg.au16Blc[2]);
            isp_pregamma_offset_gb_write(ViPipe, i, pstBeBlcCfg->stPreGammaBlc.stUsrRegCfg.au16Blc[3]);

            /*rlsc*/
            isp_rlsc_blcoffsetr_write(ViPipe, i, pstBeBlcCfg->stRLscBlc.stUsrRegCfg.au16Blc[0]);
            isp_rlsc_blcoffsetgr_write(ViPipe, i, pstBeBlcCfg->stRLscBlc.stUsrRegCfg.au16Blc[1]);
            isp_rlsc_blcoffsetgb_write(ViPipe, i, pstBeBlcCfg->stRLscBlc.stUsrRegCfg.au16Blc[2]);
            isp_rlsc_blcoffsetb_write(ViPipe, i, pstBeBlcCfg->stRLscBlc.stUsrRegCfg.au16Blc[3]);

            /*lsc*/
            isp_lsc_blc_r_write(ViPipe, i, pstBeBlcCfg->stLscBlc.stUsrRegCfg.au16Blc[0]);
            isp_lsc_blc_gr_write(ViPipe, i, pstBeBlcCfg->stLscBlc.stUsrRegCfg.au16Blc[1]);
            isp_lsc_blc_gb_write(ViPipe, i, pstBeBlcCfg->stLscBlc.stUsrRegCfg.au16Blc[2]);
            isp_lsc_blc_b_write(ViPipe, i, pstBeBlcCfg->stLscBlc.stUsrRegCfg.au16Blc[3]);

            /*Dg*/
            isp_dg_ofsr_write(ViPipe, i, pstBeBlcCfg->stDgBlc.stUsrRegCfg.au16Blc[0]);
            isp_dg_ofsgr_write(ViPipe, i, pstBeBlcCfg->stDgBlc.stUsrRegCfg.au16Blc[1]);
            isp_dg_ofsgb_write(ViPipe, i, pstBeBlcCfg->stDgBlc.stUsrRegCfg.au16Blc[2]);
            isp_dg_ofsb_write(ViPipe, i, pstBeBlcCfg->stDgBlc.stUsrRegCfg.au16Blc[3]);

            /*AE*/
            isp_ae_offset_r_write(ViPipe, i, pstBeBlcCfg->stAeBlc.stUsrRegCfg.au16Blc[0]);
            isp_ae_offset_gr_write(ViPipe, i, pstBeBlcCfg->stAeBlc.stUsrRegCfg.au16Blc[1]);
            isp_ae_offset_gb_write(ViPipe, i, pstBeBlcCfg->stAeBlc.stUsrRegCfg.au16Blc[2]);
            isp_ae_offset_b_write(ViPipe, i, pstBeBlcCfg->stAeBlc.stUsrRegCfg.au16Blc[3]);
            /*MG*/
            isp_la_offset_r_write(ViPipe, i, pstBeBlcCfg->stMgBlc.stUsrRegCfg.au16Blc[0]);
            isp_la_offset_gr_write(ViPipe, i, pstBeBlcCfg->stMgBlc.stUsrRegCfg.au16Blc[1]);
            isp_la_offset_gb_write(ViPipe, i, pstBeBlcCfg->stMgBlc.stUsrRegCfg.au16Blc[2]);
            isp_la_offset_b_write(ViPipe, i, pstBeBlcCfg->stMgBlc.stUsrRegCfg.au16Blc[3]);
            /*WB*/
            isp_wb_ofsr_write (ViPipe, i, pstBeBlcCfg->stWbBlc.stUsrRegCfg.au16Blc[0]);
            isp_wb_ofsgr_write(ViPipe, i, pstBeBlcCfg->stWbBlc.stUsrRegCfg.au16Blc[1]);
            isp_wb_ofsgb_write(ViPipe, i, pstBeBlcCfg->stWbBlc.stUsrRegCfg.au16Blc[2]);
            isp_wb_ofsb_write (ViPipe, i, pstBeBlcCfg->stWbBlc.stUsrRegCfg.au16Blc[3]);
            /*split*/
            isp_wdrsplit_offset_r_write(ViPipe, i, pstBeBlcCfg->stSplitBlc.stUsrRegCfg.au16Blc[0]);
            isp_wdrsplit_offset_gr_write(ViPipe, i, pstBeBlcCfg->stSplitBlc.stUsrRegCfg.au16Blc[1]);
            isp_wdrsplit_offset_gb_write(ViPipe, i, pstBeBlcCfg->stSplitBlc.stUsrRegCfg.au16Blc[2]);
            isp_wdrsplit_offset_b_write(ViPipe, i, pstBeBlcCfg->stSplitBlc.stUsrRegCfg.au16Blc[3]);
            isp_wdrsplit_blc_write(ViPipe, i, pstBeBlcCfg->stSplitBlc.stUsrRegCfg.u16OutBlc);

            /*LogLUT*/
            isp_loglut_offset_r_write(ViPipe, i, pstBeBlcCfg->stLogLUTBlc.stUsrRegCfg.au16Blc[0]);
            isp_loglut_offset_gr_write(ViPipe, i, pstBeBlcCfg->stLogLUTBlc.stUsrRegCfg.au16Blc[1]);
            isp_loglut_offset_gb_write(ViPipe, i, pstBeBlcCfg->stLogLUTBlc.stUsrRegCfg.au16Blc[2]);
            isp_loglut_offset_b_write(ViPipe, i, pstBeBlcCfg->stLogLUTBlc.stUsrRegCfg.au16Blc[3]);

            pstBeBlcCfg->bReshDyna = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1BeBlcCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_WdrSplitRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL  bIsOfflineMode;
    HI_U8    j;
    HI_U8    u8BlkNum = pstRegCfgInfo->u8CfgNum;
    ISP_SPLIT_STATIC_CFG_S *pstStaticRegCfg = HI_NULL;
    ISP_CTX_S              *pstIspCtx       = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));

    if (pstRegCfgInfo->unKey.bit1SplitCfg)
    {
        isp_wdrsplit1_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stSplitCfg.bEnable);

        pstStaticRegCfg = &pstRegCfgInfo->stAlgRegCfg[i].stSplitCfg.stStaticRegCfg;

        if (pstStaticRegCfg->bResh)
        {
            isp_wdrsplit_bitw_out_write(ViPipe, i, pstStaticRegCfg->u8BitDepthOut);
            isp_wdrsplit_bitw_in_write(ViPipe, i, pstStaticRegCfg->u8BitDepthIn);
            isp_wdrsplit_mode_out_write(ViPipe, i, pstStaticRegCfg->u8ModeOut);
            isp_wdrsplit_mode_in_write(ViPipe, i, pstStaticRegCfg->u8ModeIn);

            if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                isp_wdrsplit_lut_waddr_write(ViPipe, i, 0);

                for ( j = 0 ; j < 129 ; j++ )
                {
                    isp_wdrsplit_lut_wdata_write(ViPipe, i, pstStaticRegCfg->au16WdrSplitLut[j]);
                }
            }
            else
            {
                isp_wdrsplit_lut_write(ViPipe, i, pstStaticRegCfg->au16WdrSplitLut);
            }

            pstStaticRegCfg->bResh = HI_FALSE;
        }

        pstRegCfgInfo->unKey.bit1SplitCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_ClutRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL  bIsOfflineMode;
    HI_BOOL  bUsrResh   = HI_FALSE;
    HI_BOOL  bIdxResh   = HI_FALSE;
    HI_BOOL  bLutUpdate = HI_FALSE;
    HI_U16   j;
    HI_U8 u8BlkNum = pstRegCfgInfo->u8CfgNum;
    ISP_CLUT_USR_COEF_CFG_S     *pstClutUsrCoefCfg   = HI_NULL;
    ISP_CLUT_USR_CTRL_CFG_S     *pstClutUsrCtrlCfg   = HI_NULL;
    ISP_CTX_S                   *pstIspCtx           = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    bIsOfflineMode = (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                      || IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode));


    if ((pstRegCfgInfo->unKey.bit1ClutCfg)) //&&( DYNAMIC_RANGE_XDR == pstIspCtx->stHdrAttr.enDynamicRange))
    {
        isp_clut_en_write(ViPipe, i, pstRegCfgInfo->stAlgRegCfg[i].stClutCfg.bEnable);
        isp_clut_sel_write(ViPipe, i, HI_ISP_CLUT_SEL_WRITE);
        pstClutUsrCoefCfg   = &pstRegCfgInfo->stAlgRegCfg[i].stClutCfg.stUsrRegCfg.stClutUsrCoefCfg;

        bIdxResh = (isp_clut_update_index_read(ViPipe, i) != pstClutUsrCoefCfg->u32UpdateIndex);
        bUsrResh = (bIsOfflineMode) ? (pstClutUsrCoefCfg->bResh & bIdxResh) : (pstClutUsrCoefCfg->bResh);

        if (bUsrResh)
        {
            isp_clut_update_index_write(ViPipe, i, pstClutUsrCoefCfg->u32UpdateIndex);

            if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
                || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
            {
                isp_clut_lut0_waddr_write(ViPipe, i, 0);

                for (j = 0; j < HI_ISP_CLUT_LUT0; j++)
                {
                    isp_clut_lut0_wdata_write(ViPipe, i, pstClutUsrCoefCfg->au32lut0[j]);
                }

                isp_clut_lut1_waddr_write(ViPipe, i, 0);

                for (j = 0; j < HI_ISP_CLUT_LUT1; j++)
                {
                    isp_clut_lut1_wdata_write(ViPipe, i, pstClutUsrCoefCfg->au32lut1[j]);
                }

                isp_clut_lut2_waddr_write(ViPipe, i, 0);

                for (j = 0; j < HI_ISP_CLUT_LUT2; j++)
                {
                    isp_clut_lut2_wdata_write(ViPipe, i, pstClutUsrCoefCfg->au32lut2[j]);
                }

                isp_clut_lut3_waddr_write(ViPipe, i, 0);

                for (j = 0; j < HI_ISP_CLUT_LUT3; j++)
                {
                    isp_clut_lut3_wdata_write(ViPipe, i, pstClutUsrCoefCfg->au32lut3[j]);
                }

                isp_clut_lut4_waddr_write(ViPipe, i, 0);

                for (j = 0; j < HI_ISP_CLUT_LUT4; j++)
                {
                    isp_clut_lut4_wdata_write(ViPipe, i, pstClutUsrCoefCfg->au32lut4[j]);
                }

                isp_clut_lut5_waddr_write(ViPipe, i, 0);

                for (j = 0; j < HI_ISP_CLUT_LUT5; j++)
                {
                    isp_clut_lut5_wdata_write(ViPipe, i, pstClutUsrCoefCfg->au32lut5[j]);
                }

                isp_clut_lut6_waddr_write(ViPipe, i, 0);

                for (j = 0; j < HI_ISP_CLUT_LUT6; j++)
                {
                    isp_clut_lut6_wdata_write(ViPipe, i, pstClutUsrCoefCfg->au32lut6[j]);
                }

                isp_clut_lut7_waddr_write(ViPipe, i, 0);

                for (j = 0; j < HI_ISP_CLUT_LUT7; j++)
                {
                    isp_clut_lut7_wdata_write(ViPipe, i, pstClutUsrCoefCfg->au32lut7[j]);
                }
            }
            else
            {
                isp_clut_lut0_write(ViPipe, i, pstClutUsrCoefCfg->au32lut0);
                isp_clut_lut1_write(ViPipe, i, pstClutUsrCoefCfg->au32lut1);
                isp_clut_lut2_write(ViPipe, i, pstClutUsrCoefCfg->au32lut2);
                isp_clut_lut3_write(ViPipe, i, pstClutUsrCoefCfg->au32lut3);
                isp_clut_lut4_write(ViPipe, i, pstClutUsrCoefCfg->au32lut4);
                isp_clut_lut5_write(ViPipe, i, pstClutUsrCoefCfg->au32lut5);
                isp_clut_lut6_write(ViPipe, i, pstClutUsrCoefCfg->au32lut6);
                isp_clut_lut7_write(ViPipe, i, pstClutUsrCoefCfg->au32lut7);
            }

            //isp_clut_lut_update_write(ViPipe, i, HI_TRUE);
            bLutUpdate = HI_TRUE;
            pstClutUsrCoefCfg->bResh = bIsOfflineMode;
        }

        pstClutUsrCtrlCfg   = &pstRegCfgInfo->stAlgRegCfg[i].stClutCfg.stUsrRegCfg.stClutUsrCtrlCfg;

        if (pstClutUsrCtrlCfg->bResh)
        {
            isp_clut_gain2_write(ViPipe, i, pstClutUsrCtrlCfg->u32GainB);
            isp_clut_gain1_write(ViPipe, i, pstClutUsrCtrlCfg->u32GainG);
            isp_clut_gain0_write(ViPipe, i, pstClutUsrCtrlCfg->u32GainR);
            pstClutUsrCtrlCfg->bResh = bIsOfflineMode;
        }

        pstRegCfgInfo->unKey.bit1ClutCfg = bIsOfflineMode ? 1 : ((u8BlkNum <= (i + 1)) ? 0 : 1);
    }

    pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg.bClutLutUpdate = bLutUpdate;

    return HI_SUCCESS;
}

static HI_S32 ISP_FeUpdateRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo)
{
    HI_U32  i;
    VI_PIPE ViPipeBind;
    ISP_CTX_S *pstIspCtx   = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    for (i = 0; i < pstIspCtx->stWdrAttr.stDevBindPipe.u32Num; i++)
    {
        ViPipeBind = pstIspCtx->stWdrAttr.stDevBindPipe.PipeId[i];
        ISP_CHECK_PIPE(ViPipeBind);

        isp_fe_update_mode_write(ViPipeBind, HI_FALSE);
        isp_fe_update_write(ViPipeBind, HI_TRUE);

        if (pstRegCfgInfo->stAlgRegCfg[0].stFeLutUpdateCfg.bLsc1LutUpdate)
        {
            isp_lsc1_lut_update_write(ViPipeBind, pstRegCfgInfo->stAlgRegCfg[0].stFeLutUpdateCfg.bLsc1LutUpdate);
        }

        if (pstRegCfgInfo->stAlgRegCfg[0].stFeLutUpdateCfg.bAe1LutUpdate)
        {
            isp_ae1_lut_update_write(ViPipeBind, pstRegCfgInfo->stAlgRegCfg[0].stFeLutUpdateCfg.bAe1LutUpdate);
        }
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_FeSystemRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo)
{
    HI_U32  i;
    HI_BOOL bIspCropEn = HI_FALSE;
    HI_U8   u8RggbCfg;
    HI_S32  s32X, s32Y;
    HI_U32  u32Width, u32Height;
    HI_U32  u32PipeW, u32PipeH;
    VI_PIPE ViPipeBind, ViPipeId;
    ISP_CTX_S *pstIspCtx   = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    ViPipeId  = pstIspCtx->stWdrAttr.stDevBindPipe.PipeId[0];
    u8RggbCfg = hi_ext_system_rggb_cfg_read(ViPipeId);

    s32X      = pstIspCtx->stBlockAttr.stFrameRect.s32X;
    s32Y      = pstIspCtx->stBlockAttr.stFrameRect.s32Y;
    u32Width  = pstIspCtx->stBlockAttr.stFrameRect.u32Width;
    u32Height = pstIspCtx->stBlockAttr.stFrameRect.u32Height;
    u32PipeW  = pstIspCtx->stPipeSize.u32Width;
    u32PipeH  = pstIspCtx->stPipeSize.u32Height;

    /* ISP crop low-power process */
    if ((0 == s32X) &&
        (0 == s32Y) &&
        (u32Width  == u32PipeW) &&
        (u32Height == u32PipeH))
    {
        bIspCropEn = HI_FALSE;
    }
    else
    {
        bIspCropEn = HI_TRUE;
    }

    for (i = 0; i < pstIspCtx->stWdrAttr.stDevBindPipe.u32Num; i++)
    {
        ViPipeBind = pstIspCtx->stWdrAttr.stDevBindPipe.PipeId[i];
        ISP_CHECK_PIPE(ViPipeBind);

        isp_fe_crop_en_write(ViPipeBind, bIspCropEn);
        isp_crop_pos_x_write(ViPipeBind, s32X);
        isp_crop_pos_y_write(ViPipeBind, s32Y);
        isp_crop_width_out_write(ViPipeBind, u32Width - 1);
        isp_crop_height_out_write(ViPipeBind, u32Height - 1);
        isp_fe_rggb_cfg_write(ViPipeBind, u8RggbCfg);
        isp_fe_fix_timing_write(ViPipeBind, HI_ISP_FE_FIX_TIMING_STAT);
        isp_fe_width_write(ViPipeBind, u32PipeW  - 1);
        isp_fe_height_write(ViPipeBind, u32PipeH - 1);
        isp_fe_blk_width_write(ViPipeBind, u32PipeW  - 1);
        isp_fe_blk_height_write(ViPipeBind, u32PipeH - 1);
        isp_fe_blk_f_hblank_write(ViPipeBind, 0);
        isp_fe_hsync_mode_write(ViPipeBind, 0);
        isp_fe_vsync_mode_write(ViPipeBind, 0);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_SystemRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    HI_BOOL bMcdsVEn;
    HI_U32  u32RggbCfg;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    u32RggbCfg = hi_ext_system_rggb_cfg_read(ViPipe);
    bMcdsVEn = pstRegCfgInfo->stAlgRegCfg[i].stMcdsRegCfg.stStaticRegCfg.bVcdsEn;

    //isp_be_en_write(ViPipe, i,HI_TRUE);
    isp_blk_f_hblank_write(ViPipe, i, HI_ISP_BLK_F_HBLANK_DEFAULT);
    isp_blk_f_vblank_write(ViPipe, i, HI_ISP_BLK_F_VBLANK_DEFAULT);
    isp_blk_b_hblank_write(ViPipe, i, HI_ISP_BLK_B_HBLANK_DEFAULT);
    isp_blk_b_vblank_write(ViPipe, i, HI_ISP_BLK_B_VBLANK_DEFAULT);

    isp_be_rggb_cfg_write(ViPipe, i, u32RggbCfg);
    isp_format_write(ViPipe, i, !bMcdsVEn); /* ISP Be YUV format */
    //isp_be_reg_up_write(ViPipe, i, HI_TRUE);

    if ((IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)) ||
        (IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)))
    {
        isp_stt_en_write(ViPipe, i, HI_TRUE);
    }
    else
    {
        isp_stt_en_write(ViPipe, i, HI_FALSE);
    }

    if (DYNAMIC_RANGE_XDR == pstIspCtx->stHdrAttr.enDynamicRange )
    {
        isp_sqrt_en_write(ViPipe, i, HI_TRUE);//sqrt in raw
        isp_sq_en_write(ViPipe, i, HI_TRUE);//sq in rgb
        isp_sqrt1_en_write(ViPipe, i, HI_TRUE);//sqrt in rgb
    }
    else
    {
        isp_sqrt_en_write(ViPipe, i, HI_FALSE);//sqrt in raw
        isp_sq_en_write(ViPipe, i, HI_FALSE);//sq in rgb
        isp_sqrt1_en_write(ViPipe, i, HI_FALSE);//sqrt in rgb
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_DitherRegConfig (VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    /*after drc module*/
    isp_drc_dither_out_bits_write(ViPipe, i, HI_ISP_DRC_DITHER_OUT_BITS_DEFAULT);
    isp_drc_dither_round_write(ViPipe, i, HI_ISP_DRC_DITHER_ROUND_DEFAULT);
    isp_drc_dither_spatial_mode_write(ViPipe, i, HI_ISP_DRC_DITHER_SPATIAL_MODE_DEFAULT);

    /*after gamma module*/
    isp_dmnr_dither_en_write(ViPipe, i, HI_TRUE);
    isp_dmnr_dither_out_bits_write(ViPipe, i, HI_ISP_DMNR_DITHER_OUT_BITS_DEFAULT);
    isp_dmnr_dither_round_write(ViPipe, i, HI_ISP_DMNR_DITHER_ROUND_DEFAULT);
    isp_dmnr_dither_spatial_mode_write(ViPipe, i, HI_ISP_DMNR_DITHER_SPATIAL_MODE_DEFAULT);

    /*after CA module ,not uesed in Hi3559AV100*/
    //isp_acm_dither_en_write(ViPipe,i,HI_FALSE);
    //isp_acm_dither_out_bits_write(ViPipe,i,HI_ISP_ACM_DITHER_OUT_BITS_DEFAULT);
    //isp_acm_dither_round_write(ViPipe,i,HI_ISP_ACM_DITHER_ROUND_DEFAULT);
    //isp_acm_dither_spatial_mode_write(ViPipe,i,HI_ISP_ACM_DITHER_SPATIAL_MODE_DEFAULT);

    /*after sqrt1 module*/
    if ( DYNAMIC_RANGE_XDR == pstIspCtx->stHdrAttr.enDynamicRange )
    {
        isp_sqrt1_dither_en_write(ViPipe, i, HI_FALSE);
    }
    else
    {
        isp_sqrt1_dither_en_write(ViPipe, i, HI_TRUE);
        isp_sqrt1_dither_out_bits_write(ViPipe, i, HI_ISP_SQRT1_DITHER_OUT_BITS_DEFAULT);
        isp_sqrt1_dither_round_write(ViPipe, i, HI_ISP_SQRT1_DITHER_ROUND_DEFAULT);
        isp_sqrt1_dither_spatial_mode_write(ViPipe, i, HI_ISP_SQRT1_DITHER_SPATIAL_MODE_DEFAULT);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_BeAlgLutUpdateRegConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo, HI_U8 i)
{
    ISP_BE_LUT_UPDATE_REG_CFG_S  *pstBeLutUpdateCfg = &pstRegCfgInfo->stAlgRegCfg[i].stBeLutUpdateCfg;

    if (pstBeLutUpdateCfg->bAeLutUpdate)
    {
        isp_ae_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bAeLutUpdate);
    }

    if (pstBeLutUpdateCfg->bSharpenLutUpdate)
    {
        isp_sharpen_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bSharpenLutUpdate);
    }

    if (pstBeLutUpdateCfg->bNddmUsmLutUpdate)
    {
        isp_nddm_usm_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bNddmUsmLutUpdate);
    }

    if (pstBeLutUpdateCfg->bNddmGfLutUpdate)
    {
        isp_nddm_gf_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bNddmGfLutUpdate);
    }

    if (pstBeLutUpdateCfg->bLdciDrcLutUpdate)
    {
        isp_ldci_drc_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bLdciDrcLutUpdate);
    }

    if (pstBeLutUpdateCfg->bLdciCalcLutUpdate)
    {
        isp_ldci_calc_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bLdciCalcLutUpdate);
    }

    if (pstBeLutUpdateCfg->bDpcLutUpdate)
    {
        isp_dpc_ex_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bDpcLutUpdate);
    }

    if (pstBeLutUpdateCfg->bLscLutUpdate)
    {
        isp_lsc_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bLscLutUpdate);
    }

    if (pstBeLutUpdateCfg->bBnrLscLutUpdate)
    {
        isp_bnr_lsc_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bBnrLscLutUpdate);
    }

    if (pstBeLutUpdateCfg->bRlscLutUpdate)
    {
        isp_rlsc_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bRlscLutUpdate);
    }

    if (pstBeLutUpdateCfg->bGammaLutUpdate)
    {
        isp_gamma_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bGammaLutUpdate);
    }

    if (pstBeLutUpdateCfg->bCaLutUpdate)
    {
        isp_ca_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bCaLutUpdate);
    }

    if (pstBeLutUpdateCfg->bWdrLutUpdate)
    {
        isp_wdr_129x8_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bWdrLutUpdate);
    }

    if (pstBeLutUpdateCfg->bDehazeLutUpdate)
    {
        isp_dehaze_str_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bDehazeLutUpdate);
    }
    if (pstBeLutUpdateCfg->bDehazePreStatLutUpdate)
    {
        isp_dehaze_pre_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bDehazePreStatLutUpdate);
    }
    if (pstBeLutUpdateCfg->bBnrLutUpdate)
    {
        isp_bnr_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bBnrLutUpdate);
    }
    if (pstBeLutUpdateCfg->bClutLutUpdate)
    {
        isp_clut_lut_update_write(ViPipe, i, pstBeLutUpdateCfg->bClutLutUpdate);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_BeReshCfg(ISP_ALG_REG_CFG_S *pstAlgRegCfg)
{
    pstAlgRegCfg->stAwbRegCfg.stAwbRegStaCfg.bBEAwbStaCfg     = HI_TRUE;
    pstAlgRegCfg->stAwbRegCfg.stAwbRegUsrCfg.bResh            = HI_TRUE;

    pstAlgRegCfg->stDemRegCfg.stStaticRegCfg.bResh            = HI_TRUE;
    pstAlgRegCfg->stDemRegCfg.stDynaRegCfg.bResh              = HI_TRUE;

    pstAlgRegCfg->stLdciRegCfg.stStaticRegCfg.bStaticResh     = HI_TRUE;

    pstAlgRegCfg->stLCacRegCfg.stStaticRegCfg.bStaticResh     = HI_TRUE;
    pstAlgRegCfg->stLCacRegCfg.stUsrRegCfg.bResh              = HI_TRUE;
    pstAlgRegCfg->stLCacRegCfg.stDynaRegCfg.bResh             = HI_TRUE;
    pstAlgRegCfg->stGCacRegCfg.stStaticRegCfg.bStaticResh     = HI_TRUE;
    pstAlgRegCfg->stGCacRegCfg.stUsrRegCfg.bResh              = HI_TRUE;

    pstAlgRegCfg->stAntiFalseColorRegCfg.stStaticRegCfg.bResh = HI_TRUE;
    pstAlgRegCfg->stAntiFalseColorRegCfg.stDynaRegCfg.bResh   = HI_TRUE;

    pstAlgRegCfg->stDpRegCfg.stStaticRegCfg.bStaticResh           = HI_TRUE;
    pstAlgRegCfg->stDpRegCfg.stUsrRegCfg.stUsrDynaCorRegCfg.bResh = HI_TRUE;
    pstAlgRegCfg->stDpRegCfg.stUsrRegCfg.stUsrStaCorRegCfg.bResh  = HI_TRUE;
    pstAlgRegCfg->stDpRegCfg.stDynaRegCfg.bResh                   = HI_TRUE;

    pstAlgRegCfg->stGeRegCfg.stStaticRegCfg.bStaticResh       = HI_TRUE;
    pstAlgRegCfg->stGeRegCfg.stUsrRegCfg.bResh                = HI_TRUE;
    pstAlgRegCfg->stGeRegCfg.stDynaRegCfg.bResh               = HI_TRUE;

    pstAlgRegCfg->stLscRegCfg.stStaticRegCfg.bStaticResh      = HI_TRUE;
    pstAlgRegCfg->stLscRegCfg.stUsrRegCfg.bCoefUpdate         = HI_TRUE;
    pstAlgRegCfg->stLscRegCfg.stUsrRegCfg.bLutUpdate          = HI_TRUE;

    pstAlgRegCfg->stRLscRegCfg.stStaticRegCfg.bStaticResh     = HI_TRUE;
    pstAlgRegCfg->stRLscRegCfg.stUsrRegCfg.bCoefUpdate        = HI_TRUE;
    pstAlgRegCfg->stRLscRegCfg.stUsrRegCfg.bLutUpdate         = HI_TRUE;

    pstAlgRegCfg->stGammaCfg.stUsrRegCfg.bGammaLutUpdateEn    = HI_TRUE;
    pstAlgRegCfg->stCscCfg.stDynaRegCfg.bResh                 = HI_TRUE;

    pstAlgRegCfg->stCaRegCfg.stStaticRegCfg.bStaticResh       = HI_TRUE;
    pstAlgRegCfg->stCaRegCfg.stDynaRegCfg.bResh               = HI_TRUE;
    pstAlgRegCfg->stCaRegCfg.stUsrRegCfg.bResh                = HI_TRUE;
    pstAlgRegCfg->stCaRegCfg.stUsrRegCfg.bCaLutUpdateEn       = HI_TRUE;

    pstAlgRegCfg->stMcdsRegCfg.stStaticRegCfg.bStaticResh     = HI_TRUE;
    pstAlgRegCfg->stMcdsRegCfg.stDynaRegCfg.bDynaResh         = HI_TRUE;

    pstAlgRegCfg->stWdrRegCfg.stStaticRegCfg.bResh            = HI_TRUE;
    pstAlgRegCfg->stWdrRegCfg.stUsrRegCfg.bResh               = HI_TRUE;
    pstAlgRegCfg->stWdrRegCfg.stDynaRegCfg.bResh              = HI_TRUE;

    pstAlgRegCfg->stDrcRegCfg.stStaticRegCfg.bStaticResh      = HI_TRUE;
    pstAlgRegCfg->stDrcRegCfg.stUsrRegCfg.bUsrResh            = HI_TRUE;
    pstAlgRegCfg->stDrcRegCfg.stDynaRegCfg.bDynaResh          = HI_TRUE;

    pstAlgRegCfg->stDehazeRegCfg.stStaticRegCfg.bResh         = HI_TRUE;
    pstAlgRegCfg->stDehazeRegCfg.stDynaRegCfg.u32LutUpdate    = 1;

    pstAlgRegCfg->stBnrRegCfg.stStaticRegCfg.bResh            = HI_TRUE;
    pstAlgRegCfg->stBnrRegCfg.stDynaRegCfg.bResh              = HI_TRUE;
    pstAlgRegCfg->stBnrRegCfg.stUsrRegCfg.bResh               = HI_TRUE;

    pstAlgRegCfg->st4DgRegCfg.stStaticRegCfg.bResh            = HI_TRUE;
    pstAlgRegCfg->st4DgRegCfg.stDynaRegCfg.bResh              = HI_TRUE;
    pstAlgRegCfg->stDgRegCfg.stStaticRegCfg.bResh             = HI_TRUE;
    pstAlgRegCfg->stDgRegCfg.stDynaRegCfg.bResh               = HI_TRUE;

    pstAlgRegCfg->stPreGammaCfg.stStaticRegCfg.bStaticResh        = HI_TRUE;
    pstAlgRegCfg->stPreGammaCfg.stDynaRegCfg.bPreGammaLutUpdateEn = HI_TRUE;
    pstAlgRegCfg->stFlickerRegCfg.stDynaRegCfg.bResh              = HI_TRUE;
    pstAlgRegCfg->stLogLUTRegCfg.stStaticRegCfg.bStaticResh       = HI_TRUE;

    pstAlgRegCfg->stBeBlcCfg.bReshStatic                      = HI_TRUE;
    pstAlgRegCfg->stBeBlcCfg.bReshDyna                        = HI_TRUE;

    pstAlgRegCfg->stClutCfg.stUsrRegCfg.stClutUsrCtrlCfg.bResh = HI_TRUE;
    pstAlgRegCfg->stClutCfg.stUsrRegCfg.stClutUsrCoefCfg.bResh = HI_TRUE;

    pstAlgRegCfg->stSplitCfg.stStaticRegCfg.bResh              = HI_TRUE;

    pstAlgRegCfg->stSharpenRegCfg.stStaticRegCfg.bStaticResh             = HI_TRUE;
    pstAlgRegCfg->stSharpenRegCfg.stDynaRegCfg.stDefaultDynaRegCfg.bResh = HI_TRUE;
    pstAlgRegCfg->stSharpenRegCfg.stDynaRegCfg.stMpiDynaRegCfg.bResh     = HI_TRUE;

    return HI_SUCCESS;
}


static HI_S32 ISP_FeRegsConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo)
{
    ISP_CTX_S *pstIspCtx   = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (pstIspCtx->stWdrAttr.bMastPipe)
    {
        if (0 == ViPipe)
        {
            ISP_FeHrsRegConfig(ViPipe, pstRegCfgInfo); /*Hrs*/
        }

        /*FE alg cfgs setting to register*/
        ISP_FeAeRegConfig(ViPipe, pstRegCfgInfo);  /*Ae*/
        ISP_FeAwbRegConfig(ViPipe, pstRegCfgInfo); /*awb*/
        ISP_FeAfRegConfig(ViPipe, pstRegCfgInfo);  /*Af*/
        ISP_FeLscRegConfig(ViPipe, pstRegCfgInfo); /*LSC*/
        ISP_FeDgRegConfig(ViPipe, pstRegCfgInfo);  /*DG*/
        ISP_FeRcRegConfig(ViPipe, pstRegCfgInfo);  /*Rc*/
        ISP_FeBlcRegConfig(ViPipe, pstRegCfgInfo);
        ISP_FeLogLUTRegConfig(ViPipe, pstRegCfgInfo);

        ISP_FeSystemRegConfig(ViPipe, pstRegCfgInfo);

        ISP_FeUpdateRegConfig(ViPipe, pstRegCfgInfo);
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_BeRegsConfig(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo)
{
    HI_U32  i;
    HI_S32 s32Ret = 0;

    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    for (i = 0; i < pstRegCfgInfo->u8CfgNum; i++)
    {
        ISP_SystemRegConfig(ViPipe, pstRegCfgInfo, i); /*sys*/
        ISP_DitherRegConfig(ViPipe, pstRegCfgInfo, i); /*dither*/
        /*Be alg cfgs setting to register*/
        ISP_AeRegConfig(ViPipe, pstRegCfgInfo, i);   /*ae*/
        ISP_AwbRegConfig(ViPipe, pstRegCfgInfo, i);  /*awb*/
        ISP_AfRegConfig(ViPipe, pstRegCfgInfo, i);  /*AF*/
        ISP_SharpenRegConfig(ViPipe, pstRegCfgInfo, i); /*sharpen*/
        ISP_EdgeMarkRegConfig(ViPipe, pstRegCfgInfo, i); /*sharpen*/
        ISP_DemRegConfig(ViPipe, pstRegCfgInfo, i);  /*demosaic*/
        ISP_FpnRegConfig(ViPipe, pstRegCfgInfo, i);  /*FPN*/
        ISP_LdciRegConfig(ViPipe, pstRegCfgInfo, i); /*ldci*/
        ISP_LcacRegConfig(ViPipe, pstRegCfgInfo, i); /*Local cac*/
        ISP_GcacRegConfig(ViPipe, pstRegCfgInfo, i); /*global cac*/
        ISP_FcrRegConfig(ViPipe, pstRegCfgInfo, i);  /*FCR*/
        ISP_DpcRegConfig(ViPipe, pstRegCfgInfo, i);  /*dpc*/
        ISP_GeRegConfig(ViPipe, pstRegCfgInfo, i);   /*ge*/
        ISP_LscRegConfig(ViPipe, pstRegCfgInfo, i);  /*BE LSC*/
        ISP_RLscRegConfig(ViPipe, pstRegCfgInfo, i);  /*Radial LSC*/
        ISP_GammaRegConfig(ViPipe, pstRegCfgInfo, i); /*gamma*/
        ISP_CscRegConfig(ViPipe, pstRegCfgInfo, i);  /*csc*/
        ISP_CaRegConfig(ViPipe, pstRegCfgInfo, i);   /*ca*/
        ISP_McdsRegConfig(ViPipe, pstRegCfgInfo, i); /*mcds*/
        ISP_WdrRegConfig(ViPipe, pstRegCfgInfo, i);  /*wdr*/
        ISP_DrcRegConfig(ViPipe, pstRegCfgInfo, i);  /*drc*/
        ISP_DehazeRegConfig(ViPipe, pstRegCfgInfo, i);  /*Dehaze*/
        ISP_BayerNrRegConfig(ViPipe, pstRegCfgInfo, i); /*BayerNR*/
        ISP_DgRegConfig(ViPipe, pstRegCfgInfo, i);   /*DG*/
        ISP_4DgRegConfig(ViPipe, pstRegCfgInfo, i);   /*4DG*/
        ISP_PreGammaRegConfig(ViPipe, pstRegCfgInfo, i); /*PreGamma*/
        ISP_FlickRegConfig(ViPipe, pstRegCfgInfo, i); /*Flicker*/
        ISP_BeBlcRegConfig(ViPipe, pstRegCfgInfo, i);
        ISP_ClutRegConfig(ViPipe, pstRegCfgInfo, i); /*CLUT*/
        ISP_WdrSplitRegConfig(ViPipe, pstRegCfgInfo, i);
        ISP_LogLUTRegConfig(ViPipe, pstRegCfgInfo, i);

    }

    for (i = 0; i < pstRegCfgInfo->u8CfgNum; i++)
    {
        isp_be_reg_up_write(ViPipe, i, HI_TRUE);
        ISP_BeAlgLutUpdateRegConfig(ViPipe, pstRegCfgInfo, i);
    }

    if ((IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)) ||
        (IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)))
    {
        s32Ret = ISP_CfgBeBufCtl(ViPipe);

        if (HI_SUCCESS != s32Ret)
        {
            ISP_TRACE(HI_DBG_ERR, "Pipe:%d Be config bufs ctl failed %x!\n", ViPipe, s32Ret);
            return s32Ret;
        }
    }

    return HI_SUCCESS;
}

static HI_S32 ISP_BeRegsConfigInit(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfgInfo)
{
    HI_U32  i;

    ISP_CHECK_PIPE(ViPipe);

    for (i = 0; i < pstRegCfgInfo->u8CfgNum; i++)
    {
        ISP_SystemRegConfig(ViPipe, pstRegCfgInfo, i); /*sys*/
        ISP_DitherRegConfig(ViPipe, pstRegCfgInfo, i); /*dither*/
        /*Be alg cfgs setting to register*/
        ISP_AeRegConfig(ViPipe, pstRegCfgInfo, i);   /*ae*/
        ISP_AwbRegConfig(ViPipe, pstRegCfgInfo, i);  /*awb*/
        ISP_AfRegConfig(ViPipe, pstRegCfgInfo, i);  /*AF*/
        ISP_SharpenRegConfig(ViPipe, pstRegCfgInfo, i); /*sharpen*/
        ISP_EdgeMarkRegConfig(ViPipe, pstRegCfgInfo, i); /*sharpen*/
        ISP_DemRegConfig(ViPipe, pstRegCfgInfo, i);  /*demosaic*/
        ISP_FpnRegConfig(ViPipe, pstRegCfgInfo, i);  /*FPN*/
        ISP_LdciRegConfig(ViPipe, pstRegCfgInfo, i); /*ldci*/
        ISP_LcacRegConfig(ViPipe, pstRegCfgInfo, i); /*Local cac*/
        ISP_GcacRegConfig(ViPipe, pstRegCfgInfo, i); /*global cac*/
        ISP_FcrRegConfig(ViPipe, pstRegCfgInfo, i);  /*FCR*/
        ISP_DpcRegConfig(ViPipe, pstRegCfgInfo, i);  /*dpc*/
        ISP_GeRegConfig(ViPipe, pstRegCfgInfo, i);   /*ge*/
        ISP_LscRegConfig(ViPipe, pstRegCfgInfo, i);  /*BE LSC*/
        ISP_RLscRegConfig(ViPipe, pstRegCfgInfo, i);  /*Radial LSC*/
        ISP_GammaRegConfig(ViPipe, pstRegCfgInfo, i); /*gamma*/
        ISP_CscRegConfig(ViPipe, pstRegCfgInfo, i);  /*csc*/
        ISP_CaRegConfig(ViPipe, pstRegCfgInfo, i);   /*ca*/
        ISP_McdsRegConfig(ViPipe, pstRegCfgInfo, i); /*mcds*/
        ISP_WdrRegConfig(ViPipe, pstRegCfgInfo, i);  /*wdr*/
        ISP_DrcRegConfig(ViPipe, pstRegCfgInfo, i);  /*drc*/
        ISP_DehazeRegConfig(ViPipe, pstRegCfgInfo, i);  /*Dehaze*/
        ISP_BayerNrRegConfig(ViPipe, pstRegCfgInfo, i); /*BayerNR*/
        ISP_DgRegConfig(ViPipe, pstRegCfgInfo, i);   /*DG*/
        ISP_4DgRegConfig(ViPipe, pstRegCfgInfo, i);   /*4DG*/
        ISP_PreGammaRegConfig(ViPipe, pstRegCfgInfo, i); /*PreGamma*/
        ISP_FlickRegConfig(ViPipe, pstRegCfgInfo, i); /*Flicker*/
        ISP_BeBlcRegConfig(ViPipe, pstRegCfgInfo, i);
        ISP_ClutRegConfig(ViPipe, pstRegCfgInfo, i); /*CLUT*/
        ISP_WdrSplitRegConfig(ViPipe, pstRegCfgInfo, i);
        ISP_LogLUTRegConfig(ViPipe, pstRegCfgInfo, i);

    }

    for (i = 0; i < pstRegCfgInfo->u8CfgNum; i++)
    {
        isp_be_reg_up_write(ViPipe, i, HI_TRUE);
        ISP_BeAlgLutUpdateRegConfig(ViPipe, pstRegCfgInfo, i);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_RegCfgInit(VI_PIPE ViPipe, HI_VOID **ppCfg)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_REGCFG_S *pstRegCfg = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    REGCFG_GET_CTX(ViPipe, pstRegCfg);

    if (!pstRegCfg->bInit)
    {
        pstRegCfg->stRegCfg.unKey.u64Key = 0;

        pstRegCfg->bInit = HI_TRUE;
    }

    pstRegCfg->stRegCfg.u8CfgNum = pstIspCtx->stBlockAttr.u8BlockNum;

    *ppCfg = &pstRegCfg->stRegCfg;

    return HI_SUCCESS;
}

HI_S32 ISP_RegCfgInfoInit(VI_PIPE ViPipe)
{
    ISP_REGCFG_S *pstRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pstRegCfg);

    ISP_FeRegsConfig(ViPipe, &pstRegCfg->stRegCfg);
    ISP_BeRegsConfigInit(ViPipe, &pstRegCfg->stRegCfg);

    return HI_SUCCESS;
}

HI_S32 ISP_RegCfgInfoSet(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    ISP_REGCFG_S *pstRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pstRegCfg);

    ISP_FeRegsConfig(ViPipe, &pstRegCfg->stRegCfg);
    ISP_BeRegsConfig(ViPipe, &pstRegCfg->stRegCfg);

    if (pstRegCfg->stRegCfg.stAlgRegCfg[0].stKernelCfg.unKey.u32Key)
    {
        s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_REG_CFG_SET, &pstRegCfg->stRegCfg.stAlgRegCfg[0].stKernelCfg);

        if (HI_SUCCESS != s32Ret)
        {
            ISP_TRACE(HI_DBG_ERR, "Config ISP register Failed with ec %#x!\n", s32Ret);
            return s32Ret;
        }
    }

    return HI_SUCCESS;
}

HI_VOID ISP_SnsRegsInfoCheck(VI_PIPE ViPipe, ISP_SNS_REGS_INFO_S *pstSnsRegsInfo)
{
    if ((pstSnsRegsInfo->enSnsType >= ISP_SNS_TYPE_BUTT))
        //|| (pstSnsRegsInfo->enSnsType < ISP_SNS_I2C_TYPE))
    {
        ISP_TRACE(HI_DBG_ERR, "senor's regs info invalid, enSnsType %d\n", pstSnsRegsInfo->enSnsType);
        return;
    }

    if (pstSnsRegsInfo->u32RegNum > ISP_MAX_SNS_REGS)
    {
        ISP_TRACE(HI_DBG_ERR, "senor's regs info invalid, u32RegNum %d\n", pstSnsRegsInfo->u32RegNum);
        return;
    }

    return;
}

HI_S32 ISP_SyncCfgSet(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    HI_S32 s32PipeSt = 0;
    HI_S32 s32PipeEd = 0;
    HI_S8 s8StitchMainPipe;
    ISP_CTX_S *pstIspCtx   = HI_NULL;
    ISP_REGCFG_S *pstRegCfg = HI_NULL;
    ISP_SNS_REGS_INFO_S *pstSnsRegsInfo = NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (HI_TRUE == pstIspCtx->stStitchAttr.bStitchEnable)
    {
        s8StitchMainPipe = pstIspCtx->stStitchAttr.as8StitchBindId[0];

        if (IS_STITCH_MAIN_PIPE(ViPipe, s8StitchMainPipe))
        {
            s32PipeSt = 0;
            s32PipeEd = pstIspCtx->stStitchAttr.u8StitchPipeNum - 1;
        }
        else
        {
            s32PipeSt = ViPipe;
            s32PipeEd = ViPipe - 1;
        }
    }
    else
    {
        s32PipeSt = ViPipe;
        s32PipeEd = ViPipe;
    }

    while (s32PipeSt <= s32PipeEd)
    {
        if (HI_TRUE == pstIspCtx->stStitchAttr.bStitchEnable)
        {
            ViPipe = pstIspCtx->stStitchAttr.as8StitchBindId[s32PipeSt];
        }
        else
        {
            ViPipe = s32PipeSt;
        }

        ISP_GET_CTX(ViPipe, pstIspCtx);
        REGCFG_GET_CTX(ViPipe, pstRegCfg);
        ISP_CHECK_OPEN(ViPipe);

        if (HI_SUCCESS != ISP_SensorUpdateSnsReg(ViPipe))
        {
            /* If Users need to config AE sync info themselves, they can set pfn_cmos_get_sns_reg_info to NULL in cmos.c */
            /* Then there will be NO AE sync configs in kernel of firmware */
            return HI_SUCCESS;
        }

        ISP_SensorGetSnsReg(ViPipe, &pstSnsRegsInfo);
        memcpy(&pstRegCfg->stSyncCfgNode.stSnsRegsInfo, pstSnsRegsInfo, sizeof(ISP_SNS_REGS_INFO_S));
        ISP_SnsRegsInfoCheck(ViPipe, &pstRegCfg->stSyncCfgNode.stSnsRegsInfo);
        memcpy(&pstRegCfg->stSyncCfgNode.stAERegCfg, &pstRegCfg->stRegCfg.stAlgRegCfg[0].stAeRegCfg2, sizeof(ISP_AE_REG_CFG_2_S));
        memcpy(&pstRegCfg->stSyncCfgNode.stDRCRegCfg, &pstRegCfg->stRegCfg.stAlgRegCfg[0].stDrcRegCfg.stSyncRegCfg, sizeof(ISP_DRC_REG_CFG_2_S));
        memcpy(&pstRegCfg->stSyncCfgNode.stWDRRegCfg, &pstRegCfg->stRegCfg.stAlgRegCfg[0].stWdrRegCfg.stSyncRegCfg, sizeof(ISP_FSWDR_REG_CFG_2_S));

        if (HI_TRUE == pstIspCtx->stStitchAttr.bStitchEnable)
        {
            s8StitchMainPipe = pstIspCtx->stStitchAttr.as8StitchBindId[0];

            if (!IS_STITCH_MAIN_PIPE(ViPipe, s8StitchMainPipe))
            {
                memcpy(&pstRegCfg->stSyncCfgNode.stSnsRegsInfo, &g_astRegCfgCtx[s8StitchMainPipe].stSyncCfgNode.stSnsRegsInfo, sizeof(ISP_SNS_REGS_INFO_S));
                memcpy(&pstRegCfg->stSyncCfgNode.stSnsRegsInfo.unComBus, &pstSnsRegsInfo->unComBus, sizeof(ISP_SNS_COMMBUS_U));
                memcpy(&pstRegCfg->stSyncCfgNode.stSnsRegsInfo.stSlvSync.u32SlaveBindDev, &pstSnsRegsInfo->stSlvSync.u32SlaveBindDev, sizeof(HI_U32));
                memcpy(&pstRegCfg->stSyncCfgNode.stAERegCfg, &g_astRegCfgCtx[s8StitchMainPipe].stRegCfg.stAlgRegCfg[0].stAeRegCfg2, sizeof(ISP_AE_REG_CFG_2_S));
            }

        }

        pstRegCfg->stSyncCfgNode.bValid = HI_TRUE;

        s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_SYNC_CFG_SET, &pstRegCfg->stSyncCfgNode);

        if (HI_SUCCESS != s32Ret)
        {
            //printf("Config Sync register Failed with ec %#x!\n", s32Ret);
            return s32Ret;
        }

        pstSnsRegsInfo->bConfig = HI_TRUE;

        s32PipeSt++;
    }

    return HI_SUCCESS;
}


HI_S32 ISP_SnapRegCfgSet(VI_PIPE ViPipe, ISP_CONFIG_INFO_S *pstSnapInfo)
{
    HI_S32 s32Ret;

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_SNAP_INFO_SET, pstSnapInfo);

    if (HI_SUCCESS != s32Ret)
    {
        //printf("Config Sync register Failed with ec %#x!\n", s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_SnapRegCfgGet(VI_PIPE ViPipe, ISP_SNAP_INFO_S *pstSnapInfo)
{
    HI_S32 s32Ret;

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_SNAP_INFO_GET, pstSnapInfo);

    if (HI_SUCCESS != s32Ret)
    {
        //printf("Config Sync register Failed with ec %#x!\n", s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

HI_BOOL ISP_ProTriggerGet(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    HI_BOOL bEnable;

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_PRO_TRIGGER_GET, &bEnable);

    if (HI_SUCCESS != s32Ret)
    {
        //printf("Config Sync register Failed with ec %#x!\n", s32Ret);
        return HI_FALSE;
    }

    return bEnable;
}

HI_S32 ISP_RegCfgCtrl(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_CTX_S    *pstIspCtx = HI_NULL;
    ISP_REGCFG_S *pstRegCfg = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    REGCFG_GET_CTX(ViPipe, pstRegCfg);

    pstRegCfg->stRegCfg.unKey.u64Key  = 0xFFFFFFFFFFFFFFFF;

    for (i = 0; i < pstIspCtx->stBlockAttr.u8BlockNum; i++)
    {
        pstIspCtx->stSpecialOpt.abBeOnSttUpdate[i] = HI_TRUE;
    }

    for (i = pstIspCtx->stBlockAttr.u8PreBlockNum; i <  pstIspCtx->stBlockAttr.u8BlockNum; i++)
    {
        memcpy(&pstRegCfg->stRegCfg.stAlgRegCfg[i], &pstRegCfg->stRegCfg.stAlgRegCfg[0], sizeof(ISP_ALG_REG_CFG_S));
    }

    if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
        || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
    {
        for (i = pstIspCtx->stBlockAttr.u8PreBlockNum; i <  pstIspCtx->stBlockAttr.u8BlockNum; i++)
        {
            ISP_BeReshCfg(&pstRegCfg->stRegCfg.stAlgRegCfg[i]);
        }
    }

    return HI_SUCCESS;
}

HI_S32 ISP_SwitchWdrRegSet(VI_PIPE ViPipe)
{
    ISP_REGCFG_S *pstRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pstRegCfg);

    ISP_FeRegsConfig(ViPipe, &pstRegCfg->stRegCfg);

    return HI_SUCCESS;
}

HI_S32 ISP_SwitchResRegSet(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (IS_ONLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)\
        || IS_SIDEBYSIDE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode))
    {
        ISP_RegCfgInfoSet(ViPipe);
        return HI_SUCCESS;
    }

    /* record the register config infomation to fhy and kernel,and be valid in next frame. */
    s32Ret = ISP_RegCfgInfoInit(ViPipe);
    if (s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ISP_AllCfgsBeBufInit(ViPipe);

    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "Pipe:%d init all be bufs failed %x!\n", ViPipe, s32Ret);
        return s32Ret;
    }

    pstIspCtx->stBlockAttr.u8PreBlockNum = pstIspCtx->stBlockAttr.u8BlockNum;

    return HI_SUCCESS;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

