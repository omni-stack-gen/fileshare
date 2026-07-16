#include <spi_plc/spi_plc_call.h>
#include <spi_plc/spi_plc_mgnt.h>
#include <spi_plc/spi_plc_trans.h>

#include <utils/bmem.h>
#if 0
#include <utils/util.h>
#include <utils/list.h>
#include <utils/mq.h>
// #include <utils/device_sn.h>

#include <utils/circlebuf.h>
#include <utils/time_helper.h>
// #include <utils/thread_helper.h>
#include <utils/wave_file_fmt.h>

// #include <db/db.h>

// #include <AudioCore.h>

// #include <sound/sound_play.h>
#include "../DPDef.h"

#include "g711common.h"

#define LOG_TAG "spi_plc_call"
#include <utils/log.h>

#define MAX_CALL_EVENT          (32)

#define CALL_USER_S             1   // 被叫方
#define CALL_USER_M             2   // 呼叫方

#define LOOP_DELAY              200
#define MSG_SEND_TMOUT          1000
#define MSG_TYR_TIME            (MSG_SEND_TMOUT / LOOP_DELAY)

#define CALL_TIMEOUT            (30)
#define TALK_TIMEOUT            (120)

#define MAX_SESSION             (16)

// #define DEBUG_AUDIO

#define BIT_GET(value, bit)      (!!((1 << (bit)) & (value)))   /**< 获取value的第bit位的值 */
#define BIT_SET(value, bit)      ((1 << (bit)) | (value))       /**< 设置value的第bit位的值 */
#define BIT_CLR(value, bit)      ((~(1 << (bit))) & (value))    /**< 清除value的第bit位的值 */

//////////////////////////////////////////////////////////////////////////////

#define GET_CALL_CMD(event)  ((uint8_t)((event) & 0xF))

#define GET_CALL_TYPE(event)  ((uint8_t)((event) >> 4))

#define MK_CALL_EVENT(cmd, type) \
	(((uint8_t)(type) << 4) | ((uint8_t)(cmd) & 0xF))

// PLC呼叫指令应在100ms左右发送一个，防止发送太多，PLC设备会直接丢弃

#define SLICE_PER_PKT   12
#define SAMPLERATE      8000    // 默认采样率（赫兹）
#define BLOCKTIME       20      // 每个缓冲数据块的时长（毫秒）
// #define PLAYBLOCKS      12      // 播放最大缓冲数据块数
// #define RECORDBLOCKS    12      // 录音最大缓冲数据块数
#define PLAYBLOCKS      4      // 播放最大缓冲数据块数
#define RECORDBLOCKS    4      // 录音最大缓冲数据块数
#define SAMPLEBITS      16
#define SAMPLECHN       1
#define BLOCKSIZE       (SAMPLERATE * BLOCKTIME * SAMPLECHN * (SAMPLEBITS / 8) / 1000)    // 320

#define MAX_CACHE_AUDIO_SIZE (SAMPLERATE * SAMPLECHN * (SAMPLEBITS / 8))

typedef struct audio_slice
{
	uint32_t index;              // 0
	char data[BLOCKSIZE / 2];    // 4
} audio_slice_t;                 // 164

typedef struct audio_packet
{
	uint32_t max_id;
	audio_slice_t slice[SLICE_PER_PKT];
} audio_packet_t;

typedef struct audio_stream
{
	uint32_t last_play_index;

	/* Capture trans stream */
	int ca_stream;

	// pthread_mutex_t ca_mutex;
	// struct circlebuf ca_packet;

	uint8_t *ca_buf;
	int ca_buf_size;

	/* Playback trans stream */

	pthread_mutex_t pb_mutex;
	struct circlebuf pb_packet;
	short pcm_buffer[BLOCKSIZE / 2];

	int pb_stream;
	int pb_volume;
	int padding_count;

	pthread_t ca_thread;
	volatile bool quit;

#ifdef DEBUG_AUDIO
	FILE *fp;
	wave_hdr_t wave_hdr;
	long data_pos;
#endif
} audio_stream_t;

// 呼叫命令
enum
{
	CMD_CALL = 0,
	CMD_CALL_ACK,
	CMD_ACCEPT,
	CMD_ACCEPT_ACK,
	CMD_HANGUP,
	CMD_HANGUP_ACK,
} __attribute__((packed));

typedef uint8_t call_cmd_t;

// 呼叫类型
enum
{
	CALL_TYPE_INLINE = 0,     // 四方通话呼入
	CALL_TYPE_ALARM,          // 报警呼叫
	CALL_TYPE_BROADCAST       // 管理机广播
} __attribute__((packed));

typedef uint8_t call_type_t;

// 呼叫优先级
enum
{
	CALL_LEVEL_NONE = 0,      // 无呼叫
	CALL_LEVEL_LOW,           // 四方通话, 管理机广播
	CALL_LEVEL_MIDDLE,        // 暂无
	CALL_LEVEL_HIGH,          // 报警呼叫
} __attribute__((packed));

typedef uint8_t call_level_t;

// 呼叫状态
enum
{
	CALL_STATUS_IDLE = 0,           // 空闲状态, 可长时间存在
	CALL_STATUS_CALLINTG,           // 呼叫状态，只能在MSG_TYR_TIME时间段内存在
	CALL_STATUS_RINGING,            // 振铃状态, 可长时间存在
	CALL_STATUS_ACCEPTING,          // 接听状态，只能在MSG_TYR_TIME时间段内存在
	CALL_STATUS_TALKING,            // 通话状态, 可长时间存在
	CALL_STATUS_HANGING,            // 挂断状态，只能在MSG_TYR_TIME时间段内存在
} __attribute__((packed));

typedef uint8_t call_status_t;

/**
  *
  */
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
	// 2) 呼叫的类型，呼叫的优先级应是 外网呼叫 > 局域网呼叫 > 外网广播
	//      0、局域网呼叫
	//      1、外网呼叫
	//      2、外网广播，当处于外网广播时，无需进入振铃状态，接听即可
	uint8_t event;

	// 设备PLC编号，主要用于区分PLC设备，
	uint32_t dev;

	// 设备的sn号，主要用于外网呼叫，
	uint8_t sn[6];

	// 消息载体
	// 1) 呼叫命令，当设备位图使用，最高位是网关，[0:12] bit是子设备
	//
	uint8_t data[4];
} call_msg_t;

