#define _GNU_SOURCE

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
#include <utils/circlebuf.h>
#include <utils/log.h>
#include <utils/util.h>
#include <utils/bmem.h>
#include <utils/time_helper.h>

#include "goblin_plc_callcore.h"
#include "tran_audio.h"
#include "spi_plc_mgnt.h"
#include "trans.h"
#include "DPDef.h"
#include "g711common.h"
#include "../../dtln/dtln_tr.h"
#include <atomic.h>

#ifdef ENABLE_DTLN
//#define AUDIO_DTLN_NN
// #define AUDIO_NS_NN     //只降噪
#endif

#ifdef ENABLE_V6
//是否使用AI降噪算法
#define AUDIO_AI 1
#endif

//是否G711U 压缩算法后的数据
#define AUDIO_DATA_ENC_G711U     1

#define AUDIO_DATA_SVEE_FILE    0

#define AUDIO_ENC_BY_G711   1

#if AUDIO_ENC_BY_G711
#define SAMPLERATE      16000    // 默认采样率（赫兹）
#else
#define SAMPLERATE      16000    // 默认采样率（赫兹）
#endif
#define BLOCKTIME       40      // 每个缓冲数据块的时长（毫秒）

#define PLAYBLOCKS      4      // 播放最大缓冲数据块数
#define RECORDBLOCKS    4      // 录音最大缓冲数据块数
#define SAMPLEBITS      16
#define SAMPLECHN       1
#define BLOCKSIZE       (SAMPLERATE * BLOCKTIME * SAMPLECHN * (SAMPLEBITS / 8) / 1000)    // 320

#define MAX_CACHE_AUDIO_SIZE (SAMPLERATE * SAMPLECHN * (SAMPLEBITS / 8))

#define SEND_AUDIO_BUF_SIZE    (1976-sizeof(media_frame)-5)//(1600)

#define LOCAL_LOCK() pthread_mutex_lock(&g_local_mutex)

#define LOCAL_UNLOCK() pthread_mutex_unlock(&g_local_mutex)

static pthread_mutex_t g_local_mutex = PTHREAD_MUTEX_INITIALIZER;

// #ifdef AUDIO_DTLN_NN
static DtlnCreateHandle pmyDtlnHanldCreate = NULL;
static DtlnDestroyHandle pmyDtlnHanldDestroy = NULL;
static DtlnTrainByHandle pmyDtlnHanldTran = NULL;

DPLAN_PUBLIC_API void LoadDtlnFun(DtlnCreateHandle createfun, DtlnDestroyHandle destoryfun, DtlnTrainByHandle tranfun)
{
    pmyDtlnHanldCreate = createfun;
    pmyDtlnHanldDestroy = destoryfun;
    pmyDtlnHanldTran = tranfun;
}
// #endif
typedef struct _mgnt_audio_t
{
    uint32_t rdev;
    uint32_t lport;
    uint32_t rport;
    uint8_t trandirection;

    int ai_stream_id;
    int send_seq;
    unsigned int send_timestamp;
    void * pushtrans ;	//推流句柄
    void * pulltrans ;	//拉流句柄

    //发送包大小， 用于发送音频的数据包
    char send_buffer[SEND_AUDIO_BUF_SIZE+sizeof(media_frame)];
    //用于存放待g711编码的PCM数据
    char pcm_buffer[SEND_AUDIO_BUF_SIZE*2];
    struct circlebuf record_packet;
    pthread_mutex_t record_mutex;

    int ao_stream_id;
    struct circlebuf play_packet;
    pthread_mutex_t play_mutex;
    //超过PLC单包大小，用于存放g711解封装后的数据
    short pcm_ao_buffer[2048];

    FILE *pfd_read;
    FILE *pfd_ai;
    FILE *pfd_ao;

    TransAudioCB audioOutcb;
    void *puseraodata;

    TransAudioCB audioIncb;
    void *puseraidata;
    // pthread_t ca_thread;
	// volatile bool quit;

    uint32_t play_volume;

#ifdef AUDIO_DTLN_NN
    void *pdtlnhandle;
    unsigned int dtln_need_count;
    sem_t  aec_work_sem;
    pthread_t aec_thread;
    bool aec_thread_run;
    bool aec_run_enable;

    //等待被回声处理的数据包
    struct circlebuf dtln_packet;
    pthread_mutex_t dtln_mutex;

    struct circlebuf lpb_packet;
    pthread_mutex_t lpb_mutex;

    FILE *pfd_dtln;
    FILE *pfd_lpb;
#endif
} mgnt_audio_t;



