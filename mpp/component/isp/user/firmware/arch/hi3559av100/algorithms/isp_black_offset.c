/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_black_offset.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2017/04/24
  Description   :
  History       :
  1.Date        : 2017/04/24
    Modification: Created file

******************************************************************************/

#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_config.h"
#include "isp_ext_config.h"
#include "isp_proc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define LUT_FACTOR (7)

typedef struct hiISP_BLACKLEVEL_S
{
    HI_BOOL bPreDefectPixel;
    HI_U8   u8BlackLevelChange;
    HI_U8   u8WDRModeState;
    HI_U16  au16BlackLevel[4];
    HI_U16  au16ActualBlackLevel[4];
} ISP_BLACKLEVEL_S;

ISP_BLACKLEVEL_S g_astBlackLevelCtx[ISP_MAX_PIPE_NUM] = {{0}};
#define BLACKLEVEL_GET_CTX(dev, pstBlackLevelCtx)   pstBlackLevelCtx = &g_astBlackLevelCtx[dev]

static HI_VOID BlcInitialize(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_BLACKLEVEL_S        *pstBlackLevel    = HI_NULL;
    ISP_CMOS_BLACK_LEVEL_S  *pstSnsBlackLevel = HI_NULL;
    ISP_CTX_S              *pstIspCtx         = HI_NULL;
    HI_U8  u8WDRMode;

    BLACKLEVEL_GET_CTX(ViPipe, pstBlackLevel);
    ISP_SensorGetBlc(ViPipe, &pstSnsBlackLevel);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    u8WDRMode = pstIspCtx->u8SnsWDRMode;

    for (i = 0; i < 4; i++)
    {
        pstBlackLevel->au16BlackLevel[i] = pstSnsBlackLevel->au16BlackLevel[i];
    }

    if (IS_LINEAR_MODE(u8WDRMode) || IS_BUILT_IN_WDR_MODE(u8WDRMode))
    {
        pstBlackLevel->u8WDRModeState = HI_FALSE;
    }
    else
    {
        pstBlackLevel->u8WDRModeState = HI_TRUE;
    }

    pstBlackLevel->bPreDefectPixel    = HI_FALSE;

    return;
}

static HI_VOID BlcExtRegsInitialize(VI_PIPE ViPipe)
{
    ISP_BLACKLEVEL_S        *pstBlackLevel    = HI_NULL;

    BLACKLEVEL_GET_CTX(ViPipe, pstBlackLevel);

    hi_ext_system_black_level_change_write(ViPipe, HI_EXT_SYSTEM_BLACK_LEVEL_CHANGE_DEFAULT);
    hi_ext_system_black_level_00_write(ViPipe, pstBlackLevel->au16BlackLevel[0]);
    hi_ext_system_black_level_01_write(ViPipe, pstBlackLevel->au16BlackLevel[1]);
    hi_ext_system_black_level_10_write(ViPipe, pstBlackLevel->au16BlackLevel[2]);
    hi_ext_system_black_level_11_write(ViPipe, pstBlackLevel->au16BlackLevel[3]);

    hi_ext_system_black_level_query_00_write(ViPipe, pstBlackLevel->au16BlackLevel[0]);
    hi_ext_system_black_level_query_01_write(ViPipe, pstBlackLevel->au16BlackLevel[1]);
    hi_ext_system_black_level_query_10_write(ViPipe, pstBlackLevel->au16BlackLevel[2]);
    hi_ext_system_black_level_query_11_write(ViPipe, pstBlackLevel->au16BlackLevel[3]);

    return;
}

static HI_S32 LinearInterpol(HI_U8 xm, HI_U8  x0, HI_S32 y0, HI_U8 x1, HI_S32 y1)
{
    HI_S32 ym;

    if ( xm <= x0 )
    {
        return y0;
    }
    if ( xm >= x1 )
    {
        return y1;
    }

    ym = (y1 - y0) * (xm - x0) / DIV_0_TO_1(x1 - x0) + y0;
    return ym;
}

static HI_VOID GetBuiltInExpanderBlc(ISP_CMOS_SPLIT_S *pstSnsSplit, HI_U16 *pu16SensorBlc,HI_U16 *pu16ExpanderBlc)
{
    HI_U8  i, j;
    HI_U8  u8Shift    = 12 - LUT_FACTOR;
    HI_U8  u8IndexUp  = ISP_SPLIT_POINT_NUM - 1;
    HI_U8  u8IndexLow = 0;
    HI_U8  au8X[ISP_SPLIT_POINT_NUM + 1]  = {0};
    HI_U16 au16Y[ISP_SPLIT_POINT_NUM + 1] = {0};

    for (i = 1; i < ISP_SPLIT_POINT_NUM + 1; i++)
    {
        au8X[i]  = pstSnsSplit->astSplitPoint[i - 1].u8X;
        au16Y[i] = pstSnsSplit->astSplitPoint[i - 1].u16Y;
    }

    for (j = 0; j < 4; j++)
    {
        u8IndexUp  = ISP_SPLIT_POINT_NUM - 1;

        for (i = 0; i < ISP_SPLIT_POINT_NUM; i++)
        {
            if ((pu16SensorBlc[j] >> u8Shift) < au8X[i])
            {
                u8IndexUp = i;
                break;
            }
        }

        u8IndexLow = (HI_U8)MAX2((HI_S8)u8IndexUp - 1,0);

        pu16ExpanderBlc[j] = ((HI_U16)LinearInterpol(pu16SensorBlc[j] >> u8Shift,au8X[u8IndexLow],au16Y[u8IndexLow],au8X[u8IndexUp],au16Y[u8IndexUp])) >> 1;
    }

    return;
}

