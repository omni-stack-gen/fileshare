#ifndef  __SRC_MPI__
#define  __SRC_MPI__


#define IOC_TYPE_SRC 's'
#define IOCTL_SRC_CONVERT			_IOWR(IOC_TYPE_SRC,1,unsigned int)


extern int src_debug_log_level;
#define DEBUG_PRINTF(level,func,line,fmt, ...)\
do{\
	if(src_debug_log_level >= level)\
	{\
		printf("[mpi:src] %s line=%d "fmt"",func,line,##__VA_ARGS__);\
	}\
}while(0)

#define ERROR_PRINTF(func,line,fmt, ...)\
do{\
	printf("[mpi:src] %s line=%d err:"fmt"",func,line,##__VA_ARGS__);\
}while(0)


typedef struct {
	unsigned int freconver_mode;  //frequency conversion
	unsigned int single_voice_en;  //0:stereo;1:mono;
	unsigned int rx_FrameSize;  //mmzalloc rx dma buf size
	unsigned int tx_FrameSize;  //mmzalloc tx dma buf size
	unsigned long int rx_PhyAddr;  //mmzalloc rx dma buf phyaddr
	unsigned long int tx_PhyAddr;  //mmzalloc tx dma buf phyaddr
}src_cfg_t;


typedef struct {
	src_cfg_t src_cfg;
	unsigned char *rx_pVirAddr;  //mmzalloc rx dma buf viraddr
	unsigned char *tx_pVirAddr;  //mmzalloc tx dma buf viraddr
}src_cfg_user_t;

enum{
	CONVER_FREQ_8K_TO_48K = 1,
	CONVER_FREQ_12K_TO_48K,
	CONVER_FREQ_16K_TO_48K,
	CONVER_FREQ_24K_TO_48K,
	CONVER_FREQ_32K_TO_48K,
};

int MPI_SRC_Init(void);
int MPI_SRC_Start(src_cfg_t *src_cfg);
int MPI_SRC_Exit(void);

#endif
