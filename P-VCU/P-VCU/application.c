#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <utils/log.h>
#include "src/transport/trans.h"
#include "src/transport/tran_video.h"
#include "src/transport/spi_plc/spi_plc_trans.h"
#include "src/transport/event_plc/goblin_plc_process.h"
#include "src/transport/event_plc/goblin_plc_callcore.h"

#include <AudioCore.h>

#include <utils/util.h>
#include <utils/circlebuf.h>
#include <utils/list.h>
#include <utils/bmem.h>
#include <utils/time_helper.h>
#include <dpbase.h>
#include "media/video_encode.h"
#ifdef ENABLE_V1
#include <sound_play.h>
#endif
#ifdef ENABLE_NX5
#include <video/vd/VDecApi.h>
#include <video/vd/VoApi.h>
#endif

typedef struct video_play_list
{
	bool   thread_run;
	pthread_t play_thread;
	//sem_t  send_sem;				//spi发送信号
	pthread_mutex_t  play_mutex;

	bool bfirstkey;
	uint32_t playframe_count;
	struct list_head play_list;
} video_play_list_t;

typedef struct video_frame_
{
	struct list_head node;
	uint32_t datalen;
	uint8_t *pdata;
}video_frame_t;

#define LOCAL_LOCK() pthread_mutex_lock(&g_local_mutex)

#define LOCAL_UNLOCK() pthread_mutex_unlock(&g_local_mutex)

static pthread_mutex_t g_local_mutex = PTHREAD_MUTEX_INITIALIZER;

static void * m_ppushtrans = NULL;	//推流句柄

static video_play_list_t *g_pplayprocess = NULL;

static void *pmgnt_video = NULL;
void VideoTransTest(int onoff)
{
	LOCAL_LOCK();
	v_show_rct_t showrect = {0,0, 1024, 600};

	v_transmit_param_t vparaminfo = {0};
	vparaminfo.rdev = 0xFFFFFFFF;
	vparaminfo.rport = VIDEO_PORT;
	vparaminfo.trandirection = VIDEO_TRAN_SEND_AND_RECV;
	vparaminfo.prect = &showrect;

	if(onoff == 1 && pmgnt_video == NULL)
		pmgnt_video = plc_video_trans_start(&vparaminfo);
	else if (onoff == 0 && pmgnt_video!= NULL)
	{
		plc_video_trans_stop(pmgnt_video);
		pmgnt_video = NULL;
	}
	LOCAL_UNLOCK();
}

//推流发送函数
int mediatrans_send(int channel,media_frame * pframe,int dlen)
{
	goblin_put_video_frame(channel, (void *)pframe, dlen);
#if 0
	LOCAL_LOCK();
	if(pmgnt_video)
		plc_video_trans_send(pmgnt_video, channel, (void *)pframe, dlen);
	LOCAL_UNLOCK();

	static unsigned int diff = 0;
	static unsigned int curvideotick = 0;
	if(pframe->streamid == 0)
		curvideotick = pframe->timestamp;
	if(pframe->streamid == 1 && diff == 0)
	{
		if(curvideotick == 0)
			return 0;
		printf("media get diff.%u %u %u\r\n",curvideotick,pframe->timestamp,curvideotick - pframe->timestamp);
		diff = curvideotick - pframe->timestamp;
	}

	if(pframe->streamid == 1 && channel == 0)
		pframe->timestamp = pframe->timestamp + diff;


	// if(pframe->streamid == 0 && channel == 0)
	// 	DPInfo("video frame:%d %d\r\n",pframe->seq,pframe->timestamp);

	unsigned char streamid_bk = pframe->streamid;

LOCAL_LOCK();
	if(m_ppushtrans)
	{
		// 还原
		pframe->streamid = streamid_bk;
		pframe->streamid += channel*2;
		if(pframe->streamid <= 2)
		{
			// printf("transpush_send:%d %d %d\n",pframe->streamid,m_getMainStream,m_getSubStream);
			// if(pframe->streamid == 0 && m_getMainStream == 0)
			// 	return 0;
			// if(pframe->streamid == 2 && m_getSubStream == 0)
			// 	return 0;
			// printf("transpush_send id:%d seq:%d key:%d len:%d vlen:%d\n",pframe->streamid,pframe->seq,pframe->bkey,dlen,dlen-sizeof(media_frame));

			// 通话监视下不发主码流
			// if(pframe->streamid == 0 && m_getMainStream == 0)
			// 	return 0;

			// 空闲状态下不发
			// if (m_getMainStream == 0 && m_getSubStream == 0)
			// 	return 0;
#ifdef ENABLE_MEDIA
			transpush_send(m_ppushtrans,(unsigned char*)pframe,dlen,pframe->bkey);
#elif (ENABLE_V6)
			transpush_send(m_ppushtrans,(unsigned char*)pframe,dlen,pframe->bkey);
#endif
		}
	}
LOCAL_UNLOCK();
#endif
	return 0;
}



int mediatrans_init(void)
{
LOCAL_LOCK();
#ifdef ENABLE_MEDIA
	tran_push_info_s info = {0};
	memset(&info, 0, sizeof(tran_push_info_s));
	info.lport = 10101;
	info.rip = 0;
	info.rport = 0;
	info.plc_dev = 0;
	info.plc_port = 0;
	info.bplc_tran = false;
	info.bretrans = false;
    if(!m_ppushtrans)
        m_ppushtrans = transpush_create(&info);
#endif
LOCAL_UNLOCK();
    return 0;
}


int mediatrans_start(void)
{
LOCAL_LOCK();
	//设置推送目的地址开始推送媒体流
	if(m_ppushtrans)
	{
#ifdef ENABLE_MEDIA
        transpush_setremote(m_ppushtrans, 0x5901A8C0, 1234, 0xFFFFFFFF, VIDEO_PORT);
#endif
    }
LOCAL_UNLOCK();
    return 0;
}


