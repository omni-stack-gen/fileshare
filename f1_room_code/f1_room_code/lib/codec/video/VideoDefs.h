#ifndef __VIDEO_DEFS_H__
#define __VIDEO_DEFS_H__

#if __GNUC__ >= 4
	#define VIDEO_LIB_IMPORT __attribute__ ((visibility("default")))
	#define VIDEO_LIB_EXPORT __attribute__ ((visibility("default")))
	#define VIDEO_LIB_LOCAL  __attribute__ ((visibility("hidden")))
#else
	#define VIDEO_LIB_IMPORT
	#define VIDEO_LIB_EXPORT
	#define VIDEO_LIB_LOCAL
#endif

#define VIDEO_PUBLIC_API  VIDEO_LIB_EXPORT
#define VIDEO_PRIVATE_API VIDEO_LIB_LOCAL

#endif //!__VIDEO_DEFS_H__