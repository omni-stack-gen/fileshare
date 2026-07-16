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
#include <utils/cJSON.h>

#include <goblin_plc_callcore.h>
#include "DPDef.h"
#include <spi_plc/spi_plc_mgnt.h>
#include <spi_plc/spi_plc_trans.h>
#include "trans.h"
#include "tran_audio.h"
#include "tran_video.h"

#define MAX_CALL_EVENT          (32)

typedef uint8_t call_status_t;

// 呼叫状态
enum
{
	CALL_STATUS_IDLE = 0,           // 空闲状态
	CALL_STATUS_CALLINTG,           // 呼叫状态
	CALL_STATUS_RINGING,            // 振铃状态
	CALL_STATUS_ACCEPTING,          // 接听状态
	CALL_STATUS_TALKING,            // 通话状态
	CALL_STATUS_HANGING,            // 挂断状态
} ;
#pragma pack(1)
typedef struct call_msg
{
	// 高4位表示呼叫类型，低4位表示呼叫命令
	// 1) 呼叫命令字
	//      0、呼叫
	//      1、呼叫应答
	//      2、接听
	//      3、接听应答
	//      4、挂断
	//      5、挂断应答
	//
	uint8_t event;

	// 设备PLC编号，主要用于区分PLC设备，
	uint32_t dev;

	uint8_t iscrop;
	uint8_t isauido;
	uint8_t param;
	// 消息载体
	//
	uint8_t data[1024];
} call_msg_t;
#pragma pack()
// 呼叫命令
enum
{
	CMD_HEARTBEAT = 0,
	CMD_CALL ,
	CMD_CALL_ACK,
	CMD_ACCEPT,
	CMD_ACCEPT_ACK,
	CMD_HANGUP,
	CMD_HANGUP_ACK,
	CMD_REQ_IKEY,
	CMD_REQ_IKEY_ACK,
	CMD_SWITCH_VIEW,
	CMD_SWITCH_VIEW_ACK,
	CMD_SWITCH_AUDIO,
	CMD_SWITCH_AUDIO_ACK,
} __attribute__((packed));

enum
{
	CALL_EVENT,
	PARAM_EVENT
} __attribute__((packed));

typedef uint8_t call_event_type;

typedef struct button_msg
{
	uint8_t state;
	uint8_t code;
} button_msg_t;

typedef struct call_event
{
	call_event_type type;

	union
	{
		button_msg_t button;
		call_msg_t call;
	};
} call_event_t;

#define HEARTBEAT_INTERVAL (3000)
#define HEARTBEAT_TIMEOUT  (10000)
typedef struct call_session_
{
    struct list_head linked_node;
    void *psession;                             //当前通话指针
	uint32_t remote_dev;                        // 对端的PLC号
	uint32_t heartbeat_send_tick;               // 心跳包发送时间
	uint32_t heartbeat_recv_tick;               // 心跳包接收时间
    uint32_t stick;
	call_status_t status;                       // 该线路的呼叫阶段

	uint8_t is_audio_tran;                      // 是否有音频传输
	uint8_t is_callout;							//是否本地主动呼出

	uint8_t remote_video_type;                	// 对端视频类型 0:主码流 1:子码流
} call_session_s;

typedef struct goblin_plc_call_mgnt
{
	// 本机设备号
	uint32_t local_dev;
	uint8_t  isdoor;

    PlcCallEventHandleFun   handfuncCB;      //呼叫回调
	void *					handfuncParam;   //回调参数

	PlcVideoEncStatusCB     enc_status_cb; //视频编码状态回调
    void *                  enc_status_usrdata; //视频编码状态回调用户数据

    void *ptran_audio;
	//第一路视频传输
    void *ptran_video1;
	uint32_t trans_video_dev1;

	void *ptran_video2;
	uint32_t trans_video_dev2;

	pthread_mutex_t audio_mutex;
	pthread_mutex_t video_mutex;
	uint8_t is_disable_video_cache; //是否禁止视频缓存
	uint8_t is_disable_decode_show_video;	//是否禁止视频解码播放

	pthread_mutex_t session_mutex;
	struct list_head session_list;
    uint32_t session_count;

	uint32_t last_time;

	mq_t *call_queue;
	pthread_t thread;
    uint8_t isrun;

	uint32_t video_display_x;
	uint32_t video_display_y;
	uint32_t video_display_w;
	uint32_t video_display_h;

	uint32_t audio_play_volume;

	PlcVideoDropStatusCB    m_pvideo_drop_status_cb; //视频丢包统计回调
    void *                  m_pvideo_drop_status_usrdata; //视频丢包统计回调用户数据
} goblin_plc_call_mgnt_s;

static goblin_plc_call_mgnt_s *g_plc_call_mgnt = NULL;

void VideoDecFirstFrame(void)
{
	if (g_plc_call_mgnt && g_plc_call_mgnt->handfuncCB)
	{
		plc_call_info_s cbinfo = {0};
		cbinfo.type = PLC_CALL_STATE_VIDEO_DEC_START;
		g_plc_call_mgnt->handfuncCB(g_plc_call_mgnt->handfuncParam, &cbinfo);
	}
}

static const char *call_cmd_to_str(int cmd)
{
	switch (cmd)
	{
		case CMD_HEARTBEAT:
			return "heartbeat";

		case CMD_CALL:
			return "call";

		case CMD_CALL_ACK:
			return "call ack";

		case CMD_ACCEPT:
			return "accept";

		case CMD_ACCEPT_ACK:
			return "accept ack";

		case CMD_HANGUP:
			return "hangup";

		case CMD_HANGUP_ACK:
			return "hangup ack";

		case CMD_REQ_IKEY:
			return "req ikey";

		case CMD_SWITCH_VIEW:
			return "switch view";

		case CMD_SWITCH_AUDIO:
			return "switch audio";

		default :
			return "(unknown)";
	}
}

static void send_call_msg(goblin_plc_call_mgnt_s *mgnt, uint32_t dev, uint8_t event, uint8_t param)
{
	call_msg_t msg = {0};

	msg.event = event;
	msg.dev = mgnt->local_dev;
	msg.param = param;

	if (((mgnt->local_dev>>24) == 3) && (CMD_CALL == event || CMD_CALL_ACK == event))
	{
		/* code */
		cJSON *proot = cJSON_CreateObject();
		if (proot)
		{
			cJSON *pvideo = cJSON_AddObjectToObject(proot, "video");
			if (pvideo)
			{

				cJSON_AddNumberToObject(pvideo, "width", 1920);
				cJSON_AddNumberToObject(pvideo, "height", 1088);

			}
			char *pdatastr = cJSON_PrintUnformatted(proot);
			if (pdatastr)
			{
				int len = strlen(pdatastr);
				if (len > 1024)
					len = 1024;
				memcpy(msg.data, pdatastr, len);
				msg.data[len] = 0;
				cJSON_free(pdatastr);
			}
			cJSON_Delete(proot);
		}
	}

	spi_plc_mgnt_send(dev, CALL_PORT, (uint8_t *)&msg, sizeof(call_msg_t));

	LOGD("send call message to dev [0x%x] cmd [%s]  \n", dev, call_cmd_to_str(msg.event));
}

