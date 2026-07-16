#ifndef __FH_VO_MPIPARA_H__
#define __FH_VO_MPIPARA_H__
#include "fh_errno.h"
#include "fh_video.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

#define VO_DEF_CHN_BUF_LEN		8
#define VO_DEF_DISP_BUF_LEN		5
#define VO_DEF_VIRT_BUF_LEN		3
#define VO_DEF_WBC_DEPTH_LEN		8

/* vo inteface type */
#define VO_INTF_CVBS			(0x01L<<0)
#define VO_INTF_YPBPR			(0x01L<<1)
#define VO_INTF_VGA			(0x01L<<2)
#define VO_INTF_BT656			(0x01L<<3)
#define VO_INTF_BT1120			(0x01L<<4)
#define VO_INTF_HDMI			(0x01L<<5)
#define VO_INTF_LCD			(0x01L<<6)
#define VO_INTF_BT656_H			(0x01L<<7)
#define VO_INTF_BT656_L			(0x01L<<8)
#define VO_INTF_AHD			(0x01L<<9)


/* WBC channel id*/
#define VO_WBC_CHN_ID			(VO_MAX_CHN_NUM + 1)

#define VO_DEFAULT_CHN			-1	/* use vo buffer as pip buffer */

typedef FH_SINT32 VO_INTF_TYPE_E;

typedef enum
{
	VO_DHD0 = 0,				//!< 高清设备
	VO_DSD0 = 1				//!< 标清设备
}VO_DEV_ID_E;

typedef enum
{
	VO_LAYER_VHD0 = 0,			//!< 高清视频层
	VO_LAYER_VSD0 = 1,			//!< 标清视频层
	VO_LAYER_VPIP = 2,			//!< PIP层
	VO_LAYER_WBC = 3			//!< 回写视频层
}VO_LAYER_ID_E;

typedef enum
{
	VO_OUTPUT_USER = 0,			//!< 用户自定义模式

	VO_OUTPUT_PAL = 1,			//!< 720x576 @ 25fps
	VO_OUTPUT_NTSC = 2,			//!< 720x480 @ 30fps
	VO_OUTPUT_960H_NTSC = 3,     		//!< 480x960 @ 60fps
	VO_OUTPUT_576P50 = 4,			//!< 720x576 @ 50fps
	VO_OUTPUT_480P60 = 5,			//!< 720x480 @ 60fps
	VO_OUTPUT_720P50 = 6,			//!< 1280x720 @ 50fps
	VO_OUTPUT_720P60 = 7,			//!< 1280x720 @ 60fps
	VO_OUTPUT_1080P24 = 8,			//!< 1920x1080 @ 24fps
	VO_OUTPUT_1080P25 = 9,			//!< 1920x1080 @ 25fps
	VO_OUTPUT_1080P30 = 10,			//!< 1920x1080 @ 30fps
	VO_OUTPUT_1080P50 = 11,			//!< 1920x1080 @ 50fps
	VO_OUTPUT_1080P60 = 12,			//!< 1920x1080 @ 60fps
	VO_OUTPUT_1080I50 = 13,			//!< 1920x1080 @ 25fps
	VO_OUTPUT_1080I60 = 14,			//!< 1920x1080 @ 30fps
	VO_OUTPUT_800x480_60 = 15,		//!< 800x480 @ 60fps
	VO_OUTPUT_800x600_60 = 16,		//!< 800x600 @ 60fps
	VO_OUTPUT_1024x768_60 = 17,		//!< 1024x768 @ 60fps
	VO_OUTPUT_1024x600_60 = 18,		//!< 1024x600 @ 60fps
	VO_OUTPUT_1280x1024_60 = 19, 		//!< 1280x1024 @ 60fps
	VO_OUTPUT_1366x768_60 = 20,		//!< 1366x768 @ 60fps
	VO_OUTPUT_1440x900_60 = 21,		//!< 1440x900 @ 60fps
	VO_OUTPUT_1280x800_60 = 22,		//!< 1280x800 @ 60fps
	VO_OUTPUT_1680x1050_60 = 23, 		//!< 1680x1050 @ 60fps
	VO_OUTPUT_1600x1200_60 = 24, 		//!< 1600x1200 @ 60fps
	VO_OUTPUT_1920x1200_60 = 25, 		//!< 1920x1200 @ 60fps
	VO_OUTPUT_2560x1440_30 = 26, 		//!< 2560x1440 @ 30fps
	VO_OUTPUT_2560x1440_60 = 27, 		//!< 2560x1440 @ 60fps
	VO_OUTPUT_2560x1600_60 = 28, 		//!< 2560x1600 @ 60fps
	VO_OUTPUT_3840x2160_30 = 29, 		//!< 3840x2160 @ 30fps
	VO_OUTPUT_3840x2160_60 = 30, 		//!< 3840x2160 @ 60fps
	VO_OUTPUT_4096x2160_30 = 31, 		//!< 4096x2160 @ 60fps
	VO_OUTPUT_240x320_60 = 32,		//!< 240x320 @ 60fps
	VO_OUTPUT_480x854_60 = 33,		//!< 480x854 @ 60fps
	VO_OUTPUT_640x480_60 = 34,		//!< 640x480 @ 60fps
	VO_OUTPUT_800x1280_60 = 35,		//!< 800x1280 @ 60fps
	VO_OUTPUT_1200x576_50 = 36,		//!< 1200x576 @ 50fps
	VO_OUTPUT_3840x2160_15 = 37,		//!< 3840x2160 @ 15fps
	VO_OUTPUT_1280x960_60 = 38,		//!< 1280x960 @ 60fps
	VO_OUTPUT_720x540_60 = 39,		//!< 720x540 @ 60fps
	VO_OUTPUT_960x1280_60 = 40,		//!< 960x1280 @ 60fps
	VO_OUTPUT_720x575_50 = 41,		//!< 720x575 @ 50fps
	VO_OUTPUT_720x1280_60 = 42,		//!< 720x1280 @ 60fps
	VO_OUTPUT_720P25 = 43,			//!< 1280x720 @ 25fps
	VO_OUTPUT_720P30 = 44,			//!< 1280x720 @ 30fps
	VO_OUTPUT_1080x1920_60 = 45,		//!< 1080x1920 @ 60fps

	VO_OUTPUT_BUTT
} VO_INTF_SYNC_E;

