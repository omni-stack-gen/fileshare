#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <termios.h>

#include <periphery/serial.h>

#include <utils/bmem.h>

#define LOG_TAG "serial"
#include <utils/log.h>

typedef struct serial
{
	int fd;
} serial_t;

serial_t *serial_create()
{
	serial_t *serial = bmalloc(sizeof(serial_t));

	if (!serial)
	{
		return NULL;
	}

	serial->fd = -1;

	return serial;
}

void serial_destroy(serial_t *serial)
{
	if (serial)
	{
		if (serial->fd != -1)
		{
			serial_close(serial);
		}

		bfree(serial);
		serial = NULL;
	}
}

static speed_t serial_baudrate_to_speed(serial_baud_rate_t baud_rate)
{
	switch (baud_rate)
	{
		case SERIAL_BAUD_50:
			return B50;

		case SERIAL_BAUD_75:
			return B75;

		case SERIAL_BAUD_110:
			return B110;

		case SERIAL_BAUD_134:
			return B134;

		case SERIAL_BAUD_150:
			return B150;

		case SERIAL_BAUD_200:
			return B200;

		case SERIAL_BAUD_300:
			return B300;

		case SERIAL_BAUD_600:
			return B600;

		case SERIAL_BAUD_1200:
			return B1200;

		case SERIAL_BAUD_1800:
			return B1800;

		case SERIAL_BAUD_2400:
			return B2400;

		case SERIAL_BAUD_4800:
			return B4800;

		case SERIAL_BAUD_9600:
			return B9600;

		case SERIAL_BAUD_19200:
			return B19200;

		case SERIAL_BAUD_38400:
			return B38400;

		case SERIAL_BAUD_57600:
			return B57600;

		case SERIAL_BAUD_115200:
			return B115200;

		case SERIAL_BAUD_230400:
			return B230400;

		case SERIAL_BAUD_460800:
			return B460800;

		case SERIAL_BAUD_500000:
			return B500000;

		case SERIAL_BAUD_576000:
			return B576000;

		case SERIAL_BAUD_921600:
			return B921600;

		case SERIAL_BAUD_1000000:
			return B1000000;

		case SERIAL_BAUD_1152000:
			return B1152000;

		case SERIAL_BAUD_1500000:
			return B1500000;

		case SERIAL_BAUD_2000000:
			return B2000000;

		case SERIAL_BAUD_2500000:
			return B2500000;

		case SERIAL_BAUD_3000000:
			return B3000000;

		case SERIAL_BAUD_3500000:
			return B3500000;

		case SERIAL_BAUD_4000000:
			return B4000000;

		default:
			return -1;
	}
}

serial_error_t serial_open(serial_t *serial, const char *device,
                           const serial_config_t *config)
{
	struct termios opt;

	if (!serial || !device || !config)
	{
		LOGE("serial open invaild param (%p,%p,%p)\n", serial, device, config);
		return SERIAL_ERROR_PARAM;
	}

	if (serial->fd >= 0)
	{
		LOGE("serial already open %d\n", serial->fd);
		return SERIAL_ERROR_PARAM;
	}

	serial->fd = open(device, O_RDWR | O_NOCTTY | O_CLOEXEC);

	if (serial->fd < 0)
	{
		LOGE("serial open %s error errno:(%d:%s)\n", device, errno, strerror(errno));
		return SERIAL_ERROR_OPEN;
	}

	memset(&opt, 0, sizeof(struct termios));

	opt.c_iflag = IGNBRK;

	if (config->parity != SERIAL_PARITY_NONE)
	{
		opt.c_iflag |= INPCK;
	}

	if (config->parity != SERIAL_PARITY_NONE && config->data_bits != SERIAL_DATA_8)
	{
		opt.c_iflag |= ISTRIP;
	}

	// 软件流控制XON/XOFF（继续/停止）
	if (config->xonxoff)
	{
		opt.c_iflag |= (IXON | IXOFF);
	}

	opt.c_oflag = 0;
	opt.c_lflag = 0;

	// 启用接收器，忽略调制解调器控制线
	opt.c_cflag = CREAD | CLOCAL;

	switch (config->data_bits)
	{
		case SERIAL_DATA_5:
			opt.c_cflag |= CS5;
			break;

		case SERIAL_DATA_6:
			opt.c_cflag |= CS6;
			break;

		case SERIAL_DATA_7:
			opt.c_cflag |= CS7;
			break;

		default:
			opt.c_cflag |= CS8;
			break;
	}

	if (config->parity == SERIAL_PARITY_EVEN)
	{
		opt.c_cflag |= PARENB;
	}
	else if (config->parity == SERIAL_PARITY_ODD)
	{
		opt.c_cflag |= (PARENB | PARODD);
	}

	if (config->stop_bits == SERIAL_STOP_2)
	{
		opt.c_cflag |= CSTOPB;
	}
	else
	{
		opt.c_cflag &= ~CSTOPB;
	}

	// 硬件流控制（包括RTS/CTS、DTR/CTS等）
	if (config->rtscts)
	{
		opt.c_cflag |= CRTSCTS;
	}

	speed_t speed = serial_baudrate_to_speed(config->baud_rate);

	cfsetispeed(&opt, speed);
	cfsetospeed(&opt, speed);

	if (tcsetattr(serial->fd, TCSANOW, &opt) < 0)
	{
		close(serial->fd);
		serial->fd = -1;
		return SERIAL_ERROR_CONFIGURE;
	}

	return SERIAL_ERROR_NONE;
}

serial_error_t serial_close(serial_t *serial)
{
	if (!serial)
	{
		LOGE("serial close invaild param (%p)\n", serial);
		return SERIAL_ERROR_PARAM;
	}

	if (serial->fd < 0)
	{
		return SERIAL_ERROR_NONE;
	}

	if (close(serial->fd) < 0)
	{
		LOGE("serial close error\n");
		return SERIAL_ERROR_CLOSE;
	}

	serial->fd = -1;

	return SERIAL_ERROR_NONE;
}

