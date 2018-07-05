#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "mpi_sys.h"
#include "hi_comm_vb.h"
#include "mpi_vb.h"
#include "hi_comm_vo.h"
#include "mpi_vo.h"

#define       MAX_FRM_CNT 256

/* sp420 to p420 ; sp422 to p422  */
void sample_yuv_dump(VIDEO_FRAME_S * pVBuf, FILE *pfd)
{
    unsigned int    w, h;
    char *          pVBufVirt_Y;
    char *          pVBufVirt_C;
    char *          pMemContent;
    unsigned char   TmpBuff[4096];
    HI_U64          phy_addr;
    HI_U32          Ysize,Csize;
    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    HI_U32          u32UvHeight;/* uv height when saved for planar type */

    Ysize = (pVBuf->u32Stride[0])*(pVBuf->u32Height);
    if (PIXEL_FORMAT_YVU_SEMIPLANAR_420 == enPixelFormat)
    {
        Csize = (pVBuf->u32Stride[1])*(pVBuf->u32Height)/2;
        u32UvHeight = pVBuf->u32Height/2;
    }
    else
    {
        Csize = (pVBuf->u32Stride[1])*(pVBuf->u32Height);
        u32UvHeight = pVBuf->u32Height;
    }

    phy_addr = pVBuf->u64PhyAddr[0];

    //printf("phy_addr:%x, size:%d\n", phy_addr, size);
    pVBufVirt_Y = (HI_CHAR *) HI_MPI_SYS_Mmap(phy_addr, Ysize);
    if (NULL == pVBufVirt_Y)
    {
        return;
    }

    pVBufVirt_C = (HI_CHAR *) HI_MPI_SYS_Mmap(pVBuf->u64PhyAddr[1], Csize);
    if (NULL == pVBufVirt_C)
    {
        HI_MPI_SYS_Munmap(pVBufVirt_Y, Ysize);
        return;
    }

    /* save Y ----------------------------------------------------------------*/
    fprintf(stderr, "saving......Y......");
    fflush(stderr);
    for(h=0; h<pVBuf->u32Height; h++)
    {
        pMemContent = pVBufVirt_Y + h*pVBuf->u32Stride[0];
        fwrite(pMemContent, pVBuf->u32Width, 1, pfd);
    }
    fflush(pfd);


    /* save U ----------------------------------------------------------------*/
    fprintf(stderr, "U......");
    fflush(stderr);
    for(h=0; h<u32UvHeight; h++)
    {
        pMemContent = pVBufVirt_C + h*pVBuf->u32Stride[1];

        pMemContent += 1;

        for(w=0; w<pVBuf->u32Width/2; w++)
        {
            TmpBuff[w] = *pMemContent;
            pMemContent += 2;
        }
        fwrite(TmpBuff, pVBuf->u32Width/2, 1, pfd);
    }
    fflush(pfd);

    /* save V ----------------------------------------------------------------*/
    fprintf(stderr, "V......");
    fflush(stderr);
    for(h=0; h<u32UvHeight; h++)
    {
        pMemContent = pVBufVirt_C + h*pVBuf->u32Stride[1];

        for(w=0; w<pVBuf->u32Width/2; w++)
        {
            TmpBuff[w] = *pMemContent;
            pMemContent += 2;
        }
        fwrite(TmpBuff, pVBuf->u32Width/2, 1, pfd);
    }
    fflush(pfd);

    fprintf(stderr, "done %d!\n", pVBuf->u32TimeRef);
    fflush(stderr);

    HI_MPI_SYS_Munmap(pVBufVirt_Y, Ysize);
    HI_MPI_SYS_Munmap(pVBufVirt_C, Csize);

}

HI_S32 SAMPLE_MISC_WbcDump(VO_DEV VoDev, HI_U32 u32Cnt)
{
    HI_S32              i, s32Ret;
    VIDEO_FRAME_INFO_S  stFrame;
    HI_CHAR             szYuvName[128];
    HI_CHAR             szPixFrm[10];
    FILE *              pfd;
    HI_S32              s32MilliSec = 1000*2;

    printf("\nNOTICE: This tool only can be used for TESTING !!!\n");
    printf("usage: ./vo_wbc_dump 0 [frmcnt]. sample: ./vo_wbc_dump 0 5\n\n");

    s32Ret = HI_MPI_VO_SetWBCDepth(VoDev, 5);
    if (HI_SUCCESS != s32Ret)
    {
        printf("Dev(%d) HI_MPI_VO_SetWbcDepth errno %#x\n", VoDev, s32Ret);
        return s32Ret;
    }

    /* Get Frame to make file name*/
    s32Ret = HI_MPI_VO_GetWBCFrame(VoDev, &stFrame, s32MilliSec);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VO(%d)_WbcGetScreenFrame errno %#x\n", VoDev, s32Ret);
        return -1;
    }

    /* make file name */
    strncpy(szPixFrm,
    (PIXEL_FORMAT_YVU_SEMIPLANAR_420 == stFrame.stVFrame.enPixelFormat)?"p420":"p422",5);
    snprintf(szYuvName, 128, "./Wbc(%d)_%d_%d_%s_%d.yuv",VoDev,
        stFrame.stVFrame.u32Width, stFrame.stVFrame.u32Height,szPixFrm,u32Cnt);
    printf("Dump YUV frame of Wbc(%d) to file: \"%s\"\n",VoDev, szYuvName);

    s32Ret = HI_MPI_VO_ReleaseWBCFrame(VoDev, &stFrame);
    if (HI_SUCCESS != s32Ret)
    {
        printf("Dev(%d) HI_MPI_VO_ReleaseWbcFrame errno %#x\n", VoDev, s32Ret);
        return -1;
    }

    /* open file */
    pfd = fopen(szYuvName, "wb");

    if (NULL == pfd)
    {
        return -1;
    }

    /* get VO frame  */
    for (i=0; i<u32Cnt; i++)
    {
        s32Ret = HI_MPI_VO_GetWBCFrame(VoDev, &stFrame, s32MilliSec);
        if (HI_SUCCESS != s32Ret)
        {
            printf("get Wbc(%d) frame err\n", VoDev);
            printf("only get %d frame\n", i);
            break;
        }
        /* save VO frame to file */
        sample_yuv_dump(&stFrame.stVFrame, pfd);

        /* release frame after using */
        s32Ret = HI_MPI_VO_ReleaseWBCFrame(VoDev, &stFrame);
        if (HI_SUCCESS != s32Ret)
        {
            printf("release Wbc(%d) frame err\n", VoDev);
            printf("only get %d frame\n", i);
            break;
        }
    }

    fclose(pfd);

    return 0;
}

HI_S32 main(int argc, char *argv[])
{
    VO_DEV VoDev     = 0;
    HI_U32 u32FrmCnt = 1;

    /* VO dev num*/
    if (argc > 1)
    {
        VoDev = atoi(argv[1]);
    }

    /* frame number that need to capture*/
    if (argc > 2)
    {
        u32FrmCnt = atoi(argv[2]);
    }

    SAMPLE_MISC_WbcDump(VoDev, u32FrmCnt);

    return HI_SUCCESS;
}

