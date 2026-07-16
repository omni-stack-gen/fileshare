#ifndef __DPBASE_H__
#define __DPBASE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <pthread.h>
#include <stdint.h>
#include <log.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

#define SECOND_FROM_1601_TO_1970 11644473600ULL
#define SECOND_FROM_1601_TO_1900 9435513600

#ifdef ENABLE_V1
#define USERDIR					"/mnt/UDISK" //"/app/userdev"
#else
#define USERDIR					"/data" //"/app/userdev"
#endif
#define	IPTABLE_NAME			"netcfg.dat"
#define IPTABLE_BAK				"netcfg.bak"
#define FLASHDIR				"/mnt/app"
#define WINDOWSDIR				"/tmp"

#ifdef ENABLE_SPINAAD
//nand flash 升级包放到flash区
#define	DOWNLOAD_IMAGE_PATH				"/system/update.tar.gz"
#define	DOWNLOAD_IMAGE_PLC_CACHE_PATH 	"/system/app/UpdateCco.bin"
#else
#define	DOWNLOAD_IMAGE_PATH		"/tmp/upgrade.img"
#endif //ENABLE_SPINAAD
#define	DOWNLOAD_IMAGE_PLC_PATH "/tmp/upgrade_plc.img"
#define	IMAGE_APP0				"/app/app0"

typedef struct
{
	unsigned int dwLowDateTime;
	unsigned int dwHighDateTime;
} FILETIME;

typedef struct
{
	int wSecond;
	int wMinute;
	int wHour;
	int wDay;
	int wMonth;
	int wYear;
	int wDayOfWeek;
	int tm_yday;
	int tm_isdst;
} SYSTEMTIME;



//dpbase.c
void *DPThreadCreate(int stacksize, unsigned int (*func)(void *), void *arg, bool isjoin, unsigned int level);
bool DPThreadJoin(void *hThread, unsigned int *pRet);
bool DPCreateMsgQueue(const char *name, unsigned int vol, unsigned int itemsize, void **pRead, void **pWrite);
bool DPReadMsgQueue(void *hMsg, void *data, unsigned int len, unsigned int timeout);
bool DPWriteMsgQueue(void *hMsg, void *data, unsigned int len, unsigned int timeout);
void DPCloseMsgQueue(void *hMsg);
unsigned int DPGetLastError();
void *DPCreateSemaphore(unsigned int dwInitCount, unsigned int dwMaxCount);
void DPSetSemaphore(void *hSem);
bool DPGetSemaphore(void *hSem, unsigned int timeout);
void DPInitCriticalSection(pthread_mutex_t *pcsCriticalSection);
void DPDeleteCriticalSection(pthread_mutex_t *pcsCriticalSection);
void DPEnterCriticalSection(pthread_mutex_t *pcsCriticalSection);
void DPLeaveCriticalSection(pthread_mutex_t *pcsCriticalSection);

//dpfile.c
struct sdcard_info
{
	bool is_insert;
	bool is_mounted;
	char name[64];
	char serial[64];
	char manfid[64];
	char oemid[64];
	char date[64];
	char mount_point[64];
	uint64_t used_size;
	uint64_t capacity;
};


#define WINDOWSDIR					"/tmp"

int sdcard_get_info(struct sdcard_info *info);
void DPChmodFile(const char* pfile, char * type);
void DPDeleteFile(const char *pfile);
void DPMoveFile(const char *dstfile, const char *srcfile);
void DPCopyFile(const char *dstfile, const char *srcfile);
void *DPFindFirstFile(char *dir, char *pfile);
bool DPFindNextFile(void *hFind, char *pfile);
void DPFindClose(void *hFind);
int DPGetFileAttributes(const char *filename, int *filelen);
bool CheckAndCreateDir(const char *cDir);
unsigned long long CheckSpareSpace(const char *dir);
unsigned int DumpMemory(unsigned int *total, unsigned int *left);
unsigned int GetFreeMemory();
void DPUnloadFile(const char *filename);
bool DPWriteFile(const char *filename, char *pdata, int len);
void DPFlush(FILE *pFile);
int BReadFile(char *filename, char **buf);
bool DPCopyFileEx(const char *dstfile, const char *srcfile);
void FindFileFromDirectory(char *dir, void (*func)(char *directory, char *fileName, void *param), void *param);

//dptime.c
unsigned int DPGetTickCount(void);
long long DPGetTickCount64();
void DPSleep(unsigned int dwMilliseconds);
bool DPFileTimeToSystemTime(FILETIME *lpFileTime, SYSTEMTIME *lpSystemTime);
bool DPSystemTimeToFileTime(SYSTEMTIME *lpSystemTime, FILETIME *lpFileTime);
bool DPSetLocalTime(SYSTEMTIME *st);
void DPGetLocalTime(SYSTEMTIME *lpSystemTime);
FILETIME timeToFileTime(const time_t *ptime);

//DpDebug.c
#define DPDEBUG 0
#define DPINFO 1
#define DPWARNING 2
#define DPERROR 3

void InitDebugLeven(unsigned int level);
void LogOutPut(unsigned int level, const char *file, const int line, const char *func, const char *format, ...);
#define DBGMSG(level, ...) LogOutPut(level, __FILE__, __LINE__, __func__, __VA_ARGS__)


#ifdef __cplusplus
}
#endif

typedef struct
{
	int motionlevel; 	//移动侦测等级
	int soundcall;
	int soundtalk;

	int locktype;
	int lockswitch;
	int locktime;
	int menicswitch;
	int menictime;

	int lockswitch1;
	int locktime1;
	int menicswitch1;
	int menictime1;
}door_cfg_t;

void InitSystemSet(void);
void ResetSystemSet(void);
unsigned int GetTermDev(void);
void SetTermId(char *TermId);

unsigned int GetTermDev(void);

int GetMegneticDelay(void);
void SetMegneticDelay(int delay);
int GetMegneticUsed(void);
void SetMegneticUsed(int bused);
int GetLockDelay(void);
int GetLock1Delay(void);
void SetLockDelay(int delay);
int GetLockPulse(void);
void SetLockPulse(int pulse);

int GetMotionLevel(void);
void SetMotionLevel(int level);
int GetSoundcallVol(void);
void SetSoundcallVol(int vol);
int GetSoundtalkVol(void);
void SetSoundtalkVol(int vol);
char *GetLocalSn(void);
void SetLocalSn(char *psn);

int GetbLockType(void);
void GetDoorCfg(door_cfg_t *doorcfg);
void SetDoorCfg(door_cfg_t *doorcfg);
#endif //__DPBASE_H__