int serial_read(serial_t *serial, uint8_t *buf, size_t len, unsigned long milliseconds)
{
	int ret = 0;
	fd_set rfds;
	struct timeval tv;
	struct timeval *time = NULL;

	size_t read_bytes = 0;
	size_t left_bytes = len;

	if (!serial || !buf || serial->fd < 0)
	{
		LOGE("serial read invaild param\n");
		return SERIAL_ERROR_PARAM;
	}

	if (milliseconds >= 0)
	{
		tv.tv_sec  = milliseconds / 1000;
		tv.tv_usec = (milliseconds % 1000) * 1000;
		time = &tv;
	}

	do
	{
		FD_ZERO(&rfds);
		FD_SET(serial->fd, &rfds);

		if ((ret = select(serial->fd + 1, &rfds, NULL, NULL, time)) < 0)
		{
			LOGE("serial read select error\n");
			return SERIAL_ERROR_IO;
		}

		if (ret == 0)
		{
			// LOGW("serial read time out\n");
			break;
		}

		if ((ret = read(serial->fd, buf + read_bytes, left_bytes)) < 0)
		{
			LOGE("serial read error\n");
			return SERIAL_ERROR_IO;
		}

		if (ret == 0 && len != 0)
		{
			LOGE("serial read zero data\n");
			return SERIAL_ERROR_IO;
		}

		read_bytes += ret;
		left_bytes -= ret;

		// LOGI("read %d bytes\n", ret);
		// LOG_BUFFER_HEX("serail", buf, read_bytes);
	} while (left_bytes > 0);

	return read_bytes;
}

int serial_read_once(serial_t *serial, uint8_t *buf, size_t len,
                     unsigned long milliseconds)
{
	int ret = 0;
	fd_set rfds;
	struct timeval tv;
	struct timeval *time = NULL;

	if (!serial || !buf || serial->fd < 0)
	{
		LOGE("serial read invaild param\n");
		return SERIAL_ERROR_PARAM;
	}


	if (milliseconds >= 0)
	{
		tv.tv_sec  = milliseconds / 1000;
		tv.tv_usec = (milliseconds % 1000) * 1000;
		time = &tv;
	}

	FD_ZERO(&rfds);
	FD_SET(serial->fd, &rfds);

	if ((ret = select(serial->fd + 1, &rfds, NULL, NULL, time)) < 0)
	{
		LOGE("serial read select error\n");
		return SERIAL_ERROR_IO;
	}

	if (ret == 0)
	{
		// LOGE("serial read select timeout \n");
		return 0;
	}

	if ((ret = read(serial->fd, buf, len)) < 0)
	{
		LOGE("serial read error\n");
		return SERIAL_ERROR_IO;
	}

	if (ret == 0 && len != 0)
	{
		LOGE("serial read zero data\n");
		return SERIAL_ERROR_IO;
	}

	return ret;
}

int serial_write(serial_t *serial, uint8_t *buf, size_t len)
{
	int ret;

	if (!serial || !buf || serial->fd < 0)
	{
		LOGE("serial write invaild param\n");
		return SERIAL_ERROR_PARAM;
	}

	// LOG_BUFFER_HEX("serial_write", buf, len);

	if ((ret = write(serial->fd, buf, len)) < 0)
	{
		LOGE("serial write len :%d error\n", len);
		return SERIAL_ERROR_IO;
	}

	return ret;
}

serial_error_t serial_flush(serial_t *serial)
{
	if (!serial || serial->fd < 0)
	{
		LOGE("serial write invaild param\n");
		return SERIAL_ERROR_PARAM;
	}

	if (tcdrain(serial->fd) < 0)
	{
		LOGE("serial flush error\n");
		return SERIAL_ERROR_IO;
	}

	return SERIAL_ERROR_NONE;
}

serial_error_t serial_poll(serial_t *serial, unsigned long milliseconds)
{
	int ret;
	struct pollfd fds[1];

	if (!serial || serial->fd < 0)
	{
		LOGE("serial poll invaild param\n");
		return SERIAL_ERROR_PARAM;
	}

	fds[0].fd = serial->fd;
	fds[0].events = POLLIN | POLLPRI;

	if ((ret = poll(fds, 1, milliseconds)) < 0)
	{
		LOGE("serial poll error\n");
		return SERIAL_ERROR_IO;
	}

	if (ret)
	{
		return SERIAL_ERROR_NONE;
	}

	return SERIAL_ERROR_POLL_TIMEOUT;
}

int serial_input_waiting(serial_t *serial, size_t *count)
{
	if (!serial || serial->fd < 0)
	{
		LOGE("serial write invaild param\n");
		return SERIAL_ERROR_PARAM;
	}

	if (ioctl(serial->fd, TIOCINQ, count) < 0)
	{
		LOGE("serial input waiting TIOCINQ error\n");
		return SERIAL_ERROR_IO;
	}

	return SERIAL_ERROR_NONE;
}

int serial_output_waiting(serial_t *serial, size_t *count)
{
	if (!serial || serial->fd < 0)
	{
		LOGE("serial write invaild param\n");
		return SERIAL_ERROR_PARAM;
	}

	if (ioctl(serial->fd, TIOCOUTQ, count) < 0)
	{
		LOGE("serial output waiting TIOCOUTQ error\n");
		return SERIAL_ERROR_IO;
	}

	return SERIAL_ERROR_NONE;
}

int serial_get_fd(serial_t *serial)
{
	if (!serial)
	{
		return -1;
	}

	return serial->fd;
}