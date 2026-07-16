#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <inttypes.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>

#include <log.h>
#include <bmem.h>

#define LOG_MEMCHUNK_MAX (10)
#define FILENAME_LEN (256)
#define FILESIZE_LEN (10 * 1024 * 1024UL)
#define LOG_BUF_SIZE (4096 * 1)
#define LOG_TIME_SIZE (32)
#define LOG_LEVEL_SIZE (32)
#define LOG_TAG_SIZE (32)
#define LOG_PNAME_SIZE (32)
#define LOG_TEXT_SIZE (256)
#define LOG_LEVEL_DEFAULT DBG_INFO

#define LOG_PREFIX_MASK (0xFFFF)
#define LOG_TAG_MASK (0x0F)
#define LOG_TIMESTAMP_MASK (0x07)
#define LOG_PIDTID_MASK (0x03)
#define LOG_FUNCLINE_MASK (0x01)

#define UPDATE_LOG_PREFIX(log, bit) \
    log |= (((bit)&LOG_TIMESTAMP_MASK) | ((bit)&LOG_PIDTID_MASK) | ((bit)&LOG_TAG_MASK) | ((bit)&LOG_FUNCLINE_MASK))

#define CHECK_LOG_PREFIX(log, bit) ((log & LOG_PREFIX_MASK) & (bit))

typedef struct _log_chunk
{
    char *buf;
    int size;
} log_chunk;

typedef struct _log_mgnt
{
    int is_initialize;         /**< 是否初始化 */
    int is_cyclic;             /**< 是否回环 */
    int log_mode;              /**< 打印模式 */
    int log_level;             /**< log等级 */
    int log_bit;               /**< log内容标志位 */
    const char *log_ident;     /**< log文件样式 */
    uint64_t log_file_size;    /**< log文件大小 */
    char proc_name[256];       /**< 进程名字 */
    char log_path[256];        /**< 存放log的文件夹 */
    char log_name[256];        /**< 存放log的文件夹 */
    char log_name_prefix[256]; /**< log文件名的开头 */
    char log_name_time[256];   /**< log文件*/
    FILE *log_fd;              /**< 文件描述符 */
    int disable_write;
} log_mgnt;

static log_mgnt g_log_mgr = {0};

static pthread_once_t thread_once = PTHREAD_ONCE_INIT;

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *log_level[] = {"VERBOSE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", NULL};

static const char hex_digital[] = "0123456789abcdef";

uint64_t log_file_size(FILE *fp)
{
    long tmp;

    uint64_t size;

    if (!fp || fp == stderr)
    {
        return 0;
    }

    tmp = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    fseek(fp, tmp, SEEK_SET);

    return size;
}

static const char *get_dir(const char *path)
{
    char *p = (char *)path + strlen(path);

    for (; p != path; p--)
    {
        if (*p == '/')
        {
            *(p + 1) = '\0';
            break;
        }
    }

    return path;
}

static int mkdir_r(const char *path, mode_t mode)
{
    int ret = 0;

    char *temp, *pos;

    if (!path)
    {
        return -1;
    }

    temp = bstrdup(path);
    pos = temp;

    if (strncmp(temp, "/", 1) == 0)
    {
        pos += 1;
    }
    else if (strncmp(temp, "./", 2) == 0)
    {
        pos += 2;
    }
    for (; *pos != '\0'; ++pos)
    {
        if (*pos == '/')
        {
            *pos = '\0';
            if (-1 == (ret = mkdir(temp, mode)))
            {
                if (errno == EEXIST)
                {
                    ret = 0;
                }
                else
                {
                    fprintf(stderr, "failed to mkdir %s: %d:%s\n", temp, errno, strerror(errno));
                    break;
                }
            }

            *pos = '/';
        }
    }

    if (*(pos - 1) != '/')
    {
        if (-1 == (ret = mkdir(temp, mode)))
        {
            if (errno == EEXIST)
            {
                ret = 0;
            }
            else
            {
                fprintf(stderr, "failed to mkdir %s: %d:%s\n", temp, errno, strerror(errno));
            }
        }
    }

    bfree(temp);

    return ret;
}

