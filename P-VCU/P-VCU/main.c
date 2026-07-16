#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <poll.h>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <ucontext.h>
#include <execinfo.h>

#include "media/video_encode.h"
#include "dplib.h"
#include "sem.h"
#include "bmem.h"
#include "log.h"
#include "list.h"
#include "time_helper.h"
#include "common_defs.h"
#include "sys/sys_event.h"
#include "base/ServiceFile.h"
#include "dpbase.h"
#include "data/DBICCard.h"
#include "data/DBLanguage.h"
#include "hotplug.h"
#include "src/sound/keytone_mgnt.h"
#include "src/transport/trans.h"
#include "src/transport/spi_plc/spi_plc_mgnt.h"
#include "src/transport/tran_audio.h"
#include "src/transport/tran_video.h"
#include "src/transport/spi_plc/spi_plc_trans.h"
#include "src/transport/event_plc/goblin_plc_process.h"
#include "src/transport/event_plc/goblin_plc_callcore.h"

#ifdef ENABLE_V1
#include <AudioCore.h>
#include <plc_service.h>
#include <sound_play.h>
#include <gpio_service.h>
#include <hard_node_service.h>
#include <pc_service.h>
#include <watchdog_mgnt.h>
#include <hardware_test.h>
#endif

#ifdef ENABLE_NX5
#include <AudioCore.h>
#include <video/vd/VDecApi.h>
#include <video/vd/VoApi.h>
#endif

#ifdef ENABLE_V6
#include "fh_system_mpi.h"
#include "dsp/fh_venc_mpi.h"
#include "vlcview.h"
#include <plc_service.h>
#include <sound_play.h>
#include <gpio_service.h>
#include <hard_node_service.h>
#include <pc_service.h>
#include <watchdog_mgnt.h>
#include <hardware_test.h>
#include <video_service.h>
#include <card_service.h>
#endif


#include <dpinput.h>

#define PLC_DEV_DEFAULT_ADDR 0x3000002
int ota_upgrade_image(char *pfilename);

#include "main_test.cc"




#define BT_SIZE  100
#define CMD_SIZE 1024

static int signals_trace[] =
{
    SIGILL,  /* Illegal instruction (ANSI).  */
    SIGABRT, /* Abort (ANSI).  */
    SIGBUS,  /* BUS error (4.2 BSD). (unaligned access) */
    SIGFPE,  /* Floating-point exception (ANSI).  */
    SIGSEGV, /* Segmentation violation (ANSI).  */
};

#ifndef ENABLE_V1
static void *get_uc_mcontext_pc(ucontext_t *uc)
{
#if defined(__APPLE__) && !defined(MAC_OS_X_VERSION_10_6)
    /* OSX < 10.6 */
#if defined(__x86_64__)
    return (void *)uc->uc_mcontext->__ss.__rip;
#elif defined(__i386__)
    return (void *)uc->uc_mcontext->__ss.__eip;
#else
    return (void *)uc->uc_mcontext->__ss.__srr0;
#endif
#elif defined(__APPLE__) && defined(MAC_OS_X_VERSION_10_6)
    /* OSX >= 10.6 */
#if defined(_STRUCT_X86_THREAD_STATE64) && !defined(__i386__)
    return (void *)uc->uc_mcontext->__ss.__rip;
#else
    return (void *)uc->uc_mcontext->__ss.__eip;
#endif
#elif defined(__linux__)
    /* Linux */
#if defined(__i386__)
    return (void *)uc->uc_mcontext.gregs[REG_EIP]; /* Linux 32 */
#elif defined(__X86_64__) || defined(__x86_64__)
    return (void *)uc->uc_mcontext.gregs[REG_RIP]; /* Linux 64 */
#elif defined(__ia64__) /* Linux IA64 */
    return (void *)uc->uc_mcontext.sc_ip;
#elif defined(__arm__)
    return (void *)uc->uc_mcontext.arm_pc;
#elif defined(__aarch64__)
    return (void *)uc->uc_mcontext.pc;
#else
    return NULL;
#endif
#else
    return NULL;
#endif
}
#endif
static void backtrace_handler(int sig_num, siginfo_t *info, void *ucontext)
{
	//v1 不存在backtrace接口
#ifndef ENABLE_V1
    void *array[BT_SIZE];
    int i, size = 0;
    char **strings = NULL;
    void *caller_addr = get_uc_mcontext_pc((ucontext_t *)ucontext);

    fprintf(stderr, "Program received signal %s \n", strsignal(sig_num));
    fprintf(stderr, "[signum:%d]: %s [%p] \n", sig_num, strerror(sig_num), caller_addr);

    size = backtrace(array, BT_SIZE);

    // overwrite sigaction with caller's address, but seems no needed

    array[1] = caller_addr;
    strings = backtrace_symbols(array, size);

    for(i = 0; i < size; ++i)
    {
        fprintf(stderr, "#%d  %s\n", i, strings[i]);
    }
#endif
    printf("Catch a signal for something\n");
	spi_plc_mgnt_deinit();
#ifdef ENABLE_MEDIA
	video_enc_stop();
#endif
#ifdef ENABLE_V6
	_vlcview_exit();
#endif //ENABLE_V6

    sleep(1);
	printf("####################err , ready to exit####################\n");
	sync();
	kill(1,SIGTERM);
    exit(EXIT_SUCCESS);
}

