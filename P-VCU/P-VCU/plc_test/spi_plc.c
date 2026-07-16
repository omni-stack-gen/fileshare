#include <sys/select.h>
#include <poll.h>
#include <pthread.h>
#include <semaphore.h>
#include "spi_plc.h"


typedef void (*pfnSpiCb)(void*, uint8_t, uint8_t*, int);
typedef struct _spi_cb_list_
{
	struct _spi_cb_list_* pnext;
	uint8_t port;
	pfnSpiCb pfunc;
	void* hdl;
} spi_cb_list_t;

typedef struct {
	uint8_t dev;
	uint8_t port;
	uint16_t size;
}spi_plc_chn;

typedef struct {
   int start;
   pthread_mutex_t spi_trans_mutex;
   spi_cb_list_t *pSpiCbList;
   sem_t sema;
}spi_plc_param;

//#define MASTER_TEST
#define SLAVE_TEST
static spi_plc_param spiplc_para;
#ifdef MASTER_TEST
static int wait_ms = 10;
#endif
#ifdef SLAVE_TEST
static int wait_ms = 100;
#endif
unsigned int DPGetTickCount(void)
{
        struct timespec t_start;
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        return (t_start.tv_sec * 1000 + t_start.tv_nsec / 1000000);
}

void spi_trans_register(uint8_t port, void (*pfnHandler)(void* hdl, uint8_t dev, uint8_t* pdata, int len), void* hdl)
{
	spi_cb_list_t* spicb = malloc(sizeof(spi_cb_list_t));
	spicb->port = port;
	spicb->pfunc = pfnHandler;
	spicb->hdl = hdl;

	pthread_mutex_lock(&spiplc_para.spi_trans_mutex);
	spicb->pnext = spiplc_para.pSpiCbList;
	spiplc_para.pSpiCbList = spicb;
	pthread_mutex_unlock(&spiplc_para.spi_trans_mutex);
	{
		for(spicb = spiplc_para.pSpiCbList; spicb != NULL; spicb = spicb->pnext)
		{
				printf("spicb %x %p\r\n", spicb->port, spicb);
		}
	}
}

void spi_trans_unregister(uint8_t port)
{
	spi_cb_list_t* spicb;
	spi_cb_list_t* spicbprev = NULL;

	pthread_mutex_lock(&spiplc_para.spi_trans_mutex);
	for(spicb = spiplc_para.pSpiCbList; spicb->port != port; spicb = spicb->pnext)
	{
		spicbprev = spicb;
	}
	if(spicb != NULL)
	{
		if(spicbprev != NULL)
			spicbprev->pnext = spicb->pnext;
		else
			spiplc_para.pSpiCbList = spicb->pnext;
		free(spicb);
	}
	pthread_mutex_unlock(&spiplc_para.spi_trans_mutex);
	if(spiplc_para.pSpiCbList != NULL)
	{
		for(spicb = spiplc_para.pSpiCbList; spicb != NULL; spicb = spicb->pnext)
		{
				printf("spicb %x %p\r\n", spicb->port, spicb);
		}
	}
	else
		printf("no cb\r\n");
}


void SpiHander(void*hdl, uint8_t dev, uint8_t*data, int size)
{
	spi_plc_chn *spi_chn = (spi_plc_chn *)hdl;
	uint32_t *ptr = (uint32_t*)data;
	static uint32_t count = 0;
	static uint32_t lasttime = 0;//DPGetTickCount();

	if(DPGetTickCount() - lasttime > 1000)
	{
		printf("current data index %d\n",*ptr);
		lasttime = DPGetTickCount();
	}

	if(dev == spi_chn->dev)
	{
		if(size == PLC_SLICE_DATA_MAX)
		if(count == 0)
			count = *ptr;

		if(*ptr != count)
		{
			printf("recv 0x%08x  should be 0x%08x\n",*ptr,count);
			count = *ptr;
		}
		//else
		//	printf("recv 0x%08x  \n",*ptr);
		count++;
	}
	//printf("SpiHander size %d\n",size);
}

static void *SpiPlcRecvThread(void*param)
{
	int ret = 0;
	spi_cb_list_t* pSpiCb = NULL;
	pkt_list_t *pkt_list = NULL;
	struct timeval tv;
	unsigned int lasttime = DPGetTickCount();
	int fd = get_spi_plc_fd();

	if(fd < 0)
	{
		printf("%s get_spi_plc_fd faiil\n",__func__);
		return NULL;
	}

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	while(spiplc_para.start)
	{
		FD_ZERO (&fds);
		FD_SET (fd, &fds);

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		ret = select(fd+1, &fds, NULL, NULL, &tv);
		if(ret == -1)
		{
			printf("select error\n");
			return NULL;
		}
		else if(ret == 0)
		{
			// printf("select timeout,not detect read data\n");
			continue;
		}
		else
		{
			#if 1
			if (FD_ISSET(fd, &fds))
			{
				pkt_list = (pkt_list_t *)spi_plc_read();
				if(pkt_list == NULL)
					continue;
				//printf("plc_cmd %d\r\n", pkt->plc_cmd);
				if(DPGetTickCount() - lasttime > 1000)
				{
					//printf("current index %d\n",pkt_list->pkt->spi_index);
					lasttime = DPGetTickCount();
				}

				pthread_mutex_lock(&spiplc_para.spi_trans_mutex);
				pSpiCb = spiplc_para.pSpiCbList;
				while(pSpiCb != NULL)
				{
					if(pkt_list->pkt->spi_nospace)
						;//printf("spi recieve spi_nospace\n");
					else
						sem_post(&spiplc_para.sema);
					printf("recv  spi_rmt 0x%x index %d size %d\r\n", pkt_list->pkt->spi_rmt, pkt_list->pkt->spi_index,pkt_list->pkt->spi_size);
					//if(pSpiCb->port == pkt_list->pkt->plc_port)
					{
						pSpiCb->pfunc(pSpiCb->hdl, pkt_list->pkt->spi_rmt, pkt_list->pkt->spi_data, pkt_list->pkt->spi_size);
						break;
					}
					pSpiCb = pSpiCb->pnext;
				}
				pthread_mutex_unlock(&spiplc_para.spi_trans_mutex);
				//用完之后通知驱动把buf还回
				spi_release_readbuf((int*)pkt_list);

			}
			#endif
		}
	}
	printf("%s end\n",__func__);
}