static HI_VOID BE_BlcDynaRegs_Linear(VI_PIPE ViPipe, ISP_BE_BLC_CFG_S  *pstBeBlcCfg, HI_U16 *pu16BlackLevel)
{
    HI_U8 i;
    ISP_BLACKLEVEL_S *pstBlackLevel = HI_NULL;

    BLACKLEVEL_GET_CTX(ViPipe, pstBlackLevel);

    /*split*/
    pstBeBlcCfg->stSplitBlc.stUsrRegCfg.au16Blc[0]  = 0;
    pstBeBlcCfg->stSplitBlc.stUsrRegCfg.au16Blc[1]  = 0;
    pstBeBlcCfg->stSplitBlc.stUsrRegCfg.au16Blc[2]  = 0;
    pstBeBlcCfg->stSplitBlc.stUsrRegCfg.au16Blc[3]  = 0;
    pstBeBlcCfg->stSplitBlc.stUsrRegCfg.u16OutBlc   = 0;

    /*4DG*/
    for (i = 0; i < 4; i++)
    {
        pstBeBlcCfg->st4DgBlc[i].stUsrRegCfg.au16Blc[0] = pu16BlackLevel[0] << 2;
        pstBeBlcCfg->st4DgBlc[i].stUsrRegCfg.au16Blc[1] = pu16BlackLevel[0] << 2;
        pstBeBlcCfg->st4DgBlc[i].stUsrRegCfg.au16Blc[2] = pu16BlackLevel[0] << 2;
        pstBeBlcCfg->st4DgBlc[i].stUsrRegCfg.au16Blc[3] = pu16BlackLevel[0] << 2;
    }

    for (i = 0; i < 4; i++)
    {
        /*WDR*/
        pstBeBlcCfg->stWdrBlc[i].stUsrRegCfg.au16Blc[0] = 0;
        pstBeBlcCfg->stWdrBlc[i].stUsrRegCfg.au16Blc[1] = 0;
        pstBeBlcCfg->stWdrBlc[i].stUsrRegCfg.au16Blc[2] = 0;
        pstBeBlcCfg->stWdrBlc[i].stUsrRegCfg.au16Blc[3] = 0;
        pstBeBlcCfg->stWdrBlc[i].stUsrRegCfg.u16OutBlc  = 0;

        /*flicker*/
        pstBeBlcCfg->stFlickerBlc[i].stUsrRegCfg.au16Blc[0] = 0;
        pstBeBlcCfg->stFlickerBlc[i].stUsrRegCfg.au16Blc[1] = 0;
        pstBeBlcCfg->stFlickerBlc[i].stUsrRegCfg.au16Blc[2] = 0;
        pstBeBlcCfg->stFlickerBlc[i].stUsrRegCfg.au16Blc[3] = 0;
    }

    /*LogLUT*/
    pstBeBlcCfg->stLogLUTBlc.stUsrRegCfg.au16Blc[0] = pu16BlackLevel[0] << 4;
    pstBeBlcCfg->stLogLUTBlc.stUsrRegCfg.au16Blc[1] = pu16BlackLevel[1] << 4;
    pstBeBlcCfg->stLogLUTBlc.stUsrRegCfg.au16Blc[2] = pu16BlackLevel[2] << 4;
    pstBeBlcCfg->stLogLUTBlc.stUsrRegCfg.au16Blc[3] = pu16BlackLevel[3] << 4;

    /*rlsc*/
    pstBeBlcCfg->stRLscBlc.stUsrRegCfg.au16Blc[0] = pu16BlackLevel[0] << 2;
    pstBeBlcCfg->stRLscBlc.stUsrRegCfg.au16Blc[1] = pu16BlackLevel[1] << 2;
    pstBeBlcCfg->stRLscBlc.stUsrRegCfg.au16Blc[2] = pu16BlackLevel[2] << 2;
    pstBeBlcCfg->stRLscBlc.stUsrRegCfg.au16Blc[3] = pu16BlackLevel[3] << 2;
    /*lsc*/
    pstBeBlcCfg->stLscBlc.stUsrRegCfg.au16Blc[0]  = pu16BlackLevel[0] << 2;
    pstBeBlcCfg->stLscBlc.stUsrRegCfg.au16Blc[1]  = pu16BlackLevel[1] << 2;
    pstBeBlcCfg->stLscBlc.stUsrRegCfg.au16Blc[2]  = pu16BlackLevel[2] << 2;
    pstBeBlcCfg->stLscBlc.stUsrRegCfg.au16Blc[3]  = pu16BlackLevel[3] << 2;
    /*Dg*/
    pstBeBlcCfg->stDgBlc.stUsrRegCfg.au16Blc[0]   = pu16BlackLevel[0] << 2;
    pstBeBlcCfg->stDgBlc.stUsrRegCfg.au16Blc[1]   = pu16BlackLevel[1] << 2;
    pstBeBlcCfg->stDgBlc.stUsrRegCfg.au16Blc[2]   = pu16BlackLevel[2] << 2;
    pstBeBlcCfg->stDgBlc.stUsrRegCfg.au16Blc[3]   = pu16BlackLevel[3] << 2;
    /*AE*/
    pstBeBlcCfg->stAeBlc.stUsrRegCfg.au16Blc[0]   = pu16BlackLevel[0] << 2;
    pstBeBlcCfg->stAeBlc.stUsrRegCfg.au16Blc[1]   = pu16BlackLevel[1] << 2;
    pstBeBlcCfg->stAeBlc.stUsrRegCfg.au16Blc[2]   = pu16BlackLevel[2] << 2;
    pstBeBlcCfg->stAeBlc.stUsrRegCfg.au16Blc[3]   = pu16BlackLevel[3] << 2;
    /*MG*/
    pstBeBlcCfg->stMgBlc.stUsrRegCfg.au16Blc[0]   = pu16BlackLevel[0] << 2;
    pstBeBlcCfg->stMgBlc.stUsrRegCfg.au16Blc[1]   = pu16BlackLevel[1] << 2;
    pstBeBlcCfg->stMgBlc.stUsrRegCfg.au16Blc[2]   = pu16BlackLevel[2] << 2;
    pstBeBlcCfg->stMgBlc.stUsrRegCfg.au16Blc[3]   = pu16BlackLevel[3] << 2;
    /*WB*/
    pstBeBlcCfg->stWbBlc.stUsrRegCfg.au16Blc[0]   = pu16BlackLevel[0] << 2;
    pstBeBlcCfg->stWbBlc.stUsrRegCfg.au16Blc[1]   = pu16BlackLevel[1] << 2;
    pstBeBlcCfg->stWbBlc.stUsrRegCfg.au16Blc[2]   = pu16BlackLevel[2] << 2;
    pstBeBlcCfg->stWbBlc.stUsrRegCfg.au16Blc[3]   = pu16BlackLevel[3] << 2;

    /*pregamma*/
    pstBeBlcCfg->stPreGammaBlc.stUsrRegCfg.au16Blc[0] = 0;
    pstBeBlcCfg->stPreGammaBlc.stUsrRegCfg.au16Blc[1] = 0;
    pstBeBlcCfg->stPreGammaBlc.stUsrRegCfg.au16Blc[2] = 0;
    pstBeBlcCfg->stPreGammaBlc.stUsrRegCfg.au16Blc[3] = 0;

    for (i = 0; i < 4; i++)
    {
        pstBlackLevel->au16ActualBlackLevel[i] = pu16BlackLevel[i];
    }

    return;
}

