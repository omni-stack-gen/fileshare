/**
* @file dcl_mdl.h
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef INC_EXTERNAL_DCL_DCL_MODEL_H_
#define INC_EXTERNAL_DCL_DCL_MODEL_H_

#include <stddef.h>
#include <stdint.h>
#include "dcl_base.h"
typedef uint64_t dclNnHandle;

#ifdef __cplusplus
extern "C" {
#endif

#define DCL_MAX_DIM_CNT          128
#define DCL_MAX_TENSOR_NAME_LEN  128
#define DCL_MAX_BATCH_NUM        128
#define DCL_MAX_HW_NUM           128
#define DCL_MAX_SHAPE_COUNT      128
#define DCL_INVALID_NODE_INDEX   0xFFFFFFFF

#define DCL_NPU_NU_SUPPORT_DATA_FORMAT_LEN 256
#define DCL_NPU_VU_SUPPORT_DATA_FORMAT_LEN 256

#define DCL_DYNAMIC_AIPP_NAME "deepeye_dynamic_aipp_data"

typedef struct dclmdlDataset dclmdlDataset;
typedef struct dclmdlDesc dclmdlDesc;
typedef struct dclmdlAIPP dclmdlAIPP;
typedef dclPixelFormat dclAippInputFormat;


typedef enum DCL_MemAllocType {
    E_DCL_MEM_VMM_NO_CACHED = 0,/* vmm管理的物理地址连续、非cached属性内存*/
    E_DCL_MEM_VMM_CACHED,       /* vmm管理的物理地址连续、cached属性内存*/
    E_DCL_MEM_MALLOC,           /* 非vmm管理的内存*/
    E_DCL_MEM_ALLOC_TYPE_BUTT
}E_DCL_MemAllocType;

typedef struct dclmdlIODims {
    char name[DCL_MAX_TENSOR_NAME_LEN]; /**< tensor name */
    size_t dimCount;  /**< dim array count */
    int64_t dims[DCL_MAX_DIM_CNT]; /**< dim data array */
} dclmdlIODims;

typedef enum {
    DCL_DATA_WITHOUT_AIPP = 0,
    DCL_DATA_WITH_STATIC_AIPP,//当前版本不支持
    DCL_DATA_WITH_DYNAMIC_AIPP,
    DCL_DYNAMIC_AIPP_NODE
} dclmdlInputAippType;

typedef struct dclmdlBatch {
    size_t batchCount; /**< batch array count */
    uint64_t batch[DCL_MAX_BATCH_NUM]; /**< batch data array */
} dclmdlBatch;

typedef struct dclmdlHW {
    size_t hwCount; /**< height&width array count */
    uint64_t hw[DCL_MAX_HW_NUM][2]; /**< height&width data array */
} dclmdlHW;

typedef struct dclAippInfo {
    int32_t resizeSwitch;
    int32_t resizeType;   //0:normal 1:等比例
    int32_t resizeOutputW;//resizeType=0时对应真实resizeOut的宽，resizeType=1时对应等比例折算的resizeOut宽
    int32_t resizeOutputH;//resizeType=0时对应真实resizeOut的高，resizeType=1时对应等比例折算的resizeOut高
    int32_t paddingSwitch;//
    int32_t paddingType;   //0:中心对称 1:normal
    int32_t paddingOutputW;//模型输入宽
    int32_t paddingOutputH;//模型输入高
    int32_t leftPaddingSize;//paddingType=normal有效
    int32_t rightPaddingSize;
    int32_t topPaddingSize;
    int32_t bottomPaddingSize;
    int8_t  paddingConstValue;
    dclPixelFormat pixFormat;//模型输入图像格式，如PIXEL_FORMAT_RGB_888_PLANAR/PIXEL_FORMAT_BGR_888_PLANAR
} dclAippInfo;

typedef struct dclmdlTaskStatistic{
    uint32_t used;
    uint32_t free;
    uint32_t all;
    float usageRate;
} dclmdlTaskStatistic;

typedef struct dclNpuHwCapacity{
    uint32_t nuCurrFreq;  /* Mhz */
    uint32_t nuMaxFreq;   /* Mhz */
    uint32_t vuCurrFreq;  /* Mhz */
    uint32_t vuMaxFreq;   /* Mhz */
    float nuTops;         /* nu 算力 */
    float vuTops;         /* vu 算力 */
    char nuDataFmt[DCL_NPU_NU_SUPPORT_DATA_FORMAT_LEN];
    char vuDataFmt[DCL_NPU_VU_SUPPORT_DATA_FORMAT_LEN];
}dclNpuHwCapacity;

typedef struct dclNpuDrvCapacity{
    uint32_t taskQueDepth;  /* 可配置的队列深度信息 */
}dclNpuDrvCapacity;

typedef struct dclNpuCapacityInfo{
    dclNpuHwCapacity hardware;
    dclNpuDrvCapacity driver;
}dclNpuCapacityInfo;

/**
 * @ingroup DCL
 * @brief Create data of type dclmdlDesc
 *
 * @retval the dclmdlDesc pointer
 */
DCL_FUNC_VISIBILITY dclmdlDesc *dclmdlCreateDesc();

/**
 * @ingroup DCL
 * @brief destroy data of type dclmdlDesc
 *
 * @param modelDesc [IN]   Pointer to almdldlDesc to be destroyed
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlDestroyDesc(dclmdlDesc *modelDesc);

/**
 * @ingroup DCL
 * @brief Get dclmdlDesc data of the model according to the model ID
 *
 * @param  modelDesc [OUT]   dclmdlDesc pointer
 * @param  modelId [IN]      model id
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlGetDesc(dclmdlDesc *modelDesc, uint32_t modelId);

/**
 * @ingroup DCL
 * @brief Get the number of the inputs of
 *        the model according to data of dclmdlDesc
 *
 * @param  modelDesc [IN]   dclmdlDesc pointer
 *
 * @retval input size with dclmdlDesc
 */
DCL_FUNC_VISIBILITY size_t dclmdlGetNumInputs(dclmdlDesc *modelDesc);

