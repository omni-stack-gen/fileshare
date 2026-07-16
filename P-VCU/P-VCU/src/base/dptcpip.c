#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/if_ether.h>
#include <linux/if.h>
#include <linux/route.h>

#include "dplib.h"
// #include "dpnetwork.h"

static unsigned int GetTickCount()
{
    struct timespec t_start = {0};
	clock_gettime(CLOCK_MONOTONIC, &t_start);
	return (t_start.tv_sec * 1000 + t_start.tv_nsec / 1000000);
}

void SocketClose(int sock)
{
    if(sock != INVALID_SOCKET)
    {
        close(sock);
        sock = INVALID_SOCKET;
    }
}

bool SocketUnblock(int sock)
{
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    return true;
}

int TcpConnectWait(int lsock, unsigned int timeout)
{
    struct timeval tv_out;
    fd_set fWR;
    int ret;

    tv_out.tv_sec = timeout / 1000;
    tv_out.tv_usec = (timeout % 1000) * 1000;
    FD_ZERO(&fWR);
    FD_SET(lsock, &fWR);
    ret = select(lsock + 1, NULL, &fWR, NULL, &tv_out);

    if(ret == 0)
    {
        //time out
        printf("Socket1 select timeout \r\n");
        return -1;
    }

    if(ret == SOCKET_ERROR)
    {
        printf("Socket1 connect select error %d\r\n", errno);
        return -1;
    }

    if(FD_ISSET(lsock, &fWR))
    {
        return 0;
    }

    printf("Should not reach hear %d\r\n", errno);
    return -1;
}

