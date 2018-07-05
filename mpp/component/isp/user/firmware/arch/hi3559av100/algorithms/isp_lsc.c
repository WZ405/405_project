/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_lsc.c
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


#define HI_ISP_LSC_DEFAULT_WIDTH_OFFSET     0

/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/
static const  HI_U16  g_au16MeshGainDef[8] = {512, 256, 128, 64, 0, 0, 0, 0};

typedef struct hiISP_LSC
{
    HI_BOOL bLscEnable;
    HI_BOOL bLscCoefUpdata;
    HI_BOOL bLutUpdate;

    HI_U8  u8MeshScale;
    HI_U16 u16MeshStrength;
    HI_U16 u16MeshWeight;
    HI_U16 au16FirstPointPosX[ISP_STRIPING_MAX_NUM];//0:left  1:right
    HI_U16 au16CurWidth[ISP_STRIPING_MAX_NUM];//0:left  1:right
    HI_U16 u16BlkColStart;
    HI_U16 u16BlkColEnd;

    HI_U16 au16DeltaX[HI_ISP_LSC_GRID_COL - 1];
    HI_U16 au16DeltaY[(HI_ISP_LSC_GRID_ROW - 1) / 2];
    HI_U16 au16InvX[HI_ISP_LSC_GRID_COL - 1];
    HI_U16 au16InvY[(HI_ISP_LSC_GRID_ROW - 1) / 2];

    HI_U32 au32RGain[HI_ISP_LSC_GRID_POINTS];
    HI_U32 au32GrGain[HI_ISP_LSC_GRID_POINTS];
    HI_U32 au32GbGain[HI_ISP_LSC_GRID_POINTS];
    HI_U32 au32BGain[HI_ISP_LSC_GRID_POINTS];
    //ISP_LSC_CABLI_TABLE_S stLscGainInfo[2];
} ISP_LSC_S;

ISP_LSC_S g_astLscCtx[ISP_MAX_PIPE_NUM] = {{0}};
#define LSC_GET_CTX(dev, pstCtx)   pstCtx = &g_astLscCtx[dev]

static HI_VOID geometricInvSize(ISP_LSC_S *pstLsc)
{
    HI_S32 i;

    for ( i = 0 ; i < (HI_ISP_LSC_GRID_COL - 1); i++ )
    {
        if ( 0 != pstLsc->au16DeltaX[i])
        {
            pstLsc->au16InvX[i] = (4096 * 1024 / pstLsc->au16DeltaX[i] + 512) >> 10;
        }
        else
        {
            pstLsc->au16InvX[i] = 0;
        }
    }

    for ( i = 0 ; i < ((HI_ISP_LSC_GRID_ROW - 1) / 2); i++ )
    {
        if ( 0 != pstLsc->au16DeltaY[i])
        {
            pstLsc->au16InvY[i] = (4096 * 1024 / pstLsc->au16DeltaY[i] + 512) >> 10;
        }
        else
        {
            pstLsc->au16InvY[i] = 0;
        }
    }

    return;
}


HI_VOID LscGetLutIndex(HI_U8 u8CurBlk, ISP_LSC_S *pstLsc, ISP_LSC_USR_CFG_S *pstUsrRegCfg)
{
    HI_U16 u16Width = pstLsc->au16CurWidth[u8CurBlk] >> 1;
    HI_U16 u16FirstPointPosX = pstLsc->au16FirstPointPosX[u8CurBlk] >> 1;
    HI_U16 u16LastPointPosX;
    HI_U16 u16WidthSumOfBlk;
    HI_U16 u16Dis2Right, u16Dis2Left;
    HI_U16 u16BlkColStart;
    HI_U16 u16BlkColEnd;

    u16BlkColStart      = 0;
    u16BlkColEnd        = 0;
    u16WidthSumOfBlk    = pstLsc->au16DeltaX[0];

    while ((u16FirstPointPosX >= u16WidthSumOfBlk) && (u16FirstPointPosX != 0))
    {
        u16BlkColStart++;
        u16WidthSumOfBlk += pstLsc->au16DeltaX[u16BlkColStart];
    }

    u16Dis2Right = u16WidthSumOfBlk - u16FirstPointPosX;
    u16Dis2Left  = pstLsc->au16DeltaX[u16BlkColStart] - u16Dis2Right;

    pstUsrRegCfg->u16WidthOffset = u16Dis2Left;

    u16LastPointPosX = u16FirstPointPosX + u16Width;
    u16BlkColEnd     = u16BlkColStart;
    while (u16LastPointPosX >  u16WidthSumOfBlk)
    {
        u16BlkColEnd++;
        u16WidthSumOfBlk += pstLsc->au16DeltaX[u16BlkColEnd];
    }
    u16BlkColEnd += 1;

    pstLsc->u16BlkColStart  = u16BlkColStart;
    pstLsc->u16BlkColEnd    = u16BlkColEnd;

    return;
}

