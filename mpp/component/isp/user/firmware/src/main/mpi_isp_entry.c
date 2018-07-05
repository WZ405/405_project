/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : mpi_isp_entry.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2011/01/05
  Description   :
  History       :
  1.Date        : 2011/01/05
    Author      :
    Modification: Created file

******************************************************************************/

#include <stdio.h>
#include <string.h>
#if 0
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include <sys/ioctl.h>
#include <signal.h>

#include "mpi_sys.h"
#include "mkp_isp.h"
#include "hi_isp_debug.h"
#include "isp_debug.h"
#include "isp_defaults.h"
#include "isp_main.h"
#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_statistics.h"
#include "isp_regcfg.h"
#include "isp_proc.h"
#include "hi_vreg.h"
#include "isp_config.h"
#include "isp_ext_config.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/* Isp Version Proc will be Hi3518_ISP_V1.0.4.x, ISP_LIB_VERSION stands for x */
#define ISP_LIB_VERSION "0"    //[0~F]

/****************************************************************************
 * GLOBAL VARIABLES                                                         *
 ****************************************************************************/

ISP_CTX_S  g_astIspCtx[ISP_MAX_PIPE_NUM] = {{0}};
HI_S32     g_as32IspFd[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = -1};

#ifdef _MSC_VER
#define MUTEX_INIT_LOCK(mutex) InitializeCriticalSection(&mutex)
#define MUTEX_LOCK(mutex) EnterCriticalSection(&mutex)
#define MUTEX_UNLOCK(mutex) LeaveCriticalSection(&mutex)
#define MUTEX_DESTROY(mutex) DeleteCriticalSection(&mutex)
#else
#define MUTEX_INIT_LOCK(mutex) \
    do \
    { \
        (void)pthread_mutex_init(&mutex, NULL); \
    }while(0)
#define MUTEX_LOCK(mutex) \
    do \
    { \
        (void)pthread_mutex_lock(&mutex); \
    }while(0)
#define MUTEX_UNLOCK(mutex) \
    do \
    { \
        (void)pthread_mutex_unlock(&mutex); \
    }while(0)
#define MUTEX_DESTROY(mutex) \
    do \
    { \
        (void)pthread_mutex_destroy(&mutex); \
    }while(0)
#endif

static ISP_VERSION_S gs_stVersion =
{
    .au8MppVersion = ISP_LIB_VERSION,
    .au8Date = __DATE__,
    .au8Time = __TIME__,
    .u32Magic = 0,
};

/****************************************************************************
 * MACRO DEFINITION                                                         *
 ****************************************************************************/

#define ISP_CHECK_SENSOR_REGISTER(dev)\
    do {\
        if (g_astIspCtx[dev].bSnsReg != HI_TRUE)\
        {\
            ISP_TRACE(HI_DBG_ERR, "Sensor doesn't register to ISP[%d]!\n", dev);\
            return HI_ERR_ISP_SNS_UNREGISTER;\
        }\
    }while(0)

#define ISP_VERSION_MAGIC    20130305

/****************************************************************************
 * static functions
 ****************************************************************************/
static HI_BOOL ISP_GetVersion(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;

    ISP_CHECK_OPEN(ViPipe);

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_GET_VERSION, &gs_stVersion);
    if (s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "register ISP[%d] lib info ec %x!\n", ViPipe, s32Ret);
        return HI_FALSE;
    }

    /* 20130314, disable isp magic version check */
#if 0
    if (gs_stVersion.u32Magic != ISP_VERSION_MAGIC)
    {
        printf("isp lib version doesn't match with sdk!\n");
        return HI_FALSE;
    }
#endif

    return HI_TRUE;
}

/*****************************************************************************
 Prototype       : HI_MPI_ISP_SetModParam
 Description     : isp module parameter
 Input           : I_VOID  **
 Output          : None
 Return Value    :
 Process         :
 Note            :

  History
  1.Date         : 2018/2/3
    Author       :
    Modification : Created function

*****************************************************************************/

HI_S32 HI_MPI_ISP_SetModParam(const ISP_MOD_PARAM_S *pstModParam)
{
    ISP_CHECK_PIPE(0);
    ISP_CHECK_POINTER(pstModParam);
    ISP_CHECK_OPEN(0);

    return  ioctl(g_as32IspFd[0], ISP_SET_MOD_PARAM, pstModParam);
}

HI_S32 HI_MPI_ISP_GetModParam(ISP_MOD_PARAM_S *pstModParam)
{
    ISP_CHECK_PIPE(0);
    ISP_CHECK_POINTER(pstModParam);
    ISP_CHECK_OPEN(0);

    return ioctl(g_as32IspFd[0], ISP_GET_MOD_PARAM, pstModParam);
}

