#ifndef __FH_COMM_AIO_H__
#define __FH_COMM_AIO_H__

#include "fh_common.h"
#include "fh_errno.h"
#include "FHSES_param.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define MAX_AUDIO_CHN_NUM			16
#define TOTAL_CHN_NUM				20

#define MAX_AUDIO_FILE_PATH_LEN		256
#define MAX_AUDIO_FILE_NAME_LEN		256

#define MAX_VOLUME_CNT				12800
#define DEFAULT_GAIN_SIZE			MAX_VOLUME_CNT

#define DEFAULT_AI_CACHE_NUM		6
#define DEFAULT_AO_CACHE_NUM		6

#define VQE_EQ_BAND_NUM				10

#define DEV_NUM						2 /*  build-in codec max number  */
#define	ACW_VERION_ID				0x0002

#define IS_ACW_ID(id)				(id == DEV_ACW0 || id == DEV_ACW1)?FH_TRUE:FH_FALSE
#define IS_DMIC_ID(id)				(id == DEV_DMIC0 || id == DEV_DMIC1)?FH_TRUE:FH_FALSE
#define IS_ONLY_ACID(id)			(id == DEV_EXT_CODEC)?FH_TRUE:FH_FALSE

#define MPI_AIO_IOCTL(fd,cmd,arg)		fd>0?ioctl(fd, cmd, arg):FH_FAILURE
#define MPI_AIO_CHECK_EQ(A,B) \
do{\
	if(A!=B)\
	{\
	    	printf("[ERROR][%s] LINE[%d], A=%d,b=%d\n",__FUNCTION__, __LINE__,A,B);\
	    	return FH_FAILURE;\
	}\
}while(0)

typedef FH_SINT32				AUDIO_DEV;
typedef FH_SINT32				AI_CHN;
typedef FH_SINT32				AO_CHN;
typedef FH_SINT32				AENC_CHN;
typedef FH_SINT32				ADEC_CHN;

typedef enum {
	DEV_I2S0 = 0,				//!< I2S 设备号 0
	DEV_I2S1,					//!< I2S 设备号 1
	DEV_I2S2,					//!< I2S 设备号 2
	DEV_I2S3,					//!< I2S 设备号 3
	DEV_I2S_MAX,
	DEV_ACW0,					//!< 内置codec 设备号 0
	DEV_ACW1,					//!< 内置codec 设备号 1
	DEV_DMIC0,					//!< DMIC 设备号 0
	DEV_DMIC1,					//!< DMIC 设备号 1
	DEV_EXT_CODEC,
	DEV_AUDIO_BUTT
}AUDIO_DEVID;

typedef enum {
	SELECT_TYPE_AI,				//!< 音频采集模式
	SELECT_TYPE_AO				//!< 音频播放模式
}AUDIO_IO_TYPE;

typedef enum
{
	AUDIO_SAMPLE_RATE_8000   = 8000,    	//!< 采样率 8K samplerate
	AUDIO_SAMPLE_RATE_12000  = 12000,   	//!< 采样率 12K samplerate
	AUDIO_SAMPLE_RATE_11025  = 11025,   	//!< 采样率 11.025K samplerate
	AUDIO_SAMPLE_RATE_16000  = 16000,   	//!< 采样率 16K samplerate
	AUDIO_SAMPLE_RATE_22050  = 22050,  		//!< 采样率 22.050K samplerate
	AUDIO_SAMPLE_RATE_24000  = 24000,  		//!< 采样率 24K samplerate
	AUDIO_SAMPLE_RATE_32000  = 32000,   	//!< 采样率 32K samplerate
	AUDIO_SAMPLE_RATE_44100  = 44100,   	//!< 采样率 44.1K samplerate
	AUDIO_SAMPLE_RATE_48000  = 48000,   	//!< 采样率 48K samplerate
	AUDIO_SAMPLE_RATE_96000  = 96000,   	//!< 采样率 96K samplerate
	AUDIO_SAMPLE_RATE_192000  = 192000,   	//!< 采样率 192K samplerate
	AUDIO_SAMPLE_RATE_BUTT,
} AUDIO_SAMPLE_RATE_E;

