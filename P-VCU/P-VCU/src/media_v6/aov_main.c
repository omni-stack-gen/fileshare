#include "fh_common.h"
#include "sample_common.h"
#include "aov_xbus.h"
#include "fh_system_mpi.h"
#include "aov_demo.h"
#include "types/vmm_api.h"
#include <stdbool.h>
#include "utils/time_helper.h"
#include <sys/sys_event.h>

#include "video_service.h"
#include "hard_node_service.h"

#define ENC_JPG_WIDTH  800//320 //640  800
#define ENC_JPG_HEIGHT 640 //352  640
#define JPG_CHN 2

#define RTT_ENC_JPG
//#define LINUX_ENC_JPG
static pthread_t g_tAovThread;
static sem_t g_aov_sem;
//获取码流和进入休眠要锁起来
pthread_mutex_t g_aov_mutex;
static bool baov_mutex_inited = false;
static int exitflag = 0;


// static unsigned long long wakeup_time = 0;

// int g_aov_sleep_flag = 0;       //aov  启动则停止 API_ISP_RUN、NN，以防和AOV那边起冲突
int g_aov_resume_reason = 0;       //aov唤醒原因
// static unsigned long long fh_sys_get_timeus(void)
// {
//     struct timeval tv;
//     unsigned long long us;

//     gettimeofday(&tv, NULL);
//     us = (unsigned long long)tv.tv_sec * 1000000 + (unsigned long long)tv.tv_usec;
//     return us;
// }

void AovVideoMutexInit()
{
	if(!baov_mutex_inited){
		pthread_mutex_init(&g_aov_mutex, NULL);
		baov_mutex_inited = 1;
	}
}

void AovVideoMutexLock(bool block)
{

	if(block){
		pthread_mutex_lock(&g_aov_mutex);
	}
	else{
		pthread_mutex_unlock(&g_aov_mutex);
	}
}

int linux_resume_view(void)
{
    return (g_aov_resume_reason & AOV_WAKEUP_USER || g_aov_resume_reason == 0);
}

int linux_video_ready(void)
{
    AOV_XBUS_CMD_S aov_cmd;

    memset(&aov_cmd, 0, sizeof(aov_cmd));
    aov_cmd.cmd = AOV_CMD_VIDEO_READY;
    aov_cmd.data.video.AovChnNum = 1;
    aov_cmd.data.video.md_enable = 0;
    aov_cmd.data.video.nn_enable = 0;
    aov_cmd.data.video.osd_enable = 0;
//#if defined (FH_JPEG_ENABLE)
// #if defined (RTT_ENC_JPG)
    aov_cmd.data.video.jpeg_enable = 1;
// #endif
// #if defined(FH_NN_ENABLE)
    aov_cmd.data.video.nn_enable = 1;
// #endif
#if defined(FH_USING_OSD)
    aov_cmd.data.video.osd_enable = 1;
#endif
#if defined(FH_USING_MD)
    aov_cmd.data.video.md_enable = 1;
#endif

	return FH_RPC_Customer_Command(AOV_XBUS_COMMAND, &aov_cmd, sizeof(aov_cmd));
}

int linux_aov_config(int grpid, int enable, int interval_ms, int fream_num)
{
    AOV_XBUS_CMD_S aov_cmd;

    memset(&aov_cmd, 0, sizeof(aov_cmd));
	//jpg cfg
	aov_cmd.cmd = AOV_CMD_SET_JPG_PARAM;
    aov_cmd.grpid = grpid;
	aov_cmd.data.jpgcfg.chn = JPG_CHN;
	aov_cmd.data.jpgcfg.size.u32Width = ENC_JPG_WIDTH;
	aov_cmd.data.jpgcfg.size.u32Height = ENC_JPG_HEIGHT;
	aov_cmd.data.jpgcfg.qp = 30;//(0~98)越小质量越好
	if(FH_RPC_Customer_Command(AOV_XBUS_COMMAND, &aov_cmd, sizeof(aov_cmd)) != 0)
	{
		printf("set jpg param fail\n");
		return -1;
	}

    memset(&aov_cmd, 0, sizeof(aov_cmd));
    aov_cmd.cmd = AOV_CMD_START_AOV;
    aov_cmd.grpid = grpid;
    aov_cmd.data.aov.aov_enable = enable;
    aov_cmd.data.aov.aov_interval = interval_ms;
    aov_cmd.data.aov.fream_num = fream_num;  //帧号好计数一些，时间不好控制
    return FH_RPC_Customer_Command(AOV_XBUS_COMMAND, &aov_cmd, sizeof(aov_cmd));
}

