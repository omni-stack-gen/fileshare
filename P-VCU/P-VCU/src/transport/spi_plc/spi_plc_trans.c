#define _GNU_SOURCE

#include <spi_plc/spi_plc_trans.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sched.h>
#include <utils/bmem.h>
#include <utils/list.h>
#include <utils/util.h>
#include <utils/time_helper.h>
#include <utils/mq.h>
// #include <sys/sys_event.h>
#include <utils/atomic.h>

#define LOG_TAG "spi_plc_trans"
#include <utils/log.h>

#define	SPI_DATA_LIST_MAX		(512)

#define PLC_SLICE_DATA_MAX       (2032)//(1984)	//(1990)

// #define BUF_CNT                 (4)
// #define SPI_PLC_BUFSIZE         (ONE_BUF_SIZE * BUF_CNT)
#define ONE_BUF_SIZE            (2000)
#define PAGE_SIZE               (4096)

#define SPI_WRITE_IOCTL			0x100
#define SPI_READ_IOCTL			0x101
#define SPI_LOGLEVEL_IOCTL		0x102
#define SPI_GET_FREE_BUF        0x103
#define SPI_FREE_RECV_BUF		0x104
#define SPI_GET_BUFCNT		    0x105
#define SPI_SET_BUFCNT			0x106
#define SPI_ALLOC_BUF           0x107
#define SPI_CTRL				0x108

#define SPI_PLC_CTRL_PATH       "/dev/spiplc_ctrl"

// insmod /system/ko/spi_plc.ko buf_cnt=8 showlog=2

typedef enum
{
    TYPE_EMPTY               = 0x00,
    TYPE_INIT                = 0x01,             // 获取当前版本号
    TYPE_SYNC                = 0x02,             // 同步数据
    TYPE_DATA                = 0x03,             // 音视频数据
    TYPE_CONFICT             = 0x04,             // 冲突
    TYPE_DATA_TEST           = 0x05,
	TYPE_UNICAST			 = 0x07,			 //para 0:添加组播地址 1:移除组地址 spi rmt 里面为主播地址
    TYPE_WAKEUP              = 0x0A,             // cco远程唤醒sta
    TYPE_SET_POFF            = 0x0B,
    TYPE_SET_PON             = 0x0C,
    TYPE_SET_UPDATA          = 0x0D,			 //升级固件输入 spi_rmt=写入数据在文件中的位置 spi_size=数据大小 输出 spi_para=1
    TYPE_SET_REBOOT          = 0x0E,			 //复位plc，写入升级固件后，需要重启设备
    TYPE_GET_RSSI            = 0x0F,			 //获取设备的信号强度
}eAppFrameType;

typedef struct _spi_param
{
	uint32_t dev;
	uint32_t param;
	uint8_t cmd;
	uint8_t reserv[3];
}spi_param;
/*
typedef struct _pkt_t_
{
	uint8_t spi_head;                      // 0 固定为 5a a5
	uint8_t spi_nospace;					// 1 1 表示无空闲buf，本次接收失败，0表示正常
	uint16_t spi_index;						// 2 发送序号，判断是否有丢包

	uint16_t spi_size;						// 4 后面数据大小
	uint8_t  spi_cmd;						// 6 命令
	uint8_t  spi_para;						// 7 参数
	uint32_t spi_rmt;						// 8 目标设备号
	uint8_t  spi_data[PLC_SLICE_DATA_MAX];	// 12
} pkt_t;
*/

typedef struct _pkt_t_
{
    uint8_t  spi_pad;                        // 0
    uint8_t  spi_nospace;                    // 1 1 表示无空闲buf，本次接收失败，0表示正常
    uint16_t spi_head;                        // 2 校验头
    uint32_t spi_index;                        // 4 发送序号，判断是否有丢包

    uint16_t spi_size;                        // 8 后面数据大小
    uint8_t  spi_cmd;                        // 10 命令
    uint8_t  spi_para;                        // 11 参数
    uint32_t spi_rmt;                        // 12 目标设备号
    uint8_t  spi_data[PLC_SLICE_DATA_MAX];    // 16
} pkt_t;    // 2048

typedef struct _pkt_list_t_
{
	//struct _pkt_list_t_ *pnext;
	int index;
	pkt_t *pkt;
} pkt_list_t;


//输出 spi_para = 数量 spi_data内为sRssiList结构的数据
typedef struct
{
	uint32_t id;
	uint32_t c_recv;
	uint32_t c_drop;
	uint8_t rssi;
} sRssiList;


// #pragma pack(1)
typedef struct _spi_pack_head
{
	uint8_t port;
	uint32_t seq;
}__attribute__((packed))spi_packhead_t;
// #pragma pack()
typedef struct spi_plc_trans
{
	int fd;
	int exit_sockets[2];
	int log_level;
	int buf_cnt;

	pkt_list_t *pkt_list;                    // 0~2047 for tx  2048~4095 for rx   mmap的内存
	int map_size;
	char *ptrmap;//tx rx buf
	pthread_t thread;

	uint8_t recv_buffer[PLC_SLICE_DATA_MAX];  // 用于拷贝内核数据，因为对齐的问题，无法强转使用
	pthread_mutex_t callback_mutex;
	struct list_head callback_list;

	bool data_thread_run;
	mq_t *data_mq;					//spi收到的数据
	pthread_t data_thread;

	sem_t  data_sem;				//spi接收信号
	pthread_mutex_t g_data_mutex;
	struct list_head g_data_list;
} spi_plc_trans_t;

