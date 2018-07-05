
/******************************************************************************
Copyright (C), 2016-2018, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : vpss_ext.h
Version       : Initial Draft
Author        : Hisilicon multimedia software group
Created       : 2016/09/27
Last Modified :
Description   :
Function List :
******************************************************************************/

#include "hi_type.h"
#include "mod_ext.h"
#include "hi_comm_sys.h"
#include "hi_comm_vpss.h"
#include "hi_comm_vo.h"

#include "mkp_sys.h"

#ifndef __VPSS_EXT_H__
#define __VPSS_EXT_H__

typedef struct HI_VPSS_LOW_DELAY_CFG_S
{
    HI_BOOL bTunlFrame;
    HI_U64  u64TunlPhyAddr;
    HI_VOID* pTunlVirAddr;
    HI_VOID* pbFrameOk;
} VPSS_LOW_DELAY_CFG_S;

/* 80 bytes low delay information attached to the frame buffer */
/* [2       |30      |2         |30      |4       |4         |8      ]*/
/*  Yheight |reserve |Cheight   |reserve |BFrameOk|BTunlFrame|reserve*/
typedef struct HI_VPSS_LOW_DELAY_VB_INFO_S
{
    HI_U16 u16YHeight;
    HI_U16 u16Reserve1[15];
    HI_U16 u16CHeight;
    HI_U16 u16Reserve2[15];
    HI_U32 u32IsFrameOk;
    HI_U32 u32IsTunlFrame;
    HI_U16 u16Reserve3[4];
}VPSS_LOW_DELAY_VB_INFO_S;

typedef struct HI_VPSS_PIC_INFO_S
{
    VIDEO_FRAME_INFO_S stVideoFrame;
    MOD_ID_E        enModId;
    //VPSS_PRESCALE_E                enVpssPresclType;
    //HI_BOOL bFlashed;               /* Flashed Video frame or not. */
    HI_BOOL bBlock;                 /* Flashed Video frame or not. */
    HI_BOOL bVGS;
    HI_BOOL bGDC;
    VPSS_LOW_DELAY_CFG_S stLowDelayCfg;             /* low delay Video info */
} VPSS_PIC_INFO_S;


typedef struct HI_VPSS_QUERY_INFO_S
{
    VPSS_PIC_INFO_S* pstSrcPicInfo;     /*information of source pic*/
    VPSS_PIC_INFO_S* pstOldPicInfo;     /*information of backup pic*/
    HI_BOOL          bScaleCap;         /*whether scaling*/
    HI_BOOL          bTransCap;         /*whether the frame rate is doubled*/
    HI_BOOL          bMalloc;           /*whether malloc frame buffer*/
} VPSS_QUERY_INFO_S;

typedef struct HI_VPSS_INST_INFO_S
{
    HI_BOOL         bNew;               /*whether use new pic to query*/
    HI_BOOL         bVpss;              /*whether vpss need to process*/
    HI_BOOL         bDouble;            /*whether the frame rate is doubled */
    HI_BOOL         bUpdate;            /*whether update backup pic*/
    COMPRESS_MODE_E enCompressMode;     /*compress mode*/
    VPSS_PIC_INFO_S astDestPicInfo[2];
    ASPECT_RATIO_S  stAspectRatio;      /*aspect ratio configuration*/
    VO_PART_MODE_E  enPartMode;
} VPSS_INST_INFO_S;


typedef struct HI_VPSS_SEND_INFO_S
{
    HI_BOOL           bSuc;                  /*Whether successful completion*/
    VPSS_GRP          VpssGrp;
    VPSS_CHN          VpssChn;
    VPSS_PIC_INFO_S*   pstDestPicInfo[2];    /*pic processed by vpss.0:Top field 1:Bottom field*/
} VPSS_SEND_INFO_S;


typedef struct HI_VPSS_REGISTER_INFO_S
{
    MOD_ID_E    enModId;
    HI_S32      (*pVpssQuery)(HI_S32 s32DevId, HI_S32 s32ChnId, VPSS_QUERY_INFO_S*  pstQueryInfo, VPSS_INST_INFO_S* pstInstInfo);
    HI_S32      (*pVpssSend)(HI_S32 s32DevId, HI_S32 s32ChnId, VPSS_SEND_INFO_S*  pstSendInfo);
    HI_S32      (*pResetCallBack)(HI_S32 s32DevId, HI_S32 s32ChnId, HI_VOID* pvData);

} VPSS_REGISTER_INFO_S;