int AudioInCbFunc(char *DataBuffer, int DataSize, void *pContext)
{
    mgnt_audio_t *pmgnt_audio = (mgnt_audio_t*)pContext;
    //printf("pmgnt_audio:%p AudioInCbFunc DataSize:%d\n", pmgnt_audio, DataSize);
    if(pmgnt_audio)
    {
        if(pmgnt_audio->pfd_read)
        {
            if(fread(DataBuffer, 1, DataSize, pmgnt_audio->pfd_read) != DataSize)
            {
                printf("AudioInCbFunc read fail\n");
                fseek(pmgnt_audio->pfd_read, 0, SEEK_SET);
            }
        }
        if(pmgnt_audio->pfd_ai)
            fwrite(DataBuffer, 1, DataSize, pmgnt_audio->pfd_ai);
        LOCAL_LOCK();
        if(pmgnt_audio->audioIncb)
            pmgnt_audio->audioIncb(pmgnt_audio->puseraidata, (uint8_t *)DataBuffer, DataSize);
        LOCAL_UNLOCK();
#if 1
#ifdef AUDIO_DTLN_NN
        if(pmgnt_audio->pdtlnhandle && pmyDtlnHanldTran)
        {
            pthread_mutex_lock(&pmgnt_audio->dtln_mutex);
            circlebuf_push_back(&pmgnt_audio->dtln_packet, DataBuffer, DataSize);
            pthread_mutex_unlock(&pmgnt_audio->dtln_mutex);
#if 1
            //采集到3倍处理的数据，触发三次信号量
            sem_post(&pmgnt_audio->aec_work_sem);
            // sem_post(&pmgnt_audio->aec_work_sem);
            // sem_post(&pmgnt_audio->aec_work_sem);
            pmgnt_audio->dtln_need_count += DataSize;
#else
            char tmpbuf[1024]={0};
            char tmplpbbuf[1024]={0};
            char tmpoutbuf[1024]={0};
            while (pmgnt_audio->dtln_packet.size >= DTLN_NEED_LENGTH)
            {
                circlebuf_pop_front(&pmgnt_audio->dtln_packet, tmpbuf, DTLN_NEED_LENGTH);

                pthread_mutex_lock(&pmgnt_audio->lpb_mutex);
                if(pmgnt_audio->lpb_packet.size >= DTLN_NEED_LENGTH)
                {
                    memset(tmplpbbuf, 0, sizeof(tmplpbbuf));
                    circlebuf_pop_front(&pmgnt_audio->lpb_packet, tmplpbbuf, DTLN_NEED_LENGTH);
                }
                pthread_mutex_unlock(&pmgnt_audio->lpb_mutex);

                if(pmyDtlnHanldTran(pmgnt_audio->pdtlnhandle, (short *)tmpbuf, (short *)tmplpbbuf, DTLN_NEED_LENGTH/2, (short *)tmpoutbuf) > 0)
                {
                    circlebuf_push_back(&pmgnt_audio->record_packet, tmpoutbuf, DTLN_NEED_LENGTH);
                    if(pmgnt_audio->pfd_dtln)
                    {
                        fwrite(tmpoutbuf, 1, DTLN_NEED_LENGTH, pmgnt_audio->pfd_dtln);
                    }
                    if(pmgnt_audio->pfd_lpb)
                    {
                        fwrite(tmplpbbuf, 1, DTLN_NEED_LENGTH, pmgnt_audio->pfd_lpb);
                    }
                }
            }
#endif
        }
        else{
            circlebuf_push_back(&pmgnt_audio->record_packet, DataBuffer, DataSize);
        }
#else
        circlebuf_push_back(&pmgnt_audio->record_packet, DataBuffer, DataSize);
#endif //AUDIO_DTLN_NN

#ifndef AUDIO_DTLN_NN
        media_frame * ppacket = (media_frame*)pmgnt_audio->send_buffer;//(media_frame*)bmalloc(sizeof(media_frame) + audio_size);
        //memset(ppacket, 0, sizeof(media_frame) + audio_size);
        // ppacket->check = 0xabcd1234;
		// ppacket->bkey = 1;
		// ppacket->seq = pmgnt_audio->send_seq++;
		// ppacket->timestamp = pmgnt_audio->send_timestamp = pmgnt_audio->send_timestamp + BLOCKTIME;
		// ppacket->streamid = 1;


#if AUDIO_ENC_BY_G711
#if 1
        pthread_mutex_lock(&pmgnt_audio->record_mutex);
#if AUDIO_DATA_ENC_G711U
        if (pmgnt_audio->record_packet.size >= SEND_AUDIO_BUF_SIZE*2)
#else
        if (pmgnt_audio->record_packet.size >= SEND_AUDIO_BUF_SIZE)
#endif  //AUDIO_DATA_ENC_G711U
        {
            ppacket->check = 0xabcd1234;
            ppacket->bkey = 1;
            ppacket->seq = pmgnt_audio->send_seq++;
            ppacket->timestamp = pmgnt_audio->send_timestamp = pmgnt_audio->send_timestamp + BLOCKTIME;
            ppacket->streamid = 1;
#if AUDIO_DATA_ENC_G711U
            circlebuf_pop_front(&pmgnt_audio->record_packet, pmgnt_audio->pcm_buffer, SEND_AUDIO_BUF_SIZE*2);
#else
            circlebuf_pop_front(&pmgnt_audio->record_packet, pmgnt_audio->pcm_buffer, SEND_AUDIO_BUF_SIZE);
#endif  //AUDIO_DATA_ENC_G711U
            short * p_pcm = (short*)pmgnt_audio->pcm_buffer;
#if AUDIO_DATA_ENC_G711U
            int i = 0;
            // pcm转成ulaw
            for (i = 0; i < SEND_AUDIO_BUF_SIZE; ++i)
            {
                ppacket->data[i] = s16_to_ulaw(p_pcm[i]);
            }
#else
            memcpy(ppacket->data, p_pcm, SEND_AUDIO_BUF_SIZE);
#endif //AUDIO_DATA_ENC_G711U
            transpush_send(pmgnt_audio->pushtrans,(unsigned char*)ppacket, SEND_AUDIO_BUF_SIZE + sizeof(media_frame), ppacket->bkey);
        }
        else{
            //LOGW(" Audio_In_CbFunc record_packet.size:%d SEND_AUDIO_BUF_SIZE*2:%d", pmgnt_audio->record_packet.size, SEND_AUDIO_BUF_SIZE*2);
        }
        pthread_mutex_unlock(&pmgnt_audio->record_mutex);
#endif //1
#else
        if (pmgnt_audio->record_packet.size >= SEND_AUDIO_BUF_SIZE)
        {
            ppacket->check = 0xabcd1234;
            ppacket->bkey = 1;
            ppacket->seq = pmgnt_audio->send_seq++;
            ppacket->timestamp = pmgnt_audio->send_timestamp = pmgnt_audio->send_timestamp + BLOCKTIME;
            ppacket->streamid = 1;
            circlebuf_pop_front(&pmgnt_audio->record_packet, ppacket->data, SEND_AUDIO_BUF_SIZE);
            transpush_send(pmgnt_audio->pushtrans,(unsigned char*)ppacket, SEND_AUDIO_BUF_SIZE + sizeof(media_frame), ppacket->bkey);
        }
        //memcpy(ppacket->data, DataBuffer, DataSize);
        //mediatrans_send(0,ppacket,DataSize + sizeof(media_frame));
        //transpush_send(pmgnt_audio->pushtrans,(unsigned char*)ppacket,DataSize + sizeof(media_frame),ppacket->bkey);
#endif //AUDIO_ENC_BY_G711
#endif  //AUDIO_DTLN_NN
        //bfree(ppacket);
#endif //1
    }

    return DataSize;
}