int TcpConnectTry(unsigned long ip, short port)
{
    int ret;
    int m_sock;
    struct sockaddr_in sin;

    m_sock = socket(AF_INET, SOCK_STREAM, 0);

    if(m_sock == INVALID_SOCKET)
    {
        printf("Socket create error %d\r\n", errno);
        return INVALID_SOCKET;
    }

    // set unblock mode
    SocketUnblock(m_sock);

    memset((char *)&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = ip;

    // begin connect
    ret = connect(m_sock, (struct sockaddr *)&sin, sizeof(sin));

    if(ret != SOCKET_ERROR)
    {
        printf("Socket connect is OK\r\n");
        return m_sock;
    }

    if(errno == EINPROGRESS)
    {
        printf("Socket connect need select\r\n");
        return m_sock;
    }
    else
    {
        SocketClose(m_sock);
        printf("Socket connect %d\r\n", errno);
        return INVALID_SOCKET;
    }
}

int TcpConnectStr(char *rip, int rport, int timeout)
{
    int ret;
    int err = 0;
    socklen_t errlen = sizeof(err);
    struct sockaddr_in sin;
    struct timeval tv_out;
    fd_set fdR;
    fd_set fdW;
    int m_sock;

    m_sock = socket(AF_INET, SOCK_STREAM, 0);

    if(m_sock == INVALID_SOCKET)
    {
        printf("Socket create error %d\r\n", errno);
        return INVALID_SOCKET;
    }

    SocketUnblock(m_sock);

    memset((char *)&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(rport);
    sin.sin_addr.s_addr = inet_addr(rip);

    ret = connect(m_sock, (struct sockaddr *)&sin, sizeof(sin));

    if(ret == SOCKET_ERROR)
    {
        if(errno == EINPROGRESS)
        {
            tv_out.tv_sec = timeout / 1000;
            tv_out.tv_usec = (timeout % 1000) * 1000;

            FD_ZERO(&fdR);
            FD_ZERO(&fdW);

            FD_SET(m_sock, &fdR);
            FD_SET(m_sock, &fdW);

            ret = select(m_sock + 1, &fdR, &fdW, NULL, &tv_out);

            /* select调用失败*/
            if(ret < 0)
            {
                printf("select error %s \n", strerror(errno));
                SocketClose(m_sock);
                return -1;
            }

            /* 超时处理 */
            if(ret == 0)
            {
                printf("connect %s timeout \n", rip);
                SocketClose(m_sock);
                return -1;
            }

            /* 当连接成功建立时，描述符变成可写，ret == 1 */
            if(ret == 1 && FD_ISSET(m_sock, &fdW))
            {
                return m_sock;
            }

            /*当连接建立遇到错误时，描述符变为即可读，也可写，ret == 2 遇到这种情况，可调用getsockopt函数*/
            if(ret == 2)
            {
                if(getsockopt(m_sock, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1)
                {
                    printf("getsockopt(SO_ERROR): %s \n", strerror(errno));
                    SocketClose(m_sock);
                    return -1;
                }

                if(err)
                {
                    errno = err;
                    printf("connect %s error:%s \n", rip, strerror(errno));
                    SocketClose(m_sock);
                    return -1;
                }
            }
        }
        else
        {
            SocketClose(m_sock);
            printf("Socket connect %s %d errno %d\r\n", rip, rport, errno);
            return INVALID_SOCKET;
        }
    }

    return m_sock;
}

int TcpConnect(int rip, int rport, int timeout)
{
    char szMCip[256];
    sprintf(szMCip, "%d.%d.%d.%d", (rip & 0xFF), (rip >> 8) & 0xFF, (rip >> 16) & 0xFF, (rip >> 24) & 0xFF);
    return TcpConnectStr(szMCip, rport, timeout);
}

int TcpListen(char *ipaddr, short port)
{
    struct sockaddr_in sin;
    int socklisten;

    socklisten = socket(AF_INET, SOCK_STREAM, 0);

    if(socklisten == INVALID_SOCKET)
    {
        printf("socket create error %d\n", errno);
        return INVALID_SOCKET;
    }

    SocketUnblock(socklisten);  /*不阻塞*/

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    if(ipaddr != NULL)
        sin.sin_addr.s_addr = inet_addr(ipaddr);
    else
        sin.sin_addr.s_addr = htonl(INADDR_ANY);

    // int reuse_addr_opt = 1;
    // setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr_opt, sizeof(int));
    SetSockReuse(socklisten);

    if(bind(socklisten, (struct sockaddr *)&sin, sizeof(struct sockaddr)) == -1)
    {
        printf("socket bind error %d\n", errno);
        SocketClose(socklisten);
        return INVALID_SOCKET;
    }

    if(listen(socklisten, 2) == -1)
    {
        printf("socket listen error %d\n", errno);
        SocketClose(socklisten);
        return INVALID_SOCKET;
    }

    return socklisten;
}

int TcpAccpet(int socklisten, unsigned int timeout, struct sockaddr_in *psin)
{
    int ret;
    struct timeval tv_out;
    fd_set readfds;
    struct sockaddr_in sin;
    socklen_t nnn;

    tv_out.tv_sec = timeout / 1000;
    tv_out.tv_usec = (timeout % 1000) * 1000;

    FD_ZERO(&readfds);
    FD_SET(socklisten, &readfds);

    ret = select(socklisten + 1, &readfds, NULL, NULL, &tv_out);

    if(ret == 0)
    {
        return INVALID_SOCKET;
    }

    if(ret == SOCKET_ERROR)
    {
        printf("Socket accept select error %d\n", errno);
        return INVALID_SOCKET;
    }

    nnn = sizeof(sin);

    if(psin == NULL)
        psin = &sin;

    ret = accept(socklisten, (struct sockaddr *)psin, &nnn);

    if(ret == INVALID_SOCKET)
    {
        printf("Socket accept error %d\n", errno);
    }

    return ret;
}

int TcpSendData(int sock, char *pdata, int dlen, int timeout)
{
    int ret = 0;
    int i = 0;
    struct timeval tv_out;
    fd_set writefds;
    unsigned int starttick =  GetTickCount();
    unsigned int curtick;
    unsigned int maxout = timeout;

    while(i < dlen)
    {
        tv_out.tv_sec = timeout / 1000;
        tv_out.tv_usec = (timeout % 1000) * 1000;

        FD_ZERO(&writefds);
        FD_SET(sock, &writefds);

        ret = select(sock + 1, NULL, &writefds, NULL, &tv_out);

        if(ret == 0)
        {
            //time out
            return -1;
        }

        if(ret == SOCKET_ERROR)
        {
            return -2;
        }

        ret = send(sock, pdata + i, dlen - i, 0);

        if(ret == SOCKET_ERROR)
        {
            if(EALREADY != errno)
                return -3;
        }
        else
            i += ret;

        curtick =  GetTickCount();

        if((curtick - starttick) >= maxout)
            break;

        timeout = maxout - (curtick - starttick);
    }

    return i;
}

int TcpRecvData(int sock, char *pdata, int dlen, int timeout)
{
    int ret = 0;
    int i = 0;
    struct timeval tv_out;
    fd_set readfds;
    unsigned int starttick =  GetTickCount();
    unsigned int curtick;
    unsigned int maxout = timeout;

    while(i < dlen)
    {
        tv_out.tv_sec = timeout / 1000;
        tv_out.tv_usec = (timeout % 1000) * 1000;

        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        ret = select(sock + 1, &readfds, NULL, NULL, &tv_out);

        if(ret == 0)
        {
            //time out
            printf("timeout\r\n");
            return i;
        }

        if(ret == SOCKET_ERROR)
        {
            printf("select SOCKET_ERROR\r\n");
            return i;
        }

        if(FD_ISSET(sock, &readfds))
        {
            ret = recv(sock, pdata + i, dlen - i, 0);

            if(ret == SOCKET_ERROR)
            {
                printf("recv SOCKET_ERROR\r\n");

                if(EALREADY != errno)
                    return i;
            }
            else if(ret == 0)
            {
                //printf("recv 0\r\n");
                return i;
            }
            else
                i += ret;
        }

        curtick =  GetTickCount();

        if((curtick - starttick) >= maxout)
            break;

        timeout = maxout - (curtick - starttick);
    }

    return i;
}

int TcpRecvDataTry(int sock, char *pdata, int dlen, int timeout)
{
    int ret = 0;
    struct timeval tv_out;
    fd_set readfds;

    tv_out.tv_sec = timeout / 1000;
    tv_out.tv_usec = (timeout % 1000) * 1000;

    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    ret = select(sock + 1, &readfds, NULL, NULL, &tv_out);

    if(ret == 0)
    {
        //time out
        printf("recv data timeout\r\n");
        return 0;
    }

    if(ret == SOCKET_ERROR)
    {
        printf("select SOCKET_ERROR\r\n");
        return -1;
    }

    if(FD_ISSET(sock, &readfds))
    {
        ret = recv(sock, pdata, dlen, 0);

        if(ret == SOCKET_ERROR)
        {
            printf("recv SOCKET_ERROR\r\n");

            if(EALREADY != errno)
                return -1;
        }
        else if(ret == 0)
        {
            //printf("recv 0\r\n");
            return -1;
        }
        else
            return ret;
    }

    return 0;
}

int UdpCreate(short port)
{
    int sockfd;
    struct sockaddr_in servaddr;
    int b = 1;

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
    {
        printf("socket create error");
        return INVALID_SOCKET;
    }

    SocketUnblock(sockfd);

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&b, sizeof(b)) == -1)
    {
        printf("udp_create: socket SO_REUSEADDR error");
        SocketClose(sockfd);
        return INVALID_SOCKET;
    }

    /* init servaddr */
    memset((char *)&servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    /* bind address and port to socket */
    if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        printf("bind error");
        SocketClose(sockfd);
        return INVALID_SOCKET;
    }

    return sockfd;
}

int UdpCreateEx(short port, const char *eth_name, bool bBlock)
{
    int sockfd;
    struct sockaddr_in servaddr;
    int b = 1;

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
    {
        printf("socket create error");
        return INVALID_SOCKET;
    }

    /* set unblock */
    if(!bBlock)
        SocketUnblock(sockfd);

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&b, sizeof(b)) == -1)
    {
        printf("udp_create: socket SO_REUSEADDR error");
        SocketClose(sockfd);
        return INVALID_SOCKET;
    }

    /* init servaddr */
    memset((char *)&servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    /* bind address and port to socket */
    if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        printf("bind error");
        SocketClose(sockfd);
        return INVALID_SOCKET;
    }

    SockBindDevice(sockfd, eth_name);
    return sockfd;
}

