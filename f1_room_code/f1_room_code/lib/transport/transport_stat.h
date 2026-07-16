#ifndef __TRANSPORT_STAT_H__
#define __TRANSPORT_STAT_H__

#include <stdint.h>
#include "transport.h"

typedef struct transport_internal_stat
{
	uint64_t last_time_ms;
	uint64_t total_interval;
	uint64_t interval_samples;
	uint64_t min_interval;
	uint64_t max_interval;
	uint64_t total_pkts;
	uint64_t total_bytes;

	// 1秒滑动窗口
	uint64_t sec_window_start;
	uint32_t sec_window_bytes;
	uint32_t sec_window_pkts;   // 当前秒内的包数累加器

	// 字节速率 (Bytes/s)
	uint32_t current_sec_rate;
	uint32_t min_sec_rate;
	uint32_t max_sec_rate;

	// 包速率 (Packets/s)
	uint32_t current_sec_pps;
	uint32_t min_sec_pps;
	uint32_t max_sec_pps;
} transport_internal_stat_t;

void transport_update_internal_stat(transport_internal_stat_t *stat, uint32_t bytes, uint64_t now);

void transport_fill_stat_info(transport_stat_t *info, transport_internal_stat_t *stat, uint64_t run_time,
                              uint32_t max_payload_size);

#endif /* __TRANSPORT_STAT_H__ */
