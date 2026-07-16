#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/signalfd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdbool.h>
#include <pthread.h>

#include "tuyamain.h"
#include "ty_sdk_call.h"

static void cmd_help(int argc, char *args[])
{
	printf("================== help argc:%d %s================ \n", argc, args[0]);

    static const char *Usage =
    "********************************************************************************\n"
    "OVERVIEW: 测试控制台命令输入! \n"
    "\nOPTIONS:\n"
    "[start]           开启服务服务 o: init [pid] [sn] [key]\n"
    "[init]            初始化业务服务 o: tinit [pid] [sn] [key]\n"
    "[quit]             退出服务\n"
    "-a [--angle]           Video Angle to transform, defualt is 0, 1(90), 2(180), 3(270)\n"
    "\n********************************************************************************\n";

    printf("%s", Usage);
}

int32_t __main_terminated;

typedef struct cmd_table
{
	const char *cmd;
	const char *desc;
	uint16_t arg_cnt_min;
	uint16_t arg_cnt_max;
	void (*func)(int argc, char *args[]);
	const char *arg;
} cmd_table_t;

static void cmd_quit_app(int argc, char *args[])
{
    printf("cmd_quit_app\n");

    __main_terminated = 1;
}


static void cmd_start(int argc, char *args[])
{
    printf("################## call app ###################\n");
    TUYA_IPC_call_app();
}


static void cmd_system(int argc, char *argv[])
{
	char cmdbuf[512] = {0};

	if(argc > 0)
	{
		int index = 0;
		for (size_t i = 0; i < argc; i++)
		{
			index = sprintf(cmdbuf+index, "%s ", argv[i]);
		}
	}

    printf("Open argc :%d cmdbuf:%s\n", argc, cmdbuf);

	if(strlen(cmdbuf) > 0)
		system(cmdbuf);
}


//执行的命令参数
cmd_table_t cmd_table[] =
{
	//初始化mqtt服务
	{"cmd",		"cmd_system", 0, 10, cmd_system, ""},
	{"call",	"try one try", 0, 10, cmd_start, ""},

    {"help", "help 222", 0, 6, cmd_help, ""},
    {"quit",  "exit out", 0, 0, cmd_quit_app, ""},
    {"exit",  "exit out", 0, 0, cmd_quit_app, ""},
    {NULL, NULL, 0, 0, NULL},
};

#define PROMPT    "[mesh]"
#define COLOR_OFF  "\x1B[0m"
#define COLOR_BLUE "\x1B[0;34m"

#define CMD_ARGS_MAX 20

static int sigfd_setup(void)
{
	sigset_t mask;
	int fd;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
	{
		printf("Failed to set signal mask \n");
		return -1;
	}

	fd = signalfd(-1, &mask, 0);
	if (fd < 0)
	{
		printf("Failed to create signal descriptor \n");
		return -1;
	}

	return fd;
}

static void print_prompt(void)
{
	printf(COLOR_BLUE PROMPT COLOR_OFF "# ");
	fflush(stdout);
}

static void process_cmdline(char *input_str, uint32_t len)
{
	char *cmd, *arg;//, *parse_arg;
	char *args[CMD_ARGS_MAX];
	int argc = 0, i;

	if (!len)
	{
		printf("empty command \n");
		goto done;
	}

	if (!strlen(input_str))
		goto done;


	if (input_str[0] == '\n' || input_str[len - 1] == '\r')
		input_str[len - 1] = '\0';

	cmd = input_str;

    arg = strchr(input_str, ' ');

    if (arg)
    {
        *arg++ = '\0';
    }

	input_str = arg;

    int last;
    char *start, *end;

    argc = 0;
    start = input_str;

    while (argc < CMD_ARGS_MAX && start && *start != '\0')
    {
        while (*start == ' ' || *start == '\t')
            start++;
        if (*start == '\0')
            break;
        end = start;
        while (*end != ' ' && *end != '\t' && *end != '\0')
            end++;
        last = *end == '\0';
        *end = '\0';
        args[argc++] = start;
        if (last)
            break;
        start = end + 1;
    }

    if (argc > 0 && argc < CMD_ARGS_MAX)
    {
        args[argc] = NULL;
    }

	for (i = 0; cmd_table[i].cmd; i++)
	{
		if (strcmp(cmd, cmd_table[i].cmd))
			continue;

		if (cmd_table[i].func)
		{
			if (argc < cmd_table[i].arg_cnt_min ||
				argc > cmd_table[i].arg_cnt_max)
			{
				printf("%s Unexpected argc: %d range:%d-%d Please see help!\n", cmd, argc, cmd_table[i].arg_cnt_min, cmd_table[i].arg_cnt_max);
			}
			else
			{
				cmd_table[i].func(argc, args);
			}

			goto done;
		}
	}

	if (strcmp(cmd, "help"))
	{
		printf("Invalid command\n");
		goto done;
	}

	printf("Available commands:\n");

	for (i = 0; cmd_table[i].cmd; i++)
	{
		printf("\t%-20s\t\t%-35s\t\t%-15s\n", cmd_table[i].cmd,
				cmd_table[i].desc ? cmd_table[i].desc : "",
				cmd_table[i].arg  ? cmd_table[i].arg  : "");
	}

done:
	print_prompt();
	return;
}

static void stdin_read_handler(int fd)
{
    char line[1024];
    memset(line, 0, sizeof(line));

    fgets(line, sizeof(line), stdin);

	size_t read = strlen(line);

	if (read == 1)
	{
		printf("\n");
		print_prompt();
		return ;
	}

	line[read - 1] = '\0';

	process_cmdline(line, strlen(line) + 1);
}

int main_ctrl(void)
{


	int sigfd;
	struct pollfd pfd[2];

	sigfd = sigfd_setup();
	if (sigfd < 0)
		return EXIT_FAILURE;

	pfd[0].fd = sigfd;
	pfd[0].events = POLLIN | POLLHUP | POLLERR;

	pfd[1].fd = fileno(stdin);
	pfd[1].events = POLLIN | POLLHUP | POLLERR;

	while (!__main_terminated)
    {
		pfd[0].revents = 0;
		pfd[1].revents = 0;

		if (poll(pfd, 2, -1) == -1)
		{
			if (errno == EINTR)
				continue;
			printf("Poll error: %s\n", strerror(errno));
			return -1;
		}

		if (pfd[0].revents & (POLLHUP | POLLERR))
		{
			printf("Poll rdhup or hup or err\n");
			return -1;
		}

		if (pfd[1].revents & (POLLHUP | POLLERR))
		{
			printf("Poll rdhup or hup or err\n");
			return -1;
		}

		if (pfd[0].revents & POLLIN)
		{
			struct signalfd_siginfo si;
			ssize_t ret;

			ret = read(pfd[0].fd, &si, sizeof(si));

			if (ret != sizeof(si))
				return -1;

			switch (si.ssi_signo)
			{
				case SIGINT:
				case SIGTERM:
					__main_terminated = 1;
					break;
			}
		}

		if (pfd[1].revents & POLLIN)
			stdin_read_handler(pfd[1].fd);
    }

    printf("\n\n===============########out#########===============\n\n\n");
    return 0;
}

int main(int argc, char *argv[])
{
    printf("Hello, world!\n");

    StartTuyaServer();

    return main_ctrl();
    do
    {
        sleep(1);
    } while (1);

    return 0;
}