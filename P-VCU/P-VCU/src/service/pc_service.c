#include "pc_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dplib.h>
#include <dpbase.h>
#include <dpbaselib.h>
#include <sys/sys_event.h>

#include <utils/log.h>
#include <myconfig.h>

#define PCMSG_PORT 0x8868

#define	PCMSG_VERIFY			0x10000000
#define	PCMSG_SET_ROOM			0x10000001
#define	PCMSG_SWITCH_WATCHDOG	0x10000002
#define	PCMSG_UPDATE_NETCFG		0x10000003
#define	PCMSG_UPDATE_IMAGEDD	0x10000004
#define	PCMSG_REBOOT			0x10000005
#define	PCMSG_RESET				0x10000006
#define	PCMSG_CAP_PICTURE		0x10000007
#define PCMSG_SET_CLOUDINFO		0x1000000a
#if 1
typedef struct
{
	unsigned short devtype[32];
	unsigned int type;
	unsigned int length;
} PCMSG_DATA;

static bool g_bPCStart = false;
//线程句柄
static pthread_t thread;


bool DownLoadFile(int sockfd, char* filename, int nFileSize)
{
	FILE* pFile = fopen(filename, "wb");

	if(NULL == pFile)
		return false;

	int nDownLoadSize = 0x10000;

	char* pdata = (char*)malloc(nDownLoadSize);

	if(NULL == pdata)
	{
		LOGI("DownLoadFile fail");
		fclose(pFile);
		return false;
	}

	int nRecvSize = 0;
	int nCompleteSize = 0;

	while(nCompleteSize < nFileSize)
	{
		nRecvSize = nFileSize - nCompleteSize;
		if(nRecvSize > nDownLoadSize)
			nRecvSize = nDownLoadSize;

		if(TcpRecvData(sockfd, pdata, nRecvSize, 2000) != nRecvSize)
			break;

		LOGI("DownLoadFile %s %d %d %d\n", filename, nFileSize, nCompleteSize, nRecvSize);

		nCompleteSize += nRecvSize;
		fwrite(pdata, 1, nRecvSize, pFile);
	}

	SAFE_FREE(pdata);
	fclose(pFile);
	return (nCompleteSize == nFileSize);
}

static void HandleUpdateImage(int sockfd, PCMSG_DATA* pMsg)
{
	int ret = false;

	//DPPostMessage(MSG_START_FROM_ROOT, UPGRATE_APPID, 0, 0);
#ifdef ENABLE_SPINAAD
	upgrade_info info = {0};
	if(TcpRecvData(sockfd, (char*)&info, sizeof(info), 2000) == sizeof(info))
	{
		printf("recv upgrade info updatetype:%d \nsoftCo:%s \nfilemd5:%s \nsoftver:%s \nhavereg:%d \nendiantype:%d\n",
			info.updatetype, info.softCo, info.filemd5, info.softver, info.havereg, info.endiantype);
		pMsg->length -= sizeof(info);
		if(strncmp(info.softCo, MODEL_NAME, 6) != 0)
		{
			printf("recev head is not match, DevType:%s, not match:%s\n", info.softCo, MODEL_NAME);
			return;
		}
	}
#endif
	do
	{
		////NX5 不能真正关闭看门狗
		//DPPostMessage(MSG_SYSTEM, WATCHDOG_CHANGE, false, 0);

		ret = DownLoadFile(sockfd, (char*)DOWNLOAD_IMAGE_PATH, pMsg->length);
	}while(0);

	TcpSendData(sockfd, (char*)&ret, 4, 1000);

	if(!ret)
	{
		printf("DownLoadFile fail\n");
		DPDeleteFile(DOWNLOAD_IMAGE_PATH);
		//DPPostMessage(MSG_SYSTEM, WATCHDOG_CHANGE, true, 0);
		//DPPostMessage(MSG_PRIVATE, UPGRATE_APPID, 0, UPDATE_FILE_ERR);
	}
	else
	{
#ifdef ENABLE_SPINAAD
		printf("DownLoadFile success\n");
		void Md5_File(char *pfilename, char *pOutBuf);
		char md5[64] = {0};
		Md5_File(DOWNLOAD_IMAGE_PATH, md5);
		LOGI("md5:%s \n", md5);
		if (strcmp(md5, info.filemd5) == 0)
		{
			printf("md5 is match\n");
		}
		else
		{
			printf("md5 is not match\n");
			return;
		}
#endif
		//DPPostMessage(MSG_PRIVATE, UPGRATE_APPID, 0, UPDATE_FILE_START);
		sys_event_t e = {0};
		SYS_EVENT_INIT_BASE(e, SYS_EVENT_BASE_UPGRADE);
		sys_event_publish(&e);
	}
}

static void *pc_process_thread(void *args)
{
    LOGI("pc_process_thread start\n");

	char DevType[32] = {0};

	int listenSock = TcpListen(NULL, PCMSG_PORT);

	if(-1 == listenSock)
	{
		LOGE("PCThread Socket TcpListen fail:%s", strerror(errno));
		return 0;
	}

	PCMSG_DATA msg;

	while(1)
	{
		struct sockaddr_in si;

		int sockfd = TcpAccpet(listenSock, 1000, &si);

		if(-1 == sockfd)
			continue;

		SocketUnblock(sockfd);

		int ret = TcpRecvData(sockfd, (char*)&msg, sizeof(PCMSG_DATA), 1000);

		if(ret != sizeof(PCMSG_DATA))
		{
			LOGW("PCThread Recv length fail expect:%d ret:%d", sizeof(PCMSG_DATA), ret);
			SocketClose(sockfd);
			continue;
		}

		memset(DevType, 0, sizeof(DevType));

		//unicode2utf8((unsigned char*)DevType, (unsigned short*)msg.devtype);
#if 0
		if(strcmp(DevType, "X1H") != 0)
		{
			LOG(LOGWARNING, "recev head is not match, DevType:%s", DevType);
			ret = 0;
			Socket_TcpSendData(sockfd, (char*)&ret, 4, 1000);
			Socket_Close(sockfd);
			continue;
		}
#endif

		LOGI("Recv PC Msg from %s port:0x%x type:%x len:%d", inet_ntoa(si.sin_addr), PCMSG_PORT, msg.type, msg.length);

		switch(msg.type)
		{
			case PCMSG_VERIFY:
				{
					ret = 1;
					TcpSendData(sockfd, (char*)&ret, 4, 1000);
				}
				break;
			case PCMSG_SWITCH_WATCHDOG:
				{
					int ret = false;
					//ret = SwithWatchDog();
					TcpSendData(sockfd, (char*)&ret, 4, 1000);
				}
				break;
			case PCMSG_UPDATE_IMAGEDD:
				{
					HandleUpdateImage(sockfd, &msg);
				}
				break;
			case PCMSG_REBOOT:
				{
					ret = 1;
					TcpSendData(sockfd, (char*)&ret, 4, 1000);
					//DPPostMessage(MSG_SYSTEM, REBOOT_MACH, 0, 0);
				}
				break;
			case PCMSG_RESET:
				{
					ret = 1;
					TcpSendData(sockfd, (char*)&ret, 4, 1000);
					//DPPostMessage(MSG_SYSTEM, RESET_MACH, 0, 0);
				}
				break;
			case PCMSG_CAP_PICTURE:
				break;
		}

		SocketClose(sockfd);
	}

	SocketClose(listenSock);


    LOGI("pc_process_thread end\n");
    return NULL;
}

void StartPCServer(void)
{
	LOGI("StartPCServer start\n");

	if (!g_bPCStart)
	{
		g_bPCStart = true;

		pthread_create(&thread, NULL, pc_process_thread, NULL);

	}

	LOGI("StartPCServer end\n");
}

#endif