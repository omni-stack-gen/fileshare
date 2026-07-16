#ifndef __TRANS_H__
#define __TRANS_H__
#include <stdint.h>

//                        转发服务器         					   室内机                            			   					    门口机
//视频                   推流服务:1233     拉流服务:1234          拉取 本地:10100 对方:转发服务器@1234             源地址:门口机IP              推送   本地:1234   对方:转发服务器@1233
//音频门口机到室内机            推流服务:1233     拉流服务:1234          拉取 本地:10100 对方:转发服务器@1234             源地址:门口机IP              推送   本地:1234   对方:转发服务器@1233
//音频室内机到门口机            推流服务:1233     拉流服务:1234          推送 本地:10200     对方:转发服务器@1233        					     拉取 本地:1230     对方:转发服务器@1234         源地址:室内机IP

#define		TRANS_PULLSERVER_PORT		1234
#define		TRANS_PUSHSERVER_PORT		1233

#define		LOCAL_PUSH_PORT				1234
#define		LOCAL_PULL_PORT				1230

#pragma pack(1)

typedef struct
{
	unsigned int  check;		//0xabcd1234
	unsigned int  timestamp;	//时间戳
	unsigned char streamid;		//0  video  1 audio	  2 subvideo 3 抓拍
	unsigned char seq;			//包序号
	unsigned char bkey;			//是否关键帧
	char data[];				//帧数据
} media_frame;

#pragma pack()

typedef struct
{
	uint32_t remotedev;		//对端设备号
	uint32_t dropcount;		//丢包数
	uint32_t recvcount;		//收包数
	float droprate;			//丢包率
	void *puserdata;	    //用户数据
} tran_drop_info_s;
/**
 * @brief 转发丢包回调函数
 */
typedef int (*tran_drop_cb)(tran_drop_info_s *pinfo);


typedef void(onRecvCb)(void *p, unsigned char *lpdata, int dlen);

int mediatrans_send(int channel,media_frame * pframe,int dlen);

typedef struct{
	short lport;			//本地端口 UDP/TCP 时才用到
	long rip;			//对端IP
	short rport;			//对端端口
	uint32_t plc_dev;		//PLC设备号
	uint32_t plc_port;			//PLC端口
	bool bplc_tran;			//是否PLC转发
	bool bretrans;			//是否有重传

	tran_drop_cb dropcb;	//丢包回调
	void *pdropuserdata;	//自定义数据
}tran_push_info_s;

void * transpush_create(tran_push_info_s *pinfo);
void transpush_destroy(void  * p);
bool transpush_reconnet(void *p, short lport);
int transpush_check(void  * p);
int transpush_send(void * p,unsigned char * lpdata,int dlen,bool bkey);
void transpush_setremote(void * p,long rip,short rport, uint32_t dev, uint32_t port);
bool transpush_istrans(void * p);
void transpush_info(void * p);



void * transpull_create(short lport,long rip,short rport,long srcip, onRecvCb *cb, void *puserdata, bool bretrans);
void transpull_destroy(void  * p);
int transpull_check(void  * p);
void transpull_onrecv(void * p,unsigned char * lpdata,int dlen);
long transpull_getremote(void * p);

#endif

