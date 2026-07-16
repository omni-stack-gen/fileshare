#ifndef DEBUG_INTERFACE_H_
#define DEBUG_INTERFACE_H_

/**
* FH_DBI_Version
*@brief 按SDK的统一格式定义的版本信息
*@param [in] print_enable:是否打印版本信息
*@param [out] 无
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
char *FH_DBI_Version(unsigned int print_enable);

typedef int (*dbi_send)(void* obj, unsigned char *buf, int size);
typedef int (*dbi_recv)(void* obj, unsigned char *buf, int size);
typedef int (*dbi_user_write)(unsigned int grpidx,int intMemSlot, unsigned int offset, unsigned int *pstData);
typedef int (*dbi_user_read)(unsigned int grpidx,int intMemSlot, unsigned int offset, unsigned int *pstData);

typedef enum
{
	DBI_GET_DATA  		= 0xa1,	// 回调需要传递的一个参数需要是数据源的地址，第二个参数是数据长度(byte)
	DBI_SEND_DATA		= 0xa3, // 同上
	RESERVED_CMD0 		= 0xa5, // 保留
	RESERVED_CMD1 		= 0xa6,
	RESERVED_CMD2 		= 0xa7,
}DBI_CB_E;

typedef struct DI_config
{
	void *		obj;
	dbi_send	send;
	dbi_recv	recv;
	dbi_user_write write;
	dbi_user_read  read;

}DI_config;

struct Debug_Interface;

int DI_handle(struct Debug_Interface* di);

int DI_destroy(struct Debug_Interface *di);

/* object create */
struct Debug_Interface * DI_create(struct DI_config *cfg);

int DI_RegisterCallBack(DBI_CB_E enCmd, int (*func)(int *para));

#endif

