/*
 * vmm_api.h
 *
 * Copyright (c) Fullhan Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */


#ifndef VMM_API_H_
#define VMM_API_H_

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

#include "types/type_def.h"

/*Get MMZ inforfor zone 'strZone', pu32PhyAddr --MMZ start address,
            u32TLenKb-- total mmz size in KB,  pu32FLenKb-- free mmz ins in KB, pu32Blocks --mmb blocks*/
FH_SINT32 FH_SYS_VmmInfo( FH_CHAR *strZone, FH_UINT32 *pu32PhyAddr,
        FH_UINT32 *pu32TLenKb,FH_UINT32 *pu32FLenKb, FH_UINT32 *pu32Blocks);

/**
 * @brief      VMM 内存块申请，并获取 NonCached 映射的虚拟地址
 *
 * @param[out] pu32PhyAddr  申请的物理地址
 * @param[out] ppVirtAddr   物理地址对应的虚拟地址
 * @param[in]  strMmb       要申请内存块名字
 * @param[in]  strZone      从哪个 mmz 申请，如传入 NULL 则从anonymous 中申请
 * @param[in]  u32Len       要申请的内存块大小
 *
 * @retval     0            成功
 * @retval     "非0"         失败，其值参见 @namelink{错误码,vmm_errcode} 。
 *
 */
FH_SINT32 FH_SYS_VmmAlloc(FH_UINT32 *pu32PhyAddr, FH_VOID **ppVirtAddr,
        const FH_CHAR *strMmb, const FH_CHAR *strZone, FH_UINT32 u32Len);


/**
 * @brief      VMM 内存块申请，并获取 NonCached 映射的虚拟地址，可配置地址对齐
 *
 * @param[out] pu32PhyAddr  申请的物理地址
 * @param[out] ppVirtAddr   物理地址对应的虚拟地址
 * @param[in]  strMmb       要申请内存块名字
 * @param[in]  strZone      从哪个 mmz 申请，如传入 NULL 则从anonymous 中申请
 * @param[in]  u32Len       要申请的内存块大小
 * @param[in]  u32Align     地址对齐大小
 *
 * @retval     0            成功
 * @retval     "非0"        失败，其值参见 @namelink{错误码,vmm_errcode} 。
 *
 */
FH_SINT32 FH_SYS_VmmAllocEx(FH_UINT32 *pu32PhyAddr, FH_VOID **ppVirtAddr,
                const FH_CHAR *strMmb, const FH_CHAR *strZone, FH_UINT32 u32Len,
                FH_UINT32 u32Align);

/**
 * @brief      VMM 内存块申请，并获取 Cached 映射的虚拟地址
 *
 * @param[out] pu32PhyAddr  申请的物理地址
 * @param[out] ppVirtAddr   物理地址对应的虚拟地址
 * @param[in]  pstrMmb      要申请内存块名字
 * @param[in]  pstrZone     从哪个 mmz 申请，如传入 NULL 则从anonymous 中申请
 * @param[in]  u32Len       要申请的内存块大小
 *
 * @retval     0            成功
 * @retval     "非0"        失败，其值参见 @namelink{错误码,vmm_errcode} 。
 *
 */
FH_SINT32 FH_SYS_VmmAlloc_Cached(FH_UINT32 *pu32PhyAddr, FH_VOID **ppVirtAddr,
        const FH_CHAR *pstrMmb, const FH_CHAR *pstrZone, FH_UINT32 u32Len);

/**
 * @brief      VMM 内存块申请，并获取 Cached 映射的虚拟地址，可配置地址对齐
 *
 * @param[out] pu32PhyAddr  申请的物理地址
 * @param[out] ppVirtAddr   物理地址对应的虚拟地址
 * @param[in]  pstrMmb      要申请内存块名字
 * @param[in]  pstrZone     从哪个 mmz 申请，如传入 NULL 则从anonymous 中申请
 * @param[in]  u32Len       要申请的内存块大小
 * @param[in]  u32Align     地址对齐大小
 *
 * @retval     0            成功
 * @retval     "非0"         失败，其值参见 @namelink{错误码,vmm_errcode} 。
 *
 */
FH_SINT32 FH_SYS_VmmAllocEx_Cached(FH_UINT32 *pu32PhyAddr, FH_VOID **ppVirtAddr,
        const FH_CHAR *pstrMmb, const FH_CHAR *pstrZone, FH_UINT32 u32Len,
        FH_UINT32 u32Align);

/* 64bit api */

/**
 * @brief      VMM 内存块申请，并获取 NonCached 映射的虚拟地址
 *
 * @param[out] pu64PhyAddr  申请的物理地址
 * @param[out] ppVirtAddr   物理地址对应的虚拟地址
 * @param[in]  strMmb       要申请内存块名字
 * @param[in]  strZone      从哪个 mmz 申请，如传入 NULL 则从anonymous 中申请
 * @param[in]  u32Len       要申请的内存块大小
 *
 * @retval     0            成功
 * @retval     "非0"         失败，其值参见 @namelink{错误码,vmm_errcode} 。
 *
 */
FH_SINT32 FH_SYS_VmmAlloc64(FH_UINT64 *pu64PhyAddr, FH_VOID **ppVirtAddr,
        const FH_CHAR *strMmb, const FH_CHAR *strZone, FH_UINT32 u32Len);

/**
 * @brief      VMM 内存块申请，并获取 NonCached 映射的虚拟地址，可配置地址对齐
 *
 * @param[out] pu64PhyAddr  申请的物理地址
 * @param[out] ppVirtAddr   物理地址对应的虚拟地址
 * @param[in]  strMmb       要申请内存块名字
 * @param[in]  strZone      从哪个 mmz 申请，如传入 NULL 则从anonymous 中申请
 * @param[in]  u32Len       要申请的内存块大小
 * @param[in]  u32Align     地址对齐大小
 *
 * @retval     0            成功
 * @retval     "非0"         失败，其值参见 @namelink{错误码,vmm_errcode} 。
 *
 */
FH_SINT32 FH_SYS_VmmAllocEx64(FH_UINT64 *pu64PhyAddr, FH_VOID **ppVirtAddr,
                const FH_CHAR *strMmb, const FH_CHAR *strZone, FH_UINT32 u32Len,
                FH_UINT32 u32Align);

/* alloc mmz memory with cache */

/**
 * @brief      VMM 内存块申请，并获取 Cached 映射的虚拟地址
 *
 * @param[out] pu64PhyAddr  申请的物理地址
 * @param[out] ppVirtAddr   物理地址对应的虚拟地址
 * @param[in]  pstrMmb      要申请内存块名字
 * @param[in]  pstrZone     从哪个 mmz 申请，如传入 NULL 则从anonymous 中申请
 * @param[in]  u32Len       要申请的内存块大小
 *
 * @retval     0            成功
 * @retval     "非0"         失败，其值参见 @namelink{错误码,vmm_errcode} 。
 *
 */
FH_SINT32 FH_SYS_VmmAlloc_Cached64(FH_UINT64 *pu64PhyAddr, FH_VOID **ppVirtAddr,
        const FH_CHAR *pstrMmb, const FH_CHAR *pstrZone, FH_UINT32 u32Len);

/**
 * @brief      VMM 内存块申请，并获取 Cached 映射的虚拟地址，可配置地址对齐
 *
 * @param[out] pu64PhyAddr  申请的物理地址
 * @param[out] ppVirtAddr   物理地址对应的虚拟地址
 * @param[in]  pstrMmb      要申请内存块名字
 * @param[in]  pstrZone     从哪个 mmz 申请，如传入 NULL 则从anonymous 中申请
 * @param[in]  u32Len       要申请的内存块大小
 * @param[in]  u32Align     地址对齐大小
 *
 * @retval     0            成功
 * @retval     "非0"        失败，其值参见 @namelink{错误码,vmm_errcode} 。
 *
 */
FH_SINT32 FH_SYS_VmmAllocEx_Cached64(FH_UINT64 *pu64PhyAddr, FH_VOID **ppVirtAddr,
        const FH_CHAR *pstrMmb, const FH_CHAR *pstrZone, FH_UINT32 u32Len,
        FH_UINT32 u32Align);

/* free mmz memory in user context                                          */

/**
 * @brief      释放当前进程申请的指定物理内存块，并解映射对应的虚拟地址
 *
 * @param[in]  u32PhyAddr  要释放的物理地址
 *
 * @retval     0            成功
 * @retval     "非0"        失败，其值参见 @namelink{错误码,vmm_errcode} 。
 * 
 * @note
 *    - 必须确保所有已申请的内存不在使用，释放后所有已申请内存软件不可访问
 */