typedef enum
{
	VO_FIELD_TOP = 0,			//!< 通道视频输出时只处理顶场
	VO_FIELD_BOTTOM = 1,			//!< 通道视频输出时只处理底场
	VO_FIELD_BOTH = 2,			//!< 通道视频输出为两场
	VO_FIELD_BUTT
} VO_DISPLAY_FIELD_E;

typedef enum
{
	VO_CSC_MATRIX_IDENTITY = 0, 		//!< 单位转换矩阵，不改变色彩空间
	VO_CSC_MATRIX_BT601_TO_BT709 = 1,	//!< BT.601 LIMIT 到 BT.709 LIMIT色彩空间的 CSC 矩阵
	VO_CSC_MATRIX_BT709_TO_BT601 = 2,	//!< BT.709 LIMIT 到 BT.601 LIMIT色彩空间的 CSC 矩阵
	VO_CSC_MATRIX_BT601_TO_RGB_PC = 3,	//!< BT.601 FULL 到 RGB 色彩空间的 CSC 矩阵
	VO_CSC_MATRIX_BT709_TO_RGB_PC = 4,	//!< BT.709 FULL 到 RGB 色彩空间的 CSC 矩阵
	VO_CSC_MATRIX_RGB_TO_BT601_PC = 5,	//!< RGB 到 BT.601 FULL 色彩空间的 CSC 矩阵
	VO_CSC_MATRIX_RGB_TO_BT709_PC = 6,	//!< RGB 到 BT.709 FULL 色彩空间的 CSC 矩阵

	VO_CSC_MATRIX_BUTT
} VO_CSC_MATRIX_E;

typedef enum {
	HDMI_FORMAT_RGB888 = 0,
	HDMI_FORMAT_YUV444,
	HDMI_FORMAT_EXT_YUV422_8BIT,
	HDMI_FORMAT_EXT_YUV422_16BIT,
	HDMI_FORMAT_EXT_YUV444_24BIT,
	HDMI_FORMAT_BT_1120,
	HDMI_FORMAT_NUM,
} hdmi_format_e;

typedef struct
{
	FH_UINT32	u32Priority;		//!< 通道优先级
	RECT_S		stRect;			//!< 通道显示区域
	FH_BOOL 	bDeflicker; 		//!< 是否开启抗闪烁。取值范围：[0-1]
}VO_CHN_ATTR_S;

typedef enum fyASPECT_RATIO_E
{
	ASPECT_RATIO_NONE   = 0,		//!< 全屏显示
	ASPECT_RATIO_AUTO   = 1,		//!< 自动缩放, 1:1
	ASPECT_RATIO_MANUAL = 2,		//!< 手动缩放
	ASPECT_RATIO_BUTT
}ASPECT_RATIO_E;

typedef struct fyASPECT_RATIO_S
{
	ASPECT_RATIO_E enMode;			//!< 缩放模式: none/auto/manual
	FH_UINT32 u32BgColor;			//!< 背景色, RGB 888
	RECT_S  stVideoRect;			//!< 有效区域，valid in ASPECT_RATIO_MANUAL mode
} ASPECT_RATIO_S;


typedef struct fyALPHA_S
{
	FH_BOOL bAlphaEnable;			/**< alpha enable flag */
	FH_BOOL bAlphaChannel;			/**< alpha channel enable flag */
	FH_UINT8 u8Alpha0;			/**< alpha0 value, used in ARGB1555 */
	FH_UINT8 u8Alpha1;			/**< alpha1 value, used in ARGB1555 */
	FH_UINT8 u8GlobalAlpha;			/**< global alpha value */
	FH_UINT8 u8Reserved;
} ALPHA_S;


typedef struct fyBORDER_S
{
	FH_UINT32 u32TopWidth;			//!< top border weight, in pixel
	FH_UINT32 u32BottomWidth;		//!< bottom border weight, in pixel
	FH_UINT32 u32LeftWidth;			//!< left border weight, in pixel
	FH_UINT32 u32RightWidth;		//!< right border weight, in pixel
	FH_UINT32 u32Color;			//!< border color, RGB888
} BORDER_S;

typedef struct
{
	ASPECT_RATIO_S stAspectRatio;		//!< 幅型比
}VO_CHN_PARAM_S;

typedef struct
{
	FH_UINT32 	u32ChnBufUsed;		//!< 视频输出通道当前占用的视频 buffer 数目
	FH_UINT32 	u32FrameRecv;		//!< 视频输出通道当前接收的视频 buffer 数目
	FH_UINT32 	u32FrameDisp;		//!< 视频输出通道当前显示的视频 buffer 数目
	FH_UINT32 	u32FrameDrop;		//!< 视频输出通道当前丢弃的视频 buffer 数目
} VO_QUERY_STATUS_S;

typedef struct
{
	FH_BOOL		bSynm;			//!< 是否开启同步模式(0:timing,as BT.656; 1:signal,as LCD)。取值范围：[0-1]
	FH_BOOL		bIop;			//!< 隔行还是逐行输出(0:隔行; 1:逐行)。取值范围：[0-1]
	FH_UINT8	u8Intfb;		//!< 输出时交错位宽。取值范围：[0-255]

	FH_UINT16	u16Vact;		//!< 垂直活动区
	FH_UINT16	u16Vbb;			//!< 垂直消隐后肩
	FH_UINT16	u16Vfb;			//!< 垂直消隐前肩

	FH_UINT16	u16Hact;		//!< 水平活动区
	FH_UINT16	u16Hbb;			//!< 水平消隐后肩
	FH_UINT16	u16Hfb;			//!< 水平消隐前肩
	FH_UINT16	u16Hmid;		//!< 底部水平活动区域

	FH_UINT16	u16Bvact;		//!< 底部垂直活动区
	FH_UINT16	u16Bvbb;		//!< 底部垂直消隐后肩
	FH_UINT16	u16Bvfb;		//!< 底部垂直消隐前肩

	FH_UINT16	u16Hpw;			//!< 水平脉宽
	FH_UINT16	u16Vpw;			//!< 垂直脉宽

	FH_BOOL		bIdv;			//!< 是否输出反相数据。取值范围：[0-1]
	FH_BOOL		bIhs;			//!< 是否反向水平同步信号有效。取值范围：[0-1]
	FH_BOOL		bIvs;			//!< 是否反向垂直同步信号有效。取值范围：[0-1]

	FH_UINT32	u32Clk;			//!< 用户时钟
} VO_SYNC_INFO_S;

typedef struct
{
	FH_UINT32	u32BgColor;		//!< 设备背景色 RGB表示
	VO_INTF_TYPE_E	enIntfType;		//!< Vo接口类型
	VO_INTF_SYNC_E	enIntfSync;		//!< Vo接口时序类型
	VO_SYNC_INFO_S	stSyncInfo;		//!< Vo接口时序信息
} VO_PUB_ATTR_S;

typedef struct
{
	FH_SIZE			stTargetSize;	//!< 视频回写图像大小
	PIXEL_FMT_E		enPixelFormat;	//!< 视频回写图像数据格式
	FH_UINT32		u32FrameRate;	//!< 视频回写图像帧率控制
} VO_WBC_ATTR_S;

typedef enum
{
	VO_WBC_MODE_NOMAL = 0,			//!< 视频回写普通模式。仅根据设备帧率和回写帧率来控制采集图像
	VO_WBC_MODE_DROP_REPEAT = 1,		//!< 丢弃重复显示帧模式。仅在高清设备 HD 是逐行显示时才有效
	VO_WBC_MODE_PROG_TO_INTL = 2,		//!< 逐行显示图像转隔行图像模式。仅在高清设备 HD 是逐行显示时才有效
	VO_WBC_MODE_BUTT
} VO_WBC_MODE_E;

typedef enum
{
	VO_WBC_SOURCE_DEV	= 0x0,		//!< 来自设备的回写源
	VO_WBC_SOURCE_VIDEO	= 0x1,		//!< 来自视频层的回写源
	VO_WBC_SOURCE_GRAPHIC = 0x2,		//!< 来自图形层的回写源
	VO_WBC_SOURCE_BUTT
} VO_WBC_SOURCE_TYPE_E;

typedef struct
{
	VO_WBC_SOURCE_TYPE_E enSourceType;	//!< 视频回写源类型枚举。取值范围：[0x0-0x2]
	FH_UINT32 u32SourceId;			//!< 视频回写源的编号。取值范围：[设备、视频层或图像层]
} VO_WBC_SOURCE_S;

typedef enum
{
	VO_PART_MODE_SINGLE	= 0,		//!< 单分割模式，会用软件拼接多个画面
	VO_PART_MODE_MULTI	= 1,		//!< 多分割模式
	VO_PART_MODE_BUTT
} VO_PART_MODE_E;

typedef struct
{
	FH_BOOL		bSupportCompress;	//!< 是否支持压缩。取值范围：[0-1]
}VO_COMPRESS_ATTR_S;

typedef struct
{
	RECT_S		stDispRect;		//!< 显示分辨率大小
	FH_SIZE		stImageSize;		//!< 视频层画布大小
	FH_UINT32	u32DispFrmRt;		//!< 显示帧率
	PIXEL_FMT_E	enPixFormat;		//!< 视频层使用的像素格式
	FH_BOOL		bDoubleFrame;		//!< 视频层是否开启倍帧。取值范围：[0-1]
	FH_BOOL		bClusterMode;		//!< 视频层上的通道是否采用聚集的方式使用内存。取值范围：[0-1]
} VO_VIDEO_LAYER_ATTR_S;

typedef struct
{
	VO_CSC_MATRIX_E	enCscMatrix;        	//!< CSC矩阵选择
	FH_UINT32	u32Luma;		//!< 亮度值。取值范围：[0-100, 默认50]
	FH_UINT32	u32Contrast;		//!< 对比度值。取值范围：[0-100, 默认50]
	FH_UINT32	u32Hue;			//!< 色调值。取值范围：[0-100, 默认50]
	FH_UINT32	u32Saturation;		//!< 饱和度值。取值范围：[0-100, 默认50]
} VO_CSC_S;

typedef enum
{
	VO_TEST_MODE_COLOR_BLOCK	= 0x0,	//!< 彩格测试输出
	VO_TEST_MODE_COLOR_BAR		= 0x1,	//!< 彩条测试输出
	VO_TEST_MODE_PURE_COLOR		= 0x2,	//!< 纯色测试输出
	VO_TEST_MODE_CHESS_BOARD	= 0x3,	//!< 棋盘测试输出
}VO_TEST_MODE_E;

typedef struct
{
	FH_BOOL		bEnable;		//!< 测试模式是否有效
	VO_INTF_SYNC_E	enOutput;		//!< Vo接口时序类型
	VO_INTF_TYPE_E	enIntfType;		//!< Vo接口类型
	VO_TEST_MODE_E	enTestMode;		//!< 测试模式
	FH_UINT16	u16BlockW;		//!< 格宽
	FH_UINT16	u16BlockH;		//!< 格高
}VO_TEST_PARAM_S;

typedef struct
{
	FH_UINT32 bHdmiFmt;            		//!< HDMI显示分辨率
} VO_HDMI_FMT_S;

typedef enum fyVO_SAMPLE_RATE_E
{
	VO_SAMPLE_RATE_UNKNOWN	= 0,		//!< unknown sample rate
	VO_SAMPLE_RATE_8K	= 8000,		//!< 8K sample rate
	VO_SAMPLE_RATE_11K	= 11025,	//!< 11.025K sample rate
	VO_SAMPLE_RATE_12K	= 12000,	//!< 12K sample rate
	VO_SAMPLE_RATE_16K	= 16000,	//!< 16K sample rate
	VO_SAMPLE_RATE_22K	= 22050,	//!< 22.050K sample rate
	VO_SAMPLE_RATE_24K	= 24000,	//!< 24K sample rate
	VO_SAMPLE_RATE_32K	= 32000,	//!< 32K sample rate
	VO_SAMPLE_RATE_44K	= 44100,	//!< 44.1K sample rate
	VO_SAMPLE_RATE_48K	= 48000,	//!< 48K sample rate
	VO_SAMPLE_RATE_88K	= 88200,	//!< 88.2K sample rate
	VO_SAMPLE_RATE_96K	= 96000,	//!< 96K sample rate
	VO_SAMPLE_RATE_176K	= 176400,	//!< 176K sample rate
	VO_SAMPLE_RATE_192K	= 192000,	//!< 192K sample rate
	VO_SAMPLE_RATE_BUTT
} VO_SAMPLE_RATE_E;

typedef enum fyVOU_ZOOM_IN_E
{
	VOU_ZOOM_IN_RECT = 0,			//!< zoom in by rect
	VOU_ZOOM_IN_RATIO,			//!< zoom in by ratio
	VOU_ZOOM_IN_BUTT
} VOU_ZOOM_IN_E;

typedef enum fyVO_CAS_MODE_E
{
	VO_CAS_MODE_SINGLE = 0,			//!< cascade mode is single
	VO_CAS_MODE_DUAL,			//!< cascade mode is dual
	VO_CAS_MODE_BUTT,
} VO_CAS_MODE_E;

typedef enum fyVO_CAS_DATA_TRAN_MODE_E
{
	VO_CAS_DATA_SINGLE_TRAN_MODE = 0,	//!< single transition,clock rising edge or clock falling edge tigger transition
	VO_CAS_DATA_DOUBLE_TRAN_MODE,		//!< double transition,clock rising edge and clock falling edge tigger transition
	VO_CAS_DATA_MODE_BUTT,
} VO_CAS_DATA_TRAN_MODE_E;

typedef enum fyVO_CAS_RGN_E
{
	VO_CAS_64_RGN = 0,
	VO_CAS_32_RGN,
	VO_CAS_RGN_BUTT,
} VO_CAS_RGN_E; 					//!< cascade region number

typedef struct fyVO_CAS_ATTR_S
{
	FH_BOOL		bSlave;				//!< FH_TRUE: slave mode, FH_FALSE: master mode
	VO_CAS_RGN_E	enCasRgn;			//!< cascade region number
	VO_CAS_MODE_E	enCasMode;			//!< cascade mode
	VO_CAS_DATA_TRAN_MODE_E enCasDataTranMode;	//!< cascade data transition mode
} VO_CAS_ATTR_S;
typedef enum fyVOU_LAYER_DDR_E
{
	VOU_LAYER_DDR0 = 0,
	VOU_LAYER_DDR1 = 1,
	VOU_LAYER_DDR_BUTT
}VOU_LAYER_DDR_E;

typedef enum fyVO_DEVINTF_STATUS_E
{
	VO_DEV_STATUS_NOPLUG = 0,			//!< HDMI设备未插入
	VO_DEV_STATUS_PLUG,				//!< HDMI设备已接入
	VO_DEV_STATUS_BUTT
}VO_DEVINTF_STATUS_E;

typedef struct fyVO_ZOOM_RATIO_S
{
	FH_UINT32	u32XRatio;			//!< 水平坐标缩放比
	FH_UINT32	u32YRatio;			//!< 垂直坐标缩放比
	FH_UINT32	u32WRatio;			//!< 宽度缩放比
	FH_UINT32	u32HRatio;			//!< 高度缩放比
} VO_ZOOM_RATIO_S;

typedef struct fyVO_ZOOM_ATTR_S
{
	VOU_ZOOM_IN_E		enZoomType;		//!< 缩放类型
	union {
		RECT_S		stZoomRect;		//!< 缩放区域
		VO_ZOOM_RATIO_S	stZoomRatio;		//!< 缩放比列
	};
} VO_ZOOM_ATTR_S;

typedef struct fyVO_BORDER_S
{
	FH_BOOL bBorderEn;				//!< 边沿使能
	BORDER_S stBorder;				//!< 边沿参数
}VO_BORDER_S;

typedef struct fyVo_CM_S
{
	FH_BOOL		bEnable;
	FH_SINT8		s8Brightness;		//!< 亮度参数，范围	-128 ~ 127
	FH_UINT16		u16UVSatvXContrs;	//!< UV参数对比度，范围	2.9 bits
	FH_SINT8		s8HueSin;		//!< HUE sin，范围	-32 ~ 32
	FH_SINT8		s8HueCos;		//!< HUE cos，范围	-32 ~ 32
	FH_UINT8		u8SharpThr[2];		//!< 锐度阈值，范围 0 ~ 255
	FH_UINT8		u8SharpK[2];		//!< 锐度K，范围      3.1 bits
	FH_SINT8		s8YContrastOffset;	//!< 对比度偏移，范围 -128 ~ 127
	FH_UINT8		u8YContrastSlope;	//!< 对比度值，1.4 bits
} VO_CM_S;

typedef struct fyVO_VGA_PARAM_S
{
	VO_CSC_S	stCSC;				//!< 色彩空间矩阵
	FH_UINT32	u32Gain;			//!< 增益 current gain of VGA signals. [0, 64). default:0x30
	FH_SINT32	s32SharpenStrength;		//!< 锐度强度  ，current sharpen strength of VGA signals. [0, 255]. default:0x80
} VO_VGA_PARAM_S;