/**
 * @ingroup DCL
 * @brief Get the number of the output of
 *        the model according to data of dclmdlDesc
 *
 * @param  modelDesc [IN]   dclmdlDesc pointer
 *
 * @retval output size with dclmdlDesc
 */
DCL_FUNC_VISIBILITY size_t dclmdlGetNumOutputs(dclmdlDesc *modelDesc);

/**
 * @ingroup DCL
 * @brief Get the size of the specified input according to
 *        the data of type dclmdlDesc
 *
 * @param  modelDesc [IN]  dclmdlDesc pointer
 * @param  index [IN] the size of the number of inputs to be obtained,
 *         the index value starts from 0
 *
 * @retval Specify the size of the input
 */
DCL_FUNC_VISIBILITY size_t dclmdlGetInputSizeByIndex(dclmdlDesc *modelDesc, size_t index);

/**
 * @ingroup DCL
 * @brief Get the size of the specified output according to
 *        the data of type dclmdlDesc
 *
 * @param modelDesc [IN]   dclmdlDesc pointer
 * @param index [IN]  the size of the number of outputs to be obtained,
 *        the index value starts from 0
 *
 * @retval Specify the size of the output
 */
DCL_FUNC_VISIBILITY size_t dclmdlGetOutputSizeByIndex(dclmdlDesc *modelDesc, size_t index);

/**
 * @ingroup DCL
 * @brief Create data of type dclmdlDataset
 *
 * @retval the dclmdlDataset pointer
 */
DCL_FUNC_VISIBILITY dclmdlDataset *dclmdlCreateDataset();

/**
 * @ingroup DCL
 * @brief destroy data of type dclmdlDataset
 *
 * @param  dataset [IN]  Pointer to dclmdlDataset to be destroyed
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlDestroyDataset(const dclmdlDataset *dataset);

/**
 * @ingroup DCL
 * @brief Add dclDataBuffer to dclmdlDataset
 *
 * @param dataset [OUT]    dclmdlDataset address of dclDataBuffer to be added
 * @param dataBuffer [IN]  dclDataBuffer address to be added
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlAddDatasetBuffer(dclmdlDataset *dataset, dclDataBuffer *dataBuffer);

/**
 * @ingroup DCL
 * @brief Get the number of dclDataBuffer in dclmdlDataset
 *
 * @param dataset [IN]   dclmdlDataset poiter
 *
 * @retval the number of dclDataBuffer
 */
DCL_FUNC_VISIBILITY size_t dclmdlGetDatasetNumBuffers(const dclmdlDataset *dataset);

/**
 * @ingroup DCL
 * @brief Get the dclDataBuffer in dclmdlDataset by index
 *
 * @param dataset [IN]   dclmdlDataset poiter
 * @param index [IN]     the index of dclDataBuffer
 *
 * @retval Get successfully, return the address of dclDataBuffer
 * @retval Failure return NULL
 */
DCL_FUNC_VISIBILITY dclDataBuffer *dclmdlGetDatasetBuffer(const dclmdlDataset *dataset, size_t index);

/**
 * @ingroup DCL
 * @brief Load offline model data from files
 * and manage memory internally by the system
 *
 * @par Function
 * After the system finishes loading the model,
 * the model ID returned is used as a mark to identify the model
 * during subsequent operations.
 * @param modelPath [IN]   Storage path for offline model files
 * @param modelId [OUT]  Model ID generated after
 *        the system finishes loading the model
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlLoadFromFile(const char *modelPath, uint32_t *modelId);

/**
 * @ingroup DCL
 * @brief Load offline model data from memory and manage the memory of
 * model running internally by the system
 *
 * @par Function
 * After the system finishes loading the model,
 * the model ID returned is used as a mark to identify the model
 * during subsequent operations
 *
 * @param model [IN]      Model data stored in memory
 * @param modelSize [IN]  model data size
 * @param modelId [OUT]   Model ID generated after
 *        the system finishes loading the model
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlLoadFromMem(const void *model,  size_t modelSize,
                                               uint32_t *modelId);

/**
 * @ingroup DCL
 * @brief Load offline model data from memory and use the memory which must be
 * physical continuous. Can't release the memory until unloadModel.
 * @par Function
 * After the system finishes loading the model,
 * the model ID returned is used as a mark to identify the model
 * during subsequent operations
 *
 * @param model [IN]      Model data stored in physical continous memory
 * @param modelPhyAddr[IN] Physical addr of model data
 * @param modelSize [IN]  model data size
 * @param modelId [OUT]   Model ID generated after
 *        the system finishes loading the model
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlLoadFromModelMem(const void *model,
                        uint64_t modelPhyAddr,size_t modelSize,  uint32_t *modelId, E_DCL_MemAllocType memType);

/**
 * @ingroup DCL
 * @brief Load offline model data from a file,
 * and the user manages the memory of the model run by itself
 *
 * @par Function
 * After the system finishes loading the model,
 * the model ID returned is used as a mark to identify the model
 * during subsequent operations.
 * @param modelPath [IN]   Storage path for offline model files
 * @param modelId [OUT]  Model ID generated after finishes loading the model
 * @param workPtr [IN]  A pointer to the working memory
 *        required by the model on the Device,can be null
 * @param workSize [IN]  The amount of working memory required by the model
 * @param weightPtr [IN]  Pointer to model weight memory on Device
 * @param weightSize [IN]  The amount of weight memory required by the model
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlLoadFromFileWithMem(const char *modelPath,
    uint32_t *modelId, void *workPtr, size_t workSize,
    void *weightPtr, size_t weightSize);

/**
 * @ingroup DCL
 * @brief Load offline model data from memory,
 * and the user can manage the memory of model running
 *
 * @par Function
 * After the system finishes loading the model,
 * the model ID returned is used as a mark to identify the model
 * during subsequent operations
 * @param model [IN]   Model data stored in memory
 * @param modelSize [IN]  model data size
 * @param modelId [OUT]  Model ID generated after finishes loading the model
 * @param workPtr [IN]  A pointer to the working memory
 *        required by the model on the Device,can be null
 * @param workPhyAddr[IN] correspond to workPtr
 * @param workSize [IN]  work memory size, must be zero if workPtr is null
 * @param weightPtr [IN] Pointer to model weight memory on Device,can't be null
 * @param weightPhyAddr[IN] correspond to weightPtr
 * @param weightSize [IN]  The amount of weight memory required by the model
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 * @see dclmdlQuerySizeFromMem FOR workSize and weightSize
 */
