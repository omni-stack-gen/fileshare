
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <linux/input.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>

#include "dpbase.h"
//#include "config.h"

#define	FS_MSGQ			"FILESERVER"
#define	MSG_WRITEFILE	1
#define	MSG_DELFILE		2
#define	MSG_ENDFILE		3

typedef struct
{
	unsigned int msgtype;
	char filename[64];
	int filelen;
	void* filedata;
} FILEMSG;

static bool FileServerStart = false;
static void* hFileServerSendQ = NULL;
static void* hFileServerThd = NULL;
//static StaticLock g_FileServerCS;
static pthread_mutex_t g_FileServerCS;

static bool FileMsgProc(FILEMSG* msg)
{
	FILE* fd;
	char wcDelFilePath[256]={0};

	switch(msg->msgtype)
	{
	case MSG_WRITEFILE:
	case MSG_DELFILE:
		sprintf(wcDelFilePath, "%s/%s", USERDIR, msg->filename);
		if(msg->msgtype == MSG_WRITEFILE)
		{
			printf("WriteFile %s\n", wcDelFilePath);
			if(msg->filelen != 0)
			{
				fd = fopen(wcDelFilePath, "wb");
				if(fd != NULL)
				{
					fwrite(msg->filedata, 1, msg->filelen, fd);
					DPFlush(fd);
					fclose(fd);
				}
				else
				{
					printf("WriteFile %s Fail:%d\n", wcDelFilePath, DPGetLastError());
				}
				free(msg->filedata);
			}
		}
		else
		{
			printf("DeleteFile %s\n", wcDelFilePath);
			DPDeleteFile(wcDelFilePath);
		}
		break;
	case MSG_ENDFILE:
		return false;
		break;
	}
	return true;
}

static unsigned int FileServerThread(void* hMsgQR)
{
	FILEMSG smsg;

	while(FileServerStart)
	{
		if(DPReadMsgQueue(hMsgQR, &smsg, sizeof(FILEMSG), 1000000))
		{
			if(!FileMsgProc(&smsg))
				break;
		}
	}
	while(DPReadMsgQueue(hMsgQR, &smsg, sizeof(FILEMSG), 0))
		FileMsgProc(&smsg);
	DPCloseMsgQueue(hMsgQR);
	printf( "FileMsgProc end\n");
	return 0;
}

bool WriteServerFile(const char* name, int len, void* buf)
{
	FILEMSG smsg = {0};
	bool ret = false;

	DPEnterCriticalSection(&g_FileServerCS);
	if(FileServerStart)
	{
		smsg.msgtype = MSG_WRITEFILE;
		strncpy(smsg.filename, name, sizeof(smsg.filename) - 1);
		smsg.filelen = len;
		smsg.filedata = buf;
		if(DPWriteMsgQueue(hFileServerSendQ, &smsg, sizeof(FILEMSG), 0))
		{
			ret = true;
		}
		else
		{
			LOGE("WriteServerFile error:%s\n", strerror(DPGetLastError()));
		}
	}
	DPLeaveCriticalSection(&g_FileServerCS);
	return ret;
}

void DeleteServerFile(const char* name)
{
	FILEMSG smsg = {0};
	DPEnterCriticalSection(&g_FileServerCS);
	if(FileServerStart)
	{
		smsg.msgtype = MSG_DELFILE;
		strncpy(smsg.filename, name, sizeof(smsg.filename) - 1);
		if(!DPWriteMsgQueue(hFileServerSendQ, &smsg, sizeof(FILEMSG), 0))
		{
			printf("DeleteServerFile error:%d\n", DPGetLastError());
		}
	}
	DPLeaveCriticalSection(&g_FileServerCS);
	return;
}

bool InitFileServer(void)
{
	void*	hMsgQR;
	bool ret;
	DPInitCriticalSection(&g_FileServerCS);

	printf("InitFileServer start \n");
	DPEnterCriticalSection(&g_FileServerCS);
	if(!FileServerStart)
	{
		DPCreateMsgQueue(FS_MSGQ, 100, sizeof(FILEMSG), &hMsgQR, &hFileServerSendQ);
		hFileServerThd = DPThreadCreate(0x10000, FileServerThread, hMsgQR, true, 5);
		FileServerStart = true;
	}
	ret = FileServerStart;
	DPLeaveCriticalSection(&g_FileServerCS);
	printf( "InitFileServer end \n");
	return ret;
}

void DeinitFileServer(void)
{
	FILEMSG smsg;

	printf( "DeinitFileServer start\n");
	DPEnterCriticalSection(&g_FileServerCS);
	if(FileServerStart)
	{
		FileServerStart = false;
		smsg.msgtype = MSG_ENDFILE;
		if(!DPWriteMsgQueue(hFileServerSendQ, &smsg, sizeof(FILEMSG), 0))
		{
			printf("DenitFileServer WriteMsgQueue error:%d\n", DPGetLastError());
		}

		DPThreadJoin(hFileServerThd, NULL);
		DPCloseMsgQueue(hFileServerSendQ);
	}
	DPLeaveCriticalSection(&g_FileServerCS);
	printf("DeinitFileServer end\n");
}