int UdpCreate1()
{
    int sockfd;

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        printf("socket create error");
        return INVALID_SOCKET;
    }

    SocketUnblock(sockfd);
    return sockfd;
}

int UdpSend(int socket_fd, const char *rip, short port, char *buf, int len)
{
    struct sockaddr_in my_addr;

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = inet_addr(rip);
    memset((char *)&my_addr.sin_zero, 0, 8);

    // [1] UDP协议本身，UDP协议中有16位的UDP报文长度，那么UDP报文长度不能超过2^16=65536.
    // [2] 以太网(Ethernet)数据帧的长度，数据链路层的MTU(最大传输单元)。
    // [3] socket的UDP发送缓存区大小

    // 根据UDP协议，从UDP数据包的包头可以看出，UDP的最大包长度是2^16-1的个字节。由于UDP包头占8个字节，
    // 而在IP层进行封装后的IP包头占去20字节，所以这个是UDP数据包的最大理论长度是2^16 - 1 - 8 - 20 = 65507字节。
    // 如果发送的数据包超过65507字节，send或sendto函数会错误码1(Operation not permitted， Message too long)，
    // 当然啦，一个数据包能否发送65507字节，还和UDP发送缓冲区大小（linux下UDP发送缓冲区大小为：cat /proc/sys/net/core/wmem_default）相关，
    // 如果发送缓冲区小于65507字节，在发送一个数据包为65507字节的时候，
    // send或sendto函数会错误码1(Operation not permitted， No buffer space available)。

    // printf("udp send %s %d %d \r\n", rip, port, len);
    if(len > 65507)
    {
        printf("Msg Too Long Udp Send %s %d %d \n", rip, port, len);
        return -1;
    }

    if(sendto(socket_fd, buf, len, 0, (struct sockaddr *)&my_addr, sizeof(my_addr)) != len)
    {
        printf("udp_send error ip:%s port:%d, len:%d error:%s \n", rip, port, len, strerror(errno));
        return -1;
    }

    return 0;
}