int DPBacktrace(void)
{
    int i;
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_sigaction = backtrace_handler;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    int ret = 0;

    for(i = 0; i < (int)ARRAY_SIZE(signals_trace); ++i)
    {
        if(sigaction(signals_trace[i], &sa, NULL) != 0)
        {
            fprintf(stderr, "backtrace failed to set signal handler for %s(%d)! \n", strsignal(signals_trace[i]), signals_trace[i]);
            ret = -1;
            break;
        }
    }

    return ret;
}
#if defined(ENABLE_V1) || defined(ENABLE_V6)
static void UpgradePlcContent(void)
{
	//判断是否存在PLC升级文件
#ifdef DOWNLOAD_IMAGE_PLC_CACHE_PATH
	if(access(DOWNLOAD_IMAGE_PLC_CACHE_PATH, F_OK) == 0)
	{
		LOGE("Ready to upgrade PLC, file:%s\n", DOWNLOAD_IMAGE_PLC_CACHE_PATH);
		uint32_t lasttick = time_relative_ms();
		goblin_plc_upgrade_firmware(DOWNLOAD_IMAGE_PLC_CACHE_PATH);
		goblin_plc_reboot();
		remove(DOWNLOAD_IMAGE_PLC_CACHE_PATH);
		LOGE("upgrade PLC done lost time:%u, please reboot\n", time_relative_ms() - lasttick);
		sleep(1);
		sync();
		kill(1,SIGTERM);
	}
	else
	{
		LOGE("not found PLC upgrade file:%s\n", DOWNLOAD_IMAGE_PLC_CACHE_PATH);
	}
#else
	LOGE("not exit PLC cache upgrade file\n");
#endif
}
#endif//
static void sys_event_process(sys_event_t *event)
{
	//是否正在升级
	static bool bupgrade_work = false;
	if (event->type == SYS_EVENT_BASE)
	{
		LOGI("EVENT TYPE state:%d\n", event->body.base_msg.cmd);
		//重启
		if(event->body.base_msg.cmd == SYS_EVENT_BASE_REBOOT ||
			SYS_EVENT_BASE_FACTORY_RESET == event->body.base_msg.cmd ||
			SYS_EVENT_BASE_FACTORY_RESET_ALL == event->body.base_msg.cmd)
		{
			if(SYS_EVENT_BASE_FACTORY_RESET == event->body.base_msg.cmd ||
				SYS_EVENT_BASE_FACTORY_RESET_ALL == event->body.base_msg.cmd)
			{
				char strsn[128] = {0};
				strncpy(strsn, GetLocalSn(), 128-1);
				LOGI("Factory reset, strsn:%s\n", strsn);
				//删除数据库
				ResetSystemSet();
				ResetICCardDB();
				ResetLanguageDB();
				if(SYS_EVENT_BASE_FACTORY_RESET == event->body.base_msg.cmd)
				{
					//sn只有恢复出厂设置全才会删除sn, 重置值把设置参数清空
					SetLocalSn(strsn);
				}
#ifdef PLC_RUN_LOG_PATH
				//删除日志
				DPDeleteFile(PLC_RUN_LOG_PATH);
#endif
				//延时再重启，等待任务都完成
				sleep(3);
			}
			sync();
			kill(1,SIGTERM);
		}
		//进入升级
		else if(event->body.base_msg.cmd == SYS_EVENT_BASE_UPGRADE)
		{
#if defined(ENABLE_V1) || defined(ENABLE_V6)
			bupgrade_work = true;
			//升级前把视频编码关掉
			_vlcview_exit();
			LOGE("Read to upgrade, please wait, remote device:%x\n", event->body.base_msg.devid);
			uint32_t lasttick = time_relative_ms();
			ota_upgrade_image(DOWNLOAD_IMAGE_PATH);
			LOGE("upgrade done lost time:%u, please reboot\n", time_relative_ms() - lasttick);
			UpgradePlcContent();
			sync();
			kill(1,SIGTERM);
#endif
		}
		else if(event->body.base_msg.cmd == SYS_EVENT_BASE_UPGRADE_PLC)
		{
#if defined(ENABLE_V1) || defined(ENABLE_V6)
			LOGE("Read to upgrade PLC, please wait\n");
			uint32_t lasttick = time_relative_ms();
			goblin_plc_upgrade_firmware(DOWNLOAD_IMAGE_PLC_PATH);
			goblin_plc_reboot();
			LOGE("upgrade PLC done lost time:%u, please reboot\n", time_relative_ms() - lasttick);
			sleep(1);
			sync();
			kill(1,SIGTERM);
#endif
		}
	}
	else if (event->type == SYS_EVENT_TIME)
	{
	}
	else if (event->type == SYS_EVENT_KEY) //SYS_EVENT_KEY
	{
		//升级过程中不响应按键
		if(bupgrade_work){
			LOGW("In upgrade, ignore key event\n");
			return;
		}
#if defined(ENABLE_V1) || defined(ENABLE_V6)
		if(hardware_test_is_test())
		{
			LOGI("hardware test is running, ignore key event\n");
			return;
		}
#endif
		//printf("按键：%c \n", event->body.kbd_msg.code);
#ifdef  ENABLE_V1
		if(event->body.kbd_msg.code < '5')
			plc_service_key_event('5'-event->body.kbd_msg.code);
#elif ENABLE_V6
		if(event->body.kbd_msg.code < 5)
			plc_service_key_event(event->body.kbd_msg.code);
#else
		static void *psession =NULL;
		switch (event->body.kbd_msg.code)
		{
		case '1':
			{
				if(!psession)
					psession = goblin_call_by_dev(0x1000101);
				printf("call out session:%p \n", psession);

			}
			break;
		case '2':
			{
				if(!psession)
					psession = goblin_call_by_dev(0x1000102);
				printf("call out session:%p \n", psession);
			}
			break;
		case '3':
			{
				// psession = goblin_call_by_dev(0x1000001);
				// psession = goblin_call_by_dev(0x1000002);
			}
			break;
		case '4':
			{
				goblin_hangup_by_session(psession);
				psession = NULL;
			}
			break;
		default:
			break;
		}
#endif
	}
	else if (SYS_EVENT_ADDR == event->type)
	{
		LOGI("SYS_EVENT_ADDR state:%x\n", event->body.addr_switch.state);
#if defined(ENABLE_V1) || defined(ENABLE_V6)
		plc_service_devid_reload(PLC_DEV_DEFAULT_ADDR - event->body.addr_switch.state);
#endif
		goblin_plc_reload_devID(PLC_DEV_DEFAULT_ADDR - event->body.addr_switch.state);
		goblin_call_reload_devID(PLC_DEV_DEFAULT_ADDR - event->body.addr_switch.state);
	}

}