static int video_trans_drop_cb(tran_drop_info_s *pinfo)
{
	//printf("丢包发起端：%x  丢包数量：%x 丢包总数：%x  丢包率：%f \n", pinfo->remotedev, pinfo->dropcount, pinfo->recvcount, pinfo->droprate);
	//video_trans_drop_cb("");
	goblin_plc_call_mgnt_s *mgnt = (goblin_plc_call_mgnt_s*)pinfo->puserdata;
	if (mgnt->m_pvideo_drop_status_cb)
	{
		mgnt->m_pvideo_drop_status_cb(pinfo->dropcount, pinfo->recvcount, pinfo->droprate, mgnt->m_pvideo_drop_status_usrdata);
	}

	return	0;
}

static int stop_video_trans_by_remotedev(goblin_plc_call_mgnt_s *mgnt, int remotedev)
{
	int ret = 0;
	LOGI("stop_video_trans_by_remotedev remotedev:%x\n", remotedev);
	pthread_mutex_lock(&mgnt->video_mutex);
	if(mgnt->ptran_video1 && mgnt->trans_video_dev1 == remotedev)
	{
		plc_video_trans_stop(mgnt->ptran_video1);
		mgnt->ptran_video1 = NULL;
	}

	if(mgnt->ptran_video2 && mgnt->trans_video_dev2 == remotedev)
	{
		plc_video_trans_stop(mgnt->ptran_video2);
		mgnt->ptran_video2 = NULL;
	}

	if(mgnt->ptran_video1 || mgnt->ptran_video2){
		printf("remind stop video trans %p - %p\n", mgnt->ptran_video1, mgnt->ptran_video2);
		ret = 1;
	}
	pthread_mutex_unlock(&mgnt->video_mutex);
	return ret;
}

static int session_video_tran(void *param, bool onoff, uint32_t remote_dev, uint8_t videotype)
{
    if(!param)
        return -1;
    goblin_plc_call_mgnt_s *mgnt = (goblin_plc_call_mgnt_s*)param;
	pthread_mutex_lock(&mgnt->video_mutex);
	LOGI("Video tran onoff:%d  tran:%p-%p, remote_dev:%x disable_video_cache:%d\n",
		onoff, mgnt->ptran_video1, mgnt->ptran_video2, remote_dev, mgnt->is_disable_video_cache);
	bool bstop = false;
    if(onoff)
    {
		v_show_rct_t showrect = {0,0, 1024, 600};
		if(mgnt->video_display_x != 0)
			showrect.display_x = mgnt->video_display_x;
		if(mgnt->video_display_y != 0)
			showrect.display_y = mgnt->video_display_y;
		if(mgnt->video_display_w != 0)
			showrect.display_w = mgnt->video_display_w;
		if(mgnt->video_display_h != 0)
			showrect.display_h = mgnt->video_display_h;

		v_transmit_param_t vparaminfo = {0};
		vparaminfo.rdev = remote_dev;
		vparaminfo.rport = VIDEO_PORT;
		vparaminfo.rport_sub = VIDEO_PORT_SUB;
		vparaminfo.srcdev = mgnt->local_dev;

		vparaminfo.trandirection = VIDEO_TRAN_SEND_AND_RECV;
		//如果是门口机，则只接收，不发送
		if(remote_dev >> 24 == 3){
			vparaminfo.trandirection = VIDEO_TRAN_RECV;
		}else if (remote_dev >> 24 == 1){
			vparaminfo.trandirection = VIDEO_TRAN_SEND;
		}

		vparaminfo.prect = &showrect;
		vparaminfo.isdisablevideocache = mgnt->is_disable_video_cache;
		vparaminfo.isdisablevdecshow = mgnt->is_disable_decode_show_video;
		vparaminfo.videoType = videotype;

		vparaminfo.dropcb = video_trans_drop_cb;
		vparaminfo.pdropuserdata = (void*)mgnt;
        if(!mgnt->ptran_video1)
		{
			mgnt->trans_video_dev1 = remote_dev;
         	mgnt->ptran_video1 = plc_video_trans_start(&vparaminfo);
			if(mgnt->enc_status_cb){
				mgnt->enc_status_cb(mgnt->enc_status_usrdata, 1);
			}
		}
		else if (remote_dev >> 24 == 3 && !mgnt->ptran_video2)
		{
			mgnt->trans_video_dev2 = remote_dev;
         	mgnt->ptran_video2 = plc_video_trans_start(&vparaminfo);
		}

    }
    else
    {
		if(mgnt->ptran_video1)
		{
			plc_video_trans_stop(mgnt->ptran_video1);
			mgnt->ptran_video1 = NULL;

			bstop = true;
		}

		if(mgnt->ptran_video2)
		{
			plc_video_trans_stop(mgnt->ptran_video2);
			mgnt->ptran_video2 = NULL;
			bstop = true;
		}
    }
	pthread_mutex_unlock(&mgnt->video_mutex);
	if(mgnt->enc_status_cb && bstop){
				mgnt->enc_status_cb(mgnt->enc_status_usrdata, 0);
			}
    return 0;
}

static int session_audio_tran(void *param, bool onoff, uint32_t remote_dev)
{
	int ret = 0;
    if(!param)
        return ret;
    goblin_plc_call_mgnt_s *mgnt = (goblin_plc_call_mgnt_s*)param;
	pthread_mutex_lock(&mgnt->audio_mutex);
	LOGI("Audio tran onoff:%d  tran:%p  playvolume:%d\n", onoff, mgnt->ptran_audio, mgnt->audio_play_volume);
    if(onoff)
    {
        if(!mgnt->ptran_audio)
		{
            mgnt->ptran_audio = PlcAudioTrans_Start(AUDIO_PORT, remote_dev, AUDIO_PORT, AUDIO_TRAN_SEND_AND_RECV, mgnt->audio_play_volume);
			ret = 1;
		}
    }
    else
    {
        if(mgnt->ptran_audio)
        {
            PlcAudioTrans_Stop(mgnt->ptran_audio);
            mgnt->ptran_audio = NULL;
        }
    }
	pthread_mutex_unlock(&mgnt->audio_mutex);
    return ret;
}

