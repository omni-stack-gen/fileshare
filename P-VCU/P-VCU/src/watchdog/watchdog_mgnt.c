#include <watchdog/watchdog_mgnt.h>

#include <unistd.h>

#include <utils/bmem.h>
// #include <utils/thread_helper.h>
#include <utils/time_helper.h>

#define LOG_TAG "watch_mgnt"
#include <utils/log.h>

extern watchdog_ops_t soft_wdt_ops;
extern watchdog_ops_t hard_wdt_ops;

typedef struct watchdog_mgnt
{
    watchdog_type_t type;

    uint64_t process_stamp;

    watchdog_ops_t *ops;
    void *ctx;

    volatile bool quit;
    pthread_t thread;
} watchdog_mgnt_t;

static watchdog_mgnt_t *wdt_mgnt = NULL;

static watchdog_ops_t *wdt_ops[] =
{
    &soft_wdt_ops,
    &hard_wdt_ops,
    NULL
};

static const char *wdt_type_to_string(watchdog_type_t type)
{
	switch (type)
	{
		case WDT_TYPE_SOFT:
			return "software watchdog";

		case WDT_TYPE_HARD:
			return "hardware watchdog";
	}

	return "(unknown)";
}

static void *watchdog_feed_thread(void *args)
{
    LOGI("watchdog_feed_thread start \n");

    uint64_t tp_now = 0;

    watchdog_mgnt_t *mgnt = (watchdog_mgnt_t *)args;

    wdt_mgnt->ctx = wdt_mgnt->ops->open();

    while (!mgnt->quit)
    {
        tp_now = time_now();

        if (mgnt->ops->feed != NULL && tp_now - mgnt->process_stamp >= mgnt->ops->internal_ms)
        {
            mgnt->process_stamp = tp_now;

            mgnt->ops->feed(mgnt->ctx);
        }

        usleep(50*1000);
    }

    wdt_mgnt->ops->close(wdt_mgnt->ctx);

    LOGI("watchdog_feed_thread stop \n");

    return NULL;
}

int watchdog_mgnt_init(watchdog_type_t type)
{
    int ret = 0;

    if (wdt_mgnt == NULL)
    {
        wdt_mgnt = bmalloc(sizeof(watchdog_mgnt_t));

        memset(wdt_mgnt, 0, sizeof(watchdog_mgnt_t));

        wdt_mgnt->type = type;
        wdt_mgnt->ops = wdt_ops[type];

        LOGI("%s init\n", wdt_type_to_string(type));

        ret = pthread_create(&wdt_mgnt->thread, NULL, watchdog_feed_thread, wdt_mgnt);
    }

    return ret;
}

void watchdog_mgnt_deinit()
{
    if (wdt_mgnt)
    {
        wdt_mgnt->quit = true;

        if (wdt_mgnt->thread)
        {
            pthread_join(wdt_mgnt->thread, NULL);
            wdt_mgnt->thread = 0;
        }

        bfree(wdt_mgnt);

        wdt_mgnt = NULL;
    }
}