typedef struct hiVPSS_IP_ENABLE_S
{
    HI_BOOL bVpssEn[VPSS_IP_NUM];
} VPSS_IP_ENABLE_S;


typedef struct hiVPSS_EXPORT_FUNC_S
{
    HI_S32  (*pfnVpssRegister)(VPSS_REGISTER_INFO_S* pstInfo);
    HI_S32  (*pfnVpssUnRegister)(MOD_ID_E enModId);
    HI_S32  (*pfnVpssUpdateViVpssMode)(VI_VPSS_MODE_S* pstViVpssMode);
    HI_S32  (*pfnVpssViSubmitTask)(VPSS_GRP VpssGrp, VIDEO_FRAME_INFO_S* pstViFrame);
    HI_S32  (*pfnVpssViStartTask)(HI_U32 VPSSID);
    HI_S32  (*pfnVpssViTaskDone)(HI_S32 VPSSID, HI_S32 s32State);
    HI_S32  (*pfnVpssGetChnVideoFormat)(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VIDEO_FORMAT_E *penVideoFormat);
    HI_S32  (*pfnVpssGetIpEn)(VPSS_IP_ENABLE_S* pstVpssIpEn);
    HI_S32  (*pfnVpssDownSemaphore)(VPSS_GRP VpssGrp);
    HI_S32  (*pfnVpssUpSemaphore)(VPSS_GRP VpssGrp);
    HI_BOOL (*pfnVpssIsGrpExisted)(VPSS_GRP VpssGrp);
} VPSS_EXPORT_FUNC_S;

#define CKFN_VPSS_ENTRY()  CHECK_FUNC_ENTRY(HI_ID_VPSS)


#define CKFN_VPSS_Register() \
    (NULL != FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssRegister)
#define CALL_VPSS_Register(pstInfo) \
    FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssRegister(pstInfo)


#define CKFN_VPSS_UnRegister() \
    (NULL != FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssUnRegister)
#define CALL_VPSS_UnRegister(enModId) \
    FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssUnRegister(enModId)


#define CKFN_VPSS_UpdateViVpssMode() \
   (NULL != FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssUpdateViVpssMode)
#define CALL_VPSS_UpdateViVpssMode(pstViVpssMode) \
    FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssUpdateViVpssMode(pstViVpssMode)

#define CKFN_VPSS_VI_SubmitTask() \
   (NULL != FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssViSubmitTask)
#define CALL_VPSS_VI_SubmitTask(VpssGrp, pstViFrame) \
    FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssViSubmitTask(VpssGrp, pstViFrame)

#define CKFN_VPSS_VI_StartTask() \
   (NULL != FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssViStartTask)
#define CALL_VPSS_VI_StartTask(VPSSID) \
    FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssViStartTask(VPSSID)

#define CKFN_VPSS_VI_TaskDone() \
   (NULL != FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssViTaskDone)
#define CALL_VPSS_VI_TaskDone(VPSSID, s32State) \
    FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssViTaskDone(VPSSID, s32State)

#define CKFN_VPSS_GetChnVideoFormat() \
   (NULL != FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssGetChnVideoFormat)
#define CALL_VPSS_GetChnVideoFormat(VpssGrp, VpssChn, penVideoFormat) \
    FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssGetChnVideoFormat(VpssGrp, VpssChn, penVideoFormat)

#define CKFN_VPSS_GetIpEn() \
   (NULL != FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssGetIpEn)
#define CALL_VPSS_GetIpEn(pstVpssIpEn) \
    FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssGetIpEn(pstVpssIpEn)

#define CKFN_VPSS_DownSemaphore() \
    (NULL != FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssDownSemaphore)
#define CALL_VPSS_DownSemaphore(VpssGrp) \
    FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssDownSemaphore(VpssGrp)

#define CKFN_VPSS_UpSemaphore() \
    (NULL != FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssUpSemaphore)
#define CALL_VPSS_UpSemaphore(VpssGrp) \
    FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssUpSemaphore(VpssGrp)

#define CKFN_VPSS_IsGrpExisted() \
    (NULL != FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssIsGrpExisted)
#define CALL_VPSS_IsGrpExisted(VpssGrp) \
    FUNC_ENTRY(VPSS_EXPORT_FUNC_S, HI_ID_VPSS)->pfnVpssIsGrpExisted(VpssGrp)

#endif /* __VPSS_EXT_H__ */




