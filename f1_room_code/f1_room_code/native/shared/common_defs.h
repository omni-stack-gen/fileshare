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

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define unreachable() __builtin_unreachable()

#define os_assert_unreachable() (assert(!"unreachable"), unreachable())

/*****************************************************************************
 * Error values (shouldn't be exposed)
 *****************************************************************************/
#define OS_SUCCESS        (-0) /**< No error */
#define OS_EGENERIC       (-1) /**< Unspecified error */
#define OS_ENOMEM         (-2) /**< Not enough memory */
#define OS_ETIMEOUT       (-3) /**< Timeout */
#define OS_ENOMOD         (-4) /**< Module not found */
#define OS_ENOOBJ         (-5) /**< Object not found */
#define OS_ENOVAR         (-6) /**< Variable not found */
#define OS_EBADVAR        (-7) /**< Bad variable value */
#define OS_ENOITEM        (-8) /**< Item not found */

/*****************************************************************************
 * Macros and inline functions
 *****************************************************************************/

/* __MAX and __MIN: self explanatory */
#ifndef __MAX
	#define __MAX(a, b)   (((a) > (b)) ? (a) : (b))
#endif
#ifndef __MIN
	#define __MIN(a, b)   (((a) < (b)) ? (a) : (b))
#endif

/* clip v in [min, max] */
#define OS_CLIP(v, min, max)    __MIN(__MAX((v), (min)), (max))

#endif /* __COMMON_DEFS_H__ */