static void* session_info_add(void *param, call_session_s *addinfo)
{
    void *pret = NULL;
    goblin_plc_call_mgnt_s *mgnt = (goblin_plc_call_mgnt_s*)param;

    call_session_s *p = NULL;
	call_session_s *n = NULL;

	pthread_mutex_lock(&mgnt->session_mutex);

    //轮询当前列表是否存在对应设备呼叫，如果存在就返回当前的， 没有就创建的
	if (!list_empty_careful(&mgnt->session_list))
	{
		list_for_each_entry_safe(p, n, &mgnt->session_list, linked_node)
		{
			if (p->remote_dev == addinfo->remote_dev)
			{
				pret = (void*)p;
				break;
			}
		}
	}
    if(!pret)
    {
        call_session_s *pinfo = (call_session_s *)bmalloc(sizeof(call_session_s));
        if (pinfo)
        {
            memset(pinfo, 0, sizeof(call_session_s));
            memcpy(pinfo, addinfo, sizeof(call_session_s));
            pinfo->psession = (void*)pinfo;
			pinfo->stick = time_relative_ms();
			pinfo->heartbeat_send_tick = time_relative_ms();
			pinfo->heartbeat_recv_tick = time_relative_ms();
            list_add_tail(&pinfo->linked_node, &mgnt->session_list);
            mgnt->session_count++;
            pret = (void*)pinfo;
        }
        else
            LOGE("alloc msg_info_t fail \n");
    }

	pthread_mutex_unlock(&mgnt->session_mutex);

	return pret;
}

static uint32_t session_info_del_by_session_or_dev(void *param, void*psession, uint32_t dev)
{
    goblin_plc_call_mgnt_s *mgnt = (goblin_plc_call_mgnt_s*)param;
	uint32_t ret = 0;

	call_session_s *p = NULL;
	call_session_s *n = NULL;

	pthread_mutex_lock(&mgnt->session_mutex);

	if (!list_empty_careful(&mgnt->session_list))
	{
		list_for_each_entry_safe(p, n, &mgnt->session_list, linked_node)
		{
			if (psession == p || p->remote_dev == dev)
			{
				list_del(&p->linked_node);
                mgnt->session_count--;
				if(p->is_audio_tran){
					session_audio_tran(mgnt, false, p->remote_dev);
				}

				//对端是门口机，挂断就删除视频传输
				if((p->remote_dev >> 24) == 3)
					stop_video_trans_by_remotedev(mgnt, p->remote_dev);
                bfree(p);
                ret = mgnt->session_count;
				break;
			}
		}
	}
	pthread_mutex_unlock(&mgnt->session_mutex);

	LOGI("Del session list count:%d \n", ret);
	return ret;
}

static int session_info_find_by_session(void *param, void*psession, call_session_s *pinfo, bool bchangestate, uint8_t state)
{
    goblin_plc_call_mgnt_s *mgnt = (goblin_plc_call_mgnt_s*)param;
	int ret = 0;

	call_session_s *p = NULL;
	call_session_s *n = NULL;

	pthread_mutex_lock(&mgnt->session_mutex);

	if (!list_empty_careful(&mgnt->session_list))
	{
		list_for_each_entry_safe(p, n, &mgnt->session_list, linked_node)
		{
			if (psession == p )
			{
				if(bchangestate)
				{
					p->stick = time_relative_ms();
					p->status = state;
				}
                if(pinfo)
				    memcpy(pinfo, p, sizeof(call_session_s));
                ret = 1;
				break;
			}
		}
	}
	pthread_mutex_unlock(&mgnt->session_mutex);

	return ret;
}

static int session_info_find_by_dev(void *param, uint32_t dev, call_session_s *pinfo, bool brefreshheat)
{
    goblin_plc_call_mgnt_s *mgnt = (goblin_plc_call_mgnt_s*)param;
	int ret = 0;

	call_session_s *p = NULL;
	call_session_s *n = NULL;

	pthread_mutex_lock(&mgnt->session_mutex);

	if (!list_empty_careful(&mgnt->session_list))
	{
		list_for_each_entry_safe(p, n, &mgnt->session_list, linked_node)
		{
			if (dev == p->remote_dev)
			{
				if(brefreshheat)
				{
					p->heartbeat_recv_tick = time_relative_ms();
				}
                if(pinfo)
				    memcpy(pinfo, p, sizeof(call_session_s));
                ret = 1;
				break;
			}
		}
	}
	pthread_mutex_unlock(&mgnt->session_mutex);

	return ret;
}

static int session_audiotran_state_change_by_dev(void *param, uint32_t dev, uint8_t audio_tran)
{
    goblin_plc_call_mgnt_s *mgnt = (goblin_plc_call_mgnt_s*)param;
	int ret = 0;

	call_session_s *p = NULL;
	call_session_s *n = NULL;

	pthread_mutex_lock(&mgnt->session_mutex);

	if (!list_empty_careful(&mgnt->session_list))
	{
		list_for_each_entry_safe(p, n, &mgnt->session_list, linked_node)
		{
			if (dev == p->remote_dev)
			{
				LOGI("Dev:%x audio_tran:%d\n", dev, audio_tran);
				p->is_audio_tran = audio_tran;
                ret = 1;
				break;
			}
		}
	}
	pthread_mutex_unlock(&mgnt->session_mutex);

	return ret;
}

//PLC传过来的数据
static void on_recv_plc_call_msg(void *param, uint32_t dev, uint8_t *data, uint16_t data_size)
{
	goblin_plc_call_mgnt_s *mgnt = (goblin_plc_call_mgnt_s *)param;

	call_msg_t *msg = (call_msg_t *)data;

	call_event_t event = {0};

	event.type = CALL_EVENT;

	memcpy(&event.call, msg, sizeof(call_msg_t));

	LOGI("recv call message from dev [0x%x]  cmd [%s] \n", msg->dev, call_cmd_to_str(msg->event));
    //printf("#### mgnt:%p mgnt->call_queue:%p \n", mgnt, mgnt->call_queue);
	mq_send(mgnt->call_queue, &event, sizeof(call_event_t));
}

