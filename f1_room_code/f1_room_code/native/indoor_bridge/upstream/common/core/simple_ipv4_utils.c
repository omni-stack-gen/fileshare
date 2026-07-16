#include "simple_ipv4_utils.h"

#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

bool simple_ipv4_parse_host_order(const char *ip, uint32_t *out)
{
    struct in_addr addr;

    if (!ip || !out || inet_pton(AF_INET, ip, &addr) != 1)
    {
        return false;
    }

    *out = ntohl(addr.s_addr);
    return true;
}

bool simple_ipv4_get_if_host_order(const char *ifname, uint32_t *out)
{
    int fd;
    struct ifreq ifr;
    struct sockaddr_in *sin;

    if (!ifname || !out)
    {
        return false;
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        return false;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0)
    {
        close(fd);
        return false;
    }
    close(fd);

    sin = (struct sockaddr_in *)&ifr.ifr_addr;
    *out = ntohl(sin->sin_addr.s_addr);
    return true;
}

const char *simple_ipv4_to_str(uint32_t host_addr, char *buf, size_t buf_len)
{
    struct in_addr addr;

    if (!buf || buf_len == 0)
    {
        return "<null>";
    }

    addr.s_addr = htonl(host_addr);
    if (!inet_ntop(AF_INET, &addr, buf, (socklen_t)buf_len))
    {
        snprintf(buf, buf_len, "0x%x", host_addr);
    }
    return buf;
}
