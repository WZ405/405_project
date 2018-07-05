/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_sharpen.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/01/16
  Description   :
  History       :
  1.Date        : 2014/01/16
    Author      :
    Modification: Created file

******************************************************************************/
#include "isp_config.h"
#include "hi_isp_debug.h"
#include "isp_ext_config.h"
#include "isp_math_utils.h"
#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_proc.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define     ISP_SHARPEN_FREQ_CORING_LENGTH      (8)
#define     SHRP_GAIN_LUT_SFT                   (2)
#define     SHRP_GAIN_LUT_SIZE                  (64)
#define     SHRP_SHT_VAR_MUL_PRECS              (4)
#define     SHRP_SKIN_EDGE_MUL_PRECS            (4)
#define     SHRP_SKIN_ACCUM_MUL_PRECS           (3)
#define     SHRP_CHR_MUL_SFT                    (4)
#define     SHARPEN_EN                          (1)
#define     SHRP_DETAIL_SHT_MUL_PRECS           (4)


#define SHARPEN_CLIP3(min,max,x)         ( (x)<= (min) ? (min) : ((x)>(max)?(max):(x)) )

typedef struct hiISP_SHARPEN_S
{
    HI_BOOL bSharpenEn;
    HI_BOOL bSharpenMpiUpdateEn;
    HI_U32  u32IsoLast;
    /* Sharpening Yuv */
    //tmp registers
    HI_U8   u8ManualSharpenYuvEnabled;
    HI_U8   u8mfThdSftD;
    HI_U8   u8dirVarSft;
    //HI_U8 u8dirRt0;
    //HI_U8 u8dirRt1;
    HI_U8   u8selPixWgt;
    //HI_U8 u8shtNoiseMin;
    HI_U16  u16chrGGain0;
    HI_U8   u8mfThdSelUD;
    HI_U8   u8mfThdSftUD;
    //HI_U8 u8dirDiffSft;
    //HI_U16    u16oMaxGain;
    //HI_U16    u16uMaxGain;
    HI_U8   u8oshtVarWgt0;
    HI_U8   u8ushtVarWgt0;
    HI_U8   u8oshtVarDiffThd0;
    HI_U8   u8oshtVarDiffThd1;
    HI_U8   u8oshtVarDiffWgt1;
    HI_U8   u8ushtVarDiffWgt1;
    //MPI
    HI_U16 au16TextureStr[ISP_SHARPEN_GAIN_NUM];        //Undirectional sharpen strength for texture and detail enhancement. U7.5  [0, 4095]
    HI_U16 au16EdgeStr[ISP_SHARPEN_GAIN_NUM];           //Directional sharpen strength for edge enhancement.     U7.5  [0, 4095]
    HI_U8  au8LumaWgt[ISP_SHARPEN_LUMA_NUM];     //Adjust the sharpen strength according to luma. Sharpen strength will be weaker when it decrease. U7.0  [0, 127]
    HI_U16 u16TextureFreq;         //Texture frequency adjustment. Texture and detail will be finer when it increase.    U6.6  [0, 4095]
    HI_U16 u16EdgeFreq;         //Edge frequency adjustment. Edge will be narrower and thiner when it increase.      U6.6  [0, 4095]
    HI_U8  u8OverShoot;          //u8OvershootAmt       U7.0  [0, 127]
    HI_U8  u8UnderShoot;         //u8UndershootAmt      U7.0  [0, 127]
    HI_U8  u8ShootSupStr;        //overshoot and undershoot suppression strength, the amplitude and width of shoot will be decrease when shootSupSt increase.  U8.0 [0, 255]
    HI_U8  u8DetailCtrl;        //Different sharpen strength for detail and edge. When it is bigger than 128, detail sharpen strength will be stronger than edge.  //[0, 255]
    HI_U8  u8EdgeFiltStr;    //u8EdgeFiltStr    U6.0  [0, 63]
    HI_U8  u8RGain;
    HI_U8  u8BGain;
    HI_U8  u8SkinGain;

    HI_U16 au16AutoTextureStr[ISP_SHARPEN_GAIN_NUM][ISP_AUTO_ISO_STRENGTH_NUM];     //Undirectional sharpen strength for texture and detail enhancement. U7.5  [0, 4095]
    HI_U16 au16AutoEdgeStr[ISP_SHARPEN_GAIN_NUM][ISP_AUTO_ISO_STRENGTH_NUM];           //Directional sharpen strength for edge enhancement.  U7.5  [0, 4095]
    HI_U8  au8AutoLumaWgt[ISP_SHARPEN_LUMA_NUM][ISP_AUTO_ISO_STRENGTH_NUM];      //Adjust the sharpen strength according to luma. Sharpen strength will be weaker when it decrease. U7.0  [0, 127]
    HI_U16 au16TextureFreq[ISP_AUTO_ISO_STRENGTH_NUM];         //Texture frequency adjustment. Texture and detail will be finer when it increase.    U6.6  [0, 4095]
    HI_U16 au16EdgeFreq[ISP_AUTO_ISO_STRENGTH_NUM];         //Edge frequency adjustment. Edge will be narrower and thiner when it increase.      U6.6  [0, 4095]
    HI_U8  au8OverShoot[ISP_AUTO_ISO_STRENGTH_NUM];          //u8OvershootAmt       U7.0  [0, 127]
    HI_U8  au8UnderShoot[ISP_AUTO_ISO_STRENGTH_NUM];         //u8UndershootAmt      U7.0  [0, 127]
    HI_U8  au8ShootSupStr[ISP_AUTO_ISO_STRENGTH_NUM];        //overshoot and undershoot suppression strength, the amplitude and width of shoot will be decrease when shootSupSt increase.  U8.0 [0, 255]
    HI_U8  au8DetailCtrl[ISP_AUTO_ISO_STRENGTH_NUM];            //Different sharpen strength for detail and edge. When it is bigger than 128, detail sharpen strength will be stronger than edge.  //[0, 255]
    HI_U8  au8EdgeFiltStr[ISP_AUTO_ISO_STRENGTH_NUM];    //u8EdgeFiltStr    U6.0  [0, 63]
    HI_U8  au8RGain[ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U8  au8BGain[ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U8  au8SkinGain[ISP_AUTO_ISO_STRENGTH_NUM];

    //HI_U8 au8LumaWgt[ISP_SHARPEN_LUMA_NUM];        //Adjust the sharpen strength according to luma. Sharpen strength will be weaker when it decrease. U7.0  [0, 127]
} ISP_SHARPEN_S;

ISP_SHARPEN_S g_astSharpenCtx[ISP_MAX_PIPE_NUM] = {{0}};
#define SHARPEN_GET_CTX(dev, pstCtx)   pstCtx = &g_astSharpenCtx[dev]

static HI_VOID SharpenExtRegsInitialize(VI_PIPE ViPipe)
{
    HI_U16 i, j;
    ISP_CMOS_DEFAULT_S *pstSnsDft = HI_NULL;

    ISP_SensorGetDefault(ViPipe, &pstSnsDft);

    hi_ext_system_isp_sharpen_manu_mode_write(ViPipe, OP_TYPE_AUTO);
    hi_ext_system_manual_isp_sharpen_en_write(ViPipe, HI_TRUE);
    hi_ext_system_sharpen_mpi_update_en_write(ViPipe, HI_TRUE);

    //auto ExtRegs initial
    if (pstSnsDft->stIspSharpen.bInvalid)
    {
        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
        {
            for (j = 0; j < ISP_SHARPEN_GAIN_NUM ; j++)
            {
                hi_ext_system_Isp_sharpen_TextureStr_write (ViPipe, i + j * ISP_AUTO_ISO_STRENGTH_NUM, pstSnsDft->stIspSharpen.stAuto.au16TextureStr[j][i]);
                hi_ext_system_Isp_sharpen_EdgeStr_write(ViPipe, i + j * ISP_AUTO_ISO_STRENGTH_NUM, pstSnsDft->stIspSharpen.stAuto.au16EdgeStr[j][i]);
                hi_ext_system_Isp_sharpen_LumaWgt_write(ViPipe, i + j * ISP_AUTO_ISO_STRENGTH_NUM, pstSnsDft->stIspSharpen.stAuto.au8LumaWgt[j][i]);
            }
            hi_ext_system_Isp_sharpen_TextureFreq_write(ViPipe, i, pstSnsDft->stIspSharpen.stAuto.au16TextureFreq[i]);
            hi_ext_system_Isp_sharpen_EdgeFreq_write(ViPipe, i, pstSnsDft->stIspSharpen.stAuto.au16EdgeFreq[i]);
            hi_ext_system_Isp_sharpen_OverShoot_write(ViPipe, i, pstSnsDft->stIspSharpen.stAuto.au8OverShoot[i]);
            hi_ext_system_Isp_sharpen_UnderShoot_write(ViPipe, i, pstSnsDft->stIspSharpen.stAuto.au8UnderShoot[i]);
            hi_ext_system_Isp_sharpen_shootSupStr_write(ViPipe, i, pstSnsDft->stIspSharpen.stAuto.au8ShootSupStr[i]);
            hi_ext_system_Isp_sharpen_detailctrl_write(ViPipe, i, pstSnsDft->stIspSharpen.stAuto.au8DetailCtrl[i]);
            hi_ext_system_Isp_sharpen_EdgeFiltStr_write(ViPipe, i, pstSnsDft->stIspSharpen.stAuto.au8EdgeFiltStr[i]);
            hi_ext_system_Isp_sharpen_RGain_write(ViPipe, i, pstSnsDft->stIspSharpen.stAuto.au8RGain[i]);
            hi_ext_system_Isp_sharpen_BGain_write(ViPipe, i, pstSnsDft->stIspSharpen.stAuto.au8BGain[i]);
            hi_ext_system_Isp_sharpen_SkinGain_write(ViPipe, i, pstSnsDft->stIspSharpen.stAuto.au8SkinGain[i]);
        }

        //manual ExtRegs initial
        for (i = 0; i < ISP_SHARPEN_GAIN_NUM; i++)
        {
            hi_ext_system_manual_Isp_sharpen_TextureStr_write(ViPipe, i, pstSnsDft->stIspSharpen.stManual.au16TextureStr[i]);
            hi_ext_system_manual_Isp_sharpen_EdgeStr_write(ViPipe, i, pstSnsDft->stIspSharpen.stManual.au16EdgeStr[i]);
            hi_ext_system_manual_Isp_sharpen_LumaWgt_write(ViPipe, i, pstSnsDft->stIspSharpen.stManual.au8LumaWgt[i]);
        }

        hi_ext_system_manual_Isp_sharpen_TextureFreq_write(ViPipe, pstSnsDft->stIspSharpen.stManual.u16TextureFreq);
        hi_ext_system_manual_Isp_sharpen_EdgeFreq_write(ViPipe, pstSnsDft->stIspSharpen.stManual.u16EdgeFreq);
        hi_ext_system_manual_Isp_sharpen_OverShoot_write(ViPipe, pstSnsDft->stIspSharpen.stManual.u8OverShoot);
        hi_ext_system_manual_Isp_sharpen_UnderShoot_write(ViPipe, pstSnsDft->stIspSharpen.stManual.u8UnderShoot);
        hi_ext_system_manual_Isp_sharpen_shootSupStr_write(ViPipe, pstSnsDft->stIspSharpen.stManual.u8ShootSupStr);
        hi_ext_system_manual_Isp_sharpen_detailctrl_write(ViPipe, pstSnsDft->stIspSharpen.stManual.u8DetailCtrl);
        hi_ext_system_manual_Isp_sharpen_EdgeFiltStr_write(ViPipe, pstSnsDft->stIspSharpen.stManual.u8EdgeFiltStr);
        hi_ext_system_manual_Isp_sharpen_RGain_write(ViPipe, pstSnsDft->stIspSharpen.stManual.u8RGain);
        hi_ext_system_manual_Isp_sharpen_BGain_write(ViPipe, pstSnsDft->stIspSharpen.stManual.u8BGain);
        hi_ext_system_manual_Isp_sharpen_SkinGain_write(ViPipe, pstSnsDft->stIspSharpen.stManual.u8SkinGain);

        {
            hi_ext_system_actual_sharpen_overshootAmt_write(ViPipe, pstSnsDft->stIspSharpen.stManual.u8OverShoot);
            hi_ext_system_actual_sharpen_undershootAmt_write(ViPipe, pstSnsDft->stIspSharpen.stManual.u8UnderShoot);
            hi_ext_system_actual_sharpen_shootSupSt_write(ViPipe, pstSnsDft->stIspSharpen.stManual.u8ShootSupStr);
            hi_ext_system_actual_sharpen_edge_frequence_write(ViPipe, pstSnsDft->stIspSharpen.stManual.u16EdgeFreq);
            hi_ext_system_actual_sharpen_texture_frequence_write(ViPipe, pstSnsDft->stIspSharpen.stManual.u16TextureFreq);

            for (i = 0; i < ISP_SHARPEN_GAIN_NUM; i++)
            {
                hi_ext_system_actual_sharpen_edge_str_write(ViPipe, i, pstSnsDft->stIspSharpen.stManual.au16EdgeStr[i]);
                hi_ext_system_actual_sharpen_texture_str_write(ViPipe, i, pstSnsDft->stIspSharpen.stManual.au16TextureStr[i]);
            }
        }
        //for (j = 0; j < ISP_SHARPEN_LUMA_NUM; j++)
        //{
        //      hi_ext_system_manual_Isp_sharpen_LumaWgt_write(ViPipe, j, pstSnsDft->stIspSharpen.au8LumaWgt[j]);
        //}
    }

    else
    {
        //auto ExtRegs initial
        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
        {
            for (j = 0; j < ISP_SHARPEN_GAIN_NUM ; j++)
            {
                hi_ext_system_Isp_sharpen_TextureStr_write (ViPipe, i + j * ISP_AUTO_ISO_STRENGTH_NUM, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_TEXTURESTR_DEFAULT);
                hi_ext_system_Isp_sharpen_EdgeStr_write(ViPipe, i + j * ISP_AUTO_ISO_STRENGTH_NUM, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_EDGESTR_DEFAULT);
                hi_ext_system_Isp_sharpen_LumaWgt_write(ViPipe, i + j * ISP_AUTO_ISO_STRENGTH_NUM, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_LUMAWGT_DEFAULT);
            }
            hi_ext_system_Isp_sharpen_TextureFreq_write(ViPipe, i, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_TEXTUREFREQ_DEFAULT);
            hi_ext_system_Isp_sharpen_EdgeFreq_write(ViPipe, i, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_EDGEFREQ_DEFAULT);
            hi_ext_system_Isp_sharpen_OverShoot_write(ViPipe, i, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_OVERSHOOT_DEFAULT);
            hi_ext_system_Isp_sharpen_UnderShoot_write(ViPipe, i, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_UNDERSHOOT_DEFAULT);
            hi_ext_system_Isp_sharpen_shootSupStr_write(ViPipe, i, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_SHOOTSUPSTR_DEFAULT);
            hi_ext_system_Isp_sharpen_detailctrl_write(ViPipe, i, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_DETAILCTRL_DEFAULT);
            hi_ext_system_Isp_sharpen_EdgeFiltStr_write(ViPipe, i, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_EDGEFILTSTR_DEFAULT);
            hi_ext_system_Isp_sharpen_RGain_write(ViPipe, i, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_RGAIN_DEFAULT);
            hi_ext_system_Isp_sharpen_BGain_write(ViPipe, i, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_BGAIN_DEFAULT);
            hi_ext_system_Isp_sharpen_SkinGain_write(ViPipe, i, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_SKINGAIN_DEFAULT);
        }
        //manual ExtRegs initial
        for (i = 0; i < ISP_SHARPEN_GAIN_NUM; i++)
        {
            hi_ext_system_manual_Isp_sharpen_TextureStr_write(ViPipe, i, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_TEXTURESTR_DEFAULT);
            hi_ext_system_manual_Isp_sharpen_EdgeStr_write(ViPipe, i, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_EDGESTR_DEFAULT);
            hi_ext_system_manual_Isp_sharpen_LumaWgt_write(ViPipe, i, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_LUMAWGT_DEFAULT);
        }

        hi_ext_system_manual_Isp_sharpen_TextureFreq_write(ViPipe, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_TEXTUREFREQ_DEFAULT);
        hi_ext_system_manual_Isp_sharpen_EdgeFreq_write(ViPipe, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_EDGEFREQ_DEFAULT);
        hi_ext_system_manual_Isp_sharpen_OverShoot_write(ViPipe, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_OVERSHOOT_DEFAULT);
        hi_ext_system_manual_Isp_sharpen_UnderShoot_write(ViPipe, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_UNDERSHOOT_DEFAULT);
        hi_ext_system_manual_Isp_sharpen_shootSupStr_write(ViPipe, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_SHOOTSUPSTR_DEFAULT);
        hi_ext_system_manual_Isp_sharpen_detailctrl_write(ViPipe, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_DETAILCTRL_DEFAULT);
        hi_ext_system_manual_Isp_sharpen_EdgeFiltStr_write(ViPipe, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_EDGEFILTSTR_DEFAULT);
        hi_ext_system_manual_Isp_sharpen_RGain_write(ViPipe, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_RGAIN_DEFAULT);
        hi_ext_system_manual_Isp_sharpen_BGain_write(ViPipe, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_BGAIN_DEFAULT);
        hi_ext_system_manual_Isp_sharpen_SkinGain_write(ViPipe, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_SKINGAIN_DEFAULT);
        {
            hi_ext_system_actual_sharpen_overshootAmt_write(ViPipe, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_OVERSHOOT_DEFAULT);
            hi_ext_system_actual_sharpen_undershootAmt_write(ViPipe, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_UNDERSHOOT_DEFAULT);
            hi_ext_system_actual_sharpen_shootSupSt_write(ViPipe, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_SHOOTSUPSTR_DEFAULT);
            hi_ext_system_actual_sharpen_edge_frequence_write(ViPipe, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_EDGEFREQ_DEFAULT);
            hi_ext_system_actual_sharpen_texture_frequence_write(ViPipe, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_TEXTUREFREQ_DEFAULT);

            for (i = 0; i < ISP_SHARPEN_GAIN_NUM; i++)
            {
                hi_ext_system_actual_sharpen_edge_str_write(ViPipe, i, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_EDGESTR_DEFAULT);
                hi_ext_system_actual_sharpen_texture_str_write(ViPipe, i, HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_TEXTURESTR_DEFAULT);
            }
        }
        //for (j = 0; j < ISP_SHARPEN_LUMA_NUM; j++)
        //{
        //      hi_ext_system_manual_Isp_sharpen_LumaWgt_write(ViPipe, j, 127);
        //}
    }

    return;
}

static void SharpenCheckStaticReg(ISP_SHARPEN_STATIC_REG_CFG_S *pstShrpReg)
{
    HI_U32 i;
    // Direction judgement stbEnChrSad
    pstShrpReg->u8bEnChrSad   = SHARPEN_CLIP3(0, 0x1,  pstShrpReg->u8bEnChrSad);         // U1.0
    pstShrpReg->u8dirVarScale = SHARPEN_CLIP3(0, 0xC,  pstShrpReg->u8dirVarScale);       // U4.0
    pstShrpReg->u8mfThdSelD = SHARPEN_CLIP3(0, 0x2, pstShrpReg->u8mfThdSelD);    // U2.0
    pstShrpReg->u8hfThdSelD = SHARPEN_CLIP3(0, 0x2, pstShrpReg->u8hfThdSelD);    // U2.0

    for ( i = 0 ; i < 2; i++ )
    {
        pstShrpReg->u8dirRly[i] = SHARPEN_CLIP3(0, 0x7F,  pstShrpReg->u8dirRly[i]);  // U0.7
    }

    // Control sharpen based on luma
    pstShrpReg->u8lumaSrcSel  = SHARPEN_CLIP3(0, 0x1, pstShrpReg->u8lumaSrcSel);    // U1.0

    // Shoot control
    pstShrpReg->u16oMaxChg  = SHARPEN_CLIP3( 0, 0x3FF, pstShrpReg->u16oMaxChg  ); // U10.0
    pstShrpReg->u16uMaxChg  = SHARPEN_CLIP3( 0, 0x3FF, pstShrpReg->u16uMaxChg  ); // U10.0
    // Control shoot based on variance
    pstShrpReg->u8shtVarSft       = SHARPEN_CLIP3(0, 0x7,  pstShrpReg->u8shtVarSft);         // U3.0
    pstShrpReg->u8ushtVarDiffWgt0 = SHARPEN_CLIP3(0, 0x7F, pstShrpReg->u8ushtVarDiffWgt0);   // U0.7
    pstShrpReg->u8oshtVarDiffWgt0 = SHARPEN_CLIP3(0, 0x7F, pstShrpReg->u8oshtVarDiffWgt0);   // U0.7
    //pstShrpReg->u8ushtVarThd0 = SHARPEN_CLIP3(0, 0xFF, pstShrpReg->u8ushtVarThd0);   // U8.0
    //pstShrpReg->u8oshtVarThd0 = SHARPEN_CLIP3(0, 0xFF, pstShrpReg->u8oshtVarThd0);   // U8.0
    pstShrpReg->u8oshtVarWgt1 = SHARPEN_CLIP3(0, 0x7F, pstShrpReg->u8oshtVarWgt1);   // U0.7
    pstShrpReg->u8ushtVarWgt1 = SHARPEN_CLIP3(0, 0x7F, pstShrpReg->u8ushtVarWgt1);   // U0.7

    // Coring
    for ( i = 0; i < ISP_SHARPEN_FREQ_CORING_LENGTH; i++ )
    {
        pstShrpReg->u8lmtMF[i] = SHARPEN_CLIP3(0, 0x3F, pstShrpReg->u8lmtMF[i]); // U6.0
        pstShrpReg->u8lmtHF[i] = SHARPEN_CLIP3(0, 0x3F, pstShrpReg->u8lmtHF[i]); // U6.0
    }

    // Skin

    pstShrpReg->u8skinSrcSel  = SHARPEN_CLIP3(0, 0x1, pstShrpReg->u8skinSrcSel);    // U1.0

    pstShrpReg->u16skinMaxU = SHARPEN_CLIP3(0, 0x3FF, pstShrpReg->u16skinMaxU);       // U10.0
    pstShrpReg->u16skinMinU = SHARPEN_CLIP3(0, 0x3FF, pstShrpReg->u16skinMinU);       // U10.0
    pstShrpReg->u16skinMaxV = SHARPEN_CLIP3(0, 0x3FF, pstShrpReg->u16skinMaxV);       // U10.0
    pstShrpReg->u16skinMinV = SHARPEN_CLIP3(0, 0x3FF, pstShrpReg->u16skinMinV);       // U10.0

    pstShrpReg->u8skinCntThd[0] = SHARPEN_CLIP3(0, 0x9, pstShrpReg->u8skinCntThd[0]); // U4.0, [0,9]
    pstShrpReg->u8skinCntThd[1] = SHARPEN_CLIP3(0, 0x9, pstShrpReg->u8skinCntThd[1]); // U4.0, [0,9]

    pstShrpReg->u8skinEdgeSft = SHARPEN_CLIP3(0, 0xC, pstShrpReg->u8skinEdgeSft);     // U4.0

    for ( i = 0; i < 2; i++ )
    {
        // pstShrpReg->u8skinEdgeThd[i] = SHARPEN_CLIP3(0, 0xFF, pstShrpReg->u8skinEdgeThd[i]);    // U8.0

        pstShrpReg->u16skinAccumThd[i] = SHARPEN_CLIP3(0, 0x1FF, pstShrpReg->u16skinAccumThd[i]); // U9.0
        pstShrpReg->u8skinAccumWgt[i] = SHARPEN_CLIP3(0, 0x1F, pstShrpReg->u8skinAccumWgt[i]);  // U0.5
    }

    // Chroma modification

    pstShrpReg->u8chrRVarSft   = SHARPEN_CLIP3(0, 0xA, pstShrpReg->u8chrRVarSft);    // U4.0, [0,10]
    pstShrpReg->u8chrRVarScale = SHARPEN_CLIP3(0, 0xA, pstShrpReg->u8chrRVarScale);  // U4.0, [0,10]
    pstShrpReg->u16chrRScale   = SHARPEN_CLIP3(0, 0x1FF, pstShrpReg->u16chrRScale);   // U4.5

    pstShrpReg->u8chrGVarSft   = SHARPEN_CLIP3(0, 0xA, pstShrpReg->u8chrGVarSft);    // U4.0, [0,10]
    pstShrpReg->u8chrGVarScale = SHARPEN_CLIP3(0, 0xA, pstShrpReg->u8chrGVarScale);  // U4.0, [0,10]
    pstShrpReg->u16chrGScale   = SHARPEN_CLIP3(0, 0x1FF, pstShrpReg->u16chrGScale);   // U4.5

    pstShrpReg->u8chrBVarSft   = SHARPEN_CLIP3(0, 0xA, pstShrpReg->u8chrBVarSft);    // U4.0, [0,10]
    pstShrpReg->u8chrBVarScale = SHARPEN_CLIP3(0, 0xA, pstShrpReg->u8chrBVarScale);  // U4.0, [0,10]
    pstShrpReg->u16chrBScale   = SHARPEN_CLIP3(0, 0x1FF, pstShrpReg->u16chrBScale);   // U4.5
    pstShrpReg->u16chrGGain1 =  SHARPEN_CLIP3(0, 0x1FF, pstShrpReg->u16chrGGain1); // U4.5
    pstShrpReg->u8chrRGain1 =  SHARPEN_CLIP3(0, 0x1F, pstShrpReg->u8chrRGain1);  // U0.5
    pstShrpReg->u8chrBGain1 =  SHARPEN_CLIP3(0, 0x1F, pstShrpReg->u8chrBGain1);  // U0.5

    for ( i = 0; i < 2; i++ )
    {
        //pstShrpReg->u8chrROri[i]  =  SHARPEN_CLIP3(0, 0xFF, pstShrpReg->u8chrROri[i]);   // U8.0
        //pstShrpReg->u8chrRThd[i]  =  SHARPEN_CLIP3(0, 0xFF, pstShrpReg->u8chrRThd[i]);   // U8.0
        //pstShrpReg->u8chrRGain[i] =  SHARPEN_CLIP3(0, 0x1F, pstShrpReg->u8chrRGain[i]);  // U0.5

        //pstShrpReg->u8chrGOri[i]  =  SHARPEN_CLIP3(0, 0xFF, pstShrpReg->u8chrGOri[i]);   // U8.0
        //pstShrpReg->u8chrGThd[i]  =  SHARPEN_CLIP3(0, 0xFF, pstShrpReg->u8chrGThd[i]);   // U8.0

        //pstShrpReg->u8chrBOri[i]  =  SHARPEN_CLIP3(0, 0xFF, pstShrpReg->u8chrBOri[i]);   // U8.0
        //pstShrpReg->u8chrBThd[i]  =  SHARPEN_CLIP3(0, 0xFF, pstShrpReg->u8chrBThd[i]);   // U8.0
        //pstShrpReg->u8chrBGain[i] =  SHARPEN_CLIP3(0, 0x1F, pstShrpReg->u8chrBGain[i]);  // U0.5
    }

    for ( i = 0; i < 4; i++ )
    {
        pstShrpReg->u8chrRSft[i]  =  SHARPEN_CLIP3(0, 0x7, pstShrpReg->u8chrRSft[i]);   // U0.3
        pstShrpReg->u8chrGSft[i]  =  SHARPEN_CLIP3(0, 0x7, pstShrpReg->u8chrGSft[i]);   // U0.3
        pstShrpReg->u8chrBSft[i]  =  SHARPEN_CLIP3(0, 0x7, pstShrpReg->u8chrBSft[i]);   // U0.3
    }
    for ( i = 0; i < 2; i++ )
    {
        pstShrpReg->u8dirRt[i]  = SHARPEN_CLIP3(0, 0x1F,  pstShrpReg->u8dirRt[i]);   // U0.5
    }
    //pstShrpReg->u8shtNoiseMax     = SHARPEN_CLIP3(0, 0xFF,  pstShrpReg->u8shtNoiseMax);      // U8.0
    //pstShrpReg->u8shtNoiseMin     = SHARPEN_CLIP3(0, 0xFF,  pstShrpReg->u8shtNoiseMin);
    pstShrpReg->u16oMaxGain = SHARPEN_CLIP3( 0, 0x7FF, pstShrpReg->u16oMaxGain  ); // U11.0
    pstShrpReg->u16uMaxGain = SHARPEN_CLIP3( 0, 0x7FF, pstShrpReg->u16uMaxGain  ); // U11.0

    pstShrpReg->bEnShp8Dir = SHARPEN_CLIP3( 0, 0x1, pstShrpReg->bEnShp8Dir  );

    pstShrpReg->bEnShpLowPow = SHARPEN_CLIP3( 0, 0x1, pstShrpReg->bEnShpLowPow  );
    pstShrpReg->u8hfGainSft = SHARPEN_CLIP3( 0, 0x7, pstShrpReg->u8hfGainSft  );
    pstShrpReg->u8mfGainSft = SHARPEN_CLIP3( 0, 0x7, pstShrpReg->u8mfGainSft  );
    pstShrpReg->u8lpfSel = SHARPEN_CLIP3( 0, 0x1, pstShrpReg->u8lpfSel  );
    pstShrpReg->u8hsfSel = SHARPEN_CLIP3( 0, 0x2, pstShrpReg->u8hsfSel  );
    pstShrpReg->u8shtVarSel = SHARPEN_CLIP3( 0, 0x1, pstShrpReg->u8shtVarSel  );
    pstShrpReg->u8shtVar5x5Sft = SHARPEN_CLIP3( 0, 12, pstShrpReg->u8shtVar5x5Sft  );
    pstShrpReg->u8detailThdSel = SHARPEN_CLIP3( 0, 3, pstShrpReg->u8detailThdSel  );
    pstShrpReg->u8detailThdSft = SHARPEN_CLIP3( 0, 12, pstShrpReg->u8detailThdSft  );
    // pstShrpReg->u8detailOshtThr[0] = SHARPEN_CLIP3( 0, 255, pstShrpReg->u8detailOshtThr[0]  );
    //pstShrpReg->u8detailOshtThr[1] = SHARPEN_CLIP3( 0, 255, pstShrpReg->u8detailOshtThr[1]  );
    //pstShrpReg->u8detailUshtThr[0] = SHARPEN_CLIP3( 0, 255, pstShrpReg->u8detailUshtThr[0]  );
    //pstShrpReg->u8detailUshtThr[1] = SHARPEN_CLIP3( 0, 255, pstShrpReg->u8detailUshtThr[1]  );
    // U4.0
}

static void SharpenCheckDefDynaReg(ISP_SHARPEN_DYNA_REG_CFG_S *pstShrpReg)
{
    //HI_U32 i;
    ISP_SHARPEN_DEFAULT_DYNA_REG_CFG_S *pstSharpDefaultDynaRegCfg;

    pstSharpDefaultDynaRegCfg = &(pstShrpReg->stDefaultDynaRegCfg);

    pstSharpDefaultDynaRegCfg->u8mfThdSftD = SHARPEN_CLIP3(0, 0xf, pstSharpDefaultDynaRegCfg->u8mfThdSftD);    // U4.0

    pstSharpDefaultDynaRegCfg->u8mfThdSelUD  = SHARPEN_CLIP3(0, 0x2, pstSharpDefaultDynaRegCfg->u8mfThdSelUD); // U2.0
    pstSharpDefaultDynaRegCfg->u8mfThdSftUD  = SHARPEN_CLIP3(0, 0xf, pstSharpDefaultDynaRegCfg->u8mfThdSftUD); // U4.0

    pstSharpDefaultDynaRegCfg->u8hfThdSftD = SHARPEN_CLIP3(0, 0xf, pstSharpDefaultDynaRegCfg->u8hfThdSftD);    // U4.0

    pstSharpDefaultDynaRegCfg->u8hfThdSelUD = SHARPEN_CLIP3(0, 0x2, pstSharpDefaultDynaRegCfg->u8hfThdSelUD);  // U2.0
    pstSharpDefaultDynaRegCfg->u8hfThdSftUD = SHARPEN_CLIP3(0, 0xf, pstSharpDefaultDynaRegCfg->u8hfThdSftUD);  // U4.0
    pstSharpDefaultDynaRegCfg->u8dirVarSft   = SHARPEN_CLIP3(0, 0xC,  pstSharpDefaultDynaRegCfg->u8dirVarSft);         // U4.0

    //for ( i = 0; i < 2; i++ )
    //{
    //pstSharpDefaultDynaRegCfg->u8oshtVarDiffThd[i] = SHARPEN_CLIP3(0, 0xFF, pstSharpDefaultDynaRegCfg->u8oshtVarDiffThd[i]);   // U8.0
    //pstSharpDefaultDynaRegCfg->u8ushtVarDiffThd[i] = SHARPEN_CLIP3(0, 0xFF, pstSharpDefaultDynaRegCfg->u8ushtVarDiffThd[i]);//SHARPEN_CLIP3(0, 0xFF, pstSharpDefaultDynaRegCfg->u8ushtVarDiffThd[i]);   // U8.0
    //}
    pstSharpDefaultDynaRegCfg->u8ushtVarDiffWgt1 = SHARPEN_CLIP3(0, 0x7F, pstSharpDefaultDynaRegCfg->u8ushtVarDiffWgt1);   // U0.7
    pstSharpDefaultDynaRegCfg->u8oshtVarDiffWgt1 = SHARPEN_CLIP3(0, 0x7F, pstSharpDefaultDynaRegCfg->u8oshtVarDiffWgt1);   // U0.7
    pstSharpDefaultDynaRegCfg->u8oshtVarWgt0 = SHARPEN_CLIP3(0, 0x7F, pstSharpDefaultDynaRegCfg->u8oshtVarWgt0);   // U0.7
    pstSharpDefaultDynaRegCfg->u8ushtVarWgt0 = SHARPEN_CLIP3(0, 0x7F, pstSharpDefaultDynaRegCfg->u8ushtVarWgt0);   // U0.7
    pstSharpDefaultDynaRegCfg->u16chrGGain0 =  SHARPEN_CLIP3(0, 0x1FF, pstSharpDefaultDynaRegCfg->u16chrGGain0); // U4.5
    pstSharpDefaultDynaRegCfg->u8selPixWgt = SHARPEN_CLIP3( 0, 0x1F,  pstSharpDefaultDynaRegCfg->u8selPixWgt );// U0.5

}

static void SharpenCheckMpiDynaReg(ISP_SHARPEN_DYNA_REG_CFG_S *pstShrpReg)
{
    HI_U32 i;

    ISP_SHARPEN_MPI_DYNA_REG_CFG_S *pstSharpMpiDynaRegCfg;

    pstSharpMpiDynaRegCfg = &(pstShrpReg->stMpiDynaRegCfg);

    //MPI
    pstSharpMpiDynaRegCfg->u8bEnShtCtrlByVar = SHARPEN_CLIP3(0, 0x1,  pstSharpMpiDynaRegCfg->u8bEnShtCtrlByVar);   // U1.0
    pstSharpMpiDynaRegCfg->u8shtBldRt        = SHARPEN_CLIP3(0, 0xF,  pstSharpMpiDynaRegCfg->u8shtBldRt);          // U4.0
    pstSharpMpiDynaRegCfg->u8oshtAmt   = SHARPEN_CLIP3( 0, 0x7F,  pstSharpMpiDynaRegCfg->u8oshtAmt  ); // U0.7
    pstSharpMpiDynaRegCfg->u8ushtAmt   = SHARPEN_CLIP3( 0, 0x7F,  pstSharpMpiDynaRegCfg->u8ushtAmt  ); // U0.7

    pstSharpMpiDynaRegCfg->u8bEnChrCtrl = SHARPEN_CLIP3(0, 0x1, pstSharpMpiDynaRegCfg->u8bEnChrCtrl);    // U1.0
    pstSharpMpiDynaRegCfg->u8chrBGain0 =  SHARPEN_CLIP3(0, 0x1F, pstSharpMpiDynaRegCfg->u8chrBGain0);  // U0.5
    pstSharpMpiDynaRegCfg->u8chrRGain0 =  SHARPEN_CLIP3(0, 0x1F, pstSharpMpiDynaRegCfg->u8chrRGain0);  // U0.5
    pstSharpMpiDynaRegCfg->u8bEnSkinCtrl = SHARPEN_CLIP3(0, 0x1, pstSharpMpiDynaRegCfg->u8bEnSkinCtrl);   // U1.0
    pstSharpMpiDynaRegCfg->u8skinEdgeWgt[0] =  SHARPEN_CLIP3(0, 0x1F, pstSharpMpiDynaRegCfg->u8skinEdgeWgt[0]);  // U0.5
    pstSharpMpiDynaRegCfg->u8skinEdgeWgt[1] =  SHARPEN_CLIP3(0, 0x1F, pstSharpMpiDynaRegCfg->u8skinEdgeWgt[1]);  // U0.5
    pstSharpMpiDynaRegCfg->u8bEnLumaCtrl = SHARPEN_CLIP3(0, 0x1, pstSharpMpiDynaRegCfg->u8bEnLumaCtrl);   // U1.0
    for ( i = 0; i < SHRP_GAIN_LUT_SIZE; i++ )
    {
        pstSharpMpiDynaRegCfg->u16mfGainD[i]  = SHARPEN_CLIP3(0, 0xFFF, pstSharpMpiDynaRegCfg->u16mfGainD[i]);       // U7.5
        pstSharpMpiDynaRegCfg->u16mfGainUD[i] = SHARPEN_CLIP3(0, 0xFFF, pstSharpMpiDynaRegCfg->u16mfGainUD[i]);      // U7.5

        pstSharpMpiDynaRegCfg->u16hfGainD[i]  = SHARPEN_CLIP3(0, 0xFFF, pstSharpMpiDynaRegCfg->u16hfGainD[i]);       // U7.5
        pstSharpMpiDynaRegCfg->u16hfGainUD[i] = SHARPEN_CLIP3(0, 0xFFF, pstSharpMpiDynaRegCfg->u16hfGainUD[i]);      // U7.5
    }
    for ( i = 0; i < ISP_SHARPEN_LUMA_NUM; i++ )
    {
        pstSharpMpiDynaRegCfg->au8LumaWgt[i] = SHARPEN_CLIP3(0, 0x7F,  pstSharpMpiDynaRegCfg->au8LumaWgt[i]);   // U0.7
    }
    pstSharpMpiDynaRegCfg->bEnDetailCtrl   = SHARPEN_CLIP3( 0, 0x1,  pstSharpMpiDynaRegCfg->bEnDetailCtrl  );
    pstSharpMpiDynaRegCfg->u8detailOshtAmt   = SHARPEN_CLIP3( 0, 0x7F,  pstSharpMpiDynaRegCfg->u8detailOshtAmt  );
    pstSharpMpiDynaRegCfg->u8detailUshtAmt   = SHARPEN_CLIP3( 0, 0x7F,  pstSharpMpiDynaRegCfg->u8detailUshtAmt  );
    pstSharpMpiDynaRegCfg->u8dirDiffSft  = SHARPEN_CLIP3(0, 0x3F, pstSharpMpiDynaRegCfg->u8dirDiffSft);        // U6.0

}

//****Sharpen hardware Regs that will not change****//
static HI_VOID SharpenStaticRegInit(VI_PIPE ViPipe, ISP_SHARPEN_STATIC_REG_CFG_S *pstSharpenStaticRegCfg)
{
    HI_U8 i;
    pstSharpenStaticRegCfg->bStaticResh   = HI_TRUE;
    pstSharpenStaticRegCfg->u8hfThdSelD   = 1;
    pstSharpenStaticRegCfg->u8mfThdSelD   = 1;
    pstSharpenStaticRegCfg->u8dirVarScale = 0;
    pstSharpenStaticRegCfg->u8bEnChrSad   = 1;            //static
    pstSharpenStaticRegCfg->u8dirRly[0]   = 127 ;
    pstSharpenStaticRegCfg->u8dirRly[1]   = 0   ;
    pstSharpenStaticRegCfg->u16oMaxChg    = 1000;
    pstSharpenStaticRegCfg->u16uMaxChg    = 1000;
    pstSharpenStaticRegCfg->u8shtVarSft   = 2;
    for ( i = 0; i < ISP_SHARPEN_FREQ_CORING_LENGTH; i++ )
    {
        pstSharpenStaticRegCfg->u8lmtMF[i]  = 0;
        pstSharpenStaticRegCfg->u8lmtHF[i]  = 0;
    }
    pstSharpenStaticRegCfg->u8skinSrcSel        = 0;
    pstSharpenStaticRegCfg->u16skinMaxU         = 511;
    pstSharpenStaticRegCfg->u16skinMinU         = 400;
    pstSharpenStaticRegCfg->u16skinMaxV         = 600;
    pstSharpenStaticRegCfg->u16skinMinV         = 540;
    pstSharpenStaticRegCfg->u8skinCntThd[0]     = 5;
    pstSharpenStaticRegCfg->u8skinEdgeThd[0]    = 10;
    pstSharpenStaticRegCfg->u16skinAccumThd[0]  = 0;
    pstSharpenStaticRegCfg->u8skinAccumWgt[0]   = 0;
    pstSharpenStaticRegCfg->u8skinCntThd[1]     = 8;
    pstSharpenStaticRegCfg->u8skinEdgeThd[1]    = 30;
    pstSharpenStaticRegCfg->u16skinAccumThd[1]  = 10;
    pstSharpenStaticRegCfg->u8skinAccumWgt[1]   = 20;
    pstSharpenStaticRegCfg->u8skinEdgeSft       = 3;
    pstSharpenStaticRegCfg->u8chrROri[0]        = 120;
    pstSharpenStaticRegCfg->u8chrRThd[0]        = 40;
    pstSharpenStaticRegCfg->u8chrGOri[0]        = 95;
    pstSharpenStaticRegCfg->u8chrGThd[0]        = 20;
    pstSharpenStaticRegCfg->u8chrBThd[0]        = 50;
    pstSharpenStaticRegCfg->u8chrBOri[0]        = 200;
    pstSharpenStaticRegCfg->u8chrROri[1]        = 192;
    pstSharpenStaticRegCfg->u8chrRThd[1]        = 60;
    pstSharpenStaticRegCfg->u8chrRGain1         = 31;
    pstSharpenStaticRegCfg->u8chrGOri[1]        = 95;
    pstSharpenStaticRegCfg->u8chrGThd[1]        = 40;
    pstSharpenStaticRegCfg->u16chrGGain1        = 32;
    pstSharpenStaticRegCfg->u8chrBThd[1]        = 100;
    pstSharpenStaticRegCfg->u8chrBGain1         = 31;
    pstSharpenStaticRegCfg->u8chrBOri[1]        = 64;
    pstSharpenStaticRegCfg->u8chrRVarSft        = 5;
    pstSharpenStaticRegCfg->u8chrRVarScale      = 3;
    pstSharpenStaticRegCfg->u16chrRScale        = 32;
    pstSharpenStaticRegCfg->u8chrGSft[0]        = 4;
    pstSharpenStaticRegCfg->u8chrGSft[1]        = 7;
    pstSharpenStaticRegCfg->u8chrGSft[2]        = 4;
    pstSharpenStaticRegCfg->u8chrGSft[3]        = 7;
    pstSharpenStaticRegCfg->u8chrRSft[0]        = 5;
    pstSharpenStaticRegCfg->u8chrRSft[1]        = 7;
    pstSharpenStaticRegCfg->u8chrRSft[2]        = 7;
    pstSharpenStaticRegCfg->u8chrRSft[3]        = 7;
    pstSharpenStaticRegCfg->u8chrBSft[0]        = 7;
    pstSharpenStaticRegCfg->u8chrBSft[1]        = 7;
    pstSharpenStaticRegCfg->u8chrBSft[2]        = 7;
    pstSharpenStaticRegCfg->u8chrBSft[3]        = 7;
    pstSharpenStaticRegCfg->u8chrGVarSft        = 10;
    pstSharpenStaticRegCfg->u8chrGVarScale      = 0;
    pstSharpenStaticRegCfg->u16chrGScale        = 32;
    pstSharpenStaticRegCfg->u8chrBVarSft        = 4;
    pstSharpenStaticRegCfg->u8chrBVarScale      = 3;
    pstSharpenStaticRegCfg->u16chrBScale        = 32;
    pstSharpenStaticRegCfg->u8oshtVarWgt1       = 127;
    pstSharpenStaticRegCfg->u8ushtVarWgt1       = 127;
    pstSharpenStaticRegCfg->u8ushtVarDiffWgt0   = 127;
    pstSharpenStaticRegCfg->u8oshtVarDiffWgt0   = 127;
    pstSharpenStaticRegCfg->u8oshtVarThd0       = 0;
    pstSharpenStaticRegCfg->u8ushtVarThd0       = 0;
    pstSharpenStaticRegCfg->u8lumaSrcSel        = 0;
    pstSharpenStaticRegCfg->u8dirRt[0]          = 8;
    pstSharpenStaticRegCfg->u8dirRt[1]          = 16;
    pstSharpenStaticRegCfg->u8shtNoiseMin       = 0;
    pstSharpenStaticRegCfg->u8shtNoiseMax       = 0;
    pstSharpenStaticRegCfg->u16oMaxGain         = 160;
    pstSharpenStaticRegCfg->u16uMaxGain         = 160;

    pstSharpenStaticRegCfg->bEnShp8Dir = 1;

    pstSharpenStaticRegCfg->bEnShpLowPow = 1;   //choose 5x5 win to calc sad
    pstSharpenStaticRegCfg->u8hfGainSft = 5;
    pstSharpenStaticRegCfg->u8mfGainSft = 5;
    pstSharpenStaticRegCfg->u8lpfSel = 1;
    pstSharpenStaticRegCfg->u8hsfSel = 2;
    pstSharpenStaticRegCfg->u8shtVarSel = 1;
    pstSharpenStaticRegCfg->u8shtVar5x5Sft = 3;
    pstSharpenStaticRegCfg->u8detailThdSel = 0;
    pstSharpenStaticRegCfg->u8detailThdSft = 3;
    pstSharpenStaticRegCfg->u8detailOshtThr[0] = 65;
    pstSharpenStaticRegCfg->u8detailOshtThr[1] = 90;
    pstSharpenStaticRegCfg->u8detailUshtThr[0] = 65;
    pstSharpenStaticRegCfg->u8detailUshtThr[1] = 90;

    SharpenCheckStaticReg(pstSharpenStaticRegCfg);

    // Skin detection
    pstSharpenStaticRegCfg->u8skinCntMul = CalcMulCoef( pstSharpenStaticRegCfg->u8skinCntThd[0], 0, pstSharpenStaticRegCfg->u8skinCntThd[1], 31, 0);



    pstSharpenStaticRegCfg->s16skinAccumMul =  CalcMulCoef( pstSharpenStaticRegCfg->u16skinAccumThd[0], pstSharpenStaticRegCfg->u8skinAccumWgt[0],
            pstSharpenStaticRegCfg->u16skinAccumThd[1], pstSharpenStaticRegCfg->u8skinAccumWgt[1],
            SHRP_SKIN_ACCUM_MUL_PRECS );


}

//****Sharpen hardware Regs that will change with MPI and ISO****//
static HI_VOID SharpenMpiDynaRegInit(ISP_SHARPEN_MPI_DYNA_REG_CFG_S *pstSharpenMpiDynaRegCfg)
{
    HI_U8 i;

    for (i = 0; i < SHRP_GAIN_LUT_SIZE; i++)
    {
        pstSharpenMpiDynaRegCfg->u16mfGainD[i]  = 300;
        pstSharpenMpiDynaRegCfg->u16mfGainUD[i] = 200;
        pstSharpenMpiDynaRegCfg->u16hfGainD[i]  = 450;
        pstSharpenMpiDynaRegCfg->u16hfGainUD[i] = 400;
    }

    pstSharpenMpiDynaRegCfg->u8oshtAmt          = 100;
    pstSharpenMpiDynaRegCfg->u8ushtAmt          = 127;
    pstSharpenMpiDynaRegCfg->u8bEnShtCtrlByVar  = 1;
    pstSharpenMpiDynaRegCfg->u8shtBldRt         = 9;
    pstSharpenMpiDynaRegCfg->u8oshtVarThd1      = 5;
    pstSharpenMpiDynaRegCfg->u8ushtVarThd1      = 5;
    pstSharpenMpiDynaRegCfg->u8bEnSkinCtrl      = 0   ;
    pstSharpenMpiDynaRegCfg->u8skinEdgeWgt[1]   = 31;
    pstSharpenMpiDynaRegCfg->u8skinEdgeWgt[0]   = 31;
    pstSharpenMpiDynaRegCfg->u8bEnChrCtrl       = 1;
    pstSharpenMpiDynaRegCfg->u8chrBGain0        = 31;
    pstSharpenMpiDynaRegCfg->u8chrRGain0        = 31;

    pstSharpenMpiDynaRegCfg->u8bEnLumaCtrl      = 0;
    pstSharpenMpiDynaRegCfg->bEnDetailCtrl      = 0;
    pstSharpenMpiDynaRegCfg->u8detailOshtAmt    = 100;
    pstSharpenMpiDynaRegCfg->u8detailUshtAmt    = 127;
    pstSharpenMpiDynaRegCfg->u8dirDiffSft = 10;

    for ( i = 0; i < ISP_SHARPEN_LUMA_NUM; i++ )
    {
        pstSharpenMpiDynaRegCfg->au8LumaWgt[i]  = 127;
    }

    pstSharpenMpiDynaRegCfg->u32UpdateIndex    = 1;
    pstSharpenMpiDynaRegCfg->bResh             = HI_TRUE;


}

//****Sharpen hardware Regs that will change only with ISO****//
static HI_VOID SharpenDefaultDynaRegInit(ISP_SHARPEN_DEFAULT_DYNA_REG_CFG_S *pstSharpenDefaultDynaRegCfg)
{
    pstSharpenDefaultDynaRegCfg->bResh   =   HI_TRUE;
    pstSharpenDefaultDynaRegCfg->u8mfThdSftD            =   2;
    pstSharpenDefaultDynaRegCfg->u8mfThdSelUD           =   2;
    pstSharpenDefaultDynaRegCfg->u8mfThdSftUD           =   0;
    pstSharpenDefaultDynaRegCfg->u8hfThdSftD            =   2;
    pstSharpenDefaultDynaRegCfg->u8hfThdSelUD           =   2;
    pstSharpenDefaultDynaRegCfg->u8hfThdSftUD           =   0;
    pstSharpenDefaultDynaRegCfg->u8dirVarSft        =   12;
    pstSharpenDefaultDynaRegCfg->u8selPixWgt            = 31;
    pstSharpenDefaultDynaRegCfg->u8oshtVarDiffThd[0]    =       20;
    pstSharpenDefaultDynaRegCfg->u8ushtVarDiffThd[0]    =       20;
    pstSharpenDefaultDynaRegCfg->u8oshtVarWgt0      =   0;
    pstSharpenDefaultDynaRegCfg->u8ushtVarWgt0      =   0;
    pstSharpenDefaultDynaRegCfg->u8oshtVarDiffThd[1]    =       35;
    pstSharpenDefaultDynaRegCfg->u8oshtVarDiffWgt1  =       5;
    pstSharpenDefaultDynaRegCfg->u8ushtVarDiffThd[1]    =       35;
    pstSharpenDefaultDynaRegCfg->u8ushtVarDiffWgt1  =       10;
    pstSharpenDefaultDynaRegCfg->u16chrGGain0   =       48;

}

static HI_VOID SharpenDynaRegInit(ISP_SHARPEN_REG_CFG_S *pstSharpenRegCfg)
{
    ISP_SHARPEN_DEFAULT_DYNA_REG_CFG_S *pstSharpenDefaultDynaRegCfg;
    ISP_SHARPEN_MPI_DYNA_REG_CFG_S *pstSharpenMpiDynaRegCfg;
    ISP_SHARPEN_STATIC_REG_CFG_S *pstSharpenStaticRegCfg;

    pstSharpenDefaultDynaRegCfg = &(pstSharpenRegCfg->stDynaRegCfg.stDefaultDynaRegCfg);
    pstSharpenMpiDynaRegCfg = &(pstSharpenRegCfg->stDynaRegCfg.stMpiDynaRegCfg);
    pstSharpenStaticRegCfg = &(pstSharpenRegCfg->stStaticRegCfg);

    SharpenDefaultDynaRegInit(pstSharpenDefaultDynaRegCfg);
    SharpenMpiDynaRegInit(pstSharpenMpiDynaRegCfg);

    SharpenCheckDefDynaReg(&(pstSharpenRegCfg->stDynaRegCfg));
    SharpenCheckMpiDynaReg(&(pstSharpenRegCfg->stDynaRegCfg));

    /* Calc all MulCoef */
    // Control shoot based on variance
    pstSharpenMpiDynaRegCfg->u16oshtVarMul = CalcMulCoef( pstSharpenStaticRegCfg->u8oshtVarThd0, pstSharpenDefaultDynaRegCfg->u8oshtVarWgt0,
            pstSharpenMpiDynaRegCfg->u8oshtVarThd1, pstSharpenStaticRegCfg->u8oshtVarWgt1,
            SHRP_SHT_VAR_MUL_PRECS );

    pstSharpenMpiDynaRegCfg->u16ushtVarMul = CalcMulCoef( pstSharpenStaticRegCfg->u8ushtVarThd0, pstSharpenDefaultDynaRegCfg->u8ushtVarWgt0,
            pstSharpenMpiDynaRegCfg->u8ushtVarThd1, pstSharpenStaticRegCfg->u8ushtVarWgt1,
            SHRP_SHT_VAR_MUL_PRECS );
    pstSharpenDefaultDynaRegCfg->s16oshtVarDiffMul = CalcMulCoef( pstSharpenDefaultDynaRegCfg->u8oshtVarDiffThd[0], pstSharpenStaticRegCfg->u8oshtVarDiffWgt0,
            pstSharpenDefaultDynaRegCfg->u8oshtVarDiffThd[1], pstSharpenDefaultDynaRegCfg->u8oshtVarDiffWgt1,
            SHRP_SHT_VAR_MUL_PRECS );

    pstSharpenDefaultDynaRegCfg->s16ushtVarDiffMul = CalcMulCoef( pstSharpenDefaultDynaRegCfg->u8ushtVarDiffThd[0], pstSharpenStaticRegCfg->u8ushtVarDiffWgt0,
            pstSharpenDefaultDynaRegCfg->u8ushtVarDiffThd[1], pstSharpenDefaultDynaRegCfg->u8ushtVarDiffWgt1,
            SHRP_SHT_VAR_MUL_PRECS );
    pstSharpenDefaultDynaRegCfg->s16chrGMul = CalcMulCoef( pstSharpenStaticRegCfg->u8chrGThd[0], pstSharpenDefaultDynaRegCfg->u16chrGGain0,
            pstSharpenStaticRegCfg->u8chrGThd[1], pstSharpenStaticRegCfg->u16chrGGain1,
            SHRP_CHR_MUL_SFT );
    pstSharpenMpiDynaRegCfg->s16detailOshtMul = CalcMulCoef( pstSharpenStaticRegCfg->u8detailOshtThr[0], pstSharpenMpiDynaRegCfg->u8detailOshtAmt,
            pstSharpenStaticRegCfg->u8detailOshtThr[1], pstSharpenMpiDynaRegCfg->u8oshtAmt,
            SHRP_DETAIL_SHT_MUL_PRECS );

    pstSharpenMpiDynaRegCfg->s16detailUshtMul = CalcMulCoef( pstSharpenStaticRegCfg->u8detailUshtThr[0], pstSharpenMpiDynaRegCfg->u8detailUshtAmt,
            pstSharpenStaticRegCfg->u8detailUshtThr[1], pstSharpenMpiDynaRegCfg->u8ushtAmt,
            SHRP_DETAIL_SHT_MUL_PRECS );
    // Chroma modification
    pstSharpenMpiDynaRegCfg->s16chrRMul = CalcMulCoef( pstSharpenStaticRegCfg->u8chrRThd[0], pstSharpenMpiDynaRegCfg->u8chrRGain0,
                                          pstSharpenStaticRegCfg->u8chrRThd[1], pstSharpenStaticRegCfg->u8chrRGain1,
                                          SHRP_CHR_MUL_SFT );
    pstSharpenMpiDynaRegCfg->s16chrBMul = CalcMulCoef( pstSharpenStaticRegCfg->u8chrBThd[0], pstSharpenMpiDynaRegCfg->u8chrBGain0,
                                          pstSharpenStaticRegCfg->u8chrBThd[1], pstSharpenStaticRegCfg->u8chrBGain1,
                                          SHRP_CHR_MUL_SFT );

    pstSharpenMpiDynaRegCfg->s16skinEdgeMul  =  CalcMulCoef( pstSharpenStaticRegCfg->u8skinEdgeThd[0], pstSharpenMpiDynaRegCfg->u8skinEdgeWgt[0],
            pstSharpenStaticRegCfg->u8skinEdgeThd[1], pstSharpenMpiDynaRegCfg->u8skinEdgeWgt[1],
            SHRP_SKIN_EDGE_MUL_PRECS );


}

static HI_VOID SharpenRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pRegCfg)
{
    HI_U32 i;

    for (i = 0; i < pRegCfg->u8CfgNum; i++)
    {
        pRegCfg->stAlgRegCfg[i].stSharpenRegCfg.bEnable = HI_TRUE;
        SharpenStaticRegInit(ViPipe, &(pRegCfg->stAlgRegCfg[i].stSharpenRegCfg.stStaticRegCfg));
        SharpenDynaRegInit(&(pRegCfg->stAlgRegCfg[i].stSharpenRegCfg));
    }

    pRegCfg->unKey.bit1SharpenCfg = 1;

    return;
}


static HI_S32 SharpenReadExtregs(VI_PIPE ViPipe)
{
    HI_U8 i, j;
    ISP_SHARPEN_S *pstSharpen = HI_NULL;

    SHARPEN_GET_CTX(ViPipe, pstSharpen);
    pstSharpen->bSharpenMpiUpdateEn = hi_ext_system_sharpen_mpi_update_en_read(ViPipe);

    hi_ext_system_sharpen_mpi_update_en_write(ViPipe, HI_FALSE);

    if (pstSharpen->bSharpenMpiUpdateEn)
    {
        pstSharpen->u8ManualSharpenYuvEnabled = hi_ext_system_isp_sharpen_manu_mode_read(ViPipe);

        if (pstSharpen->u8ManualSharpenYuvEnabled == OP_TYPE_MANUAL)
        {
            for (j = 0; j < ISP_SHARPEN_GAIN_NUM; j++)
            {
                pstSharpen->au16TextureStr[j] = hi_ext_system_manual_Isp_sharpen_TextureStr_read(ViPipe, j);
                pstSharpen->au16EdgeStr[j] = hi_ext_system_manual_Isp_sharpen_EdgeStr_read(ViPipe, j);
                pstSharpen->au8LumaWgt[j] = hi_ext_system_manual_Isp_sharpen_LumaWgt_read(ViPipe, j);
            }
            pstSharpen->u16TextureFreq = hi_ext_system_manual_Isp_sharpen_TextureFreq_read(ViPipe);
            pstSharpen->u16EdgeFreq = hi_ext_system_manual_Isp_sharpen_EdgeFreq_read(ViPipe);
            pstSharpen->u8OverShoot = hi_ext_system_manual_Isp_sharpen_OverShoot_read(ViPipe);
            pstSharpen->u8UnderShoot = hi_ext_system_manual_Isp_sharpen_UnderShoot_read(ViPipe);
            pstSharpen->u8ShootSupStr = hi_ext_system_manual_Isp_sharpen_shootSupStr_read(ViPipe);
            pstSharpen->u8DetailCtrl = hi_ext_system_manual_Isp_sharpen_detailctrl_read(ViPipe);
            pstSharpen->u8EdgeFiltStr = hi_ext_system_manual_Isp_sharpen_EdgeFiltStr_read(ViPipe);
            pstSharpen->u8RGain = hi_ext_system_manual_Isp_sharpen_RGain_read(ViPipe);
            pstSharpen->u8BGain = hi_ext_system_manual_Isp_sharpen_BGain_read(ViPipe);
            pstSharpen->u8SkinGain = hi_ext_system_manual_Isp_sharpen_SkinGain_read(ViPipe);
        }
        else
        {
            for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
            {
                for (j = 0; j < ISP_SHARPEN_GAIN_NUM; j++)
                {
                    pstSharpen->au16AutoTextureStr[j][i] = hi_ext_system_Isp_sharpen_TextureStr_read(ViPipe, i + j * ISP_AUTO_ISO_STRENGTH_NUM);
                    pstSharpen->au16AutoEdgeStr[j][i] = hi_ext_system_Isp_sharpen_EdgeStr_read(ViPipe, i + j * ISP_AUTO_ISO_STRENGTH_NUM);
                    pstSharpen->au8AutoLumaWgt[j][i] = hi_ext_system_Isp_sharpen_LumaWgt_read(ViPipe, i + j * ISP_AUTO_ISO_STRENGTH_NUM);
                }
                pstSharpen->au16TextureFreq[i] = hi_ext_system_Isp_sharpen_TextureFreq_read(ViPipe, i);
                pstSharpen->au16EdgeFreq[i] = hi_ext_system_Isp_sharpen_EdgeFreq_read(ViPipe, i);
                pstSharpen->au8OverShoot[i] = hi_ext_system_Isp_sharpen_OverShoot_read(ViPipe, i);
                pstSharpen->au8UnderShoot[i] = hi_ext_system_Isp_sharpen_UnderShoot_read(ViPipe, i);
                pstSharpen->au8ShootSupStr[i] = hi_ext_system_Isp_sharpen_shootSupStr_read(ViPipe, i);
                pstSharpen->au8DetailCtrl[i] = hi_ext_system_Isp_sharpen_detailctrl_read(ViPipe, i);
                pstSharpen->au8EdgeFiltStr[i] = hi_ext_system_Isp_sharpen_EdgeFiltStr_read(ViPipe, i);
                pstSharpen->au8RGain[i] = hi_ext_system_Isp_sharpen_RGain_read(ViPipe, i);
                pstSharpen->au8BGain[i] = hi_ext_system_Isp_sharpen_BGain_read(ViPipe, i);
                pstSharpen->au8SkinGain[i] = hi_ext_system_Isp_sharpen_SkinGain_read(ViPipe, i);
            }
        }
        //for (j = 0; j < ISP_SHARPEN_LUMA_NUM; j++)
        //{
        //  pstSharpen->au8LumaWgt[j] = hi_ext_system_manual_Isp_sharpen_LumaWgt_read(ViPipe, j);
        //}

    }


    return 0;
}

static HI_S32 SharpenReadProMode(VI_PIPE ViPipe)
{
    HI_U8 i, j;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_SHARPEN_S *pstSharpen = HI_NULL;
    HI_U8 u8Index = 0;
    ISP_GET_CTX(ViPipe, pstIspCtx);
    SHARPEN_GET_CTX(ViPipe, pstSharpen);
    if (HI_TRUE == pstIspCtx->stProShpParamCtrl.pstProShpParam->bEnable)
    {
        u8Index = pstIspCtx->stLinkage.u8ProIndex;
        if (u8Index < 1)
        {
            return HI_SUCCESS;
        }
        u8Index -= 1;
    }
    else
    {
        return HI_SUCCESS;
    }
    pstSharpen->u8ManualSharpenYuvEnabled = OP_TYPE_AUTO;
    pstSharpen->bSharpenMpiUpdateEn = HI_TRUE;
    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        for (j = 0; j < ISP_SHARPEN_GAIN_NUM; j++)
        {
            pstSharpen->au16AutoTextureStr[j][i] = pstIspCtx->stProShpParamCtrl.pstProShpParam->astShpAttr[u8Index].au16TextureStr[j][i];
            pstSharpen->au16AutoEdgeStr[j][i] = pstIspCtx->stProShpParamCtrl.pstProShpParam->astShpAttr[u8Index].au16EdgeStr[j][i];
            pstSharpen->au8AutoLumaWgt[j][i] = pstIspCtx->stProShpParamCtrl.pstProShpParam->astShpAttr[u8Index].au8LumaWgt[j][i];
        }
        pstSharpen->au16TextureFreq[i] = pstIspCtx->stProShpParamCtrl.pstProShpParam->astShpAttr[u8Index].au16TextureFreq[i];
        pstSharpen->au16EdgeFreq[i] = pstIspCtx->stProShpParamCtrl.pstProShpParam->astShpAttr[u8Index].au16EdgeFreq[i];
        pstSharpen->au8OverShoot[i] = pstIspCtx->stProShpParamCtrl.pstProShpParam->astShpAttr[u8Index].au8OverShoot[i];
        pstSharpen->au8UnderShoot[i] = pstIspCtx->stProShpParamCtrl.pstProShpParam->astShpAttr[u8Index].au8UnderShoot[i];
        pstSharpen->au8ShootSupStr[i] = pstIspCtx->stProShpParamCtrl.pstProShpParam->astShpAttr[u8Index].au8ShootSupStr[i];
        pstSharpen->au8DetailCtrl[i] = pstIspCtx->stProShpParamCtrl.pstProShpParam->astShpAttr[u8Index].au8DetailCtrl[i];
        pstSharpen->au8EdgeFiltStr[i] = pstIspCtx->stProShpParamCtrl.pstProShpParam->astShpAttr[u8Index].au8EdgeFiltStr[i];
        pstSharpen->au8RGain[i] = pstIspCtx->stProShpParamCtrl.pstProShpParam->astShpAttr[u8Index].au8RGain[i];
        pstSharpen->au8BGain[i] = pstIspCtx->stProShpParamCtrl.pstProShpParam->astShpAttr[u8Index].au8BGain[i];
        pstSharpen->au8SkinGain[i] = pstIspCtx->stProShpParamCtrl.pstProShpParam->astShpAttr[u8Index].au8SkinGain[i];
    }
    return HI_SUCCESS;
}
static HI_S32 ISP_SharpenCtxInit(ISP_SHARPEN_S *pstSharpen)
{
    HI_U8 i, j;
    pstSharpen->bSharpenEn = 1;
    pstSharpen->bSharpenMpiUpdateEn = 1;
    pstSharpen->u32IsoLast = 0;
    /* Sharpening Yuv */
    //tmp registers
    pstSharpen->u8ManualSharpenYuvEnabled = 1;
    pstSharpen->u8mfThdSftD = 0;
    pstSharpen->u8dirVarSft = 12;
    //HI_U8 u8dirRt0;
    //HI_U8 u8dirRt1;
    pstSharpen->u8selPixWgt = 31;
    //HI_U8 u8shtNoiseMin;
    pstSharpen->u16chrGGain0 = 32;
    pstSharpen->u8mfThdSelUD = 2;
    pstSharpen->u8mfThdSftUD = 0;

    pstSharpen->u8oshtVarWgt0 = 10;
    pstSharpen->u8ushtVarWgt0 = 20;
    pstSharpen->u8oshtVarDiffThd0 = 20;
    pstSharpen->u8oshtVarDiffThd1 = 35;
    pstSharpen->u8oshtVarDiffWgt1 = 20;
    pstSharpen->u8ushtVarDiffWgt1 = 35;
    //MPI
    for (i = 0; i < ISP_SHARPEN_GAIN_NUM; i++)
    {
        pstSharpen->au16TextureStr[i] = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_TEXTURESTR_DEFAULT;
        pstSharpen->au16EdgeStr[i] = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_EDGESTR_DEFAULT;
        pstSharpen->au8LumaWgt[i] = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_LUMAWGT_DEFAULT;

    }
    pstSharpen->u16TextureFreq = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_TEXTUREFREQ_DEFAULT;         //Texture frequency adjustment. Texture and detail will be finer when it increase.    U6.6  [0, 4095]
    pstSharpen->u16EdgeFreq = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_EDGEFREQ_DEFAULT;            //Edge frequency adjustment. Edge will be narrower and thiner when it increase.      U6.6  [0, 4095]
    pstSharpen->u8OverShoot = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_OVERSHOOT_DEFAULT;          //u8OvershootAmt     U7.0  [0, 127]
    pstSharpen->u8UnderShoot = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_UNDERSHOOT_DEFAULT;         //u8UndershootAmt       U7.0  [0, 127]
    pstSharpen->u8ShootSupStr = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_SHOOTSUPSTR_DEFAULT;        //overshoot and undershoot suppression strength, the amplitude and width of shoot will be decrease when shootSupSt increase.  U10.0    [0, 255]
    pstSharpen->u8DetailCtrl = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_DETAILCTRL_DEFAULT;
    pstSharpen->u8EdgeFiltStr = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_EDGEFILTSTR_DEFAULT;
    pstSharpen->u8RGain = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_RGAIN_DEFAULT;
    pstSharpen->u8BGain = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_BGAIN_DEFAULT;
    pstSharpen->u8SkinGain = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_SKINGAIN_DEFAULT;

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        for (j = 0; j < ISP_SHARPEN_GAIN_NUM ; j++)
        {
            pstSharpen->au16AutoTextureStr[j][i] = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_TEXTURESTR_DEFAULT;
            pstSharpen->au16AutoEdgeStr[j][i] = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_EDGESTR_DEFAULT;
            pstSharpen->au8AutoLumaWgt[j][i] = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_LUMAWGT_DEFAULT;
        }
        pstSharpen->au16TextureFreq[i] = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_TEXTUREFREQ_DEFAULT;         //Texture frequency adjustment. Texture and detail will be finer when it increase.    U6.6  [0, 4095]
        pstSharpen->au16EdgeFreq[i] = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_EDGEFREQ_DEFAULT;        //Edge frequency adjustment. Edge will be narrower and thiner when it increase.      U6.6  [0, 4095]
        pstSharpen->au8OverShoot[i] = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_OVERSHOOT_DEFAULT;          //u8OvershootAmt     U7.0  [0, 127]
        pstSharpen->au8UnderShoot[i] = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_UNDERSHOOT_DEFAULT;         //u8UndershootAmt       U7.0  [0, 127]
        pstSharpen->au8ShootSupStr[i] = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_SHOOTSUPSTR_DEFAULT;        //overshoot and undershoot suppression strength, the amplitude and width of shoot will be decrease when shootSupSt increase.  U10.0    [0, 1023]
        pstSharpen->au8DetailCtrl[i] = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_DETAILCTRL_DEFAULT;
        pstSharpen->au8EdgeFiltStr[i] = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_EDGEFILTSTR_DEFAULT;
        pstSharpen->au8RGain[i] = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_RGAIN_DEFAULT;
        pstSharpen->au8BGain[i] = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_BGAIN_DEFAULT;
        pstSharpen->au8SkinGain[i] = HI_EXT_SYSTEM_MANUAL_ISP_SHARPEN_SKINGAIN_DEFAULT;
    }

    //for (i = 0; i < ISP_SHARPEN_LUMA_NUM; i++)
    //{
    //  pstSharpen->au8LumaWgt[i] = 127;        //Adjust the sharpen strength according to luma. Sharpen strength will be weaker when it decrease. U7.0  [0, 127]
    //
    //}
    return HI_SUCCESS;
}

