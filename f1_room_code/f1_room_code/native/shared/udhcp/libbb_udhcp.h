/* libbb_udhcp.h - busybox compatability wrapper */

#ifndef _LIBBB_UDHCP_H
#define _LIBBB_UDHCP_H

#ifdef BB_VER
#include "libbb.h"

#ifdef CONFIG_FEATURE_UDHCP_SYSLOG
#define SYSLOG
#endif

#ifdef CONFIG_FEATURE_UDHCP_DEBUG
#define DEBUG
#endif

#define COMBINED_BINARY
#define VERSION "0.9.8-pre"

#else /* ! BB_VER */

#define TRUE			1
#define FALSE			0

#define VERSION "0.9.8-pre"

#endif /* BB_VER */

unsigned long long monotonic_ns(void);
unsigned long long monotonic_us(void);
unsigned long long monotonic_ms(void);
unsigned monotonic_sec(void);

#endif /* _LIBBB_UDHCP_H */