void Audio_Record_start(void*param)
{
    mgnt_audio_t *pmgnt_audio = (mgnt_audio_t*)param;
    if(pmgnt_audio == NULL)
    {
        printf("Audio_Record_start pmgnt_audio is NULL\n");
        return ;
    }
    int stream_id = 0;
    int Channels = SAMPLECHN;
	int SampleBits = SAMPLEBITS;
	int SampleRate = SAMPLERATE;
	int Blocks = RECORDBLOCKS;
	int BlockTime = BLOCKTIME;
	int SamplesPerBlock = SampleRate * BlockTime / 1000;//450;//SampleRate * BlockTime / 1000;
	//int BlockSize = SamplesPerBlock * Channels * (SampleBits / 8);

    // 打开录音设备
	stream_id = Audio_Recorder_Open();
    pthread_mutex_init(&pmgnt_audio->record_mutex, NULL);

    circlebuf_init(&pmgnt_audio->record_packet);

    circlebuf_reserve(&pmgnt_audio->record_packet, MAX_CACHE_AUDIO_SIZE);

	CHECK_VAL_GOTO_IF_FAIL(stream_id, "Audio_Recorder_Open fail");

	// 设置录音参数
	int ret = Audio_Recorder_SetDevice(stream_id, "default");

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_SetDevice fail");

	ret = Audio_Recorder_SetFormat(stream_id, Channels, SampleBits, SampleRate);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_SetFormat fail");

	ret = Audio_Recorder_SetCache(stream_id, Blocks, SamplesPerBlock);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_SetCache fail");

	ret = Audio_Recorder_SetMode(stream_id, false, AudioInCbFunc, pmgnt_audio);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_SetMode fail");

    int volume = 60;
//#if defind AUDIO_DTLN_NN AUDIO_NS_NN
#if defined(AUDIO_DTLN_NN) && !defined(AUDIO_NS_NN)
    //volume = 30;
    AudioCore_SetPlayVolume(30);
    AudioCore_SetRecordVolume(6);
#endif
	ret = Audio_Recorder_SetVolume(stream_id, volume);
#if AUDIO_AI
    Audio_Recorder_SetAINS(stream_id, true);
    AINS_Control(1, 1);
#endif
	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_SetVolume fail");

	ret = Audio_Recorder_Start(stream_id);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Recorder_Start fail");

    pmgnt_audio->ai_stream_id = stream_id;
//#ifndef AUDIO_DTLN_NN
#if !defined(AUDIO_DTLN_NN) || defined(AUDIO_NS_NN)
    AudioCore_SetAecMode(true);
#ifdef AUDIO_DTLN_NN
    pmgnt_audio->aec_run_enable = true;
#endif
#endif
    return ;
fail:

	if (stream_id != 0)
	{
		Audio_Recorder_Close(stream_id);
		stream_id = 0;
	}

    pthread_mutex_destroy(&pmgnt_audio->record_mutex);

	circlebuf_free(&pmgnt_audio->record_packet);
}

