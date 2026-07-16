/**
 * @file
 * @brief simple 应用 PLC 端口规划公共头文件。
 *
 * 本文件集中定义 simple 主线在 PLC 链路上使用的应用端口，避免各模块各自硬编码。
 */

#ifndef SIMPLE_PLC_PORTS_H
#define SIMPLE_PLC_PORTS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief PLC RTP/媒体流端口。 */
#define SIMPLE_PLC_PORT_MEDIA (0x01U)
/** @brief PLC 呼叫控制端口。 */
#define SIMPLE_PLC_PORT_CALL_CONTROL (0x02U)
/** @brief PLC D2D 控制协议端口。 */
#define SIMPLE_PLC_PORT_D2D_CONTROL (0x03U)

#ifdef __cplusplus
}
#endif

#endif
