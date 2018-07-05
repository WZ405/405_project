#ifndef __SAMPLE_DSP_ENCA_H__
#define __SAMPLE_DSP_ENCA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hi_dsp.h"

/*****************************************************************************
*    Prototype     : SVP_DSP_TVL1_RUN
*    Description : TVL1 optical flow
*
*    Parameters     :  SVP_DSP_HANDLE      *phHandle           Handle
*                      SVP_SRC_IMAGE_S     *pstSrc             Input image
*                      SVP_DST_IMAGE_S     *pstDst             Output image
*                      SVP_DSP_ID_E         enDspId             DSP Core ID.
*                      SVP_DSP_PRI_E        enPri               Priority
*                      SVP_MEM_INFO_S      *pstAssistBuf       Assist buffer
*
*    Return Value : HI_SUCCESS: Success;Error codes: Failure.
*    Spec         :
*
*    History:
*
*
****************************************************************************/
HI_S32 SVP_DSP_TVL1_RUN(SVP_DSP_HANDLE *phHandle,SVP_DSP_ID_E enDspId,SVP_DSP_PRI_E enPri,
                    SVP_SRC_IMAGE_S *pstSrc1,SVP_SRC_IMAGE_S*pstSrc2,SVP_DST_IMAGE_S *pstDst,SVP_MEM_INFO_S *pstAssistBuf);


#ifdef __cplusplus
}
#endif

#endif
