#include "f1_video_codec.h"

#include <errno.h>
#include <string.h>

#include <video/vd/VDecApi.h>
#include <video/vd/VoApi.h>

#include <utils/bmem.h>
#include <utils/time_helper.h>

#define LOG_TAG "f1_video_codec"
#include <utils/log.h>

#define DISP_CHANNEL                    (0)
#define DISP_LAYER                      (0)
#define F1_VIDEO_DECODE_QUEUE_MAX_NALS  512
#define F1_VIDEO_DECODE_QUEUE_MAX_BYTES (4 * 1024 * 1024)

typedef struct f1_video_decode_nal
{
	struct list_head list;
	int size;
	uint8_t data[];
} f1_video_decode_nal_t;

static int f1_video_h264_nal_type(const uint8_t *data, int size)
{
	int offset = 0;

	if (!data || size <= 0)
	{
		return -1;
	}

	if (size >= 4 &&
	    data[0] == 0x00 && data[1] == 0x00 &&
	    data[2] == 0x00 && data[3] == 0x01)
	{
		offset = 4;
	}
	else if (size >= 3 &&
	         data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01)
	{
		offset = 3;
	}

	if (offset >= size)
	{
		return -1;
	}

	return data[offset] & 0x1f;
}

static void f1_video_decoder_free_queue_locked(f1_video_codec_data_t *codec_data)
{
	f1_video_decode_nal_t *node;
	f1_video_decode_nal_t *next;

	list_for_each_entry_safe(node, next, &codec_data->queue, list)
	{
		list_del(&node->list);
		bfree(node);
	}

	codec_data->queued_bytes = 0;
	codec_data->queued_nals = 0;
}

static void f1_video_decoder_apply_cm(f1_video_codec_data_t *codec_data)
{
	VO_LAYER_CM cm;

	if (!codec_data)
	{
		return;
	}

	memset(&cm, 0, sizeof(cm));
	cm.Brightness = codec_data->cm_configured ? codec_data->cm_brightness : 50;
	cm.Saturation = 50;
	cm.Hue = 50;
	cm.Contrast = 50;
	(void)VO_ChnSetCM(DISP_CHANNEL, DISP_LAYER, &cm);
}

static int f1_video_decoder_do_decode_sync(f1_video_codec_data_t *codec_data,
                                           uint8_t *data,
                                           int size)
{
	uint64_t start_tick = time_now();
	int nal_type = f1_video_h264_nal_type(data, size);

	VDEC_STREAM_S Stream;
	memset(&Stream, 0, sizeof(VDEC_STREAM_S));

	Stream.pu8Addr = data;
	Stream.u32Len  = size;
	Stream.u64PTS  = -1;

	if (1 != VDEC_SendStream(codec_data->decoder_chan, &Stream))
	{
		LOGE("VDEC_SendStream Error!\n");
		return -1;
	}

	VDEC_CHN_STAT_S Stat;
	memset(&Stat, 0, sizeof(VDEC_CHN_STAT_S));

	if (1 != VDEC_Query(codec_data->decoder_chan, &Stat))
	{
		LOGE("VDEC_Query Error!\n");
		return -1;
	}

	if (Stat.u32FrameNum > 0)
	{
		VDEC_STATUS Ret = VDEC_StartRecvStream(codec_data->decoder_chan, 0, 0, 0);

		switch (Ret)
		{
			case VDEC_NO_BITSTREAM:
				break;

			case VDEC_KEYFRAME_DECODED:
			case VDEC_FRAME_DECODED:
				{
					VDEC_FRAME_S Frame;
					memset(&Frame, 0, sizeof(VDEC_FRAME_S));

					if (0 != VDEC_GetImage(codec_data->decoder_chan, &Frame))
					{
						if (1 != VO_SetZoomInWindow(DISP_CHANNEL, DISP_LAYER, &Frame.SrcInfo))
						{
							LOGE("VO_SetZoomInWindow fail\n");
						}

						if (1 != VO_ChnShow(DISP_CHANNEL, DISP_LAYER, &Frame))
						{
							LOGE("VO_ChnShow fail\n");
						}

						if (1 != VDEC_ReleaseImage(codec_data->decoder_chan, &Frame))
						{
							LOGE("VDEC_ReleaseImage Error!\n");
						}
						}
						else
						{
							if (nal_type != 7 && nal_type != 8)
							{
								LOGE("VDEC_GetImage Error! nal=%d\n", nal_type);
							}
						}
					}
					break;

			default:
				break;
		}
	}


	(void)start_tick;
	return 0;
}