typedef enum
{
	AUDIO_BIT_WIDTH_8	= 8,				//!< 采样精度 8bit width
	AUDIO_BIT_WIDTH_16	= 16,				//!< 采样精度 16bit width
	AUDIO_BIT_WIDTH_20	= 20,				//!< 采样精度 20bit width
	AUDIO_BIT_WIDTH_24	= 24,				//!< 采样精度 24bit width
	AUDIO_BIT_WIDTH_32	= 32,				//!< 采样精度 32bit width
	AUDIO_BIT_WIDTH_BUTT,
} AUDIO_BIT_WIDTH_E;

typedef enum {
	AUDIO_CHN_NUM_1		= 0x1,				//!< 通道数 1
	AUDIO_CHN_NUM_2		= 0x2,				//!< 通道数 2
	AUDIO_CHN_NUM_4		= 0x4,				//!< 通道数 4
	AUDIO_CHN_NUM_8		= 0x8,				//!< 通道数 8
	AUDIO_CHN_NUM_16	= 0x10,				//!< 通道数 16
	AUDIO_CHN_NUM_20	= 0x14				//!< 通道数 20
}AUDIO_CHN_NUM_E;


typedef enum
{
	AIO_MODE_I2S_MASTER  = 0,				//!<  AIO I2S master mode
	AIO_MODE_I2S_SLAVE,						//!<  AIO I2S slave mode
	AIO_MODE_PCM_SLAVE_STD,					//!<  AIO PCM slave standard mode (not surpport)
	AIO_MODE_PCM_SLAVE_NSTD,				//!<  AIO PCM slave non-standard mode (not surpport)
	AIO_MODE_PCM_MASTER_STD,				//!<  AIO PCM master standard mode (not surpport)
	AIO_MODE_PCM_MASTER_NSTD,				//!<  AIO PCM master non-standard mode (not surpport)
	AIO_MODE_BUTT
} AIO_MODE_E;

typedef enum
{
	AUDIO_SOUND_MODE_MONO   =0,				//!< 单声道模式
	AUDIO_SOUND_MODE_STEREO =1,				//!< 立体声模式
	AUDIO_SOUND_MODE_BUTT
} AUDIO_SOUND_MODE_E;

typedef enum
{
	G726_16K = 0,							//!< G726 16kbps, see RFC3551.txt  4.5.4 G726-16
	G726_24K,								//!< G726 24kbps, see RFC3551.txt  4.5.4 G726-24 (not surpport)
	G726_32K,								//!< G726 32kbps, see RFC3551.txt  4.5.4 G726-32
	G726_40K,								//!< G726 40kbps, see RFC3551.txt  4.5.4 G726-40 (not surpport)
	MEDIA_G726_16K,							//!< G726 16kbps for ASF ... (not surpport)
	MEDIA_G726_24K,							//!< G726 24kbps for ASF ... (not surpport)
	MEDIA_G726_32K,							//!< G726 32kbps for ASF ... (not surpport)
	MEDIA_G726_40K,							//!< G726 40kbps for ASF ... (not surpport)
	G726_BUTT
} G726_BPS_E;

typedef enum {
	I2S_RTX_IDLE = 0,						//!< IDLE模式
	I2S_RXF_MODE,							//!< 数据接收模式
	I2S_TXF_MODE,							//!< 数据发送模式
	I2S_RTX_MODE							//!< 数据接收/发送复用模式
}AUDIO_RTX_MODE_E;

typedef enum
{
	VOLUME_TYPE_MIC,						//!< 内置codec MIC增益(仅MIC输入有效)
	VOLUME_TYPE_ANA,						//!< 内置codec 模拟增益(MIC/LINE IN输入/输出有效)
	VOLUME_TYPE_DIG,						//!< 内置codec 数字增益(MIC/LINE IN输入/输出有效)
	VOLUME_TYPE_VQE,						//!< VQE 模拟音量(仅 AUDIO_ALGO_MPP_MIC1 模式有效)
	VOLUME_TYPE_MAX
}VOLUME_TYPE;

typedef enum{
	IOC_SET_PUBATTR,						//!< 设置内置/外置codec设备参数
	IOC_GET_PUBATTR,						//!< 获取设备参数
	IOC_QUERY_CHN_STAT,						//!< 查询通道状态
	IOC_CMD_BUTT							//!< 外置CODEC 快启设备参数设置
}IOC_I2S_CMD;

typedef enum {
	AUDIO_AI_AO_SYNC_NONE=0xFE00,				//!< 音频AI/AO异步
	AUDIO_AI_AO_SYNC_BEGIN=0xFE01,			//!< 音频AI/AO同步开始
	AUDIO_AI_AO_SYNC_END=0xFE02,			//!< 音频AI/AO同步结束
}AUDIO_AIO_SYNC;


typedef enum{
	AC_INIT_CFG=IOC_CMD_BUTT+1,		/*  wrapper init cfg */
	AC_AI_CFG_CHN,					/*  AI config channel parameter */
	AC_AI_EN,						/*  enable AI  */
	AC_AI_DISABLE,					/*  disable AI  */
	AC_AI_PAUSE,					/*  pause AI  */
	AC_AI_RESUME,					/*  resume AI  */
	AC_AI_SET_VOL,					/*  AI set volume  */
	AC_AI_SET_DIG_VOL,				/*  AI set volume  */
	AC_AI_MICIN_SET_VOL,			/*  mic set volume  */
	AC_AI_SET_TRACK,				/*  AI set track mode  */
	AC_AI_GET_TRACK,				/*  AI get track mode  */
	AC_USING_EXTERNAL_CODEC,		/*  external api(reserved)  */
	AC_AO_CFG_CHN,					/*  AO config channel parameter */
	AC_AO_EN,						/*  enable AO  */
	AC_AO_DISABLE,					/*  disable AO  */
	AC_AO_PAUSE,					/*  pause AO  */
	AC_AO_RESUME,					/*  resume AO  */
	AC_AO_SET_VOL,					/*  AO set ANA volume  */
	AC_AO_SET_DIG_VOL,				/*  AO set digital volume  */
	AC_AO_SET_TRACK,				/*  AO set track mode  */
	AC_AO_GET_TRACK,				/*  AO get track mode  */
	AC_WORK_MODE,					/*  set workmode  */
	AC_EXT_INTF,					/*  reserved  */
	AC_GET_VERSION,					/*  codec get version  */
	AC_HPF_CTRL,					/*  high pass filter contrl  */
	AC_ALC_CTRL,					/*  automatic level control  */
	AC_NOTCE_CTRL,					/*  notch filter contrl  */
	AC_NOISE_GATE,					/*  noise gate contrl  */
	AC_DAC_LIMITER,					/*  dac limiter contrl  */
	AC_MUTE,						/*  codec output mute  */
	AC_IMCLK_CTRL,					/*  mclk contrl  */
	AC_EQ_CTRL,						/*  equalizer contrl  */
	AC_POWER_ON,					/*  codec power on  */
	AC_READ_SELECT,
	AC_READ_PTR_SYNC,
	AC_WRITE_SELECT,
	AC_WRITE_PTR_SYNC,
	AC_CMD_BUTT									//!< 内置codec 快启设备参数设置
}IOC_ACW_CMD;

typedef enum{
	DMIC_GET_PARAM=AC_CMD_BUTT+1,				//!< 获取DMIC设备参数
	DMIC_SET_PARAM,								//!< 设置DMIC设备参数
	DMIC_DEV_ENABLE,
	DMIC_DEV_DISABLE,
	DMIC_QUERY_DATA,
	DMIC_SET_FRMSIZE,
	DMIC_GET_FRAME,
	DMIC_CMD_BUTT								//!< DMIC快启设备参数配置
}IOC_DMIC_CMD;

typedef enum{
	EXT_GET_PLAYING_FRAME=DMIC_CMD_BUTT+1,		//!< 获取AO设备当前播放帧数据
	EXT_CMD_BUTT
}IOC_EXT_CMD;