void Audio_Trans_Ai_Noise_onoff(void*param, bool onoff)
{
    mgnt_audio_t *pmgnt_audio = (mgnt_audio_t*)param;
    if(pmgnt_audio == NULL)
    {
        printf("Audio_Trans_Ai_Noise_onoff pmgnt_audio is NULL\n");
        return ;
    }
#if AUDIO_AI
    //Audio_Recorder_Stop(pmgnt_audio->ai_stream_id);
    //Audio_Recorder_SetAINS(pmgnt_audio->ai_stream_id, onoff);
    AINS_Control(onoff, onoff);
    //Audio_Recorder_Start(pmgnt_audio->ai_stream_id);
    LOGI("set AINNS:%d \n", onoff);
#elif defined(AUDIO_DTLN_NN) || defined(AUDIO_NS_NN)
    atomic_exchange_bool(&pmgnt_audio->aec_run_enable, onoff);
#endif
}


void Audio_Record_stop(void*param)
{
    mgnt_audio_t *pmgnt_audio = (mgnt_audio_t*)param;
    if(pmgnt_audio == NULL)
    {
        LOGE("Audio_Record_stop pmgnt_audio is NULL\n");
        return ;
    }
    if (pmgnt_audio->ai_stream_id != 0)
	{
#if AUDIO_AI
        Audio_Recorder_SetAINS(pmgnt_audio->ai_stream_id, false);
#endif
        Audio_Recorder_Stop(pmgnt_audio->ai_stream_id);
		Audio_Recorder_Close(pmgnt_audio->ai_stream_id);
// #ifndef AUDIO_DTLN_NN
        AudioCore_SetAecMode(false);
#if AUDIO_AI
        AINS_Control(1, 1);
#endif
// #endif
        pmgnt_audio->ai_stream_id = 0;
        circlebuf_free(&pmgnt_audio->record_packet);
        pthread_mutex_destroy(&pmgnt_audio->record_mutex);
	}
}

static int Audio_playback_cb(char *DataBuffer, int DataSize, void *pContext)
{
	mgnt_audio_t *mgnt = (mgnt_audio_t *)pContext;

	int size = 0;

    if(mgnt)
	{
		pthread_mutex_lock(&mgnt->play_mutex);

		if (mgnt->play_packet.size >= DataSize)
		{
			circlebuf_pop_front(&mgnt->play_packet, DataBuffer, DataSize);

			size = DataSize;

            //回调数据给上层
            LOCAL_LOCK();
            if(mgnt->audioOutcb)
                mgnt->audioOutcb(mgnt->puseraodata, (uint8_t *)DataBuffer, DataSize);
            LOCAL_UNLOCK();
		}
		else
		{
			memset(DataBuffer, 0, DataSize);

			// LOGE("valid_len:%d need %d\n", mgnt->stream->play_packet.size, DataSize);
		}

		pthread_mutex_unlock(&mgnt->play_mutex);
	}

	return size;
}

void Audio_Play_start(void*param)
{
    mgnt_audio_t *pmgnt_audio = (mgnt_audio_t*)param;
    if(pmgnt_audio == NULL)
    {
        printf("Audio_Play_start pmgnt_audio is NULL\n");
        return ;
    }
    int stream_id = 0;
	int Channels = SAMPLECHN;
	int SampleBits = SAMPLEBITS;
	int SampleRate = SAMPLERATE;
	int Blocks = PLAYBLOCKS;
	int BlockTime = BLOCKTIME;
	int SamplesPerBlock = SampleRate * BlockTime / 1000;

	// 打开放音设备
	stream_id = Audio_Player_Open();
    pthread_mutex_init(&pmgnt_audio->play_mutex, NULL);

    circlebuf_init(&pmgnt_audio->play_packet);

    circlebuf_reserve(&pmgnt_audio->play_packet, MAX_CACHE_AUDIO_SIZE);

	CHECK_VAL_GOTO_IF_FAIL(stream_id, "Audio_Player_Open fail");

	// 设置放音参数
	int ret = Audio_Player_SetDevice(stream_id, "default");

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_SetDevice fail");

	ret = Audio_Player_SetFormat(stream_id, Channels, SampleBits, SampleRate);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_SetFormat fail");

	ret = Audio_Player_SetCache(stream_id, Blocks, SamplesPerBlock);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_SetCache fail");

	ret = Audio_Player_SetMode(stream_id, false, Audio_playback_cb, pmgnt_audio);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_SetMode fail");

    ret = Audio_Player_SetVolume(stream_id, pmgnt_audio->play_volume);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_SetVolume fail");

	ret = Audio_Player_Start(stream_id);

	CHECK_VAL_GOTO_IF_FAIL(ret, "Audio_Player_Start fail");


    pmgnt_audio->ao_stream_id = stream_id;
	return ;

fail:

	if (pmgnt_audio->ao_stream_id != 0)
	{
		Audio_Player_Close(pmgnt_audio->ao_stream_id);
		pmgnt_audio->ao_stream_id = 0;
	}

    pthread_mutex_destroy(&pmgnt_audio->play_mutex);

	circlebuf_free(&pmgnt_audio->play_packet);


    return ;

}