HI_S32 ISP_SharpenInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_SHARPEN_S *pstSharpen = HI_NULL;
    SHARPEN_GET_CTX(ViPipe, pstSharpen);

    pstSharpen->u8ManualSharpenYuvEnabled = 1;
    pstSharpen->bSharpenEn = HI_TRUE;
    pstSharpen->u32IsoLast = 0;

    ISP_SharpenCtxInit(pstSharpen);
    SharpenRegsInitialize(ViPipe, (ISP_REG_CFG_S *)pRegCfg);
    SharpenExtRegsInitialize(ViPipe);

    return HI_SUCCESS;
}

static HI_VOID ISP_SharpenWdrModeSet(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    HI_U8  i;
    HI_U32 au32UpdateIdx[ISP_STRIPING_MAX_NUM] = {0};
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        au32UpdateIdx[i] = pstRegCfg->stAlgRegCfg[i].stSharpenRegCfg.stDynaRegCfg.stMpiDynaRegCfg.u32UpdateIndex;
    }

    ISP_SharpenInit(ViPipe, pRegCfg);

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        pstRegCfg->stAlgRegCfg[i].stSharpenRegCfg.stDynaRegCfg.stMpiDynaRegCfg.u32UpdateIndex = au32UpdateIdx[i] + 1;
    }
}