static HI_VOID BE_BlcDynaRegs_Wdr(VI_PIPE ViPipe, HI_U8 u8WDRModeState, ISP_BE_BLC_CFG_S  *pstBeBlcCfg, HI_U16 *pu16BlackLevel)
{
    HI_U8 i;
    ISP_BLACKLEVEL_S *pstBlackLevel = HI_NULL;

    BLACKLEVEL_GET_CTX(ViPipe, pstBlackLevel);

    if (HI_FALSE == u8WDRModeState) //reg value same as linear mode
    {
        BE_BlcDynaRegs_Linear(ViPipe, pstBeBlcCfg, pu16BlackLevel);
    }
    else if (HI_TRUE == u8WDRModeState)
    {
        /*split*/
        pstBeBlcCfg->stSplitBlc.stUsrRegCfg.au16Blc[0]  = 0;
        pstBeBlcCfg->stSplitBlc.stUsrRegCfg.au16Blc[1]  = 0;
        pstBeBlcCfg->stSplitBlc.stUsrRegCfg.au16Blc[2]  = 0;
        pstBeBlcCfg->stSplitBlc.stUsrRegCfg.au16Blc[3]  = 0;
        pstBeBlcCfg->stSplitBlc.stUsrRegCfg.u16OutBlc   = 0;

        /*4DG*/
        for (i = 0; i < 4; i++)
        {
            pstBeBlcCfg->st4DgBlc[i].stUsrRegCfg.au16Blc[0]  = pu16BlackLevel[0] << 2;
            pstBeBlcCfg->st4DgBlc[i].stUsrRegCfg.au16Blc[1]  = pu16BlackLevel[1] << 2;
            pstBeBlcCfg->st4DgBlc[i].stUsrRegCfg.au16Blc[2]  = pu16BlackLevel[2] << 2;
            pstBeBlcCfg->st4DgBlc[i].stUsrRegCfg.au16Blc[3]  = pu16BlackLevel[3] << 2;
        }

        for (i = 0; i < 4; i++)
        {
            /*WDR*/
            pstBeBlcCfg->stWdrBlc[i].stUsrRegCfg.au16Blc[0]  = pu16BlackLevel[0] << 2;
            pstBeBlcCfg->stWdrBlc[i].stUsrRegCfg.au16Blc[1]  = pu16BlackLevel[1] << 2;
            pstBeBlcCfg->stWdrBlc[i].stUsrRegCfg.au16Blc[2]  = pu16BlackLevel[2] << 2;
            pstBeBlcCfg->stWdrBlc[i].stUsrRegCfg.au16Blc[3]  = pu16BlackLevel[3] << 2;
            pstBeBlcCfg->stWdrBlc[i].stUsrRegCfg.u16OutBlc   = pu16BlackLevel[0] << 2;
            /*flicker*/
            pstBeBlcCfg->stFlickerBlc[i].stUsrRegCfg.au16Blc[0]  = pu16BlackLevel[0] << 2;
            pstBeBlcCfg->stFlickerBlc[i].stUsrRegCfg.au16Blc[1]  = pu16BlackLevel[1] << 2;
            pstBeBlcCfg->stFlickerBlc[i].stUsrRegCfg.au16Blc[2]  = pu16BlackLevel[2] << 2;
            pstBeBlcCfg->stFlickerBlc[i].stUsrRegCfg.au16Blc[3]  = pu16BlackLevel[3] << 2;
        }

        /*LogLUT*/
        pstBeBlcCfg->stLogLUTBlc.stUsrRegCfg.au16Blc[0]  = 0;
        pstBeBlcCfg->stLogLUTBlc.stUsrRegCfg.au16Blc[1]  = 0;
        pstBeBlcCfg->stLogLUTBlc.stUsrRegCfg.au16Blc[2]  = 0;
        pstBeBlcCfg->stLogLUTBlc.stUsrRegCfg.au16Blc[3]  = 0;

        /*rlsc*/
        pstBeBlcCfg->stRLscBlc.stUsrRegCfg.au16Blc[0] = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stRLscBlc.stUsrRegCfg.au16Blc[1] = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stRLscBlc.stUsrRegCfg.au16Blc[2] = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stRLscBlc.stUsrRegCfg.au16Blc[3] = pu16BlackLevel[0] >> 4;
        /*lsc*/
        pstBeBlcCfg->stLscBlc.stUsrRegCfg.au16Blc[0]  = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stLscBlc.stUsrRegCfg.au16Blc[1]  = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stLscBlc.stUsrRegCfg.au16Blc[2]  = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stLscBlc.stUsrRegCfg.au16Blc[3]  = pu16BlackLevel[0] >> 4;

        /*Dg*/
        pstBeBlcCfg->stDgBlc.stUsrRegCfg.au16Blc[0]  = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stDgBlc.stUsrRegCfg.au16Blc[1]  = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stDgBlc.stUsrRegCfg.au16Blc[2]  = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stDgBlc.stUsrRegCfg.au16Blc[3]  = pu16BlackLevel[0] >> 4;

        /*AE*/
        pstBeBlcCfg->stAeBlc.stUsrRegCfg.au16Blc[0]  = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stAeBlc.stUsrRegCfg.au16Blc[1]  = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stAeBlc.stUsrRegCfg.au16Blc[2]  = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stAeBlc.stUsrRegCfg.au16Blc[3]  = pu16BlackLevel[0] >> 4;
        /*MG*/
        pstBeBlcCfg->stMgBlc.stUsrRegCfg.au16Blc[0]  = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stMgBlc.stUsrRegCfg.au16Blc[1]  = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stMgBlc.stUsrRegCfg.au16Blc[2]  = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stMgBlc.stUsrRegCfg.au16Blc[3]  = pu16BlackLevel[0] >> 4;

        /*WB*/
        pstBeBlcCfg->stWbBlc.stUsrRegCfg.au16Blc[0]  = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stWbBlc.stUsrRegCfg.au16Blc[1]  = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stWbBlc.stUsrRegCfg.au16Blc[2]  = pu16BlackLevel[0] >> 4;
        pstBeBlcCfg->stWbBlc.stUsrRegCfg.au16Blc[3]  = pu16BlackLevel[0] >> 4;

        /*pregamma*/
        pstBeBlcCfg->stPreGammaBlc.stUsrRegCfg.au16Blc[0] = 0;
        pstBeBlcCfg->stPreGammaBlc.stUsrRegCfg.au16Blc[1] = 0;
        pstBeBlcCfg->stPreGammaBlc.stUsrRegCfg.au16Blc[2] = 0;
        pstBeBlcCfg->stPreGammaBlc.stUsrRegCfg.au16Blc[3] = 0;

        for (i = 0; i < 4; i++)
        {
            pstBlackLevel->au16ActualBlackLevel[i] = pu16BlackLevel[i];
        }
    }
}

