#ifndef __FH_VICAP_MPI_H__
#define __FH_VICAP_MPI_H__

#include "types/type_def.h"

typedef FH_UINT32 VI_DEV;
#define VICAP_MAX_SENSOR 3
#define FH_VICAP_CALLBACK_MAXNUM 10
/**|VICAP|**/
typedef enum
{
    VICAP_SNS0 = 0, //!< sensor0的序号
    VICAP_SNS1 = 1, //!< sensor1的序号
    VICAP_SNS2 = 2, //!< sensor2的序号
}FH_VICAP_SNS_IDX_E;

typedef enum 
{
    VICAP_PIPELINE_MIPI0 = 0,   //!< MIPI0序号，本项目对应VICAP_SNS0
    VICAP_PIPELINE_MIPI1 = 1,   //!< MIPI1序号，本项目对应VICAP_SNS1
    VICAP_PIPELINE_DVP = 2,     //!< DVP序号，本项目对应VICAP_SNS2
    VICAP_PIPELINE_TESTMODE = 3,    //!< 在线模式VICAP测试模式，离线模式下不支持
    VICAP_PIPELINE_VIRDEV0 = 4,
    VICAP_PIPELINE_VIRDEV1 = 5,
    VICAP_PIPELINE_VIRDEV2 = 6,
    VICAP_PIPELINE_NUM = 7,
}FH_VICAP_IDX_E;

typedef enum 
{
    VICAP_STITCH_FREE_RUN     = 0,     //!< sns非同步模式时的拼接
    VICAP_STITCH_SYNC_MODE     = 1,     //!< sns同步模式时的拼接
    VICAP_STITCH_LARGE_PIC_MODE = 2,     //!< 硬件大图处理模式
    VICAP_STITCH_BUTT,
}FH_VICAP_STITCH_MODE_E;

typedef enum 
{
    VICAP_WORK_MODE_ONLINE     = 0,     //!< vicap工作在在线模式
    VICAP_WORK_MODE_OFFLINE    = 1,     //!< vicap工作在离线模式
    VICAP_WORK_MODE_BUTT,
}FH_VICAP_WORK_MODE_E;

typedef enum 
{
    VICAP_SFRAME = 0,   //!< 短帧通道号
    VICAP_LFRAME = 1,   //!< 长帧通道号
}FH_VICAP_WDR_CHAN_E;

typedef enum
{
	VICAP_WDR_LINEAR,
	VICAP_WDR_WDR,
	VICAP_WDR_SONY_WDR,
	VICAP_WDR_NUM
}FH_VICAP_WDR_MODE_E;

typedef enum  {
    VICAP_BAYER_RGGB = 0x0, //!< 色彩模式RGGB
    VICAP_BAYER_GRBG = 0x1, //!< 色彩模式GRBG
    VICAP_BAYER_BGGR = 0x2, //!< 色彩模式BGGR
    VICAP_BAYER_GBRG = 0x3, //!< 色彩模式GBRG
    VICAP_BAYER_TYPE_DUMMY=0xffffffff,
} FH_VICAP_BAYER_TYPE_E;

typedef enum  {
    FH_VICAP_VSYNC_ONLY = 0,  //!< 只有垂直同步信号
    FH_VICAP_VSYNC_HSYNC = 1,  //!< 垂直同步信号 + 水平同步信号
} FH_VICAP_SYNC_MODE_E;

typedef enum  {
    FH_VICAP_VSYNC_TRIGGER_INTER = 0,  //!< 周期性发送vsync和hsync
    FH_VICAP_VSYNC_TRIGGER_EXTER = 1,  //!< 外部触发发送一个周期的vsync和hsync
} FH_VICAP_SYNC_TRIGGER_MODE_E;

typedef enum  {
    VICAP_DATA_IN_RAW12 = 0,      //!< 输入的数据格式为RAW12
    VICAP_DATA_IN_RAW16 = 1,      //!< 输入的数据格式为RAW16
    VICAP_DATA_IN_YUV16 = 2,      //!< 输入的数据格式为YUV16
    VICAP_DATA_IN_YUYV = 3,      //!< 输入的数据格式为YUYV
    VICAP_DATA_IN_UYVY = 4,      //!< 输入的数据格式为UYVY
    VICAP_DATA_IN_BUTT,
} FH_VICAP_INPUT_DATA_TYPE_E;

typedef enum  {
    FH_VICAP_DVP_BIT_MODE_16BIT_YUV = 0,    //!< DVP输入格式为YUV16
    FH_VICAP_DVP_BIT_MODE_12BIT_RAW = 1,    //!< DVP输入格式为RAW12
    FH_VICAP_DVP_BIT_MODE_10BIT_RAW = 2,    //!< DVP输入格式为RAW10
}FH_VICAP_DVP_IN_BIT_MODE_E;

typedef enum  {
    FH_VICAP_BT656_BIT_MODE_16BIT_YUV = 0,    //!< BT656输入格式为YUV16
    FH_VICAP_BT656_BIT_MODE_8BIT_YUV = 1,    //!< BT656输入格式为RAW12
}FH_VICAP_BT656_IN_BIT_MODE_E;


typedef enum 
{
    FH_VICAP_DVP_SYNC_POL_HIGH = 0,     //!< 同步信号高有效
    FH_VICAP_DVP_SYNC_POL_LOW  = 1,     //!< 同步信号低有效
}FH_VICAP_SYNC_POL_E;

typedef struct
{
    FH_UINT16    u16Width;   //!< 图像的宽度。取值范围：[0~0xffff]
    FH_UINT16    u16Height;  //!< 图像的高度。取值范围：[0~0xffff]
}FH_VICAP_SIZE_S;