extern FH_SINT32 sample_npu_test_start();

#ifdef RTT_ENC_JPG
int savejpgstream(void)
{
	AOV_XBUS_CMD_S aov_cmd;
	int ret;
	FH_VOID *virAddr = NULL;
	FILE*fp = NULL;
	char filename[32];
	static int i = 0;

    memset(&aov_cmd, 0, sizeof(aov_cmd));
    aov_cmd.cmd = AOV_CMD_SAVE_JPG_FRAME;
    ret =  FH_RPC_Customer_Command(AOV_XBUS_COMMAND, &aov_cmd, sizeof(aov_cmd));
	if(ret < 0)
	{
		printf("FH_RPC_Customer_Command AOV_CMD_SAVE_JPG_FRAME fail\n");
		return ret;
	}
	printf("aov_cmd.data.buf.stream_addr 0x%p aov_cmd.data.buf.stream_size %d\n",aov_cmd.data.buf.stream_addr,aov_cmd.data.buf.stream_size);
	virAddr = FH_SYS_Mmap((FH_UINT32)aov_cmd.data.buf.stream_addr,aov_cmd.data.buf.stream_size);

	if(virAddr)
	{
		sprintf(filename,"jpg/%d.jpg",i);
		fp = fopen(filename, "wb");
		if(fp == NULL){
			printf("open savejpg.jpg failed\n");
			return -1;
		}

		fwrite(virAddr, 1, aov_cmd.data.buf.stream_size, fp);
		fclose(fp);
		i++;
	}
	FH_SYS_Munmap(aov_cmd.data.buf.stream_addr,aov_cmd.data.buf.stream_size);
	return 0;
}
#endif

#ifdef LINUX_ENC_JPG
static int savejpgstream(void)
{
	FH_VENC_STREAM jpeg_stream;
	int ret;
	FILE*fp = NULL;
	char filename[32];
	static int i = 0;
	FH_VENC_CHN_CAP cfg_jpegmem;
	FH_BIND_INFO jpeg_src, jpeg_dst;

	// 创建jpge通道
	cfg_jpegmem.support_type = FH_JPEG;
	cfg_jpegmem.max_size.u32Width = ENC_WIDTH;
	cfg_jpegmem.max_size.u32Height = ENC_HEIGHT;

	ret = FH_VENC_CreateChn(FH_GRP_NUM, &cfg_jpegmem);
	if (ret)
	{
	   printf("Error(%d - %x): jpeg FH_VENC_CreateChn\n", ret, ret);
	   return ret;
	}

	jpeg_src.obj_id = FH_OBJ_VPU_VO;
	jpeg_src.dev_id = 0;
	jpeg_src.chn_id = 0;
	jpeg_dst.obj_id = FH_OBJ_JPEG;
	jpeg_dst.dev_id = 0;
	jpeg_dst.chn_id = FH_GRP_NUM;
    ret = FH_SYS_Bind(jpeg_src, jpeg_dst);
	if (ret)
    {
        printf("Error(%d - %x):  VPU to ENC FH_SYS_Bind faild\n", ret, ret);
        goto fail0;
    }

	ret = FH_VENC_StartRecvPic(FH_GRP_NUM);
    if (ret != 0)
    {
        printf("Error(%d - %x): jpeg FH_VENC_StartRecvPic (chn):(%d)!\n", ret, ret, FH_GRP_NUM);
        goto fail1;
    }

    ret = FH_VENC_GetChnStream_Timeout(FH_GRP_NUM, &jpeg_stream, 1000);
    if (!ret)
    {
    	sprintf(filename,"jpg/%d.jpg",i);
		fp = fopen(filename, "wb");
		if(fp == NULL)
		{
			printf("open savejpg.jpg failed\n");
			goto fail2;
		}


		fwrite(jpeg_stream.jpeg_stream.start, 1, jpeg_stream.jpeg_stream.length, fp);
		fclose(fp);

		FH_VENC_ReleaseStream(&jpeg_stream);

		printf("\njpeg_stream.jpeg_stream.length = %d,addr 0x%p\n", jpeg_stream.jpeg_stream.length,(unsigned int)jpeg_stream.jpeg_stream.start);
		i++;
    }

fail2:
	FH_VENC_StopRecvPic(FH_GRP_NUM);
fail1:
	FH_SYS_UnBind(jpeg_src, jpeg_dst);
fail0:
	FH_VENC_DestroyChn(FH_GRP_NUM);
	return ret;
}

