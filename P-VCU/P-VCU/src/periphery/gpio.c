#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/epoll.h>

#include <periphery/gpio.h>
#include <utils/bmem.h>

#define LOG_TAG "gpio"
#include <utils/log.h>

#define MAX_PATH_SIZE   (256)

#define GPIO_EXPORT     "/sys/class/gpio/export"
#define GPIO_UNEXPORT   "/sys/class/gpio/unexport"
#define GPIO_DIRECTION  "/sys/class/gpio/gpio%d/direction"
#define GPIO_VALUE      "/sys/class/gpio/gpio%d/value"
#define GPIO_EDGE       "/sys/class/gpio/gpio%d/edge"

struct gpio
{
	int fd;
	uint32_t gpio_id;
};

gpio_t *gpio_create()
{
	gpio_t *gpio = (gpio_t *)bmalloc(sizeof(gpio_t));

	if (!gpio)
	{
		return NULL;
	}

	gpio->gpio_id = -1;
	gpio->fd = -1;

	return gpio;
}

static gpio_error_t gpio_set_direction(gpio_t *gpio, gpio_direction_t direction)
{
	char gpio_path[MAX_PATH_SIZE];
	const char *buf = NULL;
	int fd;

	if (direction == GPIO_DIR_IN)
	{
		buf = "in\n";
	}
	else if (direction == GPIO_DIR_OUT)
	{
		buf = "out\n";
	}
	else
	{
		LOGE("invalid direction to set gpio%d \n", gpio->gpio_id);
		return GPIO_ERROR_PARAM;
	}

	snprintf(gpio_path, sizeof(gpio_path), GPIO_DIRECTION, gpio->gpio_id);

	if ((fd = open(gpio_path, O_WRONLY | O_CLOEXEC)) < 0)
	{
		LOGE("open gpio%d direction error \n", gpio->gpio_id);
		return GPIO_ERROR_CONFIGURE;
	}

	if (write(fd, buf, strlen(buf)) < 0)
	{
		close(fd);
		LOGE("write gpio%d direction error \n", gpio->gpio_id);
		return GPIO_ERROR_CONFIGURE;
	}

	if (close(fd) < 0)
	{
		LOGE("close gpio%d direction error \n", gpio->gpio_id);
		return GPIO_ERROR_CONFIGURE;
	}

	return GPIO_ERROR_NONE;
}

static gpio_error_t gpio_set_edge(gpio_t *gpio, gpio_edge_t edge)
{
	char gpio_path[MAX_PATH_SIZE];
	const char *buf = NULL;
	int fd;

	if (edge == GPIO_EDGE_NONE)
	{
		buf = "none\n";
	}
	else if (edge == GPIO_EDGE_RISING)
	{
		buf = "rising\n";
	}
	else if (edge == GPIO_EDGE_FALLING)
	{
		buf = "falling\n";
	}
	else if (edge == GPIO_EDGE_BOTH)
	{
		buf = "both\n";
	}
	else
	{
		LOGE("invalid edge to set gpio%d \n", gpio->gpio_id);
		return GPIO_ERROR_PARAM;
	}

	snprintf(gpio_path, sizeof(gpio_path), "/sys/class/gpio/gpio%u/edge", gpio->gpio_id);

	if ((fd = open(gpio_path, O_WRONLY)) < 0)
	{
		LOGE("open gpio%d edge error \n", gpio->gpio_id);
		return GPIO_ERROR_CONFIGURE;
	}

	if (write(fd, buf, strlen(buf)) < 0)
	{
		close(fd);
		LOGE("write gpio%d edge error \n", gpio->gpio_id);
		return GPIO_ERROR_CONFIGURE;
	}

	if (close(fd) < 0)
	{
		LOGE("close gpio%d edge error \n", gpio->gpio_id);
		return GPIO_ERROR_CONFIGURE;
	}

	return GPIO_ERROR_NONE;
}

static gpio_error_t gpio_set_reversed(gpio_t *gpio, bool reversed)
{
	char gpio_path[MAX_PATH_SIZE];
	const char *buf = NULL;
	int fd;

	if (reversed)
	{
		buf = "1\n";
	}
	else
	{
		buf = "0\n";
	}

	snprintf(gpio_path, sizeof(gpio_path), "/sys/class/gpio/gpio%u/active_low", gpio->gpio_id);

	if ((fd = open(gpio_path, O_WRONLY)) < 0)
	{
		LOGE("open gpio%d active_low error \n", gpio->gpio_id);
		return GPIO_ERROR_CONFIGURE;
	}

	if (write(fd, buf, strlen(buf)) < 0)
	{
		close(fd);
		LOGE("write gpio%d active_low error \n", gpio->gpio_id);
		return GPIO_ERROR_CONFIGURE;
	}

	if (close(fd) < 0)
	{
		LOGE("close gpio%d active_low error \n", gpio->gpio_id);
		return GPIO_ERROR_CONFIGURE;
	}

	return GPIO_ERROR_NONE;
}

