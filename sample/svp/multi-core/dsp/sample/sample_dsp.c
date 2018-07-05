#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <math.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_svp.h"

#include "sample_comm_ive.h"
#include "sample_comm_svp.h"
#include "sample_dsp_main.h"
#include "sample_dsp_enca.h"
#include "mpi_dsp.h"
#include "highgui.h"
#include "cv.h"

typedef struct hiSAMPLE_SVP_DSP_DILATE_S
{
    SVP_SRC_IMAGE_S stSrc;
    SVP_DST_IMAGE_S stDst;
    SVP_MEM_INFO_S  stAssistBuf;
    SVP_DSP_PRI_E   enPri;
    SVP_DSP_ID_E    enDspId;
}SAMPLE_SVP_DSP_DILATE_S;

static SAMPLE_SVP_DSP_DILATE_S s_stDilate = {0};
static SAMPLE_VI_CONFIG_S s_stViConfig = {0};
static SAMPLE_IVE_SWITCH_S s_stDspSwitch = {HI_FALSE,HI_FALSE};
static HI_BOOL s_bDspStopSignal = HI_FALSE;
static pthread_t s_hDspThread = 0;

/*
*Uninit Dilate
*/
HI_VOID SAMPLE_SVP_DSP_DilateUninit(SAMPLE_SVP_DSP_DILATE_S *pstDilate)
{
    SAMPLE_COMM_SVP_DestroyMemInfo(&(pstDilate->stAssistBuf),0);
    SAMPLE_COMM_SVP_DestroyImage(&(pstDilate->stDst),0);
}
/*
*Init Dilate
*/
HI_S32 SAMPLE_SVP_DSP_DilateInit(SAMPLE_SVP_DSP_DILATE_S *pstDilate,
    HI_U32 u32Width,HI_U32 u32Height,SVP_DSP_ID_E enDspId,SVP_DSP_PRI_E enPri)
{
    HI_S32 s32Ret;
    HI_U32 u32Size = sizeof(SVP_IMAGE_S) * 2;
    memset(pstDilate,0,sizeof(*pstDilate));


    pstDilate->stSrc.u32Width  = u32Width;
    pstDilate->stSrc.u32Height = u32Height;
    pstDilate->stSrc.enType    = SVP_IMAGE_TYPE_U8C1;

    s32Ret = SAMPLE_COMM_SVP_CreateImage(&(pstDilate->stDst),SVP_IMAGE_TYPE_U8C1,u32Width,u32Height,0);
    SAMPLE_SVP_CHECK_EXPR_RET(HI_SUCCESS != s32Ret, s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x):SAMPLE_COMM_SVP_CreateImage failed!\n", s32Ret);

    printf("Mem Info %d \n",u32Size);

    printf("pre Adress %x\n",pstDilate->stAssistBuf.u64PhyAddr);
    s32Ret = SAMPLE_COMM_SVP_CreateMemInfo(&(pstDilate->stAssistBuf),u32Size,0);
    printf("pos Adress %x\n",pstDilate->stAssistBuf.u64PhyAddr);
    SAMPLE_SVP_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x):SAMPLE_COMM_SVP_CreateMemInfo failed!\n", s32Ret);

    pstDilate->enDspId = enDspId;
    pstDilate->enPri   = enPri;
    return s32Ret;
FAIL_0:
    SAMPLE_COMM_SVP_DestroyImage(&(pstDilate->stDst),0);
    memset(pstDilate,0,sizeof(*pstDilate));
    return s32Ret;
}
/*
*Process Dilate
*/
static HI_S32 SAMPLE_SVP_DSP_DilateProc(SAMPLE_SVP_DSP_DILATE_S* pstDilate)
{
    SVP_DSP_HANDLE hHandle;
    HI_BOOL bFinish;
    HI_BOOL bBlock = HI_TRUE;
    HI_S32 s32Ret;
    /*Ony get YVU400*/
    // pstDilate->stSrc.au64PhyAddr[0] = pstExtFrmInfo->stVFrame.u64PhyAddr[0];
    // pstDilate->stSrc.au64VirAddr[0] = pstExtFrmInfo->stVFrame.u64VirAddr[0];
    // pstDilate->stSrc.au32Stride[0]  = pstExtFrmInfo->stVFrame.u32Stride[0];
    /*Call enca mpi*/


    printf("proc-----begin\n");
    s32Ret = SAMPLE_SVP_DSP_ENCA_Dilate3x3(&hHandle, pstDilate->enDspId,pstDilate->enPri, &pstDilate->stSrc, &pstDilate->stDst, &(pstDilate->stAssistBuf));
    SAMPLE_SVP_CHECK_EXPR_RET(HI_SUCCESS != s32Ret, s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x):HI_MPI_SVP_DSP_ENCA_Dilate3x3 failed!\n", s32Ret);
    /*Wait dsp finish*/
    while(HI_ERR_SVP_DSP_QUERY_TIMEOUT == (s32Ret = HI_MPI_SVP_DSP_Query(pstDilate->enDspId, hHandle, &bFinish, bBlock)))
    {
        usleep(100);
    }

    printf("finished !!!!\n");
    return s32Ret;
}