/*DCL_FUNC_VISIBILITY dclError dclmdlLoadFromMemWithMem(const void *model, size_t modelSize,
    uint32_t *modelId, void *workPtr, size_t workSize,
    void *weightPtr, size_t weightSize);*/

DCL_FUNC_VISIBILITY dclError dclmdlLoadFromMemWithMem(const void *model, size_t modelSize,
    uint32_t *modelId, void *workPtr, uint64_t workPhyAddr, size_t workSize,
    void *weightPtr, uint64_t weightPhyAddr, size_t weightSize);

/**
 * @ingroup DCL
 * @brief load model from file with async queue
 *
 * @param modelPath  [IN]   model path
 * @param modelId [OUT]  return model id if load success
 * @param inputQ [IN]  input queue pointer
 * @param inputQNum [IN]  input queue num
 * @param outputQ [IN]  output queue pointer
 * @param outputQNum [IN]  output queue num
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlLoadFromFileWithQ(const char *modelPath, uint32_t *modelId, const uint32_t *inputQ,
    size_t inputQNum, const uint32_t *outputQ, size_t outputQNum);

/**
 * @ingroup DCL
 * @brief load model from memory with async queue
 *
 * @param model [IN]  model memory which user manages
 * @param modelSize [IN]  model size
 * @param modelId [OUT]  return model id if load success
 * @param inputQ [IN]  input queue pointer
 * @param inputQNum [IN]  input queue num
 * @param outputQ [IN]  output queue pointer
 * @param outputQNum [IN]  output queue num
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlLoadFromMemWithQ(const void *model, size_t modelSize, uint32_t *modelId,
    const uint32_t *inputQ, size_t inputQNum, const uint32_t *outputQ, size_t outputQNum);

/**
 * @ingroup DCL
 * @brief Execute model synchronous inference until the inference result is returned
 *
 * @param  modelId [IN]   ID of the model to perform inference
 * @param  input [IN]     Input data for model inference
 * @param  output [OUT]   Output data for model inference
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlExecute(uint32_t modelId, const dclmdlDataset *input, dclmdlDataset *output);

/**
 * @ingroup DCL
 * @brief Execute model synchronous inference until the inference result is returned
 *
 * @param  modelId [IN]   ID of the model to perform inference
 * @param  input [IN]     Input data for model inference
 * @param  output [OUT]   Output data for model inference
 * @param  priority [IN]  model excution priority, 0 - lowpriority, 1 - highpriority
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlExecuteWithPriority(uint32_t modelId, const dclmdlDataset *input, dclmdlDataset *output, int32_t priority);

/**
 * @ingroup DCL
 * @brief Execute model synchronous inference until the inference result is returned with user's workbuf
 *
 * @param  modelId [IN]   ID of the model to perform inference
 * @param  input [IN]     Input data for model inference
 * @param  workBuf [IN]   Input work buf memrory for inference
 * @param  output [OUT]   Output data for model inference
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlExecuteWithWorkBuf(uint32_t modelId, const dclmdlDataset *input,
                            dclDataBuffer *workBuf, dclmdlDataset *output);

/**
 * @ingroup DCL
 * @brief Execute model synchronous inference until the inference result is returned
 *        with user's workbuf/priority/profilerHwBuffer/profilerBuffer/dumpBuffer
 *
 * @param  modelId [IN]   ID of the model to perform inference
 * @param  input [IN]     Input data for model inference
 * @param  workBuf [IN]   Input work buf memrory for inference
 * @param  output [OUT]   Output data for model inference
 * @param  priority[IN]   Inference priority,  0: normal, 1: high
 * @param  profileHwBuffer[OUT] Profile buf for hw profile out
 * @param  profileBuffer[OUT] Profile buf for copy hw profile
 * @param  dumpBuffer[OUT] Dump buf
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlExecuteWithWorkBufPriorityProfileBufDumpBuf(uint32_t modelId, const dclmdlDataset *input,
                            dclDataBuffer *workBuf, dclmdlDataset *output, int32_t priority,
                            dclDataBuffer *profileHwBuffer, dclDataBuffer *profileBuffer, dclDataBuffer *dumpBuffer);

/**
 * @ingroup DCL
 * @brief Execute model asynchronous inference until the inference result is returned
 *
 * @param  modelId [IN]   ID of the model to perform inference
 * @param  input [IN]     Input data for model inference
 * @param  output [OUT]   Output data for model inference
 * @param  pNnHandle [OUT]  Handle of inference of this time
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlLoadFromFile | dclmdlLoadFromMem | dclmdlLoadFromFileWithMem |
 * dclmdlLoadFromMemWithMem
 */

DCL_FUNC_VISIBILITY dclError dclmdlExecuteAsync(uint32_t modelId, const dclmdlDataset *input,
                                                dclmdlDataset *output, dclNnHandle* pNnHandle);

/**
 * @ingroup DCL
 * @brief Execute model asynchronous inference until the inference result is returned
 *
 * @param  modelId [IN]   ID of the model to perform inference
 * @param  input [IN]     Input data for model inference
 * @param  output [OUT]   Output data for model inference
 * @param  pNnHandle [OUT]  Handle of inference of this time
 * @param  priority [IN]  model excution priority, 0 - lowpriority, 1 - highpriority
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlLoadFromFile | dclmdlLoadFromMem | dclmdlLoadFromFileWithMem |
 * dclmdlLoadFromMemWithMem
 */

DCL_FUNC_VISIBILITY dclError dclmdlExecuteAsyncWithPriority(uint32_t modelId, const dclmdlDataset *input,
                                                dclmdlDataset *output, dclNnHandle* pNnHandle, int32_t priority);