static void check_dir(const char *path)
{
    char *path_org = NULL;

    const char *dir = NULL;

    if (strstr(path, "/"))
    {
        path_org = bstrdup(path);

        dir = get_dir(path_org);

        if (-1 == access(dir, F_OK | W_OK | R_OK))
        {
            if (-1 == mkdir_r(dir, 0775))
            {
                fprintf(stderr, "mkdir %s failed\n", path_org);
            }
        }

        bfree(path_org);
    }
}

static const char *log_get_basename(const char *path)
{
	const char *file_name = strrchr(path, '/');

	return file_name ? (file_name + 1) : path;
}

static void log_get_time(char *str, int len, int is_file)
{
    char time_format[32];
    char time_str[8];
    struct timeval tv;
    struct tm now_tm;
    int now_ms;
    time_t now_sec;

    gettimeofday(&tv, NULL);

    now_sec = tv.tv_sec;
    now_ms = tv.tv_usec / 1000;

    localtime_r(&now_sec, &now_tm);

    if (is_file == 0)
    {
        strftime(time_format, sizeof(time_format), "%Y-%m-%d %H:%M:%S", &now_tm);
        snprintf(time_str, sizeof(time_str), "%03d", now_ms);
        snprintf(str, 64, "[%s.%s]", time_format, time_str);
    }
    else
    {
        strftime(time_format, sizeof(time_format), "%Y_%m_%d_%H_%M_%S", &now_tm);
        snprintf(time_str, sizeof(time_str), "%03d", now_ms);
        snprintf(str, len, "%s_%s.log", time_format, time_str);
    }
}

static int get_proc_name(char *name, size_t len)
{
    size_t i, ret;

    char proc_name[PATH_MAX];

    char *ptr = NULL;

    memset(proc_name, 0, sizeof(proc_name));

    if (-1 == readlink("/proc/self/exe", proc_name, sizeof(proc_name)))
    {
        fprintf(stderr, "readlink failed!\n");
        return -1;
    }

    ret = strlen(proc_name);

    for (i = ret, ptr = proc_name; i > 0; i--)
    {
        if (ptr[i] == '/')
        {
            ptr += i + 1;
            break;
        }
    }

    if (i == 0)
    {
        fprintf(stderr, "proc path %s is invalid\n", proc_name);
        return -1;
    }

    if (ret - i > len)
    {
        fprintf(stderr, "proc name length %zu is larger than %zu\n", ret - i, len);
        return -1;
    }

    strncpy(name, ptr, ret - i);

    return 0;
}

static void log_filename_parse(const char *path)
{
    int i;
    char *p, *q;
    char *dot;

    if (!path)
    {
        return;
    }

    memset(g_log_mgr.log_path, 0, sizeof(g_log_mgr.log_path));
    memset(g_log_mgr.log_name_prefix, 0, sizeof(g_log_mgr.log_name_prefix));

    p = (char *)strchr(path, '/');
    q = (char *)strrchr(path, '/');

    if (p == q || p == NULL)
    {
        p = (char *)path;
    }
    else
    {
        for (i = 0, p = (char *)path; p < q + 1; p++, i++)
        {
            g_log_mgr.log_path[i] = *p;
        }
    }

    dot = (char *)strrchr(path, '.');

    if (dot)
    {
        for (i = 0; p < dot; p++, i++)
        {
            g_log_mgr.log_name_prefix[i] = *p;
        }
    }
    else
    {
        for (i = 0; *p != '\0'; p++, i++)
        {
            g_log_mgr.log_name_prefix[i] = *p;
        }
    }
}

static int log_fopen(const char *path)
{
    check_dir(path);

    g_log_mgr.log_fd = fopen(path, "a+");

    if (!g_log_mgr.log_fd)
    {
        fprintf(stderr, "fopen %s failed: %s\n", path, strerror(errno));
        fprintf(stderr, "use stderr as output\n");
        g_log_mgr.log_fd = stderr;
    }

    return 0;
}

static int log_fopen_rewrite(const char *path)
{
    check_dir(path);

    g_log_mgr.log_fd = fopen(path, "w");

    if (!g_log_mgr.log_fd)
    {
        fprintf(stderr, "fopen %s failed: %s\n", path, strerror(errno));
        fprintf(stderr, "use stderr as output\n");
        g_log_mgr.log_fd = stderr;
    }

    return 0;
}

