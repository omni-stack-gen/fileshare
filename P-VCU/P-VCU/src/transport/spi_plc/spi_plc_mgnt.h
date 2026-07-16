#ifndef __SPI_PLC_MGNT_H__
#define __SPI_PLC_MGNT_H__

#include <common_defs.h>

/**
 * @brief 初始化SPI PLC管理模块
 * @param [in] level
 * @param [in] dev_num  定义的设备号
 * @return int 0:success -1:failed
 */
int spi_plc_mgnt_init(int level, unsigned int dev_num);

int spi_plc_mgnt_deinit();

int spi_plc_mgnt_register_callback(uint32_t port,
                                   void (*callback)(void *param, uint32_t dev, uint8_t *data, uint16_t len),
                                   void *param);

int spi_plc_mgnt_register_callbackEx(uint32_t port, uint32_t remotedev,
                                   void (*callback)(void *param, uint32_t dev, uint8_t *data, uint16_t len),
                                   void *param);

int spi_plc_mgnt_unregister_callback(uint8_t port);

int spi_plc_mgnt_unregister_callbackEx(uint8_t port, uint32_t remotedev);

int spi_plc_mgnt_audio_tran(uint32_t dev, uint32_t param);

int spi_plc_mgnt_retset_localdevid(uint32_t dev);

int spi_plc_mgnt_off(void);

int spi_plc_mgnt_open_dev(uint32_t devid);

int spi_plc_mgnt_upgrade(char *pfilepath);

int spi_plc_mgnt_upgrade_by_fd(FILE *pfd, uint32_t file_size);

int spi_plc_mgnt_reboot(void);

int spi_plc_mgnt_multicast_ctrl(uint32_t multicastaddr, uint8_t isremove);

int spi_plc_mgnt_get_rssi(void);

int spi_plc_mgnt_send(uint32_t dev, uint8_t port, uint8_t *data, uint16_t len);

#endif /* __SPI_PLC_MGNT_H__ */