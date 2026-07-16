#ifndef TY_NPU_H_
#define TY_NPU_H_

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)

#if (defined DP2000_300 || defined CPU_SIMU_300)
#define TY_NU_MAX_FREQ     792
#elif (defined DP2000_320 || defined CPU_SIMU_320)
    #if (defined DP2000_310 || defined CPU_SIMU_310)
        #define TY_NU_MAX_FREQ 743.1
    #elif (defined DP2000_315 || defined CPU_SIMU_315)
        #define TY_NU_MAX_FREQ 768
    #else
        #define TY_NU_MAX_FREQ 768
    #endif
#endif

#define TY_VU_MAX_FREQ     (TY_NU_MAX_FREQ/2)

#define TY_NPU_MODEL_VERSION_LEN          64
#define TY_NPU_MODEL_COMPILE_DATE_LEN     32
#define TY_NPU_NU_SUPPORT_DATA_FORMAT_LEN 256
#define TY_NPU_VU_SUPPORT_DATA_FORMAT_LEN 256
#define TY_NPU_PROFILE_MEM_THROUGHPUT_TYPE_NUM 8

#define TY_NPU_PROFILE_UNVALID          -1

typedef struct TY_ModelIODesc{
    uint32_t inputNum;
    uint32_t outputNum;
    T_TY_BlobDesc in[TY_NPU_MODLE_BLOB_MAX_INPUT_NUM];
    T_TY_BlobDesc out[TY_NPU_MODLE_BLOB_MAX_OUTPUT_NUM];
}T_TY_ModelIODesc;

typedef struct TY_ModelCommonInfo{
    char version[TY_NPU_MODEL_VERSION_LEN];
    char compileDate[TY_NPU_MODEL_COMPILE_DATE_LEN];
}T_TY_ModelCommonInfo;

typedef struct TY_ModelDesc{
    T_TY_ModelIODesc ioDesc;
    T_TY_ModelCommonInfo info;
}T_TY_ModelDesc;

typedef struct TY_ModelCfgParam{
    int32_t reserve;
}T_TY_ModelCfgParam;

typedef struct TY_TaskCfgParam{
    int32_t batchNum;
    int32_t reserve;
}T_TY_TaskCfgParam;

typedef struct TY_TaskInput{
    T_TY_Mem  dataIn;
    T_TY_BlobDesc descIn;//当T_ModelDesc.in[i].type==E_BLOB_IMAGE_WITH_PRE_PROC时，需要用户根据输入图像填写此参数，
                        //用户根据输入图像赋值dataIn.addr和size
}T_TY_TaskInput;

typedef struct TY_TaskOutput{
    T_TY_Mem dataOut;
}T_TY_TaskOutput;

typedef struct TY_OpWeightInfo{
    int32_t reserved;
}T_TY_OpWeightInfo;

typedef struct TY_OpQuaInfo{
    int32_t reserved;
}T_TY_OpWQuaInfo;

typedef struct TY_OpDesc{
    char name[TY_NPU_OPTYPE_STRING_MAX_LEN];
    int32_t id;
    int32_t numInputs;
    int32_t numOutputs;
    T_TY_BlobDesc inputDesc[TY_NPU_MODLE_BLOB_MAX_INPUT_NUM];
    T_TY_BlobDesc outputDesc[TY_NPU_MODLE_BLOB_MAX_OUTPUT_NUM];
    T_TY_OpWeightInfo weightInfo;  /* op 参数信息 */
    T_TY_OpWQuaInfo   quaInfo;     /* op 量化信息 */
}T_TY_OpDesc;

typedef struct TY_Profile{
    E_TY_EngineType type;
    /* 根据type类型不同，如下信息可能存在部分有效，无效值设置为： TY_NPU_PROFILE_UNVALID (-1) */
    float executeTime; /* 所有type均有效， 单位：ms */
    float macUtilRate;  /* E_AI_NPU有效， 范围：[0,1] */
    float ddrBandWidth; /* E_AI_NPU有效， MB/s */
    uint64_t cycles;    /* E_AI_NPU有效， 时钟周期 */
    uint64_t memThroughput[TY_NPU_PROFILE_MEM_THROUGHPUT_TYPE_NUM]; /* E_AI_NPU有效, 默认0为ddr吞吐，其他待扩展 */
}T_TY_Profile;

typedef struct TY_OpProfileInfo{
    char opName[TY_NPU_OPTYPE_STRING_MAX_LEN];
    T_TY_Profile profile;
}T_TY_OpProfileInfo;

typedef struct TY_NpuHwCapacity{
    uint32_t nuCurrFreq;  /* Mhz */
    uint32_t nuMaxFreq;   /* Mhz */
    uint32_t vuCurrFreq;  /* Mhz */
    uint32_t vuMaxFreq;   /* Mhz */
    float nuTops;         /* nu 算力 */
    float vuTops;         /* vu 算力 */
    char nuDataFmt[TY_NPU_NU_SUPPORT_DATA_FORMAT_LEN];
    char vuDataFmt[TY_NPU_VU_SUPPORT_DATA_FORMAT_LEN];
}T_TY_NpuHwCapacity;

typedef struct TY_NpuDrvCapacity{
    uint32_t taskQueDepth;  /* 可配置的队列深度信息 */
}T_TY_NpuDrvCapacity;

typedef struct TY_NpuCapacityInfo{
    T_TY_NpuHwCapacity hardware;
    T_TY_NpuDrvCapacity driver;
}T_TY_NpuCapacityInfo;

typedef struct TY_NpuPayLoadInfo{
    uint32_t waitTaskCnt;  /* 等待执行的task数目 */
    uint32_t resTaskCnt;   /* 剩余可用的task数目 */
    uint32_t allTaskCnt;   /* 总计所有的task数目 */
    float rate;            /* 负载百分比 */
}T_TY_NpuPayLoadInfo;

/*
    dump功能
    1. 应用场景：针对芯片模型部署时算子精度比对 & debug信息 ；
    2. 使用限制：
       a. 由于dump执行效率低，不建议多模型、多TaskHandle同时调度执行；否则会造成模型latency严重，任务拥塞；
       b. dump和profile功能二选一，打开dump会导致profile数据不准；
*/
typedef struct TY_OpDumpInfo{
    char name[TY_NPU_OPTYPE_STRING_MAX_LEN];
    /*  dump typeMask:
        1 ==> enable; 0 ==> disable
        bit   | 31~4      |   3   |   2    |  1  |  0  |
        type  | reserved  |  QUA  | WEIGHT | OUT |  IN |  、、
    */
    int32_t typeMask;
    int32_t numInput; /* op输入数目, 可以不填 */
    int32_t numOutput; /* op输出数目, 可以不填 */
    /*
        DUMP内存空间定义
       1. 外部负责申请、释放；
       2. 不需要物理地址连续
    */
            T_TY_Mem  weight;
            T_TY_Mem  quanti;
            /*
            inputs & outputs 数组填写顺序说明：
            1. TY_NPU_GetOpListInfo()中获取的每个op的inputDesc & outputDesc数组
            2. 按照inputDesc & outputDesc的顺序来填写inputs & outputs memInfo
            */
            T_TY_Mem  inputs[TY_NPU_MODLE_BLOB_MAX_INPUT_NUM];
            T_TY_Mem  outputs[TY_NPU_MODLE_BLOB_MAX_OUTPUT_NUM];
}T_TY_OpDumpInfo;

typedef struct TY_Mat{
    uint32_t rows;
    uint32_t cols;
    E_TY_DataType dataType;
    uint32_t rowStride; //以字节为单位，当前仅支持rowStride=cols*sizeof(dataType)
    T_TY_Mem mem;
    uint32_t reserved[2];
}T_TY_Mat;

typedef struct TY_MatMulCfg{ //AxB=C, A维度m行k列，B维度k行n列，C维度m行n列
    uint32_t maxM;//支持的最大m
    uint32_t maxN;//支持的最大n
    uint32_t maxK;//支持的最大k
    uint32_t priority; //0:普通优先级，1:相对NPU_Forward高优先级
    uint32_t reserved[2];
}T_TY_MatMulCfg;

#pragma pack()

typedef void* TY_NPU_MODEL_HANDLE;
typedef void* TY_NPU_TASK_HANDLE;
typedef uint64_t TY_NPU_NN_HANDLE;
typedef void* TY_NPU_MATMUL_HANDLE;

/*!
* @brief NPU系统初始化
* @note  调用本接口初始化成功，其它接口才能正常工作
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_SysInit(void);

/*!
* @brief NPU系统去初始化
* @note  本接口与NPU_SysInit配对使用，调用本接口后，其它接口不能正常工作
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_SysExit(void);

/*!
* @brief 查询NPU接口版本信息
* @param [out] majorVersion major版本
* @param [out] minorVersion minor版本
* @param [out] patchVersion patch版本
* @note  版本信息格式 x.y.z
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_GetVersion(int32_t *majorVersion,
                       int32_t *minorVersion,
                       int32_t *patchVersion);

/*!
* @brief 获取NPU模型所需内存大小
* @param [in] model     从离线文件读取的NPU模型二进制文件数据起始指针，由用户开辟内存
* @param [in] modelSize 离线模型文件大小
* @param [out] modelMem 创建网络模型所需内存信息，结构体中allocInfo成员有效；
*                       用户根据此信息分配对应内存，并赋值结构体中mem成员，用于创建模型
*
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_GetModelMemSize(const void *model,
                            int32_t modelSize,
                            T_TY_MemSegmentInfo *modelMemInfo);

/*!
* @brief 解析并创建NPU模型实例
* @param [in] model     从离线文件读取的NPU模型二进制文件数据起始指针，由用户开辟内存
* @param [in] modelSize 离线模型文件大小
* @param [in] configParam 解析模型的配置参数，当前版本无效
* @param [in] modelMem    为创建模型实例分配的内存信息，见TY_NPU_GetModelMemSize
* @param [out] desc        解析离线模型得到的模型输入输出信息
* @param [out] modelHandle 创建的模型实例；用户利用此实例创建推理task
*
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_CreateModel(const void *model,
                            int32_t modelSize,
                            const T_TY_ModelCfgParam *configParam,
                            const T_TY_MemSegmentInfo *modelMem,
                            T_TY_ModelDesc *desc,
                            TY_NPU_MODEL_HANDLE *modelHandle);

/*!
* @brief 直接从内存（物理地址连续）空间创建NPU模型实例，避免NPU内部cpy & 多余备份
* @param [in] model     模型对应的内存存储（MemSegMent）空间，需要保证：
*                       1. 内存空间物理地址连续；
*                       2. 内存空间首地址对齐 32 Bytes；
* @param [in] configParam 解析模型的配置参数，当前版本无效
* @param [in] modelMem  解析模型需要的额外内存空间，预留扩展，当前版本无需分配，可设置为nullptr
* @param [out] desc        解析离线模型得到的模型输入输出信息
* @param [out] modelHandle 创建的模型实例；用户利用此实例创建推理task
*
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_CreateModelFromPhyMem(const T_TY_MemSegmentInfo *model,
                            const T_TY_ModelCfgParam *configParam,
                            const T_TY_MemSegmentInfo *modelMem,
                            T_TY_ModelDesc *desc,
                            TY_NPU_MODEL_HANDLE *modelHandle);

/*!
* @brief 获取NPU子图运行所需的内存大小
* @param [in] modelHandle 已创建的有效模型实例
* @param [in] configParam 创建子图任务的配置参数
* @param [out] taskMem    子图网络运行所需内存信息，结构体中allocInfo成员有效；
*                         用户根据此信息分配对应内存，并赋值结构体中mem成员，用于创建task
*
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_GetTaskMemSize(const TY_NPU_MODEL_HANDLE modelHandle,
                            const T_TY_TaskCfgParam *configParam,
                            T_TY_MemSegmentInfo *taskMem);

/*!
* @brief 创建NPU子图的一个运行实例
* @param [in] modelHandle 已创建的有效模型实例
* @param [in] configParam 创建子图任务的配置参数
* @param [in] taskMem     为创建子图实例分配的内存信息，见TY_NPU_GetTaskMemSize
* @param [out] taskHandle 创建的模型运行实例；用户利用此实例执行推理task
*
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_CreateTask(const TY_NPU_MODEL_HANDLE modelHandle,
                            const T_TY_TaskCfgParam *configParam,
                            const T_TY_MemSegmentInfo *taskMem,
                            TY_NPU_TASK_HANDLE *taskHandle);

/*!
* @brief NPU子图同步推理，接口返回推理完成
* @param [in] taskHandle 已创建的有效运行实例
* @param [in] npuId      指定task运行的NPU，仅多NPU平台适用
* @param [in] inputNum   输入数目
* @param [in] taskInput  运行实例的输入，用户需按照TY_NPU_CreateModel接口获取的模型输入描述信息，分配内存，填充输入数据
* @param [in] outputNum  输出数目
* @param [in] taskOutput 运行实例的输出，用户需按照TY_NPU_CreateModel接口获取的模型输出描述信息，分配内存
*
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_Forward(const TY_NPU_TASK_HANDLE taskHandle,
                            E_TY_NpuID npuId,
                            uint32_t inputNum,
                            const T_TY_TaskInput taskInput[],
                            uint32_t outputNum,
                            const T_TY_TaskOutput taskOutput[]);

/*!
* @brief 支持设置优先级的NPU子图同步推理，接口返回推理完成
* @param [in] taskHandle 已创建的有效运行实例
* @param [in] npuId      指定task运行的NPU，仅多NPU平台适用
* @param [in] inputNum   输入数目
* @param [in] taskInput  运行实例的输入，用户需按照TY_NPU_CreateModel接口获取的模型输入描述信息，分配内存，填充输入数据
* @param [in] outputNum  输出数目
* @param [in] taskOutput 运行实例的输出，用户需按照TY_NPU_CreateModel接口获取的模型输出描述信息，分配内存
* @param [in] priority   推理任务优先级指示，0 ：低优先级， 1：高优先级
*
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_ForwardPriority(const TY_NPU_TASK_HANDLE taskHandle,
                            E_TY_NpuID npuId,
                            uint32_t inputNum,
                            const T_TY_TaskInput taskInput[],
                            uint32_t outputNum,
                            const T_TY_TaskOutput taskOutput[],
                            uint32_t priority);

/*!
* @brief NPU子图异步推理，接口返回时推理任务下发成功，用TY_NPU_Query接口查询是否推理完
* @param [in] taskHandle 已创建的有效运行实例
* @param [in] npuId      指定task运行的NPU，仅多NPU平台适用
* @param [in] inputNum   输入数目
* @param [in] taskInput  运行实例的输入，用户需按照TY_NPU_CreateModel接口获取的模型输入描述信息，分配内存，填充输入数据
* @param [in] outputNum  输出数目
* @param [in] taskOutput 运行实例的输出，用户需按照TY_NPU_CreateModel接口获取的模型输出描述信息，分配内存
* @param [out] nnHandle  本次推理对应的handle信息，用于非阻塞执行时状态查询
*
* 返回值：
    0：成功，其它值：失败
* 说明：使用相同taskHandle进行多个异步推理时，各推理的taskInput、taskOutput应分别分配内存
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_ForwardAsync(const TY_NPU_TASK_HANDLE taskHandle,
                            E_TY_NpuID npuId,
                            uint32_t inputNum,
                            const T_TY_TaskInput taskInput[],
                            uint32_t outputNum,
                            const T_TY_TaskOutput taskOutput[],
                            TY_NPU_NN_HANDLE* nnHandle);

/*!
* @brief 支持设置优先级的NPU子图异步推理，接口返回时推理任务下发成功，用TY_NPU_Query接口查询是否推理完
* @param [in] taskHandle 已创建的有效运行实例
* @param [in] npuId      指定task运行的NPU，仅多NPU平台适用
* @param [in] inputNum   输入数目
* @param [in] taskInput  运行实例的输入，用户需按照TY_NPU_CreateModel接口获取的模型输入描述信息，分配内存，填充输入数据
* @param [in] outputNum  输出数目
* @param [in] taskOutput 运行实例的输出，用户需按照TY_NPU_CreateModel接口获取的模型输出描述信息，分配内存
* @param [out] nnHandle  本次推理对应的handle信息，用于非阻塞执行时状态查询
* @param [in] priority   推理任务优先级指示，0 ：低优先级， 1：高优先级
*
* 返回值：
    0：成功，其它值：失败
* 说明：使用相同taskHandle进行多个异步推理时，各推理的taskInput、taskOutput应分别分配内存
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_ForwardAsyncPriority(const TY_NPU_TASK_HANDLE taskHandle,
                            E_TY_NpuID npuId,
                            uint32_t inputNum,
                            const T_TY_TaskInput taskInput[],
                            uint32_t outputNum,
                            const T_TY_TaskOutput taskOutput[],
                            TY_NPU_NN_HANDLE* nnHandle,
                            uint32_t priority);

/*!
* @brief 查询NPU指定推理任务是否完成
* @param [in] npuId      NPU编号
* @param [in] nnHandle   推理任务对应的handle，来源于TY_NPU_ForwardAsync接口
* @param [in] status     是否完成，1：完成，0：未完成
* @param [out] isBlock   指示是否阻塞查询，1：阻塞，0：非阻塞
*
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_Query( E_TY_NpuID npuId,
                            TY_NPU_NN_HANDLE nnHandle, E_TY_Bool *status, int32_t isBlock);

/*!
* @brief 释放网络运行实例和相关资源
* @param [in] taskHandle 网络运行实例
*
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_ReleaseTask(const TY_NPU_TASK_HANDLE taskHandle);

/*!
* @brief 释放模型
* @param [in] modelHandle 子图模型实例
*
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_ReleaseModel(const TY_NPU_TASK_HANDLE modelHandle);

/*!
* @brief 查询加载分辨率小于等于(m,n,k)的矩阵乘法算子所需Memory大小、特性
*        对于矩阵乘法axb = c，a维度m行k列，b矩阵维度k行n列，c矩阵维度m行n列
* @param [in]  matMulCfg 矩阵乘法配置，包括最大的m,n,k值
* @param [out] matMulMemInfo 加载MatMul算子需要的内存信息，用户根据T_MemSegmentInfo结构中allocInfo成员申请对应内存，
*                         赋值T_MemSegmentInfo结构中mem成员，通过TY_NPU_CreateMatMul创建MatMul算子库
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_GetMatMulMemSize(const T_TY_MatMulCfg *matMulCfg,
                                T_TY_MemSegmentInfo *matMulMemInfo);
/*!
* @brief 利用用户提供的内存信息加载MatMul算子库
*        对于矩阵乘法axb = c，a维度m行k列，b矩阵维度k行n列，c矩阵维度m行n列
* @param [in] matMulCfg 矩阵乘法配置，包括最大的m,n,k值，与TY_NPU_GetMatMulMemSize入参一致
* @param [in] matMulMemInfo 加载MatMul算子需要的内存信息，见TY_NPU_GetMatMulMemSize
* @param [out] handle 矩阵乘算子标识
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_CreateMatMul(const T_TY_MatMulCfg *matMulCfg,
                                const T_TY_MemSegmentInfo *matMulMemInfo, TY_NPU_MATMUL_HANDLE *handle);
/*!
* @brief 矩阵乘算子，输入输出矩阵均需物理地址连续空间
*           公式: c = a x Transpose(b),  a,b,c均为矩阵;
* @param [in] handle 算子handle，见TY_NPU_CreateMatMul
* @param [in] npuId  指定矩阵乘运行的NPU，仅多NPU平台适用
* @param [in] srcA a矩阵, 行优先存储, 维度m行k列, 仅支持int8_t
* @param [in] srcB b矩阵的转置, 行优先存储, 维度n行k列, 仅支持int8_t
* @param [out] dstC c矩阵, 行优先存储, 维度m行n列, 仅支持int32_t
* @param [out] nnHandle 任务handle，isBlock为1时，可为nullptr
* @param [in]  isBlock 是否阻塞，1：阻塞，接口返回任务完成，0：非阻塞，需TY_NPU_Query获取任务完成状态
*
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_MatMul(const TY_NPU_MATMUL_HANDLE handle, E_TY_NpuID npuId,
                            const T_TY_Mat* srcA, const T_TY_Mat* srcB, T_TY_Mat* dstC,
                            TY_NPU_NN_HANDLE *nnHandle, int32_t isBlock);

/*!
* @brief 释放矩阵乘法算子,须在TY_NPU_SysExit之前调用，且TY_NPU_MatMul任务已完成
* @param [in] handle 矩阵乘法算子实例，见TY_NPU_CreateMatMul
*
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_ReleaseMatMul(const TY_NPU_MATMUL_HANDLE handle);

/*!
* @brief 获取NPU模型fusion-op算子数目
* @param [in] modelHandle 模型handle，通过TY_NPU_CreateModel()创建
* @param [out] opNum 模型对应的fusion-op算子数目，包括（npu、cpu算子)
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_GetOpNum(
                            const TY_NPU_MODEL_HANDLE modelHandle,
                            int32_t *opNum);

/*!
* @brief 获取NPU模型fusion-op算子信息
* @param [in] modelHandle 模型handle，通过TY_NPU_CreateModel()创建
* @param [in] opNum fusion-op算子数目
* @param [out] opDescList 指向算子列表空间，由调用者提供内存空间，
*                         数组，[opNum]
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_GetOpListInfo(
                            const TY_NPU_MODEL_HANDLE modelHandle,
                            int32_t opNum,  /* 输入op数目，如果小于子图内op的数目，取前opNum个 */
                            T_TY_OpDesc opDescList[]  /* 外部申请空间 */
                            );

/*!
* @brief 查询NPU能力信息
* @param [in] id, npu id信息，当前版本不适用
* @param [out] capacity npu能力信息
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_QueryCapacity(
                            E_TY_NpuID id,
                            T_TY_NpuCapacityInfo *capacity);

/*!
* @brief 查询NPU负载信息
* @param [in] id, npu id信息，当前版本不适用
* @param [out] capacity npu能力信息
* @note 建议用户利用该信息，用户层面做流控设计；比如：rate超过一定门限后，下发task阻塞。
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_QueryLoad(
                            E_TY_NpuID id,
                            T_TY_NpuPayLoadInfo *load);

/*!
* @brief 绑定模型的profile统计功能
* @param [in] modelHandle 任务handle，通过TY_NPU_CreateModel()创建
* @note 调用该函数后，modelHandle关联的所有taskHandle均产生profile信息
*        1. 绑定后Create的TaskHandle均有效；未完成绑定，不产生profile；
*        2. 绑定动作执行1次即可，同一模型可以重复绑定；
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_SetProfileOn(const TY_NPU_MODEL_HANDLE modelHandle);

/*!
* @brief 去模型的profile统计功能绑定
* @param [in] modelHandle 任务handle，通过TY_NPU_CreateModel()创建
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_SetProfileOff(const TY_NPU_MODEL_HANDLE modelHandle);

/*!
* @brief 获取taskHandle对应的profile信息（用户确保task执行完毕后调用）
* @param [in]  taskHandle   模型handle，通过TY_NPU_CreateTask()创建
* @param [in]  opProfileNum 模型内包括的fusion-op数目，通过TY_NPU_GetOpNum()获取
* @param [out] outputProfileNum  有效profile输出的fusion-op数目
* @param [out] opProfiles op profile信息，数组，[opProfileNum]
                空间大小： opProfileNum*sizeof(TY_OpProfileInfo), 调用者提供内存空间
* @note task profile只记录最近一次的TY_NPU_Forward()执行输出，不支持同一taskHandle不同TY_NPU_Forward()调用后的反复获取！！！
*       用户需要保证： TY_NPU_Forward()与TY_NPU_GetProfileInfo()对应关系！！！
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_GetProfileInfo(
                            const TY_NPU_TASK_HANDLE taskHandle,
                            int32_t opProfileNum,
                            int32_t *outputProfileNum,
                            T_TY_OpProfileInfo opProfiles[]);

/*!
* @brief 设置task的dump信息
* @param [in] taskHandle 任务handle，通过TY_NPU_CreateTask()创建
* @param [in] opNum op数目, 范围(0, opNum]，maxOpNum由TY_NPU_GetOpNum()获取
* @param [out] opDumps  dump信息，数组[opNum]，
*                       空间大小： opNum*sizeof(TY_OpDumpInfo), 调用者提供内存空间
* @note (1) T_OpDumpInfo可以只设置一次，反复做dump动作；
*       (2) taskHandle执行前T_OpDumpInfo可动态修改，以最近一次修改为准；
*       (3) taskHandle执行完毕后，默认dump信息全部已导出，用户可直接使用；
*       (4) 当前版本只支持同步推理接口；
*       问题场景：
*           存在dump所需空间太大，用户内存不足的情况？？？
*           解决：用户拆成多次dump动作，多次跑同一个模型
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_NPU_SetDumpOp(
                            const TY_NPU_TASK_HANDLE taskHandle,
                            int32_t opNum,
                            T_TY_OpDumpInfo *opDumps);

#ifdef __cplusplus
}
#endif




#endif