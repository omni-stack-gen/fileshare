
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
#include "data/DBICCard.h"

#define	IDCARDDBFILE0		("IDCardDB0.ext")
#define	IDCARDDBFILE1		("IDCardDB1.ext")
#define	IDCARD_ID			0x49444301

typedef struct
{
	unsigned int version;		// 版本号，每做一次修改加1，同时决定当前保存在那个文件中
	unsigned int cardcount;	    // 数据库中卡的数量
	unsigned int checkid;		// 保存数据的校验和
} CardDbHead;

typedef struct
{
	CardDbHead head;
	CardList_T* pData;
} CardDbContext;

static CardDbContext* GlobalCardDb = NULL;
static bool bICCardThdRunning = false;
static pthread_mutex_t g_IdCardCS;

static void FreeDbContext(CardDbContext* pctx)
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

	len = sizeof(CardDbHead) + GlobalCardDb->head.cardcount * sizeof(CardList_T);
	GlobalCardDb->head.version++;
	pSend = (char*)malloc(len);
	memcpy(pSend, &GlobalCardDb->head, sizeof(CardDbHead));//-V575
	if(GlobalCardDb->head.cardcount != 0)
		memcpy(pSend+sizeof(CardDbHead), GlobalCardDb->pData, GlobalCardDb->head.cardcount * sizeof(CardList_T));

	if(GlobalCardDb->head.version & 1)
		ret = WriteServerFile(IDCARDDBFILE1, len, pSend);
	else
		ret = WriteServerFile(IDCARDDBFILE0, len, pSend);

	if(!ret)
		free(pSend);
	else
	{
		if(GlobalCardDb->head.version & 1)
			DeleteServerFile(IDCARDDBFILE0);
		else
			DeleteServerFile(IDCARDDBFILE1);
		//GlobalCardDb->head.version++;
	}
	return;
}

bool SyncAllICCard(int count, CardList_T* cardInfo, unsigned int version)
{
	if(bICCardThdRunning)
	{
		DPEnterCriticalSection(&g_IdCardCS);

		if(GlobalCardDb->pData != NULL)//-V809
			free(GlobalCardDb->pData);
		GlobalCardDb->pData = NULL;
		GlobalCardDb->head.cardcount = count;
		if(count != 0)
		{
			GlobalCardDb->pData = (CardList_T*)malloc(sizeof(CardList_T) * count);
			memcpy(GlobalCardDb->pData, cardInfo, sizeof(CardList_T) *count);//-V575
		}

		if(version != 0)
			GlobalCardDb->head.version = version - 1;

		FlushDb();
		DPLeaveCriticalSection(&g_IdCardCS);
	}


	return true;
}

bool DelICCard(unsigned long long cno)
{
	unsigned int i;
	bool ret = false;

	if(bICCardThdRunning)
	{
        DPEnterCriticalSection(&g_IdCardCS);
		for(i = 0; i < GlobalCardDb->head.cardcount; i++)
		{
			if(GlobalCardDb->pData[i].idcode == cno)
				break;
		}
		if(i != GlobalCardDb->head.cardcount)
		{
			CardList_T* pdata;
			GlobalCardDb->head.cardcount--;
			pdata = (CardList_T*)malloc(sizeof(CardList_T) * GlobalCardDb->head.cardcount);

			memcpy(pdata, GlobalCardDb->pData, i * sizeof(CardList_T));//-V575
			memcpy(pdata + i, GlobalCardDb->pData + i + 1, (GlobalCardDb->head.cardcount - i) * sizeof(CardList_T));

			free(GlobalCardDb->pData);
			GlobalCardDb->pData = pdata;
			FlushDb();
			ret = true;
		}
        DPLeaveCriticalSection(&g_IdCardCS);
	}

	return ret;
}

