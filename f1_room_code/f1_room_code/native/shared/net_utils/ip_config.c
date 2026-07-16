/* ip_config.c */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <features.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <net/if_arp.h>
#include <netinet/in.h>

#include <utils/bmem.h>
#include <utils/platform.h>

#include <net_utils/net_utils.h>
#include <net_utils/ip_config.h>
#include <net_utils/if_req.h>

#define LOG_TAG "net_utils"
#include <net_utils/debug.h>

#undef  INT_MAX
#undef  INT_MIN
#define INT_MAX         ((int)(~0U>>1))
#define INT_MIN         (-INT_MAX - 1)

#define PID_FILE_NAME_LEN   (40)
#define DHCP_HOSTNAME_PRE   "hostname:"

#define RESOLV_CONF         "/etc/resolv.conf"

typedef struct dhcp_handle_info
{
	char dev_name[IFNAMSIZ];
	pid_t pid;
	dhcp_status_handler handler;
	void *user_data;
} dhcp_handle_info_t;

typedef struct net_priority_info
{
	int ethernet_priority;
	int wifi_priority;
	int mobile_priority;
} net_priority_info_t;

net_priority_info_t priority_info = {100, 200, 300};

/**
 * obtain_one_num - get the number of '1' in the binary number.
 * @n: binary number.
 *
 * return the number of '1'.
 */
static int obtain_one_num(unsigned int n)
{
	int count = 0;
	int flag = 1;

	while (n)
	{
		if (n & flag)
		{
			count++;
		}

		n >>= 1;
	}

	return count;
}

/**
 * sockaddr_in_init - init sockaddr_in.
 * @sa: pointer to sockaddr struct.
 * @addr: address.
 */
static void sockaddr_in_init(struct sockaddr *sa, unsigned long addr)
{
	struct sockaddr_in *sin = (struct sockaddr_in *)sa;
	sin->sin_family = AF_INET;
	sin->sin_port = 0;
	sin->sin_addr.s_addr = addr;
}

/**
 * prefix_length_to_ipv4_netmask - exchange prefix length to IPv4 netmask.
 * @prefix_length: prefix length.
 *
 * return netmask.
 */
static unsigned int prefix_length_to_ipv4_netmask(int prefix_length)
{
	unsigned int mask = 0;

	/* C99 (6.5.7): shifts of 32 bits have undefined results */
	if (prefix_length <= 0 || prefix_length > 32)
	{
		return 0;
	}

	mask = ~mask << (32 - prefix_length);
	mask = htonl(mask);

	return mask;
}

/**
 * ifc_act_on_ipv4_route - take some action on IPv4 route.
 * @action: action.
 * @dev_name: device name.
 * @dst: destination address.
 * @prefix_length: prefix length.
 * @gw: gateway.
 *
 * return 0 if success; otherwise failed.
 */
static int ifc_act_on_ipv4_route(int action, const char *dev_name,
                                 struct in_addr dst, int prefix_length,
                                 struct in_addr gw)
{
	int fd, ret;
	unsigned int netmask;
	struct rtentry rt;

	memset(&rt, 0, sizeof(rt));
	rt.rt_dst.sa_family = AF_INET;
	rt.rt_dev = (void *)dev_name;

	netmask = prefix_length_to_ipv4_netmask(prefix_length);
	sockaddr_in_init(&rt.rt_genmask, netmask);
	sockaddr_in_init(&rt.rt_dst, dst.s_addr);
	rt.rt_flags = RTF_UP;

	if (prefix_length == 32)
	{
		rt.rt_flags |= RTF_HOST;
	}

	if (gw.s_addr != 0)
	{
		rt.rt_flags |= RTF_GATEWAY;
		sockaddr_in_init(&rt.rt_gateway, gw.s_addr);
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0)
	{
		ALOGE("socket() failed: %s", strerror(errno));
		return -1;
	}

	ret = ioctl(fd, action, &rt);

	if (ret < 0)
	{
		if (EEXIST == errno)
		{
			ret = 0;
		}
		else
		{
			ret = -errno;
			ALOGE("IOCTL action(%d) failed: %s",
			      action, strerror(errno));
		}
	}

	close(fd);

	return ret;
}

/**
 * ifc_manage_ipv4_route_priority - take some action on IPv4 route with priority.
 * @action: action.
 * @dev_name: device name.
 * @dst: destination address.
 * @prefix_length: prefix length.
 * @gw: gateway.
 *
 * return 0 if success; otherwise failed.
 */
static int ifc_manage_ipv4_route_priorty(int action, const char *dev_name,
        struct in_addr dst, int prefix_length,
        struct in_addr gw)
{
	int fd, ret;
	unsigned int netmask;
	struct rtentry rt;

	memset(&rt, 0, sizeof(rt));
	rt.rt_dst.sa_family = AF_INET;
	rt.rt_dev = (void *)dev_name;

	if (!strcmp(dev_name, "eth0"))
	{
		rt.rt_metric = priority_info.ethernet_priority + 1;
	}
	else if (!strcmp(dev_name, "wlan0"))
	{
		rt.rt_metric = priority_info.wifi_priority + 1;
	}
	else if (!strcmp(dev_name, "ppp0"))
	{
		rt.rt_metric = priority_info.mobile_priority + 1;
	}

	netmask = prefix_length_to_ipv4_netmask(prefix_length);
	sockaddr_in_init(&rt.rt_genmask, netmask);
	sockaddr_in_init(&rt.rt_dst, dst.s_addr);
	rt.rt_flags = RTF_UP;

	if (prefix_length == 32)
	{
		rt.rt_flags |= RTF_HOST;
	}

	if (gw.s_addr != 0)
	{
		rt.rt_flags |= RTF_GATEWAY;
		sockaddr_in_init(&rt.rt_gateway, gw.s_addr);
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0)
	{
		ALOGE("socket() failed: %s", strerror(errno));
		return -1;
	}

	ret = ioctl(fd, action, &rt);

	if (ret < 0)
	{
		if (EEXIST == errno)
		{
			ret = 0;
		}
		else
		{
			ret = -errno;
			ALOGE("IOCTL action(%d) failed: %s",
			      action, strerror(errno));
		}
	}

	close(fd);

	return ret;
}

static int add_dns_server(const char *interface, const char *dns0, const char *dns1)
{
	FILE *resolv_file = NULL;

	resolv_file = fopen(RESOLV_CONF, "a");

	if (resolv_file == NULL)
	{
		ALOGE("Error opening %s", RESOLV_CONF);
		return -1;
	}

	// 向 resolv.conf 添加新的 DNS 服务器
	if (dns0)
	{
		fprintf(resolv_file, "nameserver %s # %s\n", dns0, interface);
	}

	if (dns1)
	{
		fprintf(resolv_file, "nameserver %s # %s\n", dns1, interface);
	}

	fclose(resolv_file);

	return 0;
}

