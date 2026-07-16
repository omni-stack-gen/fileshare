#ifndef __MPI_REGION_H__
#define __MPI_REGION_H__

#include "fh_region_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

/*Standard API*/

/**
 * @brief      创建区域。
 *
 * @param[in]  Handle           区域句柄号。必须是未使用的 Handle 号。 取值范围：[0, RGN_HANDLE_MAX)。
 * @param[in]  pstRegion        区域属性指针。
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,region_errcode} 。
 *
 * @note
 *   - 该句柄由用户指定，意义等同于 ID 号。
 *   - 不支持重复创建。
 *   - 区域属性必须合法，具体约束参见 #RGN_ATTR_S 。  区域属性指针不能为空。
 *   - 其它的属性，如区域位置，层次等信息在调用 #FH_RGN_AttachToChn 接口时指定。
 *   - 创建区域时，本接口只进行基本的参数的检查，譬如：最小宽高，最大宽高等；当区域 attach 到通道上时，
 *     根据各通道模块支持类型的约束条件进行更加有针对性的参数检查，譬如支持的像素格式等。OverLay的宽度要求4对齐。
 *
 */
FH_SINT32 FH_RGN_Create(RGN_HANDLE Handle, const RGN_ATTR_S *pstRegion);

/**
 * @brief      销毁区域。
 *
 * @param[in]  Handle           区域句柄号。 取值范围：[0, RGN_HANDLE_MAX)。
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,region_errcode} 。
 *
 * @note
 *   - 区域必须已创建。
 *
 */
FH_SINT32 FH_RGN_Destroy(RGN_HANDLE Handle);

/**
 * @brief      获取区域属性。
 *
 * @param[in]    Handle     区域句柄号。
 * @param[out]   pstRegion  区域属性指针。
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,region_errcode} 。
 *
 * @note
 *   - 区域必须已创建。
 *   - 区域属性指针不能为空。
 *
 */
FH_SINT32 FH_RGN_GetAttr(RGN_HANDLE Handle, RGN_ATTR_S *pstRegion);
/**
 * @brief     设置区域属性。
 *
 * @param[in]  Handle     区域句柄号。
 * @param[in]  pstRegion  区域属性指针。
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,region_errcode} 。
 *
 * @note
 *   - 区域必须已创建。
 *   - 区域属性指针不能为空。
 *   - 当区域通过 #FH_RGN_AttachToChn 接口绑定到通道上时，本接口不可以用于
 *       修改静态属性，但是可以修改动态属性；当区域没有attach到任何通道上时，本
 *       接口即可用于修改静态属性，也可用于修改动态属性。
 *
 */
FH_SINT32 FH_RGN_SetAttr(RGN_HANDLE Handle, const RGN_ATTR_S *pstRegion);


/**
 * @brief      设置区域位图，即对区域进行位图填充。
 *
 * @param[in]  Handle           区域句柄号。取值范围：[0, RGN_HANDLE_MAX)。
 * @param[in]  pstBitmap        位图属性指针。
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,region_errcode} 。
 *
 * @note
 *   - 区域必须已创建。
 *   - 支持位图的大小和区域的大小不一致。
 *   - 位图从区域的(0,0)点开始加载。
 *   - 位图的像素格式必须和区域的像素格式一致。
 *   - 位图属性指针不能为空。
 *   - 支持多次调用。
 *   - 此接口只对 Overlay类型有效。
 *   - 调用了 #FH_RGN_GetCanvasInfo 之后，调用本接口无效，除非调用 #FH_RGN_UpdateCanvas 更新画布生效。
 *
 */
FH_SINT32 FH_RGN_SetBitMap(RGN_HANDLE Handle, const BITMAP_S *pstBitmap);

/**
 * @brief      将区域叠加到通道上。
 *
 * @param[in]  Handle           区域句柄号。取值范围：[0, RGN_HANDLE_MAX)。
 * @param[in]  vpss_grp         vpss group id。
 * @param[in]  vpss_chn         vpss channel id。
 * @param[in]  pstChnAttr       区域通道显示属性指针。
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,region_errcode} 。
 *
 * @note
 *   - 通道结构体指针不能为空。
 *   - 区域通道显示属性指针不能为空。
 *   - 支持多次叠加，但此接口不用于改变属性。
 *   - 同一个VPU chn只支持cover/mosaic中的一种类型。二选一，不能混合使用。
 *
 */
FH_SINT32 FH_RGN_AttachToChn(RGN_HANDLE Handle, FH_SINT32 vpss_grp, FH_SINT32 vpss_chn, const RGN_CHN_ATTR_S *pstChnAttr);

/**
 * @brief      将区域从通道中撤出。
 *
 * @param[in]  Handle           区域句柄号。取值范围：[0, RGN_HANDLE_MAX)。
 * @param[in]  vpss_grp         vpss group id。
 * @param[in]  vpss_chn         vpss channel id。
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,region_errcode} 。
 *
 * @note
 *   - 区域必须已创建。
 *   - 支持多次调用。
 *
 */
FH_SINT32 FH_RGN_DetachFromChn(RGN_HANDLE Handle, FH_SINT32 vpss_grp, FH_SINT32 vpss_chn);


/**
 * @brief      将区域叠加到设备组上。
 *
 * @param[in]  Handle      区域句柄号。
 * @param[in]  vpss_grp    vpss group
 * @param[in]  pstChnAttr  区域通道显示属性指针。
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,region_errcode} 。
 *
 * @note
 *   - 区域必须已创建。
 *   - 区域通道显示属性指针不能为空。
 *   - 支持多次叠加，但此接口不用于改变属性。
 *   - 将区域叠加到组上时，需使用此接口，通道不允许非法参数，否则叠加不成功。
 *
 */
FH_SINT32 FH_RGN_AttachToGroup(RGN_HANDLE Handle, FH_SINT32 vpss_grp, const RGN_CHN_ATTR_S *pstChnAttr);

/**
 * @brief      将区域从设备组中撤出。
 *
 * @param[in]  Handle      区域句柄号。
 * @param[in]  vpss_grp    vpss group
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,region_errcode} 。
 *
 * @note
 *   - 区域必须已创建。
 *   - 用于将区域从设备（组）中撤出；
 *   - 支持多次调用
 *
 */
FH_SINT32 FH_RGN_DetachFromGroup(RGN_HANDLE Handle, FH_SINT32 vpss_grp);

/**
 * @brief      设置区域的通道显示属性
 *
 * @param[in]  Handle      区域句柄号
 * @param[in]  vpss_grp    vpss group
 * @param[in]  vpss_chn    vpss chn
 * @param[in]  pstChnAttr  区域通道显示属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,region_errcode} 。
 *
 * @note
 *   - 区域必须已创建
 *   - 建议先获取属性，再设置
 *   - 区域通道显示属性指针不能为空
 *   - 区域必须先叠加到通道上
 *   - 静态属性不能修改，动态属性可以被修改
 *
 */
FH_SINT32 FH_RGN_SetChnDisplayAttr(RGN_HANDLE Handle, FH_SINT32 vpss_grp, FH_SINT32 vpss_chn, const RGN_CHN_ATTR_S *pstChnAttr);

/**
 * @brief      获取区域的通道显示属性
 *
 * @param[in]  Handle      区域句柄号
 * @param[in]  vpss_grp    vpss group
 * @param[in]  vpss_chn    vpss chn
 * @param[out] pstChnAttr  区域通道显示属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,region_errcode} 。
 *
 * @note
 *   - 区域必须已创建
 *   - 区域通道显示属性指针不能为空
 *
 */
FH_SINT32 FH_RGN_GetChnDisplayAttr(RGN_HANDLE Handle, FH_SINT32 vpss_grp, FH_SINT32 vpss_chn, RGN_CHN_ATTR_S *pstChnAttr);

/**
 * @brief      设置区域的设备（grp）显示属性。
 *
 * @param[in]  Handle      区域句柄号
 * @param[in]  vpss_grp    vpss group
 * @param[in]  pstChnAttr  区域通道显示属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,region_errcode} 。
 *
 * @note
 *   - 区域必须已创建
 *   - 建议先获取属性，再设置
 *   - 区域通道显示属性指针不能为空
 *   - 区域必须先叠加到通道上
 *   - 静态属性不能修改，动态属性可以被修改
 *
 */
FH_SINT32 FH_RGN_SetGroupDisplayAttr(RGN_HANDLE Handle, FH_SINT32 vpss_grp,  const RGN_CHN_ATTR_S *pstChnAttr);

/**
 * @brief      获取区域的设备（grp）显示属性
 *
 * @param[in]  Handle      区域句柄号
 * @param[in]  vpss_grp    vpss group
 * @param[in]  vpss_chn    vpss chn
 * @param[out] pstChnAttr  区域通道显示属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,region_errcode} 。
 *
 * @note
 *   - 区域必须已创建
 *   - 区域通道显示属性指针不能为空
 *
 */
FH_SINT32 FH_RGN_GetGroupDisplayAttr(RGN_HANDLE Handle, FH_SINT32 vpss_grp, RGN_CHN_ATTR_S *pstChnAttr);

/**
 * @brief      获取区域的显示画布信息。
 *
 * @param[in]  Handle           区域句柄号。取值范围：[0, RGN_HANDLE_MAX)。
 * @param[out] pstCanvasInfo    区域显示画布信息。
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,region_errcode} 。
 *
 * @note
 *   - 区域必须已创建。
 *   - 本接口与 #FH_RGN_SetBitMap 功能类似，主要用于 overlay类型导入位图数据。
 *      本接口相对于 #FH_RGN_SetBitMap 接口，用户可以直接更新显示内部画布数据，节省一次内存拷贝。
 *   - 本接口用于获取区域对应的画布信息，在得到画布地址之后，用户可直接对画布进行操作，譬如：将 bmp 数据直接填写到该画布中。
 *      然后通过调用 #FH_RGN_UpdateCanvas 接口，更新显示画布数据。
 *   - 本接口与 #FH_RGN_SetBitMap 接口互斥。如果已经使用了本接口，那么在调用 #FH_RGN_UpdateCanvas 前，
 *      调用 #FH_RGN_SetBitMap 不生效。
 *
 */
FH_SINT32 FH_RGN_GetCanvasInfo(RGN_HANDLE Handle, RGN_CANVAS_INFO_S *pstCanvasInfo);

/**
 * @brief      更新显示画布。
 *
 * @param[in]  Handle           区域句柄号。取值范围：[0, RGN_HANDLE_MAX)。
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,region_errcode} 。
 *
 * @note
 *   - 区域必须已创建。
 *   - 本接口配合 #FH_RGN_GetCanvasInfo 使用。主要用于画布内存数据更新之后，进行画布切换显示。
 *   - 每次调用本接口前，都必须先获取画布信息，然后在调用本接口进行更新。
 *   - 本接口仅支持 overlay类型的区域。
 *
 */
FH_SINT32 FH_RGN_UpdateCanvas(RGN_HANDLE Handle);




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __MPI_REGION_H__ */