bool DelICCardByRoomAndID(unsigned long long cno, unsigned long long roomid)
{
	unsigned int i;
	bool ret = false;

	if(bICCardThdRunning)
	{
        DPEnterCriticalSection(&g_IdCardCS);
		for(i = 0; i < GlobalCardDb->head.cardcount; i++)
		{
			if(GlobalCardDb->pData[i].idcode == cno && GlobalCardDb->pData[i].roomid == roomid)
				break;
		}
		if(i != GlobalCardDb->head.cardcount)
		{
			CardList_T* pdata;
			GlobalCardDb->head.cardcount--;
			pdata = (CardList_T*)malloc(sizeof(CardList_T) * GlobalCardDb->head.cardcount);

			memcpy(pdata, GlobalCardDb->pData, i * sizeof(CardList_T));//-V575
			memcpy(pdata + i, GlobalCardDb->pData + i + 1, (GlobalCardDb->head.cardcount - i) * sizeof(CardList_T));

			free(GlobalCardDb->pData);
			GlobalCardDb->pData = pdata;
			FlushDb();
			ret = true;
		}
        DPLeaveCriticalSection(&g_IdCardCS);
	}

	return ret;
}

bool AddICCard(CardList_T* cardInfo)
{
	bool bret = false;
	unsigned int i;

	if(bICCardThdRunning)
	{
        DPEnterCriticalSection(&g_IdCardCS);
		for(i = 0; i < GlobalCardDb->head.cardcount; i++)
		{
			if(GlobalCardDb->pData[i].idcode == cardInfo->idcode)
				break;
		}

		//此版本只有卡号，重复的卡不覆盖，调过
		if(i == GlobalCardDb->head.cardcount)
		{
			GlobalCardDb->head.cardcount++;
			GlobalCardDb->pData = (CardList_T*)realloc(GlobalCardDb->pData, sizeof(CardList_T) * GlobalCardDb->head.cardcount);//-V701
			memcpy(&GlobalCardDb->pData[i], cardInfo, sizeof(CardList_T));//-V522
			bret = true;
			LOGI("AddICCard idcode:%llu roomid:%llu idtype:%d ExpDate:%u\n", cardInfo->idcode, cardInfo->roomid, cardInfo->idtype, cardInfo->ExpDate);
			//FlushDb();
		}
		else
			memcpy(&GlobalCardDb->pData[i], cardInfo, sizeof(CardList_T));
		FlushDb();

        DPLeaveCriticalSection(&g_IdCardCS);
	}

	return bret;
}

bool CheckICCard(unsigned long long cno, CardList_T *cardinfo)
{
    bool bret = false;
	unsigned int i;

	if(bICCardThdRunning)
	{
        DPEnterCriticalSection(&g_IdCardCS);
		for(i = 0; i < GlobalCardDb->head.cardcount; i++)
		{
			//printf("<%d>check cardid:%llu cno:%llu\n", i, GlobalCardDb->pData[i].idcode, cno);
			if(GlobalCardDb->pData[i].idcode == cno)
				break;
		}
		if(i != GlobalCardDb->head.cardcount)
		{
            if(cardinfo)
            {
                cardinfo->idcode = cno;
			    cardinfo->roomid = GlobalCardDb->pData[i].roomid;
                cardinfo->idtype = GlobalCardDb->pData[i].idtype;
                cardinfo->ExpDate = GlobalCardDb->pData[i].ExpDate;
            }

			// SYSTEMTIME curtime = {0};
    		// DPGetLocalTime(&curtime);
			// unsigned int ptime = curtime.wYear*10000 + curtime.wMonth*100 + curtime.wDay;
			// printf("check cardtime cur:%u  ExpDate:%u\n", ptime, GlobalCardDb->pData[i].ExpDate);
			//if ((GlobalCardDb->pData[i].ExpDate != 0) && GlobalCardDb->pData[i].ExpDate < ptime)
			// {
			// 	bret = false;
			// }
			// else
            	bret = true;
		}
        DPLeaveCriticalSection(&g_IdCardCS);
	}

	return bret;
}

static CardDbContext* ReadDbIDCardFile(char* filename)
{
	CardDbContext* pCardDb;
	CardDbHead head;
	CardList_T* pdata;
	FILE* fd;
	char fullname[64];

	sprintf(fullname, "/%s/%s", USERDIR, filename);
	fd = fopen(fullname, "rb");
	if(fd == NULL)
		return NULL;

	if(fread(&head, 1, sizeof(CardDbHead), fd) != sizeof(CardDbHead))
	{
		fclose(fd);
		return NULL;
	}

	if(head.checkid != IDCARD_ID)
	{
		fclose(fd);
		return NULL;
	}

	if(head.cardcount == 0)
	{
		pCardDb = (CardDbContext*)malloc(sizeof(CardDbContext));
		memcpy(&pCardDb->head, &head, sizeof(CardDbHead));
		pCardDb->pData = NULL;//-V522
		fclose(fd);
		return pCardDb;
	}

	pdata = (CardList_T*)malloc(head.cardcount * sizeof(CardList_T));
	if(fread(pdata, sizeof(CardList_T), head.cardcount, fd) != head.cardcount)//-V575
	{
		free(pdata);
		fclose(fd);
		return NULL;
	}
	pCardDb = (CardDbContext*)malloc(sizeof(CardDbContext));
	memcpy(&pCardDb->head, &head, sizeof(CardDbHead));
	pCardDb->pData = pdata;
	fclose(fd);

	return pCardDb;
}

int GetICCardVersion(void)
{
	int ret = 0;

	if(bICCardThdRunning)
	{
        DPEnterCriticalSection(&g_IdCardCS);
		ret = GlobalCardDb->head.version;
        DPLeaveCriticalSection(&g_IdCardCS);
	}

	return ret;
}

int GetICCardData(CardList_T** pCard, int * cardcount)
{
	int ret = 0;
	CardList_T* pdata;

	if(bICCardThdRunning)
	{
		DPEnterCriticalSection(&g_IdCardCS);
		ret = *cardcount = GlobalCardDb->head.cardcount;
		int bufsize = GlobalCardDb->head.cardcount * sizeof(CardList_T);
		if(bufsize != 0)
		{
			pdata = (CardList_T*)malloc(bufsize);
			memcpy(pdata, GlobalCardDb->pData, bufsize);
			*pCard = pdata;
		}
		DPLeaveCriticalSection(&g_IdCardCS);
	}

	return ret;
}

void InitICCardDB(void)
{
	CardDbContext* pCardDb0;
	CardDbContext* pCardDb1;

	if(!bICCardThdRunning)
	{
        DPInitCriticalSection(&g_IdCardCS);

		pCardDb0 = ReadDbIDCardFile(IDCARDDBFILE0);
		pCardDb1 = ReadDbIDCardFile(IDCARDDBFILE1);
		if((pCardDb0 == NULL) && (pCardDb1 == NULL))
		{
			GlobalCardDb = (CardDbContext*)malloc(sizeof(CardDbContext));
			if(GlobalCardDb == NULL)
				return ;
			memset(GlobalCardDb, 0, sizeof(CardDbContext));//-V579 -V575 -V568 //-V512
			GlobalCardDb->head.checkid = IDCARD_ID;
			FlushDb();
		}
		else if((pCardDb0 != NULL) && (pCardDb1 != NULL))
		{
			if(pCardDb0->head.version > pCardDb1->head.version)
			{
				GlobalCardDb = pCardDb0;
				FreeDbContext(pCardDb1);
			}
			else
			{
				GlobalCardDb = pCardDb1;
				FreeDbContext(pCardDb0);
			}
		}
		else if(pCardDb0 != NULL)
			GlobalCardDb = pCardDb0;
		else
			GlobalCardDb = pCardDb1;
		bICCardThdRunning = true;

		LOGI("InitICCardDB version:%d cardcount:%d \n", GlobalCardDb->head.version, GlobalCardDb->head.cardcount);
	}

}

void DeinitICCardDB(void)
{
	if(bICCardThdRunning)
	{
        DPEnterCriticalSection(&g_IdCardCS);
		FreeDbContext(GlobalCardDb);
		bICCardThdRunning = false;

        DPLeaveCriticalSection(&g_IdCardCS);
	}
}

void ResetICCardDB(void)
{
	if(bICCardThdRunning)
	{
        DPEnterCriticalSection(&g_IdCardCS);
		GlobalCardDb->head.cardcount = 0;
		FlushDb();

        DPLeaveCriticalSection(&g_IdCardCS);
	}
}