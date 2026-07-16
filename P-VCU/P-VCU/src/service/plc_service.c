#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <utils/bmem.h>

#include <utils/util.h>
#include <utils/list.h>
#include <utils/mq.h>
#include <utils/time_helper.h>
#include <utils/log.h>
#include <dpbase.h>
#include "dpnetwork.h"
#include "media/video_encode.h"
#include <goblin_plc_process.h>
#include <goblin_plc_callcore.h>
#if defined(ENABLE_V1) || defined(ENABLE_V6)
#include <sound_play.h>
#include <gpio_service.h>
#include <sys/sys_event.h>
#include <hard_node_service.h>
#include <hardware_test.h>
#include <video_service.h>
#include <myconfig.h>
#include <md5-helper.h>
#include <card_service.h>
#include "data/DBICCard.h"
#include "data/DBLanguage.h"
#endif

#include "plc_service.h"

#define MAX_SESSION_EVENT          (32)

#define FILE_RECV_TIMEOUT_MS       (10000)

// #ifndef NDEBUG
#define DEBUG_HANGUP_BY_KEY 0 //是否测试按键挂断
// #endif //NDEBUG

#if DEBUG_HANGUP_BY_KEY
	#define SOFT_WARE_VERSION 				"1.0.0_"__DATE__"-"__TIME__
#else
	#ifdef ENABLE_SPINAAD
		#define SOFT_WARE_VERSION 				"3.2.3"//"V1.1.6_"__DATE__"-"__TIME__	//"V1.0.0_autocall_6"
	#else
		#define SOFT_WARE_VERSION 				"2.3.0"
	#endif //#ifdef ENABLE_SPINAAD
#endif

#ifdef ENABLE_V6
#define HARD_WARE_VERSION 				"2.0.0"
#else
#define HARD_WARE_VERSION 				"1.0.1"
#endif //#ifdef ENABLE_V6

#ifdef ENABLE_V6
	void RequestIFrame(void);
	void RequestCropFrame(unsigned int iscrop);
#endif

enum
{
	CALL_EVENT,
	BUTTON_EVENT,
	DEV_INFO_EVENT,
} __attribute__((packed));


enum
{
	SESSION_IDLE,
	SESSION_CALLING,
	SESSION_RINGING,
	SESSION_TALKING,
	SESSION_END,
} __attribute__((packed));

typedef uint8_t call_event_type;
typedef uint8_t call_status_t;

typedef struct button_msg
{
	uint8_t state;
	uint8_t code;
} button_msg_t;

typedef struct plc_dev_msg
{
	uint32_t devid;
	int type;
	bool isagent;
} plc_dev_info_t;

typedef struct service_event
{
	call_event_type type;

	union
	{
		button_msg_t button;
		plc_call_info_s callinfo;
		plc_dev_info_t devinfo;
	};
} service_event_t;

typedef struct _call_session_
{
    struct list_head node;
    void *psession;                         //当前通话句柄
	uint32_t devid;                        	// 对端的PLC号
	uint8_t bcallout;                       // 是否主叫
    uint32_t stick;
	call_status_t status;                   // 该线路的呼叫阶段
} call_session_t;

typedef struct _plc_service_mgnt
{
	// 本机设备号
	uint32_t dev_id;
	// PLC版本号
	uint32_t plc_version;

	//会话链表锁
	pthread_mutex_t session_mutex;
	//会话链表
	struct list_head session_list;
	//会话个数
    uint32_t session_count;

	//铃声播放句柄
	int ring_play_id;

	//事件队列
	mq_t *event_queue;
	//线程句柄
	pthread_t thread;
	//线程退出标志
    uint8_t isrun;

	uint32_t key_last_tick;
	uint8_t key_last_code;
	uint8_t bkey_call_dev;	//按键是否获取呼叫设备

	uint8_t search_count;		//搜索设备的次数
	uint32_t last_search_tick;	//上次搜索设备的时间戳

	uint8_t is_call_auto_test;		//是否测试呼叫
	uint32_t call_last_tick;		//最后一次呼叫

	uint8_t filetype;				//文件类型 1:本机升级固件
	uint32_t filelen;				//文件长度
	uint32_t filepackseq; 			//文件序号
	FILE *pfp;    					//文件操作的句柄
	pthread_mutex_t file_mutex;		//文件操作的互斥锁
	uint32_t file_last_recv_tick;	//文件接收的最后时间戳
	uint32_t file_fisrt_recv_tick;	//文件接收的第一次时间戳

	bool breportagent;				//是否上报设备代理状态
	uint32_t agent_devid;			//代理设备号
	uint32_t last_agent_tick;		//上次代理的时间戳

	void *ptalk_session;            //当前正在通话句柄
} plc_service_mgnt_t;

static plc_service_mgnt_t *plc_service_mgnt = NULL;
//默认是4键的机型
static int g_plc_dev_type = PLC_DOOR_P_VCU_D4;

static char g_szDevType[32] = "P-VCU-C0";

int get_plc_dev_type(void)
{
	return g_plc_dev_type;
}

static void stop_play_ring(plc_service_mgnt_t *mgnt)
{
	if (mgnt && mgnt->ring_play_id != 0)
	{
		sound_play_stop(mgnt->ring_play_id);
		//sound_play_ring_stop(mgnt->ring_play_id);
		mgnt->ring_play_id = 0;
	}
}


static void start_play_ring(plc_service_mgnt_t *mgnt)
{
	int volume = GetSoundcallVol();
	//stop_play_ring(mgnt);

	if (mgnt && mgnt->ring_play_id == 0)
	{
#if DEBUG_HANGUP_BY_KEY
		volume = 5;
#endif
	 	mgnt->ring_play_id = sound_play_start(RING_MP3, 2, volume, false);;//sound_play_ring_start(volume);//sound_play_start(RING_MP3, PLAY_FOREVER, volume, false);
	}
}


static void plc_wakeup_dev_by_room(uint8_t roomid)
{
	#define MAX_ROOM_DEV_NUM 4
	uint32_t dstdevid = 0x1000101;
	for (size_t i = 0; i < MAX_ROOM_DEV_NUM; i++)
	{
		dstdevid = (0x1000001 | (roomid << 8)) + i;
		goblin_plc_open_dev(dstdevid);
	}
}

static int check_device_type(plc_service_mgnt_t *mgnt)
{
	int dev_type = PLC_DOOR_P_VCU_D4;

	int fd = -1;
	char buf[8] = "0";

	fd = open("/sys/bus/iio/devices/iio:device0/in_voltage2_raw", O_RDONLY | O_CLOEXEC);

	if(fd < 0)
	{
		LOGE("open in_voltage0_raw error \n");
		return dev_type;
	}

	if (read(fd, buf, 8) < 0)
	{
		LOGE("read in_voltage0_raw error \n");
		close(fd);
		return dev_type;
	}

	printf("in_voltage2_raw:%s\n", buf);

	close(fd);

	//根据ADC值来判断按键个数的机器类型
	int value = atoi(buf);
	memset(g_szDevType, 0, sizeof(g_szDevType));
	if (value > 300 && value < 450)
	{
		LOGI("########## P-VCU-D1 Type ##########\n");
		g_plc_dev_type = PLC_DOOR_P_VCU_D1;
		strncpy(g_szDevType, "P-VCU-D1", 32);
	}
	else if (value > 650 && value < 750)
	{
		LOGI("########## P-VCU-D2 Type ##########\n");
		g_plc_dev_type = PLC_DOOR_P_VCU_D2;
		strncpy(g_szDevType, "P-VCU-D2", 32);
	}
	else
	{
		LOGI("########## P-VCU-D4 Type ##########\n");
		g_plc_dev_type = PLC_DOOR_P_VCU_D4;
		strncpy(g_szDevType, "P-VCU-D4", 32);
	}

	return dev_type;
}