typedef enum
{
	AUDIO_TRACK_LEFT		= 0,				//!< 左声道
	AUDIO_TRACK_RIGHT		= 1,				//!< 右声道
	AUDIO_TRACK_MIX			= 2,				//!< 立体声
	AUDIO_TRACK_EXCHANGE	= 3,
	AUDIO_TRACK_LEFT_MUTE	= 4,
	AUDIO_TRACK_RIGHT_MUTE	= 5,
	AUDIO_TRACK_BOTH_MUTE	= 6,
	AUDIO_TRACK_BUTT
} AUDIO_TRACK_MODE_E;

typedef enum
{
	AIO_ERR_VQE_ERR		= 65 ,					/*vqe error*/
} EN_AIO_ERR_CODE_E;

typedef enum
{
	AUDIO_ALGO_SES_MIC1	= 0,					//!< FH_SES 算法单MIC
	AUDIO_ALGO_SES_MICN,						//!< FH_SES 算法多MIC
	AUDIO_ALGO_MPP_MIC1,						//!< MPP 算法，仅支持单MIC
	AUDIO_ALGO_MOD_BUTT,
} AUDIO_ALGO_E;

typedef struct fhAUDIO_CHN_PARAM_S{
	FH_UINT32			u32FrmNum;
	FH_UINT32			u32FrmSize;
}AUDIO_CHN_PARAM_S;

typedef struct fhAIO_ATTR_S{
	AUDIO_SAMPLE_RATE_E	fs_rate;		//!< 采样率 8K:8000  16K:16000 32K:32000 48K:48000
	AUDIO_CHN_NUM_E		chn_num;		//!< 通道数 2,4,8,16,20 channel
	AUDIO_BIT_WIDTH_E	chn_width;		//!< 采样精度 8/16/32 bits
	AIO_MODE_E			is_slave;		//!< 主从模式 0: Master  1: Slave
	AUDIO_RTX_MODE_E	rxt_mode;		//!< 工作模式 1:rx 2:tx */
	AUDIO_SOUND_MODE_E	data_format;	//!< 数据格式 0:mono  1:stereo
	AUDIO_IO_TYPE		io_type;		//!< 参数类型 io type
	FH_UINT32			u32DmaNum;		//!< dma 缓存帧个数,default:3
	FH_UINT32			u32DmaSize;		//!< 单个DMA帧大小，需要和FRAME匹配
	FH_UINT32			u32Mclk;		//!< MCLK only for i2s,default: 12000000
	FH_UINT32			u32PhyAddr;		//!< 物理地址，应用层需设置为0
	FH_VOID*			pVirAddr;		//!< 虚拟地址，应用层需设置为NULL
}AIO_ATTR_S;

typedef struct fhAUDIO_PARAM_S{
	AUDIO_IO_TYPE		u32IT;
	FH_UINT32			u32Para;
}AUDIO_PARAM_S;

typedef struct fhDMIC_DMA_CFG
{
	FH_UINT32			u32DmaNum;		//!< DMIC dma 缓存帧个数,default:3
	FH_UINT32			u32DmaSize;		//!< DMIC 单个DMA帧大小，需要和FRAME匹配
	FH_UINT32			u32PhyAddr;		//!< 物理地址，应用层需设置为0
	FH_VOID*			pVirAddr;		//!< 虚拟地址，应用层需设置为NULL
}DMIC_DMA_CFG;

typedef struct fhDMIC_DEV_CFG
{
	FH_UINT32			id;				//!< DMIC 设备ID
	FH_UINT32			flt_num;		//!< 设备启动过滤无效点数，default：576，范围【0~4095】
	FH_UINT32			hpf;			//!< 高通滤波使能，【0~1】
	FH_UINT32			adc_vol;		//!< DMIC 音量,default:0x7A 范围【0x1B~0x7F】
	FH_UINT32			osr;			//!< 过采样标志，【0~1】32K/48K需配置，8K/16K不需要配置
	FH_UINT32			afifo_f;		//!< FIFO满阈值
	FH_UINT32			afifo_e;		//!< FIFO空阈值
	FH_UINT32			int_mask_dma;	//!< dma 中断屏蔽
	FH_UINT32			int_mask_dmic;	//!< dmic 中断屏蔽
	FH_UINT32			stereo_en;		//!< 立体声使能标志 【0~1】
	AUDIO_SAMPLE_RATE_E	fs_rate;		//!< 采样率
	DMIC_DMA_CFG		dma;			//!< DMA参数
}DMIC_DEV_CFG;

