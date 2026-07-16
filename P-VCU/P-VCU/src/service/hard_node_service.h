#ifndef HARD_NODE_SERVICE_H
#define HARD_NODE_SERVICE_H

enum node_type
{
    NODE_KEY1_LED = 0,
    NODE_KEY2_LED,
    NODE_KEY3_LED,
    NODE_KEY4_LED,
    NODE_MSG_LED,
    NODE_LOCK_LED,
	NODE_NUMBER
};


enum node_state_mode
{
    NODE_STATE_OFF = 0,
    NODE_STATE_ON
};

int hard_node_mgnt_init(void);

void hard_node_mgnt_deinit(void);

int hard_node_mgnt_control(enum node_type type, enum node_state_mode mode);

int SetCameraLight(int light);

int SetPwmValue(int index, int value);
#endif /* HARD_NODE_SERVICE_H */