#ifndef CV_H_
#define CV_H_

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TY_CV_MAX_IMG_NUM     32
/*TY_CV图像宽最小像素*/
#define TY_CV_MIN_IMG_WIDTH   20 
/*TY_CV图像高最小像素*/    
#define TY_CV_MIN_IMG_HEIGHT  20
/*TY_CV图像宽最大像素*/
#define TY_CV_MAX_IMG_WIDTH   8192
/*TY_CV图像高最大像素*/
#define TY_CV_MAX_IMG_HEIGHT  8192

typedef uint64_t TY_CV_TASK_HANDLE;


/*!
* @brief CV系统初始化
* @note  调用本接口初始化成功，其它TY_CV接口才能正常工作
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_CV_SysInit(void);

/*!
* @brief CV系统去初始化
* @note  本接口与TY_CV_SysInit配对使用，调用本接口后，其它TY_CV接口不能正常工作
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_CV_SysExit(void);

/*!
* @brief 查询CV接口版本信息
* @param [out] majorVersion major版本
* @param [out] minorVersion minor版本
* @param [out] patchVersion patch版本
* @note  版本信息格式 x.y.z
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_CV_GetVersion(int32_t *majorVersion, 
                       int32_t *minorVersion, 
                       int32_t *patchVersion);

/*!
* @brief 创建图像双线性插值缩放任务，源及目的图像的内存类型为VMM_CACHED，需要用户flush cache  
* @param [in]  src  源图像，支持ROI，支持stride，
*                   支持图像格式：
*                    E_TY_PIXEL_FORMAT_YUV_400
*                    E_TY_PIXEL_FORMAT_RGB_888_PLANAR
*                    E_TY_PIXEL_FORMAT_BGR_888_PLANAR
* @param [out] dst  目的图像，不支持ROI，不支持stride
*                   目的图像格式须与源图像格式相同  
* @param [in]  count 图像数目，源图像与目的图像数目相同，最大TY_CV_MAX_IMG_NUM
* @param [out] handle 任务handle，isBlock为1时，可为nullptr
* @param [in]  isBlock 是否阻塞，1：阻塞，接口返回任务完成，0：非阻塞，需CV_Query获取任务完成状态
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_CV_Resize(T_TY_Image src[], T_TY_Image dst[], 
                                int32_t count, TY_CV_TASK_HANDLE *handle, int32_t isBlock);

/*!
* @brief 创建图像格式转换任务，源及目的图像的内存类型为VMM_CACHED，需要用户flush cache
* @param [in]  src 源图像，支持ROI，支持stride，
*                  支持图像格式:
*                    E_TY_PIXEL_FORMAT_YUV_SEMIPLANAR_420、
*                    E_TY_PIXEL_FORMAT_YVU_SEMIPLANAR_420、
*                    E_TY_PIXEL_FORMAT_YUYV_PACKED_422、
*                    E_TY_PIXEL_FORMAT_UYVY_PACKED_422
* @param [out] dst  目的图像，不支持roi，不支持stride即widthStride==width，heightStride==height
*                   支持图像格式:
*                    E_TY_PIXEL_FORMAT_RGB_888_PLANAR  
*                    E_TY_PIXEL_FORMAT_BGR_888_PLANAR
*                   目的图像分辨率与源图像分辨率相同，如带ROI，目的图像分辨率与ROI宽高相同
* @param [in]  count 图像数目，源图像与目的图像数目相同，最大 TY_CV_MAX_IMG_NUM
* @param [out] handle 任务handle，isBlock为1时，可为nullptr
* @param [in]  isBlock 是否阻塞，1：阻塞，接口返回任务完成，0：非阻塞，需CV_Query获取任务完成状态
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_CV_CvtColor(T_TY_Image src[], T_TY_Image dst[], 
                                int32_t count, TY_CV_TASK_HANDLE *handle, int32_t isBlock);

/*!
* @brief 创建图像格式转换及缩放任务，源及目的图像的内存类型为VMM_CACHED，需要用户flush cache
* @param [in]  src  源图像，支持ROI，支持stride，
*                   支持图像格式:
*                    E_TY_PIXEL_FORMAT_YUV_400
*                    E_TY_PIXEL_FORMAT_RGB_888_PLANAR
*                    E_TY_PIXEL_FORMAT_BGR_888_PLANAR
*                    E_TY_PIXEL_FORMAT_YUV_SEMIPLANAR_420
*                    E_TY_PIXEL_FORMAT_YVU_SEMIPLANAR_420
*                    E_TY_PIXEL_FORMAT_YUYV_PACKED_422
*                    E_TY_PIXEL_FORMAT_UYVY_PACKED_422
* @param [out] dst  目的图像，不支持ROI，不支持stride
*                   支持图像格式：
*                    E_TY_PIXEL_FORMAT_YUV_400
*                    E_TY_PIXEL_FORMAT_RGB_888_PLANAR
*                    E_TY_PIXEL_FORMAT_BGR_888_PLANAR
* @param [in]  count 图像数目，源图像与目的图像数目相同，最大TY_CV_MAX_IMG_NUM
* @param [out] handle 任务handle，isBlock为1时，可为nullptr
* @param [in]  isBlock 是否阻塞，1：阻塞，接口返回任务完成，0：非阻塞，需CV_Query获取任务完成状态
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_CV_CvtResize(T_TY_Image src[], T_TY_Image dst[], 
                                int32_t count, TY_CV_TASK_HANDLE *handle, int32_t isBlock);

/*!
* @brief 查询图像任务结果
* @param [in] handle 任务handle
* @param [out] status 任务状态 1：完成，0：未完成
* @param [in]  isBlock 是否阻塞，1：阻塞，接口返回任务完成，0：非阻塞，需CV_Query获取任务完成状态
* 返回值：
    0：成功，其它值：失败
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_CV_Query(TY_CV_TASK_HANDLE handle, E_TY_Bool *status, int32_t isBlock);


#ifdef __cplusplus
}
#endif
#endif