/*****************************************************************************
 Prototype       : HI_MPI_ISP_MemInit
 Description     : isp initial extent memory
 Input           : I_VOID  **
 Output          : None
 Return Value    :
 Process         :
 Note            :

  History
  1.Date         : 2017/8/13
    Author       :
    Modification : Created function

*****************************************************************************/
HI_S32 HI_MPI_ISP_MemInit(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    /* 1. check status */
    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);
    ISP_CHECK_OPEN(ViPipe);
    ISP_CHECK_SENSOR_REGISTER(ViPipe);

    if (ioctl(g_as32IspFd[ViPipe], ISP_MEM_INFO_GET, &pstIspCtx->bMemInit))
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] get Mem info failed!\n", ViPipe);
        return HI_ERR_ISP_MEM_NOT_INIT;
    }

    if (HI_TRUE == pstIspCtx->bMemInit)
    {
        VReg_ReleaseAll();
        pstIspCtx->bMemInit = HI_FALSE;
        if (ioctl(g_as32IspFd[ViPipe], ISP_MEM_INFO_SET, &pstIspCtx->bMemInit))
        {
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    /* 2. Get info: PipeBindDev, RunningMode, WDR, HDR, Stitch */
    /* WDR attribute */
    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_GET_WDR_ATTR, &pstIspCtx->stWdrAttr);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] get WDR attr failed\n", ViPipe);
        return s32Ret;
    }

    /* Wdr mode abnormal */
    if ((!pstIspCtx->stWdrAttr.bMastPipe) &&
        (IS_WDR_MODE(pstIspCtx->stWdrAttr.enWDRMode)))
    {
        return HI_SUCCESS;
    }

    /* Get pipe size */
    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_GET_PIPE_SIZE, &pstIspCtx->stPipeSize);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] get Pipe size failed\n", ViPipe);
        return s32Ret;
    }

    /* HDR attribute */
    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_GET_HDR_ATTR, &pstIspCtx->stHdrAttr);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] get HDR attr failed\n", ViPipe);
        return s32Ret;
    }

    /* Stitch attribute */
    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_GET_STITCH_ATTR, &pstIspCtx->stStitchAttr);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] get Stitch attr failed\n", ViPipe);
        return s32Ret;
    }

    if (HI_TRUE != ISP_GetVersion(ViPipe))
    {
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    /* 3. creat extern registers */
    s32Ret = VReg_Init(ISP_VIR_REG_BASE(ViPipe), ISP_VREG_SIZE);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] init Mem failed\n", ViPipe);
        return s32Ret;
    }

    s32Ret = ISP_BlockInit(ViPipe, &pstIspCtx->stBlockAttr);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] init block failed!\n", ViPipe);
        s32Ret = HI_ERR_ISP_MEM_NOT_INIT;
        goto fail0;
    }

    s32Ret = ISP_CfgBeBufInit(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] init be buf failed\n", ViPipe);
        s32Ret = HI_ERR_ISP_MEM_NOT_INIT;
        goto fail0;
    }

    s32Ret = ISP_GetBeBufFirst(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] get BeBuf init failed\n", ViPipe);
        s32Ret = HI_ERR_ISP_MEM_NOT_INIT;
        goto fail1;
    }

    s32Ret = ISP_BeVregAddrInit(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] get cfgs buf addr failed!\n", ViPipe);
        s32Ret = HI_ERR_ISP_MEM_NOT_INIT;
        goto fail1;
    }

    s32Ret = ISP_SttBufInit(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] init stt buffer failed!\n", ViPipe);
        s32Ret = HI_ERR_ISP_MEM_NOT_INIT;
        goto fail1;
    }

    s32Ret = ISP_SttAddrInit(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] init stt address failed!\n", ViPipe);
        s32Ret = HI_ERR_ISP_MEM_NOT_INIT;
        goto fail2;
    }

    s32Ret = ISP_ClutBufInit(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] clut buffer init failed\n", ViPipe);
        s32Ret = HI_ERR_ISP_MEM_NOT_INIT;
        goto fail2;

    }

    s32Ret = ISP_SpecAwbBufInit(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] specAwb buffer init failed\n", ViPipe);
        s32Ret = HI_ERR_ISP_MEM_NOT_INIT;
        goto fail3;
    }
    memset(&pstIspCtx->stIspParaRec, 0, sizeof(ISP_PARA_REC_S));
    hi_ext_top_wdr_cfg_write(ViPipe, pstIspCtx->stIspParaRec.bWDRCfg);
    hi_ext_top_pub_attr_cfg_write(ViPipe, pstIspCtx->stIspParaRec.bPubCfg);

    hi_ext_top_wdr_switch_write(ViPipe, HI_FALSE);
    hi_ext_top_res_switch_write(ViPipe, HI_FALSE);

    pstIspCtx->bMemInit = HI_TRUE;
    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_MEM_INFO_SET, &pstIspCtx->bMemInit);
    if (s32Ret)
    {
        pstIspCtx->bMemInit = HI_FALSE;
        //VReg_ReleaseAll();

        ISP_TRACE(HI_DBG_ERR, "ISP[%d] set Mem info failed!\n", ViPipe);
        s32Ret = HI_ERR_ISP_MEM_NOT_INIT;
        goto fail4;
    }

    return HI_SUCCESS;

fail4:
    ISP_SpecAwbBufExit(ViPipe);

fail3:
    ISP_ClutBufExit(ViPipe);
fail2:
    ISP_SttBufExit(ViPipe);
fail1:
    ISP_CfgBeBufExit(ViPipe);
fail0:
    VReg_Exit(ISP_VIR_REG_BASE(ViPipe), ISP_VREG_SIZE);

    return s32Ret;
}

/*****************************************************************************
 Prototype       : HI_MPI_ISP_Init
 Description     : isp initial process, include extent memory, top structure,
                    default value, etc.
 Input           : I_VOID  **
 Output          : None
 Return Value    :
 Process         :
 Note             :

  History
  1.Date         : 2017/1/13
    Author       :
    Modification : Created function

*****************************************************************************/
HI_S32 HI_MPI_ISP_Init(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    HI_VOID *pRegCfg = HI_NULL;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    //HI_BOOL bHDREnable  ;
    HI_U8 u8WDRMode;
    HI_U32 u32Value = 0;
    HI_VOID *pValue = HI_NULL;
    ISP_CMOS_SENSOR_IMAGE_MODE_S stSnsImageMode;
    ISP_CMOS_DEFAULT_S *pstSnsDft = HI_NULL;
    ISP_SNS_REGS_INFO_S *pstSnsRegsInfo = NULL;

    /* 1. check status */
    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);
    ISP_CHECK_OPEN(ViPipe);
    ISP_CHECK_SENSOR_REGISTER(ViPipe);
    ISP_CHECK_MEM_INIT(ViPipe);

    /* Wdr mode abnormal */
    if ((!pstIspCtx->stWdrAttr.bMastPipe) &&
        (IS_WDR_MODE(pstIspCtx->stWdrAttr.enWDRMode)))
    {
        return HI_SUCCESS;
    }

    pstIspCtx->stIspParaRec.bWDRCfg = hi_ext_top_wdr_cfg_read(ViPipe);
    ISP_CHECK_ISP_WDR_CFG(ViPipe);

    pstIspCtx->stIspParaRec.bPubCfg = hi_ext_top_pub_attr_cfg_read(ViPipe);
    ISP_CHECK_ISP_PUB_ATTR_CFG(ViPipe);

    if (HI_TRUE == pstIspCtx->stIspParaRec.bInit)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] Init failed!\n", ViPipe);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    /* 2. init statistics bufs. */
    s32Ret = ISP_StatisticsInit(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] Init statistics failed\n", ViPipe);
        goto fail0;
    }

    /* 3. init proc bufs. */
    s32Ret = ISP_ProcInit(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        goto fail1;
    }

    /* 4. init ispupdate bufs. */
    s32Ret = ISP_UpdateInfoInit(ViPipe, &pstIspCtx->stUpdateInfoCtrl);
    if (HI_SUCCESS != s32Ret)
    {
        goto fail2;
    }

    /*  init frame info bufs. */
    s32Ret = ISP_FrameInfoInit(ViPipe, &pstIspCtx->stFrameInfoCtrl);
    if (HI_SUCCESS != s32Ret)
    {
        goto fail3;
    }

    /*  init attach info bufs. */
    s32Ret = ISP_AttachInfoInit(ViPipe, &pstIspCtx->stAttachInfoCtrl);
    if (HI_SUCCESS != s32Ret)
    {
        goto fail4;
    }

    /*  init attach info bufs. */
    s32Ret = ISP_ColorGamutInfoInit(ViPipe, &pstIspCtx->stColorGamutInfoCtrl);
    if (HI_SUCCESS != s32Ret)
    {
        goto fail5;
    }

    s32Ret = ISP_DngInfoInit(ViPipe, &pstIspCtx->stDngInfoCtrl);
    if (HI_SUCCESS != s32Ret)
    {
        goto fail6;
    }
    s32Ret = ISP_ProNrParamInit(ViPipe, &pstIspCtx->stProNrParamCtrl);
    if (HI_SUCCESS != s32Ret)
    {
        goto fail7;
    }
    s32Ret = ISP_ProShpParamInit(ViPipe, &pstIspCtx->stProShpParamCtrl);
    if (HI_SUCCESS != s32Ret)
    {
        goto fail8;
    }
    //bHDREnable = (DYNAMIC_RANGE_XDR == pstIspCtx->stHdrAttr.enDynamicRange) ? 1: 0;

    /* 5. set WDR mode to sensor and update isp param from sensor and sensor init */
    //u8WDRMode = (((pstIspCtx->stHdrAttr.bHDREnable) &0x1 ) << 0x6);
    //u8WDRMode = u8WDRMode | hi_ext_system_sensor_wdr_mode_read(ViPipe);
    u8WDRMode = hi_ext_system_sensor_wdr_mode_read(ViPipe);
    s32Ret = ISP_SensorSetWDRMode(ViPipe, u8WDRMode);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] set sensor WDR mode failed!\n", ViPipe);
        s32Ret = HI_ERR_ISP_NOT_INIT;
        goto fail9;
    }

    stSnsImageMode.u16Width = hi_ext_top_sensor_width_read(ViPipe);
    stSnsImageMode.u16Height = hi_ext_top_sensor_height_read(ViPipe);
    u32Value = hi_ext_system_fps_base_read(ViPipe);
    pValue = (HI_VOID *)&u32Value;
    stSnsImageMode.f32Fps = *(HI_FLOAT *)pValue;
    stSnsImageMode.u8SnsMode = hi_ext_system_sensor_mode_read(ViPipe);
    s32Ret = ISP_SensorSetImageMode(ViPipe, &stSnsImageMode);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] set sensor image mode failed!\n", ViPipe);
        s32Ret = HI_ERR_ISP_NOT_INIT;
        goto fail9;
    }