typedef struct spi_plc_callback
{
	uint32_t port;
	uint32_t dev;
	void (*callback)(void *param, uint32_t dev, uint8_t *data, uint16_t data_size);
	void *param;
	struct list_head linked_node;
} spi_plc_callback_t;

typedef struct spi_plc_trans_event
{
	struct list_head node;
	//uint8_t type;
	uint32_t dev;
	uint8_t port;
	uint32_t datalen;
	uint8_t *pdata;
}spi_plc_trans_event_t;


static volatile bool g_bwork_flag = true;

void set_work_flag(bool flag)
{
    atomic_exchange_bool(&g_bwork_flag, flag);
}



static uint32_t LocalPlc_Dev = 0x0;
uint32_t GetPlcDev(void)
{
	return LocalPlc_Dev;
}
#if 0
static int spi_plc_set_ctrl(spi_plc_trans_t *trans, uint32_t dev,uint8_t cmd,uint32_t param)
{
	CHECK_HANDLE_RETURN_VAL_IF_FAIL(trans, -1);

	CHECK_VAL_RETURN_VAL_IF_FAIL(trans->fd >= 0, -1, "invaild fd");

	spi_param spiparam;
	spiparam.dev = dev;
	spiparam.cmd = cmd;
	spiparam.param = param;
	if (ioctl(trans->fd, SPI_CTRL, &spiparam) < 0)
	{
		LOGE("spi_plc_set_ctrl fail fd:%d \n", trans->fd);
		return -1;
	}
	else
	{
		LOGI("dev:%x  cmd:%x  param:%x\n", dev, cmd, param);
	}

	return 0;
}
#endif
static int spi_plc_set_log_level(spi_plc_trans_t *trans, int level)
{
	CHECK_HANDLE_RETURN_VAL_IF_FAIL(trans, -1);

	CHECK_VAL_RETURN_VAL_IF_FAIL(trans->fd >= 0, -1, "invaild fd");

	// 0x4 rx 0x8 tx  0x10 int 0x2 func  0x1 read rx_timeout
	if (ioctl(trans->fd, SPI_LOGLEVEL_IOCTL, &level) < 0)
	{
		LOGE("spi_set_loglevel fail fd:%d \n", trans->fd);
		return -1;
	}

	return 0;
}

//设置buf_cnt 调用SPI_ALLOC_BUF之后不能修改。关闭/dev/spiplc_ctrl之后，再打开可以设置
static int spi_plc_set_buf_cnt(spi_plc_trans_t *trans, int bufcnt)
{
	CHECK_HANDLE_RETURN_VAL_IF_FAIL(trans, -1);

	CHECK_VAL_RETURN_VAL_IF_FAIL(trans->fd >= 0, -1, "invaild fd");

	if (ioctl(trans->fd, SPI_SET_BUFCNT, &bufcnt) < 0)
	{
		LOGE("spi_set_bufcnt fail fd:%d \n", trans->fd);
		return -1;
	}

	return 0;
}

static int spi_get_buf_cnt(spi_plc_trans_t *trans)
{
	CHECK_HANDLE_RETURN_VAL_IF_FAIL(trans, -1);

	CHECK_VAL_RETURN_VAL_IF_FAIL(trans->fd >= 0, -1, "invaild fd");

	if (ioctl(trans->fd, SPI_GET_BUFCNT, &trans->buf_cnt) < 0)
	{
		LOGE("spi_get_buf_cnt fail fd:%d \n", trans->fd);
		return -1;
	}

	return 0;
}

//通知驱动申请用来发送接收的缓冲buf
static int spi_alloc_buf(spi_plc_trans_t *trans)
{
	CHECK_HANDLE_RETURN_VAL_IF_FAIL(trans, -1);

	CHECK_VAL_RETURN_VAL_IF_FAIL(trans->fd >= 0, -1, "invaild fd");

	int size = sizeof(pkt_list_t) * trans->buf_cnt;

	trans->pkt_list = malloc(size);
	if(trans->pkt_list == NULL)
	{
		printf("%s malloc plc_ctrl.ptk_list fail\n",__func__);
		return -2;
	}

	if (ioctl(trans->fd, SPI_ALLOC_BUF) < 0)
	{
		LOGE("spi_alloc_buf fail fd:%d \n", trans->fd);
		return -1;
	}

	return 0;
}

