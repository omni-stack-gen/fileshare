#ifndef __FH_COMM_VIDEO_H__
#define __FH_COMM_VIDEO_H__

#include "fh_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


typedef enum
{
    PIC_QCIF = 0,
    PIC_CIF  = 1,
    PIC_2CIF = 2,
    PIC_HD1 = 3,
    PIC_D1 = 4,
    PIC_960H = 5,
    PIC_1280H = 6,
    PIC_1440H = 7,
    PIC_QVGA = 8,     //!<  320 * 240
    PIC_VGA = 9,      //!<  640 * 480
    PIC_XGA = 10,     //!<  1024 * 768
    PIC_SXGA = 11,    //!<  1400 * 1050
    PIC_UXGA = 12,    //!<  1600 * 1200
    PIC_QXGA = 13,    //!<  2048 * 1536
    PIC_WVGA = 14,    //!<  854 * 480
    PIC_WSXGA = 15,   //!<  1680 * 1050
    PIC_WUXGA = 16,   //!<  1920 * 1200
    PIC_WQXGA = 17,   //!<  2560 * 1600
    PIC_HD720 = 18,   //!<  1280 * 720
    PIC_HD1080 = 19,  //!<  1920 * 1080
    PIC_qHD = 20,     //!<  960 * 540
    PIC_UHD2K = 21,   //!<  2560 * 1440
    PIC_UHD4K = 22,   //!<  3840*2160
    PIC_1080LITE = 23,//!<  960*1080
    PIC_BUTT
}PIC_SIZE_E;

typedef enum
{
    VIDEO_ENCODING_MODE_PAL = 0,
    VIDEO_ENCODING_MODE_NTSC = 1,
    VIDEO_ENCODING_MODE_AUTO = 2,
    VIDEO_ENCODING_MODE_BUTT
} VIDEO_NORM_E;

typedef enum
{
    PIXEL_FMT_RGB_1BPP = 0,
    PIXEL_FMT_RGB_2BPP = 1,
    PIXEL_FMT_RGB_4BPP = 2,
    PIXEL_FMT_RGB_8BPP = 3,
    PIXEL_FMT_RGB_444 = 4,
    PIXEL_FMT_RGB_4444 = 5,            //!< ARGB4444
    PIXEL_FMT_RGB_555 = 6,
    PIXEL_FMT_RGB_565 = 7,
    PIXEL_FMT_RGB_1555 = 8,            //!< ARGB1555
    PIXEL_FMT_RGB_888 = 9,
    PIXEL_FMT_RGB_8888 = 10,           //!< ARGB8888
    PIXEL_FMT_RGB_PLANAR_888 = 11,
    PIXEL_FMT_RGB_BAYER_8BPP = 12,
    PIXEL_FMT_RGB_BAYER_10BPP = 13,
    PIXEL_FMT_RGB_BAYER_12BPP = 14,
    PIXEL_FMT_RGB_BAYER_14BPP =15,
    PIXEL_FMT_RGB_BAYER = 16,          //!<  16bpp
    PIXEL_FMT_YUV_A422 = 17,
    PIXEL_FMT_YUV_A444 = 18,
    PIXEL_FMT_YUV_PLANAR_422 = 19,
    PIXEL_FMT_YUV_PLANAR_420 = 20,
    PIXEL_FMT_YUV_PLANAR_444 = 21,
    PIXEL_FMT_YUV_SEMIPLANAR_422 = 22, //!<  NV16
    PIXEL_FMT_YUV_SEMIPLANAR_420 = 23, //!<  NV12
    PIXEL_FMT_YUV_SEMIPLANAR_444 = 24,
    PIXEL_FMT_UYVY_PACKAGE_422 = 25,
    PIXEL_FMT_YUYV_PACKAGE_422 = 26,
    PIXEL_FMT_VYUY_PACKAGE_422 = 27,
    PIXEL_FMT_YCbCr_PLANAR = 28,
    PIXEL_FMT_YUV_400 = 29,
    PIXEL_FMT_YUV_422_10BPP = 43,
    PIXEL_FMT_YUV_420_BLOCK = 44,      //!<  YUV420宏块
    PIXEL_FMT_YUV_422_MIRROR = 45,     //!<  只支持1D压缩
    PIXEL_FMT_YUV_FIELD_422 = 46,      //!<  cvbs像素格式
    PIXEL_FMT_YVU_SEMIPLANAR_420 = 47, //!<  NV21
    PIXEL_FMT_YVYU_PACKAGE_422 = 48,
    PIXEL_FMT_YUV_FIELD_420 = 49,
    PIXEL_FMT_YUV_FIELD_1D_422 = 50,
    PIXEL_FMT_ABGR_8888 = 51,
    PIXEL_FMT_RGBA_8888 = 52,
    PIXEL_FMT_BGRA_8888 = 53,
    PIXEL_FMT_BUTT
} PIXEL_FMT_E;

