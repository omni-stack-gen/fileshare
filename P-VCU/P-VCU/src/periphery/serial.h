#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 错误码.
 */
typedef enum serial_error
{
    SERIAL_ERROR_NONE            =  0,   /**< 操作成功 */
    SERIAL_ERROR_PARAM           = -1,   /**< 非法参数 */
    SERIAL_ERROR_OPEN            = -2,   /**< 打开失败 */
    SERIAL_ERROR_QUERY           = -3,   /**< 查询失败 */
    SERIAL_ERROR_CONFIGURE       = -4,   /**< 配置失败 */
    SERIAL_ERROR_IO              = -5,   /**< IO操作失败 */
    SERIAL_ERROR_CLOSE           = -6,   /**< 关闭失败 */
    SERIAL_ERROR_POLL_TIMEOUT    = -7    /**< poll超时 */
} serial_error_t;

/**
 * @brief 波特率.
 */
typedef enum serial_baud_rate
{
    SERIAL_BAUD_50        = 50,        /**< 波特率50    */
    SERIAL_BAUD_75        = 75,        /**< 波特率75    */
    SERIAL_BAUD_110       = 110,       /**< 波特率110   */
    SERIAL_BAUD_134       = 134,       /**< 波特率134   */
    SERIAL_BAUD_150       = 150,       /**< 波特率150   */
    SERIAL_BAUD_200       = 200,       /**< 波特率200   */
    SERIAL_BAUD_300       = 300,       /**< 波特率300   */
    SERIAL_BAUD_600       = 600,       /**< 波特率600   */
    SERIAL_BAUD_1200      = 1200,      /**< 波特率1200  */
    SERIAL_BAUD_1800      = 1800,      /**< 波特率1800  */
    SERIAL_BAUD_2400      = 2400,      /**< 波特率2400  */
    SERIAL_BAUD_4800      = 4800,      /**< 波特率4800  */
    SERIAL_BAUD_9600      = 9600,      /**< 波特率9600  */
    SERIAL_BAUD_19200     = 19200,     /**< 波特率19200 */
    SERIAL_BAUD_38400     = 38400,     /**< 波特率38400 */
    SERIAL_BAUD_57600     = 57600,     /**< 波特率57600 */
    SERIAL_BAUD_115200    = 115200,    /**< 波特率115200 */
    SERIAL_BAUD_230400    = 230400,    /**< 波特率115200 */
    SERIAL_BAUD_460800    = 460800,    /**< 波特率460800 */
    SERIAL_BAUD_500000    = 500000,    /**< 波特率500000 */
    SERIAL_BAUD_576000    = 576000,    /**< 波特率576000 */
    SERIAL_BAUD_921600    = 921600,    /**< 波特率921600 */
    SERIAL_BAUD_1000000   = 1000000,   /**< 波特率1000000 */
    SERIAL_BAUD_1152000   = 1152000,   /**< 波特率1152000 */
    SERIAL_BAUD_1500000   = 1500000,   /**< 波特率1500000 */
    SERIAL_BAUD_2000000   = 2000000,   /**< 波特率2000000 */
    SERIAL_BAUD_2500000   = 2500000,   /**< 波特率2500000 */
    SERIAL_BAUD_3000000   = 3000000,   /**< 波特率3000000 */
    SERIAL_BAUD_3500000   = 3500000,   /**< 波特率3500000 */
    SERIAL_BAUD_4000000   = 4000000,   /**< 波特率4000000 */
} serial_baud_rate_t;

/**
 * @brief 数据位.
 */
typedef enum serial_data_bits
{
    SERIAL_DATA_5 = 5,     /**< 5个数据位 */
    SERIAL_DATA_6,         /**< 6个数据位 */
    SERIAL_DATA_7,         /**< 7个数据位 */
    SERIAL_DATA_8          /**< 8个数据位 */
} serial_data_bits_t;

/**
 * @brief 停止位.
 */
typedef enum serial_stop_bits
{
    SERIAL_STOP_1 = 1,     /**< 1个停止位 */
    SERIAL_STOP_2          /**< 2个停止位 */
} serial_stop_bits_t;

typedef enum serial_parity
{
    SERIAL_PARITY_NONE = 0,    /**< 无校验 */
    SERIAL_PARITY_ODD,         /**< 奇校验 */
    SERIAL_PARITY_EVEN,        /**< 偶校验 */
    SERIAL_PARITY_SPACE        /**< 空校验 */
} serial_parity_t;

typedef struct serial_config
{
    serial_baud_rate_t baud_rate;
    serial_data_bits_t data_bits;
    serial_parity_t parity;
    serial_stop_bits_t stop_bits;
    bool xonxoff;
    bool rtscts;
} serial_config_t;

typedef struct serial serial_t;

#ifdef __cplusplus
extern "C"
{
#endif

serial_t *serial_create();
void serial_destroy(serial_t *serial);

serial_error_t serial_open(serial_t *serial, const char *device,
                                const serial_config_t *config);
serial_error_t serial_close(serial_t *serial);

int serial_read(serial_t *serial, uint8_t *buf, size_t len,
                    unsigned long milliseconds);

int serial_read_once(serial_t *serial, uint8_t *buf, size_t len,
                    unsigned long milliseconds);

int serial_write(serial_t *serial, uint8_t *buf, size_t len);

serial_error_t serial_flush(serial_t *serial);

serial_error_t serial_poll(serial_t *serial, unsigned long milliseconds);

serial_error_t serial_input_waiting(serial_t *serial, size_t *count);
serial_error_t serial_output_waiting(serial_t *serial, size_t *count);

int serial_get_fd(serial_t *serial);

#ifdef __cplusplus
}
#endif

#endif //!__SERIAL_H__