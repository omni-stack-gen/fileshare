#ifndef _HARDWARE_TEST_H_
#define _HARDWARE_TEST_H_

int hareware_test_service_init(void);
int hareware_test_service_deinit(void);
int hareware_test_service_push_event(uint8_t typecode, uint32_t param);

bool hardware_test_is_test(void);
int hardware_test_send(int cmd, int index, unsigned int state);
#endif //_HARDWARE_TEST_H_