static void *f1_video_decode_worker(void *args)
{
	f1_video_codec_data_t *codec_data = (f1_video_codec_data_t *)args;

	LOGI("F1 video decoder worker start\n");

	while (true)
	{
		f1_video_decode_nal_t *node = NULL;
		int ret;

		pthread_mutex_lock(&codec_data->queue_mutex);
		while (!codec_data->stop && list_empty(&codec_data->queue))
		{
			pthread_cond_wait(&codec_data->queue_cond, &codec_data->queue_mutex);
		}

		if (codec_data->stop)
		{
			f1_video_decoder_free_queue_locked(codec_data);
			pthread_mutex_unlock(&codec_data->queue_mutex);
			break;
		}

		node = list_first_entry(&codec_data->queue, f1_video_decode_nal_t, list);
		list_del(&node->list);
		codec_data->queued_bytes -= (size_t)node->size;
		codec_data->queued_nals--;
		pthread_mutex_unlock(&codec_data->queue_mutex);

		ret = f1_video_decoder_do_decode_sync(codec_data, node->data, node->size);
		if (ret < 0)
		{
			LOGE("F1 video decoder worker decode failed, ret=%d\n", ret);
		}

		bfree(node);
	}

	LOGI("F1 video decoder worker stop\n");
	return NULL;
}

static int f1_video_decoder_start_worker(f1_video_codec_data_t *codec_data)
{
	int ret;

	codec_data->stop = false;
	codec_data->worker_ready = false;
	codec_data->drop_logged = false;
	INIT_LIST_HEAD(&codec_data->queue);
	codec_data->queued_bytes = 0;
	codec_data->queued_nals = 0;

	ret = pthread_mutex_init(&codec_data->queue_mutex, NULL);
	if (ret != 0)
	{
		LOGE("F1 video decoder queue mutex init failed, ret=%d\n", ret);
		return -1;
	}

	ret = pthread_cond_init(&codec_data->queue_cond, NULL);
	if (ret != 0)
	{
		LOGE("F1 video decoder queue cond init failed, ret=%d\n", ret);
		pthread_mutex_destroy(&codec_data->queue_mutex);
		return -1;
	}

	ret = pthread_create(&codec_data->thread, NULL, f1_video_decode_worker, codec_data);
	if (ret != 0)
	{
		LOGE("F1 video decoder worker create failed, ret=%d\n", ret);
		pthread_cond_destroy(&codec_data->queue_cond);
		pthread_mutex_destroy(&codec_data->queue_mutex);
		return -1;
	}

	codec_data->worker_ready = true;
	return 0;
}

static void f1_video_decoder_stop_worker(f1_video_codec_data_t *codec_data)
{
	if (!codec_data || !codec_data->worker_ready)
	{
		return;
	}

	pthread_mutex_lock(&codec_data->queue_mutex);
	codec_data->stop = true;
	pthread_cond_signal(&codec_data->queue_cond);
	pthread_mutex_unlock(&codec_data->queue_mutex);

	pthread_join(codec_data->thread, NULL);
	codec_data->thread = 0;

	pthread_mutex_lock(&codec_data->queue_mutex);
	f1_video_decoder_free_queue_locked(codec_data);
	pthread_mutex_unlock(&codec_data->queue_mutex);

	pthread_cond_destroy(&codec_data->queue_cond);
	pthread_mutex_destroy(&codec_data->queue_mutex);
	codec_data->worker_ready = false;
	codec_data->stop = false;
	codec_data->drop_logged = false;
}