static HI_VOID BE_BlcDynaRegs_BuiltIn(VI_PIPE ViPipe, ISP_BE_BLC_CFG_S  *pstBeBlcCfg, HI_U16 *pu16BlackLevel)
{
    HI_U8  i;
    HI_U16 au16BlackLevel[4] = {0};
    ISP_BLACKLEVEL_S   *pstBlackLevel = HI_NULL;
    ISP_CMOS_DEFAULT_S *pstSnsDft     = HI_NULL;

    ISP_SensorGetDefault(ViPipe, &pstSnsDft);
    BLACKLEVEL_GET_CTX(ViPipe, pstBlackLevel);

    if (pstSnsDft->stSplit.bValid)
    {
        GetBuiltInExpanderBlc(&pstSnsDft->stSplit, pu16BlackLevel,au16BlackLevel);
    }
    else
    {
        for (i = 0; i < 4; i++)
        {
            au16BlackLevel[i] = pu16BlackLevel[i] << 2;//14bits
        }
    }

    /*split*/
    pstBeBlcCfg->stSplitBlc.stUsrRegCfg.au16Blc[0]  = 0;
    pstBeBlcCfg->stSplitBlc.stUsrRegCfg.au16Blc[1]  = 0;
    pstBeBlcCfg->stSplitBlc.stUsrRegCfg.au16Blc[2]  = 0;
    pstBeBlcCfg->stSplitBlc.stUsrRegCfg.au16Blc[3]  = 0;
    pstBeBlcCfg->stSplitBlc.stUsrRegCfg.u16OutBlc   = 0;

    /*4DG*/
    for (i = 0; i < 4; i++)
    {
        pstBeBlcCfg->st4DgBlc[i].stUsrRegCfg.au16Blc[0] = 0;
        pstBeBlcCfg->st4DgBlc[i].stUsrRegCfg.au16Blc[1] = 0;
        pstBeBlcCfg->st4DgBlc[i].stUsrRegCfg.au16Blc[2] = 0;
        pstBeBlcCfg->st4DgBlc[i].stUsrRegCfg.au16Blc[3] = 0;
    }

    for (i = 0; i < 4; i++)
    {
        /*WDR*/
        pstBeBlcCfg->stWdrBlc[i].stUsrRegCfg.au16Blc[0]  = 0;
        pstBeBlcCfg->stWdrBlc[i].stUsrRegCfg.au16Blc[1]  = 0;
        pstBeBlcCfg->stWdrBlc[i].stUsrRegCfg.au16Blc[2]  = 0;
        pstBeBlcCfg->stWdrBlc[i].stUsrRegCfg.au16Blc[3]  = 0;
        pstBeBlcCfg->stWdrBlc[i].stUsrRegCfg.u16OutBlc   = 0;

        /*flicker*/
        pstBeBlcCfg->stFlickerBlc[i].stUsrRegCfg.au16Blc[0] = 0;
        pstBeBlcCfg->stFlickerBlc[i].stUsrRegCfg.au16Blc[1] = 0;
        pstBeBlcCfg->stFlickerBlc[i].stUsrRegCfg.au16Blc[2] = 0;
        pstBeBlcCfg->stFlickerBlc[i].stUsrRegCfg.au16Blc[3] = 0;
    }

    /*LogLUT*/
    pstBeBlcCfg->stLogLUTBlc.stUsrRegCfg.au16Blc[0]  = 0;
    pstBeBlcCfg->stLogLUTBlc.stUsrRegCfg.au16Blc[1]  = 0;
    pstBeBlcCfg->stLogLUTBlc.stUsrRegCfg.au16Blc[2]  = 0;
    pstBeBlcCfg->stLogLUTBlc.stUsrRegCfg.au16Blc[3]  = 0;

    /*rlsc*/
    pstBeBlcCfg->stRLscBlc.stUsrRegCfg.au16Blc[0] = au16BlackLevel[0];
    pstBeBlcCfg->stRLscBlc.stUsrRegCfg.au16Blc[1] = au16BlackLevel[1];
    pstBeBlcCfg->stRLscBlc.stUsrRegCfg.au16Blc[2] = au16BlackLevel[2];
    pstBeBlcCfg->stRLscBlc.stUsrRegCfg.au16Blc[3] = au16BlackLevel[3];
    /*lsc*/
    pstBeBlcCfg->stLscBlc.stUsrRegCfg.au16Blc[0] = au16BlackLevel[0];
    pstBeBlcCfg->stLscBlc.stUsrRegCfg.au16Blc[1] = au16BlackLevel[1];
    pstBeBlcCfg->stLscBlc.stUsrRegCfg.au16Blc[2] = au16BlackLevel[2];
    pstBeBlcCfg->stLscBlc.stUsrRegCfg.au16Blc[3] = au16BlackLevel[3];
    /*Dg*/
    pstBeBlcCfg->stDgBlc.stUsrRegCfg.au16Blc[0]  = au16BlackLevel[0];
    pstBeBlcCfg->stDgBlc.stUsrRegCfg.au16Blc[1]  = au16BlackLevel[1];
    pstBeBlcCfg->stDgBlc.stUsrRegCfg.au16Blc[2]  = au16BlackLevel[2];
    pstBeBlcCfg->stDgBlc.stUsrRegCfg.au16Blc[3]  = au16BlackLevel[3];
    /*AE*/
    pstBeBlcCfg->stAeBlc.stUsrRegCfg.au16Blc[0]  = au16BlackLevel[0];
    pstBeBlcCfg->stAeBlc.stUsrRegCfg.au16Blc[1]  = au16BlackLevel[1];
    pstBeBlcCfg->stAeBlc.stUsrRegCfg.au16Blc[2]  = au16BlackLevel[2];
    pstBeBlcCfg->stAeBlc.stUsrRegCfg.au16Blc[3]  = au16BlackLevel[3];
    /*MG*/
    pstBeBlcCfg->stMgBlc.stUsrRegCfg.au16Blc[0]  = au16BlackLevel[0];
    pstBeBlcCfg->stMgBlc.stUsrRegCfg.au16Blc[1]  = au16BlackLevel[1];
    pstBeBlcCfg->stMgBlc.stUsrRegCfg.au16Blc[2]  = au16BlackLevel[2];
    pstBeBlcCfg->stMgBlc.stUsrRegCfg.au16Blc[3]  = au16BlackLevel[3];
    /*WB*/
    pstBeBlcCfg->stWbBlc.stUsrRegCfg.au16Blc[0]  = au16BlackLevel[0];
    pstBeBlcCfg->stWbBlc.stUsrRegCfg.au16Blc[1]  = au16BlackLevel[1];
    pstBeBlcCfg->stWbBlc.stUsrRegCfg.au16Blc[2]  = au16BlackLevel[2];
    pstBeBlcCfg->stWbBlc.stUsrRegCfg.au16Blc[3]  = au16BlackLevel[3];

    /*pregamma*/
    pstBeBlcCfg->stPreGammaBlc.stUsrRegCfg.au16Blc[0]   = 0;
    pstBeBlcCfg->stPreGammaBlc.stUsrRegCfg.au16Blc[1]   = 0;
    pstBeBlcCfg->stPreGammaBlc.stUsrRegCfg.au16Blc[2]   = 0;
    pstBeBlcCfg->stPreGammaBlc.stUsrRegCfg.au16Blc[3]   = 0;

    for (i = 0; i < 4; i++)
    {
        pstBlackLevel->au16ActualBlackLevel[i] = au16BlackLevel[i] >> 2;
    }
}