#endif

//#else
#if 0
//这种方式不行，nn里面获取的frame，不能通过FH_JPEGE_SendFrame送去编码jpg
static int savejpgstream(void)
{
	FH_VENC_STREAM jpeg_stream;
	int ret;
	FILE*fp = NULL;
	char filename[32];
	static int i = 0;
	JPEGE_RECV_PIC_PARAM_S jpeg_parm;
	JPEGE_CHN_STAT_S chn_status;
	int k = 0;

	jpeg_parm.s32RecvPicNum = 1;

	ret = FH_JPEGE_Query(FH_GRP_NUM,&chn_status);
	if(ret != 0)
	{
		printf("FH_VENC_GetChnStatus fail\n");
		return ret;
	}

	printf("chn_status.u32CurPacks %d\n",chn_status.u32CurPacks);
	if(chn_status.u32CurPacks)
	{
		for(k = 0;k < chn_status.u32CurPacks;k++)
	    {
		    ret = FH_JPEGE_GetStream(FH_GRP_NUM, &jpeg_stream);
		    if (!ret)
		    {
		    	sprintf(filename,"jpg/%d.jpg",i);
				fp = fopen(filename, "wb");
				if(fp == NULL)
				{
					printf("open savejpg.jpg failed\n");
					return -1;
				}


				fwrite(jpeg_stream.jpeg_stream.start, 1, jpeg_stream.jpeg_stream.length, fp);
				fclose(fp);

				FH_JPEGE_ReleaseStream(FH_GRP_NUM,&jpeg_stream);

				printf("\njpeg_stream.jpeg_stream.length = %d,addr 0x%p\n", jpeg_stream.jpeg_stream.length,(unsigned int)jpeg_stream.jpeg_stream.start);
				i++;
		    }
	    }
	}


	return ret;
}

//#else
static int savejpgstream(void)
{
	FH_VENC_STREAM jpeg_stream;
	int ret;
	FILE*fp = NULL;
	char filename[32];
	static int i = 0;
	JPEGE_RECV_PIC_PARAM_S jpeg_parm;
	FH_VENC_CHN_CAP cfg_jpegmem;
	FH_BIND_INFO jpeg_src, jpeg_dst;

	jpeg_parm.s32RecvPicNum = 1;
	//while(1)
	{

		// 创建jpge通道
		cfg_jpegmem.support_type = FH_JPEG;
		cfg_jpegmem.max_size.u32Width = MAX_ENC_WIDTH;
		cfg_jpegmem.max_size.u32Height = MAX_ENC_HEIGHT;

		ret = FH_VENC_CreateChn(FH_GRP_NUM, &cfg_jpegmem);
		if (ret)
		{
		   printf("Error(%d - %x): jpeg FH_VENC_CreateChn\n", ret, ret);
		   return ret;
		}

		jpeg_src.obj_id = FH_OBJ_VPU_VO;
		jpeg_src.dev_id = 0;
		jpeg_src.chn_id = 0;
		jpeg_dst.obj_id = FH_OBJ_JPEG;
		jpeg_dst.dev_id = 0;
		jpeg_dst.chn_id = FH_GRP_NUM;
        ret = FH_SYS_Bind(jpeg_src, jpeg_dst);
		if (ret)
        {
            printf("Error(%d - %x):  VPU to ENC FH_SYS_Bind faild\n", ret, ret);
            goto fail0;
        }
		//ret = FH_JPEGE_StartRecvPic(FH_GRP_NUM,&jpeg_parm);
		ret = FH_VENC_StartRecvPic(FH_GRP_NUM);
        if (ret != 0)
        {
            printf("Error(%d - %x): jpeg FH_VENC_StartRecvPic (chn):(%d)!\n", ret, ret, FH_GRP_NUM);
            goto fail1;
        }

	    ret = FH_VENC_GetChnStream_Timeout(FH_GRP_NUM, &jpeg_stream, 1000);
	    if (!ret)
	    {
	    	sprintf(filename,"jpg/%d.jpg",i);
			fp = fopen(filename, "wb");
			if(fp == NULL)
			{
				printf("open savejpg.jpg failed\n");
				goto fail2;
			}


			fwrite(jpeg_stream.jpeg_stream.start, 1, jpeg_stream.jpeg_stream.length, fp);
			fclose(fp);

			FH_VENC_ReleaseStream(&jpeg_stream);

			printf("\njpeg_stream.jpeg_stream.length = %d,addr 0x%p\n", jpeg_stream.jpeg_stream.length,(unsigned int)jpeg_stream.jpeg_stream.start);
			i++;
	    }

	  }

fail2:
	FH_JPEGE_StopRecvPic(FH_GRP_NUM);
fail1:
	FH_SYS_UnBind(jpeg_src, jpeg_dst);
fail0:
	FH_VENC_DestroyChn(FH_GRP_NUM);
	return ret;
}
#endif

static bool baov_ready = false;

int aov_ready(void)
{
	int ret = 0;
	if (!baov_ready)
	{
		baov_ready = true;

		if(!baov_mutex_inited)
			pthread_mutex_init(&g_aov_mutex, NULL);
		baov_mutex_inited = 1;
        ret = linux_video_ready();
        printf("linux video_ready ....ret:%d\n", ret);
        sleep(1);

        // if (FH_GRP_NUM > 1)
        //     ret = linux_aov_config(1, 1, 3000, 300);
        //1000 1s
        ret = linux_aov_config(0, 1, 1000, 300);
        if (ret)
            printf("linux linux_aov_config failed ret %x ....\n", ret);
    }
	return 0;
}

int aov_video_exit(void)
{
	if(baov_ready)
	{
		baov_ready = false;

		AOV_XBUS_CMD_S aov_cmd;

		memset(&aov_cmd, 0, sizeof(aov_cmd));
		aov_cmd.cmd = AOV_CMD_VIDEO_EXIT;
		FH_RPC_Customer_Command(AOV_XBUS_COMMAND, &aov_cmd, sizeof(aov_cmd));
		printf("############# aov_video_exit ###############\n");
	}

	return 0;
}

int goto_aov_sleep(void)
{
    int ret = 0;
    AOV_XBUS_CMD_S aov_cmd;
    int fd=0;
    char ctrcmdstr[16]={0};

    memset(&aov_cmd, 0, sizeof(aov_cmd));


    if (!baov_ready)
	{
		baov_ready = true;

		if(!baov_mutex_inited)
			pthread_mutex_init(&g_aov_mutex, NULL);
		baov_mutex_inited = 1;
        ret = linux_video_ready();
        printf("linux video_ready ....ret:%d\n", ret);
        sleep(1);

        // if (FH_GRP_NUM > 1)
        //     ret = linux_aov_config(1, 1, 3000, 300);
        //1000 1s
        ret = linux_aov_config(0, 1, 1000, 300);
        if (ret)
            printf("linux linux_aov_config failed ret %x ....\n", ret);
    }
	pthread_mutex_lock(&g_aov_mutex);
    aov_cmd.cmd = AOV_CMD_SLEEP;
    printf("send AOV_XBUS_COMMAND SLEEP\n");
    ret = FH_RPC_Customer_Command(AOV_XBUS_COMMAND, &aov_cmd, sizeof(aov_cmd));

    printf("linux goto suspend ....\n");

	uint32_t sleep_tick = time_relative_ms();
    printf("goto_aov_sleep,t: %u\n", sleep_tick);
    fd = open("/sys/module/printk/parameters/console_suspend", O_WRONLY);
    if (fd > 0) {
        sprintf(ctrcmdstr, "N");
        write(fd, ctrcmdstr, strlen(ctrcmdstr));
        close(fd);
    }
    else{
            printf ("\nFailed  console_suspend\n");
    }

    fd = open("/sys/power/state", O_WRONLY);
    if (fd > 0) {
        sprintf(ctrcmdstr, "mem");
        write(fd, ctrcmdstr, strlen(ctrcmdstr));
        close(fd);
    }
    else{
            printf ("\nFailed  /sys/power/state\n");
    }

    printf("linux goto wakeup ...., lost tick:%u\n", time_relative_ms()-sleep_tick);

	pthread_mutex_unlock(&g_aov_mutex);
    //request the wakeup reason
    aov_cmd.cmd = AOV_CMD_WAKE_REASON;
    ret = FH_RPC_Customer_Command(AOV_XBUS_COMMAND, &aov_cmd, sizeof(aov_cmd));
    if(0 == ret) {
        printf("GRPID %d AOV_CMD_WAKE_REASON = 0x%x\n", aov_cmd.data.wake_source.grpid, aov_cmd.data.wake_source.wakeup_reason);
    }

	g_aov_resume_reason = aov_cmd.data.wake_source.wakeup_reason;

	uint8_t key_value = 0;
	switch(g_aov_resume_reason)
	{
		case AOV_WAKEUP_NN:
			printf("nn detect\n");
			savejpgstream();
			break;
		case AOV_WAKEUP_KEY1:
			printf("key1 wakeup\n");
			key_value = 1;
			break;
		case AOV_WAKEUP_KEY2:
			printf("key2 wakeup\n");
			key_value = 2;
			break;
		case AOV_WAKEUP_KEY3:
			printf("key3 wakeup\n");
			key_value = 3;
			break;
		case AOV_WAKEUP_KEY4:
			printf("key4 wakeup\n");
			key_value = 4;
			break;
		default:
			printf("other reason wakeup %d\n",g_aov_resume_reason);
			break;
	}

	if(get_day_night_status()==1)
	{
		for(int i=0; i < 4; i++)
		{
			int pwdindex_array[4] = {1,6,9,11};
			SetPwmValue(pwdindex_array[i], 1);
		}
	}

	if(key_value > 0)
	{
		sys_event_t e;
		SYS_EVENT_INIT_KEY(e, 0, key_value);
		sys_event_publish(&e);
	}
    printf("goto_aov_sleep end\n");
    return ret;
}

