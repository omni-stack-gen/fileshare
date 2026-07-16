/* net_utils.c */

#include <features.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

#include <net_utils/net_utils.h>

#define LOG_TAG	"net_utils"
#include <net_utils/debug.h>

/**
 * ifreq_init - init ifreq.
 * @dev_name: device name.
 * @ifr: pointer to ifreq struct.
 */
void ifreq_init(const char *name, struct ifreq *ifr)
{
	memset(ifr, 0, sizeof(struct ifreq));
	strncpy(ifr->ifr_name, name, IFNAMSIZ);
	ifr->ifr_name[IFNAMSIZ - 1] = 0;
}

/**
 * ifreq_set_flags - ifreq set flags.
 * @dev_name: device name.
 * @flags: flags to set.
 *
 * return 0 if success; otherwise failed.
 */
int ifreq_set_flags(const char *name, unsigned int flags)
{
	int fd, ret;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ALOGE("socket() failed: %s", strerror(errno));
		return -1;
	}

	ifreq_init(name, &ifr);

	ret = ioctl(fd, SIOCGIFFLAGS, &ifr);
	if (ret < 0) {
		ALOGE("SIOCGIFFLAGS failed: %s", strerror(errno));
		close(fd);
		return -1;
	}

	ifr.ifr_flags |= flags;

	ret = ioctl(fd, SIOCSIFFLAGS, &ifr);
	if (ret < 0) {
		ALOGE("SIOCSIFFLAGS failed: %s", strerror(errno));
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

/**
 * ifreq_clr_flags - ifreq clear flags.
 * @dev_name: device name.
 * @flags: flags to clear.
 *
 * return 0 if success; otherwise failed.
 */
int ifreq_clr_flags(const char *name, unsigned int flags)
{
	int fd, ret;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ALOGE("socket() failed: %s", strerror(errno));
		return -1;
	}

	ifreq_init(name, &ifr);

	ret = ioctl(fd, SIOCGIFFLAGS, &ifr);
	if (ret < 0) {
		ALOGE("SIOCGIFFLAGS failed: %s", strerror(errno));
		close(fd);
		return -1;
	}

	ifr.ifr_flags &= ~flags;

	ret = ioctl(fd, SIOCSIFFLAGS, &ifr);
	if (ret < 0) {
		ALOGE("SIOCSIFFLAGS failed: %s", strerror(errno));
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

/**
 * ifreq_get_flags - ifreq get flags.
 * @dev_name: device name.
 * @flags: the flags we get.
 *
 * return 0 if success; otherwise failed.
 */
int ifreq_get_flags(const char *name, unsigned int *flags)
{
	int fd, ret;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ALOGE("socket() failed: %s", strerror(errno));
		return -1;
	}

	ifreq_init(name, &ifr);

	ret = ioctl(fd, SIOCGIFFLAGS, &ifr);
	if (ret < 0) {
		ALOGE("SIOCGIFFLAGS failed: %s", strerror(errno));
		close(fd);
		return -1;
	}

	close(fd);

	*flags = ifr.ifr_flags;

	return 0;
}

/**
 * net_dev_open - open net device.
 * @dev_name: device name.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_open(const char *dev_name)
{
	if (ifreq_set_flags(dev_name, IFF_UP)) {
		ALOGE("failed to open net device");
		return -1;
	}

	return 0;
}

/**
 * net_dev_close - close net device.
 * @dev_name: device name.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_close(const char *dev_name)
{
	if (ifreq_clr_flags(dev_name, IFF_UP)) {
		ALOGE("failed to close net device");
		return -1;
	}

	return 0;
}

/**
 * net_dev_is_open - check if net device is opened.
 * @dev_name: device name.
 *
 * return 0 if device is closed; 1 if device is opened;
 * otherwise some error happened.
 */
int net_dev_is_open(const char *dev_name)
{
	int ret;
	unsigned int flags;

	ret = ifreq_get_flags(dev_name, &flags);
	if (ret) {
		ALOGE("failed to check if net device is opened");
		return -1;
	}

	return !!(flags & IFF_UP);
}

/**
 * net_dev_set_mac_addr - set net device MAC address.
 * @dev_name: device name.
 * @mac_addr: MAC address.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_set_mac_addr(const char *dev_name, const unsigned char *mac_addr)
{
	int fd, ret;
	struct ifreq ifr;

	if (!mac_addr) {
		ALOGE("invalid MAC address");
		return -1;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ALOGE("socket() failed: %s", strerror(errno));
		return -1;
	}

	ifreq_init(dev_name, &ifr);
	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	memcpy(&ifr.ifr_hwaddr.sa_data, mac_addr, 6);

	ret = ioctl(fd, SIOCSIFHWADDR, &ifr);
	if (ret < 0) {
		ALOGE("SIOCSIFHWADDR failed: %s", strerror(errno));
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

/**
 * net_dev_get_mac_addr - get net device MAC address.
 * @dev_name: device name.
 * @mac_addr: MAC address buffer.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_get_mac_addr(const char *dev_name, unsigned char *mac_addr)
{
	int fd, ret;
	struct ifreq ifr;

	if (!mac_addr) {
		ALOGE("invalid MAC address buffer");
		return -1;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ALOGE("socket() failed: %s", strerror(errno));
		return -1;
	}

	ifreq_init(dev_name, &ifr);

	ret = ioctl(fd, SIOCGIFHWADDR, &ifr);
	if (ret < 0) {
		ALOGE("SIOCGIFHWADDR failed: %s", strerror(errno));
		close(fd);
		return -1;
	}

	close(fd);

	memcpy(mac_addr, ifr.ifr_hwaddr.sa_data, 6);

	return 0;
}
