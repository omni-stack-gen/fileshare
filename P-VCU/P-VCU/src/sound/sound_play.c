#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <utils/wave_file_fmt.h>
// #include <utils/thread_helper.h>
#include <utils/mq.h>
#include <utils/bmem.h>
#include <utils/list.h>
#include <utils/atomic.h>
#include <utils/time_helper.h>

#include <periphery/gpio.h>
#include <periphery/gpio_port.h>

// #include <db/db.h>

#include <MP3dec.h>
#include <AudioCore.h>
#define LOG_TAG "sound-play"
#include <utils/log.h>
#include <dpbase.h>
#include <sound_play.h>

#define BLOCKTIME               (40)     //每个缓冲数据块的时长（毫秒）
#define PLAYBLOCKS              (4)      //播放最大缓冲数据块数
#define MAX_SOUND_TIPS_MESSAGE  (16)

#define GPIO_SPK_PIN            (GPIO_D7)
#define GPIO_SPK_EN_VALUE       (1)

enum file_type
{
	FILE_WAVE,
	FILE_MP3
};

typedef struct sound_play_ctx
{
	int id;
	struct list_head node;

	uint32_t times;
	uint32_t volume;
	uint32_t is_play;
	bool sync;

	int stream_id;
	char *block_buffer;
	int block_size;
	int block_time;
	int sleep_time;

	pthread_t thread_play;

	enum file_type type;

	union
	{
		struct
		{
			wave_hdr_t wave_hdr;
			FILE *fp;
			long data_pos;
		} wave;

		struct
		{
			MP3File mp3file;
		} mp3;
	} ctx;
} sound_play_ctx_t;

typedef struct sound_tips_msg
{
	char file_name[64];
	int volume;
} sound_tips_msg_t;

typedef struct sound_play_mgnt
{
	mq_t *mq;
	pthread_t thread;

	bool control_speaker;
	volatile long speaker_ref;
	gpio_t *speaker;

	pthread_mutex_t mutex;
	struct list_head play_list;

	pthread_mutex_t ring_mutex;
	bool b_ring_play; // 响铃播放标志
	int ring_channels; // 响铃播放通道
	int ring_bits; // 响铃播放位宽
	int ring_sample_rate; // 响铃播放采样率
	int ring_samples_per_block; // 响铃播放每块采样数
	int ring_block_size; // 响铃播放缓存块大小

	char *pbuf_ring; // 响铃播放缓存
	int ring_size; // 响铃播放缓存大小
	int ring_pos; // 响铃播放缓存当前位置
	int ring_play_id; // 响铃播放句柄
} sound_play_mgnt_t;

static sound_play_mgnt_t *sound_mgnt = NULL;

static void speaker_init(sound_play_mgnt_t *mgnt)
{
	gpio_config_t config = {0};

	config.direction = GPIO_DIR_OUT;

	mgnt->speaker = gpio_create();
#ifdef ENABLE_V1
	gpio_open(mgnt->speaker, GPIO_SPK_PIN, &config);
#elif defined(ENABLE_V6)
	gpio_open(mgnt->speaker, 7*8+6, &config);
#endif
	if (mgnt->control_speaker == false)
	{
		gpio_write(mgnt->speaker, GPIO_SPK_EN_VALUE);
	}
	else
	{
		gpio_write(mgnt->speaker, 1 - GPIO_SPK_EN_VALUE);
	}
}

static void speaker_en(sound_play_mgnt_t *mgnt, bool enable)
{
	if (mgnt->control_speaker)
	{
		if (enable)
		{
			atomic_inc_long(&mgnt->speaker_ref);

			if (mgnt->speaker_ref == 1)
			{
				gpio_write(mgnt->speaker, GPIO_SPK_EN_VALUE);
			}
		}
		else
		{
			if (mgnt->speaker_ref > 0)
			{
				if (atomic_dec_long(&mgnt->speaker_ref) == 0)
				{
					// 延时500毫秒在关功放，不然可能声音没播放完，功放已经关闭的现象
					DPSleep(500);

					gpio_write(mgnt->speaker, 1 - GPIO_SPK_EN_VALUE);
				}
			}
		}
	}
}

