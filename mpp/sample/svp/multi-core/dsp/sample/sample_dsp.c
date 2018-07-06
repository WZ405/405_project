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
#include "dsp_tvl1.h"
#include "mpi_dsp.h"
#include "highgui.h"
#include "cv.h"

typedef struct hiSAMPLE_SVP_DSP_TVL1_S
{
    SVP_SRC_IMAGE_S stSrc1;
    SVP_SRC_IMAGE_S stSrc2;
    SVP_DST_IMAGE_S stDst;
    SVP_MEM_INFO_S  stAssistBuf;
    SVP_DSP_PRI_E   enPri;
    SVP_DSP_ID_E    enDspId;
}SAMPLE_SVP_DSP_TVL1_S;

static SAMPLE_SVP_DSP_TVL1_S s_stTVL1 = {0};
static SAMPLE_VI_CONFIG_S s_stViConfig = {0};
static SAMPLE_IVE_SWITCH_S s_stDspSwitch = {HI_FALSE,HI_FALSE};
static HI_BOOL s_bDspStopSignal = HI_FALSE;
static pthread_t s_hDspThread = 0;

/*
*Uninit TVL1
*/
HI_VOID SVP_DSP_TVL1Uninit(SAMPLE_SVP_DSP_TVL1_S *pstTVL1)
{
    SAMPLE_COMM_SVP_DestroyMemInfo(&(pstTVL1->stAssistBuf),0);
    SAMPLE_COMM_SVP_DestroyImage(&(pstTVL1->stDst),0);
}
/*
*Init TVL1
*/
HI_S32 SVP_DSP_TVL1Init(SAMPLE_SVP_DSP_TVL1_S *pstTVL1,
    HI_U32 u32Width,HI_U32 u32Height,SVP_DSP_ID_E enDspId,SVP_DSP_PRI_E enPri)
{
    HI_S32 s32Ret;
    HI_U32 u32Size = sizeof(SVP_IMAGE_S) * 3; //two input images, one output velocity map
    memset(pstTVL1,0,sizeof(*pstTVL1));

    pstTVL1->stSrc1.u32Width  = u32Width;
    pstTVL1->stSrc1.u32Height = u32Height;
    pstTVL1->stSrc1.enType    = SVP_IMAGE_TYPE_U8C1;
    pstTVL1->stSrc2.u32Width  = u32Width;
    pstTVL1->stSrc2.u32Height = u32Height;
    pstTVL1->stSrc2.enType    = SVP_IMAGE_TYPE_U8C1;

    /*malloc memory for dst image*/
    s32Ret = SAMPLE_COMM_SVP_CreateImage(&(pstTVL1->stDst),SVP_IMAGE_TYPE_U8C1,u32Width,u32Height,0);  
    SAMPLE_SVP_CHECK_EXPR_RET(HI_SUCCESS != s32Ret, s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x):SAMPLE_COMM_SVP_CreateImage failed!\n", s32Ret);

    s32Ret = SAMPLE_COMM_SVP_CreateMemInfo(&(pstTVL1->stAssistBuf),u32Size,0); // malloc the memory for assistBuf

    SAMPLE_SVP_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x):SAMPLE_COMM_SVP_CreateMemInfo failed!\n", s32Ret);

    pstTVL1->enDspId = enDspId;
    pstTVL1->enPri   = enPri;
    return s32Ret;
FAIL_0:
    SAMPLE_COMM_SVP_DestroyImage(&(pstTVL1->stDst),0);
    memset(pstTVL1,0,sizeof(*pstTVL1));
    return s32Ret;
}
/*
*Process TVL1
*/
static HI_S32 SVP_DSP_TVL1Proc(SAMPLE_SVP_DSP_TVL1_S* pstTVL1)
{
    SVP_DSP_HANDLE hHandle;
    HI_BOOL bFinish;
    HI_BOOL bBlock = HI_TRUE;
    HI_S32 s32Ret;

    /*Call enca mpi*/
    printf("proc-----begin\n");

    s32Ret = SVP_DSP_TVL1_RUN(&hHandle, pstTVL1->enDspId,pstTVL1->enPri, &pstTVL1->stSrc1,&pstTVL1->stSrc2, &pstTVL1->stDst, &(pstTVL1->stAssistBuf));
    SAMPLE_SVP_CHECK_EXPR_RET(HI_SUCCESS != s32Ret, s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x):HI_MPI_SVP_DSP_ENCA_TVL13x3 failed!\n", s32Ret);

    
    // s32Ret = SAMPLE_SVP_DSP_ENCA_Dilate3x3(&hHandle, pstTVL1->enDspId,pstTVL1->enPri, &pstTVL1->stSrc1, &pstTVL1->stDst, &(pstTVL1->stAssistBuf));
    // SAMPLE_SVP_CHECK_EXPR_RET(HI_SUCCESS != s32Ret, s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x):HI_MPI_SVP_DSP_ENCA_TVL13x3 failed!\n", s32Ret);
    /*Wait dsp finish*/
    while(HI_ERR_SVP_DSP_QUERY_TIMEOUT == (s32Ret = HI_MPI_SVP_DSP_Query(pstTVL1->enDspId, hHandle, &bFinish, bBlock)))
    {
        usleep(100);
    }

    printf("finished !!!!\n");
    return s32Ret;
}