typedef struct fyVO_HDMI_PARAM_S
{
	VO_CSC_S	stCSC;				//!< 色彩空间矩阵
} VO_HDMI_PARAM_S;

typedef struct fyVO_HDMI_AUDIO_S
{
	VO_SAMPLE_RATE_E enSampleRate;			//!<  HDMI 音频采样率
} VO_HDMI_AUDIO_S;

typedef struct fyVO_REGION_INFO_S
{
	RECT_S		*pstRegion;			//!< region 区域
	FH_UINT32	u32RegionNum;			//!< region 计数
}VO_REGION_INFO_S;

typedef struct fyVO_DEVINTF_STATUS_S
{
	VO_DEVINTF_STATUS_E enPlugStatus;		//!< 热插拔状态
}VO_DEVINTF_STATUS_S;

typedef struct fyVo_Matrix_S
{
	FH_BOOL		bEnable;			//!< 使能状态
	FH_SINT16	s16Coef[3][3];			//!< 矩阵系数
	FH_SINT16	s16Offset[3];			//!< 参数
} VO_MATRIX_S;

typedef struct fyVo_HDMI_EDID_S
{
	FH_BOOL		bValid;				//!< 有效标志
	FH_UINT32	u32Len;				//!< 数据长度
	FH_UINT8	u8Data[512];			//!< EDID内容
} VO_HDMI_EDID_S;

typedef struct fyVo_HDMI_POL_S
{
	FH_BOOL		bDV;				//!< data valid of output,    default FH_FALSE
	FH_BOOL		bHSS;				//!< horizontal synch signal, default FH_FALSE
	FH_BOOL		bVSS;				//!< vertical synch signal,   default FH_FALSE
} VO_HDMI_POL_S;

/*
typedef enum {
	HDMI_FORMAT_RGB888 = 0,
	HDMI_FORMAT_YUV444,
	HDMI_FORMAT_EXT_YUV422_8BIT,
	HDMI_FORMAT_EXT_YUV422_16BIT,
	HDMI_FORMAT_EXT_YUV444_24BIT,
	HDMI_FORMAT_NUM,
} hdmi_format_e;
*/

typedef enum
{
	EN_ERR_VO_DEV_NOT_CONFIG	= 0x40,
	EN_ERR_VO_DEV_NOT_ENABLE	= 0x41,
	EN_ERR_VO_DEV_HAS_ENABLED	= 0x42,
	EN_ERR_VO_DEV_HAS_BINDED	= 0x43,
	EN_ERR_VO_DEV_NOT_BINDED	= 0x44,

	ERR_VO_NOT_ENABLE		= 0x45,
	ERR_VO_NOT_DISABLE		= 0x46,
	ERR_VO_NOT_CONFIG		= 0x47,

	ERR_VO_CHN_NOT_DISABLE		= 0x48,
	ERR_VO_CHN_NOT_ENABLE		= 0x49,
	ERR_VO_CHN_NOT_CONFIG		= 0x4a,
	ERR_VO_CHN_NOT_ALLOC		= 0x4b,

	ERR_VO_CCD_INVALID_PAT		= 0x4c,
	ERR_VO_CCD_INVALID_POS		= 0x4d,

	ERR_VO_WAIT_TIMEOUT		= 0x4e,
	ERR_VO_INVALID_VFRAME		= 0x4f,
	ERR_VO_INVALID_RECT_PARA	= 0x50,
	ERR_VO_SETBEGIN_ALREADY 	= 0x51,
	ERR_VO_SETBEGIN_NOTYET		= 0x52,
	ERR_VO_SETEND_ALREADY		= 0x53,
	ERR_VO_SETEND_NOTYET		= 0x54,

	ERR_VO_GRP_INVALID_ID		= 0x55,
	ERR_VO_GRP_NOT_CREATE		= 0x56,
	ERR_VO_GRP_HAS_CREATED		= 0x57,
	ERR_VO_GRP_NOT_DESTROY		= 0x58,
	ERR_VO_GRP_CHN_FULL		= 0x59,
	ERR_VO_GRP_CHN_EMPTY		= 0x5a,
	ERR_VO_GRP_CHN_NOT_EMPTY	= 0x5b,
	ERR_VO_GRP_INVALID_SYN_MODE	= 0x5c,
	ERR_VO_GRP_INVALID_BASE_PTS	= 0x5d,
	ERR_VO_GRP_NOT_START		= 0x5e,
	ERR_VO_GRP_NOT_STOP		= 0x5f,
	ERR_VO_GRP_INVALID_FRMRATE	= 0x60,
	ERR_VO_GRP_CHN_HAS_REG		= 0x61,
	ERR_VO_GRP_CHN_NOT_REG		= 0x62,
	ERR_VO_GRP_CHN_NOT_UNREG	= 0x63,
	ERR_VO_GRP_BASE_NOT_CFG 	= 0x64,

	ERR_GFX_NOT_DISABLE		= 0x65,
	ERR_GFX_NOT_BIND		= 0x66,
	ERR_GFX_NOT_UNBIND		= 0x67,
	ERR_GFX_INVALID_ID		= 0x68,

	ERR_VO_WBC_NOT_DISABLE		= 0x69,
	ERR_VO_WBC_NOT_CONFIG		= 0x6a,

	ERR_VO_CHN_AREA_OVERLAP 	= 0x6b,

	EN_ERR_INVALID_WBCID		= 0x6c,
	EN_ERR_INVALID_LAYERID		= 0x6d,
	EN_ERR_VO_VIDEO_HAS_BINDED	= 0x6e,
	EN_ERR_VO_VIDEO_NOT_BINDED	= 0x6f,
	ERR_VO_WBC_HAS_BIND		= 0x70,
	ERR_VO_WBC_HAS_CONFIG		= 0x71,
	ERR_VO_WBC_NOT_BIND		= 0x72,

	/* new added */
	ERR_VO_BUTT

}EN_VOU_ERR_CODE_E;