#pragma pack (4)
typedef struct fhAUDIO_FRAME_INFO{
	FH_SINT16			s16IOMode;
	FH_UINT32			u32DevId;
	FH_UINT32			u32ChnId;
	FH_UINT32			u32FrmSize;
	FH_UINT32			u32Timeout;
	FH_UINT32			u32PhyAddr;
	FH_UINT32			u32SeqId;
	FH_VOID*			pVirAddr;
	FH_UINT64			u64Pts;
	AUDIO_SOUND_MODE_E	eDataType;
}AUDIO_FRAME_INFO;

typedef struct fhAUDIO_FRAME_S
{
	FH_VOID*			pVirAddr;		//!< 数据物理地址
	FH_UINT32			u32PhyAddr;		//!< 数据虚拟地址
	FH_UINT32			u32Seq;			//!< 帧序号
	FH_UINT32			u32Len;			//!< 帧长度
	FH_UINT32			u32TimeOut;		//!< 超时时间
	FH_UINT64			u64TimeStamp;	//!< 时间戳
	AUDIO_SAMPLE_RATE_E	enFsRate;		//!< 采样率
	AUDIO_BIT_WIDTH_E	enBitwidth;		//!< 采样精度
	AUDIO_SOUND_MODE_E	enSoundmode;	//!< 声道模式
	FH_UINT32			u32ExpID;		//!< 扩展参数
} AUDIO_FRAME_S;

typedef struct fhAIO_DEV_ATTR
{
	FH_SINT32			s32DevFd;
	FH_SINT32			s32ExtFd;
	FH_SINT32			s32Reserved[2];
	AIO_ATTR_S			setParam;
	DMIC_DEV_CFG		stDmicAttr;
	FH_UINT64			u64ThrID;
	volatile FH_BOOL 	bThrdStart;
}AIO_DEV_ATTR;

typedef struct fhAUDIO_STREAM_S
{
	FH_UINT8*			pStream;				//!< 数据物理地址
	FH_UINT32			u32PhyAddr;				//!< 数据虚拟地址
	FH_UINT32			u32Len;					//!< 数据长度
	FH_UINT64			u64TimeStamp;			//!< 时间戳
	FH_UINT32			u32Seq;					//!< 帧序号
	AUDIO_SAMPLE_RATE_E		enFsRate;			//!< 采样率
	AUDIO_BIT_WIDTH_E		enBitwidth;			//!< 采样精度
	AUDIO_SOUND_MODE_E		enSoundmode;		//!< 声道模式
} AUDIO_STREAM_S;

typedef struct fhAUDIO_CHN_S
{
	volatile FH_BOOL	bEnabled;
	FH_VOID*			pVirAddr;
	FH_UINT32			u32PhyAddr;
	FH_UINT32			u32FrmNum;
	FH_UINT32			u32FrmSize;
	FH_UINT64			u64Pts;
	FH_UINT32			u32Seq;
	FH_SINT32			reserved[15];
}AUDIO_CHN_S;
#pragma pack ()

typedef struct fhAEC_FRAME_S
{
	AUDIO_FRAME_S		stRefFrame;		//!< 回声消除数据，暂未使用
	FH_BOOL				bValid; 		//!< 有效标志
	FH_BOOL				bSysBind;		//!< 绑定标志
} AEC_FRAME_S;

/**Defines the configure parameters of AGC.*/
typedef struct fhAUDIO_AGC_CONFIG_S
{
	FH_BOOL				bUsrMode;			//!< MPP 算法，USER模式，暂未使用，范围【0~1】
	FH_SINT16			s16OutputMode;		//!< MPP 算法，AGC增益模式，范围【0~3】，值越大幅度越大
	FH_SINT32			s32TargetGain;		//!< MPP 算法，AGC目标增益，范围【-255~0】，值越大幅度越大
	FH_UINT32			u32MaxAdjustGain;	//!< MPP 算法，AGC最大可调整增益范围，范围【0~255】
	FH_UINT32			u32MinAdjustGain;	//!< MPP 算法，AGC最小可调整增益范围，范围【0~u32MaxAdjustGain】
	FH_UINT32			u32AdjustSpeed;		//!< MPP 算法，AGC最大增益幅度，范围【0~u32MaxAdjustGain】
	FH_SINT32			s32IgnoreDb;		//!< MPP 算法，AGC忽略的增益范围，负值，低于这个分贝的值不做增益调整
	FH_SINT32			s32Reserved;
} AUDIO_AGC_CONFIG_S;

