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
#include <errno.h>
#include <AudioCore.h>
#include <utils/sem.h>
#include <utils/circlebuf.h>
#include <utils/log.h>
#include <utils/util.h>
#include <utils/bmem.h>
#include <utils/list.h>
#include <utils/time_helper.h>

#include "tran_video.h"
#include "spi_plc_mgnt.h"
#include "spi_plc_trans.h"
#include "goblin_plc_callcore.h"
#include "trans.h"
#include "DPDef.h"

#ifdef ENABLE_NX5
#include <video/vd/VDecApi.h>
#include <video/vd/VoApi.h>
#endif

//是否调用子码流
#define  VIDEO_SUB_CHANNEL_ENABLE  0

typedef struct video_frame_
{
	struct list_head node;
	uint32_t datalen;
    uint8_t bkey;			//是否关键帧
	uint8_t *pdata;
}video_frame_t;


typedef struct video_tran_mgnt
{
    uint32_t rdev;
    uint32_t rport;
    uint32_t rport_sub;
    uint32_t multicastaddr;
    uint8_t trandirection;
    uint8_t isdisable_video_cache;   //是否禁止视频缓存(缓存是为了防止视频卡顿)
    uint8_t push_video_type;        //主干推流类型 0:主码流 1:子码流

    void * pushtrans ;	//推流句柄
    void * pushtrans_sub ; //子码流推流
    void * pulltrans ;	//拉流句柄
    void * pulltrans_sub ;  //拉子码流句柄

    bool bvdisabledecshow;     //视频是否禁止解码显示
    bool bvdecopen;    //是否打开解码
    int DecChannel;     //解码通道
    int display_x;
    int display_y;
    int display_h;
    int display_w;

	bool   thread_run;
	pthread_t play_thread;
	pthread_mutex_t  play_mutex;

	bool bfirstkey;
	uint32_t playframe_count;
	struct list_head play_list;

    RemoteVideoCB videocb;
    void *puserdata;

    RemoteVideoCB videocb_sub;
    void *puserdata_sub;

    uint32_t jpg_width;
    uint32_t jpg_height;
    bool bencjpg;   //是否编码jpg
    void *sem_jpg;      //编码信号
    int jpgdatalen;
    char *pjpgdata;
} mgnt_video_t;

#define LOCAL_LOCK() pthread_mutex_lock(&g_local_mutex)

#define LOCAL_UNLOCK() pthread_mutex_unlock(&g_local_mutex)

static pthread_mutex_t g_local_mutex = PTHREAD_MUTEX_INITIALIZER;

// 显示通道
#define DISP_CHANNEL 0
// 显示图层
#define DISP_LAYER 0
#define DEC_BUF_SIZE 512 * 1024

void VideoDecFirstFrame(void);

#ifdef ENABLE_NX5
void video_dec_and_show_start(mgnt_video_t *mgnt)
{
    if(mgnt->bvdisabledecshow)
    {
        LOGW("Video Dec is not disable\n");
        return ;
    }
	//初始化解码器
	VDEC_Init();
	// 使能显示
	VO_Enable();

	//设置显示位置
	//DecH264Display(NULL);
    VO_LAYER_INFO VoInfo;
    memset(&VoInfo, 0, sizeof(VO_LAYER_INFO));
    VoInfo.Rect.X = mgnt->display_x;
    VoInfo.Rect.Y = mgnt->display_y;
    VoInfo.Rect.W = mgnt->display_w;
    VoInfo.Rect.H = mgnt->display_h;
    if (!VO_EnableChn(DISP_CHANNEL, &VoInfo))
      printf("VO_EnableChn Failed!!!\n");

	VDEC_CHN_ATTR_S Attr;
	memset(&Attr, 0, sizeof(VDEC_CHN_ATTR_S));
	Attr.deType = VDEC_PAYLOAD_TYPE_H265;
	Attr.BufSize = 0x80000*4 ;  // 512KB
	Attr.u32PicWidth = 1920;//2560;     // 1280;
	Attr.u32PicHight = 1088;//1440;

	mgnt->DecChannel = VDEC_CreateChn(&Attr);
    if (mgnt->DecChannel >= 0) {
      printf("CreateChn DecChannel[%d] Successful\n", mgnt->DecChannel);
    } else {
      printf("Create First DecChannel[%d] Failed!\n", mgnt->DecChannel);
    }

    mgnt->bvdecopen = true;
}

