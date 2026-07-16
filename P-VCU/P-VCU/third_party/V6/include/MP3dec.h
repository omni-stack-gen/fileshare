#ifndef __MP3DEC_H__
#define __MP3DEC_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	long  timestamp;
	char *string;
} SyncText;

typedef struct
{
	//id3v1 info
	char *title;
	char *artist;
	char *album;
	char *year;
	char *comment;
	char *track;
	char *genre;
	//id3v2 info
	char *length;
	char *textstr;
	SyncText    *synctext;
	int  syncmode;
	int  syncnumber;
} TagInfo;

typedef struct
{
	TagInfo *ContDesc;
	int   ChannelNumber;
	int   SampleBit;
	long  SampleRate;
	long  BitRate;
	long  TotalTime;          // ms
} AudioInfo;

typedef struct
{
	char  ID[6];              // "WRAI\0\0"
	char  ChannelNumber;
	char  SampleBit;
	long  SampleRate;
	long  BitRate;
	long  TotalTime;          // ms
	long  EncodeTime;         // seconds from 1970-01-01 00:00:00
	int   EncodeConfig;
	char  Zero;
	char  Track;
} WRAInfo;

//MP3 decoder return error type
#define MP3DEC_ERR_NONE         0
#define MP3DEC_ERR_PARAMETER    1
#define MP3DEC_ERR_FILEOPEN     2
#define MP3DEC_ERR_FILETYPE     3
#define MP3DEC_ERR_VERSION      4
#define MP3DEC_ERR_SUPPORT      5
#define MP3DEC_ERR_NOMOREDATA   6
#define MP3DEC_ERR_DECODE       7


typedef void*  MP3File;

#ifdef __cplusplus
extern "C"
{
#endif

int MP3File_Open   (MP3File *mp3file, char *filename);
int MP3File_Seek   (MP3File  mp3file, int target_time, int *actual_time);
int MP3File_Decode (MP3File  mp3file, short *pcmbuf, int *nsamples, int *timestamp);
int MP3File_Close  (MP3File  mp3file);

#ifdef __cplusplus
}
#endif


//MP3 decoder return error type
#define MP3_ERR_NONE       0      // decoder one frame successfully
#define MP3_ERR_SYNC       1      // not find SYNC word, so need more data
#define MP3_ERR_CRC        2      // SYNC OK, but CRC error
#define MP3_ERR_LENGTH     3      // SYNC OK, but need more data
#define MP3_ERR_PTR        4      // involid input or output buffer pointer
#define MP3_ERR_DECODE     5      // fatal error when decode one frame


typedef struct
{
	int     skip_flag;              // (i)
	char   *input_buffer;           // (io)
	int     input_length;           // (io)
	short  *output_buffer;          // (i)
	int     output_sample;          // (o)
	int     channel_number;         // (o)
	int     sample_bit;             // (o)
	int     sample_rate;            // (o)
	int     bit_rate;               // (o)
	int     frame_length;           // (o)
	void   *decoder;
} MP3Decoder;

#ifdef __cplusplus
extern "C"
{
#endif

int  MP3Decoder_Init    (MP3Decoder **mp3dec);
int  MP3Decoder_Release (MP3Decoder  *mp3dec);
int  MP3Decoder_Frame   (MP3Decoder  *mp3dec);

#ifdef __cplusplus
}
#endif

#endif//__MP3DEC_H__
