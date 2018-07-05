/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_lsc_fe.c
  Version       : Initial Draft
  Author        :
  Created       : 2016/08/17
  Last Modified :
  Description   : Lens Shading Correction Algorithms
  Function List :
  History       :
  1.Date        : 2016/08/17
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include "isp_alg.h"
#include "isp_ext_config.h"
#include "isp_config.h"
#include "isp_sensor.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define HI_ISP_FE_LSC_DEFAULT_WIDTH_OFFSET      0

static const  HI_U16  g_au16MeshGainDef[8] = {512, 256, 128, 64, 0, 0, 0, 0};

typedef struct hiISP_FeLSC
{
    HI_BOOL bFeLscEn;
    HI_BOOL bLscCoefUpdata;
    HI_BOOL bLutUpdate;

    HI_U8  u8MeshScale;
    HI_U16 u16MeshStrength;
    HI_U16 u16MeshWeight;

    HI_U16 au16DeltaX[HI_ISP_LSC_GRID_COL - 1];
    HI_U16 au16DeltaY[(HI_ISP_LSC_GRID_ROW - 1) / 2];
    HI_U16 au16InvX[HI_ISP_LSC_GRID_COL - 1];
    HI_U16 au16InvY[(HI_ISP_LSC_GRID_ROW - 1) / 2];

    HI_U32 u32Width;
    HI_U32 au32RGain[HI_ISP_LSC_GRID_POINTS];
    HI_U32 au32GrGain[HI_ISP_LSC_GRID_POINTS];
    HI_U32 au32GbGain[HI_ISP_LSC_GRID_POINTS];
    HI_U32 au32BGain[HI_ISP_LSC_GRID_POINTS];
} ISP_FeLSC_S;


ISP_FeLSC_S g_astFeLscCtx[ISP_MAX_PIPE_NUM] = {{0}};
#define FeLSC_GET_CTX(dev, pstCtx)   pstCtx = &g_astFeLscCtx[dev]

static HI_VOID geometricInvSizeFeLsc(ISP_FeLSC_S *pstFeLsc)
{
    HI_S32 i;

    for ( i = 0 ; i < (HI_ISP_LSC_GRID_COL - 1); i++ )
    {
        if ( 0 != pstFeLsc->au16DeltaX[i])
        {
            pstFeLsc->au16InvX[i] = (4096 * 1024 / pstFeLsc->au16DeltaX[i] + 512) >> 10;
        }
        else
        {
            pstFeLsc->au16InvX[i] = 0;
        }
    }


    for ( i = 0 ; i < ((HI_ISP_LSC_GRID_ROW - 1) / 2); i++ )
    {
        if ( 0 != pstFeLsc->au16DeltaY[i])
        {
            pstFeLsc->au16InvY[i] = (4096 * 1024 / pstFeLsc->au16DeltaY[i] + 512) >> 10;
        }
        else
        {
            pstFeLsc->au16InvY[i] = 0;
        }
    }

    return;
}

static HI_VOID FeLscStaticRegInit(VI_PIPE ViPipe, ISP_FE_LSC_STATIC_CFG_S *pstStaticRegCfg)
{
    pstStaticRegCfg->bResh          = HI_TRUE;
    pstStaticRegCfg->u8WinNumH      = HI_ISP_LSC_GRID_COL - 1;
    pstStaticRegCfg->u8WinNumV      = HI_ISP_LSC_GRID_ROW - 1;
    pstStaticRegCfg->u16WidthOffset = HI_ISP_FE_LSC_DEFAULT_WIDTH_OFFSET;

    return;
}

static HI_VOID FeLscUsrRegInit(VI_PIPE ViPipe, ISP_FE_LSC_USR_CFG_S *pstUsrRegCfg)
{
    ISP_FeLSC_S *pstFeLsc         = HI_NULL;

    FeLSC_GET_CTX(ViPipe, pstFeLsc);

    pstUsrRegCfg->bResh       = HI_TRUE;
    pstUsrRegCfg->bLutUpdate  = HI_TRUE;
    pstUsrRegCfg->u8MeshScale = pstFeLsc->u8MeshScale;
    pstUsrRegCfg->u16MeshStr  = pstFeLsc->u16MeshStrength;
    pstUsrRegCfg->u16Weight   = pstFeLsc->u16MeshWeight;

    memcpy(pstUsrRegCfg->au16DeltaX, pstFeLsc->au16DeltaX, sizeof(HI_U16) * (HI_ISP_LSC_GRID_COL - 1));
    memcpy(pstUsrRegCfg->au16InvX, pstFeLsc->au16InvX, sizeof(HI_U16) * (HI_ISP_LSC_GRID_COL - 1));

    memcpy(pstUsrRegCfg->au16DeltaY, pstFeLsc->au16DeltaY, sizeof(HI_U16) * (HI_ISP_LSC_GRID_ROW - 1) / 2);
    memcpy(pstUsrRegCfg->au16InvY, pstFeLsc->au16InvY, sizeof(HI_U16) * (HI_ISP_LSC_GRID_ROW - 1) / 2);

    memcpy(pstUsrRegCfg->au32RGain, pstFeLsc->au32RGain, sizeof(HI_U32) * HI_ISP_LSC_GRID_POINTS);
    memcpy(pstUsrRegCfg->au32GrGain, pstFeLsc->au32GrGain, sizeof(HI_U32) * HI_ISP_LSC_GRID_POINTS);
    memcpy(pstUsrRegCfg->au32GbGain, pstFeLsc->au32GbGain, sizeof(HI_U32) * HI_ISP_LSC_GRID_POINTS);
    memcpy(pstUsrRegCfg->au32BGain, pstFeLsc->au32BGain, sizeof(HI_U32) * HI_ISP_LSC_GRID_POINTS);

    return;
}

static HI_VOID FeLscRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    ISP_FeLSC_S *pstFeLsc = HI_NULL;
    FeLSC_GET_CTX(ViPipe, pstFeLsc);

    FeLscStaticRegInit(ViPipe, &pstRegCfg->stAlgRegCfg[0].stFeLscRegCfg.stStaticRegCfg);
    FeLscUsrRegInit(ViPipe, &pstRegCfg->stAlgRegCfg[0].stFeLscRegCfg.stUsrRegCfg);

    pstRegCfg->stAlgRegCfg[0].stFeLscRegCfg.bLscEn = pstFeLsc->bFeLscEn;

    pstRegCfg->unKey.bit1FeLscCfg = 1;

    return;
}

static HI_VOID FeLscExtRegsInitialize(VI_PIPE ViPipe)
{
    ISP_FeLSC_S *pstFeLsc = HI_NULL;
    FeLSC_GET_CTX(ViPipe, pstFeLsc);

    hi_ext_system_isp_fe_lsc_enable_write(ViPipe, pstFeLsc->bFeLscEn);
    hi_ext_system_isp_mesh_shading_fe_lut_attr_updata_write(ViPipe, HI_FALSE);
    hi_ext_system_isp_mesh_shading_fe_attr_updata_write(ViPipe, HI_FALSE);

    return;
}

static HI_VOID geometricGridSizeFeLsc(HI_U16 *pu16Delta, HI_U16 *pu16Inv, HI_U16 u16Length, HI_U16 u16GridSize)
{
    HI_U16 i, sum;
    HI_U16 u16HalfGridSize;
    HI_U16 diff;
    HI_U16 *pu16TmpStep = HI_NULL;
    HI_U16 u16SumR;

    u16HalfGridSize = (u16GridSize - 1) >> 1;

    if (0 == u16HalfGridSize)
    {
        return;
    }

    pu16TmpStep = (HI_U16 *)malloc(sizeof(HI_U16) * u16HalfGridSize);

    if (NULL  == pu16TmpStep)
    {
        return ;
    }

    u16SumR = u16HalfGridSize;

    for ( i = 0 ; i < u16HalfGridSize; i++)
    {
        pu16TmpStep[i] = (HI_U16)((((u16Length >> 1) * 1024 / DIV_0_TO_1(u16SumR)) + 512) >> 10);
    }

    sum = 0;
    for (i = 0; i < u16HalfGridSize; i++)
    {
        sum = sum + pu16TmpStep[i];
    }

    if (sum != (u16Length >> 1))
    {
        if (sum > (u16Length >> 1))
        {
            diff = sum - (u16Length >> 1);
            for (i = 1; i <= diff; i++)
            {
                pu16TmpStep[u16HalfGridSize - i] = pu16TmpStep[u16HalfGridSize - i] - 1;
            }
        }
        else
        {
            diff = (u16Length >> 1) - sum;
            for (i = 1; i <= diff; i++)
            {
                pu16TmpStep[i - 1] = pu16TmpStep[i - 1] + 1;
            }
        }
    }

    for ( i = 0 ; i < u16HalfGridSize; i++ )
    {
        pu16Delta[i] = pu16TmpStep[i];
        pu16Inv[i]   = (pu16Delta[i] == 0) ? (0) : ((4096 * 1024 / DIV_0_TO_1(pu16Delta[i]) + 512) >> 10);
    }

    free(pu16TmpStep);

    return;
}

static HI_VOID FeLscImageSize(VI_PIPE ViPipe, ISP_FeLSC_S *pstFeLsc)
{
    HI_U8      i;
    HI_U32     u32Width, u32Height;
    ISP_CTX_S  *pstIspCtx  = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    u32Width  = pstIspCtx->stBlockAttr.stFrameRect.u32Width;
    u32Height = pstIspCtx->stBlockAttr.stFrameRect.u32Height;

    geometricGridSizeFeLsc(pstFeLsc->au16DeltaX, pstFeLsc->au16InvX, u32Width / 2, HI_ISP_LSC_GRID_COL);
    geometricGridSizeFeLsc(pstFeLsc->au16DeltaY, pstFeLsc->au16InvY, u32Height / 2, HI_ISP_LSC_GRID_ROW);

    for (i = 0; i < (HI_ISP_LSC_GRID_COL - 1) / 2; i++)
    {
        pstFeLsc->au16DeltaX[HI_ISP_LSC_GRID_COL - 2 - i] = pstFeLsc->au16DeltaX[i];
        pstFeLsc->au16InvX[HI_ISP_LSC_GRID_COL - 2 - i]   = pstFeLsc->au16InvX[i];
    }

    return;
}

