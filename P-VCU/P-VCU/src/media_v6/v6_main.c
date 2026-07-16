#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>

#include "fh_system_mpi.h"
#include "vlcview.h"


static int g_sig_stop = 0;
static void sample_vlcview_handle_sig(int signo)
{
    g_sig_stop = 1;
}

int main_v6(int argc, char *argv[])
{
    int  ret;
    char *dst_ip;
    unsigned int port;
    // Example usage of some system calls
    printf("Hello, World!\n");

    // // Fork a new process
    // pid_t pid = fork();
    // if (pid < 0) {
    //     perror("Fork failed");
    //     exit(EXIT_FAILURE);
    // } else if (pid == 0) {
    //     // Child process
    //     printf("This is the child process with PID: %d\n", getpid());
    //     exit(0);
    // } else {
    //     // Parent process
    //     wait(NULL); // Wait for child process to finish
    //     printf("Child process finished.\n");
    // }
    FH_SYS_SetReg(0x2c100094, 0x222);
    /*signal处理, CTRL+C退出*/
    signal(SIGINT,  sample_vlcview_handle_sig);
    signal(SIGQUIT, sample_vlcview_handle_sig);
    signal(SIGKILL, sample_vlcview_handle_sig);
    signal(SIGTERM, sample_vlcview_handle_sig);


    dst_ip = argc > 1 ? argv[1] : NULL;
    port   = argc > 2 ? strtol(argv[2], NULL, 0) : 1234;

    ret = _vlcview(dst_ip, port);
    if (ret != 0)
    {
        return -1;
    }

    while (!g_sig_stop)
    {
        usleep(20000);
    }

    _vlcview_exit();

    return 0;
}
// This code demonstrates the use of some system calls in C.

