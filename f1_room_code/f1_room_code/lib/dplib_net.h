#ifndef __DPLIB_NET_H__
#define __DPLIB_NET_H__

#include <stdbool.h>

#define DPLIB_NET_VER_STRING	"0.1.4"
#define NET_VERSION_MAJOR 0
#define NET_VERSION_MINOR 1
#define NET_VERSION_PATCH 4

#define DPLIB_NET_API __attribute__ ((visibility ("default")))

/**
 * internet connect status.
 */
typedef enum inet_connect_sta 
{
	INET_STA_UNKNOWN = 0,			/* unknown. */
	INET_STA_DISCONNECT = 1,		/* internet is disconnected. */
	INET_STA_CONNECT = 2,			/* internet is connected. */
} inet_connect_sta_t;

/**
 * ethernet device link status.
 */
typedef enum eth_dev_link_status 
{
	ETH_DEV_LINK_UNKNOW = -1,	/* link status unknow. */
	ETH_DEV_LINK_DOWN = 0,		/* link is down. */
	ETH_DEV_LINK_UP = 1,		/* link is up. */
} eth_dev_link_status_t;	

typedef enum net_dhcp_status
{
	NET_DHCP_ERROR = -3,
	NET_DHCP_ABORT = -2,
	NET_DHCP_TIMEOUT = -1,
	NET_DHCP_SUCCESS = 0,
} net_dhcp_status_t;

typedef struct net_ip_config
{
	unsigned char ip_addr[4];
	unsigned char netmask[4];
	unsigned char gateway[4];
	unsigned char dns0[4];
	unsigned char dns1[4];
} net_ip_config_t;

/**
 * eth_dev_link_handler - ethernet device link status callback handler.
 * @param dev_name: 网卡
 * @param status: link status.
 */
typedef void (*eth_dev_link_handler)(const char *dev_name, eth_dev_link_status_t status);

/**
 * inet_connect_handler - internet connect status callback handler.
 * 
 * @param dev_name: 网卡
 * @param status: internet connect status.
 */
typedef void (*inet_connect_handler)(const char *dev_name, inet_connect_sta_t status);

/**
 * dhcp_status_handler - DHCP status handler.
 * 
 * @param dev_name: 网卡
 * @param status: DHCP status.
 */
typedef void (*dhcp_status_handler)(const char *dev_name, net_dhcp_status_t status);

/**
 * @brief 获取net库的版本号
 * 
 * @param [out] pversion 版本号
 * @return 0: success -1:fail
 */
DPLIB_NET_API int dplib_net_get_version_info(char *pversion);

/**
 * @brief 校验net库的版本号与头文件的版本号是否相同
 * 
 * @param usr_net_ver 填写头文件中的 DPLIB_NET_VER_STRING
 * @return false:版本不一致 true:版本相同
 */
DPLIB_NET_API bool dplib_net_check_version(char *usr_net_ver);

/**
 * @brief  设置mac地址
 * 
 * @param [in] dev_name 网卡
 * @param [in] mac_addr mac地址, 6字节
 * @return int 0:success -1:fail
 */
DPLIB_NET_API int dplib_net_set_mac_addr(const char *dev_name, char *mac_addr);

/**
 * @brief 获取mac地址
 * 
 * @param [in] dev_name 网卡
 * @param [out] mac_addr mac地址, 6字节
 * @return int 0:success -1:fail
 */
DPLIB_NET_API int dplib_net_get_mac_addr(const char *dev_name, char *mac_addr);

/**
 * @brief 打开网卡
 * 
 * @param [in] dev_name 网卡
 * @return int 0:success -1:fail
 */
DPLIB_NET_API int dplib_net_dev_open(const char *dev_name);

/**
 * @brief 关闭网卡
 * 
 * @param [in] dev_name 网卡
 * @return int 0:success -1:fail
 */
DPLIB_NET_API int dplib_net_dev_close(const char *dev_name);

