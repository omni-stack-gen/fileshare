#ifndef __INPUT_EVENT_H__
#define __INPUT_EVENT_H__

#include <common_defs.h>
#include <linux/input.h>

typedef int (*ev_callback)(int fd, short revents, unsigned long ev_bits, void *data);

typedef int (*ev_set_key_callback)(int code, int value, void *data);

#ifdef __cplusplus
extern "C"
{
#endif

int input_event_init(ev_callback input_cb, void *data);

void input_event_deinit(void);

int input_event_wait(int timeout);

void input_event_dispatch(void);

int input_event_read(int fd, short revents, struct input_event *ev);

int input_event_add_fd(int fd, ev_callback cb, void *data);

#ifdef __cplusplus
}
#endif

#endif //!__INPUT_EVENT_H__