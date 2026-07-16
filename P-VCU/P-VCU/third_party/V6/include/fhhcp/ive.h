#ifndef IVE_H_
#define IVE_H_

#include <stdint.h>
#include "base.h"   

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)

typedef uint64_t TY_IVE_TASK_HANDLE;
#define TY_IVE_MAP_NUM   256

typedef enum TY_ThreshMode {
    E_TY_THRESH_MODE_TY_BINARY       =  0x0,  /*srcVal <= lowThr, dstVal = minVal; srcVal > lowThr, dstVal = maxVal.*/
    E_TY_THRESH_MODE_TY_TRUNC        =  0x1,  /*srcVal <= lowThr, dstVal = srcVal; srcVal > lowThr, dstVal = maxVal.*/
    E_TY_THRESH_MODE_TY_TO_MINVAL    =  0x2,  /*srcVal <= lowThr, dstVal = minVal; srcVal > lowThr, dstVal = srcVal.*/

    E_TY_THRESH_MODE_TY_MIN_MID_MAX  =  0x3,  /*srcVal <= lowThr, dstVal = minVal;  lowThr < srcVal <= highThr, dstVal = midVal; srcVal > highThr, dstVal = maxVal.*/
    E_TY_THRESH_MODE_TY_ORI_MID_MAX  =  0x4,  /*srcVal <= lowThr, dstVal = srcVal;  lowThr < srcVal <= highThr, dstVal = midVal; srcVal > highThr, dstVal = maxVal.*/
    E_TY_THRESH_MODE_TY_MIN_MID_ORI  =  0x5,  /*srcVal <= lowThr, dstVal = minVal;  lowThr < srcVal <= highThr, dstVal = midVal; srcVal > highThr, dstVal = srcVal.*/
    E_TY_THRESH_MODE_TY_MIN_ORI_MAX  =  0x6,  /*srcVal <= lowThr, dstVal = minVal;  lowThr < srcVal <= highThr, dstVal = srcVal; srcVal > highThr, dstVal = maxVal.*/
    E_TY_THRESH_MODE_TY_ORI_MID_ORI  =  0x7,  /*srcVal <= lowThr, dstVal = srcVal;  lowThr < srcVal <= highThr, dstVal = midVal; srcVal > highThr, dstVal = srcVal.*/

    E_TY_THRESH_MODE_TY_BUTT
}E_TY_ThreshMode;

typedef struct TY_ThreshCtrlU8 {
    E_TY_ThreshMode enMode;
    uint8_t lowThr;         /*user-defined threshold,  0<=lowThr<=255 */
    uint8_t highThr;        /*user-defined threshold, if enMode<E_THRESH_MODE_MIN_MID_MAX, highThr is not used, else 0<=lowThr<=highThr<=255;*/
    uint8_t minVal;         /*Minimum value when tri-level thresholding*/
    uint8_t midVal;         /*Middle value when tri-level thresholding, if enMode<2, u32MidVal is not used; */
    uint8_t maxVal;         /*Maxmum value when tri-level thresholding*/
    uint8_t reserve;
}T_TY_ThreshCtrlU8;


typedef enum TY_ThreshModeU16 {
    E_TY_THRESH_U16_MODE_U16_TO_U8_MIN_MID_MAX  =  0x0,
    E_TY_THRESH_U16_MODE_U16_TO_U8_MIN_ORI_MAX  =  0x1,
    E_TY_THRESH_U16_MODE_BUTT
}E_TY_ThreshModeU16;

typedef struct TY_ThreshCtrlU16 {
    E_TY_ThreshModeU16 enMode;
    uint16_t lowThr;
    uint16_t highThr;
    uint8_t  minVal;
    uint8_t  midVal;
    uint8_t  maxVal;
    uint8_t  reserve;
}T_TY_ThreshCtrlU16;


typedef struct TY_MapLutU8 {
    uint8_t map[TY_IVE_MAP_NUM];
}T_TY_MapLutU8;
typedef struct TY_MapLut16 {
    uint16_t map[TY_IVE_MAP_NUM];
}T_TY_MapLutU16;

typedef enum TY_MapMode {
    E_TY_MAP_MODE_U8  =  0x0, //U8C1->U8C1    
    E_TY_MAP_MODE_U16 =  0x2 //U8C1->U16C1
}E_TY_MapMode;   

typedef struct TY_MapCtrl{
    E_TY_MapMode mode;
}T_TY_MapCtrl;

#pragma pack()

