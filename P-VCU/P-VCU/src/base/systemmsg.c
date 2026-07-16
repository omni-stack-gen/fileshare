#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <systemmsg.h>
#include <dpbaselib.h>
#include <sem.h>

#define	MASSAGE_MAX		512
#define	MASSAGE_MASK	(MASSAGE_MAX - 1)

typedef struct
{
	SYS_MSG m_MsgBuf[MASSAGE_MAX];
	unsigned int m_ReadPtr;
	unsigned int m_WritePtr;
	unsigned int m_MsgCount;
} Msg_Channal;

typedef struct
{

}StaticLock;

static Msg_Channal m_MsgChannal[MSG_MAX_TYPE];
static void* m_hMsgSemp = NULL;
//static StaticLock m_SysMsgCs;

static pthread_mutex_t m_cs;


void InitSysMessage(void)
{
	Mutex_Init(&m_cs);
	m_hMsgSemp = CreateSemaphore(0);
	memset(m_MsgChannal, 0, sizeof(Msg_Channal) * MSG_MAX_TYPE);
	DPCreateTimeEvent();
}

bool DPPostMessage(unsigned int msgtype, unsigned int wparam, unsigned int lparam, unsigned int zParam, unsigned int type)
{
	Msg_Channal* pChanal;
	bool ret = false;
	Mutex_Lock(&m_cs);
	if(type < MSG_MAX_TYPE)
	{
		pChanal = &m_MsgChannal[type];
		if(pChanal->m_MsgCount < MASSAGE_MAX)
		{
			pChanal->m_MsgBuf[pChanal->m_WritePtr].msg = msgtype;
			pChanal->m_MsgBuf[pChanal->m_WritePtr].wParam = wparam;
			pChanal->m_MsgBuf[pChanal->m_WritePtr].lParam = lparam;
			pChanal->m_MsgBuf[pChanal->m_WritePtr].zParam = zParam;
			pChanal->m_WritePtr++;
			pChanal->m_WritePtr &= MASSAGE_MASK;
			pChanal->m_MsgCount++;
			ret = true;
		}
	}

	SetSemaphore(m_hMsgSemp);
	Mutex_UnLock(&m_cs);
	return ret;
}

unsigned int DPGetMessage(SYS_MSG* msg)
{
	unsigned int i;
	Msg_Channal* pChanal;
	unsigned int ret = MSG_MAX_TYPE;
again:
	if(GetSemaphore(m_hMsgSemp, 100000))
	{
		Mutex_Lock(&m_cs);
		for(i = 0; i < MSG_MAX_TYPE; i++)
		{
			pChanal = &m_MsgChannal[i];
			if(pChanal->m_MsgCount > 0)
			{
				memcpy(msg, &pChanal->m_MsgBuf[pChanal->m_ReadPtr], sizeof(SYS_MSG));
				pChanal->m_MsgCount--;
				pChanal->m_ReadPtr++;
				pChanal->m_ReadPtr &= MASSAGE_MASK;
				ret = i;
				break;
			}
		}
		Mutex_UnLock(&m_cs);
	}
	if(ret == MSG_MAX_TYPE)
		goto again;
	return ret;
}

void CleanUserInput(void)
{
	Mutex_Lock(&m_cs);
	memset(&m_MsgChannal[MSG_TOUCH_TYPE], 0, sizeof(Msg_Channal));
	memset(&m_MsgChannal[MSG_KEY_TYPE], 0, sizeof(Msg_Channal));
	Mutex_UnLock(&m_cs);
}