/**
 * @brief 判断网卡是否打开
 * 
 * @param [in] dev_name 网卡
 * @return int 0:close 1:open other:get fail
 */
DPLIB_NET_API int dplib_net_dev_is_open(const char *dev_name);

/**
 * @brief 检查网卡是否存在
 * 
 * @param [in] dev_name 网卡名称（如 "eth0"）
 * @return int 1=存在，0=不存在，-1=错误
 */
DPLIB_NET_API int dplib_net_dev_is_exists(const char *dev_name);

/**
 * @brief 设置IP地址
 * 
 * @param [in] dev_name 网卡
 * @param [in] ip_config IP地址信息
 * @return int 0:success -1:fail
 */
DPLIB_NET_API int dplib_net_dev_set_ip_addr(const char *dev_name, net_ip_config_t *ip_config);

/**
 * @brief 获取IP地址信息
 * 
 * @param [in] dev_name 网卡
 * @param [out] ip_config IP地址信息
 * @return int 0:success -1:fail
 */
DPLIB_NET_API int dplib_net_dev_get_ip_addr(const char *dev_name, net_ip_config_t *ip_config);

/**
 * @brief dhcp 获取网络
 * 
 * @param dev_name 网卡
 * @param handler  结果回调
 * @param is_sync  是否使用同步模式
 * @return int 0:success -1:fail
 */
DPLIB_NET_API int dplib_net_dev_start_dhcp(const char *dev_name, 
                                        dhcp_status_handler handler, bool is_sync);

/**
 * @brief 设置dns_config的路径
 * 
 * @param [in] dns_config  dns_config的路径 def: /etc/init.d/dns_config
 * @return int 0:success -1:fail
 * 
 * 手动设置dns时使用, 不存在手动设置dns的场景可不用设置
 */
DPLIB_NET_API int dplib_net_set_dns_config(char *dns_config);

/**
 * @brief 连接外网状态监视创建
 * 
 * @param [in] dev_name	网卡
 * @param [in] handler 	状态回调
 * @return int 0:success !0:fail
 * 
 */
DPLIB_NET_API int dplib_inet_monitor_start(const char *dev_name, inet_connect_handler handler);

/**
 * @brief 停止外网连接状态监控
 * 
 * @param dev_name 网卡
 * @return int 0:success !0:fail
 */
DPLIB_NET_API int dplib_inet_monitor_stop(const char *dev_name);

/**
 * @brief 设置检测外网的ip，需要当前环境能ping通
 * 
 * @param check_ip IP地址 def:114.114.114.114  eg: 14.215.177.39
 * @return int 0:success -1:fail
 */
DPLIB_NET_API int dplib_inet_set_check_ip(char *check_ip);

/**
 * @brief 检测是否连接外网
 * 
 * @param dev_name 	网卡
 * @param dst 		ping的外网地址
 * @return int 0:未联网 1:联网 other:其他错误
 */
DPLIB_NET_API int dplib_inet_is_connect(const char *dev_name, const char *dst);

/**
 * @brief 创建链路检测服务
 * 
 * @return 0:success -1:fail 
 */
DPLIB_NET_API int dplib_eth_dev_start_link_detect(void);

/**
 * @brief 停止链路检测服务
 * 
 * @return 0:success -1:fail 
 * 
 */
DPLIB_NET_API int dplib_eth_dev_stop_link_detect(void);

/**
 * @brief 添加网口检测回调
 * 
 * @param dev_name 	网口
 * @param handler 	结果回调
 * @return 0:success !0:fail 
 */
DPLIB_NET_API int dplib_eth_dev_add_link_detect(const char *dev_name, eth_dev_link_handler handler);

/**
 * @brief 清除网口检测回调
 * 
 * @param dev_name 	网口
 * @return 0:success !0:fail 
 */
DPLIB_NET_API int dplib_eth_dev_clr_link_detect(const char *dev_name);

#endif /* __DPLIB_NET_H__ */
