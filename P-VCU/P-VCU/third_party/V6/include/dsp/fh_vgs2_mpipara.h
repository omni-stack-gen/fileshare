#ifndef __FH_VGS2_MPIPARA_H__
#define __FH_VGS2_MPIPARA_H__
/**|VGS|**/

#include "fh_common.h"
#include "fh_errno.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */
#pragma pack(4)

typedef enum
{
	FH_VGS_COMP_Y   = (1 << 0), //!< Y
	FH_VGS_COMP_U   = (1 << 1), //!< U/UV
	FH_VGS_COMP_V   = (1 << 2), //!< V
}FH_VGS_COMP;

typedef enum
{
	FH_VGS_WIDTH_1BIT    = 0, //!<  1BIT
	FH_VGS_WIDTH_2BIT    = 1, //!<  2BIT
	FH_VGS_WIDTH_4BIT    = 2, //!<  4BIT
	FH_VGS_WIDTH_8BIT    = 3, //!<  8BIT
	FH_VGS_WIDTH_16BIT   = 4, //!<  16BIT
	FH_VGS_WIDTH_32BIT   = 5, //!<  32BIT
}FH_VGS_PIX_WIDTH;

typedef struct {
	FH_UINT64 handle_idx;      //!<  任务标记
	FH_UINT32 module_idx;      //!<  硬件模块标记
	FH_UINT32 prior;           //!<  优先级(0: normal 1: high 2: highest)。取值范围: [0-2]
} VGS2_HANDLE;

typedef struct {
	FH_MEM_INFO  data_addr;    //!<  图像Y数据地址
	FH_MEM_INFO  data_addr_u;  //!<  图像U数据地址
	FH_MEM_INFO  data_addr_v;  //!<  图像V数据地址
	FH_SIZE      image_size;   //!<  图像幅面
	FH_POINT     start;        //!<  顶点坐标
	FH_UINT32    pool_id;      //!<  vb pool id(非VB模式建议配置为-1UL)
} VGS_IMAGE;

typedef struct {
	FH_MEM_INFO  data_yaddr;    //!<  图像Y数据地址
	FH_MEM_INFO  data_caddr;    //!<  图像C数据地址
	FH_SIZE      image_size;    //!<  图像幅面
	FH_POINT     start;         //!<  顶点坐标
	FH_UINT32    pool_id;       //!<  vb pool id(非VB模式建议配置为-1UL)
} VGS_IMAGE_DIV;

typedef struct
{
	FH_UINT32        prior;           //!<  优先级(0: normal 1: high 2: highest)。取值范围: [0-2]
	VGS_IMAGE_DIV    src_data;        //!<  输入图像数据
	VGS_IMAGE_DIV    dst_data;        //!<  输出图像数据
	FH_SIZE          op_size;         //!<  待处理宽高
	FH_VPU_VO_MODE   src_format;      //!<  输入图像格式
	FH_UINT32        src_comp;        //!<  采样格式(bit0:Y bit1:C)。取值范围: [1-3]
	FH_VPU_VO_MODE   dst_format;      //!<  输出图像格式
} VGS_V2_CVT_CTRL;

typedef struct
{
	FH_UINT32        prior;             //!<  优先级(0: normal 1: high 2: highest)。取值范围: [0-2]
	VGS_IMAGE        src_data;          //!<  输入图像数据
	VGS_IMAGE        dst_data;          //!<  输出图像数据
	FH_VPU_VO_MODE   src_format;        //!<  输入图像格式
	FH_UINT32        src_comp;          //!<  采样格式(FH_VGS_COMP)
	FH_UINT32        mode;              //!<  马赛克格式(0:单色 1:马赛克)。取值范围: [0-1]
	FH_UINT32        size;              //!<  马赛克尺寸(0:16x16,1:32x32,2:64x64 3:128x128)。取值范围: [0-3]
	FH_AREA          mask;              //!<  马赛克配置
} VGS_V2_PM_CTRL;

typedef struct
{
	FH_UINT32        prior;             //!<  优先级(0: normal 1: high 2: highest)。取值范围: [0-2]
	VGS_IMAGE        src_data;          //!<  输入图像数据
	VGS_IMAGE        dst_data;          //!<  输出图像数据
	FH_SIZE          op_size;           //!<  待处理宽高
	FH_VPU_VO_MODE   src_format;        //!<  输入图像格式
	FH_UINT32        src_comp;          //!<  采样格式(FH_VGS_COMP)
	FH_VGS_PIX_WIDTH src_pixw;          //!<  图像位宽
	FH_ROTATE_OPS    rotate;            //!<  旋转角度
	FH_UINT32        rotmode;           //!<  422-420格式
} VGS_V2_ROT_CTRL;

typedef struct
{
	FH_UINT32        prior;             //!<  优先级(0: normal 1: high 2: highest)。取值范围: [0-2]
	VGS_IMAGE        src_data0;         //!<  输入图像数据
	VGS_IMAGE        src_data1;         //!<  输入图像数据
	VGS_IMAGE        dst_data;          //!<  输出图像数据
	FH_SIZE          op_size0;          //!<  待处理宽高
	FH_SIZE          op_size1;          //!<  待处理宽高
	FH_SIZE          op_size;           //!<  待处理宽高
	FH_VPU_VO_MODE   src_format;        //!<  输入图像格式
	FH_UINT32        src_comp;          //!<  采样格式(FH_VGS_COMP)
} VGS_V2_JNT_CTRL;

typedef struct
{
	FH_UINT32        prior;             //!<  优先级(0: normal 1: high 2: highest)。取值范围: [0-2]
	VGS_IMAGE        pic_src;           //!<  输入图像数据
	FH_SIZE          op_size_src;       //!<  待处理宽高
	VGS_IMAGE        pic_dst;           //!<  输出图像数据
	FH_SIZE          op_size_dst;       //!<  待处理宽高
	FH_VPU_VO_MODE   src_format;        //!<  输入图像格式
	FH_UINT32        src_comp;          //!<  采样格式(FH_VGS_COMP)
}VGS_V2_SCL_CTRL;

typedef struct
{
	FH_UINT32        prior;             //!<  优先级(0: normal 1: high 2: highest)。取值范围: [0-2]
	VGS_IMAGE        pic_src;           //!<  输入图像数据
	VGS_IMAGE        pic_dst;           //!<  输出图像数据
	FH_SIZE          op_size;           //!<  待处理宽高
	FH_VPU_VO_MODE   src_format;        //!<  输入图像格式
	FH_UINT32        mode;              //!<  模式(0:腐蚀 1:膨胀)。取值范围: [0-1]
	FH_UINT32        masken;            //!<  5x5窗屏蔽使能。取值范围: [0-1]
}VGS_V2_CNE_CTRL;

typedef struct
{
	FH_UINT32        prior;             //!<  优先级(0: normal 1: high 2: highest)。取值范围: [0-2]
	VGS_IMAGE        pic_src;           //!<  输入图像数据
	VGS_IMAGE        pic_dst;           //!<  输出图像数据
	FH_SIZE          op_size;           //!<  待处理宽高
	FH_VPU_VO_MODE   src_format;        //!<  输入图像格式
	FH_UINT32        src_comp;          //!<  采样格式(FH_VGS_COMP)
}VGS_V2_INTG_CTRL;

typedef struct
{
	FH_UINT32        prior;             //!<  优先级(0: normal 1: high 2: highest)。取值范围: [0-2]
	VGS_IMAGE        pic_src0;          //!<  输入图像数据A
	VGS_IMAGE        pic_src1;          //!<  输入图像数据B
	VGS_IMAGE        pic_src2;          //!<  输入图像数据C
	VGS_IMAGE        pic_dst;           //!<  输出图像数据
	FH_SIZE          op_size;           //!<  待处理宽高
	FH_VPU_VO_MODE   src_format;        //!<  输入图像格式
	FH_UINT32        src_comp;          //!<  采样格式(FH_VGS_COMP)
	FH_UINT32        mode;              //!<  运算模式。取值范围: [0-3]
}VGS_V2_ADD_CTRL;

typedef struct
{
	FH_UINT32        prior;             //!<  优先级(0: normal 1: high 2: highest)。取值范围: [0-2]
	VGS_IMAGE        pic_src0;          //!<  输入图像数据A
	VGS_IMAGE        pic_src1;          //!<  输入图像数据B
	VGS_IMAGE        pic_src2;          //!<  输入图像数据C
	VGS_IMAGE        pic_w1;            //!<  外存输入图像权重
	VGS_IMAGE        pic_w2;            //!<  外存输入图像权重
	VGS_IMAGE        pic_dst;           //!<  输出图像数据
	FH_SIZE          op_size;           //!<  待处理宽高
	FH_VPU_VO_MODE   src_format;        //!<  输入图像格式
	FH_UINT32        src_comp;          //!<  采样格式(FH_VGS_COMP)
	FH_UINT32        mode;              //!<  模式(0:merge 1:average 2:alpha blending 3:multiply)。取值范围: [0-3]
	FH_UINT32        avgrad;            //!<  均值滤波窗口半径
	FH_UINT32        avgmode;           //!<  均值滤波模式。取值范围: [0-1]
	FH_UINT32        wtype;             //!<  权重获取方式(0:外存 1:行拷贝)。取值范围: [0-1]
	FH_UINT32        absign;            //!<  alpha blending输入格式(0:无符号8bit 1:有符号16bit)。取值范围: [0-1]
	FH_UINT32        abmode;            //!<  alpha blending模式。取值范围: [0-1]
	FH_UINT32        mulmode;           //!<  乘法模式。取值范围: [0-1]
	FH_UINT32        w1[128];           //!<  行拷贝输入图像权重
	FH_UINT32        w2[128];           //!<  行拷贝输入图像权重
}VGS_V2_MRG_CTRL;