typedef struct
{
    FH_UINT16    u16X;       //!< 图像裁剪的水平偏移值。取值范围：[0~0xffff]
    FH_UINT16    u16Y;       //!< 图像裁剪的垂直偏移值。取值范围：[0~0xffff]
    FH_UINT16    u16Width;   //!< 图像裁剪后的宽度。取值范围：[0~0xffff]
    FH_UINT16    u16Height;  //!< 图像裁剪后的高度。取值范围：[0~0xffff]
}FH_VICAP_RECT_S;

typedef struct
{
    FH_BOOL            bCutEnable;  //!< 裁剪的使能寄存器。取值范围：[0~1]
    FH_VICAP_RECT_S    stRect;      //!< 裁剪的偏移及裁剪后的宽高。取值范围：[FH_VICAP_RECT_S]
}FH_VICAP_CROP_CFG_S;

typedef struct
{
    FH_UINT8 u8Priority;    //!< 离线遍历优先级。取值范围：[0~0xf]
}FH_VICAP_OFFLINE_CFG_S;

typedef struct
{
    FH_VICAP_BAYER_TYPE_E     enBayerType;   //!< bayer pattern。取值范围：[FH_VICAP_BAYER_TYPE_E]
    FH_VICAP_WORK_MODE_E      enWorkMode;    //!< 配置当前vicap的工作模式。取值范围：[FH_VICAP_WORK_MODE_E]
    FH_UINT8                  u8WdrMode;     //!< 输入sensor的线性还是wdr配置。取值范围：[0~1]
    FH_VICAP_SIZE_S           stInSize;      //!< 输入幅面。取值范围：[FH_VICAP_SIZE_S]
    FH_VICAP_CROP_CFG_S       stCropSize;    //!< 裁剪后幅面。取值范围：[FH_VICAP_CROP_CFG_S]
    FH_VICAP_OFFLINE_CFG_S    stOfflineCfg;  //!< 离线下的配置。取值范围：[FH_VICAP_OFFLINE_CFG_S]
}FH_VICAP_VI_ATTR_S;

typedef struct
{
    FH_BOOL                     bUsingVb;       //!< 是否使用VB缓存逻辑。取值范围：[0~1]
    FH_VICAP_INPUT_DATA_TYPE_E  enDataTypeIn;   //!< SENSOR输入该模块的数据格式，本芯片仅DVP有效。取值范围： [FH_VICAP_INPUT_DATA_TYPE_E]
    FH_VICAP_WORK_MODE_E        enWorkMode;     //!< 配置当前vicap的工作模式，影响内存分配。取值范围：[FH_VICAP_WORK_MODE_E]
    FH_VICAP_SIZE_S             stSize;         //!< 图像幅面, 会影响内存分配, 故需按应用需要的最大幅面配置。取值范围：[FH_VICAP_SIZE_S]
}FH_VICAP_DEV_ATTR_S;

typedef struct
{
    FH_VICAP_DVP_IN_BIT_MODE_E    enBitMode;        //!< 配置当前输出模式是raw还是yuv等。取值范围：[FH_VICAP_DVP_IN_BIT_MODE_E]
    FH_VICAP_SYNC_POL_E           enHsyncPolarity;  //!< DVP水平同步信号极性。取值范围：[FH_VICAP_SYNC_POL_E]
    FH_VICAP_SYNC_POL_E           enVsyncPolarity;  //!< DVP垂直同步信号极性。取值范围：[FH_VICAP_SYNC_POL_E]
    FH_UINT8                      u8IoRemap[16];    //!< 对应的16个管脚的映射关系。取值范围：[0~0xf]
}FH_VICAP_DVP_CFG_S;

typedef struct
{
    FH_VICAP_BT656_IN_BIT_MODE_E    enBitMode;        //!< 配置当前输出模式是raw还是yuv等。取值范围：[FH_VICAP_DVP_IN_BIT_MODE_E]
    FH_VICAP_SYNC_POL_E           enHsyncPolarity;  //!< DVP水平同步信号极性。取值范围：[FH_VICAP_SYNC_POL_E]
    FH_VICAP_SYNC_POL_E           enVsyncPolarity;  //!< DVP垂直同步信号极性。取值范围：[FH_VICAP_SYNC_POL_E]
    FH_UINT8                      u8IoRemap[16];    //!< 对应的16个管脚的映射关系。取值范围：[0~0xf]
}FH_VICAP_BT656_CFG_S;

typedef struct
{
    FH_VICAP_SYNC_POL_E           enHsyncPolarity;  //!< DVP水平同步信号极性。取值范围：[FH_VICAP_SYNC_POL_E]
    FH_VICAP_SYNC_POL_E           enVsyncPolarity;  //!< DVP垂直同步信号极性。取值范围：[FH_VICAP_SYNC_POL_E]
    FH_UINT8                      u8IoRevert;       //!< dvp管脚是否revert
    FH_UINT32 Ir_Fs_High_Period;                    //!<fs高电平持续时间
    FH_UINT32 Ir_Fs_Low_Period;                     //!<fs低电平持续时间
    FH_UINT32 Ir_Reg_Output_Delay_Period;           //!<ir静态寄存器输出延迟时间
    FH_UINT32 Ir_Ooc_Output_Delay_Period;           //!<ir ooc输出延迟时间
    FH_UINT32 Ir_Ooc_Tline_Active_Pixel;            //!<ir ooc行有效时间
    FH_UINT32 Ir_Ooc_Tline_Inactive_Period;         //!<ir ooc行间隔时间
    FH_UINT32 Ir_Ooc_Tline_Active_Line;             //!<ir ooc行有效行数
    FH_UINT32 Ir_Clk_Tx_Inv_En;                     //!<输出给IR传感器的时钟是否反向
    FH_UINT32 En_Ir_Gray2bin;                       //!<grap码转二进制使能
}FH_VICAP_IR_CFG_S;