static int log_fclose(void)
{
    return fclose(g_log_mgr.log_fd);
}

static ssize_t log_fwrite(log_chunk *p_chunk, int n)
{
    int i, ret;

    char log_rename[1024] = {0};

    unsigned long long tmp_size = log_file_size(g_log_mgr.log_fd);

    if (tmp_size > g_log_mgr.log_file_size)
    {
        if (g_log_mgr.is_cyclic)
        {
            if (EOF == log_fclose())
            {
                fprintf(stderr, "_log_fclose error: %d:%s\n", errno, strerror(errno));
            }

            log_fopen_rewrite(g_log_mgr.log_name);
        }
        else
        {
            if (CHECK_LOG_PREFIX(g_log_mgr.log_bit, LOG_VERBOSE_BIT))
            {
                fprintf(stderr, "%s size= %" PRIu64 " reach max %" PRIu64 ", splited\n",
                        g_log_mgr.log_name, (uint64_t)tmp_size, (uint64_t)g_log_mgr.log_file_size);
            }

            if (EOF == log_fclose())
            {
                fprintf(stderr, "log_fclose error: %d:%s\n", errno, strerror(errno));
            }

            log_get_time(g_log_mgr.log_name_time, sizeof(g_log_mgr.log_name_time), 1);

            snprintf(log_rename, sizeof(log_rename), "%s%s_%s", g_log_mgr.log_path, g_log_mgr.log_name_prefix, g_log_mgr.log_name_time);

            if (-1 == rename(g_log_mgr.log_name, log_rename))
            {
                fprintf(stderr, "log file splited %s error: %d:%s\n", log_rename, errno, strerror(errno));
            }

            log_fopen(g_log_mgr.log_name);

            if (CHECK_LOG_PREFIX(g_log_mgr.log_bit, LOG_VERBOSE_BIT))
            {
                fprintf(stderr, "splited file %s\n", log_rename);
            }
        }
    }

    for (i = 0; i < n; i++)
    {
        ret = fprintf(g_log_mgr.log_fd, "%s", (char *)p_chunk[i].buf);

        if (ret != p_chunk[i].size)
        {
            fprintf(stderr, "fprintf failed: %s\n", strerror(errno));
            return -1;
        }

        if (EOF == fflush(g_log_mgr.log_fd))
        {
            fprintf(stderr, "fflush failed: %s\n", strerror(errno));
            return -1;
        }
    }

    return 0;
}

static void log_initialize_once()
{
    g_log_mgr.log_level = LOG_LEVEL_DEFAULT;

    get_proc_name(g_log_mgr.proc_name, sizeof(g_log_mgr.proc_name));

    log_filename_parse(g_log_mgr.log_ident);

    memset(g_log_mgr.log_name, 0, sizeof(g_log_mgr.log_name));

    if (g_log_mgr.log_ident == NULL)
        log_get_time(g_log_mgr.log_name, sizeof(g_log_mgr.log_name), 1);
    else
        strncpy(g_log_mgr.log_name, g_log_mgr.log_ident, sizeof(g_log_mgr.log_name)-1);

    log_fopen(g_log_mgr.log_name);

    g_log_mgr.log_file_size = FILESIZE_LEN;

    g_log_mgr.is_initialize = 1;
}

void goblin_log_init(const char *path)
{
    g_log_mgr.log_ident = path;

    if (0 != pthread_once(&thread_once, log_initialize_once))
    {
        fprintf(stderr, "pthread_once failed\n");
    }

	printf("\n\n\n\n\n^^^^^^^^^^^^^^^^^^^^ g_log_mgr:%p ^^^^^^^^^^^^^^^^^^^^^^\n\n\n\n\n", &g_log_mgr);
    return ;
}

void goblin_log_deinit()
{
    if (!g_log_mgr.is_initialize)
        return;

    g_log_mgr.is_initialize = 0;

    log_fclose();

    g_log_mgr.log_fd = NULL;
}

void goblin_log_set_level(int level)
{
    if (g_log_mgr.log_level > DBG_FATAL || g_log_mgr.log_level < DBG_VERBOSE)
    {
        g_log_mgr.log_level = LOG_LEVEL_DEFAULT;
    }
    else
    {
        g_log_mgr.log_level = level;
    }
}