typedef struct
{
	FH_UINT32        prior;             //!<  优先级(0: normal 1: high 2: highest)。取值范围: [0-2]
	VGS_IMAGE        pic_src;           //!<  输入OSD数据
	VGS_IMAGE        pic_dst;           //!<  输出OSD数据
	FH_SIZE          op_size;           //!<  待处理宽高
	FH_POINT         osd_start;         //!<  数据起点
	FH_VGS_PIX_WIDTH pixw;              //!<  数据位宽
	FH_ROTATE_OPS    rotate;            //!<  数据旋转角度
}VGS_V2_OSD_CVT_CTRL;

typedef struct
{
	FH_UINT32        prior;             //!<  优先级(0: normal 1: high 2: highest)。取值范围: [0-2]
	VGS_IMAGE_DIV    pic_src;           //!<  输入图像数据
	VGS_IMAGE        osd;               //!<  OSD数据
	VGS_IMAGE_DIV    pic_dst;           //!<  输出图像数据
	FH_SIZE          op_size;           //!<  待处理宽高
	FH_VPU_VO_MODE   src_format;        //!<  输入图像格式
	FH_UINT32        src_comp;          //!<  采样格式(bit0:Y bit1:C)。取值范围: [1-3]
	FH_VGS_PIX_WIDTH osdpixw;           //!<  OSD位宽
	FH_UINT32        extmode;           //!<  0为LSB填0，1为MSB复制到LSB。取值范围: [0-1]
	FH_UINT32        magiccoloren;      //!<  是否使用magic_color,仅在16bit模式下有效。取值范围: [0-1]
	FH_UINT32        magiccolor;        //!<  magic_color的值,16bit,此颜色将作为透明。取值范围: [0-0xffff]
	FH_UINT32        alphaen;           //!<  全局透明度使能。取值范围: [0-1]
	FH_UINT32        alpha;             //!<  全局透明度。取值范围: [0-255]
	FH_UINT32        color[256];        //!<  颜色索引库,ARGB8888
}VGS_V2_OSD_OL_CTRL;


/** @brief 0xA03F8006: 输入包含空指针 */
#define FH_ERR_VGS2_NULL_PTR         FH_DEF_ERR(FH_ID_VGS2, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/** @brief 0xA03F8003: 无效参数 */
#define FH_ERR_VGS2_ILLEGAL_PARAM    FH_DEF_ERR(FH_ID_VGS2, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/** @brief 0xA03F8009: 不允许的操作 */
#define FH_ERR_VGS2_NOT_PERMITTED    FH_DEF_ERR(FH_ID_VGS2, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/** @brief 0xA03F8007: 使用前未配置 */
#define FH_ERR_VGS2_NOT_CONFIG       FH_DEF_ERR(FH_ID_VGS2, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/** @brief 0xA03F800A: 等待超时 */
#define FH_ERR_VGS2_TIMEOUT          FH_DEF_ERR(FH_ID_VGS2, EN_ERR_LEVEL_ERROR, EN_ERR_TIMEOUT)
/** @brief 0xA03F8008: 不支持的操作 */
#define FH_ERR_VGS2_NOT_SUPPORT      FH_DEF_ERR(FH_ID_VGS2, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/** @brief 0xA03F8011: 非法地址 */
#define FH_ERR_VGS2_BADADDR          FH_DEF_ERR(FH_ID_VGS2, EN_ERR_LEVEL_ERROR, EN_ERR_BADADDR)
/** @brief 0xA03F800F: 内存已满 */
#define FH_ERR_VGS2_BUF_FULL         FH_DEF_ERR(FH_ID_VGS2, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/** @brief 0xA03F8005: VGS2未初始化 */
#define FH_ERR_VGS2_UNEXIST          FH_DEF_ERR(FH_ID_VGS2, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/** @brief 0xA03F8010: 系统未准备好 */
#define FH_ERR_VGS2_SYS_NOTREADY     FH_DEF_ERR(FH_ID_VGS2, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)


#pragma pack()
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /*__FH_VGS2_MPIPARA_H__ */