static int spi_plc_trans_write(spi_plc_trans_t *trans, uint32_t dev, uint8_t port,
                               uint8_t plc_cmd, uint8_t *data, uint16_t data_size)
{
	pkt_t *pkt = NULL;

	int index = -1;

	CHECK_HANDLE_RETURN_VAL_IF_FAIL(trans, -1);

	CHECK_VAL_RETURN_VAL_IF_FAIL(trans->fd >= 0, -1, "invaild fd");

	// 获取空闲 buf index
	if (ioctl(trans->fd, SPI_GET_FREE_BUF, &index) < 0)
	{
		LOGE("spi plc get free buffer fail index:%d\n", index);
		return -1;
	}

	if (!IN_RANGE(index, 0, trans->buf_cnt))
	{
		LOGE("outof range index:%d \n", index);
		return -1;
	}

	// 向申请的buf写入数据
	pkt = trans->pkt_list[index].pkt;

	memset(pkt, 0, sizeof(pkt_t));

	pkt->spi_cmd = plc_cmd;
	pkt->spi_rmt = dev;
	pkt->spi_para = 0;
	pkt->spi_size = data_size ? data_size+sizeof(spi_packhead_t) : 0;

	if (data)
	{
		static uint32_t useq = 0;
		useq++;
		spi_packhead_t packhead = {0};
		packhead.port = port;
		packhead.seq = useq;
#if 1
		char strbuf[4]= {0};
		memcpy(strbuf, &packhead.seq, 4);
		pkt->spi_data[0] = packhead.port;
		pkt->spi_data[1] = strbuf[0];
		pkt->spi_data[2] = strbuf[1];
		pkt->spi_data[3] = strbuf[2];
		pkt->spi_data[4] = strbuf[3];
#else
		//对齐不能这样拷贝， 要不然内异常
		memcpy(pkt->spi_data, &packhead, sizeof(spi_packhead_t));
#endif
		//PLC_SLICE_DATA_MAX - sizof(spi_packhead_t)
		#define TUT_SPI_NUM	 (2032-5)
		if(data_size > TUT_SPI_NUM)
		{
			LOGW("data_size:%d > %d, limit to 1979, ALL 1984 bytes will be sent\n", data_size, TUT_SPI_NUM);
			data_size = TUT_SPI_NUM;
		}
		memcpy(pkt->spi_data+sizeof(spi_packhead_t), data, data_size);
		// printf("<%u>  data_size:%d - %d  head:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", useq, data_size, pkt->spi_size,
		// 	pkt->spi_data[0], pkt->spi_data[1], pkt->spi_data[2], pkt->spi_data[3],
		// 	pkt->spi_data[4], pkt->spi_data[5], pkt->spi_data[6], pkt->spi_data[7],
		// 	pkt->spi_data[8], pkt->spi_data[9], pkt->spi_data[10], pkt->spi_data[11]);
		// printf("<%u>  data_size:%d - %d  end:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", useq, data_size, pkt->spi_size,
		// 	pkt->spi_data[data_size-12], pkt->spi_data[data_size-11], pkt->spi_data[data_size-10], pkt->spi_data[data_size-9],
		// 	pkt->spi_data[data_size-8], pkt->spi_data[data_size-7], pkt->spi_data[data_size-6], pkt->spi_data[data_size-5],
		// 	pkt->spi_data[data_size-4], pkt->spi_data[data_size-3], pkt->spi_data[data_size-2], pkt->spi_data[data_size-1]);

		static int lasttick = 0;
		static int packcount = 0;
		static int packsize = 0;
		int curtick = time_relative_ms();
		if (curtick - lasttick >= 1000*1)
		{
			/* code */
			LOGD("#losttime :%d send pack count:%d  length:%d dev:%x port:%x\n", curtick - lasttick, packcount, packsize, dev, port);
			lasttick = curtick;
			packcount = 0;
			packsize = 0;
		}
		packcount++;
		packsize+=pkt->spi_size;
	}

	// 通知驱动发送这个buf,发送完之后驱动会把这个buf还回，不需要应用控制
	if (ioctl(trans->fd, SPI_WRITE_IOCTL, &index) < 0)
	{
		LOGE("spi plc write %d fail\n", index);
		return -1;
	}

	return 0;
}

static int spi_plc_upgrade_write(spi_plc_trans_t *trans, uint32_t offset, uint8_t *data, uint16_t data_size)
{
	pkt_t *pkt = NULL;

	int index = -1;

	CHECK_HANDLE_RETURN_VAL_IF_FAIL(trans, -1);

	CHECK_VAL_RETURN_VAL_IF_FAIL(trans->fd >= 0, -1, "invaild fd");

	// 获取空闲 buf index
	if (ioctl(trans->fd, SPI_GET_FREE_BUF, &index) < 0)
	{
		LOGE("spi plc get free buffer fail index:%d\n", index);
		return -1;
	}

	if (!IN_RANGE(index, 0, trans->buf_cnt))
	{
		LOGE("outof range index:%d \n", index);
		return -1;
	}

	// 向申请的buf写入数据
	pkt = trans->pkt_list[index].pkt;

	memset(pkt, 0, sizeof(pkt_t));

	/**
	 * @brief 升级固件
		输入 spi_rmt=写入数据在文件中的位置 spi_size=数据大小
		输出 spi_para=1
	 */
	pkt->spi_cmd = TYPE_SET_UPDATA;
	pkt->spi_rmt = offset;
	pkt->spi_para = 0;
	pkt->spi_size = data_size;

	if (data)
	{
		if(data_size > PLC_SLICE_DATA_MAX)
		{
			LOGW("data_size:%d > 1979, limit to 1979, ALL 1984 bytes will be sent\n", data_size);
			data_size = PLC_SLICE_DATA_MAX;
		}
		memcpy(pkt->spi_data, data, data_size);
	}

	// 通知驱动发送这个buf,发送完之后驱动会把这个buf还回，不需要应用控制
	if (ioctl(trans->fd, SPI_WRITE_IOCTL, &index) < 0)
	{
		LOGE("spi plc write %d fail\n", index);
		return -1;
	}

	return 0;
}

static int spi_plc_cmd_by_param(spi_plc_trans_t *trans, uint32_t dev, uint8_t plc_cmd, uint8_t param)
{
	pkt_t *pkt = NULL;

	int index = -1;

	CHECK_HANDLE_RETURN_VAL_IF_FAIL(trans, -1);

	CHECK_VAL_RETURN_VAL_IF_FAIL(trans->fd >= 0, -1, "invaild fd");

	// 获取空闲 buf index
	if (ioctl(trans->fd, SPI_GET_FREE_BUF, &index) < 0)
	{
		LOGE("spi plc get free buffer fail index:%d\n", index);
		return -1;
	}

	if (!IN_RANGE(index, 0, trans->buf_cnt))
	{
		LOGE("outof range index:%d \n", index);
		return -1;
	}

	// 向申请的buf写入数据
	pkt = trans->pkt_list[index].pkt;

	memset(pkt, 0, sizeof(pkt_t));

	pkt->spi_cmd = plc_cmd;
	pkt->spi_rmt = dev;
	pkt->spi_para = param;
	pkt->spi_size = 0;

	// 通知驱动发送这个buf,发送完之后驱动会把这个buf还回，不需要应用控制
	if (ioctl(trans->fd, SPI_WRITE_IOCTL, &index) < 0)
	{
		LOGE("spi plc write %d fail\n", index);
		return -1;
	}

	return 0;
}

