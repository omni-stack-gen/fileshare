#ifndef __VIDEO_SERVICE_H__
#define __VIDEO_SERVICE_H__


enum
{
	VIDEO_IDLE,
	VIDEO_ENC_START,
	VIDEO_ENC_STOP,
	VIDEO_ENC_DAYORNIGHT,
	VIDEO_ENC_RELOAD,
	VIDEO_TICK_REFRESH,
	VIDEO_ENC_END,
} __attribute__((packed));


int get_day_night_status(void);

int video_service_init(void);

int video_service_deinit(void);

int video_service_push_event(uint8_t typecode, uint32_t param);
#endif //__VIDEO_SERVICE_H__