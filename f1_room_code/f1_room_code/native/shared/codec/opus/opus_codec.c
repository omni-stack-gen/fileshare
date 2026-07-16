#include "opus_codec.h"
#include <opus/opus.h>
#include <utils/bmem.h>
#include <string.h>

#define LOG_TAG "opus_codec"
#include <utils/log.h>

/* Maximum size of an encoded packet.
 * If the the actual size is bigger, the encode/parse will fail.
 */
#define MAX_ENCODED_PACKET_SIZE              1280

/**
 * OPUS codec sample rate.
 *
 * Default: 16000
 */
#define OPUS_CODEC_DEFAULT_SAMPLE_RATE       16000

/**
 * OPUS codec default maximum average bit rate.
 *
 * Default: 0 (leave it to default value specified by Opus, which will
 * take into account factors such as media content (speech/music), sample
 * rate, channel count, etc).
 */
#define OPUS_CODEC_DEFAULT_BIT_RATE          0

/**
 * OPUS default encoding complexity, which is an integer from
 * 0 to 10, where 0 is the lowest complexity and 10 is the highest.
 *
 * Default: 5
 */
#define OPUS_CODEC_DEFAULT_COMPLEXITY        5

/**
 * OPUS default CBR (constant bit rate) setting
 *
 * Default: 0 (which means Opus will use VBR (variable bit rate))
 */
#define OPUS_CODEC_DEFAULT_CBR               0

/* Default packet loss concealment setting. */
#define OPUS_CODEC_DEFAULT_PLC               1

/* Default Voice Activity Detector setting. */
#define OPUS_CODEC_DEFAULT_VAD               0

/* Default frame time (msec) */
#define OPUS_CODEC_PTIME                     20

typedef struct opus_context
{
	OpusEncoder *encoder;
	OpusDecoder *decoder;
	OpusRepacketizer *repacker;
	opus_codec_config_t cfg;
} opus_context_t;

/* Opus default configuration */
static opus_codec_config_t opus_cfg =
{
	OPUS_CODEC_DEFAULT_SAMPLE_RATE,     /* Sample rate          */
	1,                                  /* Channel count        */
	OPUS_CODEC_PTIME,                   /* Frame ptime          */
	OPUS_CODEC_DEFAULT_BIT_RATE,        /* Bit rate             */
	5,                                  /* Expected packet loss */
	OPUS_CODEC_DEFAULT_COMPLEXITY,      /* Complexity           */
	OPUS_CODEC_DEFAULT_VAD,             /* Voice Activity Detector.    */
	OPUS_CODEC_DEFAULT_PLC,             /* Packet loss concealment     */
	OPUS_CODEC_DEFAULT_CBR,             /* Constant bit rate?  */
};

static int get_opus_bw_constant(unsigned sample_rate)
{
	if (sample_rate <= 8000)
	{
		return OPUS_BANDWIDTH_NARROWBAND;
	}
	else if (sample_rate <= 12000)
	{
		return OPUS_BANDWIDTH_MEDIUMBAND;
	}
	else if (sample_rate <= 16000)
	{
		return OPUS_BANDWIDTH_WIDEBAND;
	}
	else if (sample_rate <= 24000)
	{
		return OPUS_BANDWIDTH_SUPERWIDEBAND;
	}
	else
	{
		return OPUS_BANDWIDTH_FULLBAND;
	}
}

/* ================== 创建与销毁 ================== */