static pkt_t *spi_plc_dequeue_read_pkt(spi_plc_trans_t *trans, int *read_index)
{
	int index = -1;

	pkt_t *pkt = NULL;

	CHECK_HANDLE_RETURN_VAL_IF_FAIL(trans, NULL);

	CHECK_VAL_RETURN_VAL_IF_FAIL(trans->fd >= 0, NULL, "invaild fd");

	CHECK_VAL_RETURN_VAL_IF_FAIL(read_index, NULL, "invaild ponit");

	// 获取空闲buf index
	if (ioctl(trans->fd, SPI_READ_IOCTL, &index) < 0)
	{
		LOGE("get free buffer fail\n");
		return NULL;
	}

	if (!IN_RANGE(index, 0, trans->buf_cnt))
	{
		LOGE("pkt index outof range index:%d \n", index);
		return NULL;
	}

	pkt = trans->pkt_list[index].pkt;

	if (read_index)
	{
		*read_index = index;
	}

#if 0

	if (pkt->plc_port != AUDIO_PORT)
	{
		LOGI("recv index:%d head:%04x plc_cmd:0x%x loc_dev:0x%x plc_dev:0x%x plc_port:0x%x plc_index:%d plc_size:%d \n",
		     index, pkt->plc_head, pkt->plc_cmd, pkt->loc_dev, pkt->plc_dev,
		     pkt->plc_port, pkt->plc_index, pkt->plc_size);
	}

#endif

	return pkt;
}

static int spi_plc_enqueue_read_pkt(spi_plc_trans_t *trans, int index)
{
	CHECK_HANDLE_RETURN_VAL_IF_FAIL(trans, -1);

	CHECK_VAL_RETURN_VAL_IF_FAIL(trans->fd >= 0, -1, "invaild fd");

	if (!IN_RANGE(index, 0, trans->buf_cnt))
	{
		LOGE("outof range index:%d \n", index);
		return -1;
	}

	if (ioctl(trans->fd, SPI_FREE_RECV_BUF, &index) < 0)
	{
		LOGE("spi plc enqueue read pkt fail fd:%d index:%d \n", trans->fd, index);
		return -1;
	}

	return 0;
}

static void spi_plc_process(spi_plc_trans_t *trans, uint32_t dev, uint8_t port, uint8_t *data, uint16_t data_size)
{
	spi_plc_callback_t *node = NULL, *next = NULL;

	pthread_mutex_lock(&trans->callback_mutex);

	list_for_each_entry_safe(node, next, &trans->callback_list, linked_node)
	{
		if ((node->dev==0 && node->port == port) ||
			(node->dev!=0 && node->dev==dev && node->port == port))
		{

			if (node->callback)
			{
				node->callback(node->param, dev, data, data_size);
			}

			break;
		}
	}

	pthread_mutex_unlock(&trans->callback_mutex);
}

#pragma pack(1)
typedef struct {
	unsigned int seq;		//分片序号
	unsigned char end;     //bit 0  分片起始        bit 1  分片结尾        bit  7  重传分片
	char data[];
} ST_TransPacket;

#pragma pack()

static void *spi_data_thread(void *args)
{
	spi_plc_trans_t *trans = (spi_plc_trans_t *)args;

	LOGI("spi_data_thread start [%p]\n", trans);

	//spi_plc_trans_event_t event;

	while (trans->data_thread_run)
	{
		//memset(&event, 0, sizeof(event));
		// if (0 == mq_recv(trans->data_mq, &event, sizeof(spi_plc_trans_event_t), 100))
		// {
		// 	if(event.pdata && event.datalen)
		// 	{
		// 		LOGI(">>>>>>>>recv event data [%p] len [%u]\n", event.pdata, event.datalen);
		// 		spi_plc_process(trans, event.dev, event.port, event.pdata, event.datalen);
		// 		bfree(event.pdata);
		// 	}
		// }
		// else
		// 	printf("recv timeout \n");
		//process_time_event(mgnt);

		sem_wait(&trans->data_sem);

		spi_plc_trans_event_t *p = NULL;
		spi_plc_trans_event_t *n = NULL;
		pthread_mutex_lock(&trans->g_data_mutex);
		if (!list_empty_careful(&trans->g_data_list))
		{
			list_for_each_entry_safe(p, n, &trans->g_data_list, node)
			{
				list_del(&p->node);
				break;
			}
		}
		pthread_mutex_unlock(&trans->g_data_mutex);
		if(p)
		{
			//ST_TransPacket * ptp = (ST_TransPacket*)p->pdata;
			// LOGW("PLC SPI seq:%u pdata:%p len:%d end:%x c: %02x %02x %02x %02x \r\n", ptp->seq, p->pdata, p->datalen, ptp->end,
			// 			ptp->data[0], ptp->data[1], ptp->data[2], ptp->data[3]);
			spi_plc_process(trans, p->dev, p->port, p->pdata, p->datalen);
			//bfree(p->pdata);

			//memset(p, 0, sizeof(spi_plc_trans_event_t)+p->datalen);
			bfree(p);
		}
	}

	return NULL;
}

static void *spi_plc_thread(void *args)
{
	spi_plc_trans_t *trans = (spi_plc_trans_t *)args;

	fd_set rfds;
	struct timeval tv;
	int max_fd, retval;

	LOGI("spi_plc_thread start \n");
// #ifndef ENABLE_V6
	pthread_attr_t attr;

    struct sched_param sp;

    int rs = pthread_getattr_np(pthread_self(), &attr);

    if(rs != 0)
        LOGW("pthread_getattr_np fail");

    rs = SCHED_FIFO;

    // 设置线程调度策略为SCHED_FIFO
    pthread_attr_setschedpolicy(&attr, rs);

    // 实时优先级rt_priority(1-99)
    sp.sched_priority = 99;
    pthread_attr_setschedparam(&attr, &sp);

    pthread_attr_destroy(&attr);
// #endif
	// 通知PLC初始化, 不能为0
	if(GATEWAY_DEV != 0)
	{
		set_work_flag(true);
		spi_plc_trans_write(trans, GATEWAY_DEV, 0, TYPE_INIT, NULL, 0);

#ifndef __GLIBC__
		//spi_plc_set_ctrl(trans, GATEWAY_DEV, TYPE_SLOT, 3);
#endif
	}
	else
	{
		LOGW("GATEWAY_DEV is 0 \n");
	}
	while (1)
	{
		FD_ZERO(&rfds);
		FD_SET(trans->fd, &rfds);
		FD_SET(trans->exit_sockets[0], &rfds);

		max_fd = trans->exit_sockets[0] > trans->fd ? trans->exit_sockets[0] : trans->fd;

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		retval = select(max_fd + 1, &rfds, NULL, NULL, &tv);

		if (retval == 0)
		{
			continue;
		}
		else if (retval < 0 && errno != EINTR)
		{
			LOGE("Error on select retval:%d error:%d\n", retval, errno);
			continue;
		}

		if (FD_ISSET(trans->exit_sockets[0], &rfds))
		{
			char quit;

			if (read(trans->exit_sockets[0], &quit, sizeof(quit)) < 0)
			{
				LOGE("Could not read exit_sockets: (%d:%s)", errno, strerror(errno));
			}

			break;
		}

		if (FD_ISSET(trans->fd, &rfds))
		{
			int index = 0;

			pkt_t *pkt = NULL;

			pkt = spi_plc_dequeue_read_pkt(trans, &index);

			if (pkt)
			{
				//if (pkt->spi_cmd != TYPE_EMPTY && pkt->spi_cmd != TYPE_DATA)
				// 	LOGE("spi_rmt:%x cmd:%x len:%d \n", pkt->spi_rmt, pkt->spi_cmd, pkt->spi_size);
				if (pkt->spi_cmd == TYPE_INIT)
				{
					LOGI("plc dev ready, Version:>>>>> %x <<<<<\n", pkt->spi_rmt);
					int Plc_Process_CallBack_By_PlcVersion(uint32_t plcversion);
					Plc_Process_CallBack_By_PlcVersion(pkt->spi_rmt);
					// sys_event_t e = {0};

					// e.type = SYS_EVENT_PLC;

					// sys_event_publish(&e);
				}
				else if (pkt->spi_cmd == TYPE_CONFICT)
				{
					set_work_flag(false);
					int Plc_Process_CallBack_By_PlcConflict(void);
					Plc_Process_CallBack_By_PlcConflict();
				}
				else if (pkt->spi_cmd == TYPE_GET_RSSI)
				{
					sRssiList *rssi_list = (sRssiList *)pkt->spi_data;

					// LOGI("rssi_list->rssi_num:%d\n", pkt->spi_para);
					// for (size_t i = 0; i < pkt->spi_para; i++)
					// {
					// 	printf("rssi_list->rssi_list[%d]: id:%x rssi:%d recv:%x drop:%x\n", i, rssi_list[i].id, rssi_list[i].rssi, rssi_list[i].c_recv, rssi_list[i].c_drop);
					// }

					int Plc_Process_CallBack_By_PlcRssiList(uint32_t plccount, void *prssi_list);
					Plc_Process_CallBack_By_PlcRssiList(pkt->spi_para, (void *)rssi_list);
				}
				// else
				if (pkt->spi_cmd == TYPE_DATA)
				{
					uint16_t data_size = MIN(pkt->spi_size, PLC_SLICE_DATA_MAX);
					spi_packhead_t packhead = {0};
					memset(trans->recv_buffer, 0, sizeof(trans->recv_buffer));
					if(data_size > sizeof(spi_packhead_t))
					{
						memcpy(&packhead, pkt->spi_data, sizeof(spi_packhead_t));
						memcpy(trans->recv_buffer, pkt->spi_data+sizeof(spi_packhead_t), data_size-sizeof(spi_packhead_t));
						data_size = data_size-sizeof(spi_packhead_t);
					}
					// LOGI("<%u> head:%x spi_index:%u spi_rmt:%x cmd:%x len:%d = %d head:%02x %02x %02x %02x && %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", packhead.seq, pkt->spi_head, pkt->spi_index, pkt->spi_rmt, pkt->spi_cmd, pkt->spi_size, data_size,
					// 	trans->recv_buffer[0], trans->recv_buffer[1], trans->recv_buffer[2], trans->recv_buffer[3],
					// 	pkt->spi_data[0], pkt->spi_data[1], pkt->spi_data[2], pkt->spi_data[3],
					// 	pkt->spi_data[4], pkt->spi_data[5], pkt->spi_data[6], pkt->spi_data[7],
					// 	pkt->spi_data[8], pkt->spi_data[9], pkt->spi_data[10], pkt->spi_data[11]);

#if 0
					spi_plc_process(trans, pkt->spi_rmt, packhead.port, trans->recv_buffer, data_size);
#else
					//uint64_t ltick = time_relative_us();
					spi_plc_trans_event_t *pevent = (spi_plc_trans_event_t*)bmalloc(sizeof(spi_plc_trans_event_t)+data_size+1);
					pevent->dev = pkt->spi_rmt;
					pevent->port = packhead.port;
					pevent->datalen = data_size;
					if(pevent->datalen > 0)
					{
						pevent->pdata = (uint8_t*)(pevent+1);//bmalloc(pevent->datalen+1);
						memset(pevent->pdata, 0, pevent->datalen+1);
						memcpy(pevent->pdata, trans->recv_buffer, pevent->datalen);
					}

					//ST_TransPacket * ptp = (ST_TransPacket*)pevent->pdata;
					// LOGI("pull recv seq:%u pdata:%p len:%d end:%x c: %02x %02x %02x %02x \r\n", ptp->seq, pevent->pdata, pevent->datalen, ptp->end,
					// 	ptp->data[0], ptp->data[1], ptp->data[2], ptp->data[3]);
					//LOGI("<%u>  spi_index:%u spi_rmt:%x losttime:%llu\n", packhead.seq, pkt->spi_index, pkt->spi_rmt, time_relative_us()-ltick);

					// if(mq_send(trans->data_mq, &event, sizeof(spi_plc_trans_event_t)))
					// {
					// 	LOGW("Spi pkt process fail \n");
					// }
					pthread_mutex_lock(&trans->g_data_mutex);
					list_add_tail(&pevent->node, &trans->g_data_list);
					pthread_mutex_unlock(&trans->g_data_mutex);

					sem_post(&trans->data_sem);
#endif
#if 0
					static uint32_t lastseq = 0;
					static uint32_t lastindex = 0;
					int iseq = 0;
					memcpy(&iseq , &packhead.seq, 4);
					if(lastseq && iseq != (lastseq+1))
					{
						static int index = 0;
						index++;
						LOGE("$$$$$$$$$$$$$$$ seq err:%d %u - %u pkt->spi_rmt:%x spi_size:%u index: %d : %d $$$$$$$$$$$$$$$$\n", index, iseq, lastseq, pkt->spi_rmt, pkt->spi_size, pkt->spi_index, lastindex);
						// if(index>2)
						// break;
					}
					lastseq = iseq;
					lastindex = pkt->spi_index;
#endif
				}

				spi_plc_enqueue_read_pkt(trans, index);
			}
		}
	}

	LOGI("spi_plc_thread end \n");

	return NULL;
}