static HI_S32 ISP_Sharpen_GetLinearDefaultRegCfg(ISP_SHARPEN_S *pstSharpenPara, HI_U32 u32ISO, HI_U32  idxCur, HI_U32 idxPre, HI_U32  isoLvlCur, HI_U32 isoLvlPre)
{
    //Linear mode defalt regs
    const HI_U8  u8mfThdSelUDLinear[ISP_AUTO_ISO_STRENGTH_NUM] = {2   ,    2   ,    2   ,     2   ,    2    ,    1    ,    1    ,    1     ,    1    ,    1    ,    1    ,    1    ,    1    ,    1    ,    1    ,    1};
    const HI_U8  u8mfThdSftUDLinear[ISP_AUTO_ISO_STRENGTH_NUM] = {0   ,    0    ,   0   ,     0   ,    0    ,    2    ,    2    ,    2     ,    2    ,    2    ,    2    ,    2    ,    2    ,    2    ,    2    ,    2};
    //const HI_U8  u8dirDiffSftLinear[ISP_AUTO_ISO_STRENGTH_NUM]={10  ,    10   ,   10   ,    12  ,    15   ,    20   ,    20   ,    20    ,    20   ,    20   ,   20    ,   20    ,   20    ,   20    ,   20    ,   20};
    //const HI_U16 u16oMaxGainLinear[ISP_AUTO_ISO_STRENGTH_NUM]={900,       900,      900,      900,      900,       900,       900,        900,      900,       900,       900,        900,       900,      900,       900,       900};
    //const HI_U16 u16uMaxGainLinear[ISP_AUTO_ISO_STRENGTH_NUM]={900,       900,      900,      900,      900,       900,       900,        900,      900,       900,       900,        900,       900,      900,       900,       900};
    const HI_U8  u8oshtVarWgt0Linear[ISP_AUTO_ISO_STRENGTH_NUM] = {20   ,   20   ,   20   ,    20   ,   20    ,   20    ,   20    ,   20     ,   20    ,   20    ,   20    ,   20    ,   20    ,   20    ,   20    ,   20};
    const HI_U8  u8ushtVarWgt0Linear[ISP_AUTO_ISO_STRENGTH_NUM] = {30   ,   30   ,   30   ,    30   ,   30    ,   30    ,   30    ,   30     ,   30    ,   30    ,   30    ,   30    ,   30    ,   30    ,   30    ,   30};
    const HI_U8  u8oshtVarDiffThd0Linear[ISP_AUTO_ISO_STRENGTH_NUM] = {20,       22,      23,      25,      28,       30,       32,        36,       37,        38,      39,        40,       40,      40,        40,      40};
    const HI_U8  u8oshtVarDiffThd1Linear[ISP_AUTO_ISO_STRENGTH_NUM] = {35,       37,      38,      40,      40,       43,       43,        46,       47,        48,      49,        50,       50,      50,        50,      50};
    const HI_U8  u8oshtVarDiffWgt1Linear[ISP_AUTO_ISO_STRENGTH_NUM] = {0   ,     0  ,     0   ,    0  ,     5   ,     10   ,    15   ,     18    ,   20    ,    20   ,   20    ,   20    ,   20    ,   20    ,   20    ,   20};
    const HI_U8  u8ushtVarDiffWgt1Linear[ISP_AUTO_ISO_STRENGTH_NUM] = {30  ,     30  ,    30  ,     30  ,    30   ,    30   ,    30   ,    30    ,    30   ,    30   ,    20   ,    10   ,    10   ,    10   ,    10   ,    10 };

    if (u8mfThdSelUDLinear[idxPre] == u8mfThdSelUDLinear[idxCur])
    {
        pstSharpenPara->u8mfThdSelUD   =  u8mfThdSelUDLinear[idxCur];
        pstSharpenPara->u8mfThdSftUD    =  LinearInter(u32ISO, isoLvlPre, u8mfThdSftUDLinear[idxPre],
                                           isoLvlCur, u8mfThdSftUDLinear[idxCur]);
    }
    else
    {
        if ((u32ISO << 1) < (isoLvlPre + isoLvlCur))
        {
            pstSharpenPara->u8mfThdSelUD   =  u8mfThdSelUDLinear[idxPre];
            pstSharpenPara->u8mfThdSftUD   =  u8mfThdSftUDLinear[idxPre];
        }
        else
        {
            pstSharpenPara->u8mfThdSelUD   =  u8mfThdSelUDLinear[idxCur];
            pstSharpenPara->u8mfThdSftUD   =  u8mfThdSftUDLinear[idxCur];
        }
    }
    //pstSharpenPara->u8dirDiffSft  =  LinearInter(u32ISO, isoLvlPre, u8dirDiffSftLinear[idxPre],
    //                                                          isoLvlCur, u8dirDiffSftLinear[idxCur]);
    //pstSharpenPara->u16oMaxGain   =  LinearInter(u32ISO, isoLvlPre, u16oMaxGainLinear[idxPre],
    //                                                          isoLvlCur, u16oMaxGainLinear[idxCur]);
    //pstSharpenPara->u16uMaxGain   =  LinearInter(u32ISO, isoLvlPre, u16uMaxGainLinear[idxPre],
    //                                                          isoLvlCur, u16uMaxGainLinear[idxCur]);
    pstSharpenPara->u8oshtVarWgt0 =  LinearInter(u32ISO, isoLvlPre, u8oshtVarWgt0Linear[idxPre],
                                     isoLvlCur, u8oshtVarWgt0Linear[idxCur]);
    pstSharpenPara->u8ushtVarWgt0 =  LinearInter(u32ISO, isoLvlPre, u8ushtVarWgt0Linear[idxPre],
                                     isoLvlCur, u8ushtVarWgt0Linear[idxCur]);
    pstSharpenPara->u8oshtVarDiffThd0 =  LinearInter(u32ISO, isoLvlPre, u8oshtVarDiffThd0Linear[idxPre],
                                         isoLvlCur, u8oshtVarDiffThd0Linear[idxCur]);
    pstSharpenPara->u8oshtVarDiffThd1 =  LinearInter(u32ISO, isoLvlPre, u8oshtVarDiffThd1Linear[idxPre],
                                         isoLvlCur, u8oshtVarDiffThd1Linear[idxCur]);
    pstSharpenPara->u8oshtVarDiffWgt1 =  LinearInter(u32ISO, isoLvlPre, u8oshtVarDiffWgt1Linear[idxPre],
                                         isoLvlCur, u8oshtVarDiffWgt1Linear[idxCur]);
    pstSharpenPara->u8ushtVarDiffWgt1 =  LinearInter(u32ISO, isoLvlPre, u8ushtVarDiffWgt1Linear[idxPre],
                                         isoLvlCur, u8ushtVarDiffWgt1Linear[idxCur]);
    return HI_SUCCESS;
}