typedef struct
{
	FH_UINT32   u32DevId;      //!< 该路的通道号。取值范围：[0~3]
    FH_UINT32 u32PhyAddr;  //!< 数据存放地址
    FH_UINT16 u16FrameRate;  //!< 发送帧率
}FH_VICAP_VIRDEV_CFG_S;


typedef struct
{
    FH_UINT8    u8GroupId;     //!< 分组拼接的序号。取值范围：[0~0xf]
    FH_UINT32   u32DevId;      //!< 该路的通道号。取值范围：[0~3]
    FH_UINT8    u8StitchMode;  //!< 该路的拼接模式。取值范围：[0~2]
    FH_BOOL     bMainDev;      //!< 该路是否为mainPipe。取值范围：[0~1]
    FH_BOOL     bStitchEn;     //!< 该路是否开启拼接时能。取值范围：[0~1]
}FH_VICAP_STITCH_ATTR_S;

typedef struct
{
    FH_UINT8    u8GroupId;     //!< 拼接组的序号
    FH_UINT8    u8StitchMode;  //!< 拼接模式
    FH_BOOL     bStitchEn;     //!< 是否拼接
    FH_UINT32   u32MainDevId;  //!< 主通路的id
    FH_UINT32   u32DevNum;     //!< 拼接组的通路个数
    FH_UINT32   au32DevId[VICAP_MAX_SENSOR];//!<各个通路的id
}FH_VICAP_STITCH_GRP_ATTR_S;


typedef struct
{
    FH_UINT8 u8DevNum;         //!< 总共的vicap通路。取值范围：[0~VICAP_MAX_SENSOR]
    struct _buf_size_cfg
    {
        FH_UINT8            u8DevId;    //!< 设备号。取值范围：[0~VICAP_MAX_SENSOR]
        FH_VICAP_DEV_ATTR_S stDevAttr;  //!< 设备参数。取值范围：[FH_VICAP_DEV_ATTR_S]
        FH_UINT32           u32BufSize;  //!< 获取到的缓存参数。取值范围：[0~0xffffffff]
    } astBufCfg[VICAP_MAX_SENSOR];
}FH_VICAP_BUF_SIZE_ATTR_S;

typedef struct
{
    FH_VICAP_SNS_IDX_E enSnsIdx;
    FH_VICAP_SYNC_MODE_E enSyncMode;         //!< 选择vsync only还是vsync + hsync
    FH_UINT16   u16VsyncPeriod;              //!< 垂直触发信号的一个周期的cycle数。取值范围：[0~0x1fff]
    FH_UINT16   u16VsyncDuty;                //!< 垂直触发信号的有效电平cycle数  。取值范围：[0~0x1fff]
    FH_BOOL     bVsyncPolatiry;              //!< 垂直有效电平的极性。取值范围：[0~1]
    FH_UINT16   u16HsyncPeriod;              //!< 水平触发信号的一个周期的cycle数。取值范围：[0~0x1fff]
    FH_UINT16   u16HsyncDuty;                //!< 水平触发信号的有效电平cycle数  。取值范围：[0~0x1fff]
    FH_BOOL     bHsyncPolatiry;              //!< 水平有效电平的极性。取值范围：[0~1] 0:高电平有效、1:低电平有效
    FH_VICAP_SYNC_TRIGGER_MODE_E syncTriggerMode;//!<0:根据htotal/vtotal产生hsync和vysnc 1:根据外部时钟产生hsync和vsync
}FH_VICAP_FRAME_SEQ_CFG_S;

typedef struct
{
    FH_BOOL bReady;
    FH_UINT32 u32DataBufAddr;
    FH_UINT32 u32CheckSum;
    FH_UINT32 u32StartLine;
    FH_UINT32 u32FifoDepth;
}ISP_ECDC_INFO;

typedef struct
{
    FH_UINT32 u32VicapDevId;    //!< vicap的设备号
    FH_UINT64 u64TimeStamp;     //!< vicap启动帧的时间戳
    FH_UINT64 u64FrameId;       //!< 传递过来的frame id
    FH_UINT32 u32EcPara;
    FH_VICAP_SIZE_S stPic;
    ISP_ECDC_INFO stSfInfo;
    ISP_ECDC_INFO stLfInfo;
    FH_UINT32  u32Reserved[16];
}ISP_SRC_INFO_S;


#pragma pack(4)
typedef struct
{
    ISP_ECDC_INFO ispEcdcInfo;//!<ecdc信息
    FH_PHYADDR u32BufAddrPhy; //!<指向整个buf的头
    FH_UINT32  u32BufVirAddr; //!< 整个buf的头的虚拟地址，同一个进程空间可直接使用。取值范围：[FH_UINT32]
    FH_PHYADDR addrPhy;       //!< 数据的物理地址。取值范围：[FH_PHYADDR]
    FH_UINT32  u32VirAddr;    //!< 数据应用层的虚拟地址，同一个进程空间可直接使用。取值范围：[FH_UINT32]
    FH_UINT32  u32Crc;        //!< 数据校验值  。取值范围：[0~0xffffffff]
    FH_UINT64  u64TimeStamp;  //!< 数据获取的时间戳。取值范围：[0~0xffffffffffffffff]
    FH_UINT32  u32BufSize;    //!< 获取到的离线数据的缓存大小。取值范围：[0~0xffffffff]
}FH_VICAP_STREAM_COM_S;