static void *msg_process_thread(void *arg)
{
	sys_event_mgnt_init();

	while (1)
	{
		sys_event_t event;

		memset(&event, 0, sizeof(event));

		if (sys_event_dispatch(&event) == 0)
		{
			//printf("event type:%d\n", event.type);
			sys_event_process(&event);
		}
	}
	return 0;
}

void InitServer(void)
{
//#ifdef ENABLE_V1
#if defined(ENABLE_V1) || defined(ENABLE_V6)
	//1.初始化文件操作服务
	InitFileServer();

	//2.初始化系统设置
	InitSystemSet();
	InitICCardDB();
	InitLanguageDB();

	//3.初始化声音播放管理
	sound_play_mgnt_init(false);
#ifndef ENABLE_V6
	//4.初始化按键管理
	keytone_mgnt_init();
#endif
	uint32_t switch_state = 0;
	//5.初始化GPIO管理
	gpio_mgnt_init(&switch_state);

	//6.初始化硬件节点控制服务
	hard_node_mgnt_init();

	//7.初始化PLC服务
	plc_service_init(PLC_DEV_DEFAULT_ADDR - switch_state);

	//8.pc端工具
	StartPCServer();
	//9.TF卡检测
	sdcard_monitor_init();

	//10.硬件测试服务
	hareware_test_service_init();
#ifdef NDEBUG
	if(access("/mnt/UDISK/debugflag", F_OK) == -1 && access("/data/debugflag", F_OK) == -1)
	{
		watchdog_mgnt_init(WDT_TYPE_SOFT);
		printf("================== The is a release, please touch /mnt/UDISK/debugflag or /data/debugflag to debug ================\n");
	}
	else
	{
		printf("================== The is a debug ================\n");
	}
#endif //NDEBUG
#endif
}

