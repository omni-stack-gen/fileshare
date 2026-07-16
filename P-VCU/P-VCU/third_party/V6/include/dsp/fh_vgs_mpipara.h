#ifndef __FH_VGS_MPIPARA_H__
#define __FH_VGS_MPIPARA_H__
/**|VGS|**/
#include "fh_errno.h"
#include "fh_video.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


typedef FH_SINT32  VGS_HANDLE;


typedef enum
{
    COVER_RECT = 0,        //!< 矩形覆盖
    COVER_QUAD_RANGLE = 1, //!< 任意四边形覆盖。不支持
    COVER_BUTT
} VGS_COVER_TYPE_E;

typedef enum
{
    VGS_ROTATE_NONE    = 0, //!< 不旋转
    VGS_ROTATE_90      = 1, //!< 90度旋转(rot)
    VGS_ROTATE_180     = 2, //!< 180度旋转(mir&flp)
    VGS_ROTATE_270     = 3, //!< 270度旋转(mir&flp&rot)
    VGS_ROTATE_MIRROR  = 4, //!< 水平镜像(mir)
    VGS_ROTATE_FLIP    = 5, //!< 垂直镜像(flp)
    VGS_ROTATE_MIR_ROT = 6, //!< 水平镜像旋转(mir&rot)
    VGS_ROTATE_FLP_ROT = 7, //!< 垂直镜像旋转(rot&flip)
    VGS_ROTATE_BUTT
} VGS_ROTATE_E;

typedef enum
{
    VGS_SCALE_FILTER    = 0, //!< 滤波插值模式
    VGS_SCALE_LINEAR    = 1, //!< 线性插值模式
    VGS_SCALE_MINPROX   = 2, //!< 最小临近插值模式
    VGS_SCALE_BUTT
} VGS_SCALE_MODE_E;

typedef enum
{
    VGS_PTH_MODE_AUTO = 0, //!< 自动模式
    VGS_PTH_MODE_USER = 1, //!< 用户模式
}VGS_PTH_MODE_E;

typedef enum
{
    VGS_CROP_RATIO_COOR = 0, //!< 相对坐标
    VGS_CROP_ABS_COOR   = 1, //!< 绝对坐标
}VGS_CROP_COORDINATE_E;

typedef struct
{
	FH_UINT32         u32RotBuf;       //!< 通道旋转buf大小(单位KB)
	FH_UINT32         u32VgsBuf;       //!< 工作buf大小(单位KB)
	FH_UINT32         reverse[3];      //!< 保留参数
} VGS_MODULE_PARAM_S;

typedef struct
{
    FH_UINT32 u32GotFrmNum;             //!< 接收到的帧数
    FH_UINT32 u32SkipFrmNum;            //!< 丢掉的帧数
    FH_UINT32 u32SendFrmOk;             //!< 发送成功的帧数
    FH_UINT32 u32SendFrmFail;           //!< 发送失败的帧数
}VGS_CHN_STS_S;

typedef struct
{
    VGS_PTH_MODE_E    enChnMode;        //!< 路径模式。取值范围: [0-1]
    FH_UINT32         u32Width;         //!< 路径输出宽度。取值范围: [32-4096]
    FH_UINT32         u32Height;        //!< 路径输出高度。取值范围: [32-4094]
    PIXEL_FMT_E       enPixelFormat;    //!< 路径输出的格式。取值范围: [22,23]
}VGS_PTH_PARA_S;

typedef struct
{
    FH_BOOL               bEnable;          //!<裁剪使能。取值范围: [0-1]
    VGS_CROP_COORDINATE_E enCropCoordinate; //!<坐标类型。取值范围: [0-1]
    RECT_S                stCropRect;       //!<裁剪区域
}VGS_CROP_PARA_S;

typedef struct
{
    FH_BOOL               bEnable;          //!< 有效区域使能。取值范围: [0-1]
    RECT_S                stVideoRect;      //!< 有效区域
} VGS_VALID_AREA_S;

typedef struct
{
    VIDEO_FRAME_INFO_S stImgIn;             //!<原始图像
    VIDEO_FRAME_INFO_S stImgOut;            //!<目标图像
    FH_UINT32          au32privateData[4];  //!<私有信息
    FH_UINT32          reserved;            //!<保留项
}VGS_TASK_ATTR_S;

typedef struct
{
    POINT_S   stStartPoint;                 //!<线条起点坐标
    POINT_S   stEndPoint;                   //!<线条终点坐标
    FH_UINT32 u32Thick;                     //!<线条宽度，单位像素
    FH_UINT32 u32Color;                     //!<线条颜色
}VGS_LINE_S;

