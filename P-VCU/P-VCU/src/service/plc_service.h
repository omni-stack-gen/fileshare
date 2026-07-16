#ifndef _PLC_SERVICE_H_
#define _PLC_SERVICE_H_

#include <stdint.h>

#define PLC_CALL_MAX_ARGS 10

#if defined(ENABLE_V1)
    #define PLC_RUN_LOG_PATH  "/mnt/UDISK/runlog.log"
    #define PLC_RUN_LOG_BK_PATH  "/mnt/UDISK/runlogbk.log"
#else
    #define PLC_RUN_LOG_PATH  "/data/runlog.log"
    #define PLC_RUN_LOG_BK_PATH  "/data/runlogbk.log"
#endif
int plc_service_init(uint32_t devid);

int plc_service_deinit(void);

int plc_service_key_event(uint8_t keycode);

int plc_service_devid_reload(uint32_t devid);
#endif /* _PLC_SERVICE_H_ */