static HI_VOID LscGetGainLut(ISP_LSC_S *pstLsc, ISP_LSC_USR_CFG_S *pstUsrRegCfg)
{
    HI_U16 i, j;
    HI_U16 u16BlkColEnd;
    HI_U16 u16BlkColStart;
    HI_U16 u16IndexOffset;

    u16BlkColEnd   = pstLsc->u16BlkColEnd;
    u16BlkColStart = pstLsc->u16BlkColStart;

    for (j = 0; j < HI_ISP_LSC_GRID_ROW; j++)
    {
        for (i = 0; i <= (u16BlkColEnd - u16BlkColStart); i++)
        {
            u16IndexOffset = j * HI_ISP_LSC_GRID_COL;

            pstUsrRegCfg->au32RGain[u16IndexOffset + i]  = pstLsc->au32RGain[u16IndexOffset  + u16BlkColStart + i];
            pstUsrRegCfg->au32GrGain[u16IndexOffset + i] = pstLsc->au32GrGain[u16IndexOffset + u16BlkColStart + i];
            pstUsrRegCfg->au32GbGain[u16IndexOffset + i] = pstLsc->au32GbGain[u16IndexOffset + u16BlkColStart + i];
            pstUsrRegCfg->au32BGain[u16IndexOffset + i]  = pstLsc->au32BGain[u16IndexOffset  + u16BlkColStart + i] ;
        }
    }

    for (i = 0; i < (u16BlkColEnd - u16BlkColStart); i++)
    {
        pstUsrRegCfg->au16DeltaX[i] = pstLsc->au16DeltaX[u16BlkColStart + i];
        pstUsrRegCfg->au16InvX[i]   = pstLsc->au16InvX[u16BlkColStart   + i];
    }

    for (i = 0; i < (HI_ISP_LSC_GRID_ROW - 1) / 2; i++)
    {
        pstUsrRegCfg->au16DeltaY[i] = pstLsc->au16DeltaY[i];
        pstUsrRegCfg->au16InvY[i]   = pstLsc->au16InvY[i];
    }

    return;
}

static HI_VOID LscStaticRegsInitialize(HI_U8 i, VI_PIPE ViPipe, ISP_LSC_STATIC_CFG_S *pstLscStaticRegCfg)
{
    pstLscStaticRegCfg->u8WinNumH   = HI_ISP_LSC_GRID_COL - 1;
    pstLscStaticRegCfg->u8WinNumV   = HI_ISP_LSC_GRID_ROW - 1;

    pstLscStaticRegCfg->bStaticResh = HI_TRUE;

    return;
}

static HI_VOID LscUsrRegsInitialize(HI_U8 u8CurBlk, VI_PIPE ViPipe, ISP_LSC_USR_CFG_S *pstUsrRegCfg)
{
    ISP_LSC_S  *pstLsc    = HI_NULL;

    LSC_GET_CTX(ViPipe, pstLsc);

    pstUsrRegCfg->u16MeshStr  = pstLsc->u16MeshStrength;
    pstUsrRegCfg->u16Weight   = pstLsc->u16MeshWeight;
    pstUsrRegCfg->u8MeshScale = pstLsc->u8MeshScale;

    LscGetLutIndex(u8CurBlk, pstLsc, pstUsrRegCfg);

    LscGetGainLut(pstLsc, pstUsrRegCfg);

    pstUsrRegCfg->u32UpdateIndex = 1;

    pstUsrRegCfg->bLutUpdate     = HI_TRUE;
    pstUsrRegCfg->bCoefUpdate    = HI_TRUE;

    return;
}