static HI_S32 ISP_Sharpen_GetWdrDefaultRegCfg(ISP_SHARPEN_S *pstSharpenPara, HI_U32 u32ISO, HI_U32  idxCur, HI_U32 idxPre, HI_U32  isoLvlCur, HI_U32 isoLvlPre)
{
    //WDR mode defalt regs
    const HI_U8  u8mfThdSelUDWdr[ISP_AUTO_ISO_STRENGTH_NUM] = {2   ,    2   ,    2   ,     2   ,    2    ,    1    ,    1    ,    1     ,    1    ,    1    ,    1    ,    1    ,    1    ,    1    ,    1    ,    1};
    const HI_U8  u8mfThdSftUDWdr[ISP_AUTO_ISO_STRENGTH_NUM] = {0   ,    0    ,   0   ,     0   ,    0    ,    2    ,    2    ,    2     ,    2    ,    2    ,    2    ,    2    ,    2    ,    2    ,    2    ,    2};
    //const HI_U8  u8dirDiffSftWdr[ISP_AUTO_ISO_STRENGTH_NUM]={10  ,    10   ,   10   ,    12  ,    15   ,    20   ,    20   ,    20    ,    20   ,    20   ,   20    ,   20    ,   20    ,   20    ,   20    ,   20};
    //const HI_U16 u16oMaxGainWdr[ISP_AUTO_ISO_STRENGTH_NUM]={900,       900,      900,      900,      900,       900,       900,        900,      900,       900,       900,        900,       900,      900,       900,       900};
    //const HI_U16 u16uMaxGainWdr[ISP_AUTO_ISO_STRENGTH_NUM]={900,       900,      900,      900,      900,       900,       900,        900,      900,       900,       900,        900,       900,      900,       900,       900};
    const HI_U8  u8oshtVarWgt0Wdr[ISP_AUTO_ISO_STRENGTH_NUM] = {60  ,   60   ,   60   ,    60   ,   60    ,   50    ,   20    ,   20     ,   20    ,   20    ,   20    ,   20    ,   20    ,   20    ,   20    ,   20};
    const HI_U8  u8ushtVarWgt0Wdr[ISP_AUTO_ISO_STRENGTH_NUM] = {70  ,   70   ,   70   ,    70   ,   70    ,   60    ,   30    ,   30     ,   30    ,   30    ,   30    ,   30    ,   30    ,   30    ,   30    ,   30};
    const HI_U8  u8oshtVarDiffThd0Wdr[ISP_AUTO_ISO_STRENGTH_NUM] = {20,      22,      23,      25,      28,       30,       32,        36,       37,        38,      39,        40,       40,      40,        40,      40};
    const HI_U8  u8oshtVarDiffThd1Wdr[ISP_AUTO_ISO_STRENGTH_NUM] = {35,       37,      38,      40,      40,       43,       43,        46,       47,        48,      49,        50,       50,      50,        50,      50};
    const HI_U8  u8oshtVarDiffWgt1Wdr[ISP_AUTO_ISO_STRENGTH_NUM] = {5   ,    5   ,    5   ,     5   ,    5    ,    5    ,    5    ,    5     ,    5    ,    5    ,    5    ,    5    ,    5    ,    5    ,    5    ,    5 };
    const HI_U8  u8ushtVarDiffWgt1Wdr[ISP_AUTO_ISO_STRENGTH_NUM] = {10  ,    10  ,    10  ,     10  ,    10   ,    10   ,    10   ,    10    ,    10   ,    10   ,    10   ,    10   ,    10   ,    10   ,    10   ,    10};


    pstSharpenPara->u8mfThdSelUD   =  u8mfThdSelUDWdr[idxCur];
    pstSharpenPara->u8mfThdSftUD    =  LinearInter(u32ISO, isoLvlPre, u8mfThdSftUDWdr[idxPre],
                                       isoLvlCur, u8mfThdSftUDWdr[idxCur]);
    //pstSharpenPara->u8dirDiffSft  =  LinearInter(u32ISO, isoLvlPre, u8dirDiffSftWdr[idxPre],
    //                                                          isoLvlCur, u8dirDiffSftWdr[idxCur]);
    //pstSharpenPara->u16oMaxGain   =  LinearInter(u32ISO, isoLvlPre, u16oMaxGainWdr[idxPre],
    //                                                          isoLvlCur, u16oMaxGainWdr[idxCur]);
    //pstSharpenPara->u16uMaxGain   =  LinearInter(u32ISO, isoLvlPre, u16uMaxGainWdr[idxPre],
    //                                                          isoLvlCur, u16uMaxGainWdr[idxCur]);
    pstSharpenPara->u8oshtVarWgt0 =  LinearInter(u32ISO, isoLvlPre, u8oshtVarWgt0Wdr[idxPre],
                                     isoLvlCur, u8oshtVarWgt0Wdr[idxCur]);
    pstSharpenPara->u8ushtVarWgt0 =  LinearInter(u32ISO, isoLvlPre, u8ushtVarWgt0Wdr[idxPre],
                                     isoLvlCur, u8ushtVarWgt0Wdr[idxCur]);
    pstSharpenPara->u8oshtVarDiffThd0 =  LinearInter(u32ISO, isoLvlPre, u8oshtVarDiffThd0Wdr[idxPre],
                                         isoLvlCur, u8oshtVarDiffThd0Wdr[idxCur]);
    pstSharpenPara->u8oshtVarDiffThd1 =  LinearInter(u32ISO, isoLvlPre, u8oshtVarDiffThd1Wdr[idxPre],
                                         isoLvlCur, u8oshtVarDiffThd1Wdr[idxCur]);
    pstSharpenPara->u8oshtVarDiffWgt1 =  LinearInter(u32ISO, isoLvlPre, u8oshtVarDiffWgt1Wdr[idxPre],
                                         isoLvlCur, u8oshtVarDiffWgt1Wdr[idxCur]);
    pstSharpenPara->u8ushtVarDiffWgt1 =  LinearInter(u32ISO, isoLvlPre, u8ushtVarDiffWgt1Wdr[idxPre],
                                         isoLvlCur, u8ushtVarDiffWgt1Wdr[idxCur]);
    return HI_SUCCESS;
}