static void process_call_event(goblin_plc_call_mgnt_s *mgnt, call_msg_t *msg)
{
    bool bcb = true;
	bool bhangupack = false;
    plc_call_info_s cbinfo = {0};
	switch (msg->event)
	{
		case CMD_HEARTBEAT:
			{
				call_session_s sessioninfo = {0};
				if(session_info_find_by_dev(mgnt, msg->dev, &sessioninfo, true) == 0)
                {
					LOGW("heartbeat from dev [0x%x]  is not exist \n", msg->dev);
				}
				//不上报心跳包， 由下层发送心跳包
				bcb = false;
			}
			break;
		case CMD_CALL:
            {
				call_session_s sessioninfo = {0};
				if(session_info_find_by_dev(mgnt, msg->dev, &sessioninfo, true) == 0)
				{

					sessioninfo.remote_dev = msg->dev;
					sessioninfo.stick = time_relative_ms();
					sessioninfo.status = CALL_STATUS_CALLINTG;
					//加入呼叫列表
					cbinfo.psession = session_info_add(g_plc_call_mgnt, &sessioninfo);

					cJSON *proot = cJSON_Parse((char*)msg->data);
					if(proot)
					{
						cJSON *pvideo = cJSON_GetObjectItem(proot, "video");
						if(pvideo)
						{
							cJSON *pwidth = cJSON_GetObjectItem(pvideo, "width");
							cJSON *pheight = cJSON_GetObjectItem(pvideo, "height");
							if(pwidth && cJSON_IsNumber(pwidth) && pheight && cJSON_IsNumber(pheight))
							{
								cbinfo.eventinfo.callin.video_width = pwidth->valueint;
								cbinfo.eventinfo.callin.video_height = pheight->valueint;
							}
						}
						cJSON_Delete(proot);
					}
					else
						LOGW("json parse fail \n");
				}
				else{
					//已经存在， 不上报
					bcb = false;
				}

                cbinfo.type = PLC_CALL_STATE_CALLIN;
                cbinfo.dev = msg->dev;

				cbinfo.eventinfo.callin.video_type = msg->param;
            }
			break;

		case CMD_CALL_ACK:
            {
                call_session_s sessioninfo = {0};
                //查找当前通话是否存在
                if(session_info_find_by_dev(mgnt, msg->dev, &sessioninfo, true) == 1)
                {
                    cbinfo.psession = sessioninfo.psession;
                    cbinfo.type = PLC_CALL_STATE_CALLOUT_ACK;
                    cbinfo.dev = msg->dev;

                    cbinfo.eventinfo.callout_ack.state = msg->param;

					session_info_find_by_session(g_plc_call_mgnt, sessioninfo.psession, &sessioninfo, true, CALL_STATUS_RINGING);

					cJSON *proot = cJSON_Parse((char*)msg->data);
					if(proot)
					{
						cJSON *pvideo = cJSON_GetObjectItem(proot, "video");
						if(pvideo)
						{
							cJSON *pwidth = cJSON_GetObjectItem(pvideo, "width");
							cJSON *pheight = cJSON_GetObjectItem(pvideo, "height");
							if(pwidth && cJSON_IsNumber(pwidth) && pheight && cJSON_IsNumber(pheight))
							{
								cbinfo.eventinfo.callout_ack.video_width = pwidth->valueint;
								cbinfo.eventinfo.callout_ack.video_height = pheight->valueint;
							}
						}
						cJSON_Delete(proot);
					}
					else
						LOGW("json parse fail \n");
                }
                else
                    bcb = false;
            }
			//process_cmd_call_ack(mgnt, msg);
			break;

		case CMD_ACCEPT:
			{
                call_session_s sessioninfo = {0};
                //查找当前通话是否存在
                if(session_info_find_by_dev(mgnt, msg->dev, &sessioninfo, true) == 1)
                {
                    cbinfo.psession = sessioninfo.psession;
                    cbinfo.type = PLC_CALL_STATE_ACCEPT;
                    cbinfo.dev = msg->dev;

					//对端是小门口机就不先开音频， 等室内机控制
					if((sessioninfo.remote_dev >> 24) != 3)
					{
                    	if(session_audio_tran(g_plc_call_mgnt, true, sessioninfo.remote_dev)==1)
							session_audiotran_state_change_by_dev(g_plc_call_mgnt, sessioninfo.remote_dev, 1);
					}

					//session_video_tran(g_plc_call_mgnt, true, sessioninfo.remote_dev);
                }
                else
                    bcb = false;
            }
			break;

		case CMD_ACCEPT_ACK:
			//process_cmd_accept_ack(mgnt, msg);
			break;

		case CMD_HANGUP:
            {
                call_session_s sessioninfo = {0};
                //查找当前通话是否存在
                if(session_info_find_by_dev(mgnt, msg->dev, &sessioninfo, false) == 1)
                {
                    cbinfo.psession = sessioninfo.psession;
                    cbinfo.type = PLC_CALL_STATE_HUNGUP;
                    cbinfo.dev = msg->dev;
					if(mgnt->session_count==1)
						session_video_tran(g_plc_call_mgnt, false, sessioninfo.remote_dev, 0);
                    cbinfo.eventinfo.hungup.reason = msg->param;
                    if(session_info_del_by_session_or_dev(mgnt, NULL, msg->dev)==0)
                    {
						session_video_tran(g_plc_call_mgnt, false, sessioninfo.remote_dev, 0);
                        session_audio_tran(g_plc_call_mgnt, false, sessioninfo.remote_dev);
                    }
					//send_call_msg(mgnt, sessioninfo.remote_dev, CMD_HANGUP_ACK, 0);

					bhangupack = true;
                }
                else
                    bcb = false;
            }
			//process_cmd_hangup(mgnt, msg);
			break;

		case CMD_HANGUP_ACK:
			{
				call_session_s sessioninfo = {0};
				if(session_info_find_by_dev(mgnt, msg->dev, &sessioninfo,false) == 1)
                {
                    cbinfo.psession = sessioninfo.psession;
                    cbinfo.type = PLC_CALL_STATE_HANGUP_ACK;
                    cbinfo.dev = msg->dev;

                    cbinfo.eventinfo.hungup.reason = 0;
                    if(session_info_del_by_session_or_dev(mgnt, NULL, msg->dev)==0)
                    {
                        session_audio_tran(g_plc_call_mgnt, false, sessioninfo.remote_dev);
						session_video_tran(g_plc_call_mgnt, false, sessioninfo.remote_dev, 0);
                    }
                }
                else{
					//bcb = false;
					cbinfo.type = PLC_CALL_STATE_HANGUP_ACK;
                    cbinfo.dev = msg->dev;
				}
			}
			//process_cmd_hangup_ack(mgnt, msg);
			break;
		case CMD_REQ_IKEY:
			{
				call_session_s sessioninfo = {0};
                //查找当前通话是否存在
                if(session_info_find_by_dev(mgnt, msg->dev, &sessioninfo, true) == 1)
                {
                    cbinfo.psession = sessioninfo.psession;
                    cbinfo.type = PLC_CALL_STATE_REQUEST_I;
                    cbinfo.dev = msg->dev;
                }
                else
                    bcb = false;
			}
			break;
		case CMD_SWITCH_VIEW:
			{
				call_session_s sessioninfo = {0};
                //查找当前通话是否存在
                if(session_info_find_by_dev(mgnt, msg->dev, &sessioninfo, true) == 1)
                {
                    cbinfo.psession = sessioninfo.psession;
                    cbinfo.type = PLC_CALL_STATE_REQUEST_VIEW;
                    cbinfo.dev = msg->dev;

					cbinfo.eventinfo.request_view.iscrop = msg->iscrop;
                }
                else
                    bcb = false;
			}
			break;
		case CMD_SWITCH_AUDIO:
			{
				call_session_s sessioninfo = {0};
                //查找当前通话是否存在, 底下切换音频不上报上层
                if(session_info_find_by_dev(mgnt, msg->dev, &sessioninfo, true) == 1)
                {
					session_audio_tran(mgnt, msg->isauido, msg->dev);
                }
				//底下切换音频不上报上层
                bcb = false;
			}
			break;
        default:
            bcb = false;
            break;
	}

    if(bcb && mgnt->handfuncCB)
    {
        mgnt->handfuncCB(mgnt->handfuncParam, &cbinfo);
    }

	if(bhangupack)
	{
		send_call_msg(mgnt, msg->dev, CMD_HANGUP_ACK, 0);
	}
}
static void process_timeout_event(goblin_plc_call_mgnt_s *mgnt)
{
    call_session_s *p = NULL;
	call_session_s *n = NULL;
	bool bhungup = false;
	//是否执行回调
	bool bcallback = false;
	//回调参数
	plc_call_info_s cbinfo = {0};
    pthread_mutex_lock(&mgnt->session_mutex);

	if (!list_empty_careful(&mgnt->session_list))
	{
		//printf("msg_check_thread list_empty_careful g_msg_list_count：%d\n", g_msg_list_count);
        uint32_t ctick = time_relative_ms();
		list_for_each_entry_safe(p, n, &mgnt->session_list, linked_node)
		{
			// printf("<%u>msg_check_thread p->remote_dev:%x losttime:%u hearbeatlost:%u - %u p->status:%d p->psession:%p\n", ctick,
			// 	p->remote_dev, ctick-p->stick, ctick - p->heartbeat_send_tick, ctick - p->heartbeat_recv_tick, p->status, p->psession);
			//心跳超时
			if (ctick - p->heartbeat_send_tick >= HEARTBEAT_INTERVAL)
			{
				p->heartbeat_send_tick = ctick;
				send_call_msg(g_plc_call_mgnt, p->remote_dev, CMD_HEARTBEAT, 0);
			}
			if(ctick - p->heartbeat_recv_tick >= HEARTBEAT_TIMEOUT)
			{
				//回调上层，通知对端断开
				cbinfo.psession = p->psession;
				cbinfo.type = PLC_CALL_STATE_DISCONNECT;
                cbinfo.dev = p->remote_dev;

				list_del(&p->linked_node);
				mgnt->session_count--;
				if(p->is_audio_tran){
					session_audio_tran(mgnt, false, p->remote_dev);
				}
				//如果对端是门口机， 则关闭视频传输
				if((p->remote_dev >> 24) == 3)
					stop_video_trans_by_remotedev(mgnt, p->remote_dev);
				bfree(p);
				bhungup = true;
				LOGW("call_session_s heartbeat timeout dev:%x losttime:%u session_count:%d\n", p->remote_dev, ctick - p->heartbeat_recv_tick, mgnt->session_count);

#if 0
				//多个设备同时触发会导致 只回调一次
				bcallback = true;
#else
				if(mgnt->handfuncCB)
					mgnt->handfuncCB(mgnt->handfuncParam, &cbinfo);
#endif
			}
			else if(CALL_STATUS_HANGING == p->status)
			{
				//主动挂断后， 一秒内没有收到对端的回应， 认为对端挂断， 主动释放资源
				if(ctick - p->stick > 1000)
				{
					list_del(&p->linked_node);
					mgnt->session_count--;
					if(p->is_audio_tran){
						session_audio_tran(mgnt, false, p->remote_dev);
					}
					bfree(p);
					bhungup = true;
					LOGW("call_session_s hangup timeout dev:%x losttime:%u \n", p->remote_dev, ctick-p->stick);
				}
			}
			else if(CALL_STATUS_CALLINTG == p->status)
			{
				//呼叫没有收到回复，在超时时间内重新呼叫
				if(ctick - p->stick > 1500)
				{
					LOGE("call_session_s callintg timeout dev:%x losttime:%u \n", p->remote_dev, ctick-p->stick);
					p->stick = ctick;
					send_call_msg(g_plc_call_mgnt, p->remote_dev, CMD_CALL, p->remote_video_type);
				}
			}
		}
	}

	pthread_mutex_unlock(&mgnt->session_mutex);

	//主动挂断后，不存在会话就关闭传输
	if(bhungup && mgnt->session_count == 0)
	{
		session_audio_tran(g_plc_call_mgnt, false, 0);
		session_video_tran(g_plc_call_mgnt, false, 0, 0);
	}

	//等待如果存在传输结束再回调(防止用户层在音视频回调时，资源已经释放)
	if(bcallback && mgnt->handfuncCB)
		mgnt->handfuncCB(mgnt->handfuncParam, &cbinfo);
}