static int remove_dns_server(const char *interface)
{
	FILE *resolv_file = NULL, *tmp_file = NULL;
	char line[256] = {0};
	char tmp_filename[] = "/tmp/resolv_temp_XXXXXX";
	int fd = -1;

	if (access(RESOLV_CONF, R_OK) != 0)
	{
		// ALOGE("%s does not exist", RESOLV_CONF);
		return 0;
	}

	fd = mkstemp(tmp_filename);

	if (fd == -1)
	{
		ALOGE("Error creating temp file");
		return -1;
	}

	resolv_file = fopen(RESOLV_CONF, "r");

	if (resolv_file == NULL)
	{
		ALOGE("Error opening %s", RESOLV_CONF);
		close(fd);
		remove(tmp_filename);
		return -1;
	}

	tmp_file = fdopen(fd, "w");

	if (tmp_file == NULL)
	{
		ALOGE("Error opening temp file");
		fclose(resolv_file);
		close(fd);
		remove(tmp_filename);
		return -1;
	}

	// 读取 resolv.conf 并删除包含接口注释的行
	while (fgets(line, sizeof(line), resolv_file))
	{
		if (strstr(line, interface) == NULL)
		{
			fputs(line, tmp_file);
		}
	}

	fclose(resolv_file);
	fclose(tmp_file);

	// 将临时文件重命名为 resolv.conf
	if (rename(tmp_filename, RESOLV_CONF) != 0)
	{
		if (errno == EXDEV)
		{
			// 打开目标文件
			resolv_file = fopen(RESOLV_CONF, "w");

			if (resolv_file == NULL)
			{
				ALOGE("Error opening %s for writing", RESOLV_CONF);
				return -1;
			}

			// 打开临时文件并复制内容
			tmp_file = fopen(tmp_filename, "r");

			if (tmp_file == NULL)
			{
				ALOGE("Error opening %s for reading", tmp_filename);
				fclose(resolv_file);
				return -1;
			}

			// 将临时文件的内容写入到 resolv.conf
			while (fgets(line, sizeof(line), tmp_file))
			{
				fputs(line, resolv_file);
			}

			fclose(resolv_file);
			fclose(tmp_file);

			// 删除临时文件
			if (remove(tmp_filename) != 0)
			{
				ALOGE("Error deleting temporary file");
				return -1;
			}

			ALOGI("%s updated successfully using copy.", RESOLV_CONF);
		}
		else
		{
			ALOGE("Error renaming temp file to %s", RESOLV_CONF);
			return -1;
		}
	}

	return 0;
}

/**
 * net_dev_set_ip_addr - net device set IP address.
 * @dev_name: device name.
 * @ip_addr: IP address to set.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_set_ip_addr(const char *dev_name, unsigned long ip_addr)
{
	int fd, ret;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0)
	{
		ALOGE("socket() failed: %s", strerror(errno));
		return -1;
	}

	ifreq_init(dev_name, &ifr);
	sockaddr_in_init(&ifr.ifr_addr, ip_addr);

	ret = ioctl(fd, SIOCSIFADDR, &ifr);

	if (ret < 0)
	{
		ALOGE("SIOCSIFADDR failed: %s", strerror(errno));
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

/**
 * net_dev_get_ip_addr - net device get IP address.
 * @dev_name: device name.
 * @ip_addr: the IP address we get.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_get_ip_addr(const char *dev_name, unsigned long *ip_addr)
{
	int fd, ret = 0;
	struct ifreq ifr;

	if (!ip_addr)
	{
		ALOGE("invalid IP address buffer");
		return -1;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0)
	{
		ALOGE("socket() failed: %s", strerror(errno));
		return -1;
	}

	ifreq_init(dev_name, &ifr);

	ret = ioctl(fd, SIOCGIFADDR, &ifr);

	if (ret < 0)
	{
		ALOGE("SIOCGIFADDR failed: %s", strerror(errno));
		close(fd);
		return -1;
	}

	*ip_addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

	close(fd);

	return 0;
}

/**
 * net_dev_del_ip_config - net device delete IP config.
 * @dev_name: device name.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_del_ip_config(const char *dev_name)
{
	net_dev_set_ip_addr(dev_name, 0UL);

#if 0
	char cmd[64];

	memset(cmd, 0x00, sizeof(cmd));
	sprintf(cmd, "/system/misc/net/ip_drop %s", dev_name);
	return os_system(cmd);
#else
	return remove_dns_server(dev_name);
#endif
}

/**
 * net_dev_set_netmask - net device set netmask.
 * @dev_name: device name.
 * @netmask: netmask to set.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_set_netmask(const char *dev_name, unsigned long netmask)
{
	int fd, ret;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0)
	{
		ALOGE("socket() failed: %s", strerror(errno));
		return -1;
	}

	ifreq_init(dev_name, &ifr);
	sockaddr_in_init(&ifr.ifr_netmask, netmask);

	ret = ioctl(fd, SIOCSIFNETMASK, &ifr);

	if (ret)
	{
		ALOGE("SIOCSIFNETMASK failed: %s", strerror(errno));
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

/**
 * net_dev_get_netmask - net device get netmask.
 * @dev_name: device name.
 * @netmask: the netmask we get.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_get_netmask(const char *dev_name, unsigned long *netmask)
{
	int fd, ret = 0;
	struct ifreq ifr;

	if (!netmask)
	{
		ALOGE("invalid netmask buffer");
		return -1;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0)
	{
		ALOGE("socket() failed: %s", strerror(errno));
		return -1;
	}

	ifreq_init(dev_name, &ifr);

	ret = ioctl(fd, SIOCGIFNETMASK, &ifr);

	if (ret < 0)
	{
		ALOGE("SIOCGIFNETMASK failed: %s", strerror(errno));
		close(fd);
		return -1;
	}

	*netmask = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

	close(fd);

	return 0;
}

/**
 * net_dev_remove_gateway - net device remove default gateway.
 * @dev_name: device name.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_remove_gateway(const char *dev_name)
{
	int fd, ret;
	struct rtentry rt;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0)
	{
		ALOGE("socket() failed: %s", strerror(errno));
		return -1;
	}

	memset(&rt, 0, sizeof(rt));
	rt.rt_dev = (void *)dev_name;
	rt.rt_flags = RTF_UP | RTF_GATEWAY;
	sockaddr_in_init(&rt.rt_dst, 0);

	ret = ioctl(fd, SIOCDELRT, &rt);

	if ((ret < 0) && (errno != ESRCH))
	{
		ALOGE("SIOCDELRT failed: %s", strerror(errno));
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

/**
 * net_dev_set_gateway - net device set default gateway.
 * @dev_name: device name.
 * @gateway: gateway to set.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_set_gateway(const char *dev_name, unsigned long gateway)
{
	int ret;
	struct in_addr in_dst, in_gw;

	in_dst.s_addr = 0;
	in_gw.s_addr = gateway;

	ret = net_dev_remove_gateway(dev_name);

	if (ret)
	{
		ALOGE("failed to remove default route for net device %s",
		      dev_name);
		return -1;
	}

	ret = ifc_act_on_ipv4_route(SIOCADDRT, dev_name, in_dst, 0, in_gw);

	if (ret)
	{
		ALOGE("failed to set default route for net device %s",
		      dev_name);
		return -1;
	}

	return 0;
}

/**
 * net_dev_get_gateway - net device get default gateway.
 * @dev_name: device name.
 * @gateway: the gateway we get.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_get_gateway(const char *dev_name, unsigned long *gateway)
{
	FILE *fp;
	char buf[256];
	char iface[IFNAMSIZ];
	unsigned long dest_addr, gate_addr;

	*gateway = INADDR_NONE;

	fp = fopen("/proc/net/route", "r");

	if (!fp)
	{
		ALOGE("failed to open file /proc/net/route");
		return -1;
	}

	/* skip title line */
	fgets(buf, sizeof(buf), fp);

	while (fgets(buf, sizeof(buf), fp))
	{
		if (sscanf(buf, "%s\t%lx\t%lx",
		           iface, &dest_addr, &gate_addr) != 3)
		{
			continue;
		}

		if (dest_addr != 0)
		{
			continue;
		}

		if (strcmp(dev_name, iface))
		{
			continue;
		}

		*gateway = gate_addr;
		break;
	}

	fclose(fp);

	if (INADDR_NONE == (*gateway))
	{
		ALOGE("failed to found default route of net device %s",
		      dev_name);
		return -1;
	}

	return 0;
}

/**
 * net_dev_set_dns_ip - net device set dns server's IP address.
 * @dev_name: device name.
 * @dns0: first choice dns server's IP address.
 * @dns1: additional dns server's IP address.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_set_dns_ip(const char *dev_name, unsigned long dns0,
                       unsigned long dns1)
{
#if 0
	char cmd[256];

	memset(cmd, 0x00, sizeof(cmd));

	if (dns0 || dns1)
	{
		if (dns0 && dns1)
		{
			sprintf(cmd, "/system/misc/net/dns_config %s %d.%d.%d.%d "
			        "%d.%d.%d.%d", dev_name,
			        (int)(dns0 & 0xFF),
			        (int)((dns0 >> 8) & 0xFF),
			        (int)((dns0 >> 16) & 0xFF),
			        (int)((dns0 >> 24) & 0xFF),
			        (int)(dns1 & 0xFF),
			        (int)((dns1 >> 8) & 0xFF),
			        (int)((dns1 >> 16) & 0xFF),
			        (int)((dns1 >> 24) & 0xFF));
		}
		else if (dns0)
		{
			sprintf(cmd, "/system/misc/net/dns_config %s %d.%d.%d.%d",
			        dev_name,
			        (int)(dns0 & 0xFF),
			        (int)((dns0 >> 8) & 0xFF),
			        (int)((dns0 >> 16) & 0xFF),
			        (int)((dns0 >> 24) & 0xFF));
		}
		else
		{
			sprintf(cmd, "/system/misc/net/dns_config %s %d.%d.%d.%d",
			        dev_name,
			        (int)(dns1 & 0xFF),
			        (int)((dns1 >> 8) & 0xFF),
			        (int)((dns1 >> 16) & 0xFF),
			        (int)((dns1 >> 24) & 0xFF));
		}

		os_system(cmd);
	}

	return 0;
#else
	int rc = 0;

	char dns0_str[16] = {0};
	char dns1_str[16] = {0};

	if (dns0 || dns1)
	{
		rc = remove_dns_server(dev_name);

		if (rc != 0)
		{
			ALOGE("remove_dns_server %s fail", dev_name);
			return rc;
		}

		if (dns0 && dns1)
		{
			snprintf(dns0_str, sizeof(dns0_str), "%d.%d.%d.%d",
			         (int)((dns0 >> 0) & 0xFF),
			         (int)((dns0 >> 8) & 0xFF),
			         (int)((dns0 >> 16) & 0xFF),
			         (int)((dns0 >> 24) & 0xFF));

			snprintf(dns1_str, sizeof(dns1_str), "%d.%d.%d.%d",
			         (int)((dns1 >> 0) & 0xFF),
			         (int)((dns1 >> 8) & 0xFF),
			         (int)((dns1 >> 16) & 0xFF),
			         (int)((dns1 >> 24) & 0xFF));

			rc = add_dns_server(dev_name, dns0_str, dns1_str);
		}
		else if (dns0)
		{
			snprintf(dns0_str, sizeof(dns0_str), "%d.%d.%d.%d",
			         (int)((dns0 >> 0) & 0xFF),
			         (int)((dns0 >> 8) & 0xFF),
			         (int)((dns0 >> 16) & 0xFF),
			         (int)((dns0 >> 24) & 0xFF));

			rc = add_dns_server(dev_name, dns0_str, NULL);
		}
		else
		{
			snprintf(dns1_str, sizeof(dns1_str), "%d.%d.%d.%d",
			         (int)((dns1 >> 0) & 0xFF),
			         (int)((dns1 >> 8) & 0xFF),
			         (int)((dns1 >> 16) & 0xFF),
			         (int)((dns1 >> 24) & 0xFF));

			rc = add_dns_server(dev_name, NULL, dns1_str);
		}
	}

	return rc;
#endif
}

/**
 * net_dev_set_dns_ip - net device get dns server's IP address.
 * @dev_name: device name.
 * @dns0: first choice dns server's IP address.
 * @dns1: additional dns server's IP address.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_get_dns_ip(const char *dev_name, unsigned long *dns0,
                       unsigned long *dns1)
{
	FILE *fp;
	char tmp, title[256], buf[256], iface[IFNAMSIZ];
	int ip0, ip1, ip2, ip3;

	*dns0 = 0;
	*dns1 = 0;

	fp = fopen(RESOLV_CONF, "r");

	if (!fp)
	{
		ALOGE("failed to open file %s", RESOLV_CONF);
		return -1;
	}

	while (fgets(buf, sizeof(buf), fp))
	{
		if (sscanf(buf, "%s\t%d.%d.%d.%d\t%c\t%s",
		           title, &ip0, &ip1, &ip2, &ip3, &tmp, iface) != 7)
		{
			continue;
		}

		if (strcmp("nameserver", title))
		{
			continue;
		}

		if (strcmp(dev_name, iface))
		{
			continue;
		}

		if ('#' != tmp)
		{
			continue;
		}

		if (!(*dns0))
		{
			*dns0 = (unsigned long)((ip0 & 0xFF) |
			                        ((ip1 & 0xFF) << 8) | ((ip2 & 0xFF) << 16) |
			                        ((ip3 & 0xFF) << 24));
		}
		else
		{
			*dns1 = (unsigned long)((ip0 & 0xFF) |
			                        ((ip1 & 0xFF) << 8) | ((ip2 & 0xFF) << 16) |
			                        ((ip3 & 0xFF) << 24));
			break;
		}
	}

	fclose(fp);

	return 0;
}

/**
 * net_dev_change_gateway_priority - net device change priority of gateway.
 * @dev_name: device name.
 *
 * return 0 if success; otherwise failed.
 */
static int net_dev_change_gateway_priority(const char *dev_name)
{
	unsigned long gateway;
	int ret;
	struct in_addr in_dst, in_gw;

	ret = net_dev_get_gateway(dev_name, &gateway);

	if (ret)
	{
		ALOGE("failed to get gateway for %s\n", dev_name);
		goto exit;
	}

	ret = net_dev_remove_gateway(dev_name);

	if (ret)
	{
		ALOGE("failed to remove default route for net device %s",
		      dev_name);
		goto exit;
	}

	in_dst.s_addr = 0;
	in_gw.s_addr = gateway;

	ret = ifc_manage_ipv4_route_priorty(SIOCADDRT, dev_name,
	                                    in_dst, 0, in_gw);

	if (ret)
	{
		ALOGE("failed to set priority of gateway for net device %s",
		      dev_name);
		goto exit;
	}

exit:
	return ret;
}

/**
 * net_dev_change_route_priority - net device change priority of route.
 * @dev_name: device name.
 *
 * return 0 if success; otherwise failed.
 */
static int net_dev_change_route_priority(const char *dev_name)
{
	int ret, prefix_length, division_result, remainder, tmp;
	unsigned long ip_addr, netmask;
	struct in_addr in_dst, in_gw;

	ret = net_dev_get_ip_addr(dev_name, &ip_addr);

	if (ret)
	{
		ALOGE("failed to get IP Address for %s", dev_name);
		goto exit;
	}

	ret = net_dev_get_netmask(dev_name, &netmask);

	if (ret)
	{
		ALOGE("failed to get netmask for %s", dev_name);
		goto exit;
	}

	/* obtain network address */
	prefix_length = obtain_one_num(netmask);
	division_result = (32 - prefix_length) / 8;
	remainder = (32 - prefix_length) % 8;

	switch (division_result)
	{
		case 0:
			tmp = (0xFF000000 << remainder) | (0xFFFFFF);
			break;

		case 1:
			tmp = (0xFF0000 << remainder) | (0xFFFF);
			break;

		case 2:
			tmp = (0xFF00 << remainder) | (0xFF);
			break;

		case 3:
			tmp = 0xFF << remainder;
			break;

		default:
			ALOGE("failed to obtain network address %s", dev_name);
			goto exit;
	}

	in_dst.s_addr = ip_addr & tmp;

	in_gw.s_addr = 0;

	ret = ifc_act_on_ipv4_route(SIOCDELRT, dev_name, in_dst,
	                            prefix_length, in_gw);

	if (ret)
	{
		ALOGE("failed to reomve the route for net device %s",
		      dev_name);
		goto exit;
	}

	ret = ifc_manage_ipv4_route_priorty(SIOCADDRT, dev_name, in_dst,
	                                    prefix_length, in_gw);

	if (ret)
	{
		ALOGE("failed to set priority of route for net device %s",
		      dev_name);
		goto exit;
	}

exit:
	return ret;
}

/**
 * net_dev_set_static_ip - set static IP configuration.
 * @dev_name: device name.
 * @ip_config: static IP configuration.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_set_static_ip(const char *dev_name,
                          const ip_static_config_t *ip_config)
{
	unsigned long ip_addr, netmask, gateway, dns0, dns1;
	int ret, err = INT_MIN;

	if (!dev_name)
	{
		ALOGE("invalid net device name");
		err |= GSET_PARAM_ERR;
		goto exit;
	}

	if (!ip_config)
	{
		ALOGE("invalid static IP config");
		err |= GSET_PARAM_ERR;
		goto exit;
	}

	ip_addr = ((ip_config->ip_addr)[3] << 24) |
	          ((ip_config->ip_addr)[2] << 16) |
	          ((ip_config->ip_addr)[1] << 8) |
	          (ip_config->ip_addr)[0];
	netmask = ((ip_config->netmask)[3] << 24) |
	          ((ip_config->netmask)[2] << 16) |
	          ((ip_config->netmask)[1] << 8) |
	          (ip_config->netmask)[0];
	gateway = ((ip_config->gateway)[3] << 24) |
	          ((ip_config->gateway)[2] << 16) |
	          ((ip_config->gateway)[1] << 8) |
	          (ip_config->gateway)[0];
	dns0    = ((ip_config->dns0)[3] << 24) |
	          ((ip_config->dns0)[2] << 16) |
	          ((ip_config->dns0)[1] << 8) |
	          (ip_config->dns0)[0];
	dns1    = ((ip_config->dns1)[3] << 24) |
	          ((ip_config->dns1)[2] << 16) |
	          ((ip_config->dns1)[1] << 8) |
	          (ip_config->dns1)[0];

	ALOGI("ipconfig ip:%d.%d.%d.%d mask:%d.%d.%d.%d route:%d.%d.%d.%d"
	      " dns0:%d.%d.%d.%d dns1:%d.%d.%d.%d",
	      (ip_config->ip_addr)[0], (ip_config->ip_addr)[1],
	      (ip_config->ip_addr)[2], (ip_config->ip_addr)[3],
	      (ip_config->netmask)[0], (ip_config->netmask)[1],
	      (ip_config->netmask)[2], (ip_config->netmask)[3],
	      (ip_config->gateway)[0], (ip_config->gateway)[1],
	      (ip_config->gateway)[2], (ip_config->gateway)[3],
	      (ip_config->dns0)[0], (ip_config->dns0)[1],
	      (ip_config->dns0)[2], (ip_config->dns0)[3],
	      (ip_config->dns1)[0], (ip_config->dns1)[1],
	      (ip_config->dns1)[2], (ip_config->dns1)[3]);

	if (ip_addr)
	{
		ret = net_dev_set_ip_addr(dev_name, ip_addr);

		if (ret)
		{
			ALOGE("failed to set IP Address for %s", dev_name);
			err |= SET_IP_ERR;
		}
	}

	if (netmask)
	{
		ret = net_dev_set_netmask(dev_name, netmask);

		if (ret)
		{
			ALOGE("failed to set netmask for %s", dev_name);
			err |= SET_NETMASK_ERR;
		}
	}

	if (gateway)
	{
		ret = net_dev_set_gateway(dev_name, gateway);

		if (ret)
		{
			ALOGE("failed to set gateway for %s", dev_name);
			err |= SET_GATEWAY_ERR;
		}
	}

	if (dns0 || dns1)
	{
		ret = net_dev_set_dns_ip(dev_name, dns0, dns1);

		if (ret)
		{
			ALOGE("failed to set dns server's IP for %s", dev_name);
			err |= SET_DNS_ERR;
		}
	}

	if (err == INT_MIN)
	{
		err = 0;
	}

exit:
	return err;
}

/**
 * net_dev_get_static_ip - get static IP configuration.
 * @dev_name: device name.
 * @ip_config: static IP configuration.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_get_static_ip(const char *dev_name, ip_static_config_t *ip_config)
{
	unsigned long ip_addr, netmask, gateway, dns0, dns1;
	int ret, err = INT_MIN;

	if (!dev_name)
	{
		ALOGE("invalid net device name");
		err |= GSET_PARAM_ERR;
		goto exit;
	}

	if (!ip_config)
	{
		ALOGE("invalid static IP config");
		err |= GSET_PARAM_ERR;
		goto exit;
	}

	ret = net_dev_get_ip_addr(dev_name, &ip_addr);

	if (ret)
	{
		ALOGE("failed to get IP Address for %s", dev_name);
		err |= GET_IP_ERR;
	}

	ret = net_dev_get_netmask(dev_name, &netmask);

	if (ret)
	{
		ALOGE("failed to get netmask for %s", dev_name);
		err |= GET_NETMASK_ERR;
	}

	ret = net_dev_get_gateway(dev_name, &gateway);

	if (ret)
	{
		ALOGE("failed to get gateway for %s", dev_name);
		err |= GET_GATEWAY_ERR;
	}

	ret = net_dev_get_dns_ip(dev_name, &dns0, &dns1);

	if (ret)
	{
		ALOGD("not dns server's IP address for %s was set", dev_name);
	}

	(ip_config->ip_addr)[3] = (ip_addr >> 24) & 0xFF;
	(ip_config->ip_addr)[2] = (ip_addr >> 16) & 0xFF;
	(ip_config->ip_addr)[1] = (ip_addr >> 8) & 0xFF;
	(ip_config->ip_addr)[0] = ip_addr & 0xFF;

	(ip_config->netmask)[3] = (netmask >> 24) & 0xFF;
	(ip_config->netmask)[2] = (netmask >> 16) & 0xFF;
	(ip_config->netmask)[1] = (netmask >> 8) & 0xFF;
	(ip_config->netmask)[0] = netmask & 0xFF;

	(ip_config->gateway)[3] = (gateway >> 24) & 0xFF;
	(ip_config->gateway)[2] = (gateway >> 16) & 0xFF;
	(ip_config->gateway)[1] = (gateway >> 8) & 0xFF;
	(ip_config->gateway)[0] = gateway & 0xFF;

	(ip_config->dns0)[3]    = (dns0 >> 24) & 0xFF;
	(ip_config->dns0)[2]    = (dns0 >> 16) & 0xFF;
	(ip_config->dns0)[1]    = (dns0 >> 8) & 0xFF;
	(ip_config->dns0)[0]    = dns0 & 0xFF;

	(ip_config->dns1)[3]    = (dns1 >> 24) & 0xFF;
	(ip_config->dns1)[2]    = (dns1 >> 16) & 0xFF;
	(ip_config->dns1)[1]    = (dns1 >> 8) & 0xFF;
	(ip_config->dns1)[0]    = dns1 & 0xFF;

	ALOGD("ipconfig ip:%d.%d.%d.%d mask:%d.%d.%d.%d route:%d.%d.%d.%d"
	      " dns0:%d.%d.%d.%d dns1:%d.%d.%d.%d",
	      (ip_config->ip_addr)[0], (ip_config->ip_addr)[1],
	      (ip_config->ip_addr)[2], (ip_config->ip_addr)[3],
	      (ip_config->netmask)[0], (ip_config->netmask)[1],
	      (ip_config->netmask)[2], (ip_config->netmask)[3],
	      (ip_config->gateway)[0], (ip_config->gateway)[1],
	      (ip_config->gateway)[2], (ip_config->gateway)[3],
	      (ip_config->dns0)[0], (ip_config->dns0)[1],
	      (ip_config->dns0)[2], (ip_config->dns0)[3],
	      (ip_config->dns1)[0], (ip_config->dns1)[1],
	      (ip_config->dns1)[2], (ip_config->dns1)[3]);

	if (err == INT_MIN)
	{
		err = 0;
	}

exit:
	return err;
}

/**
 * net_dev_change_priority - net device change priority.
 * @dev_name: device name.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_change_priority(const char *dev_name)
{
	int ret;

	ret = net_dev_change_gateway_priority(dev_name);

	if (ret)
	{
		ALOGE("failed to change gateway priority for %s\n", dev_name);
		goto exit;
	}

	ret = net_dev_change_route_priority(dev_name);

	if (ret)
	{
		ALOGE("failed to change route priority for %s\n", dev_name);
		goto exit;
	}

exit:
	return ret;
}