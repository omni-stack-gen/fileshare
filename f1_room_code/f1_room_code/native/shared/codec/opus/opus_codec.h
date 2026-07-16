#ifndef __OPUS_CODEC_H__
#define __OPUS_CODEC_H__

#include <stdint.h>
#include <stddef.h>

/**
 * Opus codec configuration.
 */
typedef struct opus_codec_config
{
	uint32_t   sample_rate; /**< Sample rate in Hz.                     */
	uint32_t   channel_cnt; /**< Number of channels.                    */
	uint32_t   frm_ptime;   /**< Frame ptime in msec.                   */
	uint32_t   bit_rate;    /**< Encoder bit rate in bps.               */
	uint32_t   packet_loss; /**< Encoder's expected packet loss pct.    */
	uint32_t   complexity;  /**< Encoder complexity, 0-10(10 is highest)*/
    uint32_t   vad:1;       /**< Voice Activity Detector.           */
    uint32_t   plc:1;       /**< Packet loss concealment            */
    uint32_t   cbr:1;       /**< Constant bit rate?                 */
} opus_codec_config_t;

void *libopus_encoder_create(opus_codec_config_t *cfg);
void libopus_encoder_destroy(void *enc);
int libopus_encoder_encode(void *enc, const uint8_t *inp_buf, size_t inp_len, uint8_t *out_buf, size_t *out_len);

void *libopus_decoder_create(opus_codec_config_t *cfg);
void libopus_decoder_destroy(void *dec);
int libopus_decoder_decode(void *dec, const uint8_t *inp_buf, size_t inp_len, uint8_t *out_buf, size_t *out_len);
int libopus_decoder_plc(void *dec, uint8_t *out_buf, size_t *out_len);

#endif