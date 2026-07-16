#ifndef	_DPLIB_H_
#define	_DPLIB_H_
#include <netinet/in.h>

#ifdef __cplusplus
extern "C"
{
#endif
// typedef int (*Audiofun) (unsigned int userData, char * pdata, int dlen);

#define	INVALID_SOCKET	-1
#define	SOCKET_ERROR	-1

#define GPIO_DIRECTION_IN  0
#define GPIO_DIRECTION_OUT 1

// int PlayWaveStart(char* fname);
// void PlayWaveStop(int hwave);
// int AudioStart(Audiofun dwInCallback, unsigned int userData);
// int AudioStop(int hwave);
// void AudioSetVolume(int inout, int Volume);
// void AudioWrite(char * lpdata, int dlen);

// int StartEnc(int width, int height, Audiofun dwInCallback);
// int StopEnc(int chn);

void SocketClose(int sock);
bool SocketUnblock(int sock);
int TcpConnectWait(int lsock, unsigned int timeout);
int TcpConnectTry(unsigned long ip, short port);
int TcpConnectStr(char * rip, int rport, int timeout);
int TcpConnect(int rip, int rport, int timeout);
int TcpListen(char* ipaddr, short port);
int TcpAccpet(int socklisten, unsigned int timeout, struct sockaddr_in* psin);
int TcpSendData(int sock, char *pdata, int dlen, int timeout);
int	TcpRecvData(int sock, char *pdata, int dlen, int timeout);
int	TcpRecvDataTry(int sock, char *pdata, int dlen, int timeout);
int UdpCreate(short port);
int UdpCreateEx(short port, const char* eth_name, bool bBlock);
int UdpCreate1();
int UdpSend(int socket_fd, const char *rip, short port, char* buf, int len);
int UdpSend1(int socket_fd, int rip, short port, char* buf, int len);
int UdpRecv(int socketid, char* buf, int len, int itimeout, int* remoteip);
int UdpRecvEx(int sockfd, char *buf, int len, int itimeout, struct sockaddr_in *from);
int SocketSelect(int* pSock, int nSock, int timeout);
bool UdpSetRecvBuf(int sockfd, int length);
bool UdpGroupTTL(int sockfd, int mcastTTL);
bool UdpJoinGroup(int sockfd, int ip);
bool UdpLeaveGroup(int sockfd, int ip);
bool SockBindDevice(int sockfd, const char* eth_name);

bool SetIPAddress(const char *pIpAddr, const char *pMask, const char *pGateway, const char *pNetCardName);
bool SetIPAddressByInt(int ip, int mask, int gw, const char *pNetCardName);
int SetOneRandIP(void);
bool SetNetDhcp(void);
bool SetNetDhcpByPopen(void);
int GetNetWorkCardAddr(const char *pNetCardName);
void SetSockReuse(int hsock);

int GpioInit(int gpioId,unsigned char direction);
void GpioDeInit(int hHandle);
int GpioSetValue(int hHandle, unsigned char status);
int GpioGetValue(int hHandle, unsigned char* pStatus);

void test_gpio_blink(unsigned int pin);
void test_gpio_button(void);

#ifdef __cplusplus
}
#endif
#endif


