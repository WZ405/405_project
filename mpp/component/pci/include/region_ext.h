/******************************************************************************

    Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

    ******************************************************************************
    File Name     : region_ext.h
    Version        : Initial Draft
    Author        : Hisilicon multimedia software group
    Created        : 2010/12/18
    Description    :
    History        :
    1.Date        : 2010/12/18
        Author        : Z44949
        Modification: Created file

******************************************************************************/

#ifndef __REGION_EXT_H__
#define __REGION_EXT_H__

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_region.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

typedef HI_VOID (*RGN_DETCHCHN)(HI_S32 stDevId,HI_S32 stChnId,HI_BOOL bDetchChn);

/*the max pixel format of region support*/
#define PIXEL_FORMAT_NUM_MAX 3
#define RGN_HANDLE_IN_BATCH_MAX 128

typedef struct hiRGN_COMM_S
{
    HI_BOOL                 bShow;
    RGN_AREA_TYPE_E            enCoverType;     /* COVER type*/
    POINT_S                 stPoint;         /*start position*/
    SIZE_S                  stSize;          /*region size(width and height)*/
    RGN_QUADRANGLE_S          stQuadRangle;    /*config of  quadrangle */
    HI_U32                  u32Layer;        /*layer of region*/
    HI_U32                  u32BgColor;      /*background color of region */
    HI_U32                  u32GlobalAlpha;  /*overall perspective ALPHA*/
    HI_U32                  u32FgAlpha;      /* foreground ALPHA*/
    HI_U32                  u32BgAlpha;      /*background ALPHA*/
    HI_U64                  u64PhyAddr;      /*physical Address of region memory*/
    HI_U64                     u64VirtAddr;     /*virtual Address of region memory*/
    HI_U32                  u32Stride;       /*stride Address of region memory data*/
    HI_U32                  u32BufNum;       /* buffer number of region used*/
    PIXEL_FORMAT_E          enPixelFmt;      /*pixel formatof region */
    VIDEO_FIELD_E           enAttachField;   /*field of region attach*/

    OVERLAY_QP_INFO_S       stQpInfo;        /*QP information*/

    OVERLAY_INVERT_COLOR_S     stInvColInfo;    /*inverse color information*/

    ATTACH_DEST_E           enAttachDest;    /*OSD attach JPEG information*/

    MOSAIC_BLK_SIZE_E       enMosaicBlkSize;    /* MOSAIC block size */

    RGN_COORDINATE_E         enCoordinate;    /*COVER coordinate*/
}RGN_COMM_S;

typedef struct hiRGN_BUF_INFO_S
{
    HI_U32                    u32BuffIndex;     /*index of buffer*/
    HI_U32                    hHandle;          /*handle of buffer*/
}RGN_BUF_INFO_S;

typedef struct hiRGN_GET_INFO_S
{
    RGN_BUF_INFO_S stBufInfo;
    RGN_COMM_S *pstRgnComm;                  /*address of common information of  point array*/
}RGN_GET_INFO_S;

typedef struct hiRGN_INFO_S
{
    HI_U32 u32Num;           /* number of region*/
    HI_BOOL bModify;         /* modify of not*/
    RGN_GET_INFO_S *pstRgnCommInfo; /*address of common information of  point array*/
}RGN_INFO_S;

typedef struct hiRGN_INFO_EX_S
{
    //HI_BOOL    bBatchModify;
    RGN_INFO_S enRgnInfo[3];
}RGN_INFO_EX_S;

typedef struct hiRGN_PUT_INFO_S
{
    HI_U32 u32Num;          /* number of region*/
    RGN_BUF_INFO_S *pstBufInfo;
}RGN_PUT_INFO_S;

typedef struct hiRGN_REGISTER_INFO_S
{
    MOD_ID_E          enModId;
    HI_U32            u32MaxDevCnt;   /* If no dev id, should set it 1 */
    HI_U32            u32MaxChnCnt;
    RGN_DETCHCHN      pfnRgnDetchChn;
} RGN_REGISTER_INFO_S;

typedef struct hiRGN_CAPACITY_LAYER_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;

    /* Region Layer [min,max] */
    HI_U32 u32LayerMin;
    HI_U32 u32LayerMax;
}RGN_CAPACITY_LAYER_S;

typedef struct hiRGN_CAPACITY_POINT_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;

    POINT_S stPointMin;
    POINT_S stPointMax;

    /*X (start position) of pixel align number*/
    HI_U32 u32StartXAlign;

    /*Y (start position) of pixel align number*/
    HI_U32 u32StartYAlign;

}RGN_CAPACITY_POINT_S;

