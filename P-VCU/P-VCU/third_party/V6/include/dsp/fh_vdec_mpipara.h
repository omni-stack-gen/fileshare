#ifndef  __FH_VDEC_MPIPARA_H__
#define  __FH_VDEC_MPIPARA_H__
/**|VDEC|**/
#include "fh_errno.h"
#include "fh_video.h"
#include "fh_defines.h"
#include "fh_vb_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


#define FY_IO_BLOCK   FH_TRUE
#define FY_IO_NOBLOCK FH_FALSE


typedef enum
{
    VIDEO_MODE_STREAM = 0, //!< 流模式发送（不支持）
    VIDEO_MODE_FRAME = 1,  //!< 帧模式发送
    VIDEO_MODE_BUTT
}VIDEO_MODE_E;

typedef enum
{
    JPG_COLOR_FMT_YCBCR400 = 0,    //!< YUV400
    JPG_COLOR_FMT_YCBCR420 = 1,    //!< YUV420
    JPG_COLOR_FMT_YCBCR422BHP = 2, //!< YUV422 2x1（不支持）
    JPG_COLOR_FMT_YCBCR422BVP = 3, //!< YUV422 1x2（不支持）
    JPG_COLOR_FMT_YCBCR444 = 4,    //!< YUV444（不支持）
    JPG_COLOR_FMT_BUTT
} VDEC_JPG_COLOR_FMT_E;

typedef struct
{
    FH_UINT64           u64LumPixSum;       //!< 当前帧总亮度
    FH_UINT32           u32LumPixAverage;   //!< 当前帧平均亮度
    FH_UINT64           u64Pts;             //!< 当前帧显示时间戳
} VDEC_CHN_LUM_S;

typedef struct
{
    VIDEO_MODE_E         enMode;            //!< 视频流模式。取值范围：[0-1]
    VDEC_JPG_COLOR_FMT_E enJpegFormat;      //!< jpeg格式。取值范围：[0-4]
}VDEC_ATTR_JPEG_S,*PTR_VDEC_ATTR_JPEG_S;


typedef struct
{
    FH_UINT32           u32RefFrameNum;     //!< 参考帧个数。取值范围：[1-16]
    VIDEO_MODE_E        enMode;             //!< 发送码流方式。取值范围：[0-1]
    FH_BOOL             bTemporalMvpEnable; //!< 指定时间运动矢量是否用于帧间预测（不支持）
}VDEC_ATTR_VIDEO_S,*PTR_VDEC_ATTR_VIDEO_S;


typedef struct
{
    PAYLOAD_TYPE_E      enType;             //!< 解码协议类型
    FH_UINT32           u32BufSize;         //!< 码流Buffer大小
    FH_UINT32           u32Priority;        //!< 通道优先级（不支持）
    FH_UINT32           u32PicWidth;        //!< 通道宽
    FH_UINT32           u32PicHeight;       //!< 通道高
    union
    {
        VDEC_ATTR_JPEG_S  stVdecJpegAttr;   //!< jpeg或mjpeg通道的相关属性
        VDEC_ATTR_VIDEO_S stVdecVideoAttr;  //!< h264或h265通道的相关属性
    };
}VDEC_CHN_ATTR_S;

#pragma pack (4)
typedef struct
{
    FH_UINT8*           pu8Addr;            //!< 码流地址
    FH_UINT32           u32Len;             //!< 码流长度
    FH_UINT64           u64PTS;             //!< 时间戳
    FH_BOOL             bEndOfFrame;        //!< 当前帧是否结束
    FH_BOOL             bEndOfStream;       //!< 是否发完所有码流
}VDEC_STREAM_S;
#pragma pack ()

typedef struct {
    FH_UINT8*          pu8Addr;        /* userdata data vir address */
    FH_UINT32          u32PhyAddr;     /* userdata data phy address */
    FH_UINT32          u32Len;         /* userdata data len */
    FH_BOOL            bValid;         /* is valid? */
}VDEC_USERDATA_S;

typedef struct
{
    FH_SINT32           s32FormatErr;               //!< 不支持的格式
    FH_SINT32           s32PicSizeErrSet;           //!< 图像的宽（或高）比通道的宽（或高）大
    FH_SINT32           s32StreamUnsprt;            //!< 不支持的规格
    FH_SINT32           s32PackErr;                 //!< 码流错误
    FH_SINT32           s32PrtclNumErrSet;          //!< 设置的协议参数个数不够
    FH_SINT32           s32RefErrSet;               //!< 设置的参考帧个数不够
    FH_SINT32           s32PicBufSizeErrSet;        //!< 图像Buffer大小不够
    FH_SINT32           s32VdecStreamNotRelease;    //!< 码流长时间没有释放
}VDEC_DECODE_ERROR_S;


typedef struct
{
    PAYLOAD_TYPE_E      enType;                 //!< 解码协议
    FH_UINT32           u32LeftStreamBytes;     //!< 待解码码流字节数
    FH_UINT32           u32LeftStreamFrames;    //!< 待解码帧数
    FH_UINT32           u32LeftPics;            //!< 待获取的pic数
    FH_BOOL             bStartRecvStream;       //!< 是否允许接收码流
    FH_UINT32           u32RecvStreamFrames;    //!< 已接收的码流帧数
    FH_UINT32           u32DecodeStreamFrames;  //!< 已解码的码流帧数
    VDEC_DECODE_ERROR_S stVdecDecErr;           //!< 解码错误信息
}VDEC_CHN_STAT_S;

typedef struct
{
    VDEC_CHN_STAT_S     stat;                   //!< 通道状态结构体
    FH_UINT32           u32PicWidth;            //!< 码流宽度
    FH_UINT32           u32PicHeight;           //!< 码流高度
}VDEC_CHN_STAT_EX_S;

typedef struct
{
    FH_SINT32           s32DisplayFrameNum;     //!< 显示帧数
    FH_SINT32           s32ChanErrThr;          //!< 通道错误率阈值（不支持）
    FH_SINT32           s32ChanStrmOFThr;       //!< 解码前丢帧阈值（不支持）
    FH_SINT32           s32DecMode;             //!< 解码模式（无效，固定IP模式）。取值范围：[1]
    FH_SINT32           s32DecOrderOutput;      //!< 解码图像输出顺序（无效，固定解码顺序）。取值范围：[1]
    VIDEO_FORMAT_E      enVideoFormat;          //!< 解码图像数据格式（无效，固定线性）。取值范围：[0]
    COMPRESS_MODE_E     enCompressMode;         //!< 解码图像压缩模式。取值范围：[0, 0xe]
}VDEC_CHN_PARAM_S;

typedef struct
{
    VB_POOL hPicVbPool;
    VB_POOL hPmvVbPool;
}VDEC_CHN_POOL_S;

typedef struct
{
    FH_MEM_INFO         buf;
    VDEC_CHN_ATTR_S     stAttr;
    int                 reverse;
}VDEC_MEM_INFO_S;


#define VDEC_ERR_DECODE_FAILED        (EN_ERR_BUTT + 1)

/** 0xA0058002: 通道ID 超出合法范围 */
#define FH_ERR_VDEC_INVALID_CHNID     FH_DEF_ERR(FH_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/** 0xA0058003: 参数超出合法范围 */
#define FH_ERR_VDEC_ILLEGAL_PARAM     FH_DEF_ERR(FH_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/** 0xA0058004: 试图创建已经存在的通道 */
#define FH_ERR_VDEC_EXIST             FH_DEF_ERR(FH_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/** 0xA0058006: 函数参数中有空指针 */
#define FH_ERR_VDEC_NULL_PTR          FH_DEF_ERR(FH_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/** 0xA0058007: 使用前未配置 */
#define FH_ERR_VDEC_NOT_CONFIG        FH_DEF_ERR(FH_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/** 0xA0058008: 不支持的参数或者功能 */
#define FH_ERR_VDEC_NOT_SURPPORT      FH_DEF_ERR(FH_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/** 0xA0058009: 该操作不允许 */
#define FH_ERR_VDEC_NOT_PERM          FH_DEF_ERR(FH_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/** 0xA0058005: 通道未创建或已销毁 */
#define FH_ERR_VDEC_UNEXIST           FH_DEF_ERR(FH_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/** 0xA005800C: 分配内存失败，如系统内存不足 */
#define FH_ERR_VDEC_NOMEM             FH_DEF_ERR(FH_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/** 0xA005800D: 分配缓存失败，如申请的数据缓冲区太大 */
#define FH_ERR_VDEC_NOBUF             FH_DEF_ERR(FH_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/** 0xA005800E: 缓冲区中无数据 */
#define FH_ERR_VDEC_BUF_EMPTY         FH_DEF_ERR(FH_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/** 0xA005800F: 缓冲区中数据满 */
#define FH_ERR_VDEC_BUF_FULL          FH_DEF_ERR(FH_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/** 0xA0058010: 系统没有初始化或者相关依赖的模块没有加载 */
#define FH_ERR_VDEC_SYS_NOTREADY      FH_DEF_ERR(FH_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/** 0xA0058012: 系统忙 */
#define FH_ERR_VDEC_BUSY              FH_DEF_ERR(FH_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
/** 0xA0058011: 地址错误  */
#define FH_ERR_VDEC_BADADDR           FH_DEF_ERR(FH_ID_VDEC, EN_ERR_LEVEL_ERROR, EN_ERR_BADADDR)
/** 0xA0058040: 解码失败 */
#define FH_ERR_VDEC_DECODE_FAILED     FH_DEF_ERR(FH_ID_VDEC, EN_ERR_LEVEL_ERROR, VDEC_ERR_DECODE_FAILED)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef  __FH_VDEC_MPIPARA_H__ */