/**Defines the configure parameters of AEC.*/
typedef struct fhAI_AEC_CONFIG_S
{
	FH_BOOL				bUsrMode;			//!< MPP算法 用户模式
	FH_SINT16			s16gMode;			//!< MPP 算法，回声消除模式，范围【0~2】
	FH_SINT16			s16NlpMode;			//!< 回声参数1 default NlpModerate
	FH_SINT16			s16SkewMode;		//!< 回声参数2 default False
	FH_SINT16			s16MetricsMode;		//!< 回声参数3 default False
	FH_SINT16			s16DelayLogging;	//!< 回声参数4 default False
	FH_SINT32			s32Skew;			//!< 回声参数5 skew
	FH_UINT32			u32DelayTime;		//!< MPP 算法，扬声器播出到麦克风采集之间的延迟时间
	FH_SINT32			s32Reserved;		//!< 预留参数
} AI_AEC_CONFIG_S;

/**Defines the configure parameters of ANR.*/
typedef struct fhAUDIO_ANR_CONFIG_S
{
	FH_BOOL				bUsrMode;			//!< 暂未使用
	FH_SINT32			s32AnrLevel;		//!< MPP 算法，降噪级别，范围【0~3】
	FH_SINT32			s32Reserved;		//!< 预留参数
} AUDIO_ANR_CONFIG_S;

typedef struct fhAUDIO_HPF_CONFIG_S
{
	FH_UINT32			u32Freq;			//!< MPP 算法，高通滤波频段
} AUDIO_HPF_CONFIG_S;

/**Defines the configure parameters of VQE.*/
typedef struct fhAI_VQE_CONFIG_S
{
	FH_SINT32			bHpfOpen;			//!< MPP 算法，高通滤波使能标志
	FH_SINT32			bAecOpen;			//!< MPP 算法，回声消除使能标志
	FH_SINT32			bAnrOpen;			//!< MPP 算法，降噪使能标志
	FH_SINT32			bAgcOpen;			//!< MPP 算法，自动增益使能标志
	FH_SINT32			s32WorkSampleRate;	//!< MPP 算法，采样率，仅支持8K/16K

	AUDIO_HPF_CONFIG_S		stHpfCfg;		//!< MPP 算法，高通滤波参数
	AI_AEC_CONFIG_S			stAecCfg;		//!< MPP 算法，回声消除参数
	AUDIO_ANR_CONFIG_S		stAnrCfg;		//!< MPP 算法，降噪参数
	AUDIO_AGC_CONFIG_S		stAgcCfg;		//!< MPP 算法，自动增益标志
} AI_VQE_CONFIG_S;

typedef struct fhAI_VQE_PARAM_S
{
	AUDIO_DEV				AoDevID;		//!< MPP 算法，回声消除对应的AO设备ID
	AO_CHN					AoChn;			//!< MPP 算法，回声消除对应的AO通道ID
	AI_VQE_CONFIG_S			stVqeAttr;		//!< MPP 算法，VQE参数
}AI_VQE_PARAM_S;

typedef struct fhAO_VQE_CONFIG_S
{
	FH_SINT32				bHpfOpen;			//!< MPP 算法，高通滤波使能标志
	FH_SINT32				bAnrOpen;			//!< MPP 算法，降噪使能标志
	FH_SINT32				bAgcOpen;			//!< MPP 算法，自动增益使能标志
	FH_SINT32				s32WorkSampleRate;	//!< MPP 算法，采样率，仅支持8K/16K

	AUDIO_HPF_CONFIG_S		stHpfCfg;			//!< MPP 算法，高通滤波参数
	AUDIO_ANR_CONFIG_S		stAnrCfg;			//!< MPP 算法，降噪参数
	AUDIO_AGC_CONFIG_S		stAgcCfg;			//!< MPP 算法，自动增益标志
} AO_VQE_CONFIG_S;