void *spi_plc_trans_create(int level, uint32_t dev_name)
{
	int ret = 0;

	spi_plc_trans_t *trans = NULL;

	trans = bmalloc(sizeof(spi_plc_trans_t));

	if (trans == NULL)
	{
		LOGE("malloc spi_plc_trans_t failed \n");
		goto __exit;
	}

	if(dev_name!=0)
		LocalPlc_Dev = dev_name;
	LOGI("LocalPlc_Dev Reload:0x%x \n", LocalPlc_Dev);
	memset(trans, 0, sizeof(spi_plc_trans_t));

	trans->exit_sockets[0] = trans->exit_sockets[1] = -1;

	pthread_mutex_init(&trans->callback_mutex, NULL);

	INIT_LIST_HEAD(&trans->callback_list);

	trans->fd = open(SPI_PLC_CTRL_PATH, O_RDWR | O_NDELAY | O_CLOEXEC);

	if (trans->fd < 0)
	{
		LOGE("open [%s] error %d :%s \n", SPI_PLC_CTRL_PATH, errno, strerror(errno));
		goto __exit;
	}

	if (level != 0)
	{
		spi_plc_set_log_level(trans, level);
	}
#ifdef ENABLE_NX5
	int bufcnt = 512;
#else
	int bufcnt = 64;
#endif
	LOGI("spi buffer count set %d \n", bufcnt);
	//设置驱动和应用之间共享的用来发送和接收的buf个数
	spi_plc_set_buf_cnt(trans, bufcnt);

	//获取设置的buf个数
	if (spi_get_buf_cnt(trans) != 0)
	{
		goto __exit;
	}
	//申请内存(注意：申请完buf后不能调用close(trans->fd),否则报错)
	if(spi_alloc_buf(trans) != 0)
	{
		goto __exit;
	}

	LOGI("spi buffer count %d \n", trans->buf_cnt);

	trans->map_size = ALIGN_UP(sizeof(pkt_t) * trans->buf_cnt, PAGE_SIZE);

	trans->ptrmap = (char *)mmap(NULL, trans->map_size,
	                                     PROT_READ | PROT_WRITE, MAP_SHARED,
	                                     trans->fd, 0);

	if (trans->pkt_list == NULL)
	{
		LOGE("mmap failed! \n");
		goto __exit;
	}

	for(int i = 0;i < trans->buf_cnt;i++)
	{
		trans->pkt_list[i].index = i;
		trans->pkt_list[i].pkt = (pkt_t*)(trans->ptrmap + i * sizeof(pkt_t));
	}

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, trans->exit_sockets);

	if (ret != 0)
	{
		LOGE("create socketpair failed! \n");
		goto __exit;
	}

	if(!trans->data_mq)
		trans->data_mq = mq_create("plc_data_mq", sizeof(spi_plc_trans_event_t), SPI_DATA_LIST_MAX);

	INIT_LIST_HEAD(&trans->g_data_list);
	pthread_mutex_init(&trans->g_data_mutex, NULL);
	sem_init(&trans->data_sem, 0, 0); //无名信号量

	trans->data_thread_run = true;
	ret = pthread_create(&trans->thread, NULL, spi_data_thread, trans);
	if (ret != 0)
	{
		trans->data_thread_run = false;
		LOGE("pthread_create error:%d \n", ret);
		goto __exit;
	}

	ret = pthread_create(&trans->thread, NULL, spi_plc_thread, trans);

	if (ret != 0)
	{
		LOGE("pthread_create error:%d \n", ret);
		goto __exit;
	}

	return trans;

