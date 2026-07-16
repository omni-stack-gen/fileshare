#ifndef TY_BASE_H_
#define TY_BASE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)

/* 版本信息 */
#define TY_NPU_MAJOR_VERSION    1
#define TY_NPU_MINOR_VERSION    1
#define TY_NPU_PATCH_VERSION    10

#define TY_NPU_FUNC_VISIBILITY __attribute__((visibility("default")))
#define TY_NPU_MAX_TENSOR_NAME_LEN       128
#define TY_NPU_MAX_TENSOR_DIM_CNT        5
#define TY_NPU_MODLE_BLOB_MAX_INPUT_NUM  128
#define TY_NPU_MODLE_BLOB_MAX_OUTPUT_NUM 128
#define TY_NPU_MEM_MAX_SEGMENT_NUM       8
#define TY_NPU_OPTYPE_STRING_MAX_LEN     256U  /* optype 字符串最大长度, byte为单位 */

typedef enum TY_BOOL_TYPE{
    E_TY_FALSE = 0x0,
    E_TY_TRUE
}E_TY_Bool;

typedef enum TY_EngineType{
    E_TY_AI_CPU = 0x0,
    E_TY_AI_NPU,
    E_TY_AI_ARM_CPU,
    E_TY_AI_RISCV_CPU,
    E_TY_AI_DSP,
    E_TY_AI_TYPE_BUTT
}E_TY_EngineType;

typedef enum TY_NpuID {
    E_TY_NPU_ID_0 = 0,
    E_TY_NPU_ID_1,
    E_TY_NPU_ID_2,
    E_TY_NPU_ID_3,
    E_TY_NPU_ID_4,
    E_TY_NPU_ID_5,
    E_TY_NPU_ID_6,
    E_TY_NPU_ID_7,
    E_TY_NPU_ID_8,
    E_TY_NPU_ID_9,
    E_TY_NPU_ID_10,
    E_TY_NPU_ID_11,
    E_TY_NPU_ID_12,
    E_TY_NPU_ID_13,
    E_TY_NPU_ID_14,
    E_TY_NPU_ID_15,
    E_TY_NPU_ID_BUTT
}E_TY_NpuID;

typedef struct TY_Mem {
    uint64_t  phyAddr; /* 内存物理地址 */
    uint64_t  virAddr; /* 内存虚拟地址 */
    uint32_t  size;    /* 内存大小     */
}T_TY_Mem;

typedef enum TY_MemAllocType {
    E_TY_MEM_VMM_NO_CACHED = 0,/* vmm管理的物理地址连续、非cached属性内存*/
    E_TY_MEM_VMM_CACHED,       /* vmm管理的物理地址连续、cached属性内存*/
    E_TY_MEM_MALLOC,           /* 非vmm管理的内存*/
    E_TY_MEM_ALLOC_TYPE_BUTT
}E_TY_MemAllocType;

typedef enum TY_MemShareType {
    E_MEM_SHARED = 0,       /* 模型共享内存，NPU不使用期间，应用可读可写*/
    E_MEM_EXCLUSIVED,       /* 模型独享内存，应用只读*/
    E_MEM_SHARE_TYPE_BUTT
}E_TY_MemShareType;

typedef struct TY_MemAllocInfo{
    int32_t size;
    E_TY_MemAllocType allocType;
    E_TY_MemShareType shareType;
    uint32_t alignByteSize;
}T_TY_MemAllocInfo;

typedef struct TY_MemInfo{
    T_TY_MemAllocInfo allocInfo;
    T_TY_Mem mem;
}T_TY_MemInfo;

typedef struct TY_MemSegmentInfo{
    int32_t segNum;
    T_TY_MemInfo memInfo[TY_NPU_MEM_MAX_SEGMENT_NUM];
}T_TY_MemSegmentInfo;

typedef enum {
    E_TY_DT_UNDEFINED = -1,
    E_TY_FLOAT = 0,
    E_TY_FLOAT16 = 1,
    E_TY_INT8 = 2,
    E_TY_INT32 = 3,
    E_TY_UINT8 = 4,
    E_TY_INT16 = 6,
    E_TY_UINT16 = 7,
    E_TY_UINT32 = 8,
    E_TY_INT64 = 9,
    E_TY_UINT64 = 10,
    E_TY_DOUBLE = 11,
    E_TY_BOOL = 12,
    E_TY_INT4 = 13,
    E_TY_UINT4 = 14
}E_TY_DataType;

typedef enum {
    E_TY_FORMAT_UNDEFINED = -1,
    E_TY_FORMAT_NCHW = 0,
    E_TY_FORMAT_NHWC = 1,
    E_TY_FORMAT_ND = 2,
    E_TY_FORMAT_NCDHW = 3, /* D表示depth */
    E_TY_FORMAT_NDHWC = 4
}E_TY_LayoutFormat;

typedef enum  {
    E_TY_PIXEL_FORMAT_YUV_400 = 0,
    E_TY_PIXEL_FORMAT_YUV_SEMIPLANAR_420 = 1,
    E_TY_PIXEL_FORMAT_YVU_SEMIPLANAR_420 = 2,
    E_TY_PIXEL_FORMAT_YUYV_PACKED_422 = 7,
    E_TY_PIXEL_FORMAT_UYVY_PACKED_422 = 8,
    E_TY_PIXEL_FORMAT_RGB_888_PACKED = 12,
    E_TY_PIXEL_FORMAT_BGR_888_PACKED = 13,
    E_TY_PIXEL_FORMAT_RGB_888_PLANAR = 70,
    E_TY_PIXEL_FORMAT_BGR_888_PLANAR,
    E_TY_PIXEL_FORMAT_S8C1 = 76,   /* ive */
    E_TY_PIXEL_FORMAT_S8C2_PACKAGE,
    E_TY_PIXEL_FORMAT_S8C2_PLANAR,
    E_TY_PIXEL_FORMAT_S16C1,
    E_TY_PIXEL_FORMAT_U8C1,
    E_TY_PIXEL_FORMAT_U16C1,
    E_TY_PIXEL_FORMAT_S32C1,
    E_TY_PIXEL_FORMAT_U32C1,
    E_TY_PIXEL_FORMAT_U64C1,
    E_TY_PIXEL_FORMAT_S64C1,
    E_TY_PIXEL_FORMAT_YVU_PLANAR_444=22,
    E_TY_PIXEL_FORMAT_YUV_PLANAR_444=1002,
    E_TY_PIXEL_FORMAT_BUTT = 1004
}E_TY_PixelFormat;

typedef struct TY_Rect{
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
}T_TY_Rect;

typedef enum {
    E_TY_BLOB_DATA = 0,
    E_TY_BLOB_IMAGE,
    E_TY_BLOB_IMAGE_WITH_PRE_PROC,// 指示此BLOB位置需要进行预处理，仅支持基于IMAGE进行预处理，预处理的输入T_ImageDesc来自用户
    E_TY_BLOB_TYPE_TY_BUTT
}E_TY_BLOB_TYPE;

typedef struct TY_TensorDesc {
    char name[TY_NPU_MAX_TENSOR_NAME_LEN];
    E_TY_DataType dataType;
    E_TY_LayoutFormat format;
    int32_t numDims;
    int64_t dims[TY_NPU_MAX_TENSOR_DIM_CNT];
}T_TY_TensorDesc;

typedef struct TY_ImgDesc{
    E_TY_PixelFormat picFormat;
    int32_t picWidth;        /*以像素为单位*/
    int32_t picHeight;       /*以像素为单位*/
    int32_t picWidthStride;  /*以像素为单位*/
    int32_t picHeightStride; /*以图像高像素为单位*/
    T_TY_Rect roi;           /*整图时可设置为{0,0,picWidth,picHeight}或{0,0,0,0}*/
}T_TY_ImgDesc;

