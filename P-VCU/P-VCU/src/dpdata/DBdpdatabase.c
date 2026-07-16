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

#define SETFILE0 "SystemSet0.dat"
#define SETFILE1 "SystemSet1.dat"
#define SETENDID 0x55595301

typedef struct
{
    char TermId[16];                    // 0	终端号码
    unsigned int TermDev;               // 16   终端类型 1：室内机 3：小门口机
    int  motion;                        // 20   移动侦测灵敏度
    int  lockswitch;                    // 24   开锁电平
    int  locktime;                      // 28   开锁延时
    int  menicswitch;                   // 32   门磁启用
    int  menictime;                     // 36   门磁延时
    int  soundcall;                     // 40   振铃声音大小
    int  soundtalk;                     // 44   对讲声音大小
    bool bctrlreboot;                   // 48   是否手动重启
    int  lockswitch1;                   // 52   第二把开锁电平
    int  locktime1;                     // 56   第二把开锁延时
    int  menicswitch1;                  // 60   第二把门磁启用
    int  menictime1;                    // 64   第二把门磁延时
    char local_sn[64];                  // 68   SN码
    int  locktype;                      // 132  1：Lo-Z Locker：LOCK1驱动10mS开锁（持续10mS高电平） 2:Hi-Z Locker：LOCK1驱动5S开锁（持续5S高电平）
    unsigned char reserved[1016 - 68 - 64 - 4];  // 136   预留
    unsigned int VERSION;               // 1016 版本号
    unsigned int Endid;                 // 1020 结束位
} SystemSet_t;                          // 1024

typedef struct Status_Info
{
    unsigned int  u32Ip;                        //本机ip
    unsigned int  u32Mask;                      //掩码
    unsigned int  u32Gw;                        //网关
    unsigned int  u32NetWorkStatus;             //网络状态
    bool          bAutoCall;                    //是否自动呼叫
    char          strProducer[32];              //生产商
    char          strHardinfo[32];              //硬件版本号
    char          strSysVer[32];                //软件版本号
    int           iNetCfgVersion;               //网络配置表版本号
    char          strNetcfgmd5[33];             //网络配置表的MD5
}StatusInfo_S;

static SystemSet_t *m_gSystemSet = NULL;
//static StaticLock g_SystemSetCS;
static bool g_bUpdateEn = true;
static pthread_mutex_t g_DBdataCS;

static StatusInfo_S stStatusInfo = {0};
static pthread_mutex_t PublicInfoCS;

static void UpdataSystemSet(void)
{
    if (!g_bUpdateEn)
        return;

    SystemSet_t *pSet;
    bool ret;

    pSet = (SystemSet_t *)malloc(sizeof(SystemSet_t));
    if (NULL == pSet)
    {
        LOGE("UpdataSystemSet malloc fail \n");
        return;
    }

    memcpy(pSet, m_gSystemSet, sizeof(SystemSet_t));
    if (m_gSystemSet->VERSION & 1)
        ret = WriteServerFile(SETFILE1, sizeof(SystemSet_t), (char *)pSet);
    else
        ret = WriteServerFile(SETFILE0, sizeof(SystemSet_t), (char *)pSet);
    if (!ret)
        free(pSet);
    else
    {
        if (m_gSystemSet->VERSION & 1)
            DeleteServerFile(SETFILE0);
        else
            DeleteServerFile(SETFILE1);
        m_gSystemSet->VERSION++;
    }
}

static void InitDefaultSystemSet(void)
{
    memset(m_gSystemSet, 0, sizeof(SystemSet_t));

    strcpy(m_gSystemSet->TermId, "3000001");
    m_gSystemSet->TermDev = 0x3000001;
    m_gSystemSet->lockswitch = 1;
    m_gSystemSet->locktime = 1;
    m_gSystemSet->menicswitch = 1;
    m_gSystemSet->menictime = 30;
    m_gSystemSet->lockswitch1 = 1;
    m_gSystemSet->locktime1 = 1;
    m_gSystemSet->menicswitch1 = 1;
    m_gSystemSet->menictime1 = 30;

    m_gSystemSet->soundcall = 50;
    m_gSystemSet->soundtalk = 50;

    m_gSystemSet->locktype = 1;

    m_gSystemSet->Endid = SETENDID;
}

void InitSystemSet(void)
{
    DPInitCriticalSection(&g_DBdataCS);
    DPInitCriticalSection(&PublicInfoCS);
    LOGI("===============%d \n", sizeof(SystemSet_t));

    SystemSet_t *pSet0 = NULL;
    SystemSet_t *pSet1 = NULL;
    char filename[64];
    FILE *fd;

    DPEnterCriticalSection(&g_DBdataCS);
    sprintf(filename, "%s/%s", USERDIR, SETFILE0);
    fd = fopen(filename, "rb");
    if (fd != NULL)
    {
        pSet0 = (SystemSet_t *)malloc(sizeof(SystemSet_t));
        if (pSet0)
        {
            memset(pSet0, 0, sizeof(SystemSet_t));
            int ret = fread(pSet0, 1, sizeof(SystemSet_t), fd);
            if (ret != sizeof(SystemSet_t))
            {
                free(pSet0);
                pSet0 = NULL;
            }
            else
            {
                if (pSet0->Endid != SETENDID)
                {
                    free(pSet0);
                    pSet0 = NULL;
                }
            }
        }
        fclose(fd);
    }

    sprintf(filename, "%s/%s", USERDIR, SETFILE1);
    fd = fopen(filename, "rb");
    if (fd != NULL)
    {
        pSet1 = (SystemSet_t *)malloc(sizeof(SystemSet_t));
        if (pSet1)
        {
            memset(pSet1, 0, sizeof(SystemSet_t));
            int ret = fread(pSet1, 1, sizeof(SystemSet_t), fd);
            if (ret != sizeof(SystemSet_t))
            {
                free(pSet1);
                pSet1 = NULL;
            }
            else
            {
                if (pSet1->Endid != SETENDID)
                {
                    free(pSet1);
                    pSet1 = NULL;
                }
            }
        }
        fclose(fd);
    }

    if ((pSet0 == NULL) && (pSet1 == NULL))
    {
        m_gSystemSet = (SystemSet_t *)malloc(sizeof(SystemSet_t));
        if (NULL == m_gSystemSet)
        {
            LOGE("DBSystmeset fail \n");
            return;
        }

        InitDefaultSystemSet();
        UpdataSystemSet();
    }
    else
    {
        if ((pSet0 != NULL) && (pSet1 != NULL))
        {
            if (pSet0->VERSION > pSet1->VERSION)
            {
                m_gSystemSet = pSet0;
                free(pSet1);
            }
            else
            {
                m_gSystemSet = pSet1;
                free(pSet0);
            }
        }
        else if (pSet0 != NULL)
            m_gSystemSet = pSet0;
        else
            m_gSystemSet = pSet1;
        m_gSystemSet->VERSION++;
    }

    DPLeaveCriticalSection(&g_DBdataCS);
}

void ResetSystemSet(void)
{
    DPEnterCriticalSection(&g_DBdataCS);
    InitDefaultSystemSet();
    UpdataSystemSet();
    // 复位后，保存的肯定是文件SETFILE0， 所以删除文件SETFILE1即可
    DeleteServerFile(SETFILE1);
    DPLeaveCriticalSection(&g_DBdataCS);
}

char *GetTermId(char *TermId)
{
    char *pTermId = NULL;
    DPEnterCriticalSection(&g_DBdataCS);
    if (m_gSystemSet != NULL)
    {
        if (TermId)
            strcpy(TermId, m_gSystemSet->TermId);
        pTermId = m_gSystemSet->TermId;
    }
    DPLeaveCriticalSection(&g_DBdataCS);
    return pTermId;
}

void SetTermId(char *TermId)
{
    DPEnterCriticalSection(&g_DBdataCS);
    if (m_gSystemSet != NULL)
    {
        if (strcmp(TermId, m_gSystemSet->TermId) != 0)
        {
            strncpy(m_gSystemSet->TermId, TermId, sizeof(m_gSystemSet->TermId)-1);
            m_gSystemSet->TermDev = strtoll(TermId, NULL, 16);
            UpdataSystemSet();
        }
    }
    DPLeaveCriticalSection(&g_DBdataCS);
}


unsigned int GetTermDev(void)
{
    return m_gSystemSet->TermDev;
}

int GetMegneticDelay(void)
{
    return m_gSystemSet->menictime;
}

void SetMegneticDelay(int delay)
{
    DPEnterCriticalSection(&g_DBdataCS);

    m_gSystemSet->menictime = delay;
    UpdataSystemSet();

    DPLeaveCriticalSection(&g_DBdataCS);
}

int GetMegneticUsed(void)
{
    return m_gSystemSet->menicswitch;
}
void SetMegneticUsed(int bused)
{
    DPEnterCriticalSection(&g_DBdataCS);

    m_gSystemSet->menicswitch = bused;
    UpdataSystemSet();

    DPLeaveCriticalSection(&g_DBdataCS);
}

int GetLockDelay(void)
{
    return m_gSystemSet->locktime;
}

int GetLock1Delay(void)
{
    return m_gSystemSet->locktime1;
}

void SetLockDelay(int delay)
{
    DPEnterCriticalSection(&g_DBdataCS);

    m_gSystemSet->locktime = delay;
    UpdataSystemSet();

    DPLeaveCriticalSection(&g_DBdataCS);
}

int GetLockPulse(void)
{
    return m_gSystemSet->lockswitch;
}
void SetLockPulse(int pulse)
{
    DPEnterCriticalSection(&g_DBdataCS);

    m_gSystemSet->lockswitch = pulse;
    UpdataSystemSet();

    DPLeaveCriticalSection(&g_DBdataCS);
}

int GetMotionLevel(void)
{
    return m_gSystemSet->motion;
}
void SetMotionLevel(int level)
{
    DPEnterCriticalSection(&g_DBdataCS);

    m_gSystemSet->motion = level;
    UpdataSystemSet();

    DPLeaveCriticalSection(&g_DBdataCS);
}

int GetSoundcallVol(void)
{
    return m_gSystemSet->soundcall;
}
void SetSoundcallVol(int vol)
{
    DPEnterCriticalSection(&g_DBdataCS);

    m_gSystemSet->soundcall = vol;
    UpdataSystemSet();

    DPLeaveCriticalSection(&g_DBdataCS);
}

int GetSoundtalkVol(void)
{
    if(m_gSystemSet){
        return m_gSystemSet->soundtalk;
    }
    return 50;
}
void SetSoundtalkVol(int vol)
{
    DPEnterCriticalSection(&g_DBdataCS);

    m_gSystemSet->soundtalk = vol;
    UpdataSystemSet();

    DPLeaveCriticalSection(&g_DBdataCS);
}

char *GetLocalSn(void)
{
    if(m_gSystemSet == NULL)
        return "";
    return m_gSystemSet->local_sn;
}

void SetLocalSn(char *psn)
{
    DPEnterCriticalSection(&g_DBdataCS);

    if(psn)
    {
        strncpy(m_gSystemSet->local_sn, psn, sizeof(m_gSystemSet->local_sn)-1);
    }
    UpdataSystemSet();

    DPLeaveCriticalSection(&g_DBdataCS);
}

bool GetbCtrlReboot(void)
{
    return m_gSystemSet->bctrlreboot;
}
void SetbCtrlReboot(bool breboot)
{
    DPEnterCriticalSection(&g_DBdataCS);

    m_gSystemSet->bctrlreboot = breboot;
    UpdataSystemSet();

    DPLeaveCriticalSection(&g_DBdataCS);
}

int GetbLockType(void)
{
    if(m_gSystemSet == NULL)
        return 1;
    return m_gSystemSet->locktype;
}

void GetDoorCfg(door_cfg_t *doorcfg)
{
    DPEnterCriticalSection(&g_DBdataCS);
    if (doorcfg && m_gSystemSet)
    {
        doorcfg->motionlevel = m_gSystemSet->motion;
        doorcfg->soundcall = m_gSystemSet->soundcall;
        doorcfg->soundtalk = m_gSystemSet->soundtalk;

        doorcfg->locktype = m_gSystemSet->locktype;
        doorcfg->lockswitch = m_gSystemSet->lockswitch;
        doorcfg->locktime = m_gSystemSet->locktime;
        doorcfg->menicswitch = m_gSystemSet->menicswitch;
        doorcfg->menictime = m_gSystemSet->menictime;

        doorcfg->lockswitch1 = m_gSystemSet->lockswitch1;
        doorcfg->locktime1 = m_gSystemSet->locktime1;
        doorcfg->menicswitch1 = m_gSystemSet->menicswitch1;
        doorcfg->menictime1 = m_gSystemSet->menictime1;
    }
    DPLeaveCriticalSection(&g_DBdataCS);
}

