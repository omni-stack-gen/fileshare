#ifndef __FH_COMMON_H__
#define __FH_COMMON_H__
#include "types/type_def.h"
/**|System Control|**/

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */
#pragma pack(4)

#ifdef API_PRT
#define API_DEBUG(...) do{\
	fh_printf( "%s %d: ", __func__,  __LINE__);\
	fh_printf(__VA_ARGS__);\
	}while(0)
#define API_NOTICE(...) do{\
	fh_printf( "%s %d: ", __func__,  __LINE__);\
	fh_printf(__VA_ARGS__);\
	}while(0)
#else
#define API_DEBUG(...)
#define API_NOTICE(...) do{\
	fh_printf( "%s %d: ", __func__,  __LINE__);\
	fh_printf(__VA_ARGS__);\
	}while(0)
#endif

#define RETURN_NULL                          (0)
#define RETURN_OK                            (0)
#define DEFALT_ALIGN                         (16)
#define MALLOCED_MEM_BASE_DSP                (1)
#define MALLOCED_MEM_VPUPKG                  (0)

/*芯片名称,0代表未知芯片*/
#define FH8632                               (0x863200A1)
#define FH8632v2                             (0x863200A2)
#define FH8620H                              (0x863200B1)
#define FH8856                               (0x885600A1)
#define FH8852                               (0x885600B1)
#define FH8626V100                           (0x8626A100)
#define FH8852V200                           (0x8852A200)
#define FH8856V200                           (0x8856A200)
#define FH8858V200                           (0x8858A200)
#define FH8856V201                           (0x8856A201)
#define FH8858V201                           (0x8858A201)
#define FH8852V210                           (0x8852A210)
#define FH8856V210                           (0x8856A210)
#define FH8858V210                           (0x8858A210)
#define FH8652                               (0x8652A100)
#define FH8656                               (0x8656A100)
#define FH8658                               (0x8658A100)
#define FH8852V201                           (0x8852A201)
#define FH8852V202                           (0x8852A202)
#define FH8636                               (0x8636A100)
#define FH8898                               (0x8898A100)
#define FH8896                               (0x8896A100)
#define FH8210                               (0x8210A100)
#define FH8862                               (0x8862A100)

typedef enum
{
	FH_RO_OPS_0     = 0,//!< 旋转0度
	FH_RO_OPS_90    = 1,//!< 旋转90度
	FH_RO_OPS_180   = 2,//!< 旋转180度
	FH_RO_OPS_270   = 3,//!< 旋转270度
	FH_ROTATE_OPS_DUMMY= 0xffffffff,
}FH_ROTATE_OPS;

typedef enum
{
	VPU_VOMODE_BLK          = 0,  //!<  宏块格式
	VPU_VOMODE_SCAN         = 1,  //!<  光栅格式(NV12)
	VPU_VOMODE_TILE192      = 2,  //!<  Tile压缩192Byte
	VPU_VOMODE_TILE224      = 3,  //!<  Tile压缩224Byte
	VPU_VOMODE_TILE256      = 4,  //!<  Tile压缩256Byte
	VPU_VOMODE_RGB888       = 5,  //!<  RGB888
	VPU_VOMODE_ARGB888      = 5,  //!<  ARGB888
	VPU_VOMODE_YUYV         = 6,  //!<  YUYV(YUV2)
	VPU_VOMODE_RRGGBB       = 7,  //!<  RRGGBB
	VPU_VOMODE_NV16         = 10, //!<  NV16
	VPU_VOMODE_Y            = 11, //!<  Y
	VPU_VOMODE_UYVY         = 12, //!<  UYVY
	VPU_VOMODE_YU12         = 13, //!<  YU12
	VPU_VOMODE_RGB          = 14, //!<  RGB
	VPU_VOMODE_BAYER_RAW    = 15, //!<  BAYER_RAW
	VPU_VOMODE_10BIT_NV12   = 16, //!<  yuv420 10bit
	VPU_VOMODE_10BIT_NV16   = 17, //!<  yuv422 10bit
	VPU_VOMODE_BUTT            ,
	FH_VPU_VO_MODE_DUMMY    = 0xffffffff,
}FH_VPU_VO_MODE;