typedef struct hiRGN_CAPACITY_SIZE_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;

    SIZE_S stSizeMin;
    SIZE_S stSizeMax;

    /*region width of pixel align number*/
    HI_U32 u32WidthAlign;

    /*region height of pixel align number*/
    HI_U32 u32HeightAlign;

    /*maximum area of region*/
    HI_U32 u32MaxArea;
}RGN_CAPACITY_SIZE_S;

typedef struct hiRGN_CAPACITY_QUADRANGLE_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;

    HI_U32     u32MinThick;
    HI_U32     u32MaxThick;
    POINT_S stPointMin;
    POINT_S stPointMax;
    /*X (start position) of pixel align number*/
    HI_U32  u32StartXAlign;

    /*Y (start position) of pixel align number*/
    HI_U32  u32StartYAlign;
}RGN_CAPACITY_QUADRANGLE_S;

typedef struct hiRGN_CAPACITY_PIXLFMT_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;

    /*pixel format number of region*/
    HI_U32 u32PixlFmtNum;
    /* all pixel format type of region supported----------related item check :8.1,check channel all pixel pormat is same or not.  */
    PIXEL_FORMAT_E aenPixlFmt[PIXEL_FORMAT_NUM_MAX];
}RGN_CAPACITY_PIXLFMT_S;

typedef struct hiRGN_CAPACITY_ALPHA_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;

    HI_U32 u32AlphaMax;
    HI_U32 u32AlphaMin;
}RGN_CAPACITY_ALPHA_S;

typedef struct hiRGN_CAPACITY_BGCLR_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;
}RGN_CAPACITY_BGCLR_S;

typedef enum hiRGN_SORT_KEY_E
{
    RGN_SORT_BY_LAYER = 0,
    RGN_SORT_BY_POSITION,
    RGN_SRT_BUTT
}RGN_SORT_KEY_E;

typedef struct hiRGN_CAPACITY_SORT_S
{
    /* sort or not */
    HI_BOOL bNeedSort;

    /*key word used in sort*/
    RGN_SORT_KEY_E enRgnSortKey;

    /*the sort way: 1/true: small to larger;2/true: larger to small;*/
    HI_BOOL bSmallToBig;
}RGN_CAPACITY_SORT_S;

typedef struct hiRGN_CAPACITY_QP_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;

    HI_BOOL bSptQpDisable;

    HI_S32 s32QpAbsMin;
    HI_S32 s32QpAbsMax;

    HI_S32 s32QpRelMin;
    HI_S32 s32QpRelMax;

}RGN_CAPACITY_QP_S;

typedef struct hiRGN_CAPACITY_INVCOLOR_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;

    RGN_CAPACITY_SIZE_S stSizeCap;

    HI_U32 u32LumMin;
    HI_U32 u32LumMax;

    INVERT_COLOR_MODE_E enInvModMin;
    INVERT_COLOR_MODE_E enInvModMax;

    /*if inversing function used,X (start position) of pixel align number*/
    HI_U32 u32StartXAlign;
    /*if inversing function used,Y (start position) of pixel align number*/
    HI_U32 u32StartYAlign;
    /*if inversing function used,width of pixel align number*/
    HI_U32 u32WidthAlign;
    /*if inversing function used,height of pixel align number*/
    HI_U32 u32HeightAlign;

}RGN_CAPACITY_INVCOLOR_S;


typedef enum hiRGN_CAPACITY_QUADRANGLE_TYPE_E
{
    RGN_CAPACITY_QUADRANGLE_UNSUPORT = 0x0,
    RGN_CAPACITY_QUADRANGLE_TYPE_SOLID = 0x1,
    RGN_CAPACITY_QUADRANGLE_TYPE_UNSOLID = 0x2,
    RGN_CAPACITY_QUADRANGLE_TYPE_BUTT = 0x4,
}RGN_CAPACITY_QUADRANGLE_TYPE_E;

typedef struct hiRGN_CAPACITY_COVER_ATTR_S
{
    HI_BOOL bSptRect;
    RGN_CAPACITY_QUADRANGLE_TYPE_E enSptQuadRangleType;
}RGN_CAPACITY_COVER_ATTR_S;

typedef struct hiRGN_CAPACITY_MSCBLKSZ_S
{
    HI_BOOL bComm;
    HI_BOOL bSptReSet;

    HI_U32 u32MscBlkSzMax;
    HI_U32 u32MscBlkSzMin;
}RGN_CAPACITY_MSCBLKSZ_S;

typedef HI_BOOL (*RGN_CHECK_CHN_CAPACITY)(RGN_TYPE_E enType, const MPP_CHN_S *pstChn);

typedef HI_BOOL (*RGN_CHECK_ATTR)(RGN_TYPE_E enType, const MPP_CHN_S *pstChn, const RGN_CHN_ATTR_S *pstAttr);