bool video_decoder_frame(void *param, int Channel, unsigned char *pdata, int length)
{
    if(!pdata || length > DEC_BUF_SIZE || length <= 0)
    {
        LOGW("pdata:%p is null or length:%d is invalid\n", pdata, length);
        return false;
    }
    mgnt_video_t *pplayvideo_process = (mgnt_video_t *)param;
    if(!pplayvideo_process->bvdecopen)
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

                        LOCAL_LOCK();
                        if (pplayvideo_process && pplayvideo_process->bencjpg)
                        {
                            char *pjpgbuf=NULL;
                            uint64_t ltick = time_relative_us();
                            int jpglen = VO_CapJpgData(&pjpgbuf, 0, 2, pplayvideo_process->jpg_width, pplayvideo_process->jpg_height, &Frame);
                            printf("\n\n\n====================##########w:%u, h:%u jpglen:%d pjpgbuf:%p losttime:%llu ###########================\n\n\n\n",
                                    pplayvideo_process->jpg_width, pplayvideo_process->jpg_height,jpglen, pjpgbuf, time_relative_us()-ltick);
                            if (jpglen>0 && pjpgbuf)
                            {
                                pplayvideo_process->bencjpg = false;
                                pplayvideo_process->jpgdatalen = jpglen;
                                pplayvideo_process->pjpgdata = (char*)malloc(jpglen+1);
                                memset(pplayvideo_process->pjpgdata, 0, jpglen+1);
                                memcpy(pplayvideo_process->pjpgdata, pjpgbuf, jpglen);
                                SetSemaphore(pplayvideo_process->sem_jpg);
                            }

                            VO_FreeJpgData(pjpgbuf);
                        }
                        LOCAL_UNLOCK();
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

int video_decoder_set_display_area(int x, int y, int w, int h)
{
    VO_LAYER_RECT VoRect;
    memset(&VoRect, 0, sizeof(VO_LAYER_RECT));

    //显示区域
    VoRect.X = x;
    VoRect.Y = y;
    VoRect.W = w;
    VoRect.H = h;

    LOGI("area x:%d y:%d w:%d h:%d \n", x, y, w, h);
    VO_Resize(DISP_CHANNEL, DISP_LAYER, &VoRect);

    return 0;
}

void dec_and_show_video_stop(mgnt_video_t *mgnt)
{
    if(mgnt->bvdecopen)
    {
        mgnt->bvdecopen = false;
        VO_DisableVideoLayer(DISP_CHANNEL, DISP_LAYER);

        VDEC_DestroyChn(mgnt->DecChannel);
        mgnt->DecChannel = -1;
        VDEC_DeInit();

        VO_Disable();
    }
}

#endif

static void *video_play_thread(void *args)
{
#ifdef ENABLE_NX5
	#define PLAY_FRAME_RATE (3)
	#define TIME_OUT_US		(65*1000+500)
	mgnt_video_t *pplayvideo_process = (mgnt_video_t *)args;

	LOGI("video_play_thread start [%p]\n", pplayvideo_process);
	uint64_t lasttick = 0;
	bool bfullcache = false;
    bool bfirstframe = true;
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
				video_decoder_frame(pplayvideo_process, pplayvideo_process->DecChannel, p->pdata, p->datalen);

				lasttick = ltick;
                if(bfirstframe)
                {
                    bfirstframe = false;

                    VideoDecFirstFrame();
                }
LOCAL_LOCK();
                if(pplayvideo_process->videocb)
                    pplayvideo_process->videocb(pplayvideo_process->puserdata, p->pdata, p->datalen, p->bkey);
LOCAL_UNLOCK();
#endif
				bfree(p);
			}
			else
				usleep(1*1000);
		}
	}
