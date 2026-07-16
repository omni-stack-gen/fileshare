#ifndef _GOBLIN_PLC_PROCESS_H_
#define _GOBLIN_PLC_PROCESS_H_


enum{
    PLC_DEVICE_TYPE_INDOOR=1,
    PLC_DEVICE_TYPE_DOOR = 3
};

typedef enum{
    Goblin_PLC_MSG_EVENTTYPE_UNLOCK=1,              //开锁
    Goblin_PLC_MSG_EVENTTYPE_ONLINE=2,              //设备上线
    Goblin_PLC_MSG_EVENTTYPE_SEARCH_REQ=3,          //设备搜索的请求
    Goblin_PLC_MSG_EVENTTYPE_SEARCH_RSP=4,          //设备搜索的ACK
    Goblin_PLC_MSG_EVENTTYPE_SETID=5,               //设置别墅机房号 一般室内机往门口机发
    Goblin_PLC_MSG_EVENTTYPE_GETCFG=6,              //获取门口机的配置
    Goblin_PLC_MSG_EVENTTYPE_SETCFG=7,              //设置门口机属性
    Goblin_PLC_MSG_EVENTTYPE_WAKEUP_REQ=8,          //设备唤醒的请求
    Goblin_PLC_MSG_EVENTTYPE_PLC_VERSION=9,         //PLC版本信息
    Goblin_PLC_MSG_EVENTTYPE_PCL_RSSI=10,           //PLC的RSSI信息
    Goblin_PLC_MSG_EVENTTYPE_REPORTAGENT=11,        //设备上报代理
    Goblin_PLC_MSG_EVENTTYPE_DISABLEAGENT=12,       //解除代理
    Goblin_PLC_MSG_EVENTTYPE_CHECKSTATE=13,         //检测对应操作状态
    Goblin_PLC_MSG_EVENTTYPE_GETLOGFILESIZE=14,     //获取日志文件大小
    Goblin_PLC_MSG_EVENTTYPE_GETLOGFILE=15,         //获取日志 通过goblin_plc_send_file_data发回去
    Goblin_PLC_MSG_EVENTTYPE_PLC_CONFLICT=16,       //PLC地址冲突
    Goblin_PLC_MSG_EVENTTYPE_SET_SN=17,             //设置SN值
    Goblin_PLC_MSG_EVENTTYPE_GET_SN=18,             //获取SN值
    Goblin_PLC_MSG_EVENTTYPE_CARD_ADDSTATE=19,      //添加卡号状态
    Goblin_PLC_MSG_EVENTTYPE_CARD_DELSTATE=20,      //删除卡号状态
    Goblin_PLC_MSG_EVENTTYPE_CARD_DELALL=21,        //删除所有卡号
    Goblin_PLC_MSG_EVENTTYPE_CARD_DELONE=22,        //删除一张卡
    Goblin_PLC_MSG_EVENTTYPE_CARD_GETLIST=23,       //获取卡号列表
    Goblin_PLC_MSG_EVENTTYPE_CARD_MODIFY=24,        //设置卡号信息
    Goblin_PLC_MSG_EVENTTYPE_CARD_OPT=25,           //卡操作上报，一般用于门口机加、删卡过程中的上报
    Goblin_PLC_MSG_EVENTTYPE_SET_LANGUPAGE=26,      //设置语言

    Goblin_PLC_MSG_EVENTTYPE_HARDWARE_TEST=27,      //硬件测试 (小门口机无屏，室内机控制)
    Goblin_PLC_MSG_EVENTTYPE_SET_AI_NOISE=28,      //设置AI降噪作用
    Goblin_PLC_MSG_EVENTTYPE_MAX
}Goblin_PLC_EventType_E;   //PLC通讯消息类型

//命令字 1：开始 2：结束 3:喇叭测试开始 4:喇叭测试结束 5:录音开始(默认10秒) 6:录音结束 7:放音开始 8:放音结束 9：灯控制(流水线闪烁测试)
enum{
    Goblin_PLC_HARDWARW_TEST_START=1,              //硬件测试开始
    Goblin_PLC_HARDWARW_TEST_END=2,                //硬件测试结束
    Goblin_PLC_HARDWARW_TEST_SPEAKER_START=3,     //喇叭测试开始
    Goblin_PLC_HARDWARW_TEST_SPEAKER_END=4,       //喇叭测试结束
    Goblin_PLC_HARDWARW_TEST_RECORD_START=5,      //录音开始
    Goblin_PLC_HARDWARW_TEST_RECORD_END=6,        //录音结束
    Goblin_PLC_HARDWARW_TEST_PLAY_START=7,        //放音开始
    Goblin_PLC_HARDWARW_TEST_PLAY_END=8,          //放音结束
    Goblin_PLC_HARDWARW_TEST_LIGHT_START=9,       //灯控制(流水线闪烁测试 )
    Goblin_PLC_HARDWARW_TEST_KEY=101,             //按键 0：松开 1：按下
    Goblin_PLC_HARDWARW_TEST_LDR=102,             //光敏 0 1
    Goblin_PLC_HARDWARW_TEST_RESET=103,           //复位
    Goblin_PLC_HARDWARW_TEST_DIP_SWITCH=104,      //拨码开关 0 1
    Goblin_PLC_HARDWARW_TEST_CARD=105,            //刷卡卡号
};

typedef struct
{
    uint32_t lockid;                   //门锁ID，累加   lockid为1表示第一把锁 lockid为2表示第二把锁
    uint32_t lockswitch;               //开锁电平
    uint32_t locktime;                 //开锁延时
    uint32_t menicswitch;              //是否启动门磁检测
    uint32_t menictime;                //门磁延时
    uint32_t locktype;                 //锁类型  1：Lo-Z Locker：LOCK1驱动10mS开锁（持续10mS高电平） 2:Hi-Z Locker：LOCK1驱动5S开锁（持续5S高电平） 注:第二把是触点锁
}goblin_door_lock_t;

#define FILE_START_FLAG 0x01
#define FILE_END_FLAG   0x02

#define FILE_PACK_SIZE  1937 //1984-5-2-1-32-4-2-1 = 1937 2032-5-2-1-32-4-2-1 = 1985
#pragma pack(1)
typedef struct file_pack_
{
	// cmd
	// 1) 呼叫命令字
	//      0、文件ACK fileseq
    //      1、主控的OTA升级固件
    //      2、PCL的OTA升级固件
    //      3、LOG文件
	uint8_t file_type;
    // 文件包序号
	uint16_t fileseq;

	// 文件的长度
	uint32_t filelen;
	// 文件的MD5值
	uint8_t filemd5[32];
	//文件开始结束标志 1：开始 2：结束 3:文件一个包
	uint8_t fileflag;

	// 文件载体的长度
	uint16_t datalen;
	// 文件载体 1984-5-2-1-32-4-2-1
	uint8_t data[FILE_PACK_SIZE];
} file_pack_t;
#pragma pack()


#define DOOR_LOCK_COUNT     2
typedef  struct
{
    uint32_t soundcall;                //振铃声音大小
    uint32_t soundtalk;                //对讲声音大小

    goblin_door_lock_t doorlock[DOOR_LOCK_COUNT];       //锁的数组
    uint32_t motion;                   //移动侦测灵敏度
}goblin_door_cfg_t;//门口机配置

typedef struct
{
	uint32_t id;
	uint32_t c_recv;
	uint32_t c_drop;
	uint8_t rssi;
}Plc_RssiList;

typedef struct
{
	char strid[32];			//设备的ID号
	int devtype;			//家庭设备类型
	char strmodel[32];		//设备型号
	int  platform;			//设备平台(二进制数据，从右往左)：1表示Android	2表示Linux 4表示Rtos
	char strmac[32];		//设备MAC地址 11-22-33-44-55-66
	char strapptype[32];	//应用类型
	char strappver[32];		//应用版本号
	char strsystype[32];	//系统类型
	char strsysver[32];		//系统版本号
	char strhwver[32];		//硬件版本号
	char strSN[64];			//机器SN码
	char strplcver[32];		//PLC固件版本号
    char strtimezone[32];   //时区
}goblin_agent_info_s;

