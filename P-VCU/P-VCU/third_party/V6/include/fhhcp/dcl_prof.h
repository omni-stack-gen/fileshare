/**
* @file dcl_prof.h
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef INC_EXTERNAL_DCL_PROF_H_
#define INC_EXTERNAL_DCL_PROF_H_

#include "dcl_base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DCL_PROF_DCL_API                0x0001
#define DCL_PROF_TASK_TIME              0x0002
//#define DCL_PROF_AICORE_METRICS         0x0004

/**
 * @deprecated please use DclprofGetOpTypeLen and DclprofGetOpTNameLen instead
 */
#define DCL_PROF_MAX_OP_NAME_LEN        257
#define DCL_PROF_MAX_OP_TYPE_LEN        65

typedef enum {
    DCL_AICORE_ARITHMETIC_UTILIZATION = 0,
    //DCL_AICORE_PIPE_UTILIZATION = 1,
    //DCL_AICORE_MEMORY_BANDWIDTH = 2,
    //DCL_AICORE_L0B_AND_WIDTH = 3,
    //DCL_AICORE_RESOURCE_CONFLICT_RATIO = 4,
    DCL_AICORE_NONE = 0xFF
} dclprofAicoreMetrics;

typedef struct dclprofConfig dclprofConfig;
typedef struct dclprofStopConfig dclprofStopConfig;
typedef struct dclprofAicoreEvents dclprofAicoreEvents;
typedef struct dclprofSubscribeConfig dclprofSubscribeConfig;


/**
 * @ingroup DCL
 * @brief profiling initialize
 *
 * @param  profilerResultPath [IN]  path of profiling result
 * @param  length [IN]              length of profilerResultPath
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see DclprofFinalize
 */
DCL_FUNC_VISIBILITY dclError dclprofInit(const char *profilerResultPath, size_t length);

/**
 * @ingroup DCL
 * @brief profiling finalize
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see DclprofInit
 */
DCL_FUNC_VISIBILITY dclError dclprofFinalize();

/**
 * @ingroup DCL
 * @brief Start profiling modules by profilerConfig
 *
 * @param  profilerConfig [IN]  config of profiling
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see DclprofStop
 */
DCL_FUNC_VISIBILITY dclError dclprofStart(const dclprofConfig *profilerConfig);

/**
 * @ingroup DCL
 * @brief Create data of type DclprofConfig
 *
 * @param  deviceIdList [IN]      list of device id
 * @param  deviceNums [IN]        number of devices
 * @param  aicoreMetrics [IN]     type of aicore metrics
 * @param  aicoreEvents [IN]      pointer to aicore events, only support NULL now
 * @param  dataTypeConfig [IN]    config modules need profiling
 *
 * @retval the DclprofConfig pointer
 *
 * @see DclprofDestroyConfig
 */
DCL_FUNC_VISIBILITY dclprofConfig *dclprofCreateConfig(uint32_t *deviceIdList, uint32_t deviceNums,
    dclprofAicoreMetrics aicoreMetrics, dclprofAicoreEvents *aicoreEvents, uint64_t dataTypeConfig);

/**
 * @ingroup DCL
 * @brief Destroy data of type DclprofConfig
 *
 * @param  profilerConfig [IN]  config of profiling
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see DclprofCreateConfig
 */
DCL_FUNC_VISIBILITY dclError dclprofDestroyConfig(const dclprofConfig *profilerConfig);

/**
 * @ingroup DCL
 * @brief stop profiling modules by stopProfilingConfig
 *
 * @param  profilerConfig [IN]  pointer to stop config of profiling
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see DclprofStart
 */
DCL_FUNC_VISIBILITY dclError dclprofStop(const dclprofConfig *profilerConfig);


/**
 * @ingroup DCL
 * @brief subscribe profiling data of model
 *
 * @param  modelId [IN]              the model id subscribed
 * @param  profSubscribeConfig [IN]  pointer to config of model subscribe
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclprofModelUnSubscribe
 */
