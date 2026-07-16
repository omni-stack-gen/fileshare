#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <dirent.h>
#include <pthread.h>
#include <log.h>

#include <linux/netlink.h>

#include <hotplug.h>
// #include <SystemMsg.h>

#define UEVENT_BUFFER_SIZE (2048)
#define MAX_PARAMS         (32)

#define NL_ACTION_UNKOWN (0)
#define NL_ACTION_ADD    (1)
#define NL_ACTION_REMOVE (2)
#define NL_ACTION_CHANGE (3)

// #define LOGD(...)  printf(__VA_ARGS__)
// #define LOGI(...)  printf(__VA_ARGS__)
// #define LOGE(...)  printf(__VA_ARGS__)

typedef struct netlink_event
{
	int  seq;
	char *path;
	int  action;
	char *subsystem;
	char *params[MAX_PARAMS];
} netlink_event_t;

typedef struct hotplug_mgnt
{
	int uevent_fd;
	int monitor_stop;
	int sdcard_mounted;
	sdcard_mount_state_cb mount_state_cb;
	void *user;
	pthread_t monitor_thr;
} hotplug_mgnt_t;

static hotplug_mgnt_t *g_hotplug_mgnt = NULL;

static int sdcard_is_mounted(void);

// static void parse_uevent(char *message, int message_len)
// {
// #define MAX_ARGV_COUNT 64
// 	char *start = message;
// 	char *end = message + message_len;

// 	int argc = 0, i = 0;
// 	char *argv[MAX_ARGV_COUNT];

// 	memset(argv, 0, sizeof(argv));

// 	while (start != end && argc < MAX_ARGV_COUNT)
// 	{
// 		argv[argc++] = start;
// 		start += strlen(start) + 1;
// 	}

// 	LOGD("\nargc:%d ========================== \n", argc);

// 	for (i = 0; i < argc; ++i)
// 	{
// 		LOGD("#%d %s\n", i, argv[i]);
// 	}

// 	// argv[argc] = NULL;
// }

static const char *has_prefix(const char *str, const char *end, const char *prefix, size_t prefixlen)
{
	if ((end - str) >= (ptrdiff_t)prefixlen && !memcmp(str, prefix, prefixlen))
	{
		return str + prefixlen;
	}
	else
	{
		return NULL;
	}
}

#define CONST_STRLEN(x)  (sizeof(x) - 1)

#define HAS_CONST_PREFIX(str,end,prefix)  has_prefix((str), (end), prefix, CONST_STRLEN(prefix))

netlink_event_t *netlink_event_create()
{
	netlink_event_t *event = (netlink_event_t *)malloc(sizeof(*event));

	if (!event)
	{
		return NULL;
	}

	memset(event, 0, sizeof(*event));

	return event;
}

bool netlink_event_destroy(netlink_event_t *event)
{
	if (!event)
	{
		return false;
	}

	int i;

	if (event->path)
	{
		free(event->path);
	}

	if (event->subsystem)
	{
		free(event->subsystem);
	}

	for (i = 0; i < MAX_PARAMS; i++)
	{
		if (!event->params[i])
		{
			break;
		}

		free(event->params[i]);
	}

	free(event);

	return true;
}

bool netlink_event_parse(netlink_event_t *event, char *buffer, int size)
{
	const char *s = buffer;
	const char *end;
	int param_idx = 0;
	// int i;
	int first = 1;

	if (size == 0)
	{
		return false;
	}

	end = s + size;

	while (s < end)
	{
		if (first)
		{
			const char *p;

			for (p = s; *p != '@'; p++)
			{
				if (!*p)
				{
					return false;
				}
			}

			event->path = strdup(p + 1);
			first = 0;
		}
		else
		{
			const char *a;

			if ((a = HAS_CONST_PREFIX(s, end, "ACTION=")) != NULL)
			{
				if (strcmp(a, "add") == 0)
				{
					event->action = NL_ACTION_ADD;
				}
				else if (strcmp(a, "remove") == 0)
				{
					event->action = NL_ACTION_REMOVE;
				}
				else if (strcmp(a, "change") == 0)
				{
					event->action = NL_ACTION_CHANGE;
				}
			}
			else if ((a = HAS_CONST_PREFIX(s, end, "SEQNUM=")) != NULL)
			{
				event->seq = atoi(a);
			}
			else if ((a = HAS_CONST_PREFIX(s, end, "SUBSYSTEM=")) != NULL)
			{
				event->subsystem = strdup(a);
			}
			else if (param_idx < MAX_PARAMS)
			{
				event->params[param_idx++] = strdup(s);
			}
		}

		s += strlen(s) + 1;
	}

	return true;
}

