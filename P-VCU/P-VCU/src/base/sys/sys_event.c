#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/sys_timer.h>
#include <sys/sys_event.h>

#include <utils/mq.h>
#include <utils/bmem.h>

#define	SYS_EVENT_MAX		(128)

typedef struct sys_event_mgnt
{
    sys_timer_t timer;
    mq_t *event_mq;
} sys_event_mgnt_t;

static sys_event_mgnt_t *sys_mgnt = NULL;

static void timer_callback(void *ptr)
{
    sys_event_mgnt_t *mgnt = (sys_event_mgnt_t *)ptr;

    sys_event_t e;

    SYS_EVENT_INIT_TIME(e);

    //printf("==== %d ====\n", time(NULL));
    mq_send(mgnt->event_mq, &e, sizeof(sys_event_t));
}

int sys_event_mgnt_init()
{
    if (sys_mgnt == NULL)
    {
        mtime_t interval = CLOCK_FREQ;

        sys_mgnt = bmalloc(sizeof(sys_event_mgnt_t));

        memset(sys_mgnt, 0, sizeof(sys_event_mgnt_t));

        sys_timer_create(&sys_mgnt->timer, timer_callback, sys_mgnt);

        sys_mgnt->event_mq = mq_create("sys_event_mq", sizeof(sys_event_t), SYS_EVENT_MAX);

        sys_timer_schedule(sys_mgnt->timer, false, interval, interval);
    }

    return 0;
}

void sys_event_mgnt_deinit()
{
    if (sys_mgnt)
    {
        sys_timer_destroy(sys_mgnt->timer);

        bfree(sys_mgnt);

        sys_mgnt = NULL;
    }
}

int sys_event_publish(sys_event_t *event)
{
    if (sys_mgnt)
    {
        return mq_send_wait(sys_mgnt->event_mq, event, sizeof(sys_event_t), MQ_MAX_TIMEOUT);
    }

    return -1;
}

int sys_event_dispatch(sys_event_t *event)
{
    if (sys_mgnt)
    {
        return mq_recv(sys_mgnt->event_mq, event, sizeof(sys_event_t), MQ_MAX_TIMEOUT);
    }

    return -1;
}