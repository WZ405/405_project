#ifndef __SAMPLE_DSP_MAIN_H__
#define __SAMPLE_DSP_MAIN_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */
#include "hi_type.h"
#include "highgui.h"
#include "cv.h"
/*
*Dilate sample
*/
HI_VOID SVP_DSP_TVL1(IplImage *src1,IplImage *src2);
/*
*Dilate single handle
*/
HI_VOID SAMPLE_SVP_DSP_TVL1HandleSig(HI_VOID);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __SAMPLE_DSP_MAIN_H__ */
