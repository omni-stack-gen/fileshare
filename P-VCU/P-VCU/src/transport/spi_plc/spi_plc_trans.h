#ifndef __SPI_PLC_TRANS_H__
#define __SPI_PLC_TRANS_H__

#include <common_defs.h>

#define MAX_SUB_COUNT           (12)

#define GATEWAY_DEV             GetPlcDev()//(0x80)

#define BROADCAST_DEV           (0xff)

// 代理消息通道
#define PROXY_PORT              (0xa1)

// 呼叫协议的端口
#define CALL_PORT               (0x0a)

// 音频端口
#define AUDIO_PORT              (0xa8)

// 视频端口
#define VIDEO_PORT              (0xa9)
//子码流
#define VIDEO_PORT_SUB          (0xb9)

//转发端口
#define FORWARD_PORT            (0xaf)

//文件传输端口
#define FILE_PORT              (0xab)

uint32_t GetPlcDev(void);

void *spi_plc_trans_create(int level, uint32_t dev_name);

int spi_plc_trans_destroy(void **handle);

int spi_plc_trans_register(void *handle, uint32_t port, uint32_t remote_dev,
                           void (*callback)(void *param, uint32_t dev, uint8_t *data, uint16_t data_size),
                           void *param);

int spi_plc_trans_unregister(void *handle, uint8_t port, uint32_t remote_dev);

int spi_plc_set_audio_tran(void *handle, uint32_t dev, uint32_t param);

int spi_plc_retset_localId(void*handle, uint32_t devid);

int spi_plc_trans_send(void *handle, uint32_t dev, uint8_t port, uint8_t *data, uint16_t data_size);

int spi_plc_trans_off(void *handle);

int spi_plc_trans_open_dev(void *handle, uint32_t devid);

int spi_plc_trans_get_rssi(void *handle);

int spi_plc_upgrade(void *handle, char *file_path);

int spi_plc_upgrade_by_fd(void *handle, FILE *pfd, uint32_t file_size);

int spi_plc_reboot(void *handle);

int spi_plc_multicast_ctrl(void *handle, uint32_t multicastaddr, uint8_t isremove);
#endif /* __SPI_PLC_TRANS_H__ */