#if 0
    s32Ret = ISP_SensorInit(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] init sensor failed!\n", ViPipe);
        s32Ret = HI_ERR_ISP_NOT_INIT;
        goto fail7;
    }
#endif

    s32Ret = ISP_SensorUpdateAll(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] update sensor all failed!\n", ViPipe);
        s32Ret = HI_ERR_ISP_NOT_INIT;
        goto fail9;
    }

    ISP_SensorGetDefault(ViPipe, &pstSnsDft);
    pstIspCtx->stFrameInfoCtrl.pstIspFrame->u32SensorID = pstSnsDft->stSensorMode.u32SensorID;
    pstIspCtx->stFrameInfoCtrl.pstIspFrame->u32SensorMode = pstSnsDft->stSensorMode.u8SensorMode;

    /*Get Dng paramters from CMOS.c*/
    ISP_DngExtRegsInitialize(ViPipe, (ISP_DNG_COLORPARAM_S *)&pstSnsDft->stDngColorParam);
    hi_ext_system_dng_static_info_valid_write(ViPipe, pstSnsDft->stSensorMode.bValidDngRawFormat);
    if (HI_TRUE == pstSnsDft->stSensorMode.bValidDngRawFormat)
    {
        memcpy(&pstIspCtx->stDngInfoCtrl.pstIspDng->stDngRawFormat, &pstSnsDft->stSensorMode.stDngRawFormat, sizeof(DNG_RAW_FORMAT_S));
    }
    else
    {
        printf("Dng image static info have not been initialized in Cmos.c!\n");
    }

    /* 6. init the common part of extern registers and real registers */
    ISP_ExtRegsDefault(ViPipe);
    ISP_RegsDefault(ViPipe);
    ISP_ExtRegsInitialize(ViPipe);
    ISP_RegsInitialize(ViPipe);

    /* 7. isp algs global variable initialize */
    ISP_GlobalInitialize(ViPipe);

    /* 8. register all algorithm to isp, and init them. */
    ISP_AlgsRegister(ViPipe);

    /* 9. get regcfg */
    ISP_RegCfgInit(ViPipe, &pRegCfg);
    ISP_AlgsInit(pstIspCtx->astAlgs, ViPipe, pRegCfg);

    s32Ret = ISP_SensorInit(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] init sensor failed!\n", ViPipe);
        s32Ret = HI_ERR_ISP_NOT_INIT;
        goto fail10;
    }
    ISP_SensorGetSnsReg(ViPipe, &pstSnsRegsInfo);
    pstIspCtx->stLinkage.u8Cfg2ValidDelayMax = pstSnsRegsInfo->u8Cfg2ValidDelayMax;

    /* 10. regcfg info set */
    s32Ret = ISP_RegCfgInfoInit(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] set regcfgs info failed!\n", ViPipe);
        s32Ret = HI_ERR_ISP_NOT_INIT;
        goto fail11;
    }

    /* 11. set WDR mode to kernel. */
    s32Ret = ISP_WDRCfgSet(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] set WDR mode to kernel failed!\n", ViPipe);
        s32Ret = HI_ERR_ISP_NOT_INIT;
        goto fail11;
    }

    pstIspCtx->stIspParaRec.bStitchSync = HI_TRUE;
    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_SYNC_INIT_SET, &pstIspCtx->stIspParaRec.bStitchSync);
    if (s32Ret)
    {
        pstIspCtx->stIspParaRec.bStitchSync = HI_FALSE;
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] set isp stitch sync failed!\n", ViPipe);
        s32Ret = HI_ERR_ISP_NOT_INIT;
        goto fail11;
    }

    /* init isp be cfgs all buffer */
    s32Ret = ISP_AllCfgsBeBufInit(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] init Becfgs buffer failed!\n", ViPipe);
        s32Ret = HI_ERR_ISP_NOT_INIT;
        goto fail11;
    }


    /* 12. init isp global variables */
    if (pthread_mutex_init(&pstIspCtx->stLock, HI_NULL) != 0)
    {
        s32Ret = HI_FAILURE;
        goto fail11;
    }

    MUTEX_LOCK(pstIspCtx->stLock);
    pstIspCtx->stIspParaRec.bInit = HI_TRUE;
    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_INIT_INFO_SET, &pstIspCtx->stIspParaRec.bInit);
    if (s32Ret)
    {
        pstIspCtx->stIspParaRec.bInit = HI_FALSE;
        MUTEX_UNLOCK(pstIspCtx->stLock);
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] set isp init info failed!\n", ViPipe);
        s32Ret = HI_ERR_ISP_NOT_INIT;
        goto fail12;
    }
    pstIspCtx->stIspParaRec.bRunEn = HI_TRUE;
    MUTEX_UNLOCK(pstIspCtx->stLock);

    return HI_SUCCESS;

fail12:
    pthread_mutex_destroy(&pstIspCtx->stLock);
fail11:
    ISP_SensorExit(ViPipe);
fail10:
    ISP_AlgsExit(pstIspCtx->astAlgs, ViPipe);
    ISP_AlgsUnRegister(ViPipe);
fail9:
    ISP_ProShpParamExit(ViPipe, &pstIspCtx->stProShpParamCtrl);
fail8:
    ISP_ProNrParamExit(ViPipe, &pstIspCtx->stProNrParamCtrl);
fail7:
    ISP_DngInfoExit(ViPipe,  &pstIspCtx->stDngInfoCtrl);
fail6:
    ISP_ColorGamutInfoExit(ViPipe, &pstIspCtx->stColorGamutInfoCtrl);
fail5:
    ISP_AttachInfoExit(ViPipe, &pstIspCtx->stAttachInfoCtrl);
fail4:
    ISP_FrameInfoExit(ViPipe, &pstIspCtx->stFrameInfoCtrl);
fail3:
    ISP_UpdateInfoExit(ViPipe, &pstIspCtx->stUpdateInfoCtrl);
fail2:
    ISP_ProcExit(ViPipe);
fail1:
    ISP_StatisticsExit(ViPipe);
