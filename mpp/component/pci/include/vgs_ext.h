/******************************************************************************

  Copyright (C), 2013-2018, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : vgs_ext.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/04/23
  Description   :
  History       :
  1.Date        : 2013/04/23
    Author      : z00183560
    Modification: Created file
******************************************************************************/

#ifndef __VGS_EXT_H__
#define __VGS_EXT_H__

#include "hi_comm_video.h"
#include "hi_comm_vgs.h"
#include "hi_errno.h"
#include "vb_ext.h"

#define HI_TRACE_VGS(level, fmt, ...)\
do{ \
    HI_TRACE(level, HI_ID_VGS,"[Func]:%s [Line]:%d [Info]:"fmt,__FUNCTION__, __LINE__,##__VA_ARGS__);\
}while(0)


#define VGS_INVALD_HANDLE          (-1UL) /*invalid job handle*/

#define VGS_MAX_JOB_NUM             400  /*max job number*/
#define VGS_MIN_JOB_NUM             20   /*min job number*/

#define VGS_MAX_TASK_NUM            800  /*max task number*/
#define VGS_MIN_TASK_NUM            20   /*min task number*/

#define VGS_MAX_NODE_NUM            800  /*max node number*/
#define VGS_MIN_NODE_NUM            20   /*min node number*/

#define VGS_MAX_WEIGHT_THRESHOLD    100  /*max weight threshold*/
#define VGS_MIN_WEIGHT_THRESHOLD    1    /*min weight threshold*/

#define MAX_VGS_COVER               1
#define MAX_VGS_OSD                 1

#define FD_GRID_SZ 4 //grid size used in feature detection, in this version, the image is processed with 4x4 grid
#define FD_CELL_NUM FD_GRID_SZ*FD_GRID_SZ //blk number in total

#define VGS_MAX_STITCH_NUM      4

/*The type of job*/
typedef enum hiVGS_JOB_TYPE_E
{
    VGS_JOB_TYPE_BYPASS = 0,         /*BYPASS job,can only contain bypass task*/
    VGS_JOB_TYPE_NORMAL = 1,         /*normal job,can contain any task except bypass task and lumastat task*/
    VGS_JOB_TYPE_BUTT
} VGS_JOB_TYPE_E;


/*The completion status of task*/
typedef enum hiVGS_TASK_FNSH_STAT_E
{
    VGS_TASK_FNSH_STAT_OK = 1,       /*task has been completed correctly */
    VGS_TASK_FNSH_STAT_FAIL = 2,     /*task failed because of exception or not completed */
    VGS_TASK_FNSH_STAT_CANCELED = 3, /*task has been cancelled */
    VGS_TASK_FNSH_STAT_BUTT
} VGS_TASK_FNSH_STAT_E;

/* the priority of VGS task */
typedef enum hiVGS_JOB_PRI_E
{
    VGS_JOB_PRI_HIGH = 0,      /*high priority*/
    VGS_JOB_PRI_NORMAL = 1,    /*normal priority*/
    VGS_JOB_PRI_LOW = 2,       /*low priority*/
    VGS_JOB_PRI_BUTT
} VGS_JOB_PRI_E;

/* The status after VGS cancle job */
typedef struct hiVGS_CANCEL_STAT_S
{
    HI_U32  u32JobsCanceled;    /*The count of cancelled job*/
    HI_U32  u32JobsLeft;        /*The count of the rest job*/
} VGS_CANCEL_STAT_S;

/*The completion status of job*/
typedef enum hiVGS_JOB_FNSH_STAT_E
{
    VGS_JOB_FNSH_STAT_OK = 1,       /*job has been completed correctly*/
    VGS_JOB_FNSH_STAT_FAIL = 2,     /*job failed because of exception or not completed*/
    VGS_JOB_FNSH_STAT_CANCELED = 3, /*job has been cancelled*/
    VGS_JOB_FNSH_STAT_BUTT
} VGS_JOB_FNSH_STAT_E;

/* The struct of vgs job.
After complete the job,VGS call the callback function to notify the caller with this struct as an parameter.
*/
typedef struct hiVGS_JOB_DATA_S
{
    HI_U64              au64PrivateData[VGS_PRIVATE_DATA_LEN];  /* private data of job */
    VGS_JOB_FNSH_STAT_E enJobFinishStat;                        /* output param:job finish status(ok or nok)*/
    VGS_JOB_TYPE_E      enJobType;
    void                (*pJobCallBack)(MOD_ID_E enCallModId, HI_U32 u32CallDevId, HI_U32 u32CallChnId, struct hiVGS_JOB_DATA_S* pJobData); /* callback */
} VGS_JOB_DATA_S;

/* The struct of vgs task ,include input and output frame buffer,caller,and callback function .
After complete the task,VGS call the callback function to notify the caller with this struct as an parameter.
*/
typedef struct hiVGS_TASK_DATA_S
{
    VIDEO_FRAME_INFO_S      stImgIn;        /* input picture */
    VIDEO_FRAME_INFO_S      stImgOut;       /* output picture */
    HI_U64                  au64PrivateData[VGS_PRIVATE_DATA_LEN];    /* task's private data */
    void                    (*pCallBack)(MOD_ID_E enCallModId, HI_U32 u32CallDevId, HI_U32 u32CallChnId, const struct hiVGS_TASK_DATA_S* pTask); /* callback */
    MOD_ID_E                enCallModId;    /* module ID */
    HI_U32                  u32CallDevId;   /* device ID */
    HI_U32                  u32CallChnId;   /* chnnel ID */
    VGS_TASK_FNSH_STAT_E    enFinishStat;   /* output param:task finish status(ok or nok)*/
    HI_U32 reserved;
} VGS_TASK_DATA_S;

typedef struct hiVGS_TASK_DATA_STITCH_S
{
    VIDEO_FRAME_INFO_S      stImgIn[VGS_MAX_STITCH_NUM];        /* input picture */
    VIDEO_FRAME_INFO_S      stImgOut;       /* output picture */
    HI_U64                  au64PrivateData[VGS_PRIVATE_DATA_LEN];    /* task's private data */
    void                    (*pCallBack)(MOD_ID_E enCallModId, HI_U32 u32CallDevId, HI_U32 u32CallChnId, const struct hiVGS_TASK_DATA_STITCH_S* pTask); /* callback */
    MOD_ID_E                enCallModId;    /* module ID */
    HI_U32                  u32CallDevId;   /* device ID */
    HI_U32                  u32CallChnId;   /* chnnel ID */
    VGS_TASK_FNSH_STAT_E    enFinishStat;   /* output param:task finish status(ok or nok)*/
    HI_U32 reserved;
} VGS_TASK_DATA_STITCH_S;

typedef enum
{
    GHDR_SDR_IN_HDR10_OUT = 0, //hifonev500 test ok!
    GHDR_SDR_IN_HLG_OUT,       //hifonev500 test ok!
    GHDR_SDR_PREMULT,          //hifonev500 test ok!
    GHDR_SDR709_IN_2020_OUT,   //reserved!
    GHDR_BUTT
} VGS_GHDR_SCENE_MODE_E;

typedef enum
{
    VGS_HIHDR_G_TYP = 0,
    VGS_HIHDR_G_TYP1,
    VGS_HIHDR_G_RAND,
    VGS_HIHDR_G_MAX,
    VGS_HIHDR_G_MIN,
    VGS_HIHDR_G_ZERO,
    VGS_HIHDR_G_BUTT
} VGS_GHDR_G_MODE_E;

/* The information of OSD */
typedef struct hiVGS_OSD_S
{
    /* first address of bitmap */
    HI_U64 u64PhyAddr;

    PIXEL_FORMAT_E enPixelFormat;

    HI_U32 u32Stride;

    /* Alpha bit should be extended by setting Alpha0 and Alpha1, when enPixelFormat is PIXEL_FORMAT_RGB_1555 */
    HI_BOOL bAlphaExt1555; /* whether Alpha bit should be extended */
    HI_U8 u8Alpha0;        /* u8Alpha0 will be valid where alpha bit is 0 */
    HI_U8 u8Alpha1;        /* u8Alpha1 will be valid where alpha bit is 1 */
} VGS_OSD_S;

typedef struct hiVGS_HAL_OSDIMG_INFO_S
{
    HI_U64 u64PhyAddr;
    HI_U32 u32Stride;
    RECT_S stRect;
} VGS_HAL_OSDIMG_INFO_S;

/*The struct of OSD operation*/
typedef struct hiVGS_OSD_OPT_S
{
    HI_BOOL             bOsdEn;

    HI_U8               u32GlobalAlpha;
    VGS_OSD_S           stVgsOsd;
    RECT_S              stOsdRect;

    HI_BOOL             bOsdRevert;
    VGS_OSD_REVERT_S    stVgsOsdRevert;

    HI_BOOL             bGhdrEn;
} VGS_OSD_OPT_S;

typedef struct hiVGS_LUMAINFO_OPT_S
{
    RECT_S      stRect;             /*the regions to get lumainfo*/
    HI_U64*     pu64VirtAddrForResult;
    HI_U64      u64PhysAddrForResult;
} VGS_LUMASTAT_OPT_S;

typedef struct hiVGS_COVER_OPT_S
{
    HI_BOOL                     bCoverEn;
    VGS_COVER_TYPE_E            enCoverType;
    union
    {
        RECT_S                  stDstRect;      /* rectangle */
        VGS_QUADRANGLE_COVER_S  stQuadRangle;   /* arbitrary quadrilateral */
    };
    HI_U32                      u32CoverColor;
} VGS_COVER_OPT_S;

typedef struct hiVGS_CROP_OPT_S
{
    RECT_S stDestRect;
} VGS_CROP_OPT_S;

typedef enum hiVGS_SCALE_COEF_MODE_E
{
    VGS_SCALE_COEF_NORMAL   = 0,    /* normal scale coefficient*/
    VGS_SCALE_COEF_TAP4     = 1,    /* scale coefficient of 4 tap for IVE */
    VGS_SCALE_COEF_TAP6     = 2,    /* scale coefficient of 6 tap for IVE */
    VGS_SCALE_COEF_TAP8     = 3,    /* scale coefficient of 8 tap for IVE */
    VGS_SCALE_COEF_PYRAMID  = 4,    /* scale coefficient for DIS pyramid */
    VGS_SCALE_COEF_BUTT
} VGS_SCALE_COEF_MODE_E;

typedef struct hiVGS_FPD_HW_CFG_S
{
    HI_U32 u32ProcBitDepth; // 4bits; u; 4.0
    HI_BOOL bDoDetection;   // 1bit; bool

    HI_U16 u16GridRows; // 3bits; u; 3.0
    HI_U16 u16GridCols; // 3bits; u; 3.0
    HI_U16 u16GridNums; // 5bits; u,; 5.0

    HI_U8 u8QualityLevel;  // 3bits; u; 3.0
    HI_U16 u16MinEigenVal; // 16bits; u; 16.0
    HI_U8 u8HarrisK;       // 4btis; u; 4.0
    HI_U16 u16MinDistance; // 5bits; u; 5.0

    HI_U16 pu16CellRow[FD_GRID_SZ + 1]; // 9bits; u; 9.0 corner postion of the cells
    HI_U16 pu16CellCol[FD_GRID_SZ + 1]; // 9bits; u; 9.0

    HI_U16 pu16PtsNumPerCell[FD_CELL_NUM]; // 6bits; u; 6.0 maximum number of keypoints allowed in each block
    HI_U16 u16MaxPtsNumInUse;              // 9bits; u; 9.0
    HI_BOOL bSetPtsNumPerCell;             // 1bit; bool
} VGS_FPD_HW_CFG_S;

typedef struct hiVGS_ASPECTRATIO_OPT_S
{
    RECT_S stVideoRect;
    HI_U32 u32BgColor;
} VGS_ASPECTRATIO_OPT_S;

typedef struct hiVGS_ONLINE_OPT_S
{
    HI_BOOL                     bCrop;              /*if enable crop*/
    VGS_CROP_OPT_S              stCropOpt;
    HI_BOOL                     bCover;             /*if enable cover*/
    VGS_COVER_OPT_S             stCoverOpt[MAX_VGS_COVER];
    HI_BOOL                     bOsd;               /*if enable osd*/
    VGS_OSD_OPT_S               stOsdOpt[MAX_VGS_OSD];

    HI_BOOL                     bMirror;            /*if enable mirror*/
    HI_BOOL                     bFlip;              /*if enable flip*/
    HI_BOOL                     bForceHFilt;        /*Whether to force the horizontal direction filtering,it can be
                                                set while input and output pic are same size at horizontal direction*/
    HI_BOOL                     bForceVFilt;        /*Whether to force the vertical direction filtering,it can be
                                                set while input and output pic are same size at vertical direction*/
    HI_BOOL                     bDeflicker;         /*Whether decrease flicker*/
    VGS_SCALE_COEF_MODE_E       enVGSSclCoefMode;

    HI_BOOL                     bGdc;               /*The operation is belong to fisheye*/
    RECT_S                      stGdcRect;

    HI_BOOL                     bFpd;               /*if enable fpd*/
    HI_U64                      u64FpdPhyAddr;      /* physical address of fpd */
    VGS_FPD_HW_CFG_S            stFpdOpt;

    HI_BOOL                     bAspectRatio;       /*if enable LBA*/
    VGS_ASPECTRATIO_OPT_S       stAspectRatioOpt;
} VGS_ONLINE_OPT_S;

typedef struct hiVGS_STITCH_OPT_S
{
    //HI_BOOL                     bStitchEn;
    HI_U32                      u32StitchNum;
} VGS_STITCH_OPT_S;

/* vertical scanning direction */
typedef enum hiVGS_DRV_VSCAN_E
{
    VGS_SCAN_UP_DOWN = 0,    /* form up to down */
    VGS_SCAN_DOWN_UP = 1     /* form down to up */
} VGS_DRV_VSCAN_E;

/* horizontal scanning direction */
typedef enum hiVGS_DRV_HSCAN_E
{
    VGS_SCAN_LEFT_RIGHT = 0,    /* form left to right */
    VGS_SCAN_RIGHT_LEFT = 1     /* form right to left */
} VGS_DRV_HSCAN_E;

/* Definition on scanning direction */
typedef struct hiVGS_SCANDIRECTION_S
{
    /* vertical scanning direction */
    VGS_DRV_VSCAN_E enVScan;

    /* horizontal scanning direction */
    VGS_DRV_HSCAN_E enHScan;
} VGS_SCANDIRECTION_S;

typedef struct hiVGS_OSD_QUICKCOPY_OPT_S
{
    RECT_S stSrcRect;
    RECT_S stDestRect;
} VGS_OSD_QUICKCOPY_OPT_S;

typedef enum
{
    VGS_CSC_V0_TYP = 0,
    VGS_CSC_V0_TYP1,
    VGS_CSC_V0_RAND,
    VGS_CSC_V0_MAX,
    VGS_CSC_V0_MIN,
    VGS_CSC_V0_ZERO,
    VGS_CSC_V0_BUTT
} VGS_CSC_V0_MODE_E;

typedef enum
{
    VGS_VHDR_V_TYP = 0,
    VGS_VHDR_V_TYP1,
    VGS_VHDR_V_RAND,
    VGS_VHDR_V_MAX,
    VGS_VHDR_V_MIN,
    VGS_VHDR_V_ZERO,
    VGS_VHDR_V_BUTT
} VGS_VHDR_V_MODE_E;

typedef enum tagVGS_RM_COEF_MODE_E
{
    VGS_RM_COEF_MODE_TYP  = 0x0,
    VGS_RM_COEF_MODE_RAN  = 0x1,
    VGS_RM_COEF_MODE_MIN  = 0x2,
    VGS_RM_COEF_MODE_MAX  = 0x3,
    VGS_RM_COEF_MODE_ZRO  = 0x4,
    VGS_RM_COEF_MODE_CUS  = 0x5,
    VGS_RM_COEF_MODE_UP   = 0x6,
    /*VGS_RM_COEF_MODE_DOWN = 0x7,*/
    VGS_RM_COEF_MODE_BUTT
} VGS_RM_COEF_MODE_E;

typedef struct hiVGS_HDR_CDS_CFG_S
{
    HI_U32 vgs_isp_cds_cdsh_en;
    HI_U32 vgs_isp_cds_cdsv_en;
    HI_U32 vgs_isp_cds_uv2c_mode;
    HI_S32 vgs_isp_cds_coefh[8];
    HI_U32 vgs_isp_cds_coefv0;
    HI_U32 vgs_isp_cds_coefv1;
} VGS_HDR_CDS_CFG_S;

typedef enum hiVGS_VHDR_SCENE_MODE_E
{
    VGS_VHDR_HDR10_IN_SDR_OUT = 0,   //hifonev500 test ok!
    VGS_VHDR_HDR10_IN_HLG_OUT,       //hifonev500 test ok!
    VGS_VHDR_HLG_IN_SDR_OUT,         //hifonev500 test ok!
    VGS_VHDR_HLG_IN_HDR10_OUT,       //hifonev500 test ok!

    VGS_VHDR_SLF_IN_HDR10_OUT,       //need to be add !!!!!!!!!!!!!
    VGS_VHDR_HDR10_IN_HDR10_OUT,     //need to be add !!!!!!!!!!!!!
    VGS_HDR_HDR10_IN_SDR2020_OUT,    //need to be add !!!!!!!!!!!!!
    VGS_VHDR_SDR2020_IN_SDR2020_OUT, //need to be add !!!!!!!!!!!!!
    VGS_VHDR_HLG_IN_SDR2020_OUT,     //need to be add !!!!!!!!!!!!!
    VGS_VHDR_SLF_IN_SDR_OUT,         //need to be add !!!!!!!!!!!!!
    VGS_VHDR_HLG_IN_SDR10_OUT,       //need to be add !!!!!!!!!!!!!
    VGS_VHDR_SLF_IN_SDR10_OUT,       //need to be add !!!!!!!!!!!!!
    VGS_VHDR_SDR2020_IN_SDR10_OUT,   //need to be add !!!!!!!!!!!!!
    VGS_VHDR_SDR10_IN_SDR10_OUT,     //need to be add !!!!!!!!!!!!!

    VGS_VHDR_SDR2020_IN_709_OUT,     //hifonev500 test ok!
    VGS_VHDR_XVYCC,                  //new scene //hifonev500 test ok!
    VGS_VHDR_SDR2020CL_IN_709_OUT,   //new scene //hifonev500 test ok!

    VGS_VHDR_BUTT
} VGS_VHDR_SCENE_MODE_E;


typedef HI_S32      FN_VGS_Init(HI_VOID* pVrgs);

typedef HI_VOID     FN_VGS_Exit(HI_VOID);

typedef HI_S32      FN_VGS_BeginJob(VGS_HANDLE* pVgsHanle, VGS_JOB_PRI_E enPriLevel,
                                    MOD_ID_E enCallModId, HI_U32 u32CallDevId, HI_U32 u32CallChnId, VGS_JOB_DATA_S* pJobData);

typedef HI_S32      FN_VGS_EndJob(VGS_HANDLE VgsHanle, HI_BOOL bSort, VGS_JOB_DATA_S* pJobData);

typedef HI_S32      FN_VGS_EndJobBlock(VGS_HANDLE VgsHanle);

typedef HI_S32      FN_VGS_CancelJob(VGS_HANDLE hHandle);

typedef HI_S32      FN_VGS_CancelJobByModDev(MOD_ID_E enCallModId, HI_U32 u32CallDevId, HI_U32 u32CallChnId, VGS_CANCEL_STAT_S* pstVgsCancelStat);

typedef HI_S32      FN_VGS_AddCoverTask(VGS_HANDLE VgsHanle, VGS_TASK_DATA_S* pTask, VGS_COVER_OPT_S* pstCoverOpt);

typedef HI_S32      FN_VGS_AddOSDTask(VGS_HANDLE VgsHanle, VGS_TASK_DATA_S* pTask, VGS_OSD_OPT_S* pstOsdOpt);

typedef HI_S32      FN_VGS_AddOnlineTask(VGS_HANDLE VgsHanle, VGS_TASK_DATA_S* pTask, VGS_ONLINE_OPT_S* pstOnlineOpt);

typedef HI_S32      FN_VGS_Add2ScaleTask(VGS_HANDLE VgsHandle, VGS_TASK_DATA_S* pTask);

typedef HI_S32      FN_VGS_AddGetLumaStatTask(VGS_HANDLE VgsHanle, VGS_TASK_DATA_S* pTask, VGS_LUMASTAT_OPT_S* pstLumaInfoOpt);

typedef HI_S32      FN_VGS_AddQuickCopyTask(VGS_HANDLE VgsHandle, VGS_TASK_DATA_S* pTask);

typedef HI_S32      FN_VGS_AddRotationTask(VGS_HANDLE VgsHanle, VGS_TASK_DATA_S* pTask, ROTATION_E enRotationAngle);

typedef HI_S32      FN_VGS_AddStitchTask(VGS_HANDLE VgsHanle, VGS_TASK_DATA_STITCH_S* pTask, VGS_STITCH_OPT_S* pstStitchOpt);

typedef HI_S32      FN_VGS_AddBypassTask(VGS_HANDLE VgsHanle, VGS_TASK_DATA_S* pTask);

/*only for test*/
typedef HI_VOID     FN_VGS_GetMaxJobNum(HI_S32* s32MaxJobNum);
typedef HI_VOID     FN_VGS_GetMaxTaskNum(HI_S32* s32MaxTaskNum);


typedef struct hiVGS_EXPORT_FUNC_S
{
    FN_VGS_BeginJob*            pfnVgsBeginJob;
    FN_VGS_CancelJob*           pfnVgsCancelJob;
    FN_VGS_CancelJobByModDev*   pfnVgsCancelJobByModDev;
    FN_VGS_EndJob*              pfnVgsEndJob;
    FN_VGS_AddCoverTask*        pfnVgsAddCoverTask;
    FN_VGS_AddOSDTask*          pfnVgsAddOSDTask;
    FN_VGS_AddBypassTask*       pfnVgsAddBypassTask;
    FN_VGS_AddGetLumaStatTask*  pfnVgsGetLumaStatTask;
    FN_VGS_AddOnlineTask*       pfnVgsAddOnlineTask;
    /* for jpeg */
    FN_VGS_Add2ScaleTask*       pfnVgsAdd2ScaleTask;
    /* for region */
    FN_VGS_AddQuickCopyTask*    pfnVgsAddQuickCopyTask;

    FN_VGS_AddRotationTask*     pfnVgsAddRotationTask;

    FN_VGS_AddStitchTask*       pfnVgsAddStitchTask;
    /* for ive */
    FN_VGS_EndJobBlock*         pfnVgsEndJobBlock;

    /*only for test*/
    FN_VGS_GetMaxJobNum*        pfnVgsGetMaxJobNum;
    FN_VGS_GetMaxTaskNum*       pfnVgsGetMaxTaskNum;
} VGS_EXPORT_FUNC_S;


#endif /* __VGS_EXT_H__ */