#endif
	return NULL;
}

static void recv_cb(void *p, unsigned char *lpdata, int dlen)
{
#ifdef ENABLE_NX5
	mgnt_video_t *pplay = (mgnt_video_t*)p;
	media_frame *pframe = (media_frame *)lpdata;
	//printf("++++++++++++ p:%p = dlen:%d  streamid:%d pplay->videocb:%p\n", p, dlen, pframe->streamid, pplay->videocb);
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
    if(pplay->isdisable_video_cache)
    {
        video_decoder_frame(pplay, pplay->DecChannel, (unsigned char*)pframe->data, framelen);
        LOCAL_LOCK();
        if(pplay->videocb)
            pplay->videocb(pplay->puserdata, (uint8_t*)pframe->data, framelen, pframe->bkey);
        LOCAL_UNLOCK();

        if(!pplay->bfirstkey && pframe->data[4]==0x40)
		{
			pplay->bfirstkey = true;
            VideoDecFirstFrame();
        }
    }
    else
	{
#if 1
		static uint32_t llt = 0;
		uint32_t ctick = time_relative_ms();
		if(pframe->data[4]==0x40)
		{
			LOGI("<%u>$$$$$$$$$ I key:%d, len:%u lost time:%d \n", ctick, pframe->bkey, framelen, ctick-llt);
		}

		llt = ctick;
#endif

		if(pplay)
		{

			if(!pplay->bfirstkey && pframe->data[4]==0x40)
			{
				pplay->bfirstkey = true;
				LOGI("############# first key : %02x  %02x  %02x  %02x  %02x \n", pframe->data[0], pframe->data[1], pframe->data[2], pframe->data[3], pframe->data[4]);
			}
			if(pplay->bfirstkey)
			{
				video_frame_t *pvframe = (video_frame_t*)bmalloc(sizeof(video_frame_t)+framelen+1);
				memset(pvframe, 0, sizeof(video_frame_t)+framelen+1);
				pvframe->datalen = framelen;
                pvframe->bkey = pframe->bkey;
				pvframe->pdata = (uint8_t*)(pvframe+1);
				memcpy(pvframe->pdata, pframe->data, pvframe->datalen);

				//printf(">>>>>>>> first key : %02x  %02x  %02x  %02x  %02x \n", pframe->data[0], pframe->data[1], pframe->data[2], pframe->data[3], pframe->data[4]);
				pthread_mutex_lock(&pplay->play_mutex);
				pplay->playframe_count++;
				list_add_tail(&pvframe->node, &pplay->play_list);
				pthread_mutex_unlock(&pplay->play_mutex);
			}
		}
		// else
		// 	video_decoder_frame(pplay->DecChannel, pframe->data, dlen-sizeof(media_frame));

    }
#endif //ENABLE_NX5
}
#if defined(VIDEO_SUB_CHANNEL_ENABLE) && VIDEO_SUB_CHANNEL_ENABLE==1 //暂时不用子码流 屏蔽此接口
//接收子码流回调
static void recv_sub_cb(void *p, unsigned char *lpdata, int dlen)
{
#ifdef ENABLE_NX5
	mgnt_video_t *pplay = (mgnt_video_t*)p;
	media_frame *pframe = (media_frame *)lpdata;

    int framelen =  dlen-sizeof(media_frame);
    LOCAL_LOCK();
    if(pplay->videocb_sub)
        pplay->videocb_sub(pplay->puserdata_sub, (uint8_t*)pframe->data, framelen, pframe->bkey);
    LOCAL_UNLOCK();
#endif //ENABLE_NX5
}
#endif //0
DPLAN_PUBLIC_API void *plc_video_trans_start(v_transmit_param_t *pvparam)
{
    void *pret = NULL;
    uint8_t localdevtype = pvparam->srcdev>>24;
    uint8_t remotedevtype = pvparam->rdev>>24;
    LOGI("PlcVideoTrans  redev:%x srcdev:%x rport:%x rport_sub:%x trandirection:%x isdisablevideocache:%d isdisablevdecshow:%d\n",
        pvparam->rdev, pvparam->srcdev, pvparam->rport, pvparam->rport_sub , pvparam->trandirection, pvparam->isdisablevideocache, pvparam->isdisablevdecshow);

    mgnt_video_t * pmgnt_video = bmalloc(sizeof(mgnt_video_t));
    memset(pmgnt_video, 0, sizeof(mgnt_video_t));
    pmgnt_video->rdev = pvparam->rdev;
    pmgnt_video->rport = pvparam->rport;
    pmgnt_video->rport_sub = pvparam->rport_sub;
    pmgnt_video->trandirection = pvparam->trandirection;
    pmgnt_video->display_x = 0;
    pmgnt_video->display_y = 0;
    pmgnt_video->display_w = 320;
    pmgnt_video->display_h = 240;
    pmgnt_video->isdisable_video_cache = pvparam->isdisablevideocache;
    pmgnt_video->push_video_type = pvparam->videoType;
    if(pvparam->prect)
    {
        pmgnt_video->display_x = pvparam->prect->display_x;
        pmgnt_video->display_y = pvparam->prect->display_y;
        pmgnt_video->display_w = pvparam->prect->display_w;
        pmgnt_video->display_h = pvparam->prect->display_h;
    }

    //本地是室内机，远端是门口机， 组播i接收对端视频
    if(localdevtype==1 && remotedevtype==3)
    {
        pmgnt_video->multicastaddr = (pvparam->rdev&0xf)|0xff000000;
    }
    //本地门口机，远端室内机， 发组播视频
    else if(localdevtype==3 && remotedevtype==1)
    {
        pmgnt_video->multicastaddr = (pvparam->srcdev&0xf)|0xff000000;
    }
    LOGI("multicastaddr:%x \n", pmgnt_video->multicastaddr);

    if (pvparam->trandirection & VIDEO_TRAN_RECV)
    {
        /* code */
        //1.初始化播放缓存队列
        INIT_LIST_HEAD(&pmgnt_video->play_list);
        //2.创建播放队列锁
        pthread_mutex_init(&pmgnt_video->play_mutex, NULL);

        //3.创建播放线程
        pmgnt_video->thread_run = true;
        int ret = pthread_create(&pmgnt_video->play_thread, NULL, video_play_thread, pmgnt_video);
        if (ret != 0)
        {
            pmgnt_video->thread_run = false;
            LOGE("pthread_create error:%d \n", ret);
        }

#ifdef ENABLE_NX5
        pmgnt_video->bvdisabledecshow = pvparam->isdisablevdecshow;
        video_dec_and_show_start(pmgnt_video);
#endif
        //VIDEO_PORT
        uint32_t remoteport = pvparam->rport;
        //if(pvparam->rdev>0)
        //    remoteport = pvparam->rport+(pvparam->rdev&0xf)-1;

        spi_plc_mgnt_multicast_ctrl(pmgnt_video->multicastaddr, 0);
		pmgnt_video->pulltrans = transpull_create(remoteport, pvparam->rdev, pvparam->rport, pvparam->rdev, recv_cb, pmgnt_video, true);
#if defined(VIDEO_SUB_CHANNEL_ENABLE) && VIDEO_SUB_CHANNEL_ENABLE==1
        pmgnt_video->pulltrans_sub = transpull_create(pvparam->rport_sub, pvparam->rdev, pvparam->rport_sub, pvparam->rdev, recv_sub_cb, pmgnt_video, true);
#endif
    }

    if (pvparam->trandirection & VIDEO_TRAN_SEND)
    {
LOCAL_LOCK();
        //pvparam->rdev 视频发的广播 0xffffffff 端口根据本机
        uint32_t remoteport = pvparam->rport;
        //if(pvparam->srcdev>0)
        //    remoteport = pvparam->rport+(pvparam->srcdev&0xf)-1;
        tran_push_info_s push_info={0};
        memset(&push_info, 0, sizeof(tran_push_info_s));
        push_info.lport = pvparam->rport;
        push_info.rip=pvparam->rdev;
        push_info.rport = pvparam->rport;
        push_info.plc_dev = pmgnt_video->multicastaddr;
        push_info.plc_port = remoteport;
        push_info.bplc_tran = true;
        push_info.bretrans = true;
        push_info.dropcb = pvparam->dropcb;
        push_info.pdropuserdata = pvparam->pdropuserdata;

        pmgnt_video->pushtrans = transpush_create(&push_info);
#if defined(VIDEO_SUB_CHANNEL_ENABLE) && VIDEO_SUB_CHANNEL_ENABLE==1 //暂时不用子码流 屏蔽此接口
        memset(&push_info, 0, sizeof(tran_push_info_s));
        push_info.lport = pvparam->rport_sub;
        push_info.rip=pvparam->rdev;
        push_info.rport = pvparam->rport_sub;
        push_info.plc_dev = pmgnt_video->multicastaddr;
        push_info.plc_port = pvparam->rport_sub;
        push_info.bplc_tran = true;
        push_info.bretrans = true;
        push_info.dropcb = NULL;
        pmgnt_video->pushtrans_sub = transpush_create(&push_info);
#endif
LOCAL_UNLOCK();
    }

    pret = pmgnt_video;
    return pret;
}

//推流发送函数
DPLAN_PUBLIC_API int plc_video_trans_send(void*param, int channel,void *pdata,int dlen)
{
    if(!param)
        return -1;
    media_frame *pframe = (media_frame *)pdata;
    mgnt_video_t *pmgnt_video = (mgnt_video_t *)param;
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

	unsigned char streamid_bk = pframe->streamid;
LOCAL_LOCK();
	if(pmgnt_video && pmgnt_video->pushtrans)
	{
		// 还原
		pframe->streamid = streamid_bk;
		pframe->streamid += channel*2;
        //如果主推流是子码流，就只发子码流
        if(pmgnt_video->push_video_type==1 && channel==1)
        {
            transpush_send(pmgnt_video->pushtrans,(unsigned char*)pframe,dlen,pframe->bkey);
        }
		else if(pmgnt_video->push_video_type==0 && channel==0 && pframe->streamid <= 2)
		{
            transpush_send(pmgnt_video->pushtrans,(unsigned char*)pframe,dlen,pframe->bkey);
		}
        else if (channel==1 && pmgnt_video->pushtrans_sub)
        {
            //transpush_send(pmgnt_video->pushtrans_sub,(unsigned char*)pframe,dlen,pframe->bkey);
        }
	}
LOCAL_UNLOCK();
	return 0;
}