int UdpSend1(int socket_fd, int rip, short port, char *buf, int len)
{
    struct sockaddr_in my_addr;

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = rip;
    memset((char *)&my_addr.sin_zero, 0, 8);

    if(sendto(socket_fd, buf, len, 0, (struct sockaddr *)&my_addr, sizeof(my_addr)) != len)
    {
        printf("udp_send error, len:%d\r\n", len);
        return -1;
    }

    return 0;
}

int UdpRecv(int socketid, char *buf, int len, int itimeout, int *remoteip)
{
    int retlen = 0;
    struct sockaddr_in user_addr;
    socklen_t usize;

    usize = sizeof(user_addr);

    if(itimeout > 0)
    {
        fd_set fdread;
        int rc;
        struct timeval timeout;

        timeout.tv_sec = itimeout / 1000;
        timeout.tv_usec = (itimeout % 1000) * 1000;
        FD_ZERO(&fdread);
        FD_SET(socketid, &fdread);

        rc = select(socketid + 1, &fdread, NULL, NULL, &timeout);

        if(rc < 0)
            return -1;

        if(rc == 0)
            return -1;

        retlen = recvfrom(socketid, buf, len, 0, (struct sockaddr *)&user_addr, &usize);

        if(retlen <= 0)
            return -1;

        if(remoteip != NULL)
            *remoteip = user_addr.sin_addr.s_addr;
    }
    else
    {
        retlen = recvfrom(socketid, buf, len, 0, (struct sockaddr *)&user_addr, &usize);

        if(remoteip != NULL)
            *remoteip = user_addr.sin_addr.s_addr;
    }

    return retlen;
}

int UdpRecvEx(int sockfd, char *buf, int len, int itimeout, struct sockaddr_in *from)
{
    int retlen = 0;
    socklen_t socklen = sizeof(struct sockaddr_in);

    if(from == NULL)
        return -1;

    if(itimeout > 0)
    {
        fd_set fdread;
        int rc;

        struct timeval timeout;
        timeout.tv_sec = itimeout / 1000;
        timeout.tv_usec = (itimeout % 1000) * 1000;

        FD_ZERO(&fdread);
        FD_SET(sockfd, &fdread);

        rc = select(sockfd + 1, &fdread, NULL, NULL, &timeout);

        if(rc < 0)
            return -1;
        else if(rc == 0)
            return 0;

        if(FD_ISSET(sockfd, &fdread))  //检查描述符是否在这个集合里面
        {
            retlen = recvfrom(sockfd, buf, len, 0, (struct sockaddr *)from, &socklen);

            if(retlen <= 0)
                return -1;
        }

        return -1;
    }
    else
    {
        retlen = recvfrom(sockfd, buf, len, 0, (struct sockaddr *)from, &socklen);
    }

    return retlen;
}

int SocketSelect(int *pSock, int nSock, int timeout)
{
    fd_set fd;
    FD_ZERO(&fd);
    struct timeval tv_out;
    int i;
    int ret;

    int maxSock = pSock[0];

    for(i = 0; i < nSock; i++)
    {
        FD_SET(pSock[i], &fd);

        if(maxSock < pSock[i])
            maxSock = pSock[i];
    }

    tv_out.tv_sec = timeout / 1000;
    tv_out.tv_usec = (timeout % 1000) * 1000;
    ret = select(maxSock + 1, &fd, NULL, NULL, &tv_out);

    if(ret < 0)
        return -1;
    else if(ret == 0)
        return 0; // timeOut
    else
    {
        for(i = 0; i < nSock; i++)
        {
            if(FD_ISSET(pSock[i], &fd))
                return i + 1;
        }
    }

    return -2;
}