//===============================video dec=================================
// 显示通道
#define DISP_CHANNEL 0
// 显示图层
#define DISP_LAYER 0

#define DEC_BUF_SIZE 256 * 1024

#ifdef ENABLE_NX5
// 屏幕显示宽度
static int DISP_W = 1024;
// 屏幕显示高度
static int DISP_H = 600;

// 屏幕显示位置X值
static int DISP_X = 0;
// 屏幕显示位置Y值
static int DISP_Y = 0;

void DecH264Display(VO_LAYER_INFO *prt) {
  // 4个图层，从优先级低使能，优先级0 < 1 < 2 < 3
  // 不允许不使能Channel 0，Layer 0，直接使用Channel 0，Layer 1


  VO_LAYER_INFO VoInfo;
  memset(&VoInfo, 0, sizeof(VO_LAYER_INFO));
  if (prt == NULL) {
    VoInfo.Rect.X = DISP_X;
    VoInfo.Rect.Y = DISP_Y;
    VoInfo.Rect.W = DISP_W;
    VoInfo.Rect.H = DISP_H;
    // VO_EnableChn(DISP_CHANNEL,
    // &VoInfo);等价于VO_EnableVideoLayer(DISP_CHANNEL, 0, &VoInfo);
    //VoInfo.Rotate = DEC_ROTATE;
    //DEC_ROTATE++;
    if (!VO_EnableChn(DISP_CHANNEL, &VoInfo))
      printf("VO_EnableChn Failed!!!\n");
  } else {
    memcpy(&VoInfo, prt, sizeof(VO_LAYER_INFO));
    VO_EnableVideoLayer(DISP_CHANNEL, 0, &VoInfo);
  }

}

int DecChannel = -1;

void DecAndShowVideoStart(void)
{
	 //  初始化解码器
	VDEC_Init();
	// 使能显示
	VO_Enable();

	//设置显示位置
	DecH264Display(NULL);


	VDEC_CHN_ATTR_S Attr;
	memset(&Attr, 0, sizeof(VDEC_CHN_ATTR_S));
	Attr.deType = VDEC_PAYLOAD_TYPE_H265;
	Attr.BufSize = 0x80000*4 ;  // 512KB
	Attr.u32PicWidth = 2560;//2560;     // 1280;
	Attr.u32PicHight = 1440;//1440;

	DecChannel = VDEC_CreateChn(&Attr);
    if (DecChannel >= 0) {
      printf("CreateChn DecChannel[%d] Successful\n", DecChannel);
    } else {
      printf("Create First DecChannel[%d] Failed!\n", DecChannel);
    }
}

bool Video_Decoder_PutFrame(int Channel, unsigned char *pdata, int length)
{
    if(!pdata || length > DEC_BUF_SIZE || length <= 0)
    {
        return false;
    }

    VDEC_STREAM_S Stream;
    memset(&Stream, 0, sizeof(VDEC_STREAM_S));

    VDEC_FRAME_S Frame;
    memset(&Frame, 0, sizeof(VDEC_FRAME_S));

    VDEC_CHN_STAT_S Stat;
    memset(&Stat, 0, sizeof(VDEC_CHN_STAT_S));

    Stream.pu8Addr = (unsigned char*)pdata;
    Stream.u32Len = length;
    Stream.u64PTS = -1;

    // 向解码器送一帧数据
    if(1 != VDEC_SendStream(Channel, &Stream))
    {
        LOGE("VDEC_SendStream fail \n");
        return false;
    }

    // 查询通道的状态
    if(1 != VDEC_Query(Channel, &Stat))
    {
        LOGE("VDEC_Query fail \n");
        return false;
    }

    bool bret = true;

    if(Stat.u32FrameNum > 0)
    {
        // 解码一帧数据
        int Ret = VDEC_StartRecvStream(Channel, 0, 0, 0);

        switch(Ret)
        {
            case VDEC_NO_BITSTREAM:
                {
                    LOGE("VDEC_NO_BITSTREAM \n");
                    bret = false;
                }
                break;

            case VDEC_KEYFRAME_DECODED:
            case VDEC_FRAME_DECODED:
                {
                    // 获取解码数据
                    if(VDEC_GetImage(Channel, &Frame))
                    {
                        if(!VO_SetZoomInWindow(DISP_CHANNEL, DISP_LAYER, &Frame.SrcInfo))
                        {
                            LOGE("VO_SetZoomInWindow Error! \n");
                            bret = false;
                        }

                        if(!VO_ChnShow(DISP_CHANNEL, DISP_LAYER, &Frame))
                        {
                            LOGE("VO_ChnShow Error! \n");
                            bret = false;
                        }

                        // 释放解码数据
                        if(!VDEC_ReleaseImage(Channel, &Frame))
                        {
                            LOGE("VDEC_ReleaseImage Error! \n");
                            bret = false;
                        }
                    }
                    else
                    {
                        LOGE("VDEC_GetImage Error! \n");
                        bret = false;
                    }
                }
                break;

            default:
                break;
        }
    }

    return bret;
}

bool Video_Decoder_SetDisplayArea(int Channel, int x, int y, int w, int h)
{
    if(Channel == -1)
        return false;

    VO_LAYER_RECT VoRect;
    memset(&VoRect, 0, sizeof(VO_LAYER_RECT));

    //显示区域
    VoRect.X = x;
    VoRect.Y = y;
    VoRect.W = w;
    VoRect.H = h;

    return VO_Resize(DISP_CHANNEL, DISP_LAYER, &VoRect) == 1;
}

void DecAndShowVideoStop(void)
{
	VO_DisableVideoLayer(DISP_CHANNEL, DISP_LAYER);

	VDEC_DestroyChn(DecChannel);
	DecChannel = -1;
	VDEC_DeInit();
}

#endif
static void *ptran = NULL;

static bool bwritehead = false;
static FILE *pfd1 = NULL;
static FILE *pfd2 = NULL;

void SetVideoPlayColorByParam(int Brightness, int Contrast, int Saturation)
{
#ifdef ENABLE_NX5
    VO_LAYER_CM CM = {0};

    CM.Brightness = Brightness==0 ? 0 : Brightness;           //取值范围-127 ~ 128
    CM.Contrast = Contrast==0 ? 0x10 : Contrast;        // 对比度   1~0x1f    默认值 0x10
    CM.HueCos = 32;  // 色度2，取值范围-32 ~ 32     默认值 0x20
    CM.Saturation = Saturation == 0 ? 0x200 : Saturation;   // 饱和度  0~ 0x7ff     默认值：0x200

    printf("22 亮度 %d:%x  对比度 %d:%x   饱和度 %d:%x  色度:%x\n", Brightness, CM.Brightness, Contrast, CM.Contrast, Saturation, CM.Saturation, CM.HueCos);
    VO_ChnSetCM(DISP_CHANNEL, DISP_LAYER, &CM);
#endif
}

static void recv_cb(void *p, unsigned char *lpdata, int dlen)
{
#ifdef ENABLE_NX5
	video_play_list_t *pplay = (video_play_list_t*)p;
	media_frame *pframe = (media_frame *)lpdata;
	//printf("++++++++++++= dlen:%d  streamid:%d\n", dlen, pframe->streamid);
	int framelen =  dlen-sizeof(media_frame);
#if 0
	if(pframe->streamid==0 && pfd1)
	{
		if(bwritehead)
			fwrite(&framelen, 1, sizeof(int), pfd1);
		fwrite(pframe->data, 1, framelen, pfd1);
#ifdef ENABLE_NX5
		//Video_Decoder_PutFrame(DecChannel, pframe->data, dlen-sizeof(media_frame));
#endif
	}
	else if(pframe->streamid==2)
#endif
	{
#if 1
		static uint32_t llt = 0;
		uint32_t ctick = time_relative_ms();
		if(pframe->data[4]==0x40)
		{
			printf("<%u>$$$$$$$$$ I key, len:%u lost time:%d \n", ctick, framelen, ctick-llt);
		}
		// else
		// 	printf("P key len:%u lost time:%d \n", framelen, ctick-llt);
		llt = ctick;
#endif
		if(pfd2)
		{
			if(bwritehead)
				fwrite(&framelen, 1, sizeof(int), pfd2);
			fwrite(pframe->data, 1, framelen, pfd2);
		}
#ifdef ENABLE_NX5
		if(pplay)
		{

			if(!pplay->bfirstkey && pframe->data[4]==0x40)
			{
				pplay->bfirstkey = true;
				printf("############# first key : %02x  %02x  %02x  %02x  %02x \n", pframe->data[0], pframe->data[1], pframe->data[2], pframe->data[3], pframe->data[4]);
			}
			if(pplay->bfirstkey)
			{
				video_frame_t *pvframe = (video_frame_t*)bmalloc(sizeof(video_frame_t)+framelen+1);
				memset(pvframe, 0, sizeof(video_frame_t)+framelen+1);
				pvframe->datalen = framelen;

				pvframe->pdata = (uint8_t*)(pvframe+1);
				memcpy(pvframe->pdata, pframe->data, pvframe->datalen);

				//printf(">>>>>>>> first key : %02x  %02x  %02x  %02x  %02x \n", pframe->data[0], pframe->data[1], pframe->data[2], pframe->data[3], pframe->data[4]);
				pthread_mutex_lock(&pplay->play_mutex);
				pplay->playframe_count++;
				list_add_tail(&pvframe->node, &pplay->play_list);
				pthread_mutex_unlock(&pplay->play_mutex);
			}
		}
		else
			Video_Decoder_PutFrame(DecChannel, (unsigned char*)pframe->data, dlen-sizeof(media_frame));
#endif
	}
#endif
}

static void *video_play_thread(void *args)
{
#ifdef ENABLE_NX5
	#define PLAY_FRAME_RATE (5)
	#define TIME_OUT_US		(65*1000+500)
	video_play_list_t *pplayvideo_process = (video_play_list_t *)args;

	LOGI("video_play_thread start [%p]\n", pplayvideo_process);
	uint64_t lasttick = 0;
	bool bfullcache = false;
#if PLAY_FRAME_RATE
	do
	{
		usleep(50*1000);
		pthread_mutex_lock(&pplayvideo_process->play_mutex);
		if (pplayvideo_process->playframe_count > PLAY_FRAME_RATE)
		{
			bfullcache = true;
		}
		pthread_mutex_unlock(&pplayvideo_process->play_mutex);
		if(bfullcache)
			break;
	} while (pplayvideo_process->thread_run);
#endif

	uint32_t curcount = 0;
	while (pplayvideo_process->thread_run)
	{
		//if (pplayvideo_process->playframe_count > PLAY_FRAME_RATE)
		{
			video_frame_t *p = NULL;
			video_frame_t *n = NULL;
			pthread_mutex_lock(&pplayvideo_process->play_mutex);
			//if (pplayvideo_process->playframe_count > PLAY_FRAME_RATE)
			if (!list_empty_careful(&pplayvideo_process->play_list))
			{
				list_for_each_entry_safe(p, n, &pplayvideo_process->play_list, node)
				{
					pplayvideo_process->playframe_count--;
					list_del(&p->node);
					break;
				}
			}
			curcount = pplayvideo_process->playframe_count;
			pthread_mutex_unlock(&pplayvideo_process->play_mutex);
			if(p)
			{
#ifdef ENABLE_NX5
				uint64_t ltick = time_relative_us();
#if PLAY_FRAME_RATE
				if(curcount <= PLAY_FRAME_RATE && (ltick-lasttick)< TIME_OUT_US)
				{
					//printf("#### sleep time:%d %d\n", ltick-lasttick,(120-(ltick-lasttick))*1000);
					if(curcount < PLAY_FRAME_RATE)
						usleep((TIME_OUT_US-(ltick-lasttick)+(PLAY_FRAME_RATE-curcount)*1000));
					else
						usleep((TIME_OUT_US-(ltick-lasttick)));
				}
				//printf("frame <%llu> c time:%llu lt:%llu count :%d \n", time_relative_us(), time_relative_us()-lasttick, time_relative_us()-ltick, curcount);
				ltick = time_relative_us();
#endif //#if PLAY_FRAME_RATE
				Video_Decoder_PutFrame(DecChannel, p->pdata, p->datalen);

				//printf("-- <%d> cost %d ms\n", cur_tick, cur_tick - ltick);
				//if(p->pdata[4] == 0x40)
				//	printf("play== losttime %u:%u ^ %02x %02x %02x %02x %02x\n", ltick-lasttick, time_relative_ms()-ltick, p->pdata[0], p->pdata[1], p->pdata[2], p->pdata[3], p->pdata[4]);
				lasttick = ltick;
#endif
				bfree(p);
			}
			else
				usleep(1*1000);
		}
		//else
			//usleep(1*1000);
	}
#endif
	return NULL;
}