static void *wave_play_thread(void *arg)
{
	sound_play_ctx_t *ctx = (sound_play_ctx_t *)arg;

	speaker_en(sound_mgnt, true);

	if (Audio_Player_Start(ctx->stream_id) == false)
	{
		LOGE("Audio_Player_Start error\n");
		goto end;
	}

	while (ctx->is_play && ctx->times > 0)
	{
		uint32_t iread;

		iread = fread(ctx->block_buffer, 1, ctx->block_size, ctx->ctx.wave.fp);

		if (iread == 0)
		{
			if (ctx->times > 0)
			{
				ctx->times--;
				fseek(ctx->ctx.wave.fp, ctx->ctx.wave.data_pos, SEEK_SET);
				continue;
			}
			else
			{
				break;
			}
		}
		else if (iread < ctx->block_size)
		{
			memset(&ctx->block_buffer[iread], 0, ctx->block_size - iread);
		}

		if (Audio_Player_Write(ctx->stream_id, ctx->block_buffer, ctx->block_size, ctx->block_time * PLAYBLOCKS) == false)
		{
			LOGE("Audio_Player_Write false break loop \n");
			break;
		}
	}

end:

	if (ctx->stream_id != 0)
	{
		Audio_Player_Stop(ctx->stream_id);
		Audio_Player_Close(ctx->stream_id);
		ctx->stream_id = 0;
	}

	fclose(ctx->ctx.wave.fp);

	speaker_en(sound_mgnt, false);

	return NULL;
}

static void *mp3_play_thread(void *arg)
{
	sound_play_ctx_t *ctx = (sound_play_ctx_t *)arg;

	int iret = 0, nsamples = 0, cur_time = 0;

	speaker_en(sound_mgnt, true);

	if (Audio_Player_Start(ctx->stream_id) == false)
	{
		LOGE("Audio_Player_Start error \n");
		goto end;
	}

	while (ctx->is_play && ctx->times > 0)
	{
		iret = MP3File_Decode(ctx->ctx.mp3.mp3file, (short *)ctx->block_buffer, &nsamples, &cur_time);

		if (iret == MP3DEC_ERR_NOMOREDATA)
		{
			if (ctx->times > 0)
			{
				ctx->times--;
				MP3File_Seek(ctx->ctx.mp3.mp3file, 0, &cur_time);
				continue;
			}
			else
			{
				break;
			}
		}
		else if (iret != MP3DEC_ERR_NONE)
		{
			LOGE("MP3 decoder fatal inner error ! \n");
			DPSleep(ctx->sleep_time);
			break;
		}

		if (Audio_Player_Write(ctx->stream_id, ctx->block_buffer, ctx->block_size, ctx->block_time * PLAYBLOCKS) == false)
		{
			LOGE("Audio_Player_Write false break loop \n");
			break;
		}
	}

end:

	if (ctx->stream_id != 0)
	{
		Audio_Player_Stop(ctx->stream_id);
		Audio_Player_Close(ctx->stream_id);
		ctx->stream_id = 0;
	}

	MP3File_Close(ctx->ctx.mp3.mp3file);

	speaker_en(sound_mgnt, false);

	return NULL;
}