DPLAN_PUBLIC_API int plc_video_trans_set_displayarea(void*param, v_show_rct_t *prect)
{
    mgnt_video_t *pmgnt_video = (mgnt_video_t *)param;
    if(pmgnt_video && prect)
    {
#ifdef ENABLE_NX5
        if(pmgnt_video->pulltrans)
            video_decoder_set_display_area(prect->display_x, prect->display_y, prect->display_w, prect->display_h);
#else
        LOGW("The function is not exit\n");
#endif
    }

    return 0;
}

int plc_video_frame_cb_set(void*param, RemoteVideoCB pcb, void *pusrdata)
{
    mgnt_video_t *pmgnt_video = (mgnt_video_t *)param;
    if(pmgnt_video)
    {
        LOCAL_LOCK();
        pmgnt_video->videocb = pcb;
        pmgnt_video->puserdata = pusrdata;
        LOCAL_UNLOCK();
    }

    return 0;
}

//子码流回调
int plc_video_sub_frame_cb_set(void*param, RemoteVideoCB pcb, void *pusrdata)
{
    mgnt_video_t *pmgnt_video = (mgnt_video_t *)param;
    if(pmgnt_video)
    {
        LOCAL_LOCK();
        pmgnt_video->videocb_sub = pcb;
        pmgnt_video->puserdata_sub = pusrdata;
        LOCAL_UNLOCK();
    }

    return 0;
}

