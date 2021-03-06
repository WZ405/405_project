/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_mlsc_online_cali.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2018/01/04
  Description   :
  History       :
  1.Date        : 2018/01/04
    Author      :
    Modification: Created file

******************************************************************************/

#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_ext_config.h"
#include "isp_config.h"
#include "isp_proc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define MAX(a, b) (((a) < (b)) ?  (b) : (a))
#define MIN(a,b)  (((a)>(b))?(b):(a))

#define LSC_GRID_ROWS 33
#define LSC_GRID_COLS 33
#define LSC_GRID_POINTS (LSC_GRID_ROWS*LSC_GRID_COLS)
#define LSC_MEASURE_Q 1024
#define WINDOWSIZEX 20
#define WINDOWSIZEY 20

#define LSC_MAX_PIXEL_VALUE ((1<<20) - 1)

typedef struct hi_LSC_Grid_Step_S
{
    HI_U32 au32GridWidthStep[LSC_GRID_COLS];//Former expression: gridWidthStep[];
    HI_U32 au32GridHeightStep[LSC_GRID_ROWS];//Former expression: gridHeightStep[];

} ISP_LSC_Grid_Step_S;


typedef struct LS_DATA
{
    HI_U32 au32R_data[LSC_GRID_POINTS];
    HI_U32 au32Gr_data[LSC_GRID_POINTS];
    HI_U32 au32Gb_data[LSC_GRID_POINTS];
    HI_U32 au32B_data[LSC_GRID_POINTS];
} LS_DATA;

static const HI_FLOAT g_afMaxGainArray[8] =
{
    (HI_FLOAT)1023 / (HI_FLOAT)512,     //1.9
    (HI_FLOAT)1023 / (HI_FLOAT)256,     //2.8
    (HI_FLOAT)1023 / (HI_FLOAT)128,     //3.7
    (HI_FLOAT)1023 / (HI_FLOAT)64,      //4.6
    (HI_FLOAT)1023 / (HI_FLOAT)1024 + 1, //0.10
    (HI_FLOAT)1023 / (HI_FLOAT)512 + 1, //1.9
    (HI_FLOAT)1023 / (HI_FLOAT)256 + 1, //2.8
    (HI_FLOAT)1023 / (HI_FLOAT)128 + 1, //3.7
};

/**
 * [getMaxData description]
 * @param pu32data   [Input] Input array
 * @param length     [Input] array length
 * Get the maxmum data from the input pu32data array
 */
HI_U32 getMaxData(HI_U32 *pu32data, int length)
{
    int i;

    HI_U32 u32MaxData = 0;
    for (i = 0; i < length; i++)
    {
        if (pu32data[i] > u32MaxData)
        {
            u32MaxData = pu32data[i];
        }
    }
    return u32MaxData;
}

/**
 * [getMinData description]
 * @param pu32data   [Input] Input array
 * @param length     [Input] array length
 * Get the minmum data from the input pu32data array
 */
HI_U32 getMinData(HI_U32 *pu32data, int length)
{
    int i;

    HI_U32 u32MinData = LSC_MAX_PIXEL_VALUE;
    for (i = 0; i < length; i++)
    {
        if (pu32data[i] < u32MinData)
        {
            u32MinData = pu32data[i];
        }
    }
    return u32MinData;
}

/**
 * [dataEnlarge description]
 * @param u32Data        [Input] Input Data
 * @param u32MaxData     [Input] Input Target value
 * @param u32MeshScale   [Input] Input scale value
 * Generate gain value at each grid point, uniformed by input u32MeshScale
 */
HI_U32 dataEnlarge(HI_U32 u32Data, HI_U32 u32MaxData, HI_U32 u32MeshScale)
{

    HI_U32 ratio; //Using unsigned 32 bit to present the ratio;
    HI_U32 u32LscQValue;
    if (u32MeshScale < 4)
    {
        u32LscQValue = 1 << (9 - u32MeshScale);
        ratio = (HI_U32)(((HI_FLOAT)u32MaxData / (HI_FLOAT)u32Data) * u32LscQValue); //
    }
    else
    {
        u32LscQValue = 1 << (14 - u32MeshScale);
        ratio = (HI_U32)(((HI_FLOAT)u32MaxData / (HI_FLOAT)u32Data - 1) * u32LscQValue); //
    }

    return MIN(ratio, 1023);

}

/**
 * [sort description]
 * @param pu32Array  [Input] Input array
 * @param u32Num     [Input] array length
 * Sort the input array from min to max
 */
HI_VOID sort(HI_U32 *pu32Array, HI_U32 u32Num)
{
    HI_U32 i, j;
    HI_U32 temp;
    for (i = 0; i < u32Num ; i++)
    {
        for (j = 0; j < (u32Num - 1) - i; j++)
        {
            if (pu32Array[j] > pu32Array[j + 1])
            {
                temp = pu32Array[j];
                pu32Array[j] = pu32Array[j + 1];
                pu32Array[j + 1] = temp;
            }
        }
    }

}

/**
 * [Get_Lsc_Data_Channel description]
 * @param pBayerImg        [Input]  Input image data
 * @param pu32GridData     [Output] MeshLsc Grid data generated
 * @param stLSCCaliCfg     [Input]  Mesh LSC Calibration configure
 * @param pstLscGridStepXY [Input]  Input Grid width&height information
 * @param u16BlcOffset     [Input]  Input BLC value
 * @param u8Value          [Input]  Indicate channel number: 0-R, 1-Gr, 2-Gb, 3-B
 */
HI_S32 Get_Lsc_Data_Channel(HI_U16 *pBayerImg, HI_U32 *pu32GridData, ISP_MLSC_CALIBRATION_CFG_S *stLSCCaliCfg, ISP_LSC_Grid_Step_S *pstLscGridStepXY, HI_U16 u16BlcOffset, HI_U8 u8Value)
{
    HI_S32 s32Height   = (HI_S32)stLSCCaliCfg->u16ImgHeight;
    HI_S32 s32Width    = (HI_S32)stLSCCaliCfg->u16ImgWidth;
    HI_U32 size_x      = WINDOWSIZEX;
    HI_U32 size_y      = WINDOWSIZEY;
    HI_U32 grid_x_size = LSC_GRID_COLS;
    HI_U32 grid_y_size = LSC_GRID_ROWS;
    HI_S32 s32TrueBlcOffset  ;//= (HI_S32)(stLSCCaliCfg->u32BLCOffsetR<<8);
    HI_U32 u32EliminatePctLow = 10;
    HI_U32 u32EliminatePctHigh = 90;
    HI_U32 fullWidth = s32Width << 1;
    HI_U32 u32Stride = fullWidth;
    HI_U32 i, j;

    HI_U32 location_y = 0; //Added
    HI_U32 location_x;

    HI_U32 num, count, numTemp;
    //HI_S32 data_sum;
    HI_S32 x_start, x_end, y_start, y_end;

    HI_S32 x, y;

    //test
    HI_U32 pu32Data[400] = {0};

    HI_U32 u32Sum;
    HI_U32 u32CoorIndex = 0;
    HI_U32 u32ChnIndex  = 0;
    HI_U32 h, w;

    if (HI_NULL == pBayerImg || HI_NULL == pu32GridData)
    {
        printf("%s: LINE: %d pBayerImg/pls_data is NULL point !\n", __FUNCTION__, __LINE__);
        return HI_FAILURE;
    }

    u32ChnIndex = (stLSCCaliCfg->enBayer & 0x3) ^ u8Value;

    w = u32ChnIndex & 0x1;
    h = (u32ChnIndex >> 1) & 0x1;

    s32TrueBlcOffset  = (HI_S32)(((HI_U32)u16BlcOffset) << 8); //Input BLC will always be 12 bits

    for (i = 0; i < grid_y_size; i++)
    {
        location_y = location_y + pstLscGridStepXY->au32GridHeightStep[i];
        location_x = 0; //Added
        for (j = 0; j < grid_x_size; j++)
        {
            location_x = location_x + pstLscGridStepXY->au32GridWidthStep[j];
            num = 0;

            x_start = location_x - size_x / 2;
            x_end   = location_x + size_x / 2;
            y_start = location_y - size_y / 2;
            y_end   = location_y + size_y / 2;

            if (x_start < 0)
            {
                x_start = 0;
            }
            if (x_end > s32Width)
            {
                x_end = s32Width - 1;
            }
            if (y_start < 0)
            {
                y_start = 0;
            }
            if (y_end > s32Height)
            {
                y_end = s32Height - 1;
            }

            for (y = y_start; y < y_end; y++)
            {
                for (x = x_start; x < x_end; x++)
                {
                    u32CoorIndex = 2 * x + 2 * y * u32Stride;
                    pu32Data[num] = (HI_U32)MAX(0, ((HI_S32)((HI_U32)(pBayerImg[u32CoorIndex + w + h * u32Stride]) << (20 - stLSCCaliCfg->enRawBit)) - s32TrueBlcOffset ));
                    num++;
                }
            }

            sort(pu32Data, num);

            numTemp = 0;
            u32Sum  = 0;

            for (count = num * u32EliminatePctLow / 100; count < num * u32EliminatePctHigh / 100; count++)
            {
                u32Sum  += pu32Data[count];

                numTemp++;
            }

            if (numTemp)
            {
                pu32GridData[ i * grid_x_size + j]    = u32Sum / numTemp;
            }
            else
            {
                printf("Something goes wrong in getLSData()!");
                return HI_FAILURE;
            }
        }
    }

    return HI_SUCCESS;
}