int sound_play_start(char *file_name, uint32_t times, uint32_t volume, bool sync)
{
	if (!sound_mgnt || !file_name || !volume || !times)
	{
		return 0;
	}

	FILE *fp = NULL;

	wave_hdr_t wave_hdr;

	MP3File mp3file;

	AudioInfo *mp3info = NULL;

	enum file_type ftype;

	size_t length = strlen(file_name);

	int samples_pre_block = 0;

	uint32_t block_size = 0;

	uint32_t total_size = 0;

	if (length < 5)
	{
		return 0;
	}

	memset(&wave_hdr, 0, sizeof(wave_hdr));

	memset(&mp3file, 0, sizeof(mp3file));

	if (strcmp(".wav", &file_name[length - 4]) == 0)
	{
		ftype = FILE_WAVE;
	}
	else if (strcmp(".mp3", &file_name[length - 4]) == 0)
	{
		ftype = FILE_MP3;
	}
	else
	{
		return 0;
	}

	if (ftype == FILE_WAVE)
	{
		fp = fopen(file_name, "rb");

		if (!fp)
		{
			LOGE("Open file error:%s \n", file_name);
			return 0;
		}

		if (!read_wave_info(&fp, &wave_hdr))
		{
			LOGE("%s read_wave_info fail \n", file_name);
			goto error;
		}

		samples_pre_block = BLOCKTIME * wave_hdr.sample_rate / 1000;

		block_size = samples_pre_block * wave_hdr.channel * wave_hdr.bits_per_sample / 8;
	}
	else if (ftype == FILE_MP3)
	{
		int ret = MP3File_Open(&mp3file, file_name);

		if (ret != MP3DEC_ERR_NONE)
		{
			LOGE("Open file error:%s \n", file_name);
			goto error;
		}

		mp3info = (AudioInfo *)mp3file;

		samples_pre_block = (mp3info->SampleRate < 32000 ? 576 : 1152);

		block_size = samples_pre_block * mp3info->ChannelNumber * (mp3info->SampleBit / 8);
	}

	total_size = sizeof(sound_play_ctx_t) + block_size;

	sound_play_ctx_t *ctx = (sound_play_ctx_t *)bmalloc(total_size);

	if (!ctx)
	{
		goto error;
	}

	memset(ctx, 0, total_size);

	ctx->id = (int)ctx;
	ctx->volume = volume;
	ctx->type = ftype;
	ctx->times = times;
	ctx->is_play = 1;
	ctx->block_buffer = (char *)(ctx + 1);
	ctx->sync = sync;

	if (ftype == FILE_WAVE)
	{
		memcpy(&ctx->ctx.wave.wave_hdr, &wave_hdr, sizeof(wave_hdr));

		ctx->ctx.wave.fp = fp;
		ctx->ctx.wave.data_pos = ftell(fp);

		ctx->block_time = BLOCKTIME;
		ctx->block_size = block_size;

		ctx->stream_id = Audio_Player_Open();

		Audio_Player_SetDevice(ctx->stream_id, "default");
		Audio_Player_SetFormat(ctx->stream_id, wave_hdr.channel, wave_hdr.bits_per_sample, wave_hdr.sample_rate);
		Audio_Player_SetCache(ctx->stream_id, PLAYBLOCKS, samples_pre_block);
		Audio_Player_SetMode(ctx->stream_id, true, NULL, NULL);
		Audio_Player_SetVolume(ctx->stream_id, ctx->volume);

		if (sync)
		{
			wave_play_thread(ctx);

			bfree(ctx);

			ctx = NULL;
		}
		else
		{
			pthread_create(&ctx->thread_play, NULL, wave_play_thread, ctx);

			pthread_mutex_lock(&sound_mgnt->mutex);

			list_add_tail(&ctx->node, &sound_mgnt->play_list);

			pthread_mutex_unlock(&sound_mgnt->mutex);
		}
	}
	else
	{
		ctx->ctx.mp3.mp3file = mp3file;

		ctx->block_size = block_size;
		ctx->block_time = (samples_pre_block * 1000) / mp3info->SampleRate;
		ctx->sleep_time = (float)samples_pre_block / mp3info->SampleRate * 8 * 1000;

		ctx->stream_id = Audio_Player_Open();

		Audio_Player_SetDevice(ctx->stream_id, "default");
		Audio_Player_SetFormat(ctx->stream_id, mp3info->ChannelNumber, mp3info->SampleBit, mp3info->SampleRate);
		Audio_Player_SetCache(ctx->stream_id, PLAYBLOCKS, samples_pre_block);
		Audio_Player_SetMode(ctx->stream_id, true, NULL, NULL);
		Audio_Player_SetVolume(ctx->stream_id, ctx->volume);

		if (sync)
		{
			mp3_play_thread(ctx);

			bfree(ctx);

			ctx = NULL;
		}
		else
		{
			pthread_create(&ctx->thread_play, NULL, mp3_play_thread, ctx);

			pthread_mutex_lock(&sound_mgnt->mutex);

			list_add_tail(&ctx->node, &sound_mgnt->play_list);

			pthread_mutex_unlock(&sound_mgnt->mutex);
		}
	}

	return (int)ctx;

error:

	if (fp)
	{
		fclose(fp);
		fp = NULL;
	}

	return 0;
}

