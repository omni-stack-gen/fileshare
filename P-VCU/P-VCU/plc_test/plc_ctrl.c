#include "spi_plc.h"

#define PAGE_SIZE				4096
#define ALIGN(x, a)	(((x) + (a) - 1) & ~((a) - 1))

typedef struct {
   int fd;
   int buf_cnt;
   int map_size;
   char *ptrmap;//tx rx buf
   pkt_list_t * ptk_list;
}plc_ctrl_param;

static plc_ctrl_param plc_ctrl;

int spi_plc_open(void)
{
	if(plc_ctrl.fd < 0)
	{
		plc_ctrl.fd = open("/dev/spiplc_ctrl", O_RDWR|O_NDELAY);
		if(plc_ctrl.fd  < 0)
		{
			printf("open spiplc_ctrl fail\n");
			return -1;
		}
	}
	else
	{
		printf("plc dev already open\n");
		return -1;
	}

	return 0;
}

int spi_plc_close(void)
{
	if(plc_ctrl.fd  >= 0)
	{
		if(plc_ctrl.ptk_list)
		{
			free(plc_ctrl.ptk_list);
			plc_ctrl.ptk_list = NULL;
		}
		if(plc_ctrl.ptrmap && plc_ctrl.map_size)
		{
			munmap(plc_ctrl.ptrmap,plc_ctrl.map_size);
			plc_ctrl.ptrmap = NULL;
		}
		close(plc_ctrl.fd);
		plc_ctrl.fd = -1;
	}

	return 0;
}

//获取buf用来存放发送数据
int *spi_get_freebuf_for_write(void)
{
	int index;
	int *ptr = NULL;

	if(ioctl(plc_ctrl.fd,SPI_GET_FREE_BUF,&index) < 0)
	{
		//printf("SPI_GET_FREE_BUF fail\n");
		return NULL;
	}
	if(index < 0 ||  index >= plc_ctrl.buf_cnt)
	{
		printf("wrong index %d,something maybe wrong\n",index);
		return NULL;
	}

	ptr = (int*)(&plc_ctrl.ptk_list[index]);
	return ptr;
}

//通知驱动发送这个buf,发送完之后驱动会把这个buf还回，不需要应用控制
int spi_plc_write(int * ptr)
{
	pkt_list_t *pkt_ptr = (pkt_list_t*)ptr;
	int i = 0;
	int index = 0;

	index = pkt_ptr->index;

	if(index < 0 || index >= plc_ctrl.buf_cnt)
	{
		printf("%s index %d wrong \n",__func__,index);
		return -1;
	}


	if(ioctl(plc_ctrl.fd,SPI_WRITE_IOCTL,&index) < 0)
	{
		printf("start write fail\n");
		return -1;
	}

	return 0;
}

//获取接收buf
int *spi_plc_read(void)
{
	int index;
	int *ptr = NULL;


	if(ioctl(plc_ctrl.fd,SPI_READ_IOCTL,&index) < 0)
	{
		printf("start read fail\n");
		return NULL;
	}

	if(index < 0 || index >= plc_ctrl.buf_cnt)
	{
		printf("%s index %d wrong\n",__func__,index);
		return NULL;
	}

	ptr = (int*)(&plc_ctrl.ptk_list[index]);

	return ptr;
}

//用完buf之后，通知驱动把这个buf还回
int spi_release_readbuf(int *ptr)
{
	int i;
	int index;
	pkt_list_t *pkt_ptr = (pkt_list_t*)ptr;

	index = pkt_ptr->index;
	if(index < 0 || index >= plc_ctrl.buf_cnt)
	{
		printf("%s index %d wrong\n",__func__,index);
		return -1;
	}

	if(ioctl(plc_ctrl.fd,SPI_FREE_RECV_BUF,&index) < 0)
	{
		printf("start read fail\n");
		return -1;
	}
	return 0;
}

//获取当前驱动里的buf_cnt
int spi_get_bufcnt(int *bufcnt)
{
	if(ioctl(plc_ctrl.fd,SPI_GET_BUFCNT,bufcnt) < 0)
	{
		printf("spi_get_bufcnt fail\n");
		return -1;
	}

	plc_ctrl.buf_cnt = *bufcnt;
	return 0;
}

//设置buf_cnt 调用SPI_ALLOC_BUF之后不能修改。关闭/dev/spiplc_ctrl之后，再打开可以设置
int spi_set_bufcnt(int *bufcnt)
{
	if(ioctl(plc_ctrl.fd,SPI_SET_BUFCNT,bufcnt) < 0)
	{
		printf("spi_set_bufcnt fail\n");
		return -1;
	}
	plc_ctrl.buf_cnt = *bufcnt;
	return 0;
}

//通知驱动申请用来发送接收的缓冲buf
int spi_alloc_buf(void)
{
	int size = sizeof(pkt_list_t) * plc_ctrl.buf_cnt;

	plc_ctrl.ptk_list = malloc(size);
	if(plc_ctrl.ptk_list == NULL)
	{
		printf("%s malloc plc_ctrl.ptk_list fail\n",__func__);
		return -1;
	}

	if(ioctl(plc_ctrl.fd,SPI_ALLOC_BUF) < 0)
	{
		printf("spi_alloc_buf fail\n");
		free(plc_ctrl.ptk_list);
		plc_ctrl.ptk_list = NULL;
		return -1;
	}

	return 0;
}

//映射spi_alloc_buf申请的内存，避免不停的在内核和应用之间copy
int spi_plc_map(void)
{
	int i = 0;

	plc_ctrl.map_size = ALIGN(sizeof(pkt_t) * plc_ctrl.buf_cnt,PAGE_SIZE);
	plc_ctrl.ptrmap = (char *)mmap(0, plc_ctrl.map_size, PROT_READ | PROT_WRITE, MAP_SHARED, plc_ctrl.fd, 0);
	if(plc_ctrl.ptrmap == (char*)-1)
	{
		printf("mmap fail\n");
		return -1;
	}

	for(i = 0;i < plc_ctrl.buf_cnt;i++)
	{
		plc_ctrl.ptk_list[i].index = i;
		plc_ctrl.ptk_list[i].pkt = (pkt_t*)(plc_ctrl.ptrmap + i * sizeof(pkt_t));
	}

	return 0;
}

void spi_plc_unmap(void)
{
	int size = sizeof(pkt_t) * plc_ctrl.buf_cnt;

	munmap(plc_ctrl.ptrmap,plc_ctrl.map_size);
	plc_ctrl.ptrmap = NULL;
}

//spi ctrl
int spi_set_ctrl(uint32_t dev,uint8_t cmd,uint32_t param)
{
	spi_param spiparam;
	spiparam.dev = dev;
	spiparam.cmd = cmd;
	spiparam.param = param;
	if(ioctl(plc_ctrl.fd,SPI_CTRL,&spiparam) < 0)
	{
		printf("spi_set_ctrl fail\n");
		return -1;
	}
	return 0;
}

//设置plc驱动打印等级
int spi_set_loglevel(int *level)
{
	if(ioctl(plc_ctrl.fd,SPI_LOGLEVEL_IOCTL,level) < 0)
	{
		printf("spi_set_loglevel fail\n");
		return -1;
	}
	return 0;
}


//发送plc初始化命令
int spi_plc_init(uint8_t dev, uint8_t port)
{
	pkt_list_t *ptr_list = NULL;
	pkt_t *ptr = NULL;

	ptr_list = (pkt_list_t*)spi_get_freebuf_for_write();
	if(ptr_list == NULL)
	{
		printf("%s spi_get_freebuf_for_write fail\n",__func__);
		return -1;
	}

	ptr = ptr_list->pkt;
	memset(ptr,0,sizeof(pkt_t));
	ptr->spi_cmd = TYPE_INIT;//TYPE_EMPTY;//TYPE_DATA;
	ptr->spi_rmt = dev;
	ptr->spi_size = 0;
	if(spi_plc_write((int*)ptr_list) < 0)
	{
		printf("%s spi_plc_write fail\n",__func__);
		return -1;
	}

	return 0;
}


int enable_plc(int *bufcnt,uint8_t dev)
{
	int loglevel = 0x0;

	plc_ctrl.fd = -1;
	plc_ctrl.buf_cnt = bufcnt;
	plc_ctrl.map_size = 0;
	plc_ctrl.ptk_list = NULL;
	plc_ctrl.ptrmap = NULL;

	//打开plc设备
	if(spi_plc_open() < 0)
	{
		printf("%s spi_plc_open fail\n",__func__);
		return -1;
	}

	//设置plc驱动打印等级
	spi_set_loglevel(&loglevel);

	//设置驱动和应用之间共享的用来发送和接收的buf个数
	if(spi_set_bufcnt(bufcnt) < 0)
	{
		printf("%s spi_set_bufcnt %d fail\n",__func__);
		goto fail;

	}
	//获取设置的buf个数
	if(spi_get_bufcnt(bufcnt) < 0)
	{
		printf("%s spi_get_bufcnt fail\n",__func__);
		goto fail;
	}
	//申请内存
	if(spi_alloc_buf() < 0)
	{
		printf("%s spi_alloc_buf fail\n",__func__);
		goto fail;
	}

	//内存映射
	if(spi_plc_map() < 0)
	{
		printf("%s spi_plc_map fail\n",__func__);
		goto fail;
	}

	//发送plc初始化命令
	if(spi_plc_init(dev,0) < 0)
	{
		printf("%s spi_plc_init fail\n",__func__);
		goto fail;
	}

	return 0;

fail:
	spi_plc_close();
	return -1;
}


int disable_plc(void)
{
	spi_plc_close();
}

int get_spi_plc_fd(void)
{
	return plc_ctrl.fd;
}