static const char *netlink_event_action_name(int type)
{
#define NL_EVENT_ACTION_NAME(action_name) case action_name: return #action_name;

	switch (type)
	{
			NL_EVENT_ACTION_NAME(NL_ACTION_UNKOWN);
			NL_EVENT_ACTION_NAME(NL_ACTION_ADD);
			NL_EVENT_ACTION_NAME(NL_ACTION_REMOVE);
			NL_EVENT_ACTION_NAME(NL_ACTION_CHANGE);

		default:
			return NULL;
	}

#undef NL_EVENT_ACTION_NAME
}

void netlink_event_dump(netlink_event_t *event)
{
	if (!event)
	{
		return ;
	}

	LOGD("\n==================start=======================\n");
	LOGD("NL seq is '%d'\n", event->seq);
	LOGD("NL path is '%s'\n", event->path);
	LOGD("NL action is '(%d:%s)'\n", event->action, netlink_event_action_name(event->action));
	LOGD("NL subsys is '%s'\n", event->subsystem);

	int i = 0;

	for (i = 0; i < MAX_PARAMS; i++)
	{
		if (!event->params[i])
		{
			break;
		}

		LOGD("NL param[%d] '%s'\n", i, event->params[i]);
	}

	LOGD("====================end=======================\n\n");
}

const char *netlink_event_find_param(netlink_event_t *event, const char *param_name)
{
	size_t len = strlen(param_name);

	int i = 0;

	for (i = 0; i < MAX_PARAMS && event->params[i] != NULL; ++i)
	{
		const char *ptr = event->params[i] + len;

		if (!strncmp(event->params[i], param_name, len) && *ptr == '=')
		{
			return ++ptr;
		}
	}

	// LOGE("netlink_event_find_param Parameter '%s' not found \n", param_name);

	return NULL;
}

void netlink_event_handle(netlink_event_t *event, hotplug_mgnt_t *mgr)
{
	const char *path = event->path;

	const char *subsystem = event->subsystem;

	if (!path || !subsystem)
	{
		LOGE("No path or subsystem found in netlink event (%p,%p)\n", path, subsystem);
		return ;
	}

	const char *device_name = netlink_event_find_param(event, "DEVNAME");
	// const char *device_type = netlink_event_find_param(event, "DEVTYPE");
	// LOGI("path:%s subsystem:%s action:%d device_name:%s \n", path, subsystem, event->action, device_name);

	if (device_name && strstr(device_name, "mmcblk") /*&& device_type && strcmp(device_type, "partition") == 0*/)
	{
		if (event->action == NL_ACTION_ADD)
		{
			if (mgr->sdcard_mounted != 1)
			{
				mgr->sdcard_mounted = 1;

				if (mgr->mount_state_cb)
				{
					mgr->mount_state_cb(SDCARD_MOUNT, mgr->user);
				}
			}
		}
		else if (event->action == NL_ACTION_REMOVE)
		{
			if (mgr->sdcard_mounted != 0)
			{
				mgr->sdcard_mounted = 0;

				if (mgr->mount_state_cb)
				{
					mgr->mount_state_cb(SDCARD_UNMOUNT, mgr->user);
				}
			}
		}
	}

	return ;
}

