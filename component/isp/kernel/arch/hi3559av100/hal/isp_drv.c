
/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_drv.c

  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/11/07
  Description   :
  History       :
  1.Date        : 2013/11/07
    Author      :
    Modification: Created file

******************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include "isp_drv_defines.h"
#include "isp_reg_define.h"
#include "isp_stt_define.h"
#include "hi_common.h"
#include "hi_osal.h"
#include "hi_math.h"
#include "mkp_isp.h"
#include "isp_drv.h"
#include "isp.h"
#include "mm_ext.h"


extern osal_spinlock_t g_stIspLock[ISP_MAX_PIPE_NUM];
extern HI_BOOL         g_IntBothalf;
extern HI_BOOL         g_UseBothalf;

void  *reg_vicap_base_va = HI_NULL;
void  *reg_ispfe_base_va[ISP_MAX_PIPE_NUM]    = {HI_NULL};
void  *reg_ispbe_base_va[ISP_MAX_BE_NUM]      = {HI_NULL};
void  *reg_viproc_base_va[ISP_MAX_BE_NUM]     = {HI_NULL};

HI_U32 g_DrcCurLumaLut[HI_ISP_DRC_SHP_LOG_CONFIG_NUM][HI_ISP_DRC_EXP_COMP_SAMPLE_NUM - 1] =
{
    {1,     1,      5,      31,     180,    1023,   32767},
    {1,     3,      8,      52,     277,    1446,   38966},
    {2,     5,      15,     87,     427,    2044,   46337},
    {4,     9,      27,     144,    656,    2888,   55101},
    {7,     16,     48,     240,    1008,   4080,   65521},
    {12,    29,     85,     399,    1547,   5761,   77906},
    {23,    53,     151,    660,    2372,   8128,   92622},
    {42,    97,     267,    1090,   3628,   11458,  110100},
    {76,    175,    468,    1792,   5537,   16130,  130840},
    {138,   313,    816,    2933,   8423,   22664,  155417},
    {258,   555,    1412,   4770,   12758,  31760,  184476},
    {441,   977,    2420,   7699,   19215,  44338,  218711},
    {776,   1698,   4100,   12304,  28720,  61568,  258816},
    {1344,  2907,   6847,   19416,  42491,  84851,  305376},
    {2283,  4884,   11224,  30137,  62006,  115708, 358680},
    {3783,  8004,   17962,  45770,  88821,  155470, 418391},
};

static HI_U16 Sqrt32(HI_U32 u32Arg)
{
    HI_U32 u32Mask = (HI_U32)1 << 15;
    HI_U16 u16Res = 0;
    HI_U32 i = 0;

    for (i = 0; i < 16; i++)
    {
        if ((u16Res + (u32Mask >> i)) * (u16Res + (u32Mask >> i)) <= u32Arg)
        {
            u16Res = u16Res + (u32Mask >> i);
        }
    }

    /* rounding */
    if (u16Res * u16Res + u16Res < u32Arg)
    {
        ++u16Res;
    }

    return u16Res;
}


