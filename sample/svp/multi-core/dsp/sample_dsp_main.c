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
        SAMPLE_SVP_DSP_DilateHandleSig();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}
int main(int argc, char *argv[])
{
    
    int w = 352;
    int h = 288;

    if(argc !=2)
    {
        printf("please type in the image file!");
    }

    IplImage * src = cvLoadImage(argv[1],CV_LOAD_IMAGE_GRAYSCALE);
    IplImage *dst = cvCreateImage(cvSize(w,h),src->depth,src->nChannels);

    printf("w %d, h %d, c %d \n",w,h,src->nChannels);
    printf("load image success \n");

    cvResize(src,dst,CV_INTER_LINEAR);
    signal(SIGINT, SAMPLE_SVP_DSP_HandleSig);
    signal(SIGTERM, SAMPLE_SVP_DSP_HandleSig);
#endif
    cvSaveImage("src.jpg",dst,0);
    SAMPLE_SVP_DSP_Dilate(dst);

}