typedef enum
{
    VIDEO_FIELD_TOP         = 0x1,    //!< 偶数场
    VIDEO_FIELD_BOTTOM      = 0x2,    //!< 奇数场
    VIDEO_FIELD_INTERLACED  = 0x3,    //!< 交错场
    VIDEO_FIELD_FRAME       = 0x4,    //!< 帧
    VIDEO_FIELD_BUTT
} VIDEO_FIELD_E;

typedef enum
{
    VIDEO_FORMAT_LINEAR     = 0x0,      //!< 线性格式
    VIDEO_FORMAT_TILE       = 0x1,      //!< 默认的tile模式，tile单元：256pixel x 16line
    VIDEO_FORMAT_TILE64     = 0x2,      //!< tile单元：64pixel x 16line
    VIDEO_FORMAT_BLOCK      = 0x3,      //!< 宏块格式
    VIDEO_FORMAT_BUTT
} VIDEO_FORMAT_E;

typedef enum
{
    COMPRESS_MODE_NONE      = 0x0,      //!< 非压缩
    COMPRESS_MODE_SEG       = 0x1,      //!< seg256压缩（不支持）
    COMPRESS_MODE_SEG128    = 0x2,      //!< seg128压缩（不支持）
    COMPRESS_MODE_LINE      = 0x3,      //!< 整行压缩（不支持）
    COMPRESS_MODE_FRAME     = 0x4,      //!< 整帧压缩（不支持）
    COMPRESS_MODE_128byte   = 0x5,      //!< 1D压缩, 压缩率50%
    COMPRESS_MODE_160byte   = 0x6,      //!< 1D压缩, 压缩率63%
    COMPRESS_MODE_192byte   = 0x7,      //!< 1D压缩, 压缩率75%
    COMPRESS_MODE_TILE_192  = 0x8,      //!< tile压缩, 压缩率50%
    COMPRESS_MODE_SLICE     = 0x9,      //!< slice压缩
    COMPRESS_MODE_CF10      = 0xa,      //!< cf10压缩
    COMPRESS_MODE_FBC       = 0xb,      //!< fbc压缩
    COMPRESS_MODE_TILE_224  = 0xc,      //!< tile压缩, 压缩率59%
    COMPRESS_MODE_TILE_256  = 0xd,      //!< tile压缩, 压缩率67%
    COMPRESS_MODE_MFBC      = 0xe,      //!< mfbc压缩
    COMPRESS_MODE_BUTT
} COMPRESS_MODE_E;

typedef enum
{
    VIDEO_DISPLAY_MODE_PREVIEW      = 0x0,  //!< 预览模式
    VIDEO_DISPLAY_MODE_PLAYBACK     = 0x1,  //!< 回放模式
    VIDEO_DISPLAY_MODE_BUTT
} VIDEO_DISPLAY_MODE_E;

#pragma pack (4)
typedef struct
{
    FH_UINT32          u32Width;            //!< 视频帧宽
    FH_UINT32          u32Height;           //!< 视频帧高
    VIDEO_FIELD_E      u32Field;            //!< 视频帧场模式
    PIXEL_FMT_E        enPixelFormat;       //!< 视频帧像素格式
    VIDEO_FORMAT_E     enVideoFormat;       //!< 视频格式
    COMPRESS_MODE_E    enCompressMode;      //!< 压缩模式
    FH_UINT32          u32PhyAddr[3];       //!< 物理地址
    FH_VOID           *pVirAddr[3];         //!< 虚拟地址
    FH_UINT32          u32Stride[3];        //!< 水平步长
    FH_UINT32          u32HStride[3];       //!< 垂直步长(u32HStride[2]记帧号信息)
    FH_UINT32          u32HeaderPhyAddr[3]; //!< 头信息物理地址
    FH_VOID           *pHeaderVirAddr[3];   //!< 头信息虚拟地址
    FH_UINT32          u32HeaderStride[3];  //!< 头信息步长
    FH_SINT16          s16OffsetTop;        //!< 上边界偏移
    FH_SINT16          s16OffsetBottom;     //!< 下边界偏移
    FH_SINT16          s16OffsetLeft;       //!< 左边界偏移
    FH_SINT16          s16OffsetRight;      //!< 右边界偏移
    FH_UINT64          u64pts;              //!< 时间戳
    FH_UINT32          u32TimeRef;          //!< 参考时间
    FH_UINT32          u32PrivateData;      //!< 私有数据
}VIDEO_FRAME_S;
#pragma pack ()

typedef struct
{
    VIDEO_FRAME_S stVFrame;               //!< 视频帧结构
    FH_UINT32 u32PoolId;                  //!< buffer池号
} VIDEO_FRAME_INFO_S;



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __FH_COMM_VIDEO_H__ */