typedef struct
{
    FH_UINT8   u8DevId;     //!< 设备号。取值范围：[0~2]
    FH_BOOL    bV2;         //!< 是否使用升级版获取缓存，未及时取走时下次可立刻获取到。取值范围：[0~1]
    FH_BOOL    bLock;       //!< 是否加锁。取值范围：[0~1]
    FH_BOOL    bBlock;      //!< 是否使用阻塞，阻塞支持线程调度。取值范围：[0~1]
    FH_UINT32  u32TimeOut;  //!< 超时时间，阻塞和非阻塞都可以配置超时时间，单位是ms。取值范围：[0~0xffffffff]
    FH_UINT64  u64FrmCnt;   //!< 帧号。取值范围：[0~0xffffffff]
    FH_UINT32  u32LineStride;   //!< 一行的数据量，单位是字节。取值范围：[0~0xffffffff]
    FH_VICAP_SIZE_S stPic;      //!< 图像幅面相关的配置。取值范围：[FH_VICAP_SIZE_S]
    FH_VICAP_STREAM_COM_S stSf;  //!< 短帧的数据流。取值范围：[FH_VICAP_STREAM_COM_S]
    FH_VICAP_STREAM_COM_S stLf;  //!< 长帧的数据流。取值范围：[FH_VICAP_STREAM_COM_S]
    FH_UINT32 subscribe_info;
}FH_VICAP_STREAM_S;

typedef struct
{
    FH_UINT8   u8DevId;     //!< 设备号。取值范围：[0~2]
    FH_BOOL    bBlock;      //!< 是否使用阻塞，阻塞支持线程调度。取值范围：[0~1]
    FH_UINT32  u32TimeOut;  //!< 超时时间，阻塞和非阻塞都可以配置超时时间，单位是ms。取值范围：[0~0xffffffff]
    FH_UINT64  u64FrmCnt;   //!< 帧号。取值范围：[0~0xffffffff]
	FH_PHYADDR addrPhy; 	  //!< 数据的物理地址。取值范围：[FH_PHYADDR]
	FH_UINT32  u32VirAddr;	  //!< 数据应用层的虚拟地址，同一个进程空间可直接使用。取值范围：[FH_UINT32]
	FH_UINT64  u64TimeStamp;  //!< 数据获取的时间戳。取值范围：[0~0xffffffffffffffff]
	FH_PHYADDR u32BufAddrPhy; //!<指向整个buf的头
    FH_UINT32  u32BufVirAddr;
	FH_UINT32  u32BufSize;	  //!< 获取到的离线数据的缓存大小。取值范围：[0~0xffffffff]
}FH_VICAP_EBD_STREAM_S;

#pragma pack()

typedef struct
{
    FH_BOOL     bEnable;        //!< 当前通道是否使能。取值范围：[0~1]
    FH_UINT32   u32SofCnt;      //!< 帧开始信号计数。取值范围：[0~0xffffffff]
    FH_UINT32   u32EofCnt;      //!< 帧结束信号计数。取值范围：[0~0xffffffff]
    FH_UINT32   u32WrDoneCnt;   //!< 帧写完计数。取值范围：[0~0xffffffff]
    FH_UINT16   u16FrameRate;   //!< sensor输出帧率。取值范围：[0~0xffff]
}FH_VICAP_STATUS_S;

typedef struct
{
    FH_UINT32 Ir_Config_Reg0_31_0;
    FH_UINT32 Ir_Config_Reg0_63_32;
    FH_UINT32 Ir_Config_Reg0_95_64;
    FH_UINT32 Ir_Config_Reg0_127_96;
    FH_UINT32 Ir_Config_Reg1_31_0;
    FH_UINT32 Ir_Config_Reg1_63_32;
    FH_UINT32 Ir_Config_Reg1_95_64;
    FH_UINT32 Ir_Config_Reg1_127_96;
    FH_UINT32 Ir_Config_Reg2_31_0;
    FH_UINT32 Ir_Config_Reg2_63_32;
    FH_UINT32 Ir_Config_Reg2_95_64;
    FH_UINT32 Ir_Config_Reg2_127_96;
}FH_VICAP_IR_REGS_S;

typedef struct
{
    VI_DEV  id;
    FH_UINT16 buffer_cnt;//!<最大10，默认3
}FH_VICAP_Buf_Cnt_S;

typedef struct
{
    VI_DEV  id;
    FH_UINT32 subscribeInfo;//!<最大10，默认3
}FH_VICAP_Subscribe_Info_S;

typedef struct
{
    VI_DEV  id;
    FH_UINT32 pause;
}FH_VICAP_Pause_S;

typedef struct
{
    VI_DEV  id;
    FH_UINT32 validCnt;
    FH_PHYADDR phyAddr[20];
}FH_VICAP_FreeBuf_S;

#pragma pack(4)
typedef enum
{
    VICAP_CALLBACK_SOF = 0,
    VICAP_CALLBACK_EOF = 1,
    VICAP_CALLBACK_OVERFLOW = 2,
    VICAP_CALLBACK_VIRDEV_DONE = 3,
    VICAP_CALLBACK_TYPE_NUM
}Vicap_CallBack_Type_E;

typedef struct
{
    FH_UINT32 isShortFrame;
    VI_DEV id;
    Vicap_CallBack_Type_E  type;
	FH_UINT64 u64UserData;
}Vicap_Callback_info_S;

typedef struct
{
    VI_DEV  id;
	FH_UINT64 u64UserData;
    int (*callback)(Vicap_Callback_info_S param);
}FH_VICAP_Callback_Info_S;
#pragma pack()


#define SCENARIO_VICAP_NUM 4
#define CLK_SETTING_NUM_MAX 8

