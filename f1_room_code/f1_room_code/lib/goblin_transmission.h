#ifndef _GOBLIN_TRANSMISSION_H_
#define _GOBLIN_TRANSMISSION_H_

/**
 * @file goblin_transmission.h
 * @brief PLC (电力线通信) 传输 API
 *
 * 本头文件提供 PLC 传输功能的公共 API，
 * 包括初始化、RSSI 监控和回调注册。
 */

#include <stdint.h>
#include <stddef.h>

/**
 * @brief PLC RSSI 列表条目结构
 *
 * 包含单个 PLC 设备的 RSSI 信息。
 */
typedef struct
{
	uint32_t id;        /**< 设备 ID */
	uint32_t c_recv;    /**< 接收到的数据包计数 */
	uint32_t c_drop;    /**< 丢弃的数据包计数 */
	uint8_t rssi;       /**< RSSI 值（信号强度） */
} goblin_plc_rssilist_t;

/**
 * @brief PLC 版本回调函数类型
 *
 * 当收到 PLC 版本信息时调用的回调。
 *
 * @param[in] plcversion PLC 版本号
 * @return 成功返回 0，失败返回非零
 */
typedef int (*goblin_PlcVersionCallBack)(uint32_t plcversion);

/**
 * @brief PLC 冲突回调函数类型
 *
 * 当检测到 PLC 冲突时调用的回调。
 *
 * @return 成功返回 0，失败返回非零
 */
typedef int (*goblin_PlcConflictCallBack)(void);

/**
 * @brief PLC RSSI 列表回调函数类型
 *
 * 当收到 RSSI 列表信息时调用的回调。
 *
 * @param[in] plccount 列表中 PLC 设备数量
 * @param[in] prssi_list RSSI 列表条目数组指针
 * @return 成功返回 0，失败返回非零
 */
typedef int (*goblin_PlcRssiListCallBack)(uint32_t plccount, goblin_plc_rssilist_t *prssi_list);

/**
 * @brief PLC 初始化信息结构
 *
 * 包含 PLC 初始化的配置和回调函数。
 */
typedef struct
{
	uint32_t plcdevid;                           /**< 本地 PLC 设备 ID */
    goblin_PlcVersionCallBack version_callback;  /**< 版本回调函数 */
    goblin_PlcConflictCallBack conflict_callback;/**< 冲突回调函数 */
    goblin_PlcRssiListCallBack rssi_list_callback;/**< RSSI 列表回调函数 */
} goblin_plc_initinfo_t;




/**
 * @brief 初始化内存钩子函数
 *
 * 允许用户自定义 PLC 模块使用的内存分配函数。
 * 可以替换默认的 malloc/realloc/free 实现，用于内存跟踪、自定义分配器等场景。
 *
 * @param[in] malloc_fn  内存分配函数指针，原型: void *malloc(size_t sz)
 * @param[in] remalloc_fn 内存重新分配函数指针，原型: void *realloc(void *ptr, size_t sz)
 * @param[in] free_fn    内存释放函数指针，原型: void free(void *ptr)
 * @return void
 * @note 该函数应在 goblin_plc_trans_init 之前调用，否则不会生效。
 *       传入 NULL 则使用默认的标准库函数。
 * @warning 自定义的内存函数必须与标准库行为兼容，特别是 realloc 和 free 的 NULL 处理。
 */
void goblin_plc_memory_InitHooks(void *(*malloc_fn)(size_t sz),
	void *(*remalloc_fn)(void *ptr, size_t sz),
	void (*free_fn)(void *ptr));

/**
 * @brief 初始化 PLC 传输模块
 *
 * 使用提供的配置初始化 PLC 传输模块。
 *
 * @param[in] pinfo 指向初始化信息结构的指针
 * @return 成功返回 0，失败返回 -1
 * @note 该函数必须在其他 PLC 函数调用之前调用。
 *       初始化成功会通过version_callback回调返回版本信息。
 *       初始化失败会通过conflict_callback回调返回冲突信息。
 * @see goblin_plc_trans_deinit
 */
int goblin_plc_trans_init(goblin_plc_initinfo_t *pinfo);

/**
 * @brief 反初始化 PLC 传输模块
 *
 * 释放 PLC 传输模块占用的所有资源，包括关闭设备、停止线程、释放内存等。
 *
 * @return 成功返回 0，失败返回 -1
 * @note 该函数应在程序退出前调用，确保资源正确释放。
 *       调用后不应再使用任何 PLC 传输功能。
 * @see goblin_plc_trans_init
 */
int goblin_plc_trans_deinit(void);

/**
 * @brief 注册数据接收回调函数
 *
 * 为指定的端口注册一个回调函数，当从该端口接收到数据时会被调用。
 *
 * @param[in] port 端口号，取值范围为 0x01 ~ 0xFF
 * @param[in] callback 数据接收回调函数指针，函数原型：
 *                     void callback(void *param, uint32_t dev, uint8_t *data, uint16_t len)
 *                     - param: 用户自定义参数
 *                     - dev: 发送数据的设备ID
 *                     - data: 接收到的数据缓冲区
 *                     - len: 数据长度（字节）
 * @param[in] param 用户自定义参数，将在回调函数中原样返回
 * @return 成功返回 0，失败返回 -1
 * @note 每个端口只能注册一个回调函数，重复注册将覆盖之前的回调。
 *       回调函数将在独立线程中执行，需要注意线程安全。
 * @see goblin_plc_unregister_recv_callback
 */
int goblin_plc_register_recv_callback(uint8_t port,
                                   void (*callback)(void *param, uint32_t dev, uint8_t *data, uint16_t len),
                                   void *param);

/**
 * @brief 注销数据接收回调函数
 *
 * 注销指定端口的数据接收回调函数。
 *
 * @param[in] port 端口号，取值范围为 0x01 ~ 0xFF
 * @return 成功返回 0，失败返回 -1
 * @note 注销后该端口将不再接收数据，直到重新注册回调。
 * @see goblin_plc_register_recv_callback
 */
int goblin_plc_unregister_recv_callback(uint8_t port);

/**
 * @brief 重置本地设备 ID
 *
 * 更新本地 PLC 设备的标识符。
 *
 * @param[in] devid 新的本地设备 ID
 * @return 成功返回 0，失败返回 -1
 * @note 通常用于设备重新配置或网络拓扑变化时。
 *       修改后新值立即生效，但建议在网络空闲时调用。
 */
int goblin_plc_retset_localdevid(uint32_t devid);

/**
 * @brief 通过文件路径升级 PLC 固件
 *
 * 加载指定路径的固件文件并执行升级操作。
 *
 * @param[in] pfilepath 固件文件的完整路径
 * @return 成功返回 0，失败返回 -1
 * @note 升级过程中会阻塞直到完成，期间不能进行数据传输。
 *       升级成功后设备会自动重启。
 *       升级失败时会保留原有固件版本。
 * @warning 请确保固件文件完整且与硬件版本匹配，否则可能导致设备无法启动。
 */
int goblin_plc_upgrade_byfilepath(char *pfilepath);

/**
 * @brief 重启 PLC 模块
 *
 * 强制重启 PLC 硬件模块。
 *
 * @return 成功返回 0，失败返回 -1
 * @note 重启过程中会断开所有连接，重启后需要重新初始化。
 *       此操作是异步的，函数返回后重启过程可能仍在进行。
 */
int goblin_plc_reboot(void);

/**
 * @brief 控制多播组
 *
 * 添加或移除多播地址，用于组播通信。
 *
 * @param[in] multicastaddr 多播地址
 * @param[in] isremove 操作类型：0 表示添加，1 表示移除
 * @return 成功返回 0，失败返回 -1
 * @note 添加多播地址后，设备将接收发往该多播地址的数据。
 *       移除后不再接收该地址的数据。
 *       多播地址需要符合网络协议规范。
 */
int goblin_plc_multicast_ctrl(uint32_t multicastaddr, uint8_t isremove);

/**
 * @brief 发送数据到指定设备
 *
 * 将数据发送到指定设备的指定端口。
 *
 * @param[in] dev 目标设备 ID
 * @param[in] port 目标端口号，取值范围为 0x01 ~ 0xFF
 * @param[in] data 指向要发送数据的缓冲区
 * @param[in] len 数据长度（字节），最大支持 2032 字节
 * @return 成功返回 0，失败返回 -1
 * @note 数据发送是异步的，函数返回不代表数据已送达。
 *       如果目标设备不在线，数据将被缓存或丢弃（取决于实现）。
 *       大数据量发送时建议控制发送速率，避免网络拥塞。
 * @warning data 缓冲区在函数返回后即可释放，内部已完成数据拷贝。
 */
int goblin_plc_send_data(uint32_t dev, uint8_t port, uint8_t *data, uint16_t len);

/**
 * @brief 获取 PLC RSSI 信息
 *
 * 触发从 PLC 设备获取 RSSI 信息的请求。
 * 结果将通过 rssi_list_callback 回调返回。
 *
 * @return 成功返回 0，失败返回 -1
 * @note 这是一个异步操作，函数返回后需要通过回调获取结果。
 *       回调将在短时间内被触发，携带网络中所有设备的 RSSI 信息。
 * @see goblin_PlcRssiListCallBack
 */
int goblin_plc_trans_get_rssi(void);

#endif // _GOBLIN_TRANSMISSION_H_
