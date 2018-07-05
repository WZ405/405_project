/*----------------------------------------------------------------------------
 * Copyright (c) <2013-2017>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * HuaweiLite OS may be subject to applicable export control laws and regulations, which might
 * include those applicable to HuaweiLite OS of U.S. and the country in which you are located.
 * Import, export and usage of HuaweiLite OS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/

 /** @defgroup los_list Doubly linked list
 * @ingroup kernel
 */

#ifndef _LIST_H
#define _LIST_H

#include "los_base.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */


/**
 *@ingroup los_list
 *Structure of a node in a doubly linked list.
 */
typedef struct hiSVP_DSP_DL_LIST
{
    struct hiSVP_DSP_DL_LIST *pstPrev;            /**< Current node's pointer to the previous node*/
    struct hiSVP_DSP_DL_LIST *pstNext;            /**< Current node's pointer to the next node*/
} SVP_DSP_DL_LIST;

/**
 *@ingroup los_list
 *@brief Initialize a doubly linked list.
 *
 *@par Description:
 *This API is used to initialize a doubly linked list.
 *@attention
 *<ul>
 *<li>None.</li>
 *</ul>
 *
 *@param head       [IN] Node in a doubly linked list.
 *
 *@retval None.
 *@par Dependency:
 *<ul><li>los_list.h: the header file that contains the API declaration.</li></ul>
 *@see
 *@since HuaweiLite OS V100R001C00
 */
INLINE VOID SVP_DSP_InitList(SVP_DSP_DL_LIST *pstList)
{
    pstList->pstNext = pstList;
    pstList->pstPrev = pstList;
}

/**
 *@ingroup los_list
 *@brief Point to the next node pointed to by the current node.
 *
 *@par Description:
 *<ul>
 *<li>This API is used to point to the next node pointed to by the current node.</li>
 *</ul>
 *@attention
 *<ul>
 *<li>None.</li>
 *</ul>
 *
 *@param pstObject  [IN] Node in the doubly linked list.
 *
 *@retval None.
 *@par Dependency:
 *<ul><li>los_list.h: the header file that contains the API declaration.</li></ul>
 *@see
 *@since HuaweiLite OS V100R001C00
 */
#define SVP_DSP_DL_LIST_FIRST(pstObject) ((pstObject)->pstNext)


/**
 *@ingroup los_list
 *@brief Insert a new node to a doubly linked list.
 *
 *@par Description:
 *This API is used to insert a new node to a doubly linked list.
 *@attention
 *<ul>
 *<li>None.</li>
 *</ul>
 *
 *@param pstList    [IN]    Position where the new node is inserted.
 *@param pstNode    [IN]   New node to be inserted.
 *
 *@retval None
 *@par Dependency:
 *<ul><li>los_list.h: the header file that contains the API declaration.</li></ul>
 *@see
 *@since HuaweiLite OS V100R001C00
 */
INLINE VOID SVP_DSP_ListAdd(SVP_DSP_DL_LIST *pstList, SVP_DSP_DL_LIST *pstNode)
{
    pstNode->pstNext = pstList->pstNext;
    pstNode->pstPrev = pstList;
    pstList->pstNext->pstPrev = pstNode;
    pstList->pstNext = pstNode;
}

/**
 *@ingroup los_list
 *@brief Insert a node to the tail of a doubly linked list.
 *
 *@par Description:
 *This API is used to insert a new node to the tail of a doubly linked list. pstListObject and pstNewNode must point to valid memory.
 *@attention
 *<ul>
 *<li>None.</li>
 *</ul>
 *
 *@param pstList     [IN] Doubly linked list where the new node is inserted.
 *@param pstNode     [IN] New node to be inserted.
 *
 *@retval None.
 *@par Dependency:
 *<ul><li>los_list.h: the header file that contains the API declaration.</li></ul>
 *@see DSP_ListAdd
 *@since HuaweiLite OS V100R001C00
 */
INLINE VOID SVP_DSP_ListTailInsert(SVP_DSP_DL_LIST *pstList, SVP_DSP_DL_LIST *pstNode)
{
    SVP_DSP_ListAdd(pstList->pstPrev, pstNode);
}


/**
 *@ingroup los_list
 *@brief Delete a specified node from a doubly linked list.
 *
 *@par Description:
 *<ul>
 *<li>This API is used to delete a specified node from a doubly linked list.</li>
 *</ul>
 *@attention
 *<ul>
 *<li>None.</li>
 *</ul>
 *
 *@param pstNode    [IN] Node to be deleted.
 *
 *@retval None.
 *@par Dependency:
 *<ul><li>los_list.h: the header file that contains the API declaration.</li></ul>
 *@see DSP_ListAdd
 *@since HuaweiLite OS V100R001C00
 */