enum{
    VICAP_PEAKLOAD_0 = 16000000,
    VICAP_PEAKLOAD_1 = 66000000,
    VICAP_PEAKLOAD_2 = 110000000,
    VICAP_PEAKLOAD_3 = 133000000,
    VICAP_PEAKLOAD_4 = 198000000,
    VICAP_PEAKLOAD_5 = 266000000,
    VICAP_PEAKLOAD_6 = 333000000,
    VICAP_PEAKLOAD_7 = 396000000,
};

typedef struct{
    FH_UINT32 clk_rate[CLK_SETTING_NUM_MAX];
}FH_VICAP_SCENARIO_ATTR;

typedef struct
{
    FH_UINT32 curScenarioId;
    FH_VICAP_SCENARIO_ATTR ScenarioData[SCENARIO_VICAP_NUM];
}FH_VICAP_LOWPOWER_INFO_S;

typedef struct
{
    FH_UINT32 width;
    FH_UINT32 height;
    FH_UINT32 fps;
    FH_UINT32 pclk;
}FH_VICAP_SIZE_FPS_S;

typedef struct
{
    FH_UINT32 pclkSet;//blank is set;0:not value 1: valid value
    FH_VICAP_SIZE_FPS_S sizeFpsInfo[3];//mipi0 mipi1 dvp
}FH_VICAP_PEAKLOAD_INFO_S;


typedef struct
{
    VI_DEV devId;
    FH_UINT8 vicap_offline_enable;  //0: disable; 1:enable
}FH_VICAP_OFFLINE_S;

typedef struct
{
    VI_DEV  id;
    FH_UINT32 buf_size;
}FH_VICAP_Buf_Size_S;

typedef enum __VICAP_FRAME_MODE{
    VICAP_OUTPUT_MODE_NORMAL = 0x0,//!< 正常输出帧
    VICAP_OUTPUT_MODE_N = 0x1,//!< 输出N帧后停止输出并关闭CLK，resume后继续输出N帧
    VICAP_OUTPUT_MODE_NUM,
}FH_VICAP_OUTPUT_MODE_E;

typedef struct
{
    FH_VICAP_OUTPUT_MODE_E stMode;//!< VICAP帧模式
    FH_UINT32 u32FrameNum;//!< 如果输出模式为VICAP_OUTPUT_MODE_N，该变量用于配置输出多少帧
}FH_VICAP_Output_Mode_S;

typedef struct
{
    VI_DEV  id;
    FH_UINT32 u32SkipFrameNum;
}FH_VICAP_SkipFrame_S;

typedef struct
{
	VI_DEV id;
	FH_UINT32 u32EbdChnEnable;//0 disable  1enable
	FH_UINT32 u32EbdVcEnable;//是否使能ebd的virtual channel
	FH_UINT32 u32EbdVcId;	//ebd的 virtual channel id
	FH_UINT32 u32BufCnt;
	FH_UINT32 u32BufLen;
	FH_UINT32 u32ImgVcId;	//Img的 virtual channel id
}FH_VICAP_EBD_CFG_S;
typedef struct
{
    VI_DEV devId;
	FH_VICAP_WDR_MODE_E u8WdrMode;
}FH_VICAP_WdrMode_S;

typedef struct
{
    VI_DEV id;
	FH_UINT8 u8Fps;
	FH_UINT16 u16Width;
	FH_UINT16 u16Height;
}FH_VICAP_Colorbar_S;

typedef struct
{
    VI_DEV id;
	FH_UINT32 u32Enable;
}FH_VICAP_InputSwitch_S;

typedef struct
{
    VI_DEV                      devId;       //!< 设备id号]
    FH_BOOL                     enRawecBypass;   //!< 压缩是否bypass [0]:压缩 [1]:不压缩
    FH_UINT32                   reserve[8];
}FH_VICAP_MIPI_RAWECBYPASS_S;

/**
 * @brief         获取入参模式下控制的VICAP模块使用的内存大小
 *
 * @param[in]     stBufSizeAttr         初始化VICAP设备的参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - u8DevNum决定了获取的是多个还是一个VICAP设备使用的缓存大小，并且决定了使用astBufCfg数组中的前多少个结构体变量
 *    - 例如u8DevNum等于2时，只会获取到astBufCfg[0]/astBufCfg[1]对应的u8DevId设备的使用的缓存的大小
 *
 */
FH_SINT32 FH_VICAP_GetBufSize(FH_VICAP_BUF_SIZE_ATTR_S* stBufSizeAttr);

/**
 * @brief         初始化VICAP使用的内存
 *
 * @param[in]     devId              初始化的VI的设备号
 * @param[in]     pstDevAttr         初始化VICAP设备的参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_InitViDev(VI_DEV devId, FH_VICAP_DEV_ATTR_S* pstDevAttr);
/**
 * @brief         初始化VICAP硬件模块工作属性
 *
 * @param[in]     devId              初始化的VI的设备号
 * @param[in]     pstViAttr          初始化VICAP硬件参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_SetViAttr(VI_DEV devId, FH_VICAP_VI_ATTR_S* pstViAttr);
/**
 * @brief         获取当前VICAP硬件初始化属性
 *
 * @param[in]     devId              初始化的VI的设备号
 * @param[in]     pstViAttr          获取当前VICAP硬件初始化参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_GetViAttr(VI_DEV devId, FH_VICAP_VI_ATTR_S* pstViAttr);
/**
 * @brief         配置VICAP通道参见属性
 *
 * @param[in]     devId              VICAP裁剪通路的设备号
 * @param[in]     enWdrChan          VICAP裁剪通道号
 * @param[in]     pstCropCfg         VICAP裁剪配置
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_SetPipeCrop(VI_DEV devId, FH_VICAP_WDR_CHAN_E enWdrChan, FH_VICAP_CROP_CFG_S* pstCropCfg);
/**
 * @brief         获取VICAP通道裁剪的配置
 *
 * @param[in]     devId              VICAP裁剪通路的设备号
 * @param[in]     enWdrChan          VICAP裁剪通道号
 * @param[out]    pstCropCfg         获取到的VICAP的裁剪配置
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_GetPipeCrop(VI_DEV devId, FH_VICAP_WDR_CHAN_E enWdrChan, FH_VICAP_CROP_CFG_S* pstCropCfg);
/**
 * @brief         配置DVP Sensor接入的配置
 *
 * @param[in]     pstDvpCfg          DVP SENSOR接入配置
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 1. DVP输入总计16个管脚，u8IoRemap可以映射IO和实际像素点数据的映射关系
 *    - 例如，u8IoRemap[0] = 0xf，则从D0输入的数据映射到像素点的最高位
 *
 */