/*---------------------------------------------- isp drv FHY regs define -----------------------------------------------------*/
/*----------------------------------------------------------------------------------------------------------------------------*/
HI_S32 ISP_DRV_SetInputSel(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 *pu32InputSel)
{
    U_ISP_BE_INPUT_MUX o_isp_be_input_mux;
    ISP_CHECK_POINTER(pstIspBeRegs);
    ISP_CHECK_POINTER(pu32InputSel);

    o_isp_be_input_mux.u32 = pstIspBeRegs->ISP_BE_INPUT_MUX.u32;
    o_isp_be_input_mux.bits.isp_input0_sel = pu32InputSel[0];
    o_isp_be_input_mux.bits.isp_input1_sel = pu32InputSel[1];
    o_isp_be_input_mux.bits.isp_input2_sel = pu32InputSel[2];
    o_isp_be_input_mux.bits.isp_input3_sel = pu32InputSel[3];
    o_isp_be_input_mux.bits.isp_input4_sel = 0;
    pstIspBeRegs->ISP_BE_INPUT_MUX.u32 = o_isp_be_input_mux.u32;
    return 1;
}
static __inline HI_S32 ISP_DRV_SetIspDgain(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 u32IspDgain)
{
    U_ISP_DG_GAIN1 o_isp_dg_gain1;
    U_ISP_DG_GAIN2 o_isp_dg_gain2;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_dg_gain1.u32 = pstIspBeRegs->ISP_DG_GAIN1.u32;
    o_isp_dg_gain1.bits.isp_dg_rgain  = u32IspDgain;
    o_isp_dg_gain1.bits.isp_dg_grgain = u32IspDgain;
    pstIspBeRegs->ISP_DG_GAIN1.u32 = o_isp_dg_gain1.u32;

    o_isp_dg_gain2.u32 = pstIspBeRegs->ISP_DG_GAIN2.u32;
    o_isp_dg_gain2.bits.isp_dg_bgain  = u32IspDgain;
    o_isp_dg_gain2.bits.isp_dg_gbgain = u32IspDgain;
    pstIspBeRegs->ISP_DG_GAIN2.u32 = o_isp_dg_gain2.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetIsp4Dgain0(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 u32Isp4Dgain0)
{
    U_ISP_4DG_0_GAIN1 o_isp_4dg_0_gain1;
    U_ISP_4DG_0_GAIN2 o_isp_4dg_0_gain2;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_4dg_0_gain1.u32 = pstIspBeRegs->ISP_4DG_0_GAIN1.u32;
    o_isp_4dg_0_gain1.bits.isp_4dg0_rgain  = u32Isp4Dgain0;
    o_isp_4dg_0_gain1.bits.isp_4dg0_grgain = u32Isp4Dgain0;
    pstIspBeRegs->ISP_4DG_0_GAIN1.u32 = o_isp_4dg_0_gain1.u32;

    o_isp_4dg_0_gain2.u32 = pstIspBeRegs->ISP_4DG_0_GAIN2.u32;
    o_isp_4dg_0_gain2.bits.isp_4dg0_bgain  = u32Isp4Dgain0;
    o_isp_4dg_0_gain2.bits.isp_4dg0_gbgain = u32Isp4Dgain0;
    pstIspBeRegs->ISP_4DG_0_GAIN2.u32 = o_isp_4dg_0_gain2.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetIsp4Dgain1(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 u32Isp4Dgain1)
{
    U_ISP_4DG_1_GAIN1 o_isp_4dg_1_gain1;
    U_ISP_4DG_1_GAIN2 o_isp_4dg_1_gain2;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_4dg_1_gain1.u32 = pstIspBeRegs->ISP_4DG_1_GAIN1.u32;
    o_isp_4dg_1_gain1.bits.isp_4dg1_rgain  = u32Isp4Dgain1;
    o_isp_4dg_1_gain1.bits.isp_4dg1_grgain = u32Isp4Dgain1;
    pstIspBeRegs->ISP_4DG_1_GAIN1.u32 = o_isp_4dg_1_gain1.u32;

    o_isp_4dg_1_gain2.u32 = pstIspBeRegs->ISP_4DG_1_GAIN2.u32;
    o_isp_4dg_1_gain2.bits.isp_4dg1_bgain  = u32Isp4Dgain1;
    o_isp_4dg_1_gain2.bits.isp_4dg1_gbgain = u32Isp4Dgain1;
    pstIspBeRegs->ISP_4DG_1_GAIN2.u32 = o_isp_4dg_1_gain2.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetIsp4Dgain2(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 u32Isp4Dgain2)
{
    U_ISP_4DG_2_GAIN1 o_isp_4dg_2_gain1;
    U_ISP_4DG_2_GAIN2 o_isp_4dg_2_gain2;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_4dg_2_gain1.u32 = pstIspBeRegs->ISP_4DG_2_GAIN1.u32;
    o_isp_4dg_2_gain1.bits.isp_4dg2_rgain  = u32Isp4Dgain2;
    o_isp_4dg_2_gain1.bits.isp_4dg2_grgain = u32Isp4Dgain2;
    pstIspBeRegs->ISP_4DG_2_GAIN1.u32 = o_isp_4dg_2_gain1.u32;

    o_isp_4dg_2_gain2.u32 = pstIspBeRegs->ISP_4DG_2_GAIN2.u32;
    o_isp_4dg_2_gain2.bits.isp_4dg2_bgain  = u32Isp4Dgain2;
    o_isp_4dg_2_gain2.bits.isp_4dg2_gbgain = u32Isp4Dgain2;
    pstIspBeRegs->ISP_4DG_2_GAIN2.u32 = o_isp_4dg_2_gain2.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetIsp4Dgain3(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 u32Isp4Dgain3)
{
    U_ISP_4DG_3_GAIN1 o_isp_4dg_3_gain1;
    U_ISP_4DG_3_GAIN2 o_isp_4dg_3_gain2;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_4dg_3_gain1.u32 = pstIspBeRegs->ISP_4DG_3_GAIN1.u32;
    o_isp_4dg_3_gain1.bits.isp_4dg3_rgain  = u32Isp4Dgain3;
    o_isp_4dg_3_gain1.bits.isp_4dg3_grgain = u32Isp4Dgain3;
    pstIspBeRegs->ISP_4DG_3_GAIN1.u32 = o_isp_4dg_3_gain1.u32;

    o_isp_4dg_3_gain2.u32 = pstIspBeRegs->ISP_4DG_3_GAIN2.u32;
    o_isp_4dg_3_gain2.bits.isp_4dg3_bgain  = u32Isp4Dgain3;
    o_isp_4dg_3_gain2.bits.isp_4dg3_gbgain = u32Isp4Dgain3;
    pstIspBeRegs->ISP_4DG_3_GAIN2.u32 = o_isp_4dg_3_gain2.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetWdrExporratio0(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_wdr_exporatio0)
{
    U_ISP_WDR_EXPORRATIO0 o_isp_wdr_exporatio0;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_wdr_exporatio0.u32 = pstIspBeRegs->ISP_WDR_EXPORRATIO0.u32;
    o_isp_wdr_exporatio0.bits.isp_wdr_exporratio0 = uisp_wdr_exporatio0;
    pstIspBeRegs->ISP_WDR_EXPORRATIO0.u32 = o_isp_wdr_exporatio0.u32;
    return 1;
}

static __inline HI_S32 ISP_DRV_SetWdrExporratio1(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_wdr_exporatio1)
{
    U_ISP_WDR_EXPORRATIO0 o_isp_wdr_exporatio0;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_wdr_exporatio0.u32 = pstIspBeRegs->ISP_WDR_EXPORRATIO0.u32;
    o_isp_wdr_exporatio0.bits.isp_wdr_exporratio1 = uisp_wdr_exporatio1;
    pstIspBeRegs->ISP_WDR_EXPORRATIO0.u32 = o_isp_wdr_exporatio0.u32;
    return 1;
}

static __inline HI_S32 ISP_DRV_SetWdrExporratio2(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_wdr_exporatio2)
{
    U_ISP_WDR_EXPORRATIO1 o_isp_wdr_exporatio1;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_wdr_exporatio1.u32 = pstIspBeRegs->ISP_WDR_EXPORRATIO1.u32;
    o_isp_wdr_exporatio1.bits.isp_wdr_exporratio2 = uisp_wdr_exporatio2;
    pstIspBeRegs->ISP_WDR_EXPORRATIO1.u32 = o_isp_wdr_exporatio1.u32;
    return 1;
}

static __inline HI_S32 ISP_DRV_SetWdrExpoValue0(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_wdr_expovalue0)
{
    U_ISP_WDR_EXPOVALUE0 o_isp_wdr_expovalue0;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_wdr_expovalue0.u32 = pstIspBeRegs->ISP_WDR_EXPOVALUE0.u32;
    o_isp_wdr_expovalue0.bits.isp_wdr_expovalue0 = uisp_wdr_expovalue0;
    pstIspBeRegs->ISP_WDR_EXPOVALUE0.u32 = o_isp_wdr_expovalue0.u32;

    return 1;
}
static __inline HI_S32 ISP_DRV_SetWdrExpoValue1(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_wdr_expovalue1)
{
    U_ISP_WDR_EXPOVALUE0 o_isp_wdr_expovalue0;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_wdr_expovalue0.u32 = pstIspBeRegs->ISP_WDR_EXPOVALUE0.u32;
    o_isp_wdr_expovalue0.bits.isp_wdr_expovalue1 = uisp_wdr_expovalue1;
    pstIspBeRegs->ISP_WDR_EXPOVALUE0.u32 = o_isp_wdr_expovalue0.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetWdrExpoValue2(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_wdr_expovalue2)
{
    U_ISP_WDR_EXPOVALUE1 o_isp_wdr_expovalue1;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_wdr_expovalue1.u32 = pstIspBeRegs->ISP_WDR_EXPOVALUE1.u32;
    o_isp_wdr_expovalue1.bits.isp_wdr_expovalue2 = uisp_wdr_expovalue2;
    pstIspBeRegs->ISP_WDR_EXPOVALUE1.u32 = o_isp_wdr_expovalue1.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetWdrExpoValue3(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_wdr_expovalue3)
{
    U_ISP_WDR_EXPOVALUE1 o_isp_wdr_expovalue1;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_wdr_expovalue1.u32 = pstIspBeRegs->ISP_WDR_EXPOVALUE1.u32;
    o_isp_wdr_expovalue1.bits.isp_wdr_expovalue3 = uisp_wdr_expovalue3;
    pstIspBeRegs->ISP_WDR_EXPOVALUE1.u32 = o_isp_wdr_expovalue1.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetFlickExporatio0(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_flick_exporatio0)
{
    U_ISP_FLICK_EXPORATIO0 o_isp_flick_exporatio0;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_flick_exporatio0.u32 = pstIspBeRegs->ISP_FLICK_EXPORATIO0.u32;
    o_isp_flick_exporatio0.bits.isp_flick_exporatio0 = uisp_flick_exporatio0;
    pstIspBeRegs->ISP_FLICK_EXPORATIO0.u32 = o_isp_flick_exporatio0.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetFlickExporatio1(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_flick_exporatio1)
{
    U_ISP_FLICK_EXPORATIO0 o_isp_flick_exporatio0;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_flick_exporatio0.u32 = pstIspBeRegs->ISP_FLICK_EXPORATIO0.u32;
    o_isp_flick_exporatio0.bits.isp_flick_exporatio1 = uisp_flick_exporatio1;
    pstIspBeRegs->ISP_FLICK_EXPORATIO0.u32 = o_isp_flick_exporatio0.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetFlickExporatio2(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_flick_exporatio2)
{
    U_ISP_FLICK_EXPORATIO1 o_isp_flick_exporatio1;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_flick_exporatio1.u32 = pstIspBeRegs->ISP_FLICK_EXPORATIO1.u32;
    o_isp_flick_exporatio1.bits.isp_flick_exporatio2 = uisp_flick_exporatio2;
    pstIspBeRegs->ISP_FLICK_EXPORATIO1.u32 = o_isp_flick_exporatio1.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetWdrBlcComp0(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_wdr_blc_comp0)
{
    U_ISP_WDR_BLC_COMP0 o_isp_wdr_blc_comp0;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_wdr_blc_comp0.u32 = pstIspBeRegs->ISP_WDR_BLC_COMP0.u32;
    o_isp_wdr_blc_comp0.bits.isp_wdr_blc_comp0 = uisp_wdr_blc_comp0;
    pstIspBeRegs->ISP_WDR_BLC_COMP0.u32 = o_isp_wdr_blc_comp0.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetWdrBlcComp1(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_wdr_blc_comp1)
{
    U_ISP_WDR_BLC_COMP1 o_isp_wdr_blc_comp1;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_wdr_blc_comp1.u32 = pstIspBeRegs->ISP_WDR_BLC_COMP1.u32;
    o_isp_wdr_blc_comp1.bits.isp_wdr_blc_comp1 = uisp_wdr_blc_comp1;
    pstIspBeRegs->ISP_WDR_BLC_COMP1.u32 = o_isp_wdr_blc_comp1.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetWdrBlcComp2(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_wdr_blc_comp2)
{
    U_ISP_WDR_BLC_COMP2 o_isp_wdr_blc_comp2;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_wdr_blc_comp2.u32 = pstIspBeRegs->ISP_WDR_BLC_COMP2.u32;
    o_isp_wdr_blc_comp2.bits.isp_wdr_blc_comp2 = uisp_wdr_blc_comp2;
    pstIspBeRegs->ISP_WDR_BLC_COMP2.u32 = o_isp_wdr_blc_comp2.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetWdrMaxRatio(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_wdr_maxratio)
{
    U_ISP_WDR_MAXRATIO o_isp_wdr_maxratio;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_wdr_maxratio.u32 = pstIspBeRegs->ISP_WDR_MAXRATIO.u32;
    o_isp_wdr_maxratio.bits.isp_wdr_maxratio = uisp_wdr_maxratio;
    pstIspBeRegs->ISP_WDR_MAXRATIO.u32 = o_isp_wdr_maxratio.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetWdrLongThr(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_wdr_long_thr)
{
    U_ISP_WDR_WGTIDX_THR o_isp_wdr_wgtidx_thr;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_wdr_wgtidx_thr.u32 = pstIspBeRegs->ISP_WDR_WGTIDX_THR.u32;
    o_isp_wdr_wgtidx_thr.bits.isp_wdr_long_thr  = uisp_wdr_long_thr;
    pstIspBeRegs->ISP_WDR_WGTIDX_THR.u32 = o_isp_wdr_wgtidx_thr.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetWdrShortThr(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_wdr_short_thr)
{
    U_ISP_WDR_WGTIDX_THR o_isp_wdr_wgtidx_thr;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_wdr_wgtidx_thr.u32 = pstIspBeRegs->ISP_WDR_WGTIDX_THR.u32;
    o_isp_wdr_wgtidx_thr.bits.isp_wdr_short_thr  = uisp_wdr_short_thr;
    pstIspBeRegs->ISP_WDR_WGTIDX_THR.u32 = o_isp_wdr_wgtidx_thr.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetWdrMdtEn(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_wdr_mdt_en)
{
    U_ISP_WDR_CTRL o_isp_wdr_ctrl;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_wdr_ctrl.u32 = pstIspBeRegs->ISP_WDR_CTRL.u32;
    o_isp_wdr_ctrl.bits.isp_wdr_mdt_en = uisp_wdr_mdt_en;
    pstIspBeRegs->ISP_WDR_CTRL.u32     = o_isp_wdr_ctrl.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetLdciStatEvratio(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_ldci_stat_evratio)
{
    U_ISP_LDCI_STAT_EVRATIO o_isp_ldci_stat_evratio;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_ldci_stat_evratio.u32 = pstIspBeRegs->ISP_LDCI_STAT_EVRATIO.u32;
    o_isp_ldci_stat_evratio.bits.isp_ldci_stat_evratio = uisp_ldci_stat_evratio;
    pstIspBeRegs->ISP_LDCI_STAT_EVRATIO.u32 = o_isp_ldci_stat_evratio.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetDrcPrevLuma0(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_drc_prev_luma_0)
{
    U_ISP_DRC_PREV_LUMA_0 o_isp_drc_prev_luma_0;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_drc_prev_luma_0.u32 = pstIspBeRegs->ISP_DRC_PREV_LUMA_0.u32;
    o_isp_drc_prev_luma_0.bits.isp_drc_prev_luma_0 = uisp_drc_prev_luma_0;
    pstIspBeRegs->ISP_DRC_PREV_LUMA_0.u32 = o_isp_drc_prev_luma_0.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetDrcPrevLuma1(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_drc_prev_luma_1)
{
    U_ISP_DRC_PREV_LUMA_1 o_isp_drc_prev_luma_1;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_drc_prev_luma_1.u32 = pstIspBeRegs->ISP_DRC_PREV_LUMA_1.u32;
    o_isp_drc_prev_luma_1.bits.isp_drc_prev_luma_1 = uisp_drc_prev_luma_1;
    pstIspBeRegs->ISP_DRC_PREV_LUMA_1.u32 = o_isp_drc_prev_luma_1.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetDrcPrevLuma2(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_drc_prev_luma_2)
{
    U_ISP_DRC_PREV_LUMA_2 o_isp_drc_prev_luma_2;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_drc_prev_luma_2.u32 = pstIspBeRegs->ISP_DRC_PREV_LUMA_2.u32;
    o_isp_drc_prev_luma_2.bits.isp_drc_prev_luma_2 = uisp_drc_prev_luma_2;
    pstIspBeRegs->ISP_DRC_PREV_LUMA_2.u32 = o_isp_drc_prev_luma_2.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetDrcPrevLuma3(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_drc_prev_luma_3)
{
    U_ISP_DRC_PREV_LUMA_3 o_isp_drc_prev_luma_3;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_drc_prev_luma_3.u32 = pstIspBeRegs->ISP_DRC_PREV_LUMA_3.u32;
    o_isp_drc_prev_luma_3.bits.isp_drc_prev_luma_3 = uisp_drc_prev_luma_3;
    pstIspBeRegs->ISP_DRC_PREV_LUMA_3.u32 = o_isp_drc_prev_luma_3.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetDrcPrevLuma4(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_drc_prev_luma_4)
{
    U_ISP_DRC_PREV_LUMA_4 o_isp_drc_prev_luma_4;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_drc_prev_luma_4.u32 = pstIspBeRegs->ISP_DRC_PREV_LUMA_4.u32;
    o_isp_drc_prev_luma_4.bits.isp_drc_prev_luma_4 = uisp_drc_prev_luma_4;
    pstIspBeRegs->ISP_DRC_PREV_LUMA_4.u32 = o_isp_drc_prev_luma_4.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetDrcPrevLuma5(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_drc_prev_luma_5)
{
    U_ISP_DRC_PREV_LUMA_5 o_isp_drc_prev_luma_5;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_drc_prev_luma_5.u32 = pstIspBeRegs->ISP_DRC_PREV_LUMA_5.u32;
    o_isp_drc_prev_luma_5.bits.isp_drc_prev_luma_5 = uisp_drc_prev_luma_5;
    pstIspBeRegs->ISP_DRC_PREV_LUMA_5.u32 = o_isp_drc_prev_luma_5.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetDrcPrevLuma6(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_drc_prev_luma_6)
{
    U_ISP_DRC_PREV_LUMA_6 o_isp_drc_prev_luma_6;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_drc_prev_luma_6.u32 = pstIspBeRegs->ISP_DRC_PREV_LUMA_6.u32;
    o_isp_drc_prev_luma_6.bits.isp_drc_prev_luma_6 = uisp_drc_prev_luma_6;
    pstIspBeRegs->ISP_DRC_PREV_LUMA_6.u32 = o_isp_drc_prev_luma_6.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetDrcPrevLuma7(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_drc_prev_luma_7)
{
    U_ISP_DRC_PREV_LUMA_7 o_isp_drc_prev_luma_7;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_drc_prev_luma_7.u32 = pstIspBeRegs->ISP_DRC_PREV_LUMA_7.u32;
    o_isp_drc_prev_luma_7.bits.isp_drc_prev_luma_7 = uisp_drc_prev_luma_7;
    pstIspBeRegs->ISP_DRC_PREV_LUMA_7.u32 = o_isp_drc_prev_luma_7.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetDrcShpCfg(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_drc_shp_log, HI_U32 uisp_drc_shp_exp)
{
    U_ISP_DRC_SHP_CFG o_isp_drc_shp_cfg;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_drc_shp_cfg.u32 = pstIspBeRegs->ISP_DRC_SHP_CFG.u32;
    o_isp_drc_shp_cfg.bits.isp_drc_shp_log = uisp_drc_shp_log;
    o_isp_drc_shp_cfg.bits.isp_drc_shp_exp = uisp_drc_shp_exp;
    pstIspBeRegs->ISP_DRC_SHP_CFG.u32 = o_isp_drc_shp_cfg.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetDrcDivDenomLog(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_drc_div_denom_log)
{
    U_ISP_DRC_DIV_DENOM_LOG o_isp_drc_div_denom_log;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_drc_div_denom_log.u32 = pstIspBeRegs->ISP_DRC_DIV_DENOM_LOG.u32;
    o_isp_drc_div_denom_log.bits.isp_drc_div_denom_log = uisp_drc_div_denom_log;
    pstIspBeRegs->ISP_DRC_DIV_DENOM_LOG.u32 = o_isp_drc_div_denom_log.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetDrcDenomExp(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_drc_denom_exp)
{
    U_ISP_DRC_DENOM_EXP o_isp_drc_denom_exp;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_drc_denom_exp.u32 = pstIspBeRegs->ISP_DRC_DENOM_EXP.u32;
    o_isp_drc_denom_exp.bits.isp_drc_denom_exp = uisp_drc_denom_exp;
    pstIspBeRegs->ISP_DRC_DENOM_EXP.u32 = o_isp_drc_denom_exp.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetDrcLutMixCtrl(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_drc_lut_mix_ctrl)
{
    U_ISP_DRC_LUT_MIX_CRTL o_isp_drc_lut_mix_crtl;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_drc_lut_mix_crtl.u32 = pstIspBeRegs->ISP_DRC_LUT_MIX_CRTL.u32;
    o_isp_drc_lut_mix_crtl.bits.isp_drc_lut_mix_ctrl = uisp_drc_lut_mix_ctrl;
    pstIspBeRegs->ISP_DRC_LUT_MIX_CRTL.u32 = o_isp_drc_lut_mix_crtl.u32;

    return 1;
}

static __inline HI_S32 ISP_DRV_SetDrcStrength(S_ISPBE_REGS_TYPE *pstIspBeRegs, HI_U32 uisp_drc_strength)
{
    U_ISP_DRC_STRENGTH o_isp_drc_strength;
    ISP_CHECK_POINTER(pstIspBeRegs);

    o_isp_drc_strength.u32 = pstIspBeRegs->ISP_DRC_STRENGTH.u32;
    o_isp_drc_strength.bits.isp_drc_strength = uisp_drc_strength;
    pstIspBeRegs->ISP_DRC_STRENGTH.u32 = o_isp_drc_strength.u32;

    return 1;
}



static __inline HI_S32 ISP_DRV_SetIspDISDataType(S_ISPBE_REGS_TYPE *pstNode, ISP_DIS_DATA_TYPE_E enDataType)
{
    U_ISP_DIS_RAW_LUMA unDisRawLuma;

    unDisRawLuma.u32 = pstNode->ISP_DIS_RAW_LUMA.u32;

    unDisRawLuma.bits.isp_dis_raw_luma = enDataType;
    pstNode->ISP_DIS_RAW_LUMA.u32 = unDisRawLuma.u32;
    return 1;
}

static __inline HI_S32 ISP_DRV_SetIspDISGammaEn(S_ISPBE_REGS_TYPE *pstNode, HI_BOOL bEn)
{
    U_ISP_DIS_GAMMA_EN unDisGammaEn;

    unDisGammaEn.u32 = pstNode->ISP_DIS_GAMMA_EN.u32;
    unDisGammaEn.bits.isp_dis_gamma_en = bEn;
    pstNode->ISP_DIS_GAMMA_EN.u32 = unDisGammaEn.u32;
    return 1;
}

/*----------------------------------------------------------------------------------------------------------------------------*/
HI_S32 ISP_DRV_BeRemap(void)
{
    HI_U32 IspBePhyPipe = 0;

    for (IspBePhyPipe = 0; IspBePhyPipe < ISP_MAX_BE_NUM; IspBePhyPipe++)
    {
        reg_ispbe_base_va[IspBePhyPipe] = (void *)osal_ioremap(ISP_BE_REG_BASE(IspBePhyPipe), (HI_U32)VI_ISP_BE_REG_SIZE);

        if (HI_NULL == reg_ispbe_base_va[IspBePhyPipe])
        {
            osal_printk( "Remap ISP BE[%d] failed!\n", IspBePhyPipe);
            return HI_FAILURE;
        }

        IO_RW_BE_ADDRESS(IspBePhyPipe, ISP_INT_BE_MASK) = (0x0);

        reg_viproc_base_va[IspBePhyPipe] = (void *)osal_ioremap(ISP_VIPROC_REG_BASE(IspBePhyPipe), (HI_U32)VIPROC_REG_SIZE);

        if (HI_NULL == reg_viproc_base_va[IspBePhyPipe])
        {
            osal_printk( "Remap isp viproc[%d] failed!\n", IspBePhyPipe);
            return HI_FAILURE;
        }

    }

    return HI_SUCCESS;
}

HI_VOID ISP_DRV_BeUnmap(void)
{
    HI_U32 IspBePhyPipe = 0;

    for (IspBePhyPipe = 0; IspBePhyPipe < ISP_MAX_BE_NUM; IspBePhyPipe++)
    {
        if (reg_ispbe_base_va[IspBePhyPipe] != NULL)
        {
            osal_iounmap(reg_ispbe_base_va[IspBePhyPipe]);
            reg_ispbe_base_va[IspBePhyPipe] = NULL;
        }

        if (reg_viproc_base_va[IspBePhyPipe] != NULL)
        {
            osal_iounmap(reg_viproc_base_va[IspBePhyPipe]);
            reg_viproc_base_va[IspBePhyPipe] = NULL;
        }

    }
}

HI_S32 ISP_DRV_VicapRemap(void)
{
    reg_vicap_base_va = (void *)osal_ioremap(CAP_REG_BASE, (HI_U32)CAP_REG_SIZE_ALIGN);

    if (HI_NULL == reg_vicap_base_va)
    {
        osal_printk( "Remap ISP[%d] PT failed!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_VOID ISP_DRV_VicapUnmap(void)
{
    if (reg_vicap_base_va != NULL)
    {
        osal_iounmap(reg_vicap_base_va);
        reg_vicap_base_va = NULL;
    }
}

HI_S32 ISP_DRV_FeRemap(void)
{
    HI_U8   i;
    VI_PIPE ViPipe;

    for (ViPipe = 0; ViPipe < ISP_MAX_PIPE_NUM; ViPipe++)
    {
        osal_memset(&g_astIspDrvCtx[ViPipe], 0, sizeof(ISP_DRV_CTX_S));
        osal_snprintf(g_astIspDrvCtx[ViPipe].acName, sizeof(g_astIspDrvCtx[ViPipe].acName), "ISP[%d]", ViPipe);

        reg_ispfe_base_va[ViPipe] = (void *)osal_ioremap(ISP_FE_REG_BASE(ViPipe), (HI_U32)VI_ISP_FE_REG_SIZE);

        if (HI_NULL == reg_ispfe_base_va[ViPipe])
        {
            osal_printk( "Remap ISP[%d] FE failed!\n", ViPipe);
            return HI_FAILURE;
        }

        /* enable port int & isp int */
        IO_RW_PT_ADDRESS(VICAP_HD_MASK) |= VICAP_INT_MASK_PT(ViPipe);
        IO_RW_PT_ADDRESS(VICAP_HD_MASK) |= VICAP_INT_MASK_ISP(ViPipe);

        IO_RW_FE_ADDRESS(ViPipe, ISP_INT_FE_MASK) = (0x0);

        g_astIspDrvCtx[ViPipe].stWorkMode.enIspRunningMode      = ISP_MODE_RUNNING_OFFLINE;
        g_astIspDrvCtx[ViPipe].stWorkMode.u8BlockNum            = 1;

        g_astIspDrvCtx[ViPipe].stStitchAttr.bStitchEnable = HI_FALSE;
        g_astIspDrvCtx[ViPipe].stStitchAttr.u8StitchPipeNum = 2;
        g_astIspDrvCtx[ViPipe].stStitchAttr.as8StitchBindId[0] = 0;
        g_astIspDrvCtx[ViPipe].stStitchAttr.as8StitchBindId[1] = 3;
        g_astIspDrvCtx[ViPipe].stStitchAttr.as8StitchBindId[2] = -1;
        g_astIspDrvCtx[ViPipe].stStitchAttr.as8StitchBindId[3] = -1;
        g_astIspDrvCtx[ViPipe].stStitchAttr.as8StitchBindId[4] = -1;
        g_astIspDrvCtx[ViPipe].stStitchAttr.as8StitchBindId[5] = -1;
        g_astIspDrvCtx[ViPipe].astChnSelAttr[0].u32ChannelSel = 0;
        g_astIspDrvCtx[ViPipe].astChnSelAttr[1].u32ChannelSel = 0;
        g_astIspDrvCtx[ViPipe].enIspRunningState = ISP_BE_BUF_STATE_INIT;
        g_astIspDrvCtx[ViPipe].enIspExitState = ISP_BE_BUF_EXIT;

        /*snap attr init*/
        g_astIspDrvCtx[ViPipe].stSnapAttr.enSnapType = SNAP_TYPE_NORMAL;
        //g_astIspDrvCtx[ViPipe].stSnapAttr.enSnapPipeMode = ISP_SNAP_NONE;
        g_astIspDrvCtx[ViPipe].stSnapAttr.s32PicturePipeId = -1;
        g_astIspDrvCtx[ViPipe].stSnapAttr.s32PreviewPipeId = -1;
        g_astIspDrvCtx[ViPipe].stSnapAttr.bLoadCCM = HI_TRUE;
        g_astIspDrvCtx[ViPipe].stSnapAttr.stProParam.enOperationMode = OPERATION_MODE_AUTO;

        g_astIspDrvCtx[ViPipe].bKernelRunOnce = HI_FALSE;
        g_astIspDrvCtx[ViPipe].pstUseNode = HI_NULL;

        for (i = 0; i < PRO_MAX_FRAME_NUM; i++)
        {
            g_astIspDrvCtx[ViPipe].stSnapAttr.stProParam.stAutoParam.au16ProExpStep[i] = 256;
            g_astIspDrvCtx[ViPipe].stSnapAttr.stProParam.stManualParam.au32ManExpTime[i] = 10000;
            g_astIspDrvCtx[ViPipe].stSnapAttr.stProParam.stManualParam.au32ManSysgain[i] = 1024;
        }

        g_astIspDrvCtx[ViPipe].stSnapAttr.stProParam.u8ProFrameNum = 3;

        for (i = 0; i < ISP_BAYER_CHN_NUM; i++)
        {
            g_astIspDrvCtx[ViPipe].stIspBeSyncPara.au32IspDgain[i] = 0x100;
        }

        osal_wait_init(&g_astIspDrvCtx[ViPipe].stIspWait);
        osal_wait_init(&g_astIspDrvCtx[ViPipe].stIspWaitVdStart);
        osal_wait_init(&g_astIspDrvCtx[ViPipe].stIspWaitVdEnd);
        osal_wait_init(&g_astIspDrvCtx[ViPipe].stIspWaitVdBeEnd);
        osal_wait_init(&g_astIspDrvCtx[ViPipe].stIspExitWait);
        g_astIspDrvCtx[ViPipe].bEdge = HI_FALSE;
        g_astIspDrvCtx[ViPipe].bVdStart = HI_FALSE;
        g_astIspDrvCtx[ViPipe].bVdEnd = HI_FALSE;
        g_astIspDrvCtx[ViPipe].bVdBeEnd = HI_FALSE;
        g_astIspDrvCtx[ViPipe].bMemInit = HI_FALSE;
        g_astIspDrvCtx[ViPipe].bIspInit = HI_FALSE;
        g_astIspDrvCtx[ViPipe].u32ProTrigFlag = 0;
        osal_sema_init(&g_astIspDrvCtx[ViPipe].stIspSem, 1);
        osal_sema_init(&g_astIspDrvCtx[ViPipe].stIspSemVd, 1);
        osal_sema_init(&g_astIspDrvCtx[ViPipe].stIspSemBeVd, 1);
        osal_sema_init(&g_astIspDrvCtx[ViPipe].stProcSem, 1);
        SyncTaskInit(ViPipe);
    }
    return HI_SUCCESS;
}

HI_VOID ISP_DRV_FeUnmap(void)
{
    VI_PIPE ViPipe;

    for (ViPipe = 0; ViPipe < ISP_MAX_PIPE_NUM; ViPipe++)
    {
        if (reg_ispfe_base_va[ViPipe] != NULL)
        {
            osal_iounmap(reg_ispfe_base_va[ViPipe]);
            reg_ispfe_base_va[ViPipe] = NULL;
        }

        osal_sema_destory(&g_astIspDrvCtx[ViPipe].stIspSem);
        osal_sema_destory(&g_astIspDrvCtx[ViPipe].stIspSemVd);
        osal_sema_destory(&g_astIspDrvCtx[ViPipe].stIspSemBeVd);
        osal_sema_destory(&g_astIspDrvCtx[ViPipe].stProcSem);
    }
}

HI_S32 ISP_DRV_RegConfigChnSel(S_ISPBE_REGS_TYPE *pstIspBeRegs, ISP_DRV_CTX_S *pstDrvCtx)
{
    HI_BOOL bChnSwitchEnable  = HI_FALSE;
    HI_U32  u32IspFEInputMode = 0;
    HI_U32  au32ChnSwitch[4]  = {0};
    HI_U32  u32ChannelSel     = 0;

    ISP_CHECK_POINTER(pstIspBeRegs);
    ISP_CHECK_POINTER(pstDrvCtx);

    u32ChannelSel = pstDrvCtx->astChnSelAttr[0].u32ChannelSel;

    if (IS_FULL_WDR_MODE(pstDrvCtx->stSyncCfg.u8WDRMode))
    {
        switch ( u32ChannelSel & 0x3 )
        {
            case 0:
                au32ChnSwitch[0] = (pstDrvCtx->stSyncCfg.u8VCNumMax - pstDrvCtx->stSyncCfg.u8VCNum) % DIV_0_TO_1(pstDrvCtx->stSyncCfg.u8VCNumMax + 1);
                au32ChnSwitch[1] = (au32ChnSwitch[0] + 1) % DIV_0_TO_1(pstDrvCtx->stSyncCfg.u8VCNumMax + 1);
                au32ChnSwitch[2] = (au32ChnSwitch[0] + 2) % DIV_0_TO_1(pstDrvCtx->stSyncCfg.u8VCNumMax + 1);
                au32ChnSwitch[3] = (au32ChnSwitch[0] + 3) % DIV_0_TO_1(pstDrvCtx->stSyncCfg.u8VCNumMax + 1);
                break;

            case 1:
                au32ChnSwitch[1] = (pstDrvCtx->stSyncCfg.u8VCNumMax - pstDrvCtx->stSyncCfg.u8VCNum) % DIV_0_TO_1(pstDrvCtx->stSyncCfg.u8VCNumMax + 1);
                au32ChnSwitch[0] = (au32ChnSwitch[1] + 1) % DIV_0_TO_1(pstDrvCtx->stSyncCfg.u8VCNumMax + 1);
                au32ChnSwitch[2] = (au32ChnSwitch[1] + 2) % DIV_0_TO_1(pstDrvCtx->stSyncCfg.u8VCNumMax + 1);
                au32ChnSwitch[3] = (au32ChnSwitch[1] + 3) % DIV_0_TO_1(pstDrvCtx->stSyncCfg.u8VCNumMax + 1);
                break;

            case 2:
                au32ChnSwitch[2] = (pstDrvCtx->stSyncCfg.u8VCNumMax - pstDrvCtx->stSyncCfg.u8VCNum) % DIV_0_TO_1(pstDrvCtx->stSyncCfg.u8VCNumMax + 1);
                au32ChnSwitch[1] = (au32ChnSwitch[2] + 1) % DIV_0_TO_1(pstDrvCtx->stSyncCfg.u8VCNumMax + 1);
                au32ChnSwitch[0] = (au32ChnSwitch[2] + 2) % DIV_0_TO_1(pstDrvCtx->stSyncCfg.u8VCNumMax + 1);
                au32ChnSwitch[3] = (au32ChnSwitch[2] + 3) % DIV_0_TO_1(pstDrvCtx->stSyncCfg.u8VCNumMax + 1);
                break;

            case 3:
                au32ChnSwitch[3] = (pstDrvCtx->stSyncCfg.u8VCNumMax - pstDrvCtx->stSyncCfg.u8VCNum) % DIV_0_TO_1(pstDrvCtx->stSyncCfg.u8VCNumMax + 1);
                au32ChnSwitch[2] = (au32ChnSwitch[3] + 1) % DIV_0_TO_1(pstDrvCtx->stSyncCfg.u8VCNumMax + 1);
                au32ChnSwitch[1] = (au32ChnSwitch[3] + 2) % DIV_0_TO_1(pstDrvCtx->stSyncCfg.u8VCNumMax + 1);
                au32ChnSwitch[0] = (au32ChnSwitch[3] + 3) % DIV_0_TO_1(pstDrvCtx->stSyncCfg.u8VCNumMax + 1);
                break;
        }

        bChnSwitchEnable = HI_TRUE;

        ISP_DRV_SetInputSel(pstIspBeRegs, &au32ChnSwitch[0]);

    }
    else if ((IS_LINE_WDR_MODE(pstDrvCtx->stSyncCfg.u8WDRMode)) ||
             (IS_HALF_WDR_MODE(pstDrvCtx->stSyncCfg.u8WDRMode)))
    {
        switch ( u32ChannelSel & 0x3 )
        {
            case 0:
                au32ChnSwitch[0] = pstDrvCtx->astChnSelAttr[0].au32WdrChnSel[0];
                au32ChnSwitch[1] = pstDrvCtx->astChnSelAttr[0].au32WdrChnSel[1];
                au32ChnSwitch[2] = pstDrvCtx->astChnSelAttr[0].au32WdrChnSel[2];
                au32ChnSwitch[3] = pstDrvCtx->astChnSelAttr[0].au32WdrChnSel[3];
                break;

            case 1:
                au32ChnSwitch[1] = pstDrvCtx->astChnSelAttr[0].au32WdrChnSel[0];
                au32ChnSwitch[0] = pstDrvCtx->astChnSelAttr[0].au32WdrChnSel[1];
                au32ChnSwitch[2] = pstDrvCtx->astChnSelAttr[0].au32WdrChnSel[2];
                au32ChnSwitch[3] = pstDrvCtx->astChnSelAttr[0].au32WdrChnSel[3];
                break;

            case 2:
                au32ChnSwitch[2] = pstDrvCtx->astChnSelAttr[0].au32WdrChnSel[0];
                au32ChnSwitch[1] = pstDrvCtx->astChnSelAttr[0].au32WdrChnSel[1];
                au32ChnSwitch[0] = pstDrvCtx->astChnSelAttr[0].au32WdrChnSel[2];
                au32ChnSwitch[3] = pstDrvCtx->astChnSelAttr[0].au32WdrChnSel[3];
                break;

            case 3:
                au32ChnSwitch[3] = pstDrvCtx->astChnSelAttr[0].au32WdrChnSel[0];
                au32ChnSwitch[2] = pstDrvCtx->astChnSelAttr[0].au32WdrChnSel[1];
                au32ChnSwitch[1] = pstDrvCtx->astChnSelAttr[0].au32WdrChnSel[2];
                au32ChnSwitch[0] = pstDrvCtx->astChnSelAttr[0].au32WdrChnSel[3];
                break;
        }

        bChnSwitchEnable = HI_TRUE;
        u32IspFEInputMode = 1;

        /* offline mode: isp BE buffer poll, so chn switch need each frame refres */
        //if ((IS_OFFLINE_MODE(pstDrvCtx->stWorkMode.enIspRunningMode)) ||
        //    (IS_STRIPING_MODE(pstDrvCtx->stWorkMode.enIspRunningMode)))
        //{
        //    for (k = 0; k < u8BlkNum; k++)
        //    {
        //        ISP_DRV_SetInputSel(apstBeReg[k + u8BlkDev], &au32ChnSwitch[0]);
        //    }
        //}

        /* offline mode: isp BE buffer poll, so chn switch need each frame refres */
        if (IS_OFFLINE_MODE(pstDrvCtx->stWorkMode.enIspRunningMode) ||
            (IS_STRIPING_MODE(pstDrvCtx->stWorkMode.enIspRunningMode)))
        {
            ISP_DRV_SetInputSel(pstIspBeRegs, &au32ChnSwitch[0]);
        }
    }
    else
    {
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_RegConfigWdr(VI_PIPE ViPipe, S_ISPBE_REGS_TYPE *apstBeReg[], ISP_DRV_CTX_S *pstDrvCtx, ISP_SYNC_CFG_BUF_NODE_S *pstCfgNode, HI_U8 u8BlkDev, HI_U32 *u32Ratio)
{
    HI_U8   i;
    HI_U8   u8LFMode          = 0;
    HI_U8   u8BitDepthInValid = 0;
    HI_U8   u8BitShift        = 0;
    HI_U16  u16Offset0        = 0;
    HI_U16  u16Offset1        = 0;
    HI_U16  u16Offset2        = 0;
    HI_U32  au32ExpoValue[4]  = {0};
    HI_U32  au32BlcComp[3]    = {0};
    HI_S32  u32MaxRatio       = 0;
    HI_U16  u16LongThr        = 0xFFF;
    HI_U16  u16ShortThr       = 0xFFF;
    HI_BOOL bWDRMdtEn         = 0;

#if 0
    static HI_BOOL bMdtEn[ISP_MAX_PIPE_NUM][ISP_STRIPING_MAX_NUM] = \
    {[0 ... ISP_MAX_PIPE_NUM - 1] = {1, 1}};
    static HI_U16 au16ShortThr[ISP_MAX_PIPE_NUM][ISP_STRIPING_MAX_NUM] = \
    {[0 ... ISP_MAX_PIPE_NUM - 1] = {0x3F00, 0x3F00}};

    static HI_U16 au16LongThr[ISP_MAX_PIPE_NUM][ISP_STRIPING_MAX_NUM]  = \
    {[0 ... ISP_MAX_PIPE_NUM - 1] = {0xBC0, 0xBC0}};

    static HI_BOOL abLFM1st[ISP_MAX_PIPE_NUM][ISP_STRIPING_MAX_NUM] = \
    {[0 ... ISP_MAX_PIPE_NUM - 1] = {HI_TRUE, HI_TRUE}};

    static HI_BOOL abWDR1st[ISP_MAX_PIPE_NUM][ISP_STRIPING_MAX_NUM] = \
    {[0 ... ISP_MAX_PIPE_NUM - 1] = {HI_TRUE, HI_TRUE}};
#endif

    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_POINTER(pstDrvCtx);
    ISP_CHECK_POINTER(apstBeReg[u8BlkDev]);

    u8BitDepthInValid = apstBeReg[u8BlkDev]->ISP_WDR_BIT_DEPTH.bits.isp_wdr_bitdepth_invalid;
    u8BitShift = 14 - u8BitDepthInValid;

    if (IS_2to1_WDR_MODE(pstDrvCtx->stSyncCfg.u8WDRMode))
    {
        ISP_DRV_SetWdrExporratio0(apstBeReg[u8BlkDev], MIN2((ISP_BITMASK(10) * 64 / DIV_0_TO_1(u32Ratio[0])), 0x3FF));
        ISP_DRV_SetFlickExporatio0(apstBeReg[u8BlkDev], MIN2(u32Ratio[0], 0X3FFF));

        au32ExpoValue[0] = MIN2(u32Ratio[0], ISP_BITMASK(14));
        au32ExpoValue[1] = MIN2(64, ISP_BITMASK(14));
        ISP_DRV_SetWdrExpoValue0(apstBeReg[u8BlkDev], au32ExpoValue[0]);
        ISP_DRV_SetWdrExpoValue1(apstBeReg[u8BlkDev], au32ExpoValue[1]);

        u16Offset0     = apstBeReg[u8BlkDev]->ISP_WDR_F0_INBLC0.bits.isp_wdr_f0_inblc_r;
        au32BlcComp[0] = (au32ExpoValue[0] - au32ExpoValue[1]) * u16Offset0 >> u8BitShift;
        ISP_DRV_SetWdrBlcComp0(apstBeReg[u8BlkDev], au32BlcComp[0]);
    }
    else if (IS_3to1_WDR_MODE(pstDrvCtx->stSyncCfg.u8WDRMode))
    {
        ISP_DRV_SetWdrExporratio0(apstBeReg[u8BlkDev], MIN2((ISP_BITMASK(10) * 64 / DIV_0_TO_1(u32Ratio[0])), 0x3FF));
        ISP_DRV_SetWdrExporratio1(apstBeReg[u8BlkDev], MIN2((ISP_BITMASK(10) * 64 / DIV_0_TO_1(u32Ratio[1])), 0x3FF));

        ISP_DRV_SetFlickExporatio0(apstBeReg[u8BlkDev], MIN2(u32Ratio[0], 0X3FFF));
        ISP_DRV_SetFlickExporatio1(apstBeReg[u8BlkDev], MIN2(u32Ratio[1], 0X3FFF));

        au32ExpoValue[2] = MIN2(64, ISP_BITMASK(14));
        au32ExpoValue[1] = MIN2(u32Ratio[1], ISP_BITMASK(14));
        au32ExpoValue[0] = MIN2((u32Ratio[1] * u32Ratio[0]) >> 6, ISP_BITMASK(14));

        ISP_DRV_SetWdrExpoValue0(apstBeReg[u8BlkDev], au32ExpoValue[0]);
        ISP_DRV_SetWdrExpoValue1(apstBeReg[u8BlkDev], au32ExpoValue[1]);
        ISP_DRV_SetWdrExpoValue2(apstBeReg[u8BlkDev], au32ExpoValue[2]);

        u16Offset0 = apstBeReg[u8BlkDev]->ISP_WDR_F0_INBLC0.bits.isp_wdr_f0_inblc_r;
        u16Offset1 = apstBeReg[u8BlkDev]->ISP_WDR_F1_INBLC0.bits.isp_wdr_f1_inblc_r;
        au32BlcComp[0] = (au32ExpoValue[0] - au32ExpoValue[1]) * u16Offset0 >> u8BitShift;
        au32BlcComp[1] = (au32ExpoValue[1] - au32ExpoValue[2]) * u16Offset1 >> u8BitShift;
        ISP_DRV_SetWdrBlcComp0(apstBeReg[u8BlkDev], au32BlcComp[0]);
        ISP_DRV_SetWdrBlcComp1(apstBeReg[u8BlkDev], au32BlcComp[1]);
    }
    else if (IS_4to1_WDR_MODE(pstDrvCtx->stSyncCfg.u8WDRMode))
    {
        ISP_DRV_SetWdrExporratio0(apstBeReg[u8BlkDev], MIN2((ISP_BITMASK(10) * 64 / DIV_0_TO_1(u32Ratio[0])), 0x3FF));
        ISP_DRV_SetWdrExporratio1(apstBeReg[u8BlkDev], MIN2((ISP_BITMASK(10) * 64 / DIV_0_TO_1(u32Ratio[1])), 0x3FF));
        ISP_DRV_SetWdrExporratio2(apstBeReg[u8BlkDev], MIN2((ISP_BITMASK(10) * 64 / DIV_0_TO_1(u32Ratio[2])), 0x3FF));

        ISP_DRV_SetFlickExporatio0(apstBeReg[u8BlkDev], MIN2(u32Ratio[0], 0X3FFF));
        ISP_DRV_SetFlickExporatio1(apstBeReg[u8BlkDev], MIN2(u32Ratio[1], 0X3FFF));
        ISP_DRV_SetFlickExporatio2(apstBeReg[u8BlkDev], MIN2(u32Ratio[2], 0X3FFF));

        au32ExpoValue[3] = MIN2(64, ISP_BITMASK(14));
        au32ExpoValue[2] = MIN2(u32Ratio[2], ISP_BITMASK(14));
        au32ExpoValue[1] = MIN2((u32Ratio[2] * u32Ratio[1]) >> 6, ISP_BITMASK(14));
        au32ExpoValue[0] = MIN2((u32Ratio[2] * u32Ratio[1] * u32Ratio[0]) >> 12, ISP_BITMASK(14));

        ISP_DRV_SetWdrExpoValue0(apstBeReg[u8BlkDev], au32ExpoValue[0]);
        ISP_DRV_SetWdrExpoValue1(apstBeReg[u8BlkDev], au32ExpoValue[1]);
        ISP_DRV_SetWdrExpoValue2(apstBeReg[u8BlkDev], au32ExpoValue[2]);
        ISP_DRV_SetWdrExpoValue3(apstBeReg[u8BlkDev], au32ExpoValue[3]);

        u16Offset0 = apstBeReg[u8BlkDev]->ISP_WDR_F0_INBLC0.bits.isp_wdr_f0_inblc_r;
        u16Offset1 = apstBeReg[u8BlkDev]->ISP_WDR_F1_INBLC0.bits.isp_wdr_f1_inblc_r;
        u16Offset2 = apstBeReg[u8BlkDev]->ISP_WDR_F2_INBLC0.bits.isp_wdr_f2_inblc_r;
        au32BlcComp[0] = (au32ExpoValue[0] - au32ExpoValue[1]) * u16Offset0 >> u8BitShift;
        au32BlcComp[1] = (au32ExpoValue[1] - au32ExpoValue[2]) * u16Offset1 >> u8BitShift;
        au32BlcComp[2] = (au32ExpoValue[2] - au32ExpoValue[3]) * u16Offset1 >> u8BitShift;
        ISP_DRV_SetWdrBlcComp0(apstBeReg[u8BlkDev], au32BlcComp[0]);
        ISP_DRV_SetWdrBlcComp1(apstBeReg[u8BlkDev], au32BlcComp[1]);
        ISP_DRV_SetWdrBlcComp2(apstBeReg[u8BlkDev], au32BlcComp[2]);
    }

    for (i = 0; i < SYNC_EXP_RATIO_NUM; i++)
    {
        pstDrvCtx->stIspBeSyncPara.au32WdrExpRatio[i]   = MIN2((ISP_BITMASK(10) * 64 / DIV_0_TO_1(u32Ratio[i])), 0x3FF);
        pstDrvCtx->stIspBeSyncPara.au32FlickExpRatio[i] = MIN2(u32Ratio[i], 0X3FFF);
    }

    for (i = 0; i < SYNC_WDR_EXP_VAL_NUM; i++)
    {
        pstDrvCtx->stIspBeSyncPara.au32WdrExpVal[i] = au32ExpoValue[i];
    }

    for (i = 0; i < SYNC_WDR_BLC_COMP_NUM; i++)
    {
        pstDrvCtx->stIspBeSyncPara.au32WdrBlcComp[i] = au32BlcComp[i];
    }

    if ((IS_2to1_WDR_MODE(pstDrvCtx->stSyncCfg.u8WDRMode)) || (IS_3to1_WDR_MODE(pstDrvCtx->stSyncCfg.u8WDRMode)) || (IS_4to1_WDR_MODE(pstDrvCtx->stSyncCfg.u8WDRMode)))
    {
        u32MaxRatio = ((1 << 22) - 1) / DIV_0_TO_1(au32ExpoValue[0]);
        ISP_DRV_SetWdrMaxRatio(apstBeReg[u8BlkDev], u32MaxRatio);

        pstDrvCtx->stIspBeSyncPara.u32WdrMaxRatio = u32MaxRatio;
    }

    u8LFMode = pstDrvCtx->stSyncCfg.u8LFMode[pstDrvCtx->stSyncCfg.u8Cfg2VldDlyMAX - 1];

#if 0
    if (u8LFMode)
    {
        if (abLFM1st[ViPipe][u8BlkDev])
        {
            bMdtEn[ViPipe][u8BlkDev]       = apstBeReg[u8BlkDev]->ISP_WDR_CTRL.bits.isp_wdr_mdt_en;
            au16LongThr[ViPipe][u8BlkDev]  = apstBeReg[u8BlkDev]->ISP_WDR_WGTIDX_THR.bits.isp_wdr_long_thr;
            au16ShortThr[ViPipe][u8BlkDev] = apstBeReg[u8BlkDev]->ISP_WDR_WGTIDX_THR.bits.isp_wdr_short_thr;
            abLFM1st[ViPipe][u8BlkDev]     = HI_FALSE;
            abWDR1st[ViPipe][u8BlkDev]     = HI_TRUE;
        }

        ISP_DRV_SetWdrLongThr(apstBeReg[u8BlkDev], 0xFFF);
        ISP_DRV_SetWdrShortThr(apstBeReg[u8BlkDev], 0xFFF);
        ISP_DRV_SetWdrMdtEn(apstBeReg[u8BlkDev], 0);
    }
    else
    {
        if (abWDR1st[ViPipe][u8BlkDev])
        {

            ISP_DRV_SetWdrLongThr(apstBeReg[u8BlkDev], au16LongThr[ViPipe][u8BlkDev]);
            ISP_DRV_SetWdrShortThr(apstBeReg[u8BlkDev], au16ShortThr[ViPipe][u8BlkDev]);
            ISP_DRV_SetWdrMdtEn(apstBeReg[u8BlkDev], bMdtEn[ViPipe][u8BlkDev]);

            abWDR1st[ViPipe][u8BlkDev] = HI_FALSE;
            abLFM1st[ViPipe][u8BlkDev] = HI_TRUE;
        }
    }
#endif

    u16LongThr  = pstCfgNode->stWDRRegCfg.u16LongThr;
    u16ShortThr = pstCfgNode->stWDRRegCfg.u16ShortThr;
    bWDRMdtEn   = pstCfgNode->stWDRRegCfg.bWDRMdtEn;

    if ((0 != u8LFMode) && (au32ExpoValue[0] < 0x44))
    {
        u16LongThr  = 0xFFF;
        u16ShortThr = 0xFFF;
        bWDRMdtEn   = 0;
    }

    ISP_DRV_SetWdrLongThr(apstBeReg[u8BlkDev], u16LongThr);
    ISP_DRV_SetWdrShortThr(apstBeReg[u8BlkDev], u16ShortThr);
    ISP_DRV_SetWdrMdtEn(apstBeReg[u8BlkDev], bWDRMdtEn);
    pstDrvCtx->stIspBeSyncPara.u16LongThr  = u16LongThr;
    pstDrvCtx->stIspBeSyncPara.u16ShortThr = u16ShortThr;
    pstDrvCtx->stIspBeSyncPara.bWDRMdtEn   = bWDRMdtEn;

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_RegConfigLdci(S_ISPBE_REGS_TYPE *apstBeReg[], ISP_DRV_CTX_S *pstDrvCtx, HI_U8 u8BlkNum, HI_U8 u8BlkDev)
{
    HI_U8   k;
    HI_U32  u32LdciComp = 0x1000;
    HI_U32  u32LdciCompIndex = 2;

    ISP_CHECK_POINTER(pstDrvCtx);

    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_CHECK_POINTER(apstBeReg[k + u8BlkDev]);
    }

    u32LdciCompIndex = pstDrvCtx->stSyncCfg.u8Cfg2VldDlyMAX;
    u32LdciCompIndex = (u32LdciCompIndex > 2) ? (u32LdciCompIndex - 2) : 0;
    u32LdciComp = pstDrvCtx->stSyncCfg.u32DRCComp[u32LdciCompIndex];
    u32LdciComp = Sqrt32(u32LdciComp << DRC_COMP_SHIFT);
    u32LdciComp = MIN2(u32LdciComp, 0xFFFF);
    if (SNAP_STATE_CFG == pstDrvCtx->stSnapInfoLoad.enSnapState)
    {
        u32LdciComp = 0x1000;
    }

    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_DRV_SetLdciStatEvratio(apstBeReg[k + u8BlkDev], u32LdciComp);
    }

    pstDrvCtx->stIspBeSyncPara.u32LdciComp = u32LdciComp;

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_RegConfigDrc(VI_PIPE ViPipe, S_ISPBE_REGS_TYPE *apstBeReg[], ISP_DRV_CTX_S *pstDrvCtx, ISP_SYNC_CFG_BUF_NODE_S *pstCfgNode, HI_U32 *u32Ratio, HI_U8 u8BlkNum, HI_U8 u8BlkDev)
{
    HI_U8  k;
    HI_U8  i;
    HI_U32 u32DrcDivDenomLog;
    HI_U32 u32DrcDenomExp;
    HI_U32 u32DrcExpRatio = 0x1000;
    HI_U32 au32DrcPrevLuma[SYNC_DRC_PRELUMA_NUM] = {0};
    HI_BOOL bUpdateLogParam = HI_FALSE;

    HI_U32 au32DrcDivDenomLog[16] = {52429, 55188,  58254,  61681,  65536,  69905,  74898, 80659, \
                                     87379, 95319, 104843, 116472, 130980, 149557, 174114, 207870
                                    };

    HI_U32 au32DrcDenomExp[16] = {1310720, 1245184, 1179648, 1114113, 1048577, 983043, 917510, 851980, \
                                  786455,  720942,  655452,  590008,  524657, 459488, 394682, 330589
                                 };

    static HI_U8 u8DrcShpLog [ISP_MAX_PIPE_NUM][ISP_STRIPING_MAX_NUM] = \
    {[0 ... ISP_MAX_PIPE_NUM - 1] = {8, 8}};
    static HI_U8 u8DrcShpExp [ISP_MAX_PIPE_NUM][ISP_STRIPING_MAX_NUM] = \
    {[0 ... ISP_MAX_PIPE_NUM - 1] = {8, 8}};

    ISP_CHECK_POINTER(pstDrvCtx);
    ISP_CHECK_POINTER(pstCfgNode);
    ISP_CHECK_POINTER(u32Ratio);
    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_CHECK_POINTER(apstBeReg[k + u8BlkDev]);
    }

    /* config drc compensation */
    u32DrcExpRatio = pstDrvCtx->stSyncCfg.u32DRCComp[pstDrvCtx->stSyncCfg.u8Cfg2VldDlyMAX - 1];
    if (u32DrcExpRatio != 0x1000) // do division only when u32DrcExpRatio != 4096
    {
        u32DrcExpRatio = DIV_0_TO_1(u32DrcExpRatio);
        u32DrcExpRatio = osal_div_u64((1 << (DRC_COMP_SHIFT + DRC_COMP_SHIFT)), u32DrcExpRatio);
        u32DrcExpRatio = MIN2(u32DrcExpRatio, (15 << DRC_COMP_SHIFT)); // Maximum supported ratio is 15
    }

    if (SNAP_STATE_CFG == pstDrvCtx->stSnapInfoLoad.enSnapState)
    {
        u32DrcExpRatio = 0x1000;
    }

    if (u8DrcShpLog[ViPipe][u8BlkDev] != pstCfgNode->stDRCRegCfg.u8ShpLog \
        || u8DrcShpExp[ViPipe][u8BlkDev] != pstCfgNode->stDRCRegCfg.u8ShpExp)
    {
        u8DrcShpLog[ViPipe][u8BlkDev] = pstCfgNode->stDRCRegCfg.u8ShpLog;
        u8DrcShpExp[ViPipe][u8BlkDev] = pstCfgNode->stDRCRegCfg.u8ShpExp;
        bUpdateLogParam = HI_TRUE;
    }
    else
    {
        bUpdateLogParam = HI_FALSE;
    }

    // Compensate on PrevLuma when ShpLog/ShpExp is modified, but no compensation under offline repeat mode
    if (bUpdateLogParam && (!pstCfgNode->stDRCRegCfg.bIsOfflineRepeatMode))
    {
        for (i = 0; i < SYNC_DRC_PRELUMA_NUM - 1; i++)
        {
            au32DrcPrevLuma[i] = (HI_U32)((HI_S32)g_DrcCurLumaLut[u8DrcShpLog[ViPipe][u8BlkDev]][i] + pstCfgNode->stDRCRegCfg.as32PrevLumaDelta[i]);
        }
    }
    else
    {
        for (i = 0; i < SYNC_DRC_PRELUMA_NUM - 1; i++)
        {
            au32DrcPrevLuma[i] = g_DrcCurLumaLut[u8DrcShpLog[ViPipe][u8BlkDev]][i];
        }
    }
    au32DrcPrevLuma[SYNC_DRC_PRELUMA_NUM - 1] = (1 << 20);

    if ((u32DrcExpRatio != 0x1000) && (!pstCfgNode->stDRCRegCfg.bIsOfflineRepeatMode))
    {
        for (i = 0; i < SYNC_DRC_PRELUMA_NUM; i++)
        {
            au32DrcPrevLuma[i] = (HI_U32)(((HI_U64)u32DrcExpRatio * au32DrcPrevLuma[i]) >> DRC_COMP_SHIFT);
        }
    }

    u32DrcDivDenomLog = au32DrcDivDenomLog[u8DrcShpLog[ViPipe][u8BlkDev]];
    u32DrcDenomExp = au32DrcDenomExp[u8DrcShpExp[ViPipe][u8BlkDev]];
    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_DRV_SetDrcShpCfg(apstBeReg[k + u8BlkDev], u8DrcShpLog[ViPipe][u8BlkDev], u8DrcShpExp[ViPipe][u8BlkDev]);
        ISP_DRV_SetDrcDivDenomLog(apstBeReg[k + u8BlkDev], u32DrcDivDenomLog);
        ISP_DRV_SetDrcDenomExp(apstBeReg[k + u8BlkDev], u32DrcDenomExp);

        ISP_DRV_SetDrcPrevLuma0(apstBeReg[k + u8BlkDev], au32DrcPrevLuma[0]);
        ISP_DRV_SetDrcPrevLuma1(apstBeReg[k + u8BlkDev], au32DrcPrevLuma[1]);
        ISP_DRV_SetDrcPrevLuma2(apstBeReg[k + u8BlkDev], au32DrcPrevLuma[2]);
        ISP_DRV_SetDrcPrevLuma3(apstBeReg[k + u8BlkDev], au32DrcPrevLuma[3]);
        ISP_DRV_SetDrcPrevLuma4(apstBeReg[k + u8BlkDev], au32DrcPrevLuma[4]);
        ISP_DRV_SetDrcPrevLuma5(apstBeReg[k + u8BlkDev], au32DrcPrevLuma[5]);
        ISP_DRV_SetDrcPrevLuma6(apstBeReg[k + u8BlkDev], au32DrcPrevLuma[6]);
        ISP_DRV_SetDrcPrevLuma7(apstBeReg[k + u8BlkDev], au32DrcPrevLuma[7]);
    }

    for (i = 0; i < SYNC_DRC_PRELUMA_NUM; i++)
    {
        pstDrvCtx->stIspBeSyncPara.au32DrcPrevLuma[i] = au32DrcPrevLuma[i];
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_RegConfigDgain(S_ISPBE_REGS_TYPE *apstBeReg[], HI_U32  u32IspDgain, HI_U8 u8BlkNum, HI_U8 u8BlkDev)
{
    HI_U8 k;

    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_CHECK_POINTER(apstBeReg[k + u8BlkDev]);
        ISP_DRV_SetIspDgain(apstBeReg[k + u8BlkDev], u32IspDgain);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_RegConfig4Dgain(S_ISPBE_REGS_TYPE *apstBeReg[], ISP_DRV_CTX_S *pstDrvCtx, HI_U32 *au32WDRGain, HI_U8 u8BlkNum, HI_U8 u8BlkDev)
{
    HI_U8 k;

    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_CHECK_POINTER(apstBeReg[k + u8BlkDev]);
        ISP_DRV_SetIsp4Dgain0(apstBeReg[k + u8BlkDev], au32WDRGain[0]);
        ISP_DRV_SetIsp4Dgain1(apstBeReg[k + u8BlkDev], au32WDRGain[1]);
        ISP_DRV_SetIsp4Dgain2(apstBeReg[k + u8BlkDev], au32WDRGain[2]);
        ISP_DRV_SetIsp4Dgain3(apstBeReg[k + u8BlkDev], au32WDRGain[3]);
    }

    pstDrvCtx->stIspBeSyncPara.au32WDRGain[0] = au32WDRGain[0];
    pstDrvCtx->stIspBeSyncPara.au32WDRGain[1] = au32WDRGain[1];
    pstDrvCtx->stIspBeSyncPara.au32WDRGain[2] = au32WDRGain[2];
    pstDrvCtx->stIspBeSyncPara.au32WDRGain[3] = au32WDRGain[3];

    return HI_SUCCESS;
}

/*read FE statistics information*/
HI_S32 ISP_DRV_FE_AE_StatisticsRead(ISP_STAT_S *pstStat, S_ISPFE_REGS_TYPE *pstFeReg, HI_U32 k)
{
    HI_S32 i, j;
    HI_U32 u32AveMem = 0;

    ISP_CHECK_POINTER(pstStat);
    ISP_CHECK_POINTER(pstFeReg);

    pstFeReg->ISP_AE1_HIST_RADDR.u32 = 0x0;

    for (i = 0; i < 1024; i++)
    {
        pstStat->stFEAeStat1.au32HistogramMemArray[k][i] = pstFeReg->ISP_AE1_HIST_RDATA.u32;
    }

    pstStat->stFEAeStat1.u32PixelWeight[k] = pstFeReg->ISP_AE1_COUNT_STAT.u32;
    pstStat->stFEAeStat1.u32PixelCount[k]  = pstFeReg->ISP_AE1_TOTAL_STAT.u32;

    pstStat->stFEAeStat2.u16GlobalAvgR[k]  = pstFeReg->ISP_AE1_TOTAL_R_AVER.u32;
    pstStat->stFEAeStat2.u16GlobalAvgGr[k] = pstFeReg->ISP_AE1_TOTAL_GR_AVER.u32;
    pstStat->stFEAeStat2.u16GlobalAvgGb[k] = pstFeReg->ISP_AE1_TOTAL_GB_AVER.u32;
    pstStat->stFEAeStat2.u16GlobalAvgB[k]  = pstFeReg->ISP_AE1_TOTAL_B_AVER.u32;

    pstFeReg->ISP_AE1_AVER_R_GR_RADDR.u32 = 0x0;
    pstFeReg->ISP_AE1_AVER_GB_B_RADDR.u32 = 0x0;

    for (i = 0; i < AE_ZONE_ROW; i++)
    {
        for (j = 0; j < AE_ZONE_COLUMN; j++)
        {
            u32AveMem = pstFeReg->ISP_AE1_AVER_R_GR_RDATA.u32;
            pstStat->stFEAeStat3.au16ZoneAvg[k][i][j][0] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
            pstStat->stFEAeStat3.au16ZoneAvg[k][i][j][1] = (HI_U16)((u32AveMem & 0xFFFF));

            u32AveMem = pstFeReg->ISP_AE1_AVER_GB_B_RDATA.u32;
            pstStat->stFEAeStat3.au16ZoneAvg[k][i][j][2] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
            pstStat->stFEAeStat3.au16ZoneAvg[k][i][j][3] = (HI_U16)((u32AveMem & 0xFFFF));
        }
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_FE_AE_Global_StatisticsRead(ISP_STAT_S *pstStat, S_ISPFE_REGS_TYPE *pstFeReg, HI_U32 k)
{
    HI_S32 i;

    ISP_CHECK_POINTER(pstStat);
    ISP_CHECK_POINTER(pstFeReg);

    pstFeReg->ISP_AE1_HIST_RADDR.u32 = 0x0;

    for (i = 0; i < 1024; i++)
    {
        pstStat->stFEAeStat1.au32HistogramMemArray[k][i] = pstFeReg->ISP_AE1_HIST_RDATA.u32;
    }

    pstStat->stFEAeStat1.u32PixelWeight[k] = pstFeReg->ISP_AE1_COUNT_STAT.u32;
    pstStat->stFEAeStat1.u32PixelCount[k]  = pstFeReg->ISP_AE1_TOTAL_STAT.u32;

    pstStat->stFEAeStat2.u16GlobalAvgR[k]  = pstFeReg->ISP_AE1_TOTAL_R_AVER.u32;
    pstStat->stFEAeStat2.u16GlobalAvgGr[k] = pstFeReg->ISP_AE1_TOTAL_GR_AVER.u32;
    pstStat->stFEAeStat2.u16GlobalAvgGb[k] = pstFeReg->ISP_AE1_TOTAL_GB_AVER.u32;
    pstStat->stFEAeStat2.u16GlobalAvgB[k]  = pstFeReg->ISP_AE1_TOTAL_B_AVER.u32;

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_FE_AE_Local_StatisticsRead(ISP_STAT_S *pstStat, S_ISPFE_REGS_TYPE *pstFeReg, HI_U32 k)
{
    HI_S32 i, j;
    HI_U32 u32AveMem = 0;

    ISP_CHECK_POINTER(pstStat);
    ISP_CHECK_POINTER(pstFeReg);

    pstFeReg->ISP_AE1_AVER_R_GR_RADDR.u32 = 0x0;
    pstFeReg->ISP_AE1_AVER_GB_B_RADDR.u32 = 0x0;

    for (i = 0; i < AE_ZONE_ROW; i++)
    {
        for (j = 0; j < AE_ZONE_COLUMN; j++)
        {
            u32AveMem = pstFeReg->ISP_AE1_AVER_R_GR_RDATA.u32;
            pstStat->stFEAeStat3.au16ZoneAvg[k][i][j][0] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
            pstStat->stFEAeStat3.au16ZoneAvg[k][i][j][1] = (HI_U16)((u32AveMem & 0xFFFF));

            u32AveMem = pstFeReg->ISP_AE1_AVER_GB_B_RDATA.u32;
            pstStat->stFEAeStat3.au16ZoneAvg[k][i][j][2] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
            pstStat->stFEAeStat3.au16ZoneAvg[k][i][j][3] = (HI_U16)((u32AveMem & 0xFFFF));
        }
    }

    return HI_SUCCESS;
}


HI_S32 ISP_DRV_FE_AF_StatisticsRead(ISP_STAT_S *pstStat, S_ISPFE_REGS_TYPE *pstFeReg, VI_PIPE ViPipeBind, HI_U32 k)
{
    HI_S32 i, j;
    HI_U8  u8AfEnbale[ISP_MAX_PIPE_NUM] = {ISP_PIPE_FEAF_MODULE_ENABLE};
    HI_U32 u32FEAfStatData = 0;
    HI_U32 u32Zone;// = pstFeReg->ISP_AF1_ZONE.u32;
    HI_U32 u32Col;// = (u32Zone & 0x1F);
    HI_U32 u32Row;// = ((u32Zone & 0x1F00) >> 8);

    ISP_CHECK_PIPE(ViPipeBind);
    ISP_CHECK_POINTER(pstStat);
    ISP_CHECK_POINTER(pstFeReg);

    u32Zone = pstFeReg->ISP_AF1_ZONE.u32;
    u32Col  = (u32Zone & 0x1F);
    u32Row  = ((u32Zone & 0x1F00) >> 8);

    if (!u8AfEnbale[ViPipeBind])
    {
        for (i = 0; i < u32Row; i++)
        {
            for (j = 0; j < u32Col; j++)
            {
                pstStat->stFEAfStat.stZoneMetrics[k][i][j].u16h1 = 0;
                pstStat->stFEAfStat.stZoneMetrics[k][i][j].u16h2 = 0;
                pstStat->stFEAfStat.stZoneMetrics[k][i][j].u16v1 = 0;
                pstStat->stFEAfStat.stZoneMetrics[k][i][j].u16v2 = 0;
                pstStat->stFEAfStat.stZoneMetrics[k][i][j].u16HlCnt = 0;
                pstStat->stFEAfStat.stZoneMetrics[k][i][j].u16y  = 0;
            }
        }

        return HI_SUCCESS;
    }

    pstFeReg->ISP_AF1_STAT_H1_RADDR.u32 = 0;
    pstFeReg->ISP_AF1_STAT_H2_RADDR.u32 = 0;
    pstFeReg->ISP_AF1_STAT_V1_RADDR.u32 = 0;
    pstFeReg->ISP_AF1_STAT_V2_RADDR.u32 = 0;
    pstFeReg->ISP_AF1_STAT_Y_RADDR.u32  = 0;

    for (i = 0; i < u32Row; i++)
    {
        for (j = 0; j < u32Col; j++)
        {
            u32FEAfStatData = pstFeReg->ISP_AF1_STAT_H1_RDATA.u32;
            pstStat->stFEAfStat.stZoneMetrics[k][i][j].u16h1 = (HI_U16)(0xFFFF & u32FEAfStatData);
            u32FEAfStatData = pstFeReg->ISP_AF1_STAT_H2_RDATA.u32;
            pstStat->stFEAfStat.stZoneMetrics[k][i][j].u16h2 = (HI_U16)(0xFFFF & u32FEAfStatData);
            u32FEAfStatData = pstFeReg->ISP_AF1_STAT_V1_RDATA.u32;
            pstStat->stFEAfStat.stZoneMetrics[k][i][j].u16v1 = (HI_U16)(0xFFFF & u32FEAfStatData);
            u32FEAfStatData = pstFeReg->ISP_AF1_STAT_V2_RDATA.u32;
            pstStat->stFEAfStat.stZoneMetrics[k][i][j].u16v2 = (HI_U16)(0xFFFF & u32FEAfStatData);
            u32FEAfStatData = pstFeReg->ISP_AF1_STAT_Y_RDATA.u32;
            pstStat->stFEAfStat.stZoneMetrics[k][i][j].u16HlCnt = (HI_U16)((0xFFFF0000 & u32FEAfStatData) >> 16);
            pstStat->stFEAfStat.stZoneMetrics[k][i][j].u16y     = (HI_U16)(0xFFFF & u32FEAfStatData);
        }
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_FE_APB_StatisticsRead(ISP_STAT_S *pstStat, S_ISPFE_REGS_TYPE *pstFeReg, VI_PIPE ViPipeBind, HI_U32 k, ISP_STAT_KEY_U unStatkey)
{
    if (unStatkey.bit1FEAeGloStat)
    {
        ISP_DRV_FE_AE_Global_StatisticsRead(pstStat, pstFeReg, k);
    }

    if (unStatkey.bit1FEAeLocStat)
    {
        ISP_DRV_FE_AE_Local_StatisticsRead(pstStat, pstFeReg, k);
    }

    if (unStatkey.bit1FEAfStat)
    {
        ISP_DRV_FE_AF_StatisticsRead(pstStat, pstFeReg, ViPipeBind, k);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_FE_STT_StatisticsRead(ISP_STAT_S *pstStat, S_ISPFE_REGS_TYPE *pstFeReg, ISP_DRV_CTX_S *pstDrvCtx, VI_PIPE ViPipeBind, HI_U32 k, ISP_STAT_KEY_U unStatkey)
{
    return HI_SUCCESS;
}

HI_S32  ISP_DRV_FE_StitchStatisticsRead(VI_PIPE ViPipe, ISP_STAT_INFO_S *pstStatInfo)
{
    HI_U8   u8StitchNum;
    HI_U32  i, j, k, l;
    HI_U32  u32AveMem = 0;
    HI_U64  au64GlobalAvgSumR[ISP_WDR_CHN_MAX]  = {0};
    HI_U64  au64GlobalAvgSumGr[ISP_WDR_CHN_MAX] = {0};
    HI_U64  au64GlobalAvgSumGb[ISP_WDR_CHN_MAX] = {0};
    HI_U64  au64GlobalAvgSumB[ISP_WDR_CHN_MAX]  = {0};
    HI_U32  u32PixelWeight = 0;
    HI_U32  u32PixelWeightTmp = 0;
    VI_PIPE StitPipeBind, WdrPipeBind;

    ISP_DRV_CTX_S *pstDrvCtx     = HI_NULL;
    ISP_DRV_CTX_S *pstDrvBindCtx = HI_NULL;
    ISP_STAT_S    *pstStat       = HI_NULL;
    S_ISPFE_REGS_TYPE *pstFeReg  = HI_NULL;
    ISP_STAT_KEY_U unStatkey;

    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_POINTER(pstStatInfo);

    pstDrvCtx = ISP_DRV_GET_CTX(ViPipe);
    pstStat = (ISP_STAT_S * )pstStatInfo->pVirtAddr;

    if (HI_NULL == pstStat)
    {
        return HI_FAILURE;
    }

    unStatkey.u64Key = pstStatInfo->unKey.bit32IsrAccess;

    if ((HI_TRUE != pstDrvCtx->stStitchAttr.bStitchEnable) || (HI_TRUE != pstDrvCtx->stStitchAttr.bMainPipe))
    {
        return HI_FAILURE;
    }

    u8StitchNum = pstDrvCtx->stStitchAttr.u8StitchPipeNum;

    if (unStatkey.bit1FEAeStiGloStat)
    {
        for (k = 0; k < u8StitchNum; k++)
        {
            StitPipeBind = pstDrvCtx->stStitchAttr.as8StitchBindId[k];

            ISP_CHECK_PIPE(StitPipeBind);

            pstDrvBindCtx = ISP_DRV_GET_CTX(StitPipeBind);

            for (j = 0; j < pstDrvCtx->stWDRAttr.stDevBindPipe.u32Num; j++)
            {
                WdrPipeBind = pstDrvBindCtx->stWDRAttr.stDevBindPipe.PipeId[j];
                ISP_CHECK_PIPE(WdrPipeBind);

                ISP_DRV_FEREG_CTX(WdrPipeBind, pstFeReg);

                pstFeReg->ISP_AE1_HIST_RADDR.u32 = 0x0;

                if (k == 0)
                {
                    for (i = 0; i < 1024; i++)
                    {
                        pstStat->stStitchStat.stFEAeStat1.au32HistogramMemArray[j][i] = pstFeReg->ISP_AE1_HIST_RDATA.u32;
                    }

                    u32PixelWeightTmp = pstFeReg->ISP_AE1_COUNT_STAT.u32;
                    pstStat->stStitchStat.stFEAeStat1.u32PixelCount[j]  = u32PixelWeightTmp;
                    pstStat->stStitchStat.stFEAeStat1.u32PixelWeight[j] = pstFeReg->ISP_AE1_TOTAL_STAT.u32;
                }
                else
                {
                    for (i = 0; i < 1024; i++)
                    {
                        pstStat->stStitchStat.stFEAeStat1.au32HistogramMemArray[j][i] += pstFeReg->ISP_AE1_HIST_RDATA.u32;
                    }

                    u32PixelWeightTmp = pstFeReg->ISP_AE1_COUNT_STAT.u32;
                    pstStat->stStitchStat.stFEAeStat1.u32PixelCount[j]  += u32PixelWeightTmp;
                    pstStat->stStitchStat.stFEAeStat1.u32PixelWeight[j] += pstFeReg->ISP_AE1_TOTAL_STAT.u32;
                }

                au64GlobalAvgSumR[j]  += (HI_U64)pstStat->stFEAeStat2.u16GlobalAvgR[j]  * u32PixelWeightTmp;
                au64GlobalAvgSumGr[j] += (HI_U64)pstStat->stFEAeStat2.u16GlobalAvgGr[j] * u32PixelWeightTmp;
                au64GlobalAvgSumGb[j] += (HI_U64)pstStat->stFEAeStat2.u16GlobalAvgGb[j] * u32PixelWeightTmp;
                au64GlobalAvgSumB[j]  += (HI_U64)pstStat->stFEAeStat2.u16GlobalAvgB[j]  * u32PixelWeightTmp;
            }
        }

        for (j = 0; j < pstDrvCtx->stWDRAttr.stDevBindPipe.u32Num; j++)
        {
            u32PixelWeight = DIV_0_TO_1(pstStat->stStitchStat.stFEAeStat1.u32PixelCount[j]);

            pstStat->stStitchStat.stFEAeStat2.u16GlobalAvgR[j]  = osal_div_u64(au64GlobalAvgSumR[j],  u32PixelWeight);
            pstStat->stStitchStat.stFEAeStat2.u16GlobalAvgGr[j] = osal_div_u64(au64GlobalAvgSumGr[j], u32PixelWeight);
            pstStat->stStitchStat.stFEAeStat2.u16GlobalAvgGb[j] = osal_div_u64(au64GlobalAvgSumGb[j], u32PixelWeight);
            pstStat->stStitchStat.stFEAeStat2.u16GlobalAvgB[j]  = osal_div_u64(au64GlobalAvgSumB[j],  u32PixelWeight);
        }
    }

    if (unStatkey.bit1FEAeStiLocStat)
    {
        for (k = 0; k < u8StitchNum; k++)
        {
            StitPipeBind = pstDrvCtx->stStitchAttr.as8StitchBindId[k];

            ISP_CHECK_PIPE(StitPipeBind);

            pstDrvBindCtx = ISP_DRV_GET_CTX(StitPipeBind);

            for (j = 0; j < pstDrvCtx->stWDRAttr.stDevBindPipe.u32Num; j++)
            {
                WdrPipeBind = pstDrvBindCtx->stWDRAttr.stDevBindPipe.PipeId[j];
                ISP_CHECK_PIPE(WdrPipeBind);

                ISP_DRV_FEREG_CTX(WdrPipeBind, pstFeReg);

                pstFeReg->ISP_AE1_AVER_R_GR_RADDR.u32 = 0x0;
                pstFeReg->ISP_AE1_AVER_GB_B_RADDR.u32 = 0x0;

                for (i = 0; i < AE_ZONE_ROW; i++)
                {
                    for (l = 0; l < AE_ZONE_COLUMN; l++)
                    {
                        u32AveMem = pstFeReg->ISP_AE1_AVER_R_GR_RDATA.u32;
                        pstStat->stStitchStat.stFEAeStat3.au16ZoneAvg[StitPipeBind][j][i][l][0] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
                        pstStat->stStitchStat.stFEAeStat3.au16ZoneAvg[StitPipeBind][j][i][l][1] = (HI_U16)((u32AveMem & 0xFFFF));

                        u32AveMem = pstFeReg->ISP_AE1_AVER_GB_B_RDATA.u32;
                        pstStat->stStitchStat.stFEAeStat3.au16ZoneAvg[StitPipeBind][j][i][l][2] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
                        pstStat->stStitchStat.stFEAeStat3.au16ZoneAvg[StitPipeBind][j][i][l][3] = (HI_U16)((u32AveMem & 0xFFFF));
                    }
                }
            }
        }
    }

    return HI_SUCCESS;
}

/*read BE statistics information from phy:online*/
HI_S32 ISP_DRV_BE_COMM_StatisticsRead(ISP_STAT_S *pstStat, S_ISPBE_REGS_TYPE *apstBeReg[], HI_U8 u8BlkDev)
{
    ISP_CHECK_POINTER(pstStat);
    ISP_CHECK_POINTER(apstBeReg[u8BlkDev]);

    pstStat->stCommStat.au16WhiteBalanceGain[0] = apstBeReg[u8BlkDev]->ISP_WB_GAIN1.bits.isp_wb_rgain;
    pstStat->stCommStat.au16WhiteBalanceGain[1] = apstBeReg[u8BlkDev]->ISP_WB_GAIN1.bits.isp_wb_grgain;
    pstStat->stCommStat.au16WhiteBalanceGain[2] = apstBeReg[u8BlkDev]->ISP_WB_GAIN2.bits.isp_wb_gbgain;
    pstStat->stCommStat.au16WhiteBalanceGain[3] = apstBeReg[u8BlkDev]->ISP_WB_GAIN2.bits.isp_wb_bgain;

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_BE_AE_StatisticsRead(ISP_STAT_S *pstStat, S_ISPBE_REGS_TYPE *apstBeReg[], HI_U8 u8BlkNum, HI_U8 u8BlkDev)
{
    HI_U8  k;
    HI_U8  u8BlockOffset    = 0;
    HI_U8  u8BlockZoneWidth = 0;
    HI_U32 i, j;
    HI_U32 u32AveMem;
    HI_U64 u64GlobalAvgSumR  = 0;
    HI_U64 u64GlobalAvgSumGr = 0;
    HI_U64 u64GlobalAvgSumGb = 0;
    HI_U64 u64GlobalAvgSumB  = 0;
    HI_U32 u32PixelWeightTmp = 0;
    HI_U32 u32PixelWeight    = 0;

    ISP_CHECK_POINTER(pstStat);

    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_CHECK_POINTER(apstBeReg[k + u8BlkDev]);

        apstBeReg[k + u8BlkDev]->ISP_AE_HIST_RADDR.u32 = 0x0;

        for (i = 0; i < 1024; i++)
        {
            if (k == 0)
            {
                pstStat->stBEAeStat1.au32HistogramMemArray[i]  = apstBeReg[k + u8BlkDev]->ISP_AE_HIST_RDATA.u32;
            }
            else
            {
                pstStat->stBEAeStat1.au32HistogramMemArray[i] += apstBeReg[k + u8BlkDev]->ISP_AE_HIST_RDATA.u32;
            }
        }

        if (k == 0)
        {
            u32PixelWeightTmp = apstBeReg[k + u8BlkDev]->ISP_AE_COUNT_STAT.bits.isp_ae_count_pixels;
            pstStat->stBEAeStat1.u32PixelWeight = u32PixelWeightTmp;
            pstStat->stBEAeStat1.u32PixelCount  = apstBeReg[k + u8BlkDev]->ISP_AE_TOTAL_STAT.bits.isp_ae_total_pixels;
        }
        else
        {
            u32PixelWeightTmp = apstBeReg[k + u8BlkDev]->ISP_AE_COUNT_STAT.bits.isp_ae_count_pixels;
            pstStat->stBEAeStat1.u32PixelWeight += u32PixelWeightTmp;
            pstStat->stBEAeStat1.u32PixelCount  += apstBeReg[k + u8BlkDev]->ISP_AE_TOTAL_STAT.bits.isp_ae_total_pixels;
        }

        /**/
        u64GlobalAvgSumR  += ((HI_U64)apstBeReg[k + u8BlkDev]->ISP_AE_TOTAL_R_AVER.u32)  * ((HI_U64)u32PixelWeightTmp);
        u64GlobalAvgSumGr += ((HI_U64)apstBeReg[k + u8BlkDev]->ISP_AE_TOTAL_GR_AVER.u32) * ((HI_U64)u32PixelWeightTmp);
        u64GlobalAvgSumGb += ((HI_U64)apstBeReg[k + u8BlkDev]->ISP_AE_TOTAL_GB_AVER.u32) * ((HI_U64)u32PixelWeightTmp);
        u64GlobalAvgSumB  += ((HI_U64)apstBeReg[k + u8BlkDev]->ISP_AE_TOTAL_B_AVER.u32)  * ((HI_U64)u32PixelWeightTmp);

        /**/
        apstBeReg[k + u8BlkDev]->ISP_AE_AVER_R_GR_RADDR.u32 = 0x0;
        apstBeReg[k + u8BlkDev]->ISP_AE_AVER_GB_B_RADDR.u32 = 0x0;

        for (i = 0; i < AE_ZONE_ROW; i++)
        {
            if (k < (AE_ZONE_COLUMN % DIV_0_TO_1(u8BlkNum)))
            {
                u8BlockZoneWidth = (AE_ZONE_COLUMN / DIV_0_TO_1(u8BlkNum)) + 1;
            }
            else
            {
                u8BlockZoneWidth = AE_ZONE_COLUMN / DIV_0_TO_1(u8BlkNum);
            }

            for (j = 0; j < u8BlockZoneWidth; j++)
            {
                u32AveMem = apstBeReg[k + u8BlkDev]->ISP_AE_AVER_R_GR_RDATA.u32;
                pstStat->stBEAeStat3.au16ZoneAvg[i][j + u8BlockOffset][0] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
                pstStat->stBEAeStat3.au16ZoneAvg[i][j + u8BlockOffset][1] = (HI_U16)((u32AveMem & 0xFFFF));

                u32AveMem = apstBeReg[k + u8BlkDev]->ISP_AE_AVER_GB_B_RDATA.u32;
                pstStat->stBEAeStat3.au16ZoneAvg[i][j + u8BlockOffset][2] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
                pstStat->stBEAeStat3.au16ZoneAvg[i][j + u8BlockOffset][3] = (HI_U16)((u32AveMem & 0xFFFF));
            }
        }

        u8BlockOffset += u8BlockZoneWidth;
    }

    u32PixelWeight = DIV_0_TO_1(pstStat->stBEAeStat1.u32PixelWeight);

    pstStat->stBEAeStat2.u16GlobalAvgR  = osal_div_u64(u64GlobalAvgSumR,  u32PixelWeight);
    pstStat->stBEAeStat2.u16GlobalAvgGr = osal_div_u64(u64GlobalAvgSumGr, u32PixelWeight);
    pstStat->stBEAeStat2.u16GlobalAvgGb = osal_div_u64(u64GlobalAvgSumGb, u32PixelWeight);
    pstStat->stBEAeStat2.u16GlobalAvgB  = osal_div_u64(u64GlobalAvgSumB,  u32PixelWeight);

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_BE_AE_Global_StatisticsRead(ISP_STAT_S *pstStat, S_ISPBE_REGS_TYPE *apstBeReg[], HI_U8 u8BlkNum, HI_U8 u8BlkDev)
{
    HI_U32 i, k;
    HI_U64 u64GlobalAvgSumR  = 0;
    HI_U64 u64GlobalAvgSumGr = 0;
    HI_U64 u64GlobalAvgSumGb = 0;
    HI_U64 u64GlobalAvgSumB  = 0;
    HI_U32 u32PixelWeightTmp = 0;
    HI_U32 u32PixelWeight    = 0;

    ISP_CHECK_POINTER(pstStat);

    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_CHECK_POINTER(apstBeReg[k + u8BlkDev]);

        apstBeReg[k + u8BlkDev]->ISP_AE_HIST_RADDR.u32 = 0x0;

        for (i = 0; i < 1024; i++)
        {
            if (k == 0)
            {
                pstStat->stBEAeStat1.au32HistogramMemArray[i]  = apstBeReg[k + u8BlkDev]->ISP_AE_HIST_RDATA.u32;
            }
            else
            {
                pstStat->stBEAeStat1.au32HistogramMemArray[i] += apstBeReg[k + u8BlkDev]->ISP_AE_HIST_RDATA.u32;
            }
        }

        if (k == 0)
        {
            u32PixelWeightTmp = apstBeReg[k + u8BlkDev]->ISP_AE_COUNT_STAT.bits.isp_ae_count_pixels;
            pstStat->stBEAeStat1.u32PixelWeight = u32PixelWeightTmp;
            pstStat->stBEAeStat1.u32PixelCount  = apstBeReg[k + u8BlkDev]->ISP_AE_TOTAL_STAT.bits.isp_ae_total_pixels;
        }
        else
        {
            u32PixelWeightTmp = apstBeReg[k + u8BlkDev]->ISP_AE_COUNT_STAT.bits.isp_ae_count_pixels;
            pstStat->stBEAeStat1.u32PixelWeight += u32PixelWeightTmp;
            pstStat->stBEAeStat1.u32PixelCount  += apstBeReg[k + u8BlkDev]->ISP_AE_TOTAL_STAT.bits.isp_ae_total_pixels;
        }

        /**/
        u64GlobalAvgSumR  += ((HI_U64)apstBeReg[k + u8BlkDev]->ISP_AE_TOTAL_R_AVER.u32)  * ((HI_U64)u32PixelWeightTmp);
        u64GlobalAvgSumGr += ((HI_U64)apstBeReg[k + u8BlkDev]->ISP_AE_TOTAL_GR_AVER.u32) * ((HI_U64)u32PixelWeightTmp);
        u64GlobalAvgSumGb += ((HI_U64)apstBeReg[k + u8BlkDev]->ISP_AE_TOTAL_GB_AVER.u32) * ((HI_U64)u32PixelWeightTmp);
        u64GlobalAvgSumB  += ((HI_U64)apstBeReg[k + u8BlkDev]->ISP_AE_TOTAL_B_AVER.u32)  * ((HI_U64)u32PixelWeightTmp);
    }

    u32PixelWeight = DIV_0_TO_1(pstStat->stBEAeStat1.u32PixelWeight);

    pstStat->stBEAeStat2.u16GlobalAvgR  = osal_div_u64(u64GlobalAvgSumR,  u32PixelWeight);
    pstStat->stBEAeStat2.u16GlobalAvgGr = osal_div_u64(u64GlobalAvgSumGr, u32PixelWeight);
    pstStat->stBEAeStat2.u16GlobalAvgGb = osal_div_u64(u64GlobalAvgSumGb, u32PixelWeight);
    pstStat->stBEAeStat2.u16GlobalAvgB  = osal_div_u64(u64GlobalAvgSumB,  u32PixelWeight);

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_BE_AE_Local_StatisticsRead(ISP_STAT_S *pstStat, S_ISPBE_REGS_TYPE *apstBeReg[], HI_U8 u8BlkNum, HI_U8 u8BlkDev)
{
    HI_U32 i, j, k;
    HI_U8  u8BlockOffset    = 0;
    HI_U8  u8BlockZoneWidth = 0;
    HI_U32 u32AveMem;

    ISP_CHECK_POINTER(pstStat);

    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_CHECK_POINTER(apstBeReg[k + u8BlkDev]);

        /**/
        apstBeReg[k + u8BlkDev]->ISP_AE_AVER_R_GR_RADDR.u32 = 0x0;
        apstBeReg[k + u8BlkDev]->ISP_AE_AVER_GB_B_RADDR.u32 = 0x0;

        for (i = 0; i < AE_ZONE_ROW; i++)
        {
            if (k < (AE_ZONE_COLUMN % DIV_0_TO_1(u8BlkNum)))
            {
                u8BlockZoneWidth = (AE_ZONE_COLUMN / DIV_0_TO_1(u8BlkNum)) + 1;
            }
            else
            {
                u8BlockZoneWidth = AE_ZONE_COLUMN / DIV_0_TO_1(u8BlkNum);
            }

            for (j = 0; j < u8BlockZoneWidth; j++)
            {
                u32AveMem = apstBeReg[k + u8BlkDev]->ISP_AE_AVER_R_GR_RDATA.u32;
                pstStat->stBEAeStat3.au16ZoneAvg[i][j + u8BlockOffset][0] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
                pstStat->stBEAeStat3.au16ZoneAvg[i][j + u8BlockOffset][1] = (HI_U16)((u32AveMem & 0xFFFF));

                u32AveMem = apstBeReg[k + u8BlkDev]->ISP_AE_AVER_GB_B_RDATA.u32;
                pstStat->stBEAeStat3.au16ZoneAvg[i][j + u8BlockOffset][2] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
                pstStat->stBEAeStat3.au16ZoneAvg[i][j + u8BlockOffset][3] = (HI_U16)((u32AveMem & 0xFFFF));
            }
        }

        u8BlockOffset += u8BlockZoneWidth;
    }

    return HI_SUCCESS;
}


HI_S32 ISP_DRV_BE_MG_StatisticsRead(ISP_STAT_S *pstStat, S_ISPBE_REGS_TYPE *apstBeReg[], HI_U8 u8BlkNum, HI_U8 u8BlkDev)
{
    HI_U8  k;
    HI_U8  u8BlockOffset     = 0;
    HI_U8  u8BlockZoneWidth  = 0;
    HI_U32 i, j;
    HI_U32 u32AveMem;

    ISP_CHECK_POINTER(pstStat);

    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_CHECK_POINTER(apstBeReg[k + u8BlkDev]);

        apstBeReg[k + u8BlkDev]->ISP_LA_AVER_RADDR.u32 = 0x0;

        for (i = 0; i < MG_ZONE_ROW; i++)
        {
            if (k < (MG_ZONE_COLUMN % DIV_0_TO_1(u8BlkNum)))
            {
                u8BlockZoneWidth = (MG_ZONE_COLUMN / DIV_0_TO_1(u8BlkNum)) + 1;
            }
            else
            {
                u8BlockZoneWidth = MG_ZONE_COLUMN / DIV_0_TO_1(u8BlkNum);
            }

            for (j = 0; j < u8BlockZoneWidth; j++)
            {
                u32AveMem = apstBeReg[k + u8BlkDev]->ISP_LA_AVER_RDATA.u32;
                pstStat->stMgStat.au16ZoneAvg[i][j + u8BlockOffset][0] = (HI_U16)((u32AveMem & 0xFF000000) >> 24);
                pstStat->stMgStat.au16ZoneAvg[i][j + u8BlockOffset][1] = (HI_U16)((u32AveMem & 0xFF0000) >> 16);
                pstStat->stMgStat.au16ZoneAvg[i][j + u8BlockOffset][2] = (HI_U16)((u32AveMem & 0xFF00) >> 8);
                pstStat->stMgStat.au16ZoneAvg[i][j + u8BlockOffset][3] = (HI_U16)((u32AveMem & 0xFF));
            }
        }

        u8BlockOffset += u8BlockZoneWidth;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_BE_AWB_StatisticsRead(ISP_STAT_S *pstStat, S_ISPBE_REGS_TYPE *apstBeReg[], HI_U8 u8BlkNum, HI_U8 u8BlkDev, ISP_STAT_KEY_U unStatkey)
{
    HI_S32 i, j, k;
    HI_U8  u8BlockOffset = 0;
    HI_U32 u32Value;
    HI_U64 u64MeteringAwbAvgR = 0;
    HI_U64 u64MeteringAwbAvgG = 0;
    HI_U64 u64MeteringAwbAvgB = 0;
    HI_U32 u32MeteringAwbCountAll = 0;
    HI_U32 u32Zone;
    HI_U32 u32Col, u32Row;
    HI_U32 u32ZoneBin;
    HI_U32 u32WholeCol = 0;

    ISP_CHECK_POINTER(pstStat);

    u8BlockOffset = 0;

    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_CHECK_POINTER(apstBeReg[k + u8BlkDev]);
        u32Zone = apstBeReg[k + u8BlkDev]->ISP_AWB_ZONE.u32;
        u32WholeCol  += (u32Zone & 0x3F);
    }

    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_CHECK_POINTER(apstBeReg[k + u8BlkDev]);
        u32Zone = apstBeReg[k + u8BlkDev]->ISP_AWB_ZONE.u32;
        u32Col  = (u32Zone & 0x3F);
        u32Row  = ((u32Zone & 0x3F00) >> 8);
        u32ZoneBin = apstBeReg[k + u8BlkDev]->ISP_AWB_ZONE_BIN.u32;
        if (unStatkey.bit1AwbStat1)
        {
            u64MeteringAwbAvgR += (HI_U64)(apstBeReg[k + u8BlkDev]->ISP_AWB_AVG_R.bits.isp_awb_avg_r * apstBeReg[k + u8BlkDev]->ISP_AWB_CNT_ALL.bits.isp_awb_count_all);
            u64MeteringAwbAvgG += (HI_U64)(apstBeReg[k + u8BlkDev]->ISP_AWB_AVG_G.bits.isp_awb_avg_g * apstBeReg[k + u8BlkDev]->ISP_AWB_CNT_ALL.bits.isp_awb_count_all);
            u64MeteringAwbAvgB += (HI_U64)(apstBeReg[k + u8BlkDev]->ISP_AWB_AVG_B.bits.isp_awb_avg_b * apstBeReg[k + u8BlkDev]->ISP_AWB_CNT_ALL.bits.isp_awb_count_all);
            u32MeteringAwbCountAll += apstBeReg[k + u8BlkDev]->ISP_AWB_CNT_ALL.bits.isp_awb_count_all;
        }

        if (unStatkey.bit1AwbStat2)
        {
            apstBeReg[k + u8BlkDev]->ISP_AWB_STAT_RADDR.u32 = 0x0;

            for (i = 0; i < u32Row; i++)
            {

                for (j = 0; j < u32Col; j++)
                {
                    HI_U16 u16WStartAddr = (i * u32WholeCol + j + u8BlockOffset) * u32ZoneBin;
                    HI_U16 m;

                    for (m = 0; m < u32ZoneBin; m++)
                    {
                        u32Value = apstBeReg[k + u8BlkDev]->ISP_AWB_STAT_RDATA.u32;
                        pstStat->stAwbStat2.au16MeteringMemArrayAvgR[u16WStartAddr + m] = (u32Value & 0xFFFF);
                        pstStat->stAwbStat2.au16MeteringMemArrayAvgG[u16WStartAddr + m] = ((u32Value >> 16) & 0xFFFF);
                        u32Value = apstBeReg[k + u8BlkDev]->ISP_AWB_STAT_RDATA.u32;
                        pstStat->stAwbStat2.au16MeteringMemArrayAvgB[u16WStartAddr + m] = (u32Value & 0xFFFF);
                        pstStat->stAwbStat2.au16MeteringMemArrayCountAll[u16WStartAddr + m] = ((u32Value >> 16) & 0xFFFF);
                    }
                }
            }
        }

        u8BlockOffset += u32Col;
    }

    pstStat->stAwbStat1.u16MeteringAwbAvgR = (HI_U16)(osal_div_u64(u64MeteringAwbAvgR, DIV_0_TO_1(u32MeteringAwbCountAll)));
    pstStat->stAwbStat1.u16MeteringAwbAvgG = (HI_U16)(osal_div_u64(u64MeteringAwbAvgG, DIV_0_TO_1(u32MeteringAwbCountAll)));
    pstStat->stAwbStat1.u16MeteringAwbAvgB = (HI_U16)(osal_div_u64(u64MeteringAwbAvgB, DIV_0_TO_1(u32MeteringAwbCountAll)));
    pstStat->stAwbStat1.u16MeteringAwbCountAll = (HI_U16)(u32MeteringAwbCountAll / DIV_0_TO_1(u8BlkNum));

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_BE_AF_StatisticsRead(ISP_STAT_S *pstStat, S_ISPBE_REGS_TYPE *apstBeReg[], HI_U8 u8BlkDev)
{
    HI_U32 i, j;
    HI_U32 u32BEAfStatData = 0;
    HI_U32 u32Zone, u32Col, u32Row;

    ISP_CHECK_POINTER(pstStat);
    ISP_CHECK_POINTER(apstBeReg[u8BlkDev]);

    u32Zone = apstBeReg[u8BlkDev]->ISP_AF_ZONE.u32;
    u32Col  = (u32Zone & 0x1F);
    u32Row  = ((u32Zone & 0x1F00) >> 8);

    apstBeReg[u8BlkDev]->ISP_AF_STAT_H1_RADDR.u32 = 0;
    apstBeReg[u8BlkDev]->ISP_AF_STAT_H2_RADDR.u32 = 0;
    apstBeReg[u8BlkDev]->ISP_AF_STAT_V1_RADDR.u32 = 0;
    apstBeReg[u8BlkDev]->ISP_AF_STAT_V2_RADDR.u32 = 0;
    apstBeReg[u8BlkDev]->ISP_AF_STAT_Y_RADDR.u32 = 0;

    for (i = 0; i < u32Row; i++)
    {
        for (j = 0; j < u32Col; j++)
        {
            u32BEAfStatData = apstBeReg[u8BlkDev]->ISP_AF_STAT_H1_RDATA.u32;
            pstStat->stBEAfStat.stZoneMetrics[0][i][j].u16h1 = (HI_U16)(0xFFFF & u32BEAfStatData);
            u32BEAfStatData = apstBeReg[u8BlkDev]->ISP_AF_STAT_H2_RDATA.u32;
            pstStat->stBEAfStat.stZoneMetrics[0][i][j].u16h2 = (HI_U16)(0xFFFF & u32BEAfStatData);
            u32BEAfStatData = apstBeReg[u8BlkDev]->ISP_AF_STAT_V1_RDATA.u32;
            pstStat->stBEAfStat.stZoneMetrics[0][i][j].u16v1 = (HI_U16)(0xFFFF & u32BEAfStatData);
            u32BEAfStatData = apstBeReg[u8BlkDev]->ISP_AF_STAT_V2_RDATA.u32;
            pstStat->stBEAfStat.stZoneMetrics[0][i][j].u16v2 = (HI_U16)(0xFFFF & u32BEAfStatData);
            u32BEAfStatData = apstBeReg[u8BlkDev]->ISP_AF_STAT_Y_RDATA.u32;
            pstStat->stBEAfStat.stZoneMetrics[0][i][j].u16HlCnt = (HI_U16)((0xFFFF0000 & u32BEAfStatData) >> 16);
            pstStat->stBEAfStat.stZoneMetrics[0][i][j].u16y  = (HI_U16)(0xFFFF & u32BEAfStatData);
            //u32BEAfStatData = apstBeReg[u8BlkDev]->ISP_AF_STAT_RDATA.u32;
        }
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_BE_DEHAZE_StatisticsRead(ISP_STAT_S *pstStat, S_ISPBE_REGS_TYPE *apstBeReg[], HI_U8 u8BlkNum, HI_U8 u8BlkDev)
{
    HI_U32 i, j, m;

    ISP_CHECK_POINTER(pstStat);

    j = DEFOG_ZONE_NUM / 2;

    for (i = 0; i < u8BlkNum; i++)
    {
        ISP_CHECK_POINTER(apstBeReg[i + u8BlkDev]);

        apstBeReg[i + u8BlkDev]->ISP_DEHAZE_MINSTAT_RADDR.u32 = 0x0;

        for (m = 0; m < j; m++)
        {
            pstStat->stDehazeStat.au32DehazeMinDout[i][m] = apstBeReg[i + u8BlkDev]->ISP_DEHAZE_MINSTAT_RDATA.u32;
        }

        apstBeReg[i + u8BlkDev]->ISP_DEHAZE_MAXSTAT_RADDR.u32 = 0x0;

        for (m = 0; m < DEFOG_ZONE_NUM; m++)
        {
            pstStat->stDehazeStat.au32DehazeMaxStatDout[i][m] = apstBeReg[i + u8BlkDev]->ISP_DEHAZE_MAXSTAT_RDATA.u32;
        }
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_BE_APB_StatisticsRead(ISP_STAT_S *pstStat, S_ISPBE_REGS_TYPE *apstBeReg[], HI_U8 u8BlkNum, HI_U8 u8BlkDev, ISP_STAT_KEY_U unStatkey)
{
    // BE comm statistics
    if (unStatkey.bit1CommStat)
    {
        ISP_DRV_BE_COMM_StatisticsRead(pstStat, apstBeReg, u8BlkDev);
    }

    //BE AE statistics
    if (unStatkey.bit1BEAeGloStat)
    {
        ISP_DRV_BE_AE_Global_StatisticsRead(pstStat, apstBeReg, u8BlkNum, u8BlkDev);
    }

    if (unStatkey.bit1BEAeLocStat)
    {
        ISP_DRV_BE_AE_Local_StatisticsRead(pstStat, apstBeReg, u8BlkNum, u8BlkDev);
    }

    if (unStatkey.bit1MgStat)
    {
        ISP_DRV_BE_MG_StatisticsRead(pstStat, apstBeReg, u8BlkNum, u8BlkDev);
    }

    /* BE AWB statistics */

    ISP_DRV_BE_AWB_StatisticsRead(pstStat, apstBeReg, u8BlkNum, u8BlkDev, unStatkey);

    /* BE AF statistics */
    if (unStatkey.bit1BEAfStat)
    {
        ISP_DRV_BE_AF_StatisticsRead(pstStat, apstBeReg, u8BlkDev);
    }
    if (unStatkey.bit1Dehaze)
    {
        ISP_DRV_BE_DEHAZE_StatisticsRead(pstStat, apstBeReg, u8BlkNum, u8BlkDev);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_BE_STT_StatisticsRead(VI_PIPE ViPipe, ISP_STAT_S *pstStat, S_ISPBE_REGS_TYPE *apstBeReg[], HI_U8 u8BlkNum, HI_U8 u8BlkDev, ISP_STAT_KEY_U unStatkey)
{
    return HI_SUCCESS;
}
/*read BE statistics information:offline*/

HI_S32 ISP_DRV_BE_AE_OfflineStatisticsRead(ISP_STAT_S *pstStat, S_ISP_STT_REGS_TYPE *apstBeStt[], HI_U8 u8BlkNum)
{
    HI_U8  k;
    HI_U8  u8BlockOffset    = 0;
    HI_U8  u8BlockZoneWidth = 0;
    HI_U32 i, j;
    HI_U32 u32AveMem;
    HI_U64 u64GlobalAvgSumR  = 0;
    HI_U64 u64GlobalAvgSumGr = 0;
    HI_U64 u64GlobalAvgSumGb = 0;
    HI_U64 u64GlobalAvgSumB  = 0;
    HI_U32 u32PixelWeightTmp = 0;
    HI_U32 u32PixelWeight    = 0;

    ISP_CHECK_POINTER(pstStat);

    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_CHECK_POINTER(apstBeStt[k]);

        for (i = 0; i < 1024; i++)
        {
            if (k == 0)
            {
                pstStat->stBEAeStat1.au32HistogramMemArray[i] = apstBeStt[k]->ISP_AE_HIST[i].u32;
            }
            else
            {
                pstStat->stBEAeStat1.au32HistogramMemArray[i] += apstBeStt[k]->ISP_AE_HIST[i].u32;
            }
        }

        if (k == 0)
        {
            u32PixelWeightTmp = apstBeStt[k]->ISP_AE_COUNT_STAT_RSTT.bits.isp_ae_count_pixels_stt;
            pstStat->stBEAeStat1.u32PixelWeight = u32PixelWeightTmp;
            pstStat->stBEAeStat1.u32PixelCount  = apstBeStt[k]->ISP_AE_TOTAL_STAT_RSTT.bits.isp_ae_total_pixels_stt;
        }
        else
        {
            u32PixelWeightTmp = apstBeStt[k]->ISP_AE_COUNT_STAT_RSTT.bits.isp_ae_count_pixels_stt;
            pstStat->stBEAeStat1.u32PixelWeight += u32PixelWeightTmp;
            pstStat->stBEAeStat1.u32PixelCount  += apstBeStt[k]->ISP_AE_TOTAL_STAT_RSTT.bits.isp_ae_total_pixels_stt;
        }

        u64GlobalAvgSumR  += ((HI_U64)apstBeStt[k]->ISP_AE_TOTAL_R_AVER_RSTT.u32)  * ((HI_U64)u32PixelWeightTmp);
        u64GlobalAvgSumGr += ((HI_U64)apstBeStt[k]->ISP_AE_TOTAL_GR_AVER_RSTT.u32) * ((HI_U64)u32PixelWeightTmp);
        u64GlobalAvgSumGb += ((HI_U64)apstBeStt[k]->ISP_AE_TOTAL_GB_AVER_RSTT.u32) * ((HI_U64)u32PixelWeightTmp);
        u64GlobalAvgSumB  += ((HI_U64)apstBeStt[k]->ISP_AE_TOTAL_B_AVER_RSTT.u32)  * ((HI_U64)u32PixelWeightTmp);

        for (i = 0; i < AE_ZONE_ROW; i++)
        {
            if (k < (AE_ZONE_COLUMN % DIV_0_TO_1(u8BlkNum)))
            {
                u8BlockZoneWidth = (AE_ZONE_COLUMN / DIV_0_TO_1(u8BlkNum)) + 1;
            }
            else
            {
                u8BlockZoneWidth = AE_ZONE_COLUMN / DIV_0_TO_1(u8BlkNum);
            }

            for (j = 0; j < u8BlockZoneWidth; j++)
            {
                u32AveMem = apstBeStt[k]->ISP_AE_AVER_R_GR[i * u8BlockZoneWidth + j].u32;
                pstStat->stBEAeStat3.au16ZoneAvg[i][j + u8BlockOffset][0] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
                pstStat->stBEAeStat3.au16ZoneAvg[i][j + u8BlockOffset][1] = (HI_U16)((u32AveMem & 0xFFFF));

                u32AveMem = apstBeStt[k]->ISP_AE_AVER_GB_B[i * u8BlockZoneWidth + j].u32;
                pstStat->stBEAeStat3.au16ZoneAvg[i][j + u8BlockOffset][2] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
                pstStat->stBEAeStat3.au16ZoneAvg[i][j + u8BlockOffset][3] = (HI_U16)((u32AveMem & 0xFFFF));
            }
        }

        u8BlockOffset += u8BlockZoneWidth;
    }

    u32PixelWeight = DIV_0_TO_1(pstStat->stBEAeStat1.u32PixelWeight);

    pstStat->stBEAeStat2.u16GlobalAvgR  = osal_div_u64(u64GlobalAvgSumR,  u32PixelWeight);
    pstStat->stBEAeStat2.u16GlobalAvgGr = osal_div_u64(u64GlobalAvgSumGr, u32PixelWeight);
    pstStat->stBEAeStat2.u16GlobalAvgGb = osal_div_u64(u64GlobalAvgSumGb, u32PixelWeight);
    pstStat->stBEAeStat2.u16GlobalAvgB  = osal_div_u64(u64GlobalAvgSumB,  u32PixelWeight);

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_BE_AE_Global_OfflineStatisticsRead(ISP_STAT_S *pstStat, S_ISP_STT_REGS_TYPE *apstBeStt[], HI_U8 u8BlkNum)
{
    HI_U32 i, k;
    HI_U64 u64GlobalAvgSumR  = 0;
    HI_U64 u64GlobalAvgSumGr = 0;
    HI_U64 u64GlobalAvgSumGb = 0;
    HI_U64 u64GlobalAvgSumB  = 0;
    HI_U32 u32PixelWeightTmp = 0;
    HI_U32 u32PixelWeight    = 0;

    ISP_CHECK_POINTER(pstStat);

    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_CHECK_POINTER(apstBeStt[k]);

        for (i = 0; i < 1024; i++)
        {
            if (k == 0)
            {
                pstStat->stBEAeStat1.au32HistogramMemArray[i] = apstBeStt[k]->ISP_AE_HIST[i].u32;
            }
            else
            {
                pstStat->stBEAeStat1.au32HistogramMemArray[i] += apstBeStt[k]->ISP_AE_HIST[i].u32;
            }
        }

        if (k == 0)
        {
            u32PixelWeightTmp = apstBeStt[k]->ISP_AE_COUNT_STAT_RSTT.bits.isp_ae_count_pixels_stt;
            pstStat->stBEAeStat1.u32PixelWeight = u32PixelWeightTmp;
            pstStat->stBEAeStat1.u32PixelCount  = apstBeStt[k]->ISP_AE_TOTAL_STAT_RSTT.bits.isp_ae_total_pixels_stt;
        }
        else
        {
            u32PixelWeightTmp = apstBeStt[k]->ISP_AE_COUNT_STAT_RSTT.bits.isp_ae_count_pixels_stt;
            pstStat->stBEAeStat1.u32PixelWeight += u32PixelWeightTmp;
            pstStat->stBEAeStat1.u32PixelCount  += apstBeStt[k]->ISP_AE_TOTAL_STAT_RSTT.bits.isp_ae_total_pixels_stt;
        }

        u64GlobalAvgSumR  += ((HI_U64)apstBeStt[k]->ISP_AE_TOTAL_R_AVER_RSTT.u32)  * ((HI_U64)u32PixelWeightTmp);
        u64GlobalAvgSumGr += ((HI_U64)apstBeStt[k]->ISP_AE_TOTAL_GR_AVER_RSTT.u32) * ((HI_U64)u32PixelWeightTmp);
        u64GlobalAvgSumGb += ((HI_U64)apstBeStt[k]->ISP_AE_TOTAL_GB_AVER_RSTT.u32) * ((HI_U64)u32PixelWeightTmp);
        u64GlobalAvgSumB  += ((HI_U64)apstBeStt[k]->ISP_AE_TOTAL_B_AVER_RSTT.u32)  * ((HI_U64)u32PixelWeightTmp);
    }

    u32PixelWeight = DIV_0_TO_1(pstStat->stBEAeStat1.u32PixelWeight);

    pstStat->stBEAeStat2.u16GlobalAvgR  = osal_div_u64(u64GlobalAvgSumR,  u32PixelWeight);
    pstStat->stBEAeStat2.u16GlobalAvgGr = osal_div_u64(u64GlobalAvgSumGr, u32PixelWeight);
    pstStat->stBEAeStat2.u16GlobalAvgGb = osal_div_u64(u64GlobalAvgSumGb, u32PixelWeight);
    pstStat->stBEAeStat2.u16GlobalAvgB  = osal_div_u64(u64GlobalAvgSumB,  u32PixelWeight);

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_BE_AE_Local_OfflineStatisticsRead(ISP_STAT_S *pstStat, S_ISP_STT_REGS_TYPE *apstBeStt[], HI_U8 u8BlkNum)
{
    HI_U32 i, j, k;
    HI_U8  u8BlockOffset    = 0;
    HI_U8  u8BlockZoneWidth = 0;
    HI_U32 u32AveMem;;

    ISP_CHECK_POINTER(pstStat);

    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_CHECK_POINTER(apstBeStt[k]);

        for (i = 0; i < AE_ZONE_ROW; i++)
        {
            if (k < (AE_ZONE_COLUMN % DIV_0_TO_1(u8BlkNum)))
            {
                u8BlockZoneWidth = (AE_ZONE_COLUMN / DIV_0_TO_1(u8BlkNum)) + 1;
            }
            else
            {
                u8BlockZoneWidth = AE_ZONE_COLUMN / DIV_0_TO_1(u8BlkNum);
            }

            for (j = 0; j < u8BlockZoneWidth; j++)
            {
                u32AveMem = apstBeStt[k]->ISP_AE_AVER_R_GR[i * u8BlockZoneWidth + j].u32;
                pstStat->stBEAeStat3.au16ZoneAvg[i][j + u8BlockOffset][0] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
                pstStat->stBEAeStat3.au16ZoneAvg[i][j + u8BlockOffset][1] = (HI_U16)((u32AveMem & 0xFFFF));

                u32AveMem = apstBeStt[k]->ISP_AE_AVER_GB_B[i * u8BlockZoneWidth + j].u32;
                pstStat->stBEAeStat3.au16ZoneAvg[i][j + u8BlockOffset][2] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
                pstStat->stBEAeStat3.au16ZoneAvg[i][j + u8BlockOffset][3] = (HI_U16)((u32AveMem & 0xFFFF));
            }
        }

        u8BlockOffset += u8BlockZoneWidth;
    }

    return HI_SUCCESS;
}


HI_S32 ISP_DRV_BE_MG_OfflineStatisticsRead(ISP_STAT_S *pstStat, S_ISP_STT_REGS_TYPE *apstBeStt[], HI_U8 u8BlkNum)
{
    HI_U8  k;
    HI_U8  u8BlockOffset    = 0;
    HI_U8  u8BlockZoneWidth = 0;
    HI_U32 i, j;
    HI_U32 u32AveMem;

    ISP_CHECK_POINTER(pstStat);

    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_CHECK_POINTER(apstBeStt[k]);

        for (i = 0; i < MG_ZONE_ROW; i++)
        {
            if (k < (MG_ZONE_COLUMN % DIV_0_TO_1(u8BlkNum)))
            {
                u8BlockZoneWidth = (MG_ZONE_COLUMN / DIV_0_TO_1(u8BlkNum)) + 1;
            }
            else
            {
                u8BlockZoneWidth = MG_ZONE_COLUMN / DIV_0_TO_1(u8BlkNum);
            }

            for (j = 0; j < u8BlockZoneWidth; j++)
            {
                u32AveMem = apstBeStt[k]->ISP_LA_AVER[i * u8BlockZoneWidth + j].u32;
                pstStat->stMgStat.au16ZoneAvg[i][j + u8BlockOffset][0] = (HI_U16)((u32AveMem & 0xFF000000) >> 24);
                pstStat->stMgStat.au16ZoneAvg[i][j + u8BlockOffset][1] = (HI_U16)((u32AveMem & 0xFF0000) >> 16);
                pstStat->stMgStat.au16ZoneAvg[i][j + u8BlockOffset][2] = (HI_U16)((u32AveMem & 0xFF00) >> 8);
                pstStat->stMgStat.au16ZoneAvg[i][j + u8BlockOffset][3] = (HI_U16)((u32AveMem & 0xFF));
            }
        }

        u8BlockOffset += u8BlockZoneWidth;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_BE_AWB_OfflineStatisticsRead(ISP_STAT_S *pstStat, S_ISPBE_REGS_TYPE *apstBeReg[], S_ISP_STT_REGS_TYPE *apstBeStt[], HI_U8 u8BlkNum, HI_U8 u8BlkDev, ISP_STAT_KEY_U unStatkey)
{

    HI_S32 i, j, k;
    HI_U8  u8BlockOffset    = 0;
    //HI_U8  u8BlockZoneWidth = 0;
    HI_U32 u32Value;
    HI_U64 u64MeteringAwbAvgR = 0;
    HI_U64 u64MeteringAwbAvgG = 0;
    HI_U64 u64MeteringAwbAvgB = 0;
    HI_U32 u32MeteringAwbCountAll = 0;
    HI_U32 u32Zone;
    HI_U32 u32Col, u32Row;
    HI_U32 u32ZoneBin;
    HI_U32 u32WholeCol = 0;

    ISP_CHECK_POINTER(pstStat);

    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_CHECK_POINTER(apstBeReg[k + u8BlkDev]);
        u32Zone = apstBeReg[k + u8BlkDev]->ISP_AWB_ZONE.u32;
        u32WholeCol  += (u32Zone & 0x3F);
    }
    for (k = 0; k < u8BlkNum; k++)
    {
        ISP_CHECK_POINTER(apstBeStt[k]);
        ISP_CHECK_POINTER(apstBeReg[k + u8BlkDev]);
        u32Zone = apstBeReg[k + u8BlkDev]->ISP_AWB_ZONE.u32;
        u32Col  = (u32Zone & 0x3F);
        u32Row  = ((u32Zone & 0x3F00) >> 8);
        u32ZoneBin = apstBeReg[k + u8BlkDev]->ISP_AWB_ZONE_BIN.u32;

        if (unStatkey.bit1AwbStat1)
        {
            u64MeteringAwbAvgR += (HI_U64)apstBeStt[k]->ISP_AWB_AVG_R_RSTT.bits.isp_awb_avg_r_stt \
                                  * apstBeStt[k]->ISP_AWB_CNT_ALL_RSTT.bits.isp_awb_count_all_stt;
            u64MeteringAwbAvgG += (HI_U64)apstBeStt[k]->ISP_AWB_AVG_G_RSTT.bits.isp_awb_avg_g_stt \
                                  * apstBeStt[k]->ISP_AWB_CNT_ALL_RSTT.bits.isp_awb_count_all_stt;
            u64MeteringAwbAvgB += (HI_U64)apstBeStt[k]->ISP_AWB_AVG_B_RSTT.bits.isp_awb_avg_b_stt \
                                  * apstBeStt[k]->ISP_AWB_CNT_ALL_RSTT.bits.isp_awb_count_all_stt;
            u32MeteringAwbCountAll += apstBeStt[k]->ISP_AWB_CNT_ALL_RSTT.bits.isp_awb_count_all_stt;
        }

        if (unStatkey.bit1AwbStat2)
        {
            for (i = 0; i < u32Row; i++)
            {
                //if (k < (u32Col % DIV_0_TO_1(u8BlkNum)))
                //{
                //    u8BlockZoneWidth = (u32Col / DIV_0_TO_1(u8BlkNum)) + 1;
                //}
                //else
                //{
                //    u8BlockZoneWidth = u32Col / DIV_0_TO_1(u8BlkNum);
                //}

                for (j = 0; j < u32Col; j++)
                {
                    HI_U16 u16WStartAddr = (i * u32WholeCol + j + u8BlockOffset) * u32ZoneBin;
                    HI_U16 u16RStartAddr = (i * u32WholeCol + j) * u32ZoneBin * 2;//
                    HI_U16 m;

                    for (m = 0; m < u32ZoneBin; m++)
                    {
                        u32Value = apstBeStt[k]->ISP_AWB_STAT[u16RStartAddr + 2 * m + 0].u32;
                        pstStat->stAwbStat2.au16MeteringMemArrayAvgR[u16WStartAddr + m] = (u32Value & 0xFFFF);
                        pstStat->stAwbStat2.au16MeteringMemArrayAvgG[u16WStartAddr + m] = ((u32Value >> 16) & 0xFFFF);

                        u32Value = apstBeStt[k]->ISP_AWB_STAT[u16RStartAddr + 2 * m + 1].u32;
                        pstStat->stAwbStat2.au16MeteringMemArrayAvgB[u16WStartAddr + m] = (u32Value & 0xFFFF);
                        pstStat->stAwbStat2.au16MeteringMemArrayCountAll[u16WStartAddr + m] = ((u32Value >> 16) & 0xFFFF);
                    }
                }
            }
        }

        u8BlockOffset += u32Col;
    }

    pstStat->stAwbStat1.u16MeteringAwbAvgR = (HI_U16)(osal_div_u64(u64MeteringAwbAvgR, DIV_0_TO_1(u32MeteringAwbCountAll)));
    pstStat->stAwbStat1.u16MeteringAwbAvgG = (HI_U16)(osal_div_u64(u64MeteringAwbAvgG, DIV_0_TO_1(u32MeteringAwbCountAll)));
    pstStat->stAwbStat1.u16MeteringAwbAvgB = (HI_U16)(osal_div_u64(u64MeteringAwbAvgB, DIV_0_TO_1(u32MeteringAwbCountAll)));
    pstStat->stAwbStat1.u16MeteringAwbCountAll = (HI_U16)(u32MeteringAwbCountAll / DIV_0_TO_1(u8BlkNum));

    return HI_SUCCESS;
}


HI_S32 ISP_DRV_BE_AF_OfflineStatisticsRead(ISP_STAT_S *pstStat, S_ISP_STT_REGS_TYPE *apstBeStt[])
{
    HI_U8  u8WdrChn, k;
    HI_U32 i, j;
    HI_U32 u32Col = AF_ZONE_COLUMN;
    HI_U32 u32Row = AF_ZONE_ROW;
    HI_U32 u32BEAfStatData    = 0;
    u8WdrChn = 1;

    ISP_CHECK_POINTER(pstStat);

    for (k = 0; k < u8WdrChn; k++)
    {
        ISP_CHECK_POINTER(apstBeStt[k]);

        for (i = 0; i < u32Row; i++)
        {
            for (j = 0; j < u32Col; j++)
            {
                u32BEAfStatData = apstBeStt[k]->ISP_AF_STAT_H1[i * AF_ZONE_COLUMN + j].u32;
                pstStat->stBEAfStat.stZoneMetrics[k][i][j].u16h1 = (HI_U16)(0xFFFF & u32BEAfStatData);
                u32BEAfStatData = apstBeStt[k]->ISP_AF_STAT_H2[i * AF_ZONE_COLUMN + j].u32;
                pstStat->stBEAfStat.stZoneMetrics[k][i][j].u16h2 = (HI_U16)(0xFFFF & u32BEAfStatData);
                u32BEAfStatData = apstBeStt[k]->ISP_AF_STAT_V1[i * AF_ZONE_COLUMN + j].u32;
                pstStat->stBEAfStat.stZoneMetrics[k][i][j].u16v1 = (HI_U16)(0xFFFF & u32BEAfStatData);
                u32BEAfStatData = apstBeStt[k]->ISP_AF_STAT_V2[i * AF_ZONE_COLUMN + j].u32;
                pstStat->stBEAfStat.stZoneMetrics[k][i][j].u16v2 = (HI_U16)(0xFFFF & u32BEAfStatData);
                u32BEAfStatData = apstBeStt[k]->ISP_AF_STAT_Y[i * AF_ZONE_COLUMN + j].u32;
                pstStat->stBEAfStat.stZoneMetrics[k][i][j].u16HlCnt = (HI_U16)((0xFFFF0000 & u32BEAfStatData) >> 16);
                pstStat->stBEAfStat.stZoneMetrics[k][i][j].u16y  = (HI_U16)(0xFFFF & u32BEAfStatData);
            }
        }
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_BE_Dehaze_OfflineStatisticsRead(ISP_STAT_S *pstStat, S_ISP_STT_REGS_TYPE *apstBeStt[], HI_U8 u8BlkNum)
{
    HI_U32 i, j, m;

    j = DEFOG_ZONE_NUM / 2;

    ISP_CHECK_POINTER(pstStat);

    for (i = 0; i < u8BlkNum; i++)
    {
        ISP_CHECK_POINTER(apstBeStt[i]);

        for (m = 0; m < j; m++)
        {
            pstStat->stDehazeStat.au32DehazeMinDout[i][m] = apstBeStt[i]->ISP_DEHAZE_MINSTAT[m].u32;
        }

        for (m = 0; m < DEFOG_ZONE_NUM; m++)
        {
            pstStat->stDehazeStat.au32DehazeMaxStatDout[i][m] = apstBeStt[i]->ISP_DEHAZE_MAXSTAT[m].u32;
        }
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_DPC_OfflineCalibInfoRead(ISP_STAT_S *pstStat, S_ISP_STT_REGS_TYPE *apstBeStt[], HI_U8 u8BlkNum)
{
    return HI_SUCCESS;
}

HI_S32 ISP_DRV_BeOfflineAEStitchGlobalStatisticsRead(ISP_STAT_S *pstStat, ISP_DRV_CTX_S *pstDrvCtx)
{
    HI_S32 i, k = 0;
    HI_U64 u64GlobalAvgSumR = 0;
    HI_U64 u64GlobalAvgSumGr = 0;
    HI_U64 u64GlobalAvgSumGb = 0;
    HI_U64 u64GlobalAvgSumB = 0;
    HI_U32 u32PixelWeightTmp = 0;
    HI_U32 u32PixelWeight = 0;

    S_ISP_STITCH_STT_REGS_TYPE *ptmp;

    ISP_CHECK_POINTER(pstStat);
    ISP_CHECK_POINTER(pstDrvCtx);

    for (k = 0; k < pstDrvCtx->stStitchAttr.u8StitchPipeNum; k++)
    {
        ptmp = (S_ISP_STITCH_STT_REGS_TYPE * )pstDrvCtx->astBeStitchBuf[k].pVirAddr;

        if (k == 0)
        {
            for (i = 0; i < 1024; i++)
            {
                pstStat->stStitchStat.stBEAeStat1.au32HistogramMemArray[i] = ptmp->ISP_AE_HIST[i].u32;
            }

            u32PixelWeightTmp = ptmp->ISP_AE_COUNT_STAT_RSTT.bits.isp_ae_count_pixels_stt;
            pstStat->stStitchStat.stBEAeStat1.u32PixelWeight = u32PixelWeightTmp;
            pstStat->stStitchStat.stBEAeStat1.u32PixelCount  = ptmp->ISP_AE_TOTAL_STAT_RSTT.bits.isp_ae_total_pixels_stt;
        }
        else
        {
            for (i = 0; i < 1024; i++)
            {
                pstStat->stStitchStat.stBEAeStat1.au32HistogramMemArray[i] += ptmp->ISP_AE_HIST[i].u32;
            }

            u32PixelWeightTmp = ptmp->ISP_AE_COUNT_STAT_RSTT.bits.isp_ae_count_pixels_stt;
            pstStat->stStitchStat.stBEAeStat1.u32PixelWeight += u32PixelWeightTmp;
            pstStat->stStitchStat.stBEAeStat1.u32PixelCount  += ptmp->ISP_AE_TOTAL_STAT_RSTT.bits.isp_ae_total_pixels_stt;
        }

        u64GlobalAvgSumR  += ((HI_U64)ptmp->ISP_AE_TOTAL_R_AVER_RSTT.u32)  * ((HI_U64)u32PixelWeightTmp);
        u64GlobalAvgSumGr += ((HI_U64)ptmp->ISP_AE_TOTAL_GR_AVER_RSTT.u32) * ((HI_U64)u32PixelWeightTmp);
        u64GlobalAvgSumGb += ((HI_U64)ptmp->ISP_AE_TOTAL_GB_AVER_RSTT.u32) * ((HI_U64)u32PixelWeightTmp);
        u64GlobalAvgSumB  += ((HI_U64)ptmp->ISP_AE_TOTAL_B_AVER_RSTT.u32)  * ((HI_U64)u32PixelWeightTmp);
    }

    u32PixelWeight = DIV_0_TO_1(pstStat->stStitchStat.stBEAeStat1.u32PixelWeight);

    pstStat->stStitchStat.stBEAeStat2.u16GlobalAvgR  = osal_div_u64(u64GlobalAvgSumR,  u32PixelWeight);
    pstStat->stStitchStat.stBEAeStat2.u16GlobalAvgGr = osal_div_u64(u64GlobalAvgSumGr, u32PixelWeight);
    pstStat->stStitchStat.stBEAeStat2.u16GlobalAvgGb = osal_div_u64(u64GlobalAvgSumGb, u32PixelWeight);
    pstStat->stStitchStat.stBEAeStat2.u16GlobalAvgB  = osal_div_u64(u64GlobalAvgSumB,  u32PixelWeight);

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_BeOfflineAEStitchLocalStatisticsRead(ISP_STAT_S *pstStat, ISP_DRV_CTX_S *pstDrvCtx)
{
    HI_S32 i, j, k = 0;
    HI_U32 u32AveMem;
    VI_PIPE ViPipeBind = 0;

    S_ISP_STITCH_STT_REGS_TYPE *ptmp;

    ISP_CHECK_POINTER(pstStat);
    ISP_CHECK_POINTER(pstDrvCtx);

    for (k = 0; k < pstDrvCtx->stStitchAttr.u8StitchPipeNum; k++)
    {
        ptmp = (S_ISP_STITCH_STT_REGS_TYPE * )pstDrvCtx->astBeStitchBuf[k].pVirAddr;
        ViPipeBind = pstDrvCtx->stStitchAttr.as8StitchBindId[k];

        if (ViPipeBind < VI_MAX_PIPE_NUM)
        {
            for (i = 0; i < AE_ZONE_ROW; i++)
            {
                for (j = 0; j < AE_ZONE_COLUMN; j++)
                {
                    u32AveMem = ptmp->ISP_AE_AVER_R_GR[i * AE_ZONE_COLUMN + j].u32;
                    pstStat->stStitchStat.stBEAeStat3.au16ZoneAvg[ViPipeBind][i][j][0] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
                    pstStat->stStitchStat.stBEAeStat3.au16ZoneAvg[ViPipeBind][i][j][1] = (HI_U16)((u32AveMem & 0xFFFF));

                    u32AveMem = ptmp->ISP_AE_AVER_GB_B[i * AE_ZONE_COLUMN + j].u32;
                    pstStat->stStitchStat.stBEAeStat3.au16ZoneAvg[ViPipeBind][i][j][2] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
                    pstStat->stStitchStat.stBEAeStat3.au16ZoneAvg[ViPipeBind][i][j][3] = (HI_U16)((u32AveMem & 0xFFFF));
                }
            }
        }
    }

    return HI_SUCCESS;
}


HI_S32 ISP_DRV_BeOfflineAWBStitchStatisticsRead(ISP_STAT_S *pstStat, S_ISPBE_REGS_TYPE *apstBeReg[], ISP_DRV_CTX_S *pstDrvCtx, HI_U8 u8BlkDev, ISP_STAT_KEY_U unStatkey)
{
    HI_U8  u8StitchNum       = 0;
    HI_U16 u16BlockZoneWidth = 0;
    HI_U16 u16StitchWidth    = 0;
    HI_S32 i, j, k = 0;
    HI_U32 u32Zone;
    HI_U32 u32Col, u32Row;
    HI_U32 u32ZoneBin;

    S_ISP_STITCH_STT_REGS_TYPE *ptmp;

    ISP_CHECK_POINTER(pstStat);
    ISP_CHECK_POINTER(pstDrvCtx);

    u8StitchNum = pstDrvCtx->stStitchAttr.u8StitchPipeNum;
    u32Zone = apstBeReg[0 + u8BlkDev]->ISP_AWB_ZONE.u32;
    u32Col  = (u32Zone & 0x3F);
    u32Row  = ((u32Zone & 0x3F00) >> 8);
    u32ZoneBin = apstBeReg[0 + u8BlkDev]->ISP_AWB_ZONE_BIN.u32;

    if (unStatkey.bit1AwbStat2)
    {
        //if ((u8StitchNum > 1) && (u8StitchNum <= 4))
        //{
        //    u16BlockZoneWidth = AWB_ZONE_ORIG_COLUMN;
        //    u16StitchWidth = u16BlockZoneWidth * u8StitchNum;
        //}
        //else if ((u8StitchNum > 4) && (u8StitchNum <= ISP_MAX_PIPE_NUM))
        //{
        //    u16BlockZoneWidth = AWB_ZONE_ORIG_COLUMN * 4  / DIV_0_TO_1(u8StitchNum);
        u16BlockZoneWidth = u32Col;
        u16StitchWidth = u16BlockZoneWidth * u8StitchNum;

        pstStat->stStitchStat.stAwbStat2.u16ZoneRow = u32Col;
        pstStat->stStitchStat.stAwbStat2.u16ZoneCol = u16StitchWidth;


        for (k = 0; k < u8StitchNum; k++)
        {
            ptmp = (S_ISP_STITCH_STT_REGS_TYPE * )pstDrvCtx->astBeStitchBuf[k].pVirAddr;

            for (i = 0; i < u32Row; i++)
            {
                for (j = 0; j < u16BlockZoneWidth; j++)
                {
                    HI_U16 u16WStartAddr = (u16StitchWidth * i + u16BlockZoneWidth * k + j) * u32ZoneBin;
                    HI_U16 u16RStartAddr = (u16BlockZoneWidth * i + j) * 2 * u32ZoneBin;
                    HI_U16 m;

                    for (m = 0; m < u32ZoneBin; m++)
                    {
                        HI_U32 u32StatGR, u32StatCountB;

                        u32StatGR       = ptmp->ISP_AWB_STAT[u16RStartAddr + (m * 2) + 0].u32;
                        u32StatCountB = ptmp->ISP_AWB_STAT[u16RStartAddr + (m * 2) + 1].u32;

                        pstStat->stStitchStat.stAwbStat2.au16MeteringMemArrayAvgR[u16WStartAddr + m]     =  (u32StatGR & 0xFFFF);
                        pstStat->stStitchStat.stAwbStat2.au16MeteringMemArrayAvgG[u16WStartAddr + m]     =  ((u32StatGR >> 16) & 0xFFFF);
                        pstStat->stStitchStat.stAwbStat2.au16MeteringMemArrayAvgB[u16WStartAddr + m]     =  (u32StatCountB & 0xFFFF);
                        pstStat->stStitchStat.stAwbStat2.au16MeteringMemArrayCountAll[u16WStartAddr + m] =  ((u32StatCountB >> 16) & 0xFFFF);
                    }
                }
            }
        }
    }

    return HI_SUCCESS;
}



HI_S32 ISP_DRV_BE_StitchStatisticsRead(VI_PIPE ViPipe, ISP_STAT_INFO_S *pstStatInfo)
{
    HI_S32 i, j, k = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U64 u64GlobalAvgSumR = 0;
    HI_U64 u64GlobalAvgSumGr = 0;
    HI_U64 u64GlobalAvgSumGb = 0;
    HI_U64 u64GlobalAvgSumB = 0;
    HI_U32 u32PixelWeightTmp = 0;
    HI_U32 u32PixelWeight = 0;
    HI_U8  u8StitchNum = 0;
    HI_U16 u16BlockZoneWidth = 0;
    HI_U16 u16StitchWidth = 0;
    VI_PIPE ViPipeBind = 0;
    HI_U32 u32Zone;
    HI_U32 u32Col, u32Row;
    HI_U32 u32ZoneBin;
    HI_U32 u32AveMem;

    ISP_STAT_KEY_U unStatkey;
    ISP_STAT_S *pstStat = HI_NULL;
    ISP_DRV_CTX_S *pstDrvCtx = HI_NULL;
    ISP_BE_REGS_ATTR_S stIspBeRegsAttr = {0};
    S_ISPBE_REGS_TYPE *apstBeReg[ISP_STRIPING_MAX_NUM] = {HI_NULL};

    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_POINTER(pstStatInfo);
    pstStat = (ISP_STAT_S *)pstStatInfo->pVirtAddr;

    if (HI_NULL == pstStat)
    {
        return HI_FAILURE;
    }

    pstDrvCtx = ISP_DRV_GET_CTX(ViPipe);

    if ((HI_TRUE != pstDrvCtx->stStitchAttr.bStitchEnable) || (HI_TRUE != pstDrvCtx->stStitchAttr.bMainPipe) )
    {
        return HI_SUCCESS;
    }

    u8StitchNum = pstDrvCtx->stStitchAttr.u8StitchPipeNum;
    u8StitchNum = (u8StitchNum >= ISP_MAX_BE_NUM) ? ISP_MAX_BE_NUM : u8StitchNum;

    //if ((u8StitchNum > 1) && (u8StitchNum <= 4))
    //{
    //    u16BlockZoneWidth = AWB_ZONE_ORIG_COLUMN;
    //    u16StitchWidth = u16BlockZoneWidth * u8StitchNum;
    //}
    //else if ((u8StitchNum > 4) && (u8StitchNum <= ISP_MAX_PIPE_NUM))
    //{
    //    u16BlockZoneWidth = AWB_ZONE_ORIG_COLUMN * 4  / DIV_0_TO_1(u8StitchNum);
    //    u16StitchWidth = u16BlockZoneWidth * u8StitchNum;
    //}

    //pstStat->stStitchStat.stAwbStat2.u16ZoneRow = AWB_ZONE_ORIG_ROW;
    //pstStat->stStitchStat.stAwbStat2.u16ZoneCol = u16StitchWidth;
    //pstStat->stStitchStat.stAwbStat2.u16ZoneBin = AWB_ZONE_BIN;

    pstStat->bBEUpdate = HI_TRUE;
    unStatkey.u64Key = pstStatInfo->unKey.bit32IsrAccess;

    for (k = 0; k < pstDrvCtx->stStitchAttr.u8StitchPipeNum; k++)
    {
        ViPipeBind = pstDrvCtx->stStitchAttr.as8StitchBindId[k];

        ISP_CHECK_PIPE(ViPipeBind);
        s32Ret = ISP_DRV_GetBeRegsAttr(ViPipeBind, apstBeReg, &stIspBeRegsAttr);

        if (HI_SUCCESS != s32Ret)
        {
            return s32Ret;
        }

        //BE AE statistics
        if (unStatkey.bit1BEAeStiGloStat)
        {
            apstBeReg[k]->ISP_AE_HIST_RADDR.u32 = 0x0;

            if (k == 0)
            {
                for (i = 0; i < 1024; i++)
                {
                    pstStat->stStitchStat.stBEAeStat1.au32HistogramMemArray[i] = apstBeReg[k]->ISP_AE_HIST_RDATA.u32;
                }

                u32PixelWeightTmp = apstBeReg[k]->ISP_AE_COUNT_STAT.bits.isp_ae_count_pixels;
                pstStat->stStitchStat.stBEAeStat1.u32PixelWeight = u32PixelWeightTmp;
                pstStat->stStitchStat.stBEAeStat1.u32PixelCount  = apstBeReg[k]->ISP_AE_TOTAL_STAT.bits.isp_ae_total_pixels;
            }
            else
            {
                for (i = 0; i < 1024; i++)
                {
                    pstStat->stStitchStat.stBEAeStat1.au32HistogramMemArray[i] += apstBeReg[k]->ISP_AE_HIST_RDATA.u32;
                }

                u32PixelWeightTmp = apstBeReg[k]->ISP_AE_COUNT_STAT.bits.isp_ae_count_pixels;
                pstStat->stStitchStat.stBEAeStat1.u32PixelWeight += u32PixelWeightTmp;
                pstStat->stStitchStat.stBEAeStat1.u32PixelCount  += apstBeReg[k]->ISP_AE_TOTAL_STAT.bits.isp_ae_total_pixels;
            }

            u64GlobalAvgSumR  += ((HI_U64)apstBeReg[k]->ISP_AE_TOTAL_R_AVER.u32)  * ((HI_U64)u32PixelWeightTmp);
            u64GlobalAvgSumGr += ((HI_U64)apstBeReg[k]->ISP_AE_TOTAL_GR_AVER.u32) * ((HI_U64)u32PixelWeightTmp);
            u64GlobalAvgSumGb += ((HI_U64)apstBeReg[k]->ISP_AE_TOTAL_GB_AVER.u32) * ((HI_U64)u32PixelWeightTmp);
            u64GlobalAvgSumB  += ((HI_U64)apstBeReg[k]->ISP_AE_TOTAL_B_AVER.u32)  * ((HI_U64)u32PixelWeightTmp);
        }

        if (unStatkey.bit1BEAeStiLocStat)
        {
            apstBeReg[k]->ISP_AE_AVER_R_GR_RADDR.u32 = 0x0;
            apstBeReg[k]->ISP_AE_AVER_GB_B_RADDR.u32 = 0x0;

            if (ViPipeBind < VI_MAX_PIPE_NUM)
            {
                for (i = 0; i < AE_ZONE_ROW; i++)
                {
                    for (j = 0; j < AE_ZONE_COLUMN; j++)
                    {
                        u32AveMem = apstBeReg[k]->ISP_AE_AVER_R_GR_RDATA.u32;
                        pstStat->stStitchStat.stBEAeStat3.au16ZoneAvg[ViPipeBind][i][j][0] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
                        pstStat->stStitchStat.stBEAeStat3.au16ZoneAvg[ViPipeBind][i][j][1] = (HI_U16)((u32AveMem & 0xFFFF));

                        u32AveMem = apstBeReg[k]->ISP_AE_AVER_GB_B_RDATA.u32;
                        pstStat->stStitchStat.stBEAeStat3.au16ZoneAvg[ViPipeBind][i][j][2] = (HI_U16)((u32AveMem & 0xFFFF0000) >> 16);
                        pstStat->stStitchStat.stBEAeStat3.au16ZoneAvg[ViPipeBind][i][j][3] = (HI_U16)((u32AveMem & 0xFFFF));
                    }
                }
            }
        }

        if (unStatkey.bit1AwbStat2)
        {
            u32Zone = apstBeReg[k]->ISP_AWB_ZONE.u32;
            u32Col  = (u32Zone & 0x3F);
            u32Row  = ((u32Zone & 0x3F00) >> 8);
            u32ZoneBin = apstBeReg[k]->ISP_AWB_ZONE_BIN.u32;
            u16BlockZoneWidth = u32Col;
            u16StitchWidth = u16BlockZoneWidth * u8StitchNum;
            pstStat->stStitchStat.stAwbStat2.u16ZoneRow = u32Row;
            pstStat->stStitchStat.stAwbStat2.u16ZoneCol = u16StitchWidth;
            pstStat->stStitchStat.stAwbStat2.u16ZoneBin = u32ZoneBin;
            apstBeReg[k]->ISP_AWB_STAT_RADDR.u32 = 0x0;

            for (i = 0; i < u32Row; i++)
            {
                for (j = 0; j < u16BlockZoneWidth; j++)
                {
                    HI_U16 u16WStartAddr = (u16StitchWidth * i + u16BlockZoneWidth * k + j) * u32ZoneBin;
                    HI_U32 u32StatGR, u32StatCountB, m;

                    for (m = 0; m < u32ZoneBin; m++)
                    {
                        u32StatGR     = apstBeReg[k]->ISP_AWB_STAT_RDATA.u32;
                        pstStat->stStitchStat.stAwbStat2.au16MeteringMemArrayAvgR[u16WStartAddr + m] = (u32StatGR & 0xFFFF);
                        pstStat->stStitchStat.stAwbStat2.au16MeteringMemArrayAvgG[u16WStartAddr + m] = ((u32StatGR >> 16) & 0xFFFF);

                        u32StatCountB = apstBeReg[k]->ISP_AWB_STAT_RDATA.u32;
                        pstStat->stStitchStat.stAwbStat2.au16MeteringMemArrayAvgB[u16WStartAddr + m] = (u32StatCountB & 0xFFFF);
                        pstStat->stStitchStat.stAwbStat2.au16MeteringMemArrayCountAll[u16WStartAddr + m] =  ((u32StatCountB >> 16) & 0xFFFF);
                    }
                }
            }
        }
    }

    if (unStatkey.bit1BEAeStiGloStat)
    {
        u32PixelWeight = DIV_0_TO_1(pstStat->stStitchStat.stBEAeStat1.u32PixelWeight);

        pstStat->stStitchStat.stBEAeStat2.u16GlobalAvgR  = osal_div_u64(u64GlobalAvgSumR,  u32PixelWeight);
        pstStat->stStitchStat.stBEAeStat2.u16GlobalAvgGr = osal_div_u64(u64GlobalAvgSumGr, u32PixelWeight);
        pstStat->stStitchStat.stBEAeStat2.u16GlobalAvgGb = osal_div_u64(u64GlobalAvgSumGb, u32PixelWeight);
        pstStat->stStitchStat.stBEAeStat2.u16GlobalAvgB  = osal_div_u64(u64GlobalAvgSumB,  u32PixelWeight);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_BeOfflineSttBufInit(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    HI_U32 i;
    ISP_DRV_CTX_S *pstDrvCtx = HI_NULL;
    HI_U64 u64PhyAddr, u64Size;
    HI_U8  *pu8VirAddr;
    unsigned long u32Flags;
    HI_U64 u64StitchPhyAddr, u64StitchSize;
    HI_U8  *pu8StitchVirAddr;
    HI_CHAR acName[MAX_MMZ_NAMELEN] = {0};
    HI_CHAR acStitchName[MAX_MMZ_NAMELEN] = {0};

    ISP_CHECK_PIPE(ViPipe);
    pstDrvCtx = ISP_DRV_GET_CTX(ViPipe);

    osal_snprintf(acName, sizeof(acName), "ISPBeStt[%d]", ViPipe);

    u64Size = sizeof(S_ISP_STT_REGS_TYPE);

    u64Size = (u64Size + 255) / 256 * 256; //256bytes align

    s32Ret = CMPI_MmzMallocCached(HI_NULL, acName, &u64PhyAddr, (HI_VOID **)&pu8VirAddr, u64Size * ISP_STRIPING_MAX_NUM);

    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "alloc ISP BeSttBuf err\n");
        return HI_ERR_ISP_NOMEM;
    }

    osal_memset(pu8VirAddr, 0, u64Size);

    osal_spin_lock_irqsave(&g_stIspLock[ViPipe], &u32Flags);

    for (i = 0; i < ISP_STRIPING_MAX_NUM; i++)
    {
        pstDrvCtx->astBeSttBuf[i].u64PhyAddr = u64PhyAddr + i * u64Size;
        pstDrvCtx->astBeSttBuf[i].pVirAddr   = (HI_VOID *)(pu8VirAddr + i * u64Size);
        pstDrvCtx->astBeSttBuf[i].u64Size    = u64Size;
    }

    osal_spin_unlock_irqrestore(&g_stIspLock[ViPipe], &u32Flags);

    if ((pstDrvCtx->stStitchAttr.bStitchEnable == HI_TRUE) && (pstDrvCtx->stStitchAttr.bMainPipe == HI_TRUE))
    {
        osal_snprintf(acStitchName, sizeof(acStitchName), "ISPStitStat[%d]", ViPipe);

        u64StitchSize = sizeof(S_ISP_STITCH_STT_REGS_TYPE) * pstDrvCtx->stStitchAttr.u8StitchPipeNum;

        s32Ret = CMPI_MmzMallocCached(HI_NULL, acStitchName, &u64StitchPhyAddr, (HI_VOID **)&pu8StitchVirAddr, u64StitchSize);
        if (HI_SUCCESS != s32Ret)
        {
            ISP_TRACE(HI_DBG_ERR, "alloc ISP stitch statistics buf err\n");
            return HI_ERR_ISP_NOMEM;
        }

        osal_memset(pu8StitchVirAddr, 0, u64StitchSize);

        osal_spin_lock_irqsave(&g_stIspLock[ViPipe], &u32Flags);

        for (i = 0; i < pstDrvCtx->stStitchAttr.u8StitchPipeNum; i++)
        {
            pstDrvCtx->astBeStitchBuf[i].u64PhyAddr = u64StitchPhyAddr + i * sizeof(S_ISP_STITCH_STT_REGS_TYPE);
            pstDrvCtx->astBeStitchBuf[i].pVirAddr = (HI_VOID *)(pu8StitchVirAddr + i * sizeof(S_ISP_STITCH_STT_REGS_TYPE));
        }

        osal_spin_unlock_irqrestore(&g_stIspLock[ViPipe], &u32Flags);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_BeOfflineSttBufExit(VI_PIPE ViPipe)
{
    HI_U32 i;
    HI_U64 u64PhyAddr;
    HI_VOID *pVirAddr;
    HI_U64 u64PhyStitchAddr;
    HI_VOID *pStitchVirAddr;
    unsigned long u32Flags;
    ISP_DRV_CTX_S *pstDrvCtx = HI_NULL;

    ISP_CHECK_PIPE(ViPipe);

    pstDrvCtx = ISP_DRV_GET_CTX(ViPipe);

    u64PhyAddr = pstDrvCtx->astBeSttBuf[0].u64PhyAddr;
    pVirAddr   = pstDrvCtx->astBeSttBuf[0].pVirAddr;

    u64PhyStitchAddr = pstDrvCtx->astBeStitchBuf[0].u64PhyAddr;
    pStitchVirAddr   = pstDrvCtx->astBeStitchBuf[0].pVirAddr;

    osal_spin_lock_irqsave(&g_stIspLock[ViPipe], &u32Flags);

    for (i = 0; i < ISP_STRIPING_MAX_NUM; i++)
    {
        pstDrvCtx->astBeSttBuf[i].u64PhyAddr = 0;
        pstDrvCtx->astBeSttBuf[i].u64Size = 0;
        pstDrvCtx->astBeSttBuf[i].pVirAddr = HI_NULL;
    }

    for (i = 0; i < ISP_STITCH_MAX_NUM; i++)
    {
        pstDrvCtx->astBeStitchBuf[0].u64PhyAddr = 0;
        pstDrvCtx->astBeStitchBuf[0].u64Size = 0;
        pstDrvCtx->astBeStitchBuf[0].pVirAddr = HI_NULL;

    }

    osal_spin_unlock_irqrestore(&g_stIspLock[ViPipe], &u32Flags);

    if (0 != u64PhyAddr)
    {
        CMPI_MmzFree(u64PhyAddr, pVirAddr);
    }

    if (0 != u64PhyStitchAddr)
    {
        CMPI_MmzFree(u64PhyStitchAddr, pStitchVirAddr);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_FeSttAddrInit(VI_PIPE ViPipe)
{
    return HI_SUCCESS;
}

HI_S32 ISP_DRV_FeStitchSttAddrInit(VI_PIPE ViPipe)
{
    return HI_SUCCESS;
}

HI_S32 ISP_DRV_SttBufInit(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    ISP_DRV_CTX_S *pstDrvCtx = HI_NULL;

    pstDrvCtx = ISP_DRV_GET_CTX(ViPipe);

    if (IS_OFFLINE_MODE(pstDrvCtx->stWorkMode.enIspRunningMode)\
        || IS_STRIPING_MODE(pstDrvCtx->stWorkMode.enIspRunningMode))
    {
        s32Ret = ISP_DRV_BeOfflineSttBufInit(ViPipe);

        if (HI_SUCCESS != s32Ret)
        {
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}

HI_S32 ISP_DRV_SttBufExit(VI_PIPE ViPipe)
{
    HI_S32 s32Ret;
    ISP_DRV_CTX_S *pstDrvCtx = HI_NULL;

    pstDrvCtx = ISP_DRV_GET_CTX(ViPipe);

    if (IS_OFFLINE_MODE(pstDrvCtx->stWorkMode.enIspRunningMode)\
        || IS_STRIPING_MODE(pstDrvCtx->stWorkMode.enIspRunningMode))
    {
        s32Ret = ISP_DRV_BeOfflineSttBufExit(ViPipe);

        if (HI_SUCCESS != s32Ret)
        {
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