static HI_VOID LscRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_S32 i;
    ISP_LSC_S *pstLsc    = HI_NULL;

    LSC_GET_CTX(ViPipe, pstLsc);

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        LscStaticRegsInitialize(i, ViPipe, &pstRegCfg->stAlgRegCfg[i].stLscRegCfg.stStaticRegCfg);
        LscUsrRegsInitialize(i, ViPipe, &pstRegCfg->stAlgRegCfg[i].stLscRegCfg.stUsrRegCfg);

        pstRegCfg->stAlgRegCfg[i].stLscRegCfg.bLscEn = pstLsc->bLscEnable;
    }

    pstRegCfg->unKey.bit1LscCfg    = 1;

    return;
}

static HI_VOID LscExtRegsInitialize(VI_PIPE ViPipe)
{
    HI_U16 i;
    ISP_LSC_S  *pstLsc    = HI_NULL;

    LSC_GET_CTX(ViPipe, pstLsc);

    hi_ext_system_isp_mesh_shading_enable_write(ViPipe, pstLsc->bLscEnable);
    hi_ext_system_isp_mesh_shading_attr_updata_write(ViPipe, HI_FALSE);
    hi_ext_system_isp_mesh_shading_lut_attr_updata_write(ViPipe, HI_FALSE);
    hi_ext_system_isp_mesh_shading_mesh_strength_write(ViPipe, pstLsc->u16MeshStrength);
    hi_ext_system_isp_mesh_shading_blendratio_write(ViPipe, pstLsc->u16MeshWeight);
    hi_ext_system_isp_mesh_shading_mesh_scale_write(ViPipe, pstLsc->u8MeshScale);

    for (i = 0; i < (HI_ISP_LSC_GRID_COL - 1) / 2; i++)
    {
        hi_ext_system_isp_mesh_shading_xgrid_write(ViPipe, i, pstLsc->au16DeltaX[i]);
    }

    for (i = 0; i < (HI_ISP_LSC_GRID_ROW - 1) / 2; i++)
    {
        hi_ext_system_isp_mesh_shading_ygrid_write(ViPipe, i, pstLsc->au16DeltaY[i]);
    }

    for (i = 0; i < HI_ISP_LSC_GRID_POINTS; i++)
    {
        hi_ext_system_isp_mesh_shading_r_gain0_write(ViPipe, i, pstLsc->au32RGain[i] & 0x3FF);
        hi_ext_system_isp_mesh_shading_gr_gain0_write(ViPipe, i, pstLsc->au32GrGain[i] & 0x3FF);
        hi_ext_system_isp_mesh_shading_gb_gain0_write(ViPipe, i, pstLsc->au32GbGain[i] & 0x3FF);
        hi_ext_system_isp_mesh_shading_b_gain0_write(ViPipe, i, pstLsc->au32BGain[i] & 0x3FF);

        hi_ext_system_isp_mesh_shading_r_gain1_write(ViPipe, i, pstLsc->au32RGain[i]  >> 10);
        hi_ext_system_isp_mesh_shading_gr_gain1_write(ViPipe, i, pstLsc->au32GrGain[i] >> 10);
        hi_ext_system_isp_mesh_shading_gb_gain1_write(ViPipe, i, pstLsc->au32GbGain[i] >> 10);
        hi_ext_system_isp_mesh_shading_b_gain1_write(ViPipe, i, pstLsc->au32BGain[i]  >> 10);
    }

    return;
}