FH_SINT32 FH_VICAP_SetDvpCfg(FH_VICAP_DVP_CFG_S* pstDvpCfg);
/**
 * @brief         获取DVP Sensor接入的配置
 *
 * @param[out]    pstDvpCfg          DVP SENSOR接入配置
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_GetDvpCfg(FH_VICAP_DVP_CFG_S* pstDvpCfg);

/**
 * @brief         配置Bt656 Sensor接入的配置
 *
 * @param[in]     pstBt656Cfg        DVP SENSOR接入配置
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 1. Bt656复用DVp输入端口，DVP输入总计16个管脚，u8IoRemap可以映射IO和实际像素点数据的映射关系
 *    - 例如，u8IoRemap[0] = 0xf，则从D0输入的数据映射到像素点的最高位
 *
 */
FH_SINT32 FH_VICAP_SetBt656Cfg(FH_VICAP_BT656_CFG_S* pstBt656Cfg);


/**
 * @brief         配置Bt656 Sensor接入的配置
 *
 * @param[in]     pstBt656Cfg        DVP SENSOR接入配置
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 1. Bt656复用DVp输入端口，DVP输入总计16个管脚，u8IoRemap可以映射IO和实际像素点数据的映射关系
 *    - 例如，u8IoRemap[0] = 0xf，则从D0输入的数据映射到像素点的最高位
 *
 */
FH_SINT32 FH_VICAP_GetBt656Cfg(FH_VICAP_BT656_CFG_S* pstBt656Cfg);

/**
 * @brief         配置Ir Sensor接入的配置
 *
 * @param[in]     pstIrCfg           IR SENSOR接入配置
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 1. IR复用DVp输入端口，DVP输入总计16个管脚，u8IoRevert用于设置管脚是从0->15的顺序还是15->0的顺序
 *
 */
FH_SINT32 FH_VICAP_SetIrCfg(FH_VICAP_IR_CFG_S* pstIrCfg);
/**
 * @brief         获取Ir Sensor的配置
 *
 * @param[in]     pstIrCfg           IR SENSOR接入配置
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 1. IR复用DVp输入端口，DVP输入总计16个管脚，u8IoRevert用于设置管脚是从0->15的顺序还是15->0的顺序
 *
 */
FH_SINT32 FH_VICAP_GetIrCfg(FH_VICAP_IR_CFG_S* pstIrCfg);

/**
 * @brief         设置虚拟设备的配置
 *
 * @param[in]     pstVirDevCfg       拼接相关的属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_SetVirDevCfg(FH_VICAP_VIRDEV_CFG_S* pstVirDevCfg);


// 注册需要拼接的通道号，注册之后，该路ISP支持拼接，注册后即开启拼接，不想开启拼接清调用注销接口
/**
 * @brief         注册拼接的设备到拼接组里面
 *
 * @param[in]     pstStithAttr       拼接相关的属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_RegisterStitchDev(FH_VICAP_STITCH_ATTR_S* pstStithAttr);
/**
 * @brief         注销掉需要拼接的通道号，注销掉之后，该路通道不参与拼接
 *
 * @param[in]     devId              目标注销掉的设备号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 本项目不支持拼接功能
 *
 */
FH_SINT32 FH_VICAP_UnregisterStitchDev(VI_DEV devId);
/**
 * @brief         VICAP设备退出
 *
 * @param[in]     devId              需要退出的VICAP设备号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 *
 */
FH_SINT32 FH_VICAP_Exit(VI_DEV devId);
/**
 * @brief         配置目标sensor的工作时钟
 *
 * @param[in]     enSnsIdx           sensor序号
 * @param[in]     u32Clock           sensor的工作时钟
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_SetSnsClk(FH_VICAP_SNS_IDX_E enSnsIdx, FH_UINT32 u32Clock);
/**
 * @brief         配置目标sensor的工作时钟使能
 *
 * @param[in]     enSnsIdx           sensor序号
 * @param[in]     bEn                sensor工作时钟使能开关
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_SetSnsClkEn(FH_VICAP_SNS_IDX_E enSnsIdx, FH_BOOL bEn);
/**
 * @brief         配置产生触发信号模块的参数
 *
 * @param[in]     pstFrameSeqCfg     目标配置的触发信号参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - hsync的实际的cycle的单位时间单位为: 32 / vicap频率
 *    - vsync的实际输cycle的单位时间单位为: hsync总的时间 / vicap频率
 *    - vsync在５个vsync单位cycle之后，开始输出
 *    - vsync实际输出波形的频率等于:vicap工作频率 / vsync配置的cycle值
 *    - hsync实际输出波形的频率等于:vicap工作频率 / hsync配置的cycle值
 *
 */
