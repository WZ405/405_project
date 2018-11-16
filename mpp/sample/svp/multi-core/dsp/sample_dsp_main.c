#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "highgui.h"
#include "cv.h"
#include "sample_dsp_main.h"

/******************************************************************************
* function : dsp sample
******************************************************************************/
#ifdef __HuaweiLite__
int app_main(int argc, char *argv[])
{
#else
/******************************************************************************
* function : to process abnormal case
******************************************************************************/
HI_VOID SAMPLE_SVP_DSP_HandleSig(HI_S32 s32Signo)
{
    if (SIGINT == s32Signo || SIGTERM == s32Signo)
    {
        SAMPLE_SVP_DSP_TVL1HandleSig();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}
int main(int argc, char *argv[])
{
    /* for test, the image size is fixed to 352*288 */
    int w = 352;
    int h = 288;

    if(argc !=2)
    {
        printf("please type in the image file!");
    }

    IplImage * src1 = cvLoadImage(argv[1],CV_LOAD_IMAGE_GRAYSCALE);
    IplImage * src2 = cvLoadImage(argv[2],CV_LOAD_IMAGE_GRAYSCALE);
    IplImage * resize_src1 = cvCreateImage(cvSize(w,h),src1->depth,src1->nChannels);
    IplImage * resize_src2 = cvCreateImage(cvSize(w,h),src2->depth,src2->nChannels);
    //resize image
    printf("load image success \n");
    cvResize(src1,resize_src1,CV_INTER_LINEAR);
    cvResize(src2,resize_src2,CV_INTER_LINEAR);

    signal(SIGINT, SAMPLE_SVP_DSP_HandleSig);
    signal(SIGTERM, SAMPLE_SVP_DSP_HandleSig);
#endif
    SVP_DSP_TVL1(resize_src1,resize_src2);

}


