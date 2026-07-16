#ifndef __RESIZE_BUF_H__
#define __RESIZE_BUF_H__

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <utils/bmem.h>

typedef struct resize_buf
{
	uint8_t *buf;
	size_t size;
	size_t capacity;
} resize_buf_t;

static inline void resize_buf_init(resize_buf_t *rb, size_t init_capacity)
{
	rb->size = 0;
	rb->capacity = init_capacity;

	if (init_capacity > 0)
	{
		rb->buf = (uint8_t *)bmalloc(init_capacity);
	}
	else
	{
		rb->buf = NULL;
	}
}

static inline void resize_buf_resize(resize_buf_t *rb, size_t size)
{
	if (!rb->buf)
	{
		rb->buf = (uint8_t *)bmalloc(size);
		rb->capacity = size;
	}
	else if (rb->capacity < size)
	{
		size_t capx2 = rb->capacity * 2;
		size_t new_cap = capx2 > size ? capx2 : size;
		rb->buf = (uint8_t *)brealloc(rb->buf, new_cap);
		rb->capacity = new_cap;
	}

	rb->size = size;
}

static inline void resize_buf_copy(resize_buf_t *rb, const uint8_t *data, size_t len)
{
	if (len == 0 || !data)
	{
		rb->size = 0; // 置空
		return ;
	}

	resize_buf_resize(rb, len);
	memcpy(rb->buf, data, len);
}

// 辅助函数：追加数据
static inline void resize_buf_append(resize_buf_t *rb, const uint8_t *data, size_t len)
{
	if (len == 0 || !data)
	{
		return;
	}

	size_t old_size = rb->size;
	resize_buf_resize(rb, old_size + len);
	memcpy(rb->buf + old_size, data, len);
}

static inline void resize_buf_free(resize_buf_t *rb)
{
	if (rb->buf)
	{
		bfree(rb->buf);
		rb->buf = NULL;
	}

	rb->size = 0;
	rb->capacity = 0;
}

#endif /* __RESIZE_BUF_H__ */