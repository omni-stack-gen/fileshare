/**
 * @file dcl_memory.h
 * @brief DCL内存管理功能声明
 * @version 0.1
 * @date 2022-04-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef DCL_MEMORY_H_
#define DCL_MEMORY_H_

#include <stdint.h>
#include <stddef.h>
#include "dcl_base.h"
#include "dcl_rt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DCL Memory管理功能初始化
 * 
 * @param cached 底层memory是否cached，0-非cached，1-cached
 * @return  DCL_ERROR_NONE-成功，其他-失败
 */
DCL_FUNC_VISIBILITY dclError dclrtMemInit(int32_t cached);

/**
 * @brief DCL Memory管理功能去初始化
 * 
 * @return  DCL_ERROR_NONE-成功，其他-失败
 */
DCL_FUNC_VISIBILITY dclError dclrtMemDeinit(void);

/**
 * @ingroup DCL
 * @brief alloc memory on device
 *
 * @par Function
 *  alloc for size linear memory on device
 *  and return a pointer to allocated memory by *devPtr
 *
 * @par Restriction
 * @li The memory requested by the dclrtMalloc interface needs to be released
 * through the dclrtFree interface.
 * @li Before calling the media data processing interface,
 * if you need to apply memory on the device to store input or output data,
 * you need to call dcldvppMalloc to apply for memory.
 * @param devPtr [IN|OUT]  pointer to pointer to allocated memory on device
 * @param size [IN]   alloc memory size
 * @param policy [IN]   memory alloc policy
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclrtFree | dcldvppMalloc
 */
DCL_FUNC_VISIBILITY dclError dclrtMalloc(void **devPtr,
                                         size_t size,
                                         dclrtMemMallocPolicy policy);

/**
 * @ingroup DCL
 * @brief alloc memory on device
 *
 * @par Function
 *  alloc for size linear memory on device
 *  and return a pointer to allocated memory by *devPtr
 *
 * @par Restriction
 * @li The memory requested by the dclrtMalloc interface needs to be released
 * through the dclrtFree interface.
 * @li Before calling the media data processing interface,
 * if you need to apply memory on the device to store input or output data,
 * you need to call dcldvppMalloc to apply for memory.
 * @param devPtr [IN|OUT]  pointer to pointer to allocated memory on device
 * @param phyAddr [OUT] pointer to physical addr correspond to devPtr
 * @param size [IN]   alloc memory size
 * @param alignSize [IN] aligned bytes for start addr
 * @param policy [IN]   memory alloc policy
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclrtFree | dcldvppMalloc
 */                                        
DCL_FUNC_VISIBILITY dclError dclrtMallocEx(void **devPtr,
                                         uint64_t *phyAddr,
                                         size_t size,
                                         size_t alignSize,
                                         dclrtMemMallocPolicy policy);

/**
 * @ingroup DCL
 * @brief free device memory
 *
 * @par Function
 *  can only free memory allocated through the dclrtMalloc interface
 * @param  devPtr [IN]  Pointer to memory to be freed
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclrtMalloc
 */
DCL_FUNC_VISIBILITY dclError dclrtFree(void *devPtr);

/**
 * @ingroup DCL
 * @brief alloc memory on host
 *
 * @par Restriction
 * @li The requested memory cannot be used in the Device
 * and needs to be explicitly copied to the Device.
 * @li The memory requested by the dclrtMallocHost interface
 * needs to be released through the dclrtFreeHost interface.
 * @param  hostPtr [IN|OUT] pointer to pointer to allocated memory on the host
 * @param  size [IN]  alloc memory size
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclrtFreeHost
 */
DCL_FUNC_VISIBILITY dclError dclrtMallocHost(void **hostPtr, size_t size);

/**
 * @ingroup DCL
 * @brief free host memory
 *
 * @par Function
 *  can only free memory allocated through the dclrtMallocHost interface
 * @param  hostPtr [IN]   free memory pointer
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclrtMallocHost
 */
DCL_FUNC_VISIBILITY dclError dclrtFreeHost(void *hostPtr);

/**
 * @brief 获取DCL运行过程中内存使用峰值
 * @return 内存使用峰值，取整到KB
 * @note 内存使用峰值并不等于DCL所需的最大内存，因为需考虑到内存碎片的存在
 */
DCL_FUNC_VISIBILITY int32_t dclrtGetMaxMemUsage(void);

/**
 * @brief 获取DCL运行过程中内存使用峰值
 * @return 内存使用峰值，取整到KB
 * @note 内存使用峰值并不等于DCL所需的最大内存，因为需考虑到内存碎片的存在
 */
DCL_FUNC_VISIBILITY int32_t dclrtGetCurMemUsage(void);

/**
 * @brief 把data cache中的内容写回到memory中
 * @param virAddr   虚拟地址
 * @param size      内存大小 
 * @note 该函数仅能在链接了dcl_memory库时使用
 */
DCL_FUNC_VISIBILITY dclError dclrtMemFlush(void* virAddr, size_t size);

/**
 * @brief 把data cache中的内容写回到memory中
 * @param phyAddr 物理地址
 * @param virAddr   虚拟地址
 * @param size      内存大小 
 */
DCL_FUNC_VISIBILITY dclError dclrtMemFlushEx(uint64_t phyAddr, void* virAddr, size_t size);

/**
 * @brief 使data cache中的内容无效
 * @param virAddr   虚拟地址
 * @param size      内存大小 
 * @note 该函数仅能在链接了dcl_memory库时使用
 */
DCL_FUNC_VISIBILITY dclError dclrtMemInvalidate(void* virAddr, size_t size);

/**
 * @brief 使data cache中的内容无效
 * @param phyAddr 物理地址
 * @param virAddr   虚拟地址
 * @param size      内存大小 
 */
DCL_FUNC_VISIBILITY dclError dclrtMemInvalidateEx(uint64_t phyAddr, void* virAddr, size_t size);

#ifdef __cplusplus
}
#endif

#endif