static int netlink_open()
{
	int send_size = 64 * 1024;
	int on = 1;
	int uevent_fd = -1;

	struct sockaddr_nl nladdr;
	memset(&nladdr, 0, sizeof(nladdr));

	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid    = getpid();
	nladdr.nl_groups = 0xffffffff;

	if ((uevent_fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT)) < 0)
	{
		LOGE("Unable to create uevent socket: %s \n", strerror(errno));
		return -1;
	}

	if (setsockopt(uevent_fd, SOL_SOCKET, SO_RCVBUFFORCE, &send_size, sizeof(send_size)) < 0)
	{
		LOGE("Unable to set uevent socket options: %s \n", strerror(errno));
		close(uevent_fd);
		return -1;
	}

	if (setsockopt(uevent_fd, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on)) < 0)
	{
		LOGE("Unable to set uevent socket SO_PASSCRED option: %s", strerror(errno));
		close(uevent_fd);
		return -1;
	}

	if (bind(uevent_fd, (struct sockaddr *)&nladdr, sizeof(nladdr)) < 0)
	{
		LOGE("Unable to bind uevent socket: %s \n", strerror(errno));
		close(uevent_fd);
		return -1;
	}

	return uevent_fd;
}

static void *hotplug_monitor_thread(void *args)
{
	LOGI("hotplug_monitor_thread start\n");

	int length = -1;

	char buffer[UEVENT_BUFFER_SIZE * 2];

	struct pollfd fds;

	int ret = -1;

	hotplug_mgnt_t *mgr = (hotplug_mgnt_t *)args;

	while (mgr->monitor_stop == 0)
	{
		memset(buffer, 0, sizeof(buffer));

		memset(&fds, 0, sizeof(fds));

		fds.fd = mgr->uevent_fd;
		fds.events = POLLIN;
		fds.revents = 0;

		ret = poll(&fds, 1, 100);

		if (ret > 0)
		{
			length = recv(mgr->uevent_fd, &buffer, sizeof(buffer), 0);

			// log_hex_string((uint8_t *)buffer, length);

			if (length > 0)
			{
				// parse_uevent(buffer, length);

				netlink_event_t *ev = netlink_event_create();

				if (ev)
				{
					if (netlink_event_parse(ev, buffer, length))
					{
						// netlink_event_dump(ev);
						netlink_event_handle(ev, mgr);
					}

					netlink_event_destroy(ev);
				}
			}
		}
	}

	LOGI("hotplug_monitor_thread stop\n");

	return NULL;
}

static int hotplug_monitor_init(sdcard_mount_state_cb cb, void *user)
{
	if (g_hotplug_mgnt)
	{
		return 0;
	}

	int ret;

	g_hotplug_mgnt = (hotplug_mgnt_t *)malloc(sizeof(hotplug_mgnt_t));

	if (!g_hotplug_mgnt)
	{
		LOGE("malloc hotplug_mgnt_t failed \n");
		return -1;
	}

	memset(g_hotplug_mgnt, 0, sizeof(hotplug_mgnt_t));

	g_hotplug_mgnt->mount_state_cb = cb;
	g_hotplug_mgnt->user = user;

	g_hotplug_mgnt->uevent_fd = netlink_open();

	if (g_hotplug_mgnt->uevent_fd == -1)
	{
		LOGE("netlink_open error \n");
		goto error;
	}

	g_hotplug_mgnt->monitor_stop = 0;

	if (sdcard_is_mounted() == 0)
	{
		g_hotplug_mgnt->sdcard_mounted = 1;

		if (g_hotplug_mgnt->mount_state_cb)
		{
			g_hotplug_mgnt->mount_state_cb(SDCARD_MOUNT, g_hotplug_mgnt->user);
		}
	}

	ret = pthread_create(&g_hotplug_mgnt->monitor_thr, NULL, hotplug_monitor_thread, g_hotplug_mgnt);

	if (ret != 0)
	{
		LOGE("pthread_create error:%d \n", ret);
		goto error;
	}

	return 0;

error:

	if (g_hotplug_mgnt)
	{
		if (g_hotplug_mgnt->uevent_fd != -1)
		{
			close(g_hotplug_mgnt->uevent_fd);
			g_hotplug_mgnt->uevent_fd = -1;
		}

		free(g_hotplug_mgnt);

		g_hotplug_mgnt = NULL;
	}

	return -1;
}

void hotplug_monitor_deinit(void)
{
	if (g_hotplug_mgnt)
	{
		g_hotplug_mgnt->monitor_stop = 1;

		if (g_hotplug_mgnt->monitor_thr)
		{
			pthread_join(g_hotplug_mgnt->monitor_thr, NULL);
			g_hotplug_mgnt->monitor_thr = 0;
		}

		if (g_hotplug_mgnt->uevent_fd != -1)
		{
			close(g_hotplug_mgnt->uevent_fd);
			g_hotplug_mgnt->uevent_fd = -1;
		}

		free(g_hotplug_mgnt);

		g_hotplug_mgnt = NULL;
	}
}

