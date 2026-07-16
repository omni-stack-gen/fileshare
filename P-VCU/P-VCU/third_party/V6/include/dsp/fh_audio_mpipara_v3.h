/*
 * fh_auido_mpipara.h
 *
 *  Created on: 2015年2月15日
 *      Author: fanggm
 */

#ifndef FH_AUIDO_MPIPARA_H_
#define FH_AUIDO_MPIPARA_H_

typedef enum
{
    AC_SR_8K   = 8000,
    AC_SR_16K  = 16000,
    AC_SR_32K  = 32000,
    AC_SR_48K  = 48000,

    AC_SR_110K = 11025,
    AC_SR_220K = 22050,
    AC_SR_441K = 44100,
    AC_SR_DUMMY = 0x1fffffff,
}FH_AC_SAMPLE_RATE_E;

typedef enum
{
    AC_BW_8  = 8,   /*not supported*/
    AC_BW_16 = 16,
    AC_BW_24 = 24,  /*not supported*/
    AC_BW_DUMMY = 0x1fffffff,
}FH_AC_BIT_WIDTH_E;


typedef struct
{
    FH_UINT32 len;
    FH_UINT8 *data;
}FH_AC_FRAME_S;

typedef enum
{
    FH_AC_MIC_IN   = 0,
    FH_AC_LINE_IN  = 1,
    FH_AC_SPK_OUT  = 2,
    FH_AC_LINE_OUT = 3,
    FH_AC_IO_TYPE_MAX = 4,
    FH_AC_IO_DUMMY = 0x1fffffff,
}FH_AC_IO_TYPE_E;

typedef enum
{
    FH_AO_MODE_LEFT  = 0, /*It's the default value, supported by all chips*/
    FH_AO_MODE_RIGHT = 1, /*Now only support for FH8856*/
    FH_AO_MODE_BOTH  = 2, /*left channel and right channel output the same audio samples*/
    FH_AO_MODE_DUMMY = 0x1fffffff,
}FH_AC_AO_MODE_E;

typedef enum
{
    FH_PT_LPCM     = 0,
    FH_PT_G711A    = 1, /*not supported*/
    FH_PT_G711U    = 2, /*not supported*/
    FH_PT_G726_16K = 3, /*not supported*/
    FH_PT_G726_32K = 4, /*not supported*/
    FH_PT_AAC      = 5, /*not supported*/
    FH_PT_MAX      = 6,
    FH_PT_DUMMY = 0x1fffffff,
}FH_AC_PT_TYPE_E;

typedef enum
{
    AC_STORE_NO       = 0, /*don't store to memory*/
    AC_STORE_SEPERATE = 1, /*store to memory seperately for every channel*/
    AC_STORE_MIX      = 2, /*store to memory mixed for every channel*/
    AC_STORE_MODE_MAX = 3,
    AC_STORE_DUMMY = 0x1fffffff,
}FH_AC_STORE_MODE_E;


typedef enum
{
    AC_MIX_METHOD_NONE  = 0, /*don't mix, default option*/
    AC_MIX_METHOD_2TO1  = 1,
    AC_MIX_METHOD_1TO1  = 2,
    AC_MIX_METHOD_MAX   = 3,
    AC_MIX_METHOD_DUMMY = 0x1fffffff,
}FH_AC_MIX_METHOD_E;

typedef struct
{
    FH_AC_IO_TYPE_E io_type;
    FH_AC_SAMPLE_RATE_E sample_rate;
    FH_AC_BIT_WIDTH_E bit_width;
    FH_AC_PT_TYPE_E enc_type;    /*encoding type*/
    FH_UINT32 channels;    /*通道个数 */
    FH_UINT32 period_size; /*一帧数据中的采样点个数 */
    FH_UINT32 volume;
}FH_AC_CONFIG;

#define AC_AI_MIX_BIT_OFFSET (16)
typedef struct
{
    FH_UINT32 enable;
    FH_UINT32 channel_mask;
    FH_UINT32 frames_one_time; /*how many frames to be get for one time*/
    FH_AC_MIX_METHOD_E mix_method;
}FH_AC_Raw_CONFIG;

#pragma pack(4)
typedef struct
{
    FH_UINT32 samples;      //samples of this frame for every channel
    FH_UINT32 channel_mask; //raw channel mask
    FH_UINT32 far_available;//1:FAR data is available, 0:FAR data is not available
    FH_UINT32 first_frame;  //first frame since enable AI.
    FH_UINT64 pts;          //PTS of this frame.
    FH_UINT32 sample_rate;  //sample rate of this frame for every channel
    FH_UINT32 reserved[1];
 
    short     CHData[3][160];
    short     FAR[160];
}FH_AC_Raw_Frame_S;
#pragma pack()

