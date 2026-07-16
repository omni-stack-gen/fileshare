#ifndef __DPLIB_WIFI_H__
#define __DPLIB_WIFI_H__

#define DPLIB_WIFI_API 	__attribute__ ((visibility ("default")))

#define DPLIB_WIFI_VER_STRING	"0.1.6"
#define WIFI_VERSION_MAJOR 0
#define WIFI_VERSION_MINOR 1
#define WIFI_VERSION_PATCH 6

#define SSID_MAX_LENGTH				32
#define BSSID_MAX_LENGTH			17

typedef enum 
{
    WIFI_MODE_STA = 0,
} wifi_mode_t;

typedef enum 
{
	WIFI_SECURITY_OPEN = 0,
	WIFI_SECURITY_WPA_PSK,
	WIFI_SECURITY_WPA2_PSK,
} wifi_security_t;

typedef enum
{
    WIFI_STA_DISCONNECTED = 0,
	WIFI_STA_SCAN_RESULTS,
	WIFI_STA_SCAN_FAIL,
	WIFI_STA_CONNECTED,
	WIFI_STA_NETWORK_NOT_EXIST,
	WIFI_STA_PASSWD_ERROR,
	WIFI_STA_CONNECT_REJECT,
	WIFI_STA_CONNECT_ABORT,
	WIFI_STA_UNKNOWN_EVENT,
	WIFI_STA_CONNEECTING_EVENT,
} wifi_status_t;

typedef enum 
{
    WIFI_LOG_DEBUG = 0,
    WIFI_LOG_INFO,
    WIFI_LOG_WARN,
    WIFI_LOG_ERROR,
    WIFI_LOG_NONE,
} wifi_log_level_t;

typedef struct hotspot_info
{	
	int channel;
	int rssi;
	wifi_security_t security;
	char ssid[SSID_MAX_LENGTH + 1];
	char bssid[BSSID_MAX_LENGTH + 1];
} hotspot_info_t;

typedef struct scan_info
{
	int num;
	hotspot_info_t *info;
} scan_info_t;

typedef struct connect_info
{
	char ssid[SSID_MAX_LENGTH + 1];
	char bssid[BSSID_MAX_LENGTH + 1];
} connect_info_t;

/**
 * 回调函数定义
 */
typedef void (*dplib_wifi_event_cb)(wifi_status_t status, void *arg);
typedef void *(*dplib_wifi_malloc_ptr) (size_t);
typedef void (*dplib_wifi_free_ptr) (void *);

/**
 * @brief 获取版本号信息
 * 
 * @param [out] pversion 版本号
 * @return 0：success -1:fail
 */
DPLIB_WIFI_API int dplib_wifi_get_version_info(char *pversion);

/**
 * @brief 初始化wifi
 * 
 * @param [in] user_wifi_ver 使用的版本号,填写头文件中的 DPLIB_WIFI_VER_STRING 字段
 * @param [in] mode wifi的模式, 暂只支持 STA 模式
 * @param [in] cb   消息回调
 * @param [in] arg  用户数据
 * @return int  0:success other:fail
 */
DPLIB_WIFI_API int dplib_wifi_init(char *user_wifi_ver, wifi_mode_t mode, dplib_wifi_event_cb cb, void *arg);

/**
 * @brief 反初始化wifi
 * 
 * @return int  0:success other:fail
 */
DPLIB_WIFI_API int dplib_wifi_deinit(void);


/**************************************************
 * 				STA模式相关函数
 **************************************************/
/**
 * @brief 连接wifi。
 * 
 * @param [in] ssid 	wifi名
 * @param password 		wifi密码
 * @param security 		wifi加密等级
 * @return int 0:success -1:fail
 */
DPLIB_WIFI_API int dplib_wifi_connect(char *ssid, char *password, wifi_security_t security);

/**
 * @brief 断开wifi连接。
 * 
 * @return int 0:success -1:fail
 */
DPLIB_WIFI_API int dplib_wifi_disconnect(void);

/**
 * @brief 扫描wifi列表。
 * 
 * @return int 0:success -1:fail
 */