bool UdpSetRecvBuf(int sockfd, int length)
{
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char *)&length, sizeof(length)) < 0)
    {
        printf("ERROR: setsockopt(SO_RCVBUF): %d\n", errno);
        return false;
    }

    return true;
}

bool UdpGroupTTL(int sockfd, int mcastTTL)
{
    if(setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (const char *)&mcastTTL, sizeof(mcastTTL)) < 0)
    {
        printf("ERROR: setsockopt(IP_MULTICAST_TTL): %d\n", errno);
        return false;
    }

    return true;
}

bool UdpJoinGroup(int sockfd, int ip)
{
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = ip;
    mreq.imr_interface.s_addr = INADDR_ANY;

    if(setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&mreq, sizeof(mreq)) < 0)
    {
        printf("ERROR: setsockopt(IP_ADD_MEMBERSHIP): %d\n", errno);
        return false;
    }

    return true;
}

bool UdpLeaveGroup(int sockfd, int ip)
{
    struct ip_mreq mreq;

    mreq.imr_multiaddr.s_addr = ip;
    mreq.imr_interface.s_addr = INADDR_ANY;

    if(setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const char *)&mreq, sizeof(mreq)) < 0)
    {
        printf("ERROR: setsockopt(IP_DROP_MEMBERSHIP): %d\n", errno);
        return false;
    }

    return true;
}

bool SockBindDevice(int sockfd, const char *eth_name)
{
    struct ifreq ifreq;
    strncpy(ifreq.ifr_name, eth_name, IFNAMSIZ-1);

    if(setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifreq, sizeof(ifreq)) < 0)
    {
        printf("ERROR: SO_BINDTODEVICE fail:%d\r\n", errno);
        return false;
    }

    return true;
}

//设置一个6网段的随机ip
int SetOneRandIP(void)
{
	int iret=0;
	// char strip[32] = {0};

	// do
	// {
	// 	srand(time(NULL));
	// 	sprintf(strip, "192.168.6.%d", rand()%255);

	// } while (arp_check_ip(0, (int)inet_addr(strip), &iret));

	// printf( "<SetOneRandIP>------------ %s \n", strip);
	// //SetIPAddress(strip, "255.255.255.0", "192.168.6.1", "eth0");
	return iret;
}

bool SetNetDhcp(void)
{
#ifndef  DEBUG
    //int iret =  system("udhcpc -q -i eth0");
    //printf("<DHCP>============= %d \n", iret);
#endif
    return SetNetDhcpByPopen();
}

bool SetNetDhcpByPopen(void)
{
    char result_buf[256] = {0};

    // 用于接收命令返回值
    int rc = 0;

    // 执行预先设定的命令，并读出该命令的标准输出
    FILE *fp;
    unsigned int starttick =  GetTickCount();
    fp = popen("udhcpc -q -n eth0", "r");

    if(NULL == fp)
    {
        perror("popen fial\n");
        return false;
    }

    while(fgets(result_buf, sizeof(result_buf), fp) != NULL)
    {
        // 为了下面输出好看些，把命令返回的换行符去掉
        if('\n' == result_buf[strlen(result_buf) - 1])
        {
            result_buf[strlen(result_buf) - 1] = '\0';
        }

        printf("output [%s] %u %u \n", result_buf, GetTickCount(), starttick);

        if(( GetTickCount() - starttick) > 3000)
            break;
    }

    //等待命令执行完毕并关闭管道及文件指针
    rc = pclose(fp);

    if(0 != rc)
    {
        perror("close fail\n");
        return false;
    }
    else
    {
        printf("cmd:status[%d] returnvalue [%d]", rc, WEXITSTATUS(rc));
    }

    return true;
}

