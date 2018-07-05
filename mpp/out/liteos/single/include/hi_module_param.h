#ifndef __HI_MOD_PARAM__
#define __HI_MOD_PARAM__

#include "hi_type.h"
#include "hi_defines.h"

typedef struct hiHIFB_MODULE_PARAMS_S
{
    HI_CHAR video[64];
}HIFB_MODULE_PARAMS_S;

typedef struct hiVGS_MODULE_PARAMS_S
{
    HI_U32 u32MaxVgsJob;
    HI_U32 u32MaxVgsTask;
    HI_U32 u32MaxVgsNode;
    HI_U32 au32VgsEn[VGS_IP_NUM];
}VGS_MODULE_PARAMS_S;

typedef struct hiVPSS_MODULE_PARAMS_S
{
    HI_U32 u32VpssEn[VPSS_IP_NUM];
}VPSS_MODULE_PARAMS_S;

typedef struct hiGDC_MODULE_PARAMS_S
{
    HI_U32 u32MaxGdcJob;
    HI_U32 u32MaxGdcTask;
    HI_U32 u32MaxGdcNode;
    HI_U32 au32GdcEn[GDC_IP_NUM];
}GDC_MODULE_PARAMS_S;

typedef struct hiIVE_MODULE_PARAMS_S
{
    HI_BOOL bSavePowerEn;
}IVE_MODULE_PARAMS_S;

typedef struct hiACODEC_MODULE_PARAMS_S
{
    HI_U32  u32InitDelayTimeMs;
}ACODEC_MODULE_PARAMS_S;

typedef struct hiISP_MODULE_PARAMS_S
{
    HI_U32 u32PwmNum;
    HI_U32 u32ProcParam;
    HI_U32 u32UpdatePos;
    HI_U32 u32IntTimeOut;
    HI_U32 bIntBottomHalf;
    HI_U32 u32StatIntvl;

}ISP_MODULE_PARAMS_S;


#endif