static void *call_process_thread(void *args)
{
	goblin_plc_call_mgnt_s *mgnt = (goblin_plc_call_mgnt_s *)args;

	LOGI("spi_plc_call_thread start [%p] call_queue:[%p]\n", mgnt, mgnt->call_queue);

	call_event_t event = {0};

	while (mgnt->isrun)
	{
		if (0 == mq_recv(mgnt->call_queue, &event, sizeof(call_event_t), 100))
		{
			LOGD("recv event type [%d] mgnt [%p]\n", event.type, mgnt);

			if (event.type == CALL_EVENT)
			{
				process_call_event(mgnt, &event.call);
			}
			else if (event.type == PARAM_EVENT)
			{
				//process_param_event(mgnt, &event.button);
			}

            memset(&event, 0, sizeof(call_event_t));
		}

		process_timeout_event(mgnt);
	}

	return NULL;
}

DPLAN_PUBLIC_API int goblin_call_init(Call_Info_S *param)
{
    int ret = -1;

    if (g_plc_call_mgnt == NULL && param)
	{
		g_plc_call_mgnt = bmalloc(sizeof(goblin_plc_call_mgnt_s));

		if (g_plc_call_mgnt == NULL)
		{
			LOGE("malloc goblin_plc_call_mgnt_s fail \n");
			return -1;
		}

		memset(g_plc_call_mgnt, 0, sizeof(goblin_plc_call_mgnt_s));
        //1.
		INIT_LIST_HEAD(&g_plc_call_mgnt->session_list);
        //2.
		pthread_mutex_init(&g_plc_call_mgnt->session_mutex, NULL);
		pthread_mutex_init(&g_plc_call_mgnt->video_mutex, NULL);
		pthread_mutex_init(&g_plc_call_mgnt->audio_mutex, NULL);

		g_plc_call_mgnt->local_dev = param->dev_plc;
		g_plc_call_mgnt->isdoor = (param->dev_plc>>24)!=1 ? 1 : 0;
        g_plc_call_mgnt->handfuncCB = param->handfunc;
		g_plc_call_mgnt->handfuncParam = param->pusrdata;
		g_plc_call_mgnt->video_display_x = param->display_x;
		g_plc_call_mgnt->video_display_y = param->display_y;
		g_plc_call_mgnt->video_display_w = param->display_w;
		g_plc_call_mgnt->video_display_h = param->display_h;
		g_plc_call_mgnt->audio_play_volume = param->audio_play_volume;

		g_plc_call_mgnt->enc_status_cb = param->pvideo_enc_status_cb;
		g_plc_call_mgnt->enc_status_usrdata = param->pvideo_enc_status_usrdata;

		g_plc_call_mgnt->is_disable_video_cache = param->is_disable_video_cache;
		g_plc_call_mgnt->is_disable_decode_show_video = param->is_disable_video_decshow;

		g_plc_call_mgnt->m_pvideo_drop_status_cb = param->pvideo_drop_status_cb;
		g_plc_call_mgnt->m_pvideo_drop_status_usrdata = param->pvideo_drop_status_usrdata;
        //3.
		g_plc_call_mgnt->call_queue = mq_create("call_queue", sizeof(call_event_t), MAX_CALL_EVENT);
        //4.
		ret = spi_plc_mgnt_register_callback(CALL_PORT, on_recv_plc_call_msg, g_plc_call_mgnt);
        //5.
        g_plc_call_mgnt->isrun = 1;
		pthread_create(&g_plc_call_mgnt->thread, NULL, call_process_thread, g_plc_call_mgnt);
	}

    return ret;
}