gpio_error_t gpio_open(gpio_t *gpio, uint32_t gpio_id, gpio_config_t *config)
{
	if (!gpio || !config)
	{
		return GPIO_ERROR_PARAM;
	}

	int fd = -1, len = -1;

	char gpio_path[MAX_PATH_SIZE];

	struct stat stat_buf;

	snprintf(gpio_path, sizeof(gpio_path), "/sys/class/gpio/gpio%d", gpio_id);

	if (stat(gpio_path, &stat_buf) < 0)
	{
		len = snprintf(gpio_path, sizeof(gpio_path), "%d", gpio_id);

		fd = open(GPIO_EXPORT, O_WRONLY);

		if (fd < 0)
		{
			LOGE("gpio export open %d error \n", gpio_id);
			return GPIO_ERROR_OPEN;
		}

		if (write(fd, gpio_path, len) < 0)
		{
			LOGE("gpio export write %d error \n", gpio_id);
			close(fd);
			return GPIO_ERROR_OPEN;
		}

		if (close(fd) < 0)
		{
			fd = -1;
			LOGE("gpio export close %d error \n", gpio_id);
			return GPIO_ERROR_OPEN;
		}
	}

	snprintf(gpio_path, sizeof(gpio_path), GPIO_VALUE, gpio_id);

	fd = open(gpio_path, O_RDWR);

	// LOGI("open %s \n", gpio_path);

	if (fd < 0)
	{
		LOGE("gpio open %d error errno:%d:%s \n", gpio_id, errno, strerror(errno));
		return GPIO_ERROR_OPEN;
	}

	gpio->fd = fd;
	gpio->gpio_id = gpio_id;

	gpio_error_t ret = GPIO_ERROR_NONE;

	if ((ret = gpio_set_direction(gpio, config->direction)) < 0)
	{
		return ret;
	}

	if (config->direction == GPIO_DIR_IN && config->edge != GPIO_EDGE_NONE)
	{
		if ((ret = gpio_set_edge(gpio, config->edge)) < 0)
		{
			return ret;
		}
	}

	if (config->direction == GPIO_DIR_OUT)
	{
		if ((ret = gpio_set_reversed(gpio, config->reversed)) < 0)
		{
			return ret;
		}
	}

	return GPIO_ERROR_NONE;
}

gpio_error_t gpio_read(gpio_t *gpio, gpio_state_t *state)
{
	char buf[2];

	if (read(gpio->fd, buf, 2) < 0)
	{
		LOGE("read gpio%d value error \n", gpio->gpio_id);
		return GPIO_ERROR_IO;
	}

	if (lseek(gpio->fd, 0, SEEK_SET) < 0)
	{
		LOGE("lseek gpio%d error \n", gpio->gpio_id);
		return GPIO_ERROR_IO;
	}

	if (buf[0] != '0')
	{
		*state = GPIO_HIGH;
	}
	else
	{
		*state = GPIO_LOW;
	}

	return GPIO_ERROR_NONE;
}

gpio_error_t gpio_write(gpio_t *gpio, gpio_state_t state)
{
	char str[][2] = {"0\n", "1\n"};

	int ret = 0;

	if (state)
	{
		ret = write(gpio->fd, str[1], 2);
	}
	else
	{
		ret = write(gpio->fd, str[0], 2);
	}

	if (ret < 0)
	{
		LOGE("write gpio%d value error \n", gpio->gpio_id);
		return GPIO_ERROR_IO;
	}

	if (lseek(gpio->fd, 0, SEEK_SET) < 0)
	{
		LOGE("lseek gpio%d error \n", gpio->gpio_id);
		return GPIO_ERROR_IO;
	}

	return GPIO_ERROR_NONE;
}

gpio_error_t gpio_poll(gpio_t *gpio, unsigned long milliseconds)
{
	struct pollfd fds[1];
	int ret;

	fds[0].fd = gpio->fd;
	fds[0].events = POLLPRI | POLLERR;

	if ((ret = poll(fds, 1, milliseconds)) < 0)
	{
		LOGE("poll gpio%d error \n", gpio->gpio_id);
		return GPIO_ERROR_IO;
	}

	if (ret)
	{
		if (fds[0].revents & POLLERR)
		{
			LOGE("poll gpio%d error \n", gpio->gpio_id);
			return GPIO_ERROR_IO;
		}

		if (lseek(gpio->fd, 0, SEEK_SET) < 0)
		{
			LOGE("lseek gpio%d error \n", gpio->gpio_id);
			return GPIO_ERROR_IO;
		}

		return GPIO_ERROR_NONE;
	}

	return GPIO_ERROR_POLL_TIMEOUT;
}

gpio_error_t gpio_close(gpio_t *gpio)
{
	int fd, len;
	char buf[MAX_PATH_SIZE];

	if (!gpio || gpio->fd < 0)
	{
		return -1;
	}

	close(gpio->fd);
	gpio->fd = -1;

	fd = open(GPIO_UNEXPORT, O_WRONLY);

	if (fd < 0)
	{
		LOGE("gpio unexport %d error \n", gpio->gpio_id);
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", gpio->gpio_id);

	if (write(fd, buf, len) < 0)
	{
		LOGE("gpio export write %d error \n", gpio->gpio_id);
		close(fd);
		return -1;
	}

	close(fd);
	return GPIO_ERROR_NONE;
}

void gpio_destroy(gpio_t *gpio)
{
	if (gpio)
	{
		if (gpio->fd != -1)
		{
			gpio_close(gpio);
		}

		bfree(gpio);
		gpio = NULL;
	}
}

int gpio_get_fd(gpio_t *gpio)
{
	if (gpio)
	{
		return gpio->fd;
	}

	return -1;
}