#ifndef _rtp_internal_h_
#define _rtp_internal_h_

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MIN(a, b)       ((a) < (b) ? (a) : (b))
#define MAX(a, b)       ((a) > (b) ? (a) : (b))
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))

uint64_t rtpclock(void);
uint64_t ntp2clock(uint64_t ntp);
uint64_t clock2ntp(uint64_t clock);

uint32_t rtp_ssrc(void);

#endif /* !_rtp_internal_h_ */
