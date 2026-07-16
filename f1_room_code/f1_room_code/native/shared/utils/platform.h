#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

struct os_cpu_usage_info;
typedef struct os_cpu_usage_info os_cpu_usage_info_t;

struct os_proc_memory_usage
{
	uint64_t resident_size;
	uint64_t virtual_size;
};
typedef struct os_proc_memory_usage os_proc_memory_usage_t;

typedef struct os_memory_info
{
	uint64_t total;
	uint64_t free;
	uint64_t used;
} os_memory_info_t;

#ifdef __cplusplus
extern "C"
{
#endif

os_cpu_usage_info_t *os_cpu_usage_info_start(void);
double os_cpu_usage_info_query(os_cpu_usage_info_t *info);
void os_cpu_usage_info_destroy(os_cpu_usage_info_t *info);

uint64_t os_get_sys_free_size(void);
uint64_t os_get_sys_total_size(void);

bool os_get_proc_memory_usage(os_proc_memory_usage_t *usage);
uint64_t os_get_proc_resident_size(void);
uint64_t os_get_proc_virtual_size(void);

void os_meminfo_dump();
int os_memory_get_info(os_memory_info_t *mem_info);

void os_reboot();
int os_system(const char *cmd);

#ifdef __cplusplus
}
#endif

#endif //!__PLATFORM_H__