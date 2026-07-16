#ifndef _DPDEF_H	// dpdef.h
#define _DPDEF_H

typedef unsigned char		BYTE;
typedef unsigned short		WORD;
typedef int					BOOL;
typedef unsigned int		SOCKET;
typedef void				*HANDLE;

#ifndef WIN32
typedef unsigned int		DWORD;
#else
typedef unsigned long		DWORD;
#endif

#define TRUE				1
#define FALSE				0

#ifndef NULL
#define NULL				0
#endif

#define INVALID_SOCKET		(SOCKET)(-1)
#define INVALID_HANDLE		(HANDLE)(-1)
#define INFINITE	         0xFFFFFFFF

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

/** General key value */
#define  VALUE_KEY_UP          0
#define  VALUE_KEY_DOWN        1
#define  VALUE_KEY_CLICK       2
#define  VALUE_KEY_LTCLICK     3
#define  VALUE_KEY_LLTCLICK    4
#define  VALUE_KEY_DBCLICK     5


#define DP_EOK                          0               /**< There is no error */
#define DP_ERROR                        -1               /**< A generic error happens */
#define DP_ETIMEOUT                     -2               /**< Timed out */
#define DP_EFULL                        -3               /**< The resource is full */
#define DP_EEMPTY                       -4               /**< The resource is empty */
#define DP_ENOMEM                       -5               /**< No memory */
#define DP_ENOSYS                       -6               /**< No system */
#define DP_EBUSY                        -7               /**< Busy */
#define DP_EIO                          -8               /**< IO error */

// // 线程
// #define DP_PRIO_LOW			50
// #define DP_PRIO_NORMAL		200
// #define DP_PRIO_HIGH		RT_THREAD_PRIORITY_MAX

#if defined _WIN32 || defined __CYGWIN__
  #define DPLAN_LIB_IMPORT __declspec(dllimport)
  #define DPLAN_LIB_EXPORT __declspec(dllexport)
  #define DPLAN_LIB_LOCAL
#else
  #if __GNUC__ >= 4
    #define DPLAN_LIB_IMPORT __attribute__ ((visibility("default")))
    #define DPLAN_LIB_EXPORT __attribute__ ((visibility("default")))
    #define DPLAN_LIB_LOCAL  __attribute__ ((visibility("hidden")))
  #else
    #define DPLAN_LIB_IMPORT
    #define DPLAN_LIB_EXPORT
    #define DPLAN_LIB_LOCAL
  #endif
#endif

#define DPLAN_PUBLIC_API  DPLAN_LIB_EXPORT
#define DPLAN_PRIVATE_API DPLAN_LIB_LOCAL

#endif	// dpdef.h