void SetDoorCfg(door_cfg_t *doorcfg)
{
    DPEnterCriticalSection(&g_DBdataCS);

    if(doorcfg->motionlevel != -1)
        m_gSystemSet->motion = doorcfg->motionlevel;
    if(doorcfg->soundcall != -1)
        m_gSystemSet->soundcall = doorcfg->soundcall;
    if(doorcfg->soundtalk != -1)
        m_gSystemSet->soundtalk = doorcfg->soundtalk;

    if(doorcfg->locktype != -1)
        m_gSystemSet->locktype = doorcfg->locktype;
    if(doorcfg->lockswitch != -1)
        m_gSystemSet->lockswitch = doorcfg->lockswitch;
    if(doorcfg->locktime != -1){
        m_gSystemSet->locktime = doorcfg->locktime;
#if defined(ENABLE_V6)
        void SetLock1Delay(int delay);
        SetLock1Delay(doorcfg->locktime);
#endif
    }
    if(doorcfg->menicswitch != -1)
        m_gSystemSet->menicswitch = doorcfg->menicswitch;
    if(doorcfg->menictime != -1)
        m_gSystemSet->menictime = doorcfg->menictime;


    if(doorcfg->lockswitch1 != -1)
        m_gSystemSet->lockswitch1 = doorcfg->lockswitch1;
    if(doorcfg->locktime1 != -1)
        m_gSystemSet->locktime1 = doorcfg->locktime1;
    if(doorcfg->menicswitch1 != -1)
        m_gSystemSet->menicswitch1 = doorcfg->menicswitch1;
    if(doorcfg->menictime1 != -1)
        m_gSystemSet->menictime1 = doorcfg->menictime1;

    UpdataSystemSet();

    DPLeaveCriticalSection(&g_DBdataCS);
}


//================== PublicInfo =====================
int GetIpInfo( unsigned int *pIP, unsigned int *pMask, unsigned int *pGW )
{
    DPEnterCriticalSection(&PublicInfoCS);
    *pIP = stStatusInfo.u32Ip;
    *pMask = stStatusInfo.u32Mask;
    *pGW = stStatusInfo.u32Gw;
    DPLeaveCriticalSection(&PublicInfoCS);
    return 0;
}

unsigned int GetIP(void)
{
    return stStatusInfo.u32Ip;
}

int SetIpInfo( unsigned int IP, unsigned int Mask, unsigned int GW )
{
    DPEnterCriticalSection(&PublicInfoCS);
    stStatusInfo.u32Ip = IP;
    stStatusInfo.u32Mask = Mask;
    stStatusInfo.u32Gw = GW;
    DPLeaveCriticalSection(&PublicInfoCS);
    return 0;
}



int GetProducer(char *str)
{
    strcpy(str, stStatusInfo.strProducer);
    return 0;
}

int SetProducer(char *str)
{
    if(strlen(str) > 31)
        return -1;
    DPEnterCriticalSection(&PublicInfoCS);
    strcpy(stStatusInfo.strProducer, str);
    DPLeaveCriticalSection(&PublicInfoCS);
    return 0;
}

int GetHardinfo(char *str)
{
    strcpy(str, stStatusInfo.strHardinfo);
    return 0;
}

int SetHardinfo(char *str)
{
    if(strlen(str) > 31)
        return -1;
    DPEnterCriticalSection(&PublicInfoCS);
    strcpy(stStatusInfo.strHardinfo, str);
    DPLeaveCriticalSection(&PublicInfoCS);
    return 0;
}

int GetSysVer(char *str)
{
    strcpy(str, stStatusInfo.strSysVer);
    return 0;
}

int SetSysVer(char *str)
{
    if(strlen(str) > 31)
        return -1;
    DPEnterCriticalSection(&PublicInfoCS);
    strcpy(stStatusInfo.strSysVer, str);
    DPLeaveCriticalSection(&PublicInfoCS);
    return 0;
}

int GetNetCfgVersion(void)
{
    int ret = 0;
    ret = stStatusInfo.iNetCfgVersion;
    return ret;
}

int SetNetCfgVersion(int Version)
{
    DPEnterCriticalSection(&PublicInfoCS);
    stStatusInfo.iNetCfgVersion = Version;
    DPLeaveCriticalSection(&PublicInfoCS);
    return 0;
}


char *GetNetCfgMD5(void)
{
    return stStatusInfo.strNetcfgmd5;
}

int SetNetCfgMD5(char *strmd5)
{
    DPEnterCriticalSection(&PublicInfoCS);
    strcpy(stStatusInfo.strNetcfgmd5, strmd5);
    DPLeaveCriticalSection(&PublicInfoCS);

    return 0;
}

