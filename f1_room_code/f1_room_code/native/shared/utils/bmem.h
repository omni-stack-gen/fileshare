#ifndef __BASE_MEM_H__
#define __BASE_MEM_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define DEBUG_MEMORY 1

#ifdef __cplusplus
extern "C"
{
#endif

#if DEBUG_MEMORY

void *bmem_dbg_malloc(const char *file, int line, size_t size);
void *bmem_dbg_realloc(const char *file, int line, void *mem, size_t size);
void bmem_dbg_free(const char *file, int line, void *mem);

#define bmalloc(size)                  bmem_dbg_malloc(__FILE__, __LINE__, (size))
#define brealloc(p, size)              bmem_dbg_realloc(__FILE__, __LINE__, (p), (size))
#define bfree(p)                       bmem_dbg_free(__FILE__, __LINE__, (p))
#define bzalloc(size)                  bmem_dbg_malloc(__FILE__, __LINE__, (size))

#else

void *bmem_malloc(size_t size);
void *bmem_realloc(void *mem, size_t size);
void bmem_free(void *mem);

#define bmalloc(size)                  bmem_malloc((size))
#define brealloc(p, size)              bmem_realloc((p), (size))
#define bfree(p)                       bmem_free((p))

static inline void *bzalloc(size_t size)
{
	void *mem = bmalloc(size);

	if (mem)
	{
		memset(mem, 0, size);
	}

	return mem;
}

#endif

#define bmalloc0(type, size)            \
	(type *) (__extension__ ({          \
		size_t __n = (size_t) (size);   \
		void *__p;                      \
		__p = bmalloc(__n);             \
		memset(__p, 0, __n );           \
		__p;                            \
	}))

void *bmemdup(const void *mem, size_t size);
char *bstrdup(const char *str);
char *bstrndup(const char *str, size_t len);

void bmem_debug_dump(int verbose);

#ifdef __cplusplus
}
#endif

#endif /* __BASE_MEM_H__ */