DCL_FUNC_VISIBILITY dclError dclprofModelSubscribe(uint32_t modelId,
    const dclprofSubscribeConfig *profSubscribeConfig);

/**
 * @ingroup DCL
 * @brief unsubscribe profiling data of model
 *
 * @param  modelId [IN]  the model id unsubscribed
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclprofModelSubscribe
 */
DCL_FUNC_VISIBILITY dclError dclprofModelUnSubscribe(uint32_t modelId);

/**
 * @ingroup DCL
 * @brief create subscribe config
 *
 * @param  timeInfoSwitch [IN] switch whether get time info from model
 * @param  aicoreMetrics [IN]  aicore metrics
 * @param  fd [IN]             pointer to write pipe
 *
 * @retval the dclprofSubscribeConfig pointer
 *
 * @see dclprofDestroySubscribeConfig
 */
DCL_FUNC_VISIBILITY dclprofSubscribeConfig *dclprofCreateSubscribeConfig(int8_t timeInfoSwitch,
    dclprofAicoreMetrics aicoreMetrics, void *fd);

/**
 * @ingroup DCL
 * @brief destroy subscribe config
 *
 * @param  profSubscribeConfig [IN]  subscribe config
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see DclprofCreateSubscribeConfig
 */
DCL_FUNC_VISIBILITY dclError dclprofDestroySubscribeConfig(const dclprofSubscribeConfig *profSubscribeConfig);

/**
 * @ingroup DCL
 * @brief create subscribe config
 *
 * @param  opDescSize [OUT]  size of op desc
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclprofGetOpDescSize(size_t *opDescSize);

/**
 * @ingroup DCL
 * @brief get op number from subscription data
 *
 * @param  opInfo [IN]     pointer to subscription data
 * @param  opInfoLen [IN]  memory size of subscription data
 * @param  opNumber [OUT]  op number of subscription data
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclprofGetOpNum(const void *opInfo, size_t opInfoLen, uint32_t *opNumber);

/**
 * @ingroup DCL
 * @brief get length op type from subscription data
 *
 * @param  opInfo [IN]      pointer to subscription data
 * @param  opInfoLen [IN]   memory size of subscription data
 * @param  index [IN]       index of op array in opInfo
 * @param  opTypeLen [OUT]  actual length of op type string
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclprofGetOpTypeLen(const void *opInfo, size_t opInfoLen, uint32_t index,
    size_t *opTypeLen);

/**
 * @ingroup DCL
 * @brief get op type from subscription data
 *
 * @param  opInfo [IN]      pointer to subscription data
 * @param  opInfoLen [IN]   memory size of subscription data
 * @param  index [IN]       index of op array in opInfo
 * @param  opType [OUT]     obtained op type string
 * @param  opTypeLen [IN]   obtained length of op type string
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclprofGetOpType(const void *opInfo, size_t opInfoLen, uint32_t index,
    char *opType, size_t opTypeLen);

/**
 * @ingroup DCL
 * @brief get length op name from subscription data
 *
 * @param  opInfo [IN]      pointer to subscription data
 * @param  opInfoLen [IN]   memory size of subscription data
 * @param  index [IN]       index of op array in opInfo
 * @param  opNameLen [OUT]  actual length of op name string
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclprofGetOpNameLen(const void *opInfo, size_t opInfoLen, uint32_t index,
    size_t *opNameLen);

/**
 * @ingroup DCL
 * @brief get op type from subscription data
 *
 * @param  opInfo [IN]      pointer to subscription data
 * @param  opInfoLen [IN]   memory size of subscription data
 * @param  index [IN]       index of op array in opInfo
 * @param  opName [OUT]     obtained op name string
 * @param  opNameLen [IN]   obtained length of op name string
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclprofGetOpName(const void *opInfo, size_t opInfoLen, uint32_t index,
    char *opName, size_t opNameLen);

/**
 * @ingroup DCL
 * @brief get start time of specified op from subscription data
 *
 * @param  opInfo [IN]     pointer to subscription data
 * @param  opInfoLen [IN]  memory size of subscription data
 * @param  index [IN]      index of op array in opInfo
 *
 * @retval start time(us) of specified op with timestamp
 * @retval 0 for failed
 */
DCL_FUNC_VISIBILITY uint64_t dclprofGetOpStart(const void *opInfo, size_t opInfoLen, uint32_t index);

/**
 * @ingroup DCL
 * @brief get end time of specified op from subscription data
 *
 * @param  opInfo [IN]     pointer to subscription data
 * @param  opInfoLen [IN]  memory size of subscription data
 * @param  index [IN]      index of op array in opInfo
 *
 * @retval end time(us) of specified op with timestamp
 * @retval 0 for failed
 */
DCL_FUNC_VISIBILITY uint64_t dclprofGetOpEnd(const void *opInfo, size_t opInfoLen, uint32_t index);

/**
 * @ingroup DCL
 * @brief get excution time of specified op from subscription data
 *
 * @param  opInfo [IN]     pointer to subscription data
 * @param  opInfoLen [IN]  memory size of subscription data
 * @param  index [IN]      index of op array in opInfo
 *
 * @retval execution time(us) of specified op with timestamp
 * @retval 0 for failed
 */
DCL_FUNC_VISIBILITY uint64_t dclprofGetOpDuration(const void *opInfo, size_t opInfoLen, uint32_t index);

/**
 * @ingroup DCL
 * @brief get model id from subscription data
 *
 * @param  opInfo [IN]     pointer to subscription data
 * @param  opInfoLen [IN]  memory size of subscription data
 *
 * @retval model id of subscription data
 * @retval 0 for failed
 */
DCL_FUNC_VISIBILITY size_t dclprofGetModelId(const void *opInfo, size_t opInfoLen, uint32_t index);



/**
 * @ingroup DCL
 * @brief 单次推理profiler长度
 *
 * @param  descSize [OUT]  size of interference profile desc
 *
 * @retval  DCL_SUCCESS The function is successfully executed.
 * @retval  Others FAIL
 */
DCL_FUNC_VISIBILITY dclError dclprofGetInferenceDescSize(size_t *descSize);

/**
 * @ingroup DCL
 * @brief subscription data中包含的推理profiler次数
 *
 * @param  inferenceInfo[IN]      subscription data
 * @param  inferenceInfoLen[IN]   subscription data长度
 * @param  inferenceNumber [OUT]  推理次数
 *
 * @retval  DCL_SUCCESS The function is successfully executed.
 * @retval  Others FAIL
 */
DCL_FUNC_VISIBILITY dclError dclprofGetInferenceNum(const void *inferenceInfo, size_t inferenceInfoLen, uint32_t *inferenceNumber);
/**
 * @ingroup DCL
 * @brief subscription data中某次推理profiler对应的模型ID
 *
 * @param  inferenceInfo[IN]      subscription data
 * @param  inferenceInfoLen[IN]   subscription data长度
 * @param  inferenceIndex[IN]     subscription data中推理序号
 * @param  modelId [OUT]          
 * 
 * @retval  DCL_SUCCESS The function is successfully executed.
 * @retval  Others FAIL
 */
DCL_FUNC_VISIBILITY dclError dclprofGetInferenceModelId(const void *inferenceInfo, size_t inferenceInfoLen, uint32_t inferenceIndex, uint32_t *modelId);

/**
 * @ingroup DCL
 * @brief subscription data中某次推理profiler对应的下发推理接口的编号
 *
 * @param  inferenceInfo[IN]      subscription data
 * @param  inferenceInfoLen[IN]   subscription data长度
 * @param  inferenceIndex[IN]     subscription data中推理序号
 * @param  iterId [OUT]           modelId对应的推理接口序号
 * 
 * @retval  DCL_SUCCESS The function is successfully executed.
 * @retval  Others FAIL
 */
DCL_FUNC_VISIBILITY dclError dclprofGetInferenceIterId(const void *inferenceInfo, size_t inferenceInfoLen, uint32_t inferenceIndex, uint32_t *iterId);

/**
 * @ingroup DCL
 * @brief subscription data中某次推理的起始时间，单位为纳秒
 *
 * @param  inferenceInfo[IN]      subscription data
 * @param  inferenceInfoLen[IN]   subscription data长度
 * @param  inferenceIndex[IN]     subscription data中推理序号      
 * 
 * @retval  0 invalid value
 * @retval  
 */
DCL_FUNC_VISIBILITY uint64_t dclprofGetInferenceStart(const void *inferenceInfo, size_t inferenceInfoLen, uint32_t inferenceIndex);

/**
 * @ingroup DCL
 * @brief subscription data中某次推理的结束时间，单位为纳秒
 *
 * @param  inferenceInfo[IN]      subscription data
 * @param  inferenceInfoLen[IN]   subscription data长度
 * @param  inferenceIndex[IN]     subscription data中推理序号
 * @param  duration [OUT]         
 * 
 * @retval  
 * @retval  
 */
DCL_FUNC_VISIBILITY uint64_t dclprofGetInferenceEnd(const void *inferenceInfo, size_t inferenceInfoLen, uint32_t inferenceIndex);

/**
 * @ingroup DCL
 * @brief subscription data中某次推理对应的block数目
 *
 * @param  inferenceInfo[IN]      subscription data
 * @param  inferenceInfoLen[IN]   subscription data长度
 * @param  inferenceIndex[IN]     subscription data中推理序号
 * @param  blockNumber [OUT]      模型对应的block数目
 * 
 * @retval  DCL_SUCCESS The function is successfully executed.
 * @retval  Others FAIL
 */
DCL_FUNC_VISIBILITY dclError dclprofGetBlockNum(const void *inferenceInfo, size_t inferenceInfoLen, uint32_t inferenceIndex, uint32_t *blockNumber);

/**
 * @ingroup DCL
 * @brief subscription data中某次推理block的NPU耗时,如果是cpu block则无意义
 *
 * @param  inferenceInfo[IN]      subscription data
 * @param  inferenceInfoLen[IN]   subscription data长度
 * @param  inferenceIndex[IN]     subscription data中推理序号
 * @param  blockIndex [IN]]      模型对应的block数目
 * 
 * @retval  
 * @retval  
 */
DCL_FUNC_VISIBILITY uint64_t dclprofGetBlockCycles(const void *inferenceInfo, size_t inferenceInfoLen, uint32_t inferenceIndex, uint32_t blockIndex);

/**
 * @ingroup DCL
 * @brief subscription data中某次推理block的起始时间，单位为纳秒
 *
 * @param  inferenceInfo[IN]      subscription data
 * @param  inferenceInfoLen[IN]   subscription data长度
 * @param  inferenceIndex[IN]     subscription data中推理序号
 * @param  blockIndex [IN]]      模型对应的block数目
 * 
 * @retval  
 * @retval  
 */
DCL_FUNC_VISIBILITY uint64_t dclprofGetBlockStart(const void *inferenceInfo, size_t inferenceInfoLen, uint32_t inferenceIndex, uint32_t blockIndex);

/**
 * @ingroup DCL
 * @brief subscription data中某次推理block的结束时间，单位为纳秒
 *
 * @param  inferenceInfo[IN]      subscription data
 * @param  inferenceInfoLen[IN]   subscription data长度
 * @param  inferenceIndex[IN]     subscription data中推理序号
 * @param  blockIndex [IN]]      模型对应的block数目
 * 
 * @retval  
 * @retval  
 */
DCL_FUNC_VISIBILITY uint64_t dclprofGetBlockEnd(const void *inferenceInfo, size_t inferenceInfoLen, uint32_t inferenceIndex, uint32_t blockIndex);

#ifdef __cplusplus
}
#endif

#endif // INC_EXTERNAL_DCL_PROF_H_
