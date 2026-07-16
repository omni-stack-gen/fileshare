#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#include <sys/sysinfo.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <libgen.h>

#include <utils/bmem.h>
#include <utils/platform.h>

#define LOG_TAG "platform"
#include <utils/log.h>

struct os_cpu_usage_info
{
	clock_t last_cpu_time, last_sys_time, last_user_time;
	int core_count;
};

os_cpu_usage_info_t *os_cpu_usage_info_start(void)
{
	struct os_cpu_usage_info *info = (struct os_cpu_usage_info *)bmalloc(sizeof(*info));
	struct tms time_sample;

	info->last_cpu_time = times(&time_sample);
	info->last_sys_time = time_sample.tms_stime;
	info->last_user_time = time_sample.tms_utime;
	info->core_count = sysconf(_SC_NPROCESSORS_ONLN);
	return info;
}

double os_cpu_usage_info_query(os_cpu_usage_info_t *info)
{
	struct tms time_sample;
	clock_t cur_cpu_time;
	double percent;

	if (!info)
	{
		return 0.0;
	}

	cur_cpu_time = times(&time_sample);

	if (cur_cpu_time <= info->last_cpu_time ||
	    time_sample.tms_stime < info->last_sys_time ||
	    time_sample.tms_utime < info->last_user_time)
	{
		return 0.0;
	}

	percent = (double)(time_sample.tms_stime - info->last_sys_time +
	                   (time_sample.tms_utime - info->last_user_time));
	percent /= (double)(cur_cpu_time - info->last_cpu_time);
	percent /= (double)info->core_count;

	info->last_cpu_time = cur_cpu_time;
	info->last_sys_time = time_sample.tms_stime;
	info->last_user_time = time_sample.tms_utime;

	return percent * 100.0;
}

void os_cpu_usage_info_destroy(os_cpu_usage_info_t *info)
{
	if (info)
	{
		bfree(info);
	}
}

static unsigned long parse_cached_kb(void)
{
	char buf[60]; /* actual lines we expect are ~30 chars or less */

	FILE *fp;

	unsigned long cached = 0;

	fp = fopen("/proc/meminfo", "r");

	if (!fp)
	{
		return cached;
	}

	while (fgets(buf, sizeof(buf), fp) != NULL)
	{
		if (sscanf(buf, "Cached: %lu %*s\n", &cached) == 1)
		{
			break;
		}
	}

	fclose(fp);

	return cached;
}

void os_meminfo_dump()
{
	struct sysinfo info;

	unsigned long long cached;

	unsigned long long used;

	unsigned long long total;

	sysinfo(&info);

	/* Kernels prior to 2.4.x will return info.mem_unit==0, so cope... */
	unsigned mem_unit = (info.mem_unit ? info.mem_unit : 1);

	/* Extract cached from /proc/meminfo and convert to mem_units */
	cached = ((unsigned long long) parse_cached_kb() * 1024) / mem_unit;

	/* 剩余内存 */
	cached += info.freeram;

	cached += info.bufferram;

	/* 已经使用的内存 */
	total = info.totalram * mem_unit;

	used = (total - cached) * mem_unit;

	LOGI("total:[%llu byte] use:[%llu byte] free:[%llu byte] percent:[%lld%%] \n", total, used, cached,
	     (used * 100) / total);
}

int os_memory_get_info(os_memory_info_t *mem_info)
{
	struct sysinfo info;

	if (sysinfo(&info) < 0)
	{
		return -1;
	}

	/* Kernels prior to 2.4.x will return info.mem_unit==0, so cope... */
	unsigned mem_unit = (info.mem_unit ? info.mem_unit : 1);

	/* Extract cached from /proc/meminfo and convert to mem_units */
	mem_info->free = ((unsigned long long) parse_cached_kb() * 1024) / mem_unit;

	/* 剩余内存 */
	mem_info->free += info.freeram;

	mem_info->free += info.bufferram;

	/* 已经使用的内存 */
	mem_info->total = info.totalram * mem_unit;

	mem_info->used = (mem_info->total - mem_info->free) * mem_unit;

	return 0;
}

uint64_t os_get_sys_free_size(void)
{
	return 0;
}

typedef struct
{
	unsigned long virtual_size;
	unsigned long resident_size;
	unsigned long share_pages;
	unsigned long text;
	unsigned long library;
	unsigned long data;
	unsigned long dirty_pages;
} statm_t;

static inline bool os_get_proc_memory_usage_internal(statm_t *statm)
{
	const char *statm_path = "/proc/self/statm";

	FILE *f = fopen(statm_path, "r");

	if (!f)
	{
		return false;
	}

	if (fscanf(f, "%lu %lu %lu %lu %lu %lu %lu", &statm->virtual_size,
	           &statm->resident_size, &statm->share_pages, &statm->text,
	           &statm->library, &statm->data, &statm->dirty_pages) != 7)
	{
		fclose(f);
		return false;
	}

	fclose(f);
	return true;
}

bool os_get_proc_memory_usage(os_proc_memory_usage_t *usage)
{
	statm_t statm = {0};

	if (!os_get_proc_memory_usage_internal(&statm))
	{
		return false;
	}

	usage->resident_size = (uint64_t)statm.resident_size * sysconf(_SC_PAGESIZE);
	usage->virtual_size = statm.virtual_size;
	return true;
}

uint64_t os_get_proc_resident_size(void)
{
	statm_t statm = {};

	if (!os_get_proc_memory_usage_internal(&statm))
	{
		return 0;
	}

	return (uint64_t)statm.resident_size * sysconf(_SC_PAGESIZE);
}

uint64_t os_get_proc_virtual_size(void)
{
	statm_t statm = {};

	if (!os_get_proc_memory_usage_internal(&statm))
	{
		return 0;
	}

	return (uint64_t)statm.virtual_size;
}

static uint64_t total_memory = 0;
static bool total_memory_initialized = false;

static void os_get_sys_total_size_internal()
{
	total_memory_initialized = true;

	struct sysinfo info;

	if (sysinfo(&info) < 0)
	{
		return;
	}

	total_memory = (uint64_t)info.totalram * info.mem_unit;
}

uint64_t os_get_sys_total_size(void)
{
	if (!total_memory_initialized)
	{
		os_get_sys_total_size_internal();
	}

	return total_memory;
}

void os_reboot()
{
	sync();
	kill(1, SIGTERM);

	// while (1)
	// {
	// 	sleep(1);
	// }
}

static pid_t safe_waitpid(pid_t pid, int *wstat, int options)
{
	pid_t r;

	do
	{
		r = waitpid(pid, wstat, options);
	} while ((r == -1) && (errno == EINTR));

	return r;
}

static int wait4pid(pid_t pid)
{
	int status;

	if (pid <= 0)
	{
		/*errno = ECHILD; -- wrong. */
		/* we expect errno to be already set from failed [v]fork/exec */
		return -1;
	}

	if (safe_waitpid(pid, &status, 0) == -1)
	{
		return -1;
	}

	if (WIFEXITED(status))
	{
		return WEXITSTATUS(status);
	}

	if (WIFSIGNALED(status))
	{
		return WTERMSIG(status) + 0x180;
	}

	return 0;
}

int os_system(const char *cmd)
{
	pid_t pid;
	int status;

	if (cmd == NULL)
	{
		return 1;
	}

	if ((pid = vfork()) < 0)
	{
		LOGE("vfork failed for %s, %s\n", cmd, strerror(errno));
		status = -1;
	}
	else if (pid == 0)
	{
		// execle
		execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
		LOGE("execl failed for %s, %s\n", cmd, strerror(errno));
		_exit(127);
	}
	else
	{
#if 0
		safe_waitpid(pid, &status, 0);

		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		{
			return 0;
		}

#else

		if (wait4pid(pid) != 0)
		{
			status = -1;
		}

#endif
	}

	return status;
}