static HI_S32 ISP_Sharpen_GetHdrDefaultRegCfg(ISP_SHARPEN_S *pstSharpenPara, HI_U32 u32ISO, HI_U32  idxCur, HI_U32 idxPre, HI_U32  isoLvlCur, HI_U32 isoLvlPre)
{
    //HDR mode defalt regs
    const HI_U8  u8mfThdSelUDHdr[ISP_AUTO_ISO_STRENGTH_NUM] = {1    ,   1    ,    1    ,    1   ,    1    ,    1    ,    1    ,    1     ,    1    ,    1    ,    1    ,    1    ,    1    ,    1    ,    1    ,    1};
    const HI_U8  u8mfThdSftUDHdr[ISP_AUTO_ISO_STRENGTH_NUM] = {2   ,    2   ,     2   ,     2   ,    2    ,    2    ,    2    ,    2     ,    2    ,    2    ,    2    ,    2    ,    2    ,    2    ,    2    ,    2};
    //const HI_U8  u8dirDiffSftHdr[ISP_AUTO_ISO_STRENGTH_NUM]={5   ,    6   ,    7   ,     8   ,    9    ,    10    ,    11    ,   12     ,   13    ,   14    ,   15    ,   16    ,   17    ,   18    ,   19    ,   20};
    //const HI_U16 u16oMaxGainHdr[ISP_AUTO_ISO_STRENGTH_NUM]={900,       900,      900,      900,      900,       900,       900,        900,      900,       900,       900,        900,       900,      900,       900,       900};
    //const HI_U16 u16uMaxGainHdr[ISP_AUTO_ISO_STRENGTH_NUM]={900,       900,      900,      900,      900,       900,       900,        900,      900,       900,       900,        900,       900,      900,       900,       900};
    const HI_U8  u8oshtVarWgt0Hdr[ISP_AUTO_ISO_STRENGTH_NUM] = {80  ,    70  ,    60  ,     50  ,    40   ,    30   ,    20   ,    10    ,    0    ,    0    ,    0    ,    0    ,    0    ,    0    ,    0    ,    0};
    const HI_U8  u8ushtVarWgt0Hdr[ISP_AUTO_ISO_STRENGTH_NUM] = {80  ,    70  ,    60  ,     50  ,    40   ,    30   ,    20   ,    10    ,    0    ,    0    ,    0    ,    0    ,    0    ,    0    ,    0    ,    0};
    const HI_U8  u8oshtVarDiffThd0Hdr[ISP_AUTO_ISO_STRENGTH_NUM] = {20,      22,      23,      25,      28,       30,       32,        36,       37,        38,      39,        40,       40,      40,        40,      40};
    const HI_U8  u8oshtVarDiffThd1Hdr[ISP_AUTO_ISO_STRENGTH_NUM] = {35,       37,      38,      40,      40,       43,       43,        46,       47,        48,      49,        50,       50,      50,        50,      50};
    const HI_U8  u8oshtVarDiffWgt1Hdr[ISP_AUTO_ISO_STRENGTH_NUM] = {0   ,     0   ,    0   ,     0   ,    0    ,    0    ,    5    ,    5     ,    5    ,    5    ,    5    ,    5    ,    5    ,    5    ,    5    ,    5};
    const HI_U8  u8ushtVarDiffWgt1Hdr[ISP_AUTO_ISO_STRENGTH_NUM] = {0   ,     0   ,    0   ,     0   ,    0    ,    0    ,    5   ,    10    ,    10   ,    10   ,    10   ,    10   ,    10   ,    10   ,    10   ,    10 };


    pstSharpenPara->u8mfThdSelUD   =  u8mfThdSelUDHdr[idxCur];
    pstSharpenPara->u8mfThdSftUD    =  LinearInter(u32ISO, isoLvlPre, u8mfThdSftUDHdr[idxPre],
                                       isoLvlCur, u8mfThdSftUDHdr[idxCur]);
    //pstSharpenPara->u8dirDiffSft  =  LinearInter(u32ISO, isoLvlPre, u8dirDiffSftHdr[idxPre],
    //                                                          isoLvlCur, u8dirDiffSftHdr[idxCur]);
    //pstSharpenPara->u16oMaxGain   =  LinearInter(u32ISO, isoLvlPre, u16oMaxGainHdr[idxPre],
    //                                                          isoLvlCur, u16oMaxGainHdr[idxCur]);
    //pstSharpenPara->u16uMaxGain   =  LinearInter(u32ISO, isoLvlPre, u16uMaxGainHdr[idxPre],
    //                                                          isoLvlCur, u16uMaxGainHdr[idxCur]);
    pstSharpenPara->u8oshtVarWgt0 =  LinearInter(u32ISO, isoLvlPre, u8oshtVarWgt0Hdr[idxPre],
                                     isoLvlCur, u8oshtVarWgt0Hdr[idxCur]);
    pstSharpenPara->u8ushtVarWgt0 =  LinearInter(u32ISO, isoLvlPre, u8ushtVarWgt0Hdr[idxPre],
                                     isoLvlCur, u8ushtVarWgt0Hdr[idxCur]);
    pstSharpenPara->u8oshtVarDiffThd0 =  LinearInter(u32ISO, isoLvlPre, u8oshtVarDiffThd0Hdr[idxPre],
                                         isoLvlCur, u8oshtVarDiffThd0Hdr[idxCur]);
    pstSharpenPara->u8oshtVarDiffThd1 =  LinearInter(u32ISO, isoLvlPre, u8oshtVarDiffThd1Hdr[idxPre],
                                         isoLvlCur, u8oshtVarDiffThd1Hdr[idxCur]);
    pstSharpenPara->u8oshtVarDiffWgt1 =  LinearInter(u32ISO, isoLvlPre, u8oshtVarDiffWgt1Hdr[idxPre],
                                         isoLvlCur, u8oshtVarDiffWgt1Hdr[idxCur]);
    pstSharpenPara->u8ushtVarDiffWgt1 =  LinearInter(u32ISO, isoLvlPre, u8ushtVarDiffWgt1Hdr[idxPre],
                                         isoLvlCur, u8ushtVarDiffWgt1Hdr[idxCur]);
    return HI_SUCCESS;
}