/*!
* @brief IVE系统初始化
* @param [in] iveModelDir ive功能的算子模型目录
* @note  调用本接口初始化成功，IVE接口才能正常工作
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_IVE_SysInit(const char *iveModelDir);

/*!
* @brief IVE系统去初始化
* @note  本接口与IVE_SysInit配对使用，调用本接口后，其它ive接口不能正常工作
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_IVE_SysExit(void);

/*!
* @brief 查询IVE接口版本信息
* @param [out] majorVersion major版本
* @param [out] minorVersion minor版本
* @param [out] patchVersion patch版本
* @note  版本信息格式 x.y.z
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_IVE_GetVersion(int32_t *majorVersion, 
                       int32_t *minorVersion, 
                       int32_t *patchVersion);

/*!
* @brief 查询加载所有IVE算子所需Memory大小、特性
* @param [out] iveMemInfo 加载IVE算子需要的内存信息，用户根据T_MemSegmentInfo结构中allocInfo成员申请对应内存，
*                         赋值T_MemSegmentInfo结构中mem成员，通过IVE_Create创建IVE算子库
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_IVE_GetIveMemSize(T_TY_MemSegmentInfo *iveMemInfo);

/*!
* @brief 利用用户提供的内存信息加载IVE算子库
* @param [in] iveMemInfo 加载IVE算子需要的内存信息
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_IVE_Create(const T_TY_MemSegmentInfo *iveMemInfo);

/*!
* @brief 创建灰度图像阈值化任务
* @param [in]  src  源图像，支持图像格式E_PIXEL_FORMAT_U8C1
* @param [in]  ctrl 阈值化模式及阈值参数
* @param [out] dst  目的图像，支持图像格式E_PIXEL_FORMAT_U8C1
* @param [out] handle 任务handle
* @param [in]  isBlock 是否阻塞，1：阻塞，接口返回任务完成，0：非阻塞，需IVE_Query获取任务完成状态
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_IVE_ThreshU8(const T_TY_Image *src, const T_TY_ThreshCtrlU8 *ctrl, T_TY_Image *dst, 
                                        TY_IVE_TASK_HANDLE *handle, int32_t isBlock);

/*!
* @brief 创建灰度图像阈值化任务
* @param [in]  src  源图像，支持图像格式E_PIXEL_FORMAT_U16C1
* @param [in]  ctrl 阈值化模式及阈值参数
* @param [out] dst  目的图像，支持图像格式E_PIXEL_FORMAT_U8C1
* @param [out] handle 任务handle
* @param [in]  isBlock 是否阻塞，1：阻塞，接口返回任务完成，0：非阻塞，需IVE_Query获取任务完成状态
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_IVE_ThreshU16(const T_TY_Image *src, const T_TY_ThreshCtrlU16 *ctrl, T_TY_Image *dst, 
                                            TY_IVE_TASK_HANDLE *handle, int32_t isBlock);

/*!
* @brief 创建映射任务
* @param [in]  src  源图像，支持图像格式E_PIXEL_FORMAT_U8C1
* @param [in]  mapTable 映射表格内存信息，U8C1->U16C1时空间至少sizeof(T_MapLutU8);
*                       U8C1->U16C1时空间至少sizeof(T_MapLutU16)
* @param [out] dst  目的图像，支持图像格式E_PIXEL_FORMAT_U8C1/E_PIXEL_FORMAT_U16C1
* @param [in]  ctrl 控制参数，指示映射方式
* @param [out] handle 任务handle
* @param [in]  isBlock 是否阻塞，1：阻塞，接口返回任务完成，0：非阻塞，需IVE_Query获取任务完成状态
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_IVE_Map(const T_TY_Image *src, const T_TY_Mem *mapTable, T_TY_Image *dst, 
                                   T_TY_MapCtrl *ctrl, TY_IVE_TASK_HANDLE *handle, int32_t isBlock);

/*!
* @brief 创建二值图像相与任务
* @param [in]  src0  源图像，支持图像格式E_PIXEL_FORMAT_U8C1
* @param [in]  src1  源图像，支持图像格式E_PIXEL_FORMAT_U8C1
* @param [out] dst  目的图像，支持图像格式E_PIXEL_FORMAT_U8C1
* @param [out] handle 任务handle
* @param [in]  isBlock 是否阻塞，1：阻塞，接口返回任务完成，0：非阻塞，需IVE_Query获取任务完成状态
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_IVE_And(const T_TY_Image *src0, const T_TY_Image *src1, T_TY_Image *dst, 
                                    TY_IVE_TASK_HANDLE *handle, int32_t isBlock);

/*!
* @brief 创建二值图像相或任务
* @param [in]  src0  源图像，支持图像格式E_PIXEL_FORMAT_U8C1
* @param [in]  src1  源图像，支持图像格式E_PIXEL_FORMAT_U8C1
* @param [out] dst  目的图像，支持图像格式E_PIXEL_FORMAT_U8C1
* @param [out] handle 任务handle
* @param [in]  isBlock 是否阻塞，1：阻塞，接口返回任务完成，0：非阻塞，需IVE_Query获取任务完成状态
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_IVE_Or(const T_TY_Image *src0, const T_TY_Image *src1, T_TY_Image *dst, 
                                    TY_IVE_TASK_HANDLE *handle, int32_t isBlock);

/*!
* @brief 查询任务完成状态
* @param [in]  handle  任务标识
* @param [out]  status 任务是否完成
* @param [in]  isBlock 是否阻塞，1：阻塞，接口返回任务完成，0：非阻塞
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_IVE_Query(TY_IVE_TASK_HANDLE handle, E_TY_Bool *status, int32_t isBlock);



#ifdef __cplusplus
}
#endif
#endif