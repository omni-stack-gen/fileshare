#ifdef __GLIBC__
    #ifdef __GLIBC_MINOR__
        #define C_LIB_INFO "Glibc version: " __STRING(__GLIBC__) "." __STRING(__GLIBC_MINOR__)
    #else
        #define C_LIB_INFO "Glibc version: " __STRING(__GLIBC__)
    #endif
#else
    #define C_LIB_INFO "Unknown C library"
#endif
#define __STRING(x) #x

#ifdef __MUSL__
    #define C_LIB_NAME "Musl"
#else
    #define C_LIB_NAME "Unknown C library"
#endif


typedef struct plc_msg_info_{
	struct list_head node;
	int seq;
	uint32_t tick;
	int type;
	int len;
	char *data;
}plc_msg_info_t;

#define MSG_LOCK() pthread_mutex_lock(&g_msg_mutex)

#define MSG_UNLOCK() pthread_mutex_unlock(&g_msg_mutex)

// static struct list_head g_msg_list = LIST_HEAD_INIT(g_msg_list);

// static int g_msg_list_count = 0;

static pthread_mutex_t g_msg_mutex = PTHREAD_MUTEX_INITIALIZER;

// static pthread_t g_check_msg_thread = 0;

// static int g_msg_monitor_start = 0;

// static pthread_mutex_t s_CallXmutexCS = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;

int mediatrans_init(void);
int mediatrans_start(void);
int VideoRecv_start(bool bthreadplay);
int VideoRecv_stop(void);
void LocalPlcMsg(uint32_t dev, uint8_t disablecache);



void HangupSession(void);
void AcceptSession(void);
void SetSessionVolume(int vol);
void SetSessiondisplay(int x, int y, int w, int h);
void QequestIdr(void);
void QequestView(uint8_t iscrop);
void OnoffAudio(uint8_t onoff);
void GetJpgdata(uint32_t timeout);
#if 0
static void *msg_check_thread(void *arg)
{
	LOGI("msg_check_thread start \n");

	while (g_msg_monitor_start)
	{
		MSG_LOCK();

		plc_msg_info_t *p = NULL;

		plc_msg_info_t *n = NULL;

		if (!list_empty_careful(&g_msg_list))
		{
			printf("msg_check_thread list_empty_careful g_msg_list_count：%d\n", g_msg_list_count);
			list_for_each_entry_safe(p, n, &g_msg_list, node)
			{
				printf("msg_check_thread p->seq:%d p->tick:%d p->type:%d p->len:%d p->data:%p\n", p->seq, p->tick, p->type, p->len, p->data);
			}
		}

		MSG_UNLOCK();

		usleep(1000 * 1000);
	}

	LOGI("thread stop \n");

	return NULL;
}

static int message_add(plc_msg_info_t *msg_info)
{
	int ret = 0;
	plc_msg_info_t *pinfo = (plc_msg_info_t *)bmalloc(sizeof(plc_msg_info_t));

	if (!pinfo)
	{
		LOGE("alloc plc_msg_info_t fail \n");
		return -1;
	}

	memset(pinfo, 0, sizeof(plc_msg_info_t));
	memcpy(pinfo, msg_info, sizeof(plc_msg_info_t));
	pinfo->tick = time_relative_ms();
	MSG_LOCK();
	list_add_tail(&pinfo->node, &g_msg_list);
	g_msg_list_count++;

	if (g_msg_monitor_start == 0)
	{
		g_msg_monitor_start = 1;

		if (0 != pthread_create(&g_check_msg_thread, NULL, msg_check_thread, NULL))
		{
			g_msg_monitor_start = 0;

			LOGE("can not start n_thread \n");
		}
	}

	MSG_UNLOCK();

	return ret;
}


static int message_deletebyseq(int seq)
{
	int ret = 0;

	plc_msg_info_t *p = NULL;

	plc_msg_info_t *n = NULL;

	MSG_LOCK();

	if (!list_empty_careful(&g_msg_list))
	{
		list_for_each_entry_safe(p, n, &g_msg_list, node)
		{
			if (p->seq == seq)
			{
				list_del(&p->node);
				if(p->data)
					bfree(p->data);
				bfree(p);

				g_msg_list_count--;

				break;
			}
		}
	}

	MSG_UNLOCK();

	return ret;
}
#endif




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

static void cmd_enc_stop(int argc, char *args[])
{
    printf("cmd_enc_stop, argc:%d\n", argc);
	// if(argc < 2)
	// 	return;
	// char *topic = args[0];
	// char *msg = args[1];
#ifdef ENABLE_MEDIA
	video_enc_stop();
#endif
}

