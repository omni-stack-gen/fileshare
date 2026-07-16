#ifndef __FH_JPEGE_MPIPARA_H__
#define __FH_JPEGE_MPIPARA_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include "fh_errno.h"
#include "fh_video.h"


/** 0xA00B8002: 通道ID 超出合法范围 */
#define FH_ERR_JPEGE_INVALID_CHNID              FH_DEF_ERR(FH_ID_JPEGED, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/** 0xA00BB003: 参数超出合法范围 */
#define FH_ERR_JPEGE_ILLEGAL_PARAM              FH_DEF_ERR(FH_ID_JPEGED, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/** 0xA00B8006: 函数参数中有空指针 */
#define FH_ERR_JPEGE_NULL_PTR                   FH_DEF_ERR(FH_ID_JPEGED, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/** 0xA00B8007: 使用前系统未配置 */
#define FH_ERR_JPEGE_NOT_CONFIG                 FH_DEF_ERR(FH_ID_JPEGED, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/** 0xA00B8008: 不支持的参数或者功能 */
#define FH_ERR_JPEGE_NOT_SUPPORT                FH_DEF_ERR(FH_ID_JPEGED, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/** 0xA00B8009: 该操作不允许 */
#define FH_ERR_JPEGE_NOT_PERM                   FH_DEF_ERR(FH_ID_JPEGED, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/** 0xA00B800C: 分配内存失败，如系统或MMZ内存不足 */
#define FH_ERR_JPEGE_NOMEM                      FH_DEF_ERR(FH_ID_JPEGED, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/** 0xA00B800E: 编码缓冲区中无数据 */
#define FH_ERR_JPEGE_BUF_EMPTY                  FH_DEF_ERR(FH_ID_JPEGED, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/** 0xA00B800F: 待编码缓冲区中数据满 */
#define FH_ERR_JPEGE_BUF_FULL                   FH_DEF_ERR(FH_ID_JPEGED, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/** 0xA00B8010: 系统没有初始化或者相关依赖的模块没有加载 */
#define FH_ERR_JPEGE_SYS_NOTREADY               FH_DEF_ERR(FH_ID_JPEGED, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/** 0xA00B8004: 试图创建已经存在的通道 */
#define FH_ERR_JPEGE_EXIST                      FH_DEF_ERR(FH_ID_JPEGED, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/** 0xA00B8005: 通道未创建或已销毁 */
#define FH_ERR_JPEGE_UNEXIST                    FH_DEF_ERR(FH_ID_JPEGED, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/** 0xA00B8012: JPEG编码器忙 */
#define FH_ERR_JPEGE_BUSY                       FH_DEF_ERR(FH_ID_JPEGED, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)


#pragma pack (4)
typedef struct
{
    FH_UINT32  u32MaxPicWidth;              //!< 最大编码图像宽度
    FH_UINT32  u32MaxPicHeight;             //!< 最大编码图像高度
    FH_UINT32  u32BufSize;                  //!< 码流buffer大小
    FH_UINT32  u32PicWidth;                 //!< 编码图像宽度
    FH_UINT32  u32PicHeight;                //!< 编码图像高度
}JPEGE_CHN_ATTR_S;

typedef struct
{
    FH_SINT32 s32RecvPicNum;                //!< 编码通道连续接收并编码的帧数
}JPEGE_RECV_PIC_PARAM_S;

typedef struct
{
    FH_UINT32   u32PhyAddr;                 //!< 码流物理地址
    FH_UINT8   *pu8Addr;                    //!< 码流虚拟地址
    FH_UINT32   u32Len;                     //!< 码流长度
    FH_UINT64   u64PTS;                     //!< 时间戳
    FH_UINT32   u32Qfactor;                 //!< 图像质量
    FH_SINT32   s32MilliSec;                //!< 延时时间
} JPEGE_STREAM_S;

typedef struct
{
    FH_UINT32 u32LeftPics;                  //!< 待编码的帧个数
    FH_UINT32 u32LeftStreamBytes;           //!< 剩余码流字节数
    FH_UINT32 u32LeftStreamFrames;          //!< 剩余码流帧数
    FH_UINT32 u32CurPacks;                  //!< 当前帧的码流包个数
    FH_UINT32 u32LeftRecvPics;              //!< 接收到的帧数（不支持）
    FH_UINT32 u32LeftEncPics;               //!< 带编码的帧数（不支持）
}JPEGE_CHN_STAT_S;

typedef struct
{
    FH_UINT32 u32Qfactor;                   //!< 图像质量，体含义请参见RFC2435协议。取值范围：[1-99],50-设置时使用u8YQt/u8CbQt中的值作为通道的qp值
    FH_UINT8  u8YQt[64];                    //!< Y量化表。取值范围：[1-255]
    FH_UINT8  u8CbQt[64];                   //!< Cb量化表。取值范围：[1-255]
    FH_UINT8  u8CrQt[64];                   //!< Cr量化表。取值范围：[1-255]
    FH_UINT32 u32MCUPerECS;                 //!< 单个ECS的MCU数（不支持）
} JPEGE_PARAM_S;

typedef struct
{
    FH_BOOL bEnable;                        //!< 裁剪使能标志。取值范围：[0-1]
    RECT_S  stRect;                         //!< 裁剪区域，各参数必须是16对齐
}JPEGE_CROP_CFG_S;

typedef struct
{
    FH_BOOL bEnable;                        //!< 选装使能标志。取值范围：[0-1]
    ROTATE_E enRotate;                      //!< 选装角度。取值范围：[0-3]
}JPEGE_ROTATE_CFG_S;

typedef struct
{
	FH_UINT32 Luma_Addr;//!< y data, phyaddr
	FH_UINT32 Chroma_Addr;//!< uv data, phyaddr
	FH_UINT32 JPG_Addr;//!< jpg data, phyaddr
	FH_UINT32 JPG_BufSize;

	FH_UINT16 Img_W;
	FH_UINT16 Img_H;
	FH_UINT16 ROI_X;
	FH_UINT16 ROI_Y;
	FH_UINT16 ROI_W;
	FH_UINT16 ROI_H;
	FH_UINT16 Stride;//!< Input Image Address Stride.
	FH_UINT8 Format;
	FH_UINT8 EncEn;
	FH_UINT8 DcmpEn;//!< TT_ecdc enable, encoder only.
	/*ByteMode: TT_ecdc compression ratio of a tile64x4, encoder only.
	00: 192byte 50%; 01: 224byte 60%; 02: 256byte 66%.
	In TT_ecdc disable, must be 00.*/
	FH_UINT8 EcsEn;
	FH_UINT16 NRST;//!< NRST = NMCU between two reset markers - 1
	FH_UINT32 Qfactor;//!< 1--99
	FH_UINT8 ByteMode;
	/*ScanMode: 00: Block store; 10: CbCr; 11: CrCb.
	In decoder & TT_ecdc & block store, must be 00.*/
	FH_UINT8 ScanMode;
	/*RotateCfg: 00: 0; 01: 90; 10: 180; 11: 270.
	In decoder & TT_ecdc & block store, must be 00.*/
	FH_UINT8 RotateCfg;
	FH_UINT8 LumaQTbl[8][8];
	FH_UINT8 ChromaQTbl[8][8];
}JPEGE_PRG_CFG_S;
#pragma pack ()

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __FH_JPEGE_MPIPARA_H__ */