static HI_VOID FeLscInitialize(VI_PIPE ViPipe)
{
    HI_U16     i;
    HI_U32     u32DefGain;
    ISP_FeLSC_S *pstFeLsc          = HI_NULL;
    ISP_CMOS_DEFAULT_S *pstSnsDft  = HI_NULL;
    ISP_CMOS_LSC_S     *pstCmosLsc = HI_NULL;
    ISP_CTX_S          *pstIspCtx  = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    FeLSC_GET_CTX(ViPipe, pstFeLsc);
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);

    pstFeLsc->u16MeshStrength = HI_ISP_LSC_DEFAULT_MESH_STRENGTH;
    pstFeLsc->u16MeshWeight   = HI_ISP_LSC_DEFAULT_WEIGHT;

    pstCmosLsc = &pstSnsDft->stLsc;

    if (pstCmosLsc->bValid)
    {
        pstFeLsc->u8MeshScale     = pstCmosLsc->u8MeshScale;

        for (i = 0; i < HI_ISP_LSC_GRID_POINTS; i++)
        {
            pstFeLsc->au32RGain[i]  = (pstCmosLsc->stLscCalibTable[1].au16R_Gain[i]  << 10) + pstCmosLsc->stLscCalibTable[0].au16R_Gain[i];
            pstFeLsc->au32GrGain[i] = (pstCmosLsc->stLscCalibTable[1].au16Gr_Gain[i] << 10) + pstCmosLsc->stLscCalibTable[0].au16Gr_Gain[i];
            pstFeLsc->au32GbGain[i] = (pstCmosLsc->stLscCalibTable[1].au16Gb_Gain[i] << 10) + pstCmosLsc->stLscCalibTable[0].au16Gb_Gain[i];
            pstFeLsc->au32BGain[i]  = (pstCmosLsc->stLscCalibTable[1].au16B_Gain[i]  << 10) + pstCmosLsc->stLscCalibTable[0].au16B_Gain[i];
        }
    }
    else
    {
        pstFeLsc->u8MeshScale = HI_ISP_LSC_DEFAULT_MESH_SCALE;
        u32DefGain = (g_au16MeshGainDef[pstFeLsc->u8MeshScale] << 10) + g_au16MeshGainDef[pstFeLsc->u8MeshScale];
        for (i = 0; i < HI_ISP_LSC_GRID_POINTS; i++)
        {
            pstFeLsc->au32RGain[i]  = u32DefGain;
            pstFeLsc->au32GrGain[i] = u32DefGain;
            pstFeLsc->au32GbGain[i] = u32DefGain;
            pstFeLsc->au32BGain[i]  = u32DefGain;
        }
    }

    FeLscImageSize(ViPipe, pstFeLsc);

    if ( DYNAMIC_RANGE_XDR == pstIspCtx->stHdrAttr.enDynamicRange )
    {
        pstFeLsc->bFeLscEn = HI_FALSE;
    }
    else
    {
        pstFeLsc->bFeLscEn = HI_TRUE;
    }

    return;
}

static HI_VOID FeLscReadExtRegs(VI_PIPE ViPipe)
{
    HI_U16  i;
    HI_U16  au16DeltaX[(HI_ISP_LSC_GRID_COL - 1) / 2];
    HI_U16  r_gain0, r_gain1, gr_gain0, gr_gain1, gb_gain0, gb_gain1, b_gain0, b_gain1;
    ISP_FeLSC_S *pstFeLsc    = HI_NULL;
    FeLSC_GET_CTX(ViPipe, pstFeLsc);

    pstFeLsc->bLscCoefUpdata  = hi_ext_system_isp_mesh_shading_fe_attr_updata_read(ViPipe);
    hi_ext_system_isp_mesh_shading_fe_attr_updata_write(ViPipe, HI_FALSE);

    if (pstFeLsc->bLscCoefUpdata)
    {
        pstFeLsc->u16MeshStrength = hi_ext_system_isp_mesh_shading_mesh_strength_read(ViPipe);
        pstFeLsc->u16MeshWeight   = hi_ext_system_isp_mesh_shading_blendratio_read(ViPipe);
    }

    pstFeLsc->bLutUpdate      = hi_ext_system_isp_mesh_shading_fe_lut_attr_updata_read(ViPipe);
    hi_ext_system_isp_mesh_shading_fe_lut_attr_updata_write(ViPipe, HI_FALSE);

    if (pstFeLsc->bLutUpdate)
    {
        pstFeLsc->u8MeshScale = hi_ext_system_isp_mesh_shading_mesh_scale_read(ViPipe);

        for (i = 0; i < (HI_ISP_LSC_GRID_COL - 1) / 2; i++)
        {
            au16DeltaX[i] = hi_ext_system_isp_mesh_shading_xgrid_read(ViPipe, i);
        }

        for (i = 0; i < (HI_ISP_LSC_GRID_COL - 1) / 2; i++)
        {
            pstFeLsc->au16DeltaX[i] = au16DeltaX[i];
        }

        for (i = (HI_ISP_LSC_GRID_COL - 1) / 2; i < HI_ISP_LSC_GRID_COL - 1; i++)
        {
            pstFeLsc->au16DeltaX[i] = au16DeltaX[HI_ISP_LSC_GRID_COL - 2 - i];
        }

        for ( i = 0; i < (HI_ISP_LSC_GRID_ROW - 1) / 2; i++)
        {
            pstFeLsc->au16DeltaY[i] = hi_ext_system_isp_mesh_shading_ygrid_read(ViPipe, i);
        }

        geometricInvSizeFeLsc(pstFeLsc);

        for ( i = 0; i < HI_ISP_LSC_GRID_POINTS; i++)
        {

            r_gain0  = hi_ext_system_isp_mesh_shading_r_gain0_read(ViPipe, i);
            r_gain1  = hi_ext_system_isp_mesh_shading_r_gain1_read(ViPipe, i);

            gr_gain0 = hi_ext_system_isp_mesh_shading_gr_gain0_read(ViPipe, i);
            gr_gain1 = hi_ext_system_isp_mesh_shading_gr_gain1_read(ViPipe, i);

            gb_gain0 = hi_ext_system_isp_mesh_shading_gb_gain0_read(ViPipe, i);
            gb_gain1 = hi_ext_system_isp_mesh_shading_gb_gain1_read(ViPipe, i);

            b_gain0  = hi_ext_system_isp_mesh_shading_b_gain0_read(ViPipe, i);
            b_gain1  = hi_ext_system_isp_mesh_shading_b_gain1_read(ViPipe, i);

            pstFeLsc->au32RGain[i]  = (r_gain1  << 10) + r_gain0;
            pstFeLsc->au32GrGain[i] = (gr_gain1 << 10) + gr_gain0;
            pstFeLsc->au32GbGain[i] = (gb_gain1 << 10) + gb_gain0;
            pstFeLsc->au32BGain[i]  = (b_gain1  << 10) + b_gain0;
        }
    }

    return;
}