fail0:
    //VReg_Exit(ISP_VREG_BASE, ISP_VREG_SIZE);

    return s32Ret;
}

/*When offline mode user send raw to BE, firstly need call this function to insure paramters ready*/
HI_S32 HI_MPI_ISP_RunOnce(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    //HI_BOOL bEn;
    HI_U32 u32WDRmode;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    /* 1. check status */
    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);
    ISP_CHECK_OPEN(ViPipe);
    ISP_CHECK_SENSOR_REGISTER(ViPipe);
    ISP_CHECK_MEM_INIT(ViPipe);

    ISP_CHECK_ISP_INIT(ViPipe);

    /* Wdr mode abnormal */
    if ((!pstIspCtx->stWdrAttr.bMastPipe) &&
        (IS_WDR_MODE(pstIspCtx->stWdrAttr.enWDRMode)))
    {
        return HI_SUCCESS;
    }

    if (HI_TRUE == pstIspCtx->stIspParaRec.bRun)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] Run failed!\n", ViPipe);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    MUTEX_LOCK(pstIspCtx->stLock);

    /* Sometimes HI_MPI_ISP_Run thread is not scheduled to run before calling HI_MPI_ISP_Exit. */
    if (HI_FALSE == pstIspCtx->stIspParaRec.bRunEn)
    {
        MUTEX_UNLOCK(pstIspCtx->stLock);
        return HI_SUCCESS;
    }

    pstIspCtx->stIspParaRec.bRun = HI_TRUE;

    /*change  resolution  */
    ISP_SwitchImageMode(ViPipe);

    u32WDRmode = hi_ext_system_sensor_wdr_mode_read(ViPipe);
    /* swtich linear/WDR mode, width/height, fps  */
    if (pstIspCtx->u8SnsWDRMode != u32WDRmode)
    {
        pstIspCtx->u8SnsWDRMode = u32WDRmode;
        ISP_SwitchWDRMode(ViPipe);
    }

    pstIspCtx->stLinkage.bRunOnce = HI_TRUE;
    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_OPT_RUNONCE_INFO, &pstIspCtx->stLinkage.bRunOnce);
    if (HI_SUCCESS != s32Ret)
    {
        pstIspCtx->stLinkage.bRunOnce = HI_FALSE;
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] set runonce info failed!\n", ViPipe);
        MUTEX_UNLOCK(pstIspCtx->stLock);
        return s32Ret;
    }

    ISP_Run(ViPipe);

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_KERNEL_RUNONCE);
    if (HI_SUCCESS != s32Ret)
    {
        MUTEX_UNLOCK(pstIspCtx->stLock);
        return s32Ret;
    }

    pstIspCtx->stIspParaRec.bRun = HI_FALSE;

    MUTEX_UNLOCK(pstIspCtx->stLock);

    return HI_SUCCESS;
}


/*****************************************************************************
 Prototype       : HI_MPI_ISP_Run
 Description     : isp firmware recurrent task, always run in a single thread.
 Input           : I_VOID  **
 Output          : None
 Return Value    :
 Process         :
 Note             :

  History
  1.Date         : 2011/1/13
    Author       :
    Modification : Created function

*****************************************************************************/
HI_S32 HI_MPI_ISP_Run(VI_PIPE ViPipe)
{
    HI_BOOL bEn;
    HI_S32 s32Ret;
    HI_U32 u32IntStatus = 0;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    /* 1. check status */
    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);
    ISP_CHECK_OPEN(ViPipe);
    ISP_CHECK_SENSOR_REGISTER(ViPipe);
    ISP_CHECK_MEM_INIT(ViPipe);

    ISP_CHECK_ISP_INIT(ViPipe);

    /* Wdr mode abnormal */
    if ((!pstIspCtx->stWdrAttr.bMastPipe) &&
        (IS_WDR_MODE(pstIspCtx->stWdrAttr.enWDRMode)))
    {
        return HI_SUCCESS;
    }

    if (HI_TRUE == pstIspCtx->stIspParaRec.bRun)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] Run failed!\n", ViPipe);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    MUTEX_LOCK(pstIspCtx->stLock);

    /* Sometimes ISP run thread is not scheduled to run before calling ISP exit. */
    if (HI_FALSE == pstIspCtx->stIspParaRec.bRunEn)
    {
        MUTEX_UNLOCK(pstIspCtx->stLock);
        return HI_SUCCESS;
    }

    /* 2. enable interrupt */
    bEn = HI_TRUE;
    if (ioctl(g_as32IspFd[ViPipe], ISP_SET_INT_ENABLE, &bEn) < 0)
    {
        ISP_TRACE(HI_DBG_ERR, "Enable ISP[%d] interrupt failed!\n", ViPipe);
        MUTEX_UNLOCK(pstIspCtx->stLock);
        return -1;
    }

    pstIspCtx->stIspParaRec.bRun = HI_TRUE;
    MUTEX_UNLOCK(pstIspCtx->stLock);

    while (1)
    {
        MUTEX_LOCK(pstIspCtx->stLock);

        if (HI_FALSE == pstIspCtx->stIspParaRec.bRunEn)
        {
            MUTEX_UNLOCK(pstIspCtx->stLock);
            break;
        }

        /* 3. Change WDR mode  */
        s32Ret  = ISP_SwitchWDRMode(ViPipe);
        if (HI_SUCCESS != s32Ret)
        {
            ISP_TRACE(HI_DBG_ERR, "ISP[%d] switch WDR mode failed!\n", ViPipe);
            MUTEX_UNLOCK(pstIspCtx->stLock);
            break;
        }

        /* 4. Change resolution  */
        s32Ret = ISP_SwitchImageMode(ViPipe);
        if (HI_SUCCESS != s32Ret)
        {
            ISP_TRACE(HI_DBG_ERR, "ISP[%d] switch image mode failed!\n", ViPipe);
            MUTEX_UNLOCK(pstIspCtx->stLock);
            break;
        }

        MUTEX_UNLOCK(pstIspCtx->stLock);

        {
            u32IntStatus = 0;
            /* 5. waked up by the interrupt */
            s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_GET_FRAME_EDGE, &u32IntStatus);
            if (HI_SUCCESS == s32Ret)
            {
                /* 6.isp firmware calculate, include AE/AWB, etc. */
                if (ISP_1ST_INT & u32IntStatus)
                {
                    MUTEX_LOCK(pstIspCtx->stLock);

                    if (HI_FALSE == pstIspCtx->stIspParaRec.bRunEn)
                    {
                        MUTEX_UNLOCK(pstIspCtx->stLock);
                        break;
                    }

                    ISP_Run(ViPipe);

                    MUTEX_UNLOCK(pstIspCtx->stLock);
                }
            }
        }
    }

    /* 7. disable interrupt */
    bEn = HI_FALSE;
    if (ioctl(g_as32IspFd[ViPipe], ISP_SET_INT_ENABLE, &bEn) < 0)
    {
        ISP_TRACE(HI_DBG_ERR, "Disable ISP[%d] interrupt failed!\n", ViPipe);
    }

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       : HI_MPI_ISP_Exit
 Description     : control isp to exit recurrent task, always run in main process.
 Input           : I_VOID  **
 Output          : None
 Return Value    :
 Process         :
 Note             :

  History
  1.Date         : 2011/1/13
    Author       :
    Modification : Created function