#define FILE_MOUNT_CHECK        "/proc/mounts"
#define FILE_DISC_PARTS_CHECK       "/proc/partitions"
#define UDISK_DRIVER_PREFIX_NAME    "sd"

static char *get_dev_sd_block()
{
	static char block[16] = {0};

	if (strlen(block) == 0)
	{
		char buf[256] = {0};
		int fd = open("/proc/cmdline", O_RDONLY);

		if (fd > 0)
		{
			read(fd, buf, 255);

			if (strstr(buf, "mmc") != NULL)
			{
				strncpy(block, "mmcblk1", sizeof(block));
			}
			else
			{
				strncpy(block, "mmcblk0", sizeof(block));
			}
		}
		else
		{
			printf("%s:%d>>>>> open /proc/cmdline failed >>>\n",
			       __func__, __LINE__);
		}

		close(fd);
	}

	return block;
}

static int sdcard_get_partitions(char *p_str_name, int name_max_length)
{
	int ret = -1;
	int len = 0;
	int fd = 0;
	char buf[1024];
	const char *sd_block_par = get_dev_sd_block();

	if (NULL == p_str_name)
	{
		return -1;
	}

	fd = open(FILE_DISC_PARTS_CHECK, O_RDONLY);

	if (fd != -1)
	{
		memset(buf, 0, sizeof(buf));
		len = read(fd, buf, sizeof(buf));
		close(fd);

		if (len > 0)
		{
			if (strstr(buf, sd_block_par) != NULL)
			{
				snprintf(p_str_name, name_max_length, "%s", sd_block_par);
				// printf("%s:%d>>find sd_card block=%s\n",
				//        __func__, __LINE__, p_str_name);
				ret = 0;
			}
		}
	}
	else
	{
		ret = -1;
		printf("[%s:%d] Open %s fail\n", __func__, __LINE__,
		       FILE_DISC_PARTS_CHECK);
	}

	return ret;
}

static int check_mount(const char *partition_name)
{
	int fd;
	int len;
	char *seek;
	int ret = -1;
	char buf[2048];

	if (NULL == partition_name)
	{
		printf("The device is NULL\n");
		return -1;
	}

	fd = open(FILE_MOUNT_CHECK, O_RDONLY);

	if (fd != -1)
	{
		memset(buf, 0, sizeof(buf));

		while ((len = read(fd, buf, sizeof(buf) - 1)) > 0)
		{
			buf[len] = '\0';
			seek = strstr(buf, partition_name);

			if (seek != NULL)
			{
				ret = 0;
				break;
			}

			memset(buf, 0, sizeof(buf));
		}

		close(fd);
	}
	else
	{
		printf("Open file %s error\n", FILE_MOUNT_CHECK);
	}

	return ret;
}

static int sdcard_is_mounted(void)
{
	int ret = -1;
	char p_str_name[64];

	ret = sdcard_get_partitions(p_str_name, sizeof(p_str_name));

	if (0 == ret)
	{
		ret = check_mount(p_str_name);
	}

	return ret;
}

static bool is_sdcard_exist = false;
static void sdcard_mount_state_callback(sdcard_mount_state_t state, void *user)
{
	if (state == SDCARD_MOUNT)
	{
		is_sdcard_exist = true;
		LOGE("sdcard mounted \n");

		//system("mkdir /tmp/card");
		//system("mount -t vfat   /dev/mmcblk1p1 /tmp/card");
#if defined(ENABLE_V1) || defined(ENABLE_V6)
		int play_keytone(int volume);
		play_keytone(50);
#endif
	}
	else
	{
		LOGE("sdcard unmounted \n");
		is_sdcard_exist = false;
		//system("umount /tmp/card");
	}


	//TF卡插拔变化通知界面显示
	//DPPostMessage(MSG_BROADCAST, TF_CARD_CHANGE, 0, 0);
}

int sdcard_monitor_init()
{
	return hotplug_monitor_init(sdcard_mount_state_callback, NULL);
}

bool check_sdcard_status()
{
	return is_sdcard_exist;
}
