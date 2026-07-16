#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <poll.h>

#include <linux/input.h>

#include <utils/input_event.h>

#define MAX_DEVICES     (16)
#define MAX_MISC_FDS    (16)

#define BITS_PER_LONG   (sizeof(unsigned long) * 8)
#define BITS_TO_LONGS(x) (((x) + BITS_PER_LONG - 1) / BITS_PER_LONG)

#define test_bit(bit, array) ((array)[(bit) / BITS_PER_LONG] & (1 << ((bit) % BITS_PER_LONG)))

struct fd_info
{
	ev_callback cb;
	void *data;
	unsigned long ev_bits;
};

static struct pollfd ev_fds[MAX_DEVICES + MAX_MISC_FDS];
static struct fd_info ev_fdinfo[MAX_DEVICES + MAX_MISC_FDS];

static unsigned ev_count = 0;
static unsigned ev_dev_count = 0;
static unsigned ev_misc_count = 0;

int input_event_init(ev_callback input_cb, void *data)
{
	DIR *dir;
	struct dirent *de;
	int fd;

	dir = opendir("/dev/input");

	if (dir != NULL)
	{
		while ((de = readdir(dir)))
		{
			unsigned long ev_bits[BITS_TO_LONGS(EV_MAX)];

			if (strncmp(de->d_name, "event", 5))
			{
				continue;
			}

			fd = openat(dirfd(dir), de->d_name, O_RDONLY);

			if (fd < 0)
			{
				continue;
			}

			if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0)
			{
				close(fd);
				continue;
			}

			if (!test_bit(EV_KEY, ev_bits) && !test_bit(EV_ABS, ev_bits))
			{
				close(fd);
				continue;
			}

			printf("open :%s \n", de->d_name);

			ev_fds[ev_count].fd = fd;
			ev_fds[ev_count].events = POLLIN;

			ev_fdinfo[ev_count].cb = input_cb;
			ev_fdinfo[ev_count].data = data;
			ev_fdinfo[ev_count].ev_bits = ev_bits[0];

			ev_count++;
			ev_dev_count++;

			if (ev_dev_count == MAX_DEVICES)
			{
				break;
			}
		}

		closedir(dir);
	}

	return ev_count;
}

void input_event_deinit(void)
{
	while (ev_count > 0)
	{
		close(ev_fds[--ev_count].fd);
	}

	ev_misc_count = 0;
	ev_dev_count = 0;
}

int input_event_wait(int timeout)
{
	int r;

	r = poll(ev_fds, ev_count, timeout);

	if (r <= 0)
	{
		return -1;
	}

	return 0;
}

void input_event_dispatch(void)
{
	unsigned n;

	for (n = 0; n < ev_count; n++)
	{
		ev_callback cb = ev_fdinfo[n].cb;

		if (cb && (ev_fds[n].revents & ev_fds[n].events))
		{
			cb(ev_fds[n].fd, ev_fds[n].revents, ev_fdinfo[n].ev_bits, ev_fdinfo[n].data);
		}
	}
}

int input_event_read(int fd, short revents, struct input_event *ev)
{
	int r;

	if (revents & POLLIN)
	{
		r = read(fd, ev, sizeof(*ev));

		if (r == sizeof(*ev))
		{
			return 0;
		}
	}

	return -1;
}

int input_event_add_fd(int fd, ev_callback cb, void *data)
{
	if (ev_misc_count == MAX_MISC_FDS || cb == NULL)
	{
		return -1;
	}

	ev_fds[ev_count].fd = fd;
	ev_fds[ev_count].events = POLLIN;
	ev_fdinfo[ev_count].cb = cb;
	ev_fdinfo[ev_count].data = data;
	ev_count++;
	ev_misc_count++;
	return 0;
}