typedef struct {
    E_TY_BLOB_TYPE type;
    union{
        T_TY_TensorDesc tensor; //对应type==E_TY_BLOB_DATA
        T_TY_ImgDesc img;       //对应type==E_TY_BLOB_IMAGE，模型输入为image
        int32_t reserved;    //对应type==E_TY_BLOB_IMAGE_WITH_PRE_PROC,模型输入需要预处理，image由用户输入
    };
}T_TY_BlobDesc;

typedef struct TY_Image{
    T_TY_ImgDesc desc;
    T_TY_Mem mem;
}T_TY_Image;

typedef uint32_t TY_Error;

/************************************************IVE error code ***********************************/
typedef enum{
    TY_IVE_SUCCESS = 0,
    TY_ERR_IVE_UNINITIALIZE =  0xA01D8001,
    TY_ERR_IVE_REPEAT_INITIALIZE = 0xA01D8002,
    TY_ERR_IVE_ILLEGAL_PARAM  = 0xA01D8003,
    TY_ERR_IVE_NULL_PTR  = 0xA01D8004,
    TY_ERR_IVE_NOT_SUPPORT  = 0xA01D8005,
    TY_ERR_IVE_EXEC_FAIL  = 0xA01D8006,
    TY_ERROR_REPEAT_EXIT  = 0xA01D8007
}E_TY_IVE_Error;

/************************************************CV error code ***********************************/
typedef enum{
    TY_CV_SUCCESS = 0,
    TY_ERR_CV_UNINITIALIZE =  0xA01D8001,
    TY_ERR_CV_REPEAT_INITIALIZE = 0xA01D8002,
    TY_ERR_CV_ILLEGAL_PARAM  = 0xA01D8003,
    TY_ERR_CV_NULL_PTR  = 0xA01D8004,
    TY_ERR_CV_NOT_SUPPORT  = 0xA01D8005,
    TY_ERR_CV_EXEC_FAIL  = 0xA01D8006,
    TY_ERR_CV_REPEAT_EXIT  = 0xA01D8007
}E_TY_CV_Error;

/************************************************NPU error code ***********************************/
typedef enum{
    TY_NPU_SUCCESS = 0,
    TY_ERR_NPU_INVALID_PARAM = 100000,
    TY_ERR_NPU_UNINITIALIZE = 100001,
    TY_ERR_NPU_REPEAT_INITIALIZE = 100002,
    TY_ERR_NPU_INVALID_FILE = 100003,
    TY_ERR_NPU_WRITE_FILE = 100004,
    TY_ERR_NPU_INVALID_FILE_SIZE = 100005,
    TY_ERR_NPU_PARSE_FILE = 100006,
    TY_ERR_NPU_INVALID_MODEL = 100011,
    TY_ERR_NPU_INVALID_TASK = 100012,
    TY_ERR_NPU_PARSE_MODEL = 100013,
    TY_ERR_NPU_MODEL_SIZE_INVALID = 100015,
    TY_ERR_NPU_MODEL_INPUT_NOT_MATCH = 100017,
    TY_ERR_NPU_MODEL_OUTPUT_NOT_MATCH = 100018,
    TY_ERR_NPU_UNSUPPORTED_DATA_TYPE = 100026,
    TY_ERR_NPU_FORMAT_NOT_MATCH = 100027,
    TY_ERR_NPU_REPEAT_FINALIZE = 100037,
    TY_ERR_NPU_API_NOT_SUPPORT = 200001,
    TY_ERR_NPU_INVALID_DEVICE = 200002,
    TY_ERR_NPU_MEMORY_ADDRESS_UNALIGNED = 200003,
    TY_ERR_NPU_RESOURCE_NOT_MATCH = 200004,
    TY_ERR_NPU_INVALID_RESOURCE_HANDLE = 200005,
    TY_ERR_NPU_FEATURE_UNSUPPORTED = 200006
}E_TY_NPU_Error;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif
