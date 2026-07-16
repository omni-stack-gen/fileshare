#ifndef __FH_COMM_REGION_H__
#define __FH_COMM_REGION_H__

#include "types/type_def.h"
#include "fh_common.h"
#include "fh_video.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

/**********************************************************************
    PART I      - Macro
**********************************************************************/
#define RGN_MAX_BMP_UPDATE_NUM 16
#define OVERLAY_MAX_NUM_VPSS   6
#define VPSS_MAX_MASK_AREA     8

/**********************************************************************
    PART II     - Structure and Enumeration
**********************************************************************/

typedef FH_UINT32 RGN_HANDLE;

/**
 * @brief 定义区域类型。
 *
 */
typedef enum
{
    /*! 视频叠加区域。 */
    OVERLAY_RGN = 0,
    /*! 视频遮挡区域。 */
    COVER_RGN,
    /*! 扩展视频遮挡区域，不支持。 */
    COVEREX_RGN,
    /*! 扩展视频叠加区域,不支持。 */
    OVERLAYEX_RGN,
    /*! 扩展线条视频叠加区域不支持。 */
    LINE_RGN,
    /*! 马赛克区域。 */
    MOSAIC_RGN,
    RGN_BUTT
} RGN_TYPE_E;

/*Not used*/
typedef enum
{
    LESSTHAN_LUM_THRESH = 0,   /* the lum of the video is less than the lum threshold which is set by u32LumThresh  */
    MORETHAN_LUM_THRESH,       /* the lum of the video is more than the lum threshold which is set by u32LumThresh  */
    INVERT_COLOR_BUTT
}INVERT_COLOR_MODE_E;

/*Not used*/
typedef enum
{
    AREA_RECT = 0,
    AREA_QUAD_RANGLE,
    AREA_BUTT
} RGN_AREA_TYPE_E;

typedef enum
{
    MOSAIC_BLK_SIZE_8 = 0,    /*Not support,block size 8*8 of MOSAIC*/
    MOSAIC_BLK_SIZE_16,       /*block size 16*16 of MOSAIC*/
    MOSAIC_BLK_SIZE_32,       /*block size 32*32 of MOSAIC*/
    MOSAIC_BLK_SIZE_64,       /*Not support,block size 64*64 of MOSAIC*/
    MOSAIC_BLK_SIZE_BUTT
} MOSAIC_BLK_SIZE_E;

/*Not used*/
typedef enum
{
    RGN_ABS_COOR = 0,   /*Absolute coordinate*/
    RGN_RATIO_COOR      /*Ratio coordinate*/
}RGN_COORDINATE_E;

/*structure type define*/

/*Not used*/
typedef struct
{
    FH_BOOL	 bQpDisable;
    FH_BOOL  bAbsQp;
    FH_SINT32   s32Qp;
}OVERLAY_QP_INFO_S;

/*Not used*/
typedef struct
{
    FH_SIZE                 stInvColArea;   /*it must be multipe of 16 but not more than 64.*/
    FH_UINT32               u32LumThresh;   /*the threshold to decide whether invert the OSD's color or not.*/
    INVERT_COLOR_MODE_E     enChgMod;
    FH_BOOL                 bInvColEn;      /*the switch of inverting color.*/
}OVERLAY_INVERT_COLOR_S;

/**
 * @brief 定义叠加区域属性结构体。
 *
 * @note
 *   - enPixelFmt、stSize 只在调用 #FH_RGN_AttachToChn 后， #FH_RGN_DetachFromChn 之前为静态变量。
 *   - OVERLAY 的宽高与区域分配的内存大小相关，建议按实际的需要设置区域的宽高，以免造成内存浪费。
 */
typedef struct
{
    /*! 像素格式。 */
    PIXEL_FMT_E     enPixelFmt;
    /*! 区域背景色。 */
    FH_UINT32       u32BgColor;
    /*! 区域的高宽,取值范围：所有同一通道的叠加区域宽总长，高总长，不得超过vpu通道得宽高。宽的对齐要求为4对齐。 */
    FH_SIZE         stSize;
    /*! 画布个数 */
    FH_UINT32       u32CanvasNum;
    /*! 内存是否成cached */
    FH_BOOL         bCached;
}OVERLAY_ATTR_S;

/**
 * @brief 定义叠加区域的通道显示属性。
 *
 * @note
 *   - enPixelFmt、stSize 只在调用 #FH_RGN_AttachToChn 后， #FH_RGN_DetachFromChn 之前为静态变量。
 *   - OVERLAY 的宽高与区域分配的内存大小相关，建议按实际的需要设置区域的宽高，以免造成内存浪费。
 */
typedef struct
{
    /*! 区域位置。动态属性。 */
    POINT_S     stPoint;
    /*! Alpha 位为 1 的像素点的透明度。也称前景 Alpha。取值范围：[0, 255]。取值越小，越透明。动态属性。 */
    FH_UINT32   u32FgAlpha;
    /*! Alpha 位为 0 的像素点的透明度。也称背景 Alpha。不支持。 */
    FH_UINT32   u32BgAlpha;
    /*! 区域层次。不支持。 */
    FH_UINT32   u32Layer;
    /*! 此区域编码时使用的 QP 值。不支持。 */
    OVERLAY_QP_INFO_S stQpInfo;
    /*! 区域反色功能由用户实现。不支持。 */
    OVERLAY_INVERT_COLOR_S stInvertColor;
}OVERLAY_CHN_ATTR_S;

/*Not used*/
typedef struct
{
    FH_BOOL     bSolid;             /* whether solid or dashed framework */
    FH_UINT32   u32LineWidth;       /* Line Width of framework, valid when dashed framework */
    POINT_S     stPoint[4];         /* points of quadrilateral*/
} RGN_QUADRANGLE_S;

typedef struct
{
    RGN_AREA_TYPE_E     enCoverType;        /* rect or arbitary quadrilateral COVER */
    union
    {
        RECT_S              stRect;        	/* config of rect*/
        RGN_QUADRANGLE_S    stQuadRangle;  	/* Not support,config of arbitary quadrilateral COVER */
    };
    FH_UINT32 u32Color;
    FH_UINT32 u32Layer;   					/* Not support,COVER region layer range:[0,3] */
    RGN_COORDINATE_E enCoordinate;          /*Not support,ratio coordiante or abs coordinate*/
}COVER_CHN_ATTR_S;

typedef struct
{
    RGN_AREA_TYPE_E     enCoverType;       /* rect or arbitary quadrilateral COVER */
    union
    {
        RECT_S              stRect;        /* config of rect */
        RGN_QUADRANGLE_S    stQuadRangle;  /* Not support,config of arbitary quadrilateral COVER */
    };
    FH_UINT32 u32Color;
    FH_UINT32 u32Layer;   /* Not support,COVEREX region layer range:[0,7] */
}COVEREX_CHN_ATTR_S;

typedef struct
{
    RECT_S stRect;                 /*location of MOSAIC*/
    MOSAIC_BLK_SIZE_E enBlkSize;   /*block size of MOSAIC*/
    FH_UINT32 u32Layer;            /*Not support,MOSAIC region layer range:[0,3] */
}MOSAIC_CHN_ATTR_S;

typedef struct
{
    PIXEL_FMT_E enPixelFmt;
    FH_UINT32 u32BgColor;       /* background color, pixel format depends on "enPixelFmt" */
    FH_SIZE stSize;             /* region size */
    FH_UINT32 u32CanvasNum;     /* Not support*/
}OVERLAYEX_ATTR_S;

/*Not used*/
typedef struct
{
    POINT_S stPoint;        /* start point */
    FH_UINT32 u32FgAlpha;  /* foreground transparence */
    FH_UINT32 u32BgAlpha;  /* background transparence */
    FH_UINT32 u32Layer;    /* OVERLAYEX region layer */
}OVERLAYEX_CHN_ATTR_S;

/*Not used*/
typedef struct
{
    FH_UINT32  				u32LineWidth;       /* width of line */
    FH_UINT32  				u32LineColor;       /* color of line */
    POINT_S                 stLinePoints[2];    /* startpoint and endpoint of line */
}LINE_CHN_ATTR_S;

/**
 * @brief 定义区域属性联合体。
 *
 */
typedef union
{
    /*! 叠加区域属性。 */
    OVERLAY_ATTR_S      stOverlay;
    /*! 扩展叠加区域属性。不支持。 */
    OVERLAYEX_ATTR_S    stOverlayEx;
} RGN_ATTR_U;

/**
 * @brief 定义区域通道显示属性结构体。
 *
 */
typedef union
{
    /*! 叠加区域通道显示属性。 */
    OVERLAY_CHN_ATTR_S      stOverlayChn;
    /*! 遮挡区域通道显示属性。 */
    COVER_CHN_ATTR_S        stCoverChn;
    /*! 扩展遮挡区域通道显示属性。不支持。 */
    COVEREX_CHN_ATTR_S      stCoverExChn;
    /*! 扩展叠加区域通道显示属性。不支持。 */
    OVERLAYEX_CHN_ATTR_S    stOverlayExChn;
    /*! 线条叠加区域通道显示属性。不支持。 */
    LINE_CHN_ATTR_S    	    stLineChn;
    /*! 马赛克遮挡区域通道显示属性。 */
    MOSAIC_CHN_ATTR_S       stMosaicChn;
} RGN_CHN_ATTR_U;