static HI_S32 ISP_Sharpen_GetDefaultRegCfg(VI_PIPE ViPipe, HI_U32 u32ISO)
{
    const HI_U32 SharpenLutIso[ISP_AUTO_ISO_STRENGTH_NUM] = {100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600, 51200, 102400, 204800, 409600, 819200, 1638400, 3276800};
    //Common Regs
    const HI_U8  u8mfThdSftD[ISP_AUTO_ISO_STRENGTH_NUM] = { 2,        2    ,   2   ,     2   ,    2    ,    2    ,    2    ,    2     ,    2    ,    2    ,    2    ,    2    ,    2    ,    2    ,    2    ,    2};
    const HI_U8  u8dirVarSft[ISP_AUTO_ISO_STRENGTH_NUM] = {12  ,     12  ,    12  ,     12  ,    12   ,    12   ,    12   ,    10     ,    9    ,    8   ,    7   ,    6   ,    5   ,    5   ,    5   ,    5};
    //const HI_U8  u8dirRt0[ISP_AUTO_ISO_STRENGTH_NUM]={    8   ,    8   ,    8   ,    8   ,    8    ,    8   ,     12  ,     12   ,     12   ,    12   ,    12   ,    12   ,    12   ,    12   ,    12   ,    12};
    //const HI_U8  u8dirRt1[ISP_AUTO_ISO_STRENGTH_NUM]={   16  ,    16  ,    16  ,     16  ,    16   ,    16   ,    16   ,    16    ,    16   ,    16   ,    16   ,    16   ,    16   ,    16   ,    16   ,    16};
    const HI_U8  u8selPixWgt[ISP_AUTO_ISO_STRENGTH_NUM] = {31  ,    31  ,    31  ,     31  ,    31   ,    31   ,    31   ,    31    ,   31   ,    31   ,    31   ,    31   ,    31   ,    31   ,    31   ,    31};
    //const HI_U8  u8shtNoiseMin[ISP_AUTO_ISO_STRENGTH_NUM]={0   ,    0   ,    0   ,     0   ,    0    ,    0    ,    0    ,    1     ,    1    ,    1    ,    1    ,    1    ,    1    ,    1    ,    1    ,    1};
    const HI_U16 u16chrGGain0[ISP_AUTO_ISO_STRENGTH_NUM] = {32  ,   32  ,    32  ,     32  ,    32   ,    32   ,    32   ,    32    ,    32   ,    32   ,    32   ,    32   ,    32   ,    32   ,    32   ,    32};


    HI_U32 i;
    HI_U32  idxCur, idxPre;
    HI_U32  isoLvlCur, isoLvlPre;
    HI_U8  u8WDRMode;


    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_SHARPEN_S *pstSharpenPara = HI_NULL;
    SHARPEN_GET_CTX(ViPipe, pstSharpenPara);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    u8WDRMode = pstIspCtx->u8SnsWDRMode;



    // Get ISO category index
    // idxCur : current index
    // idxPre : previous level index
    idxCur = ISP_AUTO_ISO_STRENGTH_NUM - 1;

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        if (u32ISO <= SharpenLutIso[i])
        {
            idxCur = i;
            break;
        }
    }

    idxPre = (idxCur == 0) ? 0 : MAX2( idxCur - 1, 0 );

    isoLvlCur   =  SharpenLutIso[idxCur];
    isoLvlPre   =  SharpenLutIso[idxPre];
    /* Common default regs */
    pstSharpenPara->u8mfThdSftD =  LinearInter(u32ISO, isoLvlPre, u8mfThdSftD[idxPre],
                                   isoLvlCur, u8mfThdSftD[idxCur]);
    pstSharpenPara->u8dirVarSft =  LinearInter(u32ISO, isoLvlPre, u8dirVarSft[idxPre],
                                   isoLvlCur, u8dirVarSft[idxCur]);
    //pstSharpenPara->u8dirRt0  =  LinearInter(u32ISO, isoLvlPre, u8dirRt0[idxPre],
    //                                                 isoLvlCur, u8dirRt0[idxCur]);
    //pstSharpenPara->u8dirRt1  =  LinearInter(u32ISO, isoLvlPre, u8dirRt1[idxPre],
    //                                                 isoLvlCur, u8dirRt1[idxCur]);
    pstSharpenPara->u8selPixWgt =  LinearInter(u32ISO, isoLvlPre, u8selPixWgt[idxPre],
                                   isoLvlCur, u8selPixWgt[idxCur]);
    //pstSharpenPara->u8shtNoiseMin =  LinearInter(u32ISO, isoLvlPre, u8shtNoiseMin[idxPre],
    //                                                 isoLvlCur, u8shtNoiseMin[idxCur]);
    pstSharpenPara->u16chrGGain0 =  LinearInter(u32ISO, isoLvlPre, u16chrGGain0[idxPre],
                                    isoLvlCur, u16chrGGain0[idxCur]);

    /* Linear mode default regs */
    if (IS_LINEAR_MODE(u8WDRMode))
    {
        ISP_Sharpen_GetLinearDefaultRegCfg(pstSharpenPara, u32ISO, idxCur, idxPre, isoLvlCur, isoLvlPre);

    }
    else if (IS_2to1_WDR_MODE(u8WDRMode) || IS_3to1_WDR_MODE(u8WDRMode) || IS_4to1_WDR_MODE(u8WDRMode) || IS_BUILT_IN_WDR_MODE(u8WDRMode)) /* WDR mode default regs */
    {
        ISP_Sharpen_GetWdrDefaultRegCfg(pstSharpenPara, u32ISO, idxCur, idxPre, isoLvlCur, isoLvlPre);
    }

    else     //if (DYNAMIC_RANGE_XDR == pstIspCtx->stHdrAttr.enDynamicRange) /* HDR mode default regs */
    {
        ISP_Sharpen_GetHdrDefaultRegCfg(pstSharpenPara, u32ISO, idxCur, idxPre, isoLvlCur, isoLvlPre);
    }

    return HI_SUCCESS;

}

static HI_S32 ISP_Sharpen_GetMpiRegCfg(VI_PIPE ViPipe, HI_U32 u32ISO)
{
    const HI_S32 SharpenLutIso[ISP_AUTO_ISO_STRENGTH_NUM] = {100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600, 51200, 102400, 204800, 409600, 819200, 1638400, 3276800};

    HI_U32 i;
    HI_S32  idxCur, idxPre;
    HI_S32  isoLvlCur, isoLvlPre;

    ISP_SHARPEN_S *pstSharpenPara = HI_NULL;
    SHARPEN_GET_CTX(ViPipe, pstSharpenPara);


    // Get ISO category index
    // idxCur : current index
    // idxPre : previous level index
    idxCur = ISP_AUTO_ISO_STRENGTH_NUM - 1;

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
    {
        if (u32ISO <= SharpenLutIso[i])
        {
            idxCur = i;
            break;
        }
    }

    idxPre = MAX2( idxCur - 1, 0 );
    isoLvlCur   =  SharpenLutIso[idxCur];
    isoLvlPre   =  SharpenLutIso[idxPre];

    for ( i = 0 ; i < ISP_SHARPEN_GAIN_NUM; i++ )
    {
        pstSharpenPara->au16TextureStr[i] =  LinearInter(u32ISO, isoLvlPre, pstSharpenPara->au16AutoTextureStr[i][idxPre],
                                             isoLvlCur, pstSharpenPara->au16AutoTextureStr[i][idxCur]);
        pstSharpenPara->au16EdgeStr[i] =  LinearInter(u32ISO, isoLvlPre, pstSharpenPara->au16AutoEdgeStr[i][idxPre],
                                          isoLvlCur, pstSharpenPara->au16AutoEdgeStr[i][idxCur]);
        pstSharpenPara->au8LumaWgt[i] =  LinearInter(u32ISO, isoLvlPre, pstSharpenPara->au8AutoLumaWgt[i][idxPre],
                                         isoLvlCur, pstSharpenPara->au8AutoLumaWgt[i][idxCur]);
    }

    pstSharpenPara->u16TextureFreq =  LinearInter(u32ISO, isoLvlPre, pstSharpenPara->au16TextureFreq[idxPre],
                                      isoLvlCur, pstSharpenPara->au16TextureFreq[idxCur]);
    pstSharpenPara->u16EdgeFreq =  LinearInter(u32ISO, isoLvlPre, pstSharpenPara->au16EdgeFreq[idxPre],
                                   isoLvlCur, pstSharpenPara->au16EdgeFreq[idxCur]);
    pstSharpenPara->u8OverShoot =  LinearInter(u32ISO, isoLvlPre, pstSharpenPara->au8OverShoot[idxPre],
                                   isoLvlCur, pstSharpenPara->au8OverShoot[idxCur]);
    pstSharpenPara->u8UnderShoot =  LinearInter(u32ISO, isoLvlPre, pstSharpenPara->au8UnderShoot[idxPre],
                                    isoLvlCur, pstSharpenPara->au8UnderShoot[idxCur]);
    pstSharpenPara->u8ShootSupStr =  LinearInter(u32ISO, isoLvlPre, pstSharpenPara->au8ShootSupStr[idxPre],
                                     isoLvlCur, pstSharpenPara->au8ShootSupStr[idxCur]);
    pstSharpenPara->u8DetailCtrl =  LinearInter(u32ISO, isoLvlPre, pstSharpenPara->au8DetailCtrl[idxPre],
                                    isoLvlCur, pstSharpenPara->au8DetailCtrl[idxCur]);
    pstSharpenPara->u8EdgeFiltStr =  LinearInter(u32ISO, isoLvlPre, pstSharpenPara->au8EdgeFiltStr[idxPre],
                                     isoLvlCur, pstSharpenPara->au8EdgeFiltStr[idxCur]);
    pstSharpenPara->u8RGain =  LinearInter(u32ISO, isoLvlPre, pstSharpenPara->au8RGain[idxPre],
                                           isoLvlCur, pstSharpenPara->au8RGain[idxCur]);
    pstSharpenPara->u8BGain =  LinearInter(u32ISO, isoLvlPre, pstSharpenPara->au8BGain[idxPre],
                                           isoLvlCur, pstSharpenPara->au8BGain[idxCur]);
    pstSharpenPara->u8SkinGain =  LinearInter(u32ISO, isoLvlPre, pstSharpenPara->au8SkinGain[idxPre],
                                  isoLvlCur, pstSharpenPara->au8SkinGain[idxCur]);

    {
        hi_ext_system_actual_sharpen_overshootAmt_write(ViPipe, pstSharpenPara->u8OverShoot);
        hi_ext_system_actual_sharpen_undershootAmt_write(ViPipe, pstSharpenPara->u8UnderShoot);
        hi_ext_system_actual_sharpen_shootSupSt_write(ViPipe, pstSharpenPara->u8ShootSupStr);
        hi_ext_system_actual_sharpen_edge_frequence_write(ViPipe, pstSharpenPara->u16EdgeFreq);
        hi_ext_system_actual_sharpen_texture_frequence_write(ViPipe, pstSharpenPara->u16TextureFreq);

        for (i = 0; i < ISP_SHARPEN_GAIN_NUM; i++)
        {
            hi_ext_system_actual_sharpen_edge_str_write(ViPipe, i, pstSharpenPara->au16EdgeStr[i]);
            hi_ext_system_actual_sharpen_texture_str_write(ViPipe, i, pstSharpenPara->au16TextureStr[i]);
        }
    }

    return HI_SUCCESS;

}

