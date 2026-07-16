/**
 * @file dcl_base.h
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef INC_EXTERNAL_DCL_BASE_H_
#define INC_EXTERNAL_DCL_BASE_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h> //for strrchr

#ifdef __cplusplus
extern "C" {
#endif

#define FUNC_VISIBILITY
#if defined(_MSC_VER)
#ifdef FUNC_VISIBILITY
#define DCL_FUNC_VISIBILITY _declspec(dllexport)
#else
#define DCL_FUNC_VISIBILITY
#endif
#else
#ifdef FUNC_VISIBILITY
#define DCL_FUNC_VISIBILITY __attribute__((visibility("default")))
#else
#define DCL_FUNC_VISIBILITY
#endif
#endif

#ifdef __GNUC__
#define DCL_DEPRECATED __attribute__((deprecated))
#define DCL_DEPRECATED_MESSAGE(message) __attribute__((deprecated(message)))
#elif defined(_MSC_VER)
#define DCL_DEPRECATED __declspec(deprecated)
#define DCL_DEPRECATED_MESSAGE(message) __declspec(deprecated(message))
#else
#define DCL_DEPRECATED
#define DCL_DEPRECATED_MESSAGE(message)
#endif

typedef void *dclrtStream;
typedef void *dclrtEvent;
typedef void *dclrtContext;
typedef int dclError;
typedef uint16_t dclFloat16;
typedef struct dclDataBuffer dclDataBuffer;
typedef struct dclTensorDesc dclTensorDesc;

static const int DCL_ERROR_NONE = 0;
static const int DCL_SUCCESS = 0;

static const int DCL_ERROR_INVALID_PARAM = 100000;
static const int DCL_ERROR_UNINITIALIZE = 100001;
static const int DCL_ERROR_REPEAT_INITIALIZE = 100002;
static const int DCL_ERROR_INVALID_FILE = 100003;
static const int DCL_ERROR_WRITE_FILE = 100004;
static const int DCL_ERROR_INVALID_FILE_SIZE = 100005;
static const int DCL_ERROR_PARSE_FILE = 100006;
static const int DCL_ERROR_FILE_MISSING_ATTR = 100007;
static const int DCL_ERROR_FILE_ATTR_INVALID = 100008;
static const int DCL_ERROR_INVALID_DUMP_CONFIG = 100009;
static const int DCL_ERROR_INVALID_PROFILING_CONFIG = 100010;
static const int DCL_ERROR_INVALID_MODEL_ID = 100011;
static const int DCL_ERROR_DESERIALIZE_MODEL = 100012;
static const int DCL_ERROR_PARSE_MODEL = 100013;
static const int DCL_ERROR_READ_MODEL_FAILURE = 100014;
static const int DCL_ERROR_MODEL_SIZE_INVALID = 100015;
static const int DCL_ERROR_MODEL_MISSING_ATTR = 100016;
static const int DCL_ERROR_MODEL_INPUT_NOT_MATCH = 100017;
static const int DCL_ERROR_MODEL_OUTPUT_NOT_MATCH = 100018;
static const int DCL_ERROR_MODEL_NOT_DYNAMIC = 100019;
static const int DCL_ERROR_OP_TYPE_NOT_MATCH = 100020;
static const int DCL_ERROR_OP_INPUT_NOT_MATCH = 100021;
static const int DCL_ERROR_OP_OUTPUT_NOT_MATCH = 100022;
static const int DCL_ERROR_OP_ATTR_NOT_MATCH = 100023;
static const int DCL_ERROR_OP_NOT_FOUND = 100024;
static const int DCL_ERROR_OP_LOAD_FAILED = 100025;
static const int DCL_ERROR_UNSUPPORTED_DATA_TYPE = 100026;
static const int DCL_ERROR_FORMAT_NOT_MATCH = 100027;
static const int DCL_ERROR_BIN_SELECTOR_NOT_REGISTERED = 100028;
static const int DCL_ERROR_KERNEL_NOT_FOUND = 100029;
static const int DCL_ERROR_BIN_SELECTOR_ALREADY_REGISTERED = 100030;
static const int DCL_ERROR_KERNEL_ALREADY_REGISTERED = 100031;
static const int DCL_ERROR_INVALID_QUEUE_ID = 100032;
static const int DCL_ERROR_REPEAT_SUBSCRIBE = 100033;
static const int DCL_ERROR_STREAM_NOT_SUBSCRIBE = 100034;
static const int DCL_ERROR_THREAD_NOT_SUBSCRIBE = 100035;
static const int DCL_ERROR_WAIT_CALLBACK_TIMEOUT = 100036;
static const int DCL_ERROR_REPEAT_FINALIZE = 100037;
static const int DCL_ERROR_NOT_STATIC_AIPP = 100038;
static const int DCL_ERROR_COMPILING_STUB_MODE = 100039;
static const int DCL_ERROR_GROUP_NOT_SET = 100040;
static const int DCL_ERROR_GROUP_NOT_CREATE = 100041;
static const int DCL_ERROR_PROF_ALREADY_RUN = 100042;
static const int DCL_ERROR_PROF_NOT_RUN = 100043;
static const int DCL_ERROR_DUMP_ALREADY_RUN = 100044;
static const int DCL_ERROR_DUMP_NOT_RUN = 100045;
static const int DCL_ERROR_PROF_REPEAT_SUBSCRIBE = 148046;
static const int DCL_ERROR_PROF_API_CONFLICT = 148047;
static const int DCL_ERROR_INVALID_MAX_OPQUEUE_NUM_CONFIG = 148048;

static const int DCL_ERROR_BAD_ALLOC = 200000;
static const int DCL_ERROR_API_NOT_SUPPORT = 200001;
static const int DCL_ERROR_INVALID_DEVICE = 200002;
static const int DCL_ERROR_MEMORY_ADDRESS_UNALIGNED = 200003;
static const int DCL_ERROR_RESOURCE_NOT_MATCH = 200004;
static const int DCL_ERROR_INVALID_RESOURCE_HANDLE = 200005;
static const int DCL_ERROR_FEATURE_UNSUPPORTED = 200006;
static const int DCL_ERROR_PROF_MODULES_UNSUPPORTED = 200007;

static const int DCL_ERROR_STORAGE_OVER_LIMIT = 300000;

static const int DCL_ERROR_INTERNAL_ERROR = 500000;
static const int DCL_ERROR_FAILURE = 500001;
static const int DCL_ERROR_GE_FAILURE = 500002;
static const int DCL_ERROR_RT_FAILURE = 500003;
static const int DCL_ERROR_DRV_FAILURE = 500004;
static const int DCL_ERROR_PROFILING_FAILURE = 500005;

static const int DCL_ERROR_VDEC_INVALID_CHN_ID = 0xA0058002;
static const int DCL_ERROR_VDEC_ILLEGAL_PARAM = 0xA0058003;
static const int DCL_ERROR_VDEC_EXIST = 0xA0058004;
static const int DCL_ERROR_VDEC_UNEXIST = 0xA0058005;
static const int DCL_ERROR_VDEC_NULL_PTR = 0xA0058006;
static const int DCL_ERROR_VDEC_NOT_CFG = 0xA0058007;
static const int DCL_ERROR_VDEC_NOT_SUPPORT = 0xA0058008;
static const int DCL_ERROR_VDEC_NOT_PERM = 0xA0058009;
static const int DCL_ERROR_VDEC_NO_MEM = 0xA005800C;
static const int DCL_ERROR_VDEC_NO_BUF = 0xA005800D;
static const int DCL_ERROR_VDEC_BUF_EMPTY = 0xA005800E;
static const int DCL_ERROR_VDEC_BUF_FULL = 0xA005800F;
static const int DCL_ERROR_VDEC_SYS_NOT_READY = 0xA0058010;
static const int DCL_ERROR_VDEC_BAD_ADDR = 0xA0058011;
static const int DCL_ERROR_VDEC_BUSY = 0xA0058012;
static const int DCL_ERROR_VDEC_DEC_FAIL = 0xA0058040;

static const int DCL_ERROR_VENC_INVALID_CHN_ID = 0xA0068002;
static const int DCL_ERROR_VENC_ILLEGAL_PARAM = 0xA0068003;
static const int DCL_ERROR_VENC_EXIST = 0xA0068004;
static const int DCL_ERROR_VENC_UNEXIST = 0xA0068005;
static const int DCL_ERROR_VENC_NULL_PTR = 0xA0068006;
static const int DCL_ERROR_VENC_NOT_CFG = 0xA0068007;
static const int DCL_ERROR_VENC_NOT_SUPPORT = 0xA0068008;
static const int DCL_ERROR_VENC_NOT_PERM = 0xA0068009;
static const int DCL_ERROR_VENC_NO_MEM = 0xA006800C;
static const int DCL_ERROR_VENC_NO_BUF = 0xA006800D;
static const int DCL_ERROR_VENC_BUF_EMPTY = 0xA006800E;
static const int DCL_ERROR_VENC_BUF_FULL = 0xA006800F;
static const int DCL_ERROR_VENC_SYS_NOT_READY = 0xA0068010;
static const int DCL_ERROR_VENC_BAD_ADDR = 0xA0068011;
static const int DCL_ERROR_VENC_BUSY = 0xA0068012;
static const int DCL_ERROR_VENC_ENC_FAIL = 0xA0068040;

// static const int 0xA0078001 - 无效的Device ID。
static const int DCL_ERROR_VPC_INVALID_CHN_ID = 0xA0078002;
static const int DCL_ERROR_VPC_ILLEGAL_PARAM = 0xA0078003;
static const int DCL_ERROR_VPC_EXIST = 0xA0078004;
static const int DCL_ERROR_VPC_UNEXIST = 0xA0078005;
static const int DCL_ERROR_VPC_NULL_PTR = 0xA0078006;
static const int DCL_ERROR_VPC_NOT_CFG = 0xA0078007;
static const int DCL_ERROR_VPC_NOT_SUPPORT = 0xA0078008;
static const int DCL_ERROR_VPC_NOT_PERMIT = 0xA0078009;
static const int DCL_ERROR_VPC_NO_MEM = 0xA007800C;

static const int DCL_ERROR_VPC_NO_BUF = 0xA007800D;
static const int DCL_ERROR_VPC_BUF_EMPTY = 0xA007800E;
static const int DCL_ERROR_VPC_BUF_FULL = 0xA007800F;
static const int DCL_ERROR_VPC_SYS_NOT_READY = 0xA0078010;
static const int DCL_ERROR_VPC_BAD_ADDR = 0xA0078011;
static const int DCL_ERROR_VPC_BUSY = 0xA0078012;
// static const int DCL_ERROR_ = 0xA0078013 - 缓存小于实际需要的大小。
static const int DCL_ERROR_VPC_SW_HW_TIMEOUT = 0xA0078014;
static const int DCL_ERROR_VPC_INNER = 0xA0078015;
static const int DCL_ERROR_VPC_MAX = 0xA007803F;

#define DCL_TENSOR_SHAPE_RANGE_NUM 2
#define DCL_UNKNOWN_RANK 0xFFFFFFFFFFFFFFFE

typedef enum {
    DCL_DEBUG = 0,
    DCL_INFO = 1,
    DCL_WARNING = 2,
    DCL_ERROR = 3,
} dclLogLevel;

typedef enum {
    DCL_DT_UNDEFINED = -1,
    DCL_FLOAT = 0,
    DCL_FLOAT16 = 1,
    DCL_INT8 = 2,
    DCL_INT32 = 3,
    DCL_UINT8 = 4,
    DCL_INT16 = 6,
    DCL_UINT16 = 7,
    DCL_UINT32 = 8,
    DCL_INT64 = 9,
    DCL_UINT64 = 10,
    DCL_DOUBLE = 11,
    DCL_BOOL = 12,
    DCL_INT4 = 13,
    DCL_UINT4 = 14
} dclDataType;

typedef enum {
    DCL_FORMAT_UNDEFINED = -1,
    DCL_FORMAT_NCHW = 0,
    DCL_FORMAT_NHWC = 1,
    DCL_FORMAT_ND = 2,
    DCL_FORMAT_NCDHW = 3, /* D表示depth */
    DCL_FORMAT_NDHWC = 4
} dclFormat;