DPLAN_PUBLIC_API int goblin_call_deinit(void)
{
    int ret = -1;

    if (g_plc_call_mgnt)
    {
        //1.
        spi_plc_mgnt_unregister_callback(CALL_PORT);
        g_plc_call_mgnt->isrun = 0;
        //2.
        pthread_join(g_plc_call_mgnt->thread, NULL);
        //3.
        mq_destroy(g_plc_call_mgnt->call_queue);

        //4.清空线程队列缓存
		call_session_s *node = NULL, *next = NULL;
		pthread_mutex_lock(&g_plc_call_mgnt->session_mutex);
		list_for_each_entry_safe(node, next, &g_plc_call_mgnt->session_list, linked_node)
		{
			list_del(&node->linked_node);
			bfree(node);
		}
		pthread_mutex_unlock(&g_plc_call_mgnt->session_mutex);
        //5.
        pthread_mutex_destroy(&g_plc_call_mgnt->session_mutex);
		pthread_mutex_destroy(&g_plc_call_mgnt->audio_mutex);
		pthread_mutex_destroy(&g_plc_call_mgnt->video_mutex);
		bfree(g_plc_call_mgnt);
        g_plc_call_mgnt = NULL;
        ret = 0;
    }


    return ret;
}

DPLAN_PUBLIC_API  int goblin_call_reload_devID(u_int32_t devid)
{
    int ret = -1;
	if (g_plc_call_mgnt)
	{
		g_plc_call_mgnt->local_dev = devid;
		ret = 0;
	}

	return ret;
}

DPLAN_PUBLIC_API int goblin_call_reload_AudioVolume(u_int32_t volume)
{
    int ret = -1;
	if (g_plc_call_mgnt)
	{
		LOGI("volume:%d \n", volume);
		g_plc_call_mgnt->audio_play_volume = volume;
		ret = 0;
	}

	return ret;
}

DPLAN_PUBLIC_API void *goblin_call_by_dev_ex(u_int32_t dev, uint8_t video_type)
{
    void *pret = NULL;
	if (g_plc_call_mgnt && g_plc_call_mgnt->call_queue)
	{
        call_session_s sessioninfo = {0};
        sessioninfo.remote_dev = dev;
        sessioninfo.stick = time_relative_ms();
        sessioninfo.status = CALL_STATUS_CALLINTG;
		sessioninfo.is_callout = 1;
		sessioninfo.remote_video_type = video_type;
        //加入呼叫列表
        pret = session_info_add(g_plc_call_mgnt, &sessioninfo);
        //呼出
        send_call_msg(g_plc_call_mgnt, dev, CMD_CALL, video_type);
		session_video_tran(g_plc_call_mgnt, true, dev, video_type);
	}

	return pret;
}

DPLAN_PUBLIC_API void *goblin_call_by_dev(u_int32_t dev)
{
    return goblin_call_by_dev_ex(dev, 0);
}


DPLAN_PUBLIC_API int goblin_call_ack_state(void *psession, int state)
{
    int ret = 0;
	if (g_plc_call_mgnt && g_plc_call_mgnt->call_queue)
	{
        call_session_s sessioninfo = {0};
        //查找呼叫列表
        if(session_info_find_by_session(g_plc_call_mgnt, psession, &sessioninfo, true, CALL_STATUS_RINGING))
        {
            LOGI("find session dev:%x \n", sessioninfo.remote_dev);
            send_call_msg(g_plc_call_mgnt, sessioninfo.remote_dev, CMD_CALL_ACK, state);
        }
        else
            LOGW("Can not find session:%p \n", psession);
	}

	return ret;
}

DPLAN_PUBLIC_API int goblin_call_accept(void *psession, uint8_t baudiotran)
{
    int ret = 0;
	if (g_plc_call_mgnt && g_plc_call_mgnt->call_queue)
	{
        call_session_s sessioninfo = {0};
        //查找呼叫列表
        if(session_info_find_by_session(g_plc_call_mgnt, psession, &sessioninfo, true, CALL_STATUS_TALKING))
        {
            LOGI("find session dev:%x baudiotran:%d\n", sessioninfo.remote_dev, baudiotran);
            send_call_msg(g_plc_call_mgnt, sessioninfo.remote_dev, CMD_ACCEPT, 0);

			if(baudiotran)
			{
            	if(session_audio_tran(g_plc_call_mgnt, true, sessioninfo.remote_dev)==1)
					session_audiotran_state_change_by_dev(g_plc_call_mgnt, sessioninfo.remote_dev, 1);
			}
        }
        else
            LOGW("Can not find session:%p \n", psession);
	}

	return ret;
}

DPLAN_PUBLIC_API int goblin_hangup_by_sessionAndReason(void *psession, uint8_t reason)
{
    int ret = -1;
	if (g_plc_call_mgnt && g_plc_call_mgnt->call_queue)
	{
        call_session_s sessioninfo = {0};
        //查找呼叫列表
        if(session_info_find_by_session(g_plc_call_mgnt, psession, &sessioninfo, true, CALL_STATUS_HANGING))
        {
            //发出挂断信令，等待对端回复
            send_call_msg(g_plc_call_mgnt, sessioninfo.remote_dev, CMD_HANGUP, reason);
            //删除呼叫列表,不存在就关闭传输
            if(session_info_del_by_session_or_dev(g_plc_call_mgnt, NULL, sessioninfo.remote_dev)==0 ||
				(!g_plc_call_mgnt->isdoor && sessioninfo.is_callout && (sessioninfo.remote_dev>>24) == 3 && stop_video_trans_by_remotedev(g_plc_call_mgnt, sessioninfo.remote_dev)==0))
            {
                session_audio_tran(g_plc_call_mgnt, false, sessioninfo.remote_dev);
				session_video_tran(g_plc_call_mgnt, false, sessioninfo.remote_dev, 0);
            }

            ret = 0;
        }
        else
            LOGW("Can not find session:%p \n", psession);
	}

	return ret;
}

DPLAN_PUBLIC_API int goblin_hangup_by_session(void *psession)
{
	return goblin_hangup_by_sessionAndReason(psession, 0);
}

DPLAN_PUBLIC_API int goblin_call_request_ikey_by_session(void *psession)
{
    int ret = 0;
	if (g_plc_call_mgnt && g_plc_call_mgnt->call_queue)
	{
        call_session_s sessioninfo = {0};
        //查找呼叫列表
        if(session_info_find_by_session(g_plc_call_mgnt, psession, &sessioninfo, false, 0))
        {
            LOGI("find session dev:%x \n", sessioninfo.remote_dev);
            send_call_msg(g_plc_call_mgnt, sessioninfo.remote_dev, CMD_REQ_IKEY, 0);

        }
        else
            LOGW("Can not find session:%p \n", psession);
	}

	return ret;
}

DPLAN_PUBLIC_API int goblin_call_request_crop_by_session(void *psession, uint8_t iscrop)
{
    int ret = 0;
	if (g_plc_call_mgnt && g_plc_call_mgnt->call_queue)
	{
        call_session_s sessioninfo = {0};
        //查找呼叫列表
        if(session_info_find_by_session(g_plc_call_mgnt, psession, &sessioninfo, false, 0))
        {
            LOGI("find session dev:%x \n", sessioninfo.remote_dev);
            //send_call_msg(g_plc_call_mgnt, sessioninfo.remote_dev, CMD_REQ_IKEY, );
			call_msg_t msg = {0};

			msg.event = CMD_SWITCH_VIEW;
			msg.dev = g_plc_call_mgnt->local_dev;
			msg.iscrop = iscrop;

			spi_plc_mgnt_send(sessioninfo.remote_dev, CALL_PORT, (uint8_t *)&msg, sizeof(call_msg_t));
        }
        else
            LOGW("Can not find session:%p \n", psession);
	}

	return ret;
}


DPLAN_PUBLIC_API int goblin_tran_auido_by_session(void *psession, uint8_t onoff)
{
    int ret = -1;
	if (g_plc_call_mgnt && g_plc_call_mgnt->call_queue)
	{
        call_session_s sessioninfo = {0};
        //查找呼叫列表
        if(session_info_find_by_session(g_plc_call_mgnt, psession, &sessioninfo, false, 0))
        {
            LOGI("find session dev:%x \n", sessioninfo.remote_dev);
            //send_call_msg(g_plc_call_mgnt, sessioninfo.remote_dev, CMD_REQ_IKEY, );
			call_msg_t msg = {0};

			msg.event = CMD_SWITCH_AUDIO;
			msg.dev = g_plc_call_mgnt->local_dev;
			msg.isauido = onoff;

			session_audio_tran(g_plc_call_mgnt, onoff, sessioninfo.remote_dev);
			spi_plc_mgnt_send(sessioninfo.remote_dev, CALL_PORT, (uint8_t *)&msg, sizeof(call_msg_t));
        }
        else
            LOGW("Can not find session:%p \n", psession);
	}

	return ret;
}

DPLAN_PUBLIC_API int goblin_put_video_frame(int channel,void *pdata,int dlen)
{
	if (g_plc_call_mgnt)
	{
		pthread_mutex_lock(&g_plc_call_mgnt->video_mutex);
		if(g_plc_call_mgnt->ptran_video1)
			plc_video_trans_send(g_plc_call_mgnt->ptran_video1, channel,  pdata, dlen);
		pthread_mutex_unlock(&g_plc_call_mgnt->video_mutex);
	}
	return 0;
}

DPLAN_PUBLIC_API int goblin_set_volume_by_session(void *psession, int volume)
{
    int ret = -1;
	if (g_plc_call_mgnt && g_plc_call_mgnt->call_queue)
	{
        call_session_s sessioninfo = {0};
        //查找呼叫列表
        if(session_info_find_by_session(g_plc_call_mgnt, psession, &sessioninfo, false, 0))
        {
            PlcAudioTrans_SetAoVolume(g_plc_call_mgnt->ptran_audio, volume);

            ret = 0;
        }
        else
            LOGW("Can not find session:%p \n", psession);
	}

	return ret;
}