int sound_play_set_volume(int play_id, int volume)
{
	sound_play_ctx_t *pos = NULL;

	sound_play_ctx_t *next = NULL;

	pthread_mutex_lock(&sound_mgnt->mutex);

	if (!list_empty_careful(&sound_mgnt->play_list))
	{
		list_for_each_entry_safe(pos, next, &sound_mgnt->play_list, node)
		{
			if (pos->id == play_id)
			{
				if (pos->stream_id != 0)
				{
					Audio_Player_SetVolume(pos->stream_id, volume);
				}

				break;
			}
		}
	}

	pthread_mutex_unlock(&sound_mgnt->mutex);

	return 0;
}

int sound_play_stop(int play_id)
{
	sound_play_ctx_t *ctx = NULL;

	sound_play_ctx_t *next = NULL;

	pthread_mutex_lock(&sound_mgnt->mutex);

	if (!list_empty_careful(&sound_mgnt->play_list))
	{
		list_for_each_entry_safe(ctx, next, &sound_mgnt->play_list, node)
		{
			if (ctx->id == play_id)
			{
				ctx->is_play = 0;

				pthread_join(ctx->thread_play, NULL);

				list_del(&ctx->node);

				bfree(ctx);
				break;
			}
		}
	}

	pthread_mutex_unlock(&sound_mgnt->mutex);

	return 0;
}

static void *sound_tips_task(void *args)
{
	sound_play_mgnt_t *mgnt = (sound_play_mgnt_t *)args;

	while (1)
	{
		sound_tips_msg_t msg;

		if (0 == mq_recv(mgnt->mq, &msg, sizeof(sound_tips_msg_t), MQ_MAX_TIMEOUT))
		{
			sound_play_start(msg.file_name, 1, msg.volume, true);
		}
	}

	return NULL;
}

int sound_play_mgnt_init(bool control_speaker)
{
	int speaker_volume = 100;

	if (sound_mgnt == NULL)
	{
		sound_mgnt = bmalloc(sizeof(sound_play_mgnt_t));

		memset(sound_mgnt, 0, sizeof(sound_play_mgnt_t));

		pthread_mutex_init(&sound_mgnt->mutex, NULL);

		INIT_LIST_HEAD(&sound_mgnt->play_list);

		sound_mgnt->mq = mq_create("sound_play_tips", sizeof(sound_tips_msg_t), MAX_SOUND_TIPS_MESSAGE);

		sound_mgnt->control_speaker = control_speaker;

		speaker_init(sound_mgnt);

		//db_sys_param_get("speaker_volume", &speaker_volume, sizeof(speaker_volume));
		//单独播放铃声
		if(1)
		{
			unsigned int last_time = time_relative_ms();
			pthread_mutex_init(&sound_mgnt->ring_mutex, NULL);

			MP3File mp3file;
			int ret = MP3File_Open(&mp3file, RING_MP3);

			if (ret == MP3DEC_ERR_NONE)
			{
				AudioInfo *mp3info = (AudioInfo *)mp3file;

				LOGD("Open file success, SampleRate = %d, ChannelNumber = %d, SampleBit = %d TotalTime:%d \n",
						mp3info->SampleRate, mp3info->ChannelNumber, mp3info->SampleBit, mp3info->TotalTime);
				//获取铃声文件的信息，用于设置播放参数
				sound_mgnt->ring_channels = mp3info->ChannelNumber;
				sound_mgnt->ring_bits = mp3info->SampleBit;
				sound_mgnt->ring_sample_rate = mp3info->SampleRate;
				sound_mgnt->ring_samples_per_block = (mp3info->SampleRate < 32000 ? 576 : 1152);
				sound_mgnt->ring_block_size = sound_mgnt->ring_samples_per_block * mp3info->ChannelNumber * (mp3info->SampleBit / 8);

				unsigned int ring_buf_size = (sound_mgnt->ring_sample_rate * mp3info->ChannelNumber * (mp3info->SampleBit / 8) * mp3info->TotalTime) / 1000 +1;
				LOGD("ring_block_size: %d  ring_buf_size: %d \n", sound_mgnt->ring_block_size, ring_buf_size);

				sound_mgnt->pbuf_ring = (char *)bmalloc(ring_buf_size);
				memset(sound_mgnt->pbuf_ring, 0, ring_buf_size);
				int iret = 0, nsamples = 0, cur_time = 0;
				int index = 0;
				while (1)
				{
					iret = MP3File_Decode(mp3file, (short *)(sound_mgnt->pbuf_ring + sound_mgnt->ring_size), &nsamples, &cur_time);
					LOGV("<%02d> iret = %d, nsamples = %d, cur_time = %d ring_size = %d \n", ++index, iret, nsamples, cur_time, sound_mgnt->ring_size);
					if (iret == MP3DEC_ERR_NOMOREDATA)
					{
						break;
					}
					else if (iret != MP3DEC_ERR_NONE)
					{
						LOGE("MP3 decoder fatal inner error ! \n");
						break;
					}

					if(sound_mgnt->ring_size <= (ring_buf_size-(nsamples * (mp3info->SampleBit / 8) * mp3info->ChannelNumber)))
						sound_mgnt->ring_size += nsamples * (mp3info->SampleBit / 8) * mp3info->ChannelNumber;
				}

				MP3File_Close(mp3file);
			}

			LOGD("lost time:%u \n", time_relative_ms() - last_time);
		}

		AudioCore_SetPlayVolume(speaker_volume);

		pthread_create(&sound_mgnt->thread, NULL, sound_tips_task, sound_mgnt);
	}

	return 0;
}