static HI_VOID BE_BlcDynaRegs(VI_PIPE ViPipe, HI_U8  u8WdrMode, ISP_BE_BLC_CFG_S  *pstBeBlcCfg, HI_U16 *pu16BlackLevel)
{
    ISP_BLACKLEVEL_S   *pstBlackLevel    = HI_NULL;

    BLACKLEVEL_GET_CTX(ViPipe, pstBlackLevel);

    pstBlackLevel->u8WDRModeState = hi_ext_system_wdr_en_read(ViPipe);

    if (IS_LINEAR_MODE(u8WdrMode))
    {
        BE_BlcDynaRegs_Linear(ViPipe, pstBeBlcCfg, pu16BlackLevel);
    }
    else if (IS_2to1_WDR_MODE(u8WdrMode) || IS_3to1_WDR_MODE(u8WdrMode) || IS_4to1_WDR_MODE(u8WdrMode))
    {
        BE_BlcDynaRegs_Wdr(ViPipe, pstBlackLevel->u8WDRModeState, pstBeBlcCfg, pu16BlackLevel);
    }
    else if (IS_BUILT_IN_WDR_MODE(u8WdrMode))
    {
        BE_BlcDynaRegs_BuiltIn(ViPipe, pstBeBlcCfg, pu16BlackLevel);
    }

    pstBeBlcCfg->bReshDyna = HI_TRUE;
    return;
}


