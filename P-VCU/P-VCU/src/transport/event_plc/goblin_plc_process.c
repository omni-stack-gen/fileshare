#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <utils/bmem.h>

#include <utils/util.h>
#include <utils/list.h>
#include <utils/mq.h>
#include <utils/circlebuf.h>
#include <utils/log.h>
#include <utils/cJSON.h>
#include <utils/sem.h>
#include <utils/time_helper.h>

#include "goblin_plc_process.h"
#include "spi_plc/spi_plc_mgnt.h"
#include "spi_plc/spi_plc_trans.h"
#include "DPDef.h"


#define		SYNC_DEV_REPORT_INFO					2014	//家庭网设备上报室内机，检查是否可代理，成功就定时上报
#define		SYNC_DEV_REPORT_INFO_ACK				2015

#define		SYNC_DEV_REPORT_DISABLEAGENT			2016	//解除代理，停止上报
#define		SYNC_DEV_REPORT_DISABLEAGENT_ACK		2017

#define     SYNC_DEV_UNLOCK                         2030    //嵌入式设备请求嵌入式设备  开锁
#define     SYNC_DEV_UNLOCK_ACK                     2031

#define     SYNC_DEV_SET_ID                         2100    //嵌入式设备请求嵌入式设备  设置别墅机房号
#define     SYNC_DEV_SET_ID_ACK                     2101

#define     SYNC_DEV_SET_CFG                        2110    //嵌入式设备请求嵌入式设备  设置别墅机信息
#define     SYNC_DEV_SET_CFG_ACK                    2111

#define     SYNC_DEV_GET_CFG                        2120    //嵌入式设备请求嵌入式设备  获取别墅机信息
#define     SYNC_DEV_GET_CFG_ACK                    2121

#define		SYNC_DEV_SET_ADDCARD_STATE				2130	//嵌入式设备请求嵌入式设备  设置门口机的添加卡状态
#define		SYNC_DEV_SET_ADDCARD_STATE_ACK			2131

#define		SYNC_DEV_SET_DELCARD_STATE				2140	//嵌入式设备请求嵌入式设备  设置门口机的删除卡状态
#define		SYNC_DEV_SET_DELCARD_STATE_ACK			2141

#define		SYNC_DEV_DEL_ALL_CARD					2150	//嵌入式设备请求嵌入式设备  删除所有卡
#define		SYNC_DEV_DEL_ALL_CARD_ACK				2151

#define		SYNC_DEV_DEL_CARD_BY_ID					2152	//嵌入式设备请求嵌入式设备  删除指定卡号
#define		SYNC_DEV_DEL_CARD_BY_ID_ACK				2153

#define     SYNC_DEV_PLC_GET_ALL_CARD               2154    //嵌入式设备请求嵌入式设备  获取门口机所有卡
#define     SYNC_DEV_PLC_GET_ALL_CARD_ACK           2155

#define		SYNC_DEV_PLC_MODIFY_CARD_BY_ID			2156	//嵌入式设备请求嵌入式设备  修改指定卡号
#define		SYNC_DEV_PLC_MODIFY_CARD_BY_ID_ACK		2157

#define 	SYNC_DEV_PLC_REPORT_CARD_BY_OPT			2158	//嵌入式设备请求嵌入式设备  上报卡操作状态
#define		SYNC_DEV_PLC_REPORT_CARD_BY_OPT_ACK		2159

#define     SYNC_DEV_WAKEUP                         2170    //嵌入式设备请求嵌入式设备  唤醒对应房号设备
#define     SYNC_DEV_WAKEUP_ACK                     2171

#define 	SYNC_DEV_CHECK_STATE					2280	//检测设备状态
#define		SYNC_DEV_CHECK_STATE_ACK				2281

#define     SYNC_DEV_SEARCH                         9900    //嵌入式设备请求嵌入式设备  搜索
#define     SYNC_DEV_SRARCH_ACK                     9901

#define     SYNC_DEV_ONLINE                         9910    //嵌入式设备请求嵌入式设备  上线
#define     SYNC_DEV_ONLINE_ACK                     9911

#define  	SYNC_DEV_GET_LOGFILE					9994	//获取日志文件
#define		SYNC_DEV_GET_LOGFILE_ACK				9995

#define  	SYNC_DEV_GET_LOGFILESIZE				9996	//获取日志文件大小
#define		SYNC_DEV_GET_LOGFILESIZE_ACK			9997

#define  	SYNC_DEV_SET_SN							9998	//设置SN值
#define		SYNC_DEV_SET_SN_ACK						9999

#define  	SYNC_DEV_GET_SN							10000	//获取SN值
#define		SYNC_DEV_GET_SN_ACK						10001

#define  	SYNC_DEV_SET_LANGUANGE					10002	//设置语言
#define		SYNC_DEV_SET_LANGUANGE_ACK				10003

#define  	SYNC_DEV_SET_NOISE_AI					10004	//设置ai语音降噪
#define		SYNC_DEV_SET_NOISE_AI_ACK				10005

//硬件测试用到的
#define		SYNC_DEV_PLC_TEST						1000	//测试用的
#define		SYNC_DEV_PLC_TEST_ACK					1001

#define MAX_LAN_EVENT          (32)

#define INSERT_STRING2OBJECT(pjson, oname, strbuf)      if(strbuf && strlen(strbuf) > 0)  cJSON_AddStringToObject(pjson, oname, strbuf);
#define INSERT_NUMBER2OBJECT(pjson, oname, inumber) 	if(inumber != -1) cJSON_AddNumberToObject(pjson, oname, inumber)

#define STRINFOINSERT_BYJSON(strj, strdst, len)  		if(strj && cJSON_IsString(strj))  strncpy(strdst, strj->valuestring, len);
#define NUMINFOINSERT_BYJSON(strj, intdst)  			if(strj && cJSON_IsNumber(strj))    intdst = strj->valueint;else intdst = -1;


#define PLC_SEND_MSG_WAIT_ACK(plc_mgnt, remoteaddr, msgseq, cmd, cmdack, eventinfo, sendbuf) 				\
	do{																					\
		if(timeout)																		\
		{																				\
			eventinfo.seq = msgseq;														\
			eventinfo.reqcmd = cmd;														\
			eventinfo.rspcmd = cmdack;													\
			eventinfo.sem = CreateSemaphore(0);											\
			event_info_add(plc_mgnt, &eventinfo);										\
		}																				\
        spi_plc_mgnt_send(remoteaddr, PROXY_PORT, (uint8_t *)sendbuf, strlen(sendbuf));	\
		if(eventinfo.sem)																\
		{																				\
			if(GetSemaphore(eventinfo.sem, timeout))									\
				ret = 0;																\
			else																		\
				event_info_del_by_seq(plc_mgnt, seq);									\
			DestorySemaphore(eventinfo.sem);											\
		}																				\
	}while(0);
typedef struct spi_plc_lan_process_mgnt
{
	// 本机设备号
	uint32_t local_dev;
	//
	uint32_t plc_version;
	uint32_t send_seq;
	bool   is_agent;		//是否可代理其他设备上平台， 一般用于带wifi的室内机
	char local_soft_ver[32];
	char local_sys_ver[32];
	char local_hard_ver[32];
	char local_mac[32];

	pthread_mutex_t event_mutex;
	struct list_head event_list;


	uint64_t last_time;

	bool event_run;
	mq_t * event_queue;
	pthread_t thread;

	PlcEventHandleFun   m_pEventCB;
	void *pusrdata;

	PlcFileCallBackFun  m_pFileCb;              //文件传输回调
    void *              m_FileParam;           //文件传输回调参数
	void *				m_filesem;				//文件传输回调信号量
} spi_plc_lan_process_mgnt_t;

typedef struct spi_event_info_
{
	struct list_head node;
	uint32_t reqcmd;
	uint32_t rspcmd;
	uint32_t seq;
	uint32_t tick;

	void *sem;

	uint32_t ackdatalen;
	uint8_t *packdata;
}spi_event_info_s;


#define PLC_TRAN_MSG     	(0x00)
#define PLC_ONLINE_MSG      (0x01)
#define PLC_CONFICT_MSG     (0x02) //PLC地址冲突
typedef struct event_msg
{
	uint8_t event;

	// 设备PLC编号，主要用于区分PLC设备，
	uint32_t dev;
	//
	uint8_t *pdata;
	uint16_t datalen;
} event_msg_t;

static spi_plc_lan_process_mgnt_t *g_lan_plc_process_mgnt = NULL;

int Goblin_plc_Online(void);
int reply_analy_ack(uint8_t *pdata);

static int Process_CallBack_By_Event(spi_plc_lan_process_mgnt_t *pmgnt, PlcEventHandleInfo_S *pinfo)
{
	if(pmgnt && pmgnt->m_pEventCB)
	{
		return pmgnt->m_pEventCB(pmgnt->pusrdata, pinfo);
	}
	return 0;
}

int Plc_Process_CallBack_By_PlcVersion(uint32_t plcversion)
{
	event_msg_t event = {0};
	event.event = PLC_ONLINE_MSG;
	event.dev =  plcversion;
	mq_send(g_lan_plc_process_mgnt->event_queue, &event, sizeof(event_msg_t));
	return 0;
}

int Plc_Process_CallBack_By_PlcConflict(void)
{
	event_msg_t event = {0};
	event.event = PLC_CONFICT_MSG;
	mq_send(g_lan_plc_process_mgnt->event_queue, &event, sizeof(event_msg_t));
	return 0;
}

int Plc_Process_CallBack_By_PlcRssiList(uint32_t plccount, void *prssi_list)
{
	PlcEventHandleInfo_S plcinfo = {0};
	plcinfo.type = Goblin_PLC_MSG_EVENTTYPE_PCL_RSSI;
	plcinfo.eventinfo.plc_rssilist.plc_count = plccount;
	plcinfo.eventinfo.plc_rssilist.prssi_list = (Plc_RssiList *)prssi_list;
	return Process_CallBack_By_Event(g_lan_plc_process_mgnt, &plcinfo);
}

static void on_recv_lan_msg(void *param, uint32_t dev, uint8_t *data, uint16_t data_size)
{
	spi_plc_lan_process_mgnt_t *mgnt = (spi_plc_lan_process_mgnt_t *)param;


	event_msg_t event = {0};
	event.dev = dev;
	event.datalen = data_size;
	event.pdata = bmalloc(event.datalen+1);
	memset(event.pdata, 0, event.datalen+1);

	event.event = PLC_TRAN_MSG;

	memcpy(event.pdata, data, data_size);

	LOGI("recv lan message from dev [0x%x] msg [%u][%s] \n", dev, data_size, data);

	mq_send(mgnt->event_queue, &event, sizeof(event_msg_t));
}

static void on_recv_lan_file(void *param, uint32_t dev, uint8_t *data, uint16_t data_size)
{
	spi_plc_lan_process_mgnt_t *mgnt = (spi_plc_lan_process_mgnt_t *)param;

	file_pack_t *pfileinfo = (file_pack_t *)data;

	if (pfileinfo->file_type == 0)
	{
		pthread_mutex_lock(&g_lan_plc_process_mgnt->event_mutex);
		if(g_lan_plc_process_mgnt->m_filesem)
			SetSemaphore(g_lan_plc_process_mgnt->m_filesem);
		pthread_mutex_unlock(&g_lan_plc_process_mgnt->event_mutex);
	}
	else
	{
		if(mgnt->m_pFileCb)
			mgnt->m_pFileCb(mgnt->m_FileParam, pfileinfo, dev);
		pfileinfo->file_type = 0;
		pfileinfo->filelen = 0;
		spi_plc_mgnt_send(dev, FILE_PORT, (uint8_t*)pfileinfo, sizeof(file_pack_t));
	}
}

static int event_info_add(spi_plc_lan_process_mgnt_t *mgnt, spi_event_info_s *pinfo)
{
	int ret = 0;
	//这个消息队列是局部变量， 不需要申请堆内存， 剔出队列列时也不需要释放内存
	pinfo->tick = time_relative_ms();
	pthread_mutex_lock(&mgnt->event_mutex);
	list_add_tail(&pinfo->node, &mgnt->event_list);
	pthread_mutex_unlock(&mgnt->event_mutex);

	return ret;
}