static int PlcEventHandleCB(void *param, PlcEventHandleInfo_S *paraminfo)
{
	plc_service_mgnt_t *pmgnt = (plc_service_mgnt_t *)param;
	int ret = 0;
	char *ptype[Goblin_PLC_MSG_EVENTTYPE_MAX] = {"开锁", "设备上线", "设备搜索请求", "搜索ACK", "设置ID", "获取配置", "设置配置", "请求唤醒",
					"PLC版本号", "PLC的信号值", "设备代理", "解除代理", "操作状态", "获取日志大小", "获取日志文件", "PLC地址冲突",
					"设置SN值", "获取SN值", "加卡", "删除卡", "删除所有卡", "删除一张卡", "获取卡号列表", "设置卡号信息", "卡操作上报", "硬件测试"};
	LOGI("########### cb type %d:%s devid:%x ###########\n", paraminfo->type, paraminfo->type < Goblin_PLC_MSG_EVENTTYPE_MAX? (ptype[paraminfo->type-1] ? ptype[paraminfo->type-1] : "other") : "Unknown", paraminfo->remotedev);
	switch (paraminfo->type)
	{
	case Goblin_PLC_MSG_EVENTTYPE_PLC_VERSION:
		{
			printf("PLC版本号:%x\n", paraminfo->eventinfo.plc_version.version);
			pmgnt->plc_version = paraminfo->eventinfo.plc_version.version;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_PCL_RSSI:
		{
			//printf("PLC信号设备数量:%d\n", paraminfo->eventinfo.plc_rssilist.plc_count);
			service_event_t event = {0};
			for (size_t i = 0; i < paraminfo->eventinfo.plc_rssilist.plc_count; i++)
			{
				LOGI("PLC RSSI, PLC信号设备ID:%x 接收的包:%d  丢弃的包:%d  RSSI:%d\n", paraminfo->eventinfo.plc_rssilist.prssi_list[i].id,
					paraminfo->eventinfo.plc_rssilist.prssi_list[i].c_recv,
					paraminfo->eventinfo.plc_rssilist.prssi_list[i].c_drop,
					paraminfo->eventinfo.plc_rssilist.prssi_list[i].rssi);

				memset(&event, 0, sizeof(service_event_t));
				event.type = DEV_INFO_EVENT;
				event.devinfo.devid = paraminfo->eventinfo.plc_rssilist.prssi_list[i].id;
				event.devinfo.type = event.devinfo.devid>>24;
				event.devinfo.isagent = 0;

				//如果是室内机，通知呼叫管理模块
				if(event.devinfo.type==1)
					mq_send(plc_service_mgnt->event_queue, &event, sizeof(service_event_t));
			}
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_UNLOCK:
		{
			printf("开锁 设备：%x  位置：%x\n", paraminfo->eventinfo.unlock.devid, paraminfo->eventinfo.unlock.place);

			gpio_control_Ex(paraminfo->eventinfo.unlock.place == 1 ? GPIO_LOCK_0 : GPIO_LOCK_1, GPIO_MODE_ON, (paraminfo->remotedev>>8) & 0xffff);
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_ONLINE:
		{
			printf("设备上线：%x 软件版本号：%s 硬件版本号:%s\n", paraminfo->eventinfo.Onlineinfo.devid, paraminfo->eventinfo.Onlineinfo.software_version, paraminfo->eventinfo.Onlineinfo.hardware_version);
			if(pmgnt)
			{
				service_event_t event = {0};

				event.type = DEV_INFO_EVENT;
				event.devinfo.devid = paraminfo->eventinfo.Onlineinfo.devid;
				event.devinfo.type = event.devinfo.devid>>24;
				event.devinfo.isagent = paraminfo->eventinfo.Onlineinfo.benableagent;

				//如果是室内机，通知呼叫管理模块
				if(event.devinfo.type==1)
					mq_send(plc_service_mgnt->event_queue, &event, sizeof(service_event_t));
			}
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_SEARCH_REQ:
		{
			printf("有设备:%x 在搜索类型：%d  房号:%d\n", paraminfo->eventinfo.search_req.devid, paraminfo->eventinfo.search_req.type, paraminfo->eventinfo.search_req.roomid);

			if(paraminfo->eventinfo.search_req.type == 3)
			{
				paraminfo->eventinfo.search_req.ackdev = pmgnt->dev_id;
				paraminfo->eventinfo.search_req.ackstate = 0;
				paraminfo->eventinfo.search_req.acktype = 3;
				strncpy(paraminfo->eventinfo.search_req.ackhardware_version, HARD_WARE_VERSION, 32);
				strncpy(paraminfo->eventinfo.search_req.acksoftware_version, SOFT_WARE_VERSION, 32);
				paraminfo->eventinfo.search_req.ackplc_version = pmgnt->plc_version;
				strncpy(paraminfo->eventinfo.search_req.ackSN, GetLocalSn(), sizeof(paraminfo->eventinfo.search_req.ackSN)-1);
				strncpy(paraminfo->eventinfo.search_req.ackDevName, g_szDevType, sizeof(paraminfo->eventinfo.search_req.ackDevName)-1);
				ret =1;
			}
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_SEARCH_RSP:
		{
			printf("搜索到的设备:%x type:%d  languageindex:%d\n",
				paraminfo->eventinfo.search_rsp.devid, paraminfo->eventinfo.search_rsp.type, paraminfo->eventinfo.search_rsp.languageindex);
			if(pmgnt && paraminfo->eventinfo.search_rsp.state==0)
			{
				service_event_t event = {0};
				event.type = DEV_INFO_EVENT;
				event.devinfo.devid = paraminfo->eventinfo.search_rsp.devid;
				event.devinfo.type = event.devinfo.devid>>24;
				event.devinfo.isagent = paraminfo->eventinfo.search_rsp.benableagent;

				//如果是室内机，通知呼叫管理模块
				if(event.devinfo.type==1){
					mq_send(plc_service_mgnt->event_queue, &event, sizeof(service_event_t));

					//语言不是默认的就设置
					if(paraminfo->eventinfo.search_rsp.languageindex != -1 && paraminfo->eventinfo.search_rsp.languageindex != 0 &&
						((paraminfo->remotedev&0xff) == 1)) //一号为主
					{
						LanguageList_T info = {0};
						memset(&info, 0, sizeof(LanguageList_T));
						info.roomid = (paraminfo->remotedev>>8) & 0xffff;
						info.languageid = paraminfo->eventinfo.search_rsp.languageindex;
						AddLanguage(&info);
					}
				}
			}
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_SETID:
		{
			if((paraminfo->eventinfo.set_id.devid>>24)==3)
			{
				SetTermId(paraminfo->eventinfo.set_id.strid);
				goblin_plc_reload_devID(paraminfo->eventinfo.set_id.devid);
				goblin_call_reload_devID(paraminfo->eventinfo.set_id.devid);
			}
			else
				printf("类型不对：%x\n", paraminfo->eventinfo.set_id.devid>>24);
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_GETCFG:
		{
			door_cfg_t doorcfg={0};
			GetDoorCfg(&doorcfg);

			paraminfo->eventinfo.door_cfg.paraminfo.motion = doorcfg.motionlevel;
			paraminfo->eventinfo.door_cfg.paraminfo.soundcall = doorcfg.soundcall;
			paraminfo->eventinfo.door_cfg.paraminfo.soundtalk = doorcfg.soundtalk;

			paraminfo->eventinfo.door_cfg.paraminfo.doorlock[0].lockid = 1;
			paraminfo->eventinfo.door_cfg.paraminfo.doorlock[0].lockswitch = doorcfg.lockswitch;
			paraminfo->eventinfo.door_cfg.paraminfo.doorlock[0].locktime = doorcfg.locktime;
			paraminfo->eventinfo.door_cfg.paraminfo.doorlock[0].menicswitch = doorcfg.menicswitch;
			paraminfo->eventinfo.door_cfg.paraminfo.doorlock[0].menictime = doorcfg.menictime;
			paraminfo->eventinfo.door_cfg.paraminfo.doorlock[0].locktype = doorcfg.locktype;

			paraminfo->eventinfo.door_cfg.paraminfo.doorlock[1].lockid = 2;
			paraminfo->eventinfo.door_cfg.paraminfo.doorlock[1].lockswitch = doorcfg.lockswitch1;
			paraminfo->eventinfo.door_cfg.paraminfo.doorlock[1].locktime = doorcfg.locktime1;
			paraminfo->eventinfo.door_cfg.paraminfo.doorlock[1].menicswitch = doorcfg.menicswitch1;
			paraminfo->eventinfo.door_cfg.paraminfo.doorlock[1].menictime = doorcfg.menictime1;
			ret= 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_SETCFG:
		{
			goblin_door_cfg_t *pparaminfo = &paraminfo->eventinfo.door_cfg.paraminfo;

			door_cfg_t doorcfg = {0};
			doorcfg.motionlevel = pparaminfo->motion;
			doorcfg.soundcall = pparaminfo->soundcall;
			doorcfg.soundtalk = pparaminfo->soundtalk;
			doorcfg.lockswitch = pparaminfo->doorlock[0].lockswitch;
			doorcfg.locktime = pparaminfo->doorlock[0].locktime;
			doorcfg.menicswitch = pparaminfo->doorlock[0].menicswitch;
			doorcfg.menictime = pparaminfo->doorlock[0].menictime;
			doorcfg.locktype = pparaminfo->doorlock[0].locktype;

			doorcfg.lockswitch1 = pparaminfo->doorlock[1].lockswitch;
			doorcfg.locktime1 = pparaminfo->doorlock[1].locktime;
			doorcfg.menicswitch1 = pparaminfo->doorlock[1].menicswitch;
			doorcfg.menictime1 = pparaminfo->doorlock[1].menictime;
			SetDoorCfg(&doorcfg);

			gpio_control(GPIO_LOCK0TYPE_RELAOD, 0);
			//hard_node_mgnt_control(NODE_MSG_LED, NODE_STATE_ON);
			ret = 1;

			goblin_call_reload_AudioVolume(GetSoundtalkVol());
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_WAKEUP_REQ:
		{
			printf("唤醒房间 : %x 设备\n", paraminfo->eventinfo.wakeup_info.roomid);
			plc_wakeup_dev_by_room(paraminfo->eventinfo.wakeup_info.roomid);
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_DISABLEAGENT:
		{
			pmgnt->breportagent = false;
			//收到取消代理就重新搜索一次， 找可代理的设备
			goblin_plc_Search(0xffffffff, 1, 0);
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_CHECKSTATE:
		{
			if(time_relative_ms() - pmgnt->file_last_recv_tick < FILE_RECV_TIMEOUT_MS)
			{
				LOGW("Downloading the file. Other operations are not allowed.\n");
				paraminfo->eventinfo.action_state.outstate = 1;
				ret = 1;
				break;
			}
			//升级："update" 重启"reboot" 复位"reset" 关机"poweroff"
			printf("##############状态检测:%s #############\n", paraminfo->eventinfo.action_state.straction);
			if (strcmp(paraminfo->eventinfo.action_state.straction, "update")==0)
			{
				/* code */
			}
			else if (strcmp(paraminfo->eventinfo.action_state.straction, "reset")==0)
			{
				sys_event_t e = {0};
				SYS_EVENT_INIT_BASE(e, SYS_EVENT_BASE_FACTORY_RESET);
				sys_event_publish(&e);
			}
			else if (strcmp(paraminfo->eventinfo.action_state.straction, "reboot")==0)
			{
				sys_event_t e = {0};
				SYS_EVENT_INIT_BASE(e, SYS_EVENT_BASE_REBOOT);
				sys_event_publish(&e);
			}

			paraminfo->eventinfo.action_state.outstate = 0;
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_GETLOGFILESIZE:
		{
			printf("###################### 获取日志文件大小 #########################\n");
			paraminfo->eventinfo.logfile_info.outfilelen = 0;
			//获取缓存的日志文件大小
			DPGetFileAttributes(PLC_RUN_LOG_PATH, (int *)&paraminfo->eventinfo.logfile_info.outfilelen);
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_GETLOGFILE:
		{
			printf("###################### 获取日志文件 #########################\n");

			DPCopyFileEx(PLC_RUN_LOG_BK_PATH, PLC_RUN_LOG_PATH);
			DPGetFileAttributes(PLC_RUN_LOG_BK_PATH, (int *)&paraminfo->eventinfo.logfile_info.outfilelen);
			hareware_test_service_push_event(99, paraminfo->remotedev);
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_PLC_CONFLICT:
		{
			printf("###################### PLC地址冲突 #########################\n");
			sound_play_start(BEEP_WAV, 100, 100, false);
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_SET_SN:
		{
			printf("##########################  设置SN:%s #########################\n", paraminfo->eventinfo.sn_info.strsn);
			SetLocalSn(paraminfo->eventinfo.sn_info.strsn);
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_GET_SN:
		{
			printf("##########################  获取SN #########################\n");
			strncpy(paraminfo->eventinfo.sn_info.strsn, GetLocalSn(), sizeof(paraminfo->eventinfo.sn_info.strsn)-1);
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_CARD_ADDSTATE:
		{
			printf("##########################  卡片添加状态 state:%d  dev:%x ####################\n",
				paraminfo->eventinfo.card_add_state.state, paraminfo->remotedev);
			card_service_push_event(CARD_MODE_STATUS_ADD, paraminfo->eventinfo.card_add_state.state, paraminfo->remotedev);
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_CARD_DELSTATE:
		{
			printf("##########################  卡片删除状态 state:%d  dev:%x ##########\n",
				paraminfo->eventinfo.card_del_state.state, paraminfo->remotedev);
			card_service_push_event(CARD_MODE_STATUS_DEL, paraminfo->eventinfo.card_del_state.state, paraminfo->remotedev);
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_CARD_DELALL:
		{
			printf("##########################  卡片删除全部 ##########\n");
			ResetICCardDB();
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_CARD_DELONE:
		{
			printf("##########################  卡片删除一个 dev:%x card:%s ##########\n",
					paraminfo->remotedev, paraminfo->eventinfo.card_del_one.strcard);

			DelICCardByRoomAndID(strtol(paraminfo->eventinfo.card_del_one.strcard, NULL, 16),(paraminfo->remotedev>>8) & 0xffff);
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_CARD_MODIFY:
		{
			CardList_T cardinfo = {0};
			memset(&cardinfo, 0, sizeof(cardinfo));

			cardinfo.idtype = 1;
			cardinfo.idcode = strtol(paraminfo->eventinfo.card_info_modify.cardinfo.strcard, NULL, 16);
			cardinfo.roomid = (paraminfo->remotedev>>8) & 0xffff;
			cardinfo.ExpDate = paraminfo->eventinfo.card_info_modify.cardinfo.placestate;
			AddICCard(&cardinfo);
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_CARD_GETLIST:
		{
			printf("##########################  卡片获取列表 ##########\n");
			CardList_T* pCardarray = NULL;
			int cardcount = 0;
			int count =  GetICCardData(&pCardarray, &cardcount);
			if(count > 0 && pCardarray)
			{
				goblin_plc_cardinfo_s *pinfo = (goblin_plc_cardinfo_s*)malloc(sizeof(goblin_plc_cardinfo_s)*count);
				memset(pinfo, 0, sizeof(goblin_plc_cardinfo_s)*count);

				for(int i=0; i<count; i++)
				{
					if(pCardarray[i].roomid != ((paraminfo->remotedev>>8) & 0xffff))
						continue;
					sprintf(pinfo[i].strroomid, "%llx", pCardarray[i].roomid);
					sprintf(pinfo[i].strcard, "%llX", pCardarray[i].idcode);
					pinfo[i].placestate = pCardarray[i].ExpDate;
				}

				paraminfo->eventinfo.card_get_list_ack.card_count = count;
				paraminfo->eventinfo.card_get_list_ack.pcardlist = pinfo;

				free(pCardarray);
			}
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_SET_LANGUPAGE:
		{
			printf("##########################  设置房间语言 dev:0x%x index:%d ###################\n",
					paraminfo->remotedev, paraminfo->eventinfo.language_info.languageindex);
			LanguageList_T info = {0};
			memset(&info, 0, sizeof(LanguageList_T));
			info.roomid = (paraminfo->remotedev>>8) & 0xffff;
			info.languageid = paraminfo->eventinfo.language_info.languageindex;
			AddLanguage(&info);
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_SET_AI_NOISE:
		{
			printf("##########  设置AI噪声 dev:0x%x benable:%d ###################\n",
					paraminfo->remotedev, paraminfo->eventinfo.action_ai_noise.benable);

			goblin_set_ai_noise_by_session(pmgnt->ptalk_session, paraminfo->eventinfo.action_ai_noise.benable);
			ret = 1;
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_HARDWARE_TEST:
		{
			LOGI("###################### 硬件测试 : %d #########################\n", paraminfo->eventinfo.hardware_test.operate_cmd);
			//命令字 1：开始 2：结束 3:喇叭测试开始 4:喇叭测试结束 5:录音开始(默认10秒) 6:录音结束 7:放音开始 8:放音结束 9：灯控制(流水线闪烁测试)
			if(paraminfo->eventinfo.hardware_test.operate_cmd == 1000){
				void set_video_ai_work_flag(bool flag);
				set_video_ai_work_flag(atol(paraminfo->eventinfo.hardware_test.strstate)==1);
			}
			else if (paraminfo->eventinfo.hardware_test.operate_cmd == 1001){
				SetCameraLight(atol(paraminfo->eventinfo.hardware_test.strstate));
			}
			else
				hareware_test_service_push_event(paraminfo->eventinfo.hardware_test.operate_cmd, paraminfo->remotedev);
			ret = 1;
		}
		break;
	default:
		break;
	}
	return ret;
}

static int CallEventHandle(void *param, plc_call_info_s *paraminfo)
{
	char *state_str[] = {"呼入", "呼出状态", "对端接听", "挂断", "挂断Ack", "I帧请求", "请求切换视角", "断开连接"};
	printf("呼叫状态 : [%s]  dev:%x  session:%p\n", paraminfo->type < 8 ? state_str[paraminfo->type] : "Unknown", paraminfo->dev, paraminfo->psession);
#if 1
	plc_service_mgnt_t *pmgnt = (plc_service_mgnt_t *)param;
	service_event_t event = {0};
	event.type = CALL_EVENT;
	memcpy(&event.callinfo, paraminfo, sizeof(plc_call_info_s));

	mq_send(pmgnt->event_queue, &event, sizeof(service_event_t));
	return 0;
#endif
}
#ifdef ENABLE_V6
#include "vlcview.h"
#endif
static int CallVideoEncStatus(void *param, uint8_t vencstart)
{
	printf("视频编码状态 : %s\n", vencstart ? "开始编码" : "停止编码");
#if 1
	video_service_push_event(vencstart ? VIDEO_ENC_START : VIDEO_ENC_STOP, 0);
#else
	uint32_t lasttick = time_relative_ms();
	Isp_run_type(vencstart);

	printf("编码操作耗时:%u \n", time_relative_ms() - lasttick);
#endif

	// if(!vencstart)
	// {
	// 	//结束视频编码传输，获取丢包的数量
	// 	goblin_plc_get_rssi();
	// }
	return 0;
}

////==============session管理============
static int get_session_count(plc_service_mgnt_t *pmgnt)
{
	int count = 0;
	pthread_mutex_lock(&pmgnt->session_mutex);
	count = pmgnt->session_count;
	pthread_mutex_unlock(&pmgnt->session_mutex);
	return count;
}

//添加会话链表
static void* session_info_add(plc_service_mgnt_t *pmgnt, call_session_t *sessioninfo)
{
    void *pret = NULL;

    call_session_t *p = NULL;
	call_session_t *n = NULL;

	pthread_mutex_lock(&pmgnt->session_mutex);

    //轮询当前列表是否存在对应设备呼叫，如果存在就返回当前的， 没有就创建的
	if (!list_empty_careful(&pmgnt->session_list))
	{
		list_for_each_entry_safe(p, n, &pmgnt->session_list, node)
		{
			if (p->devid == sessioninfo->devid)
			{
				pret = (void*)p;
				break;
			}
		}
	}
    if(!pret)
    {
        call_session_t *pinfo = (call_session_t *)bmalloc(sizeof(call_session_t));
        if (pinfo)
        {
            memset(pinfo, 0, sizeof(call_session_t));
            memcpy(pinfo, sessioninfo, sizeof(call_session_t));
            list_add_tail(&pinfo->node, &pmgnt->session_list);
            pmgnt->session_count++;
            pret = (void*)pinfo;
        }
        else
            LOGE("alloc msg_info_t fail \n");
    }

	pthread_mutex_unlock(&pmgnt->session_mutex);

	return pret;
}

//删除会话链表
static uint32_t session_info_del_by_session_or_dev(plc_service_mgnt_t *pmgnt, void*psession, uint32_t dev)
{
	uint32_t ret = 0;

	call_session_t *p = NULL;
	call_session_t *n = NULL;

	pthread_mutex_lock(&pmgnt->session_mutex);

	if (!list_empty_careful(&pmgnt->session_list))
	{
		list_for_each_entry_safe(p, n, &pmgnt->session_list, node)
		{
			if (psession == p->psession || p->devid == dev)
			{
				list_del(&p->node);
                pmgnt->session_count--;
                bfree(p);
                ret = pmgnt->session_count;
				break;
			}
		}
	}
	pthread_mutex_unlock(&pmgnt->session_mutex);

	LOGI("Del session list count:%d \n", ret);
	return ret;
}

static uint32_t session_hangup_by_dev(plc_service_mgnt_t *pmgnt, uint32_t dev, bool breversal)
{
	uint32_t ret = 0;

	call_session_t *p = NULL;
	call_session_t *n = NULL;

	pthread_mutex_lock(&pmgnt->session_mutex);

	if (!list_empty_careful(&pmgnt->session_list))
	{
		list_for_each_entry_safe(p, n, &pmgnt->session_list, node)
		{
			if ((breversal && p->devid != dev) || (!breversal && p->devid == dev))
			{
				LOGI("Hangup session dev:%x  breversal:%d \n", p->devid, breversal);
				list_del(&p->node);
                pmgnt->session_count--;
				goblin_hangup_by_session(p->psession);
                bfree(p);
                ret = pmgnt->session_count;
				//break;
			}
		}
	}
	pthread_mutex_unlock(&pmgnt->session_mutex);

	LOGI("Hangup session list count:%d \n", ret);
	return ret;
}

static uint32_t session_hangup_by_callout(plc_service_mgnt_t *pmgnt, uint8_t iscallout)
{
	uint32_t ret = 0;

	call_session_t *p = NULL;
	call_session_t *n = NULL;

	pthread_mutex_lock(&pmgnt->session_mutex);

	if (!list_empty_careful(&pmgnt->session_list))
	{
		list_for_each_entry_safe(p, n, &pmgnt->session_list, node)
		{
			if (p->bcallout == iscallout)
			{
				list_del(&p->node);
                pmgnt->session_count--;
				goblin_hangup_by_session(p->psession);
                bfree(p);
                ret = pmgnt->session_count;
				//break;
			}
		}
	}
	pthread_mutex_unlock(&pmgnt->session_mutex);

	LOGI("Hangup session list count:%d \n", ret);
	return ret;
}

static uint32_t session_info_hangup_all(plc_service_mgnt_t *pmgnt)
{
	uint32_t ret = 0;

	call_session_t *p = NULL;
	call_session_t *n = NULL;

	pthread_mutex_lock(&pmgnt->session_mutex);

	if (!list_empty_careful(&pmgnt->session_list))
	{
		list_for_each_entry_safe(p, n, &pmgnt->session_list, node)
		{
			list_del(&p->node);
			pmgnt->session_count--;
			goblin_hangup_by_session(p->psession);
			bfree(p);
			ret = pmgnt->session_count;
		}
	}
	LOGI("Del all session list count:%d \n", ret);
	pmgnt->session_count = 0;
	pthread_mutex_unlock(&pmgnt->session_mutex);

	return ret;
}

static uint32_t session_change_status_by_dev(plc_service_mgnt_t *pmgnt, uint32_t dev, call_status_t status)
{
	uint32_t ret = -1;

	call_session_t *p = NULL;
	call_session_t *n = NULL;

	pthread_mutex_lock(&pmgnt->session_mutex);

	if (!list_empty_careful(&pmgnt->session_list))
	{
		list_for_each_entry_safe(p, n, &pmgnt->session_list, node)
		{
			if (p->devid == dev)
			{
				p->status = status;
				ret = 0;
			}
		}
	}
	pthread_mutex_unlock(&pmgnt->session_mutex);

	return ret;
}

static uint32_t session_check_dev_by_status(plc_service_mgnt_t *pmgnt, call_status_t status)
{
	uint32_t ret = 0;

	call_session_t *p = NULL;
	call_session_t *n = NULL;

	pthread_mutex_lock(&pmgnt->session_mutex);

	if (!list_empty_careful(&pmgnt->session_list))
	{
		list_for_each_entry_safe(p, n, &pmgnt->session_list, node)
		{
			if (p->status == status)
			{
				ret=1;
				break;
			}
		}
	}
	pthread_mutex_unlock(&pmgnt->session_mutex);

	return ret;
}
#if DEBUG_HANGUP_BY_KEY
//检查是否有呼入呼出的通话
static uint32_t session_check_dev_by_callout(plc_service_mgnt_t *pmgnt, uint8_t iscallout)
{
	uint32_t ret = 0;

	call_session_t *p = NULL;
	call_session_t *n = NULL;

	pthread_mutex_lock(&pmgnt->session_mutex);

	if (!list_empty_careful(&pmgnt->session_list))
	{
		list_for_each_entry_safe(p, n, &pmgnt->session_list, node)
		{
			if (p->bcallout == iscallout)
			{
				ret=1;
				break;
			}
		}
	}
	pthread_mutex_unlock(&pmgnt->session_mutex);

	return ret;
}
#endif

static int session_info_find_by_dev(plc_service_mgnt_t *pmgnt, uint32_t dev, call_session_t *pinfo)
{
	int ret = 0;

	call_session_t *p = NULL;
	call_session_t *n = NULL;

	pthread_mutex_lock(&pmgnt->session_mutex);

	if (!list_empty_careful(&pmgnt->session_list))
	{
		list_for_each_entry_safe(p, n, &pmgnt->session_list, node)
		{
			if (dev == p->devid)
			{
                if(pinfo)
				    memcpy(pinfo, p, sizeof(call_session_t));
                ret = 1;
				break;
			}
		}
	}
	pthread_mutex_unlock(&pmgnt->session_mutex);

	return ret;
}

//会话管理
static void process_call_event(plc_service_mgnt_t *mgnt, plc_call_info_s *paraminfo)
{
	if (paraminfo->type == PLC_CALL_STATE_CALLIN)
	{
		// psession= paraminfo->psession;

#if DEBUG_HANGUP_BY_KEY
		//测试可以允许多路一起监视
		if(session_check_dev_by_callout(mgnt, 1)==1 || session_check_dev_by_status(mgnt, SESSION_TALKING)==1)
#else
		//当前只能允许一个监视
		if(get_session_count(mgnt)>0)
#endif
		{
			goblin_call_ack_state(paraminfo->psession, 1);
			goblin_hangup_by_session(paraminfo->psession);
		}
		else
		{
			goblin_call_ack_state(paraminfo->psession, 0);
			//goblin_set_video_cb_by_session(psession, RemoteVideo_CB, NULL);
			goblin_call_accept(paraminfo->psession, 0);

			call_session_t session_info = {0};
			session_info.devid = paraminfo->dev;
			session_info.psession = paraminfo->psession;
			session_info.status = 0;
			session_info.stick = time_relative_ms();
			session_info.bcallout = 0;
			session_info_add(mgnt, &session_info);

			int goblin_set_videotype_trans_by_session(void *psession, uint8_t isonoff, uint8_t videotype);
			goblin_set_videotype_trans_by_session(paraminfo->psession, 1, paraminfo->eventinfo.callin.video_type);
#ifdef ENABLE_V1
			video_idr(0);
#elif defined(ENABLE_V6)
			RequestIFrame();
#endif
			mgnt->ptalk_session = paraminfo->psession;
		}

	}
	else if (paraminfo->type == PLC_CALL_STATE_CALLOUT_ACK)
	{

		if(paraminfo->eventinfo.callout_ack.state == 0)
		{
			start_play_ring(mgnt);
#ifdef ENABLE_V1
			video_crop_switch(0);
			video_idr(0);
#elif defined(ENABLE_V6)
			// RequestIFrame();
			//RequestCropFrame(0);
#endif
		}
		else{
			LOGW("Callout ack state [%d]\n", paraminfo->eventinfo.callout_ack.state);
			//最后一个繁忙挂断
			if(session_hangup_by_dev(mgnt, paraminfo->dev, false)==0 && paraminfo->eventinfo.callout_ack.state == 1)
			{
				LOGW("Last busy hangup all\n");
				sound_play_start(BEEP_WAV, 1, 60, false);
			}
		}
	}
	else if (PLC_CALL_STATE_ACCEPT == paraminfo->type)
	{
		call_session_t sessioninfo ={0};
		//在呼叫列表的设备接听才有效，否则忽略(防止同时接听互相挂断的情况)
		if(session_info_find_by_dev(mgnt, paraminfo->dev, &sessioninfo)==1)
		{
			mgnt->ptalk_session = paraminfo->psession;
			stop_play_ring(mgnt);
			//接听后挂断其他设备
			session_hangup_by_dev(mgnt, paraminfo->dev, true);
			session_change_status_by_dev(mgnt, paraminfo->dev, SESSION_TALKING);
		}
		else {
			LOGW("The Dev:%x is not in call list, ignore it\n", paraminfo->dev);
		}
	}
	else if (PLC_CALL_STATE_HUNGUP == paraminfo->type ||
			PLC_CALL_STATE_DISCONNECT == paraminfo->type ||
			PLC_CALL_STATE_HANGUP_ACK == paraminfo->type)
	{
		//对端挂断时，挂断会话列表所有通话
		call_session_t sessioninfo ={0};
		//呼出的有被挂断就挂断所有
		if(session_info_find_by_dev(mgnt, paraminfo->dev, &sessioninfo) && sessioninfo.bcallout ==1 &&
			PLC_CALL_STATE_HUNGUP == paraminfo->type){
			//超时挂断的只挂断当前设备
			if(paraminfo->eventinfo.hungup.reason == 1)
			{
				session_hangup_by_dev(mgnt, paraminfo->dev, false);
			}
			else
				session_info_hangup_all(mgnt);
		}
		if(session_info_del_by_session_or_dev(mgnt, paraminfo->psession, 0)==0){
			stop_play_ring(mgnt);

			if(PLC_CALL_STATE_HANGUP_ACK != paraminfo->type)
			{
				//被挂断后可以立马发起呼叫，不用等待延时结束
				LOGI("The last session is hangup, start new lunch, lost time:%u\n", time_relative_ms()-mgnt->key_last_tick);
				mgnt->key_last_tick = 0;
			}
		}
	}
	else if (paraminfo->type == PLC_CALL_STATE_REQUEST_I)
	{
#ifdef ENABLE_V1
		printf("Request send data to channel\n");
		video_idr(0);
#elif defined(ENABLE_V6)
	 	RequestIFrame();
#endif
	}
	else if (paraminfo->type == PLC_CALL_STATE_REQUEST_VIEW)
	{
		/* code */
#ifdef ENABLE_V1
		video_crop_switch(paraminfo->eventinfo.request_view.iscrop);
#else
		RequestCropFrame(paraminfo->eventinfo.request_view.iscrop);
#endif
	}
}

#define KEY_EVENT_TIMEOUT_MS (5000)
static void process_button_event(plc_service_mgnt_t *mgnt, button_msg_t *msg)
{
	if (msg->code == 99)
	{
		printf("############## Ready to auto test call out ################\n");
#if DEBUG_HANGUP_BY_KEY
		mgnt->is_call_auto_test = 1;
		sound_play_start(BEEP_WAV, 1, 100, true);
#endif
		return;
	}

	//刷新休眠状态时间戳
	video_service_push_event(VIDEO_TICK_REFRESH, 0);
	uint32_t cur_tick = time_relative_ms();
	uint32_t diff_tick = cur_tick - mgnt->key_last_tick;
	LOGI("button event code [%d] state [%d] diff_tick [%d]\n", msg->code, msg->state, diff_tick);

	//呼出时，挂断所有呼入(监视)，呼叫大于监视
	session_hangup_by_callout(mgnt, 0);

	if(diff_tick > KEY_EVENT_TIMEOUT_MS)
	{
		if(mgnt->key_last_code !=msg->code && get_session_count(mgnt))
		{
			LOGW("The key [%d] is pressed, but the last key [%d] session is not released, ignore it\n", msg->code, mgnt->key_last_code);
			return;
		}
#if DEBUG_HANGUP_BY_KEY
		if(mgnt->key_last_code == msg->code && get_session_count(mgnt))
		{
			LOGW("The key [%d] is pressed, but the last key [%d] session is exist, hangupall\n", msg->code, mgnt->key_last_code);
			session_info_hangup_all(mgnt);
			stop_play_ring(mgnt);
			return;
		}
#else
		if(mgnt->key_last_code == msg->code && get_session_count(mgnt))
		{
			LOGW("The key [%d] is pressed, but the last key [%d] session is exist, ignore it\n", msg->code, mgnt->key_last_code);
			return;
		}
#endif //DEBUG_HANGUP_BY_KEY
		if(session_check_dev_by_status(mgnt, SESSION_TALKING)==1)
		{
			LOGI("The key [%d] is pressed, but the last session is talking, ignore it\n", msg->code);
			return;
		}
		//获取存在的设备,直接用于呼叫，防止发搜索包没有及时回复还占用带宽
		goblin_plc_get_rssi();

		uint32_t dstdevid = 0x1000101;
		for (size_t i = 0; i < 4; i++)
		{
			dstdevid = (0x1000001 | (msg->code << 8)) + i;
			goblin_plc_open_dev(dstdevid);
		}

		mgnt->key_last_tick = cur_tick;
		mgnt->key_last_code = msg->code;
		mgnt->bkey_call_dev = true;

		if(get_session_count(mgnt)==0)
			stop_play_ring(mgnt);
		goblin_plc_Search(0xffffffff, 1, msg->code);

		// mgnt->search_count = 2;
		// mgnt->last_search_tick = cur_tick;
		return;
	}
	else
	{
#if DEBUG_HANGUP_BY_KEY
		if(mgnt->key_last_code == msg->code && get_session_count(mgnt))
		{
			LOGW("The key [%d] is pressed, but the last key [%d] session is exist, hangupall\n", msg->code, mgnt->key_last_code);
			session_info_hangup_all(mgnt);
			stop_play_ring(mgnt);
			//手动挂断后可以立即呼叫
			mgnt->key_last_tick = 0;
			return;
		}
#endif //DEBUG_HANGUP_BY_KEY
	}
}

//请求代理
static void RequestAgent(plc_service_mgnt_t *mgnt, uint32_t devid)
{
	goblin_agent_info_s ininfo = {0};

	unsigned char mac[6] = {0};
	DpGetMacByNetcworkard(mac, "eth0");
	sprintf(ininfo.strmac, "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#ifdef ENABLE_V1
		strncpy(ininfo.strmodel, "P-VCU-A0", 32);
#elif ENABLE_V6
		strncpy(ininfo.strmodel, g_szDevType, 32);
#endif
	sprintf(ininfo.strid, "%x", mgnt->dev_id);

	sprintf(ininfo.strplcver, "%x", mgnt->plc_version);
	ininfo.platform = 2;

	strncpy(ininfo.strhwver, HARD_WARE_VERSION, 32);
	strncpy(ininfo.strsysver, "1.0.0", 32);
	strncpy(ininfo.strappver, SOFT_WARE_VERSION, 32);

	if(strlen(GetLocalSn())==0){
		unsigned char mac[6] = {0};
		char strbuf[63] = {0};
		DpGetMacByNetcworkard(mac, "eth0");
		sprintf(strbuf, "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		strncpy(ininfo.strSN, strbuf, 64-1);
	}
	else
		strncpy(ininfo.strSN, GetLocalSn(), 64-1);

	strncpy(ininfo.strtimezone, "+08:00", 32);
	goblin_agent_ack_s outinfo = {0};
	int ret = goblin_plc_check_agent(devid, &ininfo, &outinfo, 1000);

	if(ret == 0 && strlen(outinfo.strdatetime) > 0)
	{
		struct tm ltime = {0};
		time_get_local_time(&ltime);

		int year = 0;
		int month = 0;
		int day = 0;
		int hour = 0;
		int minute = 0;
		int second = 0;
		sscanf(outinfo.strdatetime, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

		ltime.tm_year = year;
		ltime.tm_mon = month;
		ltime.tm_mday = day;
		ltime.tm_hour = hour;
		ltime.tm_min = minute;
		ltime.tm_sec = second;
		time_set_local_time(&ltime);
	}

	LOGI("goblin_plc_check_agent ret:%d b:%d time:%s\n", ret, outinfo.benableagent, outinfo.strdatetime);
	mgnt->breportagent = ret==0;
	mgnt->last_agent_tick = time_relative_ms();
}

static void process_devinfo_event(plc_service_mgnt_t *mgnt, plc_dev_info_t *msg)
{
	uint32_t cur_tick = time_relative_ms();
	uint32_t diff_tick = cur_tick - mgnt->key_last_tick;
	//LOGI("Dev find lost time [%u] code [%d] dev [%x] room [%d]\n", diff_tick, mgnt->key_last_code, msg->devid, (msg->devid>>8)&0xff);
	if (diff_tick < KEY_EVENT_TIMEOUT_MS && ((msg->devid>>8)&0xff) == mgnt->key_last_code)
	{
		if(mgnt->bkey_call_dev)
		{
			mgnt->bkey_call_dev = false;
		}
		//防止提前在线的设备已经呼叫接听了， 还有其他设备上线
		if(session_check_dev_by_status(mgnt, SESSION_TALKING)==1)
		{
			LOGI("The last session is talking, ignore it [%x]\n", msg->devid);
			return;
		}
		call_session_t sessioninfo ={0};
		//已在呼叫列表的设备不再重复呼叫
		if(session_info_find_by_dev(mgnt, msg->devid, &sessioninfo)==0)
		{
			void *psession = goblin_call_by_dev(msg->devid);
			LOGI("Time to call dev [%x] psession [%p]\n", msg->devid, psession);

			mgnt->call_last_tick = cur_tick;

			call_session_t session_info = {0};
			session_info.devid = msg->devid;
			session_info.psession = psession;
			session_info.status = 0;
			session_info.stick = cur_tick;
			session_info.bcallout = 1;
			session_info_add(mgnt, &session_info);
		}
		else {
			LOGW("The dev [%x] is in session list, ignore it\n", msg->devid);
		}
	}

	if(!mgnt->breportagent && msg->isagent)
	{
		mgnt->agent_devid = msg->devid;
		RequestAgent(mgnt, msg->devid);
	}
}

static void process_timeout_event(plc_service_mgnt_t *mgnt)
{
	uint32_t ctick = time_relative_ms();
    call_session_t *p = NULL;
	call_session_t *n = NULL;
    pthread_mutex_lock(&mgnt->session_mutex);

	if (!list_empty_careful(&mgnt->session_list))
	{
		//printf("msg_check_thread list_empty_careful g_msg_list_count：%d\n", g_msg_list_count);
        //uint32_t ctick = time_relative_ms();
		list_for_each_entry_safe(p, n, &mgnt->session_list, node)
		{
			//printf("<## %u ##>msg_check_thread p->remote_dev:%x p->stick:%u losttime:%u p->status:%d p->psession:%p\n", ctick, p->devid, p->stick, ctick-p->stick, p->status, p->psession);
		}
	}

	if(mgnt->bkey_call_dev)
	{
		if(ctick - mgnt->key_last_tick > KEY_EVENT_TIMEOUT_MS)
		{
			mgnt->bkey_call_dev = false;
			LOGE("The key [%d] is timeout, call dev [%d] fail\n", mgnt->key_last_code, mgnt->key_last_code);

			sound_play_start(OFFLINE_MP3, 1, 100, true);
		}
	}

	pthread_mutex_unlock(&mgnt->session_mutex);

	pthread_mutex_lock(&mgnt->file_mutex);
	//文件接收超时时间
	if (mgnt->pfp && time_relative_ms() - mgnt->file_last_recv_tick > FILE_RECV_TIMEOUT_MS)
	{
		LOGW("The file recv timeout, close it\n");
		fclose(mgnt->pfp);
		mgnt->pfp = NULL;
		mgnt->file_last_recv_tick = 0;
		if(mgnt->filetype==1){
			_vlcview(NULL, 1234);
		}
	}
	pthread_mutex_unlock(&mgnt->file_mutex);

	if (mgnt->is_call_auto_test && ctick - mgnt->call_last_tick > 10*1000 && get_session_count(mgnt)==0)
	{
		mgnt->call_last_tick = ctick ;
		static uint8_t key_code = 0;

		if((mgnt->dev_id&0xf) == 1)
		{
			key_code++;
			if(key_code > 4)
				key_code=1;
		}
		else{
			if(key_code>0)
				key_code--;
			if(key_code <= 0)
				key_code=4;
		}

		//key_code = 1;
		static int autocallindex = 0;
		autocallindex++;
		/* code */

		bmem_debug_dump();
		LOGE("###########msg_check_thread %d key_code:%d g_msg_list_count:%d\n", autocallindex, key_code, get_session_count(mgnt));
		unsigned int dwTotal, dwFree;
		DumpMemory(&dwTotal, &dwFree);
		LOGI("Memory usage: %d%%, %u, %u \n",  (dwTotal - dwFree) * 100 / dwTotal, dwFree, dwTotal);
#if 1
		plc_service_key_event(key_code);
#else
		uint32_t dstdevid = 0x1000103;
		void *psession = goblin_call_by_dev(dstdevid);
		LOGI("2222 Time to call dev [%x] psession [%p]\n", dstdevid, psession);

		mgnt->call_last_tick = time_relative_ms();;

		call_session_t session_info = {0};
		session_info.devid = dstdevid;
		session_info.psession = psession;
		session_info.status = 0;
		session_info.stick = time_relative_ms();;
		session_info.bcallout = 1;
		session_info_add(mgnt, &session_info);
#endif
	}

	//存在代理的情况下， 每分钟上报一次，当心跳包
	if (mgnt->breportagent && ctick-mgnt->last_agent_tick > 60*1000)
	{
		LOGI("The agent is timeout, try it, dev:%x\n", mgnt->agent_devid);
		RequestAgent(mgnt, mgnt->agent_devid);
	}
	//如果要持续搜索，则每500ms搜索一次
	if(mgnt->search_count > 0 && ctick - mgnt->last_search_tick > 500)
	{
		goblin_plc_Search(0xffffffff, 1, mgnt->key_last_code);
		mgnt->search_count--;
		mgnt->last_search_tick = ctick;
	}
}

static void *call_process_thread(void *args)
{
	plc_service_mgnt_t *mgnt = (plc_service_mgnt_t *)args;

	LOGI("spi_plc_call_thread start [%p] event_queue:[%p]\n", mgnt, mgnt->event_queue);
	service_event_t event = {0};

	while (mgnt->isrun)
	{
		if (0 == mq_recv(mgnt->event_queue, &event, sizeof(service_event_t), 100))
		{
			LOGD("recv event type [%d] mgnt [%p]\n", event.type, mgnt);

			if (CALL_EVENT == event.type)
			{
				process_call_event(mgnt, &event.callinfo);
			}
			else if (BUTTON_EVENT == event.type)
			{
				process_button_event(mgnt, &event.button);
			}
			else if (DEV_INFO_EVENT == event.type)
			{
				process_devinfo_event(mgnt, &event.devinfo);
			}

            memset(&event, 0, sizeof(service_event_t));
		}

		process_timeout_event(mgnt);
	}

	return NULL;
}


void Md5_File(char *pfilename, char *pOutBuf);
#ifdef ENABLE_SPINAAD
static	upgrade_info upgradeinfo = {0};
static md5_ctx_t g_ctx;
#endif //#ifdef ENABLE_SPINAAD
int PlcFileCallBackFunc(void *param, file_pack_t *paraminfo, uint32_t remotedev)
{
	int ret = -1;
	plc_service_mgnt_t *mgnt = (plc_service_mgnt_t *)param;

	pthread_mutex_lock(&plc_service_mgnt->file_mutex);

	if((paraminfo->fileflag&FILE_START_FLAG) && mgnt->pfp == NULL)
	{
		mgnt->filelen = 0;
		mgnt->filepackseq = paraminfo->fileseq;
		mgnt->filetype = paraminfo->file_type;
		mgnt->file_last_recv_tick = time_relative_ms();
		mgnt->file_fisrt_recv_tick = mgnt->file_last_recv_tick;
		if(mgnt->filetype==1){
			//编码会影响SPI传输数据，因为共用DMA
			_vlcview_exit();
			mgnt->pfp = fopen(DOWNLOAD_IMAGE_PATH, "wb");
		}
		else if(mgnt->filetype==2){
			mgnt->pfp = fopen(DOWNLOAD_IMAGE_PLC_PATH, "wb");
		}
		LOGI("recv filetype:%d  open file :%p ==========\n", mgnt->filetype, mgnt->pfp);
	}
	if (mgnt->pfp)
	{
		uint8_t *pdata = (uint8_t *)paraminfo->data;
		uint32_t datalen = paraminfo->datalen;

#ifdef ENABLE_SPINAAD
		//SPINAAD升级的包头信息单独处理
		if(paraminfo->fileflag&FILE_START_FLAG && paraminfo->datalen > sizeof(upgradeinfo))
		{
			memcpy(&upgradeinfo, paraminfo->data, sizeof(upgradeinfo));
			printf("recv upgrade info updatetype:%d \nsoftCo:%s \nfilemd5:%s \nsoftver:%s \nhavereg:%d \nendiantype:%d\n",
			upgradeinfo.updatetype, upgradeinfo.softCo, upgradeinfo.filemd5, upgradeinfo.softver, upgradeinfo.havereg, upgradeinfo.endiantype);

			pdata = (uint8_t *)((uint8_t *)paraminfo->data + sizeof(upgradeinfo));
			datalen = paraminfo->datalen - sizeof(upgradeinfo);
			//MD5是一块块送进去的，需要重新计算
			md5_begin(&g_ctx);

			md5_hash(&upgradeinfo, sizeof(upgradeinfo), &g_ctx);
		}
#endif
		//序号要连续
		if(paraminfo->fileseq == (mgnt->filepackseq+1) || (paraminfo->fileflag&FILE_START_FLAG))
		{
			mgnt->file_last_recv_tick = time_relative_ms();
			mgnt->filelen+=datalen;
			mgnt->filepackseq = paraminfo->fileseq;
			fwrite(pdata, 1, datalen, mgnt->pfp);
#ifdef ENABLE_SPINAAD
			md5_hash(pdata, datalen, &g_ctx);
#endif
		}
		else
		{
			LOGE("The file seq %u-%u is not continuous, ignore it\n", paraminfo->fileseq, mgnt->filepackseq);
		}
	}

	if((paraminfo->fileflag&FILE_END_FLAG) && mgnt->pfp)
	{
		DPFlush(mgnt->pfp);
		fclose(mgnt->pfp);
		mgnt->pfp = NULL;
		mgnt->file_last_recv_tick = 0;
		char strmd5[33] = {0};
		char strmd5_tmp[33] = {0};
		if(mgnt->filetype==1)
		{
#ifdef ENABLE_SPINAAD
			char md5_buf[16] = {0};
			md5_end(md5_buf, &g_ctx);
			for (int i = 0; i < 16; i++)
				sprintf(strmd5_tmp + 2 * i, "%02x", md5_buf[i]);

			strmd5_tmp[32] = 0;
			char md5[64] = {0};
			Md5_File(DOWNLOAD_IMAGE_PATH, md5);
			LOGI("md5:%s \n", md5);
			if (strncmp(upgradeinfo.softCo, MODEL_NAME, 6) == 0 && strcmp(md5, upgradeinfo.filemd5) == 0)
			{
				LOGI("md5 is match\n");
			}
			else
			{
				LOGW("md5 is not match, softCo:%s, MODEL_NAME:%s, md5:%s, filemd5:%s\n",
					upgradeinfo.softCo, MODEL_NAME, md5, upgradeinfo.filemd5);
				remove(DOWNLOAD_IMAGE_PATH);
			}
#else
			Md5_File(DOWNLOAD_IMAGE_PATH, strmd5_tmp);
#endif
			memcpy(strmd5, paraminfo->filemd5, 32);
			printf("md5:%s\n", strmd5_tmp);
			//MD5一样
			if (strcmp(strmd5_tmp, strmd5) == 0)
			{
#if 1
				sys_event_t e = {0};
				SYS_EVENT_INIT_BASE(e, SYS_EVENT_BASE_UPGRADE);
				e.body.base_msg.devid = remotedev;
				sys_event_publish(&e);
#else
				printf("=============== upgrade file success, file:%s md5:%s curr md5:%s ================\n", DOWNLOAD_IMAGE_PATH, strmd5, strmd5_tmp);
				_vlcview(NULL, 1234);
#endif
			}
			else{
				LOGW("The md5 of file is not match, file:%s md5:%s curr md5:%s\n", DOWNLOAD_IMAGE_PATH, strmd5, strmd5_tmp);
				remove(DOWNLOAD_IMAGE_PATH);
				//下载失败，重新开始编码
				if(mgnt->filetype==1)
				{
					_vlcview(NULL, 1234);
				}
			}
		}
		else if (mgnt->filetype==2)
		{
			Md5_File(DOWNLOAD_IMAGE_PLC_PATH, strmd5_tmp);
			memcpy(strmd5, paraminfo->filemd5, 32);
			printf("md5:%s\n", strmd5_tmp);
			//MD5一样
			if (strcmp(strmd5_tmp, strmd5) == 0)
			{
				sys_event_t e = {0};
				SYS_EVENT_INIT_BASE(e, SYS_EVENT_BASE_UPGRADE_PLC);
				sys_event_publish(&e);
			}
			else{
				LOGW("The md5 of file is not match, file:%s md5:%s curr md5:%s\n", DOWNLOAD_IMAGE_PLC_PATH, strmd5, strmd5_tmp);
				remove(DOWNLOAD_IMAGE_PLC_PATH);
			}
		}

		//if(strcmp(strmd5, paraminfo->filemd5)==0)
		LOGI("\n\n=====================remotedev:%d end of file filelen:%d lost time:%d ================\n\n\n", remotedev, mgnt->filelen, time_relative_ms()-mgnt->file_fisrt_recv_tick);

	}

	pthread_mutex_unlock(&plc_service_mgnt->file_mutex);
	return ret;
}

static int LocalVideoDropStatusCB(uint32_t dropcount, uint32_t recvcount, float droprate, void *puserdata)
{
	//printf("LocalVideoDropStatusCB dropcount:%d recvcount:%d droprate:%f\n", dropcount, recvcount, droprate);
	//丢包要超过10个，接收包要超过1500个
	if(dropcount > 10 && recvcount > 1500){
		if(droprate >= 1.0)
		{
			video_service_push_event(VIDEO_ENC_RELOAD, 2);
		}
		else if (droprate >= 10.0)
		{
			video_service_push_event(VIDEO_ENC_RELOAD, 3);
		}
		else if (droprate >= 20.0)
		{
			video_service_push_event(VIDEO_ENC_RELOAD, 5);
		}
	}
	else{
		LOGW("LocalVideoDropStatusCB dropcount:%d recvcount:%d droprate:%f\n", dropcount, recvcount, droprate);
	}
	return 0;
}

int plc_service_init(uint32_t devid)
{
	int ret = -1;

    if (plc_service_mgnt == NULL)
	{
		plc_service_mgnt = bmalloc(sizeof(plc_service_mgnt_t));

		if (plc_service_mgnt == NULL)
		{
			LOGE("malloc plc_service_mgnt_t fail \n");
			return -1;
		}

		check_device_type(plc_service_mgnt);

		memset(plc_service_mgnt, 0, sizeof(plc_service_mgnt_t));
        //1.
		INIT_LIST_HEAD(&plc_service_mgnt->session_list);
        //2.
		pthread_mutex_init(&plc_service_mgnt->session_mutex, NULL);
		pthread_mutex_init(&plc_service_mgnt->file_mutex, NULL);

		if (access("/data/autocall", F_OK) == 0)
		{
			plc_service_mgnt->is_call_auto_test = 1;
			plc_service_mgnt->call_last_tick = time_relative_ms();
		}

		plc_service_mgnt->dev_id = devid;
        //3.
		plc_service_mgnt->event_queue = mq_create("plc_event_queue", sizeof(service_event_t), MAX_SESSION_EVENT);
        //4.
        plc_service_mgnt->isrun = 1;
		pthread_create(&plc_service_mgnt->thread, NULL, call_process_thread, plc_service_mgnt);

		//5.
		PlcEventHandler  param = {0};
		param.m_pfuncb = PlcEventHandleCB;
		param.m_pcbparam = (void*)plc_service_mgnt;
		param.m_pfilecb = PlcFileCallBackFunc;
		param.m_filecbparam = (void*)plc_service_mgnt;
		param.m_pdevplc = devid;

		param.m_pdevver_hw = HARD_WARE_VERSION;
		param.m_pdevver_app = SOFT_WARE_VERSION;
		goblin_plc_process_start(&param);

		//6.初始化本地通话
		Call_Info_S callinfo = {0};
		callinfo.dev_plc = devid;
		callinfo.handfunc = CallEventHandle;
		callinfo.pusrdata = (void*)plc_service_mgnt;
		callinfo.pvideo_enc_status_cb = CallVideoEncStatus;
		callinfo.pvideo_enc_status_usrdata = (void*)plc_service_mgnt;

		callinfo.audio_play_volume = GetSoundtalkVol();

		callinfo.pvideo_drop_status_cb = LocalVideoDropStatusCB;
		goblin_call_init(&callinfo);
		ret = 0;
	}
    return ret;
}

int plc_service_deinit(void)
{
    int ret = -1;

    if (plc_service_mgnt)
    {
		//1.
		goblin_call_deinit();
		//2.
		goblin_plc_process_stop();
        //3.
        plc_service_mgnt->isrun = 0;
        pthread_join(plc_service_mgnt->thread, NULL);
        //4.
        mq_destroy(plc_service_mgnt->event_queue);

        //5.
		call_session_t *node = NULL, *next = NULL;
		pthread_mutex_lock(&plc_service_mgnt->session_mutex);
		list_for_each_entry_safe(node, next, &plc_service_mgnt->session_list, node)
		{
			list_del(&node->node);
			bfree(node);
		}
		pthread_mutex_unlock(&plc_service_mgnt->session_mutex);
        //6.
        pthread_mutex_destroy(&plc_service_mgnt->session_mutex);
		pthread_mutex_destroy(&plc_service_mgnt->file_mutex);
		bfree(plc_service_mgnt);
        plc_service_mgnt = NULL;
        ret = 0;
    }

    return ret;
}

int plc_service_key_event(uint8_t keycode)
{
	if (plc_service_mgnt && plc_service_mgnt->event_queue)
	{
		service_event_t event = {0};

		event.type = BUTTON_EVENT;
		event.button.code = keycode;

		return mq_send(plc_service_mgnt->event_queue, &event, sizeof(service_event_t));
	}

	return -1;
}

int plc_service_devid_reload(uint32_t devid)
{
	if (plc_service_mgnt)
	{
		plc_service_mgnt->dev_id = devid;
	}

	return 0;
}