FH_SINT32 FH_VICAP_SetFrameSeqGenCfg(FH_VICAP_FRAME_SEQ_CFG_S* pstFrameSeqCfg);

/**
 * @brief         配置特定的触发信号模块的参数
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - hsync的实际的cycle的单位时间单位为: 32 / vicap频率
 *    - vsync的实际输cycle的单位时间单位为: hsync总的时间 / vicap频率
 *    - vsync在５个vsync单位cycle之后，开始输出
 *    - vsync实际输出波形的频率等于:vicap工作频率 / vsync配置的cycle值
 *    - hsync实际输出波形的频率等于:vicap工作频率 / hsync配置的cycle值
 *
 */
FH_SINT32 FH_VICAP_GetFrameSeqGenCfg(FH_VICAP_FRAME_SEQ_CFG_S* pstFrameSeqCfg);
/**
 * @brief         配置触发信号模块输出使能
 *
 * @param[in]     u16EnCfg           触发信号使能，一次性配置多个
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - u16EnCfg = 1，打开通道0触发信号输出，关闭其他通道
 *    - u16EnCfg = 0x6，打开通道1/2触发信号输出，关闭其他通道
 *
 */
FH_SINT32 FH_VICAP_SetFrameSeqGenEn(FH_UINT16 u16EnCfg);
/**
 * @brief         获取当前配置的触发信号模块输出使能
 *
 * @param[out]    u16EnCfg           当前配置使能开关值
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_GetFrameSeqGenEn(FH_UINT16 *u16EnCfg);
/**
 * @brief      获取指定通道的一帧数据流
 *
 * @param[out]     stStream  配置的信息，以及返回获取到的数据地址
 *
 * @retval     0         成功
 * @retval     "非0"      失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 */
FH_SINT32 FH_VICAP_GetStream(FH_VICAP_STREAM_S *stStream);
/**
 * @brief         释放获取的一帧数据流
 *
 * @param[in]     pstStream  #FH_VICAP_GetStream 配置/获取到的指针信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_ReleaseStream(FH_VICAP_STREAM_S *pstStream);
/**
 * @brief         获取VICAP当前工作状态
 *
 * @param[in]     enSnsIdx           sensor序号
 * @param[out]    stStatus           获取到的VICAP的工作状态信息，具体数据参看结构体定义
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_UINT32 FH_VICAP_GetStatus(FH_VICAP_SNS_IDX_E enSnsIdx, FH_VICAP_STATUS_S *stStatus);
/**
 * @brief         设置红外的寄存器
 *
 * @param[in]     irRegs             ir reg的配置
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_UINT32 FH_VICAP_SetIrRegs(FH_VICAP_IR_REGS_S *irRegs);

/**
 * @brief         获取红外的寄存器
 *
 * @param[out]    irRegs             ir reg的配置
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_UINT32 FH_VICAP_GetIrRegs(FH_VICAP_IR_REGS_S *irRegs);

/**
 * @brief         设置红外的ooc寄存器地址
 *
 * @param[in]     u32AddrPhy         设置红外的ooc寄存器地址
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_UINT32 FH_VICAP_SetIrOocAddr(FH_PHYADDR u32AddrPhy);

/**
 * @brief         获取红外的ooc寄存器地址
 *
 * @param[out]    u32AddrPhy         获取红外的ooc寄存器地址
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_UINT32 FH_VICAP_GetIrOocAddr(FH_PHYADDR *u32AddrPhy);

/**
 * @brief         设置拼接组的参数
 *
 * @param[in]     pstStithAttr       拼接相关的参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_SetStitchGrpAttr(FH_VICAP_STITCH_GRP_ATTR_S* pstStithAttr);
/**
 * @brief         设置vicap的负载情况
 *
 * @param[in]     peakload           负载情况
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_SetPeakLoad(FH_VICAP_PEAKLOAD_INFO_S *peakload);

/**
 * @brief         设置vicap的bufcnt
 *
 * @param[in]     bufCnt             buf cnt
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_SetBufCnt(FH_VICAP_Buf_Cnt_S *bufCnt);

/**
 * @brief      设置subscribeInfo
 *
 * @param      subscribeInfo  The subscribe information
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 */
FH_SINT32 FH_VICAP_SetScribeInfo(FH_VICAP_Subscribe_Info_S *subscribeInfo);


/**
 * @brief      设置暂停和继续的接口，暂停后buffer返回会放到freelist，但是不写到地址寄存器
 *
 * @param      pPauseInfo  0：继续； 1：暂停
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 */
FH_SINT32 FH_VICAP_SetPause(FH_VICAP_Pause_S *pPauseInfo);


/**
 * @brief      获取freebuf的信息
 *
 * @param      freeBuf  The free buffer
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 */
FH_SINT32 FH_VICAP_Get_FreeBuf(FH_VICAP_FreeBuf_S *freeBuf);

/**
 * @brief     初始化VICAP硬件模块工作属性
 *
 * @param      offline  offline setting
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 */
FH_SINT32 FH_VICAP_Set_Offline(FH_VICAP_OFFLINE_S *offline);

/**
 * @brief      设置phy的lane数
 *
 * @param[in]  num   lane num
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 */
int FH_VICAP_Set_Phy_Lane(int num);

/**
 * @brief		  设置vicap的bufSize
 *
 * @param[in]	  buf_size
 *
 * @retval	0			成功
 * @retval	"非0"		失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *	  - 无
 *
 */
FH_SINT32 FH_VICAP_SetBufSize(FH_VICAP_Buf_Size_S *buf_size);
/**
 * @brief         配置该路的输出方式
 *
 * @param[in]     stOutputMode           工作模式
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_SetOutputMode(FH_VICAP_Output_Mode_S stOutputMode);

/**
 * @brief		  挂起休眠
 *
 * @param[in]	  block
 *
 * @retval	0			   成功
 * @retval	"非0"			失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *	  - 无
 *
 */