static HI_VOID FeLsc_Usr_Fw(ISP_FeLSC_S *pstFeLsc, ISP_FE_LSC_USR_CFG_S *pstUsrRegCfg)
{
    pstUsrRegCfg->bResh      = HI_TRUE;
    pstUsrRegCfg->bLutUpdate = HI_TRUE;
    pstUsrRegCfg->u8MeshScale = pstFeLsc->u8MeshScale;

    memcpy(pstUsrRegCfg->au16DeltaX, pstFeLsc->au16DeltaX, sizeof(HI_U16) * (HI_ISP_LSC_GRID_COL - 1));
    memcpy(pstUsrRegCfg->au16InvX, pstFeLsc->au16InvX, sizeof(HI_U16) * (HI_ISP_LSC_GRID_COL - 1));

    memcpy(pstUsrRegCfg->au16DeltaY, pstFeLsc->au16DeltaY, sizeof(HI_U16) * (HI_ISP_LSC_GRID_ROW - 1) / 2);
    memcpy(pstUsrRegCfg->au16InvY, pstFeLsc->au16InvY, sizeof(HI_U16) * (HI_ISP_LSC_GRID_ROW - 1) / 2);

    memcpy(pstUsrRegCfg->au32RGain, pstFeLsc->au32RGain, sizeof(HI_U32) * HI_ISP_LSC_GRID_POINTS);
    memcpy(pstUsrRegCfg->au32GrGain, pstFeLsc->au32GrGain, sizeof(HI_U32) * HI_ISP_LSC_GRID_POINTS);
    memcpy(pstUsrRegCfg->au32GbGain, pstFeLsc->au32GbGain, sizeof(HI_U32) * HI_ISP_LSC_GRID_POINTS);
    memcpy(pstUsrRegCfg->au32BGain, pstFeLsc->au32BGain, sizeof(HI_U32) * HI_ISP_LSC_GRID_POINTS);

    return;
}

static HI_S32 FeLscImageResWrite(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    ISP_FeLSC_S *pstFeLsc   = HI_NULL;

    FeLSC_GET_CTX(ViPipe, pstFeLsc);

    FeLscImageSize(ViPipe, pstFeLsc);

    FeLsc_Usr_Fw(pstFeLsc, &pstRegCfg->stAlgRegCfg[0].stFeLscRegCfg.stUsrRegCfg);

    pstRegCfg->unKey.bit1FeLscCfg = 1;

    return HI_SUCCESS;
}

static HI_VOID ISP_FeLscWdrModeSet(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    pstRegCfg->unKey.bit1FeLscCfg = 1;
    pstRegCfg->stAlgRegCfg[0].stFeLscRegCfg.stStaticRegCfg.bResh   = HI_TRUE;
    pstRegCfg->stAlgRegCfg[0].stFeLscRegCfg.stUsrRegCfg.bResh      = HI_TRUE;
    pstRegCfg->stAlgRegCfg[0].stFeLscRegCfg.stUsrRegCfg.bLutUpdate = HI_TRUE;

    return;
}

HI_S32 ISP_FeLscInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    FeLscInitialize(ViPipe);
    FeLscRegsInitialize(ViPipe, pstRegCfg);
    FeLscExtRegsInitialize(ViPipe);

    return HI_SUCCESS;
}

static HI_BOOL __inline CheckFeLscOpen(ISP_FeLSC_S *pstFeLsc)
{
    return (HI_TRUE == pstFeLsc->bFeLscEn);
}

HI_S32 ISP_FeLscRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                    HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    ISP_FeLSC_S *pstFeLsc = HI_NULL;
    ISP_CTX_S *pstIspCtx  = HI_NULL;
    ISP_REG_CFG_S *pstRegCfg  = (ISP_REG_CFG_S *)pRegCfg;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    FeLSC_GET_CTX(ViPipe, pstFeLsc);

    /* calculate every two interrupts */
    if ((0 != pstIspCtx->u32FrameCnt % 2) && (HI_TRUE != pstIspCtx->stLinkage.bSnapState))
    {
        return HI_SUCCESS;
    }

    pstFeLsc->bFeLscEn = hi_ext_system_isp_mesh_shading_enable_read(ViPipe);//hi_ext_system_isp_fe_lsc_enable_read(ViPipe);

    pstRegCfg->stAlgRegCfg[0].stFeLscRegCfg.bLscEn = pstFeLsc->bFeLscEn;
    pstRegCfg->unKey.bit1FeLscCfg = 1;

    if (!CheckFeLscOpen(pstFeLsc))
    {
        return HI_SUCCESS;
    }

    FeLscReadExtRegs(ViPipe);

    if (pstFeLsc->bLscCoefUpdata)
    {
        pstRegCfg->stAlgRegCfg[0].stFeLscRegCfg.stUsrRegCfg.bResh      = HI_TRUE;
        pstRegCfg->stAlgRegCfg[0].stFeLscRegCfg.stUsrRegCfg.u16MeshStr = pstFeLsc->u16MeshStrength;
        pstRegCfg->stAlgRegCfg[0].stFeLscRegCfg.stUsrRegCfg.u16Weight  = pstFeLsc->u16MeshWeight;
    }

    if (pstFeLsc->bLutUpdate)
    {
        FeLsc_Usr_Fw(pstFeLsc, &pstRegCfg->stAlgRegCfg[0].stFeLscRegCfg.stUsrRegCfg);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_FeLscCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);

    switch (u32Cmd)
    {
        case ISP_WDR_MODE_SET :
            ISP_FeLscWdrModeSet(ViPipe, (HI_VOID *)&pRegCfg->stRegCfg);
            break;
        case ISP_CHANGE_IMAGE_MODE_SET:
            FeLscImageResWrite(ViPipe, &pRegCfg->stRegCfg);
            break;
        default :
            break;
    }
    return HI_SUCCESS;
}

HI_S32 ISP_FeLscExit(VI_PIPE ViPipe)
{
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);

    pRegCfg->stRegCfg.stAlgRegCfg[0].stFeLscRegCfg.bLscEn = HI_FALSE;
    pRegCfg->stRegCfg.unKey.bit1FeLscCfg                  = 1;

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterFeLsc(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_FeLSC;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_FeLscInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_FeLscRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_FeLscCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_FeLscExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
