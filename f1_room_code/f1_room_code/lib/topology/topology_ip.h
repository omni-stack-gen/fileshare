/**
 * @file topology_ip.h
 * @brief 拓扑设备 IP / device_id 编码解析接口。
 */
#ifndef __TOPOLOGY_IP_H__
#define __TOPOLOGY_IP_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 拓扑设备 ID 字符串最大长度。 */
#define TOPOLOGY_DEVICE_ID_MAX (96U)

/** @brief IPv4 地址字符串最大长度，包含字符串结束符。 */
#define TOPOLOGY_IPV4_STR_MAX (16U)

/** @brief 短横线格式 MAC 地址字符串最大长度，包含字符串结束符。 */
#define TOPOLOGY_MAC_STR_MAX (18U)

/** @brief 室内机 device_id 使用的 devType 值。 */
#define TOPOLOGY_DEV_TYPE_INDOOR (1U)

/** @brief 二次确认机 device_id 使用的 devType 值。 */
#define TOPOLOGY_DEV_TYPE_SECONDARY_CONFIRM (3U)

/** @brief 单元机 device_id 使用的 devType 值。 */
#define TOPOLOGY_DEV_TYPE_UNIT (2U)

/** @brief 管理机 / 保安机 device_id 使用的 devType 值。 */
#define TOPOLOGY_DEV_TYPE_MANAGEMENT (6U)

/** @brief 转换盒 device_id 使用的 devType 值。 */
#define TOPOLOGY_DEV_TYPE_CONVERTER_BOX (500U)

/**
 * @brief 拓扑设备类型。
 */
typedef enum
{
    /** @brief 未知设备。 */
    TOPOLOGY_DEVICE_UNKNOWN = 0,
    /** @brief 围墙机，当前编码规则待确认。 */
    TOPOLOGY_DEVICE_WALL_MACHINE,
    /** @brief 管理机 / 保安机。 */
    TOPOLOGY_DEVICE_MANAGEMENT_MACHINE,
    /** @brief 单元机。 */
    TOPOLOGY_DEVICE_UNIT_MACHINE,
    /** @brief 转换盒。 */
    TOPOLOGY_DEVICE_CONVERTER_BOX,
    /** @brief 室内机。 */
    TOPOLOGY_DEVICE_INDOOR_MACHINE,
    /** @brief 二次确认机。 */
    TOPOLOGY_DEVICE_SECONDARY_CONFIRM_MACHINE,
} topology_device_type_t;

/**
 * @brief IP / device_id 解析结果或格式化输入。
 *
 * @note `device_type` 是拓扑库内部设备类型枚举，`dev_type` 是 device_id 字符串
 *       前缀中的业务编码值，例如室内机为 `1`、单元机为 `2`、管理机为 `6`。
 */
typedef struct
{
    /** @brief 设备类型。 */
    topology_device_type_t device_type;
    /** @brief device_id 前缀中的 devType；解析时由实现填充。 */
    uint16_t dev_type;
    /** @brief device_id 字符串；解析时由实现填充。 */
    char device_id[TOPOLOGY_DEVICE_ID_MAX];

    /** @brief `10.X.Y.Z` 中的 X，表示楼栋号后两位。 */
    uint8_t building;
    /** @brief `10.X.Y.Z` 中的 Y。 */
    uint8_t raw_y;
    /** @brief `10.X.Y.Z` 中的 Z。 */
    uint16_t raw_z;

    /** @brief 室内机楼层 / 户号前半部分。 */
    uint8_t floor;
    /** @brief 室内机户号后半部分；分离住户时为 D 类减 200 后的值。 */
    uint8_t room;
    /** @brief 是否为分离住户室内机。 */
    bool separated_household;

    /** @brief 单元机或管理机编号。 */
    uint8_t device_no;

    /** @brief 转换盒上游 1号单元机裸 IP，例如 `10.2.0.1`。 */
    char upstream_unit_ip[TOPOLOGY_IPV4_STR_MAX];
    /** @brief 转换盒短横线格式 MAC，例如 `aa-bb-cc-dd-ee-ff`。 */
    char converter_mac[TOPOLOGY_MAC_STR_MAX];
    /** @brief 二次确认机所属室内机 `device_id`。 */
    char owner_device_id[TOPOLOGY_DEVICE_ID_MAX];
} topology_ip_info_t;

/**
 * @brief 设备网络参数输出结果。
 */