typedef enum  {
    DCL_PIXEL_FORMAT_YUV_400 = 0,
    DCL_PIXEL_FORMAT_YUV_SEMIPLANAR_420 = 1,
    DCL_PIXEL_FORMAT_YVU_SEMIPLANAR_420 = 2,
    DCL_PIXEL_FORMAT_YUV_SEMIPLANAR_422 = 3,
    DCL_PIXEL_FORMAT_YVU_SEMIPLANAR_422 = 4,
    DCL_PIXEL_FORMAT_YUV_SEMIPLANAR_444 = 5,
    DCL_PIXEL_FORMAT_YVU_SEMIPLANAR_444 = 6,
    DCL_PIXEL_FORMAT_YUYV_PACKED_422 = 7,
    DCL_PIXEL_FORMAT_UYVY_PACKED_422 = 8,
    DCL_PIXEL_FORMAT_YVYU_PACKED_422 = 9,
    DCL_PIXEL_FORMAT_VYUY_PACKED_422 = 10,
    DCL_PIXEL_FORMAT_YUV_PACKED_444 = 11,
    DCL_PIXEL_FORMAT_RGB_888 = 12,
    DCL_PIXEL_FORMAT_BGR_888 = 13,
    DCL_PIXEL_FORMAT_ARGB_8888 = 14,
    DCL_PIXEL_FORMAT_ABGR_8888 = 15,
    DCL_PIXEL_FORMAT_RGBA_8888 = 16,
    DCL_PIXEL_FORMAT_BGRA_8888 = 17,
    DCL_PIXEL_FORMAT_YUV_SEMI_PLANNER_420_10BIT = 18,
    DCL_PIXEL_FORMAT_YVU_SEMI_PLANNER_420_10BIT = 19,
    DCL_PIXEL_FORMAT_YVU_PLANAR_420 = 20,
    DCL_PIXEL_FORMAT_YVU_PLANAR_422,
    DCL_PIXEL_FORMAT_YVU_PLANAR_444,
    DCL_PIXEL_FORMAT_RGB_444 = 23,
    DCL_PIXEL_FORMAT_BGR_444,
    DCL_PIXEL_FORMAT_ARGB_4444,
    DCL_PIXEL_FORMAT_ABGR_4444,
    DCL_PIXEL_FORMAT_RGBA_4444,
    DCL_PIXEL_FORMAT_BGRA_4444,
    DCL_PIXEL_FORMAT_RGB_555,
    DCL_PIXEL_FORMAT_BGR_555,
    DCL_PIXEL_FORMAT_RGB_565,
    DCL_PIXEL_FORMAT_BGR_565,
    DCL_PIXEL_FORMAT_ARGB_1555,
    DCL_PIXEL_FORMAT_ABGR_1555,
    DCL_PIXEL_FORMAT_RGBA_1555,
    DCL_PIXEL_FORMAT_BGRA_1555,
    DCL_PIXEL_FORMAT_ARGB_8565,
    DCL_PIXEL_FORMAT_ABGR_8565,
    DCL_PIXEL_FORMAT_RGBA_8565,
    DCL_PIXEL_FORMAT_BGRA_8565,
    DCL_PIXEL_FORMAT_RGB_BAYER_8BPP = 50,
    DCL_PIXEL_FORMAT_RGB_BAYER_10BPP,
    DCL_PIXEL_FORMAT_RGB_BAYER_12BPP,
    DCL_PIXEL_FORMAT_RGB_BAYER_14BPP,
    DCL_PIXEL_FORMAT_RGB_BAYER_16BPP,
    DCL_PIXEL_FORMAT_RGB_888_PLANAR = 70,
    DCL_PIXEL_FORMAT_BGR_888_PLANAR,
    DCL_PIXEL_FORMAT_HSV_888_PACKAGE,
    DCL_PIXEL_FORMAT_HSV_888_PLANAR,
    DCL_PIXEL_FORMAT_LAB_888_PACKAGE,
    DCL_PIXEL_FORMAT_LAB_888_PLANAR,
    DCL_PIXEL_FORMAT_S8C1,
    DCL_PIXEL_FORMAT_S8C2_PACKAGE,
    DCL_PIXEL_FORMAT_S8C2_PLANAR,
    DCL_PIXEL_FORMAT_S16C1,
    DCL_PIXEL_FORMAT_U8C1,
    DCL_PIXEL_FORMAT_U16C1,
    DCL_PIXEL_FORMAT_S32C1,
    DCL_PIXEL_FORMAT_U32C1,
    DCL_PIXEL_FORMAT_U64C1,
    DCL_PIXEL_FORMAT_S64C1,
    DCL_PIXEL_FORMAT_F32C1,
    DCL_PIXEL_FORMAT_YUV_SEMIPLANAR_440 = 1000,
    DCL_PIXEL_FORMAT_YVU_SEMIPLANAR_440,
    DCL_PIXEL_FORMAT_YUV_PLANAR_444,
    DCL_PIXEL_FORMAT_BUTT,
    DCL_PIXEL_FORMAT_UNKNOWN = 10000
}dclPixelFormat;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} dclRect;

typedef enum {
    False = 0,
    True
} dclBool;

/**
 * @ingroup DCL
 * @brief Converts data of type dclFloat16 to data of type float
 *
 * @param value [IN]   Data to be converted
 * @retval Transformed data
 */
DCL_FUNC_VISIBILITY float dclFloat16ToFloat(dclFloat16 value);

/**
 * @ingroup DCL
 * @brief Converts data of type float to data of type dclFloat16
 *
 * @param value [IN]   Data to be converted
 * @retval Transformed data
 */
DCL_FUNC_VISIBILITY dclFloat16 dclFloatToFloat16(float value);

/**
 * @ingroup DCL
 * @brief create data of dclDataBuffer
 *
 * @param data [IN]    pointer to data
 * @li Need to be managed by the user,
 *  call dclrtMalloc interface to apply for memory,
 *  call dclrtFree interface to release memory
 *
 * @param size [IN]    size of data in bytes
 *
 * @retval pointer to created instance. nullptr if run out of memory
 *
 * @see dclrtMalloc | dclrtFree
 */
DCL_FUNC_VISIBILITY dclDataBuffer *dclCreateDataBuffer(void *data, size_t size);

/**
 * @ingroup DCL
 * @brief create data of dclDataBuffer using user'd phyAddr
 *
 * @param data [IN]    pointer to data
 * @li Need to be managed by the user,
 *  call dclrtMallocEx interface to apply for memory,
 *  call dclrtFree interface to release memory
 *
 * @param size [IN]    size of data in bytes
 *
 * @retval pointer to created instance. nullptr if run out of memory
 *
 * @see
 */
DCL_FUNC_VISIBILITY dclDataBuffer *dclCreateDataBufferWithPhyAddr(void *data, uint64_t phyAddr, size_t size);

/**
 * @ingroup DCL
 * @brief destroy data of dclDataBuffer
 *
 * @par Function
 *  Only the dclDataBuffer type data is destroyed here.
 *  The memory of the data passed in when the dclDataDataBuffer interface
 *  is called to create dclDataBuffer type data must be released by the user
 *
 * @param  dataBuffer [IN]   pointer to the dclDataBuffer
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclCreateDataBuffer
 */
DCL_FUNC_VISIBILITY dclError dclDestroyDataBuffer(const dclDataBuffer *dataBuffer);

/**
 * @ingroup DCL
 * @brief get data address from dclDataBuffer
 *
 * @param dataBuffer [IN]    pointer to the data of dclDataBuffer
 * @retval data address
 */
DCL_FUNC_VISIBILITY void *dclGetDataBufferAddr(const dclDataBuffer *dataBuffer);

/**
 * @ingroup DCL
 * @brief update new data of dclDataBuffer
 *
 * @param dataBuffer [OUT]    pointer to dclDataBuffer
 * @li The old data need to be released by the user, otherwise it may occur memory leak leakage
 *  call dclGetDataBufferAddr interface to get old data address
 *  call dclrtFree interface to release memory
 *
 * @param data [IN]    pointer to new data
 * @li Need to be managed by the user,
 *  call dclrtMalloc interface to apply for memory,
 *  call dclrtFree interface to release memory
 *
 * @param size [IN]    size of data in bytes
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclrtMalloc | dclrtFree | dclGetDataBufferAddr
 */
