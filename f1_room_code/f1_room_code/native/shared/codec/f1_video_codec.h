#ifndef __F1_VIDEO_CODEC_H__
#define __F1_VIDEO_CODEC_H__

#include <common_defs.h>

#include <utils/list.h>

#include <video/vd/VDecApi.h>
#include <video/vd/VoApi.h>

typedef struct f1_video_codec_data
{
	int decoder_chan;
	bool stop;
	pthread_t thread;
	pthread_mutex_t queue_mutex;
	pthread_cond_t queue_cond;
	struct list_head queue;
	size_t queued_bytes;
	int queued_nals;
	bool worker_ready;
	bool drop_logged;
	bool cm_configured;
	int cm_brightness;
} f1_video_codec_data_t;

int f1_video_decoder_open(f1_video_codec_data_t *codec_data, int x, int y, int w, int h,
                          VDEC_PAYLOAD_TYPE_E deType);

int f1_video_decoder_do_decode(f1_video_codec_data_t *codec_data, uint8_t *data, int size);

int f1_video_decoder_close(f1_video_codec_data_t *codec_data);

int f1_video_decoder_set_brightness(f1_video_codec_data_t *codec_data, int level);

#endif /* __F1_VIDEO_CODEC_H__ */