typedef struct
{
    /** @brief 设备 IP 地址，例如 `10.2.0.10`。 */
    char ip_addr[TOPOLOGY_IPV4_STR_MAX];
    /** @brief 子网掩码，当前固定为 `255.255.0.0`。 */
    char netmask[TOPOLOGY_IPV4_STR_MAX];
    /** @brief 网关地址，例如 `10.2.0.254`。 */
    char gateway[TOPOLOGY_IPV4_STR_MAX];
} topology_ip_network_config_t;

/**
 * @brief 解析 `devType:IP` 字符串形式的 device_id。
 *
 * 当前支持室内机 `1:10.X.Y.Z`、单元机 `2:10.X.0.Z`、管理机
 * `6:10.X.128.Z`、转换盒 `500:<上游1号单元机IP>:<转换盒MAC>` 和二次确认机
 * `3:<所属室内机device_id>`。其中 `devType:IP` 类身份必须与 IP 地址段匹配。
 *
 * @param device_id device_id 字符串，不可为 `NULL`。
 * @param out 解析结果，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 */
int topology_ip_parse_device_id(const char *device_id, topology_ip_info_t *out);

/**
 * @brief 根据结构化字段格式化 `devType:IP` device_id。
 *
 * @param info 设备编码信息，不可为 `NULL`。
 * @param buf 输出缓冲区，不可为 `NULL`。
 * @param len 输出缓冲区长度。
 * @return 成功返回 `0`；失败返回负数错误码。
 */
int topology_ip_format_device_id(const topology_ip_info_t *info, char *buf, size_t len);

/**
 * @brief 根据室内机设置格式化 `device_id`。
 *
 * 输出格式为 `1:10.X.Y.Z`。普通室内机的 `room` 取值范围为 `1..99`；
 * 分离住户室内机的 `room` 取值范围为 `1..50`，输出 IP 的 D 类字段会自动加 200。
 *
 * @param[in] building 楼栋号后两位，对应 `10.X.Y.Z` 中的 `X`，范围为 `0..99`。
 * @param[in] floor 楼层 / 户号前半部分，对应 `10.X.Y.Z` 中的 `Y`，范围为 `1..99`。
 * @param[in] room 户号后半部分；分离住户时为未加 200 前的户号。
 * @param[in] separated_household 是否为分离住户室内机。
 * @param[out] buf 输出缓冲区，不可为 `NULL`。
 * @param[in] len 输出缓冲区长度，单位为字节。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 生成成功。
 * @retval -EINVAL 参数非法或字段超出当前规则范围。
 * @retval -ENOSPC 输出缓冲区空间不足。
 */
int topology_ip_format_indoor_machine(uint8_t building,
                                      uint8_t floor,
                                      uint8_t room,
                                      bool separated_household,
                                      char *buf,
                                      size_t len);

/**
 * @brief 根据别墅型室内机设置格式化 `device_id`。
 *
 * 第一版别墅型室内机设备侧只设置本别墅 PLC 域内的 `Z`。本接口固定使用
 * `X=1`、`Y=1`，输出格式为 `1:10.1.1.Z`。
 *
 * @param[in] z 别墅 PLC 域内室内机编号，范围为 `1..99`。
 * @param[out] buf 输出缓冲区，不可为 `NULL`。
 * @param[in] len 输出缓冲区长度，单位为字节。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 生成成功。
 * @retval -EINVAL 参数非法或字段超出当前规则范围。
 * @retval -ENOSPC 输出缓冲区空间不足。
 *
 * @note `1:10.1.1.Z` 在字符串形态上可能与真实楼宇 “1栋1层Z户” 室内机重合；
 *       后续若同一全局管理域内需要同时管理多个别墅域，需要引入别墅域 ID、
 *       后台分配规则或图模型 domain 参与全局唯一性管理。
 */
int topology_ip_format_villa_indoor_machine(uint8_t z,
                                            char *buf,
                                            size_t len);

/**
 * @brief 根据单元机设置格式化 `device_id`。
 *
 * 输出格式为 `2:10.X.0.Z`，其中 `Z` 为单元机编号。
 *
 * @param[in] building 楼栋号后两位，对应 `10.X.0.Z` 中的 `X`，范围为 `0..99`。
 * @param[in] device_no 单元机编号，对应 `10.X.0.Z` 中的 `Z`，范围为 `1..99`。
 * @param[out] buf 输出缓冲区，不可为 `NULL`。
 * @param[in] len 输出缓冲区长度，单位为字节。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 生成成功。
 * @retval -EINVAL 参数非法或字段超出当前规则范围。
 * @retval -ENOSPC 输出缓冲区空间不足。
 */
int topology_ip_format_unit_machine(uint8_t building,
                                    uint8_t device_no,
                                    char *buf,
                                    size_t len);

