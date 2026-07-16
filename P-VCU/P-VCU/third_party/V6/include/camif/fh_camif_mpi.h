#ifndef __FH_CAMIF_MPI_H__
#define __FH_CAMIF_MPI_H__
#include <fh_camif_mpipara.h>
#include "types/type_def.h"


typedef int                     FY_S32;

/**
 * @brief         CAMIF 初始化函数
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败
 *
 * @note
 *    在CAMIF其他函数之前调用
 *
 */
FY_S32 MPI_CAMIF_INIT(void);
/**
 * @brief         配置接入sensor信息
 *
 * @param[in]     portConf        各种接入sensor的信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败
 *
 * @note
 *    在MPI_CAMIF_STREAMON之前调用
 *
 */
FY_S32 MPI_CAMIF_SET_PORT_INFO(Camif_Port_Conf_S portConf);
/**
 * @brief         使能CAMIF
 *
 * @param[in]     streamOn    streamon的模式：
 *                CAMIF_STREAMON_NORMAL:用于linux配置CAMIF，prerolling使能CAMIF的场景
                  CAMIF_STREAMON_TUNE_OFFLINE:用于linux配置和使能CAMIF，CAMIF数据输出到DDR的场景
                  CAMIF_STREAMON_TUNE_ONLINE:用于LinUX配置和使能CAIF，CAMIF数据直连ISP460的场景
 *
 * @retval  0              成功
 * @retval  "非0"          失败
 *
 * @note
 *
 */
FY_S32 MPI_CAMIF_STREAMON(Camif_StreamOn_S streamOn);
/**
 * @brief         发送buf给CAMIF
 *
 * @param[in]     rawecBuf    发送ae/awb/rawec buf给CAMIF
 *                CAMIF的BUF由应用管理，需要应用分配buf并发送给CAMIF使用，应用负责buf的分配和释放
 *
 * @retval  0              成功
 * @retval  "非0"          失败
 *
 * @note
 *              只有在CAMIF_STREAMON_TUNE_OFFLINE模式下需要调用
 *              在CAMIF_STREAMON_TUNE_OFFLINE模式下MPI_CAMIF_STREAMON之前调用之前提供buf给CAMIF
 *              在CAMIF_STREAMON_TUNE_OFFLINE模式下MPI_CAMIF_STREAMON之后，一般在MPI_CAMIF_GET_STREAM之后调用将buf还给CAMIF
 */
FY_S32 MPI_CAMIF_SET_BUF(Camif_Buf_S rawecBuf);
/**
 * @brief         设置AE的配置
 *
 * @param[in]     config              配置AE统计所需要的各种参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败
 *
 * @note
 *
 */
FY_S32 MPI_CAMIF_SET_AECONFIG(Camif_Ae_Conf_S config);
/**
 * @brief         设置AWB的配置
 *
 * @param[in]     config              配置AWB统计所需要的各种参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败
 *
 * @note
 *
 */
FY_S32 MPI_CAMIF_SET_AWBCONFIG(Camif_Awb_Conf_S config);

/**
 * @brief         获取AWB的配置
 *
 * @param[in]     config              配置AWB统计所需要的各种参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败
 *
 * @note
 *
 */
FY_S32 MPI_CAMIF_GET_AWBCONFIG(Camif_Awb_Conf_S *config);

/**
 * @brief         设置BLC的配置
 *
 * @param[in]     config              配置BLC统计所需要的各种参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败
 *
 * @note
 *
 */
FY_S32 MPI_CAMIF_SET_BLCCONFIG(Camif_Blc_Conf_S config);
/**
 * @brief         获取BLC的配置
 *
 * @param[in]     config              配置BLC统计所需要的各种参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败
 *
 * @note
 *
 */
FY_S32 MPI_CAMIF_GET_BLCCONFIG(Camif_Blc_Conf_S *config);

/**
 * @brief         获取CAMIF层的BUF
 *
 * @param[in]     pInfo              配置需要的数据类型并返回数据
 *
 * @retval  0              成功
 * @retval  "非0"          失败
 *
 * @note
 *       只有CAMIF_STREAMON_TUNE_OFFLINE有效
 *       获取stream后常通过MPI_CAMIF_SET_BUF将buf返还给CAMIF
 */
FY_S32 MPI_CAMIF_GET_STREAM(Camif_GetStream_S* pInfo);
/**
 * @brief         停止CAMIF
 *
 * @param[in]     streamOff              streamoff需要带接入的sensor类型
 *
 * @retval  0              成功
 * @retval  "非0"          失败
 *
 * @note
 */
FY_S32 MPI_CAMIF_STREAMOFF(Camif_StreamOff_S streamOff);
/**
 * @brief         设置裁剪信息
 *
 * @param[in]     iParam              设置裁剪的信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败
 *
 * @note
 */
FY_S32 MPI_CAMIF_SET_CROP(Camif_Crop_Conf_S iParam);
/**
 * @brief         设置幅面信息
 *
 * @param[in]     iParam              设置输入幅面的信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败
 *
 * @note
 */
FY_S32 MPI_CAMIF_SET_SIZE(Camif_Size_Conf_S iParam);
/**
 * @brief         设置格式信息
 *
 * @param[in]     iParam              设置输入数据格式信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败
 *
 * @note
 */
FY_S32 MPI_CAMIF_SET_FORMAT(Camif_Format_Conf_S iParam);
/**
 * @brief         设置丢帧信息
 *
 * @param[in]     iParam              设置开流后丢掉多少帧数据
 *
 * @retval  0              成功
 * @retval  "非0"          失败
 *
 * @note
 */
FY_S32 MPI_CAMIF_SET_SKIP_CNT(Camif_Skip_Frame_Cnt_S iParam);
/**
 * @brief         CAMIF DEinIT
 *
 * @param[in]     input_param              CAMIF deinit，释放资源，设置变量
 *
 * @retval  0              成功
 * @retval  "非0"          失败
 *
 * @note
 */
FY_S32 MPI_CAMIF_DEINIT(Camif_Deinit_S input_param);
#endif // __FH_CAMIF_MPI_H__