typedef struct hiRGN_CAPACITY_S
{
    /*layer of region*/
    RGN_CAPACITY_LAYER_S stLayer;

     /*start position*/
    RGN_CAPACITY_POINT_S stPoint;

    /*region size(width and height)*/
    RGN_CAPACITY_SIZE_S  stSize;

    /*attribute of quadrangle*/
    RGN_CAPACITY_QUADRANGLE_S stQuadRangle;

    /*pixel format*/
    HI_BOOL bSptPixlFmt;
    RGN_CAPACITY_PIXLFMT_S stPixlFmt;

    /*alpha */
    /* foreground alpha */
    HI_BOOL bSptFgAlpha;
    RGN_CAPACITY_ALPHA_S stFgAlpha;

    /*background alpha */
    HI_BOOL bSptBgAlpha;
    RGN_CAPACITY_ALPHA_S stBgAlpha;

    /*overall perspective alpha */
    HI_BOOL bSptGlobalAlpha;
    RGN_CAPACITY_ALPHA_S stGlobalAlpha;

    /*background color of region*/
    HI_BOOL bSptBgClr;
    RGN_CAPACITY_BGCLR_S stBgClr;

    /*sort*/
    HI_BOOL bSptChnSort;
    RGN_CAPACITY_SORT_S stChnSort;

    /*capability of QP*/
    HI_BOOL bSptQp;
    RGN_CAPACITY_QP_S stQp;

    /*inverse color*/
    HI_BOOL bSptInvColoar;
    RGN_CAPACITY_INVCOLOR_S stInvColor;

    RGN_CAPACITY_COVER_ATTR_S stCoverAttr; /* cover attribute */

    /* msc attribute*/
    HI_BOOL bSptMsc;
    RGN_CAPACITY_MSCBLKSZ_S stMscBlkSz;

    /*support bitmap or not*/
    HI_BOOL bSptBitmap;

    /*support overlap or not*/
    HI_BOOL bsptOverlap;

    /*stride align*/
    HI_U32 u32Stride;

    /*the maximumegion of channel*/
    HI_U32 u32RgnNumInChn;

    RGN_CHECK_CHN_CAPACITY pfnCheckChnCapacity; /*Check whether channel support a region type*/

    RGN_CHECK_ATTR pfnCheckAttr; /*Check whether attribute is legal*/

}RGN_CAPACITY_S;

typedef struct hiRGN_EXPORT_FUNC_S
{
    HI_S32 (*pfnRgnRegisterMod)(RGN_TYPE_E enType, const RGN_CAPACITY_S *pstCapacity, const RGN_REGISTER_INFO_S *pstRgtInfo);
    HI_S32 (*pfnUnRgnRegisterMod)(RGN_TYPE_E enType, MOD_ID_E enModId);

    HI_S32 (*pfnRgnGetRegion)(RGN_TYPE_E enType, const MPP_CHN_S *pstChn, RGN_INFO_S *pstRgnInfo);
    HI_S32 (*pfnRgnPutRegion)(RGN_TYPE_E enType, const MPP_CHN_S *pstChn, RGN_PUT_INFO_S *pstRgnPutInfo);
    HI_S32 (*pfnRgnSetModifyFalse)(RGN_TYPE_E enType, const MPP_CHN_S *pstChn);
}RGN_EXPORT_FUNC_S;


#define CKFN_RGN() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN))


#define CKFN_RGN_RegisterMod() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnRegisterMod)


#define CALL_RGN_RegisterMod(enType, pstCapacity, pstRgtInfo) \
    FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnRegisterMod(enType, pstCapacity, pstRgtInfo)


#define CKFN_RGN_UnRegisterMod() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnUnRgnRegisterMod)


#define CALL_RGN_UnRegisterMod(enType, enModId) \
    FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnUnRgnRegisterMod(enType, enModId)


#define CKFN_RGN_GetRegion() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnGetRegion)


#define CALL_RGN_GetRegion(enType, pstChn, pstRgnInfo) \
    FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnGetRegion(enType, pstChn, pstRgnInfo)



#define CKFN_RGN_PutRegion() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnPutRegion)

#define CALL_RGN_PutRegion(enType, pstChn, pstPutInfo) \
    FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnPutRegion(enType, pstChn, pstPutInfo)


#define CKFN_RGN_SetModifyFalse() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnSetModifyFalse)

#define CALL_RGN_SetModifyFalse(enType, pstChn) \
    FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnSetModifyFalse(enType, pstChn)


#ifdef __cplusplus
    #if __cplusplus
    }
    #endif
#endif /* End of #ifdef __cplusplus */

#endif /* __REGION_EXT_H__ */



