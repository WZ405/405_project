/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_frameinfo.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       :
  Description   :
  History       :
  1.Date        :
    Author      :
    Modification: Created file

******************************************************************************/
#ifndef __ISP_FRAMEINFO_H__
#define __ISP_FRAMEINFO_H__

#include "hi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct hiISP_FRAME_INFO_CTRL_S
{
    HI_BOOL                     bSupportFrame;
    HI_U64                      u64FrameInfoPhyaddr;
    ISP_FRAME_INFO_S            *pstIspFrame;
} ISP_FRAME_INFO_CTRL_S;


typedef struct hiISP_ATTACH_INFO_CTRL_S
{
    HI_BOOL                     bSupportAttach;
    HI_U64                      u64AttachPhyaddr;
    ISP_ATTACH_INFO_S           *pstAttachInfo;
} ISP_ATTACH_INFO_CTRL_S;

typedef struct hiISP_COLORGAMMUT_INFO_CTRL_S
{
    HI_BOOL                     bSupportColorGamut;
    HI_U64                      u64ColorGamutPhyaddr;
    ISP_COLORGAMMUT_INFO_S           *pstColorGamutInfo;
} ISP_COLORGAMMUT_INFO_CTRL_S;

typedef struct hiISP_PRO_NR_PARAM_CTRL_S
{
    HI_BOOL                     bSupportProNrParam;
    HI_U64                      u64ProNrParamPhyaddr;
    ISP_PRO_NR_PARAM_S         *pstProNrParam;
} ISP_PRO_NR_PARAM_CTRL_S;

typedef struct hiISP_PRO_SHP_PARAM_CTRL_S
{
    HI_BOOL                     bSupportProShpParam;
    HI_U64                      u64ProShpParamPhyaddr;
    ISP_PRO_SHP_PARAM_S         *pstProShpParam;
} ISP_PRO_SHP_PARAM_CTRL_S;

HI_S32 ISP_FrameInfoInit(VI_PIPE ViPipe, ISP_FRAME_INFO_CTRL_S *pstFrameInfoCtrl);
HI_S32 ISP_FrameInfoExit(VI_PIPE ViPipe, ISP_FRAME_INFO_CTRL_S *pstFrameInfoCtrl);

HI_S32 ISP_AttachInfoInit(VI_PIPE ViPipe, ISP_ATTACH_INFO_CTRL_S *pstAttachInfoCtrl);
HI_S32 ISP_AttachInfoExit(VI_PIPE ViPipe, ISP_ATTACH_INFO_CTRL_S *pstAttachInfoCtrl);



HI_S32 ISP_ColorGamutInfoInit(VI_PIPE ViPipe, ISP_COLORGAMMUT_INFO_CTRL_S *pstColorGamutInfoCtrl);
HI_S32 ISP_ColorGamutInfoExit(VI_PIPE ViPipe, ISP_COLORGAMMUT_INFO_CTRL_S *pstColorGamutInfoCtrl);
HI_S32 ISP_ProNrParamInit(VI_PIPE ViPipe, ISP_PRO_NR_PARAM_CTRL_S *pstProNrParamCtrl);
HI_S32 ISP_ProNrParamExit(VI_PIPE ViPipe, ISP_PRO_NR_PARAM_CTRL_S *pstProNrParamCtrl);
HI_S32 ISP_ProShpParamInit(VI_PIPE ViPipe, ISP_PRO_SHP_PARAM_CTRL_S *pstProShpParamCtrl);
HI_S32 ISP_ProShpParamExit(VI_PIPE ViPipe, ISP_PRO_SHP_PARAM_CTRL_S *pstProShpParamCtrl);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif
