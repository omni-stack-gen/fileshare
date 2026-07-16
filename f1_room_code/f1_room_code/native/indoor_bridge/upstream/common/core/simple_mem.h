/**
 * @file
 * @brief simple 示例应用公共模块 simple_mem 的公共头文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#ifndef __SIMPLE_MEM_H__
#define __SIMPLE_MEM_H__

#include <stddef.h>

struct call_mem_hooks;
struct mtrans_mem_hooks;
struct transport_mem_hooks;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief simple 示例内存后端。
 *
 * 后端函数由进程入口注册。未注册时 simple_mem 使用 libc fallback，便于 host 测试和早期
 * 初始化；板端主线进程应注册 bmem 后端以保留泄漏诊断能力。
 */
typedef struct simple_mem_backend
{
    /** 后端私有上下文，可为 NULL。 */
    void *ctx;
    /** 申请内存。 */
    void *(*malloc_fn)(void *ctx, size_t size, const char *file, int line);
    /** 调整内存大小。 */
    void *(*realloc_fn)(void *ctx, void *ptr, size_t size, const char *file, int line);
    /** 释放内存。 */
    void (*free_fn)(void *ctx, void *ptr, const char *file, int line);
    /** 输出后端内存诊断信息，可为 NULL。 */
    void (*debug_dump_fn)(void *ctx, int verbose);
} simple_mem_backend_t;

/**
 * @brief 注册 simple 示例内存后端。
 *
 * @param[in] backend 后端描述，函数指针在调用期间会被复制，可为 NULL 表示恢复 libc fallback。
 *
 * @return 成功返回 0；参数非法返回负数。
 */
int simple_mem_register_backend(const simple_mem_backend_t *backend);

/**
 * @brief 申请 simple 示例应用层内存。
 *
 * @param[in] size 需要申请的字节数。
 * @param[in] file 调用点文件名。
 * @param[in] line 调用点行号。
 *
 * @return 成功返回内存指针；失败返回 NULL。
 */
void *simple_example_malloc(size_t size, const char *file, int line);

/**
 * @brief 申请并清零 simple 示例应用层数组内存。
 *
 * @param[in] count 元素数量。
 * @param[in] size 单个元素字节数。
 * @param[in] file 调用点文件名。
 * @param[in] line 调用点行号。
 *
 * @return 成功返回内存指针；失败返回 NULL。
 */
void *simple_example_calloc(size_t count, size_t size, const char *file, int line);

/**
 * @brief 调整 simple 示例应用层内存大小。
 *
 * @param[in,out] ptr 原内存指针，可为 NULL。
 * @param[in] size 新字节数；为 0 时等价于释放。
 * @param[in] file 调用点文件名。
 * @param[in] line 调用点行号。
 *
 * @return 成功返回新内存指针；失败返回 NULL。
 */
void *simple_example_realloc(void *ptr, size_t size, const char *file, int line);

/**
 * @brief 复制字符串到 simple 示例应用层内存。
 *
 * @param[in] src 源字符串，可为 NULL。
 * @param[in] file 调用点文件名。
 * @param[in] line 调用点行号。
 *
 * @return 成功返回字符串副本；失败或 src 为 NULL 返回 NULL。
 */
char *simple_example_strdup(const char *src, const char *file, int line);

/**
 * @brief 释放 simple 示例应用层内存。
 *
 * @param[in,out] ptr 待释放指针，可为 NULL。
 * @param[in] file 调用点文件名。
 * @param[in] line 调用点行号。
 */
void simple_example_free(void *ptr, const char *file, int line);

/**
 * @brief 获取 simple 示例提供给 mtrans 的内存 hook。
 *
 * @return 静态 hook 指针，调用方不需要释放。
 */
const struct mtrans_mem_hooks *simple_example_mtrans_mem_hooks(void);

/**
 * @brief 获取 simple 示例提供给 call 模块的内存 hook。
 *
 * @return 静态 hook 指针，调用方不需要释放。
 */
const struct call_mem_hooks *simple_example_call_mem_hooks(void);

/**
 * @brief 获取 simple 示例提供给 transport 模块的内存 hook。
 *
 * @return 静态 hook 指针，调用方不需要释放。
 */
const struct transport_mem_hooks *simple_example_transport_mem_hooks(void);

/**
 * @brief 打印 simple 示例内存 hook 的调试信息。
 *
 * @param[in] verbose 非 0 表示输出更详细的诊断信息。
 */
void simple_example_mem_debug_dump(int verbose);

/** @brief 申请 simple 示例应用层内存。 */
#define SIMPLE_MALLOC(size) \
    simple_example_malloc((size), __FILE__, __LINE__)
/** @brief 申请并清零 simple 示例应用层数组内存。 */
#define SIMPLE_CALLOC(count, size) \
    simple_example_calloc((count), (size), __FILE__, __LINE__)
/** @brief 调整 simple 示例应用层内存大小。 */
#define SIMPLE_REALLOC(ptr, size) \
    simple_example_realloc((ptr), (size), __FILE__, __LINE__)
/** @brief 复制字符串到 simple 示例应用层内存。 */
#define SIMPLE_STRDUP(src) \
    simple_example_strdup((src), __FILE__, __LINE__)
/** @brief 释放 simple 示例应用层内存。 */
#define SIMPLE_FREE(ptr) \
    simple_example_free((ptr), __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif
