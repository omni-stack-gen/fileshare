#ifndef __FH_CAMIF_MPIPARA_H__
#define __FH_CAMIF_MPIPARA_H__
#include "types/type_def.h"

typedef enum{
    CAMIF_PORT_TYPE_DVP,
    CAMIF_PORT_TYPE_P2S,//只支持raw8
    CAMIF_PORT_TYPE_SCB,
    CAMIF_PORT_TYPE_SPI,
    CAMIF_PORT_TYPE_PIXART,
    CAMIF_PORT_TYPE_NUM
}Camif_Port_Type_E;

typedef enum{
    CAMIF_PIXART_TYPE_PAG7920LT,
    CAMIF_PIXART_TYPE_NUM
}Camif_Pixart_Type_E;

typedef enum{
    CAMIF_BUF_TYPE_AE = 1 << 0,
    CAMIF_BUF_TYPE_AWB = 1 << 1,
    CAMIF_BUF_TYPE_RAWEC = 1 << 2,
}Camif_Buf_Type_E;

typedef enum{
    CAMIF_STREAMON_NORMAL,//just prerolling
    CAMIF_STREAMON_TUNE_OFFLINE,//tune mode -> ddr
    CAMIF_STREAMON_TUNE_ONLINE,//tune mode->isp460, not need buf
    CAMIF_STREAMON_NUM
}Camif_Stream_On_Mode_E;

typedef struct
{
    FH_UINT8 hsync_pol; //0:high; 1:low
    FH_UINT8 vsync_pol; //0:high; 1:low
    FH_UINT8 dvpin_mode; //0:normal 1:hfp
    FH_UINT32 dvpin_hfp;  //跳过多少个有效数据
    FH_UINT8 dvp_inverted_pad_enable;//inverted_pad使能
}Camif_Dvp_Conf_S;

typedef struct
{
    FH_UINT8 hsync_pol; //0:high; 1:low
    FH_UINT8 vsync_pol; //0:high; 1:low
    FH_UINT8 dvp_inverted_pad_enable;//inverted_pad使能
    FH_UINT8 p2s_pin_num; //多少个pin转8bit;//1,2,4
}Camif_P2s_Conf_S;

typedef struct
{
    FH_UINT8 sck_inv_cfg;//上升沿下降沿取反
    FH_UINT8 sync_code_mid_byte_mode;//mtk模式: 0:8h00, 1:8hff
    FH_UINT8 sync_code_led_byte;
    FH_UINT8 sync_code_lst_byte;
    FH_UINT8 sync_code_fed_byte;
    FH_UINT8 sync_code_fst_byte;
}Camif_Scb_Conf_S;

typedef struct
{
    FH_UINT8 sck_inv_cfg;//上升沿下降沿取反
    FH_UINT8 sync_code_mid_byte_mode;//mtk模式: 0:8h00, 1:8hff
    FH_UINT8 spi_bit_mode;//0:8bits 1:10bits
    FH_UINT8 spi_cap_mode;//spi模式：0单边沿 1双边沿
    FH_UINT8 spi_line_mode;//0:1wire 1:2wire 2:4wire
    //FH_UINT8 mtk_crc_en; //must 0
    FH_UINT8 mtk_mode;
    FH_UINT8 gc_mode;
    FH_UINT8 sst_mode;
    FH_UINT8 sync_code_led_byte;
    FH_UINT8 sync_code_lst_byte;
    FH_UINT8 sync_code_fed_byte;
    FH_UINT8 sync_code_fst_byte;
    FH_UINT32 sync_code_data_byte;
}Camif_Spi_Conf_S;

typedef struct
{
    Camif_Pixart_Type_E pixart_type;//pixart只需要size，驱动底层写死PAG7920LT流程。
}Camif_Pixart_Conf_S;
typedef struct
{
    Camif_Port_Type_E port_type;
    FH_UINT32 low_pos_8bit_en;//8bit转10bit补0方法，0:高位补0，1:低位补0
    FH_UINT32 Camif_Data_En;//数据进入rawec时10bit转12方法，0:低位补0 1:高位补0
    union Camif_Port_Config
    {
        Camif_Dvp_Conf_S dvp_conf;
        Camif_P2s_Conf_S p2s_conf;
        Camif_Scb_Conf_S scb_conf;
        Camif_Spi_Conf_S spi_conf;
        Camif_Pixart_Conf_S pixart_conf;
    }conf;
}Camif_Port_Conf_S;