INLINE VOID SVP_DSP_ListDelete(SVP_DSP_DL_LIST *pstNode)
{
    pstNode->pstNext->pstPrev = pstNode->pstPrev;
    pstNode->pstPrev->pstNext = pstNode->pstNext;
    pstNode->pstNext = (SVP_DSP_DL_LIST *)NULL;
    pstNode->pstPrev = (SVP_DSP_DL_LIST *)NULL;
}

/**
 *@ingroup los_list
 *@brief Identify whether a specified doubly linked list is empty.
 *
 *@par Description:
 *<ul>
 *<li>This API is used to return whether a doubly linked list is empty.</li>
 *</ul>
 *@attention
 *<ul>
 *<li>None.</li>
 *</ul>
 *
 *@param pstListObject  [IN] Node in the doubly linked list.
 *
 *@retval TRUE The doubly linked list is empty.
 *@retval FALSE The doubly linked list is not empty.
 *@par Dependency:
 *<ul><li>los_list.h: the header file that contains the API declaration.</li></ul>
 *@see
 *@since HuaweiLite OS V100R001C00
 */
INLINE BOOL SVP_DSP_ListEmpty(SVP_DSP_DL_LIST *pstNode)
{
    return (BOOL)(pstNode->pstNext == pstNode);
}

/**
 * @ingroup los_list
 * @brief Obtain the offset of a field to a structure address.
 *
 *@par  Description:
 *This API is used to obtain the offset of a field to a structure address.
 *@attention
 *<ul>
 *<li>None.</li>
 *</ul>
 *
 *@param type   [IN] Structure name.
 *@param field  [IN] Name of the field of which the offset is to be measured.
 *
 *@retval Offset of the field to the structure address.
 *@par Dependency:
 *<ul><li>los_list.h: the header file that contains the API declaration.</li></ul>
 *@see
 *@since HuaweiLite OS V100R001C00
 */
#define OFFSET_OF_FIELD( type, field )    ((UINT32)&(((type *)0)->field))

/**
 *@ingroup los_list
 *@brief Obtain the pointer to a doubly linked list in a structure.
 *
 *@par Description:
 *This API is used to obtain the pointer to a doubly linked list in a structure.
 *@attention
 *<ul>
 *<li>None.</li>
 *</ul>
 *
 *@param type    [IN] Structure name.
 *@param member  [IN] Member name of the doubly linked list in the structure.
 *
 *@retval Pointer to the doubly linked list in the structure.
 *@par Dependency:
 *<ul><li>los_list.h: the header file that contains the API declaration.</li></ul>
 *@see
 *@since HuaweiLite OS V100R001C00
 */
#define SVP_DSP_OFF_SET_OF(type, member) ((long)&((type *)0)->member)

#define container_of SVP_DSP_DL_LIST_ENTRY

/**
 *@ingroup los_list
 *@brief Obtain the pointer to a structure that contains a doubly linked list. *
 *
 *@par Description:
 *This API is used to obtain the pointer to a structure that contains a doubly linked list.
 *<ul>
 *<li>None.</li>
 *</ul>
 *@attention
 *<ul>
 *<li>None.</li>
 *</ul>
 *
 *@param item    [IN] Current node's pointer to the next node.
 *@param type    [IN] Structure name.
 *@param member  [IN] Member name of the doubly linked list in the structure.
 *
 *@retval Pointer to the structure that contains the doubly linked list.
 *@par Dependency:
 *<ul><li>los_list.h: the header file that contains the API declaration.</li></ul>
 *@see
 *@since HuaweiLite OS V100R001C00
 */

#define SVP_DSP_DL_LIST_ENTRY(item, type, member) \
    ((type *)((char *)item - SVP_DSP_OFF_SET_OF(type, member))) \

/**
 *@ingroup los_list
 *@brief Traverse a doubly linked list.
 *
 *@par Description:
 *This API is used to traverse all nodes in a doubly linked list.
 *@attention
 *<ul>
 *<li>None.</li>
 *</ul>
 *
 *@param item           [IN] Pointer to the structure that contains the doubly linked list that is to be traversed.
 *@param list           [IN] Pointer to the doubly linked list to be traversed.
 *@param type           [IN] Structure name.
 *@param member         [IN] Member name of the doubly linked list in the structure. *
 *
 *@retval None.
 *@par Dependency:
 *<ul><li>los_list.h: the header file that contains the API declaration.</li></ul>
 *@see
 *@since HuaweiLite OS V100R001C00
 */
#define SVP_DSP_DL_LIST_FOR_EACH(item, list, type, member) \
    for (item = SVP_DSP_DL_LIST_ENTRY((list)->pstNext, type, member); \
        &item->member != (list); \
        item = SVP_DSP_DL_LIST_ENTRY(item->member.pstNext, type, member)) \


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#endif /* _LIST_H */