static HI_VOID LscReadExtRegs(VI_PIPE ViPipe)
{
    HI_U16  i;
    HI_U16  r_gain0, r_gain1, gr_gain0, gr_gain1, gb_gain0, gb_gain1, b_gain0, b_gain1;
    ISP_LSC_S *pstLsc    = HI_NULL;
    LSC_GET_CTX(ViPipe, pstLsc);

    pstLsc->bLscCoefUpdata  = hi_ext_system_isp_mesh_shading_attr_updata_read(ViPipe);
    hi_ext_system_isp_mesh_shading_attr_updata_write(ViPipe, HI_FALSE);

    if (pstLsc->bLscCoefUpdata)
    {
        pstLsc->u16MeshStrength = hi_ext_system_isp_mesh_shading_mesh_strength_read(ViPipe);
        pstLsc->u16MeshWeight   = hi_ext_system_isp_mesh_shading_blendratio_read(ViPipe);
    }

    pstLsc->bLutUpdate      = hi_ext_system_isp_mesh_shading_lut_attr_updata_read(ViPipe);
    hi_ext_system_isp_mesh_shading_lut_attr_updata_write(ViPipe, HI_FALSE);

    if (pstLsc->bLutUpdate)
    {
        pstLsc->u8MeshScale = hi_ext_system_isp_mesh_shading_mesh_scale_read(ViPipe);

        for (i = 0; i < (HI_ISP_LSC_GRID_COL - 1) / 2; i++)
        {
            pstLsc->au16DeltaX[i] = hi_ext_system_isp_mesh_shading_xgrid_read(ViPipe, i);
        }

        for (i = (HI_ISP_LSC_GRID_COL - 1) / 2; i < HI_ISP_LSC_GRID_COL - 1; i++)
        {
            pstLsc->au16DeltaX[i] = pstLsc->au16DeltaX[HI_ISP_LSC_GRID_COL - 2 - i];
        }

        for ( i = 0; i < (HI_ISP_LSC_GRID_ROW - 1) / 2; i++)
        {
            pstLsc->au16DeltaY[i] = hi_ext_system_isp_mesh_shading_ygrid_read(ViPipe, i);
        }

        geometricInvSize(pstLsc);

        for ( i = 0; i < HI_ISP_LSC_GRID_POINTS; i++)
        {

            r_gain0 = hi_ext_system_isp_mesh_shading_r_gain0_read(ViPipe, i);
            r_gain1 = hi_ext_system_isp_mesh_shading_r_gain1_read(ViPipe, i);

            gr_gain0 = hi_ext_system_isp_mesh_shading_gr_gain0_read(ViPipe, i);
            gr_gain1 = hi_ext_system_isp_mesh_shading_gr_gain1_read(ViPipe, i);

            gb_gain0 = hi_ext_system_isp_mesh_shading_gb_gain0_read(ViPipe, i);
            gb_gain1 = hi_ext_system_isp_mesh_shading_gb_gain1_read(ViPipe, i);

            b_gain0 = hi_ext_system_isp_mesh_shading_b_gain0_read(ViPipe, i);
            b_gain1 = hi_ext_system_isp_mesh_shading_b_gain1_read(ViPipe, i);

            pstLsc->au32RGain[i]  = (r_gain1  << 10) + r_gain0;
            pstLsc->au32GrGain[i] = (gr_gain1 << 10) + gr_gain0;
            pstLsc->au32GbGain[i] = (gb_gain1 << 10) + gb_gain0;
            pstLsc->au32BGain[i]  = (b_gain1  << 10) + b_gain0;
        }
    }

    return;
}

