/**
* @file dcl_kcf.h
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef DCL_KCF_H
#define DCL_KCF_H

#include <stdint.h>
#include "dcl_base.h"
#include "dcl_dvpp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

typedef uint64_t dclKcfHandle;


/* 
	kcf 跟踪框描述 
*/
typedef struct 
{
    dclRect      rect;  /* 跟踪框roi信息 */
    uint32_t     roiId; /* 跟踪框id */
} dclKcfRoi;


typedef struct dclKcfObjNode dclKcfObjNode;

/* 
	kcf 跟踪目标链表结构
*/
typedef struct dclKcfObjList dclKcfObjList;

/* 
	kcf 目标跟踪结果描述
*/
typedef struct 
{
    dclKcfObjNode   *pNode;          /* 目标链表节点指针 */
    int32_t         response;        /* 目标跟踪response值 */
    dclKcfRoi       roi;             /* 目标跟踪roi信息 */
    dclBool         trackOk;         /* 目标跟踪是否成功 */
} dclKcfBbox;



/*****************************************************************************
*   函数名    : dclKcfGetMemSize
*   功能描述  : KCF 获取目标内存大小
*   参数输入  : uint32_t maxObjNum      最大跟踪目标数
*	参数输出  : uint32_t *memSize       所有目标所需内存大小
*   函数返回 : dclKcfError，参考定义
*   历史记录:
*
*       1.  时间         : 2020-12-29
*           修改人       :
*           修改点       : 创建函数
*
*****************************************************************************/
DCL_FUNC_VISIBILITY dclError dclKcfGetMemSize(uint32_t maxObjNum, uint32_t* memSize);

/*****************************************************************************
*   函数名    : dclKcfCreateObjList
*   功能描述  : KCF 创建目标链表
*   参数输入  :  dclMemInfo *pMemInfo  创建目标链表内存指针, 不能为空。
*		        uint32_t maxObjNum     最大目标数
*               dclKcfObjList *pObjList  目标链表指针，不能为空。
*   函数返回 : dclKcfError，参考定义
*   历史记录:
*
*       1.  时间         : 2020-12-29
*           修改人       :
*           修改点       : 创建函数
*
*****************************************************************************/
DCL_FUNC_VISIBILITY dclError dclKcfCreateObjList(dclDataBuffer* pMemInfo, uint32_t maxObjNum, dclKcfObjList** pObjList);

/*****************************************************************************
*   函数名    : dclKcfDestroyObjList
*   功能描述  : KCF 销毁目标链表
*   参数输入  :  dclKcfObjList *pObjList  目标链表指针，不能为空。
*                  
*   函数返回 : dclKcfError，参考定义
*   历史记录:
*
*       1.  时间         : 2020-12-29
*           修改人       :
*           修改点       : 创建函数
*
*****************************************************************************/
DCL_FUNC_VISIBILITY dclError dclKcfDestroyObjList(dclKcfObjList** pObjList);

/*****************************************************************************
*   函数名    : dclKcfProcess
*   功能描述  : 提交目标给硬件处理.
*   参数输入   : dclImage    *picDesc        跟踪图像源数据指针。不能为空。
*               dclKcfObjList *pObjList     目标链表指针。不能为空.
*				dclKcfProCtrl *pKcfProCtrl  目标跟踪控制指针，不能为空。
*   函数返回 : dclKcfError，参考定义
*   历史记录:
*
*       1.  时间         : 2020-12-29
*           修改人       :
*           修改点       : 创建函数
*
*****************************************************************************/
DCL_FUNC_VISIBILITY dclError dclKcfProcess(dclKcfHandle* pKcfHandle, dcldvppPicDesc* picDesc, dclDataBuffer* picDataBuffer, 
                                            dclKcfObjList* pObjList, dclBool bInstant, dclBool bProfile);

/*****************************************************************************
*   函数名    : dclKcfSetInitObj
*   功能描述  : KCF 设置init跟踪目标
*   参数输入  :  dclKcfObjList *pObjList 目标链表指针。不能为空.
*               dclRoiInfoFloat roiInfo[] 新增init的目标
*               uint32_t objNum            新增init目标个数               
*   函数返回 : dclKcfError，参考定义
*   历史记录:
*
*       1.  时间         : 2020-12-29
*           修改人       :
*           修改点       : 创建函数
*
*****************************************************************************/
DCL_FUNC_VISIBILITY dclError dclKcfSetInitObj(dclKcfObjList* pObjList, dclKcfRoi roiInfo[], uint32_t objNum);

