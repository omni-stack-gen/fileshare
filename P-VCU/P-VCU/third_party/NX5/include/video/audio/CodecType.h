/******************************************************************************
Copyright(c) 2016-2018 Digital Power Inc.
File name: CodecType.h
Author: LiuZhengzhong
Version: 1.0.0
Date: 2018/3/14
Description: Platform of DP X5 audio encode and decode common define
History:
Bug report: liuzhengzhong@d-power.com.cn
******************************************************************************/


#ifndef __CODECTYPE_H__
#define __CODECTYPE_H__


#define MAX_AUD_DECODE_CHANNEL     8
#define MAX_AUD_ENCODE_CHANNEL     8

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    PTT_PCM = 0,
    PTT_G711A = 1,
    PTT_G711U = 2,
    PTT_AAC = 3,
    PTT_MP3 = 4,
    PTT_BUT,

} PAYLOAD_TYPE_TTE;

typedef enum
{
    FILE_MODE = 0,
    STREAM_MODE = 1
}MODE_E;

typedef struct x5ADEC_CH_ATTR_S
{
    PAYLOAD_TYPE_TTE  enType;         // 编解码类型
    MODE_E          Mode;           // 编解码模式仅对MP3和AAC编解码有效
    char            *FileName;      // MP3，AAC编码会用到此字段 仅文件模式有效
    unsigned int    u32BufSize;     // 编解码缓冲区长度，字节
    unsigned int    u32SampleRate;  // 原始码流采样率，MP3，AAC编码会用到此字段
    unsigned int    u32Channels;    // 声道(单声道、双声道, 底下默认为单声道)
    unsigned int    u32BitRate;     // 比特流(底下默认64000) MP3，AAC编码会用到此字段
} AUDIO_CHN_ATTR_S;

typedef struct x5AUDIO_FILE_CODEC_S
{
    void *UserData;
    void (*EncCallback)(unsigned char *Data0,       // 编码回调，编码器每次会进入该回调函数取数据
                        unsigned char *Data1,       // Data1暂时未用
                        int EncLen,                 // 编码器回调上来的数据长度，用户根据此数据长度往Data0里写数据
                        int *End,                   // 结束标志，当*End=1时编码结束
                        void *UserData);            // 用户自定义数据

    void (*DecCallback)(unsigned char *Data0,       // 解码回调，解码后的数据地址
                        unsigned char *Data1,       // 若双声道，则Data1不为空
                        int DecLen,                 // 解码器回调上来的数据长度，是解码后PCM数据的长度
                        int Channel,                // 原始文件若单声道，Channel=1，若双声道，Channel=2
                        int SampleRate,             // 原始文件中的采样率
                        int BitRate,                // 原始文件中的比特率
                        int BitWidth,               // 原始文件的比特位宽
                        void *UserData);            // 用户自定义数据

} AUDIO_FILE_CODEC;


typedef struct x5AUDIO_STREAM_CODEC_S
{
    unsigned char           *pStream;       // 码流虚拟地址
    unsigned int            u32Len;         // 输入时为原始码流长度，输出时为编码后码流长度

} AUDIO_STREAM_CODEC;


typedef struct x5AUDIO_STREAM_S
{
    union
    {
        AUDIO_FILE_CODEC        File;       // 文件式编解码，MP3、AAC
        AUDIO_STREAM_CODEC      Stream;     // 流式编解码，PCMA，PCMU
    };
    unsigned long long      u64TimeStamp;   // 时间戳
    unsigned int            u32Seq;         // 帧序号
    long long               Pts;
    long long               Dts;
} ADUIO_STREAM_S;

#ifdef __cplusplus
}
#endif

#endif // !__CODECTYPE_H__