DPLAN_PUBLIC_API int goblin_set_video_displayarea_by_session(void *psession, int x, int y, int w, int h)
{
    int ret = -1;
	if (g_plc_call_mgnt && g_plc_call_mgnt->call_queue)
	{
        call_session_s sessioninfo = {0};
        //查找呼叫列表
        if(session_info_find_by_session(g_plc_call_mgnt, psession, &sessioninfo, false, 0))
        {
			v_show_rct_t displayrect = {x, y, w, h};
			pthread_mutex_lock(&g_plc_call_mgnt->video_mutex);
			if(g_plc_call_mgnt->ptran_video1)
            	plc_video_trans_set_displayarea(g_plc_call_mgnt->ptran_video1, &displayrect);
			pthread_mutex_unlock(&g_plc_call_mgnt->video_mutex);
            ret = 0;
        }
        else
            LOGW("Can not find session:%p \n", psession);
	}

	return ret;
}

int plc_audio_frame_cb_set(void*param, TransAudioCB paocb, void *pusraodata, TransAudioCB paicb, void *pusraidata);
int plc_video_frame_cb_set(void*param, RemoteVideoCB pcb, void *pusrdata);
int plc_video_sub_frame_cb_set(void*param, RemoteVideoCB pcb, void *pusrdata);
DPLAN_PUBLIC_API int goblin_set_audio_cb_by_session(void *psession, TransAudioCB paocb, void *pusraodata, TransAudioCB paicb, void *pusraidata)
{
    int ret = -1;
	if (g_plc_call_mgnt)
	{
        call_session_s sessioninfo = {0};
        //查找呼叫列表,
        if(session_info_find_by_session(g_plc_call_mgnt, psession, &sessioninfo, false, 0))
        {
			if(g_plc_call_mgnt->ptran_audio)
            	plc_audio_frame_cb_set(g_plc_call_mgnt->ptran_audio, paocb, pusraodata, paicb, pusraidata);

            ret = 0;
        }
        else
            LOGW("Can not find session:%p \n", psession);
	}

	return ret;
}


DPLAN_PUBLIC_API int goblin_set_video_cb_by_session(void *psession, RemoteVideoCB pcb, void *pusrdata)
{
    int ret = -1;
	if (g_plc_call_mgnt)
	{
        call_session_s sessioninfo = {0};
        //查找呼叫列表
        if(session_info_find_by_session(g_plc_call_mgnt, psession, &sessioninfo, false, 0))
        {
			pthread_mutex_lock(&g_plc_call_mgnt->video_mutex);
			if(g_plc_call_mgnt->ptran_video1 && g_plc_call_mgnt->trans_video_dev1 == sessioninfo.remote_dev)
            	plc_video_frame_cb_set(g_plc_call_mgnt->ptran_video1, pcb, pusrdata);
			else if(g_plc_call_mgnt->ptran_video2 && g_plc_call_mgnt->trans_video_dev2 == sessioninfo.remote_dev)
            	plc_video_frame_cb_set(g_plc_call_mgnt->ptran_video2, pcb, pusrdata);
			pthread_mutex_unlock(&g_plc_call_mgnt->video_mutex);
            ret = 0;
        }
        else
            LOGW("Can not find session:%p \n", psession);
	}

	return ret;
}

DPLAN_PUBLIC_API int goblin_set_video_sub_cb_by_session(void *psession, RemoteVideoCB pcb, void *pusrdata)
{
    int ret = -1;
	if (g_plc_call_mgnt)
	{
        call_session_s sessioninfo = {0};
        //查找呼叫列表
        if(session_info_find_by_session(g_plc_call_mgnt, psession, &sessioninfo, false, 0))
        {
			pthread_mutex_lock(&g_plc_call_mgnt->video_mutex);
			if(g_plc_call_mgnt->ptran_video1 && g_plc_call_mgnt->trans_video_dev1 == sessioninfo.remote_dev)
				plc_video_sub_frame_cb_set(g_plc_call_mgnt->ptran_video1, pcb, pusrdata);
			else if(g_plc_call_mgnt->ptran_video2 && g_plc_call_mgnt->trans_video_dev2 == sessioninfo.remote_dev)
				plc_video_sub_frame_cb_set(g_plc_call_mgnt->ptran_video2, pcb, pusrdata);
			pthread_mutex_unlock(&g_plc_call_mgnt->video_mutex);
            ret = 0;
        }
        else
            LOGW("Can not find session:%p \n", psession);
	}

	return ret;
}

DPLAN_PUBLIC_API int goblin_set_videotype_trans_by_session(void *psession, uint8_t isonoff, uint8_t videotype)
{
    int ret = -1;
	if (g_plc_call_mgnt)
	{
        call_session_s sessioninfo = {0};
        //查找呼叫列表
        if(session_info_find_by_session(g_plc_call_mgnt, psession, &sessioninfo, false, 0))
        {
			session_video_tran(g_plc_call_mgnt, isonoff, sessioninfo.remote_dev, videotype);
            ret = 0;
        }
        else
            LOGW("Can not find session:%p \n", psession);
	}

	return ret;
}

DPLAN_PUBLIC_API int goblin_set_video_trans_by_session(void *psession, uint8_t isonoff)
{
	return goblin_set_videotype_trans_by_session(psession, isonoff, 0);
}

DPLAN_PUBLIC_API int goblin_get_jpgdata_by_session(void *psession, uint32_t width, uint32_t height, char **pjpgdata, uint32_t timeout)
{
    int ret = 0;
	if (g_plc_call_mgnt)
	{
        call_session_s sessioninfo = {0};
        //查找呼叫列表
        if(session_info_find_by_session(g_plc_call_mgnt, psession, &sessioninfo, false, 0))
        {
			pthread_mutex_lock(&g_plc_call_mgnt->video_mutex);
			if(g_plc_call_mgnt->ptran_video1)
            	ret = plc_video_enc_jpg(g_plc_call_mgnt->ptran_video1, width, height, pjpgdata, timeout);
			pthread_mutex_unlock(&g_plc_call_mgnt->video_mutex);
        }
        else
            LOGW("Can not find session:%p \n", psession);
	}

	return ret;
}

DPLAN_PUBLIC_API int goblin_set_ai_noise_by_session(void *psession, uint8_t isonoff)
{
    int ret = -1;
	if (g_plc_call_mgnt)
	{
        call_session_s sessioninfo = {0};
        //查找呼叫列表
        if(session_info_find_by_session(g_plc_call_mgnt, psession, &sessioninfo, false, 0))
        {
			pthread_mutex_lock(&g_plc_call_mgnt->audio_mutex);
			Audio_Trans_Ai_Noise_onoff(g_plc_call_mgnt->ptran_audio, isonoff);
			pthread_mutex_unlock(&g_plc_call_mgnt->audio_mutex);
            ret = 0;
        }
        else
            LOGW("Can not find session:%p \n", psession);
	}

	return ret;
}