#ifdef ENABLE_NX5
// 绘制红色到 Framebuffer
void draw_red_on_framebuffer(int fbfd, struct fb_var_screeninfo vinfo, struct fb_fix_screeninfo finfo, char *fbp) {
    long int location;
    int x, y;
    for (y = 0; y < vinfo.yres; y++) {
        for (x = 0; x < vinfo.xres; x++) {
            // 计算像素位置
            location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) +
                       (y + vinfo.yoffset) * finfo.line_length;

            // 根据位深度设置红色像素
            if (vinfo.bits_per_pixel == 32) {
                // 32位深度
                *(fbp + location) = 0;        // 蓝色
                *(fbp + location + 1) = 0;    // 绿色
                *(fbp + location + 2) = 255;  // 红色
                *(fbp + location + 3) = 100;    // 透明度
            } else if (vinfo.bits_per_pixel == 16) {
                // 16位深度
                unsigned short int t = ((255 >> 3) << 11) | ((0 >> 2) << 5) | (0 >> 3);
                *((unsigned short int*)(fbp + location)) = t;
            }
        }
    }
}
#ifdef ENABLE_NX5
static int clearfb(void)
{
	VO_Enable();
	int fbfd = 0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    char *fbp = 0;

    // 打开Framebuffer设备
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("无法打开Framebuffer设备");
        return 1;
    }

    // 获取可变屏幕信息
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("无法获取可变屏幕信息");
        close(fbfd);
        return 1;
    }

    // 获取固定屏幕信息
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("无法获取固定屏幕信息");
        close(fbfd);
        return 1;
    }

    // 计算屏幕大小
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    // 映射Framebuffer到用户空间
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (fbp == MAP_FAILED) {
        perror("无法映射Framebuffer");
        close(fbfd);
        return 1;
    }

    // 清空Framebuffer
    for (long int i = 0; i < screensize; i++) {
        fbp[i] = 0;
    }
	draw_red_on_framebuffer(fbfd, vinfo, finfo, fbp);
	//sleep(5);
    // 解除映射
    if (munmap(fbp, screensize) == -1) {
        perror("无法解除映射");
    }

    // 关闭设备
    close(fbfd);

	printf("###################### clearfb success %s %s #################\n", __DATE__, __TIME__);
	//VO_Disable();
	return 0;
}
#endif //ENABLE_NX5
#endif

#ifdef ENABLE_DTLN
//测试AI声音降噪
#include "dtln_tr.h"
int mymain(int argc,char **argv);
void LoadDtlnFun(DtlnCreateHandle createfun, DtlnDestroyHandle destoryfun, DtlnTrainByHandle tranfun);
#endif

void Clear_Upgrade_File(void);

