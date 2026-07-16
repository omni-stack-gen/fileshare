#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum gpio_error
{
    GPIO_ERROR_NONE          =  0,   /**< 操作成功 */
    GPIO_ERROR_PARAM         = -1,   /**< 非法参数 */
    GPIO_ERROR_OPEN          = -2,   /**< 打开失败 */
    GPIO_ERROR_QUERY         = -3,   /**< 查询失败 */
    GPIO_ERROR_CONFIGURE     = -4,   /**< 配置失败 */
    GPIO_ERROR_IO            = -5,   /**< IO操作失败 */
    GPIO_ERROR_POLL_TIMEOUT  = -6,   /**< poll超时 */
    GPIO_ERROR_CLOSE         = -7    /**< 关闭失败 */
} gpio_error_t;

typedef enum gpio_direction
{
    GPIO_DIR_IN = 0,
    GPIO_DIR_OUT,
} gpio_direction_t;

typedef enum gpio_edge
{
    GPIO_EDGE_NONE = 0,     /**< 无中断边缘 */
    GPIO_EDGE_RISING,       /**< 上升沿 */
    GPIO_EDGE_FALLING,      /**< 下降沿 */
    GPIO_EDGE_BOTH          /**< 双边触发 */
} gpio_edge_t;

typedef enum gpio_state
{
    GPIO_LOW  = 0,
    GPIO_HIGH,
} gpio_state_t;

typedef struct gpio_config
{
    gpio_direction_t direction;
    gpio_edge_t edge;
    bool reversed;
} gpio_config_t;

typedef struct gpio gpio_t;

#ifdef __cplusplus
extern "C"
{
#endif

gpio_t *gpio_create();
void gpio_destroy(gpio_t *gpio);

gpio_error_t gpio_open(gpio_t *gpio, uint32_t gpio_id, gpio_config_t *config);
gpio_error_t gpio_close(gpio_t *gpio);

gpio_error_t gpio_read(gpio_t *gpio, gpio_state_t *state);
gpio_error_t gpio_write(gpio_t *gpio, gpio_state_t state);

gpio_error_t gpio_poll(gpio_t *gpio, unsigned long milliseconds);

int gpio_get_fd(gpio_t *gpio);

#ifdef __cplusplus
}
#endif

#endif //!__GPIO_H__