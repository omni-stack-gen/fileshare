#ifndef __WAVE_FILE_FMT_H__
#define __WAVE_FILE_FMT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define RIFF_TAG (('F' << 24) | ('F' << 16) | ('I' << 8) | 'R')
#define WAVE_TAG (('E' << 24) | ('V' << 16) | ('A' << 8) | 'W')
#define FMT_TAG  ((' ' << 24) | ('t' << 16) | ('m' << 8) | 'f')
#define DATA_TAG (('a' << 24) | ('t' << 16) | ('a' << 8) | 'd')
#define FACT_TAG (('t' << 24) | ('c' << 16) | ('a' << 8) | 'f')

#define TAG_FOURCC_RIFF       0x52494646
#define TAG_FOURCC_WAVE       0x57415645
#define TAG_FOURCC_fmt        0x666d7420
#define TAG_FOURCC_data       0x64617461
#define TAG_FORMAT_PCM        0x0001

typedef struct wave_hdr
{
    unsigned int   riff;            /**< "RIFF" ASCII tag.		*/
    unsigned int   file_len;        /**< File length minus 8 bytes	*/
    unsigned int   wave;            /**< "WAVE" ASCII tag.		*/
    unsigned int   fmt;             /**< "fmt " ASCII tag.		*/
    unsigned int   fmt_len;         /**< 16 for PCM.		*/
    unsigned short fmt_tag;         /**< 1 for PCM			*/
    unsigned short channel;         /**< Number of channels.	*/
    unsigned int   sample_rate;     /**< Sampling rate.		*/
    unsigned int   bytes_per_sec;   /**< Average bytes per second.	*/
    unsigned short block_align;     /**< channel * bits / 8	*/
    unsigned short bits_per_sample; /**< Bits per sample.		*/
    unsigned int   data;            /**< "data" ASCII tag.		*/
    unsigned int   data_len;        /**< Data length.		*/
} wave_hdr_t;

typedef enum
{
    WAVE_FMT_TAG_PCM  = 1,
    WAVE_FMT_TAG_ALAW = 6,
    WAVE_FMT_TAG_ULAW = 7
} wave_fmt_tag;

typedef struct wave_subchunk
{
    unsigned int id;
    unsigned int len;
} wave_subchunk_t;

int read_wave_info(FILE **fd, wave_hdr_t *wave_hdr);

int write_wave_info(FILE **fd, int samplerate, int samplebit, int channels);

void update_wav_info(FILE **fd);

#endif //!__WAV_FILE_FMT_H__