typedef struct fhAI_FHSES_PARAM_S
{
	FH_SINT16			s16Idx;					//!< FHSES 算法，算法ID
	AUDIO_DEV			AoDevID;				//!< FHSES 算法，AO设备ID
	AO_CHN				AoChn;					//!< FHSES 算法，AO通道ID
	FH_AC_SesParam		stVqeAttr;				//!< FHSES 算法，算法参数
}AI_FHSES_PARAM_S;

typedef struct fhAO_FHSES_PARAM_S
{
	FH_SINT16			s16Idx;					//!< FHSES 算法，算法ID
	FH_AC_SesParam		stVqeAttr;				//!< FHSES 算法，算法参数
}AO_FHSES_PARAM_S;

/*Defines the configure parameters of AI saving file.*/
typedef struct fhAUDIO_SAVE_FILE_INFO_S
{
	FH_BOOL				bEnable;							//!< 使能标志
	FH_CHAR  			aFilePath[MAX_AUDIO_FILE_PATH_LEN];	//!< 文件保存路径
	FH_CHAR  			aFileName[MAX_AUDIO_FILE_NAME_LEN];	//!< 文件名
	FH_UINT32 			u32FileSize;						//!< 文件大小
} AUDIO_SAVE_FILE_INFO_S;

typedef struct fhAO_CHN_STATE_S
{
	FH_UINT32			u32ChnTotalNum;			//!< 总的缓存数目
	FH_UINT32			u32ChnFreeNum;			//!< 空闲的缓存数目
	FH_UINT32			u32ChnBusyNum;			//!< 使用的缓存数目
} AO_CHN_STATE_S;

/*Defines whether the file is saving or not .*/
typedef struct fhAUDIO_FILE_STATUS_S
{
	FH_BOOL				bSaving;
} AUDIO_FILE_STATUS_S;

#pragma pack (4)
typedef struct fhAIO_CHN_PARAM_S
{
	FH_SINT32			s32Dev;
	FH_SINT32			s32Chn;
	AUDIO_CHN_PARAM_S	stParam;
}AIO_CHN_PARAM_S;
#pragma pack ()

typedef struct fhVOLUME_PARAM_S
{
	FH_SINT32			s32Chn;					//!< 通道ID
	FH_UINT32			u32Vol;					//!< 音量
	VOLUME_TYPE			stType;					//!< 音量类型
}VOLUME_PARAM_S;

typedef struct fhRESAMPLE_PARAM_S
{
	AUDIO_SAMPLE_RATE_E	fs;						//!< 重采样后采样率
	FH_SINT32			param;					//!< 重采样后参数
}RESAMPLE_PARAM_S;

typedef struct fhDMIC_EN_CFG
{
	FH_SINT32			s32DevID;				//!< DMIC 设备ID
	FH_SINT32			s32ChnID;				//!< DMIC 通道ID
	FH_UINT32			u32Para;				//!< DMIC 参数
}DMIC_EN_CFG;

typedef struct fhFRAME_SYNC_INFO
{
	FH_SINT32			sw_ptr;
	FH_SINT32			data_len;
	FH_UINT32			timeout_ms;
}FRAME_SYNC_INFO;

typedef struct fhAUDIO_EXT_INFO
{
	FH_UINT16			u16Cmd;
	FH_UINT16			u16Len;
	FH_VOID*			p_param;
}AUDIO_EXT_INFO;

typedef struct
{
	FH_BOOL				bEnable;
	AUDIO_DEV			NearDev;
	AI_CHN				NearChn;
	AUDIO_DEV			FarDev;
	AI_CHN				FarChn;
}AI_HW_ECHO_INFO;

#pragma pack (4)
typedef struct
{
	FH_BOOL				bVqeEn;
	FH_SINT32			s32DevID;
	FH_SINT32			s32Chn;
	FH_UINT32			u32FrmTp;
	AUDIO_CHN_S			stCfg;
	AI_FHSES_PARAM_S	stVqe;
}AI_CHN_CFG_PROC;
#pragma pack ()

/* System define error code */
/** 无效的设备 	ID */
#define FH_ERR_AI_INVALID_DEVID     FH_DEF_ERR(FH_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
/** 无效的通道 	ID */
#define FH_ERR_AI_INVALID_CHNID     FH_DEF_ERR(FH_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/** 非法传入参数 */
#define FH_ERR_AI_ILLEGAL_PARAM     FH_DEF_ERR(FH_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/** 参数指针为空 */
#define FH_ERR_AI_NULL_PTR          FH_DEF_ERR(FH_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/** 参数未配置 */
#define FH_ERR_AI_NOT_CONFIG        FH_DEF_ERR(FH_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/** 不支持的参数/接口 */
#define FH_ERR_AI_NOT_SUPPORT       FH_DEF_ERR(FH_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/** 无效的操作 */
#define FH_ERR_AI_NOT_PERM          FH_DEF_ERR(FH_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/** 设备/通道未使能 */
#define FH_ERR_AI_NOT_ENABLED       FH_DEF_ERR(FH_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/** 内存申请失败 */
#define FH_ERR_AI_NOMEM             FH_DEF_ERR(FH_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/** 无内存 */
#define FH_ERR_AI_NOBUF             FH_DEF_ERR(FH_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/** 缓存为空 */
#define FH_ERR_AI_BUF_EMPTY         FH_DEF_ERR(FH_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/** 缓存已满 */
#define FH_ERR_AI_BUF_FULL          FH_DEF_ERR(FH_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/** 系统未初始化 */
#define FH_ERR_AI_SYS_NOTREADY      FH_DEF_ERR(FH_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/** 设备忙 */
#define FH_ERR_AI_BUSY              FH_DEF_ERR(FH_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
/** VQE配置出错 */
#define FH_ERR_AI_VQE_ERR       FH_DEF_ERR(FH_ID_AI, EN_ERR_LEVEL_ERROR, AIO_ERR_VQE_ERR)

/** 无效的设备 	ID */
#define FH_ERR_AO_INVALID_DEVID     FH_DEF_ERR(FH_ID_AO, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
/** 无效的通道 	ID */
#define FH_ERR_AO_INVALID_CHNID     FH_DEF_ERR(FH_ID_AO, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/** 非法传入参数 */
#define FH_ERR_AO_ILLEGAL_PARAM     FH_DEF_ERR(FH_ID_AO, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/** 参数指针为空 */
#define FH_ERR_AO_NULL_PTR          FH_DEF_ERR(FH_ID_AO, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/** 参数未配置 */
#define FH_ERR_AO_NOT_CONFIG        FH_DEF_ERR(FH_ID_AO, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/** 不支持的参数/接口 */
#define FH_ERR_AO_NOT_SUPPORT       FH_DEF_ERR(FH_ID_AO, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/** 无效的操作 */
#define FH_ERR_AO_NOT_PERM          FH_DEF_ERR(FH_ID_AO, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/** 设备/通道未使能 */
#define FH_ERR_AO_NOT_ENABLED       FH_DEF_ERR(FH_ID_AO, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/** 内存申请失败 */
#define FH_ERR_AO_NOMEM             FH_DEF_ERR(FH_ID_AO, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/** 无内存 */
#define FH_ERR_AO_NOBUF             FH_DEF_ERR(FH_ID_AO, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/** 缓存为空 */
#define FH_ERR_AO_BUF_EMPTY         FH_DEF_ERR(FH_ID_AO, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/** 缓存已满 */
#define FH_ERR_AO_BUF_FULL          FH_DEF_ERR(FH_ID_AO, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/** 系统未初始化 */
#define FH_ERR_AO_SYS_NOTREADY      FH_DEF_ERR(FH_ID_AO, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/** 设备忙 */
#define FH_ERR_AO_BUSY              FH_DEF_ERR(FH_ID_AO, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
/** VQE配置出错 */
#define FH_ERR_AO_VQE_ERR       FH_DEF_ERR(FH_ID_AO, EN_ERR_LEVEL_ERROR, AIO_ERR_VQE_ERR)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __FH_COMM_AIO_H__ */
