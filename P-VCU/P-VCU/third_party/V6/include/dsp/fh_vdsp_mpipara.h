
#ifndef _FH_COMM_DSP_H_
#define _FH_COMM_DSP_H_

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "fh_common.h"
#include "fh_errno.h"

#define FH_MAX_LK_NUM 5

/**********************************************************************
    PART I      - Macro
**********************************************************************/

typedef FH_SINT32 SVP_DSP_HANDLE;


/**********************************************************************
    PART II     - Structure and Enumeration
**********************************************************************/
typedef enum
{
    ERR_SVP_DSP_SYS_TIMEOUT    = 0x40,   //!<  处理任务超时
    ERR_SVP_DSP_QUERY_TIMEOUT  = 0x41,   //!<  查询任务超时
    ERR_SVP_DSP_OPEN_FILE      = 0x42,   //!<  打开模型文件错误
    ERR_SVP_DSP_READ_FILE      = 0x43,   //!<  读模型文件错误

    ERR_SVP_DSP_BUTT
}EN_SVP_DSP_ERR_CODE_E;

/*SVP_DSP core id*/
typedef enum
{
    SVP_DSP_ID_0 = 0x0,

    SVP_DSP_ID_BUTT
}SVP_DSP_ID_E;

/*SVP_DSP memory type*/
typedef enum
{
    SVP_DSP_MEM_TYPE_SYS_DDR_DSP_0 = 0x0,
    SVP_DSP_MEM_TYPE_IRAM_DSP_0    = 0x1,
    SVP_DSP_MEM_TYPE_DRAM_0_DSP_0  = 0x2,
    SVP_DSP_MEM_TYPE_DRAM_1_DSP_0  = 0x3,

    SVP_DSP_MEM_TYPE_BUTT
}SVP_DSP_MEM_TYPE_E;

typedef struct
{
    FH_UINT32   u32Width;           //!<  输入图像数据宽度
    FH_UINT32   u32Height;          //!<  输入图像数据高度
    FH_UINT32   u32Stride;          //!<  输入图像的stride
    FH_MEM_INFO stSrcData[2];       //!<  输入图像的地址信息
    FH_UINT32   u16Reserved;
}SVP_DSP_SRC_IMAGE_S;

typedef struct
{
    FH_MEM_INFO stDstData;          //!<    输入图像的地址信息
}SVP_DSP_DST_IMAGE_S;

typedef struct  
{
	FH_FLOAT x;     //!<  坐标x
	FH_FLOAT y;     //!<  坐标y
}POINT_F_S;


typedef struct
{
	FH_FLOAT l;     //!<  矩形框lef坐标
	FH_FLOAT t;     //!<  矩形框top坐标
	FH_FLOAT r;     //!<  矩形框right坐标
	FH_FLOAT b;     //!<  矩形框bottom坐标
}RECT_F_S;

typedef struct
{
	RECT_F_S      stBbox;                    //!< 矩形信息
	POINT_F_S     stLandmark[FH_MAX_LK_NUM]; //!< 人脸关键点信息
}SVP_DSP_BBOX_S;

typedef struct {
	FH_UINT32   x; //!<  顶点坐标x
	FH_UINT32   y; //!<  顶点坐标y
}SVP_DSP_VERTEX_S;

/*Image type*/
typedef enum
{
   SVP_IMAGE_TYPE_YUV400         =  0x1,     //!<  YUV400
   SVP_IMAGE_TYPE_YUV420SP       =  0x2,     //!<  YUV420 SemiPlanar
   SVP_IMAGE_TYPE_YUV422SP       =  0x3,     //!<  YUV422 SemiPlanar
   SVP_IMAGE_TYPE_YUV420P        =  0x4,     //!<  YUV420 Planar
   SVP_IMAGE_TYPE_YUV422P        =  0x5,     //!<  YUV422 planar

   SVP_IMAGE_TYPE_RGB888         =  0x6,     //!<  RGB888
   SVP_IMAGE_TYPE_ARGB8888       =  0x7,     //!<  ARGB8888

   SVP_IMAGE_TYPE_BUTT
}SVP_DSP_IMAGE_TYPE_E;

