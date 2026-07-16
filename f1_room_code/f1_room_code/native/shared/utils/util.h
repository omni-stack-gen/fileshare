#ifndef __UTIL_H__
#define __UTIL_H__

#include <common_defs.h>

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define unreachable() __builtin_unreachable()

#define ENUM_CASE_2STR(x)   case x: return (#x)

#define _STRINGIFY(x)   #x
#define STRINGIFY(s)    _STRINGIFY(s)

#define POINTER_TO_UINT(x) ((uintptr_t) (x))

#define UINT_TO_POINTER(x) ((void *) (uintptr_t) (x))

#define POINTER_TO_INT(x)  ((intptr_t) (x))

#define INT_TO_POINTER(x)  ((void *) (intptr_t) (x))

#define ALIGN_UP(size, align)           (((size) + (align) - 1) & ~((align) - 1))

#define ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

/* conditional checks */

#define CHECK_VAL_RETURN_VAL_IF_FAIL(expr, val, fmt, ...)                       \
	do                                                                          \
	{                                                                           \
		if (!(expr))                                                            \
		{                                                                       \
			LOGE(fmt, ##__VA_ARGS__);                                           \
			return val;                                                         \
		}                                                                       \
	} while (0)

#define CHECK_VAL_RETURN_IF_FAIL(expr, fmt, ...)                                \
	do                                                                          \
	{                                                                           \
		if (!(expr))                                                            \
		{                                                                       \
			LOGE(fmt, ##__VA_ARGS__);                                           \
			return ;                                                            \
		}                                                                       \
	} while (0)

#define CHECK_VAL_GOTO_IF_FAIL(expr, fmt, ...)                                  \
	do                                                                          \
	{                                                                           \
		if (!(expr))                                                            \
		{                                                                       \
			LOGE(fmt, ##__VA_ARGS__);                                           \
			goto fail;                                                          \
		}                                                                       \
	} while (0)

#define CHECK_HANDLE_RETURN_VAL_IF_FAIL(expr, val)                              \
	do                                                                          \
	{                                                                           \
		if (!(expr))                                                            \
		{                                                                       \
			LOGE("Null '%s' parameter", #expr);                                 \
			return val;                                                         \
		}                                                                       \
	} while (0)

#define CHECK_HANDLE_RETURN_IF_FAIL(expr)                                       \
	do                                                                          \
	{                                                                           \
		if (!(expr))                                                            \
		{                                                                       \
			LOGE("Null '%s' parameter", #expr);                                 \
			return ;                                                            \
		}                                                                       \
	} while (0)

/* bit operations macros */

#define BIT(n)  (1UL << (n))

#define NUM_BITS(t) (sizeof(t) * 8)

#define MASK(b_lo, b_hi)    \
	(((1ULL << ((b_hi) - (b_lo) + 1ULL)) - 1ULL) << (b_lo))

#define WRITE_BIT(var, bit, set) \
	((var) = (set) ? ((var) | BIT(bit)) : ((var) & ~BIT(bit)))

#if 0
	#define SET_BIT(b, x)       ((1UL << (b)) | (x))
#else
	#define SET_BIT(b, x)       (((x) & 1) << (b))
#endif

#define SET_BITS(b_lo, b_hi, x) \
	(((x) & ((1ULL << ((b_hi) - (b_lo) + 1ULL)) - 1ULL)) << (b_lo))

#define GET_BIT(b, x) \
	(((x) & (1ULL << (b))) >> (b))

#define GET_BITS(b_lo, b_hi, x) \
	(((x) & MASK(b_lo, b_hi)) >> (b_lo))

#ifndef ARRAY_SIZE
	#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, field) \
	((type *)(((char *)(ptr)) - offsetof(type, field)))
#endif

#ifndef ROUND_UP
#define ROUND_UP(x, align)                                 \
	(((unsigned long)(x) + ((unsigned long)(align) - 1)) & \
	 ~((unsigned long)(align) - 1))
#endif

#ifndef ROUND_DOWN
#define ROUND_DOWN(x, align)                                 \
	((unsigned long)(x) & ~((unsigned long)(align) - 1))
#endif

#if 0
#ifndef MAX
	#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
	#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef CLAMP
	#define CLAMP(val, low, high) (((val) <= (low)) ? (low) : MIN(val, high))
#endif
#else

#ifndef MAX
#define MAX(a, b) ({ \
		/* random suffix to avoid naming conflict */ \
		__typeof__(a) _value_a_ = (a); \
		__typeof__(b) _value_b_ = (b); \
		_value_a_ > _value_b_ ? _value_a_ : _value_b_; \
	})
#endif

#ifndef MIN
#define MIN(a, b) ({ \
		/* random suffix to avoid naming conflict */ \
		__typeof__(a) _value_a_ = (a); \
		__typeof__(b) _value_b_ = (b); \
		_value_a_ < _value_b_ ? _value_a_ : _value_b_; \
	})
#endif

#ifndef CLAMP
#define CLAMP(val, low, high) ({                                       \
		/* random suffix to avoid naming conflict */                   \
		__typeof__(val) _value_val_ = (val);                           \
		__typeof__(low) _value_low_ = (low);                           \
		__typeof__(high) _value_high_ = (high);                        \
		(_value_val_ < _value_low_)  ? _value_low_ :                   \
		(_value_val_ > _value_high_) ? _value_high_ :                  \
		_value_val_;                                \
	})
#endif
#endif

#ifndef IN_RANGE
#define IN_RANGE(val, min, max)                                        \
	((val) >= (min) && (val) <= (max))
#endif

#define IS_POWER_OF_TWO(x) (((x) != 0U) && (((x) & ((x) - 1U)) == 0U))

bool is_power_of_two(unsigned int x);

void byteswp(void *a, void *b, size_t size);

int char2hex(char c, uint8_t *x);

int hex2char(uint8_t x, char *c);

size_t bin2hex(const uint8_t *buf, size_t buflen, char *hex, size_t hexlen);

size_t hex2bin(const char *hex, size_t hexlen, uint8_t *buf, size_t buflen);

uint8_t bcd2bin(uint8_t bcd);

uint8_t bin2bcd(uint8_t bin);

#define TFR(expression)                            \
	(__extension__                                 \
	({ long int __result;                          \
		do __result = (long int) (expression);     \
		while (__result == -1L && errno == EINTR); \
		__result; }))

size_t safe_strncpy(char *dst, const char *src, size_t dst_len);

size_t safe_memcpy(void *dst, const void *src, size_t count, size_t dst_len);

#define MAX_SIZE_OF_UUID_STR (36)

int uuid_str_generate(char *dest, size_t dest_size);

#define IS_VALID_STRING(str) \
    ((str) != NULL && (str)[0] != '\0')

#define SAFE_STRING(str) \
    ((str) != NULL ? (str) : "")

#define IS_VALID_STRING_LEN(str) \
    ((str) != NULL && strlen(str) > 0)

#define SAFE_STRING_CHECK(str) \
    ({ \
        const char *_s = (str); \
        _s != NULL && _s[0] != '\0'; \
    })

#endif /* __UTIL_H__ */