/**
 * @ingroup DCL
 * @brief Execute model asynchronous inference until the inference result is returned
 *        with user's workbuf/priority/profilerHwBuffer/profilerBuffer/dumpBuffer
 *
 * @param  modelId [IN]   ID of the model to perform inference
 * @param  input [IN]     Input data for model inference
 * @param  workBuf [IN]   Input work buf memrory for inference
 * @param  output [OUT]   Output data for model inference
 * @param  priority[IN]   Inference priority,  0: normal, 1: high
 * @param  profileHwBuffer[OUT] Profile buf for hw profile out
 * @param  profileBuffer[OUT] Profile buf for copy hw profile
 * @param  dumpBuffer[OUT] Dump buf
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlExecuteAsyncWithWorkBufPriorityProfileBufDumpBuf(uint32_t modelId, const dclmdlDataset *input,
                            dclDataBuffer *workBuf, dclmdlDataset *output, dclNnHandle* pNnHandle, int32_t priority,
                            dclDataBuffer *profileHwBuffer, dclDataBuffer *profileBuffer, dclDataBuffer *dumpBuffer);

/**
 * @ingroup DCL
 * @brief Execute model asynchronous inference until the inference result is returned
 *
 * @param  modelId [IN]   ID of the model to perform inference
 * @param  input [IN]     Input data for model inference
 * @param  workBuf [IN]   Input work buf memrory for inference
 * @param  output [OUT]   Output data for model inference
 * @param  pNnHandle [OUT]  Handle of inference of this time
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlLoadFromFile | dclmdlLoadFromMem | dclmdlLoadFromFileWithMem |
 * dclmdlLoadFromMemWithMem
 */
DCL_FUNC_VISIBILITY dclError dclmdlExecuteAsyncWithWorkBuf(uint32_t modelId, const dclmdlDataset *input,
                            dclDataBuffer *workBuf, dclmdlDataset *output, dclNnHandle* pNnHandle);

/**
 * @ingroup DCL
 * @brief Execute model asynchronous inference until the inference result is returned
 *
 * @param  modelId [IN]   ID of the model to perform inference
 * @param  input [IN]   Input data for model inference
 * @param  output [OUT]   Output data for model inference
 * @param  stream [IN]   stream
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlLoadFromFile | dclmdlLoadFromMem | dclmdlLoadFromFileWithMem |
 * dclmdlLoadFromMemWithMem
 */
DCL_FUNC_VISIBILITY dclError dclmdlExecuteAsyncByStream(uint32_t modelId, const dclmdlDataset *input,
    dclmdlDataset *output, dclrtStream stream);


/*
---------------------------------------------------------------------------------------------------
	函数名  : dclmdlExecuteQuery
    功能描述: 查询模型推理执行是否完成
    参数输入: nnHandle 执行模型返回的handle
			  IsBlock 表示是否阻塞，true表示一直查询直至超时或者完成，false表示查询后不管是否执行完成直接返回
	参数输出: pbFinish 是否执行完成，true表示完成，false表示未完成
    函数返回: deNnError，参考定义
    历史记录:
        1. 时间         : 2020-12-06
           修改人       :
---------------------------------------------------------------------------------------------------
*/
DCL_FUNC_VISIBILITY dclError dclmdlExecuteQuery(dclNnHandle nnHandle, bool *pbFinish, bool isBlock);


/**
 * @ingroup DCL
 * @brief unload model with model id
 *
 * @param  modelId [IN]   model id to be unloaded
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlUnload(uint32_t modelId);

/**
 * @ingroup DCL
 * @brief Get the weight memory size and working memory size
 * required for model execution according to the model file
 *
 * @param  fileName [IN]  Model path to get memory information
 * @param  workSize [OUT]  The amount of working memory for model executed
 * @param  weightSize [OUT]  The amount of weight memory for model executed
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlQuerySize(const char *fileName, size_t *workSize, size_t *weightSize);

/**
 * @ingroup DCL
 * @brief Obtain the weights required for
 * model execution according to the model data in memory
 *
 * @par Restriction
 * The execution and weight memory is Device memory,
 * and requires user application and release.
 * @param  model [IN]        model memory which user manages
 * @param  modelSize [IN]    model data size
 * @param  workSize [OUT]    The amount of working memory for model executed
 * @param  weightSize [OUT]  The amount of weight memory for model executed
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlQuerySizeFromMem(const void *model, size_t modelSize, size_t *workSize,
                                                    size_t *weightSize);

/**
 * @ingroup DCL
 * @brief In dynamic batch scenarios,
 * it is used to set the number of images processed
 * at one time during model inference
 *
 * @param  modelId [IN]    model id
 * @param  dataset [IN]    data for model inference
 * @param  index [IN]    index of dynamic tensor
 * @param  batchSize [IN]    Number of images processed at a time during model
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlLoadFromFile | dclmdlLoadFromMem | dclmdlLoadFromFileWithMem |
 * dclmdlLoadFromMemWithMem | dclmdlGetInputIndexByName
 */
DCL_FUNC_VISIBILITY dclError dclmdlSetDynamicBatchSize(uint32_t modelId, dclmdlDataset *dataset, size_t index,
    uint64_t batchSize);

/**
 * @ingroup DCL
 * @brief Sets the H and W of the specified input of the model
 *
 * @param  modelId [IN]    model id
 * @param  dataset [IN]    data for model inference
 * @param  index [IN]    index of dynamic tensor
 * @param  height [IN]    model height
 * @param  width [IN]    model width
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlLoadFromFile | dclmdlLoadFromMem | dclmdlLoadFromFileWithMem |
 * dclmdlLoadFromMemWithMem | dclmdlGetInputIndexByName
 */
DCL_FUNC_VISIBILITY dclError dclmdlSetDynamicHWSize(uint32_t modelId, dclmdlDataset *dataset, size_t index,
    uint64_t height, uint64_t width);

/**
 * @ingroup DCL
 * @brief get input dims info
 *
 * @param modelDesc [IN]  model description
 * @param index [IN]  input tensor index
 * @param dims [OUT]  dims info
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlGetInputDimsV2
 */
DCL_FUNC_VISIBILITY dclError dclmdlGetInputDims(const dclmdlDesc *modelDesc, size_t index, dclmdlIODims *dims);

/**
 * @ingroup DCL
 * @brief get input dims info(version 2), especially for static aipp
 * it is the same with dclmdlGetInputDims while model without static aipp
 *
 * @param modelDesc [IN]  model description
 * @param index [IN]  input tensor index
 * @param dims [OUT]  dims info
 *
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlGetInputDims
 */
DCL_FUNC_VISIBILITY dclError dclmdlGetInputDimsV2(const dclmdlDesc *modelDesc, size_t index, dclmdlIODims *dims);

/**
 * @ingroup DCL
 * @brief get output dims info
 *
 * @param modelDesc [IN] model description
 * @param index [IN]     output tensor index
 * @param dims [OUT]     dims info
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlGetOutputDims(const dclmdlDesc *modelDesc, size_t index, dclmdlIODims *dims);

/**
 * @ingroup DCL
 * @brief get current output dims info
 *
 * @par Function
 * The following use cases are supported:
 * @li Get current output shape when model is dynamic and
 * dynamic shape info is set
 * @li Get max output shape when model is dynamic and
 * dynamic shape info is not set
 * @li Get actual output shape when model is static
 *
 * @param modelDesc [IN]  model description
 * @param index [IN]  output tensor index
 * @param dims [OUT]  dims info
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlGetCurOutputDims(const dclmdlDesc *modelDesc, size_t index, dclmdlIODims *dims);

/**
 * @ingroup DCL
 * @brief get input name by index
 *
 * @param modelDesc [IN]  model description
 * @param index [IN]  intput tensor index
 * @retval input tensor name,the same life cycle with modelDesc
 */
DCL_FUNC_VISIBILITY const char *dclmdlGetInputNameByIndex(const dclmdlDesc *modelDesc, size_t index);

/**
 * @ingroup DCL
 * @brief get output name by index
 *
 * @param modelDesc [IN]  model description
 * @param index [IN]  output tensor index
 * @retval output tensor name,the same life cycle with modelDesc
 */
DCL_FUNC_VISIBILITY const char *dclmdlGetOutputNameByIndex(const dclmdlDesc *modelDesc, size_t index);


/**
 * @ingroup DCL
 * @brief get input format by index
 *
 * @param modelDesc [IN]  model description
 * @param index [IN]      intput tensor index
 *
 * @retval input tensor format
 */
DCL_FUNC_VISIBILITY dclFormat dclmdlGetInputFormat(const dclmdlDesc *modelDesc, size_t index);

/**
 * @ingroup DCL
 * @brief get output format by index
 *
 * @param modelDesc [IN]  model description
 * @param index [IN]      output tensor index
 *
 * @retval output tensor format
 */
DCL_FUNC_VISIBILITY dclFormat dclmdlGetOutputFormat(const dclmdlDesc *modelDesc, size_t index);

/**
 * @ingroup DCL
 * @brief get input data type by index
 *
 * @param modelDesc [IN]  model description
 * @param index [IN]  intput tensor index
 * @retval input tensor data type
 */
DCL_FUNC_VISIBILITY dclDataType dclmdlGetInputDataType(const dclmdlDesc *modelDesc, size_t index);

/**
 * @ingroup DCL
 * @brief get output data type by index
 *
 * @param modelDesc [IN]  model description
 * @param index [IN]  output tensor index
 * @retval output tensor data type
 */
DCL_FUNC_VISIBILITY dclDataType dclmdlGetOutputDataType(const dclmdlDesc *modelDesc, size_t index);

/**
 * @ingroup DCL
 * @brief get input tensor index by name
 *
 * @param modelDesc [IN]  model description
 * @param name [IN]  intput tensor name
 * @param index [OUT]  intput tensor index
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlGetInputIndexByName(const dclmdlDesc *modelDesc, const char *name, size_t *index);

/**
 * @ingroup DCL
 * @brief get output tensor index by name
 *
 * @param modelDesc [IN]  model description
 * @param name [IN]  output tensor name
 * @param index [OUT]  output tensor index
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlGetOutputIndexByName(const dclmdlDesc *modelDesc, const char *name, size_t *index);

/**
 * @ingroup DCL
 * @brief get dynamic batch info
 *
 * @param modelDesc [IN]  model description
 * @param batch [OUT]  dynamic batch info
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlGetDynamicBatch(const dclmdlDesc *modelDesc, dclmdlBatch *batch);

/**
 * @ingroup DCL
 * @brief get dynamic height&width info
 *
 * @param modelDesc [IN]  model description
 * @param index [IN]  input tensor index
 * @param hw [OUT]  dynamic height&width info
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlGetDynamicHW(const dclmdlDesc *modelDesc, size_t index, dclmdlHW *hw);

/**
 * @ingroup DCL
 * @brief get version info
 *
 * @param modelDesc [IN]  model description
 * @param version [OUT]  input tensor index, length must be greater than 16
 * @param len [IN] length of version array
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlGetVersion(const dclmdlDesc *modelDesc, char *version, size_t len);

/**
 * @ingroup DCL
 * @brief get date info
 *
 * @param modelDesc [IN]  model description
 * @param date [OUT]  input tensor index, length must be greater than 16
 * @param len [IN] length of date array
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlGetCompileData(const dclmdlDesc *modelDesc, char *date, size_t len);


/**
 * @ingroup DCL
 * @brief Create data of type dclmdlAIPP
 *
 * @param batchSize [IN]    batchsizes of model
 *
 * @retval the dclmdlAIPP pointer
 */
DCL_FUNC_VISIBILITY dclmdlAIPP *dclmdlCreateAIPP();

/**
 * @ingroup DCL
 * @brief Create data of type dclmdlAIPP
 *
 * @param batchSize [IN]    batchsizes of model
 * @retval the dclmdlAIPP pointer
 */
DCL_FUNC_VISIBILITY dclmdlAIPP *dclmdlCreateAIPPBySize(uint64_t batchSize);

/**
 * @ingroup DCL
 * @brief destroy data of type dclmdlAIPP
 *
 * @param aippParmsSet [IN]    Pointer for dclmdlAIPP to be destroyed
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmdlDestroyAIPP(const dclmdlAIPP *aippParmsSet);

/**
 * @ingroup DCL
 * @brief set InputFormat of type dclmdlAIPP
 *
 * @param aippParmsSet [OUT]  Pointer for dclmdlAIPP
 * @param inputFormat [IN]    The inputFormat of aipp
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlCreateAIPP
 */
DCL_FUNC_VISIBILITY dclError dclmdlSetAIPPInputFormat(dclmdlAIPP *aippParmsSet, dclAippInputFormat inputFormat);

/**
 * @ingroup DCL
 * @brief set cscParms of type dclmdlAIPP
 *
 * @param aippParmsSet [IN]     Pointer for dclmdlAIPP
 * @param csc_switch [IN]       Csc switch
 * @param cscMatrixR0C0 [IN]    Csc_matrix_r0_c0
 * @param cscMatrixR0C1 [IN]    Csc_matrix_r0_c1
 * @param cscMatrixR0C2 [IN]    Csc_matrix_r0_c2
 * @param cscMatrixR1C0 [IN]    Csc_matrix_r1_c0
 * @param cscMatrixR1C1 [IN]    Csc_matrix_r1_c1
 * @param cscMatrixR1C2 [IN]    Csc_matrix_r1_c2
 * @param cscMatrixR2C0 [IN]    Csc_matrix_r2_c0
 * @param cscMatrixR2C1 [IN]    Csc_matrix_r2_c1
 * @param cscMatrixR2C2 [IN]    Csc_matrix_r2_c2
 * @param cscOutputBiasR0 [IN]  Output Bias for RGB to YUV, element of row 0, unsigned number
 * @param cscOutputBiasR1 [IN]  Output Bias for RGB to YUV, element of row 1, unsigned number
 * @param cscOutputBiasR2 [IN]  Output Bias for RGB to YUV, element of row 2, unsigned number
 * @param cscInputBiasR0 [IN]   Input Bias for YUV to RGB, element of row 0, unsigned number
 * @param cscInputBiasR1 [IN]   Input Bias for YUV to RGB, element of row 1, unsigned number
 * @param cscInputBiasR2 [IN]   Input Bias for YUV to RGB, element of row 2, unsigned number
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlCreateAIPP
*/
DCL_FUNC_VISIBILITY dclError dclmdlSetAIPPCscParams(dclmdlAIPP *aippParmsSet, int8_t csc_switch,
                                                    int16_t cscMatrixR0C0, int16_t cscMatrixR0C1, int16_t cscMatrixR0C2,
                                                    int16_t cscMatrixR1C0, int16_t cscMatrixR1C1, int16_t cscMatrixR1C2,
                                                    int16_t cscMatrixR2C0, int16_t cscMatrixR2C1, int16_t cscMatrixR2C2,
                                                    uint8_t cscOutputBiasR0, uint8_t cscOutputBiasR1,
                                                    uint8_t cscOutputBiasR2, uint8_t cscInputBiasR0,
                                                    uint8_t cscInputBiasR1, uint8_t cscInputBiasR2);

/**
 * @ingroup DCL
 * @brief set rb/ub swap switch of type dclmdlAIPP
 *
 * @param aippParmsSet [IN]   Pointer for dclmdlAIPP
 * @param rbuvSwapSwitch [IN] rb/ub swap switch
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlCreateAIPP
*/
DCL_FUNC_VISIBILITY dclError dclmdlSetAIPPRbuvSwapSwitch(dclmdlAIPP *aippParmsSet, int8_t rbuvSwapSwitch);

/**
 * @ingroup DCL
 * @brief set RGBA->ARGB, YUVA->AYUV swap switch of type dclmdlAIPP
 *
 * @param aippParmsSet [IN]   Pointer for dclmdlAIPP
 * @param axSwapSwitch [IN]   RGBA->ARGB, YUVA->AYUV swap switch
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlCreateAIPP
*/
DCL_FUNC_VISIBILITY dclError dclmdlSetAIPPAxSwapSwitch(dclmdlAIPP *aippParmsSet, int8_t axSwapSwitch);

/**
 * @ingroup DCL
 * @brief set source image of type dclmdlAIPP
 *
 * @param aippParmsSet [OUT]  Pointer for dclmdlAIPP
 * @param srcImageSizeW [IN]  Source image width
 * @param srcImageSizeH [IN]  Source image height
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlCreateAIPP
*/
DCL_FUNC_VISIBILITY dclError dclmdlSetAIPPSrcImageSize(dclmdlAIPP *aippParmsSet, int32_t srcImageSizeW,
                                                       int32_t srcImageSizeH);

/**
 * @ingroup DCL
 * @brief set resize switch of type dclmdlAIPP
 *
 * @param aippParmsSet [IN]   Pointer for dclmdlAIPP
 * @param scfSwitch [IN]      Resize switch
 * @param scfInputSizeW [IN]  Input width of scf
 * @param scfInputSizeH [IN]  Input height of scf
 * @param scfOutputSizeW [IN] Output width of scf
 * @param scfOutputSizeH [IN] Output height of scf
 * @param batchIndex [IN]     Batch parameter index
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlCreateAIPP
*/
DCL_FUNC_VISIBILITY dclError dclmdlSetAIPPScfParams(dclmdlAIPP *aippParmsSet,
                                                    int8_t scfSwitch,
                                                    int32_t scfInputSizeW,
                                                    int32_t scfInputSizeH,
                                                    int32_t scfOutputSizeW,
                                                    int32_t scfOutputSizeH,
                                                    uint64_t batchIndex);

/**
 * @ingroup DCL
 * @brief set cropParams of type dclmdlAIPP
 *
 * @param aippParmsSet [OUT]  Pointer for dclmdlAIPP
 * @param cropSwitch [IN]     Crop switch
 * @param cropStartPosW [IN]  The start horizontal position of cropping
 * @param cropStartPosH [IN]  The start vertical position of cropping
 * @param cropSizeW [IN]      Crop width
 * @param cropSizeH [IN]      Crop height
 * @param batchIndex [IN]     Batch parameter index
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlCreateAIPP
*/
DCL_FUNC_VISIBILITY dclError dclmdlSetAIPPCropParams(dclmdlAIPP *aippParmsSet,
                                                     int8_t cropSwitch,
                                                     int32_t cropStartPosW,
                                                     int32_t cropStartPosH,
                                                     int32_t cropSizeW,
                                                     int32_t cropSizeH);