int plc_video_enc_jpg(void*param, uint32_t width, uint32_t height, char **pjpgdata, uint32_t timeout)
{
    int jpglen = 0;
    mgnt_video_t *pmgnt_video = (mgnt_video_t *)param;

    do
    {
        if(!pmgnt_video)
            break;
        //如果正在编码就退出
        if(pmgnt_video->bencjpg)
            break;
        LOCAL_LOCK();
        if(pmgnt_video->pjpgdata)
            free(pmgnt_video->pjpgdata);
        pmgnt_video->pjpgdata = NULL;
        pmgnt_video->jpgdatalen = 0;
        pmgnt_video->jpg_width = width;
        pmgnt_video->jpg_height = height;

        pmgnt_video->bencjpg = true;
        pmgnt_video->sem_jpg = CreateSemaphore(0);
        LOCAL_UNLOCK();

        GetSemaphore(pmgnt_video->sem_jpg, timeout);
        LOCAL_LOCK();
        pmgnt_video->bencjpg = false;
        if(pmgnt_video->pjpgdata)
        {
            jpglen = pmgnt_video->jpgdatalen;
            *pjpgdata = pmgnt_video->pjpgdata;

            pmgnt_video->pjpgdata = NULL;
            pmgnt_video->jpgdatalen = 0;
        }
        DestorySemaphore(pmgnt_video->sem_jpg);
        pmgnt_video->sem_jpg =NULL;
        LOCAL_UNLOCK();

    }while (0);

    return jpglen;
}

DPLAN_PUBLIC_API void plc_video_trans_stop(void*param)
{
    mgnt_video_t *pmgnt_video = (mgnt_video_t *)param;
    if(pmgnt_video)
    {
        if(pmgnt_video->pulltrans_sub)
        {
            transpull_destroy(pmgnt_video->pulltrans_sub);
		    pmgnt_video->pulltrans_sub = NULL;
        }
        if(pmgnt_video->pulltrans)
        {
            transpull_destroy(pmgnt_video->pulltrans);
		    pmgnt_video->pulltrans = NULL;

            //1.停止播放线程
            if (pmgnt_video->thread_run)
            {
                pmgnt_video->thread_run = false;
                pthread_join(pmgnt_video->play_thread, NULL);
                pmgnt_video->play_thread = 0;
            }

            //2.清空线程队列缓存
            video_frame_t *node = NULL, *next = NULL;
            pthread_mutex_lock(&pmgnt_video->play_mutex);
            list_for_each_entry_safe(node, next, &pmgnt_video->play_list, node)
            {
                list_del(&node->node);
                bfree(node);
            }
            pthread_mutex_unlock(&pmgnt_video->play_mutex);
            //3.销毁播放队列锁
            pthread_mutex_destroy(&pmgnt_video->play_mutex);

#ifdef ENABLE_NX5
            dec_and_show_video_stop(pmgnt_video);
#endif
            //退出组播
            spi_plc_mgnt_multicast_ctrl(pmgnt_video->multicastaddr, 1);
        }

        LOCAL_LOCK();
        if(pmgnt_video->pushtrans)
        {
            transpush_destroy(pmgnt_video->pushtrans);
            pmgnt_video->pushtrans = NULL;
        }

        if(pmgnt_video->pushtrans_sub)
        {
            transpush_destroy(pmgnt_video->pushtrans_sub);
            pmgnt_video->pushtrans_sub = NULL;
        }
        LOCAL_UNLOCK();

        bfree(pmgnt_video);
    }
    LOGW("end \n");
}