/**
 * [getLSData description]
 * @param BayerImg         [Input]  Input image data
 * @param pstLSCGridData   [Output] MeshLsc Grid data generated
 * @param pstLscGridStepXY [Input]  Input Grid width&height information
 * @param pstLSCCaliCfg    [Input]  MeshLsc Calibration results
 */
HI_S32 getLSData(HI_U16 *BayerImg, LS_DATA *pstLSCGridData, ISP_LSC_Grid_Step_S *pstLscGridStepXY, ISP_MLSC_CALIBRATION_CFG_S *pstLSCCaliCfg)
{
    HI_S32 s32Ret = HI_FAILURE;

    s32Ret = Get_Lsc_Data_Channel(BayerImg, pstLSCGridData->au32R_data, pstLSCCaliCfg, pstLscGridStepXY, pstLSCCaliCfg->u16BLCOffsetR, 0);

    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: LINE: %d Get_Lsc_Data of R Channel failure !\n", __FUNCTION__, __LINE__);
        return HI_FAILURE;
    }

    s32Ret = Get_Lsc_Data_Channel(BayerImg, pstLSCGridData->au32Gr_data, pstLSCCaliCfg, pstLscGridStepXY, pstLSCCaliCfg->u16BLCOffsetGr, 1);

    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: LINE: %d Get_Lsc_Data of R Channel failure !\n", __FUNCTION__, __LINE__);
        return HI_FAILURE;
    }

    s32Ret = Get_Lsc_Data_Channel(BayerImg, pstLSCGridData->au32Gb_data, pstLSCCaliCfg, pstLscGridStepXY, pstLSCCaliCfg->u16BLCOffsetGb, 2);

    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: LINE: %d Get_Lsc_Data of R Channel failure !\n", __FUNCTION__, __LINE__);
        return HI_FAILURE;
    }

    s32Ret = Get_Lsc_Data_Channel(BayerImg, pstLSCGridData->au32B_data, pstLSCCaliCfg, pstLscGridStepXY, pstLSCCaliCfg->u16BLCOffsetB, 3);

    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: LINE: %d Get_Lsc_Data of R Channel failure !\n", __FUNCTION__, __LINE__);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

/**
 * [lc_normalization description]
 * @param ls_data            [Input]  Input Grid Data
 * @param u32LSCGridPoints   [Input] MeshLsc Calibration results
 * @param u32MeshScale       [Input]  Input meshLsc calibration config
 * This function is used for testing if the input mesh scale is suitable for the current condition
 */
HI_S32 MeshScaleTest(LS_DATA *ls_data, HI_U32 u32LSCGridPoints, HI_U32 u32MeshScale)
{
    HI_U32 b_max_data, gb_max_data, gr_max_data, r_max_data;
    HI_U32 b_min_data, gb_min_data, gr_min_data, r_min_data;
    HI_FLOAT b_max_gain, gb_max_gain, gr_max_gain, r_max_gain;
    HI_FLOAT max_gain;

    // for b channel
    b_max_data  = getMaxData(ls_data->au32B_data, u32LSCGridPoints);
    // for gb channel
    gb_max_data = getMaxData(ls_data->au32Gb_data, u32LSCGridPoints);
    // for gr channel
    gr_max_data = getMaxData(ls_data->au32Gr_data, u32LSCGridPoints);
    // for r channel
    r_max_data  = getMaxData(ls_data->au32R_data, u32LSCGridPoints);


    // for b channel
    b_min_data  = getMinData(ls_data->au32B_data, u32LSCGridPoints);
    // for gb channel
    gb_min_data = getMinData(ls_data->au32Gb_data, u32LSCGridPoints);
    // for gr channel
    gr_min_data = getMinData(ls_data->au32Gr_data, u32LSCGridPoints);
    // for r channel
    r_min_data  = getMinData(ls_data->au32R_data, u32LSCGridPoints);

    b_max_gain  = (HI_FLOAT)b_max_data  / (HI_FLOAT)b_min_data;
    gb_max_gain = (HI_FLOAT)gb_max_data / (HI_FLOAT)gb_min_data;
    gr_max_gain = (HI_FLOAT)gr_max_data / (HI_FLOAT)gr_min_data;
    r_max_gain  = (HI_FLOAT)r_max_data  / (HI_FLOAT)r_min_data;

    max_gain = MAX(MAX3(b_max_gain, gb_max_gain, gr_max_gain), r_max_gain);


    if (max_gain > g_afMaxGainArray[u32MeshScale])
    {
        printf("WARNING:\n");
        printf("Max gain = %f\n", max_gain);

        if (u32MeshScale < 4)  //min gain's value is 0
        {
            if ((max_gain > g_afMaxGainArray[0]) && (max_gain <= g_afMaxGainArray[1]))  //(2,4]
            {
                printf("Please set Mesh scale to %d\n", 1);
            }
            else if ((max_gain > g_afMaxGainArray[1]) && (max_gain <= g_afMaxGainArray[2]))//(4,8]
            {
                printf("Please set Mesh scale to %d\n", 2);
            }
            else if ((max_gain > g_afMaxGainArray[2]) && (max_gain <= g_afMaxGainArray[3]))//(8,16]
            {
                printf("Please set Mesh scale to %d\n", 3);
            }
            else//  >16
            {
                printf("Max gain is too large to find accurate mesh scale,please set Mesh Scale to %d to reduce loss\n", 3);
            }
        }
        else   //max gain's value is 1
        {
            if ((max_gain > g_afMaxGainArray[4]) && (max_gain <= g_afMaxGainArray[5]))  //(2,3]
            {
                printf("Please set Mesh scale to %d\n", 5);
            }
            else if ((max_gain > g_afMaxGainArray[5]) && (max_gain <= g_afMaxGainArray[6]))//(3,5]
            {
                printf("Please set Mesh scale to %d\n", 6);
            }
            else if ((max_gain > g_afMaxGainArray[6]) && (max_gain <= g_afMaxGainArray[7]))//(5,9]
            {
                printf("Please set Mesh scale to %d\n", 7);
            }
            else if ((max_gain > g_afMaxGainArray[7]) && (max_gain <= g_afMaxGainArray[3])) //(9,16]
            {
                printf("Please set Mesh scale to %d\n", 3);
            }
            else  //>16
            {
                printf("Max gain is too large to find accurate mesh scale,please set Mesh Scale to %d to reduce loss\n", 3);
            }
        }
    }

    return HI_SUCCESS;
}


/**
 * [lc_normalization description]
 * @param pstLSCGridData     [Input]  Input Grid Data
 * @param pstLSCCaliResult   [Output] MeshLsc Calibration results
 * @param pstLSCCaliCfg      [Input]  Input meshLsc calibration config
 */
