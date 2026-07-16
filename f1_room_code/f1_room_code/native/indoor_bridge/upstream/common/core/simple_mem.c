/**
 * @file
 * @brief simple 示例应用公共模块 simple_mem 的实现文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#include "simple_mem.h"

#include "call_flow_runtime.h"
#include "mtrans.h"
#include "transport.h"

#include <stdlib.h>
#include <string.h>

static simple_mem_backend_t simple_mem_backend;
static int simple_mem_backend_registered = 0;

static void simple_mem_free(void *ptr, const char *file, int line);

void simple_example_mem_debug_dump(int verbose)
{
    if (simple_mem_backend_registered && simple_mem_backend.debug_dump_fn)
    {
        simple_mem_backend.debug_dump_fn(simple_mem_backend.ctx, verbose);
    }
}

static void *simple_mem_malloc(size_t size, const char *file, int line)
{
    (void)file;
    (void)line;

    if (simple_mem_backend_registered && simple_mem_backend.malloc_fn)
    {
        return simple_mem_backend.malloc_fn(simple_mem_backend.ctx, size, file, line);
    }

    return malloc(size);
}

static void *simple_mem_realloc(void *ptr, size_t size, const char *file, int line)
{
    if (size == 0)
    {
        simple_mem_free(ptr, file, line);
        return NULL;
    }

    if (simple_mem_backend_registered && simple_mem_backend.realloc_fn)
    {
        return simple_mem_backend.realloc_fn(simple_mem_backend.ctx, ptr, size, file, line);
    }

    return realloc(ptr, size);
}

static void simple_mem_free(void *ptr, const char *file, int line)
{
    (void)file;
    (void)line;

    if (!ptr)
    {
        return;
    }

    if (simple_mem_backend_registered && simple_mem_backend.free_fn)
    {
        simple_mem_backend.free_fn(simple_mem_backend.ctx, ptr, file, line);
        return;
    }

    free(ptr);
}

int simple_mem_register_backend(const simple_mem_backend_t *backend)
{
    if (!backend)
    {
        memset(&simple_mem_backend, 0, sizeof(simple_mem_backend));
        simple_mem_backend_registered = 0;
        return 0;
    }
    if (!backend->malloc_fn || !backend->realloc_fn || !backend->free_fn)
    {
        return -1;
    }

    simple_mem_backend = *backend;
    simple_mem_backend_registered = 1;
    return 0;
}

void *simple_example_malloc(size_t size, const char *file, int line)
{
    return simple_mem_malloc(size, file, line);
}

void *simple_example_calloc(size_t count, size_t size, const char *file, int line)
{
    void *ptr;

    if (count != 0U && size > ((size_t)-1) / count)
    {
        return NULL;
    }

    ptr = simple_example_malloc(count * size, file, line);
    if (ptr)
    {
        memset(ptr, 0, count * size);
    }

    return ptr;
}

void *simple_example_realloc(void *ptr, size_t size, const char *file, int line)
{
    return simple_mem_realloc(ptr, size, file, line);
}

char *simple_example_strdup(const char *src, const char *file, int line)
{
    size_t len;
    char *dst;

    if (!src)
    {
        return NULL;
    }

    len = strlen(src) + 1U;
    dst = (char *)simple_example_malloc(len, file, line);
    if (!dst)
    {
        return NULL;
    }

    memcpy(dst, src, len);
    return dst;
}

void simple_example_free(void *ptr, const char *file, int line)
{
    simple_mem_free(ptr, file, line);
}

const struct mtrans_mem_hooks *simple_example_mtrans_mem_hooks(void)
{
    static const struct mtrans_mem_hooks hooks =
    {
        simple_mem_malloc,
        simple_mem_realloc,
        simple_mem_free,
    };

    return &hooks;
}

const struct call_mem_hooks *simple_example_call_mem_hooks(void)
{
    static const struct call_mem_hooks hooks =
    {
        simple_mem_malloc,
        simple_mem_realloc,
        simple_mem_free,
    };

    return &hooks;
}

const struct transport_mem_hooks *simple_example_transport_mem_hooks(void)
{
    static const struct transport_mem_hooks hooks =
    {
        simple_mem_malloc,
        simple_mem_realloc,
        simple_mem_free,
    };

    return &hooks;
}