static HI_VOID geometricGridSizeLsc(HI_U16 *pu16Delta, HI_U16 *pu16Inv, HI_U16 u16Length, HI_U16 u16GridSize)
{
    HI_U16 i, sum;
    HI_U16 u16HalfGridSize;
    HI_U16 diff;
    HI_U16 *pu16TmpStep;
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

static HI_VOID LscImageSize(VI_PIPE ViPipe, HI_U8 u8BlkNum, ISP_LSC_S  *pstLsc)
{
    HI_U16     i;
    ISP_RECT_S stBlockRect;
    ISP_CTX_S  *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    geometricGridSizeLsc(pstLsc->au16DeltaX, pstLsc->au16InvX, pstIspCtx->stBlockAttr.stFrameRect.u32Width / 2, HI_ISP_LSC_GRID_COL);
    geometricGridSizeLsc(pstLsc->au16DeltaY, pstLsc->au16InvY, pstIspCtx->stBlockAttr.stFrameRect.u32Height / 2, HI_ISP_LSC_GRID_ROW);

    for (i = 0; i < (HI_ISP_LSC_GRID_COL - 1) / 2; i++)
    {
        pstLsc->au16DeltaX[HI_ISP_LSC_GRID_COL - 2 - i] = pstLsc->au16DeltaX[i];
        pstLsc->au16InvX[HI_ISP_LSC_GRID_COL - 2 - i]   = pstLsc->au16InvX[i];
    }

    for (i = 0; i < u8BlkNum; i++)
    {
        ISP_GetBlockRect(&stBlockRect, &pstIspCtx->stBlockAttr, i);

        pstLsc->au16CurWidth[i]       = stBlockRect.u32Width;
        pstLsc->au16FirstPointPosX[i] = stBlockRect.s32X;
    }

    return;
}

static HI_VOID LscInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U16     i;
    HI_U32     u32DefGain;
    ISP_LSC_S  *pstLsc    = HI_NULL;
    ISP_CTX_S  *pstIspCtx = HI_NULL;
    ISP_CMOS_DEFAULT_S *pstSnsDft  = HI_NULL;
    ISP_CMOS_LSC_S     *pstCmosLsc = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    LSC_GET_CTX(ViPipe, pstLsc);
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);

    pstLsc->u16MeshStrength = HI_ISP_LSC_DEFAULT_MESH_STRENGTH;
    pstLsc->u16MeshWeight   = HI_ISP_LSC_DEFAULT_WEIGHT;

    pstCmosLsc = &pstSnsDft->stLsc;

    if (pstCmosLsc->bValid)
    {
        pstLsc->u8MeshScale = pstCmosLsc->u8MeshScale;

        for (i = 0; i < HI_ISP_LSC_GRID_POINTS; i++)
        {
            pstLsc->au32RGain[i]  = (pstCmosLsc->stLscCalibTable[1].au16R_Gain[i]  << 10) + pstCmosLsc->stLscCalibTable[0].au16R_Gain[i];
            pstLsc->au32GrGain[i] = (pstCmosLsc->stLscCalibTable[1].au16Gr_Gain[i] << 10) + pstCmosLsc->stLscCalibTable[0].au16Gr_Gain[i];
            pstLsc->au32GbGain[i] = (pstCmosLsc->stLscCalibTable[1].au16Gb_Gain[i] << 10) + pstCmosLsc->stLscCalibTable[0].au16Gb_Gain[i];
            pstLsc->au32BGain[i]  = (pstCmosLsc->stLscCalibTable[1].au16B_Gain[i]  << 10) + pstCmosLsc->stLscCalibTable[0].au16B_Gain[i];
        }
    }
    else
    {
        pstLsc->u8MeshScale = HI_ISP_LSC_DEFAULT_MESH_SCALE;
        u32DefGain = (g_au16MeshGainDef[pstLsc->u8MeshScale] << 10) + g_au16MeshGainDef[pstLsc->u8MeshScale];

        for (i = 0; i < HI_ISP_LSC_GRID_POINTS; i++)
        {
            pstLsc->au32RGain[i]  = u32DefGain;
            pstLsc->au32GrGain[i] = u32DefGain;
            pstLsc->au32GbGain[i] = u32DefGain;
            pstLsc->au32BGain[i]  = u32DefGain;
        }
    }

    LscImageSize(ViPipe, pstRegCfg->u8CfgNum, pstLsc);

    if ( DYNAMIC_RANGE_XDR == pstIspCtx->stHdrAttr.enDynamicRange)
    {
        pstLsc->bLscEnable = HI_FALSE;
    }
    else
    {
        pstLsc->bLscEnable = HI_TRUE;
    }

    return;
}

static HI_VOID Lsc_Usr_Fw(HI_U8 u8CurBlk, ISP_LSC_S *pstLsc, ISP_LSC_USR_CFG_S *pstUsrRegCfg)
{
    LscGetLutIndex(u8CurBlk, pstLsc, pstUsrRegCfg);

    LscGetGainLut(pstLsc, pstUsrRegCfg);

    pstUsrRegCfg->u8MeshScale     = pstLsc->u8MeshScale;
    pstUsrRegCfg->u32UpdateIndex += 1;
    pstUsrRegCfg->bLutUpdate      = HI_TRUE;

    return;
}