typedef struct
{
    Camif_Stream_On_Mode_E streamOnMode;
    int pic_bypass;
}Camif_StreamOn_S;

typedef struct
{
    Camif_Buf_Type_E bufType;
    FH_UINT32 phyAddr;
    FH_UINT32 buf_len;
}Camif_Buf_S;

typedef struct
{
    _Bool enable;//0:disable 1:enable
    FH_UINT32 statwinhc;       //统计窗口水平方向个数减1
    FH_UINT32 statwinvc;       //统计窗口垂直方向个数减1
    FH_UINT32 statwinw;        //统计窗口宽度/2-1
    FH_UINT32 statwinh;        //统计窗口高度/2-1
    FH_UINT32 ae_mode;         //ae模式
    FH_UINT32 statwinly;       //统计窗口起始纵坐标
    FH_UINT32 statwinlx;       //统计窗口起始横坐标
    FH_UINT32 ae_addr;         //ae输出的base地址,并非ae的输出地址
    FH_UINT32 statsamph;       //垂直采样间隔减1
    FH_UINT32 statsampv;       //水平采样间隔减1
    FH_UINT32 statthh;         //饱和像素阈值
    FH_UINT32 statthl;         //饱和像素阈值
    FH_UINT32 stat_picheight;  //STAT的输入图像高
    FH_UINT32 stat_picwidth;   //STAT的输入图像宽
}Camif_Ae_Conf_S;


typedef struct
{
    _Bool enable;//0:disable 1:enable
    FH_UINT32 awb_cntshift;     //cnt偏移，cnt >> cntshift
    FH_UINT32 awb_bitmove;      //bit偏移，目标是12bit
    FH_UINT32 awb_flagmode;     //全局统计模式：0考虑白区 1不考虑白区
    FH_UINT32 awb_calgmodex;    //白区计算：0用Y 1用G
    FH_UINT32 awb_calgmodeth;   //白区计算模式：0兼容模式 1新模式
    FH_UINT32 leftsft;          //数据左移信号
    FH_UINT32 shiftall;         //数据移位
    FH_UINT32 high_offset_en;   //ae/awb输入是高位还是低位补0;0:低位补0;1:高位补0
    FH_UINT32 awb_statwinstartx;//水平方向起点/2
    FH_UINT32 awb_statwinstarty;//垂直方向起点/2
    FH_UINT32 awb_v_num;//统计窗垂直个数-1
    FH_UINT32 awb_h_num;//统计窗水平个数-1
    FH_UINT32 awb_z_height;//统计窗口高度/2-1
    FH_UINT32 awb_h_width;//统计窗口宽度/2-1
    FH_UINT32 awb_bx;//B点横坐标
    FH_UINT32 awb_ay;//A点纵坐标
    FH_UINT32 awb_cx;//C点横坐标
    FH_UINT32 awb_by;//B点纵坐标
    FH_UINT32 awb_ex;//e点横坐标
    FH_UINT32 awb_dx;//d点横坐标
    FH_UINT32 awb_fy;//f点纵坐标
    FH_UINT32 awb_ey;//e点纵坐标
    FH_UINT32 awb_gy;//g点纵坐标
    FH_UINT32 awb_gx;//g点横坐标
    FH_UINT32 Awb_Calmode;//白区计算模式，0:兼容模式 1:新模式
    FH_UINT32 awb_th_l;//统计阈值下限
    FH_UINT32 awb_th_h;//统计阈值上限
    FH_UINT32 awb_picheight;//awb的输入图像高
    FH_UINT32 awb_picwidth;//awb的输入图像宽
    FH_UINT32 awb_bayerfmt;//awb的输入raw pattern,0:rggb,1:grbg,2:bggr,3:gbrg
}Camif_Awb_Conf_S;