__exit:

	spi_plc_trans_destroy((void **)&trans);

	return NULL;
}

int spi_plc_trans_destroy(void **handle)
{
	spi_plc_trans_t *trans = (spi_plc_trans_t *)handle;
	spi_plc_callback_t *node = NULL, *next = NULL;

	if (handle == NULL || *handle == NULL)
	{
		return -1;
	}

	trans = (spi_plc_trans_t *)*handle;

	if (trans->thread)
	{
		if (trans->exit_sockets[1] >= 0)
		{
			write(trans->exit_sockets[1], "q", 1);
		}

		pthread_join(trans->thread, NULL);
		trans->thread = 0;
	}

	if (trans->exit_sockets[0] >= 0)
	{
		close(trans->exit_sockets[0]);
		trans->exit_sockets[0] = -1;
	}

	if (trans->exit_sockets[1] >= 0)
	{
		close(trans->exit_sockets[1]);
		trans->exit_sockets[1] = -1;
	}

	if(trans->pkt_list)
	{
		free(trans->pkt_list);
		trans->pkt_list = NULL;
	}

	if (trans->ptrmap)
	{
		munmap(trans->ptrmap, trans->map_size);
		trans->ptrmap = NULL;
	}

	if (trans->fd >= 0)
	{
		close(trans->fd);
		trans->fd = -1;
	}

	if(trans->data_mq)
	{
		mq_destroy(trans->data_mq);
		trans->data_mq = NULL;
	}

	if (trans->data_thread)
	{
		trans->data_thread_run = false;
		sem_post(&trans->data_sem);
		pthread_join(trans->data_thread, NULL);
		trans->data_thread = 0;
		sem_destroy(&trans->data_sem);
	}

	spi_plc_trans_event_t *dnode = NULL, *dnext = NULL;
	pthread_mutex_lock(&trans->g_data_mutex);
	if(trans->g_data_list.next){
		list_for_each_entry_safe(dnode, dnext, &trans->g_data_list, node)
		{
			list_del(&dnode->node);
			bfree(dnode);
		}
	}

	pthread_mutex_unlock(&trans->g_data_mutex);

	pthread_mutex_destroy(&trans->g_data_mutex);


	pthread_mutex_lock(&trans->callback_mutex);

	list_for_each_entry_safe(node, next, &trans->callback_list, linked_node)
	{
		list_del(&node->linked_node);
		bfree(node);
	}

	pthread_mutex_unlock(&trans->callback_mutex);

	pthread_mutex_destroy(&trans->callback_mutex);

	bfree(trans);
	*handle = NULL;

	return 0;
}

int spi_plc_trans_register(void *handle, uint32_t port, uint32_t remote_dev,
                           void (*callback)(void *param, uint32_t dev, uint8_t *data, uint16_t data_size),
                           void *param)
{
	spi_plc_trans_t *trans = (spi_plc_trans_t *)handle;
	spi_plc_callback_t *node = NULL, *next = NULL;

	if (trans == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&trans->callback_mutex);

	list_for_each_entry_safe(node, next, &trans->callback_list, linked_node)
	{
		if ((remote_dev==0 && node->port == port) ||
			(remote_dev!=0 && node->port == port && node->dev == remote_dev))
		{
			node->param = param;
			pthread_mutex_unlock(&trans->callback_mutex);
			return 0;
		}
	}

	node = bmalloc(sizeof(spi_plc_callback_t));

	if (node == NULL)
	{
		pthread_mutex_unlock(&trans->callback_mutex);
		return -1;
	}

	memset(node, 0, sizeof(spi_plc_callback_t));

	node->port = port;
	node->dev = remote_dev;
	node->callback = callback;
	node->param = param;

	list_add_tail(&node->linked_node, &trans->callback_list);

	pthread_mutex_unlock(&trans->callback_mutex);

	LOGI("spi trans register remotedev [0x%x] port [0x%x] callback [%p] param [%p] success\n", remote_dev, port, callback, param);

	return 0;
}

int spi_plc_trans_unregister(void *handle, uint8_t port, uint32_t remote_dev)
{
	spi_plc_trans_t *trans = (spi_plc_trans_t *)handle;
	spi_plc_callback_t *node = NULL, *next = NULL;

	if (trans == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&trans->callback_mutex);

	list_for_each_entry_safe(node, next, &trans->callback_list, linked_node)
	{
		if ((remote_dev==0 && node->port == port) ||
			(remote_dev!=0 && node->port == port && node->dev == remote_dev))
		{
			LOGI("spi trans unregister remotedev [0x%x] port [0x%x] callback [%p] param [%p] success\n", node->dev, node->port, node->callback, node->param);
			list_del(&node->linked_node);
			bfree(node);
			break;
		}
	}

	pthread_mutex_unlock(&trans->callback_mutex);

	return 0;
}