static HI_BOOL __inline CheckLscOpen(ISP_LSC_S *pstLsc)
{
    return (HI_TRUE == pstLsc->bLscEnable);
}

static HI_S32 LscImageResWrite(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8 i;
    ISP_LSC_S  *pstLsc    = HI_NULL;

    LSC_GET_CTX(ViPipe, pstLsc);

    LscImageSize(ViPipe, pstRegCfg->u8CfgNum, pstLsc);

    LscExtRegsInitialize(ViPipe);

    for ( i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        Lsc_Usr_Fw(i, pstLsc, &pstRegCfg->stAlgRegCfg[i].stLscRegCfg.stUsrRegCfg);
    }

    pstRegCfg->unKey.bit1LscCfg = 1;

    return HI_SUCCESS;
}

HI_S32 ISP_LscInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    LscInitialize(ViPipe, pstRegCfg);
    LscRegsInitialize(ViPipe, pstRegCfg);
    LscExtRegsInitialize(ViPipe);


    return HI_SUCCESS;
}

static HI_VOID ISP_LscWdrModeSet(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    return;
}

HI_S32 ISP_LscRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                  HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_S32 i;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_LSC_S *pstLsc = HI_NULL;
    ISP_REG_CFG_S *pstRegCfg  = (ISP_REG_CFG_S *)pRegCfg;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    LSC_GET_CTX(ViPipe, pstLsc);

    if (pstIspCtx->stLinkage.bDefectPixel)
    {
        return HI_SUCCESS;
    }

    pstLsc->bLscEnable = hi_ext_system_isp_mesh_shading_enable_read(ViPipe);

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        pstRegCfg->stAlgRegCfg[i].stLscRegCfg.bLscEn = pstLsc->bLscEnable;
    }

    pstRegCfg->unKey.bit1LscCfg = 1;

    /*check hardware setting*/
    if (!CheckLscOpen(pstLsc))
    {
        return HI_SUCCESS;
    }

    LscReadExtRegs(ViPipe);

    if (pstLsc->bLscCoefUpdata)
    {
        for (i = 0; i < pstRegCfg->u8CfgNum; i++)
        {
            pstRegCfg->stAlgRegCfg[i].stLscRegCfg.stUsrRegCfg.bCoefUpdate   = HI_TRUE;
            pstRegCfg->stAlgRegCfg[i].stLscRegCfg.stUsrRegCfg.u16MeshStr    = pstLsc->u16MeshStrength;
            pstRegCfg->stAlgRegCfg[i].stLscRegCfg.stUsrRegCfg.u16Weight     = pstLsc->u16MeshWeight;
        }
    }

    if (pstLsc->bLutUpdate)
    {
        for ( i = 0; i < pstRegCfg->u8CfgNum; i++)
        {
            Lsc_Usr_Fw(i, pstLsc, &pstRegCfg->stAlgRegCfg[i].stLscRegCfg.stUsrRegCfg);
        }
    }

    return HI_SUCCESS;
}

HI_S32 ISP_LscCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);

    switch (u32Cmd)
    {
        case ISP_WDR_MODE_SET :
            ISP_LscWdrModeSet(ViPipe, (HI_VOID *)&pRegCfg->stRegCfg);
            break;
        case ISP_CHANGE_IMAGE_MODE_SET:
            LscImageResWrite(ViPipe, &pRegCfg->stRegCfg);
            break;
        default :
            break;
    }
    return HI_SUCCESS;
}

HI_S32 ISP_LscExit(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);

    for (i = 0; i < pRegCfg->stRegCfg.u8CfgNum; i++)
    {
        pRegCfg->stRegCfg.stAlgRegCfg[i].stLscRegCfg.bLscEn = HI_FALSE;
    }

    pRegCfg->stRegCfg.unKey.bit1LscCfg = 1;

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterLsc(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_LSC;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_LscInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_LscRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_LscCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_LscExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