*****************************************************************************/
HI_S32 HI_MPI_ISP_Exit(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;

    /* 1. Check status */
    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);
    ISP_CHECK_OPEN(ViPipe);
    ISP_CHECK_MEM_INIT(ViPipe);

    /* Wdr mode abnormal */
    if ((!pstIspCtx->stWdrAttr.bMastPipe) &&
        (IS_WDR_MODE(pstIspCtx->stWdrAttr.enWDRMode)))
    {
        return HI_SUCCESS;
    }

    ISP_StitchSyncExit(ViPipe);

    MUTEX_LOCK(pstIspCtx->stLock);

    /* 2. exit global variables */
    pstIspCtx->stIspParaRec.bInit = HI_FALSE;
    pstIspCtx->stIspParaRec.bStitchSync = HI_FALSE;
    pstIspCtx->bMemInit = HI_FALSE;

    if (ioctl(g_as32IspFd[ViPipe], ISP_RESET_CTX))
    {
        MUTEX_UNLOCK(pstIspCtx->stLock);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    memset(&pstIspCtx->stIspParaRec, 0, sizeof(ISP_PARA_REC_S));
    hi_ext_top_wdr_cfg_write(ViPipe, pstIspCtx->stIspParaRec.bWDRCfg);
    hi_ext_top_pub_attr_cfg_write(ViPipe, pstIspCtx->stIspParaRec.bPubCfg);

    /* 3. exit algorthmics */
    ISP_AlgsExit(pstIspCtx->astAlgs, ViPipe);

    /* 4. unregister algorthmics, 3a libs */
    ISP_AlgsUnRegister(ViPipe);
    ISP_LibsUnRegister(ViPipe);

    /* 5. sensor exit */
    ISP_SensorExit(ViPipe);

    /* 6. release proc bufs. */
    ISP_ProcExit(ViPipe);

    /* 7. exit be bufs. */
    ISP_SpecAwbBufExit(ViPipe);
    ISP_ClutBufExit(ViPipe);
    ISP_CfgBeBufExit(ViPipe);
    ISP_SttBufExit(ViPipe);
    ISP_BlockExit(ViPipe);
    ISP_ProShpParamExit(ViPipe, &pstIspCtx->stProShpParamCtrl);
    ISP_ProNrParamExit(ViPipe, &pstIspCtx->stProNrParamCtrl);

    ISP_DngInfoExit( ViPipe,  &pstIspCtx->stDngInfoCtrl);

    ISP_ColorGamutInfoExit(ViPipe, &pstIspCtx->stColorGamutInfoCtrl);

    /* 8. exit frame bufs. */
    ISP_AttachInfoExit(ViPipe, &pstIspCtx->stAttachInfoCtrl);

    /* 9. exit frame bufs. */
    ISP_FrameInfoExit(ViPipe, &pstIspCtx->stFrameInfoCtrl);

    /* 10. exit dcf bufs. */
    ISP_UpdateInfoExit(ViPipe, &pstIspCtx->stUpdateInfoCtrl);

    /* 11. exit statistics bufs. */
    ISP_StatisticsExit(ViPipe);

    /* 12. release vregs */
    VReg_Munmap(ISP_FE_REG_BASE(ViPipe), FE_REG_SIZE_ALIGN);
    VReg_Munmap(ISP_BE_REG_BASE(ViPipe), BE_REG_SIZE_ALIGN);
    VReg_Exit(ISP_VIR_REG_BASE(ViPipe), ISP_VREG_SIZE);

    MUTEX_UNLOCK(pstIspCtx->stLock);

    if (0 != pthread_mutex_destroy(&pstIspCtx->stLock))
    {
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       : HI_MPI_ISP_SetRegister
 Description     : set isp register, include extent memory.
 Input           : u32Addr   **
                   u32Value  **
 Output          : None
 Return Value    :
 Process         :
 Note             :

  History
  1.Date         : 2011/1/13
    Author       :
    Modification : Created function

*****************************************************************************/
HI_S32 HI_MPI_ISP_SetRegister(VI_PIPE ViPipe, HI_U32 u32Addr, HI_U32 u32Value)
{
    ISP_CHECK_PIPE(ViPipe);
    IO_WRITE32(u32Addr, u32Value);

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       : HI_MPI_ISP_GetRegister
 Description     : get isp register, include extent memory.
 Input           : u32Addr    **
                   pu32Value  **
 Output          : None
 Return Value    :
 Process         :
 Note             :

  History
  1.Date         : 2011/1/13
    Author       :
    Modification : Created function

*****************************************************************************/
HI_S32 HI_MPI_ISP_GetRegister(VI_PIPE ViPipe, HI_U32 u32Addr, HI_U32 *pu32Value)
{
    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_POINTER(pu32Value);
    *pu32Value = IO_READ32(u32Addr);

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       : HI_MPI_ISP_SensorRegister
 Description     : sensor register callback function to isp firmware
 Input           : pstIspSensorRegister  **
 Output          : None
 Return Value    :
 Process         :
 Note             :

  History
  1.Date         : 2011/1/21
    Author       :
    Modification : Created function

*****************************************************************************/

HI_S32 HI_MPI_ISP_SetSnsSlaveAttr(SLAVE_DEV SlaveDev, const ISP_SLAVE_SNS_SYNC_S *pstSnsSync)
{
    SLAVE_CHECK_DEV(SlaveDev);
    ISP_CHECK_POINTER(pstSnsSync);

    hi_isp_slave_mode_configs_write(SlaveDev, pstSnsSync->unCfg.u32Bytes);
    hi_isp_slave_mode_vstime_low_write(SlaveDev, pstSnsSync->u32VsTime);
    hi_isp_slave_mode_vstime_high_write(SlaveDev, 0);
    hi_isp_slave_mode_hstime_write(SlaveDev, pstSnsSync->u32HsTime);
    hi_isp_slave_mode_vscyc_low_write(SlaveDev, pstSnsSync->u32VsCyc);
    hi_isp_slave_mode_vscyc_high_write(SlaveDev, 0);
    hi_isp_slave_mode_hscyc_write(SlaveDev, pstSnsSync->u32HsCyc);

    return HI_SUCCESS;
}

HI_S32 HI_MPI_ISP_GetSnsSlaveAttr(SLAVE_DEV SlaveDev, ISP_SLAVE_SNS_SYNC_S *pstSnsSync)
{
    SLAVE_CHECK_DEV(SlaveDev);
    ISP_CHECK_POINTER(pstSnsSync);

    pstSnsSync->unCfg.u32Bytes = hi_isp_slave_mode_configs_read(SlaveDev);
    pstSnsSync->u32VsTime = hi_isp_slave_mode_vstime_low_read(SlaveDev);
    pstSnsSync->u32HsTime = hi_isp_slave_mode_hstime_read(SlaveDev);
    pstSnsSync->u32VsCyc = hi_isp_slave_mode_vscyc_low_read(SlaveDev);
    pstSnsSync->u32HsCyc = hi_isp_slave_mode_hscyc_read(SlaveDev);

    return HI_SUCCESS;
}

HI_S32 HI_MPI_ISP_SensorRegCallBack(VI_PIPE ViPipe, ISP_SNS_ATTR_INFO_S *pstSnsAttrInfo, ISP_SENSOR_REGISTER_S *pstRegister)
{
    HI_S32 s32Ret;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_POINTER(pstRegister);

    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);

    if (HI_TRUE == pstIspCtx->bSnsReg)
    {
        ISP_TRACE(HI_DBG_ERR, "Reg ERR! Sensor have registered to ISP[%d]!\n", ViPipe);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    s32Ret = ISP_SensorRegCallBack(ViPipe, pstSnsAttrInfo, pstRegister);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    pstIspCtx->stBindAttr.SensorId = pstSnsAttrInfo->eSensorId;
    pstIspCtx->bSnsReg = HI_TRUE;

    return HI_SUCCESS;
}

HI_S32 HI_MPI_ISP_AELibRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib,
                                   ISP_AE_REGISTER_S *pstRegister)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_LIB_NODE_S *pstAeLibNode = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);

    /* check null point */
    ISP_CHECK_POINTER(pstAeLib);
    ISP_CHECK_POINTER(pstRegister);

    ISP_CHECK_POINTER(pstRegister->stAeExpFunc.pfn_ae_init);
    ISP_CHECK_POINTER(pstRegister->stAeExpFunc.pfn_ae_run);
    ISP_CHECK_POINTER(pstRegister->stAeExpFunc.pfn_ae_ctrl);
    ISP_CHECK_POINTER(pstRegister->stAeExpFunc.pfn_ae_exit);

    /* whether the lib have been registered */
    s32Ret = ISP_FindLib(pstIspCtx->stAeLibInfo.astLibs, pstAeLib);
    if (-1 != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "Reg ERR! aelib have registered to ISP[%d].\n", ViPipe);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    /* whether can we register a new lib  */
    pstAeLibNode = ISP_SearchLib(pstIspCtx->stAeLibInfo.astLibs);
    if (HI_NULL == pstAeLibNode)
    {
        ISP_TRACE(HI_DBG_ERR, "can't register aelib to ISP[%d], there is too many libs.\n", ViPipe);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    /* record register info */
    memcpy(&pstAeLibNode->stAlgLib, pstAeLib, sizeof(ALG_LIB_S));
    memcpy(&pstAeLibNode->stAeRegsiter, pstRegister, sizeof(ISP_AE_REGISTER_S));
    pstAeLibNode->bUsed = HI_TRUE;

    /* set active lib */
    pstIspCtx->stAeLibInfo.u32ActiveLib = ISP_FindLib(pstIspCtx->stAeLibInfo.astLibs, pstAeLib);
    memcpy(&pstIspCtx->stBindAttr.stAeLib, pstAeLib, sizeof(ALG_LIB_S));

    return HI_SUCCESS;
}

HI_S32 HI_MPI_ISP_AWBLibRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib,
                                    ISP_AWB_REGISTER_S *pstRegister)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_LIB_NODE_S *pstAwbLibNode = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);

    /* check null point */
    ISP_CHECK_POINTER(pstAwbLib);
    ISP_CHECK_POINTER(pstRegister);

    ISP_CHECK_POINTER(pstRegister->stAwbExpFunc.pfn_awb_init);
    ISP_CHECK_POINTER(pstRegister->stAwbExpFunc.pfn_awb_run);
    ISP_CHECK_POINTER(pstRegister->stAwbExpFunc.pfn_awb_ctrl);
    ISP_CHECK_POINTER(pstRegister->stAwbExpFunc.pfn_awb_exit);

    /* whether the lib have been registered */
    s32Ret = ISP_FindLib(pstIspCtx->stAwbLibInfo.astLibs, pstAwbLib);
    if (-1 != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "Reg ERR! awblib have registered to ISP[%d].\n", ViPipe);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    /* whether can we register a new lib  */
    pstAwbLibNode = ISP_SearchLib(pstIspCtx->stAwbLibInfo.astLibs);
    if (HI_NULL == pstAwbLibNode)
    {
        ISP_TRACE(HI_DBG_ERR, "can't register awblib to ISP[%d], there is too many libs.\n", ViPipe);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    /* record register info */
    memcpy(&pstAwbLibNode->stAlgLib, pstAwbLib, sizeof(ALG_LIB_S));
    memcpy(&pstAwbLibNode->stAwbRegsiter, pstRegister, sizeof(ISP_AWB_REGISTER_S));
    pstAwbLibNode->bUsed = HI_TRUE;

    /* set active lib */
    pstIspCtx->stAwbLibInfo.u32ActiveLib = ISP_FindLib(pstIspCtx->stAwbLibInfo.astLibs, pstAwbLib);
    memcpy(&pstIspCtx->stBindAttr.stAwbLib, pstAwbLib, sizeof(ALG_LIB_S));

    return HI_SUCCESS;
}