FH_SINT32 FH_VICAP_Suspend(int block);

/**
 * @brief         继续输出图片，仅对VICAP_OUTPUT_MODE_N有效
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *    - 无
 *
 */
FH_SINT32 FH_VICAP_Resume(FH_VOID);
/**
 * @brief		  设置vicap的回调事件
 *
 * @param[in]	  cb
 *
 * @retval	0			成功
 * @retval	"非0"		失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 * @note
 *	  - 无
 *
 */
FH_SINT32 FH_VICAP_Register_Callback(FH_VICAP_Callback_Info_S *cb);


/**
 * @brief      debug vicap info
 *
 * @retval  0           成功
 * @retval  "非0"        失败，其值参见 @namelink{错误码,vicap_errcode} 。
 *
 */
FH_SINT32 FH_VICAP_Debug(void);

/*
*   Name: FH_VICAP_Set_SkipFrame
*            offline模式丢弃发送给下一级N帧
*
*   Parameters:
*
*       [IN] pstSkipFrame
*            skip frame
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FH_VICAP_Set_SkipFrame(FH_VICAP_SkipFrame_S *pstSkipFrame);

/*
*   Name: FH_VICAP_SetWdrMode
*			设置Wdr模式
*
*   Parameters:
*
*	   [IN] VI_DEV devId
*			初始化的VI的设备号
*
*	   [IN] u8WdrMode;
*			输入sensor的线性还是wdr配置。取值范围：[0~1]
*
*   Return:
*		   0(成功)
*		  非0(失败，详见错误码)
*
*   Note:
*	  无
*/
FH_SINT32 FH_VICAP_SetWdrMode(VI_DEV devId, FH_VICAP_WDR_MODE_E u8WdrMode);

/*
*   Name: FH_VICAP_SetEbdCfg
*			配置Mipi Sensor Ebd通路的配置
*
*   Parameters:
*
*	   [IN] FH_VICAP_EBD_CFG_S* pstEbdCfg
*			Mipi Sensor Ebd接入配置
*
*   Return:
*		   0(成功)
*		  非0(失败，详见错误码)
*
*   Note:
*/
FH_SINT32 FH_VICAP_SetEbdCfg(FH_VICAP_EBD_CFG_S* pstEbdCfg);

/*
*   Name: FH_VICAP_GetEbdCfg
*			获取Mipi Sensor Ebd的配置
*
*   Parameters:
*
*	   [IN] FH_VICAP_EBD_CFG_S* pstEbdCfg
*			MIPI SENSOR EBD配置
*
*   Return:
*		   0(成功)
*		  非0(失败，详见错误码)
*
*   Note:
*/
FH_SINT32 FH_VICAP_GetEbdCfg(FH_VICAP_EBD_CFG_S* pstEbdCfg);

/*
*   Name: FH_VICAP_GetEbdStream
*			获取指定通道的EBD数据流
*
*   Parameters:
*
*	   [IN/OUT] FH_VICAP_EBD_STREAM_S *stStream
*			配置的信息，以及返回获取到的数据地址
*
*   Return:
*		   0(成功)
*		  非0(失败，详见错误码)
*
*   Note:
*	  无
*/
FH_SINT32 FH_VICAP_GetEbdStream(FH_VICAP_EBD_STREAM_S *stStream);

/*
*   Name: FH_VICAP_ReleaseEbdStream
*			释放指定通道的一帧数据流
*
*   Parameters:
*
*	   [IN/OUT] FH_VICAP_EBD_STREAM_S *stStream
*			FH_VICAP_GetEbdStream获取到的指针信息
*
*   Return:
*		   0(成功)
*		  非0(失败，详见错误码)
*
*   Note:
*	  无
*/
FH_SINT32 FH_VICAP_ReleaseEbdStream(FH_VICAP_EBD_STREAM_S *pstStream);

/*
*	Name: FH_VICAP_SetColorbar
*			配置Colorbar通路的配置
*
*	Parameters:
*
*	   [IN] VI_DEV id
*			FH_UINT8 u8Fps 0表示不输出colorbar
*			FH_UINT16 u16Width
*			FH_UINT16 u16Height
*	Return:
*		   0(成功)
*		  非0(失败，详见错误码)
*
*	Note:
*/

FH_SINT32 FH_VICAP_SetColorbar(VI_DEV id, FH_UINT8 u8Fps, FH_UINT16 u16Width, FH_UINT16 u16Height);

/*
*	Name: FH_VICAP_SetInputSwitch
*			动态设置通路的输入使能，一般用于部分sensor的suspend之前
*
*	Parameters:
*
*	   [IN] VI_DEV id
*			FH_UINT32 u32Enable 0：关闭使能 1：打开使能
*	Return:
*		   0(成功)
*		  非0(失败，详见错误码)
*
*	Note:
*/

FH_SINT32 FH_VICAP_SetInputSwitch(VI_DEV id, FH_UINT32 u32Enable);

/*
*   Name: FH_VICAP_SetMipiRawecBypass
*			设置mipi rawec bypass配置
*
*   Parameters:
*
*	   [IN] pstRawecBypass
*			rawec bypass的配置
*
*   Return:
*		   0(成功)
*		  非0(失败，详见错误码)
*
*   Note:
*	  无
*/
FH_SINT32 FH_VICAP_SetMipiRawecBypass(FH_VICAP_MIPI_RAWECBYPASS_S* pstRawecBypass);

#endif // __FH_VICAP_MPI_H__