static void cmd_enc_start(int argc, char *args[])
{
    printf("Open cmd_enc_start \n");
#ifdef ENABLE_MEDIA
	video_enc_start(GetNetWorkCardAddr("eth0"));
#endif
	// video_draw2mainstream();
	// video_draw2substream();
}

static void cmd_catch_jpeg(int argc, char *argv[])
{
#ifdef ENABLE_MEDIA
	if(argc == 0){
		printf("argc is 0 \n");
		return;
	}
	char filename[128] = {0};
	strncpy(filename, argv[0], 128);
	unsigned char *pbuf = NULL;
	unsigned int buflen = 0;
	int ret = video_catch_jpeg(0, &pbuf, &buflen);
	if(ret==0 && pbuf)
	{
		FILE *pfd = fopen(filename, "wb");
		if (pfd)
		{
			fwrite(pbuf, 1, buflen, pfd);
			fclose(pfd);
			printf("write jpge success \n");
		}
		else{
			printf("file open fail\n");
		}

		free(pbuf);
	}
	else{
		printf("catch jpeg fail, ret:%d\n", ret);
	}
#elif ENABLE_V6
	if(argc == 0){
		printf("argc name is 0 \n");
		return;
	}
	printf("Open cmd_catch_jpeg \n");
	// 这里可以调用V6的抓图函数
	FH_SINT32 sample_common_start_mjpeg(FH_VOID);
	FH_SINT32 sample_common_stop_mjpeg_gend_stream(FH_VOID);
	FH_SINT32 sample_common_start_mjpeg_gend_stream(FH_CHAR *pFileName);
	if(sample_common_start_mjpeg()==0)
	{
		sample_common_start_mjpeg_gend_stream(argv[0]);

		sample_common_stop_mjpeg_gend_stream();
	}
	else{
		printf("sample_common_start_mjpeg fail \n");
	}
#endif
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

	// 自定义结构体
	typedef struct {
		int id;
		char name[20];
		timer_t timerid;

		void *psem;
	} CustomData;
static unsigned int testindex = 0;
static void TimeThread2(union sigval v)
{
    //DPPostMessage(TIME_MESSAGE, 0, 0, 0, MSG_TIME_TYPE);
	// 获取传递的整数参数
    int int_param = v.sival_int;
    printf("[%d]接收到的整数参数: %d\n", time_relative_ms(), int_param);
	MSG_LOCK();
	//printf("############### :%u testindex:%u\n", time_relative_ms(), testindex);
	testindex = 0;
	MSG_UNLOCK();

	CustomData *p = (CustomData *)v.sival_ptr;
	printf("定时器句柄:%p  id:%d  name:%s\n", p->timerid, p->id, p->name);
	sleep(10);
	printf("--==--\n");
#if 0
	if (timer_delete((timer_t)p->timerid) == 0) {
		printf("定时器删除成功\n");
	} else {
		perror("定时器删除失败");
	}

	if(p->psem)
		SetSemaphore(p->psem);
	free(p);
#endif
}

#define ONE_SECOND		(1000)
void DPCreateTimeEvent2(void *psem)
{
    timer_t timerid;
    struct sigevent evp;
    memset(&evp, 0, sizeof(struct sigevent));

    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = TimeThread2;




	// 初始化自定义结构体
    CustomData *custom_data = (CustomData*)malloc(sizeof(CustomData));//{123, "example"};
	memset(custom_data, 0, sizeof(CustomData));
	 // 设置自定义参数
    evp.sigev_value.sival_int = 456;  // 整数参数
    evp.sigev_value.sival_ptr = custom_data;  // 结构体指针参数


    int ret = timer_create(CLOCK_MONOTONIC, &evp, &timerid);

	custom_data->timerid = timerid;
	custom_data->psem = psem;
	printf("int: %d ptr: %p  timerid:%p\n", evp.sigev_value.sival_int, evp.sigev_value.sival_ptr, timerid);
    if(ret)
        printf("CreateTimeEvent fail \n");
    else
    {
        struct itimerspec ts;

		ts.it_interval.tv_sec =  ONE_SECOND / 1000;
		ts.it_interval.tv_nsec = (ONE_SECOND % 1000) * 1000000;
		ts.it_value.tv_sec = ONE_SECOND / 1000;
		ts.it_value.tv_nsec = (ONE_SECOND % 1000) * 1000000;

		printf("[%d]------------>>>>> run time tick \n", time_relative_ms());
		ret = timer_settime(timerid, 0, &ts, NULL);

        if(ret)
            printf("timer_settime fail \n");
    }

//sleep(1);
}


////==============================================


static pthread_t g_spi_thread = 0;

static int g_spi_start = 0;
#pragma pack(1)

typedef struct {
	unsigned int seq;
	char data[2007];
} test_pack;

typedef struct {
	unsigned int head;		//分片序号
	unsigned int dstdev;
	unsigned int dstport;
	unsigned int datalen;
	test_pack pack;
} ST_TransPacket;
#pragma pack(0)

static void on_recv_data(void *param, uint32_t dev, uint8_t *data, uint16_t data_size)
{
	static int index_seq = 0;
	ST_TransPacket *pack = (ST_TransPacket *)data;
	if((index_seq+1) != pack->pack.seq)
		printf("test from:%x seq:%d but last seq:%d\n", dev, pack->pack.seq, index_seq);

	index_seq = pack->pack.seq;
}

static void *Spi_send_thread(void *arg)
{
	#define TEST_PORT 0x99
	spi_plc_mgnt_register_callback(TEST_PORT, on_recv_data, NULL);
	uint32_t remidedev = (uint32_t)arg;
	LOGI("Spi_send_thread start , remidedev:%x\n", remidedev);
	uint8_t buf[2048] = {0};
	int index = 0;
	while (g_spi_start)
	{
		//index++;
		ST_TransPacket *pack = (ST_TransPacket *)buf;
		pack->head = 0x1234abcd;
		pack->dstdev = remidedev;
		pack->dstport = TEST_PORT;
		pack->datalen = sizeof(test_pack);

		//for (size_t i = 0; i < 400; i++)
		{
			index++;
			pack->pack.seq = index;
			spi_plc_mgnt_send(remidedev, TEST_PORT, buf, sizeof(ST_TransPacket));
		}
#ifdef ENABLE_V6
		usleep(10*1000);
#else
		DPSleep(10);
#endif
		printf("test send seq:%d \n", pack->pack.seq);
	}
	spi_plc_mgnt_unregister_callback(TEST_PORT);
	LOGI("########## out #########\n");
	return 0;
}

#include <md5-helper.h>
void Md5_File(char *pfilename, char *pOutBuf)
{
	unsigned char buf[16] = {0};
	int i;
	md5sum(pfilename, buf);

	for (i = 0; i < 16; i++)
		sprintf(pOutBuf + 2 * i, "%02x", buf[i]);

	pOutBuf[32] = 0;
}

static void cmd_test(int argc, char *argv[])
{
	if (strcmp(argv[0], "drop") == 0)
	{
		system("echo 3 > /proc/sys/vm/drop_caches");
	}
	else if (strcmp(argv[0], "upgrade") == 0 && argc==2)
	{
		// int ret = goblin_plc_upgrade_firmware(argv[1]);
		// printf("upgrade ret:%d\n", ret);
#ifdef ENABLE_V1
		plc_service_deinit();
		printf("upgrade reboot start111\n");
		sleep(1);
		plc_service_init(PLC_DEV_DEFAULT_ADDR - 1);
#endif
		printf("upgrade reboot end\n");
		return ;
	}
	else if (strcmp(argv[0], "ota") == 0 && argc==2)
	{
#ifdef ENABLE_V6
		int ret = ota_upgrade_image(argv[1]);
		printf("upgrade ret:%d\n", ret);
		printf("upgrade reboot end\n");
#endif
		return ;
	}
	else if (strcmp(argv[0], "rssi") == 0)
	{
		int ret = goblin_plc_get_rssi();
		printf("rssi end ret:%d\n", ret);
		return ;
	}
	else if(strcmp(argv[0], "test")==0)
	{
		static uint32_t remidedev = 0;

		/* code */
		printf("test spi trans \n");
		if(!g_spi_start && argc==2)
		{
			remidedev = strtol(argv[1], NULL, 16);
			g_spi_start = 1;
			if (0 != pthread_create(&g_spi_thread, NULL, Spi_send_thread, (void*)remidedev))
			{
				g_spi_start = 0;

				LOGE("can not start n_thread \n");
			}

			LOGW("Spi trans test start\n");
		}
		else if(g_spi_start==1)
		{
			g_spi_start = 0;
			pthread_join(g_spi_thread, NULL);
			LOGW("Spi trans test stop\n");
		}

		return ;
	}
	else if (strcmp(argv[0],"call") == 0)
	{
		/* code */

		return;
	}
	else if (strcmp(argv[0], "brg") == 0)
	{
		/* code */
		void video_setbr(int value);
		int value = strtol(argv[1], NULL, 10);
		printf("============= value:%d \n", value);
#ifdef ENABLE_MEDIA
		video_setbr(value);
#endif
		return;
	}
	else if (strcmp(argv[0], "wdr") == 0)
	{
		void video_wdr(int value);
		int value = strtol(argv[1], NULL, 10);
		printf("=============wdr value:%d \n", value);
#ifdef ENABLE_MEDIA
		video_wdr(value);
#endif
		return;
	}
	else if (strcmp(argv[0], "color") == 0)
	{
		/* code */
		void SetVideoPlayColorByParam(int Brightness, int Contrast, int Saturation);
		int Brightness = strtol(argv[1], NULL, 10);
		int Contrast = strtol(argv[2], NULL, 10);
		int Saturation = strtol(argv[3], NULL, 10);

		printf("Brightness:%x Contrast:%x  Saturation:%x\n", Brightness, Contrast, Saturation);
		SetVideoPlayColorByParam(Brightness, Contrast, Saturation);
		return;
	}
	else if (strcmp(argv[0], "dec") == 0)
	{
		/* code */
		void video_test(void);
		video_test();
		return;
	}
	else if (strcmp(argv[0], "icut")==0)
	{
		/* code */
		if (argc == 2)
		{
			if(strtol(argv[1], NULL, 10)==1)
				system("echo 1 > /sys/class/gpio/gpio235/value");
			else
				system("echo 0 > /sys/class/gpio/gpio235/value");
		}
		else
		{
			printf("param number is err:%d \n", argc);
		}
		return;
	}
	else if (strcmp(argv[0], "unlock")==0)
	{
		/* code */
		uint32_t remotedev = strtoll(argv[1], NULL, 16);
		unsigned int timeout = strtol(argv[2], NULL, 10);
		uint32_t ltick = time_relative_ms();
		LOGI("send dev:%x  timeout:%u\n", remotedev, timeout);
		int ret = goblin_plc_Unlock(remotedev, 1, timeout);
		LOGI("unlock end , ret:%d, lost time:%u\n", ret, time_relative_ms()-ltick);
		return ;
	}
	else if (strcmp(argv[0], "search")==0 && argc==4)
	{
		/* code */
		uint32_t remotedev = strtoll(argv[1], NULL, 16);
		unsigned int type = strtol(argv[2], NULL, 10);
		unsigned int roomid = strtol(argv[3], NULL, 10);
		uint32_t ltick = time_relative_ms();
		LOGI("send search dev:%x  type:%d roomid:%d\n", remotedev, type, roomid);
		int ret = goblin_plc_Search(remotedev, type, roomid);
		LOGI("search end , ret:%d, lost time:%u\n", ret, time_relative_ms()-ltick);
		return ;
	}
	else if (strcmp(argv[0], "wakeup")==0 && argc==3)
	{
		/* code */
		uint32_t remotedev = strtoll(argv[1], NULL, 16);
		unsigned int roomid = strtol(argv[2], NULL, 10);
		uint32_t ltick = time_relative_ms();
		LOGI("send wakeup dev:%x   roomid:%d\n", remotedev, roomid);
		int ret = goblin_plc_wakeup_dev_by_roomid(remotedev, roomid, 1000);
		LOGI("wakeup end , ret:%d, lost time:%u\n", ret, time_relative_ms()-ltick);
		return ;
	}
	else if (strcmp(argv[0], "setid") ==0 && argc==3)
	{
		/* code */
		//uint32_t remotedev = strtoll(argv[1], NULL, 16);
		//LOGI("send setid dev:%x  ID:%s   result:%d\n", remotedev, argv[2], goblin_plc_set_remote_id(remotedev, argv[2], 1000));
		return;
	}
	else if (strcmp(argv[0], "getcfg") ==0 && argc==2)
	{
		/* code */
		uint32_t remotedev = strtoll(argv[1], NULL, 16);
		LOGI("send getcfg dev:%x  ID:%s   r\n", remotedev, argv[2]);
		uint32_t ltick = time_relative_ms();
		goblin_door_cfg_t doorcfg = {0};
		int ret = goblin_plc_get_door_cfg(remotedev, &doorcfg, 1000);
		printf("ret:%d  losttime:%d\n", ret, time_relative_ms()-ltick);

		printf("获取门口机配置: 移动侦测灵敏度:%d  呼叫铃声:%d  通话音量:%d\n", doorcfg.motion, doorcfg.soundcall, doorcfg.soundtalk);
		for (int i = 0; i < DOOR_LOCK_COUNT; i++)
		{
			printf("锁ID:%d 开锁电平:%d 开锁延时:%d 门磁开关:%d 门磁延时:%d \n", doorcfg.doorlock[i].lockid, doorcfg.doorlock[i].lockswitch, doorcfg.doorlock[i].locktime,doorcfg.doorlock[i].menicswitch, doorcfg.doorlock[i].menictime);
		}
		return;
	}
	else if (strcmp(argv[0], "setcfg") ==0 && argc==2)
	{
		/* code */
		uint32_t remotedev = strtoll(argv[1], NULL, 16);
		goblin_door_cfg_t doorcfg = {0};
		doorcfg.motion = 1;
		for (size_t i = 0; i < DOOR_LOCK_COUNT; i++)
		{
			doorcfg.doorlock[i].lockid = i+1;
			doorcfg.doorlock[i].lockswitch = 1;
			doorcfg.doorlock[i].locktime = 3+i;
			doorcfg.doorlock[i].menicswitch = 1;
			doorcfg.doorlock[i].menictime = 20+i;
		}

		doorcfg.soundcall = 50;
		doorcfg.soundtalk = 60;
		LOGI("send setid dev:%x    result:%d\n", remotedev,  goblin_plc_set_door_cfg(remotedev, &doorcfg, 1000));
		return;
	}
	else if (strcmp(argv[0], "ring") ==0)
	{
		void testring(void);
		 testring();
		 return ;
	}
	else if (strcmp(argv[0], "close") ==0)
	{
		/* code */
		int ret = goblin_plc_close();
		printf("=========== plc close =================, ret:%d\n", ret);
		return;
	}
	else if (strcmp(argv[0], "sendfile") ==0 && argc==3)
	{
		uint32_t remotedev = strtoll(argv[1], NULL, 16);
		char *pfile = argv[2];
		LOGI("send file dev:%x  file:%s\n", remotedev, pfile);

		file_pack_t filedata = {0};
		int seq = 1;
		//filedata.fileseq = seq++;
		FILE *fd = NULL;
		if (access(pfile, F_OK) == 0 && (fd = fopen(pfile, "rb")))
		{
			int len;
			char *pdata = NULL;

			fseek(fd, 0, SEEK_END);
			len = ftell(fd);
			fseek(fd, 0, SEEK_SET);

			pdata = (char *)malloc(len);
			fread(pdata, 1, len, fd);
			fclose(fd);

			char strmd5[64] = {0};
			Md5_File(pfile, strmd5);
			printf("============file len:%d md5: %s\n", len, strmd5);

			filedata.file_type = 1;
			memcpy(filedata.filemd5, strmd5, 32);
			filedata.filelen = len;

			int filereslen = len;
			int sendlen = 0;
			int sendfilelen = 0;
			int ret = 0;

			uint32_t ltick = time_relative_ms();
			uint32_t lasttick = time_relative_ms();
			do
			{
				filedata.fileflag = 0;
				if(seq==1)
					filedata.fileflag |= FILE_START_FLAG;
				if(filereslen > FILE_PACK_SIZE)
				{
					sendlen = FILE_PACK_SIZE;
					filereslen -= FILE_PACK_SIZE;
				}
				else{
					sendlen = filereslen;
					filereslen = 0;
					filedata.fileflag |= FILE_END_FLAG;
				}
				filedata.fileseq = seq++;
				filedata.datalen = sendlen;
				memcpy(filedata.data, pdata+sendfilelen, sendlen);

				ltick = time_relative_ms();
				ret = goblin_plc_send_file_data(remotedev, &filedata, 1000);
				printf("send file data ret:%d seq:%d len:%d lost time:%u\n 0x%02x 0x%02x  len:%d\n", ret, filedata.fileseq, sendlen, time_relative_ms()-ltick,
									filedata.data[sendlen-2], filedata.data[sendlen-1], filereslen);
				sendfilelen+=sendlen;
			} while (filereslen);

			printf("send file end, filelen:%d lost time:%u\n", len, time_relative_ms()-lasttick);
			free(pdata);
		}

		return;
	}
	else if (strcmp(argv[0], "agent") ==0)
	{
		/* code */
		//goblin_plc_close();
		goblin_agent_info_s ininfo = {0};
 		strncpy(ininfo.strtimezone, "+05:00", 32);
		goblin_agent_ack_s outinfo = {0};
		int ret = goblin_plc_check_agent(0x1000101, &ininfo, &outinfo, 1000);
		printf("=========ret:%d \n", ret);
		return;
	}
	else if (strcmp(argv[0], "htest") ==0)
	{
		/* code */
		int cmd = strtol(argv[1], NULL, 10);
		golbin_hardware_test_s testinfo = {0};
		testinfo.operate_cmd = cmd;
		int ret = goblin_plc_hardware_test(0x3000001, &testinfo, 3000);
		printf("=========htest ret:%d \n", ret);
		return;
	}
	else if (strcmp(argv[0], "loglen") ==0)
	{
		/* code */
		int ret = goblin_plc_getlogfilesize(0x3000001, 3000);
		printf("=========loglen ret:%d \n", ret);
		return;
	}
	else if (strcmp(argv[0], "loginfo") ==0)
	{
		/* code */
		int ret = goblin_plc_getlogfile(0x3000001, 3000);
		printf("=========loginfo ret:%d \n", ret);
		return;
	}

	int scount = strtol(argv[0], NULL, 10);
	printf("scount:%d\n", scount);
	void *psem = CreateSemaphore(scount);
	printf("psem:%p\n", psem);
	// if (psem)
	// {
	// 	int index = 0;
	// 	do
	// 	{
	// 		if(GetSemaphore(psem, 1000))
	// 		{
	// 			printf("get sem success, index:%d\n", ++index);
	// 		}
	// 		else
	// 		{
	// 			printf("wait sem timeout\n");
	// 			break;
	// 		}
	// 	} while (1);

	// 	DestorySemaphore(psem);
	// }

	// typedef struct sem_test_
	// {
	// 	void *psem;
	// }sem_test_s;


	//uint32_t lasttick = time_relative_ms();
	// sem_test_s *parray = (sem_test_s*)malloc(scount*sizeof(sem_test_s));
	// memset(parray, 0, scount*sizeof(sem_test_s));
	// printf("sem_test_s size is %d \n",sizeof(sem_test_s));
	// for (size_t i = 0; i < scount; i++)
	// {
	// 	parray[i].psem = CreateSemaphore(scount);

	// 	//printf("<%03d>psemw:%p\n", i, psemw);
	// }

	// printf("sem create:%d, lost time:%u \n", scount, time_relative_ms()-lasttick);

#if 0
	int index = 0;
	do
	{
		index++;
		if(index>10)
			break;

		lasttick = time_relative_ms();
		DPCreateTimeEvent(psem);
		//sleep(2);
		bool bret = GetSemaphore(psem, 1500);
		printf(">>>>>>>>>>. index:%d bret:%d lost time:%d\n\n\n\n", index, bret, time_relative_ms()-lasttick);
	} while (1);
#else
	//DPCreateTimeEvent2(NULL);
#endif
	printf("############# out ##############\n");
	//lasttick = time_relative_ms();
	// do
	// {
	// 	MSG_LOCK();
	// 	testindex++;
	// 	MSG_UNLOCK();
	// 	/* code */
	// 	// if(time_relative_ms()-lasttick>=1000)
	// 	// 	break;
	// } while (1);

	//printf("lostonetime %d \n", testindex);
}

static void cmd_requestIdr(int argc, char *argv[])
{
#ifdef ENABLE_MEDIA
	int channel = strtol(argv[0], NULL, 10);
	printf("Request send data to channel:%d\n", channel);
	video_idr(channel);
#elif defined(ENABLE_V6)
	FH_VENC_RequestIDR(0);
#endif
}

static void cmd_send(int argc, char *argv[])
{
	//static void *ptran = NULL;
	mediatrans_init();

	mediatrans_start();
}




static void cmd_recv(int argc, char *argv[])
{
	uint32_t onoff = strtoll(argv[0], NULL, 16);
	if(onoff == 1)
		VideoRecv_start(strtoll(argv[1], NULL, 10)==1);
	else
		VideoRecv_stop();
}

static void cmd_send_spi(int argc, char *argv[])
{
	uint32_t dev = strtoll(argv[0], NULL, 16);
	uint8_t port = strtol(argv[1], NULL, 10);

	uint8_t data[1024] = {0};
	strncpy((char*)data, argv[2], 1023);
	uint16_t len = strlen((char*)data);

	printf("dev:%x port:%d data %d: %s \n", dev, port, len, data);
	spi_plc_mgnt_send(dev, port, data, len);
}

static void cmd_video_crop(int argc, char *argv[])
{
	void video_crop(int x, int y, int w, int h);
#ifdef ENABLE_MEDIA
	int x = strtol(argv[0], NULL, 10);
	int y = strtol(argv[1], NULL, 10);
	int w = strtol(argv[2], NULL, 10);
	int h = strtol(argv[3], NULL, 10);
	printf("================ x:%u y:%u w:%u h:%u\n", x, y, w, h);
	video_crop(x, y, w, h);
#endif
}

static void cmd_audio(int argc, char *argv[])
{
#ifdef ENABLE_V6
	void fh_audio_test(int type, int volume);
	if(strcmp("test", argv[0]) == 0)
	{
		int volume = 999;
		if(argc > 2)
			volume = strtol(argv[2], NULL, 16);
		printf("Audio test, type:%s volume:0x%x\n", argv[1], volume);
		fh_audio_test(atol(argv[1]), volume);
		return;
	}
#endif
	void Audio_test_Record_start(void);
	void Audio_test_Record_stop(void);
	void *Audio_test_Play_start(void);
	void Audio_test_Play_stop(void*pmgnt);
	if(strcmp("in", argv[0]) == 0)
	{
		int onff = strtol(argv[1], NULL, 10);
		printf("Audio in test, onff:%d\n", onff);
		if(onff == 1)
			Audio_test_Record_start();
		else
			Audio_test_Record_stop();
	}
	else if (strcmp("out", argv[0]) ==0)
	{
		static void *pmgnt = NULL;
		int onff = strtol(argv[1], NULL, 10);
		printf("Audio out test, onff:%d\n", onff);
		if(onff == 1 && pmgnt == NULL)
			pmgnt = Audio_test_Play_start();
		else if (onff == 0 && pmgnt!= NULL)
		{
			Audio_test_Play_stop(pmgnt);
			pmgnt = NULL;
		}
	}
	else if (strcmp("tran", argv[0]) ==0)
	{
		/* code */
		static void *pmgnt = NULL;
		int onff = strtol(argv[1], NULL, 10);
		printf("Audio tran test, onff:%d\n", onff);
		if(onff == 1 && pmgnt == NULL)
			pmgnt = PlcAudioTrans_Start(AUDIO_PORT, 0xFFFFFFFF, AUDIO_PORT, AUDIO_TRAN_SEND_AND_RECV, 10);
		else if (onff == 0 && pmgnt!= NULL)
		{
			PlcAudioTrans_Stop(pmgnt);
			pmgnt = NULL;
		}
	}
}

static void cmd_video(int argc, char *argv[])
{
	void VideoTransTest(int onoff);

	int onff = strtol(argv[0], NULL, 10);
	printf("Video tran test, onff:%d\n", onff);
#ifdef ENABLE_V1
	VideoTransTest(onff);
#elif defined(ENABLE_V6)
	if(onff > 1)
	{
		for (int i = 0; i < onff; i++)
		{
			printf("################# i:%d \n", i);
			_vlcview_exit();
			sleep(1);
			_vlcview(NULL, 1234);
			sleep(3);
		}

		_vlcview_exit();
		return;
	}
	if (onff == 1)
	{
		_vlcview(NULL, 1234);
	}
	else
	{
		_vlcview_exit();
	}
#endif
}


static void cmd_videoReload(int argc, char *argv[])
{

	int state = strtol(argv[0], NULL, 10);
	printf("Video reload, state:%d\n", state);
#ifdef ENABLE_V1

#elif defined(ENABLE_V6)
	video_service_push_event(VIDEO_ENC_RELOAD, state);
#endif
}


static void cmd_call(int argc, char *argv[])
{
	#define MAX_SESSION_NUM 2
	static void *psessionarray[MAX_SESSION_NUM] = {0};
	uint32_t dev = strtoll(argv[1], NULL, 16);
	if (strcmp("out", argv[0]) ==0)
	{
		for(int i=0; i<MAX_SESSION_NUM; i++){
			if(psessionarray[i] == NULL){
				psessionarray[i] = goblin_call_by_dev(dev);
				printf("<%d>call out session:%p \n", i, psessionarray[i]);
				break;
			}
		}
	}
	else if(strcmp("hangup", argv[0]) ==0)
	{
		if(dev < MAX_SESSION_NUM && psessionarray[dev]){
			printf("hangup one session:%p \n", psessionarray[dev]);
			goblin_hangup_by_session(psessionarray[dev]);
			psessionarray[dev] = NULL;
		}
		else{
			for(int i=0; i<MAX_SESSION_NUM; i++){
				if(psessionarray[i]){
					printf("<%d>hangup session:%p \n", i, psessionarray[i]);
					goblin_hangup_by_session(psessionarray[i]);
					psessionarray[i] = NULL;
				}
			}

			void FinishRecvVideo(void);
			FinishRecvVideo();
		}

	}
	else if(strcmp("accept", argv[0]) ==0)
	{
		AcceptSession();
	}
	else if(strcmp("hungup", argv[0]) ==0)
	{
		HangupSession();
	}
	else if (argc==2 && strcmp("vol", argv[0]) ==0)
	{
		SetSessionVolume(strtol(argv[1], NULL, 10));
	}
	else if (argc==5 && strcmp("display", argv[0]) ==0)
	{
		SetSessiondisplay(strtol(argv[1], NULL, 10), strtol(argv[2], NULL, 10), strtol(argv[3], NULL, 10), strtol(argv[4], NULL, 10));
	}
	else if (strcmp("ikey", argv[0]) ==0)
	{
		QequestIdr();
	}
	else if (argc==2 && strcmp("crop", argv[0]) ==0)
	{
		QequestView(strtol(argv[1], NULL, 10));
	}
	else if (argc==2 && strcmp("audio", argv[0]) ==0)
	{
		if(psessionarray[0])
			goblin_tran_auido_by_session(psessionarray[0], strtol(argv[1], NULL, 10));
		else
			OnoffAudio(strtol(argv[1], NULL, 10));
	}
	else if (argc==2 && strcmp("jpg", argv[0]) ==0)
	{
		GetJpgdata(strtol(argv[1], NULL, 10));
	}

}

static void cmd_dtlm(int argc, char *argv[])
{
#ifdef ENABLE_V6
	int mymain(int argc,char **argv);
	//mymain(argc, argv);
	int goto_aov_sleep(void);
	void ReLoadDayNightParam(unsigned int isDay);
	if(strcmp("sleep", argv[0]) ==0){
		goto_aov_sleep();
	}
	else if (strcmp("day", argv[0]) ==0)
	{
		ReLoadDayNightParam(1);
		uint32_t ltick = time_relative_ms();
		//重新加载视频编码
#if 0
		_vlcview_exit();
		usleep(1000*100);
		_vlcview(NULL, 1234);
#else
		Isp_run_type(0);
		usleep(1000*50);
		Isp_run_type(1);
#endif
		LOGI("ReloadVideoEnc lost time:%d\n", time_relative_ms() - ltick);
	}
	else if (strcmp("night", argv[0]) ==0)
	{
		ReLoadDayNightParam(0);
		uint32_t ltick = time_relative_ms();
#if 0
		_vlcview_exit();
		usleep(1000*100);
		_vlcview(NULL, 1234);
#else
		//重新加载视频编码
		Isp_run_type(0);
		usleep(1000*50);
		Isp_run_type(1);
#endif
		LOGI("ReloadVideoEnc lost time:%d\n", time_relative_ms() - ltick);
	}
	else if (strcmp("save", argv[0]) ==0){
		void set_video_frame_save_flag(bool flag);
		set_video_frame_save_flag(true);
	}
	else{
		printf("dtlm test error, %s\n", argv[0]);
	}
#endif
}

//执行的命令参数
cmd_table_t cmd_table[] =
{
	//初始化mqtt服务
    {"enc_start", "enc start", 0, 3, cmd_enc_start, ""},
    {"enc_stop", "enc send", 0, 6, cmd_enc_stop, ""},
	{"cmd",		"cmd_system", 0, 10, cmd_system, ""},
	{"jpeg",	"cmd_catch_jpeg", 1, 2, cmd_catch_jpeg, ""},
	{"test",	"try one try", 1, 10, cmd_test, ""},
	{"IDR",		"RequestIDR", 1, 10, cmd_requestIdr, ""},
	{"crop",	"video crop",  4, 10, cmd_video_crop, ""},

	{"send",	"send data to", 0, 0, cmd_send, ""},
	{"recv",    "recv data from", 2, 10, cmd_recv, ""},
	{"senddata", "send plc data", 3, 10, cmd_send_spi, ""},
	{"audio",	 "audio in or out", 2, 3, cmd_audio, ""},
	{"video",	 "video in or out", 1, 3, cmd_video, ""},
	{"videoreload", "video reload", 1, 10, cmd_videoReload, "0:unload 1:load"},
	{"call",	 "call in or out", 2, 10, cmd_call, ""},
	{"dtlm",	 "dtlm test", 1, 10, cmd_dtlm, ""},

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

	spi_plc_mgnt_deinit();
#ifdef ENABLE_MEDIA
	video_enc_stop();
#endif
#ifdef ENABLE_V6
		_vlcview_exit();
#endif //ENABLE_V6
    return 0;
}

#ifndef NDEBUG
static void signal_handler(int sig)
{
	static int terminated = 0;

	if (!terminated)
	{
		terminated = 1;

		spi_plc_mgnt_deinit();
#ifdef ENABLE_MEDIA
	video_enc_stop();
#endif
		LOGI("Ctrl-C pressed, cleaning up and exiting..\n");
#ifdef ENABLE_V6
		_vlcview_exit();
#endif //ENABLE_V6
		exit(EXIT_SUCCESS);
	}
}
#endif //#ifdef NDEBUG