DCL_FUNC_VISIBILITY dclError dclUpdateDataBuffer(dclDataBuffer *dataBuffer, void *data, size_t size);

/**
 * @ingroup DCL
 * @brief get data address from dclDataBuffer
 *
 * @param dataBuffer [IN]    pointer to the data of dclDataBuffer
 *
 * @retval data address
 */
DCL_FUNC_VISIBILITY void *dclGetDataBufferAddr(const dclDataBuffer *dataBuffer);

/**
 * @ingroup DCL
 * @brief get data size of dclDataBuffer
 *
 * @param  dataBuffer [IN]    pointer to the data of dclDataBuffer
 *
 * @retval data size
 */
DCL_FUNC_VISIBILITY uint32_t dclGetDataBufferSize(const dclDataBuffer *dataBuffer);

/**
 * @ingroup DCL
 * @brief get data size of dclDataBuffer to replace dclGetDataBufferSize
 *
 * @param  dataBuffer [IN]    pointer to the data of dclDataBuffer
 *
 * @retval data size
 */
DCL_FUNC_VISIBILITY size_t dclGetDataBufferSizeV2(const dclDataBuffer *dataBuffer);

/**
 * @ingroup DCL
 * @brief get size of dclDataType
 *
 * @param  dataType [IN]    dclDataType data the size to get
 * @retval size of the dclDataType
 */