HI_S32 HI_MPI_ISP_SensorUnRegCallBack(VI_PIPE ViPipe, SENSOR_ID SensorId)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);
    ISP_CHECK_SENSOR_REGISTER(ViPipe);

    /* check sensor id */
    if (pstIspCtx->stBindAttr.SensorId != SensorId)
    {
        ISP_TRACE(HI_DBG_ERR, "UnReg ERR! ISP[%d] Registered sensor is %d, present sensor is %d.\n",
                  ViPipe, pstIspCtx->stBindAttr.SensorId, SensorId);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    pstIspCtx->stBindAttr.SensorId = 0;
    pstIspCtx->bSnsReg = HI_FALSE;

    return HI_SUCCESS;
}

HI_S32 HI_MPI_ISP_AELibUnRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib)
{
    HI_S32 s32SearchId;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);

    /* check null point */
    ISP_CHECK_POINTER(pstAeLib);

    s32SearchId = ISP_FindLib(pstIspCtx->stAeLibInfo.astLibs, pstAeLib);
    if (-1 == s32SearchId)
    {
        ISP_TRACE(HI_DBG_ERR, "can't find ae lib in ISP[%d].\n", ViPipe);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    memset(&pstIspCtx->stAeLibInfo.astLibs[s32SearchId], 0, sizeof(ISP_LIB_NODE_S));

    /* set active lib */
    pstIspCtx->stAeLibInfo.u32ActiveLib = 0;

    return HI_SUCCESS;
}

HI_S32 HI_MPI_ISP_AWBLibUnRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib)
{
    HI_S32 s32SearchId;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);

    /* check null point */
    ISP_CHECK_POINTER(pstAwbLib);

    s32SearchId = ISP_FindLib(pstIspCtx->stAwbLibInfo.astLibs, pstAwbLib);
    if (-1 == s32SearchId)
    {
        ISP_TRACE(HI_DBG_ERR, "can't find awb lib in ISP[%d].\n", ViPipe);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    memset(&pstIspCtx->stAwbLibInfo.astLibs[s32SearchId], 0, sizeof(ISP_LIB_NODE_S));

    /* set active lib */
    pstIspCtx->stAwbLibInfo.u32ActiveLib = 0;

    return HI_SUCCESS;
}

HI_S32 HI_MPI_ISP_SetBindAttr(VI_PIPE ViPipe, const ISP_BIND_ATTR_S *pstBindAttr)
{
    SENSOR_ID SensorId;
    HI_S32    s32SearchId;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);

    ISP_CHECK_POINTER(pstBindAttr);

    ISP_SensorGetId(ViPipe, &SensorId);
    /* check sensor id */
    if (pstBindAttr->SensorId != SensorId)
    {
        ISP_TRACE(HI_DBG_ERR, "ISP[%d] Register sensor is %d, present sensor is %d.\n",
                  ViPipe, SensorId, pstBindAttr->SensorId);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    /* check ae lib */
    s32SearchId = ISP_FindLib(pstIspCtx->stAeLibInfo.astLibs, &pstBindAttr->stAeLib);
    if (-1 != s32SearchId)
    {
        pstIspCtx->stAeLibInfo.u32ActiveLib = s32SearchId;
    }
    else
    {
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    /* check awb lib */
    s32SearchId = ISP_FindLib(pstIspCtx->stAwbLibInfo.astLibs, &pstBindAttr->stAwbLib);
    if (-1 != s32SearchId)
    {
        pstIspCtx->stAwbLibInfo.u32ActiveLib = s32SearchId;
    }
    else
    {
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    /* check af lib */
    s32SearchId = ISP_FindLib(pstIspCtx->stAfLibInfo.astLibs, &pstBindAttr->stAfLib);
    if (-1 != s32SearchId)
    {
        pstIspCtx->stAfLibInfo.u32ActiveLib = s32SearchId;
    }
    else
    {
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    /* save global variable */
    memcpy(&pstIspCtx->stBindAttr, pstBindAttr, sizeof(ISP_BIND_ATTR_S));

    return HI_SUCCESS;
}

HI_S32 HI_MPI_ISP_GetBindAttr(VI_PIPE ViPipe, ISP_BIND_ATTR_S *pstBindAttr)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);

    ISP_CHECK_POINTER(pstBindAttr);

    /* get global variable */
    memcpy(pstBindAttr, &pstIspCtx->stBindAttr, sizeof(ISP_BIND_ATTR_S));

    return HI_SUCCESS;
}

HI_S32 HI_MPI_ISP_GetVDTimeOut(VI_PIPE ViPipe, ISP_VD_TYPE_E enIspVDType, HI_U32 u32MilliSec)
{
    ISP_VD_TIMEOUT_S stIspVdTimeOut;
    HI_S32 s32Ret;

    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_OPEN(ViPipe);

    stIspVdTimeOut.u32MilliSec = u32MilliSec;
    stIspVdTimeOut.s32IntStatus = 0x0;

    switch (enIspVDType)
    {
        case ISP_VD_FE_START:
            s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_GET_VD_TIMEOUT, &stIspVdTimeOut);
            if (HI_SUCCESS != s32Ret)
            {
                return s32Ret;
            }
            break;
        case ISP_VD_FE_END:
            s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_GET_VD_END_TIMEOUT, &stIspVdTimeOut);
            if (HI_SUCCESS != s32Ret)
            {
                return s32Ret;
            }
            break;
        case ISP_VD_BE_END:
            s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_GET_VD_BEEND_TIMEOUT, &stIspVdTimeOut);
            if (HI_SUCCESS != s32Ret)
            {
                return s32Ret;
            }
            break;
        default:
            ISP_TRACE(HI_DBG_ERR, "ISP[%d] Get VD type %d not support!\n", ViPipe, enIspVDType);
            break;
    }

    return HI_SUCCESS;
}

#ifdef ENABLE_JPEGEDCF
/*****************************************************************************
 Prototype       : HI_MPI_ISP_SetDCFInfo
 Description     : set dcf info to isp firmware
 Input           : ISP_DCF_S *pstIspDCF
 Output          : None
 Return Value    :
 Process         :
 Note            :

  History
  1.Date         : 2014/6/13
    Author       :
    Modification : Created function

*****************************************************************************/
HI_S32 HI_MPI_ISP_SetDCFInfo(VI_PIPE ViPipe, const ISP_DCF_INFO_S *pstIspDCF)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    HI_BOOL bTempMap = HI_FALSE;
    HI_U64 u64PhyAddrHigh;
    HI_U64 u64PhyAddrTemp;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);
    ISP_CHECK_POINTER(pstIspDCF);
    ISP_CHECK_OPEN(ViPipe);

    u64PhyAddrHigh = (HI_U64)hi_ext_system_update_info_high_phyaddr_read(ViPipe);
    u64PhyAddrTemp = (HI_U64)hi_ext_system_update_info_low_phyaddr_read(ViPipe);
    u64PhyAddrTemp |= (u64PhyAddrHigh << 32);

    if (HI_NULL == pstIspCtx->stUpdateInfoCtrl.u64UpdateInfoPhyaddr)
    {
        pstIspCtx->stUpdateInfoCtrl.u64UpdateInfoPhyaddr = u64PhyAddrTemp;

        pstIspCtx->stUpdateInfoCtrl.pstIspUpdateInfo = HI_MPI_SYS_Mmap(pstIspCtx->stUpdateInfoCtrl.u64UpdateInfoPhyaddr,
                (sizeof(ISP_DCF_UPDATE_INFO_S) * ISP_MAX_UPDATEINFO_BUF_NUM + sizeof(ISP_DCF_CONST_INFO_S)));
        if (HI_NULL == pstIspCtx->stUpdateInfoCtrl.pstIspUpdateInfo)
        {
            printf("set dcf info mmap failed!\n");
            return HI_ERR_ISP_NOMEM;
        }

        pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo = (ISP_DCF_CONST_INFO_S *)(pstIspCtx->stUpdateInfoCtrl.pstIspUpdateInfo + ISP_MAX_UPDATEINFO_BUF_NUM);

        bTempMap = HI_TRUE;
    }

    memcpy(pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo,
           pstIspDCF, sizeof(HI_U8)*DCF_DRSCRIPTION_LENGTH * 4);

    pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8LightSource           = pstIspDCF->stIspDCFConstInfo.u8LightSource;
    pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u32FocalLength          = pstIspDCF->stIspDCFConstInfo.u32FocalLength;
    pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8SceneType             = pstIspDCF->stIspDCFConstInfo.u8SceneType;
    pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8CustomRendered        = pstIspDCF->stIspDCFConstInfo.u8CustomRendered;
    pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8FocalLengthIn35mmFilm = pstIspDCF->stIspDCFConstInfo.u8FocalLengthIn35mmFilm;
    pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8SceneCaptureType      = pstIspDCF->stIspDCFConstInfo.u8SceneCaptureType;
    pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8GainControl           = pstIspDCF->stIspDCFConstInfo.u8GainControl;
    pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8Contrast              = pstIspDCF->stIspDCFConstInfo.u8Contrast;
    pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8Saturation            = pstIspDCF->stIspDCFConstInfo.u8Saturation;
    pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8Sharpness             = pstIspDCF->stIspDCFConstInfo.u8Sharpness;

    if (bTempMap)
    {
        HI_MPI_SYS_Munmap(pstIspCtx->stUpdateInfoCtrl.pstIspUpdateInfo,
                          (sizeof(ISP_DCF_UPDATE_INFO_S) * ISP_MAX_UPDATEINFO_BUF_NUM + sizeof (ISP_DCF_CONST_INFO_S)));

        pstIspCtx->stUpdateInfoCtrl.u64UpdateInfoPhyaddr = HI_NULL;
    }

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       : HI_MPI_ISP_GetDCFInfo
 Description     : get dcf info from isp firmware
 Input           : ISP_DCF_S *pstIspDCF
 Output          : None
 Return Value    :
 Process         :
 Note            :

  History
  1.Date         : 2014/6/16
    Author       :
    Modification : Created function

