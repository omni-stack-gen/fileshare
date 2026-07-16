#include "util.h"

bool is_power_of_two(unsigned int x)
{
	return IS_POWER_OF_TWO(x);
}

void byteswp(void *a, void *b, size_t size)
{
	uint8_t t;
	uint8_t *aa = (uint8_t *)a;
	uint8_t *bb = (uint8_t *)b;

	for (; size > 0; --size)
	{
		t = *aa;
		*aa++ = *bb;
		*bb++ = t;
	}
}

size_t safe_strncpy(char *dst, const char *src, size_t dst_len)
{
	size_t src_len = src ? strlen(src) : 0;

	if (!src)
	{
		if (dst_len)
		{
			dst[0] = '\0';
		}
		goto done;
	}

	if (dst_len)
	{
		if (src_len < dst_len)
		{
			dst_len = src_len + 1;
		}
		else
		{
			dst_len -= 1;
			dst[dst_len] = '\0';
		}

		memcpy(dst, src, dst_len);
	}

done:
	return src_len;
}

size_t safe_memcpy(void *dst, const void *src, size_t count, size_t dst_len)
{
	if (dst == NULL || src == NULL || dst_len == 0)
	{
		return 0;
	}

	size_t copy_size = (count < dst_len) ? count : dst_len;

	memcpy(dst, src, copy_size);
	return copy_size;
}