typedef struct
{
    _Bool enable;//0:disable 1:enable
    FH_UINT32 Gain_PicInHeight;
    FH_UINT32 Gain_PicInWidth;
    FH_UINT32 Gaincolor1;
    FH_UINT32 Gaincolor0;
    FH_UINT32 Gaincolor3;
    FH_UINT32 Gaincolor2;
    FH_UINT32 Offsetcolor1;
    FH_UINT32 Offsetcolor0;
    FH_UINT32 Offsetcolor3;
    FH_UINT32 Offsetcolor2;
    FH_UINT32 raw_10bit_en;//blc模块是运行在10bit模式还是12bit模式,1:10bit 0:12bit
    FH_UINT32 gain_low_bit_en;//blc ae awb模块 10bit转12bit是低位补0还是高位补0;1:低位补0，0:高位补0
}Camif_Blc_Conf_S;

typedef struct
{
    FH_UINT32 phyAddr;//指向整个buffer
    FH_UINT32 buf_len;
    FH_UINT32 frame_id;
}Camif_Ae_info_S;


typedef struct
{
    FH_UINT32 is_pulse;//0:表示不是pulse 1：表示pulse
    FH_UINT32 frame_addr;//指向每一个帧实际数据的地址物理地址
    FH_UINT32 bufLen;
    FH_UINT32 frame_id;
    FH_UINT32 register_PPS_0;
    FH_UINT32 register_PPS_1;
    FH_UINT32 register_PPS_2;
    FH_UINT32 register_PPS_3;
    FH_UINT32 register_PPS_4;
    FH_UINT32 register_PPS_5;
    FH_UINT32 register_PPS_6;
    FH_UINT32 register_PPS_7;
    FH_UINT32 register_PPS_8;
    FH_UINT32 register_PPS_9;
    FH_UINT32 register_PPS_10;
    FH_UINT32 register_PPS_11;
    FH_UINT32 register_PPS_12;
    FH_UINT32 register_PPS_13;
    FH_UINT32 register_PPS_14;
    FH_UINT32 register_PPS_15;
    FH_UINT32 register_STAT_0;
    FH_UINT32 register_STAT_1;
    FH_UINT32 register_STAT_2;
    FH_UINT32 register_STAT_3;
}Camif_Frame_info_S;

typedef struct
{
    Camif_Frame_info_S frame_info;
}Camif_Buf_info_S;

typedef struct
{
    FH_UINT32 buf_len;
    FH_UINT32 phyAddr;//指向整个buffer, ddr有效
    FH_UINT32 frame_id;
    FH_UINT32 global_r;
    FH_UINT32 global_g;
    FH_UINT32 global_b;
    FH_UINT32 global_cnt;
}Camif_Awb_info_S;

typedef struct
{
    Camif_Buf_Type_E type;
    FH_UINT32  u32TimeOut;//ms
    union{
        Camif_Ae_info_S aeInfo;
        Camif_Buf_info_S bufInfo;
        Camif_Awb_info_S awbInfo;
    }info;
}Camif_GetStream_S;

typedef struct
{
    Camif_Port_Type_E port_type;
    FH_UINT32 reserve;
}Camif_StreamOff_S;

typedef struct
{
    _Bool enable;//0:disable 1:enable
    FH_UINT32 crop_x;
    FH_UINT32 crop_y;
    FH_UINT32 crop_width;
    FH_UINT32 crop_height;
}Camif_Crop_Conf_S;

typedef struct
{
    FH_UINT16  width; //输入图片的宽度
    FH_UINT16  height; //输入图片的高度
}Camif_Size_Conf_S;

typedef enum{
    CAM_BAYER_RGGB = 0,
    CAM_BAYER_GRBG = 1,
    CAM_BAYER_BGGR = 2,
    CAM_BAYER_GBRG = 3,
    CAM_BAYER_NUM,
}Cam_Bayer_Pattern_E;

typedef struct
{
    //Cam_Pixel_Fmt_E pixel_fmt;//输入pixel的数据格式
    Cam_Bayer_Pattern_E bayer_pattern;
}Camif_Format_Conf_S;

typedef struct
{
    FH_UINT16 skip_frame_cnt;
}Camif_Skip_Frame_Cnt_S;

typedef struct
{
    FH_UINT32 reserve;
}Camif_Deinit_S;
#endif // __FH_CAMIF_MPIPARA_H__