enum
{
	CALL_EVENT,
	BUTTON_EVENT
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

// 呼叫会话
typedef struct call_session
{
	struct list_head linked_node;
	// uint8_t dev;                   /* 对方的PLC设备号 */

	// 呼叫等级
	call_level_t call_level;

	// 暂时先用老周的看看效果
	call_status_t cur_status;

	// 当前状态起始时间
	uint64_t status_start_time;

	uint8_t cur_role;
	uint8_t try_times;

	// 对方设备
	uint8_t remote_dev[MAX_SESSION];
	uint8_t remote_sn[MAX_SESSION][6];
	uint8_t remote_count;
} call_session_t;

typedef struct line_session
{
	// 下面两个参数用于标识一个PLC会话
	uint8_t remote_dev;
	uint8_t remote_sn[6];

	call_status_t status;           // 该线路的呼叫阶段
} line_session_t;

typedef struct call_session2
{
	line_session_t l_session[MAX_SESSION];      // 当前通话线路信息
	uint32_t line_count;                        // 当前通话线路的个数
	call_status_t status;                       // 该线路的呼叫阶段
	// DWORD m_callid;                          // 当前通话的id
	// DWORD c_stage;                           // 当前通话的阶段
	// DWORD m_remotetype;                      // 对方设备类型
	// BOOL m_bMonitor;                         //
} call_session2_t;

typedef struct spi_plc_call_mgnt
{
	// uint8_t cur_role;

	// 本机设备号
	uint8_t local_dev;

	// 本机sn码
	//uint8_t local_sn[MAX_SIZE_OF_SN];

	// 呼叫等级
	call_level_t level;

	// 呼叫状态
	call_status_t status;

	uint64_t status_end_time;                // 当前呼叫状态的超时时间

	// uint8_t try_times;
	// uint8_t remote_dev[MAX_SESSION];
	// uint8_t remote_count;

	// 呼叫设备的位图
	uint16_t call_map;

	// 先用之前老周写的
	call_session_t session;

	audio_stream_t *stream;

	pthread_mutex_t session_mutex;
	struct list_head session_list;

	int ring_play_id;

	uint64_t last_time;

	mq_t *call_queue;
	pthread_t thread;
} spi_plc_call_mgnt_t;

static spi_plc_call_mgnt_t *g_call_mgnt = NULL;

static const char *call_cmd_to_str(call_cmd_t cmd)
{
	switch (cmd)
	{
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

		default :
			return "(unknown)";
	}
}

static const char *call_type_to_str(call_type_t type)
{
	switch (type)
	{
		case CALL_TYPE_INLINE:
			return "inline";

		case CALL_TYPE_ALARM:
			return "alarm";

		case CALL_TYPE_BROADCAST:
			return "broadcast";

		default :
			return "(unknown)";
	}
}

static const char *call_status_to_str(call_status_t status)
{
	switch (status)
	{
		case CALL_STATUS_IDLE:
			return "idle";

		case CALL_STATUS_CALLINTG:
			return "calling";

		case CALL_STATUS_RINGING:
			return "ringing";

		case CALL_STATUS_ACCEPTING:
			return "accepting";

		case CALL_STATUS_TALKING:
			return "talking";

		case CALL_STATUS_HANGING:
			return "hanguping";
	}

	return "(unknown)";
}

// static void call_status_change(spi_plc_call_mgnt_t *mgnt, call_status_t status)
// {
//  // 状态不同则更新状态和时间，状态相同仅更新时间

//  if (mgnt->status != status)
//  {
//      LOGI("call status stage [%s] >>> [%s] \n", call_status_to_str(mgnt->status), call_status_to_str(status));

//      mgnt->status = status;
//  }

//  // 主动呼叫
//  if (mgnt->status == CALL_STATUS_CALLINTG ||
//      mgnt->status == CALL_STATUS_RINGING)
//  {
//      mgnt->status_end_time = time_now() + (CALL_TIMEOUT * 1000);
//  }
// }

#if 1
#define call_status_change(a, b) _call_status_change(a, b, __FUNCTION__, __LINE__)

static void _call_status_change(call_session_t *session, call_status_t status, const char *func, const long line)
{
	// 状态不同则更新状态和时间，状态相同仅更新时间

	if (session->cur_status != status)
	{
		LOGI("[%s():%ld] call status stage [%s] >>> [%s] \n", func, line,
		     call_status_to_str(session->cur_status), call_status_to_str(status));

		session->cur_status = status;

		session->status_start_time = time_now();
	}

	// 主动呼叫
	//  if (session->cur_status == CALL_STATUS_CALLINTG ||
	//      session->cur_status == CALL_STATUS_RINGING)
	//  {
	//      mgnt->status_end_time = time_now() + (CALL_TIMEOUT * 1000);
	//  }
}
#else
static void call_status_change(call_session_t *session, call_status_t status)
{
	// 状态不同则更新状态和时间，状态相同仅更新时间

	if (session->cur_status != status)
	{
		LOGI("call status stage [%s] >>> [%s] \n", call_status_to_str(session->cur_status),
		     call_status_to_str(status));

		session->cur_status = status;

		session->status_start_time = time_now();
	}

	// 主动呼叫
	//  if (session->cur_status == CALL_STATUS_CALLINTG ||
	//      session->cur_status == CALL_STATUS_RINGING)
	//  {
	//      mgnt->status_end_time = time_now() + (CALL_TIMEOUT * 1000);
	//  }
}
#endif

static void add_call_session(spi_plc_call_mgnt_t *mgnt)
{
	// RETURN_IF_FAIL(session && Session_Context)

	// locker_lock(&Session_Context->SessionMutex);

	// list_add_tail(&session->list, &mgnt->session_list);

	// info("Add Session[%x] lid:%d rid:%d ip:%d.%d.%d.%d \n",
	//      (int)session, session->lid, session->rid,
	//      session->rip & 0xFF, (session->rip >> 8) & 0xFF,
	//      (session->rip >> 16) & 0xFF, (session->rip >> 24) & 0xFF);

	// locker_unlock(&Session_Context->SessionMutex);
}

static void stop_play_ring(spi_plc_call_mgnt_t *mgnt)
{
	if (mgnt->ring_play_id != 0)
	{
		//sound_play_stop(mgnt->ring_play_id);
		mgnt->ring_play_id = 0;
	}
}

static void start_play_ring(spi_plc_call_mgnt_t *mgnt)
{
	int volume = 0;

	// stop_play_ring(mgnt);

	// db_sys_param_get("speaker_volume_call_ring", &volume, sizeof(volume));

	// if (volume != 0)
	// {
	// 	mgnt->ring_play_id = sound_play_start(RING_MP3, PLAY_FOREVER, volume, false);
	// }
}

///////////////////////////////////////////////////////////////////////

static void on_recv_audio(void *param, uint32_t dev, uint8_t *data, uint16_t data_size)
{
	spi_plc_call_mgnt_t *mgnt = (spi_plc_call_mgnt_t *)param;

	uint32_t mod_index = 0;

	int i = 0;

	audio_packet_t *packet = (audio_packet_t *)data;

	if (packet->max_id < SLICE_PER_PKT)
	{
		mod_index = 0;
	}
	else
	{
		mod_index = packet->max_id - SLICE_PER_PKT;
	}

	// printf("%d %d %d \n", mgnt->stream->last_play_index, mod_index, packet->max_id);

	if (mgnt->stream->last_play_index < mod_index)
	{
		printf("chg id from %u to %u\r\n", mgnt->stream->last_play_index, mod_index);
		mgnt->stream->last_play_index = mod_index;
	}
	else if (mgnt->stream->last_play_index > packet->max_id)
	{
		printf("chg id from %u to %u\r\n", mgnt->stream->last_play_index, mod_index);
		mgnt->stream->last_play_index = mod_index;
	}

	for (; mgnt->stream->last_play_index < packet->max_id; mgnt->stream->last_play_index++)
	{
		mod_index = mgnt->stream->last_play_index % SLICE_PER_PKT;

		// printf("scan %u %u %u\r\n", mod_index, packet->slice[mod_index].index, mgnt->stream->last_play_index);

		if (packet->slice[mod_index].index == mgnt->stream->last_play_index)
		{
			// if (show_log & 2)
			// {
			// printf("aud out %u \n", packet->slice[mod_index].index);
			// }

			for (i = 0; i < BLOCKSIZE / 2; ++i)
			{
				mgnt->stream->pcm_buffer[i] = ulaw_to_s16(packet->slice[mod_index].data[i]);
			}

			pthread_mutex_lock(&mgnt->stream->pb_mutex);

			while (mgnt->stream->pb_packet.size >= MAX_CACHE_AUDIO_SIZE)
			{
				LOGW("drop audio packet size:%d \n", mgnt->stream->pb_packet.size);
				circlebuf_pop_front(&mgnt->stream->pb_packet, NULL, BLOCKSIZE);
			}

			// #ifdef DEBUG_AUDIO

			//          if (mgnt->stream->fp)
			//          {
			//              size_t iread = fread(packet->slice[mod_index].data, 1, BLOCKSIZE, mgnt->stream->fp);

			//              if (iread == 0)
			//              {
			//                  fseek(mgnt->stream->fp, mgnt->stream->data_pos, SEEK_SET);

			//                  iread = fread(packet->slice[mod_index].data, 1, BLOCKSIZE, mgnt->stream->fp);
			//              }
			//              else if (iread < BLOCKSIZE)
			//              {
			//                  memset(&packet->slice[mod_index].data[iread], 0, BLOCKSIZE - iread);
			//              }
			//          }

			// #endif

			circlebuf_push_back(&mgnt->stream->pb_packet, mgnt->stream->pcm_buffer, BLOCKSIZE);

			pthread_mutex_unlock(&mgnt->stream->pb_mutex);
		}
	}
}

static int playback_cb(char *DataBuffer, int DataSize, void *pContext)
{
	spi_plc_call_mgnt_t *mgnt = (spi_plc_call_mgnt_t *)pContext;

	int size = 0;

	// 前面80毫秒填充静音
	if (mgnt->stream->padding_count < PLAYBLOCKS)
	{
		memset(DataBuffer, 0, DataSize);

		mgnt->stream->padding_count++;

		size = DataSize;
	}
	else
	{
		pthread_mutex_lock(&mgnt->stream->pb_mutex);

		if (mgnt->stream->pb_packet.size >= DataSize)
		{
			circlebuf_pop_front(&mgnt->stream->pb_packet, DataBuffer, DataSize);

			size = DataSize;
		}
		else
		{
			// memset(DataBuffer, 0, DataSize);

			// LOGE("valid_len:%d need %d\n", mgnt->stream->pb_packet.size, DataSize);
		}

		pthread_mutex_unlock(&mgnt->stream->pb_mutex);
	}

	return size;
}

static void close_playback(spi_plc_call_mgnt_t *mgnt)
{
	if (mgnt->stream->pb_stream)
	{
		// Audio_Player_Stop(mgnt->stream->pb_stream);
		// Audio_Player_Close(mgnt->stream->pb_stream);

		mgnt->stream->pb_stream = 0;
	}
}

static bool open_playback(spi_plc_call_mgnt_t *mgnt)
{
	bool ret = false;
#if 0
	int StreamID = 0;
	int Channels = SAMPLECHN;
	int SampleBits = SAMPLEBITS;
	int SampleRate = SAMPLERATE;
	int Blocks = PLAYBLOCKS;
	int BlockTime = BLOCKTIME;
	int SamplesPerBlock = SampleRate * BlockTime / 1000;

	// 打开放音设备
	StreamID = Audio_Player_Open();

	CHECK_VAL_GOTO_IF_FAIL(StreamID, "Audio_Player_Open fail");

	// 设置放音参数
	ret = Audio_Player_SetDevice(StreamID, "default");

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_SetDevice fail");

	ret = Audio_Player_SetFormat(StreamID, Channels, SampleBits, SampleRate);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_SetFormat fail");

	ret = Audio_Player_SetCache(StreamID, Blocks, SamplesPerBlock);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_SetCache fail");

	ret = Audio_Player_SetMode(StreamID, false, playback_cb, mgnt);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_SetMode fail");

	ret = Audio_Player_SetVolume(StreamID, mgnt->stream->pb_volume);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_SetVolume fail");

	ret = Audio_Player_Start(StreamID);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_Start fail");

	mgnt->stream->pb_stream = StreamID;

	return true;

fail:

	if (StreamID != 0)
	{
		Audio_Player_Close(StreamID);
		StreamID = 0;
	}
#endif
	return false;
}

static void close_capture(spi_plc_call_mgnt_t *mgnt)
{
	if (mgnt->stream->ca_stream)
	{
		mgnt->stream->quit = true;

		if (mgnt->stream->ca_thread)
		{
			pthread_join(mgnt->stream->ca_thread, NULL);
			mgnt->stream->ca_thread = 0;
		}

		// Audio_Recorder_Stop(mgnt->stream->ca_stream);
		// Audio_Recorder_Close(mgnt->stream->ca_stream);

#ifdef DEBUG_AUDIO

		if (mgnt->stream->fp)
		{
			fclose(mgnt->stream->fp);
			mgnt->stream->fp = NULL;
		}

#endif

		mgnt->stream->ca_stream = 0;
	}
}

static void *audio_ca_thread(void *args)
{
	LOGI("audio_ca_thread start \n");

	spi_plc_call_mgnt_t *mgnt = (spi_plc_call_mgnt_t *)args;

	uint64_t last_send_time = 0, tp_now = 0;

	uint32_t cur_index = 0, mod_index = 0;

	int i = 0;

	short pcm_buffer[BLOCKSIZE / 2] = {0};
#if 0
	audio_packet_t *packet = (audio_packet_t *)bmalloc(sizeof(audio_packet_t));

	memset(packet, 0, sizeof(audio_packet_t));

	last_send_time = time_now();

	while (!mgnt->stream->quit)
	{
		mod_index = cur_index % SLICE_PER_PKT;

		if (Audio_Recorder_Read(mgnt->stream->ca_stream,
		                        (char *)pcm_buffer,
		                        BLOCKSIZE,
		                        BLOCKTIME * RECORDBLOCKS) == false)
		{
			LOGE("Audio_Recorder_Read FAILED !!!\n");
			continue;
		}

#ifdef DEBUG_AUDIO

		if (mgnt->stream->fp)
		{
			size_t iread = fread(pcm_buffer, 1, BLOCKSIZE, mgnt->stream->fp);

			if (iread == 0)
			{
				fseek(mgnt->stream->fp, mgnt->stream->data_pos, SEEK_SET);

				iread = fread(pcm_buffer, 1, BLOCKSIZE, mgnt->stream->fp);
			}
			else if (iread < BLOCKSIZE)
			{
				char *temp = (char *)pcm_buffer;

				memset(&temp[iread], 0, BLOCKSIZE - iread);
			}
		}

#endif

		// pcm转成ulaw
		for (i = 0; i < BLOCKSIZE / 2; ++i)
		{
			packet->slice[mod_index].data[i] = s16_to_ulaw(pcm_buffer[i]);
		}

		packet->slice[mod_index].index = cur_index++;

		tp_now = time_now();

		if ((tp_now - last_send_time) >= 40)
		{
			// 40毫秒发送一次数据包，plc基本是每秒发送40次包,不能发送太快
			packet->max_id = cur_index;

			// if(show_log & 2)

			// printf("aud in %d %d \r\n", cur_index, sizeof(audio_packet_t));

			// LOGI("send audio to remote dev:%d cur_index:%d \n", mgnt->session.remote_dev[0], cur_index);

			spi_plc_mgnt_send(mgnt->session.remote_dev[0], AUDIO_PORT, (uint8_t *)packet, sizeof(audio_packet_t));

			last_send_time = tp_now;
		}
	}

	bfree(packet);
#endif
	LOGI("audio_ca_thread end \n");

	return NULL;
}

static bool open_capture(spi_plc_call_mgnt_t *mgnt)
{
	bool ret = false;
#if 0
	int StreamID = 0;
	int Channels = SAMPLECHN;
	int SampleBits = SAMPLEBITS;
	int SampleRate = SAMPLERATE;
	int Blocks = RECORDBLOCKS;
	int BlockTime = BLOCKTIME;
	int SamplesPerBlock = SampleRate * BlockTime / 1000;
	int BlockSize = SamplesPerBlock * Channels * (SampleBits / 8);

	mgnt->stream->ca_buf_size = BlockSize;
	mgnt->stream->ca_buf = bmalloc(BlockSize);

	CHECK_VAL_GOTO_IF_FAIL(mgnt->stream->ca_buf, "malloc ca_buf fail");

	// 打开录音设备
	StreamID = Audio_Recorder_Open();

	CHECK_VAL_GOTO_IF_FAIL(StreamID, "Audio_Recorder_Open fail");

	// 设置录音参数
	ret = Audio_Recorder_SetDevice(StreamID, "default");

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_SetDevice fail");

	ret = Audio_Recorder_SetFormat(StreamID, Channels, SampleBits, SampleRate);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_SetFormat fail");

	ret = Audio_Recorder_SetCache(StreamID, Blocks, SamplesPerBlock);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_SetCache fail");

	ret = Audio_Recorder_SetMode(StreamID, true, NULL, NULL);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_SetMode fail");

	ret = Audio_Recorder_SetVolume(StreamID, 100);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_SetVolume fail");

	ret = Audio_Recorder_Start(StreamID);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_Start fail");

	mgnt->stream->ca_stream = StreamID;

#ifdef DEBUG_AUDIO

	mgnt->stream->fp = fopen("test.wav", "rb");

	if (mgnt->stream->fp)
	{
		if (!read_wave_info(&mgnt->stream->fp, &mgnt->stream->wave_hdr))
		{
			LOGE("read_wave_info fail \n");

			fclose(mgnt->stream->fp);
			mgnt->stream->fp = NULL;
		}
	}

#endif

	pthread_create(&mgnt->stream->ca_thread, NULL, audio_ca_thread, mgnt);

	return true;

fail:

	if (StreamID != 0)
	{
		Audio_Recorder_Close(StreamID);
		StreamID = 0;
	}

	if (mgnt->stream->ca_buf)
	{
		bfree(mgnt->stream->ca_buf);
		mgnt->stream->ca_buf = NULL;
	}
#endif
	return false;
}

static void audio_stream_start(spi_plc_call_mgnt_t *mgnt)
{
	if (mgnt->stream == NULL)
	{
		mgnt->stream = bmalloc(sizeof(audio_stream_t));

		memset(mgnt->stream, 0, sizeof(audio_stream_t));

		mgnt->stream->pb_volume = 100;

		pthread_mutex_init(&mgnt->stream->pb_mutex, NULL);

		circlebuf_init(&mgnt->stream->pb_packet);

		circlebuf_reserve(&mgnt->stream->pb_packet, MAX_CACHE_AUDIO_SIZE);

		open_capture(mgnt);

		open_playback(mgnt);

		//AudioCore_SetAecMode(true);

		spi_plc_mgnt_register_callback(AUDIO_PORT, on_recv_audio, mgnt);
	}
}

static void audio_stream_stop(spi_plc_call_mgnt_t *mgnt)
{
	if (mgnt->stream)
	{
		spi_plc_mgnt_unregister_callback(AUDIO_PORT);

		//AudioCore_SetAecMode(false);

		close_playback(mgnt);

		close_capture(mgnt);

		pthread_mutex_destroy(&mgnt->stream->pb_mutex);

		circlebuf_free(&mgnt->stream->pb_packet);

		bfree(mgnt->stream);
		mgnt->stream = NULL;
	}
}

///////////////////////////////////////

static void on_recv_call_msg(void *param, uint32_t dev, uint8_t *data, uint16_t data_size)
{
	spi_plc_call_mgnt_t *mgnt = (spi_plc_call_mgnt_t *)param;

	call_msg_t *msg = (call_msg_t *)data;

	call_event_t event = {0};

	event.type = CALL_EVENT;

	memcpy(&event.call, msg, sizeof(call_msg_t));

	LOGI("recv call message from dev [0x%x] sn [%02X-%02X-%02X-%02X-%02X-%02X] cmd [%s] type [%s] \n",
	     msg->dev, msg->sn[0], msg->sn[1], msg->sn[2], msg->sn[3], msg->sn[4], msg->sn[5],
	     call_cmd_to_str(GET_CALL_CMD(msg->event)),  call_type_to_str(GET_CALL_TYPE(msg->event)));

	mq_send(mgnt->call_queue, &event, sizeof(call_event_t));
}

static void send_call_msg(spi_plc_call_mgnt_t *mgnt, uint32_t dev, uint8_t event)
{
	call_msg_t msg = {0};

	msg.event = event;
	msg.dev = mgnt->local_dev;

	//memcpy(msg.sn, mgnt->local_sn, 6);

	if (msg.event == CMD_CALL)
	{
		memcpy(msg.data, &mgnt->call_map, 2);
	}

	spi_plc_mgnt_send(dev, CALL_PORT, (uint8_t *)&msg, sizeof(call_msg_t));

	LOGI("send call message to dev [0x%x] cmd [%s] type [%s] \n",
	     dev, call_cmd_to_str(GET_CALL_CMD(msg.event)),  call_type_to_str(GET_CALL_TYPE(msg.event)));
}

static void process_cmd_call(spi_plc_call_mgnt_t *mgnt, call_msg_t *msg)
{
	call_session_t *session = &mgnt->session;

	uint16_t call_map = msg->data[0] | (msg->data[1] << 8);

	bool match_map = false;

	// 收到对方呼叫消息
	switch (session->cur_status)
	{
		case CALL_STATUS_IDLE:  // 当前在空闲状态进入振铃状态
			{
				memset(session, 0, sizeof(call_session_t));

				// 检查呼叫设备位图是否有自己，没有自己无需应答，
				if (mgnt->local_dev == GATEWAY_DEV)
				{
					match_map = call_map & (GATEWAY_DEV << 8);
				}
				else
				{
					match_map = BIT_GET(call_map, mgnt->local_dev);
				}

				if (match_map)
				{
					session->remote_count = 1;
					session->remote_dev[0] = msg->dev;
					session->cur_role = CALL_USER_S;

					call_status_change(session, CALL_STATUS_RINGING);

					send_call_msg(mgnt, msg->dev, CMD_CALL_ACK);

					start_play_ring(mgnt);
				}
				else
				{
					LOGW("local dev [0x%x] no in call map [0x%x] \n", mgnt->local_dev, call_map);
				}
			}
			break;

		case CALL_STATUS_RINGING:  // 当前在振铃状态说明对方没有收到ACK
			{
				if (session->remote_dev[0] == msg->dev)
				{
					if (session->cur_role == CALL_USER_S)
					{
						// 检查呼叫设备位图是否有自己，没有自己无需应答，

						if (mgnt->local_dev == GATEWAY_DEV)
						{
							match_map = call_map & (GATEWAY_DEV << 8);
						}
						else
						{
							match_map = BIT_GET(call_map, mgnt->local_dev);
						}

						if (match_map)
						{
							send_call_msg(mgnt, msg->dev, CMD_CALL_ACK);
						}
					}
					else
					{
						LOGW("local dev [0x%x] no in call map [0x%x] \n", mgnt->local_dev, call_map);
					}
				}
			}
			break;

		default:
			// 其他状态不响应
			LOGW("cur_status [%s] cur_role [%d] no process call \n",
			     call_status_to_str(session->cur_status), session->cur_role);
			break;
	}
}

static void process_cmd_call_ack(spi_plc_call_mgnt_t *mgnt, call_msg_t *msg)
{
	uint8_t i;

	call_session_t *session = &mgnt->session;

	// 主叫端收到了呼叫ACK消息
	switch (session->cur_status)
	{
		case CALL_STATUS_CALLINTG:
		case CALL_STATUS_RINGING:
			{
				// 主呼方在呼叫/振铃状态下收到呼叫应答
				if (session->cur_role == CALL_USER_M)
				{
					// TODO：是否需要加一条busy指令

					// 将返回应答的设备号加入到列表中
					// 检测返回应答的设备号是否已经在列表中
					for (i = 0; i < session->remote_count; i++)
					{
						if (session->remote_dev[i] == msg->dev)
						{
							break;
						}
					}

					if (i == session->remote_count)
					{
						// 不在列表中就加入
						session->remote_dev[i] = msg->dev;
						session->remote_count++;

						if (msg->dev == GATEWAY_DEV)
						{
							mgnt->call_map &= ~(GATEWAY_DEV << 8);
						}
						else
						{
							mgnt->call_map = BIT_CLR(mgnt->call_map, msg->dev);
						}

						LOGI("add dev [0x%02x] to session list, total count [%d] call map:0x%x \n",
						     msg->dev, session->remote_count, mgnt->call_map);

						// 呼叫位图为0，表示所有被呼设备都已经应答，将呼叫状态修改为振铃状态
						if (mgnt->call_map == 0)
						{
							call_status_change(session, CALL_STATUS_RINGING);

							session->try_times = 0;
						}
					}
				}
				else
				{
					LOGW("cur_role [%d] no process call ack \n", session->cur_role);
				}
			}
			break;

		default:
			LOGW("cur_status [%s] cur_role [%d] no process call ack \n",
			     call_status_to_str(session->cur_status), session->cur_role);
			break;
	}
}

static void process_cmd_accept(spi_plc_call_mgnt_t *mgnt, call_msg_t *msg)
{
	uint8_t i;

	call_session_t *session = &mgnt->session;

	// 主叫端收到了接听消息
	switch (session->cur_status)
	{
		case CALL_STATUS_CALLINTG:
		case CALL_STATUS_RINGING:
			{
				// 检测对方是否在列表中
				for (i = 0; i < session->remote_count; i++)
				{
					if (session->remote_dev[i] == msg->dev)
					{
						break;
					}
				}

				if (i != session->remote_count)
				{
					// 对方在列表中
					call_status_change(session, CALL_STATUS_TALKING);

					send_call_msg(mgnt, msg->dev, CMD_ACCEPT_ACK);

					// 将其他几路都挂断
					for (i = 0; i < session->remote_count; i++)
					{
						if (session->remote_dev[i] != msg->dev)
						{
							send_call_msg(mgnt, session->remote_dev[i], CMD_HANGUP);
						}
					}

					// 仅保留当前通话的一路
					session->remote_count = 1;
					session->remote_dev[0] = msg->dev;

					stop_play_ring(mgnt);

					// 开启音频传输
					audio_stream_start(mgnt);
				}
			}
			break;

		case CALL_STATUS_TALKING:
			{
				if (msg->dev == session->remote_dev[0])
				{
					send_call_msg(mgnt, msg->dev, CMD_ACCEPT_ACK);
				}
				else
				{
					send_call_msg(mgnt, msg->dev, CMD_HANGUP);
				}
			}
			break;

		default:
			LOGW("cur_status [%s] cur_role [%d] no process call accept \n",
			     call_status_to_str(session->cur_status), session->cur_role);
			break;
	}
}

static void process_cmd_accept_ack(spi_plc_call_mgnt_t *mgnt, call_msg_t *msg)
{
	call_session_t *session = &mgnt->session;

	// 被叫端收到了接听ack消息
	switch (session->cur_status)
	{
		case CALL_STATUS_ACCEPTING:
			{
				if (msg->dev == session->remote_dev[0])
				{
					call_status_change(session, CALL_STATUS_TALKING);

					stop_play_ring(mgnt);

					// 开启音频传输
					audio_stream_start(mgnt);
				}
			}
			break;

		default:
			LOGW("cur_status [%s] cur_role [%d] no process call accept ack\n",
			     call_status_to_str(session->cur_status), session->cur_role);
			break;
	}
}

static void process_cmd_hangup(spi_plc_call_mgnt_t *mgnt, call_msg_t *msg)
{
	uint8_t i;

	call_session_t *session = &mgnt->session;

	// 收到挂断消息，都给对面回ack
	send_call_msg(mgnt, msg->dev, CMD_HANGUP_ACK);

	switch (session->cur_status)
	{
		case CALL_STATUS_CALLINTG:
		case CALL_STATUS_RINGING:
			{
				// 检测对方是否在列表中
				for (i = 0; i < session->remote_count; i++)
				{
					if (session->remote_dev[i] == msg->dev)
					{
						memmove(&session->remote_dev[i], &session->remote_dev[i + 1], session->remote_count - i - 1);
						session->remote_count--;
						break;
					}
				}

				if (session->remote_count == 0)
				{
					stop_play_ring(mgnt);

					call_status_change(session, CALL_STATUS_IDLE);

					memset(session, 0, sizeof(call_session_t));
				}
			}
			break;

		case CALL_STATUS_TALKING:
			{
				// 当前在通话状态，需关闭音频传输，
				// 通话中只存在一路会话，清空所有状态
				audio_stream_stop(mgnt);

				call_status_change(session, CALL_STATUS_IDLE);

				memset(session, 0, sizeof(call_session_t));
			}
			break;

		default:
			// stop_play_ring(mgnt);
			// send_call_msg(mgnt, msg->dev, CMD_HANGUP_ACK);
			// call_status_change(session, CALL_STATUS_IDLE);
			// memset(session, 0, sizeof(call_session_t));

			LOGW("cur_status [%s] cur_role [%d] no process call hangup\n",
			     call_status_to_str(session->cur_status), session->cur_role);
			break;
	}
}

static void process_cmd_hangup_ack(spi_plc_call_mgnt_t *mgnt, call_msg_t *msg)
{
	uint8_t i;

	call_session_t *session = &mgnt->session;

	// 收到了挂断消息的ack
	switch (session->cur_status)
	{
		case CALL_STATUS_HANGING:
			{
				for (i = 0; i < session->remote_count; i++)
				{
					if (session->remote_dev[i] == msg->dev)
					{
						memmove(&session->remote_dev[i], &session->remote_dev[i + 1], session->remote_count - i - 1);
						session->remote_count--;
						break;
					}
				}

				if (session->remote_count == 0)
				{
					call_status_change(session, CALL_STATUS_IDLE);

					memset(session, 0, sizeof(call_session_t));
				}
			}
			break;

		default:
			LOGW("cur_status [%s] cur_role [%d] no process call hangup ack\n",
			     call_status_to_str(session->cur_status), session->cur_role);
			break;
	}
}

static void process_call_event(spi_plc_call_mgnt_t *mgnt, call_msg_t *msg)
{
	switch (msg->event)
	{
		case CMD_CALL:
			process_cmd_call(mgnt, msg);
			break;

		case CMD_CALL_ACK:
			process_cmd_call_ack(mgnt, msg);
			break;

		case CMD_ACCEPT:
			process_cmd_accept(mgnt, msg);
			break;

		case CMD_ACCEPT_ACK:
			process_cmd_accept_ack(mgnt, msg);
			break;

		case CMD_HANGUP:
			process_cmd_hangup(mgnt, msg);
			break;

		case CMD_HANGUP_ACK:
			process_cmd_hangup_ack(mgnt, msg);
			break;
	}
}

static void process_button_event(spi_plc_call_mgnt_t *mgnt, button_msg_t *msg)
{
	uint8_t i;

	call_session_t *session = &mgnt->session;

	LOGE("cur call status [%s] \n", call_status_to_str(session->cur_status));

	switch (session->cur_status)
	{
		case CALL_STATUS_IDLE:
			{
				// 当前在空闲状态进入呼叫
				memset(session, 0, sizeof(call_session_t));

				// 短按报警，长按四方通话
				// if (button_state == SHORT_BUTTON_RELEASE)
				// {
				//  // 只呼叫网关，报警呼叫
				//  send_call_msg(GATEWAY_DEV, CMD_CALL); // 进行群呼
				// }
				// else
				// {
				//  send_call_msg(mgnt, BROADCAST_DEV, CMD_CALL); // 进行群呼
				// }

				// 网关在最高位，后面需要改成是呼叫哪一局的
				uint16_t sub_dev_map[4] =
				{
					0x07,                   /* bit[2:0]  1局 */
					0x38,                   /* bit[5:3]  2局 */
					0x1c0,                  /* bit[8:6]  3局 */
					0xe00,                  /* bit[11:9] 4局 */
				};

				uint8_t idx;

				// 网关在最高位，子设备根据设备型号以及设备属于第几局电梯生成位图
				if (mgnt->local_dev == GATEWAY_DEV)
				{
					// 后面需要改成根据按钮判断呼叫的是第几局的设备
					idx = msg->code;

					mgnt->call_map |= sub_dev_map[idx];
				}
				else
				{
					idx = mgnt->local_dev / 3;

					mgnt->call_map |= (GATEWAY_DEV << 8);
					mgnt->call_map |= sub_dev_map[idx];
					mgnt->call_map = BIT_CLR(mgnt->call_map, mgnt->local_dev);
				}

				{
					send_call_msg(mgnt, BROADCAST_DEV, CMD_CALL); // 进行群呼
				}

				session->cur_role = CALL_USER_M;

				call_status_change(session, CALL_STATUS_CALLINTG);

				start_play_ring(mgnt);
			}
			break;

		case CALL_STATUS_CALLINTG:
			{
				call_status_change(session, CALL_STATUS_HANGING);

				for (i = 0; i < session->remote_count; i++)
				{
					send_call_msg(mgnt, session->remote_dev[i], CMD_HANGUP);
				}

				session->try_times = 0;

				stop_play_ring(mgnt);
			}
			break;

		case CALL_STATUS_RINGING:
			{
				// 在振铃状态下
				if (session->cur_role == CALL_USER_S)
				{
					// 被叫方收到按键消息为接听
					call_status_change(session, CALL_STATUS_ACCEPTING);

					send_call_msg(mgnt, session->remote_dev[0], CMD_ACCEPT);
				}
				else
				{
					// 主叫方收到按键消息为挂断
					call_status_change(session, CALL_STATUS_HANGING);

					for (i = 0; i < session->remote_count; i++)
					{
						send_call_msg(mgnt, session->remote_dev[i], CMD_HANGUP);
					}
				}

				session->try_times = 0;

				stop_play_ring(mgnt);
			}
			break;

		case CALL_STATUS_TALKING:
			{
				// 在通话状态，进行挂断
				call_status_change(session, CALL_STATUS_HANGING);

				if (session->remote_count != 0)
				{
					send_call_msg(mgnt, session->remote_dev[0], CMD_HANGUP);
				}

				session->try_times = 0;

				// 关闭音频传输
				audio_stream_stop(mgnt);
			}
			break;

		// 在呼叫状态下是否需要挂断
		case CALL_STATUS_ACCEPTING:

		// 在等待状态下是否需要挂断
		case CALL_STATUS_HANGING:
			break;
	}
}

static bool call_status_time_expired(call_session_t *session)
{
	uint64_t tp_now = time_now();

	if (tp_now < session->status_start_time)
	{
		session->status_start_time = tp_now;
	}

	if (tp_now - session->status_start_time >= LOOP_DELAY)
	{
		session->status_start_time = tp_now;

		return true;
	}

	return false;
}

static void process_time_event(spi_plc_call_mgnt_t *mgnt)
{
	uint8_t i;

	call_session_t *session = &mgnt->session;

	// uint64_t tp_now = time_now();

	// if (tp_now < mgnt->last_time)
	// {
	//  mgnt->last_time = tp_now;
	// }

	// // if (tp_now - mgnt->last_time < LOOP_DELAY)
	// if (tp_now - mgnt->last_time < 100)
	// {
	//  return ;
	// }

	// mgnt->last_time = tp_now;

	// LOGE("cur_status:%s role:%d try_times:%d \n",
	//      call_status_to_str(session->cur_status), session->cur_role, session->try_times);

	// printf("st %d %d %d\r\n", cur_status, cur_role, try_times);
	switch (session->cur_status)
	{
		case CALL_STATUS_CALLINTG:  // 呼叫方会进入这个状态
			{
				if (call_status_time_expired(session))
				{
					// 在呼叫状态，再次发送呼叫消息
					if (session->try_times < MSG_TYR_TIME)
					{
						send_call_msg(mgnt, BROADCAST_DEV, CMD_CALL);

						session->try_times++;
					}
					else
					{
						// 处于呼叫状态下若无设备应答，结束该次通话
						if (session->remote_count == 0)
						{
							call_status_change(session, CALL_STATUS_IDLE);

							memset(session, 0, sizeof(call_session_t));

							stop_play_ring(mgnt);
						}
						else
						{
							LOGI("send call message retry end call map:%x total count [%d] in session list\n",
							     mgnt->call_map, session->remote_count);

							call_status_change(session, CALL_STATUS_RINGING);

							mgnt->call_map = 0;

							session->try_times = 0;
						}
					}
				}
			}

			break;

		case CALL_STATUS_HANGING:   // 双方都会进入这个状态
			{
				if (call_status_time_expired(session))
				{
					// 在挂断状态，再次发送挂断消息
					if (session->try_times < MSG_TYR_TIME)
					{
						for (i = 0; i < session->remote_count; i++)
						{
							send_call_msg(mgnt, session->remote_dev[i], CMD_HANGUP);
						}

						session->try_times++;
					}
					else
					{
						call_status_change(session, CALL_STATUS_IDLE);

						memset(session, 0, sizeof(call_session_t));
					}
				}
			}

			break;

		case CALL_STATUS_ACCEPTING: // 被叫方会进入这个状态
			{
				if (call_status_time_expired(session))
				{
					// 在接听状态，再次发送接听消息
					if (session->try_times < MSG_TYR_TIME)
					{
						send_call_msg(mgnt, session->remote_dev[0], CMD_ACCEPT);

						session->try_times++;
					}
					else
					{
						call_status_change(session, CALL_STATUS_IDLE);

						memset(session, 0, sizeof(call_session_t));
					}
				}
			}

			break;

		case CALL_STATUS_RINGING:
#if 0

			// 3秒自动接听，测试用
			if (session->cur_role == CALL_USER_S)
			{
				session->try_times++;

				if (session->try_times > 3)
				{
					call_status_change(session, CALL_STATUS_ACCEPTING);
					send_call_msg(mgnt, session->remote_dev[0], CMD_ACCEPT);
					session->try_times = 0;
				}
			}

#endif
			break;
	}
}

static void *spi_plc_call_thread(void *args)
{
	spi_plc_call_mgnt_t *mgnt = (spi_plc_call_mgnt_t *)args;

	LOGI("spi_plc_call_thread start [%p]\n", mgnt);

	call_event_t event;

	while (1)
	{
		if (0 == mq_recv(mgnt->call_queue, &event, sizeof(call_event_t), 100))
		{
			// LOGI("recv event type [%d] mgnt [%p]\n", event.type, mgnt);

			if (event.type == CALL_EVENT)
			{
				process_call_event(mgnt, &event.call);
			}
			else if (event.type == BUTTON_EVENT)
			{
				process_button_event(mgnt, &event.button);
			}
		}

		process_time_event(mgnt);
	}

	return NULL;
}

DPLAN_PUBLIC_API int spi_plc_call_init()
{
	int ret = 0;

	if (g_call_mgnt == NULL)
	{
		g_call_mgnt = bmalloc(sizeof(spi_plc_call_mgnt_t));

		if (g_call_mgnt == NULL)
		{
			LOGE("malloc spi_plc_call_mgnt_t fail \n");
			return -1;
		}

		memset(g_call_mgnt, 0, sizeof(spi_plc_call_mgnt_t));

		INIT_LIST_HEAD(&g_call_mgnt->session_list);

		pthread_mutex_init(&g_call_mgnt->session_mutex, NULL);

		//generate_device_sn(SN_FROM_FLASH_ID, g_call_mgnt->local_sn);

		g_call_mgnt->local_dev = GATEWAY_DEV;
		g_call_mgnt->call_queue = mq_create(sizeof(call_event_t), MAX_CALL_EVENT);

		ret = spi_plc_mgnt_register_callback(CALL_PORT, on_recv_call_msg, g_call_mgnt);

		ret = pthread_create(&g_call_mgnt->thread, NULL, spi_plc_call_thread, g_call_mgnt);
	}

	return ret;
}

int spi_plc_call_deinit()
{
	return 0;
}

int spi_plc_call_inline()
{
	if (g_call_mgnt && g_call_mgnt->call_queue)
	{
		call_event_t event = {0};

		event.type = BUTTON_EVENT;
		event.button.code = 0;

		return mq_send(g_call_mgnt->call_queue, &event, sizeof(call_event_t));
	}

	return -1;
}

int spi_plc_call_hangup()
{
	if (g_call_mgnt && g_call_mgnt->call_queue)
	{
		call_event_t event = {0};

		event.type = BUTTON_EVENT;
		event.button.code = 0;

		return mq_send(g_call_mgnt->call_queue, &event, sizeof(call_event_t));
	}

	return -1;
}

DPLAN_PUBLIC_API void DoOneCall(uint32_t dev_id)
{
	send_call_msg(g_call_mgnt, dev_id, CMD_CALL); // 进行群呼
}

#endif // SPI_PLC_CALL_ENABLE