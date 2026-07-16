#ifndef GPIO_SERVICE_H
#define GPIO_SERVICE_H

#include <stdint.h>


#define GPIO_LOCK0TYPE_RELAOD  99  //重新刷新锁1的类型
#define GPIO_TEST_TYPE        100 //测试类型  0:停止 1:开始 在测试过程中复位和拨码开关上报给远端室内机

enum gpio_type
{
    GPIO_LOCK_0,
    GPIO_LOCK_1,
    GPIO_I_CUT,
    GPIO_RESET_KEY,
    GPIO_ADDRESS_SET,
    GPIO_OVER_DET1,
    GPIO_OVER_DET2,
    GPIO_LOCK_CHECK,
    GPIO_LED_EN,
    GPIO_DNIGHT_DET,
	GPIO_NUMBER
};


enum gpio_mode
{
    GPIO_MODE_OFF = 0,
    GPIO_MODE_ON,
    GPIO_MODE_BLINK_SLOW,
    GPIO_MODE_BLINK_FAST,
};

int gpio_mgnt_init(uint32_t *paddrswitch_state);

void gpio_mgnt_deinit(void);

int gpio_control(enum gpio_type type, enum gpio_mode mode);

int gpio_control_Ex(enum gpio_type type, enum gpio_mode mode, uint32_t param);

#endif // GPIO_SERVICE_H