FH_SINT32 FH_SYS_VmmFree(FH_UINT32 u32PhyAddr);

/**
 * @brief      释放当前进程申请的指定物理内存块，并解映射对应的虚拟地址
 *
 * @param[in]  u64PhyAddr  要释放的物理地址
 *
 * @retval     0           成功
 * @retval     "非0"       失败，其值参见 @namelink{错误码,vmm_errcode} 。
 *
 * @note       
 *    - 必须确保所有已申请的内存不在使用，释放后所有已申请内存软件不可访问
 *
 */
FH_SINT32 FH_SYS_VmmFree64(FH_UINT64 u64PhyAddr);

/**
 * @brief      释放当前进程申请的指定物理内存块，并解映射对应的虚拟地址
 *
 * @param[in]  u32PhyAddr  要释放的物理内存起始地址
 *
 * @retval     0           成功
 * @retval     "非0"       失败，其值参见 @namelink{错误码,vmm_errcode} 。
 *
 * @note     
 *    - 必须确保所有已申请的内存不在使用，释放后所有已申请内存软件不可访问
 *
 */
FH_SINT32 FH_SYS_VmmFreeOne(FH_UINT32 u32PhyAddr);

/**
 * @brief      释放当前进程申请的指定物理内存块，并解映射对应的虚拟地址
 *
 * @param[in]  u64PhyAddr  要释放的物理内存起始地址
 *
 * @retval     0           成功
 * @retval     "非0"       失败，其值参见 @namelink{错误码,vmm_errcode} 。
 *
 * @note     
 *    - 必须确保所有已申请的内存不在使用，释放后所有已申请内存软件不可访问
 *
 */
FH_SINT32 FH_SYS_VmmFreeOne64(FH_UINT64 u64PhyAddr);

/*
** flush cache. This API must be called if FH_SYS_VmmAlloc_CachedXXX is called
*/


/**
 * @brief      对指定地址范围执行 Flush Cache 操作
 *
 * @param[in]  u32PhyAddr  物理内存起始地址
 * @param[in]  pVirtAddr   对应的虚拟地址
 * @param[in]  u32Size     内存大小
 *
 * @retval     0           成功
 * @retval     "非0"       失败，其值参见 @namelink{错误码,vmm_errcode} 。
 * 
 */
FH_SINT32 FH_SYS_VmmFlushCache(FH_UINT32 u32PhyAddr, FH_VOID *pVirtAddr, FH_UINT32 u32Size);

/**
 * @brief      对指定地址范围执行 Flush Cache 操作
 *
 * @param[in]  u64PhyAddr  物理内存起始地址
 * @param[in]  pVirtAddr   对应的虚拟地址
 * @param[in]  u32Size     内存大小
 *
 * @retval     0           成功
 * @retval     "非0"       失败，其值参见 @namelink{错误码,vmm_errcode} 。
 *
 */
FH_SINT32 FH_SYS_VmmFlushCache64(FH_UINT64 u64PhyAddr, FH_VOID *pVirtAddr, FH_UINT32 u32Size);

/*
** flush cache.This MPI must be called if FH_SYS_MmapCachedXXX is called.
*/

/**
 * @brief      对指定地址范围执行 Flush Cache 操作
 *
 * @param[in]  u32PhyAddr  物理内存起始地址
 * @param[in]  pVirAddr    对应的虚拟地址
 * @param[in]  u32Size     内存大小
 *
 * @retval     0           成功
 * @retval     "非0"       失败，其值参见 @namelink{错误码,vmm_errcode} 。
 * 
 */
FH_SINT32 FH_SYS_MflushCache(FH_UINT32 u32PhyAddr, FH_VOID *pVirAddr, FH_UINT32 u32Size);

/*
** Call the mmap function to map physical address to virtual address
** The system function mmap is too complicated, so we packge it.
** 存储映射接口
*/



/**
 * @brief      映射指定物理地址范围，返回 NonCached 映射的虚拟地址
 *
 * @param[in]  u32PhyAddr  物理内存起始地址
 * @param[in]  u32Size    内存大小
 *
 * @retval     0           成功
 * @retval     "非0"       失败，其值参见 @namelink{错误码,vmm_errcode} 。 
 *
 */