/* System define error code */
/** @brief 0xA00F8012: 资源忙 */
#define FH_ERR_VO_BUSY				FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
/** @brief 0xA00F800C: 内存不足 */
#define FH_ERR_VO_NO_MEM			FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/** @brief 0xA00F8006: 函数参数中有空指针 */
#define FH_ERR_VO_NULL_PTR			FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/** @brief 0xA00F8010: 系统未初始化 */
#define FH_ERR_VO_SYS_NOTREADY			FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/** @brief 0xA00F8001: 设备 ID 超出合法范围 */
#define FH_ERR_VO_INVALID_DEVID			FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
/** @brief 0xA00F8002: 通道 ID 超出合法范围 */
#define FH_ERR_VO_INVALID_CHNID			FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/** @brief 0xA00F8003: 参数超出合法范围 */
#define FH_ERR_VO_ILLEGAL_PARAM			FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/** @brief 0xA00F8008: 不支持的操作 */
#define FH_ERR_VO_NOT_SUPPORT			FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/** @brief 0xA00F8009: 操作不允许 */
#define FH_ERR_VO_NOT_PERMIT			FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/** @brief 0xA00F806C: WBC 号超出范围 */
#define FH_ERR_VO_INVALID_WBCID 		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_WBCID)
/** @brief 0xA00F806D: 视频层号超出范围 */
#define FH_ERR_VO_INVALID_LAYERID		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_LAYERID)


/** @brief 0xA00F8040: 设备未配置 */
#define FH_ERR_VO_DEV_NOT_CONFIG		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_VO_DEV_NOT_CONFIG)
/** @brief 0xA00F8041: 设备未使能 */
#define FH_ERR_VO_DEV_NOT_ENABLE		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_VO_DEV_NOT_ENABLE)
/** @brief 0xA00F8042: 设备已使能 */
#define FH_ERR_VO_DEV_HAS_ENABLED		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_VO_DEV_HAS_ENABLED)
/** @brief 0xA00F8043: 设备已被绑定 */
#define FH_ERR_VO_DEV_HAS_BINDED		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_VO_DEV_HAS_BINDED)
/** @brief 0xA00F8044: 设备未被绑定 */
#define FH_ERR_VO_DEV_NOT_BINDED		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_VO_DEV_NOT_BINDED)

/** @brief 0xA00F8045: 视频层未使能 */
#define FH_ERR_VO_VIDEO_NOT_ENABLE		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_NOT_ENABLE)
/** @brief 0xA00F8046: 视频层未禁止 */
#define FH_ERR_VO_VIDEO_NOT_DISABLE		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_NOT_DISABLE)
/** @brief 0xA00F8047: 视频层未配置 */
#define FH_ERR_VO_VIDEO_NOT_CONFIG		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_NOT_CONFIG)
/** @brief 0xA00F806E: 视频层已被绑定 */
#define FH_ERR_VO_VIDEO_HAS_BINDED		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_VO_VIDEO_HAS_BINDED)
/** @brief 0xA00F806F: 视频层未被绑定 */
#define FH_ERR_VO_VIDEO_NOT_BINDED		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_VO_VIDEO_NOT_BINDED)

/** @brief 0xA00F8069: WBC 未禁止 */
#define FH_ERR_VO_WBC_NOT_DISABLE		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_WBC_NOT_DISABLE)
/** @brief 0xA00F806A: WBC 属性未设置 */
#define FH_ERR_VO_WBC_NOT_CONFIG		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_WBC_NOT_CONFIG)
/** @brief 0xA00F8071: WBC 属性已设置 */
#define FH_ERR_VO_WBC_HAS_CONFIG		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_WBC_HAS_CONFIG)
/** @brief 0xA00F8072: WBC 未绑定 */
#define FH_ERR_VO_WBC_NOT_BIND			FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_WBC_NOT_BIND)
/** @brief 0xA00F8070: WBC 已绑定 */
#define FH_ERR_VO_WBC_HAS_BIND			FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_WBC_HAS_BIND)