static void SharpenShootSupStr2Reg(HI_U8 u8ShootSupStr, HI_U8 *u8bEnShtCtrlByVar,  HI_U8 *u8shtBldRt, HI_U8 *u8oshtVarThd1)
{
    HI_U8   ShtVarMax[25]   = {1, 2, 1, 2, 3, 2, 3, 4, 5, 3, 4, 5, 6, 4, 5, 6, 7, 8, 5, 6, 7, 8, 9, 10, 6};
    HI_U8   ShtSupBldr[25]  = {3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8,  9};
    HI_U16  shootSupStIdx;

    if (u8ShootSupStr == 0)
    {
        *u8bEnShtCtrlByVar     = 0;
        *u8shtBldRt = 0;
        *u8oshtVarThd1 = 0;
    }
    else if (u8ShootSupStr < 26)
    {
        *u8bEnShtCtrlByVar  = 1;
        shootSupStIdx = u8ShootSupStr - 1;
        *u8oshtVarThd1 = ShtVarMax[shootSupStIdx];
        *u8shtBldRt = ShtSupBldr[shootSupStIdx];
    }
    else if (u8ShootSupStr < 73)
    {
        *u8bEnShtCtrlByVar  = 1;
        *u8shtBldRt = 9;
        *u8oshtVarThd1 = u8ShootSupStr - 19;        //7 + u16shootSupStr -26    //7--53

    }
    else if (u8ShootSupStr < 123)
    {
        *u8bEnShtCtrlByVar  = 1;
        *u8shtBldRt = 10;
        *u8oshtVarThd1 = u8ShootSupStr - 26;        //47--96

    }
    else if (u8ShootSupStr < 173)
    {
        *u8bEnShtCtrlByVar  = 1;
        *u8shtBldRt = 11;
        *u8oshtVarThd1 = u8ShootSupStr - 33;        //90--139

    }
    else if (u8ShootSupStr < 223)
    {
        *u8bEnShtCtrlByVar  = 1;
        *u8shtBldRt = 12;
        *u8oshtVarThd1 = u8ShootSupStr - 46;        //127--176

    }
    else
    {
        *u8bEnShtCtrlByVar  = 1;
        *u8shtBldRt = 13;
        *u8oshtVarThd1 = u8ShootSupStr - 53;        //170--202

    }

}

static void SharpenMPI2Reg(ISP_SHARPEN_REG_CFG_S *pstSharpenRegCfg, ISP_SHARPEN_S *pstSharpen)
{
    HI_U8 i, j;

    ISP_SHARPEN_DEFAULT_DYNA_REG_CFG_S *pstSharpenDefaultDynaRegCfg;
    ISP_SHARPEN_MPI_DYNA_REG_CFG_S *pstSharpenMpiDynaRegCfg;
    ISP_SHARPEN_STATIC_REG_CFG_S *pstSharpenStaticRegCfg;
    ISP_SHARPEN_DYNA_REG_CFG_S *pstSharpenDynaRegCfg;

    pstSharpenDynaRegCfg = &(pstSharpenRegCfg->stDynaRegCfg);
    pstSharpenDefaultDynaRegCfg = &(pstSharpenDynaRegCfg->stDefaultDynaRegCfg);
    pstSharpenMpiDynaRegCfg = &(pstSharpenDynaRegCfg->stMpiDynaRegCfg);
    pstSharpenStaticRegCfg = &(pstSharpenRegCfg->stStaticRegCfg);

    if (pstSharpenDefaultDynaRegCfg->bResh)
    {
        pstSharpenDefaultDynaRegCfg->u8mfThdSftD            =   pstSharpen->u8mfThdSftD         ;
        pstSharpenDefaultDynaRegCfg->u8mfThdSelUD           =   pstSharpen->u8mfThdSelUD        ;
        pstSharpenDefaultDynaRegCfg->u8mfThdSftUD           =   pstSharpen->u8mfThdSftUD        ;
        pstSharpenDefaultDynaRegCfg->u8hfThdSftD            =   pstSharpenDefaultDynaRegCfg->u8mfThdSftD        ;
        pstSharpenDefaultDynaRegCfg->u8hfThdSelUD           =   pstSharpenDefaultDynaRegCfg->u8mfThdSelUD       ;
        pstSharpenDefaultDynaRegCfg->u8hfThdSftUD           =   pstSharpenDefaultDynaRegCfg->u8mfThdSftUD       ;
        pstSharpenDefaultDynaRegCfg->u8dirVarSft            =   pstSharpen->u8dirVarSft         ;
        pstSharpenDefaultDynaRegCfg->u8selPixWgt            =   pstSharpen->u8selPixWgt         ;
        pstSharpenDefaultDynaRegCfg->u8oshtVarDiffThd[0]        =   pstSharpen->u8oshtVarDiffThd0;
        pstSharpenDefaultDynaRegCfg->u8ushtVarDiffThd[0]        =   pstSharpenDefaultDynaRegCfg->u8oshtVarDiffThd[0];
        pstSharpenDefaultDynaRegCfg->u8oshtVarWgt0          =   pstSharpen->u8oshtVarWgt0       ;
        pstSharpenDefaultDynaRegCfg->u8ushtVarWgt0          =   pstSharpen->u8ushtVarWgt0       ;
        pstSharpenDefaultDynaRegCfg->u8oshtVarDiffThd[1]        =   pstSharpen->u8oshtVarDiffThd1;
        pstSharpenDefaultDynaRegCfg->u8oshtVarDiffWgt1      =   pstSharpen->u8oshtVarDiffWgt1   ;
        pstSharpenDefaultDynaRegCfg->u8ushtVarDiffThd[1]        =   pstSharpenDefaultDynaRegCfg->u8oshtVarDiffThd[1];
        pstSharpenDefaultDynaRegCfg->u8ushtVarDiffWgt1      =   pstSharpen->u8ushtVarDiffWgt1   ;
        pstSharpenDefaultDynaRegCfg->u16chrGGain0           =   pstSharpen->u16chrGGain0        ;
        //SharpenCheckDefDynaReg(pstSharpenDynaRegCfg);

    }

    if (pstSharpenMpiDynaRegCfg->bResh)
    {
        for (i = 0; i < ISP_SHARPEN_GAIN_NUM; i++)
        {
            j = i << 1;
            if (i < ISP_SHARPEN_GAIN_NUM - 1)
            {
                pstSharpenMpiDynaRegCfg->u16mfGainD[j]  = SHARPEN_CLIP3(0, 0xFFF, (32 + pstSharpen->au16EdgeStr[i]));
                pstSharpenMpiDynaRegCfg->u16mfGainUD[j] = SHARPEN_CLIP3(0, 0xFFF, (32 + pstSharpen->au16TextureStr[i]));
                pstSharpenMpiDynaRegCfg->u16mfGainD[j + 1]  = SHARPEN_CLIP3(0, 0xFFF, (32 + ((pstSharpen->au16EdgeStr[i] + pstSharpen->au16EdgeStr[i + 1]) >> 1)));
                pstSharpenMpiDynaRegCfg->u16mfGainUD[j + 1] = SHARPEN_CLIP3(0, 0xFFF, (32 + ((pstSharpen->au16TextureStr[i] + pstSharpen->au16TextureStr[i + 1]) >> 1)));
            }
            else//31
            {
                pstSharpenMpiDynaRegCfg->u16mfGainD[j]  = SHARPEN_CLIP3(0, 0xFFF, (32 + pstSharpen->au16EdgeStr[i]));
                pstSharpenMpiDynaRegCfg->u16mfGainUD[j] = SHARPEN_CLIP3(0, 0xFFF, (32 + pstSharpen->au16TextureStr[i]));
                pstSharpenMpiDynaRegCfg->u16mfGainD[j + 1]  = SHARPEN_CLIP3(0, 0xFFF, (32 + pstSharpen->au16EdgeStr[i]));
                pstSharpenMpiDynaRegCfg->u16mfGainUD[j + 1] = SHARPEN_CLIP3(0, 0xFFF, (32 + pstSharpen->au16TextureStr[i]));
            }
            pstSharpenMpiDynaRegCfg->u16hfGainD[j]  = (HI_U16)SHARPEN_CLIP3(0, 0xFFF, ((((HI_U32)(pstSharpenMpiDynaRegCfg->u16mfGainD[j])) * (pstSharpen->u16EdgeFreq)) >> 6));
            pstSharpenMpiDynaRegCfg->u16hfGainUD[j] = (HI_U16)SHARPEN_CLIP3(0, 0xFFF, ((((HI_U32)(pstSharpenMpiDynaRegCfg->u16mfGainUD[j])) * (pstSharpen->u16TextureFreq)) >> 6));
            pstSharpenMpiDynaRegCfg->u16hfGainD[j + 1]  = (HI_U16)SHARPEN_CLIP3(0, 0xFFF, ((((HI_U32)(pstSharpenMpiDynaRegCfg->u16mfGainD[j + 1])) * (pstSharpen->u16EdgeFreq)) >> 6));
            pstSharpenMpiDynaRegCfg->u16hfGainUD[j + 1] = (HI_U16)SHARPEN_CLIP3(0, 0xFFF, ((((HI_U32)(pstSharpenMpiDynaRegCfg->u16mfGainUD[j + 1])) * (pstSharpen->u16TextureFreq)) >> 6));
        }
        pstSharpenMpiDynaRegCfg->u8oshtAmt              = pstSharpen->u8OverShoot;
        pstSharpenMpiDynaRegCfg->u8ushtAmt              = pstSharpen->u8UnderShoot;
        // skin Ctrl
        if (31 == pstSharpen->u8SkinGain)
        {
            pstSharpenMpiDynaRegCfg->u8bEnSkinCtrl          =    0;
        }
        else
        {
            pstSharpenMpiDynaRegCfg->u8bEnSkinCtrl          =    1;
            pstSharpenMpiDynaRegCfg->u8skinEdgeWgt[1]       = SHARPEN_CLIP3(0, 0x1F, (31 - pstSharpen->u8SkinGain));
            pstSharpenMpiDynaRegCfg->u8skinEdgeWgt[0]       = SHARPEN_CLIP3(0, 0x1F, (pstSharpenMpiDynaRegCfg->u8skinEdgeWgt[1] << 1));

        }

        // Chr Ctrl
        if ((31 == pstSharpenDefaultDynaRegCfg->u16chrGGain0) & (31 == pstSharpenMpiDynaRegCfg->u8chrRGain0) & (31 == pstSharpenMpiDynaRegCfg->u8chrBGain0))
        {
            pstSharpenMpiDynaRegCfg->u8bEnChrCtrl       =       0;
        }
        else
        {
            pstSharpenMpiDynaRegCfg->u8bEnChrCtrl       =       1;
            pstSharpenMpiDynaRegCfg->u8chrRGain0            = pstSharpen->u8RGain;
            pstSharpenMpiDynaRegCfg->u8chrBGain0            = pstSharpen->u8BGain;
        }

        if ( 128 == pstSharpen->u8DetailCtrl )
        {
            pstSharpenMpiDynaRegCfg->bEnDetailCtrl = 0;
        }
        else
        {
            pstSharpenMpiDynaRegCfg->bEnDetailCtrl = 1;
        }
        pstSharpenMpiDynaRegCfg->u8detailOshtAmt = SHARPEN_CLIP3(0, 127, (pstSharpenMpiDynaRegCfg->u8oshtAmt) + (pstSharpen->u8DetailCtrl) - 128);
        pstSharpenMpiDynaRegCfg->u8detailUshtAmt = SHARPEN_CLIP3(0, 127, (pstSharpenMpiDynaRegCfg->u8ushtAmt) + (pstSharpen->u8DetailCtrl) - 128);

        pstSharpenMpiDynaRegCfg->u8dirDiffSft = 63 - pstSharpen->u8EdgeFiltStr;

        SharpenShootSupStr2Reg(pstSharpen->u8ShootSupStr,  &(pstSharpenMpiDynaRegCfg->u8bEnShtCtrlByVar), &(pstSharpenMpiDynaRegCfg->u8shtBldRt), &(pstSharpenMpiDynaRegCfg->u8oshtVarThd1));
        pstSharpenMpiDynaRegCfg->u8ushtVarThd1 = pstSharpenMpiDynaRegCfg->u8oshtVarThd1;

        pstSharpenMpiDynaRegCfg->u8bEnLumaCtrl  =       0;
        for ( i = 0; i < ISP_SHARPEN_LUMA_NUM; i++ )
        {
            pstSharpenMpiDynaRegCfg->au8LumaWgt[i] = pstSharpen->au8LumaWgt[i];
            if (pstSharpenMpiDynaRegCfg->au8LumaWgt[i] < 127)
            {
                pstSharpenMpiDynaRegCfg->u8bEnLumaCtrl = 1;
            }
        }

        //SharpenCheckMpiDynaReg(pstSharpenDynaRegCfg);
    }

    /* Calc all MulCoef */
    // Control shoot based on variance
    pstSharpenDefaultDynaRegCfg->s16oshtVarDiffMul = CalcMulCoef( pstSharpenDefaultDynaRegCfg->u8oshtVarDiffThd[0], pstSharpenStaticRegCfg->u8oshtVarDiffWgt0,
            pstSharpenDefaultDynaRegCfg->u8oshtVarDiffThd[1], pstSharpenDefaultDynaRegCfg->u8oshtVarDiffWgt1,
            SHRP_SHT_VAR_MUL_PRECS );

    pstSharpenDefaultDynaRegCfg->s16ushtVarDiffMul = CalcMulCoef( pstSharpenDefaultDynaRegCfg->u8ushtVarDiffThd[0], pstSharpenStaticRegCfg->u8ushtVarDiffWgt0,
            pstSharpenDefaultDynaRegCfg->u8ushtVarDiffThd[1], pstSharpenDefaultDynaRegCfg->u8ushtVarDiffWgt1,
            SHRP_SHT_VAR_MUL_PRECS );
    pstSharpenDefaultDynaRegCfg->s16chrGMul = CalcMulCoef( pstSharpenStaticRegCfg->u8chrGThd[0], pstSharpenDefaultDynaRegCfg->u16chrGGain0,
            pstSharpenStaticRegCfg->u8chrGThd[1], pstSharpenStaticRegCfg->u16chrGGain1,
            SHRP_CHR_MUL_SFT );
    pstSharpenMpiDynaRegCfg->u16oshtVarMul = CalcMulCoef( pstSharpenStaticRegCfg->u8oshtVarThd0, pstSharpenDefaultDynaRegCfg->u8oshtVarWgt0,
            pstSharpenMpiDynaRegCfg->u8oshtVarThd1, pstSharpenStaticRegCfg->u8oshtVarWgt1,
            SHRP_SHT_VAR_MUL_PRECS );

    pstSharpenMpiDynaRegCfg->u16ushtVarMul = CalcMulCoef( pstSharpenStaticRegCfg->u8ushtVarThd0, pstSharpenDefaultDynaRegCfg->u8ushtVarWgt0,
            pstSharpenMpiDynaRegCfg->u8ushtVarThd1, pstSharpenStaticRegCfg->u8ushtVarWgt1,
            SHRP_SHT_VAR_MUL_PRECS );
    pstSharpenMpiDynaRegCfg->s16detailOshtMul = CalcMulCoef( pstSharpenStaticRegCfg->u8detailOshtThr[0], pstSharpenMpiDynaRegCfg->u8detailOshtAmt,
            pstSharpenStaticRegCfg->u8detailOshtThr[1], pstSharpenMpiDynaRegCfg->u8oshtAmt,
            SHRP_DETAIL_SHT_MUL_PRECS );

    pstSharpenMpiDynaRegCfg->s16detailUshtMul = CalcMulCoef( pstSharpenStaticRegCfg->u8detailUshtThr[0], pstSharpenMpiDynaRegCfg->u8detailUshtAmt,
            pstSharpenStaticRegCfg->u8detailUshtThr[1], pstSharpenMpiDynaRegCfg->u8ushtAmt,
            SHRP_DETAIL_SHT_MUL_PRECS );

    pstSharpenMpiDynaRegCfg->s16chrRMul = CalcMulCoef( pstSharpenStaticRegCfg->u8chrRThd[0], pstSharpenMpiDynaRegCfg->u8chrRGain0,
                                          pstSharpenStaticRegCfg->u8chrRThd[1], pstSharpenStaticRegCfg->u8chrRGain1,
                                          SHRP_CHR_MUL_SFT );
    pstSharpenMpiDynaRegCfg->s16chrBMul = CalcMulCoef( pstSharpenStaticRegCfg->u8chrBThd[0], pstSharpenMpiDynaRegCfg->u8chrBGain0,
                                          pstSharpenStaticRegCfg->u8chrBThd[1], pstSharpenStaticRegCfg->u8chrBGain1,
                                          SHRP_CHR_MUL_SFT );

    pstSharpenMpiDynaRegCfg->s16skinEdgeMul  =  CalcMulCoef( pstSharpenStaticRegCfg->u8skinEdgeThd[0], pstSharpenMpiDynaRegCfg->u8skinEdgeWgt[0],
            pstSharpenStaticRegCfg->u8skinEdgeThd[1], pstSharpenMpiDynaRegCfg->u8skinEdgeWgt[1],
            SHRP_SKIN_EDGE_MUL_PRECS );

}