/**
 * @brief 定义区域属性结构体。
 *
 */
typedef struct
{
    /*! 区域类型。 */
    RGN_TYPE_E enType;
    /*! 区域属性。 */
    RGN_ATTR_U unAttr;
} RGN_ATTR_S;

/**
 * @brief 定义区域通道显示属性结构体。
 *
 */
typedef struct
{
    /*! 区域是否显示。取值范围：FY_TRUE 或者 FY_FALSE。动态属性。 */
    FH_BOOL           bShow;
    /*! 区域类型。静态属性。 */
    RGN_TYPE_E        enType;     /* region type */
    /*! 区域通道显示属性。 */
    RGN_CHN_ATTR_U    unChnAttr;  /* region attribute */
} RGN_CHN_ATTR_S;


typedef struct
{
    PIXEL_FMT_E enPixelFormat;          /* Bitmap's pixel format */
    FH_UINT32   u32Width;               /* Bitmap's width */
    FH_UINT32   u32Height;              /* Bitmap's height */
    FH_VOID    *pData;                  /* Address of Bitmap's data */
} BITMAP_S;



typedef struct
{
    POINT_S             stPoint;
    BITMAP_S            stBmp;
    FH_UINT32           u32Stride;
} RGN_BMP_UPDATE_S;

typedef struct
{
    FH_UINT32           u32BmpCnt;
    RGN_BMP_UPDATE_S    astBmpUpdate[RGN_MAX_BMP_UPDATE_NUM];
} RGN_BMP_UPDATE_CFG_S;

/**
 * @brief 定义画布信息结构体。
 *
 */
typedef struct
{
    /*! 画布物理地址。 */
    FH_UINT32         u32PhyAddr;
    /*! 画布虚拟地址。 */
    FH_UINT32         u32VirtAddr;
    /*! 画布尺寸，宽/高。 */
    FH_SIZE           stSize;
    /*! 画布行跨度。 */
    FH_UINT32         u32Stride;
    /*! 画布的像素格式。 */
    PIXEL_FMT_E       enPixelFmt;
    /*! 画布的调色板。 */
    FH_UINT32         *pu32Clut;
} RGN_CANVAS_INFO_S;

/**********************************************************************
    PART III    - Error Code
**********************************************************************/

/* PingPong buffer change when set attr, it needs to remap memory in mpi interface */
#define FH_NOTICE_RGN_BUFFER_CHANGE  FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_NOTICE, FH_SUCCESS)

/** 0xA0034001: 设备ID超出合法范围 */
#define FH_ERR_RGN_INVALID_DEVID     FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
/** 0xA0034002: 通道组号错误或无效区域句柄 */
#define FH_ERR_RGN_INVALID_CHNID     FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/** 0xA0034003: 参数超出合法范围  */
#define FH_ERR_RGN_ILLEGAL_PARAM     FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/** 0xA0034004: 重复创建已存在的设备、通道或资源 */
#define FH_ERR_RGN_EXIST             FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/** 0xA0034005: 试图使用或者销毁不存在的设备、通道或者资源*/
#define FH_ERR_RGN_UNEXIST           FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/** 0xA0034006: 函数参数中有空指针 */
#define FH_ERR_RGN_NULL_PTR          FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/** 0xA0034007: 模块没有配置 */
#define FH_ERR_RGN_NOT_CONFIG        FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/** 0xA0034008: 不支持的参数或者功能 */
#define FH_ERR_RGN_NOT_SUPPORT       FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/** 0xA0034009: 该操作不允许，如试图修改静态配置参数 */
#define FH_ERR_RGN_NOT_PERM          FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/** 0xA003400C: 分配内存失败，如系统内存不足 */
#define FH_ERR_RGN_NOMEM             FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/** 0xA003400D: 分配缓存失败，如申请的数据缓冲区太大 */
#define FH_ERR_RGN_NOBUF             FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/** 0xA003400E: 缓冲区中无数据 */
#define FH_ERR_RGN_BUF_EMPTY         FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/** 0xA003400F: 缓冲区中数据满 */
#define FH_ERR_RGN_BUF_FULL          FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/** 0xA0038011: 地址非法 */
#define FH_ERR_RGN_BADADDR           FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_BADADDR)
/** 0xA0038012: 系统忙 */
#define FH_ERR_RGN_BUSY              FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)

/** 系统没有初始化或没有加载相应模块 */
#define FH_ERR_RGN_NOTREADY          FH_DEF_ERR(FH_ID_RGN, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __FH_COMM_REGION_H__ */