typedef struct
{
    SVP_DSP_IMAGE_TYPE_E  enType;    //!<  图像数据格式
    FH_UINT32    u32Width;           //!<  图像数据宽度
    FH_UINT32    u32Height;          //!<  图像数据高度
    FH_UINT32    au32PhyAddr[3];     //!<  图像的地址信息
    FH_UINT32    au32Stride[3];      //!<  图像的stride
}SVP_DSP_IMAGE_EXT_S;

typedef SVP_DSP_IMAGE_EXT_S SVP_DSP_SRC_IMAGE_EXT_S;
typedef SVP_DSP_IMAGE_EXT_S SVP_DSP_DST_IMAGE_EXT_S;

typedef struct
{
    FH_UINT32      u32CmdId;            //!<  命令ID
    FH_UINT32      u32ParamPhyAddr;     //!<  配置参数地址
    FH_UINT32      u32ParamLen;         //!<  配置参数长度
    FH_SINT32      u32TimeoutMs;        //!<  超时时间(单位ms)
} SVP_DSP_REQ_PARAMS_S;

/**********************************************************************
    PART III    - Error Code
**********************************************************************/
/*Invalid device ID*/
#define FH_ERR_SVP_DSP_INVALID_DEVID     FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
/*Invalid channel ID*/
#define FH_ERR_SVP_DSP_INVALID_CHNID     FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/*At least one parameter is illegal. For example, an illegal enumeration value exists.*/
#define FH_ERR_SVP_DSP_ILLEGAL_PARAM     FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/*The channel exists.*/
#define FH_ERR_SVP_DSP_EXIST             FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/*The UN exists.*/
#define FH_ERR_SVP_DSP_UNEXIST           FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/*A null point is used.*/
#define FH_ERR_SVP_DSP_NULL_PTR          FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/*Try to enable or initialize the system, device, or channel before configuring attributes.*/
#define FH_ERR_SVP_DSP_NOT_CONFIG        FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/*The operation is not supported currently.*/
#define FH_ERR_SVP_DSP_NOT_SURPPORT      FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/*The operation, changing static attributes for example, is not permitted.*/
#define FH_ERR_SVP_DSP_NOT_PERM          FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/*A failure caused by the malloc memory occurs.*/
#define FH_ERR_SVP_DSP_NOMEM             FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/*A failure caused by the malloc buffer occurs.*/
#define FH_ERR_SVP_DSP_NOBUF             FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/*The buffer is empty.*/
#define FH_ERR_SVP_DSP_BUF_EMPTY         FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/*No buffer is provided for storing new data.*/
#define FH_ERR_SVP_DSP_BUF_FULL          FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/*The system is not ready because it may be not initialized or loaded.
 *The error code is returned when a device file fails to be opened. */
#define FH_ERR_SVP_DSP_NOTREADY          FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/*The source address or target address is incorrect during the operations such as calling copy_from_user or copy_to_user.*/
#define FH_ERR_SVP_DSP_BADADDR           FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, EN_ERR_BADADDR)
/*The resource is busy during the operations such as destroying a VENC channel without deregistering it.*/
#define FH_ERR_SVP_DSP_BUSY              FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
/*SVP_DSP process timeout*/
#define FH_ERR_SVP_DSP_SYS_TIMEOUT       FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, ERR_SVP_DSP_SYS_TIMEOUT)
/*SVP_DSP query timeout*/
#define FH_ERR_SVP_DSP_QUERY_TIMEOUT     FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, ERR_SVP_DSP_QUERY_TIMEOUT)
/*SVP_DSP open file error*/
#define FH_ERR_SVP_DSP_OPEN_FILE         FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, ERR_SVP_DSP_OPEN_FILE)
/*SVP_DSP read file error*/
#define FH_ERR_SVP_DSP_READ_FILE         FH_DEF_ERR(FH_OBJ_VDSP, EN_ERR_LEVEL_ERROR, ERR_SVP_DSP_READ_FILE)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif/*_FH_COMM_DSP_H_*/