/**
 * @ingroup DCL
 * @brief set cropParams of type dclmdlAIPP
 *
 * @param aippParmsSet [IN]   Pointer for dclmdlAIPP
 * @param cropSwitch [IN]     Crop switch
 * @param cropStartPosW [IN]  The start horizontal position of cropping
 * @param cropStartPosH [IN]  The start vertical position of cropping
 * @param cropSizeW [IN]      Crop width
 * @param cropSizeH [IN]      Crop height
 * @param batchIndex [IN]     Batch parameter index
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlCreateAIPP
*/
DCL_FUNC_VISIBILITY dclError dclmdlSetAIPPCropParamsByIndex(dclmdlAIPP *aippParmsSet,
                                                     int8_t cropSwitch,
                                                     int32_t cropStartPosW,
                                                     int32_t cropStartPosH,
                                                     int32_t cropSizeW,
                                                     int32_t cropSizeH,
                                                     uint64_t batchIndex);

/**
 * @ingroup DCL
 * @brief set paddingParams of type dclmdlAIPP
 *
 * @param aippParmsSet [IN]       Pointer for dclmdlAIPP
 * @param paddingSwitch [IN]      Padding switch
 * @param paddingSizeTop [IN]     Top padding size
 * @param paddingSizeBottom [IN]  Bottom padding size
 * @param paddingSizeLeft [IN]    Left padding size
 * @param paddingSizeRight [IN]   Right padding size
 * @param batchIndex [IN]         Batch parameter index
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlCreateAIPP
*/
DCL_FUNC_VISIBILITY dclError dclmdlSetAIPPPaddingParams(dclmdlAIPP *aippParmsSet, int8_t paddingSwitch,
                                                        int32_t paddingSizeTop, int32_t paddingSizeBottom,
                                                        int32_t paddingSizeLeft, int32_t paddingSizeRight,
                                                        uint64_t batchIndex);

/**
 * @ingroup DCL
 * @brief set DtcPixelMean of type dclmdlAIPP
 *
 * @param aippParmsSet [IN]       Pointer for dclmdlAIPP
 * @param dtcPixelMeanChn0 [IN]   Mean value of channel 0
 * @param dtcPixelMeanChn1 [IN]   Mean value of channel 1
 * @param dtcPixelMeanChn2 [IN]   Mean value of channel 2
 * @param dtcPixelMeanChn3 [IN]   Mean value of channel 3
 * @param batchIndex [IN]         Batch parameter index
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlCreateAIPP
*/
DCL_FUNC_VISIBILITY dclError dclmdlSetAIPPDtcPixelMean(dclmdlAIPP *aippParmsSet,
                                                       int16_t dtcPixelMeanChn0,
                                                       int16_t dtcPixelMeanChn1,
                                                       int16_t dtcPixelMeanChn2,
                                                       int16_t dtcPixelMeanChn3,
                                                       uint64_t batchIndex);

/**
 * @ingroup DCL
 * @brief set DtcPixelMin of type dclmdlAIPP
 *
 * @param aippParmsSet [IN]     Pointer for dclmdlAIPP
 * @param dtcPixelMinChn0 [IN]  Min value of channel 0
 * @param dtcPixelMinChn1 [IN]  Min value of channel 1
 * @param dtcPixelMinChn2 [IN]  Min value of channel 2
 * @param dtcPixelMinChn3 [IN]  Min value of channel 3
 * @param batchIndex [IN]       Batch parameter index
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlCreateAIPP
*/
DCL_FUNC_VISIBILITY dclError dclmdlSetAIPPDtcPixelMin(dclmdlAIPP *aippParmsSet,
                                                      float dtcPixelMinChn0,
                                                      float dtcPixelMinChn1,
                                                      float dtcPixelMinChn2,
                                                      float dtcPixelMinChn3,
                                                      uint64_t batchIndex);

/**
 * @ingroup DCL
 * @brief set PixelVarReci of type dclmdlAIPP
 *
 * @param aippParmsSet [IN]        Pointer for dclmdlAIPP
 * @param dtcPixelVarReciChn0 [IN] sfr_dtc_pixel_variance_reci_ch0
 * @param dtcPixelVarReciChn1 [IN] sfr_dtc_pixel_variance_reci_ch1
 * @param dtcPixelVarReciChn2 [IN] sfr_dtc_pixel_variance_reci_ch2
 * @param dtcPixelVarReciChn3 [IN] sfr_dtc_pixel_variance_reci_ch3
 * @param batchIndex [IN]          Batch parameter index
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlCreateAIPP
*/
DCL_FUNC_VISIBILITY dclError dclmdlSetAIPPPixelVarReci(dclmdlAIPP *aippParmsSet,
                                                       float dtcPixelVarReciChn0,
                                                       float dtcPixelVarReciChn1,
                                                       float dtcPixelVarReciChn2,
                                                       float dtcPixelVarReciChn3,
                                                       uint64_t batchIndex);

/**
 * @ingroup DCL
 * @brief set aipp parameters to model
 *
 * @param modelId [IN]        model id
 * @param dataset [IN]        Pointer of dataset
 * @param index [IN]          index of input for aipp data(DCL_DYNAMIC_AIPP_NODE)
 * @param aippParmsSet [IN]   Pointer for dclmdlAIPP
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlLoadFromFile | dclmdlLoadFromMem | dclmdlLoadFromFileWithMem |
 * dclmdlLoadFromMemWithMem | dclmdlGetInputIndexByName | dclmdlCreateAIPP
*/
DCL_FUNC_VISIBILITY dclError dclmdlSetInputAIPP(uint32_t modelId,
                                                dclmdlDataset *dataset,
                                                size_t index,
                                                const dclmdlAIPP *aippParmsSet);

/**
 * @ingroup DCL
 * @brief set aipp parameters to model
 *
 * @param modelId [IN]        model id
 * @param dataset [IN]        Pointer of dataset
 * @param index [IN]          index of input for data which linked dynamic aipp(DCL_DATA_WITH_DYNAMIC_AIPP)
 * @param aippParmsSet [IN]   Pointer for dclmdlAIPP
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlLoadFromFile | dclmdlLoadFromMem | dclmdlLoadFromFileWithMem |
 * dclmdlLoadFromMemWithMem | dclmdlGetInputIndexByName | dclmdlCreateAIPP
*/
DCL_FUNC_VISIBILITY dclError dclmdlSetAIPPByInputIndex(uint32_t modelId,
                                                       dclmdlDataset *dataset,
                                                       size_t index,
                                                       const dclmdlAIPP *aippParmsSet);