extern int __main_terminated;
void *aov_run(void *arg)
{
    int ret = 0;
    AOV_XBUS_CMD_S aov_cmd;
    //int grploop;
	//char c = 0;
	int sleep_flag = 0;
	//int enc_start = 0;

    int fd=0;
    char ctrcmdstr[16]={0};
	//jpeg_create_chn();

    ret = linux_video_ready();
    printf("linux video_ready ....ret:%d\n", ret);
    sleep(1);

    // if (FH_GRP_NUM > 1)
    //     ret = linux_aov_config(1, 1, 3000, 300);
	//1000 1s
    ret = linux_aov_config(0, 1, 1000, 300);
    if (ret)
        printf("linux linux_aov_config failed ret %x ....\n", ret);

    //printf("linux aov_config ....\n");
	//sleep(20);
    printf("linux aov_run ....\n");
    while(!__main_terminated) {
		exitflag = 0;
sleep_again:
		printf("sleep_flag %d\n",sleep_flag);
        memset(&aov_cmd, 0, sizeof(aov_cmd));

        pthread_mutex_lock(&g_aov_mutex);
        aov_cmd.cmd = AOV_CMD_SLEEP;
				printf("send AOV_XBUS_COMMAND SLEEP\n");
        ret = FH_RPC_Customer_Command(AOV_XBUS_COMMAND, &aov_cmd, sizeof(aov_cmd));

        printf("linux goto suspend ....\n");
#if 0
        system("echo N > /sys/module/printk/parameters/console_suspend");
        //system("echo 9 > /proc/sys/kernel/printk");
        system("echo mem > /sys/power/state");
#else


        fd = open("/sys/module/printk/parameters/console_suspend", O_WRONLY);
        if (fd > 0) {
            sprintf(ctrcmdstr, "N");
            write(fd, ctrcmdstr, strlen(ctrcmdstr));
            close(fd);
        }
        else{
             printf ("\nFailed  console_suspend\n");
        }

        fd = open("/sys/power/state", O_WRONLY);
        if (fd > 0) {
            sprintf(ctrcmdstr, "mem");
            write(fd, ctrcmdstr, strlen(ctrcmdstr));
            close(fd);
        }
        else{
             printf ("\nFailed  /sys/power/state\n");
        }

#endif
        printf("linux goto wakeup ....\n");

        pthread_mutex_unlock(&g_aov_mutex);
        //request the wakeup reason
        aov_cmd.cmd = AOV_CMD_WAKE_REASON;
        ret = FH_RPC_Customer_Command(AOV_XBUS_COMMAND, &aov_cmd, sizeof(aov_cmd));
        if(0 == ret) {
            printf("GRPID %d AOV_CMD_WAKE_REASON = 0x%x\n", aov_cmd.data.wake_source.grpid, aov_cmd.data.wake_source.wakeup_reason);
        }
        g_aov_resume_reason = aov_cmd.data.wake_source.wakeup_reason;


		if(g_aov_resume_reason == 0x10)
		{
			sleep(5);
			printf("goto sleep\n");
			goto sleep_again;
		}
		else
		{
			switch(g_aov_resume_reason)
			{
				case AOV_WAKEUP_NN:
					printf("nn detect\n");
					break;
				case AOV_WAKEUP_KEY1:
					printf("key1 wakeup\n");
					break;
				case AOV_WAKEUP_KEY2:
					printf("key2 wakeup\n");
					break;
				case AOV_WAKEUP_KEY3:
					printf("key3 wakeup\n");
					break;
				case AOV_WAKEUP_KEY4:
					printf("key4 wakeup\n");
					break;
				default:
					printf("other reason wakeup %d\n",g_aov_resume_reason);
					break;
			}
			//start_enc();
			//while(!g_sig_stop)
			//	sleep(1);
		}

		sleep_flag = 0;

        savejpgstream();
        printf("sleep for work\n");
        sleep(30);

		// if(enc_start)
		// 	stop_enc();

		#if 0
		nnp_module_hd_init();
		int ret = 0;
		while(1)
		{
			ret = nnp_process_frame(0,3,0,NULL);
			if(ret > 0)
			{
			printf("hd detect \n");
			}
		}
		//linux休眠时 ARC每秒1帧,buf 满了唤醒linux
		if(g_aov_resume_reason == AOV_WAKEUP_BUFFFULL)
			sleep(2);
		else
		{
			//判断wakeup模式做相应的出来，测试直接用sleep
			if(g_aov_resume_reason == AOV_WAKEUP_USER)
				printf("plc wakeup\n");
			else if(g_aov_resume_reason == AOV_WAKEUP_NN)
				printf("nn wakeup\n");
			//测试直接用sleep
			sleep(30);
		}
		#endif

    }

    return NULL;
}

void aov_creat(void)
{
    pthread_attr_t attr;

	if(!baov_mutex_inited)
   		pthread_mutex_init(&g_aov_mutex, NULL);
	baov_mutex_inited = 1;
	sem_init(&g_aov_sem, 0, 1); //上电初始化完毕即休眠
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    printf("linux aov_creat ....\n");
    pthread_create(&g_tAovThread,  &attr, aov_run, NULL);

    pthread_attr_destroy(&attr);
}
