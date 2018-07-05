/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : pciv_firmware.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/07/16
  Description   :
  History       :
  1.Date        : 2009/07/16
    Author      : Z44949
    Modification: Created file
  2.Date        : 2009/10/13
    Author      : P00123320
    Modification: Start timer when the front is vdec
  3.Date        : 2009/10/15
    Author      : P00123320
    Modification: Add the Synchronization mechanism between VDEC and VO
  4.Date        : 2010/2/24
    Author      : P00123320
    Modification:
  5.Date        : 2010/4/14
    Author      : P00123320
    Modification:
  6.Date        : 2010/11/12
    Author      : P00123320
    Modification: Add the Osd function
******************************************************************************/
//#include "hi_osal.h"
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/module.h>

#include "hi_osal.h"

#include "hi_common.h"
#include "hi_comm_pciv.h"
#include "hi_comm_region.h"
#include "hi_comm_vo.h"
#include "pciv_firmware.h"
#include "mod_ext.h"
#include "dev_ext.h"
#include "vb_ext.h"
#include "vgs_ext.h"
#include "sys_ext.h"
#include "proc_ext.h"
#include "pciv_fmwext.h"
#include "pciv_pic_queue.h"
#include "vpss_ext.h"
#include "region_ext.h"

#define  PCIVFMW_STATE_STARTED   0
#define  PCIVFMW_STATE_STOPPING  1
#define  PCIVFMW_STATE_STOPED    2

/*get the function entrance*/
#define FUNC_ENTRANCE(type,id) ((type*)(g_astModules[id].pstExportFuncs))
static PCIV_VBPOOL_S     g_stVbPool;

typedef enum hiPCIVFMW_SEND_STATE_E
{
    PCIVFMW_SEND_OK = 0,
    PCIVFMW_SEND_NOK,
    PCIVFMW_SEND_ING,
    PCIVFMW_SEND_BUTT
} PCIVFMW_SEND_STATE_E;

typedef struct hiPCIV_FMWCHANNEL_S
{
    HI_BOOL  bCreate;
    HI_BOOL  bStart;
    HI_BOOL  bBlock;
	HI_BOOL  bMaster;		   /* the flag of master or not */

    HI_U32 u32RgnNum;          /* number of channel region */
    HI_U32 u32TimeRef;         /*The serial number of VI source Image*/
    HI_U32 u32GetCnt;          /*The times of sender get VI Image, or the receiver get the Image fo VO's Dispay*/
    HI_U32 u32SendCnt;         /*The times of sender send the Image, or the receiver send to VO displaying*/
    HI_U32 u32RespCnt;         /*The times of sender finish sending the Image and releasing, or the receiver finsh sending to VO displaying*/
    HI_U32 u32LostCnt;         /*The times of sender fail sending the Image , or the receiver fail sending to VO displaying*/
    HI_U32 u32TimerCnt;        /*The times of the timer runing to send VDEC Image*/

    HI_U32 u32AddJobSucCnt;    /*Success submitting the job*/
    HI_U32 u32AddJobFailCnt;   /*fail submitting the job*/

    HI_U32 u32MoveTaskSucCnt;  /*Move Task success*/
    HI_U32 u32MoveTaskFailCnt; /*Move Task fail*/

    HI_U32 u32OsdTaskSucCnt;   /*osd  task success*/
    HI_U32 u32OsdTaskFailCnt;  /*osd task  fail*/

    HI_U32 u32ZoomTaskSucCnt;  /*zoom task success*/
    HI_U32 u32ZoomTaskFailCnt; /*zoom task fail*/

    HI_U32 u32EndJobSucCnt;    /*vgs end job success*/
    HI_U32 u32EndJobFailCnt;   /*vgs end job fail*/

    HI_U32 u32MoveCbCnt;       /*vgs move callback success*/
    HI_U32 u32OsdCbCnt;        /*vgs osd callback success*/
    HI_U32 u32ZoomCbCnt;       /*vgs zoom callback success*/

    HI_U32 u32NewDoCnt;
    HI_U32 u32OldUndoCnt;

    PCIVFMW_SEND_STATE_E enSendState;
	MOD_ID_E             enModId;

    PCIV_PIC_QUEUE_S stPicQueue;   /*vdec image queue*/
    PCIV_PIC_NODE_S *pCurVdecNode; /*the current vdec node*/

	/*record the tartget Image attr after zoom*/
    PCIV_PIC_ATTR_S stPicAttr;    /*record the target Image attr of PCI transmit*/
    HI_U32          u32Offset[3];
    HI_U32          u32BlkSize;
    HI_U32          u32Count;     /* The total buffer count */
    HI_U64          au64PhyAddr[PCIV_MAX_BUF_NUM];
    HI_U32          au32PoolId[PCIV_MAX_BUF_NUM];
    VB_BLKHANDLE    vbBlkHdl[PCIV_MAX_BUF_NUM];     /*Vb handle,used to check the VB is release by VO or not*/
    HI_BOOL         bPcivHold[PCIV_MAX_BUF_NUM];     /*Buffer is been hold by the pciv queue or not*/
    VGS_ONLINE_OPT_S   stVgsOpt;
    PCIV_PREPROC_CFG_S stPreProcCfg;

    struct timer_list stBufTimer;
} PCIV_FWMCHANNEL_S;

static osal_dev_t * astPcivfmwDevice = NULL;
static PCIV_FWMCHANNEL_S g_stFwmPcivChn[PCIVFMW_MAX_CHN_NUM];
static struct timer_list g_timerPcivSendVdecPic;

static spinlock_t           g_PcivFmwLock;
#define PCIVFMW_SPIN_LOCK   spin_lock_irqsave(&g_PcivFmwLock,flags)
#define PCIVFMW_SPIN_UNLOCK spin_unlock_irqrestore(&g_PcivFmwLock,flags)

#define VDEC_MAX_SEND_CNT 	6

static PCIVFMW_CALLBACK_S g_stPcivFmwCallBack;

static HI_U32 s_u32PcivFmwState = PCIVFMW_STATE_STOPED;

static int drop_err_timeref = 1;

HI_VOID PcivFirmWareRecvPicFree(unsigned long data);
HI_VOID PcivFmwPutRegion(PCIV_CHN PcivChn, RGN_TYPE_E enType);


HI_S32 PCIV_FirmWareResetChnQueue(PCIV_CHN pcivChn)
{
	HI_S32               i          = 0;
	HI_S32               s32Ret;
	HI_U32               u32BusyNum = 0;
	PCIV_PIC_NODE_S     *pNode   = NULL;
    PCIV_FWMCHANNEL_S   *pFmwChn = NULL;
	unsigned long      	flags;

	PCIVFMW_SPIN_LOCK;
    pFmwChn = &g_stFwmPcivChn[pcivChn];

	if (HI_TRUE == pFmwChn->bStart)
	{
		PCIVFMW_SPIN_UNLOCK;
		PCIV_FMW_TRACE(HI_DBG_ERR, "pcivChn:%d has not stop, please stop it first!\n", pcivChn);
	    return HI_FAILURE;
	}
	PCIVFMW_SPIN_UNLOCK;

	/*It is in the sending proccess,wait for sending finshed*/
	while(1)
	{
		PCIVFMW_SPIN_LOCK;
		if (PCIVFMW_SEND_ING == pFmwChn->enSendState)
		{
			PCIVFMW_SPIN_UNLOCK;
			msleep(10);
			continue;
		}
		else
		{
			/*if send fail, release the vdec buffer*/
			if ((PCIVFMW_SEND_NOK == pFmwChn->enSendState) && (HI_ID_VDEC == pFmwChn->enModId))
			{
				HI_ASSERT(pFmwChn->pCurVdecNode != NULL);
				s32Ret = CALL_VB_UserSub(pFmwChn->pCurVdecNode->stPcivPic.stVideoFrame.u32PoolId, pFmwChn->pCurVdecNode->stPcivPic.stVideoFrame.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
		        HI_ASSERT(s32Ret == HI_SUCCESS);

		        PCIV_PicQueuePutFree(&pFmwChn->stPicQueue, pFmwChn->pCurVdecNode);
				pFmwChn->enSendState  = PCIVFMW_SEND_OK;
		        pFmwChn->pCurVdecNode = NULL;
			}

			break;
		}
	}

	/*put the node in the busy queue to free queue*/
	u32BusyNum = PCIV_PicQueueGetBusyNum(&pFmwChn->stPicQueue);

	for (i=0; i<u32BusyNum; i++)
	{
		pNode = PCIV_PicQueueGetBusy(&pFmwChn->stPicQueue);
		if (NULL == pNode)
		{
			PCIVFMW_SPIN_UNLOCK;
			PCIV_FMW_TRACE(HI_DBG_ERR, "PCIV_PicQueueGetBusy failed! pciv chn %d.\n", pcivChn);
			return HI_FAILURE;
		}

		if (HI_FALSE == pFmwChn->bMaster)
		{
			s32Ret = CALL_VB_UserSub(pNode->stPcivPic.stVideoFrame.u32PoolId, pNode->stPcivPic.stVideoFrame.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
        	HI_ASSERT(s32Ret == HI_SUCCESS);
		}

		PCIV_PicQueuePutFree(&pFmwChn->stPicQueue, pNode);
		pNode = NULL;
	}

	PCIVFMW_SPIN_UNLOCK;

	return HI_SUCCESS;

}


HI_S32 PCIV_FirmWareCreate(PCIV_CHN pcivChn, PCIV_ATTR_S *pAttr, HI_S32 s32LocalId)
{
    HI_S32 s32Ret, i;
    HI_U32 u32NodeNum;
    PCIV_FWMCHANNEL_S *pChn = NULL;
    unsigned long flags;

    PCIVFMW_CHECK_CHNID(pcivChn);

    pChn = &g_stFwmPcivChn[pcivChn];

    if (HI_TRUE == pChn->bCreate)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn%d have already created \n", pcivChn);
        return HI_ERR_PCIV_EXIST;
    }

    s32Ret = PCIV_FirmWareSetAttr(pcivChn, pAttr, s32LocalId);
    if (s32Ret != HI_SUCCESS)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "attr of pciv chn%d is invalid \n", pcivChn);
        return s32Ret;
    }

    pChn->bStart             = HI_FALSE;
    pChn->bBlock             = HI_FALSE;
	/*[HSCP201307040004],l00181524 The member shoud initialize to 0,otherwise int re-creat and re-dostroy and switch the bind-ship situation,
	it may occur receive the wrong image for out-of-order*/
    pChn->u32TimeRef         = 0;
    pChn->u32SendCnt         = 0;
    pChn->u32GetCnt          = 0;
    pChn->u32RespCnt         = 0;
    pChn->u32LostCnt         = 0;
    pChn->u32NewDoCnt        = 0;
    pChn->u32OldUndoCnt      = 0;
    pChn->enSendState        = PCIVFMW_SEND_OK;
    pChn->u32TimerCnt        = 0;
    pChn->u32RgnNum          = 0;

    pChn->u32AddJobSucCnt    = 0;
    pChn->u32AddJobFailCnt   = 0;

    pChn->u32MoveTaskSucCnt  = 0;
    pChn->u32MoveTaskFailCnt = 0;

    pChn->u32OsdTaskSucCnt   = 0;
    pChn->u32OsdTaskFailCnt  = 0;

    pChn->u32ZoomTaskSucCnt  = 0;
    pChn->u32ZoomTaskFailCnt = 0;

    pChn->u32EndJobSucCnt    = 0;
    pChn->u32EndJobFailCnt   = 0;

    pChn->u32MoveCbCnt       = 0;
    pChn->u32OsdCbCnt        = 0;
    pChn->u32ZoomCbCnt       = 0;

    for (i=0; i<PCIV_MAX_BUF_NUM; i++)
    {
        pChn->bPcivHold[i] = HI_FALSE;
    }

     /*Master Chip*/
    if (0 == s32LocalId)
    {
        u32NodeNum = pAttr->u32Count;
    }
    else
    {
        u32NodeNum = VDEC_MAX_SEND_CNT;
    }

    s32Ret = PCIV_CreatPicQueue(&pChn->stPicQueue, u32NodeNum);
    if (s32Ret != HI_SUCCESS)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn%d create pic queue failed\n", pcivChn);
        return s32Ret;
    }
    pChn->pCurVdecNode = NULL;

    PCIVFMW_SPIN_LOCK;
    pChn->bCreate = HI_TRUE;
	if (0 == s32LocalId)
    {
		pChn->bMaster = HI_TRUE;
    }
    else
    {
		pChn->bMaster = HI_FALSE;
    }
    PCIVFMW_SPIN_UNLOCK;
    PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn%d create ok \n", pcivChn);
    return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareDestroy(PCIV_CHN pcivChn)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;
    unsigned long flags;
	HI_S32        s32Ret;

    PCIVFMW_CHECK_CHNID(pcivChn);

    pChn = &g_stFwmPcivChn[pcivChn];

    if (HI_FALSE == pChn->bCreate)
    {
        return HI_SUCCESS;
    }
    if (HI_TRUE == pChn->bStart)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn%d is running,you should stop first \n", pcivChn);
        return HI_ERR_PCIV_NOT_PERM;
    }

	s32Ret = PCIV_FirmWareResetChnQueue(pcivChn);
	if (HI_SUCCESS != s32Ret)
	{
		PCIV_FMW_TRACE(HI_DBG_ERR, "pcivfmw chn%d stop failed!\n", pcivChn);
		return s32Ret;

	}

	PCIVFMW_SPIN_LOCK;
    PCIV_DestroyPicQueue(&pChn->stPicQueue);

    pChn->bCreate = HI_FALSE;
    PCIVFMW_SPIN_UNLOCK;
    PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn%d destroy ok \n", pcivChn);
    return HI_SUCCESS;
}

HI_S32 PcivFirmWareCheckAttr(PCIV_ATTR_S *pAttr)
{
    VB_BASE_INFO_S    stVBBaseInfo;
    VB_CAL_CONFIG_S   stVBCalConfig;

	/*Check The Image Width and Height*/
    if (!pAttr->stPicAttr.u32Height || !pAttr->stPicAttr.u32Width)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pic w:%d, h:%d invalid\n",
            pAttr->stPicAttr.u32Width, pAttr->stPicAttr.u32Height);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
    if (pAttr->stPicAttr.u32Height%2 || pAttr->stPicAttr.u32Width%2)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pic w:%d, h:%d invalid\n",
            pAttr->stPicAttr.u32Width, pAttr->stPicAttr.u32Height);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
    if (pAttr->stPicAttr.u32Stride[0] < pAttr->stPicAttr.u32Width)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pic stride0:%d,stride1:%d invalid\n",
            pAttr->stPicAttr.u32Stride[0], pAttr->stPicAttr.u32Stride[1]);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
	if ((pAttr->stPicAttr.u32Stride[0] & 0xf)||(pAttr->stPicAttr.u32Stride[1] & 0xf))
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "illegal param: not align Stride(YStride:%d,CStride:%d)\n",
			pAttr->stPicAttr.u32Stride[0],pAttr->stPicAttr.u32Stride[1]);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
    if((pAttr->stPicAttr.enPixelFormat != PIXEL_FORMAT_YVU_SEMIPLANAR_420)
        &&(pAttr->stPicAttr.enPixelFormat != PIXEL_FORMAT_YVU_SEMIPLANAR_422))
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "illegal param: Illegal PixelFormat:%d)\n",
			pAttr->stPicAttr.enPixelFormat);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
    if((pAttr->stPicAttr.enCompressMode <0)||(pAttr->stPicAttr.enCompressMode >4))
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "illegal param: not permmit Compress mode:%d)\n",
			pAttr->stPicAttr.enCompressMode);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
    if((pAttr->stPicAttr.enDynamicRange <0)||(pAttr->stPicAttr.enDynamicRange >5))
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "illegal param: not permmit Compress mode:%d)\n",
			pAttr->stPicAttr.enCompressMode);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
    if((pAttr->stPicAttr.enVideoFormat <0)||(pAttr->stPicAttr.enVideoFormat >3))
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "illegal param: not permmit Compress mode:%d)\n",
			pAttr->stPicAttr.enCompressMode);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

    stVBBaseInfo.b3DNRBuffer    = HI_FALSE;
    stVBBaseInfo.u32Align       = 0;

    stVBBaseInfo.enCompressMode = pAttr->stPicAttr.enCompressMode;
    stVBBaseInfo.enDynamicRange = pAttr->stPicAttr.enDynamicRange;
    stVBBaseInfo.enVideoFormat  = pAttr->stPicAttr.enVideoFormat;

    stVBBaseInfo.enPixelFormat  = pAttr->stPicAttr.enPixelFormat;
    stVBBaseInfo.u32Width       = pAttr->stPicAttr.u32Width;
    stVBBaseInfo.u32Height      = pAttr->stPicAttr.u32Height;

    if (!CKFN_SYS_GetVBCfg())
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "SYS_GetVBCfg is NULL!\n");
        return HI_FAILURE;
    }
    CALL_SYS_GetVBCfg(&stVBBaseInfo, &stVBCalConfig);

	/*Check the Image attr is match or not with the buffer size*/
    if (pAttr->u32BlkSize < stVBCalConfig.u32VBSize)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Buffer block is too smail(%d < %d)\n", pAttr->u32BlkSize, stVBCalConfig.u32VBSize);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareSetAttr(PCIV_CHN pcivChn, PCIV_ATTR_S *pAttr, HI_S32 s32LocalId)
{
	HI_S32 i;
    HI_S32 s32Ret;
    PCIV_FWMCHANNEL_S *pChn = NULL;
    unsigned long flags;

    PCIVFMW_CHECK_CHNID(pcivChn);
    PCIVFMW_CHECK_PTR(pAttr);

    pChn = &g_stFwmPcivChn[pcivChn];

    PCIVFMW_SPIN_LOCK;

	/*The channel is in the process of start,it cannot alter*/
    if (HI_TRUE == pChn->bStart)
    {
        PCIVFMW_SPIN_UNLOCK;
        PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn%d is running\n", pcivChn);
        return HI_ERR_PCIV_NOT_PERM;
    }

	/*check the valid of attr*/
    s32Ret = PcivFirmWareCheckAttr(pAttr);
    if (s32Ret != HI_SUCCESS)
    {
        PCIVFMW_SPIN_UNLOCK;
        return s32Ret;
    }

    memcpy(&pChn->stPicAttr, &pAttr->stPicAttr, sizeof(PCIV_PIC_ATTR_S));
	if (0 == s32LocalId)
	{
		for (i=0; i<pAttr->u32Count; i++)
		{
			if (0 == pChn->au64PhyAddr[i])
			{
				pChn->au64PhyAddr[i] = pAttr->u64PhyAddr[i];
			}
			else
			{
				if (pChn->au64PhyAddr[i] != pAttr->u64PhyAddr[i])
				{
					PCIVFMW_SPIN_UNLOCK;
					PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn: %d, buffer address: 0x%x is invalid!\n", pcivChn, pAttr->u64PhyAddr[i]);
		        	return HI_ERR_PCIV_NOT_PERM;
				}
			}
		}
	}
	else
	{
		memcpy(pChn->au64PhyAddr, pAttr->u64PhyAddr, sizeof(pAttr->u64PhyAddr));
	}

    pChn->u32BlkSize = pAttr->u32BlkSize;
    pChn->u32Count   = pAttr->u32Count;

	/*Set the YUV offset of tartget Image */
    pChn->u32Offset[0] = 0;
    switch (pAttr->stPicAttr.enPixelFormat)
    {
        case PIXEL_FORMAT_YVU_SEMIPLANAR_420:
        /* fall through */
        case PIXEL_FORMAT_YVU_SEMIPLANAR_422:
        {
            /* Sem-planar format do not need u32Offset[2](V offset) */
            pChn->u32Offset[1] = pAttr->stPicAttr.u32Stride[0] * pAttr->stPicAttr.u32Height;
            break;
        }

        default:
        {
            PCIVFMW_SPIN_UNLOCK;
            PCIV_FMW_TRACE(HI_DBG_ERR, "Pixel format(%d) unsupported\n", pAttr->stPicAttr.enPixelFormat);
            return HI_ERR_PCIV_NOT_SUPPORT;
        }
    }

    PCIVFMW_SPIN_UNLOCK;

    return HI_SUCCESS;
}

