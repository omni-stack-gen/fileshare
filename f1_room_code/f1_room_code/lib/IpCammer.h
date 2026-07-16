/******************************************************************************
Copyright(c) 2011 - 2018 Digital Power Inc.
File name:
Author: liaosenlin
Version: 1.0.0
Date: 2018/09/17
Description:
History:
Bug report: liaosenlin@d-power.com.cn
******************************************************************************/
#ifndef __IPCAMMER_H__
#define __IPCAMMER_H__

#include <stdbool.h>
#include <stddef.h>

#if defined _WIN32 || defined __CYGWIN__
	#define IPCAM_LIB_IMPORT __declspec(dllimport)
	#define IPCAM_LIB_EXPORT __declspec(dllexport)
	#define IPCAM_LIB_LOCAL
#else
	#if __GNUC__ >= 4
		#define IPCAM_LIB_IMPORT __attribute__((visibility("default")))
		#define IPCAM_LIB_EXPORT __attribute__((visibility("default")))
		#define IPCAM_LIB_LOCAL __attribute__((visibility("hidden")))
	#else
		#define IPCAM_LIB_IMPORT
		#define IPCAM_LIB_EXPORT
		#define IPCAM_LIB_LOCAL
	#endif
#endif

#define IPCAM_PUBLIC_API IPCAM_LIB_EXPORT
#define IPCAM_PRIVATE_API IPCAM_LIB_LOCAL

typedef void (*ipc_frame_cb)(void *handle, unsigned char *buf, unsigned int len, bool keyframe);

typedef void (*ipc_status_cb)(void *handle, int type);

typedef struct ipc_info
{
	const char *ip;
	const char *rtsp_url;
	int video_width;
	int video_height;
} ipc_info_t;

typedef void (*ipc_info_cb)(void *handle, ipc_info_t *info);

enum
{
	ERR_AUTHENTICATE = 0,
	ERR_REMOTE_OFFLINE,
	ERR_ONVIF_SEARCH
};

enum
{
	VIDEO_H264 = 0,
	VIDEO_H265
};

typedef struct _ipc_setting_s
{
	/* 不开启onvif选项的 必须填写rtsp_url*/
	bool  use_onvif;

	short port;

	char  ip[16];

	char  user[32];

	char  passwd[32];

	char  rtsp_url[256];

	ipc_frame_cb frame_cb;

	ipc_status_cb status_cb;

	ipc_info_cb info_cb;
} ipc_setting_s;

typedef struct _onvif_setting
{
	/* 搜索超时 */
	int   timeout;

	/* 设备ip */
	char  ip[16];

	/* 用户名 */
	char  user[32];

	/* 密码 */
	char  passwd[32];
} onvif_setting;

typedef struct _onvif_result
{
	/* 视频宽度 */
	int video_width;

	/* 视频高度 */
	int video_height;

	/* 视频格式 */
	int video_format;

	/* rtsp地址 */
	char rtsp_url[256];
} onvif_result;

typedef struct _ipc_param
{
	char ip[16];

	char mac[32];
} ipc_param;

typedef struct _ipc_get
{
	int count;
	ipc_param *param;
} ipc_get;

typedef struct _ipc_mem_hooks
{
	void *(*allocate)(size_t size);
	void (*deallocate)(void *ptr);
	void *(*reallocate)(void *ptr, size_t size);
} ipc_mem_hooks;

typedef struct _onvif_discover_param
{
	char ip[16];
	char msgid[256];
	char xaddrs[256];
} onvif_discover_param_t;

typedef struct _onvif_discover_result
{
	int count;
	onvif_discover_param_t *param;
} onvif_discover_result;

#ifdef  __cplusplus
extern "C" {
#endif

/******************************************************************************
Function: ipc_start_monitor
Description: 开始网络摄像头监视
Param:
    setting in 账号、密码、ip等
Return:
    成功返回句柄 失败返回空
Others: None
******************************************************************************/
IPCAM_PUBLIC_API void *ipc_start_monitor(ipc_setting_s *setting);

/******************************************************************************
Function: ipc_stop_monitor
Description: 关闭网络摄像头监视
Param:
    handle  in ipc_start_monitor返回的句柄
Return: None
Others: None
******************************************************************************/
IPCAM_PUBLIC_API void ipc_stop_monitor(void *handle);

/******************************************************************************
Function: ipc_get_codec_type
Description: 获取网络摄像头的编码类型
Param:
    handle  in ipc_start_monitor返回的句柄
Return:
    成功返回编码类型 失败返回-1
Others: None
******************************************************************************/
IPCAM_PUBLIC_API int ipc_get_codec_type(void *handle);

/******************************************************************************
Function: ipc_onvif_search
Description: 搜索局域网内指定信息的IPC
Param:
    setting in 账号、密码、ip，搜索超时时间
    result  in 搜索结果
Return:
    成功返回1 失败返回0
Others: None
******************************************************************************/
IPCAM_PUBLIC_API int ipc_onvif_search(onvif_setting *setting, onvif_result *result);

/******************************************************************************
Function: ipc_set_network_interface
Description: 设置网络摄像头库使用网卡
Param:
    interface in 网卡名称
Return: None
Others: None
******************************************************************************/
IPCAM_PUBLIC_API void ipc_set_network_interface(char *interface);

/******************************************************************************
Function: ipc_onvif_search_all
Description: 搜索局域网内所有的IPC
Param:
    time_out in 超时
    pget     in 保存搜索的ipc相关信息
Return:
    成功返回1 失败返回0
Others: None
******************************************************************************/
IPCAM_PUBLIC_API int ipc_onvif_search_all(int time_out, ipc_get *pget);

/******************************************************************************
Function: ipc_onvif_set_select_video_resolution
Description: 设备onvif搜索视频分辨率
Param:
    width   in 视频宽度
    height  in 视频高度
Return: None
Others: None
******************************************************************************/
IPCAM_PUBLIC_API void ipc_onvif_set_select_video_resolution(int width, int height);

/******************************************************************************
Function: ipc_init_mem_hook
Description: 初始内存钩子
Param:
    hooks   in 内存钩子结构体描述
Return: None
Others: None
******************************************************************************/
IPCAM_PUBLIC_API void ipc_init_mem_hook(ipc_mem_hooks *hooks);

/******************************************************************************
Function: ipc_onvif_discover_devices
Description: 探索局域网支持onvif协议的设备
Param:
    time_out in 超时
    result   in 保存搜索的ipc相关信息
Return: None
Others: None
******************************************************************************/
IPCAM_PUBLIC_API void ipc_onvif_discover_devices(int time_out, onvif_discover_result *result);

#ifdef  __cplusplus
}
#endif

#endif //!__IPCAMMER_H__