DCL_FUNC_VISIBILITY size_t dclDataTypeSize(dclDataType dataType);

// interfaces of tensor desc
/**
 * @ingroup DCL
 * @brief create data dclTensorDesc
 *
 * @param  dataType [IN]    Data types described by tensor
 * @param  numDims [IN]     the number of dimensions of the shape
 * @param  dims [IN]        the size of the specified dimension
 * @param  format [IN]      tensor format
 * @retval dclTensorDesc pointer.
 * @retval nullptr if param is invalid or run out of memory
 */
DCL_FUNC_VISIBILITY dclTensorDesc *dclCreateTensorDesc(dclDataType dataType, int numDims, const int64_t *dims,
                                                       dclFormat format);

/**
 * @ingroup DCL
 * @brief destroy data dclTensorDesc
 *
 * @param desc [IN]     pointer to the data of dclTensorDesc to destroy
 */
DCL_FUNC_VISIBILITY void dclDestroyTensorDesc(const dclTensorDesc *desc);

/**
 * @ingroup DCL
 * @brief get data type specified by the tensor description
 *
 * @param desc [IN]        pointer to the instance of dclTensorDesc
 * @retval data type specified by the tensor description.
 * @retval DCL_DT_UNDEFINED if description is null
 */
DCL_FUNC_VISIBILITY dclDataType dclGetTensorDescType(const dclTensorDesc *desc);

/**
 * @ingroup DCL
 * @brief get data format specified by the tensor description
 *
 * @param  desc [IN]        pointer to the instance of dclTensorDesc
 * @retval data format specified by the tensor description.
 * @retval DCL_FORMAT_UNDEFINED if description is null
 */
DCL_FUNC_VISIBILITY dclFormat dclGetTensorDescFormat(const dclTensorDesc *desc);

/**
 * @ingroup DCL
 * @brief get tensor size specified by the tensor description
 *
 * @param  desc [IN]        pointer to the instance of dclTensorDesc
 * @retval data size specified by the tensor description.
 * @retval 0 if description is null
 */
DCL_FUNC_VISIBILITY size_t dclGetTensorDescSize(const dclTensorDesc *desc);

/**
 * @ingroup DCL
 * @brief get element count specified by the tensor description
 *
 * @param  desc [IN]        pointer to the instance of dclTensorDesc
 * @retval element count specified by the tensor description.
 * @retval 0 if description is null
 */
DCL_FUNC_VISIBILITY size_t dclGetTensorDescElementCount(const dclTensorDesc *desc);

