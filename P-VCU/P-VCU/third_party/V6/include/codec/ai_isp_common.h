/*
 * ai_isp_common.h
 *
 *  Created on: 2024-04-23
 *      Author: zhansc427
 */
#ifndef __FH_AI_ISP_COMMON_H_
#define __FH_AI_ISP_COMMON_H_

#include "types/type_def.h"
#include "dsp/fh_common.h"
#define MALLOCED_MEM_BASE_AIISP 2
typedef struct _ISP_AiIsp_CFG_S_
{
    FH_UINT8  ctrl_mode;// AI控制模式 0:ByPASS 1:Gain mapping 2: Manual mode

    FH_UINT8  u08lumaScale;//手动模式下 控制画面明亮程度，值越高越亮，值越高噪声越大，建议值100~150
    FH_UINT16 u16shift;     //手动模式下 对应当前isp链路的BLC
    FH_UINT8  u08bgStable;//手动模式下 背景噪声稳定程度上限，值越高画面越稳定但可能更新缓慢，建议范围50~100
    FH_UINT8  u08sharpScale;//手动模式下 背景的降噪力度，值越高降噪力度越强，清晰度越低，建议值0~50
    FH_UINT8  u08ispScale;//手动模式下 运动前景降噪强度，值越高降噪力度越强，建议值0~100，(sharpScale+ispScale <=100)
    FH_UINT8  u08Threshold;//手动模式下 背景噪声稳定程度，值越大越稳定，建议100~150
    FH_SINT8  s08bgScale;// 手动模式下运动前景的透明程度，值越大越穿透，建议值-50~50

    FH_UINT8  u08lumaScaleMap[12];// 控制画面明亮程度，值越高越亮，值越高噪声越大，建议值100~150
    FH_UINT16 u16shiftMap[12];// 对应当前isp链路的BLC
    FH_UINT8  u08bgStableMap[12];// 背景噪声稳定程度上限，值越高画面越稳定但可能更新缓慢，建议范围50~100
    FH_UINT8  u08sharpScaleMap[12];// 背景的降噪力度，值越高降噪力度越强，清晰度越低，建议值0~50
    FH_UINT8  u08ispScaleMap[12];// 0db时，运动前景降噪强度，值越高降噪力度越强，建议值0~100，(sharpScale+ispScale <=100)
    FH_UINT8  u08ThresholdMap[12];// 背景噪声稳定程度，值越大越稳定，建议100~150
    FH_SINT8  s08bgScaleMap[12];// 运动前景的透明程度，值越大越穿透，建议值-50~50
} ISP_AiIsp_CFG;

int API_ISP_SetAiIspCfg(FH_UINT32 u32IspDevId, ISP_AiIsp_CFG *pstAiIspCfg);

int API_ISP_GetAiIspCfg(FH_UINT32 u32IspDevId, ISP_AiIsp_CFG *pstAiIspCfg);

FH_SINT32 API_ISP_LoadAiIspParam(FH_UINT32 u32IspDevId, char *isp_param_buff);

/*
*   Name: FH_AIISP_WriteMallocedMem
*            写指定偏移的VMM分配的内存的值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_SINT32 intMemSlot
*            分配的内存的类型，决定基地址。
*
*       [IN]  FH_UINT32 offset
*            偏移地址，选定的内存会给定其基地址。
*
*       [IN]  FH_UINT32 *pstData
*            目标值，将该值写入目标地址。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 FH_AIISP_WriteMallocedMem(FH_UINT32 grpidx,FH_SINT32 intMemSlot, FH_UINT32 offset, FH_UINT32 *pstData);

/*
*   Name: FH_AIISP_ReadMallocedMem
*            读取指定偏移的VMM分配的内存的值
*
*   Parameters:
*
*       [IN]  FH_UINT32 u32IspDevId
*            ISP的设备号
*
*       [IN] FH_SINT32 intMemSlot
*            分配的内存的类型，决定基地址。
*
*       [IN]  FH_UINT32 offset
*            偏移地址，选定的内存会给定其基地址。
*
*       [OUT]  FH_UINT32 *pstData
*            存放读取到数据的地址。
*
*   Return:
*            0(正确)
*            非0(失败，详见错误码)
*   Note:
*       无
*/
FH_SINT32 FH_AIISP_ReadMallocedMem(FH_UINT32 grpidx,FH_SINT32 intMemSlot, FH_UINT32 offset, FH_UINT32 *pstData);

#endif /* __FH_AI_ISP_COMMON_H_ */