int VideoRecv_start(bool bthreadplay)
{
	LOGI(">>>>>>>>>>>>>> start:%p bthreadplay:%d\n", ptran, bthreadplay);

	bwritehead = true;
	if(!pfd1)
	{
		if(bwritehead)
			pfd1 = fopen("./remote1_head.h265", "wb");
		else
			pfd1 = fopen("./remote1.h265", "wb");
	}

	if(!pfd2)
	{
		if(bwritehead)
			pfd2 = fopen("./remote2_head.h265", "wb");
		else
			pfd2 = fopen("./remote2.h265", "wb");
	}

	if(!ptran)
	{
		if(bthreadplay && !g_pplayprocess)
		{
			g_pplayprocess = bmalloc(sizeof(video_play_list_t));
			memset(g_pplayprocess, 0, sizeof(video_play_list_t));
			//1.初始化播放缓存队列
			INIT_LIST_HEAD(&g_pplayprocess->play_list);
			//2.创建播放队列锁
			pthread_mutex_init(&g_pplayprocess->play_mutex, NULL);

			//3.创建播放线程
			g_pplayprocess->thread_run = true;
			int ret = pthread_create(&g_pplayprocess->play_thread, NULL, video_play_thread, g_pplayprocess);
			if (ret != 0)
			{
				g_pplayprocess->thread_run = false;
				LOGE("pthread_create error:%d \n", ret);
			}
		}
#ifdef ENABLE_NX5
		DecAndShowVideoStart();
#endif
		ptran = transpull_create(VIDEO_PORT, 0, 1234, 0x0F01A8C0, recv_cb, g_pplayprocess, true);
		printf("tran create %s \n", ptran ? "success" : "fail");
	}
	else{
		printf("tran is already exit\n");
	}
	return 0;
}


int VideoRecv_stop(void)
{
	LOGI(">>>>>>>>>>>>>> stop:%p\n", ptran);
	if(ptran)
	{
		transpull_destroy(ptran);
		ptran = NULL;

	}

	if(g_pplayprocess)
	{
		//1.停止播放线程
		if (g_pplayprocess->thread_run)
		{
			g_pplayprocess->thread_run = false;
			pthread_join(g_pplayprocess->play_thread, NULL);
			g_pplayprocess->play_thread = 0;
		}

		//2.清空线程队列缓存
		video_frame_t *node = NULL, *next = NULL;
		pthread_mutex_lock(&g_pplayprocess->play_mutex);
		list_for_each_entry_safe(node, next, &g_pplayprocess->play_list, node)
		{
			list_del(&node->node);
			bfree(node);
		}
		pthread_mutex_unlock(&g_pplayprocess->play_mutex);
		//3.销毁播放队列锁
		pthread_mutex_destroy(&g_pplayprocess->play_mutex);

		bfree(g_pplayprocess);
		g_pplayprocess = NULL;
	}
#ifdef ENABLE_NX5
	DecAndShowVideoStop();
#endif
	if(pfd1)
	{
		fclose(pfd1);
		pfd1 = NULL;
	}

	if(pfd2)
	{
		fclose(pfd2);
		pfd2 = NULL;
	}
	LOGI(">>>>>>>>>>>>>> stop out\n");
	return 0;
}


void video_test(void)
{
#ifdef ENABLE_NX5
	DecAndShowVideoStart();

	int len = 0;
	FILE *pfd_v = fopen("./remote2_head.h265", "rb");
	printf("file lpd :%p \n", pfd_v);
	char buf[512*1024];
	int index = 0;
	if (pfd_v)
	{
		while (1)
		{
			if (fread(&len, 1, sizeof(int), pfd_v) <=0)
			{
				/* code */
				fseek(pfd_v, 0, SEEK_SET);
				continue;
			}
			//printf("len111 :%d \n", len);
			if (fread(buf, 1, len, pfd_v) <=0)
			{
				/* code */
				fseek(pfd_v, 0, SEEK_SET);
				continue;
			}

			index++;
			//printf("len :%d \n", len);
			Video_Decoder_PutFrame(DecChannel, (unsigned char *)buf, len);
			printf("dec index:%d \n", index);
			if(index > 200)
			{
				break;
			}
		}


		fclose(pfd_v);
	}

	DecAndShowVideoStop();
	printf("============= out v t ===================\n");
#endif
}

//====================Audio test==================================
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

static int AI_StreamID = 0;
//static int AO_StreamID = 0;
FILE *pfd_ai = NULL;

int  AudioInCbFunc(char *DataBuffer, int DataSize, void *pContext)
{

    //printf("AudioInCbFunc DataSize:%d\n", DataSize);
    if(pfd_ai)
        fwrite(DataBuffer, 1, DataSize, pfd_ai);
    return DataSize;
}

