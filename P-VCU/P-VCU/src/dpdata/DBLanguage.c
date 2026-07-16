#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <netinet/if_ether.h>
#include <linux/if.h>
#include <linux/route.h>

#include "dpbase.h"

#include "systemmsg.h"
#include "ServiceFile.h"
#include "data/DBLanguage.h"


#define	LANGUAGEDBFILE0		("LanguageDB0.ext")
#define	LANGUAGEDBFILE1		("LanguageDB1.ext")
#define	LANGUAGE_ID			0x4C414E01

typedef struct
{
	unsigned int version;		// 版本号，每做一次修改加1，同时决定当前保存在那个文件中
	unsigned int langcount;	    // 数据库中房间的数量
	unsigned int checkid;		// 保存数据的校验和
} LanguageDbHead;

typedef struct
{
	LanguageDbHead head;
	LanguageList_T* pData;
} LanguageDbContext;


static LanguageDbContext* GlobalLanguageDb = NULL;
static bool bLanguageThdRunning = false;
static pthread_mutex_t g_LanguageCS;

static void FreeDbContext(LanguageDbContext* pctx)
{
	if(pctx->pData != NULL)//-V809
		free(pctx->pData);
	free(pctx);
}

static void FlushDb()
{
	char* pSend;
	bool ret;
	int len;

	len = sizeof(LanguageDbHead) + GlobalLanguageDb->head.langcount * sizeof(LanguageList_T);
	GlobalLanguageDb->head.version++;
	pSend = (char*)malloc(len);
	memcpy(pSend, &GlobalLanguageDb->head, sizeof(LanguageDbHead));//-V575
	if(GlobalLanguageDb->head.langcount != 0)
		memcpy(pSend+sizeof(LanguageDbHead), GlobalLanguageDb->pData, GlobalLanguageDb->head.langcount * sizeof(LanguageList_T));

	if(GlobalLanguageDb->head.version & 1)
		ret = WriteServerFile(LANGUAGEDBFILE1, len, pSend);
	else
		ret = WriteServerFile(LANGUAGEDBFILE0, len, pSend);

	if(!ret)
		free(pSend);
	else
	{
		if(GlobalLanguageDb->head.version & 1)
			DeleteServerFile(LANGUAGEDBFILE0);
		else
			DeleteServerFile(LANGUAGEDBFILE1);
	}
	return;
}



bool DelLanguage(uint16_t roomid)
{
    bool ret = false;
    if(bLanguageThdRunning)
	{
        DPEnterCriticalSection(&g_LanguageCS);
		int i;
		for(i=0; i<GlobalLanguageDb->head.langcount; i++)
		{
			if(GlobalLanguageDb->pData[i].roomid == roomid)
			{
				GlobalLanguageDb->head.langcount--;
				if(i < GlobalLanguageDb->head.langcount)         //-V575
					memcpy(&GlobalLanguageDb->pData[i], &GlobalLanguageDb->pData[GlobalLanguageDb->head.langcount], sizeof(LanguageList_T));
				break;
			}
		}
		if(i < GlobalLanguageDb->head.langcount)
		{
			FlushDb();
			ret = true;
		}

        DPLeaveCriticalSection(&g_LanguageCS);
	}
	return ret;
}


bool AddLanguage(LanguageList_T* languageInfo)
{
    bool bret = false;
	unsigned int i;

    bool bsave = true;
    if(bLanguageThdRunning)
	{
        DPEnterCriticalSection(&g_LanguageCS);
		for(i = 0; i < GlobalLanguageDb->head.langcount; i++)
		{
			if(GlobalLanguageDb->pData[i].roomid == languageInfo->roomid)
				break;
		}

		//此版本只有房间号，重复的房间不覆盖，调过
		if(i == GlobalLanguageDb->head.langcount)
		{
			GlobalLanguageDb->head.langcount++;
			GlobalLanguageDb->pData = (LanguageList_T*)realloc(GlobalLanguageDb->pData, sizeof(LanguageList_T) * GlobalLanguageDb->head.langcount);//-V701
			memcpy(&GlobalLanguageDb->pData[i], languageInfo, sizeof(LanguageList_T));//-V522
			bret = true;
			LOGI("AddLanguage roomid:%d lang:%d\n", languageInfo->roomid, languageInfo->languageid);
		}
		else{
            if(GlobalLanguageDb->pData[i].languageid == languageInfo->languageid)
                bsave = false;
            else
                memcpy(&GlobalLanguageDb->pData[i], languageInfo, sizeof(LanguageList_T));
        }

        if(bsave)
		    FlushDb();

        DPLeaveCriticalSection(&g_LanguageCS);
	}
	return bret;
}