typedef struct
{
    bool benableagent;	    //是否支持代理
    char strtimezone[32];   //时区
	char strdatetime[32];	//时间 yy-mm-dd hh:mm:ss
}goblin_agent_ack_s;

typedef struct
{
    char strcard[32];   //卡号 //16进制字符串
    char strroomid[32]; //房间号
    unsigned int placestate; //开锁位置二进制数据，从右往左依次表示第1把锁，第2把锁....1表示开锁，0表示不开锁
}goblin_plc_cardinfo_s;

typedef struct
{
    char stropt[32];    //操作内容 加卡: add  删卡: del
    char strcard[32];   //卡号 //16进制字符串
}goblin_plc_card_optinfo_s;
typedef struct
{
    //上位机命令字 1：开始 2：结束 3:喇叭测试开始 4:喇叭测试结束 5:录音开始(默认10秒) 6:录音结束 7:放音开始 8:放音结束 9：灯控制(流水线闪烁测试)
    //下位机命令字 101:按键序号  0：松开 1：按下 index序号从1开始   strstate 状态
    //            102:光敏     0：常态(光线正常) 1：触发(光线暗)用手按着即可  strstate 状态
    //            103:复位按钮  0：松开 1：按下  strstate状态
    //            104:拨码开关  0或1 代表切换1号或2号主机 strstate状态
    //            105:刷卡卡号   strstate 放的是卡号
    //            1000:控制视频AI增强 0：关闭 1：开启  strstate状态
    //            1001:控制摄像头补光灯亮度 0~100  strstate状态
    int  operate_cmd;  //操作命令字
    int  index;        //按键序号
    char strstate[32]; //状态信息
}golbin_hardware_test_s;

typedef struct plcprocess_eventinfo_
{
    Goblin_PLC_EventType_E type;
    uint32_t remotedev;                //远端设备的PLC号
    union
    {
        struct
        {
            uint32_t devid;                //设备的PLC号
            int place;                  //开锁位置二进制数据，从右往左依次表示第1把锁，第2把锁....1表示开锁，0表示不开锁
        }unlock; //Goblin_PLC_MSG_EVENTTYPE_UNLOCK

        struct
        {
            uint32_t devid;                //设备的PLC号
            int type;                    //设备类型
            char hardware_version[32];   //设备硬件版本号
            char software_version[32];   //设备软件版本号
            uint32_t plc_version;           //设备的PLC固件版本号
            bool benableagent;	            //是否支持代理
        }Onlineinfo; //Goblin_PLC_MSG_EVENTTYPE_ONLINE

        struct
        {
            uint32_t devid;                //设备的PLC号
            int type;                      //设备类型
            int roomid;                    //房间号

            //回复的赋值
            uint32_t ackdev;               //
            int acktype;
            char ackhardware_version[32];   //设备硬件版本号
            char acksoftware_version[32];   //设备软件版本号
            int ackstate;                   //0：空虚 1：繁忙
            uint32_t ackplc_version;        //设备的PLC固件版本号
            bool ackbenableagent;	            //是否支持代理
            char ackSN[64];                 //设备SN号
            char ackDevName[64];            //设备名称
            uint32_t languageindex;         //语言索引 1：阿拉伯语 2: 德语 3.俄语 4.法语 5.韩语 6:日语 7:土耳其语 8:西班牙语 9:希伯来语 10:英文
        }search_req; //Goblin_PLC_MSG_EVENTTYPE_SEARCH_REQ

        struct
        {
            uint32_t devid;                //设备的PLC号
            int type;                    //设备类型
            int state;                   //0：空虚 1：繁忙
            char hardware_version[32];   //设备硬件版本号
            char software_version[32];   //设备软件版本号
            uint32_t plc_version;           //设备的PLC固件版本号
            bool benableagent;	            //是否支持代理
            char strSN[64];                 //设备SN号
            char strDevName[64];            //设备名称
            uint32_t languageindex;         //语言索引
        }search_rsp; //Goblin_PLC_MSG_EVENTTYPE_SEARCH_RSP

        struct
        {
            uint32_t devid;                //设备的PLC号
            char strid[32];                    //PLC ID的字符串， 按16进制
        }set_id;//Goblin_PLC_MSG_EVENTTYPE_SETID

        struct
        {
            goblin_door_cfg_t paraminfo;
        }door_cfg;//Goblin_PLC_MSG_EVENTTYPE_GETCFG(out)/Goblin_PLC_MSG_EVENTTYPE_SETCFG

        struct
        {
            uint32_t roomid;    //房间号
        }wakeup_info;   //Goblin_PLC_MSG_EVENTTYPE_WAKEUP_REQ

        struct
        {
            uint32_t version;                //设备的PLC固件版本号
        }plc_version; //Goblin_PLC_MSG_EVENTTYPE_PLC_VERSION

        struct
        {
            uint32_t plc_count;              //局域网PLC数量
            Plc_RssiList *prssi_list;         //各个PLC的rssi列表
        }plc_rssilist;             //Goblin_PLC_MSG_EVENTTYPE_PCL_RSSI

        struct
        {
            goblin_agent_info_s agentinfo;      //代理设备信息
            goblin_agent_ack_s outinfo;         //收到消息回复给对端的
        }agent_dev;//Goblin_PLC_MSG_EVENTTYPE_REPORTAGENT

        struct
        {
            /* data */
            char straction[64];                 //执行的动作 例如： 升级："update" 重启"reboot" 复位"reset" 关机"poweroff"

            int outstate;                       //动作状态 0:空闲状态(即可执行对应动作)  1:忙碌状态(不可执行)
        }action_state;//Goblin_PLC_MSG_EVENTTYPE_CHECKSTATE

        struct
        {
            uint32_t outfilelen;                //要发送的文件的长度
        }logfile_info;//Goblin_PLC_MSG_EVENTTYPE_GETLOGFILE 、Goblin_PLC_MSG_EVENTTYPE_GETLOGFILESIZE

        struct
        {
            char strsn[64];                //设置/获取的SN值
        }sn_info;//Goblin_PLC_MSG_EVENTTYPE_SET_SN /Goblin_PLC_MSG_EVENTTYPE_GET_SN

        struct
        {
            int state;             //卡号添加状态 0:停止加卡 1:开始加卡
        }card_add_state; //Goblin_PLC_MSG_EVENTTYPE_CARD_ADDSTATE

        struct
        {
            int state;             //卡号删除状态 0:停止删除 1:开始删除
        }card_del_state; //Goblin_PLC_MSG_EVENTTYPE_CARD_DELSTATE

        struct
        {
            char strcard[32];              //卡号
        }card_del_one; //Goblin_PLC_MSG_EVENTTYPE_CARD_DELONE

        struct
        {
            uint32_t card_count;                //卡号数量
            goblin_plc_cardinfo_s *pcardlist;   //卡号列表  应用层malloc申请的内存，库里面释放
        }card_get_list_ack; //Goblin_PLC_MSG_EVENTTYPE_CARD_GETLIST

        struct
        {
            goblin_plc_cardinfo_s cardinfo;    //卡号信息
        }card_info_modify; //Goblin_PLC_MSG_EVENTTYPE_CARD_MODIFY

        struct
        {
            goblin_plc_card_optinfo_s card_opt_info;
        }card_operation_report; //Goblin_PLC_MSG_EVENTTYPE_CARD_OPT

        struct
        {
            /*1：阿拉伯语 2: 德语 3.俄语 4.法语 5.韩语
            6:日语 7:土耳其语 8:西班牙语 9:希伯来语 10:英文
            */
            uint32_t languageindex;     //
        }language_info; //Goblin_PLC_MSG_EVENTTYPE_SET_LANGUPAGE

        struct
        {
            bool benable;
        }action_ai_noise; //Goblin_PLC_MSG_EVENTTYPE_SET_AI_NOISE

        golbin_hardware_test_s hardware_test; //Goblin_PLC_MSG_EVENTTYPE_HARDWARE_TEST

    }eventinfo;

}PlcEventHandleInfo_S;  //信息处理

typedef int (*PlcEventHandleFun)(void *param, PlcEventHandleInfo_S *paraminfo);
typedef int (*PlcFileCallBackFun)(void *param, file_pack_t *paraminfo, uint32_t remotedev);

typedef struct {
    uint32_t            m_pdevplc;              //设备本身ID plc号
    PlcEventHandleFun   m_pfuncb;               //传输消息回调
    void *              m_pcbparam;             //回调参数
    PlcFileCallBackFun  m_pfilecb;              //文件传输回调
    void *              m_filecbparam;          //文件传输回调参数

    int                 m_devinfo_type;         //设备类型

    char *              m_pdevver_hw;           //硬件版本号(非必填)
    char *              m_pdevver_sys;          //系统版本号
    char *              m_pdevver_systype;      //系统类型(非必填)
    char *              m_pdevver_app;          //应用版本类型
    char *              m_pdevver_apptype;      //应用类型(非必填)

    bool                m_benableagent;         //是否支持代理
} PlcEventHandler;



/**
 * @brief 开启本地信息传输的通道
 * @param [in] param 传参
 * @return int 0:success -1:failed
 */
int goblin_plc_process_start(PlcEventHandler *param);
// 使用 typedef 定义函数指针类型
typedef int (*GoblinPlcProcessStartFunc)(PlcEventHandler *param);

/**
 * @brief 停止本地信息传输的通道
 * @return int 0:success
 */
int goblin_plc_process_stop(void);
typedef int (*GoblinPlcProcessStopFunc)(void);

/**
 * @brief 重新设置本地PLC号
 * @param [in] devid
 * @return int 0 :success
 */
int goblin_plc_reload_devID(uint32_t devid);
typedef int (*GoblinPlcReloaDevID)(uint32_t devid);

/**
 * @brief 重新设置本代理状态
 * @param [in] benableagent
 * @return int 0:success
 */
int goblin_plc_reload_agent(bool benableagent);
typedef int (*GoblinPlcReloadAgent)(bool benableagent);

/**
 * @brief 开锁
 * @param [in] remotedev 对端设备号
 * @param [in] lockp   //开锁位置
二进制数据，从右往左
依次表示第1把锁，第2把锁....
1表示开锁，0表示不开锁
 * @param [in] timeout      超时时间(毫秒) 0的时候不等待，直接退出
 * @return int 0:成功 其他失败
 */
int goblin_plc_Unlock(uint32_t remotedev, int lockp, unsigned int timeout);
typedef int (*GoblinPlcUnlockFunc)(uint32_t remotedev, int lockp, unsigned int timeout);

/**
 * @brief 搜索设备
 * @param [in] remotedev 要搜索的设备PLC号， 0xFFFFFFFF是广播，所有都可以收到
 * @param [in] searchtype 要搜索的类型
 * @param [in] roomid 要搜索的房间号 0：所有房间
 * @return int  0:成功 其他失败
 */
int goblin_plc_Search(uint32_t remotedev, uint32_t searchtype, uint32_t roomid);
typedef int (*GoblinPlcSearchFunc)(uint32_t remotedev, uint32_t searchtype, uint32_t roomid);


/**
 * @brief 设置远端设备ID
 * @param [in] remotedev    远端的PLC号
 * @param [in] strdevid     设置的设备ID号
 * @param [in] timeout      超时时间(毫秒) 0的时候不等待，直接退出
 * @return int  0：成功
 */
 int goblin_plc_set_remote_id(uint32_t remotedev, char *strdevid, unsigned int timeout);
 typedef int (*GoblinPlcSetRemoteId)(uint32_t remotedev, char *strdevid, unsigned int timeout);

/**
 * @brief 获取门口机信息
 * @param [in] remotedev
 * @param [out] doorcfg
 * @param [in] timeout
 * @return int  0:success
 */