void Audio_Play_stop(void*param)
{
    mgnt_audio_t *pmgnt_audio_out = (mgnt_audio_t*)param;
    if(pmgnt_audio_out == NULL)
    {
        printf("Audio_Play_stop pmgnt_audio_out is NULL\n");
        return ;
    }
    if (pmgnt_audio_out->ao_stream_id != 0)
	{
        Audio_Player_Stop(pmgnt_audio_out->ao_stream_id);
		Audio_Player_Close(pmgnt_audio_out->ao_stream_id);
		pmgnt_audio_out->ao_stream_id = 0;

        pthread_mutex_destroy(&pmgnt_audio_out->play_mutex);
	    circlebuf_free(&pmgnt_audio_out->play_packet);
	}
}

static void recv_audio_cb(void *p, unsigned char *lpdata, int dlen)
{
    mgnt_audio_t *pmgnt_audio = (mgnt_audio_t*)p;
	media_frame *pframe = (media_frame *)lpdata;
	//printf("Audio++++++++++++pmgnt_audio:%p = dlen:%d  streamid:%d\n", pmgnt_audio, dlen, pframe->streamid);
    if(pframe && pmgnt_audio->pfd_ao)
    {
        fwrite(pframe->data, 1, dlen-sizeof(media_frame), pmgnt_audio->pfd_ao);
    }

    if(pmgnt_audio)
    {
#if AUDIO_ENC_BY_G711
        int audio_size = dlen - sizeof(media_frame);
#if AUDIO_DATA_ENC_G711U
        int i = 0;
        for (i = 0; i < audio_size; ++i)
        {
            pmgnt_audio->pcm_ao_buffer[i] = ulaw_to_s16(pframe->data[i]);
        }
        pthread_mutex_lock(&pmgnt_audio->play_mutex);
        circlebuf_push_back(&pmgnt_audio->play_packet, pmgnt_audio->pcm_ao_buffer, audio_size*2);
        pthread_mutex_unlock(&pmgnt_audio->play_mutex);
#else
        pthread_mutex_lock(&pmgnt_audio->play_mutex);
        circlebuf_push_back(&pmgnt_audio->play_packet, pframe->data, audio_size);
        pthread_mutex_unlock(&pmgnt_audio->play_mutex);
#endif
#ifdef AUDIO_DTLN_NN
#ifndef AUDIO_NS_NN
        pthread_mutex_lock(&pmgnt_audio->lpb_mutex);
#if AUDIO_DATA_ENC_G711U
        circlebuf_push_back(&pmgnt_audio->lpb_packet, pmgnt_audio->pcm_ao_buffer, audio_size*2);
#else
        circlebuf_push_back(&pmgnt_audio->lpb_packet, pframe->data, audio_size);
#endif //#if AUDIO_DATA_ENC_G711U
        pthread_mutex_unlock(&pmgnt_audio->lpb_mutex);
#else
        //单降噪的在这里保存软回采数据
        if(pmgnt_audio->pfd_lpb)
        {
            fwrite(pmgnt_audio->pcm_ao_buffer, 1, audio_size*2, pmgnt_audio->pfd_lpb);
        }
#endif //AUDIO_NS_NN
#endif
#else
        pthread_mutex_lock(&pmgnt_audio->play_mutex);
        circlebuf_push_back(&pmgnt_audio->play_packet, pframe->data, dlen-sizeof(media_frame));
        pthread_mutex_unlock(&pmgnt_audio->play_mutex);
#endif
    }
}
#ifdef AUDIO_DTLN_NN
static void *aec_thread_func(void *args)
{
    LOGI("dtln_thread_func enter\n");
    mgnt_audio_t * pmgnt_audio = (mgnt_audio_t*)args;
#if 1
    pthread_attr_t attr;

    struct sched_param sp;
    int rs = pthread_getattr_np(pthread_self(), &attr);

    if(rs != 0)
        LOGW("pthread_getattr_np fail");

    rs = SCHED_FIFO;

    // 设置线程调度策略为SCHED_FIFO
    pthread_attr_setschedpolicy(&attr, rs);

    // 实时优先级rt_priority(1-99)
    sp.sched_priority = 99;
    pthread_attr_setschedparam(&attr, &sp);

    pthread_attr_destroy(&attr);
#endif

    char tmpbuf[1024] = {0};
    char tmplpbbuf[1024]={0};
    char tmpoutbuf[1024]={0};
    unsigned int rindex = 0;
    bool benough_data = false;
    while(pmgnt_audio->aec_thread_run)
    {
        benough_data = false;
        //等待信号
        sem_wait(&pmgnt_audio->aec_work_sem);


        do
        {
            benough_data = false;
            pthread_mutex_lock(&pmgnt_audio->dtln_mutex);
            if(pmgnt_audio->dtln_packet.size >= DTLN_NEED_LENGTH)
            {
                memset(tmpbuf, 0, sizeof(tmpbuf));
                memset(tmplpbbuf, 0, sizeof(tmplpbbuf));
                memset(tmpoutbuf, 0, sizeof(tmpoutbuf));
                circlebuf_pop_front(&pmgnt_audio->dtln_packet, tmpbuf, DTLN_NEED_LENGTH);
#ifndef AUDIO_NS_NN
                pthread_mutex_lock(&pmgnt_audio->lpb_mutex);
                if(pmgnt_audio->lpb_packet.size >= DTLN_NEED_LENGTH)
                {
                    memset(tmplpbbuf, 0, sizeof(tmplpbbuf));
                    circlebuf_pop_front(&pmgnt_audio->lpb_packet, tmplpbbuf, DTLN_NEED_LENGTH);
                }
                pthread_mutex_unlock(&pmgnt_audio->lpb_mutex);
#endif //#ifndef AUDIO_NS_NN
                benough_data = true;
                rindex+=DTLN_NEED_LENGTH;
            }
            pthread_mutex_unlock(&pmgnt_audio->dtln_mutex);
            //不够数据处理就退出循环
            if(!benough_data)
                break;

            //printf("dtln_thread_func recv_audio_cb rindex:%u benough_data:%d pmgnt_audio->pdtlnhandle:%p\n", rindex, benough_data, pmgnt_audio->pdtlnhandle);
            if(benough_data && pmgnt_audio->pdtlnhandle)
            {
                //得到大于0的数据才是成功处理后的
                if(atomic_load_bool(&pmgnt_audio->aec_run_enable) && pmyDtlnHanldTran(pmgnt_audio->pdtlnhandle, (short *)tmpbuf, (short *)tmplpbbuf, DTLN_NEED_LENGTH, (short *)tmpoutbuf) > 0)
                {
                    //pthread_mutex_lock(&pmgnt_audio->record_mutex);
                    circlebuf_push_back(&pmgnt_audio->record_packet, tmpoutbuf, DTLN_NEED_LENGTH);
                    //pthread_mutex_unlock(&pmgnt_audio->record_mutex);
                    if(pmgnt_audio->pfd_dtln)
                    {
                        fwrite(tmpoutbuf, 1, DTLN_NEED_LENGTH, pmgnt_audio->pfd_dtln);
                    }
                }
                else{
                    printf("dtln_thread_func pmyDtlnHanldTran fail\n");
                    circlebuf_push_back(&pmgnt_audio->record_packet, tmpbuf, DTLN_NEED_LENGTH);
                }

                // if(pfd_ai)
                //     fwrite(tmpbuf, 1, DTLN_NEED_LENGTH, pfd_ai);
#ifndef AUDIO_NS_NN
                if(pmgnt_audio->pfd_lpb)
                {
                    fwrite(tmplpbbuf, 1, DTLN_NEED_LENGTH, pmgnt_audio->pfd_lpb);
                }
#endif //#ifndef AUDIO_NS_NN
            }

            //============
            //处理完，够数据就发送
            do{
#if AUDIO_DATA_ENC_G711U
                if (pmgnt_audio->record_packet.size >= SEND_AUDIO_BUF_SIZE*2)
#else
                if (pmgnt_audio->record_packet.size >= SEND_AUDIO_BUF_SIZE)
#endif //#if AUDIO_DATA_ENC_G711U
                {
                    media_frame * ppacket = (media_frame*)pmgnt_audio->send_buffer;
                    ppacket->check = 0xabcd1234;
                    ppacket->bkey = 1;
                    ppacket->seq = pmgnt_audio->send_seq++;
                    ppacket->timestamp = pmgnt_audio->send_timestamp = pmgnt_audio->send_timestamp + BLOCKTIME;
                    ppacket->streamid = 1;
                    //circlebuf_pop_front(&pmgnt_audio->record_packet, pmgnt_audio->pcm_buffer, SEND_AUDIO_BUF_SIZE*2);
                    //circlebuf_pop_front(&pmgnt_audio->record_packet, pmgnt_audio->pcm_buffer, SEND_AUDIO_BUF_SIZE);
                    short * p_pcm = (short*)pmgnt_audio->pcm_buffer;
#if AUDIO_DATA_ENC_G711U
                    circlebuf_pop_front(&pmgnt_audio->record_packet, pmgnt_audio->pcm_buffer, SEND_AUDIO_BUF_SIZE*2);
                    int i = 0;
                    // pcm转成ulaw
                    for (i = 0; i < SEND_AUDIO_BUF_SIZE; ++i)
                    {
                        ppacket->data[i] = s16_to_ulaw(p_pcm[i]);
                    }
#else
                    circlebuf_pop_front(&pmgnt_audio->record_packet, pmgnt_audio->pcm_buffer, SEND_AUDIO_BUF_SIZE);
                    memcpy(ppacket->data, p_pcm, SEND_AUDIO_BUF_SIZE);
#endif //AUDIO_DATA_ENC_G711U
                    transpush_send(pmgnt_audio->pushtrans,(unsigned char*)ppacket, SEND_AUDIO_BUF_SIZE + sizeof(media_frame), ppacket->bkey);
                }
                else{
                    //LOGW(" Audio_In_CbFunc record_packet.size:%d SEND_AUDIO_BUF_SIZE*2:%d", pmgnt_audio->record_packet.size, SEND_AUDIO_BUF_SIZE*2);
                    break;
                }
            }while(1);
        }while(1);
    }

    LOGI("dtln_thread_func exit, rindex:%u\n", rindex);
    return NULL;
}
#endif // #ifdef AUDIO_DTLN_NN
#if AUDIO_DATA_SVEE_FILE
static int aec_index = 0;
#endif
DPLAN_PUBLIC_API void *PlcAudioTrans_Start(uint32_t lport, uint32_t rdev, uint32_t rport, uint8_t trandirection, uint32_t play_volume)
{
    void *pret = NULL;
    printf("PlcAudioTrans lport:%d redev:%x rport:%d trandirection:%x\n", lport, rdev, rport, trandirection);

    mgnt_audio_t * pmgnt_audio = bmalloc(sizeof(mgnt_audio_t));
    memset(pmgnt_audio, 0, sizeof(mgnt_audio_t));
    pmgnt_audio->rdev = rdev;
    pmgnt_audio->lport = lport;
    pmgnt_audio->rport = rport;
    pmgnt_audio->trandirection = trandirection;
    pmgnt_audio->send_timestamp = 0;
    pmgnt_audio->play_volume = play_volume;
#if AUDIO_DATA_SVEE_FILE
    char savefilename[256] = {0};
#endif
#ifdef AUDIO_DTLN_NN
    if(pmyDtlnHanldCreate && pmyDtlnHanldDestroy && pmyDtlnHanldTran)
    {
        pmgnt_audio->pdtlnhandle = pmyDtlnHanldCreate();
        pthread_mutex_init(&pmgnt_audio->dtln_mutex, NULL);
        circlebuf_init(&pmgnt_audio->dtln_packet);
        circlebuf_reserve(&pmgnt_audio->dtln_packet, 1024);

        pthread_mutex_init(&pmgnt_audio->lpb_mutex, NULL);
        circlebuf_init(&pmgnt_audio->lpb_packet);
        circlebuf_reserve(&pmgnt_audio->lpb_packet, 1024);
#if defined(AUDIO_DTLN_NN) && !defined(AUDIO_NS_NN)
        //提前塞一点数据进去，软回采的数据不统一
        char tmpbuf[1024] = {0};
        circlebuf_push_back(&pmgnt_audio->lpb_packet, tmpbuf, 1024);
#endif //AUDIO_NS_NN

        sem_init(&pmgnt_audio->aec_work_sem, 0, 0); //无名信号量
        pmgnt_audio->aec_thread_run = true;
        if(0 != pthread_create(&pmgnt_audio->aec_thread, NULL, aec_thread_func, pmgnt_audio))
        {
            pmgnt_audio->aec_thread_run = false;
        }

        if (access("/tmp/sdcard/dtnl", F_OK) == 0)
        {
            struct tm ltime = {0};
			time_get_local_time(&ltime);
            char dtlnfile[128] = {0};
            char lpbfile[128] = {0};

            snprintf(dtlnfile, sizeof(dtlnfile), "/tmp/sdcard/dtlnfile/dtln_%04d%02d%02d_%02d%02d%02d.pcm",
                ltime.tm_year, ltime.tm_mon, ltime.tm_mday,
                ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
            snprintf(lpbfile, sizeof(lpbfile), "/tmp/sdcard/dtlnfile/lpb_%04d%02d%02d_%02d%02d%02d.pcm",
                ltime.tm_year, ltime.tm_mon, ltime.tm_mday,
                ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
            pmgnt_audio->pfd_dtln = fopen(dtlnfile, "wb");
            pmgnt_audio->pfd_lpb = fopen(lpbfile, "wb");
        }
        else
        {
#if AUDIO_DATA_SVEE_FILE
            if(aec_index>5)
                aec_index = 0;
            sprintf(savefilename, "./aec/dtln_%02d.pcm", aec_index);
            pmgnt_audio->pfd_dtln = fopen(savefilename, "wb");
            sprintf(savefilename, "./aec/lpb_%02d.pcm", aec_index);
            pmgnt_audio->pfd_lpb = fopen(savefilename, "wb");
#endif
        }
    }
#endif
    if (trandirection & AUDIO_TRAN_RECV)
    {
        //pmgnt_audio->pfd_ao = fopen("./local_tran_ao.pcm", "wb");
        /* code */
        Audio_Play_start(pmgnt_audio);
        pmgnt_audio->pulltrans  = transpull_create(lport, rdev, 1234, rdev, recv_audio_cb, pmgnt_audio, false);
    }

    if (trandirection & AUDIO_TRAN_SEND)
    {
        //pmgnt_audio->pfd_read = fopen("../../../test.wav", "rb");
        if (access("/tmp/sdcard/dtnl", F_OK) == 0)
        {
            struct tm ltime = {0};
            time_get_local_time(&ltime);
            char readfile[128] = {0};
            snprintf(readfile, sizeof(readfile), "/tmp/sdcard/dtlnfile/ai_%04d%02d%02d_%02d%02d%02d.pcm",
                ltime.tm_year, ltime.tm_mon, ltime.tm_mday,
                ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
            pmgnt_audio->pfd_ai = fopen(readfile, "wb");
        }
#if AUDIO_DATA_SVEE_FILE
        else{
            sprintf(savefilename, "./aec/mic_%02d.pcm", aec_index);
            pmgnt_audio->pfd_ai = fopen(savefilename, "wb");
            aec_index++;
            if(aec_index>5)
                aec_index = 0;
        }
#endif

        tran_push_info_s push_info={0};
        memset(&push_info, 0, sizeof(tran_push_info_s));
        push_info.lport = 10101;
        push_info.rip=0;
        push_info.rport = 0;
        push_info.plc_dev = rdev;
        push_info.plc_port = rport;
        push_info.bplc_tran = true;
        push_info.bretrans = false;
        pmgnt_audio->pushtrans = transpush_create(&push_info);
        /* code */
        Audio_Record_start(pmgnt_audio);
    }


#ifndef __GLIBC__
    //spi_plc_mgnt_audio_tran(rdev, rdev);
#endif
    pret = pmgnt_audio;

    return pret;
}

DPLAN_PUBLIC_API void PlcAudioTrans_SetAoVolume(void*param, int volume)
{
    mgnt_audio_t *pmgnt_audio = (mgnt_audio_t*)param;
    if(pmgnt_audio)
    {
        LOGI("Set AO Vol:%d \n", volume);
        Audio_Player_SetVolume(pmgnt_audio->ao_stream_id, volume);
    }
}

int plc_audio_frame_cb_set(void*param, TransAudioCB paocb, void *pusraodata, TransAudioCB paicb, void *pusraidata)
{
    mgnt_audio_t *pmgnt_audio = (mgnt_audio_t *)param;
    if(pmgnt_audio)
    {
        LOCAL_LOCK();
        pmgnt_audio->audioOutcb = paocb;
        pmgnt_audio->puseraodata = pusraodata;

        pmgnt_audio->audioIncb = paicb;
        pmgnt_audio->puseraidata = pusraidata;
        LOCAL_UNLOCK();
    }

    return 0;
}

DPLAN_PUBLIC_API void PlcAudioTrans_Stop(void*param)
{
    mgnt_audio_t *pmgnt_audio = (mgnt_audio_t*)param;
    if(pmgnt_audio == NULL)
    {
        printf("Audio_Record_stop pmgnt_audio is NULL\n");
        return ;
    }

    if (pmgnt_audio->pulltrans)
    {
        /* code */
        transpull_destroy(pmgnt_audio->pulltrans);
        pmgnt_audio->pulltrans = NULL;
    }
#ifdef AUDIO_DTLN_NN
    if(pmgnt_audio->aec_thread_run)
    {
        printf("aec_work_sem pmgnt_audio->aec_thread_run:%d\n", pmgnt_audio->aec_thread_run);
        pmgnt_audio->aec_thread_run = false;
        sem_post(&pmgnt_audio->aec_work_sem);
        pthread_join(pmgnt_audio->aec_thread, NULL);
        pmyDtlnHanldDestroy(pmgnt_audio->pdtlnhandle);
        usleep(10*1000);
    }
#endif
    Audio_Play_stop(pmgnt_audio);

    Audio_Record_stop(pmgnt_audio);

#ifdef AUDIO_DTLN_NN
    if(pmyDtlnHanldCreate && pmyDtlnHanldDestroy && pmyDtlnHanldTran)
    {
        sem_destroy(&pmgnt_audio->aec_work_sem);
        //关闭声音处理
        pmyDtlnHanldDestroy(pmgnt_audio->pdtlnhandle);

        pthread_mutex_destroy(&pmgnt_audio->dtln_mutex);
	    circlebuf_free(&pmgnt_audio->dtln_packet);

        pthread_mutex_destroy(&pmgnt_audio->lpb_mutex);
	    circlebuf_free(&pmgnt_audio->lpb_packet);

        if(pmgnt_audio->pfd_dtln)
            fclose(pmgnt_audio->pfd_dtln);
        pmgnt_audio->pfd_dtln = NULL;
        if(pmgnt_audio->pfd_lpb)
            fclose(pmgnt_audio->pfd_lpb);
        pmgnt_audio->pfd_lpb = NULL;
    }

    printf("================pmgnt_audio->dtln_need_count:%d ====================\n", pmgnt_audio->dtln_need_count);
    sync();
#endif

    if(pmgnt_audio->pushtrans){
        transpush_destroy(pmgnt_audio->pushtrans);
        pmgnt_audio->pushtrans = NULL;
    }

    if(pmgnt_audio->pfd_ai)
    {
        fclose(pmgnt_audio->pfd_ai);
        pmgnt_audio->pfd_ai = NULL;
    }
    if(pmgnt_audio->pfd_ao)
    {
        fclose(pmgnt_audio->pfd_ao);
        pmgnt_audio->pfd_ao = NULL;
    }
    if (pmgnt_audio->pfd_read)
    {
        fclose(pmgnt_audio->pfd_read);
        pmgnt_audio->pfd_read = NULL;
    }
    bfree(pmgnt_audio);
    LOGI("Audio tran end \n");
#ifndef __GLIBC__
    //spi_plc_mgnt_audio_tran(0, 0);
#endif
}