static HI_VOID SAMPLE_SVP_DSP_CV(IplImage*src1,IplImage*src2, SAMPLE_SVP_DSP_TVL1_S* pstTVL1)
{
    /*-------------Fill two input images---------------*/
    printf("filling image\n");
    HI_S32 s32Ret;
    HI_U32 stride = pstTVL1->stSrc1.au32Stride[0];

    for (int i = 0;i<src1->height;i++)
        for(int j=0;j<src1->width;j++)
        {
            uchar val = ((uchar*)(src1->imageData+i*(src1->widthStep)))[j];
            *((uchar*)(pstTVL1->stSrc1.au64VirAddr[0])+i*stride+j) = val;
        }
    for (int i = 0;i<src2->height;i++)
        for(int j=0;j<src2->width;j++)
        {
            uchar val = ((uchar*)(src2->imageData+i*(src2->widthStep)))[j];
            *((uchar*)(pstTVL1->stSrc2.au64VirAddr[0])+i*stride+j) = val;
        }

    printf("end filling image\n");

    /*--------------Processing TVL1--------------*/
    s32Ret = SVP_DSP_TVL1Proc(pstTVL1);

}

static HI_VOID SAVE_IMAGE(IplImage*src, SAMPLE_SVP_DSP_TVL1_S* pstTVL1)
{
    printf("saving image\n");
    IplImage *outImage = cvCreateImage(cvSize(src->width,src->height),src->depth,src->nChannels);
    HI_U32 stride = pstTVL1->stDst.au32Stride[0];
    for (int i = 0;i<outImage->width;i++)
        for(int j=0;j<outImage->height;j++)
        {
            uchar val = *((uchar*)pstTVL1->stDst.au64VirAddr[0]+j*stride+i);
            //printf("%d ",val);
            ((uchar*)(outImage->imageData+j*(outImage->widthStep)))[i] = val;
        }
    
    cvSaveImage("out.jpg",outImage,0);
}

static HI_VOID SVP_DSP_TVL1Core(SVP_DSP_ID_E enDspId,SVP_DSP_PRI_E enPri,IplImage*src1,IplImage *src2)
{
    HI_S32 s32Ret;
    PIC_SIZE_E enSize = PIC_CIF;
    SIZE_S stSize;

    /*--------------------Init MPI-------------------*/
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


    /*------------------Load bin----------------------*/
    printf("load bin-----begin\n");
    s32Ret = SAMPLE_COMM_SVP_LoadCoreBinary(enDspId);
    SAMPLE_SVP_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret,END_DSP_0, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x):SAMPLE_COMM_SVP_LoadCoreBinary failed!\n", s32Ret);
    /*------------------Init TVL1----------------------*/
    printf("init-----begin\n");
    s32Ret = SVP_DSP_TVL1Init(&s_stTVL1,stSize.u32Width,stSize.u32Height,enDspId,enPri);
    SAMPLE_SVP_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_DSP_1, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x):SAMPLE_SVP_DSP_TVL1Init failed!\n", s32Ret);

    printf("creat input image \n");
    s32Ret = SAMPLE_COMM_SVP_CreateImage(&(s_stTVL1.stSrc1),SVP_IMAGE_TYPE_U8C1,stSize.u32Width,stSize.u32Height,0);
    SAMPLE_SVP_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_DSP_1, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x):SAMPLE_SVP_DSP_TVL1Init failed!\n", s32Ret);
    s32Ret = SAMPLE_COMM_SVP_CreateImage(&(s_stTVL1.stSrc2),SVP_IMAGE_TYPE_U8C1,stSize.u32Width,stSize.u32Height,0);
    SAMPLE_SVP_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, END_DSP_1, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x):SAMPLE_SVP_DSP_TVL1Init failed!\n", s32Ret);
    SAMPLE_SVP_CHECK_EXPR_RET(HI_SUCCESS != s32Ret, s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error(%#x):SAMPLE_COMM_SVP_CreateImage failed!\n", s32Ret);

    /*---------------PROC TVL1------------------*/
    SAMPLE_SVP_DSP_CV(src1,src2,&s_stTVL1);
    SAVE_IMAGE(src1,&s_stTVL1);
    printf("Processing finished! \n");
    SAMPLE_PAUSE();

    /*---------------Free memory----------------*/
    SVP_DSP_TVL1Uninit(&s_stTVL1);
END_DSP_1:
    SAMPLE_COMM_SVP_UnLoadCoreBinary(enDspId);
END_DSP_0:
    printf("exit\n");
    exit(0);
    //SAMPLE_COMM_IVE_StopViVpssVencVo(&s_stViConfig,&s_stDspSwitch);

}

/*
*TVL1 sample
*/
HI_VOID SVP_DSP_TVL1(IplImage *src1,IplImage *src2)
{
    SVP_DSP_PRI_E enPri = SVP_DSP_PRI_0;
    SVP_DSP_ID_E enDspId = SVP_DSP_ID_0;
    printf("begin\n");
    SVP_DSP_TVL1Core(enDspId,enPri,src1,src2);
}

/*
*TVL1 single handle
*/
HI_VOID SAMPLE_SVP_DSP_TVL1HandleSig(HI_VOID)
{
    s_bDspStopSignal = HI_TRUE;
    if (0 != s_hDspThread)
    {
        pthread_join(s_hDspThread, HI_NULL);
        s_hDspThread = 0;
    }
    SVP_DSP_TVL1Uninit(&s_stTVL1);
    SAMPLE_COMM_SVP_UnLoadCoreBinary(s_stTVL1.enDspId);
    memset(&s_stTVL1,0,sizeof(s_stTVL1));
   // SAMPLE_COMM_IVE_StopViVpssVencVo(&s_stViConfig,&s_stDspSwitch);
}