void goblin_log_set_file_size(int size)
{
    if ((uint32_t)size > FILESIZE_LEN || size < 0)
    {
        g_log_mgr.log_file_size = FILESIZE_LEN;
    }
    else
    {
        g_log_mgr.log_file_size = size;
    }
}

void goblin_log_set_cyclic(int enable)
{
    g_log_mgr.is_cyclic = enable;
}

void goblin_log_set_file_bit(int bit)
{
    //UPDATE_LOG_PREFIX(g_log_mgr.log_bit, bit);
	g_log_mgr.log_bit = bit;

	printf("g_log_mgr.log_bit cnt:%x\n", g_log_mgr.log_bit);

}

void goblin_log_set_path(const char *path)
{
    if (!path)
    {
        fprintf(stderr, "invalid path!\n");
        return;
    }

    if (strlen(path) == 0)
    {
        fprintf(stderr, "invalid path!\n");
        return;
    }

    strncpy(g_log_mgr.log_path, path, sizeof(g_log_mgr.log_path)-1);
}

void goblin_log_set_write_disable()
{
    g_log_mgr.disable_write = 1;
}

//这钩子函数给库里面用
static GoblinLogOutput g_log_output = NULL;

void goblin_log_set_hook(GoblinLogOutput log_output)
{
	g_log_output = log_output;
}