void Audio_test_Record_start(void)
{
	if(AI_StreamID)
	{
		return;
	}
    int Channels = SAMPLECHN;
	int SampleBits = SAMPLEBITS;
	int SampleRate = SAMPLERATE;
	int Blocks = RECORDBLOCKS;
	int BlockTime = BLOCKTIME;
	int SamplesPerBlock = SampleRate * BlockTime / 1000;
	//int BlockSize = SamplesPerBlock * Channels * (SampleBits / 8);

    // 打开录音设备
	AI_StreamID = Audio_Recorder_Open();

	CHECK_VAL_GOTO_IF_FAIL(AI_StreamID, "Audio_Recorder_Open fail");

	// 设置录音参数
	int ret = Audio_Recorder_SetDevice(AI_StreamID, "default");

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_SetDevice fail");

	ret = Audio_Recorder_SetFormat(AI_StreamID, Channels, SampleBits, SampleRate);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_SetFormat fail");

	ret = Audio_Recorder_SetCache(AI_StreamID, Blocks, SamplesPerBlock);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_SetCache fail");

    if(!pfd_ai)
    {
        pfd_ai = fopen("/tmp/local_ai.pcm", "wb");
    }

	ret = Audio_Recorder_SetMode(AI_StreamID, false, AudioInCbFunc, NULL);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_SetMode fail");

	AudioCore_SetRecordVolume(30);
	ret = Audio_Recorder_SetVolume(AI_StreamID, 100);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_SetVolume fail");

	ret = Audio_Recorder_Start(AI_StreamID);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_Start fail");
    return ;
fail:

	if (AI_StreamID != 0)
	{
		Audio_Recorder_Close(AI_StreamID);
		AI_StreamID = 0;
	}
}


void Audio_test_Record_stop(void)
{
    if (AI_StreamID != 0)
	{
        Audio_Recorder_Stop(AI_StreamID);
		Audio_Recorder_Close(AI_StreamID);
		AI_StreamID = 0;
	}

    if(pfd_ai)
    {
        fclose(pfd_ai);
        pfd_ai = NULL;
    }
}


typedef struct _mgnt_ao_t
{
    int stream_id;
    struct circlebuf pb_packet;
    pthread_mutex_t pb_mutex;
    pthread_t ca_thread;
	volatile bool quit;
} mgnt_ao_t;
static int playback_cb(char *DataBuffer, int DataSize, void *pContext)
{
	mgnt_ao_t *mgnt = (mgnt_ao_t *)pContext;

	int size = 0;

	{
		pthread_mutex_lock(&mgnt->pb_mutex);

		if (mgnt->pb_packet.size >= DataSize)
		{
			circlebuf_pop_front(&mgnt->pb_packet, DataBuffer, DataSize);

			size = DataSize;
		}
		else
		{
			memset(DataBuffer, 0, DataSize);

			// LOGE("valid_len:%d need %d\n", mgnt->stream->pb_packet.size, DataSize);
		}

		pthread_mutex_unlock(&mgnt->pb_mutex);
	}

	return size;
}

static void *audio_read_thread(void *args)
{
	LOGI("audio_ca_thread start \n");
    FILE *fp = fopen("/tmp/local_ai.pcm", "rb");
    if(fp == NULL)
    {
        LOGE("open local_ai.pcm fail\n");
        return NULL;
    }

    mgnt_ao_t * mgnt = (mgnt_ao_t *)args;

    while(!mgnt->quit)
    {
        char buf[640] = {0};
        //char *data = NULL;
        //int size = 0;

        int read_size = fread(buf, 1, BLOCKSIZE, fp);
        if(read_size <= 0)
        {
            usleep(10000);
            fseek(fp, 0, SEEK_SET);
            continue;
        }
        pthread_mutex_lock(&mgnt->pb_mutex);
        circlebuf_push_back(&mgnt->pb_packet, buf, read_size);
        pthread_mutex_unlock(&mgnt->pb_mutex);

        DPSleep(10);
    }

	if(fp)
	{
		fclose(fp);
		fp = NULL;
	}

	LOGI("audio_ca_thread end \n");

	return NULL;
}

void *Audio_test_Play_start(void)
{
    void *pret =NULL;
    mgnt_ao_t * ao_mgnt = (mgnt_ao_t *)bmalloc(sizeof(mgnt_ao_t));
    memset(ao_mgnt, 0, sizeof(mgnt_ao_t));
	int Channels = SAMPLECHN;
	int SampleBits = SAMPLEBITS;
	int SampleRate = SAMPLERATE;
	int Blocks = PLAYBLOCKS;
	int BlockTime = BLOCKTIME;
	int SamplesPerBlock = SampleRate * BlockTime / 1000;

	// 打开放音设备
	ao_mgnt->stream_id = Audio_Player_Open();
    pthread_mutex_init(&ao_mgnt->pb_mutex, NULL);

    circlebuf_init(&ao_mgnt->pb_packet);

    circlebuf_reserve(&ao_mgnt->pb_packet, MAX_CACHE_AUDIO_SIZE);

	CHECK_VAL_GOTO_IF_FAIL(ao_mgnt->stream_id, "Audio_Player_Open fail");

	// 设置放音参数
	int ret = Audio_Player_SetDevice(ao_mgnt->stream_id, "default");

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_SetDevice fail");

	ret = Audio_Player_SetFormat(ao_mgnt->stream_id, Channels, SampleBits, SampleRate);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_SetFormat fail");

	ret = Audio_Player_SetCache(ao_mgnt->stream_id, Blocks, SamplesPerBlock);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_SetCache fail");

	ret = Audio_Player_SetMode(ao_mgnt->stream_id, false, playback_cb, ao_mgnt);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_SetMode fail");

	ret = Audio_Player_SetVolume(ao_mgnt->stream_id, 50);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_SetVolume fail");

	ret = Audio_Player_Start(ao_mgnt->stream_id);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_Start fail");

    pthread_create(&ao_mgnt->ca_thread, NULL, audio_read_thread, ao_mgnt);
	//ao_mgnt->stream_id = AO_StreamID;
    pret = (void*)ao_mgnt;
	return pret;

fail:

	if (ao_mgnt->stream_id != 0)
	{
		Audio_Player_Close(ao_mgnt->stream_id);
		ao_mgnt->stream_id = 0;
	}

    pthread_mutex_destroy(&ao_mgnt->pb_mutex);

	circlebuf_free(&ao_mgnt->pb_packet);

    bfree(ao_mgnt);
    return pret;
}