typedef enum
{
	FH_OBJ_NONE    = 0, //!<  空
	FH_OBJ_VICAP   = 1, //!<  VICAP
	FH_OBJ_ISP     = 2, //!<  ISP
	FH_OBJ_VPU_VI  = 3, //!<  VPU输入
	FH_OBJ_VPU_VO  = 4, //!<  VPU通道输出
	FH_OBJ_VPU_BGM = 5, //!<  VPU-BGMY输出
	FH_OBJ_ENC     = 6, //!<  H264＆H265视频编码
	FH_OBJ_JPEG    = 7, //!<  JPEG编码
	FH_OBJ_BGM     = 8, //!<  BGM
	FH_OBJ_NN      = 9, //!<  NN
	FH_OBJ_VOU     = 10,//!<  VOU
	FH_OBJ_VPPU    = 11,//!<  VPPU
	FH_OBJ_VDEC    = 12,//!<  H264＆H265视频解码
	FH_OBJ_JPEGD   = 13,//!<  JPEG解码
	FH_OBJ_VDSP    = 14,//!<  JPEG解码
	FH_OBJ_MAX_NUM,
	FH_BIND_OBJ_ID_DUMMY=0xffffffff,
}FH_BIND_OBJ_ID;

typedef struct fhVERSION_INFO_
{
	FH_UINT32           build_date;  //!<  编译时间
	FH_UINT32           sw_version;  //!<  软件版本号
	FH_UINT32           hw_version;  //!<  硬件驱动版本号
}FH_VERSION_INFO;

typedef struct
{
	FH_UINT32           u32X;        //!<  水平坐标
	FH_UINT32           u32Y;        //!<  垂直坐标
}FH_POINT;

typedef struct
{
	FH_UINT32           u32Width;    //!<  宽度
	FH_UINT32           u32Height;   //!<  高度
} FH_SIZE;

typedef struct
{
	FH_UINT32           u32X;        //!<  起始点水平坐标
	FH_UINT32           u32Y;        //!<  起始点垂直坐标
	FH_UINT32           u32Width;    //!<  区域宽度
	FH_UINT32           u32Height;   //!<  区域高度
}FH_AREA;

typedef struct
{
	FH_ADDR             addr;        //!<  虚拟地址
	FH_UINT32           size;        //!<  内存大小
}FH_ADDR_INFO;

typedef struct
{
	FH_PHYADDR          base;        //!<  物理地址
	FH_VOID *           vbase;       //!<  虚拟地址
	FH_UINT32           size;        //!<  内存大小
}FH_MEM_INFO;

typedef struct
{
	FH_UINT16           frame_count; //!<  帧数
	FH_UINT16           frame_time;  //!<  统计时间(s)
} FH_FRAMERATE;

typedef struct
{
	FH_UINT32           y;           //!<  亮度
	FH_UINT32           cb;          //!<  蓝色浓度偏移量
	FH_UINT32           cr;          //!<  红色浓度偏移量
	FH_UINT32           alpha;       //!<  透明度
} FH_YC_COLOR;

typedef struct
{
	FH_BIND_OBJ_ID      obj_id;         //!<  模块号
	FH_UINT32           dev_id;         //!<  设备号
	FH_UINT32           chn_id;         //!<  通道号
}FH_BIND_INFO;

typedef struct
{
	FH_UINT32 avgTime;
	FH_UINT32 maxTime;
	FH_UINT32 avgCycle;
	FH_UINT32 maxCycle;
	FH_UINT32 win_size;
	FH_UINT64 time_stamp;
}FH_HW_AVG_TIME;


typedef struct fyPOINT_S
{
    FH_SINT32 s32X;
    FH_SINT32 s32Y;
}POINT_S;



typedef struct fyRECT_S
{
    FH_SINT32 s32X;
    FH_SINT32 s32Y;
    FH_UINT32 u32Width;
    FH_UINT32 u32Height;
}RECT_S;

typedef struct fyCROP_INFO_S
{
	FH_BOOL bEnable;
	RECT_S  stRect;
}CROP_INFO_S;

typedef enum fyROTATE_E
{
    ROTATE_NONE   = 0,               //!< 无旋转
    ROTATE_90     = 1,               //!< 顺时针90°旋转
    ROTATE_180    = 2,               //!< 顺时针180°旋转
    ROTATE_270    = 3,               //!< 顺时针270°旋转
    ROTATE_MIRROR = 4,               //!< 水平镜像
    ROTATE_FLIP   = 5,               //!< 垂直翻转
    ROTATE_BUTT
} ROTATE_E;

typedef struct
{
	FH_MEM_INFO data;                     //!< 数据地址
	FH_UINT32   stride;                   //!< 数据stride
} FH_FRM_BUF;

typedef struct
{
	FH_FRM_BUF luma;                      //!< 亮度数据地址
	FH_FRM_BUF chroma;                    //!< 色度数据地址
} FH_FRM_YUV_2P;

typedef struct
{
	FH_FRM_BUF r_data;                    //!< R数据地址
	FH_FRM_BUF g_data;                    //!< G数据地址
	FH_FRM_BUF b_data;                    //!< B数据地址
} FH_FRM_RRGGBB;