FH_VOID *FH_SYS_Mmap(FH_UINT32 u32PhyAddr, FH_UINT32 u32Size);

/**
 * @brief      映射指定物理地址范围，返回 NonCached 映射的虚拟地址
 *
 * @param[in]  u64PhyAddr  物理内存起始地址
 * @param[in]  u32Size    内存大小
 *
 * @retval     0           成功
 * @retval     "非0"       失败，其值参见 @namelink{错误码,vmm_errcode} 。 
 *
 */
FH_VOID *FH_SYS_Mmap64(FH_UINT64 u64PhyAddr, FH_UINT32 u32Size);

/**
 * @brief      映射指定物理地址范围，返回 Cached 映射的虚拟地址
 *
 * @param[in]  u32PhyAddr  物理内存起始地址
 * @param[in]  u32Size     内存大小
 *
 * @retval     0           成功
 * @retval     "非0"       失败，其值参见 @namelink{错误码,vmm_errcode} 。 
 *
 */
FH_VOID *FH_SYS_MmapCached(FH_UINT32 u32PhyAddr, FH_UINT32 u32Size);

/**
 * @brief      映射指定物理地址范围，返回 Cached 映射的虚拟地址
 *
 * @param[in]  u64PhyAddr  物理内存起始地址
 * @param[in]  u32Size     内存大小
 *
 * @retval     0           成功
 * @retval     "非0"        失败，其值参见 @namelink{错误码,vmm_errcode} 。
 *
 */
FH_VOID *FH_SYS_MmapCached64(FH_UINT64 u64PhyAddr, FH_UINT32 u32Size);


/**
 * @brief      解映射虚拟地址
 *
 * @param[in]  pVirtAddr  虚拟起始地址
 * @param[in]  u32Size    内存大小
 *
 * @retval     0           成功
 * @retval     "非0"        失败，其值参见 @namelink{错误码,vmm_errcode} 。
 *
 */
FH_SINT32 FH_SYS_Munmap(FH_VOID* pVirtAddr, FH_UINT32 u32Size);

/* Get address of virtual register */
FH_SINT32 FH_SYS_GetVRegAddr(FH_UINT32 *pu32Addr);

/* Get physical address by virtual address */


/**
 * @brief      获取指定物理地址对应的虚拟地址
 *
 * @param[in]      u32PhyAddr  物理内存起始地址
 * @param[out]     pVirtAddr   虚拟地址
 *
 * @retval     0           成功
 * @retval     "非0"        失败，其值参见 @namelink{错误码,vmm_errcode} 。
 * 
 * @note 
 *    - 仅对FH_SYS_VmmAlloc开头的 API 申请的物理地址生效，无法根据FH_SYS_Mmap等接口返回的虚拟地址获取对应的物理地址
 */
FH_SINT32 FH_SYS_VmmGetPaddr_ByVaddr(FH_UINT32 *u32PhyAddr, FH_VOID *pVirtAddr);

/**
 * @brief      获取指定物理地址对应的虚拟地址
 *
 * @param[in]      u64PhyAddr  物理内存起始地址
 * @param[out]     pVirtAddr   虚拟地址
 *
 * @retval     0           成功
 * @retval     "非0"        失败，其值参见 @namelink{错误码,vmm_errcode} 。
 * 
 * @note 
 *    - 仅对FH_SYS_VmmAlloc开头的 API 申请的物理地址生效，无法根据FH_SYS_Mmap等接口返回的虚拟地址获取对应的物理地址
 * 
 */
FH_SINT32 FH_SYS_VmmGetPaddr_ByVaddr64(FH_UINT64 *u64PhyAddr, FH_VOID *pVirtAddr);

/* Close all the FD which is used by sys module */


/**
 * @brief     close vmm 设备的 fd，释放 vmm 所有资源
 *
 *
 * @retval     0           成功
 * @retval     "非0"        失败，其值参见 @namelink{错误码,vmm_errcode} 。
 * 
 * @note
 *    - 请谨慎调用此接口，正常业务逻辑代码中请勿调用
 *  
 */
FH_SINT32 FH_SYS_CloseFd(FH_VOID);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* VMM_API_H_ */