static HI_VOID PCIV_FirmWareInitPreProcCfg(PCIV_CHN pcivChn)
{
    VGS_ONLINE_OPT_S *pstVgsCfg = &g_stFwmPcivChn[pcivChn].stVgsOpt;
    PCIV_PREPROC_CFG_S *pstPreProcCfg = &g_stFwmPcivChn[pcivChn].stPreProcCfg;

    /* init VGS cfg */
    memset(pstVgsCfg, 0, sizeof(VGS_ONLINE_OPT_S));

    pstVgsCfg->bCrop       = HI_FALSE;
    pstVgsCfg->bCover      = HI_FALSE;
    pstVgsCfg->bOsd        = HI_FALSE;
    ////pstVgsCfg->bHSharpen   = HI_FALSE;
    ////pstVgsCfg->bBorder     = HI_FALSE;
    ////pstVgsCfg->bDci        = HI_FALSE;
    pstVgsCfg->bForceHFilt = HI_FALSE;
    pstVgsCfg->bForceVFilt = HI_FALSE;
    pstVgsCfg->bDeflicker  = HI_FALSE;

    /* init pre-process cfg */
    pstPreProcCfg->enFieldSel   = PCIV_FIELD_BOTH;
    pstPreProcCfg->enFilterType = PCIV_FILTER_TYPE_NORM;

}

HI_S32 PCIV_FirmWareSetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pAttr)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;

    PCIVFMW_CHECK_CHNID(pcivChn);

    if ((pAttr->enFieldSel >= PCIV_FIELD_BUTT) || (pAttr->enFilterType >= PCIV_FILTER_TYPE_BUTT))
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "args invalid \n");
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

    pChn = &g_stFwmPcivChn[pcivChn];

	/*store the pre-process configuration of user set*/
    memcpy(&pChn->stPreProcCfg, pAttr, sizeof(PCIV_PREPROC_CFG_S));

    PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn%d set preproccfg ok\n", pcivChn);
    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareGetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pAttr)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;

    PCIVFMW_CHECK_CHNID(pcivChn);

    pChn = &g_stFwmPcivChn[pcivChn];

    memcpy(pAttr, &pChn->stPreProcCfg, sizeof(PCIV_PREPROC_CFG_S));

    return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareStart(PCIV_CHN pcivChn)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;
    unsigned long flags;

    PCIVFMW_CHECK_CHNID(pcivChn);

    pChn = &g_stFwmPcivChn[pcivChn];

    if (pChn->bCreate != HI_TRUE)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn%d not create\n", pcivChn);
        return HI_ERR_PCIV_UNEXIST;
    }

    PCIVFMW_SPIN_LOCK;
    pChn->bStart = HI_TRUE;
    PCIVFMW_SPIN_UNLOCK;
    PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn%d start ok \n", pcivChn);
    return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareStop(PCIV_CHN pcivChn)
{
    unsigned long flags;
    PCIVFMW_CHECK_CHNID(pcivChn);

    PCIVFMW_SPIN_LOCK;
    g_stFwmPcivChn[pcivChn].bStart = HI_FALSE;

    if (0 != g_stFwmPcivChn[pcivChn].u32RgnNum)
    {
        PCIV_FMW_TRACE(HI_DBG_INFO, "Region number of channel %d is %d, now free the region!\n", pcivChn, g_stFwmPcivChn[pcivChn].u32RgnNum);
        PcivFmwPutRegion(pcivChn, OVERLAYEX_RGN);
        g_stFwmPcivChn[pcivChn].u32RgnNum = 0;
    }

    PCIVFMW_SPIN_UNLOCK;
    PCIV_FMW_TRACE(HI_DBG_INFO, "pcivfmw chn%d stop ok \n", pcivChn);

    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareWindowVbCreate(PCIV_WINVBCFG_S *pCfg)
{
    HI_S32 	s32Ret, i;
    HI_U32 	u32PoolId;
	HI_CHAR *pBufName = "PcivVbFromWindow";

    if( g_stVbPool.u32PoolCount != 0)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Video buffer pool has created\n");
        return HI_ERR_PCIV_BUSY;
    }

    for(i=0; i < pCfg->u32PoolCount; i++)
    {
        s32Ret = CALL_VB_CreatePool(&u32PoolId, pCfg->u32BlkCount[i],
                                         pCfg->u32BlkSize[i], RESERVE_MMZ_NAME, pBufName, VB_REMAP_MODE_NONE);
        if(HI_SUCCESS != s32Ret)
        {
            PCIV_FMW_TRACE(HI_DBG_ALERT, "Create pool(Index=%d, Cnt=%d, Size=%d) fail\n",
                                  i, pCfg->u32BlkCount[i], pCfg->u32BlkSize[i]);
            break;
        }
        g_stVbPool.u32PoolCount = i + 1;
        g_stVbPool.u32PoolId[i] = u32PoolId;
        g_stVbPool.u32Size[i]   = pCfg->u32BlkSize[i];
    }

	/*if one pool created not success, then rollback*/
    if ( g_stVbPool.u32PoolCount != pCfg->u32PoolCount)
    {
        for(i=0; i < g_stVbPool.u32PoolCount; i++)
        {
            (HI_VOID)CALL_VB_DestroyPool(g_stVbPool.u32PoolId[i]);
            g_stVbPool.u32PoolId[i] = VB_INVALID_POOLID;
        }

        g_stVbPool.u32PoolCount = 0;

        return HI_ERR_PCIV_NOMEM;
    }

    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareWindowVbDestroy(HI_VOID)
{
    HI_S32 i;

    for(i=0; i < g_stVbPool.u32PoolCount; i++)
    {
        (HI_VOID)CALL_VB_DestroyPool(g_stVbPool.u32PoolId[i]);
        g_stVbPool.u32PoolId[i] = VB_INVALID_POOLID;
    }

    g_stVbPool.u32PoolCount = 0;
    return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareMalloc(HI_U32 u32Size, HI_S32 s32LocalId, HI_U64 *pPhyAddr)
{
    HI_S32       i;
    VB_BLKHANDLE handle = VB_INVALID_HANDLE;

    HI_CHAR azMmzName[MAX_MMZ_NAME_LEN] = {0};

    if(s32LocalId == 0)
    {
        handle = CALL_VB_GetBlkBySize(u32Size, VB_UID_PCIV, azMmzName);
        if(VB_INVALID_HANDLE == handle)
        {
            PCIV_FMW_TRACE(HI_DBG_ERR,"CALL_VB_GetBlkBySize fail,size:%d!\n", u32Size);
            return HI_ERR_PCIV_NOBUF;
        }

        *pPhyAddr = CALL_VB_Handle2Phys(handle);

        return HI_SUCCESS;
    }

    /*if in the slave chip, then alloc buffer from special VB*/
    for(i=0; i < g_stVbPool.u32PoolCount; i++)
    {
        if(u32Size > g_stVbPool.u32Size[i]) continue;

        handle = CALL_VB_GetBlkByPoolId(g_stVbPool.u32PoolId[i], VB_UID_PCIV);

        if(VB_INVALID_HANDLE != handle) break;
    }

    if(VB_INVALID_HANDLE == handle)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR,"CALL_VB_GetBlkBySize fail,size:%d!\n", u32Size);
        return HI_ERR_PCIV_NOBUF;
    }

    *pPhyAddr = CALL_VB_Handle2Phys(handle);
    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareFree(HI_S32 s32LocalId, HI_U64 u64PhyAddr)
{
	HI_S32        s32Ret;
    VB_BLKHANDLE  vbHandle;
	unsigned long flags;

	PCIVFMW_SPIN_LOCK;
    vbHandle = CALL_VB_Phy2Handle(u64PhyAddr);
    if(VB_INVALID_HANDLE == vbHandle)
    {
    	PCIVFMW_SPIN_UNLOCK;
        PCIV_FMW_TRACE(HI_DBG_ERR, "Invalid Physical Address 0x%08x\n", u64PhyAddr);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

    s32Ret = CALL_VB_UserSub(CALL_VB_Handle2PoolId(vbHandle), u64PhyAddr, VB_UID_PCIV);
	PCIVFMW_SPIN_UNLOCK;

	return s32Ret;
}


HI_S32 PCIV_FirmWareMallocChnBuffer(PCIV_CHN pcivChn, HI_U32 u32Index, HI_U32 u32Size, HI_S32 s32LocalId, HI_U64 *pPhyAddr)
{
    VB_BLKHANDLE       handle = VB_INVALID_HANDLE;
    HI_CHAR            azMmzName[MAX_MMZ_NAME_LEN] = {0};
	unsigned long      flags;
	PCIV_FWMCHANNEL_S *pFmwChn = NULL;

	PCIVFMW_SPIN_LOCK;
	if (0 != s32LocalId)
	{
		PCIVFMW_SPIN_UNLOCK;
		PCIV_FMW_TRACE(HI_DBG_ERR, "Slave Chip: %d, doesn't need chn buffer!\n", s32LocalId);
		return HI_ERR_PCIV_NOT_PERM;
	}

    pFmwChn = &g_stFwmPcivChn[pcivChn];
	if (HI_TRUE == pFmwChn->bStart)
	{
		PCIVFMW_SPIN_UNLOCK;
		PCIV_FMW_TRACE(HI_DBG_ERR, "Pciv Chn: %d, has malloc chn buffer!\n", pcivChn);
		return HI_ERR_PCIV_NOT_PERM;
	}

    handle = CALL_VB_GetBlkBySize(u32Size, VB_UID_PCIV, azMmzName);
    if(VB_INVALID_HANDLE == handle)
    {
    	PCIVFMW_SPIN_UNLOCK;
        PCIV_FMW_TRACE(HI_DBG_ERR,"CALL_VB_GetBlkBySize fail,size:%d!\n", u32Size);
        return HI_ERR_PCIV_NOBUF;
    }

    *pPhyAddr = CALL_VB_Handle2Phys(handle);
	pFmwChn->au64PhyAddr[u32Index] = *pPhyAddr;
	PCIVFMW_SPIN_UNLOCK;

    return HI_SUCCESS;

}


HI_S32 PCIV_FirmWareFreeChnBuffer(PCIV_CHN pcivChn, HI_U32 u32Index, HI_S32 s32LocalId)
{
	HI_S32             s32Ret = HI_SUCCESS;
    VB_BLKHANDLE       vbHandle;
	unsigned long      flags;
	PCIV_FWMCHANNEL_S *pFmwChn = NULL;

    PCIVFMW_SPIN_LOCK;
	if (0 != s32LocalId)
	{
		PCIVFMW_SPIN_UNLOCK;
		PCIV_FMW_TRACE(HI_DBG_ERR, "Slave Chip: %d, has no chn buffer, doesn't need to free chn buffer!\n", s32LocalId);
		return HI_ERR_PCIV_NOT_PERM;
	}

	pFmwChn = &g_stFwmPcivChn[pcivChn];
	if (HI_TRUE == pFmwChn->bStart)
	{
		PCIVFMW_SPIN_UNLOCK;
		PCIV_FMW_TRACE(HI_DBG_ERR, "Pciv Chn: %d has not stopped! Please stop it first!\n", pcivChn);
		return HI_ERR_PCIV_NOT_PERM;
	}

    vbHandle = CALL_VB_Phy2Handle(pFmwChn->au64PhyAddr[u32Index]);
    if(VB_INVALID_HANDLE == vbHandle)
    {
    	PCIVFMW_SPIN_UNLOCK;
        PCIV_FMW_TRACE(HI_DBG_ERR, "Invalid Physical Address 0x%08x\n", pFmwChn->au64PhyAddr[u32Index]);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

	PCIV_FMW_TRACE(HI_DBG_WARN, "start to free buffer, PoolId: %d, au64PhyAddr: 0x%x.\n", CALL_VB_Handle2PoolId(vbHandle), pFmwChn->au64PhyAddr[u32Index]);
    s32Ret = CALL_VB_UserSub(CALL_VB_Handle2PoolId(vbHandle), pFmwChn->au64PhyAddr[u32Index], VB_UID_PCIV);
	PCIV_FMW_TRACE(HI_DBG_WARN, "finish to free buffer, PoolId: %d, au64PhyAddr: 0x%x.\n", CALL_VB_Handle2PoolId(vbHandle), pFmwChn->au64PhyAddr[u32Index]);
	PCIVFMW_SPIN_UNLOCK;

	return s32Ret;
}


HI_S32 PCIV_FirmWarePutPicToQueue(PCIV_CHN pcivChn, const VIDEO_FRAME_INFO_S *pstVideoFrmInfo, HI_U32 u32Index, HI_BOOL bBlock)
{
    PCIV_PIC_NODE_S     *pNode   = NULL;
    PCIV_FWMCHANNEL_S   *pFmwChn = NULL;

    pFmwChn = &g_stFwmPcivChn[pcivChn];

	if(HI_TRUE != pFmwChn->bStart)
    {
        return HI_ERR_PCIV_SYS_NOTREADY;
    }

    pFmwChn->bPcivHold[u32Index] = HI_TRUE;
    pNode = PCIV_PicQueueGetFree(&pFmwChn->stPicQueue);
    if (NULL == pNode)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pcivChn:%d no free node\n", pcivChn);
        return HI_FAILURE;
    }

    pNode->stPcivPic.enModId  = HI_ID_VDEC;
    pNode->stPcivPic.bBlock   = bBlock;
    pNode->stPcivPic.u32Index = u32Index;
    memcpy(&pNode->stPcivPic.stVideoFrame, pstVideoFrmInfo, sizeof(VIDEO_FRAME_INFO_S));

    PCIV_PicQueuePutBusy(&pFmwChn->stPicQueue, pNode);

    return HI_SUCCESS;

}

HI_S32 PCIV_FirmWareGetPicFromQueueAndSend(PCIV_CHN pcivChn)
{
    HI_S32               s32Ret   = HI_SUCCESS;
    HI_S32               s32DevId = 0;
    PCIV_FWMCHANNEL_S   *pFmwChn  = NULL;
    VIDEO_FRAME_INFO_S  *pstVFrameInfo = NULL;

    pFmwChn = &g_stFwmPcivChn[pcivChn];

	if(HI_TRUE != pFmwChn->bStart)
    {
        return HI_ERR_PCIV_SYS_NOTREADY;
    }

	/*send the data in cycle queue, until the data in queue is not less or send fail */
    while(PCIV_PicQueueGetBusyNum(&pFmwChn->stPicQueue))
    {
        pFmwChn->pCurVdecNode = PCIV_PicQueueGetBusy(&pFmwChn->stPicQueue);
        if (NULL == pFmwChn->pCurVdecNode)
        {
            PCIV_FMW_TRACE(HI_DBG_ERR, "pcivChn:%d busy list is empty, no vdec pic\n", pcivChn);
            return HI_FAILURE;
        }

		/*send the vdec image to vpss or venc or vo, if success, put the node to fee queue, else nothing to do
		it will send by the time or next interrupt*/
        pFmwChn->enSendState  = PCIVFMW_SEND_ING;
        pstVFrameInfo         = &pFmwChn->pCurVdecNode->stPcivPic.stVideoFrame;
        PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, start send pic to VPSS/VO.\n", pcivChn, pFmwChn->bStart);
        s32Ret = CALL_SYS_SendData(HI_ID_PCIV, s32DevId, pcivChn, pFmwChn->pCurVdecNode->stPcivPic.bBlock, MPP_DATA_VDEC_FRAME, pstVFrameInfo);
        if ((HI_SUCCESS != s32Ret) && (HI_TRUE == pFmwChn->pCurVdecNode->stPcivPic.bBlock))
        {
			/* bBlock is ture(playback mode),if failed,get out of the circle,do nothing ,it will send by the timer or next DMA interrupt*/
            /* set the point NULL,put the node to the head of busy,whie send again ,get it from the header of busy */
			PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, finish send pic to VPSS/VO.\n", pcivChn, pFmwChn->bStart);
			PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d send pic failed, put to queue and send again. s32Ret: 0x%x\n", pcivChn, s32Ret);
			PCIV_PicQueuePutBusyHead(&pFmwChn->stPicQueue, pFmwChn->pCurVdecNode);
            pFmwChn->enSendState  = PCIVFMW_SEND_NOK;
            pFmwChn->pCurVdecNode = NULL;
            s32Ret                = HI_SUCCESS;
            break;
        }
        else
        {
			/*bBlock is true(playback mode),if success, put the node to free*/
			/*bBlock is false(preview mode),no matter success or not, put the node to free,and do not send the Image again*/
			PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, finish send pic to VPSS/VO.\n", pcivChn, pFmwChn->bStart);
			PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d send pic ok\n", pcivChn);
			HI_ASSERT(pFmwChn->pCurVdecNode != NULL);
            pFmwChn->bPcivHold[pFmwChn->pCurVdecNode->stPcivPic.u32Index] = HI_FALSE;
            PCIV_PicQueuePutFree(&pFmwChn->stPicQueue, pFmwChn->pCurVdecNode);
            pFmwChn->enSendState  = PCIVFMW_SEND_OK;
            pFmwChn->pCurVdecNode = NULL;
        }
    }

    return s32Ret;
}

HI_S32 PCIV_FirmWareReceiverSendVdecPic(PCIV_CHN pcivChn, VIDEO_FRAME_INFO_S  *pstVideoFrmInfo, HI_U32 u32Index, HI_BOOL bBlock)
{
    HI_S32               s32Ret;
    HI_S32               s32DevId = 0;
    HI_S32               s32ChnId = 0;
    HI_U32               u32BusyNum = 0;
    PCIV_FWMCHANNEL_S   *pFmwChn = NULL;

    s32ChnId = pcivChn;
    pFmwChn = &g_stFwmPcivChn[pcivChn];

	if(HI_TRUE != pFmwChn->bStart)
    {
        return HI_ERR_PCIV_SYS_NOTREADY;
    }

    u32BusyNum = PCIV_PicQueueGetBusyNum(&pFmwChn->stPicQueue);
    if (0 != u32BusyNum)
    {
		/*if the current queue has data, put the Image to the tail of the queue*/
        s32Ret = PCIV_FirmWarePutPicToQueue(pcivChn, pstVideoFrmInfo, u32Index, bBlock);
        if (HI_SUCCESS != s32Ret)
        {
            return HI_FAILURE;
        }

		/*Get the data from the header to send*/
        s32Ret = PCIV_FirmWareGetPicFromQueueAndSend(pcivChn);
        if (HI_SUCCESS != s32Ret)
        {
            return s32Ret;
        }
    }
    else
    {
		/*if the current queue has no data, send the current Image directly, if success,return success,else return failure,put the Image to the tail of the queue*/
		PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, start send pic to VPSS/VO.\n", pcivChn, pFmwChn->bStart);
		pFmwChn->bPcivHold[u32Index] = HI_TRUE;
		s32Ret = CALL_SYS_SendData(HI_ID_PCIV, s32DevId, s32ChnId, bBlock, MPP_DATA_VDEC_FRAME, pstVideoFrmInfo);
        if ((HI_SUCCESS != s32Ret) && (HI_TRUE == bBlock))
        {
			/*bBlock is true(playback mode),if failure,put the Image to the tail of the queue*/
			/*bBlock is true(preview mode),no matter success or not, we think it as success, do not put it to queue to send again*/
			PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, finish send pic to VPSS/VO.\n", pcivChn, pFmwChn->bStart);
			PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d send pic failed, put to queue and send again. s32Ret: 0x%x\n", pcivChn, s32Ret);
			if (PCIV_FirmWarePutPicToQueue(pcivChn, pstVideoFrmInfo, u32Index, bBlock))
            {
                return HI_FAILURE;
            }
            s32Ret = HI_SUCCESS;
        }
		else
		{
			PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, finish send pic to VPSS/VO.\n", pcivChn, pFmwChn->bStart);
			PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d send pic ok\n", pcivChn);
			pFmwChn->bPcivHold[u32Index] = HI_FALSE;
		}
    }

    return s32Ret;
}


HI_S32 PCIV_FirmWareRecvPicAndSend(PCIV_CHN pcivChn, PCIV_PIC_S *pRecvPic)
{
    HI_S32               s32Ret;
    VIDEO_FRAME_INFO_S   stVideoFrmInfo={0};
    //VI_FRAME_INFO_S      stViFrmInfo;
    PCIV_FWMCHANNEL_S   *pFmwChn;
    VIDEO_FRAME_S       *pVfrm = NULL;
    unsigned long        flags;
    HI_S32               s32DevId = 0;
    VB_BASE_INFO_S       stVBBaseInfo;
    VB_CAL_CONFIG_S      stVBCalConfig;
    HI_S32               s32ChnId = pcivChn;
    VB_BLKHANDLE         VbHandle;

    PCIVFMW_CHECK_CHNID(pcivChn);

    PCIVFMW_SPIN_LOCK;
    pFmwChn = &g_stFwmPcivChn[pcivChn];
    if(HI_TRUE != pFmwChn->bStart)
    {
        PCIVFMW_SPIN_UNLOCK;
        return HI_ERR_PCIV_SYS_NOTREADY;
    }

    pFmwChn->u32GetCnt++;
    s32Ret = HI_SUCCESS;

    stVBBaseInfo.b3DNRBuffer    = HI_FALSE;
    stVBBaseInfo.u32Align       = 0;

    stVBBaseInfo.enCompressMode = pRecvPic->enCompressMode;
    stVBBaseInfo.enDynamicRange = pRecvPic->enDynamicRange;
    stVBBaseInfo.enVideoFormat  = pRecvPic->enVideoFormat;

    stVBBaseInfo.enPixelFormat  = pFmwChn->stPicAttr.enPixelFormat;
    stVBBaseInfo.u32Width       = pFmwChn->stPicAttr.u32Width;
    stVBBaseInfo.u32Height      = pFmwChn->stPicAttr.u32Height;

    if (!CKFN_SYS_GetVBCfg())
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "SYS_GetVBCfg is NULL!\n");
        return HI_FAILURE;
    }
    CALL_SYS_GetVBCfg(&stVBBaseInfo, &stVBCalConfig);

    VbHandle = CALL_VB_Phy2Handle(pFmwChn->au64PhyAddr[pRecvPic->u32Index]);
    if (VB_INVALID_HANDLE == VbHandle)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pcivfmw %d get buffer fail!\n", pcivChn);
        return HI_FAILURE;
    }

    stVideoFrmInfo.u32PoolId = CALL_VB_Handle2PoolId(VbHandle);
    stVideoFrmInfo.enModId   = pRecvPic->enModId;
    pVfrm                    = &stVideoFrmInfo.stVFrame;

    pVfrm->u32Width          = pFmwChn->stPicAttr.u32Width;
    pVfrm->u32Height         = pFmwChn->stPicAttr.u32Height;
    pVfrm->enPixelFormat     = pFmwChn->stPicAttr.enPixelFormat;

    pVfrm->u64PTS            = pRecvPic->u64Pts;
    pVfrm->u32TimeRef        = pRecvPic->u32TimeRef;
    pVfrm->enField           = pRecvPic->enFiled;
    pVfrm->enDynamicRange    = pRecvPic->enDynamicRange;
    pVfrm->enCompressMode    = pRecvPic->enCompressMode;
    pVfrm->enVideoFormat     = pRecvPic->enVideoFormat;
    pVfrm->enColorGamut      = pRecvPic->enColorGamut;

    pVfrm->u64HeaderPhyAddr[0] = CALL_VB_Handle2Phys(VbHandle);
    pVfrm->u64HeaderPhyAddr[1] = pVfrm->u64HeaderPhyAddr[0] + stVBCalConfig.u32HeadYSize;
    pVfrm->u64HeaderPhyAddr[2] = pVfrm->u64HeaderPhyAddr[1];

    pVfrm->u64HeaderVirAddr[0] = CALL_VB_Handle2Kern(VbHandle);
    pVfrm->u64HeaderVirAddr[1] = pVfrm->u64HeaderVirAddr[0] + stVBCalConfig.u32HeadYSize;
    pVfrm->u64HeaderVirAddr[2] = pVfrm->u64HeaderVirAddr[1];

    pVfrm->u32HeaderStride[0]  = stVBCalConfig.u32HeadStride;
    pVfrm->u32HeaderStride[1]  = stVBCalConfig.u32HeadStride;
    pVfrm->u32HeaderStride[2]  = stVBCalConfig.u32HeadStride;

    pVfrm->u64PhyAddr[0]  = pVfrm->u64HeaderPhyAddr[0] + stVBCalConfig.u32HeadSize;
    pVfrm->u64PhyAddr[1]  = pVfrm->u64PhyAddr[0] + stVBCalConfig.u32MainYSize;
    pVfrm->u64PhyAddr[2]  = pVfrm->u64PhyAddr[1];

    pVfrm->u64VirAddr[0]  = pVfrm->u64HeaderVirAddr[0] + stVBCalConfig.u32HeadSize;
    pVfrm->u64VirAddr[1]  = pVfrm->u64VirAddr[0] + stVBCalConfig.u32MainYSize;
    pVfrm->u64VirAddr[2]  = pVfrm->u64VirAddr[1];

    pVfrm->u32Stride[0]   = stVBCalConfig.u32MainStride;
    pVfrm->u32Stride[1]   = stVBCalConfig.u32MainStride;
    pVfrm->u32Stride[2]   = stVBCalConfig.u32MainStride;

    pVfrm->u64ExtPhyAddr[0] = pVfrm->u64PhyAddr[0] + stVBCalConfig.u32MainSize;
    pVfrm->u64ExtPhyAddr[1] = pVfrm->u64ExtPhyAddr[0] + stVBCalConfig.u32ExtYSize;
    pVfrm->u64ExtPhyAddr[2] = pVfrm->u64ExtPhyAddr[1];

    pVfrm->u64ExtVirAddr[0] = pVfrm->u64VirAddr[0] + stVBCalConfig.u32MainSize;
    pVfrm->u64ExtVirAddr[1] = pVfrm->u64ExtVirAddr[0] + stVBCalConfig.u32ExtYSize;
    pVfrm->u64ExtVirAddr[2] = pVfrm->u64ExtVirAddr[1];

    pVfrm->u32ExtStride[0] = stVBCalConfig.u32ExtStride;
    pVfrm->u32ExtStride[1] = stVBCalConfig.u32ExtStride;
    pVfrm->u32ExtStride[2] = stVBCalConfig.u32ExtStride;

    if (PCIV_BIND_VI == pRecvPic->enSrcType)
    {
        s32Ret = CALL_SYS_SendData(HI_ID_PCIV, s32DevId, s32ChnId, pRecvPic->bBlock, MPP_DATA_VI_FRAME, &stVideoFrmInfo);
    }
    else if (PCIV_BIND_VO == pRecvPic->enSrcType)
    {
        s32Ret = CALL_SYS_SendData(HI_ID_PCIV, s32DevId, s32ChnId, pRecvPic->bBlock, MPP_DATA_VOU_FRAME, &stVideoFrmInfo);
    }
    else if (PCIV_BIND_VDEC == pRecvPic->enSrcType)
    {
         /*When the DMA arrive,first query the queue if has data or not, if yes,send the data in the queue first,if not,send the cuurent Image directly; if success, return,
	     else put the node queue,and trig by the timer next time*/
         s32Ret = PCIV_FirmWareReceiverSendVdecPic(pcivChn, &stVideoFrmInfo, pRecvPic->u32Index, pRecvPic->bBlock);
    }
    else
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Pciv chn %d bind type error, type value: %d.\n", pcivChn, pRecvPic->enSrcType);
        s32Ret = HI_FAILURE;

    }

    if((HI_SUCCESS != s32Ret) && (HI_ERR_VO_CHN_NOT_ENABLE != s32Ret) )
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Pciv chn %d send failed, ret:0x%x\n", pcivChn, s32Ret);
        pFmwChn->u32LostCnt++;
    }
    else
    {
        pFmwChn->u32SendCnt++;
    }

    /* HSCP201306030001 when start timer, PcivFirmWareVoPicFree not have lock synchronous
       int the PcivFirmWareVoPicFree function, need separate lock */
    PCIVFMW_SPIN_UNLOCK;
    PcivFirmWareRecvPicFree(pcivChn);

    return s32Ret;
}