/*****************************************************************************
*   函数名    : dclKcfSetUpdateObj
*   功能描述  : KCF update目标更新
*   参数输入  :  dclKcfObjList *pObjList 目标链表指针。不能为空.
*               dclKcfBbox bbox   需要更新的目标信息
*               uint32_t bboxObjNum 需要更新的目标个数
*   函数返回 : dclKcfError，参考定义
*   历史记录:
*
*       1.  时间         : 2020-12-29
*           修改人       :
*           修改点       : 创建函数
*
*****************************************************************************/
DCL_FUNC_VISIBILITY dclError dclKcfSetUpdateObj(dclKcfObjList* pObjList, dclKcfBbox bbox[], uint32_t bboxObjNum);

/*****************************************************************************
*   函数名    : dclKcfGetObjBbox
*   功能描述  : KCF 获取目标区域跟踪结果信息
*   参数输入  :  dclKcfObjList *pObjList 目标链表指针，不能为空。
*   参数输出  :  dclKcfBbox bbox   目标区域跟踪结果信息
*               uint32_t *bboxObjNum   目标区域跟踪有效结果个数
*   函数返回 : dclKcfError，参考定义
*   历史记录:
*
*       1.  时间         : 2020-12-29
*           修改人       :
*           修改点       : 创建函数
*
*****************************************************************************/
DCL_FUNC_VISIBILITY dclError dclKcfGetObjBbox(dclKcfObjList* pObjList, dclKcfBbox bbox[], uint32_t* bboxObjNum);


/*****************************************************************************
*   函数名    : dclKcfJudgeObjBboxTrackState
*   功能描述  : KCF 判断目标区域跟踪状态
*   参数输入  :  dclRoiInfo* pRoiInfo 目标区域信息指针。
*                dclKcfBbox* pBox     目标跟踪结果信息指针
*                thresh      跟踪状态判断阈值，response小于此值跟踪失败，否则跟踪成功，推荐值为64
*   参数输出  :  Bool* pTrackOk   目标区域跟踪状态指针
*   函数返回 : dclKcfError，参考定义
*   历史记录:
*
*       1.  时间         : 2020-12-29
*           修改人       :
*           修改点       : 创建函数
*
*****************************************************************************/
DCL_FUNC_VISIBILITY dclError dclKcfJudgeObjBboxTrackState(const dclKcfRoi* detectedRoi, 
                                         const dclKcfBbox* trackedBox, 
                                         dclBool* pTrackOk, 
                                         int32_t thresh);

/*****************************************************************************
*   Prototype    : dclKcfQuery
*   Description  : KCF 查询硬件是否完成
*   Parameters  :  dclKcfHandle kcfHandle kcf处理函数返回的handle
*                  Bool *pbFinish   是否完成标识.
*                  Bool bBlock     是否阻塞查询
*   Return Value : DCL_SUCCESS: Success;Error codes: Failure.
*   Spec         : 
*   History:
*
*       1.  Date         : 2020-12-29
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
DCL_FUNC_VISIBILITY dclError dclKcfQuery(dclKcfObjList* pObjList, dclKcfHandle kcfHandle, dclBool *pbFinish, dclBool bBlock);

/*****************************************************************************
*   Prototype    : dclKcfGetProfile
*   Description  : KCF 获取本次跟踪profile信息
*   Parameters  :  dclKcfHandle kcfHandle kcf处理函数返回的handle
*                  uint64_t *startTimeStamp   本次跟踪任务硬件处理起始时间点.
*                  uint64_t *endTimeStamp     本次跟踪任务硬件处理结束时间点
*   Return Value : DCL_SUCCESS: Success;Error codes: Failure.
*   Spec         :  该函数需要在调用dclKcfQuery之后方可调用
*   History:
*
*       1.  Date         : 2020-12-29
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
DCL_FUNC_VISIBILITY dclError dclKcfGetProfile(dclKcfObjList* pObjList, dclKcfHandle kcfHandle, uint64_t *startTimeStamp, uint64_t* endTimeStamp);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