static int event_info_del_by_seq(spi_plc_lan_process_mgnt_t *mgnt, int seq)
{
	int ret = 0;

	spi_event_info_s *p = NULL;
	spi_event_info_s *n = NULL;

	pthread_mutex_lock(&mgnt->event_mutex);

	if (!list_empty_careful(&mgnt->event_list))
	{
		list_for_each_entry_safe(p, n, &mgnt->event_list, node)
		{
			if (p->seq == seq)
			{
				list_del(&p->node);
				break;
			}
		}
	}

	pthread_mutex_unlock(&mgnt->event_mutex);

	return ret;
}

static int event_info_del_by_seq_and_rspcmd(spi_plc_lan_process_mgnt_t *mgnt, int seq, int cmd, uint8_t *pdata, uint32_t datalen)
{
	int ret = 0;

	spi_event_info_s *p = NULL;
	spi_event_info_s *n = NULL;

	pthread_mutex_lock(&mgnt->event_mutex);

	if (!list_empty_careful(&mgnt->event_list))
	{
		list_for_each_entry_safe(p, n, &mgnt->event_list, node)
		{
			if (p->seq == seq && p->rspcmd == cmd)
			{
				list_del(&p->node);

				p->ackdatalen = datalen;
				//阻塞等待的接口释放
				p->packdata = bmalloc(datalen+1);
				memset(p->packdata, 0, datalen+1);
				memcpy(p->packdata, pdata, datalen);
				if(p->sem)
					SetSemaphore(p->sem);
				//break;
			}
		}
	}

	pthread_mutex_unlock(&mgnt->event_mutex);

	return ret;
}

static void process_time_event(spi_plc_lan_process_mgnt_t *mgnt)
{
	pthread_mutex_lock(&mgnt->event_mutex);
	spi_event_info_s *p = NULL;

	spi_event_info_s *n = NULL;

	if (!list_empty_careful(&mgnt->event_list))
	{
		//printf("msg_check_thread list_empty_careful g_msg_list_count：%d\n", g_msg_list_count);
		list_for_each_entry_safe(p, n, &mgnt->event_list, node)
		{
			LOGI("msg_check_thread p->seq:%d p->tick:%d p->reqcmd:%d\n", p->seq, p->tick, p->reqcmd);
		}
	}

	pthread_mutex_unlock(&mgnt->event_mutex);

	static int online_time = 0;
	if(mgnt->plc_version && online_time < 2)
	{
		online_time++;
		Goblin_plc_Online();
	}
}

static void ReplyAck(uint32_t rdev, uint32_t ackcmd, uint32_t ackseq, int result)
{
	cJSON *proot_ack = cJSON_CreateObject();

	cJSON_AddNumberToObject(proot_ack, "cmd", ackcmd);
	cJSON_AddNumberToObject(proot_ack, "result", result);
	cJSON_AddNumberToObject(proot_ack, "seq", ackseq);

	char * pbuf = cJSON_PrintUnformatted(proot_ack);
	if (pbuf)
	{
		spi_plc_mgnt_send(rdev, PROXY_PORT, (uint8_t *)pbuf, strlen(pbuf));
		cJSON_free(pbuf);
	}

	cJSON_Delete(proot_ack);
}

static void *spi_plc_process_thread(void *args)
{
	spi_plc_lan_process_mgnt_t *mgnt = (spi_plc_lan_process_mgnt_t *)args;

	LOGI("spi_plc_call_thread start [%p]\n", mgnt);

	usleep(10*1000);
	//
	event_msg_t event = {0};
	cJSON *proot =NULL;
	while (mgnt->event_run)
	{
		if (0 == mq_recv(mgnt->event_queue, &event, sizeof(event_msg_t), 100))
		{
			LOGI("recv event type [%d] data [%s]\n", event.event, event.pdata);
			if(PLC_ONLINE_MSG == event.event)
			{
				mgnt->plc_version = event.dev;
				Goblin_plc_Online();

				PlcEventHandleInfo_S plcinfo = {0};
				plcinfo.type = Goblin_PLC_MSG_EVENTTYPE_PLC_VERSION;
				plcinfo.eventinfo.plc_version.version = mgnt->plc_version;

				Process_CallBack_By_Event(g_lan_plc_process_mgnt, &plcinfo);
				continue;
			}
			else if (PLC_CONFICT_MSG == event.event)
			{
				PlcEventHandleInfo_S plcinfo = {0};
				plcinfo.type = Goblin_PLC_MSG_EVENTTYPE_PLC_CONFLICT;

				Process_CallBack_By_Event(g_lan_plc_process_mgnt, &plcinfo);
				continue;
			}


			proot = cJSON_Parse((char*)event.pdata);
			if (proot)
			{
				cJSON *pcmd = cJSON_GetObjectItem(proot, "cmd");
				cJSON *pseq = cJSON_GetObjectItem(proot, "seq");
				if(pcmd && cJSON_IsNumber(pcmd) && pseq && cJSON_IsNumber(pseq))
				{
					switch (pcmd->valueint)
					{
					case SYNC_DEV_UNLOCK:
						{
							PlcEventHandleInfo_S unlockinfo = {0};
							cJSON *pid = cJSON_GetObjectItem(proot, "id");
							cJSON *pplace = cJSON_GetObjectItem(proot, "place");
							unlockinfo.type = Goblin_PLC_MSG_EVENTTYPE_UNLOCK;
							unlockinfo.remotedev = event.dev;
							if(pid && cJSON_IsString(pid))
								unlockinfo.eventinfo.unlock.devid = strtoll(pid->valuestring, NULL, 16);
							if(pplace && cJSON_IsNumber(pplace))
								unlockinfo.eventinfo.unlock.place = pplace->valueint;

							int ackret = Process_CallBack_By_Event(mgnt, &unlockinfo);

							ReplyAck(event.dev, SYNC_DEV_UNLOCK_ACK, pseq->valueint, ackret);
						}
						break;
					case SYNC_DEV_ONLINE:
						{
							PlcEventHandleInfo_S reqinfo = {0};
							cJSON *pid = cJSON_GetObjectItem(proot, "id");
							cJSON *phardware = cJSON_GetObjectItem(proot, "hardware");
							cJSON *psoftware = cJSON_GetObjectItem(proot, "software");
							cJSON *pplcversion = cJSON_GetObjectItem(proot, "plcversion");
							cJSON *pbenableagent = cJSON_GetObjectItem(proot, "enableagent");
							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_ONLINE;
							reqinfo.remotedev = event.dev;

							if(pid && cJSON_IsString(pid))
								reqinfo.eventinfo.Onlineinfo.devid = strtoll(pid->valuestring, NULL, 16);
							if(phardware && cJSON_IsString(phardware))
								strncpy(reqinfo.eventinfo.Onlineinfo.hardware_version, phardware->valuestring, 32-1);
							if(psoftware && cJSON_IsString(psoftware))
								strncpy(reqinfo.eventinfo.Onlineinfo.software_version, psoftware->valuestring, 32-1);
							if(pplcversion && cJSON_IsNumber(pplcversion))
								reqinfo.eventinfo.Onlineinfo.plc_version = pplcversion->valueint;
							if(pbenableagent && cJSON_IsBool(pbenableagent))
								reqinfo.eventinfo.Onlineinfo.benableagent = pbenableagent->valueint;

							Process_CallBack_By_Event(mgnt, &reqinfo);
						}
						break;
					case SYNC_DEV_SEARCH:
						{
							PlcEventHandleInfo_S reqinfo = {0};
							cJSON *pid = cJSON_GetObjectItem(proot, "id");
							cJSON *psearchtype = cJSON_GetObjectItem(proot, "searchtype");
							cJSON *proomid = cJSON_GetObjectItem(proot, "roomid");
							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_SEARCH_REQ;
							reqinfo.remotedev = event.dev;
							if(pid && cJSON_IsString(pid))
								reqinfo.eventinfo.search_req.devid = strtoll(pid->valuestring, NULL, 16);
							if(psearchtype && cJSON_IsNumber(psearchtype))
								reqinfo.eventinfo.search_req.type = psearchtype->valueint;
							if(proomid && cJSON_IsNumber(proomid))
								reqinfo.eventinfo.search_req.roomid = proomid->valueint;

							if(Process_CallBack_By_Event(mgnt, &reqinfo)==1)
							{
								cJSON *proot_ack = cJSON_CreateObject();

								cJSON_AddNumberToObject(proot_ack, "cmd", SYNC_DEV_SRARCH_ACK);
								cJSON_AddNumberToObject(proot_ack, "result", 1);
								cJSON_AddNumberToObject(proot_ack, "seq", pseq->valueint);

								char strid[16] = {0};
								sprintf(strid, "%x", reqinfo.eventinfo.search_req.ackdev);
								cJSON_AddStringToObject(proot_ack, "id", strid);
								cJSON_AddNumberToObject(proot_ack, "type", reqinfo.eventinfo.search_req.acktype);
								cJSON_AddNumberToObject(proot_ack, "state", reqinfo.eventinfo.search_req.ackstate);
								cJSON_AddStringToObject(proot_ack, "hardware", reqinfo.eventinfo.search_req.ackhardware_version);
								cJSON_AddStringToObject(proot_ack, "software", reqinfo.eventinfo.search_req.acksoftware_version);
								cJSON_AddNumberToObject(proot_ack, "plcversion", reqinfo.eventinfo.search_req.ackplc_version);
								cJSON_AddBoolToObject(proot_ack, "enableagent", reqinfo.eventinfo.search_req.ackbenableagent);
								if(reqinfo.eventinfo.search_req.languageindex != 0)
									cJSON_AddNumberToObject(proot_ack, "language", reqinfo.eventinfo.search_req.languageindex);
								if(strlen(reqinfo.eventinfo.search_req.ackSN))
									cJSON_AddStringToObject(proot_ack, "sn", reqinfo.eventinfo.search_req.ackSN);
								if(strlen(reqinfo.eventinfo.search_req.ackDevName))
									cJSON_AddStringToObject(proot_ack, "devname", reqinfo.eventinfo.search_req.ackDevName);

								char * pbuf = cJSON_PrintUnformatted(proot_ack);
								if (pbuf)
								{
									spi_plc_mgnt_send(event.dev, PROXY_PORT, (uint8_t *)pbuf, strlen(pbuf));
									cJSON_free(pbuf);
								}

								cJSON_Delete(proot_ack);
							}
						}
						break;
					case SYNC_DEV_SRARCH_ACK:
						{
							PlcEventHandleInfo_S reqinfo = {0};
							cJSON *pid = cJSON_GetObjectItem(proot, "id");
							cJSON *ptype = cJSON_GetObjectItem(proot, "type");
							cJSON *pstate = cJSON_GetObjectItem(proot, "state");
							cJSON *phardware = cJSON_GetObjectItem(proot, "hardware");
							cJSON *psoftware = cJSON_GetObjectItem(proot, "software");
							cJSON *pplcversion = cJSON_GetObjectItem(proot, "plcversion");
							cJSON *penableagent = cJSON_GetObjectItem(proot, "enableagent");
							cJSON *psn = cJSON_GetObjectItem(proot, "sn");
							cJSON *pdevname = cJSON_GetObjectItem(proot, "devname");
							cJSON *planguage = cJSON_GetObjectItem(proot, "language");

							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_SEARCH_RSP;
							reqinfo.remotedev = event.dev;

							if(pid && cJSON_IsString(pid))
								reqinfo.eventinfo.search_rsp.devid = strtoll(pid->valuestring, NULL, 16);
							if(ptype && cJSON_IsNumber(ptype))
								reqinfo.eventinfo.search_rsp.type = ptype->valueint;
							if(pstate && cJSON_IsNumber(pstate))
								reqinfo.eventinfo.search_rsp.state = pstate->valueint;
							if(phardware && cJSON_IsString(phardware))
								strncpy(reqinfo.eventinfo.search_rsp.hardware_version, phardware->valuestring, 32-1);
							if(psoftware && cJSON_IsString(psoftware))
								strncpy(reqinfo.eventinfo.search_rsp.software_version, psoftware->valuestring, 32-1);
							if(pplcversion && cJSON_IsNumber(pplcversion))
								reqinfo.eventinfo.search_rsp.plc_version = pplcversion->valueint;
							if(penableagent && cJSON_IsBool(penableagent))
								reqinfo.eventinfo.search_rsp.benableagent = penableagent->valueint;
							if(psn && cJSON_IsString(psn))
								strncpy(reqinfo.eventinfo.search_rsp.strSN, psn->valuestring, sizeof(reqinfo.eventinfo.search_rsp.strSN)-1);
							if(pdevname && cJSON_IsString(pdevname))
								strncpy(reqinfo.eventinfo.search_rsp.strDevName, pdevname->valuestring, sizeof(reqinfo.eventinfo.search_rsp.strDevName)-1);
							if(planguage && cJSON_IsNumber(planguage))
								reqinfo.eventinfo.search_rsp.languageindex = planguage->valueint;

							Process_CallBack_By_Event(mgnt, &reqinfo);
						}
						break;
					case SYNC_DEV_SET_ID:
						{
							PlcEventHandleInfo_S reqinfo = {0};
							cJSON *pvillaid = cJSON_GetObjectItem(proot, "villaid");

							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_SETID;
							reqinfo.remotedev = event.dev;
							if(pvillaid && cJSON_IsString(pvillaid))
							{
								reqinfo.eventinfo.set_id.devid = strtoll(pvillaid->valuestring, NULL, 16);
								sprintf(reqinfo.eventinfo.set_id.strid, "%x", reqinfo.eventinfo.set_id.devid);
							}

							reqinfo.remotedev = event.dev;
							int ackret = Process_CallBack_By_Event(mgnt, &reqinfo);
							ReplyAck(event.dev, SYNC_DEV_SET_ID_ACK, pseq->valueint, ackret);
						}
						break;
					case SYNC_DEV_SET_CFG:
						{
							PlcEventHandleInfo_S reqinfo = {0};
							cJSON *pbase = cJSON_GetObjectItem(proot, "base");
							cJSON *pentrance = cJSON_GetObjectItem(proot, "entrance");
							cJSON *pai = cJSON_GetObjectItem(proot, "ai");

							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_SETCFG;
							reqinfo.remotedev = event.dev;
							if(pbase && cJSON_IsObject(pbase))
							{
								cJSON *psoundcall = cJSON_GetObjectItem(pbase, "soundcall");
								cJSON *psoundtalk = cJSON_GetObjectItem(pbase, "soundtalk");

								reqinfo.eventinfo.door_cfg.paraminfo.soundcall = psoundcall && cJSON_IsNumber(psoundcall) ? psoundcall->valueint : -1;
								reqinfo.eventinfo.door_cfg.paraminfo.soundtalk = psoundtalk && cJSON_IsNumber(psoundtalk) ? psoundtalk->valueint : -1;
							}
							if(pentrance && cJSON_IsObject(pentrance))
							{
								cJSON *pdoorlockarray = cJSON_GetObjectItem(pentrance, "doorlock");
								if (pdoorlockarray && cJSON_IsArray(pdoorlockarray))
								{
									int arraysize = cJSON_GetArraySize(pdoorlockarray);
									for (int i = 0; i < arraysize && i < DOOR_LOCK_COUNT; i++)
									{
										cJSON *pitem = cJSON_GetArrayItem(pdoorlockarray, i);
										if(!pitem)
											continue;
										cJSON *pdoorid = cJSON_GetObjectItem(pitem, "doorid");
										cJSON *plockswitch = cJSON_GetObjectItem(pitem, "lockswitch");
										cJSON *plocktime = cJSON_GetObjectItem(pitem, "locktime");
										cJSON *pmenicswitch = cJSON_GetObjectItem(pitem, "menicswitch");
										cJSON *pmenictime = cJSON_GetObjectItem(pitem, "menictime");
										cJSON *plocktype = cJSON_GetObjectItem(pitem, "locktype");

										reqinfo.eventinfo.door_cfg.paraminfo.doorlock[i].lockid = pdoorid && cJSON_IsNumber(pdoorid) ? pdoorid->valueint : -1;
										reqinfo.eventinfo.door_cfg.paraminfo.doorlock[i].lockswitch = plockswitch && cJSON_IsNumber(plockswitch) ? plockswitch->valueint : -1;
										reqinfo.eventinfo.door_cfg.paraminfo.doorlock[i].locktime = plocktime && cJSON_IsNumber(plocktime) ? plocktime->valueint : -1;
										reqinfo.eventinfo.door_cfg.paraminfo.doorlock[i].menicswitch = pmenicswitch && cJSON_IsNumber(pmenicswitch) ? pmenicswitch->valueint : -1;
										reqinfo.eventinfo.door_cfg.paraminfo.doorlock[i].menictime = pmenictime && cJSON_IsNumber(pmenictime) ? pmenictime->valueint : -1;
										reqinfo.eventinfo.door_cfg.paraminfo.doorlock[i].locktype = plocktype && cJSON_IsNumber(plocktype) ? plocktype->valueint : -1;
									}
								}
							}
							if(pai && cJSON_IsObject(pai))
							{
								cJSON *pmotion = cJSON_GetObjectItem(pai, "motion");

								reqinfo.eventinfo.door_cfg.paraminfo.motion = pmotion && cJSON_IsNumber(pmotion) ? pmotion->valueint : -1;
							}

							int ackret = Process_CallBack_By_Event(mgnt, &reqinfo);
							ReplyAck(event.dev, SYNC_DEV_SET_CFG_ACK, pseq->valueint, ackret);
						}
						break;
					case SYNC_DEV_GET_CFG:
						{
							PlcEventHandleInfo_S reqinfo = {0};

							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_GETCFG;
							reqinfo.remotedev = event.dev;
							if(Process_CallBack_By_Event(mgnt, &reqinfo)==1)
							{
								goblin_door_cfg_t *pdoorcfg = &reqinfo.eventinfo.door_cfg.paraminfo;

								cJSON *proot_ack = cJSON_CreateObject();

								cJSON_AddNumberToObject(proot_ack, "cmd", SYNC_DEV_GET_CFG_ACK);
								cJSON_AddNumberToObject(proot_ack, "result", 1);
								cJSON_AddNumberToObject(proot_ack, "seq", pseq->valueint);

								cJSON *pbase = cJSON_AddObjectToObject(proot_ack, "base");
								cJSON *pentrance = cJSON_AddObjectToObject(proot_ack, "entrance");
								cJSON *pai = cJSON_AddObjectToObject(proot_ack, "ai");
								if(pbase)
								{
									cJSON_AddNumberToObject(pbase, "soundcall", pdoorcfg->soundcall);
									cJSON_AddNumberToObject(pbase, "soundtalk", pdoorcfg->soundtalk);
								}
								if(pentrance)
								{
									cJSON *pdoorlockarray = cJSON_AddArrayToObject(pentrance, "doorlock");
									for (int i = 0; i < DOOR_LOCK_COUNT; i++)
									{
										cJSON *pdoorlock = cJSON_CreateObject();
										if(!pdoorlock)
											continue;

										cJSON_AddNumberToObject(pdoorlock, "doorid", pdoorcfg->doorlock[i].lockid);
										cJSON_AddNumberToObject(pdoorlock, "lockswitch", pdoorcfg->doorlock[i].lockswitch);
										cJSON_AddNumberToObject(pdoorlock, "locktime", pdoorcfg->doorlock[i].locktime);
										cJSON_AddNumberToObject(pdoorlock, "menicswitch", pdoorcfg->doorlock[i].menicswitch);
										cJSON_AddNumberToObject(pdoorlock, "menictime", pdoorcfg->doorlock[i].menictime);
										cJSON_AddNumberToObject(pdoorlock, "locktype", pdoorcfg->doorlock[i].locktype);

										cJSON_AddItemToArray(pdoorlockarray, pdoorlock);
									}
								}
								if(pai)
								{
									cJSON_AddNumberToObject(pai, "motion", pdoorcfg->motion);
								}

								char * pbuf = cJSON_PrintUnformatted(proot_ack);
								if (pbuf)
								{
									spi_plc_mgnt_send(event.dev, PROXY_PORT, (uint8_t *)pbuf, strlen(pbuf));
									cJSON_free(pbuf);
								}

								cJSON_Delete(proot_ack);
							}
						}
						break;
					case SYNC_DEV_WAKEUP:
						{
							PlcEventHandleInfo_S reqinfo = {0};
							cJSON *proomid = cJSON_GetObjectItem(proot, "roomid");
							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_WAKEUP_REQ;
							reqinfo.remotedev = event.dev;
							if(proomid && cJSON_IsNumber(proomid))
								reqinfo.eventinfo.wakeup_info.roomid = proomid->valueint;

							int ackret = Process_CallBack_By_Event(mgnt, &reqinfo);
							ReplyAck(event.dev, SYNC_DEV_WAKEUP_ACK, pseq->valueint, ackret);
						}
						break;
					case SYNC_DEV_REPORT_INFO:
						{
							PlcEventHandleInfo_S reqinfo = {0};
							cJSON *pdevinfo = cJSON_GetObjectItem(proot, "devinfo");
							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_REPORTAGENT;
							reqinfo.remotedev = event.dev;

							if(pdevinfo && cJSON_IsObject(pdevinfo))
							{
								cJSON *pdevtype = cJSON_GetObjectItem(pdevinfo, "devtype");
								cJSON *pplatform = cJSON_GetObjectItem(pdevinfo, "platform");
								cJSON *pmodel = cJSON_GetObjectItem(pdevinfo, "model");
								cJSON *pmac = cJSON_GetObjectItem(pdevinfo, "mac");
								cJSON *papptype = cJSON_GetObjectItem(pdevinfo, "apptype");
								cJSON *pappver = cJSON_GetObjectItem(pdevinfo, "appver");
								cJSON *psystype = cJSON_GetObjectItem(pdevinfo, "systype");
								cJSON *psysver = cJSON_GetObjectItem(pdevinfo, "sysver");
								cJSON *phwver = cJSON_GetObjectItem(pdevinfo, "hwver");
								cJSON *pid = cJSON_GetObjectItem(pdevinfo, "id");
								cJSON *psn = cJSON_GetObjectItem(pdevinfo, "sn");
								cJSON *pplcver = cJSON_GetObjectItem(pdevinfo, "plcver");
								cJSON *ptimezone = cJSON_GetObjectItem(pdevinfo, "timezone");

								NUMINFOINSERT_BYJSON(pdevtype, reqinfo.eventinfo.agent_dev.agentinfo.devtype)
								NUMINFOINSERT_BYJSON(pplatform, reqinfo.eventinfo.agent_dev.agentinfo.platform)

								STRINFOINSERT_BYJSON(pmodel, reqinfo.eventinfo.agent_dev.agentinfo.strmodel, sizeof(reqinfo.eventinfo.agent_dev.agentinfo.strmodel)-1)
								STRINFOINSERT_BYJSON(pmac, reqinfo.eventinfo.agent_dev.agentinfo.strmac, sizeof(reqinfo.eventinfo.agent_dev.agentinfo.strmac)-1)
								STRINFOINSERT_BYJSON(papptype, reqinfo.eventinfo.agent_dev.agentinfo.strapptype, sizeof(reqinfo.eventinfo.agent_dev.agentinfo.strapptype)-1)
								STRINFOINSERT_BYJSON(pappver, reqinfo.eventinfo.agent_dev.agentinfo.strappver, sizeof(reqinfo.eventinfo.agent_dev.agentinfo.strappver)-1)
								STRINFOINSERT_BYJSON(psystype, reqinfo.eventinfo.agent_dev.agentinfo.strsystype, sizeof(reqinfo.eventinfo.agent_dev.agentinfo.strsystype)-1)
								STRINFOINSERT_BYJSON(psysver, reqinfo.eventinfo.agent_dev.agentinfo.strsysver, sizeof(reqinfo.eventinfo.agent_dev.agentinfo.strsysver)-1)
								STRINFOINSERT_BYJSON(phwver, reqinfo.eventinfo.agent_dev.agentinfo.strhwver, sizeof(reqinfo.eventinfo.agent_dev.agentinfo.strhwver)-1)
								STRINFOINSERT_BYJSON(pid, reqinfo.eventinfo.agent_dev.agentinfo.strid, sizeof(reqinfo.eventinfo.agent_dev.agentinfo.strid)-1)
								STRINFOINSERT_BYJSON(psn, reqinfo.eventinfo.agent_dev.agentinfo.strSN, sizeof(reqinfo.eventinfo.agent_dev.agentinfo.strSN)-1)
								STRINFOINSERT_BYJSON(pplcver, reqinfo.eventinfo.agent_dev.agentinfo.strplcver, sizeof(reqinfo.eventinfo.agent_dev.agentinfo.strplcver)-1)
								STRINFOINSERT_BYJSON(ptimezone, reqinfo.eventinfo.agent_dev.agentinfo.strtimezone, sizeof(reqinfo.eventinfo.agent_dev.agentinfo.strtimezone)-1)
							}


							if(Process_CallBack_By_Event(mgnt, &reqinfo)==1)
							{
								goblin_agent_ack_s *packinfo = &reqinfo.eventinfo.agent_dev.outinfo;

								cJSON *proot_ack = cJSON_CreateObject();

								cJSON_AddNumberToObject(proot_ack, "cmd", SYNC_DEV_REPORT_INFO_ACK);
								cJSON_AddNumberToObject(proot_ack, "result", 1);
								cJSON_AddNumberToObject(proot_ack, "seq", pseq->valueint);

								cJSON_AddStringToObject(proot_ack, "timezone", packinfo->strtimezone);
								cJSON_AddStringToObject(proot_ack, "datetime", packinfo->strdatetime);
								cJSON_AddBoolToObject(proot_ack, "enableagent", packinfo->benableagent);

								char * pbuf = cJSON_PrintUnformatted(proot_ack);
								if (pbuf)
								{
									spi_plc_mgnt_send(event.dev, PROXY_PORT, (uint8_t *)pbuf, strlen(pbuf));
									cJSON_free(pbuf);
								}

								cJSON_Delete(proot_ack);

							}
						}
						break;
					case SYNC_DEV_REPORT_DISABLEAGENT:
						{
							PlcEventHandleInfo_S reqinfo = {0};
							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_DISABLEAGENT;
							reqinfo.remotedev = event.dev;

							int ackret = Process_CallBack_By_Event(mgnt, &reqinfo);
							ReplyAck(event.dev, SYNC_DEV_REPORT_DISABLEAGENT_ACK, pseq->valueint, ackret);
						}
						break;
					case SYNC_DEV_CHECK_STATE:
						{
							PlcEventHandleInfo_S reqinfo = {0};

							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_CHECKSTATE;
							reqinfo.remotedev = event.dev;

							cJSON *paction = cJSON_GetObjectItem(proot, "action");
							if (paction && cJSON_IsString(paction))
							{
								strncpy(reqinfo.eventinfo.action_state.straction, paction->valuestring, sizeof(reqinfo.eventinfo.action_state.straction)-1);
							}

							if(Process_CallBack_By_Event(mgnt, &reqinfo)==1)
							{
								cJSON *proot_ack = cJSON_CreateObject();

								cJSON_AddNumberToObject(proot_ack, "cmd", SYNC_DEV_CHECK_STATE_ACK);
								cJSON_AddNumberToObject(proot_ack, "result", 1);
								cJSON_AddNumberToObject(proot_ack, "seq", pseq->valueint);

								cJSON_AddNumberToObject(proot_ack, "state", reqinfo.eventinfo.action_state.outstate);

								char * pbuf = cJSON_PrintUnformatted(proot_ack);
								if (pbuf)
								{
									spi_plc_mgnt_send(event.dev, PROXY_PORT, (uint8_t *)pbuf, strlen(pbuf));
									cJSON_free(pbuf);
								}

								cJSON_Delete(proot_ack);
							}
						}
						break;
					case SYNC_DEV_GET_LOGFILE:
					case SYNC_DEV_GET_LOGFILESIZE:
						{
							PlcEventHandleInfo_S reqinfo = {0};

							reqinfo.type = (pcmd->valueint==SYNC_DEV_GET_LOGFILE) ? Goblin_PLC_MSG_EVENTTYPE_GETLOGFILE : Goblin_PLC_MSG_EVENTTYPE_GETLOGFILESIZE;
							reqinfo.remotedev = event.dev;

							if(Process_CallBack_By_Event(mgnt, &reqinfo)==1)
							{
								cJSON *proot_ack = cJSON_CreateObject();

								cJSON_AddNumberToObject(proot_ack, "cmd",  (pcmd->valueint==SYNC_DEV_GET_LOGFILE) ? SYNC_DEV_GET_LOGFILE_ACK : SYNC_DEV_GET_LOGFILESIZE_ACK);
								cJSON_AddNumberToObject(proot_ack, "result", 1);
								cJSON_AddNumberToObject(proot_ack, "seq", pseq->valueint);

								cJSON_AddNumberToObject(proot_ack, "filelen", reqinfo.eventinfo.logfile_info.outfilelen);

								char * pbuf = cJSON_PrintUnformatted(proot_ack);
								if (pbuf)
								{
									spi_plc_mgnt_send(event.dev, PROXY_PORT, (uint8_t *)pbuf, strlen(pbuf));
									cJSON_free(pbuf);
								}

								cJSON_Delete(proot_ack);
							}
						}
						break;
					case SYNC_DEV_SET_SN:
						{
							PlcEventHandleInfo_S reqinfo = {0};

							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_SET_SN;
							reqinfo.remotedev = event.dev;

							cJSON *psn = cJSON_GetObjectItem(proot, "sn");

							STRINFOINSERT_BYJSON(psn, reqinfo.eventinfo.sn_info.strsn, sizeof(reqinfo.eventinfo.sn_info.strsn)-1)

							ReplyAck(event.dev, SYNC_DEV_SET_SN_ACK, pseq->valueint, Process_CallBack_By_Event(mgnt, &reqinfo));
						}
						break;
					case SYNC_DEV_GET_SN:
						{
							PlcEventHandleInfo_S reqinfo = {0};

							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_GET_SN;
							reqinfo.remotedev = event.dev;

							if(Process_CallBack_By_Event(mgnt, &reqinfo)==1)
							{
								cJSON *proot_ack = cJSON_CreateObject();

								cJSON_AddNumberToObject(proot_ack, "cmd",  SYNC_DEV_GET_SN_ACK);
								cJSON_AddNumberToObject(proot_ack, "result", 1);
								cJSON_AddNumberToObject(proot_ack, "seq", pseq->valueint);

								cJSON_AddStringToObject(proot_ack, "sn", reqinfo.eventinfo.sn_info.strsn);

								char * pbuf = cJSON_PrintUnformatted(proot_ack);
								if (pbuf)
								{
									spi_plc_mgnt_send(event.dev, PROXY_PORT, (uint8_t *)pbuf, strlen(pbuf));
									cJSON_free(pbuf);
								}

								cJSON_Delete(proot_ack);
							}
						}
						break;
					case SYNC_DEV_SET_LANGUANGE:
						{
							PlcEventHandleInfo_S reqinfo = {0};
							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_SET_LANGUPAGE;
							reqinfo.remotedev = event.dev;

							cJSON *planguage = cJSON_GetObjectItem(proot, "language");

							NUMINFOINSERT_BYJSON(planguage, reqinfo.eventinfo.language_info.languageindex)

							ReplyAck(event.dev, SYNC_DEV_SET_LANGUANGE_ACK, pseq->valueint, Process_CallBack_By_Event(mgnt, &reqinfo));
						}
						break;
					case SYNC_DEV_SET_NOISE_AI:
						{
							PlcEventHandleInfo_S reqinfo = {0};
							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_SET_AI_NOISE;
							reqinfo.remotedev = event.dev;

							cJSON *planguage = cJSON_GetObjectItem(proot, "onoff");

							if(cJSON_IsBool(planguage))
								reqinfo.eventinfo.action_ai_noise.benable = planguage->valueint;

							ReplyAck(event.dev, SYNC_DEV_SET_NOISE_AI_ACK, pseq->valueint, Process_CallBack_By_Event(mgnt, &reqinfo));
						}
						break;
					case SYNC_DEV_SET_ADDCARD_STATE:
						{
							PlcEventHandleInfo_S reqinfo = {0};

							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_CARD_ADDSTATE;
							reqinfo.remotedev = event.dev;


							cJSON *pstate = cJSON_GetObjectItem(proot, "state");

							NUMINFOINSERT_BYJSON(pstate, reqinfo.eventinfo.card_add_state.state)

							ReplyAck(event.dev, SYNC_DEV_SET_ADDCARD_STATE_ACK, pseq->valueint, Process_CallBack_By_Event(mgnt, &reqinfo));
						}
						break;
					case SYNC_DEV_SET_DELCARD_STATE:
						{
							PlcEventHandleInfo_S reqinfo = {0};
							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_CARD_DELSTATE;
							reqinfo.remotedev = event.dev;

							cJSON *pstate = cJSON_GetObjectItem(proot, "state");

							NUMINFOINSERT_BYJSON(pstate, reqinfo.eventinfo.card_del_state.state)

							ReplyAck(event.dev, SYNC_DEV_SET_DELCARD_STATE_ACK, pseq->valueint, Process_CallBack_By_Event(mgnt, &reqinfo));
						}
						break;
					case SYNC_DEV_DEL_ALL_CARD:
						{
							PlcEventHandleInfo_S reqinfo = {0};
							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_CARD_DELALL;
							reqinfo.remotedev = event.dev;

							ReplyAck(event.dev, SYNC_DEV_DEL_ALL_CARD_ACK, pseq->valueint, Process_CallBack_By_Event(mgnt, &reqinfo));
						}
						break;
					case SYNC_DEV_DEL_CARD_BY_ID:
						{
							PlcEventHandleInfo_S reqinfo = {0};
							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_CARD_DELONE;
							reqinfo.remotedev = event.dev;

							cJSON *pcardarray = cJSON_GetObjectItem(proot, "num");
							if(pcardarray && cJSON_IsArray(pcardarray))
							{
								int arraysize = cJSON_GetArraySize(pcardarray);
								for (int i = 0; i < arraysize ; i++)
								{
									cJSON *pitem = cJSON_GetArrayItem(pcardarray, i);
									if(!pitem)
										continue;
									//目前只支持删除一个
									STRINFOINSERT_BYJSON(pitem, reqinfo.eventinfo.card_del_one.strcard, sizeof(reqinfo.eventinfo.card_del_one.strcard)-1);
									break;
								}
							}

							ReplyAck(event.dev, SYNC_DEV_DEL_CARD_BY_ID_ACK, pseq->valueint, Process_CallBack_By_Event(mgnt, &reqinfo));
						}
						break;
					case SYNC_DEV_PLC_GET_ALL_CARD:
						{
							PlcEventHandleInfo_S reqinfo = {0};
							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_CARD_GETLIST;
							reqinfo.remotedev = event.dev;

							if(Process_CallBack_By_Event(mgnt, &reqinfo)==1)
							{
								cJSON *proot_ack = cJSON_CreateObject();

								cJSON_AddNumberToObject(proot_ack, "cmd",  SYNC_DEV_PLC_GET_ALL_CARD_ACK);
								cJSON_AddNumberToObject(proot_ack, "result", 1);
								cJSON_AddNumberToObject(proot_ack, "seq", pseq->valueint);

								cJSON *pcardsarray = cJSON_AddArrayToObject(proot_ack, "cards");
								for (int i = 0; i < reqinfo.eventinfo.card_get_list_ack.card_count && reqinfo.eventinfo.card_get_list_ack.pcardlist; i++)
								{
									if(strlen(reqinfo.eventinfo.card_get_list_ack.pcardlist[i].strcard)==0)
										continue;
									cJSON *pcard = cJSON_CreateObject();
									if(!pcard)
										continue;

									cJSON_AddStringToObject(pcard, "num", reqinfo.eventinfo.card_get_list_ack.pcardlist[i].strcard);
									cJSON_AddStringToObject(pcard, "room", reqinfo.eventinfo.card_get_list_ack.pcardlist[i].strroomid);
									cJSON_AddNumberToObject(pcard, "place", reqinfo.eventinfo.card_get_list_ack.pcardlist[i].placestate);

									cJSON_AddItemToArray(pcardsarray, pcard);
								}
								if(reqinfo.eventinfo.card_get_list_ack.pcardlist)
									free(reqinfo.eventinfo.card_get_list_ack.pcardlist);

								char * pbuf = cJSON_PrintUnformatted(proot_ack);
								if (pbuf)
								{
									spi_plc_mgnt_send(event.dev, PROXY_PORT, (uint8_t *)pbuf, strlen(pbuf));
									cJSON_free(pbuf);
								}

								cJSON_Delete(proot_ack);
							}
						}
						break;
					case SYNC_DEV_PLC_MODIFY_CARD_BY_ID:
						{
							PlcEventHandleInfo_S reqinfo = {0};
							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_CARD_MODIFY;
							reqinfo.remotedev = event.dev;

							cJSON *pcardnum = cJSON_GetObjectItem(proot, "num");
							cJSON *proom = cJSON_GetObjectItem(proot, "room");
							cJSON *pplacestate = cJSON_GetObjectItem(proot, "placestate");

							STRINFOINSERT_BYJSON(pcardnum, reqinfo.eventinfo.card_info_modify.cardinfo.strcard, sizeof(reqinfo.eventinfo.card_info_modify.cardinfo.strcard)-1);
							STRINFOINSERT_BYJSON(proom, reqinfo.eventinfo.card_info_modify.cardinfo.strroomid, sizeof(reqinfo.eventinfo.card_info_modify.cardinfo.strroomid)-1);
							if(pplacestate && cJSON_IsNumber(pplacestate))
							{
								reqinfo.eventinfo.card_info_modify.cardinfo.placestate = pplacestate->valueint;
							}
							ReplyAck(event.dev, SYNC_DEV_PLC_MODIFY_CARD_BY_ID_ACK, pseq->valueint, Process_CallBack_By_Event(mgnt, &reqinfo));
						}
						break;
					case SYNC_DEV_PLC_REPORT_CARD_BY_OPT:
						{
							PlcEventHandleInfo_S reqinfo = {0};
							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_CARD_OPT;
							reqinfo.remotedev = event.dev;

							cJSON *popt = cJSON_GetObjectItem(proot, "opt");
							cJSON *pcardid = cJSON_GetObjectItem(proot, "cardid");

							STRINFOINSERT_BYJSON(popt, reqinfo.eventinfo.card_operation_report.card_opt_info.stropt, sizeof(reqinfo.eventinfo.card_operation_report.card_opt_info.stropt)-1);
							STRINFOINSERT_BYJSON(pcardid, reqinfo.eventinfo.card_operation_report.card_opt_info.strcard, sizeof(reqinfo.eventinfo.card_operation_report.card_opt_info.strcard)-1);
							ReplyAck(event.dev, SYNC_DEV_PLC_REPORT_CARD_BY_OPT_ACK, pseq->valueint, Process_CallBack_By_Event(mgnt, &reqinfo));
						}
						break;
					case SYNC_DEV_PLC_TEST:
						{
							PlcEventHandleInfo_S reqinfo = {0};

							reqinfo.type = Goblin_PLC_MSG_EVENTTYPE_HARDWARE_TEST;
							reqinfo.remotedev = event.dev;

							cJSON *poperate = cJSON_GetObjectItem(proot, "operate");
							cJSON *pindex = cJSON_GetObjectItem(proot, "index");
							cJSON *pstate = cJSON_GetObjectItem(proot, "state");

							NUMINFOINSERT_BYJSON(poperate, reqinfo.eventinfo.hardware_test.operate_cmd)
							NUMINFOINSERT_BYJSON(pindex, reqinfo.eventinfo.hardware_test.index)
							STRINFOINSERT_BYJSON(pstate, reqinfo.eventinfo.hardware_test.strstate, sizeof(reqinfo.eventinfo.hardware_test.strstate)-1);
							if(Process_CallBack_By_Event(mgnt, &reqinfo)==1)
							{
								ReplyAck(event.dev, SYNC_DEV_PLC_TEST_ACK, pseq->valueint, 1);
							}
						}
						break;
					default:
						event_info_del_by_seq_and_rspcmd(mgnt, pseq->valueint, pcmd->valueint, event.pdata, event.datalen);
						break;
					}
				}

				cJSON_Delete(proot);
				proot = NULL;
			}
			else{
				LOGW("Format is not josn \n");
			}
			bfree(event.pdata);
			memset(&event, 0, sizeof(event_msg_t));
		}

		process_time_event(mgnt);
	}

	LOGI("goblin_plc_process_thread exit \n");
	return NULL;
}

