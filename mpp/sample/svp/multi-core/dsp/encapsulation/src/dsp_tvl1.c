#include "dsp_tvl1.h"
#include "mpi_dsp.h"
#include <string.h>
/*****************************************************************************
*    Prototype     : SAMPLE_SVP_DSP_ENCA_Dilate3x3
*    Description : Encapsulate Dilate 3x3
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
*
****************************************************************************/
HI_S32 SVP_DSP_TVL1_RUN(SVP_DSP_HANDLE *phHandle,SVP_DSP_ID_E enDspId,SVP_DSP_PRI_E enPri,
                    SVP_SRC_IMAGE_S *pstSrc1,SVP_SRC_IMAGE_S *pstSrc2,SVP_DST_IMAGE_S *pstDst,SVP_MEM_INFO_S *pstAssistBuf)
{
    SVP_DSP_MESSAGE_S stMsg = {0};
    HI_U8 *pu8Tmp =  NULL;
    /*Fill Message*/

    printf("CMD:    %d\n",SVP_DSP_CMD_TVL1);
    stMsg.u32CMD = SVP_DSP_CMD_TVL1;            //command
    stMsg.u32MsgId = 0;                         //command ID                                  
    stMsg.u64Body = pstAssistBuf->u64PhyAddr; 
    stMsg.u32BodyLen = 2*sizeof(SVP_SRC_IMAGE_S) + sizeof(SVP_DST_IMAGE_S); /*SRC1 +SRC2 + DST*/

    pu8Tmp = (HI_U8*) pstAssistBuf->u64VirAddr;

    memcpy(pu8Tmp,pstSrc1,sizeof(*pstSrc1)); //copy src1 image adress from pstAssistBuf to pstSrc
    pu8Tmp += sizeof(*pstSrc1);
    memcpy(pu8Tmp,pstSrc2,sizeof(*pstSrc2)); //copy src2 image adress from pstAssistBuf to pstSrc
    pu8Tmp += sizeof(*pstSrc2);

    memcpy(pu8Tmp,pstDst,sizeof(*pstDst)); //copy dst2 image adress from pstAssistBuf to pstDst

    printf("end Memcpy\n");
    /*It must flush cache ,if the buffer pstAssistBuf.u64VirAddr malloc with cache!*/
    return HI_MPI_SVP_DSP_RPC(phHandle,&stMsg, enDspId, enPri);
}




