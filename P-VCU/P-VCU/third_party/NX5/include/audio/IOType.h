/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : mm_comm_aio.h
  Version       : Initial Draft
  Author        : Allwinner BU3-PD2 Team
  Created       : 2016/03/23
  Last Modified :
  Description   : common struct definition for AENC
  Function List :
  History       :
******************************************************************************/

#ifndef __IOTYPE_H__
#define __IOTYPE_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // 通道号
    typedef int AIO_CHN;

    // 采样率
    typedef enum AUDIO_SAMPLE_RATE_E
    {
        /* 8K samplerate*/
        AUDIO_SAMPLE_RATE_8000 = 8000,
        /* 11.025K samplerate*/
        AUDIO_SAMPLE_RATE_11025 = 11025,
        /* 12K samplerate*/
        AUDIO_SAMPLE_RATE_12000 = 12000,
        /* 16K samplerate*/
        AUDIO_SAMPLE_RATE_16000 = 16000,
        /* 22.050K samplerate*/
        AUDIO_SAMPLE_RATE_22050 = 22050,
        /* 24K samplerate*/
        AUDIO_SAMPLE_RATE_24000 = 24000,
        /* 32K samplerate*/
        AUDIO_SAMPLE_RATE_32000 = 32000,
        /* 44.1K samplerate*/
        AUDIO_SAMPLE_RATE_44100 = 44100,
        /* 48K samplerate*/
        AUDIO_SAMPLE_RATE_48000 = 48000,

    } AUDIO_SAMPLE_RATE_E;

    // 位宽
    typedef enum AUDIO_BIT_WIDTH_E
    {
        /* 8bit width */
        AUDIO_BIT_WIDTH_8 = 8,
        /* 16bit width*/
        AUDIO_BIT_WIDTH_16 = 16,
        /* 32bit width*/
        AUDIO_BIT_WIDTH_32 = 32,

    } AUDIO_BIT_WIDTH_E;

    // 同步/异步模式
    typedef enum AIO_MODE_E
    {
        // 同步模式
        AI_MODE_I2S_MASTER = 0,
        // 异步模式
        AI_MODE_I2S_SLAVE,

    } AIO_MODE_E;

    // 声道
    typedef enum AIO_SOUND_MODE_E
    {
        // 单声道
        AUDIO_SOUND_MODE_MONO = 1,
        // 双声道
        AUDIO_SOUND_MODE_STEREO = 2,

    } AIO_SOUND_MODE_E;

    // 音频输入输出缓存属性
    typedef struct AIO_CACHE_S
    {
        // 数据缓存块个数
        int u32Blocks;
        // 每个数据缓存块缓存的音频数据时长(ms)
        int u32BlockTime;

    } AIO_CACHE_S;

    // 音频输入输出属性
    typedef struct AIO_ATTR_S
    {
        // 采样率，默认8000Hz
        AUDIO_SAMPLE_RATE_E enSampleRate;
        // 位宽，默认16bit
        AUDIO_BIT_WIDTH_E enBitWidth;
        // 同步/异步模式，默认同步
        AIO_MODE_E enWorkMode;
        // 声道，默认单声道
        AIO_SOUND_MODE_E enSoundMode;
        // 缓存属性，默认3个缓存块，20ms音频数据时长
        AIO_CACHE_S Cache;
        // 异步模式的（录/放）音回调函数
        int(*AudioCallBack)(char *DataBuffer, int DataSize, void *pContext);
        // 异步模式（录/放）音用户数据
        void *UserData;

        // 一帧采样大小(填0底下计算)
        int SampleBlock;
    } AIO_ATTR_S;

    // 音频帧结构
    typedef struct AUDIO_FRAME_S
    {
        // 虚拟地址
        void *pVirAddr[2];
        // 物理地址
        unsigned int u32PhyAddr[2];
        // 时间戳
        unsigned long long u64TimeStamp;
        // 帧序号
        unsigned int u32Seq;
        // 数据长度
        unsigned int u32Len;

    } AUDIO_FRAME_S;


// 接口返回状态：成功
#define SUCCESS 0
// 接口返回状态：失败
#define FAILURE (-1)

// AI接口返回状态：无效的设备ID
#define ERR_AI_INVALID_DEVID (-2)
// AI接口返回状态：无效的通道ID
#define ERR_AI_INVALID_CHNID (-3)
// AI接口返回状态：非法参数
#define ERR_AI_ILLEGAL_PARAM (-4)
// AI接口返回状态：AI通道已存在
#define ERR_AI_EXIST (-5)
// AI接口返回状态：AI通道不存在
#define ERR_AI_UNEXIST (-6)
// AI接口返回状态：使用了空指针
#define ERR_AI_NULL_PTR (-7)
// AI接口返回状态：AI通道未配置
#define ERR_AI_NOT_CONFIG (-8)
// AI接口返回状态：当前不允许操作
#define ERR_AI_NOT_SUPPORT (-9)
// AI接口返回状态：不允许此种操作
#define ERR_AI_NOT_PERM (-10)
// AI接口返回状态：AI设备不可用
#define ERR_AI_NOT_ENABLED (-11)
// AI接口返回状态：内存分配错误
#define ERR_AI_NOMEM (-12)
// AI接口返回状态：分配缓冲区错误
#define ERR_AI_NOBUF (-13)
// AI接口返回状态：缓冲区内无数据
#define ERR_AI_BUF_EMPTY (-14)
// AI接口返回状态：缓冲区已满
#define ERR_AI_BUF_FULL (-15)
// AI接口返回状态：未就绪
#define ERR_AI_SYS_NOTREADY (-16)
// AI接口返回状态：AI设备正忙
#define ERR_AI_BUSY (-17)


// AO接口返回状态：无效的AO设备
#define ERR_AO_INVALID_DEVID (-22)
// AO接口返回状态：无效的AO通道ID
#define ERR_AO_INVALID_CHNID (-23)
// AO接口返回状态：非法参数
#define ERR_AO_ILLEGAL_PARAM (-24)
// AO接口返回状态：AO设备已存在
#define ERR_AO_EXIST (-25)
// AO接口返回状态：AO设备不存在
#define ERR_AO_UNEXIST (-26)
// AO接口返回状态：使用了空指针
#define ERR_AO_NULL_PTR (-27)
// AO接口返回状态：AO通道未配置
#define ERR_AO_NOT_CONFIG (-28)
// AO接口返回状态：AO通道现不可用
#define ERR_AO_NOT_SUPPORT (-29)
// AO接口返回状态：不允许此种操作
#define ERR_AO_NOT_PERM (-30)
// AO接口返回状态：AO通道不可用
#define ERR_AO_NOT_ENABLED (-31)
// AO接口返回状态：内存分配错误
#define ERR_AO_NOMEM (-32)
// AO接口返回状态：分配缓冲区错误
#define ERR_AO_NOBUF (-33)
// AO接口返回状态：缓冲区内无数据
#define ERR_AO_BUF_EMPTY (-34)
// AO接口返回状态：缓冲区已满
#define ERR_AO_BUF_FULL (-35)
// AO接口返回状态：未就绪
#define ERR_AO_SYS_NOTREADY (-36)
// AO接口返回状态：AO设备正忙
#define ERR_AO_BUSY (-37)

#ifdef __cplusplus
}
#endif

#endif /* __IOTYPE_H__ */