bool CheckLanguage(uint16_t roomid, LanguageList_T *languageinfo)
{
    bool bret = false;
	unsigned int i;

	if(bLanguageThdRunning)
	{
        DPEnterCriticalSection(&g_LanguageCS);
		for(i = 0; i < GlobalLanguageDb->head.langcount; i++)
		{
			if(GlobalLanguageDb->pData[i].roomid == roomid)
				break;
		}
		if(i != GlobalLanguageDb->head.langcount)
		{
            if(languageinfo)
            {
                languageinfo->roomid = GlobalLanguageDb->pData[i].roomid;
                languageinfo->languageid = GlobalLanguageDb->pData[i].languageid;
            }

            bret = true;
		}
        DPLeaveCriticalSection(&g_LanguageCS);
	}

	return bret;
}


static LanguageDbContext* ReadDbLanguageFile(char* filename)
{
	LanguageDbContext* pCardDb;
	LanguageDbHead head;
	LanguageList_T* pdata;
	FILE* fd;
	char fullname[64];

	sprintf(fullname, "/%s/%s", USERDIR, filename);
	fd = fopen(fullname, "rb");
	if(fd == NULL)
		return NULL;

	if(fread(&head, 1, sizeof(LanguageDbHead), fd) != sizeof(LanguageDbHead))
	{
		fclose(fd);
		return NULL;
	}

	if(head.checkid != LANGUAGE_ID)
	{
		fclose(fd);
		return NULL;
	}

	if(head.langcount == 0)
	{
		pCardDb = (LanguageDbContext*)malloc(sizeof(LanguageDbContext));
		memcpy(&pCardDb->head, &head, sizeof(LanguageDbHead));
		pCardDb->pData = NULL;
		fclose(fd);
		return pCardDb;
	}

	pdata = (LanguageList_T*)malloc(head.langcount * sizeof(LanguageList_T));
	if(fread(pdata, sizeof(LanguageList_T), head.langcount, fd) != head.langcount)//-V575
	{
		free(pdata);
		fclose(fd);
		return NULL;
	}
	pCardDb = (LanguageDbContext*)malloc(sizeof(LanguageDbContext));
	memcpy(&pCardDb->head, &head, sizeof(LanguageDbHead));
	pCardDb->pData = pdata;
	fclose(fd);

	return pCardDb;
}

void InitLanguageDB(void)
{
    LanguageDbContext* pCardDb0;
	LanguageDbContext* pCardDb1;

	if(!bLanguageThdRunning)
	{
        DPInitCriticalSection(&g_LanguageCS);

		pCardDb0 = ReadDbLanguageFile(LANGUAGEDBFILE0);
		pCardDb1 = ReadDbLanguageFile(LANGUAGEDBFILE1);
		if((pCardDb0 == NULL) && (pCardDb1 == NULL))
		{
			GlobalLanguageDb = (LanguageDbContext*)malloc(sizeof(LanguageDbContext));
			if(GlobalLanguageDb == NULL)
				return ;
			memset(GlobalLanguageDb, 0, sizeof(LanguageDbContext));
			GlobalLanguageDb->head.checkid = LANGUAGE_ID;
			FlushDb();
		}
		else if((pCardDb0 != NULL) && (pCardDb1 != NULL))
		{
			if(pCardDb0->head.version > pCardDb1->head.version)
			{
				GlobalLanguageDb = pCardDb0;
				FreeDbContext(pCardDb1);
			}
			else
			{
				GlobalLanguageDb = pCardDb1;
				FreeDbContext(pCardDb0);
			}
		}
		else if(pCardDb0 != NULL)
			GlobalLanguageDb = pCardDb0;
		else
			GlobalLanguageDb = pCardDb1;
		bLanguageThdRunning = true;

		LOGI("InitLanguageDB version:%d langcount:%d \n", GlobalLanguageDb->head.version, GlobalLanguageDb->head.langcount);
	}
}

void ResetLanguageDB(void)
{
	if(bLanguageThdRunning)
	{
        DPEnterCriticalSection(&g_LanguageCS);
		GlobalLanguageDb->head.langcount = 0;
		FlushDb();

        DPLeaveCriticalSection(&g_LanguageCS);
	}
}