bool ExecuteCmdByShell(char*cmd);
int main(int argc, char **argv)
{
#ifdef ENABLE_AOV
	void AovVideoMutexInit();
	AovVideoMutexInit();
#endif
#ifndef NDEBUG //NDEBUG//ENABLE_DEBUG
#ifndef ENABLE_AOV
	// _vlcview(NULL, 1234);
    // return main_ctrl();
#endif
#endif
#ifdef ENABLE_SPINAAD
	//防止升级文件没有删除，上电清空一下
	remove(DOWNLOAD_IMAGE_PATH);
	Clear_Upgrade_File();

	//设置bootdelay为0，防止启动过程中误入
	ExecuteCmdByShell("fw_setenv bootdelay 0");
#endif
#ifdef ENABLE_V6
	/*
	dev:    size   erasesize  name				out(对应的升级文件)
	mtd0: 00010000 00010000 "spl"				u-boot-spl_nor_header.bin
	mtd1: 00100000 00010000 "uboot"				uboot_nor_header.bin
	mtd2: 00400000 00010000 "kernel-dtb"		zImage-dtb
	mtd3: 00200000 00010000 "rootfs"			rootfs_squashfs.img
	mtd4: 006e0000 00010000 "system"			system_squashfs.img
	mtd5: 00200000 00010000 "data"
	*/
	int V6_updata_part(char*part,char*filename);
	if(argc >= 3)
	{
		if(strcmp(argv[1], "upgrade") ==0 && argc >= 4)
		{
			V6_updata_part(argv[2], argv[3]);
		}
		else if(strcmp(argv[1], "ota") ==0)
		{
			ota_upgrade_image(argv[2]);
		}
		return 0;
	}
#endif
#ifdef ENABLE_DTLN
#if defined(ENABLE_NS_NN)
	LoadDtlnFun(NS_create_handle, NS_destroy_handle, NS_train_by_handle);
#else
	LoadDtlnFun(dtln_create_handle, dtln_destroy_handle, dtln_train_by_handle);
	//LoadDtlnFun(AEC_create_handle, AEC_destroy_handle, AEC_train_by_handle);
#endif
#endif
#ifndef NDEBUG //NDEBUG
	signal(SIGINT, signal_handler);
#ifdef ENABLE_DTLN
#if 0
	system("echo 62 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio62/direction");
    system("echo 1 > /sys/class/gpio/gpio62/value");
	_vlcview(NULL, 1234);
	mymain(argc, argv);
	sleep(1);
	_vlcview_exit();
	return 0;
#endif
#endif
#else
	if (access("/data/norun", F_OK) == 0)
	{
		printf("debug not run, %s %s\n", __DATE__, __TIME__);
		return 0;
	}

	DPBacktrace();
#endif
    LOGE("start build %s %s \n", __DATE__, __TIME__);
	#define  BUILD_DATE_FILE  "/mnt/app/buildtime"
	if (access(BUILD_DATE_FILE, F_OK) == 0)
	{
		FILE *fp = fopen(BUILD_DATE_FILE, "rb");
		if (fp)
		{
			char date[100] = {0};
			fgets(date, sizeof(date), fp);
			fclose(fp);
			LOGE("build date:%s\n", date);
		}
	}

	goblin_log_set_file_bit(LOG_TAG_BIT | LOG_TIMESTAMP_BIT | LOG_FUNCLINE_BIT);
#ifdef PLC_RUN_LOG_PATH
    goblin_log_init(PLC_RUN_LOG_PATH);
#else
	goblin_log_init("/tmp/runlog.log");
#endif
    goblin_log_set_cyclic(1);
#ifndef NDEBUG
    goblin_log_set_level(DBG_DEBUG);
#else
	goblin_log_set_level(DBG_INFO);
#endif

#if defined(ENABLE_V1)
    goblin_log_set_file_size(1 * 256 *1024);
#else
	goblin_log_set_file_size(1 * 1024 *1024);
#endif
	void goblin_plc_log_hook(GoblinLogOutput log_output);
	goblin_plc_log_hook(goblin_log_def_output);

	//LOGI("C_LIB_INFO:%s \n", C_LIB_INFO);

// #ifdef __GLIBC__
// 	#ifdef __GLIBC_PATCHLEVEL__
//     	printf("Glibc version: %d.%d.%d\n", __GLIBC__, __GLIBC_MINOR__, __GLIBC_PATCHLEVEL__);
// 	#else
// 		printf("Glibc version: %d.%d \n", __GLIBC__, __GLIBC_MINOR__);
// 	#endif
// 	//const char *version = gnu_get_libc_version();
//     //printf("Glibc version: %s\n", version);
// #endif
#if defined(ENABLE_V1) || defined(ENABLE_V6)
	if (argc > 1 && access(argv[1], F_OK) == 0)
	{
		LOGE("Ready to upgrade, file:%s\n", argv[1]);
		return ota_upgrade_image(argv[1]);
	}
	sound_play_start(RING_MP3, 1, 2, true);
#endif //#ifdef ENABLE_V1
	InitServer();
	//printf("============GetTermDev:%x\n", GetTermDev());
#if 1
#ifdef __GLIBC__
	unsigned int devn = 0;
	uint8_t isdiable = 0;
	if(argc > 1)
		devn = strtol(argv[1], NULL, 16);
	if(argc > 2)
		isdiable = strtol(argv[2], NULL, 10);
	printf("devn:%x isdiable:%d\n", devn, isdiable);
#ifdef ENABLE_V6
	devn=0x3000001;
	system("echo 62 > /sys/class/gpio/export");
	system("echo out > /sys/class/gpio/gpio62/direction");
	system("echo 1 > /sys/class/gpio/gpio62/value");
#else
	system("echo 39 > /sys/class/gpio/export");
	system("echo out > /sys/class/gpio/gpio39/direction");
	system("echo 1 > /sys/class/gpio/gpio39/value");
#endif
	//printf("devn:%x isdiable:%d\n", devn, isdiable);
#ifdef ENABLE_NX5
	LocalPlcMsg(devn, isdiable);
	clearfb();
#elif ENABLE_V6
	FH_SYS_SetReg(0x2c100094, 0x222);

	DPCreateInputEvent();
	video_service_init();
	card_service_init();
	_vlcview(NULL, 1234);
#endif //ENABLE_NX5
#else
	#define     PE0     128
	#define     PF0     160
	#define     PG0     192
	#define     PH0     224
	//LocalPlcMsg(GetTermDev());
// 	#ifdef ENABLE_MEDIA
// 	video_enc_start(GetNetWorkCardAddr("eth0"));
// #endif
	//PH11
	// system("echo 235 > /sys/class/gpio/export");
	// system("echo out > /sys/class/gpio/gpio235/direction");
	// system("echo 0 > /sys/class/gpio/gpio235/value");

	DPCreateInputEvent();
	AudioCore_SetAecConfigFile("/mnt/app/AECpara.inf");
#ifdef ENABLE_MEDIA
	video_enc_start(GetNetWorkCardAddr("eth0"));
#endif
#endif
	//mediatrans_init();
	pthread_t msg_thread = 0;
	if (0 != pthread_create(&msg_thread, NULL, msg_process_thread, NULL))
	{
		LOGE("can not start n_thread \n");
	}

#ifndef NDEBUG //NDEBUG//ENABLE_DEBUG
    return main_ctrl();
#else
#if defined(ENABLE_V1) || defined(ENABLE_V6)
	UpgradePlcContent();
#endif
	while(1)
	{
		sleep(10);
#if 0
		static int index = 0;
		static bool brunning = false;
		static void *pmgnt = NULL;
		if(!brunning){
			printf("\n\n\
					######################################\n\
					Audio tran %d, brunning:%d pmgnt:%p \n\
					######################################\n\n\n",
				++index, brunning, pmgnt);
		}
#if 0
#else
		if(!brunning){
			pmgnt = PlcAudioTrans_Start(AUDIO_PORT, 0xFFFFFFFF, AUDIO_PORT, AUDIO_TRAN_SEND_AND_RECV, 10);
			brunning = true;
		}
		else if (brunning)
		{
			PlcAudioTrans_Stop(pmgnt);
			pmgnt = NULL;
			brunning = false;
		}
#endif
#endif //0
	}
#endif //ENABLE_DEBUG

#endif

	return 0;
}