bool SetIPAddress(const char *pIpAddr, const char *pMask, const char *pGateway, const char *pNetCardName)
{
    struct sockaddr_in sin;
    struct ifreq ifr;

    printf("SetIPAddress %s %s %s\n", pIpAddr, pMask, pGateway);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (0 > fd)
    {
        printf("%s(%d): socket error: %d \n", __FILE__, __LINE__, errno);
        return false;
    }

    // eth0
    strcpy(ifr.ifr_name, pNetCardName != NULL ? pNetCardName : "eth0");
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;

    //  IP
    sin.sin_addr.s_addr = inet_addr(pIpAddr == NULL ? "0" : pIpAddr);
    memcpy(&ifr.ifr_addr, &sin, sizeof(sin));

    if (ioctl(fd, SIOCSIFADDR, &ifr) < 0)
    {
        printf("%s(%d): SIOCSIFADDR error: %s \n", __FILE__, __LINE__, strerror(errno));
        close(fd);
        return false;
    }

    // MASK
    sin.sin_addr.s_addr = inet_addr(pMask == NULL ? "255.255.255.0" : pMask);
    memcpy(&ifr.ifr_addr, &sin, sizeof(sin));

    if (ioctl(fd, SIOCSIFNETMASK, &ifr) < 0)
    {
        printf("%s(%d): SIOCSIFNETMASK errno: %s \n", __FILE__, __LINE__, strerror(errno));
        close(fd);
        return false;
    }

    struct rtentry rt;
    memset(&rt, 0, sizeof(struct rtentry));
    rt.rt_dst.sa_family = AF_INET;
    ((struct sockaddr_in *)&rt.rt_dst)->sin_addr.s_addr = 0;
    rt.rt_genmask.sa_family = AF_INET;
    ((struct sockaddr_in *)&rt.rt_genmask)->sin_addr.s_addr = 0;

    if (pGateway)
    {
        sin.sin_addr.s_addr = inet_addr(pGateway);
        memcpy(&rt.rt_gateway, &sin, sizeof(sin));
        ((struct sockaddr_in *)&rt.rt_dst)->sin_family = AF_INET;
        ((struct sockaddr_in *)&rt.rt_genmask)->sin_family = AF_INET;
        rt.rt_flags = RTF_UP | RTF_GATEWAY;

        if (ioctl(fd, SIOCADDRT, &rt) < 0)
        {
            printf("%s(%d): Gateway SIOCADDRT errno:%s \n", __FILE__, __LINE__, strerror(errno));
        }
    }

    close(fd);

    char cmdline[256] = {0};
    sprintf(cmdline, "/sbin/route add -net 224.0.0.0 netmask 240.0.0 dev %s", pNetCardName == NULL ? "eth0" : pNetCardName);
    system(cmdline);

    return true;
}

static void IPAddrToStr(char *szStr, unsigned int IPAddr)
{
	sprintf(szStr, "%d.%d.%d.%d", IPAddr & 0xFF, (IPAddr >> 8) & 0xFF,
			(IPAddr >> 16) & 0xFF, (IPAddr >> 24) & 0xFF);
}

bool SetIPAddressByInt(int ip, int mask, int gw, const char *pNetCardName)
{
    char IpAddr[20], MaskAddr[20], GwAddr[20];
    IPAddrToStr(IpAddr, ip);
    IPAddrToStr(MaskAddr, mask);
    IPAddrToStr(GwAddr, gw);
    return SetIPAddress(IpAddr, MaskAddr, GwAddr, pNetCardName);
}

int GetNetWorkCardAddr(const char *pNetCardName)
{
	struct sockaddr_in* paddr;
	struct ifreq ifr;
	int sockfd;
	int ret;

	strcpy(ifr.ifr_name, pNetCardName);

    sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if(sockfd < 0)
		return -1;

	ret = ioctl(sockfd, SIOCGIFADDR, &ifr);
	if (ret < 0)
	{
        printf("ioctl error:SIOCGIFADDR: %d \n", errno);
        close(sockfd);
		return -1;
	}

    paddr = (struct sockaddr_in *)&(ifr.ifr_addr);
	close(sockfd);

	return (int)paddr->sin_addr.s_addr;
}

void SetSockReuse(int hsock)
{
    //SO_REUSeADDR选项：重用本地地址
    //未设置此项前，若服务端开启后，又关闭，此时sock处于TIME_WAIT状态，与之绑定的socket地址不可重用，而导致再次开启服务端失败。
    //经过setsockopt设置之后， 即使处于TIME_WAIT些状态也可以立即被重用。
    int reuse = 1;
    if(setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) == -1)
    {
        printf("SetSockReuse SO_REUSEADDR error\n");
    }
}