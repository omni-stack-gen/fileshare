#ifndef _SYSTEMMSG_H_
#define _SYSTEMMSG_H_

#include <stdbool.h>

typedef struct
{
	unsigned int x;
	unsigned int y;
	unsigned int flag;
} TOUCHDATA;

typedef struct
{
	unsigned int key;
	unsigned int flag;
} KBDDATA;

typedef struct
{
	unsigned int msg;
	unsigned int wParam;
	unsigned int lParam;
	unsigned int zParam;
} SYS_MSG;


#define	MSG_URGENT_TYPE			0
#define	MSG_USER_TYPE			1
#define	MSG_TIME_TYPE			2
#define	MSG_TOUCH_TYPE			3
#define	MSG_KEY_TYPE			4
#define	MSG_MAX_TYPE			5

#define	TIME_MESSAGE			10000		// 时间消息

#define HARDKBD_MESSAGE  		10001		// 按键的原始消息
#define	KBD_MESSAGE				10002
#define	KBD_DOWN				0
#define	KBD_UP					1
#define KBD_CTRL				2			// 键盘控件消息
#define	KBD_LONGPRESS			3			// 长按
#define	KBD_GUARD				4			// 按呼管理中心
#define KBD_CANCEL				89
#define KBD_SURE				0x8
#define KEY_CALL_MANAGE			0x1002


#define KBD_ARROWUP				0x3000
#define KBD_ARROWDOWN			0x3001


#define	MSG_SYSTEM				10005		// 系统消息
#define	REBOOT_MACH				1			// 重启终端
#define	UPDATE_NETCFG			2			// 更新配置
#define	CODE_CHANGE				3			// 号码变化
#define	RESET_MACH				4			// 恢复出厂设置
#define WATCHDOG_CHANGE			5			// 是否开始看门狗
#define TAMPER_ALARM_HAPPEN		6			// 防拆报警

#define	MSG_BROADCAST			10006		// 广播消息，当前存在的所有窗口都可以收到
#define	NETWORK_CHANGE			1			// 网络状态变化
#define	PHOTO_CHANGE			2			// 留影状态变化
#define	MESSAGE_CHANGE			3			// 消息变化
#define	SAFE_CHANGE				4			// 安防变化
#define ALARM_CHANGE			5			// 报警变化
#define	CONFIG_GET				6			// 获取配置
#define ELEVATOR_RET            7			// 梯控消息
#define VOLUEME_CHANGE			8			// 声音改变
#define WEATHER_CHANGE			9			// 温度变化
#define CALLLOG_CHANGE			10			// 未接通话
#define SIP_REGISTER_CHANGE		11			// 注册状态变化
#define LANGUAGE_CHANGE			12			// 语言变化

#define	MSG_PRIVATE				10007		// 私有消息，发送给指定的窗口, wparam为窗口的id
#define MSG_PRIVATE_SPEAKER_IO	1			// 喇叭功放控制

#define	MSG_SHOW_STATUE			10008	// 提示消息
#define	MODIFYOK				0		// 修改成功
#define	IDCARD_UNLOCK			1		// 用户使用ID卡开锁
#define	BUTTON_UNLOCK			2		// 用户在门内按开锁按钮开锁
#define	USER_UNLOCK				3		// 用户输入用户密码开锁
#define	MANAGE_UNLOCK			4		// 管理中心开锁
#define	ROOM_UNLOCK				5		// 室内机开锁
#define	PUB_UNLOCK				6		// 公共密码开锁
#define	SET_RIGHT				7		// 设置成功
#define	SET_ERROR				8		// 设置失败
#define SET_ROOMERROR			9		// 房号错误
#define SET_PWDERROR			10		// 密码错误
#define DOOR_ADDCARD			11		// 添加卡
#define	CARD_ERROR				12		// 无效卡，刷卡失败
#define	ROOMERROR				13		// 房号错误
#define	PASSWDERROR				14		// 密码错误
#define	PASSWORDDIFF			15		// 两次密码不相同
#define	ROOMSETOK				16
#define	CARD_ADD				17		// 添加ICCARD
#define NOR_HANGUP				18		// Nor版本处理按键挂断
#define	DOOR_UNLOCK				22
#define	CALL_TIP_BUSY			23		//占线
#define	CALL_TIP_NORESPONSE		24		//无人接听
#define	CALL_TIP_LEAVEMSG		25		//留言
#define	DOOR_STOPADDCARD		26		//停止添加卡状态
#define DOOR_NETWORK_CHANGE		27		//Ip冲突
#define ONLY_MAIN				28		//需在主门口机设置
#define UN_USE					29		//该功能被禁用
#define INPUTROOM				30		//请输入房号
#define INPUTPWD				31		//请输入密码
#define ROOMERR					32		//房号不存在
#define PWDERR					33		//密码错误
#define IP_CONFLIC				34		//ip冲突

#define DOOR_ADDCARD_OK			40		//添加卡成功
#define DOOR_ADDCARD_FAIL		41		//添加卡失败
#define DOOR_DELCARD_OK			42		//删除卡成功
#define DOOR_DELCARD_FAIL		43		//删除卡失败
#define DOOR_DELCARD_NONE		44		//删除卡卡号不存在

#define	MSG_PHONECALL			10010		// 呼叫消息
#define CALL_ERROR				1
#define	CALL_BUSY				2
#define	CALL_TRANSBREAK			3
#define	CALL_RING				4
#define	CALL_ACCEPT				5
#define	CALL_HUNGUP				6
#define CALL_NEWCALLIN			7
#define CALL_MESSAGE			8
#define CALL_MSGERR				10
#define CALL_UNLOCK				11

#define	MSG_WECHAT_CALL			10012		// websocket  rtmp与微信小程序通信消息
#define	WECHAT_CALL_BUSY		0
#define	WECHAT_CALL_RING		1
#define	WECHAT_CALL_ACCEPT		2
#define	WECHAT_CALL_HANGUP		3
#define	WECHAT_CALL_ERROR		4
#define WECHAT_CALL_UNLOCK		5

#define STYPE_IR				4	// 红外
#define STYPE_TAMPER			8	// 防拆


#define	MSG_START_APP			10020		// 启动新的窗口
#define	MSG_END_APP				10021		// 结束指定的窗口
#define	MSG_START_FROM_ROOT		10022		// 启动指定的窗口，并结束当前所有的窗口
#define	MSG_EXTSTART_APP		10023		// 其他线程往窗口线程发送的窗口启动消息



void InitSysMessage(void);
bool DPPostMessage(unsigned int msgtype, unsigned int wParam, unsigned int lParam, unsigned int zParam, unsigned int type );
unsigned int DPGetMessage(SYS_MSG* msg);
void CleanUserInput(void);

#endif //_SYSTEMMSG_H_