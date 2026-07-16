#ifndef _BUFCTRL_H_
#define _BUFCTRL_H_

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

typedef struct mem_desc {
	unsigned int base;    //!< 申请的物理地址
	void        *vbase;   //!< 物理地址对应的虚拟地址
	unsigned int size;    //!< 要申请的内存块大小
	unsigned int align;   //!< 地址对齐大小
} MEM_DESC;

typedef struct mem_desc64 {
	unsigned long long base;
	void        *vbase;
	unsigned int size;
	unsigned int align;
} MEM_DESC64;

/**
 * @brief      VMM 内存块申请，并获取 NonCached 映射的虚拟地址
 *
 * @param[out]  mem    申请的物理地址和虚拟地址信息
 * @param[in]  	size   要申请的内存块大小
 * @param[in]  	align  地址对齐大小
 *
 * @retval     0            成功
 * @retval     "非0"        失败，其值参见 @namelink{错误码,vmm_errcode} 。
 * 
 */
int buffer_malloc(MEM_DESC *mem, int size, int align);


/**
 * @brief     VMM 内存块申请，并获取 Cached 映射的虚拟地址
 *
 * @param[out]  mem    申请的物理地址和虚拟地址信息
 * @param[in]  	size   要申请的内存块大小
 * @param[in]  	align  地址对齐大小
 * @param[in]   name   要申请内存块名字
 *
 * @retval     0            成功
 * @retval     "非0"        失败，其值参见 @namelink{错误码,vmm_errcode} 。
 */
int buffer_malloc_withname_cached(MEM_DESC *mem, int size, int align,const char* name);

/**
 * @brief      VMM 内存块申请，并获取 NonCached 映射的虚拟地址
 *
 * @param[out]  mem    申请的物理地址和虚拟地址信息
 * @param[in]  	size   要申请的内存块大小
 * @param[in]  	align  地址对齐大小
 * @param[in]   name   要申请内存块名字
 *
 * @retval     0            成功
 * @retval     "非0"        失败，其值参见 @namelink{错误码,vmm_errcode} 。
 * 
 */
int buffer_malloc_withname(MEM_DESC *mem, int size, int align, const char* name);
int buffer_malloc_withname_sram(MEM_DESC *mem, int size, int align, const char* name);

int buffer_malloc64(MEM_DESC64 *mem, int size, int align);
int buffer_malloc_withname_cached64(MEM_DESC64 *mem, int size, int align,const char* name);
int buffer_malloc_withname64(MEM_DESC64 *mem, int size, int align, const char* name);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif
