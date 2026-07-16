#include <utils/log.h>

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>

#include <pthread.h>
#include <libgen.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

#define USE_SYSLOG

#ifdef USE_SYSLOG
	#include <sys/syslog.h>
#else
	#include <android/log.h>
	#include <utils/logcat.h>
#endif

#define LOG_LEVEL_SIZE            (32)
#define LOG_TIMESTAMP_SIZE        (32)
#define LOG_COLOR_SIZE            (32)
#define LOG_TAG_SIZE              (32)

// #define LOG_BUFFER_PADDING_SIZE   (LOG_LEVEL_SIZE + LOG_TIMESTAMP_SIZE + LOG_TEXT_SIZE + LOG_COLOR_SIZE)
#define LOG_BUFFER_PADDING_SIZE   (LOG_LEVEL_SIZE + LOG_TIMESTAMP_SIZE + LOG_TAG_SIZE)
#define LOG_BUFFER_SIZE           (4000)

#define LOG_HEXBUFFER_SIZE        (128)

#define COLOR_START                    "\033["
#define COLOR_END                      "\033[0m"

/* output log front color */
#define F_BLACK                        "30;"
#define F_RED                          "31;"
#define F_GREEN                        "32;"
#define F_YELLOW                       "33;"
#define F_BLUE                         "34;"
#define F_MAGENTA                      "35;"
#define F_CYAN                         "36;"
#define F_WHITE                        "37;"

/* output log background color */
#define B_NULL                         ""
#define B_BLACK                        "40;"
#define B_RED                          "41;"
#define B_GREEN                        "42;"
#define B_YELLOW                       "43;"
#define B_BLUE                         "44;"
#define B_MAGENTA                      "45;"
#define B_CYAN                         "46;"
#define B_WHITE                        "47;"

/* output log fonts style */
#define S_BOLD                         "1m"
#define S_UNDERLINE                    "4m"
#define S_BLINK                        "5m"
#define S_NORMAL                       "22m"

static log_level_t log_level = LOG_LEVEL_INFO;

static volatile bool log_console_enable = true;

static const char *log_level_str[] =
{
	[LOG_LEVEL_FATAL]   = "F",
	[LOG_LEVEL_ERROR]   = "E",
	[LOG_LEVEL_WARN]    = "W",
	[LOG_LEVEL_INFO]    = "I",
	[LOG_LEVEL_DEBUG]   = "D",
	[LOG_LEVEL_VERBOSE] = "V",
};

static const char *log_color_bold[] =
{
	[LOG_LEVEL_FATAL]   = F_MAGENTA B_NULL S_BOLD,
	[LOG_LEVEL_ERROR]   = F_RED B_NULL S_BOLD,
	[LOG_LEVEL_WARN]    = F_YELLOW B_NULL S_BOLD,
	[LOG_LEVEL_INFO]    = F_GREEN B_NULL S_BOLD,
	[LOG_LEVEL_DEBUG]   = F_WHITE B_NULL S_BOLD,
	[LOG_LEVEL_VERBOSE] = F_CYAN B_NULL S_BOLD,
};

static const char *log_color_normal[] =
{
	[LOG_LEVEL_FATAL]   = F_MAGENTA B_NULL S_NORMAL,
	[LOG_LEVEL_ERROR]   = F_RED B_NULL S_NORMAL,
	[LOG_LEVEL_WARN]    = F_YELLOW B_NULL S_NORMAL,
	[LOG_LEVEL_INFO]    = F_GREEN B_NULL S_NORMAL,
	[LOG_LEVEL_DEBUG]   = F_WHITE B_NULL S_NORMAL,
	[LOG_LEVEL_VERBOSE] = F_CYAN B_NULL S_NORMAL,
};

static const char *log_get_basename(const char *path)
{
	const char *file_name = strrchr(path, '/');

	return file_name ? (file_name + 1) : path;
}

static int log_get_time(char *buf, int buf_len)
{
	char time_format[64];
	char time_str[16];
	struct timeval tv;
	struct tm now_tm;
	int now_ms;
	time_t now_sec;

	gettimeofday(&tv, NULL);

	now_sec = tv.tv_sec;
	now_ms  = tv.tv_usec / 1000;

	localtime_r(&now_sec, &now_tm);

	strftime(time_format, sizeof(time_format), "%Y-%m-%d %H:%M:%S", &now_tm);
	snprintf(time_str, sizeof(time_str), "%03d", now_ms);

	return snprintf(buf, buf_len, "[%s.%s]", time_format, time_str);
}

static void syslog_output(char *buf, int len)
{
	char syslog_buf[LOG_BUFFER_SIZE + LOG_BUFFER_PADDING_SIZE];
	int syslog_len = 0;

	char *data = buf;
	int size = len;

	while (size && syslog_len < (LOG_BUFFER_SIZE + LOG_BUFFER_PADDING_SIZE - 1))
	{
		if (*data != '\r')
		{
			syslog_buf[syslog_len] = *data;
			syslog_len++;
		}

		++data;
		--size;
	}

	if (syslog_len > 0)
	{
		syslog_buf[syslog_len] = '\0';

		syslog(LOG_INFO, "%.*s", syslog_len, syslog_buf);
	}
}

int __wrap_printf(const char *format, ...)
{
	va_list arg;

	int n;
	int write_len = 0;

	char buf[LOG_BUFFER_SIZE + LOG_BUFFER_PADDING_SIZE];

	int buffer_size = sizeof(buf);

	va_start(arg, format);
	n = vsnprintf(buf, sizeof(buf), format, arg);
	va_end(arg);

	if (n <= 0)
	{
		// 格式化错误，直接返回
		return n;
	}

	// 使用处理后的长度进行写入
	// 处理字符串结尾，确保以\r\n\0结尾
	// n >= buffer_size 表示缓冲区不足
	if (n >= buffer_size)
	{
		// 设置倒数第三个字符是'\r'，倒数第二个字符是'\n'，最后一个字符是'\0'
		buf[buffer_size - 3] = '\r';
		buf[buffer_size - 2] = '\n';
		buf[buffer_size - 1] = '\0';

		write_len = buffer_size - 1; // 写入到\n的位置（不包括\0）
	}
	// n 小于缓冲区
	else
	{
		// 检查结尾处是否包含\r\n
		// 情况1：以\r\n结尾
		if (n >= 2 && buf[n - 2] == '\r' && buf[n - 1] == '\n')
		{
			// 已经以\r\n结尾，只需要确保有\0
			buf[n] = '\0';
			write_len = n; // 写入整个字符串（包括\r\n，不包括\0）
		}
		// 情况2：以\n结尾但没有\r
		else if (n >= 1 && buf[n - 1] == '\n')
		{
			// 检查前面是否有足够的空间插入\r
			if (n < buffer_size - 2)
			{
				// 将\n向后移动一位，前面插入\r
				buf[n] = '\n';      // 原来的\n移动到新位置
				buf[n - 1] = '\r';  // 在原来\n的位置插入\r
				buf[n + 1] = '\0';  // 添加终止符

				write_len = n + 1;  // 写入到新的\n的位置（不包括\0）
			}
			else
			{
				// 空间不足，强制覆盖最后三个字符为\r\n\0
				buf[buffer_size - 3] = '\r';
				buf[buffer_size - 2] = '\n';
				buf[buffer_size - 1] = '\0';

				write_len = buffer_size - 2; // 写入到\n的位置（不包括\0）
			}
		}
		// 情况3：不以\n结尾
		else
		{
			// 直接添加\r\n\0
			if (n < buffer_size - 3)
			{
				buf[n] = '\r';
				buf[n + 1] = '\n';
				buf[n + 2] = '\0';

				write_len = n + 2; // 写入到\n的位置（不包括\0）
			}
			else
			{
				// 空间不足，强制覆盖最后三个字符为\r\n\0
				buf[buffer_size - 3] = '\r';
				buf[buffer_size - 2] = '\n';
				buf[buffer_size - 1] = '\0';

				write_len = buffer_size - 2; // 写入到\n的位置（不包括\0）
			}
		}
	}

	if (log_console_enable)
	{
		write(STDOUT_FILENO, buf, write_len);
	}

#ifdef USE_SYSLOG
	syslog_output(buf, write_len);
#endif

	return n;
}

static void term_set_color(log_level_t level)
{
	char ansi_color[LOG_COLOR_SIZE] = {0};

	int len = snprintf(ansi_color, sizeof(ansi_color), "%s%s", COLOR_START, log_color_normal[level]);

	write(STDOUT_FILENO, ansi_color, len);
}

static void term_restore_color(void)
{
	char ansi_color[LOG_COLOR_SIZE] = {0};

	int len = snprintf(ansi_color, sizeof(ansi_color), "%s", COLOR_END);

	write(STDOUT_FILENO, ansi_color, len);
}

static void log_default(log_level_t level, const char *tag, const char *file, const long line,
                        const char *func, const char *buf)
{
	int out_buf_len = 0, len = 0;
	int remaining = 0;
	int ret = 0;

	char out_buf[LOG_BUFFER_SIZE + LOG_BUFFER_PADDING_SIZE];

	out_buf_len = sizeof(out_buf);

	// 确保缓冲区以空字符开头
	out_buf[0] = '\0';

	// 1、添加日志时间
	remaining = out_buf_len - len;

	if (remaining > 0)
	{
		ret = log_get_time(out_buf + len, remaining);

		if (ret < 0)
		{
			// 处理错误
			ret = 0;
		}
		else if (ret >= remaining)
		{
			// 发生截断，调整len到缓冲区末尾
			len = out_buf_len - 1;
			out_buf[len] = '\0';
		}
		else
		{
			len += ret;
		}
	}

	// 2. 添加日志级别
	remaining = out_buf_len - len;

	if (remaining > 0)
	{
		ret = snprintf(out_buf + len, remaining, " [%s]", log_level_str[level]);

		if (ret < 0)
		{
			// 格式化错误
			ret = 0;
		}
		else if (ret >= remaining)
		{
			// 发生截断，调整len到缓冲区末尾
			len = out_buf_len - 1;
			out_buf[len] = '\0';
		}
		else
		{
			len += ret;
		}
	}

	// 3. 添加文件、函数和行号信息
	remaining = out_buf_len - len;

	if (remaining > 0)
	{
		ret = snprintf(out_buf + len, remaining, " [%s|%s(%ld)]", file, func, line);

		if (ret < 0)
		{
			// 格式化错误
			ret = 0;
		}
		else if (ret >= remaining)
		{
			// 发生截断，调整len到缓冲区末尾
			len = out_buf_len - 1;
			out_buf[len] = '\0';
		}
		else
		{
			len += ret;
		}
	}

	// 4. 添加日志消息内容
	remaining = out_buf_len - len;

	if (remaining > 0)
	{
		ret = snprintf(out_buf + len, remaining, " %s", buf);

		if (ret < 0)
		{
			// 格式化错误
			ret = 0;
		}
		else if (ret >= remaining)
		{
			// 发生截断，调整len到缓冲区末尾
			len = out_buf_len - 1;
			out_buf[len] = '\0';
		}
		else
		{
			len += ret;
		}
	}

	// 确保字符串以\0结尾
	if (len >= out_buf_len)
	{
		len = out_buf_len - 1;
	}

	out_buf[len] = '\0';

	if (log_console_enable)
	{
		term_set_color(level);

		write(STDOUT_FILENO, out_buf, len);

		term_restore_color();
	}

#ifdef USE_SYSLOG
	syslog_output(out_buf, len);
#endif
}

static int convert_level_to_prio(log_level_t level)
{
	int prio;

#ifdef USE_SYSLOG

	switch (level)
	{
		case LOG_LEVEL_VERBOSE:
			prio = LOG_DEBUG;
			break;

		case LOG_LEVEL_DEBUG:
			prio = LOG_DEBUG;
			break;

		case LOG_LEVEL_INFO:
			prio = LOG_INFO;
			break;

		case LOG_LEVEL_WARN:
			prio = LOG_WARNING;
			break;

		case LOG_LEVEL_ERROR:
			prio = LOG_ERR;
			break;

		case LOG_LEVEL_FATAL:
			prio = LOG_CRIT;
			break;

		default:
			prio = LOG_INFO;
			break;
	}

#else

	switch (level)
	{
		case LOG_LEVEL_VERBOSE:
			prio = ANDROID_LOG_VERBOSE;
			break;

		case LOG_LEVEL_DEBUG:
			prio = ANDROID_LOG_DEBUG;
			break;

		case LOG_LEVEL_INFO:
			prio = ANDROID_LOG_INFO;
			break;

		case LOG_LEVEL_WARN:
			prio = ANDROID_LOG_WARN;
			break;

		case LOG_LEVEL_ERROR:
			prio = ANDROID_LOG_ERROR;
			break;

		case LOG_LEVEL_FATAL:
			prio = ANDROID_LOG_FATAL;
			break;

		default:
			prio = ANDROID_LOG_DEFAULT;
			break;
	}

#endif

	return prio;
}

void log_init(const char *ident)
{
#ifdef USE_SYSLOG
	openlog(ident, LOG_NOWAIT | LOG_NDELAY/*| LOG_PID*/, LOG_USER);
#else
	logcat_init("time", NULL, "/var/log/evt_ai_terminal.log", 512 * 1024, 1);
#endif
}

