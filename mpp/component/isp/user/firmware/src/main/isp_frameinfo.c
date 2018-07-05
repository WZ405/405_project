/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_frameinfo.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       :
  Description   :
  History       :
  1.Date        :
    Author      :
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <sys/ioctl.h>
#include "mkp_isp.h"
#include "mpi_sys.h"
#include "isp_frameinfo.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

extern HI_S32     g_as32IspFd[ISP_MAX_PIPE_NUM];
HI_S32 ISP_FrameInfoInit(VI_PIPE ViPipe, ISP_FRAME_INFO_CTRL_S *pstFrameInfoCtrl)
{
    HI_S32 s32Ret;
    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_POINTER(pstFrameInfoCtrl);

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_FRAME_INFO_BUF_INIT, &pstFrameInfoCtrl->u64FrameInfoPhyaddr);
    if (HI_SUCCESS != s32Ret)
    {
        pstFrameInfoCtrl->bSupportFrame = HI_FALSE;

        if (HI_ERR_ISP_NOT_SUPPORT == s32Ret)
        {
            s32Ret = HI_SUCCESS;
        }
        else
        {
            printf("init frame bufs failed %x!\n", s32Ret);
        }
        return s32Ret;
    }

    pstFrameInfoCtrl->pstIspFrame = HI_MPI_SYS_Mmap(pstFrameInfoCtrl->u64FrameInfoPhyaddr,
                                    sizeof(ISP_FRAME_INFO_S) * ISP_MAX_FRAMEINFO_BUF_NUM);
    if (HI_NULL == pstFrameInfoCtrl->pstIspFrame)
    {
        printf("mmap frame info buf failed!\n");
        s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_FRAME_INFO_BUF_EXIT);

        if (HI_SUCCESS != s32Ret)
        {
            printf("exit frame info buf failed %x!\n", s32Ret);
            return s32Ret;
        }

        return HI_ERR_ISP_NOMEM;
    }

    pstFrameInfoCtrl->bSupportFrame = HI_TRUE;

    return HI_SUCCESS;
}

HI_S32 ISP_FrameInfoExit(VI_PIPE ViPipe, ISP_FRAME_INFO_CTRL_S *pstFrameInfoCtrl)
{
    HI_S32 s32Ret;
    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_POINTER(pstFrameInfoCtrl);

    if (HI_FALSE == pstFrameInfoCtrl->bSupportFrame)
    {
        return HI_SUCCESS;
    }

    if (HI_NULL != pstFrameInfoCtrl->pstIspFrame)
    {
        HI_MPI_SYS_Munmap(pstFrameInfoCtrl->pstIspFrame, sizeof(ISP_FRAME_INFO_S) * ISP_MAX_FRAMEINFO_BUF_NUM);
        pstFrameInfoCtrl->pstIspFrame = HI_NULL;
    }

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_FRAME_INFO_BUF_EXIT);
    if (HI_SUCCESS != s32Ret)
    {
        printf("exit frame info buf failed %x!\n", s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_AttachInfoInit(VI_PIPE ViPipe, ISP_ATTACH_INFO_CTRL_S *pstAttachInfoCtrl)
{
    HI_S32 s32Ret;
    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_POINTER(pstAttachInfoCtrl);

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_ATTACH_INFO_BUF_INIT, &pstAttachInfoCtrl->u64AttachPhyaddr);
    if (HI_SUCCESS != s32Ret)
    {
        pstAttachInfoCtrl->bSupportAttach = HI_FALSE;

        if (HI_ERR_ISP_NOT_SUPPORT == s32Ret)
        {
            s32Ret = HI_SUCCESS;
        }
        else
        {
            printf("init attach bufs failed %x!\n", s32Ret);
        }
        return s32Ret;
    }

    pstAttachInfoCtrl->pstAttachInfo = HI_MPI_SYS_Mmap(pstAttachInfoCtrl->u64AttachPhyaddr,
                                       sizeof(ISP_ATTACH_INFO_S));
    if (HI_NULL == pstAttachInfoCtrl->pstAttachInfo)
    {
        printf("mmap attach info buf failed!\n");

        s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_ATTACH_INFO_BUF_EXIT);
        if (HI_SUCCESS != s32Ret)
        {
            printf("exit attach info buf failed %x!\n", s32Ret);
            return s32Ret;
        }
        return HI_ERR_ISP_NOMEM;
    }
    pstAttachInfoCtrl->bSupportAttach = HI_TRUE;

    return HI_SUCCESS;
}

