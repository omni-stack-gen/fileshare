#ifndef CARD_SERVICE_H
#define CARD_SERVICE_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum  {
    CARD_MODE_STATUS_NORMAL = 0,
    CARD_MODE_STATUS_ADD,
    CARD_MODE_STATUS_DEL,
    CARD_MODE_STATUS_DEL_ALL,
    CARD_MODE_STATUS_READ,
    CARD_MODE_STATUS_MAX
}card_mode_status_e;

int card_service_init(void);

int card_service_deinit(void);

int card_service_push_event(card_mode_status_e mode, uint32_t param, uint32_t param2);

#endif // CARD_SERVICE_H