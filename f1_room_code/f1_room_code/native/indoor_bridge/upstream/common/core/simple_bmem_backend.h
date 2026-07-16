/**
 * @file
 * @brief simple 示例 bmem 内存后端注册接口。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#ifndef __SIMPLE_BMEM_BACKEND_H__
#define __SIMPLE_BMEM_BACKEND_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 将 examples/support/utils/bmem 注册为 simple 内存后端。
 *
 * @return 成功返回 0；失败返回负数。
 */
int simple_bmem_backend_register(void);

#ifdef __cplusplus
}
#endif

#endif
