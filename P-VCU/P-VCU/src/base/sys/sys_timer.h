#ifndef __SYS_TIMER_H__
#define __SYS_TIMER_H__

#include <common_defs.h>

#define CLOCK_FREQ INT64_C(1000000)

typedef int64_t mtime_t;

mtime_t mdate(void);

typedef struct sys_timer *sys_timer_t;

int sys_timer_create(sys_timer_t *id, void (*func)(void *), void *data);
void sys_timer_destroy(sys_timer_t timer);
void sys_timer_schedule(sys_timer_t timer, bool absolute, mtime_t value, mtime_t interval);
unsigned sys_timer_getoverrun(sys_timer_t timer);

#endif /* __SYS_TIMER_H__ */