void goblin_log_def_output(int level, const char *tag, const char *file, const long line, const char *func, const char *format, va_list ap)
{
	if (level < g_log_mgr.log_level /*|| !g_log_mgr.is_initialize*/)
		return;
	char s_buf[LOG_BUF_SIZE];
    char s_time[LOG_TIME_SIZE];
    char s_lvl[LOG_LEVEL_SIZE];
    char s_tag[LOG_TAG_SIZE];
    char s_pname[512];
    char s_pid[LOG_PNAME_SIZE];
    char s_tid[LOG_PNAME_SIZE];
    char s_file[LOG_TEXT_SIZE];
    char s_msg[LOG_BUF_SIZE+16];

    pthread_mutex_lock(&log_mutex);

	file = log_get_basename(file);
    // va_list arg;

    // va_start(arg, format);

    vsnprintf(s_buf, LOG_BUF_SIZE, format, ap);

    // va_end(arg);

    log_get_time(s_time, sizeof(s_time), 0);

#if 1
    switch (level)
    {
    case DBG_VERBOSE:
        snprintf(s_lvl, sizeof(s_lvl), B_CYAN("[%7s]"), log_level[level]);
        snprintf(s_msg, sizeof(s_msg), CYAN("%s"), s_buf);
        break;
    case DBG_DEBUG:
        snprintf(s_lvl, sizeof(s_lvl), B_WHITE("[%7s]"), log_level[level]);
        snprintf(s_msg, sizeof(s_msg), WHITE("%s"), s_buf);
        break;
    case DBG_INFO:
        snprintf(s_lvl, sizeof(s_lvl), B_GREEN("[%7s]"), log_level[level]);
        snprintf(s_msg, sizeof(s_msg), GREEN("%s"), s_buf);
        break;
    case DBG_WARN:
        snprintf(s_lvl, sizeof(s_lvl), B_YELLOW("[%7s]"), log_level[level]);
        snprintf(s_msg, sizeof(s_msg), YELLOW("%s"), s_buf);
        break;
    case DBG_ERROR:
        snprintf(s_lvl, sizeof(s_lvl), B_RED("[%7s]"), log_level[level]);
        snprintf(s_msg, sizeof(s_msg), RED("%s"), s_buf);
        break;
    case DBG_FATAL:
        snprintf(s_lvl, sizeof(s_lvl), B_MAGENTA("[%7s]"), log_level[level]);
        snprintf(s_msg, sizeof(s_msg), MAGENTA("%s"), s_buf);
        break;
    }
#else
    snprintf(s_lvl, sizeof(s_lvl), "[%7s]", log_level[level]);
    snprintf(s_msg, sizeof(s_msg), "%s", s_buf);
#endif

    snprintf(s_pname, sizeof(s_pname), "[%s ", g_log_mgr.proc_name);
    snprintf(s_pid, sizeof(s_pid), "pid:%d ", getpid());
    snprintf(s_tid, sizeof(s_tid), "tid:%d]", (int)syscall(__NR_gettid));
    snprintf(s_tag, sizeof(s_tag), "[%s]", tag);
    snprintf(s_file, sizeof(s_file), "[%s:%ld: %s] ", file, line, func);

    int cnt = 0, vidx = 0, midx = 0;

    log_chunk log_vector[LOG_MEMCHUNK_MAX];

    if (CHECK_LOG_PREFIX(g_log_mgr.log_bit, LOG_TIMESTAMP_BIT))
    {
        log_vector[cnt].buf = s_time;
        log_vector[cnt++].size = strlen(s_time);
    }

    if (CHECK_LOG_PREFIX(g_log_mgr.log_bit, LOG_PIDTID_BIT))
    {
        log_vector[cnt].buf = s_pname;
        log_vector[cnt++].size = strlen(s_pname);
        log_vector[cnt].buf = s_pid;
        log_vector[cnt++].size = strlen(s_pid);
        log_vector[cnt].buf = s_tid;
        log_vector[cnt++].size = strlen(s_tid);
    }

    vidx = cnt;

    log_vector[cnt].buf = s_lvl;
    log_vector[cnt++].size = strlen(s_lvl);

    if (CHECK_LOG_PREFIX(g_log_mgr.log_bit, LOG_TAG_BIT))
    {
        log_vector[cnt].buf = s_tag;
        log_vector[cnt++].size = strlen(s_tag);
    }

    if (CHECK_LOG_PREFIX(g_log_mgr.log_bit, LOG_FUNCLINE_BIT))
    {
        log_vector[cnt].buf = s_file;
        log_vector[cnt++].size = strlen(s_file);
    }

    midx = cnt;

    log_vector[cnt].buf = s_msg;
    log_vector[cnt++].size = strlen(s_msg);

    int i = 0, ret = 0, size = 0;

    FILE *fp = stdout;

    if (level > DBG_INFO)
        fp = stderr;

    for (i = 0; i < cnt; i++)
    {
        ret = fprintf(fp, "%s", log_vector[i].buf);

        if (ret != log_vector[i].size)
        {
            fprintf(stderr, "fprintf failed: %s\n", strerror(errno));
			pthread_mutex_unlock(&log_mutex);
            return;
        }

        size += ret;

        if (EOF == fflush(fp))
        {
            fprintf(stderr, "fflush failed: %s\n", strerror(errno));
			pthread_mutex_unlock(&log_mutex);
            return;
        }
    }

    if (level > DBG_DEBUG && g_log_mgr.log_fd && !g_log_mgr.disable_write)
    {
// #if 1
//         snprintf(s_lvl, sizeof(s_lvl), "[%7s]", log_level[level]);
//         snprintf(s_msg, sizeof(s_msg), "%s", s_buf);

//         log_vector[vidx].buf = s_lvl;
//         log_vector[vidx].size = strlen(s_lvl);

//         log_vector[midx].buf = s_msg;
//         log_vector[midx].size = strlen(s_msg);
// #endif

        snprintf(s_lvl, sizeof(s_lvl), "[%7s]", log_level[level]);
        snprintf(s_msg, sizeof(s_msg), "%s", s_buf);

        log_vector[vidx].buf = s_lvl;
        log_vector[vidx].size = strlen(s_lvl);

        log_vector[midx].buf = s_msg;
        log_vector[midx].size = strlen(s_msg);

        log_fwrite(log_vector, cnt);
    }

    pthread_mutex_unlock(&log_mutex);
}

void goblin_log_output(int level, const char *tag, const char *file, const long line, const char *func, const char *format, ...)
{
	if(g_log_output)
	{
		va_list ap;

		va_start(ap, format);
		g_log_output(level, tag, file, line, func, format, ap);
		va_end(ap);
		return ;
	}
    if (level < g_log_mgr.log_level /*|| !g_log_mgr.is_initialize*/)
        return;

    char s_buf[LOG_BUF_SIZE];
    char s_time[LOG_TIME_SIZE];
    char s_lvl[LOG_LEVEL_SIZE];
    char s_tag[LOG_TAG_SIZE];
    char s_pname[512];
    char s_pid[LOG_PNAME_SIZE];
    char s_tid[LOG_PNAME_SIZE];
    char s_file[LOG_TEXT_SIZE];
    char s_msg[LOG_BUF_SIZE+16];

    pthread_mutex_lock(&log_mutex);

	file = log_get_basename(file);
    va_list arg;

    va_start(arg, format);

    vsnprintf(s_buf, LOG_BUF_SIZE, format, arg);

    va_end(arg);

    log_get_time(s_time, sizeof(s_time), 0);

#if 1
    switch (level)
    {
    case DBG_VERBOSE:
        snprintf(s_lvl, sizeof(s_lvl), B_CYAN("[%7s]"), log_level[level]);
        snprintf(s_msg, sizeof(s_msg), CYAN("%s"), s_buf);
        break;
    case DBG_DEBUG:
        snprintf(s_lvl, sizeof(s_lvl), B_WHITE("[%7s]"), log_level[level]);
        snprintf(s_msg, sizeof(s_msg), WHITE("%s"), s_buf);
        break;
    case DBG_INFO:
        snprintf(s_lvl, sizeof(s_lvl), B_GREEN("[%7s]"), log_level[level]);
        snprintf(s_msg, sizeof(s_msg), GREEN("%s"), s_buf);
        break;
    case DBG_WARN:
        snprintf(s_lvl, sizeof(s_lvl), B_YELLOW("[%7s]"), log_level[level]);
        snprintf(s_msg, sizeof(s_msg), YELLOW("%s"), s_buf);
        break;
    case DBG_ERROR:
        snprintf(s_lvl, sizeof(s_lvl), B_RED("[%7s]"), log_level[level]);
        snprintf(s_msg, sizeof(s_msg), RED("%s"), s_buf);
        break;
    case DBG_FATAL:
        snprintf(s_lvl, sizeof(s_lvl), B_MAGENTA("[%7s]"), log_level[level]);
        snprintf(s_msg, sizeof(s_msg), MAGENTA("%s"), s_buf);
        break;
    }
#else
    snprintf(s_lvl, sizeof(s_lvl), "[%7s]", log_level[level]);
    snprintf(s_msg, sizeof(s_msg), "%s", s_buf);
#endif

    snprintf(s_pname, sizeof(s_pname), "[%s ", g_log_mgr.proc_name);
    snprintf(s_pid, sizeof(s_pid), "pid:%d ", getpid());
    snprintf(s_tid, sizeof(s_tid), "tid:%d]", (int)syscall(__NR_gettid));
    snprintf(s_tag, sizeof(s_tag), "[%s]", tag);
    snprintf(s_file, sizeof(s_file), "[%s:%ld: %s] ", file, line, func);

    int cnt = 0, vidx = 0, midx = 0;

    log_chunk log_vector[LOG_MEMCHUNK_MAX];

    if (CHECK_LOG_PREFIX(g_log_mgr.log_bit, LOG_TIMESTAMP_BIT))
    {
        log_vector[cnt].buf = s_time;
        log_vector[cnt++].size = strlen(s_time);
    }

    if (CHECK_LOG_PREFIX(g_log_mgr.log_bit, LOG_PIDTID_BIT))
    {
        log_vector[cnt].buf = s_pname;
        log_vector[cnt++].size = strlen(s_pname);
        log_vector[cnt].buf = s_pid;
        log_vector[cnt++].size = strlen(s_pid);
        log_vector[cnt].buf = s_tid;
        log_vector[cnt++].size = strlen(s_tid);
    }

    vidx = cnt;

    log_vector[cnt].buf = s_lvl;
    log_vector[cnt++].size = strlen(s_lvl);

    if (CHECK_LOG_PREFIX(g_log_mgr.log_bit, LOG_TAG_BIT))
    {
        log_vector[cnt].buf = s_tag;
        log_vector[cnt++].size = strlen(s_tag);
    }

    if (CHECK_LOG_PREFIX(g_log_mgr.log_bit, LOG_FUNCLINE_BIT))
    {
        log_vector[cnt].buf = s_file;
        log_vector[cnt++].size = strlen(s_file);
    }

    midx = cnt;

    log_vector[cnt].buf = s_msg;
    log_vector[cnt++].size = strlen(s_msg);

    int i = 0, ret = 0, size = 0;

    FILE *fp = stdout;

    if (level > DBG_INFO)
        fp = stderr;

    for (i = 0; i < cnt; i++)
    {
        ret = fprintf(fp, "%s", log_vector[i].buf);

        if (ret != log_vector[i].size)
        {
            fprintf(stderr, "fprintf failed: %s\n", strerror(errno));
			pthread_mutex_unlock(&log_mutex);
            return;
        }

        size += ret;

        if (EOF == fflush(fp))
        {
            fprintf(stderr, "fflush failed: %s\n", strerror(errno));
			pthread_mutex_unlock(&log_mutex);
            return;
        }
    }

    if (level > DBG_DEBUG && g_log_mgr.log_fd && !g_log_mgr.disable_write)
    {
// #if 1
//         snprintf(s_lvl, sizeof(s_lvl), "[%7s]", log_level[level]);
//         snprintf(s_msg, sizeof(s_msg), "%s", s_buf);

//         log_vector[vidx].buf = s_lvl;
//         log_vector[vidx].size = strlen(s_lvl);

//         log_vector[midx].buf = s_msg;
//         log_vector[midx].size = strlen(s_msg);
// #endif

        snprintf(s_lvl, sizeof(s_lvl), "[%7s]", log_level[level]);
        snprintf(s_msg, sizeof(s_msg), "%s", s_buf);

        log_vector[vidx].buf = s_lvl;
        log_vector[vidx].size = strlen(s_lvl);

        log_vector[midx].buf = s_msg;
        log_vector[midx].size = strlen(s_msg);

        log_fwrite(log_vector, cnt);
    }

    pthread_mutex_unlock(&log_mutex);
}

