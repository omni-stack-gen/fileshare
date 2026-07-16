#include <watchdog/watchdog_mgnt.h>

#include <periphery/gpio.h>

#include <utils/time_helper.h>
// #include <utils/thread_helper.h>
#include <utils/bmem.h>

#define LOG_TAG "hard_watchdog"
#include <utils/log.h>

#define WATCHDOG_ENABLE     (0)

#define WATCHDOG_EN_PIN     (70)
#define WATCHDOG_FEED_PIN   (68)

typedef struct wdt_gpio_state
{
    uint64_t start_time;    // 当前状态的启动时间
    uint32_t last_state;	// 上一次的状态
} wdt_gpio_state_t;

typedef struct hard_watchdog_ctx
{
    gpio_t *wdt_enable;
    wdt_gpio_state_t wdt_enable_state;

    gpio_t *wdt_feed;
    wdt_gpio_state_t wdt_feed_state;
} hard_watchdog_ctx_t;

static void gpio_watchdog_enable_check(hard_watchdog_ctx_t *ctx)
{
    if (ctx->wdt_enable_state.last_state == 1 - WATCHDOG_ENABLE)
    {
        uint64_t tp_now = time_now();

        if (tp_now - ctx->wdt_enable_state.start_time >= 200)
        {
            // LOGI("hardware watchdog enable now \n");

            gpio_write(ctx->wdt_enable, WATCHDOG_ENABLE);

            ctx->wdt_enable_state.last_state = WATCHDOG_ENABLE;
            ctx->wdt_enable_state.start_time = tp_now;
        }
    }
}

static void gpio_watchdog_feed(hard_watchdog_ctx_t *ctx)
{
    uint64_t tp_now = time_now();

    if (tp_now - ctx->wdt_feed_state.start_time >= 100)
    {
        // LOGI("hardware watchdog feed \n");

        gpio_write(ctx->wdt_feed, 1 - ctx->wdt_feed_state.last_state);

        ctx->wdt_feed_state.last_state = 1 - ctx->wdt_feed_state.last_state;
        ctx->wdt_feed_state.start_time = tp_now;
    }
}

static void *hard_watchdog_open()
{
    hard_watchdog_ctx_t *ctx = NULL;

    gpio_config_t config;

    ctx = bmalloc(sizeof(hard_watchdog_ctx_t));

    if (ctx == NULL)
    {
        return NULL;
    }

    memset(ctx, 0, sizeof(hard_watchdog_ctx_t));

    memset(&config, 0, sizeof(config));

    config.direction = GPIO_DIR_OUT;
    config.edge = GPIO_EDGE_NONE;
    config.reversed = false;

    ctx->wdt_feed = gpio_create();
    gpio_open(ctx->wdt_feed, WATCHDOG_FEED_PIN, &config);
    gpio_write(ctx->wdt_feed, 1);

    ctx->wdt_feed_state.last_state = 1;
    ctx->wdt_feed_state.start_time = time_now();

    ctx->wdt_enable = gpio_create();
    gpio_open(ctx->wdt_enable, WATCHDOG_EN_PIN, &config);
    gpio_write(ctx->wdt_enable, 1 - WATCHDOG_ENABLE);

    ctx->wdt_enable_state.last_state = 1 - WATCHDOG_ENABLE;
    ctx->wdt_enable_state.start_time = time_now();

    return ctx;
}

static int hard_watchdog_feed(void *handle)
{
	int ret = -1;

	hard_watchdog_ctx_t *ctx = (hard_watchdog_ctx_t *)handle;

	if (ctx == NULL)
	{
		return -1;
	}

    gpio_watchdog_enable_check(ctx);

    gpio_watchdog_feed(ctx);

    ret = 0;
    return ret;
}

static int hard_watchdog_close(void *handle)
{
	hard_watchdog_ctx_t *ctx = (hard_watchdog_ctx_t *)handle;

	if (ctx == NULL)
	{
		return -1;
	}

    if (ctx->wdt_enable)
    {
        gpio_write(ctx->wdt_enable, 1 - WATCHDOG_ENABLE);
        gpio_destroy(ctx->wdt_enable);
    }

    if (ctx->wdt_feed)
    {
        gpio_destroy(ctx->wdt_feed);
    }

    bfree(ctx);

    ctx = NULL;

    return 0;
}

watchdog_ops_t hard_wdt_ops =
{
    .internal_ms = 50,
    .open = hard_watchdog_open,
    .feed = hard_watchdog_feed,
    .close = hard_watchdog_close
};