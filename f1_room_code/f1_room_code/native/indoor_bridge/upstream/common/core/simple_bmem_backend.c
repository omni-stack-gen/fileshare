/**
 * @file
 * @brief simple 示例 bmem 内存后端适配实现。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#include "simple_bmem_backend.h"

#include "simple_mem.h"

#include <utils/bmem.h>

static void *simple_bmem_malloc(void *ctx, size_t size, const char *file, int line)
{
    (void)ctx;
    return bmem_dbg_malloc(file ? file : "unknown", line, size);
}

static void *simple_bmem_realloc(void *ctx, void *ptr, size_t size, const char *file, int line)
{
    (void)ctx;
    return bmem_dbg_realloc(file ? file : "unknown", line, ptr, size);
}

static void simple_bmem_free(void *ctx, void *ptr, const char *file, int line)
{
    (void)ctx;
    bmem_dbg_free(file ? file : "unknown", line, ptr);
}

static void simple_bmem_debug_dump(void *ctx, int verbose)
{
    (void)ctx;
    bmem_debug_dump(verbose);
}

int simple_bmem_backend_register(void)
{
    static const simple_mem_backend_t backend =
    {
        NULL,
        simple_bmem_malloc,
        simple_bmem_realloc,
        simple_bmem_free,
        simple_bmem_debug_dump,
    };

    return simple_mem_register_backend(&backend);
}