/**
 * @ingroup DCL
 * @brief get input aipp type
 *
 * @param modelId [IN]        model id
 * @param index [IN]          index of input
 * @param type [OUT]          aipp type for input.refrer to dclmdlInputAippType(enum)
 * @param dynamicAttachedDataIndex [OUT]     index for dynamic attached data(DCL_DYNAMIC_AIPP_NODE)
 *        valid when type is DCL_DATA_WITH_DYNAMIC_AIPP, invalid value is DCL_INVALID_NODE_INDEX
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlLoadFromFile | dclmdlLoadFromMem | dclmdlLoadFromFileWithMem |
 * dclmdlLoadFromMemWithMem | dclmdlGetInputIndexByName | dclmdlCreateAIPP
*/
DCL_FUNC_VISIBILITY dclError dclmdlGetAippType(uint32_t modelId,
                                               size_t index,
                                               dclmdlInputAippType *type,
                                               size_t *dynamicAttachedDataIndex);

/**
 * @ingroup DCL
 * @brief get static aipp parameters from model
 *
 * @param modelId [IN]        model id
 * @param index [IN]          index of tensor
 * @param aippinfo [OUT]      Pointer for static aipp info
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval DCL_ERROR_MODEL_AIPP_NOT_EXIST The tensor of index is not configured with aipp
 * @retval OtherValues Failure
 *
 * @see dclmdlLoadFromFile | dclmdlLoadFromMem | dclmdlLoadFromFileWithMem |
 * dclmdlLoadFromMemWithMem | dclmdlGetInputIndexByName
*/
DCL_FUNC_VISIBILITY dclError dclmdlGetFirstAippInfo(uint32_t modelId, size_t index, dclAippInfo *aippinfo);


/**
 * @ingroup DCL
 * @brief disable aipp operation for model with aipp when dclmdlExecute/dclmdlExecuteAsync;
 *        only for using before first dclmdlExecute/dclmdlExecuteAsync;
 *        suggest using after model loaded at once if need
 *
 * @param modelId [IN]        model id
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmdlLoadFromFile | dclmdlLoadFromMem | dclmdlLoadFromFileWithMem |
 * dclmdlLoadFromMemWithMem
*/
DCL_FUNC_VISIBILITY dclError dclmdlSetAippDisable(uint32_t modelId);


/**
 * @ingroup DCL
 * @brief 海康接口扩展：获取模型profile时硬件执行需要缓存大小，byte为单位
 *
 * @param modelId [IN]        model id
 * @param buffSize [OUT]      缓存大小 ，byte为单位
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
*/
DCL_FUNC_VISIBILITY dclError dclmdlGetProfileHwBuffSize(uint32_t modelId, uint32_t *buffSize);

/*!
* @brief 查询NPU能力信息
* @param [in] id, npu id信息，当前版本不适用
* @param [out] capacity npu能力信息
* 返回值：
    0：成功，其它值：错误码
*/
DCL_FUNC_VISIBILITY int32_t dclmdlQueryCapacity(
                            int32_t devId,
                            dclNpuCapacityInfo *capacity);

/**
 * @brief  海康接口扩展：获取模型下发任务的执行情况
 * @param[in]
 * @return              int32_t        0: 正常； 其他失败
*/
DCL_FUNC_VISIBILITY dclError dclmdlQueryTaskStatistics(dclmdlTaskStatistic *statis);

/**
 * @ingroup DCL
 * @brief 海康接口扩展：获取模型的fused_op_num，包括nnp、cpu
 *
 * @param modelId [IN]        model id
 * @param opNum [OUT]         融合算子数目
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
*/
DCL_FUNC_VISIBILITY dclError dclmdlGetOpNum(uint32_t modelId, uint32_t *opNum);

/**
 * @ingroup DCL
 * @brief 海康接口扩展：获取模型指定op的信息
 *
 * @param modelId [IN]        model id
 * @param opListNum [IN]      待获取op数目，不超过dclmdlGetOpNum()返回的数目
 * @param opList [OUT]        获取的op信息，每个opList对应 base.h中TY_OpDesc， ！！！必须保持对齐
 * @param validOpNum [OUT]    获取的有效op信息数目
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
*/
DCL_FUNC_VISIBILITY dclError dclmdlGetOpList(uint32_t modelId, uint32_t opListNum, void *opList, uint32_t *validOpNum);

/**
 * @ingroup DCL
 * @brief 接口扩展：设置npu调度率信息（类似cpu占用率）
 *
 * @param peroid [IN]     设置npu设备的调度率统计周期, 单位：s，推荐配置：1s
 * @param devId [IN]      多个npu设备时某个dev的id，
 *                        正常值范围[0, maxDeviceCount)，缺省： 0
 *                        如果设置 -1， 表示默认设置所有当前open的device设备
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
*/
DCL_FUNC_VISIBILITY dclError dclNpuSetSchRate(uint32_t peroid, int32_t devId);


typedef struct{
    int32_t  devId;          /* 设备ID,取值范围[0, maxDeviceCount)*/
    float    schedRate;      /* 最近一次统计周期内的调度率，取值范围[0,1.0] */
    uint64_t startTimeStamp; /* 最近一次统计周期的起始时间戳 */
}dclNpuSchRate;

/**
 * @ingroup DCL
 * @brief 接口扩展：获取npu设备调度率信息，支持多设备扩展
 *
 * @param devNum [OUT]   获取npu设备的个数
 * @param rate   [OUT]   获取npu设备的调度率信息，数组形式，有效个数未devNum
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
*/
DCL_FUNC_VISIBILITY dclError dclNpuGetSchRate(int32_t* devNum, dclNpuSchRate rate[]);

#ifdef __cplusplus
}
#endif

#endif // INC_EXTERNAL_DCL_DCL_MODEL_H_