static HI_VOID SAMPLE_SVP_DSP_CV(IplImage*src, SAMPLE_SVP_DSP_DILATE_S* pstDilate)
{
    //fill image

    printf("filling image\n");
    HI_S32 s32Ret;
    HI_U32 stride = pstDilate->stSrc.au32Stride[0];
    printf("stride %d \n",stride);
    printf("ADDR3 %x\n",s_stDilate.stAssistBuf.u64PhyAddr);
    printf("Vddr %x\n",pstDilate->stSrc.au64VirAddr[0]);

    printf("w %d, h %d \n",src->width,src->height);
    for (int i = 0;i<src->height;i++)
        for(int j=0;j<src->width;j++)
        {
            //printf("(%d,%d) ",i,j);
            uchar val = ((uchar*)(src->imageData+i*(src->widthStep)))[j];
            *((uchar*)(pstDilate->stSrc.au64VirAddr[0])+i*stride+j) = val;
        }


    printf("end filling image\n");
    printf("ADDR1 %x\n",s_stDilate.stAssistBuf.u64PhyAddr);

    s32Ret = SAMPLE_SVP_DSP_DilateProc(pstDilate);

}

static HI_VOID SAVE_IMAGE(IplImage*src, SAMPLE_SVP_DSP_DILATE_S* pstDilate)
{
    printf("saving image\n");
    IplImage *outImage = cvCreateImage(cvSize(src->width,src->height),src->depth,src->nChannels);
    HI_U32 stride = pstDilate->stSrc.au32Stride[0];
    for (int i = 0;i<outImage->width;i++)
        for(int j=0;j<outImage->height;j++)
        {
            uchar val = *((uchar*)pstDilate->stDst.au64VirAddr[0]+j*stride+i);
            ((uchar*)(outImage->imageData+j*(outImage->widthStep)))[i] = val;
        }
    
    cvSaveImage("out.jpg",outImage,0);
}

static HI_VOID SAMPLE_SVP_DSP_DilateCore(SVP_DSP_ID_E enDspId,SVP_DSP_PRI_E enPri,IplImage*src)
{
    HI_S32 s32Ret;
    PIC_SIZE_E enSize = PIC_CIF;
    SIZE_S stSize;


    printf("-----------Init MPI---------\n");

    VB_CONFIG_S stVbConf;
    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
    
    stVbConf.u32MaxPoolCnt = 128;
    HI_U64 u64BlkSize;
    printf("SAMPLE_COMM_SYS_GetPicSize------begin\n");
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSize, &stSize);
    SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_DSP_0,
        "Error(%#x),SAMPLE_COMM_SYS_GetPicSize failed!\n", s32Ret);
    u64BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height,
                            PIXEL_FORMAT_U8C1, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    
    stVbConf.astCommPool[0].u64BlkSize = u64BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt  = 10;
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_DSP_0,
        "SAMPLE_COMM_SYS_Init failed,Error(%#x)!\n", s32Ret);

    /*Load bin*/
    printf("load bin-----begin\n");
    s32Ret = SAMPLE_COMM_SVP_LoadCoreBinary(enDspId);
    SAMPLE_SVP_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret,END_DSP_0, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x):SAMPLE_COMM_SVP_LoadCoreBinary failed!\n", s32Ret);
    /*Init*/
    printf("init-----begin\n");
    s32Ret = SAMPLE_SVP_DSP_DilateInit(&s_stDilate,stSize.u32Width,stSize.u32Height,enDspId,enPri);
    SAMPLE_SVP_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_DSP_1, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x):SAMPLE_SVP_DSP_DilateInit failed!\n", s32Ret);

    printf("ADDR0 %x\n",s_stDilate.stAssistBuf.u64PhyAddr);

    printf("creat input image \n");
    
    s32Ret = SAMPLE_COMM_SVP_CreateImage(&(s_stDilate.stSrc),SVP_IMAGE_TYPE_U8C1,stSize.u32Width,stSize.u32Height,0);
    SAMPLE_SVP_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_DSP_1, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x):SAMPLE_SVP_DSP_DilateInit failed!\n", s32Ret);

    printf("ADDR2 %x\n",s_stDilate.stAssistBuf.u64PhyAddr);
    SAMPLE_SVP_CHECK_EXPR_RET(HI_SUCCESS != s32Ret, s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x):SAMPLE_COMM_SVP_CreateImage failed!\n", s32Ret);

    SAMPLE_SVP_DSP_CV(src,&s_stDilate);
    SAVE_IMAGE(src,&s_stDilate);
    printf("Processing finsih! \n");

    SAMPLE_PAUSE();

    SAMPLE_SVP_DSP_DilateUninit(&s_stDilate);
END_DSP_1:
    SAMPLE_COMM_SVP_UnLoadCoreBinary(enDspId);
END_DSP_0:
    SAMPLE_COMM_IVE_StopViVpssVencVo(&s_stViConfig,&s_stDspSwitch);

}

/*
*Dilate sample
*/
HI_VOID SAMPLE_SVP_DSP_Dilate(IplImage *src)
{
    SVP_DSP_PRI_E enPri = SVP_DSP_PRI_0;
    SVP_DSP_ID_E enDspId = SVP_DSP_ID_0;
    printf("begin\n");
    SAMPLE_SVP_DSP_DilateCore(enDspId,enPri,src);
}

/*
*Dilate single handle
*/
HI_VOID SAMPLE_SVP_DSP_DilateHandleSig(HI_VOID)
{
    s_bDspStopSignal = HI_TRUE;
    if (0 != s_hDspThread)
    {
        pthread_join(s_hDspThread, HI_NULL);
        s_hDspThread = 0;
    }
    SAMPLE_SVP_DSP_DilateUninit(&s_stDilate);
    SAMPLE_COMM_SVP_UnLoadCoreBinary(s_stDilate.enDspId);
    memset(&s_stDilate,0,sizeof(s_stDilate));
    SAMPLE_COMM_IVE_StopViVpssVencVo(&s_stViConfig,&s_stDspSwitch);
}


