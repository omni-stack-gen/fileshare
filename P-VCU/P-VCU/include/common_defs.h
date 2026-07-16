#ifndef __COMMON_DEFS_H__
#define __COMMON_DEFS_H__

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
#include <errno.h>
#include <fcntl.h>

#include <pthread.h>
#include <semaphore.h>

#ifdef __cplusplus
	#define BEGIN_C_DECLS extern "C" {
	#define END_C_DECLS }
#else
	#define BEGIN_C_DECLS
	#define END_C_DECLS
#endif

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define unreachable() __builtin_unreachable()

#define UNUSED_PARAMETER(param) (void)param

#define PRAGMA_WARN_PUSH _Pragma("GCC diagnostic push")
#define PRAGMA_WARN_POP _Pragma("GCC diagnostic pop")
#define PRAGMA_WARN_DEPRECATION \
	_Pragma("GCC diagnostic warning \"-Wdeprecated-declarations\"")

#define THREAD_LOCAL __thread

#define OS_FORMAT(x,y) __attribute__ ((format(printf,x,y)))
#define OS_FORMAT_ARG(x) __attribute__ ((format_arg(x)))
#define OS_MALLOC __attribute__ ((malloc))
#define OS_USED __attribute__ ((warn_unused_result))
#define OS_DEPRECATED __attribute__((deprecated))

// #define likely(x)   __builtin_expect(!!(x), 1)
// #define unlikely(x) __builtin_expect(!!(x), 0)
// #define unreachable() __builtin_unreachable()

#define os_assert_unreachable() (assert(!"unreachable"), unreachable())

#endif /* __COMMON_DEFS_H__ */