DPLAN_PUBLIC_API int goblin_plc_process_start(PlcEventHandler *param)
{
    int ret = -1;

    if (g_lan_plc_process_mgnt == NULL)
	{
		g_lan_plc_process_mgnt = bmalloc(sizeof(spi_plc_lan_process_mgnt_t));

		if (g_lan_plc_process_mgnt == NULL)
		{
			LOGE("malloc spi_plc_lan_process_mgnt_t fail \n");
			return -1;
		}
		memset(g_lan_plc_process_mgnt, 0, sizeof(spi_plc_lan_process_mgnt_t));

		g_lan_plc_process_mgnt->local_dev = param->m_pdevplc;
		g_lan_plc_process_mgnt->m_pEventCB = param->m_pfuncb;
		g_lan_plc_process_mgnt->pusrdata = param->m_pcbparam;
		g_lan_plc_process_mgnt->m_pFileCb = param->m_pfilecb;
		g_lan_plc_process_mgnt->m_FileParam = param->m_filecbparam;
		g_lan_plc_process_mgnt->is_agent = param->m_benableagent;
		if(param->m_pdevver_hw)
			strncpy(g_lan_plc_process_mgnt->local_hard_ver, param->m_pdevver_hw, 32-1);
		if(param->m_pdevver_sys)
			strncpy(g_lan_plc_process_mgnt->local_sys_ver, param->m_pdevver_sys, 32-1);
		if(param->m_pdevver_app)
			strncpy(g_lan_plc_process_mgnt->local_soft_ver, param->m_pdevver_app, 32-1);

		INIT_LIST_HEAD(&g_lan_plc_process_mgnt->event_list);

		pthread_mutex_init(&g_lan_plc_process_mgnt->event_mutex, NULL);

		g_lan_plc_process_mgnt->event_queue = mq_create("plc_process_event", sizeof(event_msg_t), MAX_LAN_EVENT);

		if(spi_plc_mgnt_init(0x0, param->m_pdevplc) != 0)
		{
			LOGE("spi_plc_mgnt_init fail \n");
			mq_destroy(g_lan_plc_process_mgnt->event_queue);
			pthread_mutex_destroy(&g_lan_plc_process_mgnt->event_mutex);
			bfree(g_lan_plc_process_mgnt);
			g_lan_plc_process_mgnt = NULL;
			return -1;
		}

		spi_plc_mgnt_register_callback(PROXY_PORT, on_recv_lan_msg, g_lan_plc_process_mgnt);

		spi_plc_mgnt_register_callback(FILE_PORT, on_recv_lan_file, g_lan_plc_process_mgnt);

		g_lan_plc_process_mgnt->event_run = true;
		ret = pthread_create(&g_lan_plc_process_mgnt->thread, NULL, spi_plc_process_thread, g_lan_plc_process_mgnt);
		if(ret != 0)
		{
			g_lan_plc_process_mgnt->event_run = false;
			LOGW("pthread_create fail \n");
		}

		LOGI("goblin_plc_process_start success, build at %s %s \n", __DATE__, __TIME__);
	}

    return ret;
}