int f1_video_decoder_open(f1_video_codec_data_t *codec_data, int x, int y, int w, int h,
                          VDEC_PAYLOAD_TYPE_E deType)
{
	VO_LAYER_INFO VoInfo;

	if (!codec_data || deType != VDEC_PAYLOAD_TYPE_H264)
	{
		LOGE("unsupported f1 video payload type [%d]\n", deType);
		return -1;
	}

	memset(&VoInfo, 0, sizeof(VO_LAYER_INFO));
	VoInfo.Rect.X = x;
	VoInfo.Rect.Y = y;
	VoInfo.Rect.W = w;
	VoInfo.Rect.H = h;

	if (1 != VO_Enable())
	{
		LOGE("VO_Enable Fail \n");
		return -1;
	}

	if (1 != VO_EnableVideoLayer(DISP_CHANNEL, DISP_LAYER, &VoInfo))
	{
		LOGE("VO_EnableVideoLayer Fail \n");
		VO_Disable();
		return -1;
	}
	f1_video_decoder_apply_cm(codec_data);

	if (1 != VDEC_Init())
	{
		LOGE("VDEC_Init Fail \n");
		VO_DisableVideoLayer(DISP_CHANNEL, DISP_LAYER);
		VO_Disable();
		return -1;
	}

	VDEC_CHN_ATTR_S DecAttr;
	memset(&DecAttr, 0, sizeof(VDEC_CHN_ATTR_S));

	DecAttr.deType      = deType;
	DecAttr.FormatType  = FORMAT_NV12;
	DecAttr.u32PicWidth = 0;
	DecAttr.u32PicHight = 0;
	DecAttr.BufSize     = 0x100000;

	codec_data->decoder_chan = VDEC_CreateChn(&DecAttr);

	if (codec_data->decoder_chan == -1)
	{
		LOGE("VDEC_CreateChn Fail \n");
		VDEC_DeInit();
		VO_DisableVideoLayer(DISP_CHANNEL, DISP_LAYER);
		VO_Disable();
		return -1;
	}

	if (f1_video_decoder_start_worker(codec_data) != 0)
	{
		VDEC_DestroyChn(codec_data->decoder_chan);
		codec_data->decoder_chan = -1;
		VDEC_DeInit();
		VO_DisableVideoLayer(DISP_CHANNEL, DISP_LAYER);
		VO_Disable();
		return -1;
	}

	return 0;
}

int f1_video_decoder_set_brightness(f1_video_codec_data_t *codec_data, int level)
{
	if (!codec_data)
	{
		return -EINVAL;
	}

	if (level < 0)
	{
		level = 0;
	}
	else if (level > 100)
	{
		level = 100;
	}

	codec_data->cm_brightness = level;
	codec_data->cm_configured = true;
	if (codec_data->decoder_chan >= 0)
	{
		f1_video_decoder_apply_cm(codec_data);
	}
	return 0;
}

int f1_video_decoder_do_decode(f1_video_codec_data_t *codec_data, uint8_t *data, int size)
{
	f1_video_decode_nal_t *node;

	if (!codec_data || !data || size <= 0 || !codec_data->worker_ready)
	{
		return -1;
	}

	node = (f1_video_decode_nal_t *)bmalloc(sizeof(*node) + (size_t)size);
	if (!node)
	{
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&node->list);
	node->size = size;
	memcpy(node->data, data, (size_t)size);

	pthread_mutex_lock(&codec_data->queue_mutex);
	if (codec_data->stop ||
	    codec_data->queued_nals >= F1_VIDEO_DECODE_QUEUE_MAX_NALS ||
	    codec_data->queued_bytes + (size_t)size > F1_VIDEO_DECODE_QUEUE_MAX_BYTES)
	{
		if (!codec_data->drop_logged)
		{
			codec_data->drop_logged = true;
			LOGW("F1 video decoder queue drop: nals=%d bytes=%zu size=%d stop=%d\n",
			     codec_data->queued_nals,
			     codec_data->queued_bytes,
			     size,
			     codec_data->stop ? 1 : 0);
		}
		pthread_mutex_unlock(&codec_data->queue_mutex);
		bfree(node);
		return -ENOBUFS;
	}

	list_add_tail(&node->list, &codec_data->queue);
	codec_data->queued_bytes += (size_t)size;
	codec_data->queued_nals++;
	pthread_cond_signal(&codec_data->queue_cond);
	pthread_mutex_unlock(&codec_data->queue_mutex);

	return 0;
}

int f1_video_decoder_close(f1_video_codec_data_t *codec_data)
{
	if (codec_data->decoder_chan >= 0)
	{
		f1_video_decoder_stop_worker(codec_data);

		VO_DisableVideoLayer(DISP_CHANNEL, DISP_LAYER);

		VDEC_DestroyChn(codec_data->decoder_chan);

		codec_data->decoder_chan = -1;

		VDEC_DeInit();

		VO_Disable();
	}

	return 0;
}
