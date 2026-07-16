#ifndef __WAVE_FILE_FMT_H__
#define __WAVE_FILE_FMT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_SIZE_OF_WAVE_FILE_NAME (64)

#define RIFF_TAG (('F' << 24) | ('F' << 16) | ('I' << 8) | 'R')
#define WAVE_TAG (('E' << 24) | ('V' << 16) | ('A' << 8) | 'W')
#define FMT_TAG  ((' ' << 24) | ('t' << 16) | ('m' << 8) | 'f')
#define DATA_TAG (('a' << 24) | ('t' << 16) | ('a' << 8) | 'd')
#define FACT_TAG (('t' << 24) | ('c' << 16) | ('a' << 8) | 'f')

#define WAVE_DATA_HDR_SIZE  (8)
#define WAVE_FMT_HDR_SIZE   (24)
#define WAVE_RIFF_HDR_SIZE  (12)

typedef struct wave_hdr
{
	uint32_t riff;            /**< "RIFF" ASCII tag.      */
	uint32_t file_len;        /**< File length minus 8 bytes  */
	uint32_t wave;            /**< "WAVE" ASCII tag.      */

	uint32_t fmt;             /**< "fmt " ASCII tag.      */
	uint32_t fmt_len;         /**< 16 for PCM.        */
	uint16_t fmt_tag;         /**< 1 for PCM          */
	uint16_t channel;         /**< Number of channels.    */
	uint32_t sample_rate;     /**< Sampling rate.     */
	uint32_t bytes_per_sec;   /**< Average bytes per second.  */
	uint16_t block_align;     /**< channel * bits / 8 */
	uint16_t bits_per_sample; /**< Bits per sample.       */

	uint32_t data;            /**< "data" ASCII tag.  */
	uint32_t data_len;        /**< Data length.       */
} wave_hdr_t;

typedef enum wave_fmt_tag
{
	WAVE_FMT_TAG_PCM  = 1,
	WAVE_FMT_TAG_ALAW = 6,
	WAVE_FMT_TAG_ULAW = 7
} wave_fmt_tag_t;

typedef struct wave_subchunk
{
	uint32_t id;
	uint32_t len;
} wave_subchunk_t;

typedef struct wav_writer
{
	FILE *file;
	char file_name[MAX_SIZE_OF_WAVE_FILE_NAME];
	uint32_t data_size;
	uint32_t sample_rate;
	uint16_t bytes_per_sample;
	uint16_t num_channels;
	wave_fmt_tag_t fmt_tag;
} wav_writer_t;

int wav_writer_open(wav_writer_t *writer, const char *file_name, wave_fmt_tag_t fmt_tag,
                    int sample_rate, int num_channels);
int wav_writer_write(wav_writer_t *writer, const void *data, size_t bytes);
int wav_writer_close(wav_writer_t *writer);

typedef struct wav_reader
{
	FILE *file;

	uint16_t fmt_tag;
	uint16_t channel;
	uint32_t sample_rate;
	uint16_t bits_per_sample;
	uint32_t data_len;

	uint32_t file_size;		   // 文件大小
	uint32_t data_start;       // 音频数据起始位置
	uint32_t data_position;    // 当前读取位置
} wav_reader_t;

int wav_reader_open(wav_reader_t *reader, const char *file_name);
size_t wav_reader_read(wav_reader_t *reader, void *buffer, size_t buffer_size);
int wav_reader_seek(wav_reader_t *reader, uint32_t position);
void wav_reader_close(wav_reader_t *reader);

#endif //!__WAV_FILE_FMT_H__