HI_S32 ISP_AttachInfoExit(VI_PIPE ViPipe, ISP_ATTACH_INFO_CTRL_S *pstAttachInfoCtrl)
{
    HI_S32 s32Ret;
    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_POINTER(pstAttachInfoCtrl);

    if (HI_FALSE == pstAttachInfoCtrl->bSupportAttach)
    {
        return HI_SUCCESS;
    }

    if (HI_NULL != pstAttachInfoCtrl->pstAttachInfo)
    {
        HI_MPI_SYS_Munmap(pstAttachInfoCtrl->pstAttachInfo, sizeof(ISP_ATTACH_INFO_S));
        pstAttachInfoCtrl->pstAttachInfo = HI_NULL;
    }

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_ATTACH_INFO_BUF_EXIT);
    if (HI_SUCCESS != s32Ret)
    {
        printf("exit attach info buf failed %x!\n", s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}




HI_S32 ISP_ColorGamutInfoInit(VI_PIPE ViPipe, ISP_COLORGAMMUT_INFO_CTRL_S *pstColorGamutInfoCtrl)
{
    HI_S32 s32Ret;
    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_POINTER(pstColorGamutInfoCtrl);

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_COLORGAMUT_INFO_BUF_INIT, &pstColorGamutInfoCtrl->u64ColorGamutPhyaddr);
    if (HI_SUCCESS != s32Ret)
    {
        pstColorGamutInfoCtrl->bSupportColorGamut = HI_FALSE;

        if (HI_ERR_ISP_NOT_SUPPORT == s32Ret)
        {
            s32Ret = HI_SUCCESS;
        }
        else
        {
            printf("init color gamut bufs failed %x!\n", s32Ret);
        }
        return s32Ret;
    }

    pstColorGamutInfoCtrl->pstColorGamutInfo = HI_MPI_SYS_Mmap(pstColorGamutInfoCtrl->u64ColorGamutPhyaddr,
            sizeof(ISP_COLORGAMMUT_INFO_S));

    if (HI_NULL == pstColorGamutInfoCtrl->pstColorGamutInfo)
    {
        printf("mmap color gamut info buf failed!\n");

        s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_COLORGAMUT_INFO_BUF_EXIT);
        if (HI_SUCCESS != s32Ret)
        {
            printf("Exit color gamut bufs failed %x!\n", s32Ret);
            return s32Ret;
        }

        return HI_ERR_ISP_NOMEM;
    }
    pstColorGamutInfoCtrl->bSupportColorGamut = HI_TRUE;

    return HI_SUCCESS;
}


