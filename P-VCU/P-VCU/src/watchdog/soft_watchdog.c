#include <watchdog/watchdog_mgnt.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>

#include <linux/watchdog.h>

#include <utils/bmem.h>

#define LOG_TAG "soft_watchdog"
#include <utils/log.h>

#define WATCHDOG_DEV "/dev/watchdog0"
#define WATCHDOG_TIMEOUT (10) //秒

typedef struct soft_watchdog_ctx
{
	int fd;
	struct watchdog_info wi;
} soft_watchdog_ctx_t;

static int watchdog_disable(soft_watchdog_ctx_t *ctx)
{
	int opt = WDIOS_DISABLECARD;

	if (!ctx)
	{
		return -1;
	}

	if (ctx->fd <= 0)
	{
		return -1;
	}

	return ioctl(ctx->fd, WDIOC_SETOPTIONS, &opt);
}

static int watchdog_enable(soft_watchdog_ctx_t *ctx)
{
	int opt = WDIOS_ENABLECARD;

	if (!ctx)
	{
		return -1;
	}

	if (ctx->fd <= 0)
	{
		return -1;
	}

	return ioctl(ctx->fd, WDIOC_SETOPTIONS, &opt);
}

static int watchdog_set_timeout(soft_watchdog_ctx_t *ctx, int timeout)
{
	if (!ctx)
	{
		return -1;
	}

	if (ctx->fd <= 0)
	{
		return -1;
	}

	return ioctl(ctx->fd, WDIOC_SETTIMEOUT, &timeout);
}

static int watchdog_feed(soft_watchdog_ctx_t *ctx)
{
	int feeddogvalue = 0;

	if (!ctx)
	{
		return -1;
	}

	if (ctx->fd <= 0)
	{
		return -1;
	}

	return ioctl(ctx->fd, WDIOC_KEEPALIVE, &feeddogvalue);
}

static void *soft_watchdog_open()
{
	soft_watchdog_ctx_t *ctx = NULL;

	ctx = bmalloc(sizeof(soft_watchdog_ctx_t));

	if (ctx == NULL)
	{
		return NULL;
	}

	ctx->fd = open(WATCHDOG_DEV, O_RDWR | O_NONBLOCK | O_CLOEXEC);

	if (ctx->fd < 0)
	{
		LOGE("cannot open the watchdog device.\n");
		goto __exit;
	}

	ioctl(ctx->fd, WDIOC_GETSUPPORT, &ctx->wi);

	LOGI("watchdog info: option:%x, firmware_version:%x identity:%s\n",
	     ctx->wi.options, ctx->wi.firmware_version, ctx->wi.identity);

	watchdog_disable(ctx);
	watchdog_set_timeout(ctx, WATCHDOG_TIMEOUT);
	watchdog_enable(ctx);

	return ctx;

__exit:

	if (ctx)
	{
		if (ctx->fd >= 0)
		{
			close(ctx->fd);

			ctx->fd = -1;
		}

		bfree(ctx);
	}

	return NULL;
}

static int soft_watchdog_feed(void *handle)
{
	soft_watchdog_ctx_t *ctx = (soft_watchdog_ctx_t *)handle;

	if (ctx == NULL)
	{
		return -1;
	}

    // LOGI("software watchdog feed \n");

	return watchdog_feed(ctx);
}

static int soft_watchdog_close(void *handle)
{
	soft_watchdog_ctx_t *ctx = (soft_watchdog_ctx_t *)handle;

	if (ctx == NULL)
	{
		return -1;
	}

	if (ctx->fd >= 0)
	{
		watchdog_disable(ctx);

		close(ctx->fd);
	}

	bfree(ctx);

	ctx = NULL;

	return 0;
}

watchdog_ops_t soft_wdt_ops =
{
	.internal_ms = 1000,
	.open = soft_watchdog_open,
	.feed = soft_watchdog_feed,
	.close = soft_watchdog_close
};