static void *SpiPlcThread(void*param)
{
	spi_plc_chn spi_chn;
	pthread_t pid0;
	pkt_list_t * ptk_list = NULL;
	pkt_t * ptr = NULL;
	int count = 0;
	int i = 0;
	uint32_t starttime = 0;


	spi_chn.dev = 0x80;
	spi_chn.port = 0x0;

	spi_trans_register(spi_chn.port,SpiHander,(void*)&spi_chn);

	spiplc_para.start = 1;
	//sema_init(&spiplc_para.sema);
	sem_init (&spiplc_para.sema,0, 1);
	pthread_create(&pid0, NULL, SpiPlcRecvThread, NULL);

	while(spiplc_para.start) {
	#ifdef SLAVE_TEST
		sem_wait(&spiplc_para.sema);
	#endif
		{
			ptk_list = (pkt_list_t*)spi_get_freebuf_for_write();
			if(ptk_list == NULL)
			{
				//printf("%s spi_get_freebuf_for_write fail\n",__func__);
				#ifdef SLAVE_TEST
				usleep(wait_ms*1000);
				#endif
				#ifdef MASTER_TEST
				usleep(5000);
				#endif
				continue;
			}
			ptr = ptk_list->pkt;
			//向申请的buf ptr写入数据
			memset(ptr,0,sizeof(pkt_t));
			ptr->spi_cmd = TYPE_DATA;//TYPE_DATA;//TYPE_EMPTY;//TYPE_DATA;
			ptr->spi_rmt = spi_chn.dev;
			ptr->spi_size = PLC_SLICE_DATA_MAX;
			memcpy(ptr->spi_data,&count,4);
			//通知驱动发送
			if(spi_plc_write((int*)ptk_list) < 0)
			{
				printf("%s spi_plc_write fail\n",__func__);
				continue;
			}
			//usleep(wait_ms*1000);
			count++;
			if(count == 1)
				starttime = DPGetTickCount();
			if(count == 10001)
				printf("costtime %d\n",(DPGetTickCount() - starttime));
		}
	}

	printf("%s %d\n",__func__,__LINE__);
	pthread_join(pid0,NULL);

	printf("%s end\n",__func__);
}

int main( int argc, char *argv[] )
{
	pthread_t pid0;
	int buf_cnt = 8;
	uint32_t dev = 0x80;
	uint32_t param = 0x1;//主模式 0 从模式
	uint8_t cmd;
	int loglevel = 0;
	uint32_t type = 1;

	printf("test start\n");

	if(argc > 1)
		buf_cnt = atoi(argv[1]);
	if(argc > 2)
		loglevel = atoi(argv[2]);

	if(argc > 3)
		type = atoi(argv[3]);

	if(argc > 4)
		wait_ms = atoi(argv[3]);

	printf("buf_cnt %d loglevel %d type %d\n",buf_cnt,loglevel,type);


	if(enable_plc(&buf_cnt,dev) < 0)
	{
		printf("enable_plc fail\n");
		return -1;
	}
	spi_set_loglevel(&loglevel);

#if 0
	cmd = TYPE_SWITCH;
	param = type;
	printf("spi_set_ctrl TYPE_SWITCH\n");
	if(spi_set_ctrl(dev,cmd,param) < 0)
	{
		printf("switch type fail\n");
		goto end;
	}
	printf("spi type switch ok\n");
	//return 0;
#endif
#if 0
	cmd = TYPE_SET_AUDIO;
	if(spi_set_ctrl(dev,cmd,param) < 0)
	{
		printf("TYPE_SET_AUDIO fail\n");
		goto end;
	}
	printf("TYPE_SET_AUDIO ok\n");
#endif
	#if 0
	cmd = TYPE_SLOT;
	if(spi_set_ctrl(dev,cmd,param) < 0)
	{
		printf("switch type fail\n");
		goto end;
	}
	#endif

	pthread_mutex_init(&spiplc_para.spi_trans_mutex, NULL);

	pthread_create(&pid0, NULL, SpiPlcThread, NULL);

	getchar();
	getchar();
	spiplc_para.start = 0;
	printf("%s %d  spiplc_para.start %d\n",__func__,__LINE__,spiplc_para.start);
	pthread_join(pid0,NULL);
	pthread_mutex_destroy(&spiplc_para.spi_trans_mutex);
	//sema_destroy(&spiplc_para.sema);
end:

	disable_plc();
	printf("test end\n");
	return 0;
}