HI_S32 lc_normalization(LS_DATA *pstLSCGridData, ISP_MESH_SHADING_TABLE_S *pstLSCCaliResult, ISP_MLSC_CALIBRATION_CFG_S *pstLSCCaliCfg)
{

    HI_U32 b_max_data, gb_max_data, gr_max_data, r_max_data;
    HI_U32 u32LSCGridPoints;
    HI_U32 i;
    HI_S32 s32Ret;

    u32LSCGridPoints = LSC_GRID_POINTS;//33*33

    s32Ret = MeshScaleTest(pstLSCGridData, LSC_GRID_POINTS, pstLSCCaliCfg->u32MeshScale);
    if (s32Ret != HI_SUCCESS)
    {
        printf("Fail in MeshScaleTest!\n");
        return HI_FAILURE;
    }

    //find the Max data of each channel
    // for b channel
    b_max_data  = getMaxData(pstLSCGridData->au32B_data, u32LSCGridPoints);
    // for gb channel
    gb_max_data = getMaxData(pstLSCGridData->au32Gb_data, u32LSCGridPoints);
    // for gr channel
    gr_max_data = getMaxData(pstLSCGridData->au32Gr_data, u32LSCGridPoints);
    // for r channel
    r_max_data  = getMaxData(pstLSCGridData->au32R_data, u32LSCGridPoints);
    //printf("\nb_max_data=%d,gb_max_data=%d,gr_max_data=%d,r_max_data=%d\n",b_max_data,gb_max_data,gr_max_data,r_max_data);
    for ( i = 0 ; i < u32LSCGridPoints ; i++ )
    {
        //normalization process
        pstLSCCaliResult->au16BGain[i]  = (HI_U16)(MIN(dataEnlarge(pstLSCGridData->au32B_data[i], b_max_data, pstLSCCaliCfg->u32MeshScale), LSC_MAX_PIXEL_VALUE));
        pstLSCCaliResult->au16GbGain[i] = (HI_U16)(MIN(dataEnlarge(pstLSCGridData->au32Gb_data[i], gb_max_data, pstLSCCaliCfg->u32MeshScale), LSC_MAX_PIXEL_VALUE));
        pstLSCCaliResult->au16GrGain[i] = (HI_U16)(MIN(dataEnlarge(pstLSCGridData->au32Gr_data[i], gr_max_data, pstLSCCaliCfg->u32MeshScale), LSC_MAX_PIXEL_VALUE));
        pstLSCCaliResult->au16RGain[i]  = (HI_U16)(MIN(dataEnlarge(pstLSCGridData->au32R_data[i], r_max_data, pstLSCCaliCfg->u32MeshScale), LSC_MAX_PIXEL_VALUE));

    }

    return HI_SUCCESS;

}


/**
 * [GeometricGridSizeY description]
 * @param pstLSCCaliCfg      [Input]  Input meshLsc calibration config
 * @param pstLscGridStepXY   [Output]  Grid width&height info
 * @param pstLSCCaliResult   [Output] MeshLsc Calibration results
 */
HI_S32 GeometricGridSizeX(ISP_MLSC_CALIBRATION_CFG_S *pstLSCCaliCfg, ISP_LSC_Grid_Step_S *pstLscGridStepXY, ISP_MESH_SHADING_TABLE_S *pstLSCCaliResult)
{
    //Parameters Definition
    HI_U32   u32TmpStepX[(LSC_GRID_COLS - 1) / 2]; //Former expression: tmpStepX
    HI_FLOAT f16Rx[(LSC_GRID_COLS - 1) / 2]; //Former expression: rx
    HI_FLOAT f16SumR; //Former expression: sum_r

    HI_U32 i, sum;
    HI_U32 u32HalfGridSizeX = (LSC_GRID_COLS - 1) / 2; //Former expression: u32HalfGridSizeX;
    HI_U32 u32Width, diff; //Former expression: u32Width, diff

    HI_FLOAT f16R1 = 1.0f;
    //Read Image info

    u32Width = pstLSCCaliCfg->u16ImgWidth;

    //Function start
    //Horizental direction
    f16Rx[0] = 1.0f;
    for ( i = 1 ; i < u32HalfGridSizeX; i++ )
    {
        f16Rx[i] = f16Rx[i - 1] * f16R1;
    }
    f16SumR = 0;
    for ( i = 0 ; i < u32HalfGridSizeX; i++ )
    {
        f16SumR = f16SumR + f16Rx[i];
    }
    for ( i = 0 ; i < u32HalfGridSizeX; i++ )
    {
        u32TmpStepX[i] = (int)(((u32Width / 2 * 1024 / DIV_0_TO_1(f16SumR)) * f16Rx[i] + (1024 >> 1)) / 1024);
    }

    sum = 0;
    for (i = 0; i < u32HalfGridSizeX; i++)
    {
        sum = sum + u32TmpStepX[i];
    }
    if (sum != (u32Width / 2))
    {
        if (sum > (u32Width / 2))
        {
            diff = sum - u32Width / 2;
            for (i = 1; i <= diff; i++)
            {
                u32TmpStepX[u32HalfGridSizeX - i] = u32TmpStepX[u32HalfGridSizeX - i] - 1;
            }
        }
        else
        {
            diff = u32Width / 2 - sum;
            for (i = 1; i <= diff; i++)
            {
                u32TmpStepX[i - 1] = u32TmpStepX[i - 1] + 1;
            }
        }
    }

    //Return the step length value to gridStepWidth and gridStepHeight
    pstLscGridStepXY->au32GridWidthStep[0] = 0;
    for ( i = 1 ; i <= u32HalfGridSizeX; i++ )
    {
        pstLscGridStepXY->au32GridWidthStep[i] = u32TmpStepX[i - 1];
        pstLscGridStepXY->au32GridWidthStep[LSC_GRID_COLS - i] = u32TmpStepX[i - 1];
        pstLSCCaliResult->au16XGridWidth[i - 1] = pstLscGridStepXY->au32GridWidthStep[i];
    }

    return HI_SUCCESS;

}


/**
 * [GeometricGridSizeY description]
 * @param pstLSCCaliCfg      [Input]  Input meshLsc calibration config
 * @param pstLscGridStepXY   [Output]  Grid width&height info
 * @param pstLSCCaliResult   [Output] MeshLsc Calibration results
 */
HI_S32 GeometricGridSizeY(ISP_MLSC_CALIBRATION_CFG_S *pstLSCCaliCfg, ISP_LSC_Grid_Step_S *pstLscGridStepXY, ISP_MESH_SHADING_TABLE_S *pstLSCCaliResult)
{
    //param definition
    HI_FLOAT f16Ry[(LSC_GRID_ROWS - 1) / 2]; //Former expression: ry
    HI_FLOAT f16SumR; //Former expression: sum_r
    HI_U32   u32TmpStepY[(LSC_GRID_ROWS - 1) / 2]; //Former expression: u32TmpStepY

    HI_U32 j, sum;
    HI_U32 u32HalfGridSizeY = (LSC_GRID_ROWS - 1) / 2; //Former expression: u32HalfGridSizeY
    HI_U32 u32Height, diff;//Former expression: u32Height, diff

    //Read Image info
    u32Height = pstLSCCaliCfg->u16ImgHeight;

    HI_FLOAT f16R1 = 1.0f;

    //Function start: Horizental direction
    f16Ry[0] = 1.0f;

    //Vertical direction
    for ( j = 1 ; j < u32HalfGridSizeY; j++ )
    {
        f16Ry[j] = f16Ry[j - 1] * f16R1;
    }
    f16SumR = 0;
    for ( j = 0 ; j < u32HalfGridSizeY; j++ )
    {
        f16SumR = f16SumR + f16Ry[j];
    }
    for ( j = 0 ; j < u32HalfGridSizeY; j++ )
    {
        u32TmpStepY[j] = (int)(((u32Height / 2 * 1024 / DIV_0_TO_1(f16SumR)) * f16Ry[j] + (1024 >> 1)) / 1024);
    }

    sum = 0;
    for ( j = 0 ; j < u32HalfGridSizeY; j++ )
    {
        sum = sum + u32TmpStepY[j];
    }
    if (sum != (u32Height / 2))
    {
        if (sum > (u32Height / 2))
        {
            diff = sum - u32Height / 2;
            for (j = 1; j <= diff; j++)
            {
                u32TmpStepY[u32HalfGridSizeY - j] = u32TmpStepY[u32HalfGridSizeY - j] - 1;
            }
        }
        else
        {
            diff = u32Height / 2 - sum;
            for (j = 1; j <= diff; j++)
            {
                u32TmpStepY[j - 1] = u32TmpStepY[j - 1] + 1;
            }
        }
    }

    //Return the step length value to gridStepWidth and gridStepHeight
    pstLscGridStepXY->au32GridHeightStep[0] = 0;
    for ( j = 1 ; j <= u32HalfGridSizeY; j++ )
    {
        pstLscGridStepXY->au32GridHeightStep[j] = u32TmpStepY[j - 1];
        pstLscGridStepXY->au32GridHeightStep[LSC_GRID_ROWS - j] = u32TmpStepY[j - 1];
        pstLSCCaliResult->au16YGridWidth[j - 1] = pstLscGridStepXY->au32GridHeightStep[j];
    }

    return HI_SUCCESS;
}

