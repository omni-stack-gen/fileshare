#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/if_ether.h>
#include <netinet/ether.h>
#include <net/ethernet.h>

#include <linux/if.h>
#include <linux/route.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <errno.h>

#include <pthread.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include "log.h"

bool DPReadChipID(unsigned char *pChipID)
{
    bool bret = false;
    int fd;
    if ((fd = open("/sys/bus/nvmem/devices/nvmem0/nvmem", O_RDWR)) < 0)
    {
        LOGE("open i2c-0 failed\n");
        return bret;
    }
    char buf[256] = {0};

    int ret = read(fd, buf, 256);
    if (ret == 256)
    {
        if(pChipID)
            memcpy(pChipID, buf, 6);

        LOGI("###############chip id = %02x %02x %02x %02x %02x %02x\n", buf[0xea], buf[0xeb], buf[0xec], buf[0xed], buf[0xee], buf[0xef]);
    }

    close(fd);
    return bret;
}

bool DpGetMacByNetcworkard(unsigned char *pMac, char *pnetworkcard)
{
    bool bret = false;
    struct ifreq ifreq;
    int sock;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        LOGE("GetMacAddr socket \n");
        return bret;
    }

    strcpy(ifreq.ifr_name, pnetworkcard ? pnetworkcard : "eth0");

    if (ioctl(sock, SIOCGIFHWADDR, &ifreq) < 0)
    {
        LOGE("GetMacAddr ioctl \n");
        close(sock);

        return DPReadChipID(pMac);
    }

    memcpy(pMac, ifreq.ifr_hwaddr.sa_data, 6);
    close(sock);

    bret = true;
    return bret;
}