#ifndef __WATCHDOG_MGNT_H__
#define __WATCHDOG_MGNT_H__

#include <common_defs.h>

typedef enum watchdog_type
{
	WDT_TYPE_SOFT,
	WDT_TYPE_HARD,
} watchdog_type_t;

typedef struct watchdog_ops
{
    // 喂狗的间隔时长
    int internal_ms;

	void *(*open)();
	int (*feed)(void *handle);
	int (*close)(void *handle);
} watchdog_ops_t;

int watchdog_mgnt_init(watchdog_type_t type);

void watchdog_mgnt_deinit();

#endif /* __WATCHDOG_MGNT_H__ */