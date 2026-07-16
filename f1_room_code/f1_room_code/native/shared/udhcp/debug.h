#ifndef _DEBUG_H
#define _DEBUG_H

#include "libbb_udhcp.h"

#include <stdio.h>
#ifdef SYSLOG
#include <syslog.h>
#endif

#include <utils/bmem.h>

#define LOG_TAG "udhcp"
#include <utils/log.h>

// #define DEBUG

#if 0
#ifdef SYSLOG
# define LOG(level, str, args...) do { printf(str, ## args); \
				printf("\n"); \
				syslog(level, str, ## args); } while(0)
# define OPEN_LOG(name) openlog(name, 0, 0)
#define CLOSE_LOG() closelog()
#else
# define LOG_EMERG	"EMERGENCY!"
# define LOG_ALERT	"ALERT!"
# define LOG_CRIT	"critical!"
# define LOG_WARNING	"warning"
# define LOG_ERR	"error"
# define LOG_INFO	"info"
# define LOG_DEBUG	"debug"
# define LOG(level, str, args...) do { printf("%s, ", level); \
				printf(str, ## args); \
				printf("\n"); } while(0)
# define OPEN_LOG(name) do {;} while(0)
#define CLOSE_LOG() do {;} while(0)
#endif
#else

#define LOG_CRIT  		1
#define LOG_ERR  		2
#define LOG_WARNING   	3
#define LOG_INFO   		4
#define LOG_DEBUG  		5

#endif

#ifdef DEBUG
# undef DEBUG
# define DEBUG(level, str, args...) log_output(level, LOG_TAG, __FILE__, __LINE__, __func__, str, ## args)
# define DEBUGGING
#else
# define DEBUG(level, str, args...) do {;} while(0)
#endif

#endif