void goblin_log_hex(const char *file, const long line, uint8_t *data, size_t len)
{
	size_t i;

	char buffer[50], *ptr;

	ptr = buffer;

	for (i = 0; i < len; i++)
	{
		*ptr++ = hex_digital[0x0f & (data[i] >> 4)];
		*ptr++ = hex_digital[0x0f & data[i]];

		if ((i & 0x0f) == 0x0f)
		{
			*ptr = '\0';
			ptr = buffer;
			printf("(%s:%ld) %s\n", file, line, buffer);
		}
		else
		{
			*ptr++ = ' ';
		}
	}

	if (i & 0x0f)
	{
		*ptr = '\0';
		printf("(%s:%ld) %s\n", file, line, buffer);
	}

    printf("\n");
}

void goblin_log_hex_string(const char *file, const long line, uint8_t *data, size_t len)
{
#define BP_OFFSET 9
#define BP_GRAPH 60
#define BP_LEN 80

	char buffer[BP_LEN];
	size_t i;

	buffer[0] = '\0';

	for (i = 0; i < len; i++)
	{
		int n = i % 16;
		unsigned off;

		if (!n)
		{
			if (i)
			{
				printf("(%s:%ld) %s\n", file, line, buffer);
			}

			memset(buffer, ' ', sizeof(buffer) - 2);
			buffer[sizeof(buffer) - 2] = '\0';

			off = i % 0x0ffffU;

			buffer[2] = hex_digital[0x0f & (off >> 12)];
			buffer[3] = hex_digital[0x0f & (off >> 8)];
			buffer[4] = hex_digital[0x0f & (off >> 4)];
			buffer[5] = hex_digital[0x0f & off];
			buffer[6] = ':';
		}

		off = BP_OFFSET + n * 3 + ((n >= 8) ? 1 : 0);
		buffer[off] = hex_digital[0x0f & (data[i] >> 4)];
		buffer[off + 1] = hex_digital[0x0f & data[i]];

		off = BP_GRAPH + n + ((n >= 8) ? 1 : 0);

		if (isprint(data[i]))
		{
			buffer[BP_GRAPH + n] = data[i];
		}
		else
		{
			buffer[BP_GRAPH + n] = '.';
		}
	}

	printf("(%s:%ld) %s\n", file, line, buffer);
}