HI_S32 ISP_ColorGamutInfoExit(VI_PIPE ViPipe, ISP_COLORGAMMUT_INFO_CTRL_S *pstColorGamutInfoCtrl)
{
    HI_S32 s32Ret;
    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_POINTER(pstColorGamutInfoCtrl);

    if (HI_FALSE == pstColorGamutInfoCtrl->bSupportColorGamut)
    {
        return HI_SUCCESS;
    }

    if (HI_NULL != pstColorGamutInfoCtrl->pstColorGamutInfo)
    {
        HI_MPI_SYS_Munmap(pstColorGamutInfoCtrl->pstColorGamutInfo, sizeof(ISP_COLORGAMMUT_INFO_S));
        pstColorGamutInfoCtrl->pstColorGamutInfo = HI_NULL;
    }

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_COLORGAMUT_INFO_BUF_EXIT);
    if (HI_SUCCESS != s32Ret)
    {
        printf("exit color gamut info buf failed %x!\n", s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_ProNrParamInit(VI_PIPE ViPipe, ISP_PRO_NR_PARAM_CTRL_S *pstProNrParamCtrl)
{
    HI_S32 s32Ret;
    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_POINTER(pstProNrParamCtrl);
    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_PRO_NR_PARAM_BUF_INIT, &pstProNrParamCtrl->u64ProNrParamPhyaddr);
    if (HI_SUCCESS != s32Ret)
    {
        pstProNrParamCtrl->bSupportProNrParam = HI_FALSE;
        if (HI_ERR_ISP_NOT_SUPPORT == s32Ret)
        {
            s32Ret = HI_SUCCESS;
        }
        else
        {
            printf("init pro nr paramt bufs failed %x!\n", s32Ret);
        }
        return s32Ret;
    }
    pstProNrParamCtrl->pstProNrParam = HI_MPI_SYS_Mmap(pstProNrParamCtrl->u64ProNrParamPhyaddr,
                                       sizeof(ISP_PRO_NR_PARAM_S));
    if (HI_NULL == pstProNrParamCtrl->pstProNrParam)
    {
        printf("mmap pro nr paramt buf failed!\n");
        s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_PRO_NR_PARAM_BUF_EXIT);
        if (HI_SUCCESS != s32Ret)
        {
            printf("init nr paramt bufs failed %x!\n", s32Ret);
            return s32Ret;
        }
        return HI_ERR_ISP_NOMEM;
    }
    pstProNrParamCtrl->bSupportProNrParam = HI_TRUE;
    return HI_SUCCESS;
}
HI_S32 ISP_ProNrParamExit(VI_PIPE ViPipe, ISP_PRO_NR_PARAM_CTRL_S *pstProNrParamCtrl)
{
    HI_S32 s32Ret;
    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_POINTER(pstProNrParamCtrl);
    if (HI_FALSE == pstProNrParamCtrl->bSupportProNrParam)
    {
        return HI_SUCCESS;
    }
    if (HI_NULL != pstProNrParamCtrl->pstProNrParam)
    {
        HI_MPI_SYS_Munmap(pstProNrParamCtrl->pstProNrParam, sizeof(ISP_PRO_NR_PARAM_S));
        pstProNrParamCtrl->pstProNrParam = HI_NULL;
    }
    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_PRO_NR_PARAM_BUF_EXIT);
    if (HI_SUCCESS != s32Ret)
    {
        printf("exit pro nr paramt buf failed %x!\n", s32Ret);
        return s32Ret;
    }
    return HI_SUCCESS;
}
HI_S32 ISP_ProShpParamInit(VI_PIPE ViPipe, ISP_PRO_SHP_PARAM_CTRL_S *pstProShpParamCtrl)
{
    HI_S32 s32Ret;
    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_POINTER(pstProShpParamCtrl);
    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_PRO_SHP_PARAM_BUF_INIT, &pstProShpParamCtrl->u64ProShpParamPhyaddr);
    if (HI_SUCCESS != s32Ret)
    {
        pstProShpParamCtrl->bSupportProShpParam = HI_FALSE;
        if (HI_ERR_ISP_NOT_SUPPORT == s32Ret)
        {
            s32Ret = HI_SUCCESS;
        }
        else
        {
            printf("init pro shp paramt bufs failed %x!\n", s32Ret);
        }
        return s32Ret;
    }
    pstProShpParamCtrl->pstProShpParam = HI_MPI_SYS_Mmap(pstProShpParamCtrl->u64ProShpParamPhyaddr,
                                         sizeof(ISP_PRO_SHP_PARAM_S));
    if (HI_NULL == pstProShpParamCtrl->pstProShpParam)
    {
        printf("mmap pro shp paramt buf failed!\n");
        s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_PRO_SHP_PARAM_BUF_EXIT);
        if (HI_SUCCESS != s32Ret)
        {
            printf("init nr paramt bufs failed %x!\n", s32Ret);
            return s32Ret;
        }
        return HI_ERR_ISP_NOMEM;
    }
    pstProShpParamCtrl->bSupportProShpParam = HI_TRUE;
    return HI_SUCCESS;
}
HI_S32 ISP_ProShpParamExit(VI_PIPE ViPipe, ISP_PRO_SHP_PARAM_CTRL_S *pstProShpParamCtrl)
{
    HI_S32 s32Ret;
    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_POINTER(pstProShpParamCtrl);
    if (HI_FALSE == pstProShpParamCtrl->bSupportProShpParam)
    {
        return HI_SUCCESS;
    }
    if (HI_NULL != pstProShpParamCtrl->pstProShpParam)
    {
        HI_MPI_SYS_Munmap(pstProShpParamCtrl->pstProShpParam, sizeof(ISP_PRO_SHP_PARAM_S));
        pstProShpParamCtrl->pstProShpParam = HI_NULL;
    }
    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_PRO_SHP_PARAM_BUF_EXIT);
    if (HI_SUCCESS != s32Ret)
    {
        printf("exit pro shp paramt buf failed %x!\n", s32Ret);
        return s32Ret;
    }
    return HI_SUCCESS;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