int sound_play_tips(char *file_name, bool sync)
{
	bool enable = false;
	int volume = 10;

	sound_tips_msg_t msg = {0};

	if (sound_mgnt && sound_mgnt->mq)
	{
		// db_sys_param_get("narrator", &enable, sizeof(enable));
		// db_sys_param_get("speaker_volume_narrator", &volume, sizeof(volume));

		if (enable && volume)
		{
			if (sync)
			{
				return sound_play_start(file_name, 1, volume, true);
			}
			else
			{
				snprintf(msg.file_name, sizeof(msg.file_name), "%s", file_name);

				return mq_send(sound_mgnt->mq, &msg, sizeof(sound_tips_msg_t));
			}
		}
	}

	return -1;
}

int sound_play_speaker_en(bool enable)
{
	if (sound_mgnt)
	{
		speaker_en(sound_mgnt, enable);

		return 0;
	}

	return -1;
}


//================ring play=====================

int AudioPlayRingCbFunc(char *DataBuffer, int DataSize, void *pContext)
{
	sound_play_mgnt_t *mgnt = (sound_play_mgnt_t *)pContext;
	if (mgnt && mgnt->pbuf_ring && mgnt->ring_size)
	{
		if(mgnt->ring_pos < mgnt->ring_size){
			memcpy(DataBuffer, mgnt->pbuf_ring + mgnt->ring_pos, DataSize);
			mgnt->ring_pos += DataSize;
		}
		else{
			mgnt->ring_pos = 0;
			memcpy(DataBuffer, mgnt->pbuf_ring, DataSize);
			mgnt->ring_pos += DataSize;
		}
	}

	return DataSize;
}

int sound_play_ring_start(int volume)
{
	int play_id = 0;
	if (sound_mgnt && sound_mgnt->pbuf_ring && sound_mgnt->ring_size)
	{
		play_id = Audio_Player_Open();
		Audio_Player_SetDevice(play_id, "default");
		Audio_Player_SetFormat(play_id, sound_mgnt->ring_channels, sound_mgnt->ring_bits, sound_mgnt->ring_sample_rate);
		Audio_Player_SetCache(play_id, PLAYBLOCKS, sound_mgnt->ring_samples_per_block);
		Audio_Player_SetMode(play_id, false, AudioPlayRingCbFunc, sound_mgnt);
		Audio_Player_SetVolume(play_id, volume);
		pthread_mutex_lock(&sound_mgnt->mutex);
		sound_mgnt->ring_pos = 0;
		pthread_mutex_unlock(&sound_mgnt->mutex);
		Audio_Player_Start(play_id);

		sound_mgnt->b_ring_play = true;
	}

	return play_id;
}

void sound_play_ring_stop(int play_id)
{
	if (sound_mgnt && sound_mgnt->b_ring_play && sound_mgnt->pbuf_ring)
	{
		Audio_Player_Stop(play_id);
		Audio_Player_Close(play_id);
		sound_mgnt->ring_pos = 0;
		sound_mgnt->b_ring_play = false;
	}
}