typedef struct
{
	FH_FRM_BUF st_data;                    //!< 短帧数据
	FH_FRM_BUF lf_data;                    //!< 长帧数据
	FH_UINT32  sf_crc;                     //!< 短帧校验值　
	FH_UINT32  lf_crc;                     //!< 长帧校验值　
} FH_FRM_RAW;

typedef struct
{
	FH_VPU_VO_MODE         data_format;  //!< 图像格式
	FH_SIZE                size;         //!< 图像尺寸
	FH_UINT64              time_stamp;   //!< 时间戳
	FH_UINT64              frame_id;     //!< 帧号
	FH_UINT32              pool_id;      //!< vb pool id(非VB模式建议配置为-1UL)
	FH_BIND_INFO           frame_src;    //!< 数据来源
	FH_UINT64              private_data[4];   //!< 私有数据
	FH_SIZE                size_usr;     //!< 用户配置尺寸(未做对齐)
	union {
		FH_FRM_YUV_2P      frm_blk;      //!< block格式输出信息
		FH_FRM_YUV_2P      frm_scan;     //!< scan(nv12)格式输出信息
		FH_FRM_BUF         frm_tile192;  //!< tile192格式输出信息
		FH_FRM_BUF         frm_tile224;  //!< tile224格式输出信息
		FH_FRM_BUF         frm_tile256;  //!< tile256格式输出信息
		FH_FRM_BUF         frm_argb8888; //!< argb8888格式输出信息
		FH_FRM_BUF         frm_yuyv;     //!< yuyv格式输出信息
		FH_FRM_RRGGBB      frm_rrggbb;   //!< rrggbb格式输出信息
		FH_FRM_YUV_2P      frm_nv16;     //!< nv16格式输出信息
		FH_FRM_BUF         frm_y;        //!< y格式输出信息
		FH_FRM_BUF         frm_uyvy;     //!< uyvy格式输出信息
		FH_FRM_YUV_2P      frm_yu12;     //!< yu12格式输出信息
		FH_FRM_BUF         frm_rgb;      //!< rgb格式输出信息
		FH_FRM_RAW         frm_raw;      //!< raw格式输出信息
		FH_FRM_YUV_2P      frm_nv12p10;  //!< nv12 10bit格式输出信息
		FH_FRM_YUV_2P      frm_nv16p10;  //!< nv16 10bit格式输出信息
	};
}FH_VIDEO_FRAME;

typedef FH_SINT32 JPEGE_CHN;
typedef FH_SINT32 VDEC_CHN;
typedef FH_SINT32 VO_DEV;
typedef FH_SINT32 VO_LAYER;
typedef FH_SINT32 VO_CHN;
typedef FH_SINT32 VO_WBC;
typedef FH_SINT32 GRAPHIC_LAYER;
typedef FH_SINT32 VO_GRP;
typedef FH_SINT32 VGS_CHN;
typedef FH_SINT32 VGS_PTH;


#define FY_INVALID_CHN (-1)
#define FY_INVALID_WAY (-1)
#define FY_INVALID_LAYER (-1)
#define FY_INVALID_DEV (-1)
#define FY_INVALID_HANDLE (-1)
#define FY_INVALID_VALUE (-1)
#define FY_INVALID_TYPE (-1)