int spi_plc_set_audio_tran(void *handle, uint32_t dev, uint32_t param)
{
	spi_plc_trans_t *trans = (spi_plc_trans_t *)handle;

	if (trans == NULL)
	{
		return -1;
	}
	return 1;////spi_plc_set_ctrl(trans, dev, TYPE_SET_AUDIO, param);
}

int spi_plc_retset_localId(void*handle, uint32_t devid)
{
	spi_plc_trans_t *trans = (spi_plc_trans_t *)handle;
	int ret = -1;
	if(trans && devid!=0)
	{
		LocalPlc_Dev = devid;

		set_work_flag(true);
		// 通知PLC初始化, 不能为0
		spi_plc_trans_write(trans, devid, 0, TYPE_INIT, NULL, 0);
	}

	return ret;
}

int spi_plc_trans_send(void *handle, uint32_t dev, uint8_t port, uint8_t *data, uint16_t data_size)
{
	spi_plc_trans_t *trans = (spi_plc_trans_t *)handle;

	if (trans == NULL)
	{
		return -1;
	}

	if(atomic_load_bool(&g_bwork_flag))
	{
		return spi_plc_trans_write(trans, dev, port, TYPE_DATA, data, data_size);
	}
	else
	{
		LOGW("spi_plc_trans_send: not working, drop data\n");
		return -1;
	}
}

int spi_plc_trans_off(void *handle)
{
	spi_plc_trans_t *trans = (spi_plc_trans_t *)handle;

	if (trans == NULL)
	{
		return -1;
	}

	printf("set poweroff \n");
	//return spi_plc_set_ctrl(trans, 0, TYPE_SET_POFF, 0);
	return spi_plc_trans_write(trans, 0, 0, TYPE_SET_POFF, NULL, 0);
}

int spi_plc_trans_open_dev(void *handle, uint32_t devid)
{
	spi_plc_trans_t *trans = (spi_plc_trans_t *)handle;

	if (trans == NULL)
	{
		return -1;
	}

	return spi_plc_trans_write(trans, devid, 0, TYPE_WAKEUP, NULL, 0);
}

int spi_plc_trans_get_rssi(void *handle)
{
	spi_plc_trans_t *trans = (spi_plc_trans_t *)handle;

	if (trans == NULL)
	{
		return -1;
	}
	if(!atomic_load_bool(&g_bwork_flag))
	{
		LOGW("spi_plc_trans_get_rssi: not working, return -1\n");
		return -1;
	}
	return spi_plc_trans_write(trans, 0, 0, TYPE_GET_RSSI, NULL, 0);
}

int spi_plc_upgrade(void *handle, char *file_path)
{
	spi_plc_trans_t *trans = (spi_plc_trans_t *)handle;

	if (trans == NULL)
	{
		return -1;
	}

	if(!file_path){
		LOGW("file_path is null\n");
		return -1;
	}
	FILE *pfd = fopen(file_path, "rb");
	if(!pfd){
		LOGE("open file %s failed\n", file_path);
		return -1;
	}

	uint8_t buffer[PLC_SLICE_DATA_MAX] ={0};
	int read_size = 0;
	int write_size = 0;
	do
	{
		read_size = fread(buffer, 1, PLC_SLICE_DATA_MAX, pfd);
		if(read_size > 0)
		{
			spi_plc_upgrade_write(trans, write_size, buffer, read_size);
			usleep(10);
			write_size+=read_size;
			printf("send upgrade data size:%d  write_size:%d\n", read_size, write_size);
		}
		else
		{
			LOGI("read file end\n");
			break;
		}
	} while (1);

	fclose(pfd);
	return 0;
}

int spi_plc_upgrade_by_fd(void *handle, FILE *pfd, uint32_t file_size)
{
	spi_plc_trans_t *trans = (spi_plc_trans_t *)handle;

	if (trans == NULL)
	{
		return -1;
	}

	if(!pfd){
		LOGW("pfd is null\n");
		return -1;
	}

	uint8_t buffer[PLC_SLICE_DATA_MAX] ={0};
	int read_size = 0;
	int write_size = 0;
	int write_total = 0;
	while(file_size > 0)
	{
		if(file_size > PLC_SLICE_DATA_MAX)
		{
			write_size = PLC_SLICE_DATA_MAX;
		}
		else
		{
			write_size = file_size;
		}
		file_size -= write_size;

		read_size = fread(buffer, 1, write_size, pfd);
		if(read_size > 0)
		{
			spi_plc_upgrade_write(trans, write_total, buffer, read_size);
			usleep(20);
			write_total+=read_size;
			printf("send upgrade data size:%d  write_total:%d\n", read_size, write_total);
		}
		else
		{
			LOGI("read file end\n");
			break;
		}
	}

	return 0;
}

int spi_plc_reboot(void *handle)
{
	spi_plc_trans_t *trans = (spi_plc_trans_t *)handle;

	if (trans == NULL)
	{
		return -1;
	}
	return spi_plc_trans_write(trans, 0, 0, TYPE_SET_REBOOT, NULL, 0);
}

//组播操作
int spi_plc_multicast_ctrl(void *handle, uint32_t multicastaddr, uint8_t isremove)
{
	spi_plc_trans_t *trans = (spi_plc_trans_t *)handle;

	if (trans == NULL)
	{
		return -1;
	}

	return spi_plc_cmd_by_param(trans, multicastaddr, TYPE_UNICAST, isremove);
}