static HI_BOOL __inline CheckSharpenOpen(ISP_SHARPEN_S *pstSharpen)
{
    return (HI_TRUE == pstSharpen->bSharpenEn);
}

HI_S32 SharpenProcWrite(VI_PIPE ViPipe, ISP_CTRL_PROC_WRITE_S *pstProc)
{
    ISP_CTRL_PROC_WRITE_S stProcTmp;

    ISP_SHARPEN_S *pstSharpen = HI_NULL;

    SHARPEN_GET_CTX(ViPipe, pstSharpen);

    if ((HI_NULL == pstProc->pcProcBuff) || (0 == pstProc->u32BuffLen))
    {
        return HI_FAILURE;
    }

    stProcTmp.pcProcBuff = pstProc->pcProcBuff;
    stProcTmp.u32BuffLen = pstProc->u32BuffLen;

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "-----SHARPEN INFO--------------------------------------------------------------\n");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s\n",
                    "LumaWgt 0--7:");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u\n\n",
                    (HI_U16)pstSharpen->au8LumaWgt[0],
                    (HI_U16)pstSharpen->au8LumaWgt[1],
                    (HI_U16)pstSharpen->au8LumaWgt[2],
                    (HI_U16)pstSharpen->au8LumaWgt[3],
                    (HI_U16)pstSharpen->au8LumaWgt[4],
                    (HI_U16)pstSharpen->au8LumaWgt[5],
                    (HI_U16)pstSharpen->au8LumaWgt[6],
                    (HI_U16)pstSharpen->au8LumaWgt[7]
                   );

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s\n",
                    "LumaWgt 8--15:");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u\n\n",
                    (HI_U16)pstSharpen->au8LumaWgt[8],
                    (HI_U16)pstSharpen->au8LumaWgt[9],
                    (HI_U16)pstSharpen->au8LumaWgt[10],
                    (HI_U16)pstSharpen->au8LumaWgt[11],
                    (HI_U16)pstSharpen->au8LumaWgt[12],
                    (HI_U16)pstSharpen->au8LumaWgt[13],
                    (HI_U16)pstSharpen->au8LumaWgt[14],
                    (HI_U16)pstSharpen->au8LumaWgt[15]
                   );

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s\n",
                    "LumaWgt 16--23:");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u\n\n",
                    (HI_U16)pstSharpen->au8LumaWgt[16],
                    (HI_U16)pstSharpen->au8LumaWgt[17],
                    (HI_U16)pstSharpen->au8LumaWgt[18],
                    (HI_U16)pstSharpen->au8LumaWgt[19],
                    (HI_U16)pstSharpen->au8LumaWgt[20],
                    (HI_U16)pstSharpen->au8LumaWgt[21],
                    (HI_U16)pstSharpen->au8LumaWgt[22],
                    (HI_U16)pstSharpen->au8LumaWgt[23]
                   );

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s\n",
                    "LumaWgt 24--31:");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u\n\n",
                    (HI_U16)pstSharpen->au8LumaWgt[24],
                    (HI_U16)pstSharpen->au8LumaWgt[25],
                    (HI_U16)pstSharpen->au8LumaWgt[26],
                    (HI_U16)pstSharpen->au8LumaWgt[27],
                    (HI_U16)pstSharpen->au8LumaWgt[28],
                    (HI_U16)pstSharpen->au8LumaWgt[29],
                    (HI_U16)pstSharpen->au8LumaWgt[30],
                    (HI_U16)pstSharpen->au8LumaWgt[31]
                   );

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s\n",
                    "TextureStr 0--7:");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u\n\n",
                    (HI_U16)pstSharpen->au16TextureStr[0],
                    (HI_U16)pstSharpen->au16TextureStr[1],
                    (HI_U16)pstSharpen->au16TextureStr[2],
                    (HI_U16)pstSharpen->au16TextureStr[3],
                    (HI_U16)pstSharpen->au16TextureStr[4],
                    (HI_U16)pstSharpen->au16TextureStr[5],
                    (HI_U16)pstSharpen->au16TextureStr[6],
                    (HI_U16)pstSharpen->au16TextureStr[7]
                   );

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s\n",
                    "TextureStr 8--15:");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u\n\n",
                    (HI_U16)pstSharpen->au16TextureStr[8],
                    (HI_U16)pstSharpen->au16TextureStr[9],
                    (HI_U16)pstSharpen->au16TextureStr[10],
                    (HI_U16)pstSharpen->au16TextureStr[11],
                    (HI_U16)pstSharpen->au16TextureStr[12],
                    (HI_U16)pstSharpen->au16TextureStr[13],
                    (HI_U16)pstSharpen->au16TextureStr[14],
                    (HI_U16)pstSharpen->au16TextureStr[15]
                   );

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s\n",
                    "TextureStr 16--23:");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u\n\n",
                    (HI_U16)pstSharpen->au16TextureStr[16],
                    (HI_U16)pstSharpen->au16TextureStr[17],
                    (HI_U16)pstSharpen->au16TextureStr[18],
                    (HI_U16)pstSharpen->au16TextureStr[19],
                    (HI_U16)pstSharpen->au16TextureStr[20],
                    (HI_U16)pstSharpen->au16TextureStr[21],
                    (HI_U16)pstSharpen->au16TextureStr[22],
                    (HI_U16)pstSharpen->au16TextureStr[23]
                   );

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s\n",
                    "TextureStr 24--31:");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u\n\n",
                    (HI_U16)pstSharpen->au16TextureStr[24],
                    (HI_U16)pstSharpen->au16TextureStr[25],
                    (HI_U16)pstSharpen->au16TextureStr[26],
                    (HI_U16)pstSharpen->au16TextureStr[27],
                    (HI_U16)pstSharpen->au16TextureStr[28],
                    (HI_U16)pstSharpen->au16TextureStr[29],
                    (HI_U16)pstSharpen->au16TextureStr[30],
                    (HI_U16)pstSharpen->au16TextureStr[31]
                   );

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s\n",
                    "EdgeStr 0--7:");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u\n\n",
                    (HI_U16)pstSharpen->au16EdgeStr[0],
                    (HI_U16)pstSharpen->au16EdgeStr[1],
                    (HI_U16)pstSharpen->au16EdgeStr[2],
                    (HI_U16)pstSharpen->au16EdgeStr[3],
                    (HI_U16)pstSharpen->au16EdgeStr[4],
                    (HI_U16)pstSharpen->au16EdgeStr[5],
                    (HI_U16)pstSharpen->au16EdgeStr[6],
                    (HI_U16)pstSharpen->au16EdgeStr[7]
                   );

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s\n",
                    "EdgeStr 8--15:");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u\n\n",
                    (HI_U16)pstSharpen->au16EdgeStr[8],
                    (HI_U16)pstSharpen->au16EdgeStr[9],
                    (HI_U16)pstSharpen->au16EdgeStr[10],
                    (HI_U16)pstSharpen->au16EdgeStr[11],
                    (HI_U16)pstSharpen->au16EdgeStr[12],
                    (HI_U16)pstSharpen->au16EdgeStr[13],
                    (HI_U16)pstSharpen->au16EdgeStr[14],
                    (HI_U16)pstSharpen->au16EdgeStr[15]
                   );

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s\n",
                    "EdgeStr 16--23:");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u\n\n",
                    (HI_U16)pstSharpen->au16EdgeStr[16],
                    (HI_U16)pstSharpen->au16EdgeStr[17],
                    (HI_U16)pstSharpen->au16EdgeStr[18],
                    (HI_U16)pstSharpen->au16EdgeStr[19],
                    (HI_U16)pstSharpen->au16EdgeStr[20],
                    (HI_U16)pstSharpen->au16EdgeStr[21],
                    (HI_U16)pstSharpen->au16EdgeStr[22],
                    (HI_U16)pstSharpen->au16EdgeStr[23]
                   );

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s\n",
                    "EdgeStr 24--31:");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u"  "%8u\n\n",
                    (HI_U16)pstSharpen->au16EdgeStr[24],
                    (HI_U16)pstSharpen->au16EdgeStr[25],
                    (HI_U16)pstSharpen->au16EdgeStr[26],
                    (HI_U16)pstSharpen->au16EdgeStr[27],
                    (HI_U16)pstSharpen->au16EdgeStr[28],
                    (HI_U16)pstSharpen->au16EdgeStr[29],
                    (HI_U16)pstSharpen->au16EdgeStr[30],
                    (HI_U16)pstSharpen->au16EdgeStr[31]
                   );

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%12s" "%12s" "%12s" "%12s" "%12s" "%12s" "%12s" "%12s" "%12s" "%12s\n",
                    "TextureFreq", "EdgeFreq", "OverShoot", "UnderShoot", "ShootSupStr",
                    "DetailCtrl", "EdgeFiltStr", "RGain", "BGain", "SkinGain");


    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%12u"  "%12u"  "%12u"  "%12u"  "%12u"  "%12u"  "%12u"  "%12u"  "%12u"  "%12u\n",
                    (HI_U16)pstSharpen->u16TextureFreq,
                    (HI_U16)pstSharpen->u16EdgeFreq,
                    (HI_U16)pstSharpen->u8OverShoot,
                    (HI_U16)pstSharpen->u8UnderShoot,
                    (HI_U16)pstSharpen->u8ShootSupStr,
                    (HI_U16)pstSharpen->u8DetailCtrl,
                    (HI_U16)pstSharpen->u8EdgeFiltStr,
                    (HI_U16)pstSharpen->u8RGain,
                    (HI_U16)pstSharpen->u8BGain,
                    (HI_U16)pstSharpen->u8SkinGain
                   );

    pstProc->u32WriteLen += 1;

    return HI_SUCCESS;
}

HI_S32 ISP_SharpenRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo, HI_VOID *pRegCfg, HI_S32 s32Rsv)
{

    HI_U8  i;
    HI_U32 u32Iso = 0;

    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_SHARPEN_S *pstSharpen = HI_NULL;
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    SHARPEN_GET_CTX(ViPipe, pstSharpen);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (pstIspCtx->stLinkage.bDefectPixel)
    {
        return HI_SUCCESS;
    }

    /* calculate every two interrupts */
    if ((0 != pstIspCtx->u32FrameCnt % 10) && (HI_TRUE != pstIspCtx->stLinkage.bSnapState) && (pstIspCtx->stLinkage.u8ProIndex < 1))
    {
        return HI_SUCCESS;
    }

    pstSharpen->bSharpenEn = hi_ext_system_manual_isp_sharpen_en_read(ViPipe);

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        pstRegCfg->stAlgRegCfg[i].stSharpenRegCfg.bEnable = pstSharpen->bSharpenEn;
    }

    pstRegCfg->unKey.bit1SharpenCfg = 1;

    /*check hardware setting*/
    if (!CheckSharpenOpen(pstSharpen))
    {
        return HI_SUCCESS;
    }

    /* sharpen strength linkage with the u32ISO calculated by ae */
    u32Iso = pstIspCtx->stLinkage.u32Iso;

    SharpenReadExtregs(ViPipe);
    SharpenReadProMode(ViPipe);
    if (u32Iso != pstSharpen->u32IsoLast)       //will not work if ISO is the same
    {
        ISP_Sharpen_GetDefaultRegCfg(ViPipe, u32Iso);
        for (i = 0; i < pstRegCfg->u8CfgNum; i++)
        {
            pstRegCfg->stAlgRegCfg[i].stSharpenRegCfg.stDynaRegCfg.stDefaultDynaRegCfg.bResh = HI_TRUE;
        }
    }

    if (pstSharpen->bSharpenMpiUpdateEn)
    {
        if (pstSharpen->u8ManualSharpenYuvEnabled == OP_TYPE_AUTO)  //auto mode
        {
            ISP_Sharpen_GetMpiRegCfg(ViPipe, u32Iso);
        }
        for (i = 0; i < pstRegCfg->u8CfgNum; i++)
        {
            pstRegCfg->stAlgRegCfg[i].stSharpenRegCfg.stDynaRegCfg.stMpiDynaRegCfg.bResh = HI_TRUE;
            pstRegCfg->stAlgRegCfg[i].stSharpenRegCfg.stDynaRegCfg.stMpiDynaRegCfg.u32UpdateIndex      += 1;
            SharpenMPI2Reg(&(pstRegCfg->stAlgRegCfg[i].stSharpenRegCfg), pstSharpen);
        }
    }
    else
    {
        if (u32Iso != pstSharpen->u32IsoLast)
        {
            if (pstSharpen->u8ManualSharpenYuvEnabled == OP_TYPE_AUTO)  //auto mode
            {
                ISP_Sharpen_GetMpiRegCfg(ViPipe, u32Iso);
                for (i = 0; i < pstRegCfg->u8CfgNum; i++)
                {
                    pstRegCfg->stAlgRegCfg[i].stSharpenRegCfg.stDynaRegCfg.stMpiDynaRegCfg.bResh = HI_TRUE;
                    pstRegCfg->stAlgRegCfg[i].stSharpenRegCfg.stDynaRegCfg.stMpiDynaRegCfg.u32UpdateIndex      += 1;
                    SharpenMPI2Reg(&(pstRegCfg->stAlgRegCfg[i].stSharpenRegCfg), pstSharpen);
                }
            }
            else
            {
                for (i = 0; i < pstRegCfg->u8CfgNum; i++)
                {
                    pstRegCfg->stAlgRegCfg[i].stSharpenRegCfg.stDynaRegCfg.stMpiDynaRegCfg.bResh = HI_FALSE;
                    SharpenMPI2Reg(&(pstRegCfg->stAlgRegCfg[i].stSharpenRegCfg), pstSharpen);
                }
            }
        }
        else
        {
            //pstRegCfg->unKey.bit1SharpenCfg = 0;
        }
    }

    pstSharpen->u32IsoLast = u32Iso;    //will not work if ISO is the same

    return HI_SUCCESS;
}

HI_S32 ISP_SharpenCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);
    switch (u32Cmd)
    {
        case ISP_WDR_MODE_SET :
            ISP_SharpenWdrModeSet(ViPipe, (HI_VOID *)&pRegCfg->stRegCfg);
            break;
        case ISP_PROC_WRITE:
            SharpenProcWrite(ViPipe, (ISP_CTRL_PROC_WRITE_S *)pValue);
            break;
        default :
            break;
    }

    return HI_SUCCESS;
}

HI_S32 ISP_SharpenExit(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    for (i = 0; i < pstIspCtx->stBlockAttr.u8BlockNum; i++)
    {
        isp_sharpen_en_write(ViPipe, i, HI_FALSE);
    }

    hi_ext_system_isp_sharpen_manu_mode_write(ViPipe, HI_FALSE);


    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterSharpen(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_SHARPEN;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_SharpenInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_SharpenRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_SharpenCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_SharpenExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