/*After vo displaying and vpss and venc used,the function register by PCIV or FwmDccs mode is been called*/
HI_VOID PcivFirmWareRecvPicFree(unsigned long data)
{
    HI_U32   			i = 0;
	HI_U32   			u32Count = 0;
    HI_S32   			s32Ret;
    HI_BOOL  			bHit    = HI_FALSE;
    PCIV_FWMCHANNEL_S 	*pFmwChn;
    PCIV_CHN 			pcivChn = (PCIV_CHN)data;
    PCIV_PIC_S 			stRecvPic;
    unsigned long 		flags;

    PCIVFMW_SPIN_LOCK;
    pFmwChn = &g_stFwmPcivChn[pcivChn];

    if (pFmwChn->bStart != HI_TRUE)
    {
        PCIVFMW_SPIN_UNLOCK;
        return;
    }

    for(i=0; i<pFmwChn->u32Count; i++)
    {
		/*if bPcivHold is false,only when the VB count occupied by vo/vpss/venc is 0,the VB can release*/
        if(CALL_VB_InquireOneUserCnt(CALL_VB_Phy2Handle(pFmwChn->au64PhyAddr[i]), VB_UID_VO) != 0)
        {
            continue;
        }
        if(CALL_VB_InquireOneUserCnt(CALL_VB_Phy2Handle(pFmwChn->au64PhyAddr[i]), VB_UID_VPSS) != 0)
        {
            continue;
        }
        //if(CALL_VB_InquireOneUserCnt(CALL_VB_Phy2Handle(pFmwChn->au64PhyAddr[i]), VB_UID_GRP) != 0)
		if(CALL_VB_InquireOneUserCnt(CALL_VB_Phy2Handle(pFmwChn->au64PhyAddr[i]), VB_UID_VENC) != 0)
        {
            continue;
        }
        if (HI_TRUE == pFmwChn->bPcivHold[i])
        {
            continue;
        }

		/*The function register by the PCIV is called to handle the action afte the VO displaying or vpss using and venc coding*/
        if(g_stPcivFmwCallBack.pfRecvPicFree)
        {
            stRecvPic.u32Index = i;      /*the index of buffer can release*/
            stRecvPic.u64Pts   = 0;       /* PTS */
            stRecvPic.u32Count = u32Count;/* not used*/
            s32Ret = g_stPcivFmwCallBack.pfRecvPicFree(pcivChn, &stRecvPic);
            if (s32Ret != HI_SUCCESS)
            {
                PCIV_FMW_TRACE(HI_DBG_ERR, "pcivfmw chn%d pfRecvPicFree failed with:0x%x.\n", pcivChn, s32Ret);
                continue;
            }
            bHit = HI_TRUE;
            pFmwChn->u32RespCnt++;
        }
    }

	/*if the buffer has not release by vo/vpss/venc,then start the time after 10ms to check*/
    if(bHit != HI_TRUE)
    {
        pFmwChn->stBufTimer.function = PcivFirmWareRecvPicFree;
        pFmwChn->stBufTimer.data     = data;
        mod_timer(&(pFmwChn->stBufTimer), jiffies + 1);
    }
    PCIVFMW_SPIN_UNLOCK;

    return ;
}


/*After transmit, release the Image buffer after VGS Zoom*/
HI_S32 PCIV_FirmWareSrcPicFree(PCIV_CHN pcivChn, PCIV_PIC_S *pSrcPic)
{
    HI_S32 s32Ret;

    g_stFwmPcivChn[pcivChn].u32RespCnt++;

	/*if the mpp is deinit,the the sys will release the buffer*/
    if (PCIVFMW_STATE_STOPED == s_u32PcivFmwState)
    {
        return HI_SUCCESS;
    }

    PCIV_FMW_TRACE(HI_DBG_DEBUG,"- --> addr:0x%x\n", pSrcPic->u64PhyAddr);
    s32Ret = CALL_VB_UserSub(pSrcPic->u32PoolId, pSrcPic->u64PhyAddr, VB_UID_PCIV);
    //HI_ASSERT(HI_SUCCESS == s32Ret);
    return s32Ret;
}