/**
 * @ingroup DCL
 * @brief get number of dims specified by the tensor description
 *
 * @param  desc [IN]        pointer to the instance of dclTensorDesc
 * @retval number of dims specified by the tensor description.
 * @retval 0 if description is null
 */
DCL_FUNC_VISIBILITY size_t dclGetTensorDescNumDims(const dclTensorDesc *desc);

/**
 * @ingroup DCL
 * @brief Get the size of the specified dim in the tensor description
 *
 * @param  desc [IN]        pointer to the instance of dclTensorDesc
 * @param  index [IN]       index of dims, start from 0.
 * @retval dim specified by the tensor description and index.
 * @retval -1 if description or index is invalid
 */
DCL_FUNC_VISIBILITY int64_t dclGetTensorDescDim(const dclTensorDesc *desc, size_t index);

/**
 * @ingroup DCL
 * @brief Get the size of the specified dim in the tensor description
 *
 * @param  desc [IN]        pointer to the instance of aclTensorDesc
 * @param  index [IN]       index of dims, start from 0.
 * @param  dimSize [OUT]    size of the specified dim.
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclGetTensorDescDimV2(const dclTensorDesc *desc, size_t index, int64_t *dimSize);

/**
 * @ingroup DCL
 * @brief set tensor description name
 *
 * @param desc [IN]        pointer to the instance of dclTensorDesc
 * @param name [IN]        tensor description name
 */
DCL_FUNC_VISIBILITY void dclSetTensorDescName(dclTensorDesc *desc, const char *name);

/**
 * @ingroup DCL
 * @brief get tensor description name
 *
 * @param  desc [IN]        pointer to the instance of dclTensorDesc
 * @retval tensor description name.
 * @retval empty string if description is null
 */
DCL_FUNC_VISIBILITY const char *dclGetTensorDescName(dclTensorDesc *desc);

/**
 * @ingroup DCL
 * @brief Convert the format in the source dclTensorDesc according to
 * the specified dstFormat to generate a new target dclTensorDesc.
 * The format in the source dclTensorDesc remains unchanged.
 *
 * @param  srcDesc [IN]     pointer to the source tensor desc
 * @param  dstFormat [IN]   destination format
 * @param  dstDesc [OUT] pointer to the pointer to the destination tensor desc
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclTransTensorDescFormat(const dclTensorDesc *srcDesc, dclFormat dstFormat,
                                                      dclTensorDesc **dstDesc);

/**
 * @ingroup DCL
 * @brief Set the storage format specified by the tensor description
 *
 * @param  desc [IN|OUT]     pointer to the instance of dclTensorDesc
 * @param  format [IN]       the storage format
 * @retval DCL_ERROR_NONE    The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclSetTensorStorageFormat(dclTensorDesc *desc, dclFormat format);

/**
 * @ingroup DCL
 * @brief Set the storage shape specified by the tensor description
 *
 * @param  desc [IN|OUT]      pointer to the instance of dclTensorDesc
 * @param  numDims [IN]       the number of dimensions of the shape
 * @param  dims [IN]          the size of the specified dimension
 * @retval DCL_ERROR_NONE     The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclSetTensorStorageShape(dclTensorDesc *desc, int numDims, const int64_t *dims);

/**
 * @ingroup CL
 * @brief an interface for users to output  APP logs
 *
 * @param logLevel [IN]    the level of current log
 * @param func [IN]        the function where the log is located
 * @param file [IN]        the file where the log is located
 * @param line [IN]        Number of source lines where the log is located
 * @param fmt [IN]         the format of current log
 * @param ... [IN]         the value of current log
 */
DCL_FUNC_VISIBILITY void dclAppLog(dclLogLevel logLevel, const char *func, const char *file, uint32_t line,
                                   const char *fmt, ...);

#define DCL_APP_LOG(level, fmt, ...) dclAppLog(level, __FUNCTION__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define TY_APP_LOG(level, fmt, ...)                                                                                    \
  dclAppLog(level, __FUNCTION__, strrchr(__FILE__, '/') + 1, __LINE__, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // INC_EXTERNAL_DCL_DCL_H_