DPLIB_WIFI_API int dplib_wifi_scan(void);

/**
 * @brief 获取wifi列表.
 * 结果请使用 dplib_wifi_free_scan_results 进行释放,避免异常或内存泄漏
 * 
 * @param [out] scan_info WiFi列表结果
 * @return int 
 *
 */
DPLIB_WIFI_API int dplib_wifi_get_scan_results(scan_info_t *scan_info);

/**
 * @brief 释放WiFi列表的内存 
 * 
 * @param [in] scan_info WiFi列表
 * @return int 
 *
 */
DPLIB_WIFI_API int dplib_wifi_free_scan_results(scan_info_t *scan_info);

/**
 * @brief 获取wifi连接状态
 * 	
 * @return int 1:已连接 0:未连接 -1:获取失败
 */
DPLIB_WIFI_API int dplib_wifi_is_sta_connected(void);

/**
 * @brief 获取当前连接的wifi信息
 * 	
 * @param [out] info  返回的wifi信息
 * @return int 0:success  -1:fail
 */
DPLIB_WIFI_API int dplib_wifi_sta_get_connect_info(connect_info_t *info);

/**
 * @brief 删除指定ssid的wifi配置
 * 
 * @param [in] ssid 	wifi名
 * @param security 		wifi加密等级
 * @return int 0:success -1:fail
 */
DPLIB_WIFI_API int dplib_wifi_sta_delete_by_ssid(const char *ssid, int security);

/**************************************************
 * 				配置文件相关函数
 **************************************************/

/**
 * @brief 设置supp 配置文件路径
 * 
 * @param [in] path 路径 def: "/system/wifi/wpa_supplicant.conf"
 * @return int  0:success -1:fail
 */
DPLIB_WIFI_API int dplib_wifi_set_supp_config_path(char *path);

/**
 * @brief 设置supp 配置文件模板路径
 * 
 * @param [in] path 路径 def: "/system/wifi/wpa_supplicant_src.conf"
 * @return int 0:success -1:fail 
 */
DPLIB_WIFI_API int dplib_wifi_set_supp_config_template(char *path);

/**
 * @brief 设置wifi iface dir
 * 
 * @param [in] iface_dir 路径 def: "/data/misc/wifi/wpa_supplicant"
 * @return int 0:success -1:fail
 */
DPLIB_WIFI_API int dplib_wifi_set_wifi_iface_dir(char *iface_dir);

/**
 * @brief 设置 wifi_sta 脚本的路径
 * 
 * @param [in] wifi_sta_pat 路径 def: "/system/wifi/wifi_sta"
 * @return int 0:success -1:fail
 */
DPLIB_WIFI_API int dplib_wifi_set_wifi_sta_path(char *wifi_sta_pat);

/**
 * @brief 设置wifi网卡的名字
 * 
 * @param [in] interface 网卡 def: wlan0
 * @return int 0:success -1:fail
 */
DPLIB_WIFI_API int dplib_wifi_set_interface(char *interface);

/**************************************************
 * 				LOG相关函数
 **************************************************/

/**
 * @brief 获取当前log等级
 * 
 * @return int 返回log等级
 */
int dplib_wifi_log_get_level(void);

/**
 * @brief 设置当前log等级
 * 
 * @param [in] level log等级
 */
void dplib_wifi_log_set_level(wifi_log_level_t level);

/**
 * @brief 设置log回调
 * 
 * @param callback 回调
 */
void dplib_wifi_log_set_callback(void (*callback)(int, const char *, const long, const char *,
                               const char *, va_list));


/**************************************************
 * 				存储相关函数
 **************************************************/

/**
 * @brief 设置存储相关回调
 * 
 * @param [in] malloc_fn 申请内存的回调
 * @param [in] free_fn   释放内存的回调
 * @return int 0:success -1:fail
 */
DPLIB_WIFI_API int dplib_wifi_init_mem_hook(dplib_wifi_malloc_ptr malloc_fn, dplib_wifi_free_ptr free_fn);


#endif /* __DPLIB_WIFI_H__ */