void Audio_test_Play_stop(void*pmgnt)
{
    mgnt_ao_t * mgnt = (mgnt_ao_t *)pmgnt;
    if(mgnt)
    {
        if (mgnt->stream_id != 0)
        {
            Audio_Player_Stop(mgnt->stream_id);
            Audio_Player_Close(mgnt->stream_id);
            mgnt->stream_id = 0;
        }

        mgnt->quit = true;
        pthread_join(mgnt->ca_thread, NULL);
        pthread_mutex_destroy(&mgnt->pb_mutex);

	    circlebuf_free(&mgnt->pb_packet);
        bfree(mgnt);
    }

}

//======================plc msg=======================

static uint32_t g_l_dev_id = 0;
int PlcEventHandleCB(void*param, PlcEventHandleInfo_S *paraminfo)
{
	int ret = 0;
	printf("########### cb type:%u ###########\n", paraminfo->type);
	switch (paraminfo->type)
	{
	case Goblin_PLC_MSG_EVENTTYPE_PLC_VERSION:
		printf("PLC版本号:%x\n", paraminfo->eventinfo.plc_version.version);
		break;
	case Goblin_PLC_MSG_EVENTTYPE_PCL_RSSI:
		{
			printf("PCL信号设备数量:%d\n", paraminfo->eventinfo.plc_rssilist.plc_count);
			for (size_t i = 0; i < paraminfo->eventinfo.plc_rssilist.plc_count; i++)
			{
				printf("PCL信号设备ID:%x 接收的包:%d  丢弃的包:%d  RSSI:%d\n", paraminfo->eventinfo.plc_rssilist.prssi_list[i].id,
					paraminfo->eventinfo.plc_rssilist.prssi_list[i].c_recv,
					paraminfo->eventinfo.plc_rssilist.prssi_list[i].c_drop,
					paraminfo->eventinfo.plc_rssilist.prssi_list[i].rssi);
			}
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_UNLOCK:
		printf("开锁 设备：%x  位置：%x\n", paraminfo->eventinfo.unlock.devid, paraminfo->eventinfo.unlock.place);
		ret = 1;
		break;
	case Goblin_PLC_MSG_EVENTTYPE_ONLINE:
		printf("设备上线：%x 软件版本号：%s 硬件版本号:%s Plc 版本号:%x\n", paraminfo->eventinfo.Onlineinfo.devid,
			paraminfo->eventinfo.Onlineinfo.software_version,
			paraminfo->eventinfo.Onlineinfo.hardware_version,
			paraminfo->eventinfo.Onlineinfo.plc_version);
		break;
	case Goblin_PLC_MSG_EVENTTYPE_SEARCH_REQ:
		{
			printf("有设备:%x 在搜索类型：%d  roomid:%d\n", paraminfo->eventinfo.search_req.devid, paraminfo->eventinfo.search_req.type, paraminfo->eventinfo.search_req.roomid);

			if((paraminfo->eventinfo.search_req.type == (g_l_dev_id>>24)) &&
				(paraminfo->eventinfo.search_req.roomid==0 || paraminfo->eventinfo.search_req.roomid==((g_l_dev_id>>8)&0xff)))
			{
				paraminfo->eventinfo.search_req.ackdev = g_l_dev_id;
				paraminfo->eventinfo.search_req.ackstate = 0;
				paraminfo->eventinfo.search_req.acktype = 3;
				strncpy(paraminfo->eventinfo.search_req.ackhardware_version, "2.0.1", 32);
				strncpy(paraminfo->eventinfo.search_req.acksoftware_version, "2.0.2", 32);
				ret =1;
			}
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_SEARCH_RSP:
		printf("搜索到的设备:%x \n", paraminfo->eventinfo.search_rsp.devid);
		break;
	case Goblin_PLC_MSG_EVENTTYPE_SETID:
		{
			printf("远端:%x设置本地房号为:%s = %x \n", paraminfo->remotedev, paraminfo->eventinfo.set_id.strid, paraminfo->eventinfo.set_id.devid);

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
			printf("获取设备配置\n");

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

			paraminfo->eventinfo.door_cfg.paraminfo.doorlock[1].lockid = 2;
			paraminfo->eventinfo.door_cfg.paraminfo.doorlock[1].lockswitch = doorcfg.lockswitch1;
			paraminfo->eventinfo.door_cfg.paraminfo.doorlock[1].locktime = doorcfg.locktime1;
			paraminfo->eventinfo.door_cfg.paraminfo.doorlock[1].menicswitch = doorcfg.menicswitch1;
			paraminfo->eventinfo.door_cfg.paraminfo.doorlock[1].menictime = doorcfg.menictime1;
			ret= 1;

			goblin_door_cfg_t *pparaminfo = &paraminfo->eventinfo.door_cfg.paraminfo;
			printf("+++门口机配置: 移动侦测灵敏度:%d  呼叫铃声:%d  通话音量:%d\n", pparaminfo->motion, pparaminfo->soundcall, pparaminfo->soundtalk);
			for (int i = 0; i < DOOR_LOCK_COUNT; i++)
			{
				printf("锁ID:%d  开锁延时:%d 门磁开关:%d 门磁延时:%d \n", pparaminfo->doorlock[i].lockswitch,
					pparaminfo->doorlock[i].locktime, pparaminfo->doorlock[i].menicswitch, pparaminfo->doorlock[i].menictime);
			}
		}
		break;
	case Goblin_PLC_MSG_EVENTTYPE_SETCFG:
		{
			goblin_door_cfg_t *pparaminfo = &paraminfo->eventinfo.door_cfg.paraminfo;
			printf("设置门口机配置: 移动侦测灵敏度:%d  呼叫铃声:%d  通话音量:%d\n", pparaminfo->motion, pparaminfo->soundcall, pparaminfo->soundtalk);
			for (int i = 0; i < DOOR_LOCK_COUNT; i++)
			{
				printf("锁ID:%d 开锁电平:%d 开锁延时:%d 门磁开关:%d 门磁延时:%d \n", pparaminfo->doorlock[i].lockid, pparaminfo->doorlock[i].lockswitch,
					pparaminfo->doorlock[i].locktime, pparaminfo->doorlock[i].menicswitch, pparaminfo->doorlock[i].menictime);
			}


			door_cfg_t doorcfg = {0};
			doorcfg.motionlevel = pparaminfo->motion;
			doorcfg.soundcall = pparaminfo->soundcall;
			doorcfg.soundtalk = pparaminfo->soundtalk;
			doorcfg.lockswitch = pparaminfo->doorlock[0].lockswitch;
			doorcfg.locktime = pparaminfo->doorlock[0].locktime;
			doorcfg.menicswitch = pparaminfo->doorlock[0].menicswitch;
			doorcfg.menictime = pparaminfo->doorlock[0].menictime;

			doorcfg.lockswitch1 = pparaminfo->doorlock[1].lockswitch;
			doorcfg.locktime1 = pparaminfo->doorlock[1].locktime;
			doorcfg.menicswitch1 = pparaminfo->doorlock[1].menicswitch;
			doorcfg.menictime1 = pparaminfo->doorlock[1].menictime;
			SetDoorCfg(&doorcfg);
		}
	case Goblin_PLC_MSG_EVENTTYPE_REPORTAGENT:
		{
			printf("上传代理  \n");
			ret = 1;
		}
		break;
	default:
		break;
	}
	return ret;
}

static void *psession = NULL;
void HangupSession(void)
{
	goblin_hangup_by_session(psession);
}

FILE *pfd_a = NULL;
int RemoteAudio_CB(void *param, uint8_t *pframedate, uint32_t framelen)
{
	if(pfd_a)
		fwrite(pframedate, 1, framelen, pfd_a);
	return 0;
}

void AcceptSession(void)
{
	goblin_call_accept(psession, 1);
	goblin_set_audio_cb_by_session(psession, RemoteAudio_CB, NULL, NULL, NULL);
}

void SetSessionVolume(int vol)
{
	goblin_set_volume_by_session(psession, vol);
}

void SetSessiondisplay(int x, int y, int w, int h)
{
	goblin_set_video_displayarea_by_session(psession, x, y, w, h);
}

void QequestIdr(void)
{
	goblin_call_request_ikey_by_session(psession);
}

void QequestView(uint8_t iscrop)
{
	goblin_call_request_crop_by_session(psession, iscrop);
}

void OnoffAudio(uint8_t onoff)
{
	goblin_tran_auido_by_session(psession, onoff);
}

void GetJpgdata(uint32_t timeout)
{
	// char *pjpgdata = NULL;
	// uint32_t ltick = time_relative_ms();
	// int len = goblin_get_jpgdata_by_session(psession, 1024, 600, &pjpgdata,  timeout);
	// printf("================== get jpg:%p len:%u losttime:%u\n", pjpgdata, len, time_relative_ms()-ltick);
	// if (pjpgdata && len)
	// {
	// 	FILE *pdf = fopen("enc.jpg", "wb");
	// 	if (pdf)
	// 	{
	// 		fwrite(pjpgdata, 1, len, pdf);
	// 		fclose(pdf);
	// 	}
	// }
}

FILE *pfd_v = NULL;
FILE *pfd_v_sub = NULL;
#define MAX_VIDEO_FILE 2
FILE *pfd_recv[MAX_VIDEO_FILE] = {0};
void FinishRecvVideo(void)
{
	for (size_t i = 0; i < MAX_VIDEO_FILE; i++)
	{
		if(pfd_recv[i])
			fclose(pfd_recv[i]);
		pfd_recv[i] = NULL;
	}
}

int RemoteVideo_CB(void *param, uint8_t *pframedate, uint32_t framelen, uint8_t bkey)
{
	printf("========main I:%d =len:%d  userdata:%p\n", bkey, framelen, param);
	FILE *pfd = (FILE*)param;
	if(pfd)
		fwrite(pframedate, 1, framelen, pfd);
	return 0;
}
int RemoteVideoSub_CB(void *param, uint8_t *pframedate, uint32_t framelen, uint8_t bkey)
{
	if(pfd_v_sub)
		fwrite(pframedate, 1, framelen, pfd_v_sub);
	return 0;
}

#ifdef ENABLE_V1
static int ring_play_id = 0;
#endif
static void stop_play_ring(void)
{
#ifdef ENABLE_V1
	if (ring_play_id != 0)
	{
		sound_play_stop(ring_play_id);
		ring_play_id = 0;
	}
#endif
}

static void start_play_ring(void)
{
#ifdef ENABLE_V1
	int volume = 100;
	// stop_play_ring(mgnt);

	// db_sys_param_get("speaker_volume_call_ring", &volume, sizeof(volume));

	if (ring_play_id == 0)
	{
	 	ring_play_id = sound_play_start(RING_MP3, PLAY_FOREVER, volume, false);
	}
#endif
}

void testring(void)
{
	static int onoff = 0;
	if (onoff == 0)
	{
		onoff = 1;
		start_play_ring();
	}
	else
	{
		onoff = 0;
		stop_play_ring();
	}

}

int CallEventHandle(void *param, plc_call_info_s *paraminfo)
{
	printf("呼叫状态:%d  dev:%x  session:%p\n", paraminfo->type, paraminfo->dev, paraminfo->psession);
	if (paraminfo->type == PLC_CALL_STATE_CALLIN)
	{
		psession= paraminfo->psession;
		goblin_call_ack_state(paraminfo->psession, 0);

		printf("########### 呼入 视频分辨率:%dx%d ###########\n", paraminfo->eventinfo.callin.video_width, paraminfo->eventinfo.callin.video_height);
		goblin_set_video_trans_by_session(psession, 1);
#ifdef ENABLE_MEDIA
		video_idr(0);
#endif
		goblin_call_accept(psession, 1);
		if(!pfd_a)
			pfd_a = fopen("./remte.pcm", "wb");
		goblin_set_audio_cb_by_session(psession, RemoteAudio_CB, NULL, NULL, NULL);
		goblin_set_video_cb_by_session(psession, RemoteVideo_CB, NULL);
		goblin_set_video_sub_cb_by_session(psession, RemoteVideoSub_CB, NULL);

		goblin_set_volume_by_session(psession, 30);
	}
	else if (paraminfo->type == PLC_CALL_STATE_CALLOUT_ACK)
	{
#ifdef ENABLE_MEDIA
		video_crop_switch(0);
		video_idr(0);

		start_play_ring();
#endif
	}
	else if (PLC_CALL_STATE_ACCEPT == paraminfo->type)
	{
		stop_play_ring();
		void *p=NULL;
		for (size_t i = 0; i < MAX_VIDEO_FILE; i++)
		{
			if(pfd_recv[i] == NULL){
				char filename[128] = {0};
				snprintf(filename, sizeof(filename), "./video/remote_%d.h265", i);
				pfd_recv[i] = fopen(filename, "wb");
				p = pfd_recv[i];
				break;
			}
		}

		goblin_set_video_cb_by_session(paraminfo->psession, RemoteVideo_CB, p);
		goblin_set_video_sub_cb_by_session(paraminfo->psession, RemoteVideoSub_CB, NULL);
	}
	else if (PLC_CALL_STATE_HUNGUP == paraminfo->type)
	{
		stop_play_ring();
		if(pfd_a)
			fclose(pfd_a);
		pfd_a = NULL;
	}
	else if (paraminfo->type == PLC_CALL_STATE_HANGUP_ACK)
	{
		stop_play_ring();
	}
	else if (paraminfo->type == PLC_CALL_STATE_REQUEST_I)
	{
#ifdef ENABLE_MEDIA
		printf("Request send data to channel\n");
		video_idr(0);
#endif
	}
	else if (paraminfo->type == PLC_CALL_STATE_REQUEST_VIEW)
	{

		/* code */
#ifdef ENABLE_MEDIA
		video_crop_switch(paraminfo->eventinfo.request_view.iscrop);
#endif
	}
	else if (paraminfo->type == PLC_CALL_STATE_VIDEO_DEC_START)
	{
		printf("===============[%u]收到第一帧图片解码，打开显示===================\n\n", time_relative_ms());
	}


	return 0;
}

void Md5_File(char *pfilename, char *pOutBuf);
static FILE *pfdc = NULL;
int MyFileCallBackFunc(void *param, file_pack_t *paraminfo, uint32_t remotedev)
{
	int ret = -1;


	if((paraminfo->fileflag&FILE_START_FLAG) && pfdc == NULL)
	{
			pfdc = fopen("./test.log", "wb");

		LOGI("recv filetype:%d  open file :%p ==========\n", paraminfo->file_type, pfdc);
	}
	if (pfdc)
	{
		//序号要连续
		if(paraminfo->fileseq != (paraminfo->fileseq+1))
		{
			printf("writ file seq:%d \n", paraminfo->fileseq);
			fwrite(paraminfo->data, 1, paraminfo->datalen, pfdc);
		}
		else
		{
			LOGW("The file seq %u-%d is not continuous, ignore it\n", paraminfo->fileseq, paraminfo->fileseq);
		}
	}

	if((paraminfo->fileflag&FILE_END_FLAG) && pfdc)
	{
		fclose(pfdc);
		pfdc = NULL;
		printf("====================filemd5:%s \n", paraminfo->filemd5);
		char strmd5[33] = {0};
		char strmd5_tmp[33] = {0};
		Md5_File("./test.log", strmd5_tmp);
		memcpy(strmd5, paraminfo->filemd5, 32);
		if(strcmp(strmd5_tmp, strmd5) == 0)
		{
			LOGI("recv file md5:%s  match \n", strmd5_tmp);
			ret = 0;
		}
		else
		{
			LOGW("recv file md5:%s  not match \n", strmd5_tmp);
			ret = -1;
		}
	}

	return ret;
}

void LocalPlcMsg(uint32_t dev, uint8_t disablecache)
{
	g_l_dev_id = dev;
	pfd_v = fopen("./remte.h265", "wb");
	pfd_v_sub = fopen("./remte_sub.h265", "wb");
	// pfd_a = fopen("./remte.pcm", "wb");
	//1.初始化本地PLC服务
	PlcEventHandler  param = {0};
	param.m_pfuncb = PlcEventHandleCB;
	param.m_pfilecb = MyFileCallBackFunc;
	param.m_pdevplc = dev;
	param.m_devinfo_type = PLC_DEVICE_TYPE_DOOR;

	param.m_pdevver_hw = "2.0.1";
	param.m_pdevver_app = "2.0.2";
	if(goblin_plc_process_start(&param) != 0)
	{
		LOGE("goblin_plc_process_start failed\n");
		return ;
	}

	//2.初始化本地通话
	Call_Info_S callinfo = {0};
	callinfo.dev_plc = dev;
	callinfo.handfunc = CallEventHandle;
	callinfo.display_x = 0;
	callinfo.display_y = 0;
	callinfo.display_w = 1024;
	callinfo.display_h = 600;
	callinfo.is_disable_video_cache = disablecache;
	callinfo.audio_play_volume = 100;

	callinfo.is_disable_video_decshow=1;
	goblin_call_init(&callinfo);
}