/** @brief 0xA00F8048: 通道未禁止 */
#define FH_ERR_VO_CHN_NOT_DISABLE		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_CHN_NOT_DISABLE)
/** @brief 0xA00F8049: 通道未使能 */
#define FH_ERR_VO_CHN_NOT_ENABLE		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_CHN_NOT_ENABLE)
/** @brief 0xA00F804A: 通道未配置 */
#define FH_ERR_VO_CHN_NOT_CONFIG		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_CHN_NOT_CONFIG)
/** @brief 0xA00F804B: 通道未分配资源 */
#define FH_ERR_VO_CHN_NOT_ALLOC 		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_CHN_NOT_ALLOC)
/** @brief 0xA00F806B: VO 通道区域重叠 */
#define FH_ERR_VO_CHN_AREA_OVERLAP		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_CHN_AREA_OVERLAP)

/* cascade relatvie error code */
/** @brief 0xA00F804C: 无效样式 */
#define FH_ERR_VO_INVALID_PATTERN		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_CCD_INVALID_PAT)
/** @brief 0xA00F804D: 无效级联位置 */
#define FH_ERR_VO_INVALID_POSITION		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_CCD_INVALID_POS)

/** @brief 0xA00F804E: 等待超时 */
#define FH_ERR_VO_WAIT_TIMEOUT			FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_WAIT_TIMEOUT)
/** @brief 0xA00F804F: 无效视频帧 */
#define FH_ERR_VO_INVALID_VFRAME		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_INVALID_VFRAME)
/** @brief 0xA00F8050: 无效矩形参数 */
#define FH_ERR_VO_INVALID_RECT_PARA		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_INVALID_RECT_PARA)
/** @brief 0xA00F8051: 已设置 */
#define FH_ERR_VO_SETBEGIN_ALREADY		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_SETBEGIN_ALREADY)
/** @brief 0xA00F8052: 没有设置 */
#define FH_ERR_VO_SETBEGIN_NOTYET		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_SETBEGIN_NOTYET)
/** @brief 0xA00F8053: 已设置 */
#define FH_ERR_VO_SETEND_ALREADY		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_SETEND_ALREADY)
/** @brief 0xA00F8054: 没有设置 */
#define FH_ERR_VO_SETEND_NOTYET 		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_SETEND_NOTYET)

/* sync group relative error code */
#define FH_ERR_VO_GRP_INVALID_ID		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_INVALID_ID)
#define FH_ERR_VO_GRP_NOT_CREATE		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_NOT_CREATE)
#define FH_ERR_VO_GRP_HAS_CREATED		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_HAS_CREATED)
#define FH_ERR_VO_GRP_NOT_DESTROY		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_NOT_DESTROY)
#define FH_ERR_VO_GRP_CHN_FULL			FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_CHN_FULL)
#define FH_ERR_VO_GRP_CHN_EMPTY 		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_CHN_EMPTY)
#define FH_ERR_VO_GRP_CHN_NOT_EMPTY		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_CHN_NOT_EMPTY)
#define FH_ERR_VO_GRP_INVALID_SYN_MODE		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_INVALID_SYN_MODE)
#define FH_ERR_VO_GRP_INVALID_BASE_PTS		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_INVALID_BASE_PTS)
#define FH_ERR_VO_GRP_NOT_START 		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_NOT_START)
#define FH_ERR_VO_GRP_NOT_STOP			FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_NOT_STOP)
#define FH_ERR_VO_GRP_INVALID_FRMRATE		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_INVALID_FRMRATE)
#define FH_ERR_VO_GRP_CHN_HAS_REG		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_CHN_HAS_REG)
#define FH_ERR_VO_GRP_CHN_NOT_REG		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_CHN_NOT_REG)
#define FH_ERR_VO_GRP_CHN_NOT_UNREG		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_CHN_NOT_UNREG)
#define FH_ERR_VO_GRP_BASE_NOT_CFG		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_BASE_NOT_CFG)


/* graphics relative error code */
#define FH_ERR_VO_GFX_NOT_DISABLE		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_GFX_NOT_DISABLE)
#define FH_ERR_VO_GFX_NOT_BIND			FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_GFX_NOT_BIND)
#define FH_ERR_VO_GFX_NOT_UNBIND		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_GFX_NOT_UNBIND)
#define FH_ERR_VO_GFX_INVALID_ID		FH_DEF_ERR(FH_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_GFX_INVALID_ID)



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __FY_COMM_VO_H__ */