void log_output(log_level_t level, const char *tag, const char *file, const long line,
                const char *func, const char *format, ...)
{
	int n, prio;

	va_list ap;

	char buf[LOG_BUFFER_SIZE];

	const char *base_file = NULL;

	if (level > log_level)
	{
		return ;
	}

	va_start(ap, format);
	n = vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	// n < 0, 表示格式化错误
	// n 为 0，表示生成的字符串为空。
	if (n <= 0)
	{
		buf[0] = '\r';
		buf[1] = '\n';
		buf[2] = '\0';
	}
	// 缓冲区不足
	else if (n >= LOG_BUFFER_SIZE)
	{
		// 确保字符串以 "\r\n\0" 结尾
		buf[LOG_BUFFER_SIZE - 3] = '\r';
		buf[LOG_BUFFER_SIZE - 2] = '\n';
		buf[LOG_BUFFER_SIZE - 1] = '\0';
	}
	// n 小于缓冲区
	else
	{
		// 检查结尾处是否包含\r\n
		// 情况1：以\r\n结尾
		if (n >= 2 && buf[n - 2] == '\r' && buf[n - 1] == '\n')
		{
			// 已经以\r\n结尾，只需要确保有\0
			if (n < LOG_BUFFER_SIZE - 1)
			{
				buf[n] = '\0';
			}
			else
			{
				// 理论上不会走到这里，因为n<LOG_BUFFER_SIZE
				buf[LOG_BUFFER_SIZE - 1] = '\0';
			}
		}
		// 情况2：以\n结尾但没有\r
		else if (n >= 1 && buf[n - 1] == '\n')
		{
			// 检查前面是否有足够的空间插入\r
			if (n < LOG_BUFFER_SIZE - 3)
			{
				// 将\n向后移动一位，前面插入\r
				buf[n] = '\n';      // 原来的\n移动到新位置
				buf[n - 1] = '\r';  // 在原来\n的位置插入\r
				buf[n + 1] = '\0';  // 添加终止符
			}
			else
			{
				// 空间不足，强制覆盖最后三个字符为\r\n\0
				buf[LOG_BUFFER_SIZE - 3] = '\r';
				buf[LOG_BUFFER_SIZE - 2] = '\n';
				buf[LOG_BUFFER_SIZE - 1] = '\0';
			}
		}
		// 情况3：不以\n结尾
		else
		{
			// 直接添加\r\n\0
			if (n < LOG_BUFFER_SIZE - 3)
			{
				buf[n] = '\r';
				buf[n + 1] = '\n';
				buf[n + 2] = '\0';
			}
			else
			{
				// 空间不足，强制覆盖最后三个字符为\r\n\0
				buf[LOG_BUFFER_SIZE - 3] = '\r';
				buf[LOG_BUFFER_SIZE - 2] = '\n';
				buf[LOG_BUFFER_SIZE - 1] = '\0';
			}
		}
	}

	base_file = log_get_basename(file);

	prio = convert_level_to_prio(level);

	log_default(level, tag, base_file, line, func, buf);
}

void log_hex_buffer(const char *tag, const char *file,
                    const long line, const char *func,
                    const void *data, size_t size)
{
#define HEXDUMP_WIDTH                 16

	static const char hex_digital[] = "0123456789ABCDEF";

	size_t i = 0, j = 0;

	int offset;

	const uint8_t *hex_buffer = NULL;

	const char *base_file = NULL;

	int out_buf_len = 0, len = 0;

	char out_buf[LOG_BUFFER_PADDING_SIZE + LOG_HEXBUFFER_SIZE];

	char buf[LOG_HEXBUFFER_SIZE];

	out_buf_len = sizeof(out_buf);

	hex_buffer = (const uint8_t *)data;

	base_file = log_get_basename(file);

	len += log_get_time(out_buf, out_buf_len);

	len += snprintf(out_buf + len, out_buf_len - len, " %s%s[%s]%s", COLOR_START, log_color_bold[LOG_LEVEL_DEBUG],
	                log_level_str[LOG_LEVEL_DEBUG], COLOR_END);

	len += snprintf(out_buf + len, out_buf_len - len, " %s [%s|%s(%ld)]", tag, base_file, func, line);

	printf("%s length:%zu data:\r\n", out_buf, size);

	printf("Offset (h) 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");

	for (i = 0; i < size; i += HEXDUMP_WIDTH)
	{
		offset = 0;

		offset += snprintf(buf, LOG_HEXBUFFER_SIZE - offset, "[%08zX] ", i);

		/* dump hex */
		for (j = 0; j < HEXDUMP_WIDTH; j++)
		{
			if (i + j < size)
			{
				buf[offset++] = hex_digital[0x0f & (hex_buffer[i + j] >> 4)];
				buf[offset++] = hex_digital[0x0f & (hex_buffer[i + j] >> 0)];
			}
			else
			{
				buf[offset++] = ' ';
				buf[offset++] = ' ';
			}

			buf[offset++] = ' ';
		}

		/* dump char for hex */
		for (j = 0; j < HEXDUMP_WIDTH; j++)
		{
			if (i + j < size)
			{
				buf[offset++] = isprint(hex_buffer[i + j]) ? hex_buffer[i + j] : '.';
			}
		}

		buf[offset] = 0;

		printf("%s\r\n", buf);
	}

	// printf("offset :%d\n", offset);
	printf("\r\n");
}

void log_console_onoff(bool onoff)
{
	log_console_enable = onoff;
}