static HI_VOID BE_BlcStaticRegs(HI_U8  u8WdrMode, ISP_BE_BLC_CFG_S  *pstBeBlcCfg)
{
    HI_U8 i;

    /*4DG*/
    for (i = 0; i < 4; i++)
    {
        pstBeBlcCfg->st4DgBlc[i].stStaticRegCfg.bBlcIn  = HI_TRUE;
        pstBeBlcCfg->st4DgBlc[i].stStaticRegCfg.bBlcOut = HI_TRUE;
    }

    /*WDR*/
    pstBeBlcCfg->stWdrBlc[0].stStaticRegCfg.bBlcOut = HI_TRUE;
    /*rlsc*/
    pstBeBlcCfg->stRLscBlc.stStaticRegCfg.bBlcIn    = HI_TRUE;
    pstBeBlcCfg->stRLscBlc.stStaticRegCfg.bBlcOut   = HI_TRUE;
    /*lsc*/
    pstBeBlcCfg->stLscBlc.stStaticRegCfg.bBlcIn     = HI_TRUE;
    pstBeBlcCfg->stLscBlc.stStaticRegCfg.bBlcOut    = HI_TRUE;
    /*Dg*/
    pstBeBlcCfg->stDgBlc.stStaticRegCfg.bBlcIn      = HI_TRUE;
    pstBeBlcCfg->stDgBlc.stStaticRegCfg.bBlcOut     = HI_FALSE;
    /*AE*/
    pstBeBlcCfg->stAeBlc.stStaticRegCfg.bBlcIn      = HI_FALSE;
    /*MG*/
    pstBeBlcCfg->stMgBlc.stStaticRegCfg.bBlcIn      = HI_FALSE;
    /*WB*/
    pstBeBlcCfg->stWbBlc.stStaticRegCfg.bBlcIn      = HI_FALSE;
    pstBeBlcCfg->stWbBlc.stStaticRegCfg.bBlcOut     = HI_FALSE;

    pstBeBlcCfg->bReshStatic = HI_TRUE;
    return;
}

static HI_VOID FE_BlcDynaRegs(ISP_FE_BLC_CFG_S  *pstFeBlcCfg, HI_U16 *pu16BlackLevel)
{
    /*Fe Dg*/
    pstFeBlcCfg->stFeDgBlc.stUsrRegCfg.au16Blc[0] = pu16BlackLevel[0] << 2;
    pstFeBlcCfg->stFeDgBlc.stUsrRegCfg.au16Blc[1] = pu16BlackLevel[1] << 2;
    pstFeBlcCfg->stFeDgBlc.stUsrRegCfg.au16Blc[2] = pu16BlackLevel[2] << 2;
    pstFeBlcCfg->stFeDgBlc.stUsrRegCfg.au16Blc[3] = pu16BlackLevel[3] << 2;

    /*Fe WB*/
    pstFeBlcCfg->stFeWbBlc.stUsrRegCfg.au16Blc[0] = pu16BlackLevel[0] << 2;
    pstFeBlcCfg->stFeWbBlc.stUsrRegCfg.au16Blc[1] = pu16BlackLevel[1] << 2;
    pstFeBlcCfg->stFeWbBlc.stUsrRegCfg.au16Blc[2] = pu16BlackLevel[2] << 2;
    pstFeBlcCfg->stFeWbBlc.stUsrRegCfg.au16Blc[3] = pu16BlackLevel[3] << 2;

    /*Fe AE*/
    pstFeBlcCfg->stFeAeBlc.stUsrRegCfg.au16Blc[0] = pu16BlackLevel[0] << 2;
    pstFeBlcCfg->stFeAeBlc.stUsrRegCfg.au16Blc[1] = pu16BlackLevel[1] << 2;
    pstFeBlcCfg->stFeAeBlc.stUsrRegCfg.au16Blc[2] = pu16BlackLevel[2] << 2;
    pstFeBlcCfg->stFeAeBlc.stUsrRegCfg.au16Blc[3] = pu16BlackLevel[3] << 2;

    /*Fe LSC*/
    pstFeBlcCfg->stFeLscBlc.stUsrRegCfg.au16Blc[0] = pu16BlackLevel[0] << 2;
    pstFeBlcCfg->stFeLscBlc.stUsrRegCfg.au16Blc[1] = pu16BlackLevel[1] << 2;
    pstFeBlcCfg->stFeLscBlc.stUsrRegCfg.au16Blc[2] = pu16BlackLevel[2] << 2;
    pstFeBlcCfg->stFeLscBlc.stUsrRegCfg.au16Blc[3] = pu16BlackLevel[3] << 2;

    /*RC*/
    pstFeBlcCfg->stRcBlc.stUsrRegCfg.au16Blc[0] = pu16BlackLevel[0] << 2;
    pstFeBlcCfg->stRcBlc.stUsrRegCfg.au16Blc[1] = pu16BlackLevel[1] << 2;
    pstFeBlcCfg->stRcBlc.stUsrRegCfg.au16Blc[2] = pu16BlackLevel[2] << 2;
    pstFeBlcCfg->stRcBlc.stUsrRegCfg.au16Blc[3] = pu16BlackLevel[3] << 2;

    /*Fe LogLUT*/
    pstFeBlcCfg->stFeLogLUTBlc.stUsrRegCfg.au16Blc[0] = pu16BlackLevel[0] << 4;
    pstFeBlcCfg->stFeLogLUTBlc.stUsrRegCfg.au16Blc[1] = pu16BlackLevel[1] << 4;
    pstFeBlcCfg->stFeLogLUTBlc.stUsrRegCfg.au16Blc[2] = pu16BlackLevel[2] << 4;
    pstFeBlcCfg->stFeLogLUTBlc.stUsrRegCfg.au16Blc[3] = pu16BlackLevel[3] << 4;

    pstFeBlcCfg->bReshDyna = HI_TRUE;

    return;
}


static HI_VOID FE_BlcStaticRegs(ISP_FE_BLC_CFG_S  *pstFeBlcCfg)
{
    /*Fe Dg*/
    pstFeBlcCfg->stFeDgBlc.stStaticRegCfg.bBlcIn = HI_TRUE;
    pstFeBlcCfg->stFeDgBlc.stStaticRegCfg.bBlcOut = HI_TRUE;
    /*Fe WB*/
    pstFeBlcCfg->stFeWbBlc.stStaticRegCfg.bBlcIn = HI_TRUE;
    pstFeBlcCfg->stFeWbBlc.stStaticRegCfg.bBlcOut = HI_TRUE;
    /*Fe AE*/
    pstFeBlcCfg->stFeAeBlc.stStaticRegCfg.bBlcIn = HI_FALSE;
    /*Fe LSC*/
    pstFeBlcCfg->stFeLscBlc.stStaticRegCfg.bBlcIn = HI_TRUE;
    pstFeBlcCfg->stFeLscBlc.stStaticRegCfg.bBlcOut = HI_TRUE;
    /*RC*/
    pstFeBlcCfg->stRcBlc.stStaticRegCfg.bBlcIn    = HI_FALSE;
    pstFeBlcCfg->stRcBlc.stStaticRegCfg.bBlcOut   = HI_FALSE;

    pstFeBlcCfg->bReshStatic = HI_TRUE;
    return;
}

