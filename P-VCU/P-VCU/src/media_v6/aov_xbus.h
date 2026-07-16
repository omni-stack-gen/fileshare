#ifndef __AOV_XBUS_COMMAND_H__
#define __AOV_XBUS_COMMAND_H__



#define AOV_XBUS_COMMAND   0x78414F56 //xAOV


typedef enum  {
	AOV_TASK_STOP = 0,
	AOV_TASK_RUN,
}AOV_TASK_STATE;


typedef enum {
	AOV_CMD_QUERY_STATE = 0x41305600,
    AOV_CMD_VIDEO_READY,
    AOV_CMD_VIDEO_EXIT,
	AOV_CMD_START_AOV,
	AOV_CMD_SLEEP,
	AOV_CMD_WAKE_REASON,
	AOV_CMD_SAVE_JPG_FRAME,
	AOV_CMD_SET_JPG_PARAM,
} AVO_RTT_CMD_ID;

typedef enum
{
    AOV_WAKEUP_NONE = (1 << 0),
    AOV_WAKEUP_PIR = (1 << 1),
    AOV_WAKEUP_USER = (1 << 2),
    AOV_WAKEUP_NN = (1 << 3),
    AOV_WAKEUP_BUFFFULL = (1 << 4),
    AOV_WAKEUP_PLC = (1 << 5),
    AOV_WAKEUP_KEY1 = (1 << 6),
    AOV_WAKEUP_KEY2 = (1 << 7),
    AOV_WAKEUP_KEY3 = (1 << 8),
    AOV_WAKEUP_KEY4 = (1 << 9),
} AOV_WAKEUP_REASON;


struct aov_ctrl {
    int aov_enable;
    int fream_num;      //nn或pir触发时连续抓取图片数量
    int aov_interval;
};

struct stream_buf {
    void* stream_addr;
    int   stream_size;
};

struct video_cfg
{
    int AovChnNum;
    int jpeg_enable;
    int nn_enable;
    int md_enable;
    int osd_enable;
    int gpio_enable;
};

struct wake_source {
    int grpid;
    AOV_WAKEUP_REASON   wakeup_reason;      //rtt唤醒linux原因
};

struct jpg_param {
	FH_UINT32 chn;
	FH_SIZE size;
	FH_UINT32 qp;
};

typedef struct {
	int cmd;
    int grpid;
    //wake reason

    union {
        //rtt stream
        struct stream_buf buf;
        //stream buf
        struct aov_ctrl aov;

        struct video_cfg video;

        struct wake_source wake_source;
		struct jpg_param jpgcfg;
    } data;

} AOV_XBUS_CMD_S;


#endif // __AOV_XBUS_COMMAND_H__