typedef struct
{
    FH_AC_Raw_Frame_S *pFrame;
    FH_UINT32          nFrame; //how many frames, it will be equal to frames_one_time in FH_AC_Raw_CONFIG
}FH_AC_Raw_S;

#define FH_AC_EXT_CMD_RINGBUF_CLR   (0)
#define FH_AC_EXT_CMD_RINGBUF_RESET (1)
#define FH_AC_EXT_CMD_AI_HPF        (2)
#define FH_AC_EXT_CMD_AO_HPF        (3)
#define FH_AC_EXT_CMD_AEC_DEFCONFIG (4)
#define FH_AC_EXT_CMD_RESERVED      (5)
#define FH_AC_EXT_CMD_POWERDOWN_MICBIAS (6)
#define FH_AC_EXT_CMD_AEC_FIXED_DELAY   (7)
#define FH_AC_EXT_CMD_PLAY_AGC          (8)
#define FH_AC_EXT_CMD_AI_HPF_COFF       (9)
#define FH_AC_EXT_CMD_SET_MIX_METHOD    (10)
#define FH_AC_EXT_CMD_SET_AI_ALC        (11)
#define FH_AC_EXT_CMD_SET_AO_ALC        (12)
#define FH_AC_EXT_CMD_TP_GET_TP_NUM     (13)
#define FH_AC_EXT_CMD_TP_CLR            (14)
#define FH_AC_EXT_CMD_TP_RESET          (15)
#define FH_AC_EXT_CMD_CAP_AGC           (16)

#define FH_AC_EXT_CMD_AEC_PROCESS   (30)
#define FH_AC_EXT_CMD_NR_PROCESS    (31)
#define FH_AC_EXT_CMD_NR2_PROCESS   (32)
#define FH_AC_EXT_CMD_AGC_PROCESS   (33)
#define FH_AC_EXT_CMD_DOWN_SAMPLE   (34)
#define FH_AC_EXT_CMD_UP_SAMPLE     (35)
#define FH_AC_EXT_CMD_AEC_INIT      (36)
#define FH_AC_EXT_CMD_NR_INIT       (37)
#define FH_AC_EXT_CMD_NR2_INIT      (38)

#define FH_AC_EXT_CMD_FLAG_MASK     (0xffff<<16) /*高16bit是特殊标志,小心使用*/
#define FH_AC_EXT_CMD_FLAG_LARGE_STACK  (1<<31)  /*firmware使用大堆栈标志,目前,audio 3A service需要大堆栈*/
typedef struct
{
    FH_UINT32 cmd;     /* 0 -19: reserved for fullhan
                        * 20-39: Aec
                        */
    FH_UINT32 arg_num; /*real arg number...*/
    FH_UINT32 args[8];
}FH_AC_EXT_IOCTL_CMD;

#define FH_AC_EXT2_CMD_READBACK_FLAG   (0x80) //don't care it
#define FH_AC_EXT2_CMD_SET_INIT_PARAM  (0) //don't care it
#define FH_AC_EXT2_CMD_SET_AI_EQ_PARAM (1) //don't care it
#define FH_AC_EXT2_CMD_SET_AO_EQ_PARAM (2) //don't care it
#define FH_AC_EXT2_CMD_GET_ALG_PARAM   (3 | FH_AC_EXT2_CMD_READBACK_FLAG) //don't care it
#define FH_AC_EXT2_CMD_SET_ALG_PARAM   (4) //don't care it
typedef struct
{
    FH_UINT32 cmd;
    FH_UINT32 subcmd;
    FH_UINT32 arg_bytes; /*real arg bytes...*/
    FH_UINT8  arg[0];
}FH_AC_EXT2_IOCTL_CMD;

#define FH_AC_AEC_MAGIC_DEBUG_ON_SITE (0xDA98E5C0) //used for debug,don't care it
#define FH_AC_NR2_MAGIC_TP_CLEAR      (0x3B89D7B2) //used for debug,don't care it
#define FH_AC_NR2_MAGIC_TP_MEM_ADDR   (0x3B89D7B3) //used for debug,don't care it
#define FH_AC_NR2_MAGIC_TP_MEM_SIZE   (0x3B89D7B4) //used for debug,don't care it

#endif /* FH_AUIDO_MPIPARA_H_ */