DPLAN_PUBLIC_API int goblin_plc_process_stop(void)
{
	int ret = -1;
	if(g_lan_plc_process_mgnt)
	{
		spi_plc_mgnt_unregister_callback(PROXY_PORT);
		spi_plc_mgnt_unregister_callback(FILE_PORT);
		g_lan_plc_process_mgnt->event_run = false;
		pthread_join(g_lan_plc_process_mgnt->thread, NULL);

		//2.清空线程队列缓存
		spi_event_info_s *node = NULL, *next = NULL;
		pthread_mutex_lock(&g_lan_plc_process_mgnt->event_mutex);
		list_for_each_entry_safe(node, next, &g_lan_plc_process_mgnt->event_list, node)
		{
			list_del(&node->node);
			if(node->sem)
				SetSemaphore(node->sem);
			//bfree(node);
		}
		pthread_mutex_unlock(&g_lan_plc_process_mgnt->event_mutex);

		mq_destroy(g_lan_plc_process_mgnt->event_queue);
		pthread_mutex_destroy(&g_lan_plc_process_mgnt->event_mutex);
		bfree(g_lan_plc_process_mgnt);
		g_lan_plc_process_mgnt = NULL;

		spi_plc_mgnt_deinit();
	}

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_reload_devID(uint32_t devid)
{
	int ret =-1;
	if(g_lan_plc_process_mgnt)
	{
		g_lan_plc_process_mgnt->local_dev = devid;
		ret = spi_plc_mgnt_retset_localdevid(devid);

		Goblin_plc_Online();
	}

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_reload_agent(bool benableagent)
{
	int ret = -1;

	if(g_lan_plc_process_mgnt)
	{
		g_lan_plc_process_mgnt->is_agent = benableagent;
		ret = 0;
	}

	return ret;
}



DPLAN_PUBLIC_API int goblin_plc_check_agent(uint32_t remotedev, goblin_agent_info_s *pinfo, goblin_agent_ack_s *packinfo, unsigned int timeout)
{
	if(!g_lan_plc_process_mgnt || !pinfo || !packinfo)
		return -1;
	int ret = -1;

	int seq = g_lan_plc_process_mgnt->send_seq++;
	cJSON *pRoot = cJSON_CreateObject();
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_REPORT_INFO);
	cJSON_AddNumberToObject(pRoot, "seq", seq);

	cJSON *pdevinfo = cJSON_AddObjectToObject(pRoot, "devinfo");

	INSERT_NUMBER2OBJECT(pdevinfo, "devtype", pinfo->devtype);
	INSERT_NUMBER2OBJECT(pdevinfo, "platform", pinfo->platform);

	INSERT_STRING2OBJECT(pdevinfo, "model", pinfo->strmodel);
	INSERT_STRING2OBJECT(pdevinfo, "mac", pinfo->strmac);
	INSERT_STRING2OBJECT(pdevinfo, "apptype", pinfo->strapptype);
	INSERT_STRING2OBJECT(pdevinfo, "appver", pinfo->strappver);
	INSERT_STRING2OBJECT(pdevinfo, "systype", pinfo->strsystype);
	INSERT_STRING2OBJECT(pdevinfo, "sysver", pinfo->strsysver);
	INSERT_STRING2OBJECT(pdevinfo, "hwver", pinfo->strhwver);
	INSERT_STRING2OBJECT(pdevinfo, "id", pinfo->strid);
	INSERT_STRING2OBJECT(pdevinfo, "sn", pinfo->strSN);
	INSERT_STRING2OBJECT(pdevinfo, "plcver", pinfo->strplcver);
	INSERT_STRING2OBJECT(pdevinfo, "timezone", pinfo->strtimezone);

	char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		if(timeout)
		{
			einfo.seq = seq;
			einfo.reqcmd = SYNC_DEV_REPORT_INFO;
			einfo.rspcmd = SYNC_DEV_REPORT_INFO_ACK;
			einfo.sem = CreateSemaphore(0);

			event_info_add(g_lan_plc_process_mgnt, &einfo);
		}

        spi_plc_mgnt_send(remotedev, PROXY_PORT, (uint8_t *)pjsonbuf, strlen(pjsonbuf));
		uint32_t ltick = time_relative_ms();
		if(einfo.sem)
		{
			if(GetSemaphore(einfo.sem, timeout))
				ret = 1;
			else
				event_info_del_by_seq(g_lan_plc_process_mgnt, seq);
			DestorySemaphore(einfo.sem);
		}

        if(einfo.packdata)
        {
			LOGI("#######ret:%u timeout:%u - %u ack data:%s \n", ret, timeout, time_relative_ms()-ltick, einfo.packdata);
			cJSON *pAck = cJSON_Parse((char*)einfo.packdata);
			if (pAck)
			{
				/* code */
				cJSON *pdatetime = cJSON_GetObjectItem(pAck, "datetime");
				cJSON *penableagent = cJSON_GetObjectItem(pAck, "enableagent");
				cJSON *ptimezone = cJSON_GetObjectItem(pAck, "timezone");
				if(pdatetime && cJSON_IsString(pdatetime))
				{
					strncpy(packinfo->strdatetime, pdatetime->valuestring, sizeof(packinfo->strdatetime)-1);
				}
				if(ptimezone && cJSON_IsString(ptimezone)){
					strncpy(packinfo->strtimezone, ptimezone->valuestring, sizeof(packinfo->strtimezone)-1);
				}
				if(penableagent && cJSON_IsBool(penableagent))
					packinfo->benableagent = penableagent->valueint;
				cJSON_Delete(pAck);
				ret = 0;
			}

            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_disable_agent_dev(uint32_t remotedev, uint32_t timeout)
{
	if(!g_lan_plc_process_mgnt)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_REPORT_DISABLEAGENT);
	cJSON_AddNumberToObject(pRoot, "seq", seq);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		if(timeout)
		{
			einfo.seq = seq;
			einfo.reqcmd = SYNC_DEV_REPORT_DISABLEAGENT;
			einfo.rspcmd = SYNC_DEV_REPORT_DISABLEAGENT_ACK;
			einfo.sem = CreateSemaphore(0);

			event_info_add(g_lan_plc_process_mgnt, &einfo);
		}
        ret = spi_plc_mgnt_send(remotedev, PROXY_PORT, (uint8_t *)pjsonbuf, strlen(pjsonbuf));

		if(einfo.sem)
		{
			if(GetSemaphore(einfo.sem, timeout))
				ret = 0;
			else{
				event_info_del_by_seq(g_lan_plc_process_mgnt, seq);
				ret = -2;
			}
			DestorySemaphore(einfo.sem);
		}

        if(einfo.packdata)
        {
			ret = reply_analy_ack(einfo.packdata);
            bfree(einfo.packdata);
        }

        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_checkstate(uint32_t remotedev, char *paction, unsigned int timeout)
{
	if(!g_lan_plc_process_mgnt)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_CHECK_STATE);
	cJSON_AddNumberToObject(pRoot, "seq", seq);

    cJSON_AddStringToObject(pRoot, "action", paction);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		if(timeout)
		{
			einfo.seq = seq;
			einfo.reqcmd = SYNC_DEV_CHECK_STATE;
			einfo.rspcmd = SYNC_DEV_CHECK_STATE_ACK;
			einfo.sem = CreateSemaphore(0);

			event_info_add(g_lan_plc_process_mgnt, &einfo);
		}

        spi_plc_mgnt_send(remotedev, PROXY_PORT, (uint8_t *)pjsonbuf, strlen(pjsonbuf));

		if(einfo.sem)
		{
			if(GetSemaphore(einfo.sem, timeout))
				ret = 1;
			else
				event_info_del_by_seq(g_lan_plc_process_mgnt, seq);
			DestorySemaphore(einfo.sem);
		}

        if(einfo.packdata)
        {
			cJSON *pAck = cJSON_Parse((char*)einfo.packdata);
			if (pAck)
			{
				/* code */
				cJSON *pstate = cJSON_GetObjectItem(pAck, "state");
				cJSON *presult = cJSON_GetObjectItem(pAck, "result");
				if (presult && cJSON_IsNumber(presult) && presult->valueint==1
				    && pstate && cJSON_IsNumber(pstate) && pstate->valueint == 0)
				{
					/* code */
					ret = 0;
				}
				cJSON_Delete(pAck);
			}
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_getlogfilesize(uint32_t remotedev, unsigned int timeout)
{
	int ret = -1;

	if(!g_lan_plc_process_mgnt)
		return -1;

    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_GET_LOGFILESIZE);
	cJSON_AddNumberToObject(pRoot, "seq", seq);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		if(timeout)
		{
			einfo.seq = seq;
			einfo.reqcmd = SYNC_DEV_GET_LOGFILESIZE;
			einfo.rspcmd = SYNC_DEV_GET_LOGFILESIZE_ACK;
			einfo.sem = CreateSemaphore(0);

			event_info_add(g_lan_plc_process_mgnt, &einfo);
		}

        spi_plc_mgnt_send(remotedev, PROXY_PORT, (uint8_t *)pjsonbuf, strlen(pjsonbuf));

		if(einfo.sem)
		{
			if(GetSemaphore(einfo.sem, timeout))
				ret = 0;
			else
				event_info_del_by_seq(g_lan_plc_process_mgnt, seq);
			DestorySemaphore(einfo.sem);
		}

        if(einfo.packdata)
        {
			cJSON *pAck = cJSON_Parse((char*)einfo.packdata);
			if (pAck)
			{
				cJSON *pfilelen = cJSON_GetObjectItem(pAck, "filelen");
				cJSON *presult = cJSON_GetObjectItem(pAck, "result");
				if (presult && cJSON_IsNumber(presult) && presult->valueint==1
				    && pfilelen && cJSON_IsNumber(pfilelen) )
				{
					ret = pfilelen->valueint;
				}
				cJSON_Delete(pAck);
			}
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_getlogfile(uint32_t remotedev, unsigned int timeout)
{
	int ret = -1;

	if(!g_lan_plc_process_mgnt)
		return -1;

    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_GET_LOGFILE);
	cJSON_AddNumberToObject(pRoot, "seq", seq);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		if(timeout)
		{
			einfo.seq = seq;
			einfo.reqcmd = SYNC_DEV_GET_LOGFILE;
			einfo.rspcmd = SYNC_DEV_GET_LOGFILE_ACK;
			einfo.sem = CreateSemaphore(0);

			event_info_add(g_lan_plc_process_mgnt, &einfo);
		}

        spi_plc_mgnt_send(remotedev, PROXY_PORT, (uint8_t *)pjsonbuf, strlen(pjsonbuf));

		if(einfo.sem)
		{
			if(GetSemaphore(einfo.sem, timeout))
				ret = 0;
			else
				event_info_del_by_seq(g_lan_plc_process_mgnt, seq);
			DestorySemaphore(einfo.sem);
		}

        if(einfo.packdata)
        {
			cJSON *pAck = cJSON_Parse((char*)einfo.packdata);
			if (pAck)
			{
				/* code */
				cJSON *pfilelen = cJSON_GetObjectItem(pAck, "filelen");
				cJSON *presult = cJSON_GetObjectItem(pAck, "result");
				if (presult && cJSON_IsNumber(presult) && presult->valueint==1
				    && pfilelen && cJSON_IsNumber(pfilelen) )
				{
					ret = pfilelen->valueint;
				}
				cJSON_Delete(pAck);
			}
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_Unlock(uint32_t remotedev, int lockp, unsigned int timeout)
{
	if(!g_lan_plc_process_mgnt)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_UNLOCK);
	cJSON_AddNumberToObject(pRoot, "seq", seq);
	char strid[32] ={0};
	sprintf(strid, "%x", g_lan_plc_process_mgnt->local_dev);
    cJSON_AddStringToObject(pRoot, "id", strid);
    cJSON_AddNumberToObject(pRoot, "place", lockp);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		if(timeout)
		{
			einfo.seq = seq;
			einfo.reqcmd = SYNC_DEV_UNLOCK;
			einfo.rspcmd = SYNC_DEV_UNLOCK_ACK;
			einfo.sem = CreateSemaphore(0);

			event_info_add(g_lan_plc_process_mgnt, &einfo);
		}

        spi_plc_mgnt_send(remotedev, PROXY_PORT, (uint8_t *)pjsonbuf, strlen(pjsonbuf));
		uint32_t ltick = time_relative_ms();
		if(einfo.sem)
		{
			if(GetSemaphore(einfo.sem, timeout))
				ret = 0;
			else
				event_info_del_by_seq(g_lan_plc_process_mgnt, seq);
			DestorySemaphore(einfo.sem);
		}

        if(einfo.packdata)
        {
			printf("#######ret:%u timeout:%u - %u ack data:%s \n", ret, timeout, time_relative_ms()-ltick, einfo.packdata);
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_setsn(uint32_t remotedev, char *psn, unsigned int timeout)
{
	int ret = -1;

	if(!g_lan_plc_process_mgnt)
		return -1;

    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_SET_SN);
	cJSON_AddNumberToObject(pRoot, "seq", seq);
	INSERT_STRING2OBJECT(pRoot, "sn", psn);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		if(timeout)
		{
			einfo.seq = seq;
			einfo.reqcmd = SYNC_DEV_SET_SN;
			einfo.rspcmd = SYNC_DEV_SET_SN_ACK;
			einfo.sem = CreateSemaphore(0);

			event_info_add(g_lan_plc_process_mgnt, &einfo);
		}

        spi_plc_mgnt_send(remotedev, PROXY_PORT, (uint8_t *)pjsonbuf, strlen(pjsonbuf));

		if(einfo.sem)
		{
			if(GetSemaphore(einfo.sem, timeout))
				ret = 0;
			else
				event_info_del_by_seq(g_lan_plc_process_mgnt, seq);
			DestorySemaphore(einfo.sem);
		}

        if(einfo.packdata)
        {
			ret = reply_analy_ack(einfo.packdata);
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_getsn(uint32_t remotedev, char *poutsn, unsigned int timeout)
{
	int ret = -1;

	if(!g_lan_plc_process_mgnt)
		return -1;

    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_GET_SN);
	cJSON_AddNumberToObject(pRoot, "seq", seq);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		if(timeout)
		{
			einfo.seq = seq;
			einfo.reqcmd = SYNC_DEV_GET_SN;
			einfo.rspcmd = SYNC_DEV_GET_SN_ACK;
			einfo.sem = CreateSemaphore(0);

			event_info_add(g_lan_plc_process_mgnt, &einfo);
		}

        spi_plc_mgnt_send(remotedev, PROXY_PORT, (uint8_t *)pjsonbuf, strlen(pjsonbuf));

		if(einfo.sem)
		{
			if(GetSemaphore(einfo.sem, timeout))
				ret = 0;
			else
				event_info_del_by_seq(g_lan_plc_process_mgnt, seq);
			DestorySemaphore(einfo.sem);
		}

        if(einfo.packdata)
        {
			cJSON *pAck = cJSON_Parse((char*)einfo.packdata);
			if (pAck)
			{
				cJSON *psn = cJSON_GetObjectItem(pAck, "sn");
				if (poutsn && psn && cJSON_IsString(psn))
				{
					strncpy(poutsn, psn->valuestring, 64-1);
					ret = 0;
				}
				cJSON_Delete(pAck);
			}
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_setlanguage(uint32_t remotedev, uint32_t languageindex, unsigned int timeout)
{
	int ret = -1;

	if(!g_lan_plc_process_mgnt)
		return -1;

    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_SET_LANGUANGE);
	cJSON_AddNumberToObject(pRoot, "seq", seq);
	cJSON_AddNumberToObject(pRoot, "language", languageindex);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};

		PLC_SEND_MSG_WAIT_ACK(g_lan_plc_process_mgnt, remotedev, seq, SYNC_DEV_SET_LANGUANGE, SYNC_DEV_SET_LANGUANGE_ACK, einfo, pjsonbuf)

        if(einfo.packdata)
        {
			ret = reply_analy_ack(einfo.packdata);
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_set_ai_noise(uint32_t remotedev, bool isonoff, unsigned int timeout)
{
	int ret = -1;

	if(!g_lan_plc_process_mgnt)
		return -1;

    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_SET_NOISE_AI);
	cJSON_AddNumberToObject(pRoot, "seq", seq);
	cJSON_AddBoolToObject(pRoot, "onoff", isonoff);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};

		PLC_SEND_MSG_WAIT_ACK(g_lan_plc_process_mgnt, remotedev, seq, SYNC_DEV_SET_NOISE_AI, SYNC_DEV_SET_NOISE_AI_ACK, einfo, pjsonbuf)

        if(einfo.packdata)
        {
			ret = reply_analy_ack(einfo.packdata);
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

//加卡状态设置 state 1：开始 0：结束
DPLAN_PUBLIC_API int goblin_plc_addcard_state(uint32_t remotedev, int state, unsigned int timeout)
{
	if(!g_lan_plc_process_mgnt)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_SET_ADDCARD_STATE);
	cJSON_AddNumberToObject(pRoot, "seq", seq);
	char strid[32] ={0};
	sprintf(strid, "%x", g_lan_plc_process_mgnt->local_dev);
    cJSON_AddStringToObject(pRoot, "id", strid);
    cJSON_AddNumberToObject(pRoot, "state", state);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		uint32_t ltick = time_relative_ms();

		PLC_SEND_MSG_WAIT_ACK(g_lan_plc_process_mgnt, remotedev, seq, SYNC_DEV_SET_ADDCARD_STATE, SYNC_DEV_SET_ADDCARD_STATE_ACK, einfo, pjsonbuf)

        if(einfo.packdata)
        {
			LOGI("#######ret:%u timeout:%u - %u ack data:%s \n", ret, timeout, time_relative_ms()-ltick, einfo.packdata);
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

//删卡状态设置 1：开始 0：结束
DPLAN_PUBLIC_API int goblin_plc_delcard_state(uint32_t remotedev, int state, unsigned int timeout)
{
	if(!g_lan_plc_process_mgnt)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_SET_DELCARD_STATE);
	cJSON_AddNumberToObject(pRoot, "seq", seq);
	char strid[32] ={0};
	sprintf(strid, "%x", g_lan_plc_process_mgnt->local_dev);
    cJSON_AddStringToObject(pRoot, "id", strid);
    cJSON_AddNumberToObject(pRoot, "state", state);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		uint32_t ltick = time_relative_ms();

		PLC_SEND_MSG_WAIT_ACK(g_lan_plc_process_mgnt, remotedev, seq, SYNC_DEV_SET_DELCARD_STATE, SYNC_DEV_SET_DELCARD_STATE_ACK, einfo, pjsonbuf)

        if(einfo.packdata)
        {
			LOGI("#######ret:%u timeout:%u - %u ack data:%s \n", ret, timeout, time_relative_ms()-ltick, einfo.packdata);
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

//删除所有卡
DPLAN_PUBLIC_API int goblin_plc_delcard_all(uint32_t remotedev,  unsigned int timeout)
{
	if(!g_lan_plc_process_mgnt)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_DEL_ALL_CARD);
	cJSON_AddNumberToObject(pRoot, "seq", seq);
	char strid[32] ={0};
	sprintf(strid, "%x", g_lan_plc_process_mgnt->local_dev);
    cJSON_AddStringToObject(pRoot, "id", strid);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		uint32_t ltick = time_relative_ms();

		PLC_SEND_MSG_WAIT_ACK(g_lan_plc_process_mgnt, remotedev, seq, SYNC_DEV_DEL_ALL_CARD, SYNC_DEV_DEL_ALL_CARD_ACK, einfo, pjsonbuf)

        if(einfo.packdata)
        {
			LOGI("#######ret:%u timeout:%u - %u ack data:%s \n", ret, timeout, time_relative_ms()-ltick, einfo.packdata);
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}


//删除对应卡号
DPLAN_PUBLIC_API int goblin_plc_delcard_by_id(uint32_t remotedev, char *cardid,  unsigned int timeout)
{
	if(!g_lan_plc_process_mgnt || !cardid)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_DEL_CARD_BY_ID);
	cJSON_AddNumberToObject(pRoot, "seq", seq);
	char strid[32] ={0};
	sprintf(strid, "%x", g_lan_plc_process_mgnt->local_dev);
    cJSON_AddStringToObject(pRoot, "id", strid);

	cJSON *pnumarray = cJSON_AddArrayToObject(pRoot, "num");
	cJSON_AddItemToArray(pnumarray, cJSON_CreateString(cardid));

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};

		uint32_t ltick = time_relative_ms();

		PLC_SEND_MSG_WAIT_ACK(g_lan_plc_process_mgnt, remotedev, seq, SYNC_DEV_DEL_CARD_BY_ID, SYNC_DEV_DEL_CARD_BY_ID_ACK, einfo, pjsonbuf)

        if(einfo.packdata)
        {
			LOGI("#######ret:%u timeout:%u - %u ack data:%s \n", ret, timeout, time_relative_ms()-ltick, einfo.packdata);
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_get_door_card(uint32_t remotedev, goblin_plc_cardinfo_s **pcardinfo, unsigned int *pcardnum, unsigned int timeout)
{
	if(!g_lan_plc_process_mgnt || !pcardinfo || !pcardnum)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_PLC_GET_ALL_CARD);
	cJSON_AddNumberToObject(pRoot, "seq", seq);
	char strid[32] ={0};
	sprintf(strid, "%x", g_lan_plc_process_mgnt->local_dev);
    cJSON_AddStringToObject(pRoot, "id", strid);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};

		PLC_SEND_MSG_WAIT_ACK(g_lan_plc_process_mgnt, remotedev, seq, SYNC_DEV_PLC_GET_ALL_CARD, SYNC_DEV_PLC_GET_ALL_CARD_ACK, einfo, pjsonbuf)

        if(einfo.packdata)
        {
			cJSON *pAck = cJSON_Parse((char*)einfo.packdata);
			if (pAck)
			{
				cJSON *presult = cJSON_GetObjectItem(pAck, "result");
				if(presult && cJSON_IsNumber(presult))
					ret = presult->valueint==1 ? 0 : 1;

				//解析卡列表
				cJSON *pcardsarray = cJSON_GetObjectItem(pAck, "cards");
				if (pcardsarray && cJSON_IsArray(pcardsarray) && cJSON_GetArraySize(pcardsarray) > 0)
				{
					int arraysize = cJSON_GetArraySize(pcardsarray);
					goblin_plc_cardinfo_s *pinfo = (goblin_plc_cardinfo_s *)malloc(arraysize*sizeof(goblin_plc_cardinfo_s));
					memset(pinfo, 0, sizeof(goblin_plc_cardinfo_s)*arraysize);
					for (int i = 0; i < arraysize; i++)
					{
						cJSON *pitem = cJSON_GetArrayItem(pcardsarray, i);
						if(!pitem)
							continue;
						// cJSON *pserial = cJSON_GetObjectItem(pitem, "serial");
						cJSON *pnum = cJSON_GetObjectItem(pitem, "num");
						cJSON *proom = cJSON_GetObjectItem(pitem, "room");
						cJSON *pplace = cJSON_GetObjectItem(pitem, "place");

						STRINFOINSERT_BYJSON(pnum, pinfo[i].strcard, sizeof(pinfo[i].strcard)-1)
						STRINFOINSERT_BYJSON(proom, pinfo[i].strroomid, sizeof(pinfo[i].strroomid)-1)
						if(pplace && cJSON_IsNumber(pplace))
							pinfo[i].placestate = pplace->valueint;
					}

					*pcardinfo = pinfo;
					*pcardnum = arraysize;
				}

				cJSON_Delete(pAck);
			}

            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

//修改对应卡号信息
DPLAN_PUBLIC_API int goblin_plc_cardinfo_modify(uint32_t remotedev, goblin_plc_cardinfo_s *pcardinfo, unsigned int timeout)
{
	if(!g_lan_plc_process_mgnt || !pcardinfo)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_PLC_MODIFY_CARD_BY_ID);
	cJSON_AddNumberToObject(pRoot, "seq", seq);
	char strid[32] ={0};
	sprintf(strid, "%x", g_lan_plc_process_mgnt->local_dev);
    cJSON_AddStringToObject(pRoot, "id", strid);

	INSERT_STRING2OBJECT(pRoot, "num", pcardinfo->strcard);
	INSERT_STRING2OBJECT(pRoot, "room", pcardinfo->strroomid);
	INSERT_NUMBER2OBJECT(pRoot, "placestate", pcardinfo->placestate);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		uint32_t ltick = time_relative_ms();

		PLC_SEND_MSG_WAIT_ACK(g_lan_plc_process_mgnt, remotedev, seq, SYNC_DEV_PLC_MODIFY_CARD_BY_ID, SYNC_DEV_PLC_MODIFY_CARD_BY_ID_ACK, einfo, pjsonbuf)

        if(einfo.packdata)
        {
			LOGI("#######ret:%u timeout:%u - %u ack data:%s \n", ret, timeout, time_relative_ms()-ltick, einfo.packdata);
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_report_card_opt(uint32_t remotedev, goblin_plc_card_optinfo_s *pcardopt, unsigned int timeout)
{
	if(!g_lan_plc_process_mgnt || !pcardopt)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_PLC_REPORT_CARD_BY_OPT);
	cJSON_AddNumberToObject(pRoot, "seq", seq);
    cJSON_AddStringToObject(pRoot, "opt", pcardopt->stropt);
    cJSON_AddStringToObject(pRoot, "cardid", pcardopt->strcard);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		uint32_t ltick = time_relative_ms();

		PLC_SEND_MSG_WAIT_ACK(g_lan_plc_process_mgnt, remotedev, seq, SYNC_DEV_PLC_REPORT_CARD_BY_OPT, SYNC_DEV_PLC_REPORT_CARD_BY_OPT_ACK, einfo, pjsonbuf)

        if(einfo.packdata)
        {
			LOGI("#######ret:%u timeout:%u - %u ack data:%s \n", ret, timeout, time_relative_ms()-ltick, einfo.packdata);
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}


DPLAN_PUBLIC_API int goblin_plc_hardware_test(uint32_t remotedev, golbin_hardware_test_s *operateinfo, unsigned int timeout)
{
	if(!g_lan_plc_process_mgnt || !operateinfo)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_PLC_TEST);
	cJSON_AddNumberToObject(pRoot, "seq", seq);
	char strid[32] ={0};
	sprintf(strid, "%x", g_lan_plc_process_mgnt->local_dev);
    cJSON_AddStringToObject(pRoot, "id", strid);
    cJSON_AddNumberToObject(pRoot, "operate", operateinfo->operate_cmd);
	INSERT_NUMBER2OBJECT(pRoot, "index", operateinfo->index);
	INSERT_STRING2OBJECT(pRoot, "state", operateinfo->strstate);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		uint32_t ltick = time_relative_ms();

		PLC_SEND_MSG_WAIT_ACK(g_lan_plc_process_mgnt, remotedev, seq, SYNC_DEV_PLC_TEST, SYNC_DEV_PLC_TEST_ACK, einfo, pjsonbuf)

        if(einfo.packdata)
        {
			printf("#######ret:%u timeout:%u - %u ack data:%s \n", ret, timeout, time_relative_ms()-ltick, einfo.packdata);
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

//======================================= 和PLC模块交互的 ========================================

DPLAN_PUBLIC_API int goblin_plc_close(void)
{
	return spi_plc_mgnt_off();
}

DPLAN_PUBLIC_API int goblin_plc_open_dev(uint32_t devid)
{
	return spi_plc_mgnt_open_dev(devid);
}

DPLAN_PUBLIC_API int goblin_plc_get_rssi(void)
{
	return spi_plc_mgnt_get_rssi();
}

DPLAN_PUBLIC_API int goblin_plc_upgrade_firmware(char *pfilepath)
{
	return spi_plc_mgnt_upgrade(pfilepath);
}

DPLAN_PUBLIC_API int goblin_plc_upgrade_firmware_by_fd(FILE *pfd, uint32_t file_size)
{
	return spi_plc_mgnt_upgrade_by_fd(pfd, file_size);
}

DPLAN_PUBLIC_API int goblin_plc_reboot(void)
{
	return spi_plc_mgnt_reboot();
}

DPLAN_PUBLIC_API int Goblin_plc_Online(void)
{
	if(!g_lan_plc_process_mgnt)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_ONLINE);
	cJSON_AddNumberToObject(pRoot, "seq", seq);

	char strid[32] ={0};
	sprintf(strid, "%x", g_lan_plc_process_mgnt->local_dev);
    cJSON_AddStringToObject(pRoot, "id", strid);
    cJSON_AddStringToObject(pRoot, "flag", "DP");
    cJSON_AddNumberToObject(pRoot, "type", g_lan_plc_process_mgnt->local_dev>>24);
	cJSON_AddStringToObject(pRoot, "hardware", g_lan_plc_process_mgnt->local_hard_ver);
	cJSON_AddStringToObject(pRoot, "software", g_lan_plc_process_mgnt->local_soft_ver);

	cJSON_AddNumberToObject(pRoot, "plcversion", g_lan_plc_process_mgnt->plc_version);
	cJSON_AddBoolToObject(pRoot, "enableagent", g_lan_plc_process_mgnt->is_agent);
    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
        ret = spi_plc_mgnt_send(0xFFFFFFFF, PROXY_PORT, (uint8_t *)pjsonbuf, strlen(pjsonbuf));

        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_wakeup_dev_by_roomid(uint32_t remotedev, uint32_t roomid, uint32_t timeout)
{
	if(!g_lan_plc_process_mgnt)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_WAKEUP);
	cJSON_AddNumberToObject(pRoot, "seq", seq);

	char strid[32] ={0};
	sprintf(strid, "%x", g_lan_plc_process_mgnt->local_dev);
    cJSON_AddStringToObject(pRoot, "id", strid);
    cJSON_AddStringToObject(pRoot, "flag", "DP");
    cJSON_AddNumberToObject(pRoot, "type", g_lan_plc_process_mgnt->local_dev>>24);
	cJSON_AddNumberToObject(pRoot, "roomid", roomid);
    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		if(timeout)
		{
			einfo.seq = seq;
			einfo.reqcmd = SYNC_DEV_WAKEUP;
			einfo.rspcmd = SYNC_DEV_WAKEUP_ACK;
			einfo.sem = CreateSemaphore(0);

			event_info_add(g_lan_plc_process_mgnt, &einfo);
		}
        ret = spi_plc_mgnt_send(remotedev, PROXY_PORT, (uint8_t *)pjsonbuf, strlen(pjsonbuf));

		if(einfo.sem)
		{
			if(GetSemaphore(einfo.sem, timeout))
				ret = 0;
			else{
				event_info_del_by_seq(g_lan_plc_process_mgnt, seq);
				ret = -2;
			}
			DestorySemaphore(einfo.sem);
		}

        if(einfo.packdata)
        {
			ret = reply_analy_ack(einfo.packdata);
            bfree(einfo.packdata);
        }

        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_Search(uint32_t remotedev, uint32_t searchtype, uint32_t roomid)
{
	if(!g_lan_plc_process_mgnt)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_SEARCH);
	cJSON_AddNumberToObject(pRoot, "seq", seq);

	char strid[32] ={0};
	sprintf(strid, "%x", g_lan_plc_process_mgnt->local_dev);
    cJSON_AddStringToObject(pRoot, "id", strid);
    cJSON_AddStringToObject(pRoot, "flag", "DP");
    cJSON_AddNumberToObject(pRoot, "type", g_lan_plc_process_mgnt->local_dev>>24);
	cJSON_AddNumberToObject(pRoot, "searchtype", searchtype);
	cJSON_AddNumberToObject(pRoot, "roomid", roomid);
    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
        ret = spi_plc_mgnt_send(remotedev, PROXY_PORT, (uint8_t *)pjsonbuf, strlen(pjsonbuf));

        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

int reply_analy_ack(uint8_t *pdata)
{
	int ret = 1;
	cJSON *proot = cJSON_Parse((char*)pdata);
	if (proot)
	{
		/* code */
		cJSON * presult = cJSON_GetObjectItem(proot, "result");
		if(presult && cJSON_IsNumber(presult))
		{
			ret = presult->valueint== 1 ? 0 : 2;
		}
		cJSON_Delete(proot);
	}

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_set_remote_id(uint32_t remotedev, char *strdevid, unsigned int timeout)
{
	if(!g_lan_plc_process_mgnt)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_SET_ID);
	cJSON_AddNumberToObject(pRoot, "seq", seq);

	char strid[32] ={0};
	sprintf(strid, "%x", g_lan_plc_process_mgnt->local_dev);
    cJSON_AddStringToObject(pRoot, "id", strid);
    cJSON_AddStringToObject(pRoot, "villaid", strdevid);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		if(timeout)
		{
			einfo.seq = seq;
			einfo.reqcmd = SYNC_DEV_SET_ID;
			einfo.rspcmd = SYNC_DEV_SET_ID_ACK;
			einfo.sem = CreateSemaphore(0);

			event_info_add(g_lan_plc_process_mgnt, &einfo);
		}

        ret = spi_plc_mgnt_send(remotedev, PROXY_PORT, (uint8_t*)pjsonbuf, strlen(pjsonbuf));

		if(einfo.sem)
		{
			if(GetSemaphore(einfo.sem, timeout))
				ret = 0;
			else
				event_info_del_by_seq(g_lan_plc_process_mgnt, seq);
			DestorySemaphore(einfo.sem);
		}

        if(einfo.packdata)
        {
			ret = reply_analy_ack(einfo.packdata);
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}


DPLAN_PUBLIC_API int goblin_plc_get_door_cfg(uint32_t remotedev, goblin_door_cfg_t *doorcfg, unsigned int timeout)
{
	if(!g_lan_plc_process_mgnt || !doorcfg)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_GET_CFG);
	cJSON_AddNumberToObject(pRoot, "seq", seq);
	char strid[32] ={0};
	sprintf(strid, "%x", g_lan_plc_process_mgnt->local_dev);
    cJSON_AddStringToObject(pRoot, "id", strid);

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		if(timeout)
		{
			einfo.seq = seq;
			einfo.reqcmd = SYNC_DEV_GET_CFG;
			einfo.rspcmd = SYNC_DEV_GET_CFG_ACK;
			einfo.sem = CreateSemaphore(0);

			event_info_add(g_lan_plc_process_mgnt, &einfo);
		}

        spi_plc_mgnt_send(remotedev, PROXY_PORT, (uint8_t*)pjsonbuf, strlen(pjsonbuf));

		if(einfo.sem)
		{
			if(GetSemaphore(einfo.sem, timeout))
				ret = 0;
			else
				event_info_del_by_seq(g_lan_plc_process_mgnt, seq);
			DestorySemaphore(einfo.sem);
		}

        if(einfo.packdata)
        {
			cJSON *pAck = cJSON_Parse((char*)einfo.packdata);
			if (pAck)
			{
				cJSON *presult = cJSON_GetObjectItem(pAck, "result");
				if(presult && cJSON_IsNumber(presult))
					ret = presult->valueint==1 ? 0 : 1;
				cJSON *pbase = cJSON_GetObjectItem(pAck, "base");
				cJSON *pentrance = cJSON_GetObjectItem(pAck, "entrance");
				cJSON *pai = cJSON_GetObjectItem(pAck, "ai");

				if(pbase && cJSON_IsObject(pbase))
				{
					cJSON *psoundcall = cJSON_GetObjectItem(pbase, "soundcall");
					cJSON *psoundtalk = cJSON_GetObjectItem(pbase, "soundtalk");

					doorcfg->soundcall = psoundcall && cJSON_IsNumber(psoundcall) ? psoundcall->valueint : -1;
					doorcfg->soundtalk = psoundtalk && cJSON_IsNumber(psoundtalk) ? psoundtalk->valueint : -1;
				}
				if(pentrance && cJSON_IsObject(pentrance))
				{
					cJSON *pdoorlockarray = cJSON_GetObjectItem(pentrance, "doorlock");
					if (pdoorlockarray && cJSON_IsArray(pdoorlockarray))
					{
						int arraysize = cJSON_GetArraySize(pdoorlockarray);
						for (int i = 0; i < arraysize && i < DOOR_LOCK_COUNT; i++)
						{
							cJSON *pitem = cJSON_GetArrayItem(pdoorlockarray, i);
							if(!pitem)
								continue;
							cJSON *pdoorid = cJSON_GetObjectItem(pitem, "doorid");
							cJSON *plockswitch = cJSON_GetObjectItem(pitem, "lockswitch");
							cJSON *plocktime = cJSON_GetObjectItem(pitem, "locktime");
							cJSON *pmenicswitch = cJSON_GetObjectItem(pitem, "menicswitch");
							cJSON *pmenictime = cJSON_GetObjectItem(pitem, "menictime");
							cJSON *plocktype = cJSON_GetObjectItem(pitem, "locktype");

							doorcfg->doorlock[i].lockid = pdoorid && cJSON_IsNumber(pdoorid) ? pdoorid->valueint : -1;
							doorcfg->doorlock[i].lockswitch = plockswitch && cJSON_IsNumber(plockswitch) ? plockswitch->valueint : -1;
							doorcfg->doorlock[i].locktime = plocktime && cJSON_IsNumber(plocktime) ? plocktime->valueint : -1;
							doorcfg->doorlock[i].menicswitch = pmenicswitch && cJSON_IsNumber(pmenicswitch) ? pmenicswitch->valueint : -1;
							doorcfg->doorlock[i].menictime = pmenictime && cJSON_IsNumber(pmenictime) ? pmenictime->valueint : -1;
							doorcfg->doorlock[i].locktype = plocktype && cJSON_IsNumber(plocktype) ? plocktype->valueint : -1;
						}
					}
				}
				if(pai && cJSON_IsObject(pai))
				{
					cJSON *pmotion = cJSON_GetObjectItem(pai, "motion");

					doorcfg->motion = pmotion && cJSON_IsNumber(pmotion) ? pmotion->valueint : -1;
				}

				cJSON_Delete(pAck);
			}

            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_set_door_cfg(uint32_t remotedev, goblin_door_cfg_t *doorcfg, unsigned int timeout)
{
	if(!g_lan_plc_process_mgnt)
		return -1;
	int ret = -1;
    cJSON *pRoot = cJSON_CreateObject();

	int seq = g_lan_plc_process_mgnt->send_seq++;
    cJSON_AddNumberToObject(pRoot, "cmd", SYNC_DEV_SET_CFG);
	cJSON_AddNumberToObject(pRoot, "seq", seq);

	char strid[32] ={0};
	sprintf(strid, "%x", g_lan_plc_process_mgnt->local_dev);
    cJSON_AddStringToObject(pRoot, "id", strid);

	cJSON *pbase = cJSON_AddObjectToObject(pRoot, "base");
	cJSON *pentrance = cJSON_AddObjectToObject(pRoot, "entrance");
	cJSON *pai = cJSON_AddObjectToObject(pRoot, "ai");
	if(pbase)
	{
		cJSON_AddNumberToObject(pbase, "soundcall", doorcfg->soundcall);
		cJSON_AddNumberToObject(pbase, "soundtalk", doorcfg->soundtalk);
	}
	if(pentrance)
	{
		cJSON *pdoorlockarray = cJSON_AddArrayToObject(pentrance, "doorlock");
		for (int i = 0; i < DOOR_LOCK_COUNT; i++)
		{
			cJSON *pdoorlock = cJSON_CreateObject();
			if(!pdoorlock)
				continue;

			cJSON_AddNumberToObject(pdoorlock, "doorid", doorcfg->doorlock[i].lockid);
			cJSON_AddNumberToObject(pdoorlock, "lockswitch", doorcfg->doorlock[i].lockswitch);
			cJSON_AddNumberToObject(pdoorlock, "locktime", doorcfg->doorlock[i].locktime);
			cJSON_AddNumberToObject(pdoorlock, "menicswitch", doorcfg->doorlock[i].menicswitch);
			cJSON_AddNumberToObject(pdoorlock, "menictime", doorcfg->doorlock[i].menictime);
			cJSON_AddNumberToObject(pdoorlock, "locktype", doorcfg->doorlock[i].locktype);

			cJSON_AddItemToArray(pdoorlockarray, pdoorlock);
		}
	}
	if(pai)
	{
		cJSON_AddNumberToObject(pai, "motion", doorcfg->motion);
	}

    char *pjsonbuf = cJSON_PrintUnformatted(pRoot);
    if (pjsonbuf)
    {
		spi_event_info_s einfo ={0};
		if(timeout)
		{
			einfo.seq = seq;
			einfo.reqcmd = SYNC_DEV_SET_CFG;
			einfo.rspcmd = SYNC_DEV_SET_CFG_ACK;
			einfo.sem = CreateSemaphore(0);

			event_info_add(g_lan_plc_process_mgnt, &einfo);
		}

        ret = spi_plc_mgnt_send(remotedev, PROXY_PORT, (uint8_t*)pjsonbuf, strlen(pjsonbuf));

		if(einfo.sem)
		{
			if(GetSemaphore(einfo.sem, timeout))
				ret = 0;
			else
				event_info_del_by_seq(g_lan_plc_process_mgnt, seq);
			DestorySemaphore(einfo.sem);
		}

        if(einfo.packdata)
        {
			ret = reply_analy_ack(einfo.packdata);
            bfree(einfo.packdata);
        }
        cJSON_free(pjsonbuf);
    }
    cJSON_Delete(pRoot);

	return ret;
}

DPLAN_PUBLIC_API int goblin_plc_set_door_card_operation(uint32_t remotedev, char *popt, unsigned int timeout)
{
	int ret = -1;
	if(!g_lan_plc_process_mgnt)
		return -1;

	return ret;
}

//---------------------文件发送-------------------------


DPLAN_PUBLIC_API int goblin_plc_send_file_data(uint32_t remotedev, file_pack_t *pfiledata, unsigned int timeout)
{
	if(!g_lan_plc_process_mgnt)
		return -1;
	int ret = -1;

	if(timeout)
	{
		pthread_mutex_lock(&g_lan_plc_process_mgnt->event_mutex);
		g_lan_plc_process_mgnt->m_filesem = CreateSemaphore(0);
		pthread_mutex_unlock(&g_lan_plc_process_mgnt->event_mutex);
	}

	spi_plc_mgnt_send(remotedev, FILE_PORT, (uint8_t*)pfiledata, sizeof(file_pack_t));

	if(g_lan_plc_process_mgnt->m_filesem)
	{
		if(GetSemaphore(g_lan_plc_process_mgnt->m_filesem, timeout))
			ret = 0;

		pthread_mutex_lock(&g_lan_plc_process_mgnt->event_mutex);
		DestorySemaphore(g_lan_plc_process_mgnt->m_filesem);
		g_lan_plc_process_mgnt->m_filesem = NULL;
		pthread_mutex_unlock(&g_lan_plc_process_mgnt->event_mutex);
	}


	return ret;
}

DPLAN_PUBLIC_API void goblin_plc_log_hook(GoblinLogOutput log_output)
{
	void goblin_log_set_hook(GoblinLogOutput log_output);
	goblin_log_set_hook(log_output);
}