typedef enum fyMOD_ID_E
{
    FH_ID_CMPI    = 0,
    FH_ID_VB      = 1,
    FH_ID_SYS     = 2,
    FH_ID_RGN     = 3,
    FH_ID_LCM     = 4,
    FH_ID_VDEC    = 5,
    FH_ID_GROUP   = 6,
    FH_ID_VPSS    = 7,
    FH_ID_VENC    = 8,
    FH_ID_VDA     = 9,
    FH_ID_H264E   = 10,
    FH_ID_JPEGED  = 11,
    FH_ID_MPEG4E  = 12,
    FH_ID_H264D   = 13,
    FH_ID_JPEGD   = 14,
    FH_ID_VOU     = 15,
    FH_ID_VIU     = 16,
    FH_ID_DSU     = 17,
    FH_ID_VALG    = 18,
    FH_ID_RC      = 19,
    FH_ID_AIO     = 20,
    FH_ID_AI      = 21,
    FH_ID_AO      = 22,
    FH_ID_AENC    = 23,
    FH_ID_ADEC    = 24,
    FH_ID_AVENC   = 25,
    FH_ID_PCIV    = 26,
    FH_ID_PCIVFMW = 27,
    FH_ID_ISP     = 28,
    FH_ID_IVE     = 29,
    FH_ID_MMZ     = 30,

    FH_ID_DCCM    = 31,
    FH_ID_DCCS    = 32,
    FH_ID_PROC    = 33,
    FH_ID_LOG     = 34,
    FH_ID_MST_LOG = 35,
    FH_ID_VD      = 36,

    FH_ID_VCMP    = 38,
    FH_ID_FB      = 39,
    FH_ID_HDMI    = 40,
    FH_ID_VOIE    = 41,
    FH_ID_TDE     = 42,
    FH_ID_USR     = 43,
    FH_ID_VEDU    = 44,
    FH_ID_VGS     = 45,
    FH_ID_H265E   = 46,
    FH_ID_FD      = 47,
    FH_ID_ODT     = 48,
    FH_ID_VQA     = 49,
    FH_ID_LPR     = 50,
    FH_ID_FISHEYE = 51,
    FH_ID_VPPU    = 45,
    /* Extension */
    FH_ID_BGM     = 60,
    FH_ID_BGMX    = 61,
    FH_ID_BGMSW   = 62,
    FH_ID_VGS2    = 63,
    FH_ID_DMIC0   = 64,
    FH_ID_DMIC1   = 65,
    FH_ID_VICAP   = 66,
    FH_ID_CAMIF   = 67,
    FH_ID_VDSP    = 68,
    FH_ID_PRE_ROLLING    = 69,
    FH_ID_I2S0	= 70,
    FH_ID_I2S1  = 71,
    FH_ID_SRC  = 72,
    FH_ID_APP     = 73,
    FH_ID_AES     = 74,
    FH_ID_NNP     = 75,
    FH_ID_GYRO     = 76,

    FH_ID_BUTT,
    /* Extension only for log */
    FH_ID_VIU_LLD,
    FH_ID_VOU_LLD,
    FH_ID_G2D_LLD,
    FH_ID_VPPU_LLD,
    FH_ID_VPU_LLD,
    FH_ID_JPEG_LLD,
    FH_ID_LOG_BUTT,
} MOD_ID_E;


typedef enum
{
    PT_PCMU          = 0,
    PT_1016          = 1,
    PT_G721          = 2,
    PT_GSM           = 3,
    PT_G723          = 4,
    PT_DVI4_8K       = 5,
    PT_DVI4_16K      = 6,
    PT_LPC           = 7,
    PT_PCMA          = 8,
    PT_G722          = 9,
    PT_S16BE_STEREO  = 10,
    PT_S16BE_MONO    = 11,
    PT_QCELP         = 12,
    PT_CN            = 13,
    PT_MPEGAUDIO     = 14,
    PT_G728          = 15,
    PT_DVI4_3        = 16,
    PT_DVI4_4        = 17,
    PT_G729          = 18,
    PT_G711A         = 19,
    PT_G711U         = 20,
    PT_G726          = 21,
    PT_G726_32K      = 22,
    PT_G729A         = 23,
    PT_LPCM          = 24,
    PT_CelB          = 25,
    PT_JPEG          = 26,
    PT_CUSM          = 27,
    PT_NV            = 28,
    PT_PICW          = 29,
    PT_CPV           = 30,
    PT_H261          = 31,
    PT_MPEGVIDEO     = 32,
    PT_MPEG2TS       = 33,
    PT_H263          = 34,
    PT_SPEG          = 35,
    PT_MPEG2VIDEO    = 36,
    PT_AAC           = 37,
    PT_WMA9STD       = 38,
    PT_HEAAC         = 39,
    PT_PCM_VOICE     = 40,
    PT_PCM_AUDIO     = 41,
    PT_AACLC         = 42,
    PT_MP3           = 43,
    PT_ADPCMA        = 49,
    PT_AEC           = 50,
    PT_X_LD          = 95,
    PT_H264          = 96,
    PT_D_GSM_HR      = 200,
    PT_D_GSM_EFR     = 201,
    PT_D_L8          = 202,
    PT_D_RED         = 203,
    PT_D_VDVI        = 204,
    PT_D_BT656       = 220,
    PT_D_H263_1998   = 221,
    PT_D_MP1S        = 222,
    PT_D_MP2P        = 223,
    PT_D_BMPEG       = 224,
    PT_MP4VIDEO      = 230,
    PT_MP4AUDIO      = 237,
    PT_VC1           = 238,
    PT_JVC_ASF       = 255,
    PT_D_AVI         = 256,
    PT_DIVX3		 = 257,
    PT_AVS			 = 258,
    PT_REAL8		 = 259,
    PT_REAL9		 = 260,
    PT_VP6			 = 261,
    PT_VP6F			 = 262,
    PT_VP6A			 = 263,
    PT_SORENSON	 	 = 264,
    PT_H265          = 265,
    PT_VP8           = 266,
    PT_MVC           = 267,
    PT_MAX           = 268,

    PT_AMR           = 1001,
    PT_MJPEG         = 1002,
    PT_AMRWB         = 1003,
    PT_BUTT
}PAYLOAD_TYPE_E;

#pragma pack()
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* _FH_COMMON_H_ */