int goblin_plc_get_door_cfg(uint32_t remotedev, goblin_door_cfg_t *doorcfg, unsigned int timeout);
typedef int (*GoblinPlcGetDoorCfg)(uint32_t remotedev, goblin_door_cfg_t *doorcfg, unsigned int timeout);


/**
 * @brief 设置门口机信息
 * @param [in] remotedev
 * @param [in] doorcfg
 * @param [in] timeout
 * @return int 0:success
 */
int goblin_plc_set_door_cfg(uint32_t remotedev, goblin_door_cfg_t *doorcfg, unsigned int timeout);
typedef int (*GoblinPlcSetDoorCfg)(uint32_t remotedev, goblin_door_cfg_t *doorcfg, unsigned int timeout);


/**
 * @brief 上传代理信息
 * @param [in] remotedev 对端设备
 * @param [in] pinfo 上报的信息
 * @param [out] packinfo 对端回复的消息
 * @param [in] timeout 超时(毫秒)
 * @return int 0：收到对端回复 other:没有收到
 */
int goblin_plc_check_agent(uint32_t remotedev, goblin_agent_info_s *pinfo, goblin_agent_ack_s *packinfo, unsigned int timeout);
typedef int (*GoblinPlcCheckAgent)(uint32_t remotedev, goblin_agent_info_s *pinfo, goblin_agent_ack_s *packinfo, unsigned int timeout);

/**
 * @brief 解除代理
 * @param [in] remotedev 对端设备地址
 * @param [in] timeout  超时时间（毫秒）
 * @return int 0:成功 其他失败
 * 注：代理和被代理的可以互发
 */
int goblin_plc_disable_agent_dev(uint32_t remotedev, uint32_t timeout);
typedef int (*GoblinPlcDisableAgentDev)(uint32_t remotedev, uint32_t timeout);

/**
 * @brief 检测对端是否可以执行某个动作(例如升级前查询设备是否可以， 防止资源不够)
 * @param [in] remotedev  对端地址
 * @param [in] paction  要执行的动作 例如; 升级"upgrade"
 * @param [in] timeout 超时时间（毫秒）
 * @return int 0:成功，对端可以执行该动作 other：不可执行
 */
int goblin_plc_checkstate(uint32_t remotedev, char *paction, unsigned int timeout);
typedef int (*GoblinPlcCheckState)(uint32_t remotedev, char *paction, unsigned int timeout);


/**
 * @brief 获取日志文件大小
 * @param [in] remotedev 对端设备地址
 * @param [in] timeout  超时时间(毫秒) 0的时候不等待，直接退出
 * @return int ：要接收的文件的长度 ,大于0表示成功
 */
int goblin_plc_getlogfilesize(uint32_t remotedev, unsigned int timeout);
typedef int (*GoblinPlcGetLogFileSize)(uint32_t remotedev, unsigned int timeout);

/**
 * @brief 获取log文件
 * @param [in] remotedev
 * @param [in] timeout
 * @return int ： 要接收的文件的长度
 */
int goblin_plc_getlogfile(uint32_t remotedev, unsigned int timeout);
typedef int (*GoblinPlcGetLogFile)(uint32_t remotedev, unsigned int timeout);

/**
 * @brief 设置远端的SN值
 * @param [in] remotedev 远端地址
 * @param [in] psn   sn值
 * @param [in] timeout 超时时间（毫秒）
 * @return int 0:成功 other:fail
 */
int goblin_plc_setsn(uint32_t remotedev, char *psn, unsigned int timeout);
typedef int (*GoblinPlcSetsn)(uint32_t remotedev, char *psn, unsigned int timeout);


/**
 * @brief 获取对端SN值
 * @param [in] remotedev  对端地址
 * @param [out] poutsn  sn值缓冲区(至少64字节)
 * @param [in] timeout  超时时间（毫秒）
 * @return int 0:成功 other:fail
 */
int goblin_plc_getsn(uint32_t remotedev, char *poutsn, unsigned int timeout);
typedef int (*GoblinPlcGetsn)(uint32_t remotedev, char *poutsn, unsigned int timeout);

/**
 * @brief 设置本房间在门口机的播放语言
 * @param [in] remotedev  对端设备地址
 * @param [in] languageindex     语言索引
 * @param [in] timeout  超时时间(毫秒) 0的时候不等待，直接退出
 * @return int 0:success other:fail
 * 1：阿拉伯语 2: 德语 3.俄语 4.法语 5.韩语
            6:日语 7:土耳其语 8:西班牙语 9:希伯来语 10:英文
 */
int goblin_plc_setlanguage(uint32_t remotedev, uint32_t languageindex, unsigned int timeout);
typedef int (*GoblinPlcSetLanguage)(uint32_t remotedev, uint32_t languageindex, unsigned int timeout);

/**
 * @brief 获取设置对端是否启用AI噪声
 * @param [in] remotedev  对端设备地址
 * @param [out] isonoff  是否
 * @param [in] timeout  超时时间(毫秒) 0的时候不等待，直接退出
 * @return int 0:success other:fail
 */
int goblin_plc_set_ai_noise(uint32_t remotedev, bool isonoff, unsigned int timeout);
typedef int (*GoblinPlcSetAiNoise)(uint32_t remotedev, bool isonoff, unsigned int timeout);

/**
 * @brief 硬件测试操作
 * @param [in] remotedev  对端设备地址
 * @param [in] operateinfo  操作相关信息
 * @param [in] timeout  超时时间(毫秒) 0的时候不等待，直接退出
 * @return int 0:成功 其他失败
 */
int goblin_plc_hardware_test(uint32_t remotedev, golbin_hardware_test_s *operateinfo, unsigned int timeout);
typedef int (*GoblinPlcHardwareTest)(uint32_t remotedev, golbin_hardware_test_s *operateinfo, unsigned int timeout);

/**
 * @brief 通知PCL关机
 * @return int 0：成功
 */
int goblin_plc_close(void);
typedef int (*GoblinPlcClose)(void);


/**
 * @brief 打开对应slave设备的plc供电(master端控制)
 * @param [in] devid
 * @return int 0:成功
 */
int goblin_plc_open_dev(uint32_t devid);
typedef int (*GoblinPlcOpenDev)(uint32_t devid);

/**
 * @brief 通过房间号唤醒设备(slave端发给master端)
 * @param [in] remotedev  master端的PLC号
 * @param [in] roomid     房间号
 * @param [in] timeout    超时时间(毫秒) 0的时候不等待，直接退出
 * @return int 0:成功
 */
int goblin_plc_wakeup_dev_by_roomid(uint32_t remotedev, uint32_t roomid, uint32_t timeout);
typedef int (*GoblinPlcWakeupDevByRoomid)(uint32_t remotedev, uint32_t roomid, uint32_t timeout);

/**
 * @brief 获取PLC的信号值
 * @return int
 * 获取到的信号通过Goblin_PLC_MSG_EVENTTYPE_PCL_RSSI回调返回，每次获取都会刷新
 */
int goblin_plc_get_rssi(void);
typedef int (*GoblinPlcGetRssi)(void);

/**
 * @brief 升级PLC固件
 * @param [in] pfilepath  固件文件路径
 * @return int
 * 升级完要rebooot才能生效
 */
int goblin_plc_upgrade_firmware(char *pfilepath);
typedef int (*GoblinPlcUpgradeFirmware)(char *pfilepath);

/**
 * @brief 重启PLC
 * @return int 0：成功
 * 升级完要rebooot才能生效
 */
int goblin_plc_reboot(void);
typedef int (*GoblinPlcReboot)(void);

/**
 * @brief 使能门口机的加卡状态
 * @param [in] remotedev 对端设备地址
 * @param [in] state    1：开始 0：停止
 * @param [in] timeout  超时时间(毫秒) 0的时候不等待，直接退出
 * @return int 0:success other:fail
 */
int goblin_plc_addcard_state(uint32_t remotedev, int state, unsigned int timeout);
typedef int (*GoblinPlcAddcardState)(uint32_t remotedev, int state, unsigned int timeout);

/**
 * @brief 使能门口机的删除卡状态
 * @param [in] remotedev    对端设备地址
 * @param [in] state        1：开始 0：停止
 * @param [in] timeout      超时时间(毫秒) 0的时候不等待，直接退出
 * @return int 0:success other:fail
 */
int goblin_plc_delcard_state(uint32_t remotedev, int state, unsigned int timeout);
typedef int (*GoblinPlcDelcardState)(uint32_t remotedev, int state, unsigned int timeout);


/**
 * @brief 删除所有门口机的卡号
 * @param [in] remotedev    对端设备地址
 * @param [in] timeout  超时时间(毫秒) 0的时候不等待，直接退出
 * @return int 0:success other:fail
 */
int goblin_plc_delcard_all(uint32_t remotedev,  unsigned int timeout);
typedef int (*GoblinPlcDelcardAll)(uint32_t remotedev,  unsigned int timeout);

/**
 * @brief 删除门口机的指定卡号
 * @param [in] remotedev    对端设备地址
 * @param [in] cardid       卡号
 * @param [in] timeout      超时时间(毫秒) 0的时候不等待，直接退出
 * @return int  0:success other:fail
 */
int goblin_plc_delcard_by_id(uint32_t remotedev, char *cardid,  unsigned int timeout);
typedef int (*GoblinPlcDelcardById)(uint32_t remotedev, char *cardid,  unsigned int timeout);

/**
 * @brief 获取门口机的卡号列表
 * @param [in] remotedev    对端设备地址
 * @param [out] pcardinfo   卡号列表
 * @param [out] pcardnum    卡号数量
 * @param [in] timeout      超时时间(毫秒) 0的时候不等待，直接退出
 * @return int  0:success other:fail
 */
int goblin_plc_get_door_card(uint32_t remotedev, goblin_plc_cardinfo_s **pcardinfo, unsigned int *pcardnum, unsigned int timeout);
typedef int (*GoblinPlcGetDoorCard)(uint32_t remotedev, goblin_plc_cardinfo_s **pcardinfo, unsigned int *pcardnum, unsigned int timeout);

/**
 * @brief 修改门口机的卡号信息
 * @param [in] remotedev    对端设备地址
 * @param [in] pcardinfo    卡号信息
 * @param [in] timeout      超时时间(毫秒) 0的时候不等待，直接退出
 * @return int 0:success other:fail
 */
int goblin_plc_cardinfo_modify(uint32_t remotedev, goblin_plc_cardinfo_s *pcardinfo, unsigned int timeout);
typedef int (*GoblinPlcCardinfoModify)(uint32_t remotedev, goblin_plc_cardinfo_s *pcardinfo, unsigned int timeout);

/**
 * @brief 上报卡操作信息
 * @param [in] remotedev    对端设备地址
 * @param [in] pcardopt     卡操作信息
 * @param [in] timeout      超时时间(毫秒) 0的时候不等待，直接退出
 * @return int 0:success other:fail
 */
int goblin_plc_report_card_opt(uint32_t remotedev, goblin_plc_card_optinfo_s *pcardopt, unsigned int timeout);
typedef int (*GoblinPlcReportCardOpt)(uint32_t remotedev, goblin_plc_card_optinfo_s *pcardopt, unsigned int timeout);

//=====================文件发送相关接口================================

/**
 * @brief 发送文件数据
 * @param [in] remotedev 对端地址
 * @param [in] pfiledata  文件数据内容
 * @param [in] timeout   超时时间(毫秒) 0的时候不等待，直接退出
 * @return int 0 :success -1:失败
 */
int goblin_plc_send_file_data(uint32_t remotedev, file_pack_t *pfiledata, unsigned int timeout);
typedef int (*GoblinPlcSendFileData)(uint32_t remotedev, file_pack_t *pfiledata, unsigned int timeout);

#endif /* _GOBLIN_PLC_PROCESS_H_ */