*****************************************************************************/
HI_S32 HI_MPI_ISP_GetDCFInfo(VI_PIPE ViPipe, ISP_DCF_INFO_S *pstIspDCF)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    HI_BOOL bTempMap = HI_FALSE;
    HI_U64 u64PhyAddrHigh;
    HI_U64 u64PhyAddrTemp;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);
    ISP_CHECK_POINTER(pstIspDCF);
    ISP_CHECK_OPEN(ViPipe);

    u64PhyAddrHigh = (HI_U64)hi_ext_system_update_info_high_phyaddr_read(ViPipe);
    u64PhyAddrTemp = (HI_U64)hi_ext_system_update_info_low_phyaddr_read(ViPipe);
    u64PhyAddrTemp |= (u64PhyAddrHigh << 32);

    if (HI_NULL == pstIspCtx->stUpdateInfoCtrl.u64UpdateInfoPhyaddr)
    {
        pstIspCtx->stUpdateInfoCtrl.u64UpdateInfoPhyaddr = u64PhyAddrTemp;

        pstIspCtx->stUpdateInfoCtrl.pstIspUpdateInfo = HI_MPI_SYS_Mmap(pstIspCtx->stUpdateInfoCtrl.u64UpdateInfoPhyaddr,
                (sizeof(ISP_DCF_UPDATE_INFO_S) * ISP_MAX_UPDATEINFO_BUF_NUM + sizeof(ISP_DCF_CONST_INFO_S)));
        if (HI_NULL == pstIspCtx->stUpdateInfoCtrl.pstIspUpdateInfo)
        {
            printf("get dcf info mmap failed!\n");
            return HI_ERR_ISP_NOMEM;
        }
        pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo = (ISP_DCF_CONST_INFO_S *)(pstIspCtx->stUpdateInfoCtrl.pstIspUpdateInfo + ISP_MAX_UPDATEINFO_BUF_NUM);

        bTempMap = HI_TRUE;
    }

    memcpy(pstIspDCF, pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo,
           sizeof(HI_U8)*DCF_DRSCRIPTION_LENGTH * 4);

    pstIspDCF->stIspDCFConstInfo.u8LightSource            = pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8LightSource;
    pstIspDCF->stIspDCFConstInfo.u32FocalLength           = pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u32FocalLength;
    pstIspDCF->stIspDCFConstInfo.u8SceneType              = pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8SceneType;
    pstIspDCF->stIspDCFConstInfo.u8CustomRendered         = pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8CustomRendered;
    pstIspDCF->stIspDCFConstInfo.u8FocalLengthIn35mmFilm  = pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8FocalLengthIn35mmFilm;
    pstIspDCF->stIspDCFConstInfo.u8SceneCaptureType       = pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8SceneCaptureType;
    pstIspDCF->stIspDCFConstInfo.u8GainControl            = pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8GainControl;
    pstIspDCF->stIspDCFConstInfo.u8Contrast               = pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8Contrast;
    pstIspDCF->stIspDCFConstInfo.u8Saturation             = pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8Saturation;
    pstIspDCF->stIspDCFConstInfo.u8Sharpness              = pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8Sharpness;
    pstIspDCF->stIspDCFConstInfo.u8MeteringMode           = pstIspCtx->stUpdateInfoCtrl.pstIspDCFConstInfo->u8MeteringMode;

    pstIspDCF->stIspDCFUpdateInfo.u32ISOSpeedRatings       = pstIspCtx->stUpdateInfoCtrl.pstIspUpdateInfo->u32ISOSpeedRatings;
    pstIspDCF->stIspDCFUpdateInfo.u32ExposureTime          = pstIspCtx->stUpdateInfoCtrl.pstIspUpdateInfo->u32ExposureTime;
    pstIspDCF->stIspDCFUpdateInfo.u32ExposureBiasValue     = pstIspCtx->stUpdateInfoCtrl.pstIspUpdateInfo->u32ExposureBiasValue;
    pstIspDCF->stIspDCFUpdateInfo.u8ExposureProgram        = pstIspCtx->stUpdateInfoCtrl.pstIspUpdateInfo->u8ExposureProgram;
    pstIspDCF->stIspDCFUpdateInfo.u8ExposureMode           = pstIspCtx->stUpdateInfoCtrl.pstIspUpdateInfo->u8ExposureMode;
    pstIspDCF->stIspDCFUpdateInfo.u32FNumber               = pstIspCtx->stUpdateInfoCtrl.pstIspUpdateInfo->u32FNumber;
    pstIspDCF->stIspDCFUpdateInfo.u32MaxApertureValue      = pstIspCtx->stUpdateInfoCtrl.pstIspUpdateInfo->u32MaxApertureValue;
    pstIspDCF->stIspDCFUpdateInfo.u8WhiteBalance           = pstIspCtx->stUpdateInfoCtrl.pstIspUpdateInfo->u8WhiteBalance;

    if (bTempMap)
    {
        HI_MPI_SYS_Munmap(pstIspCtx->stUpdateInfoCtrl.pstIspUpdateInfo,
                          (sizeof(ISP_DCF_UPDATE_INFO_S) * ISP_MAX_UPDATEINFO_BUF_NUM + sizeof (ISP_DCF_CONST_INFO_S)));

        pstIspCtx->stUpdateInfoCtrl.u64UpdateInfoPhyaddr = HI_NULL;
    }

    return HI_SUCCESS;
}


HI_S32 HI_MPI_ISP_SetFrameInfo(const ISP_FRAME_INFO_S *pstIspFrame)
{
    VI_PIPE ViPipe = 0;
    HI_S32 s32Ret;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx->stFrameInfoCtrl.pstIspFrame);
    ISP_CHECK_POINTER(pstIspFrame);
    ISP_CHECK_OPEN(ViPipe);

    s32Ret = ioctl(g_as32IspFd[ViPipe], ISP_FRAME_INFO_SET, pstIspFrame);
    if (s32Ret)
    {
        return s32Ret;
    }

    memcpy(pstIspCtx->stFrameInfoCtrl.pstIspFrame, pstIspFrame, sizeof(ISP_FRAME_INFO_S));
    return HI_SUCCESS;
}

HI_S32 HI_MPI_ISP_GetFrameInfo(ISP_FRAME_INFO_S *pstIspFrame)
{
    VI_PIPE ViPipe = 0;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_CHECK_POINTER(pstIspCtx);
    ISP_CHECK_POINTER(pstIspFrame);
    ISP_CHECK_OPEN(ViPipe);

    if (ioctl(g_as32IspFd[ViPipe], ISP_FRAME_INFO_GET, pstIspFrame))
    {
        return HI_ERR_ISP_NOMEM;
    }

    return HI_SUCCESS;
}


#endif
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