static HI_S32 PcivFmwSrcPicSend(PCIV_CHN pcivChn, PCIV_BIND_OBJ_S *pBindObj,
  const VIDEO_FRAME_INFO_S *pstVFrame, const VIDEO_FRAME_INFO_S *pstVdecFrame)
{
    HI_S32 s32Ret;
    PCIV_FWMCHANNEL_S *pChn = &g_stFwmPcivChn[pcivChn];
    PCIV_PIC_S stSrcPic;
    unsigned long flags;

    stSrcPic.u32PoolId      = pstVFrame->u32PoolId;
    stSrcPic.enSrcType      = pBindObj->enType;
    stSrcPic.bBlock         = pChn->bBlock;
    stSrcPic.enModId        = pstVFrame->enModId;
    stSrcPic.u64Pts         = pstVFrame->stVFrame.u64PTS;
    stSrcPic.u32TimeRef     = pstVFrame->stVFrame.u32TimeRef;
    stSrcPic.enFiled        = pstVFrame->stVFrame.enField;
	stSrcPic.enColorGamut   = pstVFrame->stVFrame.enColorGamut;
    stSrcPic.enCompressMode = pstVFrame->stVFrame.enCompressMode;
    stSrcPic.enDynamicRange = pstVFrame->stVFrame.enDynamicRange;
    stSrcPic.enVideoFormat  = pstVFrame->stVFrame.enVideoFormat;
    /*The reason why here add if&else is that threr is some situation that
    some modual doesn't set value to u64HeaderPhyAddr in the mode of unCompress*/
    if(COMPRESS_MODE_NONE ==pstVFrame->stVFrame.enCompressMode)
    {
      stSrcPic.u64PhyAddr   = pstVFrame->stVFrame.u64PhyAddr[0];
    }
    else
    {
      stSrcPic.u64PhyAddr   = pstVFrame->stVFrame.u64HeaderPhyAddr[0];
    }
    PCIVFMW_SPIN_LOCK;

    if (pChn->bStart != HI_TRUE)
    {
        PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn %d have stoped \n", pcivChn);
        PCIVFMW_SPIN_UNLOCK;
        return HI_FAILURE;
    }
    PCIVFMW_SPIN_UNLOCK;

	/*the callback function register by the upper mode is called to send the zoom Image by VGS*/
    s32Ret = g_stPcivFmwCallBack.pfSrcSendPic(pcivChn, &stSrcPic);
    if (s32Ret != HI_SUCCESS)
    {
        PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn %d pfSrcSendPic failed! s32Ret: 0x%x.\n", pcivChn, s32Ret);
        return HI_FAILURE;
    }

	/*if success,add the VB( release int PCIV_FirmWareSrcPicFree)*/
    if(COMPRESS_MODE_NONE == pstVFrame->stVFrame.enCompressMode)
    {
        s32Ret = CALL_VB_UserAdd(pstVFrame->u32PoolId, pstVFrame->stVFrame.u64PhyAddr[0], VB_UID_PCIV);
	    HI_ASSERT(HI_SUCCESS == s32Ret);
    }
    else
    {
        s32Ret = CALL_VB_UserAdd(pstVFrame->u32PoolId, pstVFrame->stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
	    HI_ASSERT(HI_SUCCESS == s32Ret);
    }

    if ((NULL != pstVdecFrame) && (PCIV_BIND_VDEC == pBindObj->enType) && (HI_FALSE == pBindObj->bVpssSend))
    {
		/*if success,the Image send by vdec directly bind PCIV need release here */
        PCIVFMW_SPIN_LOCK;
        if(COMPRESS_MODE_NONE == pstVFrame->stVFrame.enCompressMode)
        {
            s32Ret = CALL_VB_UserSub(pstVdecFrame->u32PoolId, pstVdecFrame->stVFrame.u64PhyAddr[0], VB_UID_PCIV);
            HI_ASSERT(s32Ret == HI_SUCCESS);
        }
        else
        {
            s32Ret = CALL_VB_UserSub(pstVdecFrame->u32PoolId, pstVdecFrame->stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
            HI_ASSERT(s32Ret == HI_SUCCESS);
        }

        HI_ASSERT(pChn->pCurVdecNode != NULL);
        PCIV_PicQueuePutFree(&pChn->stPicQueue, pChn->pCurVdecNode);
        pChn->pCurVdecNode = NULL;
        PCIVFMW_SPIN_UNLOCK;
    }

    pChn->u32SendCnt++;

    return HI_SUCCESS;
}

HI_S32 PcivFmwGetRegion(PCIV_CHN PcivChn, RGN_TYPE_E enType, RGN_INFO_S *pstRgnInfo)
{
    HI_S32 s32Ret = HI_FAILURE;
    MPP_CHN_S stChn;

    if ((!CKFN_RGN()) || (!CKFN_RGN_GetRegion()))
    {
        return HI_FAILURE;
    }

    stChn.enModId  = HI_ID_PCIV;
    stChn.s32ChnId = PcivChn;
    stChn.s32DevId = 0;
    s32Ret = CALL_RGN_GetRegion(enType, &stChn, pstRgnInfo);
    HI_ASSERT(HI_SUCCESS == s32Ret);

    return s32Ret;
}

HI_VOID PcivFmwPutRegion(PCIV_CHN PcivChn, RGN_TYPE_E enType)
{
    HI_S32 s32Ret = HI_FAILURE;
    MPP_CHN_S stChn;

    if ((!CKFN_RGN()) || (!CKFN_RGN_PutRegion()))
    {
        return;
    }

    stChn.enModId  = HI_ID_PCIV;
    stChn.s32ChnId = PcivChn;
    stChn.s32DevId = 0;

	s32Ret = CALL_RGN_PutRegion(enType, &stChn, NULL);
    HI_ASSERT(HI_SUCCESS == s32Ret);
    return;
}

static HI_VOID PcivFmwSrcPicZoomCb(MOD_ID_E enCallModId, HI_U32 u32CallDevId, HI_U32 u32CalChnId,const VGS_TASK_DATA_S *pstVgsTask)
{

    HI_S32                   s32Ret;
    PCIV_CHN                 pcivChn;
    PCIV_BIND_OBJ_S          stBindObj = {0};
    //VI_MIXCAP_STAT_S         stMixCapState;
    PCIV_FWMCHANNEL_S        *pFmwChn = NULL;
    const VIDEO_FRAME_INFO_S *pstImgIn = NULL;
    const VIDEO_FRAME_INFO_S *pstImgOut = NULL;
    unsigned long            flags;

    if (NULL == pstVgsTask)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "In Function PcivFmwSrcPicZoomCb: pstVgsTask is null, return!\n");
        return;
    }

    pstImgIn  = &pstVgsTask->stImgIn;
    pstImgOut = &pstVgsTask->stImgOut;
    pcivChn   = pstVgsTask->u32CallChnId;
    HI_ASSERT((pcivChn >= 0) && (pcivChn < PCIVFMW_MAX_CHN_NUM));
    pFmwChn   = &g_stFwmPcivChn[pcivChn];

    pFmwChn->u32ZoomCbCnt++;

    if(COMPRESS_MODE_NONE == pstImgIn->stVFrame.enCompressMode)
    {
        CALL_VB_UserSub(pstImgIn->u32PoolId, pstImgIn->stVFrame.u64PhyAddr[0], VB_UID_PCIV);
    }
    else
    {
        CALL_VB_UserSub(pstImgIn->u32PoolId, pstImgIn->stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
    }

    stBindObj.enType    = pstVgsTask->au64PrivateData[0];
    stBindObj.bVpssSend = pstVgsTask->au64PrivateData[1];

	/*In VGS interrupt,maybe the pcivchn is stopped*/
    if (HI_TRUE != pFmwChn->bStart)
    {
        PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn %d have stoped \n", pcivChn);
        if (PCIV_BIND_VDEC == stBindObj.enType)
        {
            PCIVFMW_SPIN_LOCK;
            pFmwChn->enSendState = PCIVFMW_SEND_NOK;
            PCIVFMW_SPIN_UNLOCK;
        }

        goto out;
    }

   /***********************************************************************************
     * [HSCP201308020003] l00181524,2013.08.16,if the task fail,maybe the action of cancle job occur in pciv ower interrupt
     * maybe out of its ower job,it need judge next step, we cannot add lock,else maybe lock itself and other abnormal
     ************************************************************************************/
     /*if the vgs task fail,then retuen with failure*/
    if (VGS_TASK_FNSH_STAT_OK != pstVgsTask->enFinishStat)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "PcivFmwSrcPicZoomCb vgs task finish status is no ok, chn:%d\n", pcivChn);
        if (PCIV_BIND_VDEC == stBindObj.enType)
        {
            //PCIVFMW_SPIN_LOCK;
            pFmwChn->enSendState = PCIVFMW_SEND_NOK;
            //PCIVFMW_SPIN_UNLOCK;
        }

        goto out;
    }

	/*send the video after zoom*/
    s32Ret = PcivFmwSrcPicSend(pcivChn, &stBindObj, pstImgOut, pstImgIn);
    if (HI_SUCCESS != s32Ret)
    {
        if (PCIV_BIND_VDEC == stBindObj.enType)
        {
            PCIVFMW_SPIN_LOCK;
            pFmwChn->enSendState = PCIVFMW_SEND_NOK;
            PCIVFMW_SPIN_UNLOCK;
        }

        goto out;
    }
    if (PCIV_BIND_VDEC == stBindObj.enType)
    {
        PCIVFMW_SPIN_LOCK;
        pFmwChn->enSendState = PCIVFMW_SEND_OK;
        PCIVFMW_SPIN_UNLOCK;
    }

    if (0 != pFmwChn->u32RgnNum)
    {
        pFmwChn->u32RgnNum = 0;
    }

out:
	/*no matter success or not,in this callback ,the output vb will released*/
    if(COMPRESS_MODE_NONE == pstImgIn->stVFrame.enCompressMode)
    {
        s32Ret = CALL_VB_UserSub(pstImgOut->u32PoolId, pstImgOut->stVFrame.u64PhyAddr[0], VB_UID_PCIV);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }
    else
    {
        s32Ret = CALL_VB_UserSub(pstImgOut->u32PoolId, pstImgOut->stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }

    return;
}