/**
 * [LSC_GenerateGridInfo description]
 * @param pu16Data           [Input]  Input Raw Data that used for calibration
 * @param pstLSCCaliCfg      [Input]  Input meshLsc calibration config
 * @param pstLscGridStepXY   [Input]  Grid width&height info
 * @param pstLSCCaliResult   [Output] MeshLsc Calibration results
 */
HI_S32 LSC_GenerateGridInfo(HI_U16 *pu16Data, ISP_MLSC_CALIBRATION_CFG_S *pstLSCCaliCfg, ISP_LSC_Grid_Step_S *pstLscGridStepXY, ISP_MESH_SHADING_TABLE_S *pstLSCCaliResult)
{
    //Pass all the input params to the function
    LS_DATA *pstLSData;
    //LS_DATA* pstLSGain;

    //HI_U32 i,j;
    HI_S32 s32Ret = HI_SUCCESS;
    //HI_FLOAT f16Gain;

    //memory allocation
    pstLSData = (LS_DATA *)malloc(sizeof(LS_DATA));
    if ( HI_NULL == pstLSData )
    {
        printf("pstLSData allocation fail!\n");
        return HI_FAILURE;
    }

    //get Lens Correction Coefficients
    s32Ret = getLSData(pu16Data, pstLSData, pstLscGridStepXY, pstLSCCaliCfg);
    if (s32Ret != HI_SUCCESS)
    {
        printf("There are some errors in getLSData()!\n");
        free(pstLSData);
        return HI_FAILURE;
    }

    s32Ret = lc_normalization(pstLSData, pstLSCCaliResult, pstLSCCaliCfg);

    free(pstLSData);
    return s32Ret;

}

/**
 * [ISP_MeshShadingCalibration description]
 * @param pu16SrcRaw     [Input]  Input Raw Data that used for calibration
 * @param pstMLSCCaliCfg [Input]  Input meshLsc calibration config
 * @param pstMLSCTable   [Output] MeshLsc Calibration results
 */
HI_S32 ISP_MeshShadingCalibration(HI_U16 *pu16SrcRaw,
                                  ISP_MLSC_CALIBRATION_CFG_S *pstMLSCCaliCfg,
                                  ISP_MESH_SHADING_TABLE_S *pstMLSCTable)
{
    HI_S32 S32Ret;

    ISP_LSC_Grid_Step_S stLscGridStepXY;
    pstMLSCCaliCfg->u16ImgWidth /= 2;
    pstMLSCCaliCfg->u16ImgHeight /= 2;

    /* Step 1: Get Grid info*/
    S32Ret = GeometricGridSizeX(pstMLSCCaliCfg, &stLscGridStepXY, pstMLSCTable);
    if (S32Ret == HI_FAILURE)
    {
        printf("There are some errors in LSC_GenerateGridInfo()!\n");
        return HI_FAILURE;
    }
    S32Ret = GeometricGridSizeY(pstMLSCCaliCfg, &stLscGridStepXY, pstMLSCTable);
    if (S32Ret == HI_FAILURE)
    {
        printf("There are some errors in LSC_GenerateGridInfo()!\n");
        return HI_FAILURE;
    }

    //malloc memory
    printf("Shading Calibration is processing!!!...\n");
    S32Ret = LSC_GenerateGridInfo(pu16SrcRaw, pstMLSCCaliCfg, &stLscGridStepXY, pstMLSCTable);
    if (S32Ret == HI_FAILURE)
    {
        printf("There are some errors in LSC_GenerateGridInfo()!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */



