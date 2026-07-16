/*
 * fh_audio_mpi.h
 *
 *  Created on: 2015/2/15
 *      Author: fanggm
 */

#ifndef FH_AC_MPI_H_
#define FH_AC_MPI_H_

#include "types/type_def.h"
#include "fh_audio_mpipara.h"
#include "FHSES_param.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/*
 * 以下内容,仅供参考,请以SDK发布文档<<音频函数开发参考手册.pdf>>为准
 */

/**
*@brief 音频系统初始化，完成打开驱动设备
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
FH_SINT32 FH_AC_Init(void);

/**
 * *@brief 外部codec音频系统初始化，完成打开驱动设备
 * *@return 是否成功
 * * - RETURN_OK(0):  成功
 * * - 其他：失败,错误详见错误号
 */
FH_SINT32 FH_AC_Init_WithExternalCodec(void);


FH_SINT32 FH_AC_AI_PowerDown_MicBias(FH_UINT32 channel_mask);


/**
*@brief 音频系统释放资源，关闭音频设备
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
FH_SINT32 FH_AC_DeInit(void);

/**
*@brief 设置AI、AO设备参数
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
FH_SINT32 FH_AC_Set_Config(FH_AC_CONFIG *pstConfig);

/**
*@brief 设置AI、AO设备参数
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
FH_SINT32 FH_AC_Set_Config_Ext(FH_AC_CONFIG *pstConfig, FH_UINT32 frame_num);

/**
*@brief 使能AI设备
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
FH_SINT32 FH_AC_AI_Enable(void);

/**
*@brief 禁用AI设备
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
FH_SINT32 FH_AC_AI_Disable(void);

/**
*@brief 使能AO设备
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
FH_SINT32 FH_AC_AO_Enable(void);


/*
 * 硬件时序上同时使能AI/AO,做AEC回声消除的时候,为了
 * 保证对讲的效果,必须调用这个函数同时使能AI/AO
 *
 * 注意: 如果调用FH_AC_AI_Enable/FH_AC_AO_Enable使能,
 *       那么回声消除模块不会起作用.
 */
FH_SINT32 FH_AC_AI_AO_SYNC_Enable(void);

/**
 * Set AO's workmode, only support for FH8856
 */
FH_SINT32 FH_AC_AO_Set_Mode(FH_AC_AO_MODE_E mode);

/**
*@brief 禁用AO设备
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
FH_SINT32 FH_AC_AO_Disable(void);

/**
*@brief 暂停AI设备运行
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
FH_SINT32 FH_AC_AI_Pause(void);

/**
*@brief 恢复AI设备运行
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
FH_SINT32 FH_AC_AI_Resume(void);

/**
*@brief 暂停AO设备运行
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
FH_SINT32 FH_AC_AO_Pause(void);

/**
*@brief 恢复AO设备运行
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
FH_SINT32 FH_AC_AO_Resume(void);

/**
*@brief 设置AI设备音量大小
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
FH_SINT32 FH_AC_AI_SetVol(FH_SINT32 volume);

FH_SINT32 FH_AC_AI_MICIN_SetVol(FH_SINT32 volume);

FH_SINT32 FH_AC_AO_SetVol(FH_SINT32 volume);

FH_SINT32 FH_AC_AO_SetDigitalVol(FH_SINT32 volume);

/**
*@brief 获取一帧音频数据
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
FH_SINT32 FH_AC_AI_GetFrame(FH_AC_FRAME_S *pstFrame);

/*
 * this function support PTS, it increase 1000000 per second...
 * don't use FH_AC_AI_GetFrameWithPts and FH_AC_AI_GetFrame the same time.
 * Only use one of them is OK!!!!
 */
FH_SINT32 FH_AC_AI_GetFrameWithPts(FH_AC_FRAME_S *pstFrame, FH_UINT64 *pPts);


/*Fast version, recomended to use this one...*/
FH_SINT32 FH_AC_AI_GetFrameWithPtsFast(FH_AC_FRAME_S *pstFrame, FH_UINT64 *pPts);


/**
*@brief 发送一帧音频数据
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
FH_SINT32 FH_AC_AO_SendFrame(FH_AC_FRAME_S *pstFrame);

/*
 * extended ioctl interface, for special case, don't use it...
 */
FH_SINT32 FH_AC_Ext_Ioctl(FH_AC_EXT_IOCTL_CMD *pIoctl);
FH_SINT32 FH_AC_Ext2_Ioctl(FH_UINT32 cmd, FH_UINT32 subcmd, void *param,  FH_UINT32 len);

FH_SINT32 FH_AC_Raw_SetConfig(FH_AC_Raw_CONFIG *pRawCfg);
FH_SINT32 FH_AC_Raw_GetFrameFast(FH_AC_Raw_S *pRawInfo);

FH_SINT32 FH_AC_AI_Bind(FH_UINT32 channel);

FH_SINT32 FH_AC_AI_CH_SetAnologVol (FH_UINT32 channel_mask, FH_UINT32 mic_vol, FH_UINT32 vol);

/*
 * 功能: 设置多个音频输入通道的数字增益
 * vol:  取值范围[0-5], 0[0db], 1[1.2db], 2[2.4db], 3[3.6db], 4[4.8db], 5[6db]
 *       如果此函数不调用,那么就是default值0db.
 * 返回值: 错误码
 */
FH_SINT32 FH_AC_AI_CH_SetDigitalVol(FH_UINT32 channel_mask, FH_UINT32 vol);

/*
 * method, 当前没有用，置为0即可
 * level, 取值范围[0，3]， 缺省值为2
 */
FH_SINT32 FH_AC_AI_SetMixMethod(FH_UINT32 method, FH_UINT32 level);

FH_SINT32 FH_AC_AI_QueryBufSize(FH_UINT32 *pBufSize);
FH_SINT32 FH_AC_AO_QueryBufSize(FH_UINT32 *pBufSize);

FH_SINT32 FH_AC_AI_ClearBuf(void);
FH_SINT32 FH_AC_AO_ClearBuf(void);
FH_SINT32 FH_AC_AO_WaitPlayComplete(void);

FH_SINT32 FH_AC_AI_Get_Algo_Param(void *param, FH_UINT32 len);
FH_SINT32 FH_AC_AO_Get_Algo_Param(void *param, FH_UINT32 len);
FH_SINT32 FH_AC_AI_Set_Algo_Param(void *param, FH_UINT32 len);
FH_SINT32 FH_AC_AO_Set_Algo_Param(void *param, FH_UINT32 len);
FH_SINT32 FH_AC_Print_Algo_Param(void *param, FH_UINT32 len);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* FH_AC_MPI_H_ */