typedef struct
{
    RECT_S    stRect;                       //!<角框起始位置及宽高信息
    FH_UINT32 u32HorLen;                    //!<角框水平线长
    FH_UINT32 u32VerLen;                    //!<角框垂直线长
    FH_UINT32 u32Thick;                     //!<角框线宽
    FH_UINT32 u32Color;                     //!<角框颜色
}VGS_CONER_RECT_S;

typedef struct
{
    FH_SINT32     handle;                   //!<拼接任务句柄
    VIDEO_FRAME_S stVFrameIn;               //!<拼接原始图像
    RECT_S        stRectIn;                 //!<原始图像拼接区域
    VIDEO_FRAME_S stVFrameOut;              //!<拼接目标图像
    RECT_S        stRectOut;                //!<目标图像拼接区域
}VGS_COMBINE_S;

typedef struct
{
    FH_BOOL    bSolid;                      //!<区域是否实心。不支持
    FH_UINT32  u32Thick;                    //!<虚心时的厚度。不支持
    POINT_S    stPoint[4];                  //!<四边形坐标点。不支持
} VGS_QUADRANGLE_COVER_S;

typedef struct
{
    VGS_COVER_TYPE_E           enCoverType; //!<覆盖类型。取值范围: [只支持矩形]
    union
    {
        RECT_S                 stDstRect;   //!<矩形区域
        VGS_QUADRANGLE_COVER_S stQuadRangle;//!<任意四边形区域。不支持
    };
    FH_UINT32                  u32Color;    //!<覆盖的颜色
}VGS_COVER_S;

typedef struct
{
    RECT_S         stRect;                     //!<区域信息
    FH_UINT32      u32BgColor;                 //!<背景颜色。不支持
    PIXEL_FMT_E    enPixelFmt;                 //!<OSD像素格式。取值范围: [5,8,10]
    FH_UINT32      u32PhyAddr;                 //!<OSD物理地址
    FH_UINT32      u32Stride;                  //!<OSD的步长,需4像素对齐
    FH_UINT32      u32BgAlpha;                 //!<背景透明度。取值范围: [0-255]
    FH_UINT32      u32FgAlpha;                 //!<前景透明度。取值范围: [0-255]
}VGS_OSD_S;

typedef struct
{
    FH_BOOL        bBgValid;                   //!<背景层使能。取值范围: [0,1]
    FH_UINT32      u32BgAlpha;                 //!<背景层透明度。取值范围: [0-128]
    FH_UINT32      u32BgColor;                 //!<背景层颜色,RGB888值
    FH_UINT32      u32GlbAlphaA;               //!<上层图像透明度。取值范围: [0-128]
    FH_UINT32      u32GlbAlphaB;               //!<下层图像透明度。取值范围: [0-128]
    RECT_S         stRect;                     //!<上层图像位于结果中的区域
}VGS_OPT_S;

/** @brief 0xA02D800D: 内存不足 */
#define FH_ERR_VGS_NOBUF             FH_DEF_ERR(FH_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/** @brief 0xA02D800E: 内存为空 */
#define FH_ERR_VGS_BUF_EMPTY         FH_DEF_ERR(FH_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/** @brief 0xA02D8006: 输入包含空指针 */
#define FH_ERR_VGS_NULL_PTR          FH_DEF_ERR(FH_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/** @brief 0xA02D8003: 无效参数 */
#define FH_ERR_VGS_ILLEGAL_PARAM     FH_DEF_ERR(FH_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/** @brief 0xA02D800F: 内存已满 */
#define FH_ERR_VGS_BUF_FULL          FH_DEF_ERR(FH_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/** @brief 0xA02D8010: 系统未准备好 */
#define FH_ERR_VGS_SYS_NOTREADY      FH_DEF_ERR(FH_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/** @brief 0xA02D8008: 不支持的操作 */
#define FH_ERR_VGS_NOT_SUPPORT       FH_DEF_ERR(FH_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/** @brief 0xA02D8009: 不允许的操作 */
#define FH_ERR_VGS_NOT_PERMITTED     FH_DEF_ERR(FH_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/** @brief 0xA02D8002: 通道号无效 */
#define FH_ERR_VGS_INVALID_CHNID     FH_DEF_ERR(FH_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/** @brief 0xA02D8004: 通道已存在 */
#define FH_ERR_VGS_EXIST             FH_DEF_ERR(FH_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/** @brief 0xA02D800A: 等待超时 */
#define FH_ERR_VGS_TIMEOUT           FH_DEF_ERR(FH_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_TIMEOUT)
/** @brief 0xA02D8005: VGS通道未创建 */
#define FH_ERR_VGS_UNEXIST           FH_DEF_ERR(FH_ID_VGS, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __FH_VGS_MPIPARA_H__ */
