/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hi_drv_vreg.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/01/09
  Description   :
  History       :
  1.Date        : 2013/01/09
    Author      :
    Modification: Created file

******************************************************************************/
#include "hi_osal.h"
#include "mm_ext.h"
#include "hi_drv_vreg.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


static DRV_VREG_ARGS_S g_stVreg[VREG_MAX_NUM] = {{0}};
DRV_VREG_ARGS_S *VREG_DRV_Search(HI_U64 u64BaseAddr)
{
    HI_S32  i;
    for (i = 0; i < VREG_MAX_NUM; i++)
    {
        if ((0 != g_stVreg[i].u64PhyAddr)
            && (u64BaseAddr == g_stVreg[i].u64BaseAddr))
        {
            return &g_stVreg[i];
        }
    }

    return HI_NULL;
}

HI_S32 VREG_DRV_Init(HI_U64 u64BaseAddr, HI_U64 u64Size)
{
    HI_S32 s32Ret;
    HI_S32  i;
    HI_U64 u64PhyAddr;
    HI_U8 *pu8VirAddr;
    HI_CHAR acName[16] = {0};
    DRV_VREG_ARGS_S *pstVreg = HI_NULL;

    /* check param */
    if (0 == u64Size)
    {
        ISP_TRACE(HI_DBG_ERR, "The vreg's size is 0!\n");
        return HI_FAILURE;
    }

    pstVreg = VREG_DRV_Search(u64BaseAddr);
    if (HI_NULL != pstVreg)
    {
        ISP_TRACE(HI_DBG_ERR, "The vreg of u64BaseAddr 0x%llx has registerd!\n", u64BaseAddr);
        return HI_FAILURE;
    }

    /* search pos */
    for (i = 0; i < VREG_MAX_NUM; i++)
    {
        if (0 == g_stVreg[i].u64PhyAddr)
        {
            pstVreg = &g_stVreg[i];
            break;
        }
    }

    if (HI_NULL == pstVreg)
    {
        ISP_TRACE(HI_DBG_ERR, "The vreg is too many, can't register!\n");
        return HI_FAILURE;
    }

    /* Mmz malloc memory */
    osal_snprintf(acName, sizeof(acName), "VirtReg[%d]", i);
    s32Ret = CMPI_MmzMallocNocache(HI_NULL, acName, &u64PhyAddr, (HI_VOID **)&pu8VirAddr, u64Size);
    if (HI_SUCCESS != s32Ret)
    {
        ISP_TRACE(HI_DBG_ERR, "alloc virt regs buf err\n");
        return HI_FAILURE;
    }

    osal_memset(pu8VirAddr, 0, u64Size);

    pstVreg->u64PhyAddr  = u64PhyAddr;
    pstVreg->pVirtAddr   = (HI_VOID *)pu8VirAddr;
    pstVreg->u64BaseAddr = u64BaseAddr;

    return HI_SUCCESS;
}

HI_S32 VREG_DRV_Exit(HI_U64 u64BaseAddr)
{
    DRV_VREG_ARGS_S *pstVreg = HI_NULL;

    pstVreg = VREG_DRV_Search(u64BaseAddr);
    if (HI_NULL == pstVreg)
    {
        ISP_TRACE(HI_DBG_ERR, "The vreg of u64BaseAddr 0x%llx has not registerd!\n", u64BaseAddr);
        return HI_FAILURE;
    }

    if (0 != pstVreg->u64PhyAddr)
    {
        CMPI_MmzFree(pstVreg->u64PhyAddr, pstVreg->pVirtAddr);
        pstVreg->u64PhyAddr  = 0;
        pstVreg->u64Size     = 0;
        pstVreg->u64BaseAddr = 0;
        pstVreg->pVirtAddr   = HI_NULL;
    }

    return HI_SUCCESS;
}

HI_S32 VREG_DRV_GetAddr(HI_U64 u64BaseAddr, HI_U64 *pu64PhyAddr)
{
    DRV_VREG_ARGS_S *pstVreg = HI_NULL;

    pstVreg = VREG_DRV_Search(u64BaseAddr);
    if (HI_NULL == pstVreg)
    {
        ISP_TRACE(HI_DBG_ERR, "The vreg of u64BaseAddr 0x%llx has not registerd!\n", u64BaseAddr);
        return HI_FAILURE;
    }

    *pu64PhyAddr = pstVreg->u64PhyAddr;

    return HI_SUCCESS;
}

HI_VOID VREG_DRV_ReleaseAll(HI_VOID)
{
    HI_S32  i;

    for (i = 0; i < VREG_MAX_NUM; i++)
    {
        if (0 != g_stVreg[i].u64PhyAddr)
        {
            CMPI_MmzFree(g_stVreg[i].u64PhyAddr, g_stVreg[i].pVirtAddr);
            g_stVreg[i].u64PhyAddr  = 0;
            g_stVreg[i].u64BaseAddr = 0;
            g_stVreg[i].u64Size     = 0;
        }
    }

    return;
}

//long VREG_DRV_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
long VREG_DRV_ioctl(unsigned int cmd, unsigned long arg, void *private_data)
{
    unsigned int   *argp = (unsigned int *)arg;
    HI_S32  s32Ret;

    switch (cmd)
    {
            /* malloc memory for vregs, and record information in kernel. */
        case VREG_DRV_INIT :
        {
            DRV_VREG_ARGS_S *pstVreg = (DRV_VREG_ARGS_S *)argp;

            s32Ret = VREG_DRV_Init(pstVreg->u64BaseAddr, pstVreg->u64Size);
            return s32Ret;
        }
        /* free the memory of vregs, and clean information in kernel. */
        case VREG_DRV_EXIT :
        {
            DRV_VREG_ARGS_S *pstVreg = (DRV_VREG_ARGS_S *)argp;
            s32Ret = VREG_DRV_Exit(pstVreg->u64BaseAddr);
            return s32Ret;
        }
        /* free the memory of vregs, and clean information in kernel. */
        case VREG_DRV_RELEASE_ALL :
        {
            VREG_DRV_ReleaseAll();
            return 0;
        }
        /* get the mapping relation between vreg addr and physical addr. */
        case VREG_DRV_GETADDR :
        {
            DRV_VREG_ARGS_S *pstVreg = (DRV_VREG_ARGS_S *)argp;

            s32Ret = VREG_DRV_GetAddr(pstVreg->u64BaseAddr, &pstVreg->u64PhyAddr);
            if (HI_SUCCESS != s32Ret)
            {
                return s32Ret;
            }

            return 0;
        }
        default:
        {

            return HI_FAILURE;
        }
    }

    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