void *libopus_encoder_create(opus_codec_config_t *cfg)
{
	int err;
	opus_context_t *ctx = bzalloc(sizeof(opus_context_t));

	if (!ctx)
	{
		return NULL;
	}

	if (cfg)
	{
		memcpy(&ctx->cfg, cfg, sizeof(opus_codec_config_t));
	}
	else
	{
		memcpy(&ctx->cfg, &opus_cfg, sizeof(opus_codec_config_t));
	}

	ctx->encoder = opus_encoder_create(ctx->cfg.sample_rate,
	                                   ctx->cfg.channel_cnt,
	                                   OPUS_APPLICATION_VOIP,
	                                   &err);

	if (err != OPUS_OK)
	{
		LOGE("Failed to create Opus encoder: %s\n", opus_strerror(err));
		bfree(ctx);
		return NULL;
	}

	ctx->repacker = opus_repacketizer_create();

	/* Set signal type */
	opus_encoder_ctl(ctx->encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
	/* Set bitrate */
	opus_encoder_ctl(ctx->encoder, OPUS_SET_BITRATE(ctx->cfg.bit_rate ?
	                 (int)ctx->cfg.bit_rate :
	                 OPUS_AUTO));
	/* Set VAD */
	opus_encoder_ctl(ctx->encoder, OPUS_SET_DTX(ctx->cfg.vad ? 1 : 0));
	/* Set PLC */
	opus_encoder_ctl(ctx->encoder,  OPUS_SET_INBAND_FEC(1));
	/* Set bandwidth */
	opus_encoder_ctl(ctx->encoder, OPUS_SET_MAX_BANDWIDTH(get_opus_bw_constant(ctx->cfg.sample_rate)));
	/* Set expected packet loss */
	opus_encoder_ctl(ctx->encoder,
	                 OPUS_SET_PACKET_LOSS_PERC(ctx->cfg.packet_loss));
	/* Set complexity */
	opus_encoder_ctl(ctx->encoder,
	                 OPUS_SET_COMPLEXITY(ctx->cfg.complexity));
	/* Set constant bit rate */
	opus_encoder_ctl(ctx->encoder,
	                 OPUS_SET_VBR(ctx->cfg.cbr ? 0 : 1));

	LOGI("Initialize Opus encoder, sample rate: %d, ch: %d, "
	     "avg bitrate: %d%s, vad: %d, plc: %d, pkt loss: %d, "
	     "complexity: %d, constant bit rate: %d, "
	     "ptime: %d \n",
	     ctx->cfg.sample_rate,
	     ctx->cfg.channel_cnt,
	     ctx->cfg.bit_rate,
	     (ctx->cfg.bit_rate ? "" : "(auto)"),
	     ctx->cfg.vad ? 1 : 0,
	     ctx->cfg.plc ? 1 : 0,
	     ctx->cfg.packet_loss,
	     ctx->cfg.complexity,
	     ctx->cfg.cbr ? 1 : 0,
	     ctx->cfg.frm_ptime);

	return ctx;
}

int libopus_encoder_encode(void *enc, const uint8_t *inp_buf, size_t inp_len, uint8_t *out_buf, size_t *out_len)
{
	opus_context_t *ctx = enc;

	if (!ctx || !inp_buf || !out_buf || !out_len)
	{
		return -1;
	}

	opus_int32 size  = 0;
	unsigned in_pos  = 0;
	unsigned out_pos = 0;
	unsigned frame_size;
	unsigned samples_per_frame;
	unsigned char tmp_buf[MAX_ENCODED_PACKET_SIZE];
	unsigned tmp_bytes_left = sizeof(tmp_buf);

	samples_per_frame = (ctx->cfg.sample_rate * ctx->cfg.frm_ptime / 1000);

	frame_size = samples_per_frame * ctx->cfg.channel_cnt * sizeof(opus_int16);

	if (inp_len < frame_size)
	{
		return -1;
	}

	// 初始化重打包器
	opus_repacketizer_init(ctx->repacker);

	while (inp_len - in_pos >= frame_size)
	{
		size = opus_encode(ctx->encoder,
		                   (const opus_int16 *)(inp_buf + in_pos),
		                   samples_per_frame,
		                   tmp_buf + out_pos,
		                   (tmp_bytes_left < frame_size ?
		                    tmp_bytes_left : frame_size));

		if (size < 0)
		{
			LOGE("Opus encode failed: %s\n", opus_strerror(size));
			return -1;
		}

		if (size > 0)
		{
			int err = opus_repacketizer_cat(ctx->repacker, tmp_buf + out_pos, size);

			if (err != OPUS_OK)
			{
				return -1;
			}
		}

		out_pos += size;
		tmp_bytes_left -= size;
		in_pos += frame_size;
	}

	if (!opus_repacketizer_get_nb_frames(ctx->repacker))
	{
		return -1;
	}

	// 输出最终的复合 RTP 载荷
	int ret = opus_repacketizer_out(ctx->repacker, out_buf, *out_len);

	if (ret < 0)
	{
		return -1;
	}

	*out_len = ret;
	return 0;
}


void libopus_encoder_destroy(void *enc)
{
	opus_context_t *ctx = enc;

	if (!ctx)
	{
		return;
	}

	if (ctx->encoder)
	{
		opus_encoder_destroy(ctx->encoder);
	}

	if (ctx->repacker)
	{
		opus_repacketizer_destroy(ctx->repacker);
	}

	bfree(ctx);
}

void *libopus_decoder_create(opus_codec_config_t *cfg)
{
	int err;
	opus_context_t *ctx = bzalloc(sizeof(opus_context_t));

	if (!ctx)
	{
		return NULL;
	}

	if (cfg)
	{
		memcpy(&ctx->cfg, cfg, sizeof(opus_codec_config_t));
	}
	else
	{
		memcpy(&ctx->cfg, &opus_cfg, sizeof(opus_codec_config_t));
	}

	ctx->decoder = opus_decoder_create(ctx->cfg.sample_rate,
	                                   ctx->cfg.channel_cnt,
	                                   &err);

	if (err != OPUS_OK)
	{
		bfree(ctx);
		return NULL;
	}

	ctx->repacker = opus_repacketizer_create();

	return ctx;
}

void libopus_decoder_destroy(void *dec)
{
	opus_context_t *ctx = dec;

	if (!ctx)
	{
		return;
	}

	if (ctx->decoder)
	{
		opus_decoder_destroy(ctx->decoder);
	}

	if (ctx->repacker)
	{
		opus_repacketizer_destroy(ctx->repacker);
	}

	bfree(ctx);
}

int libopus_decoder_plc(void *dec, uint8_t *out_buf, size_t *out_len)
{
	opus_context_t *ctx = dec;

	if (!ctx || !out_buf || !out_len)
	{
		return -1;
	}

	// 1. 计算一帧（例如 20ms）包含多少个单声道采样点
	int samples_per_frame = (ctx->cfg.sample_rate * ctx->cfg.frm_ptime) / 1000;
	// 2. 计算一帧 PCM 数据占用的字节数
	int frame_bytes = samples_per_frame * ctx->cfg.channel_cnt * sizeof(opus_int16);

	// 3. 根据外部提供的缓冲区大小，计算需要推断补偿多少帧
	int requested_frames = *out_len / frame_bytes;

	if (requested_frames <= 0)
	{
		LOGE("PLC buffer too small\n");
		return -1;
	}

	int total_plc_samples = 0;
	opus_int16 *pcm_out_ptr = (opus_int16 *)out_buf;

	for (int i = 0; i < requested_frames; i++)
	{
		/*
		 * 触发 PLC 的核心：
		 * 传入的 payload 为 NULL，长度为 0。
		 * 告诉解码器需要补多少个采样点 (samples_per_frame)。
		 */
		int ret = opus_decode(ctx->decoder,
		                      NULL,
		                      0,
		                      pcm_out_ptr,
		                      samples_per_frame,
		                      0); // PLC 时 fec 固定填 0

		if (ret < 0)
		{
			LOGE("Opus PLC failed: %s\n", opus_strerror(ret));
			return -1;
		}

		total_plc_samples += ret;
		// 指针偏移，准备生成下一帧的假音
		pcm_out_ptr += ret * ctx->cfg.channel_cnt;
	}

	// 将实际生成的 PCM 字节数通过出参返回
	*out_len = total_plc_samples * ctx->cfg.channel_cnt * sizeof(opus_int16);

	return 0; // 成功返回 0
}

int libopus_decoder_decode(void *dec, const uint8_t *inp_buf, size_t inp_len, uint8_t *out_buf, size_t *out_len)
{
	opus_context_t *ctx = dec;

	if (!ctx || !out_buf || !out_len)
	{
		return -1;
	}

	// =============== 分支 1：PLC (丢包隐藏) ===============
	if (inp_buf == NULL || inp_len == 0)
	{
		return libopus_decoder_plc(dec, out_buf, out_len);
	}

	// =============== 分支 2：正常解码复合包 ===============
	unsigned char tmp_buf[MAX_ENCODED_PACKET_SIZE];
	unsigned char frame_buf[MAX_ENCODED_PACKET_SIZE];
	int i, num_frames;
	int frame_size;
	unsigned samples_per_frame = 0;
	int total_decoded_samples = 0;
	int max_out_samples = *out_len / (sizeof(opus_int16) * ctx->cfg.channel_cnt);
	opus_int16 *pcm_out = (opus_int16 *)out_buf;

	if (inp_len > sizeof(tmp_buf))
	{
		return -1;
	}

	memcpy(tmp_buf, inp_buf, inp_len);
	opus_repacketizer_init(ctx->repacker);

	if (opus_repacketizer_cat(ctx->repacker, tmp_buf, inp_len) != OPUS_OK)
	{
		return -1;
	}

	num_frames = opus_repacketizer_get_nb_frames(ctx->repacker);

	if (num_frames <= 0)
	{
		return -1;
	}

	// Output buffer is too small for all frames #1 max_out_samples: 1920 total_decoded_samples 1920 samples_per_frame 320
	for (i = 0; i < num_frames; ++i)
	{
		frame_size = opus_repacketizer_out_range(ctx->repacker, i, i + 1,
		             frame_buf, sizeof(frame_buf));

		if (frame_size < 0)
		{
			LOGE("Parse failed! (inp_len=%lu, err=%d)\n",
			     (unsigned long)inp_len, frame_size);
			return -1;
		}

		if (i == 0)
		{
			samples_per_frame = opus_packet_get_nb_samples(frame_buf, frame_size, ctx->cfg.sample_rate);

			if (samples_per_frame <= 0)
			{
				LOGE("Parse failed to get samples number! (err=%d)\n", samples_per_frame);
				return -1;
			}
		}

		if (max_out_samples - total_decoded_samples < samples_per_frame)
		{
			LOGE("Output buffer is too small for all frames #%d max_out_samples: %d "
			     "total_decoded_samples %d samples_per_frame %d\n",
			     i, max_out_samples, total_decoded_samples, samples_per_frame);
			return -1;
		}

		int decoded = opus_decode(ctx->decoder, frame_buf, frame_size,
		                          pcm_out + (total_decoded_samples * ctx->cfg.channel_cnt),
		                          max_out_samples - total_decoded_samples, 0);

		if (decoded < 0)
		{
			LOGE("Opus decode failed: %s\n", opus_strerror(decoded));
			return -1;
		}

		total_decoded_samples += decoded;
	}

	*out_len = total_decoded_samples * ctx->cfg.channel_cnt * sizeof(opus_int16);
	return 0;
}