/**
 * @brief 根据管理机 / 保安机设置格式化 `device_id`。
 *
 * 输出格式为 `6:10.X.128.Z`，其中 `Z` 为管理机 / 保安机编号。
 *
 * @param[in] building 楼栋号后两位，对应 `10.X.128.Z` 中的 `X`，范围为 `0..99`。
 * @param[in] device_no 管理机 / 保安机编号，对应 `10.X.128.Z` 中的 `Z`，
 *                      范围为 `1..99`。
 * @param[out] buf 输出缓冲区，不可为 `NULL`。
 * @param[in] len 输出缓冲区长度，单位为字节。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 生成成功。
 * @retval -EINVAL 参数非法或字段超出当前规则范围。
 * @retval -ENOSPC 输出缓冲区空间不足。
 */
int topology_ip_format_management_machine(uint8_t building,
                                          uint8_t device_no,
                                          char *buf,
                                          size_t len);

/**
 * @brief 根据转换盒设置格式化 `device_id`。
 *
 * 输出格式为 `500:<上游1号单元机IP>:<转换盒MAC>`。上游 1号单元机 IP 必须是裸
 * IP，例如 `10.2.0.1`，不带 `2:` 前缀；MAC 必须使用短横线格式。
 *
 * @param[in] upstream_unit_ip 上游 1号单元机裸 IP，不可为 `NULL`。
 * @param[in] converter_mac 转换盒短横线格式 MAC，不可为 `NULL`。
 * @param[out] buf 输出缓冲区，不可为 `NULL`。
 * @param[in] len 输出缓冲区长度，单位为字节。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 生成成功。
 * @retval -EINVAL 参数非法或字段不符合当前规则。
 * @retval -ENOSPC 输出缓冲区空间不足。
 */
int topology_ip_format_converter_box(const char *upstream_unit_ip,
                                     const char *converter_mac,
                                     char *buf,
                                     size_t len);

/**
 * @brief 根据所属室内机格式化二次确认机 `device_id`。
 *
 * 输出格式为 `3:<所属室内机device_id>`。所属设备必须是当前拓扑规则可解析的
 * 室内机 `device_id`，例如 `1:10.2.17.1` 或 `1:10.1.1.1`。
 *
 * @param[in] owner_device_id 所属室内机 `device_id`，不可为 `NULL`。
 * @param[out] buf 输出缓冲区，不可为 `NULL`。
 * @param[in] len 输出缓冲区长度，单位为字节。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 生成成功。
 * @retval -EINVAL 参数非法或所属设备不是室内机。
 * @retval -ENOSPC 输出缓冲区空间不足。
 */
int topology_ip_format_secondary_confirm_machine(const char *owner_device_id,
                                                 char *buf,
                                                 size_t len);

/**
 * @brief 根据设备设置生成 IP、子网掩码和网关。
 *
 * 调用方按设备类型填充 @p info 中对应字段。例如单元机填写
 * `device_type`、`building` 和 `device_no`，室内机填写 `device_type`、
 * `building`、`floor`、`room` 和 `separated_household`。
 * 本接口输出裸 IP 地址，不输出 `devType:IP` device_id。
 *
 * @param[in] info 设备编码信息，不可为 `NULL`。
 * @param[out] out 网络参数输出，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 生成成功。
 * @retval -EINVAL 参数非法或字段超出当前规则范围。
 * @retval -ENOTSUP 当前设备类型尚未支持生成 IP 网络参数。
 */
int topology_ip_format_network_config(const topology_ip_info_t *info,
                                      topology_ip_network_config_t *out);

/**
 * @brief 格式化 `10.X.Y.Z` IPv4 字符串。
 *
 * @param x `10.X.Y.Z` 中的 X。
 * @param y `10.X.Y.Z` 中的 Y。
 * @param z `10.X.Y.Z` 中的 Z。
 * @param buf 输出缓冲区，不可为 `NULL`。
 * @param len 输出缓冲区长度。
 * @return 成功返回 `0`；失败返回负数错误码。
 */
int topology_ip_format_ipv4(uint8_t x, uint8_t y, uint16_t z, char *buf, size_t len);

/**
 * @brief 按 `10.X.Y.Z` 字段分类设备类型。
 *
 * @param x `10.X.Y.Z` 中的 X。
 * @param y `10.X.Y.Z` 中的 Y。
 * @param z `10.X.Y.Z` 中的 Z。
 * @param out_type 输出设备类型，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 */
int topology_ip_classify(uint8_t x, uint8_t y, uint16_t z,
                         topology_device_type_t *out_type);

#ifdef __cplusplus
}
#endif

#endif /* __TOPOLOGY_IP_H__ */