static HI_VOID BlcRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_S32 i;
    HI_U8  u8WdrMode;
    ISP_BLACKLEVEL_S       *pstBlackLevel     = HI_NULL;
    ISP_CTX_S              *pstIspCtx         = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    BLACKLEVEL_GET_CTX(ViPipe, pstBlackLevel);

    u8WdrMode = pstIspCtx->u8SnsWDRMode;

    /*BE*/
    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        BE_BlcDynaRegs(ViPipe, u8WdrMode, &pstRegCfg->stAlgRegCfg[i].stBeBlcCfg, pstBlackLevel->au16BlackLevel);
        BE_BlcStaticRegs(u8WdrMode, &pstRegCfg->stAlgRegCfg[i].stBeBlcCfg);
    }
    pstRegCfg->unKey.bit1BeBlcCfg = 1;

    /*FE*/
    FE_BlcDynaRegs(&pstRegCfg->stAlgRegCfg[0].stFeBlcCfg, pstBlackLevel->au16BlackLevel);
    FE_BlcStaticRegs(&pstRegCfg->stAlgRegCfg[0].stFeBlcCfg);
    pstRegCfg->unKey.bit1FeBlcCfg = 1;

    return;
}

static HI_S32 BlcReadExtregs(VI_PIPE ViPipe)
{

    ISP_BLACKLEVEL_S *pstBlackLevel = HI_NULL;
    BLACKLEVEL_GET_CTX(ViPipe, pstBlackLevel);

    pstBlackLevel->u8BlackLevelChange = hi_ext_system_black_level_change_read(ViPipe);
    hi_ext_system_black_level_change_write(ViPipe, HI_FALSE);

    if (pstBlackLevel->u8BlackLevelChange)
    {
        pstBlackLevel->au16BlackLevel[0]  = hi_ext_system_black_level_00_read(ViPipe);
        pstBlackLevel->au16BlackLevel[1]  = hi_ext_system_black_level_01_read(ViPipe);
        pstBlackLevel->au16BlackLevel[2]  = hi_ext_system_black_level_10_read(ViPipe);
        pstBlackLevel->au16BlackLevel[3]  = hi_ext_system_black_level_11_read(ViPipe);
    }

    return 0;
}

static HI_VOID BlcWriteActualValue(VI_PIPE ViPipe, HI_U16 *pu16BlackLevel)
{
    hi_ext_system_black_level_query_00_write(ViPipe, pu16BlackLevel[0]);
    hi_ext_system_black_level_query_01_write(ViPipe, pu16BlackLevel[1]);
    hi_ext_system_black_level_query_10_write(ViPipe, pu16BlackLevel[2]);
    hi_ext_system_black_level_query_11_write(ViPipe, pu16BlackLevel[3]);
}

static HI_S32 ISP_BlcProcWrite(VI_PIPE ViPipe, ISP_CTRL_PROC_WRITE_S *pstProc)
{
    ISP_CTRL_PROC_WRITE_S stProcTmp;
    ISP_BLACKLEVEL_S      *pstBlackLevel = HI_NULL;

    BLACKLEVEL_GET_CTX(ViPipe, pstBlackLevel);

    if ((HI_NULL == pstProc->pcProcBuff) || (0 == pstProc->u32BuffLen))
    {
        return HI_FAILURE;
    }

    stProcTmp.pcProcBuff = pstProc->pcProcBuff;
    stProcTmp.u32BuffLen = pstProc->u32BuffLen;

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "-----Black Level Actual INFO--------------------------------------------------------------\n");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s"     "%16s"     "%16s"     "%16s\n",
                    "BlcR", "BlcGr", "BlcGb", "BlcB");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16u"     "%16u"     "%16u"     "%16u\n",
                    pstBlackLevel->au16ActualBlackLevel[0],
                    pstBlackLevel->au16ActualBlackLevel[1],
                    pstBlackLevel->au16ActualBlackLevel[2],
                    pstBlackLevel->au16ActualBlackLevel[3]
                   );

    pstProc->u32WriteLen += 1;

    return HI_SUCCESS;
}

HI_S32 CheckWDRState(VI_PIPE ViPipe, ISP_CTX_S *pstIspCtx, ISP_BLACKLEVEL_S *pstBlackLevel)
{
    HI_S32 s32WDRStateChange;
    HI_U8 bWDREn;

    bWDREn = hi_ext_system_wdr_en_read(ViPipe);

    if (IS_2to1_WDR_MODE(pstIspCtx->u8SnsWDRMode) || IS_3to1_WDR_MODE(pstIspCtx->u8SnsWDRMode) || IS_4to1_WDR_MODE(pstIspCtx->u8SnsWDRMode))
    {
        s32WDRStateChange = (pstBlackLevel->u8WDRModeState == bWDREn) ? HI_FALSE : HI_TRUE;

    }
    else
    {
        s32WDRStateChange = HI_FALSE;
    }

    pstBlackLevel->u8WDRModeState = bWDREn;


    return s32WDRStateChange;

}

static HI_S32 ISP_BlcWdrModeSet(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    HI_U8 i;
    ISP_CTX_S        *pstIspCtx     = HI_NULL;
    ISP_BLACKLEVEL_S *pstBlackLevel = HI_NULL;
    ISP_REG_CFG_S    *pstRegCfg     = (ISP_REG_CFG_S *)pRegCfg;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    BLACKLEVEL_GET_CTX(ViPipe, pstBlackLevel);

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        BE_BlcDynaRegs(ViPipe, pstIspCtx->u8SnsWDRMode, &pstRegCfg->stAlgRegCfg[i].stBeBlcCfg, pstBlackLevel->au16BlackLevel);
    }

    pstRegCfg->unKey.bit1BeBlcCfg = 1;
    pstRegCfg->unKey.bit1FeBlcCfg = 1;
    pstRegCfg->stAlgRegCfg[0].stFeBlcCfg.bReshDyna   = HI_TRUE;
    pstRegCfg->stAlgRegCfg[0].stFeBlcCfg.bReshStatic = HI_TRUE;

    return HI_SUCCESS;
}