static HI_S32 PcivFmwSrcPicZoom(PCIV_CHN pcivChn, PCIV_BIND_OBJ_S *pObj, const VIDEO_FRAME_INFO_S *pstSrcFrame)
{
    HI_S32            s32Ret;
    MPP_CHN_S         stChn;
    PCIV_FWMCHANNEL_S *pFmwChn = NULL;
    VIDEO_FRAME_S     *pstOutFrame = NULL;
    HI_VOID           *pMmzName = HI_NULL;
    VB_BLKHANDLE      VbHandle;
    VGS_HANDLE        VgsHandle;
    VGS_TASK_DATA_S   stVgsTask;
    VGS_JOB_DATA_S    JobData;
    VGS_EXPORT_FUNC_S *pfnVgsExpFunc = (VGS_EXPORT_FUNC_S *)CMPI_GetModuleFuncById(HI_ID_VGS);
    VB_BASE_INFO_S    stVBBaseInfo;
    VB_CAL_CONFIG_S   stVBCalConfig;

    pFmwChn = &g_stFwmPcivChn[pcivChn];

	/*configure the input video frame*/
    memcpy(&stVgsTask.stImgIn, pstSrcFrame, sizeof(VIDEO_FRAME_INFO_S));

    /* the output Image is same with the input Image */
    memcpy(&stVgsTask.stImgOut, &stVgsTask.stImgIn, sizeof(VIDEO_FRAME_INFO_S));

    stVBBaseInfo.b3DNRBuffer    = HI_FALSE;
    stVBBaseInfo.u32Align       = 0;

    stVBBaseInfo.enCompressMode = pstSrcFrame->stVFrame.enCompressMode;
    stVBBaseInfo.enDynamicRange = pstSrcFrame->stVFrame.enDynamicRange;
    stVBBaseInfo.enVideoFormat  = pstSrcFrame->stVFrame.enVideoFormat;

    stVBBaseInfo.enPixelFormat  = pFmwChn->stPicAttr.enPixelFormat;
    stVBBaseInfo.u32Width       = pFmwChn->stPicAttr.u32Width;
    stVBBaseInfo.u32Height      = pFmwChn->stPicAttr.u32Height;

    if (!CKFN_SYS_GetVBCfg())
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "SYS_GetVBCfg is NULL!\n");
        return HI_FAILURE;
    }
    CALL_SYS_GetVBCfg(&stVBBaseInfo, &stVBCalConfig);

	/*Get the output Image VB after VGS zoom*/
    HI_ASSERT(HI_TRUE == CKFN_SYS_GetMmzName());

    stChn.enModId  = HI_ID_PCIV;
    stChn.s32DevId = 0;
    stChn.s32ChnId = pcivChn;
    CALL_SYS_GetMmzName(&stChn, &pMmzName);

    VbHandle = CALL_VB_GetBlkBySize(pFmwChn->u32BlkSize, VB_UID_PCIV, pMmzName);
    if (VB_INVALID_HANDLE == VbHandle)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Get VB(%dByte) fail\n",pFmwChn->u32BlkSize);
        return HI_FAILURE;
    }
    PCIV_FMW_TRACE(HI_DBG_DEBUG, "+ --> addr:0x%x\n", CALL_VB_Handle2Phys(VbHandle));

    /*config the output video info*/
    stVgsTask.stImgOut.u32PoolId = CALL_VB_Handle2PoolId(VbHandle);
    pstOutFrame                  = &stVgsTask.stImgOut.stVFrame;

    pstOutFrame->u32Width        = pFmwChn->stPicAttr.u32Width;
    pstOutFrame->u32Height       = pFmwChn->stPicAttr.u32Height;
    pstOutFrame->enPixelFormat   = pFmwChn->stPicAttr.enPixelFormat;

    pstOutFrame->u64HeaderPhyAddr[0] = CALL_VB_Handle2Phys(VbHandle);
    pstOutFrame->u64HeaderPhyAddr[1] = pstOutFrame->u64HeaderPhyAddr[0] + stVBCalConfig.u32HeadYSize;
    pstOutFrame->u64HeaderPhyAddr[2] = pstOutFrame->u64HeaderPhyAddr[1];

    pstOutFrame->u64HeaderVirAddr[0] = CALL_VB_Handle2Kern(VbHandle);
    pstOutFrame->u64HeaderVirAddr[1] = pstOutFrame->u64HeaderVirAddr[0] + stVBCalConfig.u32HeadYSize;
    pstOutFrame->u64HeaderVirAddr[2] = pstOutFrame->u64HeaderVirAddr[1];

    pstOutFrame->u32HeaderStride[0]  = stVBCalConfig.u32HeadStride;
    pstOutFrame->u32HeaderStride[1]  = stVBCalConfig.u32HeadStride;
    pstOutFrame->u32HeaderStride[2]  = stVBCalConfig.u32HeadStride;

    pstOutFrame->u64PhyAddr[0]  = pstOutFrame->u64HeaderPhyAddr[0] + stVBCalConfig.u32HeadSize;
    pstOutFrame->u64PhyAddr[1]  = pstOutFrame->u64PhyAddr[0] + stVBCalConfig.u32MainYSize;
    pstOutFrame->u64PhyAddr[2]  = pstOutFrame->u64PhyAddr[1];

    pstOutFrame->u64VirAddr[0]  = pstOutFrame->u64HeaderVirAddr[0] + stVBCalConfig.u32HeadSize;
    pstOutFrame->u64VirAddr[1]  = pstOutFrame->u64VirAddr[0] + stVBCalConfig.u32MainYSize;
    pstOutFrame->u64VirAddr[2]  = pstOutFrame->u64VirAddr[1];

    pstOutFrame->u32Stride[0]   = stVBCalConfig.u32MainStride;
    pstOutFrame->u32Stride[1]   = stVBCalConfig.u32MainStride;
    pstOutFrame->u32Stride[2]   = stVBCalConfig.u32MainStride;

    pstOutFrame->u64ExtPhyAddr[0] = pstOutFrame->u64PhyAddr[0] + stVBCalConfig.u32MainSize;
    pstOutFrame->u64ExtPhyAddr[1] = pstOutFrame->u64ExtPhyAddr[0] + stVBCalConfig.u32ExtYSize;
    pstOutFrame->u64ExtPhyAddr[2] = pstOutFrame->u64ExtPhyAddr[1];

    pstOutFrame->u64ExtVirAddr[0] = pstOutFrame->u64VirAddr[0] + stVBCalConfig.u32MainSize;
    pstOutFrame->u64ExtVirAddr[1] = pstOutFrame->u64ExtVirAddr[0] + stVBCalConfig.u32ExtYSize;
    pstOutFrame->u64ExtVirAddr[2] = pstOutFrame->u64ExtVirAddr[1];

    pstOutFrame->u32ExtStride[0] = stVBCalConfig.u32ExtStride;
    pstOutFrame->u32ExtStride[1] = stVBCalConfig.u32ExtStride;
    pstOutFrame->u32ExtStride[2] = stVBCalConfig.u32ExtStride;

    JobData.enJobType    = VGS_JOB_TYPE_NORMAL;
    JobData.pJobCallBack = HI_NULL;

    if(COMPRESS_MODE_NONE != pstOutFrame->enCompressMode)
    {
        s32Ret = CALL_VB_UserAdd(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
    }
    else
    {
        s32Ret = CALL_VB_UserAdd(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
    }
    HI_ASSERT(HI_SUCCESS == s32Ret);

    /* 1.Begin VGS job-----------------------------------------------------------------------------------------------------------*/

    s32Ret = pfnVgsExpFunc->pfnVgsBeginJob(&VgsHandle, VGS_JOB_PRI_NORMAL, HI_ID_PCIV, pcivChn, pcivChn, &JobData);
    if (HI_SUCCESS != s32Ret)
    {
        pFmwChn->u32AddJobFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "pfnVgsBeginJob failed ! pcivChn:%d \n", pcivChn);
        if(COMPRESS_MODE_NONE != pstOutFrame->enCompressMode)
        {
		    CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
            CALL_VB_UserSub(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
        }
        else
        {
            CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
            CALL_VB_UserSub(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
        }
        return HI_FAILURE;
    }
    pFmwChn->u32AddJobSucCnt++;

    /* 2.zoom the pic-----------------------------------------------------------------------------------------------------------*/

	/*configure other item of the vgs task info(in the callback of vgs will perform the action send image)*/
    stVgsTask.pCallBack          = PcivFmwSrcPicZoomCb;
    stVgsTask.enCallModId        = HI_ID_PCIV;
    stVgsTask.u32CallDevId       = pcivChn;
    stVgsTask.u32CallChnId       = pcivChn;
    stVgsTask.au64PrivateData[0] = pObj->enType;
    stVgsTask.au64PrivateData[1] = pObj->bVpssSend;
    stVgsTask.reserved           = 0;

    s32Ret = pfnVgsExpFunc->pfnVgsAddOnlineTask(VgsHandle, &stVgsTask, &pFmwChn->stVgsOpt);
    if (HI_SUCCESS != s32Ret)
    {
        pFmwChn->u32ZoomTaskFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "create vgs task failed,errno:%x,will lost this frame\n",s32Ret);
        if(COMPRESS_MODE_NONE != pstOutFrame->enCompressMode)
        {
		    CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
            CALL_VB_UserSub(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
        }
        else
        {
            CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
            CALL_VB_UserSub(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
        }
		pfnVgsExpFunc->pfnVgsCancelJob(VgsHandle);
        return HI_FAILURE;
    }

    pFmwChn->u32ZoomTaskSucCnt++;

    /* 3.End VGS job-----------------------------------------------------------------------------------------------------------*/

    /* Notes: If EndJob failed, callback will called auto */
    s32Ret = pfnVgsExpFunc->pfnVgsEndJob(VgsHandle, HI_TRUE,HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        pFmwChn->u32EndJobFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "pfnVgsEndJob failed ! pcivChn:%d \n", pcivChn);
		if(COMPRESS_MODE_NONE != pstOutFrame->enCompressMode)
        {
		    CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
            CALL_VB_UserSub(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
        }
        else
        {
            CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
            CALL_VB_UserSub(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
        }
        pfnVgsExpFunc->pfnVgsCancelJob(VgsHandle);
        return HI_FAILURE;
    }

    pFmwChn->u32EndJobSucCnt++;

    return HI_SUCCESS;
}


static HI_VOID PcivFmwSrcPicMoveOSdZoomCb(MOD_ID_E enCallModId, HI_U32 u32CallDevId, HI_U32 u32CallChnId, const VGS_TASK_DATA_S *pstVgsTask)
{
    HI_S32                   s32Ret;
    PCIV_CHN                 pcivChn;
    PCIV_BIND_OBJ_S          stBindObj = {0};
    //VI_MIXCAP_STAT_S         stMixCapState;
    PCIV_FWMCHANNEL_S        *pFmwChn = NULL;
    const VIDEO_FRAME_INFO_S *pstImgIn = NULL;
    const VIDEO_FRAME_INFO_S *pstImgOut = NULL;
    unsigned long            flags;

    if (NULL == pstVgsTask)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "In Function PcivFmwSrcPicMoveOSdZoomCb: pstVgsTask is null, return!\n");
        return;
    }

    pstImgIn  = &pstVgsTask->stImgIn;
    pstImgOut = &pstVgsTask->stImgOut;
    pcivChn   = pstVgsTask->u32CallChnId;
    HI_ASSERT((pcivChn >= 0) && (pcivChn < PCIVFMW_MAX_CHN_NUM));
    pFmwChn   = &g_stFwmPcivChn[pcivChn];

    pFmwChn->u32MoveCbCnt++;
    pFmwChn->u32OsdCbCnt++;
    pFmwChn->u32ZoomCbCnt++;

	/*the Image finish used should released*/
    if(COMPRESS_MODE_NONE == pstImgIn->stVFrame.enCompressMode)
    {
        CALL_VB_UserSub(pstImgIn->u32PoolId, pstImgIn->stVFrame.u64PhyAddr[0], VB_UID_PCIV);
    }
    else
    {
        CALL_VB_UserSub(pstImgIn->u32PoolId, pstImgIn->stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
    }

    /* release the region gotten*/
    PcivFmwPutRegion(pcivChn, OVERLAYEX_RGN);
    pFmwChn->u32RgnNum = 0;

    stBindObj.enType    = pstVgsTask->au64PrivateData[0];
    stBindObj.bVpssSend = pstVgsTask->au64PrivateData[1];

	/*In the VGS interrupt,the channel maybe stop*/
    if (HI_FALSE == pFmwChn->bStart)
    {
        PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn %d have stoped \n", pcivChn);
        if (PCIV_BIND_VDEC == stBindObj.enType)
        {
            PCIVFMW_SPIN_LOCK;
            pFmwChn->enSendState = PCIVFMW_SEND_NOK;
            PCIVFMW_SPIN_UNLOCK;
        }

        goto out;
    }

	/*if vgs task failue, return with failure*/
    if (VGS_TASK_FNSH_STAT_OK != pstVgsTask->enFinishStat)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "PcivFmwSrcPicMoveOSdZoomCb vgs task finish status is no ok, chn:%d\n", pcivChn);
        if (PCIV_BIND_VDEC == stBindObj.enType)
        {
            //PCIVFMW_SPIN_LOCK;
            pFmwChn->enSendState = PCIVFMW_SEND_NOK;
            //PCIVFMW_SPIN_UNLOCK;
        }

        goto out;
    }

	/*send the video after zoom*/
	s32Ret = PcivFmwSrcPicSend(pcivChn, &stBindObj, pstImgOut, pstImgIn);
    if (HI_SUCCESS != s32Ret)
    {
        if (PCIV_BIND_VDEC == stBindObj.enType)
        {
            PCIVFMW_SPIN_LOCK;
            pFmwChn->enSendState = PCIVFMW_SEND_NOK;
            PCIVFMW_SPIN_UNLOCK;
        }

        goto out;
    }

    if (PCIV_BIND_VDEC == stBindObj.enType)
    {
        PCIVFMW_SPIN_LOCK;
        pFmwChn->enSendState = PCIVFMW_SEND_OK;
        PCIVFMW_SPIN_UNLOCK;
    }

out:
	/*no matter success or not,this callback mst release the VB*/
    if(COMPRESS_MODE_NONE == pstImgIn->stVFrame.enCompressMode)
    {
        s32Ret = CALL_VB_UserSub(pstImgOut->u32PoolId, pstImgOut->stVFrame.u64PhyAddr[0], VB_UID_PCIV);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }
    else
    {
        s32Ret = CALL_VB_UserSub(pstImgOut->u32PoolId, pstImgOut->stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }

    return;
}


static HI_S32 PcivFmwSrcPicMoveOsdZoom(PCIV_CHN pcivChn, PCIV_BIND_OBJ_S *pObj,
        const VIDEO_FRAME_INFO_S *pstSrcFrame)
{
    HI_S32            		i;
    HI_S32            		s32Ret;
    HI_S32            		s32Value;
    MPP_CHN_S         		stChn;
    RGN_INFO_S        		stRgnInfo;
    VGS_HANDLE        		VgsHandle;
    VGS_JOB_DATA_S    		JobData;
    VGS_TASK_DATA_S   		stVgsTask;
    VB_BLKHANDLE      		VbHandle;
    VIDEO_FRAME_S     		*pstOutFrame = NULL;
    HI_VOID           		*pMmzName = HI_NULL;
    PCIV_FWMCHANNEL_S 		*pFmwChn = &g_stFwmPcivChn[pcivChn];
	VGS_EXPORT_FUNC_S 		*pfnVgsExpFunc = (VGS_EXPORT_FUNC_S *)CMPI_GetModuleFuncById(HI_ID_VGS);
	static VGS_ONLINE_OPT_S stVgsOpt;
    VB_BASE_INFO_S          stVBBaseInfo;
    VB_CAL_CONFIG_S         stVBCalConfig;
    stRgnInfo.u32Num = 0;
    s32Ret   = PcivFmwGetRegion(pcivChn, OVERLAYEX_RGN, &stRgnInfo);
    s32Value = s32Ret;
    if (stRgnInfo.u32Num <= 0)
    {
        PcivFmwPutRegion(pcivChn, OVERLAYEX_RGN);
        s32Ret = PcivFmwSrcPicZoom(pcivChn, pObj, pstSrcFrame);
        return s32Ret;
    }

    /* config VGS optional */
	//memset(&stVgsOpt, 0, sizeof(VGS_ONLINE_OPT_S));
    memcpy(&stVgsOpt, &pFmwChn->stVgsOpt, sizeof(VGS_ONLINE_OPT_S));

    /* the Input Image info is from pstSrcFrame */
    memcpy(&stVgsTask.stImgIn,  pstSrcFrame, sizeof(VIDEO_FRAME_INFO_S));

    /* the output Image is same with the input Image */
    memcpy(&stVgsTask.stImgOut, &stVgsTask.stImgIn, sizeof(VIDEO_FRAME_INFO_S));

    stVBBaseInfo.b3DNRBuffer    = HI_FALSE;
    stVBBaseInfo.u32Align       = 0;

    stVBBaseInfo.enCompressMode = pstSrcFrame->stVFrame.enCompressMode;
    stVBBaseInfo.enDynamicRange = pstSrcFrame->stVFrame.enDynamicRange;
    stVBBaseInfo.enVideoFormat  = pstSrcFrame->stVFrame.enVideoFormat;

    stVBBaseInfo.enPixelFormat  = pFmwChn->stPicAttr.enPixelFormat;
    stVBBaseInfo.u32Width       = pFmwChn->stPicAttr.u32Width;
    stVBBaseInfo.u32Height      = pFmwChn->stPicAttr.u32Height;

    if (!CKFN_SYS_GetVBCfg())
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "SYS_GetVBCfg is NULL!\n");
        return HI_FAILURE;
    }
    CALL_SYS_GetVBCfg(&stVBBaseInfo, &stVBCalConfig);

	/*the output Image is same to pciv channel size, request VB*/
    HI_ASSERT(HI_TRUE == CKFN_SYS_GetMmzName());

    stChn.enModId  = HI_ID_PCIV;
    stChn.s32DevId = 0;
    stChn.s32ChnId = pcivChn;
    CALL_SYS_GetMmzName(&stChn, &pMmzName);

    VbHandle = CALL_VB_GetBlkBySize(pFmwChn->u32BlkSize, VB_UID_PCIV, pMmzName);
    if (VB_INVALID_HANDLE == VbHandle)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Get VB(%dByte) buffer for image out fail,chn: %d.=======\n", pFmwChn->u32BlkSize, pcivChn);
        return HI_FAILURE;
    }

	/*config the outout video info*/
    stVgsTask.stImgOut.u32PoolId = CALL_VB_Handle2PoolId(VbHandle);
    pstOutFrame                  = &stVgsTask.stImgOut.stVFrame;
    pstOutFrame->u32Width        = pFmwChn->stPicAttr.u32Width;
    pstOutFrame->u32Height       = pFmwChn->stPicAttr.u32Height;
    pstOutFrame->enPixelFormat   = pFmwChn->stPicAttr.enPixelFormat;

    pstOutFrame->u64HeaderPhyAddr[0] = CALL_VB_Handle2Phys(VbHandle);
    pstOutFrame->u64HeaderPhyAddr[1] = pstOutFrame->u64HeaderPhyAddr[0] + stVBCalConfig.u32HeadYSize;
    pstOutFrame->u64HeaderPhyAddr[2] = pstOutFrame->u64HeaderPhyAddr[1];

    pstOutFrame->u64HeaderVirAddr[0] = CALL_VB_Handle2Kern(VbHandle);
    pstOutFrame->u64HeaderVirAddr[1] = pstOutFrame->u64HeaderVirAddr[0] + stVBCalConfig.u32HeadYSize;
    pstOutFrame->u64HeaderVirAddr[2] = pstOutFrame->u64HeaderVirAddr[1];

    pstOutFrame->u32HeaderStride[0]  = stVBCalConfig.u32HeadStride;
    pstOutFrame->u32HeaderStride[1]  = stVBCalConfig.u32HeadStride;
    pstOutFrame->u32HeaderStride[2]  = stVBCalConfig.u32HeadStride;

    pstOutFrame->u64PhyAddr[0]  = pstOutFrame->u64HeaderPhyAddr[0] + stVBCalConfig.u32HeadSize;
    pstOutFrame->u64PhyAddr[1]  = pstOutFrame->u64PhyAddr[0] + stVBCalConfig.u32MainYSize;
    pstOutFrame->u64PhyAddr[2]  = pstOutFrame->u64PhyAddr[1];

    pstOutFrame->u64VirAddr[0]  = pstOutFrame->u64HeaderVirAddr[0] + stVBCalConfig.u32HeadSize;
    pstOutFrame->u64VirAddr[1]  = pstOutFrame->u64VirAddr[0] + stVBCalConfig.u32MainYSize;
    pstOutFrame->u64VirAddr[2]  = pstOutFrame->u64VirAddr[1];

    pstOutFrame->u32Stride[0]   = stVBCalConfig.u32MainStride;
    pstOutFrame->u32Stride[1]   = stVBCalConfig.u32MainStride;
    pstOutFrame->u32Stride[2]   = stVBCalConfig.u32MainStride;

    pstOutFrame->u64ExtPhyAddr[0] = pstOutFrame->u64PhyAddr[0] + stVBCalConfig.u32MainSize;
    pstOutFrame->u64ExtPhyAddr[1] = pstOutFrame->u64ExtPhyAddr[0] + stVBCalConfig.u32ExtYSize;
    pstOutFrame->u64ExtPhyAddr[2] = pstOutFrame->u64ExtPhyAddr[1];

    pstOutFrame->u64ExtVirAddr[0] = pstOutFrame->u64VirAddr[0] + stVBCalConfig.u32MainSize;
    pstOutFrame->u64ExtVirAddr[1] = pstOutFrame->u64ExtVirAddr[0] + stVBCalConfig.u32ExtYSize;
    pstOutFrame->u64ExtVirAddr[2] = pstOutFrame->u64ExtVirAddr[1];

    pstOutFrame->u32ExtStride[0] = stVBCalConfig.u32ExtStride;
    pstOutFrame->u32ExtStride[1] = stVBCalConfig.u32ExtStride;
    pstOutFrame->u32ExtStride[2] = stVBCalConfig.u32ExtStride;

	/*config vgs task, put osd or not*/
    if (s32Value == HI_SUCCESS && stRgnInfo.u32Num > 0)
    {
        HI_ASSERT(stRgnInfo.u32Num <= OVERLAYEX_MAX_NUM_PCIV);
        pFmwChn->u32RgnNum = stRgnInfo.u32Num;

        for (i=0; i<stRgnInfo.u32Num; i++)
        {
            stVgsOpt.stOsdOpt[i].bOsdEn         = HI_TRUE;
            stVgsOpt.stOsdOpt[i].u32GlobalAlpha = 255;

            stVgsOpt.stOsdOpt[i].stVgsOsd.u64PhyAddr    = stRgnInfo.pstRgnCommInfo->pstRgnComm->u64PhyAddr;
            stVgsOpt.stOsdOpt[i].stVgsOsd.enPixelFormat = stRgnInfo.pstRgnCommInfo->pstRgnComm->enPixelFmt;
            stVgsOpt.stOsdOpt[i].stVgsOsd.u32Stride     = stRgnInfo.pstRgnCommInfo->pstRgnComm->u32Stride;

            if (PIXEL_FORMAT_ARGB_1555 == stVgsOpt.stOsdOpt[i].stVgsOsd.enPixelFormat)
            {
                stVgsOpt.stOsdOpt[i].stVgsOsd.bAlphaExt1555 = HI_TRUE;
                stVgsOpt.stOsdOpt[i].stVgsOsd.u8Alpha0      = stRgnInfo.pstRgnCommInfo->pstRgnComm->u32BgAlpha;
                stVgsOpt.stOsdOpt[i].stVgsOsd.u8Alpha1      = stRgnInfo.pstRgnCommInfo->pstRgnComm->u32FgAlpha;
            }

            stVgsOpt.stOsdOpt[i].stOsdRect.s32X      = stRgnInfo.pstRgnCommInfo->pstRgnComm->stPoint.s32X;
            stVgsOpt.stOsdOpt[i].stOsdRect.s32Y      = stRgnInfo.pstRgnCommInfo->pstRgnComm->stPoint.s32Y;
            stVgsOpt.stOsdOpt[i].stOsdRect.u32Height = stRgnInfo.pstRgnCommInfo->pstRgnComm->stSize.u32Height;
            stVgsOpt.stOsdOpt[i].stOsdRect.u32Width  = stRgnInfo.pstRgnCommInfo->pstRgnComm->stSize.u32Width;
        }

        stVgsOpt.bOsd = HI_TRUE;

    }

	/*config the member or vgs taks structure(sending Image in VGS callback function)*/
    stVgsTask.pCallBack          = PcivFmwSrcPicMoveOSdZoomCb;
    stVgsTask.enCallModId        = HI_ID_PCIV;
    stVgsTask.u32CallChnId       = pcivChn;
    stVgsTask.au64PrivateData[0] = pObj->enType;
    stVgsTask.au64PrivateData[1] = pObj->bVpssSend;

    stVgsTask.reserved           = 0;

    JobData.enJobType            = VGS_JOB_TYPE_NORMAL;
    JobData.pJobCallBack         = HI_NULL;
    if(COMPRESS_MODE_NONE != pstOutFrame->enCompressMode)
    {
        CALL_VB_UserAdd(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
    }
    else
    {
        CALL_VB_UserAdd(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
    }

    /* 1.Begin VGS job-----------------------------------------------------------------------------------------------------------*/

    s32Ret = pfnVgsExpFunc->pfnVgsBeginJob(&VgsHandle, VGS_JOB_PRI_NORMAL, HI_ID_PCIV, pcivChn, pcivChn, &JobData);
    if (s32Ret != HI_SUCCESS)
    {
        pFmwChn->u32AddJobFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "pfnVgsBeginJob failed ! pcivChn:%d \n", pcivChn);
        if(COMPRESS_MODE_NONE != pstOutFrame->enCompressMode)
        {
		    CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
		    CALL_VB_UserSub(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
        }
        else
        {
            CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
		    CALL_VB_UserSub(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
        }
        return HI_FAILURE;
    }
    pFmwChn->u32AddJobSucCnt++;

    /* 2.move the picture, add osd, scale picture------------------------------------------------------------------------------*/

    /* add task to VGS job */
    s32Ret = pfnVgsExpFunc->pfnVgsAddOnlineTask(VgsHandle, &stVgsTask, &stVgsOpt);
    if (HI_SUCCESS != s32Ret)
    {
        pFmwChn->u32MoveTaskFailCnt++;
        pFmwChn->u32OsdTaskFailCnt++;
        pFmwChn->u32ZoomTaskFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "create vgs task failed,errno:%x,will lost this frame\n", s32Ret);
        if(COMPRESS_MODE_NONE != pstOutFrame->enCompressMode)
        {
		    CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
		    CALL_VB_UserSub(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
        }
        else
        {
            CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
		    CALL_VB_UserSub(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
        }
        pfnVgsExpFunc->pfnVgsCancelJob(VgsHandle);
        PcivFmwPutRegion(pcivChn, OVERLAYEX_RGN);
        return HI_FAILURE;
    }

    pFmwChn->u32MoveTaskSucCnt++;
    pFmwChn->u32OsdTaskSucCnt++;
    pFmwChn->u32ZoomTaskSucCnt++;

    /* 3.End DSU job------------------------------------------------------------------------------------------------------*/

    /* Notes: If EndJob failed, callback will called auto */
    s32Ret = pfnVgsExpFunc->pfnVgsEndJob(VgsHandle, HI_TRUE,HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        pFmwChn->u32EndJobFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "pfnVgsEndJob failed ! pcivChn:%d \n", pcivChn);
        if(COMPRESS_MODE_NONE != pstOutFrame->enCompressMode)
        {
		    CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
		    CALL_VB_UserSub(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
        }
        else
        {
            CALL_VB_UserSub(stVgsTask.stImgOut.u32PoolId, stVgsTask.stImgOut.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
		    CALL_VB_UserSub(stVgsTask.stImgIn.u32PoolId, stVgsTask.stImgIn.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
        }
        pfnVgsExpFunc->pfnVgsCancelJob(VgsHandle);
        return HI_FAILURE;
    }

    pFmwChn->u32EndJobSucCnt++;
    return HI_SUCCESS;

}

static HI_S32 PcivFirmWareSrcPreProc(PCIV_CHN pcivChn,PCIV_BIND_OBJ_S *pObj,
            const VIDEO_FRAME_INFO_S *pstSrcFrame)
{
    HI_S32 s32Ret = HI_SUCCESS;
    PCIV_FWMCHANNEL_S *pChn = NULL;
    pChn = &g_stFwmPcivChn[pcivChn];

    if (drop_err_timeref == 1)
    {
		/*Prevent out-of-order when send VI source Image, drop the out-of-order fram in the video(u32TimeRef must increased) */
        if ((PCIV_BIND_VI == pObj->enType) || (PCIV_BIND_VO == pObj->enType))
        {
            if (pstSrcFrame->stVFrame.u32TimeRef < pChn->u32TimeRef)
            {
                PCIV_FMW_TRACE(HI_DBG_ERR, "Pciv %d, TimeRef err, (%d,%d)\n",
                pcivChn, pstSrcFrame->stVFrame.u32TimeRef, pChn->u32TimeRef);
                return HI_FAILURE;
            }
            pChn->u32TimeRef = pstSrcFrame->stVFrame.u32TimeRef;
        }
    }

	/*if need put osd to the source vide, Process:Move->put OSD->Zoom */
    if ((PCIV_BIND_VI == pObj->enType) || (PCIV_BIND_VO == pObj->enType) || (PCIV_BIND_VDEC == pObj->enType))
    {
        PCIV_FMW_TRACE(HI_DBG_INFO, "Pciv channel %d support osd right now\n", pcivChn);
        s32Ret = PcivFmwSrcPicMoveOsdZoom(pcivChn, pObj, pstSrcFrame);
    }
    else
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Pciv channel %d not support type:%d\n", pcivChn, pObj->enType);
    }

    return s32Ret;
}

/* be called in VIU interrupt handler or VDEC interrupt handler or vpss send handler . */
HI_S32 PCIV_FirmWareSrcPicSend(PCIV_CHN pcivChn, PCIV_BIND_OBJ_S *pObj, const VIDEO_FRAME_INFO_S *pPicInfo)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;
    HI_BOOL bWaitDsu;

    bWaitDsu = HI_FALSE;

    pChn = &g_stFwmPcivChn[pcivChn];

   /****************************************************************************************
     * The Image send to PCIV from VDEC is 16-byte align,but the Image send by pciv is not align.Then when PCIV send the Image date from vdec directly, at this
     * time, when the master chip get data,it will calculate the address of UV, the err of 8 lines image data dislocation will appear, So we propose to the data should
     * through VPSS or VGS in the slave chip.It will reload as the format that PCIV sending need.
     * But,if the Image data do not through VPSS, because of the performance of VGS if weak, At this time, the system send spead is limited by VGS, so for the diffrence
     * diffrence of align format, PCIV cannot send the Image directly from vdec, but through this function handle.
     ****************************************************************************************/


	/*PCIV channel must be start*/
    if (pChn->bStart != HI_TRUE)
    {
        return HI_FAILURE;
    }
    pChn->u32GetCnt++;

	/*perform the pre_process then send the Image*/
    if (PcivFirmWareSrcPreProc(pcivChn, pObj, pPicInfo))
    {
        pChn->u32LostCnt++;
        return HI_FAILURE;
    }

    if (PCIV_BIND_VDEC == pObj->enType)
    {
        pChn->enSendState = PCIVFMW_SEND_ING;
    }
    bWaitDsu = HI_TRUE;

    return HI_SUCCESS;
}

/* be called in VIU interrupt handler */
HI_S32 PCIV_FirmWareViuSendPic(VI_DEV viDev, VI_CHN viChn,
        const VIDEO_FRAME_INFO_S *pPicInfo)
{
    return HI_SUCCESS;
}


/* timer function of Receiver(master chip): Get data from pciv and send to vpss/venc/vo */
/* timer function of sender(slave chip): Get data from vdec and send to pciv */

HI_VOID PCIV_SendVdecPicTimerFunc(unsigned long data)
{
    HI_S32 s32Ret;
    PCIV_CHN pcivChn;
    PCIV_FWMCHANNEL_S *pChn;
    VIDEO_FRAME_INFO_S *pstVFrameInfo;
    unsigned long flags;
    HI_S32 s32DevId = 0;
    PCIV_BIND_OBJ_S Obj = {0};
    //VI_MIXCAP_STAT_S  stMixCapState;

    /* timer will be restarted after 1 tick */
    g_timerPcivSendVdecPic.expires  = jiffies + msecs_to_jiffies(PCIV_TIMER_EXPIRES);
    g_timerPcivSendVdecPic.function = PCIV_SendVdecPicTimerFunc;
    g_timerPcivSendVdecPic.data     = 0;
    if (!timer_pending(&g_timerPcivSendVdecPic))
    {
        add_timer(&g_timerPcivSendVdecPic);
    }

    for (pcivChn=0; pcivChn<PCIVFMW_MAX_CHN_NUM; pcivChn++)
	{
        PCIVFMW_SPIN_LOCK;
        pChn = &g_stFwmPcivChn[pcivChn];

        pChn->u32TimerCnt++;

        if (HI_FALSE == pChn->bStart)
        {
            PCIVFMW_SPIN_UNLOCK;
            continue;
        }

        if (PCIVFMW_SEND_ING == pChn->enSendState)
        {
            PCIVFMW_SPIN_UNLOCK;
            continue;
        }
        else if (PCIVFMW_SEND_OK == pChn->enSendState)
        {
			/*check the last time is success or not(the first is the success state)
			get the new vdec Image info*/
            pChn->pCurVdecNode = PCIV_PicQueueGetBusy(&pChn->stPicQueue);
            if (NULL == pChn->pCurVdecNode)
            {
                PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d no vdec pic\n", pcivChn);
                PCIVFMW_SPIN_UNLOCK;
                continue;
            }
        }
	    else if (PCIVFMW_SEND_NOK == pChn->enSendState)
        {
        	if (HI_TRUE == pChn->bMaster)
        	{
				/*If the last time is not success,get and send  the data of last time again */
	            pChn->pCurVdecNode = PCIV_PicQueueGetBusy(&pChn->stPicQueue);
	            if (NULL == pChn->pCurVdecNode)
	            {
	                PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d no vdec pic\n", pcivChn);
	                PCIVFMW_SPIN_UNLOCK;
	                continue;
	            }
        	}
			else
			{
				/*If the last time is not success, get the data of last time to send again*/
	            if (pChn->pCurVdecNode == NULL)
	            {
	                PCIVFMW_SPIN_UNLOCK;
	                continue;
	            }
			}
        }
	    else
        {
            PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn %d send vdec pic state error %#x\n", pcivChn, pChn->enSendState);
            PCIVFMW_SPIN_UNLOCK;
            continue;
        }

		if (HI_TRUE == pChn->bMaster)
		{

	        pChn->enSendState = PCIVFMW_SEND_ING;
	        pstVFrameInfo     = &pChn->pCurVdecNode->stPcivPic.stVideoFrame;

			/*send the Image frome vdec to VPSS/VENC/VO*/
			PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, start send pic to VPSS/VO.\n", pcivChn, pChn->bStart);
	        s32Ret = CALL_SYS_SendData(HI_ID_PCIV, s32DevId, pcivChn, pChn->pCurVdecNode->stPcivPic.bBlock, MPP_DATA_VDEC_FRAME, pstVFrameInfo);
	        if ((s32Ret != HI_SUCCESS) && (HI_TRUE == pChn->pCurVdecNode->stPcivPic.bBlock))
	        {

				PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, finish send pic to VPSS/VO.\n", pcivChn, pChn->bStart);
				PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d send pic failed, put to queue and send again. s32Ret: 0x%x\n", pcivChn, s32Ret);
				PCIV_PicQueuePutBusyHead(&pChn->stPicQueue, pChn->pCurVdecNode);
	            pChn->enSendState  = PCIVFMW_SEND_NOK;
	            pChn->pCurVdecNode = NULL;
	            PCIVFMW_SPIN_UNLOCK;
	        }
	        else
	        {
				PCIV_FMW_TRACE(HI_DBG_WARN, "pcivChn:%d, bStart: %d, finish send pic to VPSS/VO.\n", pcivChn, pChn->bStart);
				PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d send pic ok\n", pcivChn);
				HI_ASSERT(pChn->pCurVdecNode != NULL);
	            pChn->bPcivHold[pChn->pCurVdecNode->stPcivPic.u32Index] = HI_FALSE;
	            PCIV_PicQueuePutFree(&pChn->stPicQueue, pChn->pCurVdecNode);
	            pChn->enSendState  = PCIVFMW_SEND_OK;
	            pChn->pCurVdecNode = NULL;
	            PCIVFMW_SPIN_UNLOCK;

	            PcivFirmWareRecvPicFree(pcivChn);
	        }
		}
		else
		{

	        pstVFrameInfo = &pChn->pCurVdecNode->stPcivPic.stVideoFrame;

			/*send the vdec Image to PCI target*/
	        Obj.enType    = PCIV_BIND_VDEC;
	        Obj.bVpssSend = HI_FALSE;
			pChn->enModId = HI_ID_VDEC;
	        //s32Ret = PCIV_FirmWareSrcPicSend(pcivChn, &Obj, pstVFrameInfo, &stMixCapState);
			s32Ret = PCIV_FirmWareSrcPicSend(pcivChn, &Obj, pstVFrameInfo);
	        if (s32Ret != HI_SUCCESS)/*if send failed,the next time use the backup data*/
	        {
	            pChn->enSendState = PCIVFMW_SEND_NOK;
	        }
	        PCIVFMW_SPIN_UNLOCK;
		}
	}

	return;
}


/* Called in VIU, virtual VOU or VDEC interrupt handler */
HI_S32 PCIV_FirmWareSendPic(HI_S32 s32DevId, HI_S32 s32ChnId, HI_BOOL bBlock, MPP_DATA_TYPE_E enDataType, void *pPicInfo)
{
    HI_S32             s32Ret;
    PCIV_CHN           pcivChn = s32ChnId;
    unsigned long      flags;
    PCIV_BIND_OBJ_S    Obj = {0};
    RGN_INFO_S         stRgnInfo;
    //VI_MIXCAP_STAT_S   stMixCapState;
    PCIV_FWMCHANNEL_S  *pChn = NULL;
    VIDEO_FRAME_INFO_S *pstVifInfo   = NULL;
    VIDEO_FRAME_INFO_S *pstVofInfo   = NULL;
    VIDEO_FRAME_INFO_S *pstVdecfInfo = NULL;

    PCIVFMW_CHECK_CHNID(pcivChn);
    PCIVFMW_CHECK_PTR(pPicInfo);

    if (MPP_DATA_VI_FRAME == enDataType)
    {
        Obj.enType    = PCIV_BIND_VI;
        Obj.bVpssSend = HI_FALSE;
        pstVifInfo = (VIDEO_FRAME_INFO_S *)pPicInfo;

        stRgnInfo.u32Num = 0;
        s32Ret = PcivFmwGetRegion(pcivChn, OVERLAYEX_RGN, &stRgnInfo);
        if (HI_SUCCESS != s32Ret)
        {
            PcivFmwPutRegion(pcivChn, OVERLAYEX_RGN);
            return s32Ret;
        }

        PCIVFMW_SPIN_LOCK;

        pChn          = &g_stFwmPcivChn[s32ChnId];
        pChn->bBlock  = bBlock;
        pChn->enModId = HI_ID_VI;

        if (pChn->stPicAttr.u32Width  != pstVifInfo->stVFrame.u32Width
         || pChn->stPicAttr.u32Height != pstVifInfo->stVFrame.u32Height
         || (stRgnInfo.u32Num > 0))
        {
        	PcivFmwPutRegion(pcivChn, OVERLAYEX_RGN);
            /* the pic size is not the same or it needs to add osd */
			 s32Ret = PCIV_FirmWareSrcPicSend(pcivChn, &Obj, pstVifInfo);
        }
        else
        {
            /* send picture directly */
            PCIVFMW_SPIN_UNLOCK;
            s32Ret = PcivFmwSrcPicSend(pcivChn, &Obj, pstVifInfo, NULL);
            PCIVFMW_SPIN_LOCK;
        }
        PCIVFMW_SPIN_UNLOCK;

        if (HI_SUCCESS != s32Ret)
        {
            PCIV_FMW_TRACE(HI_DBG_DEBUG, "pcivChn:%d viu frame pic send failed\n",s32DevId);
            return HI_FAILURE;
        }
    }
    else if (MPP_DATA_VOU_FRAME == enDataType)
    {
        Obj.enType                  = PCIV_BIND_VO;
        Obj.bVpssSend               = HI_FALSE;

        pstVofInfo                  = (VIDEO_FRAME_INFO_S *)pPicInfo;

        PCIVFMW_SPIN_LOCK;

        pChn          = &g_stFwmPcivChn[s32ChnId];
        pChn->bBlock  = bBlock;
        pChn->enModId = HI_ID_VO;

        s32Ret = PCIV_FirmWareSrcPicSend(pcivChn, &Obj, pstVofInfo);
        PCIVFMW_SPIN_UNLOCK;

        if (HI_SUCCESS != s32Ret)
        {
            PCIV_FMW_TRACE(HI_DBG_DEBUG, "pcivChn:%d virtual vou frame pic send failed\n",s32DevId);
            return HI_FAILURE;
        }
    }
    else if (MPP_DATA_VDEC_FRAME == enDataType)
    {
        PCIV_PIC_NODE_S *pNode = NULL;

        pstVdecfInfo = (VIDEO_FRAME_INFO_S *)pPicInfo;

        PCIVFMW_SPIN_LOCK;

        pChn = &g_stFwmPcivChn[pcivChn];

		if (HI_TRUE != pChn->bStart)
		{
			PCIVFMW_SPIN_UNLOCK;
        	PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn %d have stoped \n", pcivChn);
        	return HI_FAILURE;
		}

        pChn->bBlock = bBlock;

        pNode = PCIV_PicQueueGetFree(&pChn->stPicQueue);
        if (NULL == pNode)
        {
            PCIV_FMW_TRACE(HI_DBG_DEBUG, "pcivChn:%d no free node\n",s32DevId);
            PCIVFMW_SPIN_UNLOCK;
            return HI_FAILURE;
        }

        pNode->stPcivPic.enModId = HI_ID_VDEC;

        memcpy(&pNode->stPcivPic.stVideoFrame, pstVdecfInfo, sizeof(VIDEO_FRAME_INFO_S));
        s32Ret = CALL_VB_UserAdd(pstVdecfInfo->u32PoolId, pstVdecfInfo->stVFrame.u64HeaderPhyAddr[0], VB_UID_PCIV);
		HI_ASSERT(HI_SUCCESS == s32Ret);

        PCIV_PicQueuePutBusy(&pChn->stPicQueue, pNode);

        PCIVFMW_SPIN_UNLOCK;
    }
    else
    {
        PCIV_FMW_TRACE(HI_DBG_DEBUG, "pcivChn:%d data type:%d error\n",s32DevId,enDataType);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 PcivFmw_VpssQuery(HI_S32 s32DevId, HI_S32 s32ChnId, VPSS_QUERY_INFO_S *pstQueryInfo, VPSS_INST_INFO_S *pstInstInfo)
{
    VB_BLKHANDLE VbHandle;
    MOD_ID_E enModId;
    VIDEO_FRAME_S *pstVFrame;
    HI_U32   u32PicWidth;
    HI_U32   u32PicHeight;
    HI_U32   u32CurTimeRef;
    HI_S32   s32Ret;
    MPP_CHN_S  stChn ={0};
    HI_CHAR *pMmzName = NULL;
    unsigned long flags;
    VB_BASE_INFO_S    stVBBaseInfo;
    VB_CAL_CONFIG_S   stVBCalConfig;

    PCIV_FWMCHANNEL_S   *pFmwChn = NULL;
    PCIV_CHN PcivChn = s32ChnId;

    PCIVFMW_CHECK_CHNID(PcivChn);
    PCIVFMW_CHECK_PTR(pstQueryInfo);
    PCIVFMW_CHECK_PTR(pstInstInfo);


	PCIVFMW_SPIN_LOCK;
    pFmwChn = &g_stFwmPcivChn[PcivChn];
    if (HI_FALSE == pFmwChn->bCreate)
	{
		PCIV_FMW_TRACE(HI_DBG_ALERT, "pciv channel doesn't exist, please create it!\n");
		PCIVFMW_SPIN_UNLOCK;
		return HI_FAILURE;
	}

	if (HI_FALSE == pFmwChn->bStart)
	{
		PCIV_FMW_TRACE(HI_DBG_ALERT, "pciv channel has stoped!\n");
		PCIVFMW_SPIN_UNLOCK;
        return HI_FAILURE;
	}

    if (NULL == pstQueryInfo->pstSrcPicInfo)
    {
		PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn:%d SrcPicInfo is NULL!\n",PcivChn);
		PCIVFMW_SPIN_UNLOCK;
        goto OLD_UNDO;
    }

    enModId = pstQueryInfo->pstSrcPicInfo->enModId;
    u32CurTimeRef = pstQueryInfo->pstSrcPicInfo->stVideoFrame.stVFrame.u32TimeRef;

    if (((HI_ID_VI == enModId) || (HI_ID_VO == enModId)) && (pFmwChn->u32TimeRef == u32CurTimeRef))
    {
        //Duplicate frame not recived again
		PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn:%d duplicated frame!\n",PcivChn);
		PCIVFMW_SPIN_UNLOCK;
        goto OLD_UNDO;
    }

    if (HI_ID_VDEC == enModId)
    {
        //Duplicate frame not recived again
        s32Ret = g_stPcivFmwCallBack.pfQueryPcivChnShareBufState(PcivChn);
        if (s32Ret == HI_FAILURE)
        {
			PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn:%d has no free share buf to receive pic!\n",PcivChn);
			PCIVFMW_SPIN_UNLOCK;
			goto OLD_UNDO;
        }

		if (PCIVFMW_SEND_OK != pFmwChn->enSendState)
		{
		    PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn:%d last pic send no ok, send again!\n", PcivChn);
			PCIVFMW_SPIN_UNLOCK;
			goto OLD_UNDO;
		}
    }
    if (HI_TRUE == pstQueryInfo->bScaleCap)
    {
        u32PicWidth       = pFmwChn->stPicAttr.u32Width;
        u32PicHeight      = pFmwChn->stPicAttr.u32Height;

    }
    else
    {
        u32PicWidth  = pstQueryInfo->pstSrcPicInfo->stVideoFrame.stVFrame.u32Width;
        u32PicHeight = pstQueryInfo->pstSrcPicInfo->stVideoFrame.stVFrame.u32Height;

    }

	if (HI_FALSE == pstQueryInfo->bMalloc)
	{
		/* bypass */
        /* if picture from VIU or USER, send to PCIV Firmware directly */
        if ((HI_ID_VI == enModId) || (HI_ID_USER == enModId))
        {
		    pstInstInfo->bNew = HI_TRUE;
		    pstInstInfo->bVpss = HI_TRUE;
        }
        /* if picture from VEDC, send to PCIV queue and then deal with DSU, check the queue is full or not  */
        else if (HI_ID_VDEC == enModId)
        {
            //PCIVFMW_SPIN_LOCK;
            if (0 == PCIV_PicQueueGetFreeNum(&(pFmwChn->stPicQueue)))
            {
                /* if PCIV queue is full, old undo */
                PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn:%d queue is full!\n",PcivChn);
                PCIVFMW_SPIN_UNLOCK;
                goto OLD_UNDO;
            }
            //PCIVFMW_SPIN_UNLOCK;

            pstInstInfo->bNew = HI_TRUE;
		    pstInstInfo->bVpss = HI_TRUE;
        }
	}
	else
	{
        stVBBaseInfo.b3DNRBuffer    = HI_FALSE;
        stVBBaseInfo.u32Align       = 0;

        stVBBaseInfo.enCompressMode = pFmwChn->stPicAttr.enCompressMode;
        stVBBaseInfo.enDynamicRange = pFmwChn->stPicAttr.enDynamicRange;
        stVBBaseInfo.enVideoFormat  = pFmwChn->stPicAttr.enVideoFormat;

        stVBBaseInfo.enPixelFormat  = pFmwChn->stPicAttr.enPixelFormat;
        stVBBaseInfo.u32Width       = pFmwChn->stPicAttr.u32Width;
        stVBBaseInfo.u32Height      = pFmwChn->stPicAttr.u32Height;

        if (!CKFN_SYS_GetVBCfg())
        {
            PCIV_FMW_TRACE(HI_DBG_ERR, "SYS_GetVBCfg is NULL!\n");
            PCIVFMW_SPIN_UNLOCK;
            return HI_FAILURE;
        }
        CALL_SYS_GetVBCfg(&stVBBaseInfo, &stVBCalConfig);

        stChn.enModId = HI_ID_PCIV;
        stChn.s32DevId = s32DevId;
        stChn.s32ChnId = s32ChnId;
        s32Ret = CALL_SYS_GetMmzName(&stChn, (void**)&pMmzName);
        HI_ASSERT(HI_SUCCESS == s32Ret);

        VbHandle = CALL_VB_GetBlkBySize(pFmwChn->u32BlkSize, VB_UID_VPSS, pMmzName);
        if (VB_INVALID_HANDLE == VbHandle)
        {
            PCIV_FMW_TRACE(HI_DBG_ALERT, "get VB for u32PicSize: %d from ddr:%s fail,for grp %d VPSS Query\n",pFmwChn->u32BlkSize,pMmzName,s32DevId);
			PCIVFMW_SPIN_UNLOCK;
			goto OLD_UNDO;
        }

        pstInstInfo->astDestPicInfo[0].stVideoFrame.u32PoolId = CALL_VB_Handle2PoolId(VbHandle);

        pstVFrame = &pstInstInfo->astDestPicInfo[0].stVideoFrame.stVFrame;
        pstVFrame->u32Width      = u32PicWidth;
        pstVFrame->u32Height     = u32PicHeight;
        pstVFrame->enPixelFormat = pFmwChn->stPicAttr.enPixelFormat;
        pstVFrame->enCompressMode = pFmwChn->stPicAttr.enCompressMode;
        pstVFrame->enDynamicRange = pFmwChn->stPicAttr.enDynamicRange;
        pstVFrame->enVideoFormat  = pFmwChn->stPicAttr.enVideoFormat;
        pstVFrame->enField        = VIDEO_FIELD_FRAME;

        pstVFrame->u64HeaderPhyAddr[0] = CALL_VB_Handle2Phys(VbHandle);
        pstVFrame->u64HeaderPhyAddr[1] = pstVFrame->u64HeaderPhyAddr[0] + stVBCalConfig.u32HeadYSize;
        pstVFrame->u64HeaderPhyAddr[2] = pstVFrame->u64HeaderPhyAddr[1];

        pstVFrame->u64HeaderVirAddr[0] = CALL_VB_Handle2Kern(VbHandle);
        pstVFrame->u64HeaderVirAddr[1] = pstVFrame->u64HeaderVirAddr[0] + stVBCalConfig.u32HeadYSize;
        pstVFrame->u64HeaderVirAddr[2] = pstVFrame->u64HeaderVirAddr[1];

        pstVFrame->u32HeaderStride[0]  = stVBCalConfig.u32HeadStride;
        pstVFrame->u32HeaderStride[1]  = stVBCalConfig.u32HeadStride;
        pstVFrame->u32HeaderStride[2]  = stVBCalConfig.u32HeadStride;

        pstVFrame->u64PhyAddr[0]  = pstVFrame->u64HeaderPhyAddr[0] + stVBCalConfig.u32HeadSize;
        pstVFrame->u64PhyAddr[1]  = pstVFrame->u64PhyAddr[0] + stVBCalConfig.u32MainYSize;
        pstVFrame->u64PhyAddr[2]  = pstVFrame->u64PhyAddr[1];

        pstVFrame->u64VirAddr[0]  = pstVFrame->u64HeaderVirAddr[0] + stVBCalConfig.u32HeadSize;
        pstVFrame->u64VirAddr[1]  = pstVFrame->u64VirAddr[0] + stVBCalConfig.u32MainYSize;
        pstVFrame->u64VirAddr[2]  = pstVFrame->u64VirAddr[1];

        pstVFrame->u32Stride[0]   = stVBCalConfig.u32MainStride;
        pstVFrame->u32Stride[1]   = stVBCalConfig.u32MainStride;
        pstVFrame->u32Stride[2]   = stVBCalConfig.u32MainStride;

        pstVFrame->u64ExtPhyAddr[0] = pstVFrame->u64PhyAddr[0] + stVBCalConfig.u32MainSize;
        pstVFrame->u64ExtPhyAddr[1] = pstVFrame->u64ExtPhyAddr[0] + stVBCalConfig.u32ExtYSize;
        pstVFrame->u64ExtPhyAddr[2] = pstVFrame->u64ExtPhyAddr[1];

        pstVFrame->u64ExtVirAddr[0] = pstVFrame->u64VirAddr[0] + stVBCalConfig.u32MainSize;
        pstVFrame->u64ExtVirAddr[1] = pstVFrame->u64ExtVirAddr[0] + stVBCalConfig.u32ExtYSize;
        pstVFrame->u64ExtVirAddr[2] = pstVFrame->u64ExtVirAddr[1];

        pstVFrame->u32ExtStride[0] = stVBCalConfig.u32ExtStride;
        pstVFrame->u32ExtStride[1] = stVBCalConfig.u32ExtStride;
        pstVFrame->u32ExtStride[2] = stVBCalConfig.u32ExtStride;

        pstInstInfo->bVpss = HI_TRUE;
        pstInstInfo->bNew  = HI_TRUE;
	}
    pFmwChn->u32NewDoCnt++;
	PCIVFMW_SPIN_UNLOCK;
	return HI_SUCCESS;

OLD_UNDO:
    pstInstInfo->bVpss   = HI_FALSE;
    pstInstInfo->bNew    = HI_FALSE;
    pstInstInfo->bDouble = HI_FALSE;
    pstInstInfo->bUpdate = HI_FALSE;
    pFmwChn->u32OldUndoCnt++;
	return HI_SUCCESS;
}


HI_S32 PcivFmw_VpssSend(HI_S32 s32DevId, HI_S32 s32ChnId, VPSS_SEND_INFO_S *pstSendInfo)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;
    MOD_ID_E          enModId;
    HI_S32            s32Ret;
    PCIV_BIND_OBJ_S   BindObj;
    unsigned long     flags;
    RGN_INFO_S        stRgnInfo = {0};
    //VI_MIXCAP_STAT_S  stMixCapState;
    PCIV_CHN PcivChn  = s32ChnId;

    PCIVFMW_CHECK_CHNID(PcivChn);
    PCIVFMW_CHECK_PTR(pstSendInfo);
    PCIVFMW_CHECK_PTR(pstSendInfo->pstDestPicInfo[0]);

    pChn = &g_stFwmPcivChn[PcivChn];
    if (HI_FALSE == pChn->bCreate)
	{
		PCIV_FMW_TRACE(HI_DBG_ALERT, "pciv channel doesn't exist, please create it!\n");
        return HI_FAILURE;
	}

	if (HI_FALSE == pChn->bStart)
	{
		PCIV_FMW_TRACE(HI_DBG_ALERT, "pciv channel has stoped!\n");
        return HI_FAILURE;
	}

    if (HI_FALSE == pstSendInfo->bSuc)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "bsuc is failure.\n");
        return HI_FAILURE;
    }

    enModId = pstSendInfo->pstDestPicInfo[0]->enModId;

    if (HI_ID_VDEC == enModId)
    {
        BindObj.enType = PCIV_BIND_VDEC;
	}
    else if (HI_ID_VO == enModId)
    {
        BindObj.enType = PCIV_BIND_VO;
    }
    else
    {
        BindObj.enType = PCIV_BIND_VI;
    }

    BindObj.bVpssSend           = HI_TRUE;

    s32Ret = PcivFmwGetRegion(PcivChn, OVERLAYEX_RGN, &stRgnInfo);

    /* use which flag to know VPSS is bypass or not */
    if (pChn->stPicAttr.u32Width  != pstSendInfo->pstDestPicInfo[0]->stVideoFrame.stVFrame.u32Width
     || pChn->stPicAttr.u32Height != pstSendInfo->pstDestPicInfo[0]->stVideoFrame.stVFrame.u32Height
     || (stRgnInfo.u32Num > 0))
	{
		/* bypass */
        PcivFmwPutRegion(PcivChn, OVERLAYEX_RGN);
        PCIVFMW_SPIN_LOCK;
        pChn->bBlock = pstSendInfo->pstDestPicInfo[0]->bBlock;
        s32Ret = PCIV_FirmWareSrcPicSend(PcivChn, &BindObj, &(pstSendInfo->pstDestPicInfo[0]->stVideoFrame));
        if (HI_SUCCESS != s32Ret)
        {
            PCIV_FMW_TRACE(HI_DBG_ALERT, "PCIV_FirmWareSrcPicSend failed! pciv chn %d\n", PcivChn);
            PCIVFMW_SPIN_UNLOCK;
            return HI_FAILURE;
        }
        PCIVFMW_SPIN_UNLOCK;
	}
	else
	{
        pChn->u32GetCnt++;
        /* [HSCP201308020003] l00181524,2013.08.16, PcivFmwSrcPicSend add lock inner, if add lock the will appeare lock itself bug,cannot add lock*/
        //PCIVFMW_SPIN_LOCK;
        pChn->bBlock = pstSendInfo->pstDestPicInfo[0]->bBlock;
        s32Ret = PcivFmwSrcPicSend(PcivChn, &BindObj, &(pstSendInfo->pstDestPicInfo[0]->stVideoFrame), NULL);
        if (HI_SUCCESS != s32Ret)
        {
            pChn->u32LostCnt++;
            PCIV_FMW_TRACE(HI_DBG_ALERT, "PcivFmwSrcPicSend failed! pciv chn %d, s32Ret: 0x%x.\n", PcivChn, s32Ret);
            //PCIVFMW_SPIN_UNLOCK;
            return HI_FAILURE;
        }
        //PCIVFMW_SPIN_UNLOCK;
	}

	return HI_SUCCESS;
}

HI_S32 PcivFmw_ResetCallBack(HI_S32 s32DevId, HI_S32 s32ChnId, HI_VOID *pvData)
{
	HI_S32   i;
	HI_S32   s32Ret;
    PCIV_CHN PcivChn = s32ChnId;
    PCIV_FWMCHANNEL_S *pChn = NULL;
	unsigned long      flags;
	PCIV_PIC_NODE_S   *pNode = NULL;
	HI_U32             u32BusyNum;

	PCIVFMW_SPIN_LOCK;
    pChn         = &g_stFwmPcivChn[PcivChn];
	pChn->bStart = HI_FALSE;
	PCIVFMW_SPIN_UNLOCK;

	/*during the send process wait for finished sending*/
	while(1)
	{
		PCIVFMW_SPIN_LOCK;
		if (PCIVFMW_SEND_ING == pChn->enSendState)
		{
			PCIVFMW_SPIN_UNLOCK;
			msleep(10);
			continue;
		}
		else
		{
			/*send failure,release the vb add by vdec*/
			if ((PCIVFMW_SEND_NOK == pChn->enSendState) && (HI_ID_VDEC == pChn->enModId))
			{
				HI_ASSERT(pChn->pCurVdecNode != NULL);
				s32Ret = CALL_VB_UserSub(pChn->pCurVdecNode->stPcivPic.stVideoFrame.u32PoolId, pChn->pCurVdecNode->stPcivPic.stVideoFrame.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
		        HI_ASSERT(s32Ret == HI_SUCCESS);

		        PCIV_PicQueuePutFree(&pChn->stPicQueue, pChn->pCurVdecNode);
				pChn->enSendState  = PCIVFMW_SEND_OK;
		        pChn->pCurVdecNode = NULL;
			}

			break;
		}
	}

	/*put the node from busy to free*/
	u32BusyNum = PCIV_PicQueueGetBusyNum(&pChn->stPicQueue);
	for (i=0; i<u32BusyNum; i++)
	{
		pNode = PCIV_PicQueueGetBusy(&pChn->stPicQueue);
		if (NULL == pNode)
		{
			PCIVFMW_SPIN_UNLOCK;
			PCIV_FMW_TRACE(HI_DBG_ERR, "PCIV_PicQueueGetBusy failed! pciv chn %d.\n", PcivChn);
			return HI_FAILURE;
		}

		s32Ret = CALL_VB_UserSub(pNode->stPcivPic.stVideoFrame.u32PoolId, pNode->stPcivPic.stVideoFrame.stVFrame.u64PhyAddr[0], VB_UID_PCIV);
        HI_ASSERT(s32Ret == HI_SUCCESS);

		PCIV_PicQueuePutFree(&pChn->stPicQueue, pNode);
	}

	pChn->stPicQueue.u32Max     = VDEC_MAX_SEND_CNT;
	pChn->stPicQueue.u32FreeNum = VDEC_MAX_SEND_CNT;
	pChn->stPicQueue.u32BusyNum = 0;
	//pChn->bStart = HI_TRUE;

	PCIVFMW_SPIN_UNLOCK;

    return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareRegisterFunc(PCIVFMW_CALLBACK_S *pstCallBack)
{
    PCIVFMW_CHECK_PTR(pstCallBack);
    PCIVFMW_CHECK_PTR(pstCallBack->pfSrcSendPic);
    PCIVFMW_CHECK_PTR(pstCallBack->pfRecvPicFree);
    PCIVFMW_CHECK_PTR(pstCallBack->pfQueryPcivChnShareBufState);

    memcpy(&g_stPcivFmwCallBack, pstCallBack, sizeof(PCIVFMW_CALLBACK_S));

    return HI_SUCCESS;
}


HI_S32 PCIV_FmwInit(void *p)
{
    HI_S32 i, s32Ret;
    BIND_SENDER_INFO_S stSenderInfo;
    BIND_RECEIVER_INFO_S stReceiver;
    VPSS_REGISTER_INFO_S stVpssRgstInfo;
    RGN_REGISTER_INFO_S stRgnInfo;

    if (PCIVFMW_STATE_STARTED == s_u32PcivFmwState)
    {
        return HI_SUCCESS;
    }

    spin_lock_init(&g_PcivFmwLock);

    stSenderInfo.enModId      = HI_ID_PCIV;
    stSenderInfo.u32MaxDevCnt = 1;
    stSenderInfo.u32MaxChnCnt = PCIV_MAX_CHN_NUM;
    s32Ret = CALL_SYS_RegisterSender(&stSenderInfo);
    HI_ASSERT(HI_SUCCESS == s32Ret);

    stReceiver.enModId        = HI_ID_PCIV;
    stReceiver.u32MaxDevCnt   = 1;
    stReceiver.u32MaxChnCnt   = PCIV_MAX_CHN_NUM;
    stReceiver.pCallBack      = PCIV_FirmWareSendPic;
	stReceiver.pResetCallBack = PcivFmw_ResetCallBack;
    s32Ret = CALL_SYS_RegisterReceiver(&stReceiver);
    HI_ASSERT(HI_SUCCESS == s32Ret);

    init_timer(&g_timerPcivSendVdecPic);
    g_timerPcivSendVdecPic.expires  = jiffies + msecs_to_jiffies(3*PCIV_TIMER_EXPIRES);
    g_timerPcivSendVdecPic.function = PCIV_SendVdecPicTimerFunc;
    g_timerPcivSendVdecPic.data     = 0;
    add_timer(&g_timerPcivSendVdecPic);

    g_stVbPool.u32PoolCount = 0;

    memset(g_stFwmPcivChn, 0, sizeof(g_stFwmPcivChn));
    for (i=0; i<PCIVFMW_MAX_CHN_NUM; i++)
    {
        g_stFwmPcivChn[i].bStart     = HI_FALSE;
        g_stFwmPcivChn[i].u32SendCnt = 0;
        g_stFwmPcivChn[i].u32GetCnt  = 0;
        g_stFwmPcivChn[i].u32RespCnt = 0;
        g_stFwmPcivChn[i].u32Count   = 0;
        g_stFwmPcivChn[i].u32RgnNum  = 0;
        init_timer(&g_stFwmPcivChn[i].stBufTimer);
        PCIV_FirmWareInitPreProcCfg(i);
    }

    if ((CKFN_VPSS_ENTRY()) && (CKFN_VPSS_Register()))
    {
        stVpssRgstInfo.enModId        = HI_ID_PCIV;
        stVpssRgstInfo.pVpssQuery     = PcivFmw_VpssQuery;
        stVpssRgstInfo.pVpssSend      = PcivFmw_VpssSend;
        stVpssRgstInfo.pResetCallBack = PcivFmw_ResetCallBack;
        s32Ret = CALL_VPSS_Register(&stVpssRgstInfo);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }

    /* register OVERLAY to region*/
    if ((CKFN_RGN()) && (CKFN_RGN_RegisterMod()))
    {
        RGN_CAPACITY_S stCapacity;

		/*the level of region*/
        stCapacity.pfnCheckAttr = HI_NULL;
        stCapacity.pfnCheckChnCapacity = HI_NULL;
        stCapacity.stLayer.bComm = HI_FALSE;

        stCapacity.stLayer.bSptReSet = HI_TRUE;
        stCapacity.stLayer.u32LayerMin = 0;
        stCapacity.stLayer.u32LayerMax = 7;

		/*the start position of region*/
        stCapacity.stPoint.bComm = HI_FALSE;
        stCapacity.stPoint.bSptReSet = HI_TRUE;
        stCapacity.stPoint.u32StartXAlign = 2;
        stCapacity.stPoint.u32StartYAlign = 2;
        stCapacity.stPoint.stPointMin.s32X = RGN_OVERLAYEX_MIN_X;
        stCapacity.stPoint.stPointMin.s32Y = RGN_OVERLAYEX_MIN_Y;
        stCapacity.stPoint.stPointMax.s32X = RGN_OVERLAYEX_MAX_X;
        stCapacity.stPoint.stPointMax.s32Y = RGN_OVERLAYEX_MAX_Y;

		/*the weight and height of region*/
        stCapacity.stSize.bComm = HI_TRUE;
        stCapacity.stSize.bSptReSet = HI_FALSE;
        stCapacity.stSize.u32WidthAlign = 2;
        stCapacity.stSize.u32HeightAlign = 2;
        stCapacity.stSize.stSizeMin.u32Width = 2;
        stCapacity.stSize.stSizeMin.u32Height = 2;
        stCapacity.stSize.stSizeMax.u32Width = RGN_OVERLAYEX_MAX_WIDTH;
        stCapacity.stSize.stSizeMax.u32Height = RGN_OVERLAYEX_MAX_HEIGHT;
        stCapacity.stSize.u32MaxArea = RGN_OVERLAYEX_MAX_WIDTH * RGN_OVERLAYEX_MAX_HEIGHT;

        /*the pixel format of region*/
        stCapacity.bSptPixlFmt = HI_TRUE;
        stCapacity.stPixlFmt.bComm = HI_TRUE;
        stCapacity.stPixlFmt.bSptReSet = HI_FALSE;
        stCapacity.stPixlFmt.u32PixlFmtNum = 3;
        stCapacity.stPixlFmt.aenPixlFmt[0] = PIXEL_FORMAT_ARGB_1555;
        stCapacity.stPixlFmt.aenPixlFmt[1] = PIXEL_FORMAT_RGB_444;
        stCapacity.stPixlFmt.aenPixlFmt[2] = PIXEL_FORMAT_RGB_888;

        /*the front Alpha of region*/
        stCapacity.bSptFgAlpha = HI_TRUE;
        stCapacity.stFgAlpha.bComm = HI_FALSE;
        stCapacity.stFgAlpha.bSptReSet = HI_TRUE;
        stCapacity.stFgAlpha.u32AlphaMax = 128;
        stCapacity.stFgAlpha.u32AlphaMin = 0;

        /*the back Alpha of region*/
        stCapacity.bSptBgAlpha = HI_TRUE;
        stCapacity.stBgAlpha.bComm = HI_FALSE;
        stCapacity.stBgAlpha.bSptReSet = HI_TRUE;
        stCapacity.stBgAlpha.u32AlphaMax = 128;
        stCapacity.stBgAlpha.u32AlphaMin = 0;

        /*the panoramic Alpha of region*/
        stCapacity.bSptGlobalAlpha = HI_FALSE;
        stCapacity.stGlobalAlpha.bComm = HI_FALSE;
        stCapacity.stGlobalAlpha.bSptReSet = HI_TRUE;
        stCapacity.stGlobalAlpha.u32AlphaMax = 128;
        stCapacity.stGlobalAlpha.u32AlphaMin = 0;

        /*suport backcolor or not */
        stCapacity.bSptBgClr = HI_TRUE;
        stCapacity.stBgClr.bComm = HI_TRUE;
        stCapacity.stBgClr.bSptReSet = HI_TRUE;

        /*rank*/
        stCapacity.stChnSort.bNeedSort = HI_TRUE;
        stCapacity.stChnSort.enRgnSortKey = RGN_SORT_BY_LAYER;
        stCapacity.stChnSort.bSmallToBig = HI_TRUE;

        /*QP*/
        stCapacity.bSptQp = HI_FALSE;
        stCapacity.stQp.bComm = HI_FALSE;
        stCapacity.stQp.bSptReSet = HI_TRUE;
        stCapacity.stQp.s32QpAbsMax = 51;
        stCapacity.stQp.s32QpAbsMin = 0;
        stCapacity.stQp.s32QpRelMax = 51;
        stCapacity.stQp.s32QpRelMin = -51;

        /*inverse color*/
        stCapacity.bSptInvColoar = HI_FALSE;
        stCapacity.stInvColor.bComm = HI_FALSE;
        stCapacity.stInvColor.bSptReSet = HI_TRUE;
        stCapacity.stInvColor.stSizeCap.stSizeMax.u32Height = 64;
        stCapacity.stInvColor.stSizeCap.stSizeMin.u32Height = 16;
        stCapacity.stInvColor.stSizeCap.stSizeMax.u32Width= 64;
        stCapacity.stInvColor.stSizeCap.stSizeMin.u32Width = 16;
        stCapacity.stInvColor.stSizeCap.u32HeightAlign = 16;
        stCapacity.stInvColor.stSizeCap.u32WidthAlign = 16;
        stCapacity.stInvColor.u32LumMax = 255;
        stCapacity.stInvColor.u32LumMin = 0;
        stCapacity.stInvColor.enInvModMax = MORETHAN_LUM_THRESH;
        stCapacity.stInvColor.enInvModMin = LESSTHAN_LUM_THRESH;
        stCapacity.stInvColor.u32StartXAlign = 16;
        stCapacity.stInvColor.u32StartYAlign = 16;
        stCapacity.stInvColor.u32WidthAlign = 16;
        stCapacity.stInvColor.u32HeightAlign = 16;
        stCapacity.stInvColor.stSizeCap.u32MaxArea = RGN_OVERLAYEX_MAX_WIDTH * RGN_OVERLAYEX_MAX_HEIGHT;

        /*suport bitmap or not */
        stCapacity.bSptBitmap = HI_TRUE;

        /*suport  Overlap or not*/
        stCapacity.bsptOverlap = HI_TRUE;

        /*memery STRIDE*/
        stCapacity.u32Stride = 64;

        stCapacity.u32RgnNumInChn = OVERLAYEX_MAX_NUM_PCIV;

        stRgnInfo.enModId      = HI_ID_PCIV;
        stRgnInfo.u32MaxDevCnt = 1;
        stRgnInfo.u32MaxChnCnt = PCIV_MAX_CHN_NUM;

        /* register OVERLAY */
        s32Ret = CALL_RGN_RegisterMod(OVERLAYEX_RGN, &stCapacity, &stRgnInfo);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }

    s_u32PcivFmwState = PCIVFMW_STATE_STARTED;
	return HI_SUCCESS;
}


HI_VOID PCIV_FmwExit(HI_VOID)
{
    HI_S32 i, s32Ret;
    if (PCIVFMW_STATE_STOPED == s_u32PcivFmwState)
    {
        return;
    }

    CALL_SYS_UnRegisterReceiver(HI_ID_PCIV);
    CALL_SYS_UnRegisterSender(HI_ID_PCIV);

    for (i=0; i<PCIVFMW_MAX_CHN_NUM; i++)
    {
        /* stop channel */
        (HI_VOID)PCIV_FirmWareStop(i);

        /* destroy channel */
        (HI_VOID)PCIV_FirmWareDestroy(i);

        del_timer_sync(&g_stFwmPcivChn[i].stBufTimer);
    }

    del_timer_sync(&g_timerPcivSendVdecPic);

    if ((CKFN_VPSS_ENTRY()) && (CKFN_VPSS_UnRegister()))
    {
        HI_ASSERT(HI_SUCCESS == CALL_VPSS_UnRegister(HI_ID_PCIV));
    }

    if ((CKFN_RGN()) && (CKFN_RGN_UnRegisterMod()))
    {
        /* unregister OVERLAY */
        s32Ret = CALL_RGN_UnRegisterMod(OVERLAYEX_RGN, HI_ID_PCIV);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }

    s_u32PcivFmwState = PCIVFMW_STATE_STOPED;
    return;
}

static HI_VOID PCIV_FmwNotify(MOD_NOTICE_ID_E enNotice)
{
    return;
}

static HI_VOID PCIV_FmwQueryState(MOD_STATE_E *pstState)
{
    *pstState = MOD_STATE_FREE;
    return;
}

#ifndef DISABLE_DEBUG_INFO
HI_S32 PCIV_FmwProcShow(osal_proc_entry_t *s)
{
    PCIV_FWMCHANNEL_S  *pstChnCtx;
    PCIV_CHN         pcivChn;
    HI_CHAR *pszPcivFieldStr[] = {"top","bottom","both"}; /* PCIV_PIC_FIELD_E */
	extern char proc_title[1024];

	osal_seq_printf(s, "\n[PCIVF] Version: ["MPP_VERSION"], %s",proc_title);
    //osal_seq_printf(s, "\n[PCIVF] Version: ["MPP_VERSION"], Build Time:["__DATE__", "__TIME__"]\n");

    osal_seq_printf(s, "\n-----PCIV FMW CHN INFO----------------------------------------------------------\n");
    osal_seq_printf(s, "%6s"     "%8s"   "%8s"    "%8s"     "%12s"    "%12s"     "%12s"     "%12s"     "%12s"   "%12s"     "%8s\n" ,
                  "PciChn", "Width","Height","Stride0","GetCnt","SendCnt","RespCnt","LostCnt","NewDo","OldUndo","PoolId0");
    for (pcivChn = 0; pcivChn < PCIVFMW_MAX_CHN_NUM; pcivChn++)
    {
        pstChnCtx = &g_stFwmPcivChn[pcivChn];
        if (HI_FALSE == pstChnCtx->bCreate) continue;
        osal_seq_printf(s, "%6d" "%8d" "%8d" "%8d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d" "%8d\n",
            pcivChn,
            pstChnCtx->stPicAttr.u32Width,
            pstChnCtx->stPicAttr.u32Height,
            pstChnCtx->stPicAttr.u32Stride[0],
            pstChnCtx->u32GetCnt,
            pstChnCtx->u32SendCnt,
            pstChnCtx->u32RespCnt,
            pstChnCtx->u32LostCnt,
            pstChnCtx->u32NewDoCnt,
            pstChnCtx->u32OldUndoCnt,
            pstChnCtx->au32PoolId[0]);
    }

    osal_seq_printf(s, "\n-----PCIV FMW CHN PREPROC INFO--------------------------------------------------\n");
    osal_seq_printf(s, "%6s"     "%8s"    "%8s\n"
                 ,"PciChn", "FiltT", "Field");
    for (pcivChn = 0; pcivChn < PCIVFMW_MAX_CHN_NUM; pcivChn++)
    {
        pstChnCtx = &g_stFwmPcivChn[pcivChn];
        if (HI_FALSE == pstChnCtx->bCreate) continue;
        osal_seq_printf(s, "%6d" "%8d" "%8s\n",
            pcivChn,
            pstChnCtx->stPreProcCfg.enFilterType,
            pszPcivFieldStr[pstChnCtx->stPreProcCfg.enFieldSel]);
    }

    osal_seq_printf(s, "\n-----PCIV CHN QUEUE INFO----------------------------------------------------------\n");
    osal_seq_printf(s, "%6s"     "%8s"     "%8s"     "%8s"    "%12s\n",
                  "PciChn", "busynum","freenum","state", "Timer");
    for (pcivChn = 0; pcivChn < PCIVFMW_MAX_CHN_NUM; pcivChn++)
    {
        pstChnCtx = &g_stFwmPcivChn[pcivChn];
        if (HI_FALSE == pstChnCtx->bCreate) continue;
        osal_seq_printf(s, "%6d" "%8d" "%8d" "%8d" "%12d\n",
            pcivChn,
            pstChnCtx->stPicQueue.u32BusyNum,
            pstChnCtx->stPicQueue.u32FreeNum,
            pstChnCtx->enSendState,
            pstChnCtx->u32TimerCnt);
    }

    osal_seq_printf(s, "\n-----PCIV CHN CALL VGS INFO----------------------------------------------------------\n");
    osal_seq_printf(s, "%6s"     "%12s"     "%12s"     "%12s"    "%12s"      "%12s"      "%12s"       "%12s"     "%12s"      "%12s"      "%12s"       "%12s"     "%12s"   "%12s\n",
                  "PciChn", "JobSuc","JobFail","EndSuc", "EndFail", "MoveSuc", "MoveFail", "OsdSuc", "OsdFail", "ZoomSuc", "ZoomFail", "MoveCb", "OsdCb","ZoomCb");
    for (pcivChn = 0; pcivChn < PCIVFMW_MAX_CHN_NUM; pcivChn++)
    {
        pstChnCtx = &g_stFwmPcivChn[pcivChn];
        if (HI_FALSE == pstChnCtx->bCreate) continue;
        osal_seq_printf(s, "%6d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d" "%12d\n",
            pcivChn,
            pstChnCtx->u32AddJobSucCnt,
            pstChnCtx->u32AddJobFailCnt,
            pstChnCtx->u32EndJobSucCnt,
            pstChnCtx->u32EndJobFailCnt,
            pstChnCtx->u32MoveTaskSucCnt,
            pstChnCtx->u32MoveTaskFailCnt,
            pstChnCtx->u32OsdTaskSucCnt,
            pstChnCtx->u32OsdTaskFailCnt,
            pstChnCtx->u32ZoomTaskSucCnt,
            pstChnCtx->u32ZoomTaskFailCnt,
            pstChnCtx->u32MoveCbCnt,
            pstChnCtx->u32OsdCbCnt,
            pstChnCtx->u32ZoomCbCnt);
    }

    return HI_SUCCESS;
}
#endif

static PCIV_FMWEXPORT_FUNC_S s_stExportFuncs = {
    .pfnPcivSendPic = PCIV_FirmWareViuSendPic,
};

static HI_U32 PCIV_GetVerMagic(HI_VOID)
{
	return VERSION_MAGIC;
}

static UMAP_MODULE_S s_stPcivFmwModule = {
    .enModId  		= HI_ID_PCIVFMW,
	.aModName   	= "pcivfmw",

    .pfnInit        = PCIV_FmwInit,
    .pfnExit        = PCIV_FmwExit,
    .pfnQueryState  = PCIV_FmwQueryState,
    .pfnNotify      = PCIV_FmwNotify,
    .pfnVerChecker  = PCIV_GetVerMagic,

    .pstExportFuncs = &s_stExportFuncs,
    .pData          = HI_NULL,
};

static int pcivfmwOpen(void *private_data)
{
    return 0;
}

static int pcivfmwClose(void *private_data)
{
    return 0;
}

static long pcivfmwIoctl(unsigned int  cmd, unsigned long arg,void *private_data)
{
	return 0;
}

static struct osal_fileops pcivfmw_fops =
{
	.open		= pcivfmwOpen,
	.unlocked_ioctl = pcivfmwIoctl,
	.release    = pcivfmwClose
};

static int __init PCIV_FmwModInit(void)
{
#ifndef DISABLE_DEBUG_INFO
    osal_proc_entry_t *proc;
#endif
	char buf[20];

	osal_snprintf(buf, 20, "%s", UMAP_DEVNAME_PCIVFMW_BASE);
	astPcivfmwDevice = osal_createdev(buf);
	if (NULL == astPcivfmwDevice)
    {
        printk("pciv: create device failed\n");
        return -1;
    }

    astPcivfmwDevice->fops  = &pcivfmw_fops;
    astPcivfmwDevice->minor = UMAP_PCIVFMW_MINOR_BASE;
    astPcivfmwDevice->osal_pmops = NULL;
    if (osal_registerdevice(astPcivfmwDevice)<0)
    {
		printk("Regist pciv device err.\n");
        return HI_FAILURE;
	}

    if (CMPI_RegisterModule(&s_stPcivFmwModule))
    {
        osal_deregisterdevice(astPcivfmwDevice);
		printk("Regist pciv firmware module err.\n");
        return HI_FAILURE;
    }

#ifndef DISABLE_DEBUG_INFO
    proc = osal_create_proc_entry(PROC_ENTRY_PCIVFMW, NULL);
    if (HI_NULL == proc)
    {
    	CMPI_UnRegisterModule(HI_ID_PCIVFMW);
		osal_deregisterdevice(astPcivfmwDevice);
        printk("PCIV firmware create proc error\n");
        return HI_FAILURE;
    }
	proc->read = PCIV_FmwProcShow;
#endif
    return HI_SUCCESS;
}


static void __exit PCIV_FmwModExit(void)
{
#ifndef DISABLE_DEBUG_INFO
    osal_remove_proc_entry(PROC_ENTRY_PCIVFMW,NULL);
#endif
	CMPI_UnRegisterModule(HI_ID_PCIVFMW);
	osal_deregisterdevice(astPcivfmwDevice);
	osal_destroydev(astPcivfmwDevice);

    return ;
}

EXPORT_SYMBOL(PCIV_FirmWareCreate);
EXPORT_SYMBOL(PCIV_FirmWareDestroy);
EXPORT_SYMBOL(PCIV_FirmWareSetAttr);
EXPORT_SYMBOL(PCIV_FirmWareStart);
EXPORT_SYMBOL(PCIV_FirmWareStop);
EXPORT_SYMBOL(PCIV_FirmWareWindowVbCreate);
EXPORT_SYMBOL(PCIV_FirmWareWindowVbDestroy);
EXPORT_SYMBOL(PCIV_FirmWareMalloc);
EXPORT_SYMBOL(PCIV_FirmWareFree);
EXPORT_SYMBOL(PCIV_FirmWareMallocChnBuffer);
EXPORT_SYMBOL(PCIV_FirmWareFreeChnBuffer);
EXPORT_SYMBOL(PCIV_FirmWareRecvPicAndSend);
EXPORT_SYMBOL(PCIV_FirmWareSrcPicFree);
EXPORT_SYMBOL(PCIV_FirmWareRegisterFunc);
EXPORT_SYMBOL(PCIV_FirmWareSetPreProcCfg);
EXPORT_SYMBOL(PCIV_FirmWareGetPreProcCfg);

module_param(drop_err_timeref, uint, S_IRUGO);
MODULE_PARM_DESC(drop_err_timeref, "whether drop err timeref video frame. 1:yes,0:no.");

module_init(PCIV_FmwModInit);
module_exit(PCIV_FmwModExit);

MODULE_AUTHOR("HiMPP GRP");
MODULE_LICENSE("GPL");
MODULE_VERSION(MPP_VERSION);
