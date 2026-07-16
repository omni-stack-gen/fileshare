#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/kernel.h>


#define	PLC_SLICE_DATA_MAX		1984

//#define SPI_PLC_BUFSIZE  4096
#define SPI_WRITE_IOCTL 		0x100
#define SPI_READ_IOCTL			0x101
#define SPI_LOGLEVEL_IOCTL		0x102
#define SPI_GET_FREE_BUF        0x103
#define SPI_FREE_RECV_BUF		0x104
#define SPI_GET_BUFCNT		    0x105
#define SPI_SET_BUFCNT			0x106
#define SPI_ALLOC_BUF           0x107
#define SPI_CTRL				0x108

#define PAGE_SIZE				4096
#define ALIGN(x, a)	(((x) + (a) - 1) & ~((a) - 1))

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

typedef enum
{
    TYPE_EMPTY               = 0x00,
    TYPE_INIT                = 0x01,             // 获取当前版本号
    TYPE_SYNC                = 0x02,             // 同步数据
    TYPE_DATA                = 0x03,             // 音视频数据
    TYPE_CONFICT             = 0x04,             // 冲突
    TYPE_DATA_TEST           = 0x05,
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
    uint8_t  spi_data[2032];    // 16
} pkt_t;    // 2048


typedef struct _pkt_list_t_
{
	int index;
	pkt_t *pkt;
} pkt_list_t;


int enable_plc(int *bufcnt,uint8_t dev);
int disable_plc(void);
int get_spi_plc_fd(void);
int *spi_get_freebuf_for_write(void);
int spi_plc_write(int * ptr);
int *spi_plc_read(void);
int spi_release_readbuf(int *ptr);
int spi_set_ctrl(uint32_t dev,uint8_t cmd,uint32_t param);
