#ifndef _GOBLIN_PLC_CALLCORE_H_
#define _GOBLIN_PLC_CALLCORE_H_

#include <sys/types.h>


// 呼叫状态
typedef enum
{
	PLC_CALL_STATE_CALLIN = 0,              // 呼入
	PLC_CALL_STATE_CALLOUT_ACK,             // 呼出状态
    PLC_CALL_STATE_ACCEPT,                  // 对端接听
	PLC_CALL_STATE_HUNGUP,                  // 挂断
    PLC_CALL_STATE_HANGUP_ACK,              // 挂断Ack
    PLC_CALL_STATE_REQUEST_I,               // I帧请求
    PLC_CALL_STATE_REQUEST_VIEW,            // 请求切换视角
    PLC_CALL_STATE_DISCONNECT,              // 断开连接
    PLC_CALL_STATE_VIDEO_DEC_START,         // 收到视频开始解I帧
	PLC_CALL_STATE_MAX,                     //
} PLC_CALL_STATE_E;

typedef struct plc_call_eventinfo_
{
    PLC_CALL_STATE_E type;
    void *psession;              //呼叫的session
    uint32_t dev;                //设备的PLC号

    union
    {
        struct
        {
            int state;
            int video_width;    //传过来的对端视频宽
            int video_height;   //传过来的对端视频高

            uint8_t video_type;   //对端请求的视频类型 0:主码流 1:子码流
        }callin; //PLC_CALL_STATE_CALLIN

        struct
        {
            int state;                  //0:空虚，可接听 1：繁忙
            int video_width;            //传过来的对端视频宽
            int video_height;           //传过来的对端视频高
        }callout_ack; //PLC_CALL_STATE_CALLOUT_ACK

        struct
        {
            int reason;     //0:正常挂断 1:超时挂断
        }hungup;//PLC_CALL_STATE_HUNGUP

        struct
        {
            int iscrop;       //是否裁剪视角 0:全 1:居中
        }request_view;  //PLC_CALL_STATE_REQUEST_VIEW

    }eventinfo;

}plc_call_info_s;  //信息处理

typedef int (*PlcCallEventHandleFun)(void *param, plc_call_info_s *paraminfo);
typedef int (*PlcVideoEncStatusCB)(void *param, uint8_t vencstart);
typedef int (*PlcVideoDropStatusCB)(uint32_t dropcount, uint32_t recvcount, float droprate, void *puserdata); //收到的丢包统计上报 对发送端有效

typedef struct _call_info_
{
    /* 本机终端对应Plc 号 */
    uint32_t  			    dev_plc;
    PlcCallEventHandleFun   handfunc;
    void *                  pusrdata;//用户自定义数据

    //视频显示的初始位置
    uint32_t display_x;
	uint32_t display_y;
	uint32_t display_w;
	uint32_t display_h;

    uint32_t audio_play_volume; //通话音频播放音量
    uint8_t is_disable_video_cache; //是否禁止视频缓存(默认缓存的，置1就不启用缓存)
    uint8_t is_disable_video_decshow;//是否禁止传输库解码显示视频流(默认是解码显示的， 1为禁止)

    PlcVideoEncStatusCB     pvideo_enc_status_cb; //视频编码状态回调
    void *                  pvideo_enc_status_usrdata; //视频编码状态回调用户数据

    PlcVideoDropStatusCB    pvideo_drop_status_cb; //视频丢包统计回调
    void *                  pvideo_drop_status_usrdata; //视频丢包统计回调用户数据
} Call_Info_S;


typedef int (*RemoteVideoCB)(void *param, uint8_t *pframedate, uint32_t framelen, uint8_t bkey);
typedef int (*TransAudioCB)(void *param, uint8_t *pframedate, uint32_t framelen);

/**
 * @brief 初始化PLC通话库
 * @param [in] param
 * @return int
 */
int goblin_call_init(Call_Info_S *param);
typedef int (*GoblinCallInit)(Call_Info_S *param);

/**
 * @brief 反初始化PLC通话库
 * @return int
 */
int goblin_call_deinit(void);
typedef int (*GoblinCallDeinit)(void);
/**
 * @brief 设置房号后，重新加载PLC号
 * @param [in] devid
 * @return int 0:success -1:fail
 */
int goblin_call_reload_devID(u_int32_t devid);
typedef int (*GoblinCallReloadDevID)(u_int32_t devid);

/**
 * @brief 重新设置默认通话音量
 * @param [in] volume 通话音量 0-100
 * @return int 0：成功 -1：失败
 */
int goblin_call_reload_AudioVolume(u_int32_t volume);
typedef int (*GoblinCallReloadAudioVolume)(u_int32_t volume);

/**
 * @brief 根据设备号发起呼叫
 * @param [in] dev
 * @return void*  会话指针
 */
void *goblin_call_by_dev(u_int32_t dev);
typedef void *(*GoblinCallByDev)(u_int32_t dev);

/**
 * @brief 根据设备号发起呼叫
 * @param [in] dev
 * @param [in] video_type 0:主码流 1:子码流 请求对端视频流(一般用于室内机对门口机)
 * @return void*  会话指针
 */
void *goblin_call_by_dev_ex(u_int32_t dev, uint8_t video_type);
typedef void *(*GoblinCallByDevEx)(u_int32_t dev, uint8_t video_type);

/**
 * @brief 呼叫ACK
 * @param [in] psession
 * @param [in] state
 * @return int
 */
int goblin_call_ack_state(void *psession, int state);
typedef int (*GoblinCallAckState)(void *psession, int state);

/**
 * @brief 接听
 * @param [in] psession
 * @return int
 */
int goblin_call_accept(void *psession, uint8_t baudiotran);
typedef int (*GoblinCallAccept)(void *psession, uint8_t baudiotran);

/**
 * @brief 挂断
 * @param [in] psession
 * @return int
 */
int goblin_hangup_by_session(void *psession);
typedef int (*GoblinHangupBySession)(void *psession);


/**
 * @brief 挂断会话
 * @param [in] psession 会话指针
 * @param [in] reason 挂断原因 0:正常挂断 1:超时挂断(在未接通的情况下，室内机对门口机发正常挂断，门口机全部挂断， 如果是超时挂断，则只挂断对应的)
 * @return int 0:成功 -1:失败
 */
int goblin_hangup_by_sessionAndReason(void *psession, uint8_t reason);
typedef int (*GoblinHangupBySessionAndReason)(void *psession, uint8_t reason);

/**
 * @brief 请求I帧
 * @param [in] psession
 * @return int
 */
int goblin_call_request_ikey_by_session(void *psession);
typedef int (*GoblinCallRequestIkeyBySession)(void *psession);

/**
 * @brief 请求图像视角裁剪
 * @param [in] psession
 * @param [in] iscrop
 * @return int
 */
int goblin_call_request_crop_by_session(void *psession, uint8_t iscrop);
typedef int (*GoblinCallRequestCropBySession)(void *psession, uint8_t iscrop);

/**
 * @brief 请求是否开启音频(在监视过程中打开对讲)
 * @param [in] psession
 * @param [in] onoff
 * @return int
 */
int goblin_tran_auido_by_session(void *psession, uint8_t onoff);
typedef int (*GoblinTranAudioBySession)(void *psession, uint8_t onoff);



/**
 * @brief 设置播放音量
 * @param [] psession
 * @param [] volume
 * @return * int
 */
int goblin_set_volume_by_session(void *psession, int volume);
typedef int (*GoblinSetVolumeBySession)(void *psession, int volume);

/**
 * @brief 设置视频传输
 * @param [in] psession
 * @param [in] isonoff 开(1)或者关(0)
 * @return int 0:成功 -1:失败
 */
int goblin_set_video_trans_by_session(void *psession, uint8_t isonoff);
typedef int (*GoblinSetVideoTransBySession)(void *psession, uint8_t isonoff);

/**
 * @brief 设置视频显示范围
 * @param [in] psession 会话指针
 * @param [in] x
 * @param [in] y
 * @param [in] w
 * @param [in] h
 * @return int 0:成功
 */
int goblin_set_video_displayarea_by_session(void *psession, int x, int y, int w, int h);
typedef int (*GoblinSetVideoDisplayareaBySession)(void *psession, int x, int y, int w, int h);

/**
 * @brief 设置接收视频流回调
 * @param [in] psession
 * @param [in] pcb
 * @param [in] pusrdata
 * @return int
 */
int goblin_set_video_cb_by_session(void *psession, RemoteVideoCB pcb, void *pusrdata);
typedef int (*GoblinSetVideoCbBySession)(void *psession, RemoteVideoCB pcb, void *pusrdata);

/**
 * @brief 设置子码流码视频数据回调
 * @param [in] psession
 * @param [in] pcb
 * @param [in] pusrdata
 * @return int
 */
int goblin_set_video_sub_cb_by_session(void *psession, RemoteVideoCB pcb, void *pusrdata);
typedef int (*GoblinSetVideoSubCbBySession)(void *psession, RemoteVideoCB pcb, void *pusrdata);

/**
 * @brief 设置声音传输数据回调
 * @param [in] psession
 * @param [in] paocb        播放音频回调， 对端传过来的数据
 * @param [in] pusraodata
 * @param [in] paicb        采集音频回调， 传到对端的数据
 * @param [in] pusraidata
 * @return int
 */
int goblin_set_audio_cb_by_session(void *psession, TransAudioCB paocb, void *pusraodata, TransAudioCB paicb, void *pusraidata);
typedef int (*GoblinSetAudioCbBySession)(void *psession, TransAudioCB paocb, void *pusraodata, TransAudioCB paicb, void *pusraidata);

/**
 * @brief 推送视频流
 * @param [in] channel
 * @param [in] pdata
 * @param [in] dlen
 * @return int
 */
int goblin_put_video_frame(int channel,void *pdata,int dlen);


/**
 * @brief 设置AI噪声
 * @param [in] psession
 * @param [in] isonoff 开(1)或者关(0)
 * @return int 0:成功 -1:失败
 * @note 该接口用于控制AI噪声 目前只V6门口机支持
 */
int goblin_set_ai_noise_by_session(void *psession, uint8_t isonoff);


#endif //_GOBLIN_PLC_CALLCORE_H_