HI_S32 ISP_BlcInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    BlcInitialize(ViPipe);
    BlcExtRegsInitialize(ViPipe);
    BlcRegsInitialize(ViPipe, pstRegCfg);

    return HI_SUCCESS;
}

HI_S32 ISP_BlcRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo, HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_U8 i;
    HI_S16 s16BlkOffset = 0;
    ISP_CTX_S *pstIspCtx  = HI_NULL;
    ISP_CMOS_BLACK_LEVEL_S *pstSnsBlackLevel = HI_NULL;
    ISP_BLACKLEVEL_S       *pstBlackLevel    = HI_NULL;
    ISP_REG_CFG_S *pstRegCfg  = (ISP_REG_CFG_S *)pRegCfg;
    HI_S32 s32WDRStateChange;

    BLACKLEVEL_GET_CTX(ViPipe, pstBlackLevel);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    s32WDRStateChange = CheckWDRState(ViPipe, pstIspCtx, pstBlackLevel);

    if (0 == hi_ext_system_dpc_static_defect_type_read(ViPipe)) //hot pixel
    {
        if (pstIspCtx->stLinkage.bDefectPixel)
        {
            if (HI_FALSE == pstBlackLevel->bPreDefectPixel)
            {
                for (i = 0; i < pstRegCfg->u8CfgNum; i++)
                {
                    pstRegCfg->stAlgRegCfg[i].stBeBlcCfg.stWbBlc.stStaticRegCfg.bBlcIn = HI_TRUE;
                    pstRegCfg->stAlgRegCfg[i].stBeBlcCfg.bReshStatic                   = HI_TRUE;
                }
            }
        }
        else if (pstBlackLevel->bPreDefectPixel)
        {
            for (i = 0; i < pstRegCfg->u8CfgNum; i++)
            {
                pstRegCfg->stAlgRegCfg[i].stBeBlcCfg.stWbBlc.stStaticRegCfg.bBlcIn = HI_FALSE;
                pstRegCfg->stAlgRegCfg[i].stBeBlcCfg.bReshStatic                   = HI_TRUE;
            }
        }

        pstBlackLevel->bPreDefectPixel = pstIspCtx->stLinkage.bDefectPixel;
    }

    BlcReadExtregs(ViPipe);

    pstRegCfg->unKey.bit1FeBlcCfg = 1;
    pstRegCfg->unKey.bit1BeBlcCfg = 1;

    /* some sensors's blacklevel is changed with iso. */
    ISP_SensorGetBlc(ViPipe, &pstSnsBlackLevel);

    /* ISP Multi-pipe blackLevel different configs */
    if (HI_TRUE == pstIspCtx->stStitchAttr.bStitchEnable)
    {
        for (i = 0; i < ISP_BAYER_CHN_NUM; i++)
        {
            s16BlkOffset = (HI_S16)hi_ext_system_isp_pipe_diff_offset_read(ViPipe, i);
            pstSnsBlackLevel->au16BlackLevel[i] = CLIP3((HI_S16)pstSnsBlackLevel->au16BlackLevel[i] + s16BlkOffset, 0, 0xFFF);
            pstBlackLevel->au16BlackLevel[i] = CLIP3((HI_S16)pstBlackLevel->au16BlackLevel[i] + s16BlkOffset, 0, 0xFFF);
        }
    }

    if (HI_TRUE == pstSnsBlackLevel->bUpdate || HI_TRUE == s32WDRStateChange)
    {
        ISP_SensorUpdateBlc(ViPipe);
        memcpy(&pstBlackLevel->au16BlackLevel[0], &pstSnsBlackLevel->au16BlackLevel[0], 4 * sizeof(HI_U16));

        FE_BlcDynaRegs(&pstRegCfg->stAlgRegCfg[0].stFeBlcCfg, pstBlackLevel->au16BlackLevel);

        for (i = 0; i < pstRegCfg->u8CfgNum; i++)
        {
            BE_BlcDynaRegs(ViPipe, pstIspCtx->u8SnsWDRMode, &pstRegCfg->stAlgRegCfg[i].stBeBlcCfg, pstBlackLevel->au16BlackLevel);
        }

        BlcWriteActualValue(ViPipe, pstBlackLevel->au16BlackLevel);

        return HI_SUCCESS;
    }

    /*sensors's blacklevel is changed by mpi. */
    if (HI_TRUE == pstBlackLevel->u8BlackLevelChange)
    {
        FE_BlcDynaRegs(&pstRegCfg->stAlgRegCfg[0].stFeBlcCfg, pstBlackLevel->au16BlackLevel);
        for (i = 0; i < pstRegCfg->u8CfgNum; i++)
        {
            BE_BlcDynaRegs(ViPipe, pstIspCtx->u8SnsWDRMode, &pstRegCfg->stAlgRegCfg[i].stBeBlcCfg, pstBlackLevel->au16BlackLevel);
        }

        BlcWriteActualValue(ViPipe, pstBlackLevel->au16BlackLevel);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_BlcCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);

    switch (u32Cmd)
    {
        case ISP_WDR_MODE_SET :
            ISP_BlcWdrModeSet(ViPipe, (HI_VOID *)&pRegCfg->stRegCfg);
            break;
        case ISP_PROC_WRITE :
            ISP_BlcProcWrite(ViPipe, (ISP_CTRL_PROC_WRITE_S *)pValue);
            break;
        default :
            break;
    }
    return HI_SUCCESS;
}

HI_S32 ISP_BlcExit(VI_PIPE ViPipe)
{
    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterBlc(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_BLC;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_BlcInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_BlcRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_BlcCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_BlcExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


