#include <pthread.h>
#include "DPBase.h"
#include <stdbool.h>
#include <netinet/in.h>
#include "DPDef.h"
#include "trans.h"
#include "list.h"
#define	dpmodel "[trans]"
// #include "../startup/DPLog.h"
#include <sys/socket.h>
#include <log.h>
#include <utils/mq.h>

#include "spi_plc/spi_plc_mgnt.h"
#include "spi_plc/spi_plc_trans.h"

#define		TRANS_MTU	(2032-5)//1976 //1984

#define SAFEFREE(p)  {if((p) != NULL){free(p);(p) = NULL;}}

unsigned int DPGetTickCount(void)
{
	struct timespec t_start;
	clock_gettime(CLOCK_MONOTONIC, &t_start);
	return (t_start.tv_sec * 1000 + t_start.tv_nsec / 1000000);
}

void DPSleep(unsigned int dwMilliseconds)
{
	struct timespec ts;
    ts.tv_sec  =  dwMilliseconds / 1000;
    ts.tv_nsec = (dwMilliseconds % 1000) * 1000000LL;

    nanosleep(&ts, NULL);
}

#pragma pack(1)

typedef struct {
	unsigned int type;		//0  请求数据      1 收到分片
	unsigned int data0;		//源ip地址     已收分片序号
	unsigned int data1;
} ST_TransCPacket;

typedef struct {
	unsigned int seq;		//分片序号
	unsigned char end;     //bit 0  分片起始        bit 1  分片结尾        bit  7  重传分片
	char data[];
} ST_TransPacket;

typedef struct {
	unsigned int type;		//0  请求重传      1 收到分片
	unsigned int count;		//分片总数
	unsigned int seqarray[10];		//分片序号组
}ST_ReReqPacket;

#pragma pack()

typedef struct {
	BaseListNode  listnode;
	int len;
	unsigned int tick;		//包创建时间
	unsigned int stick;		//包上次发送
	ST_TransPacket  tp;
} ST_Packet;

typedef struct {
	DWORD	dwlastreq;
	BOOL	btrans;
	HANDLE	hCRecvThread;
	HANDLE	hRSendThread;
	int  c;
	struct sockaddr_in disaddr;

	BaseList sendlist;
	HANDLE	hMutex;
	unsigned int sendseq;
	int cachelength;
	int cachecount;
	//重传的信号队列
	mq_t * resens_event_queue;
	// 是否启用重传
	bool bReReq;

	int cachetime;
	int cachemaxlength;

	unsigned int nnetsend;
	unsigned int nnetrsend;

	bool   bPlcTran;
	uint32_t dst_plcdev;		//目标PLC设备
	uint32_t dst_plcport;

	tran_drop_cb drop_callback;		//丢包重传回调
	void *drop_callback_param;		//丢包重传回调参数
} ST_TransPush;

typedef struct resend_event_msg
{
	uint8_t event;

	uint32_t seq;		//分片序号
	uint32_t dev;		//目标设备
} resend_event_msg_t;

HANDLE DPCreateThread(DWORD stacksize, const char *pname, void *(*func)(void *), void *arg)
{
	pthread_t *thread = (pthread_t *)malloc(sizeof(pthread_t));
	//pthread_t *thread = POOL_ALLOC(s_pool, pthread_t);
	if (thread)
	{
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, stacksize);
#if 0
		{
			pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
			int policy = pthread_attr_setschedpolicy(&attr, SCHED_RR);
			if (policy == 0)
			{
				struct sched_param sp;
				sp.sched_priority = sched_get_priority_min(SCHED_RR);
				printf("sched_priority:%d\n", sp.sched_priority);
				policy = pthread_attr_setschedparam(&attr, &sp);
			}
		}
#endif
		if (pthread_create(thread, &attr, func, (void *)arg) != 0)
		{
			SAFEFREE(thread);
			//pool_delete(s_pool, thread);
		}
		pthread_attr_destroy(&attr);
	}
	return thread;
}

BOOL DPCloseThread(HANDLE hThread)
{
	pthread_t *thread = (pthread_t *)hThread;
	BOOL bret;
	void *pret;

	if (thread == NULL)
		return TRUE;
	bret = (pthread_join(*thread, &pret) == 0);
	SAFEFREE(thread);
	//pool_delete(s_pool, thread);
	return bret;
}

HANDLE DPCreateMutex()
{
	pthread_mutex_t *plock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	memset(plock, 0, sizeof(pthread_mutex_t));

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(plock, &attr);
	pthread_mutexattr_destroy(&attr);

	return (HANDLE)plock;
}

void DPDeleteMutex(HANDLE hMutex)
{
	if (hMutex)
	{
		pthread_mutex_t *plock = (pthread_mutex_t *)hMutex;
		pthread_mutex_destroy(plock);
		free(plock);
		plock = NULL;
	}
}

void DPLockMutex(HANDLE hMutex)
{
	if (hMutex)
	{
		pthread_mutex_t *plock = (pthread_mutex_t *)hMutex;
		pthread_mutex_lock(plock);
	}
}

void DPUnlockMutex(HANDLE hMutex)
{
	if (hMutex)
	{
		pthread_mutex_t *plock = (pthread_mutex_t *)hMutex;
		pthread_mutex_unlock(plock);
	}
}

static int CreatUDPSocket(short lport)
{
	int c = socket (AF_INET, SOCK_DGRAM, 0);
	if(c == -1)
		return -1;
	struct sockaddr_in sin;
	memset (& sin, 0, sizeof (struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons (lport);
	sin.sin_addr.s_addr = INADDR_ANY;

	int on = 1;
	setsockopt(c, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	int ret = bind (c, (struct sockaddr *) & sin, sizeof (struct sockaddr_in));
	if(ret == -1)
	{
		printf("bind fail\r\n");
		close(c);
		return -1;
	}
	return c;
}

//接收到数据请求
static void TransPushOnReq(ST_TransPush * ptranspush,long srcip,long ip,short port)
{
	printf("TransPushOnReq:%p\r\n",ptranspush);
//	ptranspush->disaddr.sin_addr.s_addr = ip;
//	ptranspush->disaddr.sin_port = port;
//	if(srcip != ptranspush->disaddr.sin_addr.s_addr)
//		return;
	ptranspush->dwlastreq = DPGetTickCount();
}

//接收到分片ack
static void TransPushOnAck(ST_TransPush * ptranspush,unsigned int seq_ack,unsigned int seq_done)
{
	ST_Packet * ppacket;

//	printf("TransPushOnAck:%u %u\r\n",seq_ack,seq_done);

	DPLockMutex(ptranspush->hMutex);
	ppacket = (ST_Packet*)base_list_begin(&(ptranspush->sendlist));
	while(ppacket != (ST_Packet*)base_list_end(&(ptranspush->sendlist)))
	{
		if(ppacket->tp.seq >= seq_ack)
		{
			if(ppacket->tp.seq == seq_ack)
			{
				base_list_remove(&ppacket->listnode);
				ptranspush->cachelength -= ppacket->len;
				free(ppacket);

//				printf("push recv ack:%d local:%d\r\n",seq_ack,ptranspush->sendseq);
			}
			break;
		}
		ppacket = (ST_Packet*)(base_list_next(&ppacket->listnode));
	}

	//删除已处理的分片
	if(seq_done != 0xFFFFFFFE)
	{
		while(!base_list_empty(&(ptranspush ->sendlist)))
		{
			ppacket = (ST_Packet*)base_list_begin(&(ptranspush->sendlist));
			if(ppacket->tp.seq > seq_done)
				break;
//			printf("push remove done:%d %d local:%d\r\n",ppacket->tp.seq,seq_done,ptranspush->sendseq);

			base_list_remove(&ppacket->listnode);
			ptranspush->cachelength -= ppacket->len;
			free(ppacket);
		}
	}
	DPUnlockMutex(ptranspush->hMutex);
}

void * transpush_crecv_proc(void*param)
{
	ST_TransPush * ptrans = (ST_TransPush*)param;
	char recvBuf[64];
	ST_TransCPacket * pcPacket = (ST_TransCPacket*)recvBuf;
	struct sockaddr_in from;
	int addrlen,recvlen;
	printf("+++++transpush_crecv_proc++++\r\n");
	while(ptrans->btrans&&!ptrans->bPlcTran)
	{
		addrlen = sizeof(struct sockaddr_in);
		recvlen = recvfrom(ptrans->c, recvBuf, 64, 0,(struct sockaddr *)&from,(socklen_t*)&addrlen);
		if( recvlen == 0 || recvlen == -1)
		{
			printf("recvfrom error\r\n");
			DPSleep(50);
			continue;
		}

//		printf("recvfrom addr %x:%d\r\n",from.sin_addr.s_addr,ntohs(from.sin_port));
		if(pcPacket->type == 0)
			TransPushOnReq(ptrans,pcPacket->data0,from.sin_addr.s_addr,from.sin_port);
		else if(pcPacket->type == 1)
			TransPushOnAck(ptrans,pcPacket->data0,pcPacket->data1);
	}
	printf("-----transpush_crecv_proc----\r\n");
	return 0;
}

//重传线程
void * transpush_rsend_proc(void*param)
{
	ST_TransPush * ptrans = (ST_TransPush*)param;
	ST_Packet * ppacket;
	DWORD curtick;
	resend_event_msg_t event;
	printf("+++++transpush_rsend_proc++++\r\n");
	uint32_t resend_count = 0;
	uint32_t last_resend_seq = 0;

	tran_drop_info_s drop_info = {0};
	while(ptrans->btrans)
	{
#if 0
		DPLockMutex(ptrans->hMutex);

		curtick =  DPGetTickCount();

		//删除超过缓存时间的数据包
		while(!base_list_empty(&(ptrans ->sendlist)))
		{
			ppacket = (ST_Packet*)base_list_begin(&(ptrans ->sendlist));
			if(abs(curtick - ppacket->tick) < ptrans->cachetime)
			{
				if(curtick - ppacket->stick >= 200 && ptrans->disaddr.sin_addr.s_addr)
				{
//					printf("push rsend:%u\r\n",ppacket->tp.seq);
					ppacket->stick = curtick;
					ptrans->nnetrsend += ppacket->len;
					if(!ptrans->bPlcTran)
					sendto(ptrans->c,&ppacket->tp,ppacket->len,0,(struct sockaddr *)&ptrans->disaddr,sizeof (struct sockaddr_in));
				}
				break;
			}
//			printf("push remove old:%d %u %u\r\n",ppacket->tp.seq,curtick,ppacket->tick);
			base_list_remove(&ppacket->listnode);
			ptrans->cachelength -= ppacket->len;
			free(ppacket);
		}

		DPUnlockMutex(ptrans->hMutex);
		DPSleep(50);
#else
		memset(&event, 0, sizeof(resend_event_msg_t));
		if (0 == mq_recv(ptrans->resens_event_queue, &event, sizeof(resend_event_msg_t), 500))
		{
			resend_count++;
			last_resend_seq = event.seq;
			float drop_rate = ((float)resend_count / last_resend_seq) * 100;//(float)resend_count / (float)(last_resend_seq - ptrans->sendseq + 1);
			LOGW("Recv resend event:%d seq:%d resend_count:%d  round:%f\n", event.event, event.seq, resend_count, drop_rate);
			if(ptrans->drop_callback){
				memset(&drop_info, 0, sizeof(tran_drop_info_s));
				drop_info.remotedev = event.dev;
				drop_info.dropcount = resend_count;
				drop_info.recvcount = event.seq;
				drop_info.droprate = drop_rate;
				drop_info.puserdata = ptrans->drop_callback_param;
				ptrans->drop_callback(&drop_info);
			}

			DPLockMutex(ptrans->hMutex);
			ppacket = (ST_Packet*)base_list_back(&ptrans->sendlist);

			while (ppacket != (ST_Packet*)(base_list_end (&ptrans->sendlist)))
			{
				//查询是否存在该分片，有则发送
				if(ppacket->tp.seq == event.seq)
				{
					LOGI("Do send resend:%d seq:%d remotedev:0x%x  dstdev:0x%x\n", event.event, event.seq, event.dev, ptrans->dst_plcdev);
					spi_plc_mgnt_send(ptrans->dst_plcdev, ptrans->dst_plcport, (uint8_t*)&ppacket->tp,ppacket->len);
					break;
				}

				ppacket = (ST_Packet*)(base_list_previous (&ppacket->listnode));
			}

			DPUnlockMutex(ptrans->hMutex);
		}
		//else
		{
			DPLockMutex(ptrans->hMutex);

			curtick =  DPGetTickCount();
			//删除超过缓存时间的数据包
			while(!base_list_empty(&(ptrans ->sendlist)))
			{
				ppacket = (ST_Packet*)base_list_begin(&(ptrans ->sendlist));
				if(abs(curtick - ppacket->tick) < ptrans->cachetime)
				{
					break;
				}
	//			printf("push remove old:%d %u %u\r\n",ppacket->tp.seq,curtick,ppacket->tick);
				base_list_remove(&ppacket->listnode);
				ptrans->cachelength -= ppacket->len;
				ptrans->cachecount--;
				free(ppacket);
			}
			DPUnlockMutex(ptrans->hMutex);
		}
#endif
	}
	printf("-----transpush_rsend_proc : %d %d------\r\n", resend_count, last_resend_seq);
	return 0;
}

//收到重传请求
static void on_recv_rereq_event(void *param, uint32_t dev, uint8_t *data, uint16_t data_size)
{
	ST_TransPush *ptrans = (ST_TransPush *)param;

	//printf("on_recv_rereq_event:%p %d %d\n", ptrans, dev, data_size);

	resend_event_msg_t event = {0};
	event.event = 0;
	event.seq =  11;
	event.dev = dev;
	ST_ReReqPacket * pReReqPacket = (ST_ReReqPacket*)data;
	for (size_t i = 0; i < pReReqPacket->count; i++)
	{
		event.seq = pReReqPacket->seqarray[i];
		mq_send(ptrans->resens_event_queue, &event, sizeof(resend_event_msg_t));
	}
}

DPLAN_PUBLIC_API void * transpush_create(tran_push_info_s *pinfo)
{
	if(!pinfo)
	{
		printf("push create null param\r\n");
		return NULL;
	}
	printf("push create:%d %x:%d mf:%zu tp:%zu tcp:%zu\r\n", pinfo->lport,(unsigned int)pinfo->rip,
		pinfo->rport,sizeof(media_frame),sizeof(ST_TransPacket),sizeof(ST_TransCPacket));
	printf("Plc push dev:%x  port:%d \n", pinfo->plc_dev, pinfo->plc_port);
	//1.申请传输媒体资源
	ST_TransPush * ptrans = (ST_TransPush*)malloc(sizeof(ST_TransPush));
	if(ptrans == NULL)
		return NULL;
	// if(!pinfo->bplc_tran)
	// {
	// 	ptrans->c = CreatUDPSocket(pinfo->lport);
	// 	if(ptrans->c == -1)
	// 	{
	// 		printf("push create socket fail\r\n");
	// 		free(ptrans);
	// 		return NULL;
	// 	}
	// }
	// else{
	// 	printf("The Plc Tran\n");
	// }

	//置零
	memset(ptrans,0,sizeof(ST_TransPush));
	memset (&ptrans->disaddr, 0, sizeof (struct sockaddr_in));
	ptrans->nnetsend = 0;
	ptrans->nnetrsend = 0;
	ptrans->disaddr.sin_family = AF_INET;
	ptrans->disaddr.sin_port = htons (pinfo->rport);
	ptrans->disaddr.sin_addr.s_addr = pinfo->rip;
	ptrans->cachetime = 1000;
	ptrans->cachemaxlength = 2*1024*1024;
	ptrans->dst_plcdev = pinfo->plc_dev;
	ptrans->bPlcTran = pinfo->bplc_tran;
	ptrans->dst_plcport = pinfo->plc_port;

	//2.重传数据队列
	base_list_clear(&ptrans->sendlist);
	//3.创建互斥锁
	ptrans->hMutex = DPCreateMutex();
	ptrans->dwlastreq = DPGetTickCount();
	ptrans->sendseq = 0;
	ptrans->cachelength = 0;
	ptrans->cachecount = 0;
	ptrans->bReReq = pinfo->bretrans;
	ptrans->drop_callback = pinfo->dropcb;
	ptrans->drop_callback_param = pinfo->pdropuserdata;
	//4.创建重传消息队列
	ptrans->resens_event_queue = mq_create("resend_event_queue", sizeof(resend_event_msg_t), 64);
	ptrans->btrans = true;
	//5.创建非PLC接收线程
	if(!ptrans->bPlcTran)
		ptrans->hCRecvThread = DPCreateThread(204800,"pushcrecv", transpush_crecv_proc,(void*)ptrans/*, DP_PRIO_NORMAL*/);
	//6.创建重传线程
	ptrans->hRSendThread = DPCreateThread(204800,"pushrsend", transpush_rsend_proc,(void*)ptrans/*, DP_PRIO_NORMAL*/);
	//7.注册PLC接收重传包消息回调
	if(ptrans->bReReq)
		spi_plc_mgnt_register_callback(ptrans->dst_plcport+1, on_recv_rereq_event, ptrans);
	return (void*)ptrans;
}

void transpush_destroy(void  * p)
{
	ST_TransPush * ptrans = (ST_TransPush*)p;
	printf("push destroy:%p\r\n", ptrans);
	if(!ptrans || ptrans->btrans == false)
		return;
	ptrans->btrans = false;
	// if(ptrans->c != -1)
	// 	close(ptrans->c);
	//1.启用重传机制的情况下， 关闭重传接收
	if(ptrans->bReReq)
	{
		spi_plc_mgnt_unregister_callback(ptrans->dst_plcport+1);
	}
	//2.等待接收线程退出
	if(ptrans->hCRecvThread)
		DPCloseThread(ptrans->hCRecvThread);
	//3.等待重传线程退出
	if(ptrans->hRSendThread)
		DPCloseThread(ptrans->hRSendThread);
	//4.销毁互斥锁
	DPDeleteMutex(ptrans->hMutex);
	//5.销毁重传消息队列
	mq_destroy(ptrans->resens_event_queue);
	ST_Packet * ppacket;
	LOGI("push packet destroy free cachecount:%d\n", ptrans->cachecount);
	//6.释放发送缓存
	while(!base_list_empty(&(ptrans->sendlist)))
	{
		ppacket = (ST_Packet*)base_list_begin(&(ptrans->sendlist));
		base_list_remove(&ppacket->listnode);
		free(ppacket);
	}
	//7.释放传输资源
	free(ptrans);
}

bool transpush_reconnet(void *p, short lport)
{
	if (!p)
		return false;

	ST_TransPush *ptrans = (ST_TransPush*)p;

	shutdown(ptrans->c, SHUT_RDWR);
	close(ptrans->c);

	ptrans->c = CreatUDPSocket(lport);

	struct ip_mreq mreq; // 多播地址结构体
	mreq.imr_multiaddr.s_addr = htonl(0x0201A8C0 | 0xEF000000);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	int loop = 0;
	setsockopt(ptrans->c, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
	setsockopt(ptrans->c, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

	printf("++++++++++++++++++++++++transpush_reconnet sock: %d\r\n", ptrans->c);

	return true;
}

int transpush_check(void  * p)
{
	ST_TransPush * ptrans = (ST_TransPush*)p;
/*
	if(ptrans->disaddr.sin_addr.s_addr == 0)
	{
		printf("check push no disaddr\r\n");
		return -1;
	}
*/
	DWORD curtick =  DPGetTickCount();
	if(curtick - ptrans->dwlastreq >= 4000)
	{
		printf("check push req timeout:%p %u %u \r\n",ptrans,curtick,ptrans->dwlastreq);
		return -1;
	}

	return 0;
}

DPLAN_PUBLIC_API int transpush_send(void * p,unsigned char * lpdata,int dlen,bool bkey)
{
	ST_TransPush * ptrans = (ST_TransPush*)p;
	DPLockMutex(ptrans->hMutex);

	if(ptrans->cachelength + dlen > ptrans->cachemaxlength)
	{
		printf("transpush overflow:%p %d %d %d\r\n",ptrans,ptrans->cachelength,dlen,ptrans->cachemaxlength);
		DPUnlockMutex(ptrans->hMutex);
		return 0;
	}
	//printf("transpush_send:%d %x:%d %d %d\r\n",ptrans->sendseq,ptrans->disaddr.sin_addr.s_addr,ntohs(ptrans->disaddr.sin_port),dlen,bkey);

	int slen = 0;
	int tslen = 0;
	ST_Packet * ppacket;
	int ret = 0;
	while(tslen < dlen)
	{
		slen = dlen - tslen;
		if(slen > TRANS_MTU - sizeof(ST_TransPacket))
			slen = TRANS_MTU - sizeof(ST_TransPacket);

		ppacket = (ST_Packet*)malloc(sizeof(ST_Packet) + sizeof(ST_TransPacket) + slen);
		if(ppacket == NULL)
		{
			DPUnlockMutex(ptrans->hMutex);
			return -1;
		}
		ppacket->len = slen + sizeof(ST_TransPacket);
		memcpy(ppacket->tp.data,lpdata + tslen,slen);
		ppacket->tp.seq = ptrans->sendseq++;

		ppacket->tp.end = 0;
		if(tslen == 0)
			ppacket->tp.end = 1;
		tslen += slen;
		if(tslen == dlen)
			ppacket->tp.end += 2;
		if(bkey)
			ppacket->tp.end += 4;

		if((ptrans->disaddr.sin_addr.s_addr && ptrans->disaddr.sin_port) || ptrans->bPlcTran)
		{
			ptrans->nnetsend += ppacket->len;

//			if(ppacket->tp.seq%100 != 0)
			if(ptrans->bPlcTran)
			{
				// printf("----->send seq:%u len:%d end:%x  c: %02x %02x %02x %02x - %02x %02x %02x %02x\n", ppacket->tp.seq, ppacket->len, ppacket->tp.end,
				// ppacket->tp.data[0], ppacket->tp.data[1], ppacket->tp.data[2], ppacket->tp.data[3],
				// ppacket->tp.data[ppacket->len-4], ppacket->tp.data[ppacket->len-3], ppacket->tp.data[ppacket->len-2], ppacket->tp.data[ppacket->len-1]);
				spi_plc_mgnt_send(ptrans->dst_plcdev, ptrans->dst_plcport, (uint8_t*)&ppacket->tp,ppacket->len);
				ret = 0;
			}
			else
				ret = sendto(ptrans->c,&ppacket->tp,ppacket->len,0,(struct sockaddr *)&ptrans->disaddr,sizeof (struct sockaddr_in));

			// printf("push send:%d %d, ret:%d, errno:%d, errmsg:%s\r\n",ppacket->tp.seq,ppacket->len, ret, errno, strerror(errno));

			if (ret == -1 && errno == ENETUNREACH)
			{
				// 301011
				// int code = 301011;
				// int type = code / 100000; // 3
				// int index = code % 10; // 1
				// int rport = 10000 + type*100 + index; // 10000 + 300 + 1
				// long rip = 0;
				// GetRoomIP("0101", &rip);
				// ptrans->disaddr.sin_family = AF_INET;
				// ptrans->disaddr.sin_port = htons(rport);
				// ptrans->disaddr.sin_addr.s_addr = rip;

				// shutdown(ptrans->c, SHUT_RDWR);
				// close(ptrans->c);
				// ptrans->c = CreatUDPSocket(10101);
				// printf("push send recreate sockfd %d, errno :%d, errmsg :%s, 0x%x\r\n", ptrans->c, errno, strerror(errno), rip);
				// errno = 0;

				printf("push send fail, fd %d, errno :%d, errmsg :%s, 0x%x\r\n", ptrans->c, errno, strerror(errno), ptrans->disaddr.sin_addr.s_addr);
			}
/*
			nsend++;
			if(nsend >= 8)
			{
				nsend = 0;
				DPSleep(1);
			}
*/
		}
#if 1
		ppacket->stick = ppacket->tick = DPGetTickCount();
		ppacket->tp.end = ppacket->tp.end | (1<<7);
		base_list_insert (base_list_end (&(ptrans->sendlist)), ppacket);
		ptrans->cachelength += ppacket->len;
		ptrans->cachecount++;
#else
		free(ppacket);
#endif
	}

	DPUnlockMutex(ptrans->hMutex);
	return 0;
}

DPLAN_PUBLIC_API void transpush_setremote(void * p,long rip,short rport, uint32_t dev, uint32_t port)
{
	ST_TransPush * ptrans = (ST_TransPush*)p;
	printf("transpush_setremote:%p 0x%x:%d\r\n",ptrans,(unsigned int)rip,rport);
	ptrans->disaddr.sin_addr.s_addr = rip;
	ptrans->disaddr.sin_port = htons(rport);
	ptrans->dst_plcdev = dev;
	ptrans->dst_plcport = port;
	if(rip != 0)
		ptrans->dwlastreq = DPGetTickCount();
}

bool transpush_istrans(void * p)
{
	ST_TransPush * ptrans = (ST_TransPush*)p;
	return ptrans->disaddr.sin_addr.s_addr != 0;
}

void transpush_info(void * p)
{
	ST_TransPush * ptrans = (ST_TransPush*)p;
	printf("push net send (%d %d) bytes.\r\n",ptrans->nnetsend,ptrans->nnetrsend);
	ptrans->nnetsend = 0;
	ptrans->nnetrsend = 0;
}

typedef struct {
	int		itick;
	BOOL	btrans;
	HANDLE	hRecvThread;
	HANDLE	hRecvCheckThread;
	int  c;
	struct sockaddr_in disaddr;
	BaseList packetlist;
	HANDLE	hMutex;
	long  srcip;

	unsigned int recvseq;
	//上一次收到的包seq
	unsigned int lastrecvseq;
	// 是否启用重传
	bool bReReq;

	int nrecv;		//收到的包
	int nout;		//输出的包

	short lport;

	onRecvCb *cb;
	void *puserdata;
	sem_t  frame_sem;				//接收信号
} ST_TransPull;

// static void TransPullSendAck(ST_TransPull * ptrans,unsigned int seq, unsigned int seq_done)
// {
// 	ST_TransCPacket cPacket;
// 	cPacket.type = 1;
// 	cPacket.data0 = seq;
// 	cPacket.data1 = seq_done;
// 	sendto(ptrans->c,(char*)&cPacket,sizeof(ST_TransCPacket),0,(struct sockaddr *)&ptrans->disaddr,sizeof (struct sockaddr_in));
// }

static void TransPullSendReq(ST_TransPull * ptrans)
{
	ST_TransCPacket cPacket;
	cPacket.type = 0;
	cPacket.data0 = ptrans->srcip;
	sendto(ptrans->c, (char*)&cPacket,sizeof(ST_TransCPacket),0,(struct sockaddr *)&ptrans->disaddr,sizeof (struct sockaddr_in));
}

static void TransPullNew(ST_TransPull * ptrans)
{
	ST_Packet * ppacket;
	DPLockMutex(ptrans->hMutex);
	while (!base_list_empty(&(ptrans->packetlist)))
	{
		ppacket = (ST_Packet*)(base_list_begin(&(ptrans->packetlist)));
		base_list_remove(&ppacket->listnode);
		free(ppacket);
	}
	ptrans->recvseq = 0xFFFFFFFE;
	ptrans->lastrecvseq = 0;
	DPUnlockMutex(ptrans->hMutex);
}

//分片按从小到大顺序添加到列表
static bool TransPullAddList(BaseList * plist,ST_Packet * padd)
{
	ST_Packet * ppacket = (ST_Packet*)base_list_back(plist);

	while (ppacket != (ST_Packet*)(base_list_end (plist)))
	{
		if(ppacket->tp.seq < padd->tp.seq)
		{
			base_list_insertafter(&ppacket->listnode, &padd->listnode);
			return true;
		}
		else if(ppacket->tp.seq == padd->tp.seq)
		{
			LOGD("dorp same packet:%u \n", padd->tp.seq);
			return false;
		}
		ppacket = (ST_Packet*)(base_list_previous (&ppacket->listnode));
	}

	base_list_insert(base_list_begin(plist), padd);
	return true;
}

//拼装分片
static void BuildRecvPacket(ST_TransPull * ptrans,ST_Packet * pend)
{
	int len = 0;
	ST_Packet * p = (ST_Packet*)(base_list_begin(&ptrans->packetlist));
	//第一个必须为起始分片
	if((p->tp.end & 1) != 1)
		return;
	while(p != (ST_Packet*)(base_list_end(&ptrans->packetlist)))
	{
		len += p->len - sizeof(ST_TransPacket);
		if (p == pend)
			break;
		p = (ST_Packet*)(base_list_next(&p->listnode));
	}

	unsigned char * lpdata = (unsigned char*)malloc(len);
	if(lpdata)
	{
		unsigned char * lp = lpdata;
		p = (ST_Packet*)(base_list_begin(&ptrans->packetlist));
		while (p != (ST_Packet*)(base_list_end(&ptrans->packetlist)))
		{
			memcpy(lp,p->tp.data,p->len - sizeof(ST_TransPacket));
			lp += p->len - sizeof(ST_TransPacket);
			if (p == pend)
				break;
			p = (ST_Packet*)(base_list_next(&p->listnode));
		}
		ptrans->nout++;
		//transpull_onrecv(ptrans,lpdata,len);

		// media_frame *pframe = (media_frame *)lpdata;
		// printf("++++++++++++= len:%d  streamid:%d\n", len, pframe->streamid);
		if (ptrans->cb)
		{
			ptrans->cb(ptrans->puserdata, lpdata, len);
		}

		free(lpdata);
	}
}

//释放已完成拼装的分片
static void FreeRecvPacket(ST_TransPull * ptrans,ST_Packet * pend)
{
	ST_Packet * pfree;
	while(true)
	{
		pfree = (ST_Packet*)base_list_remove(base_list_begin(&ptrans->packetlist));
		free(pfree);

		if (pfree == pend)
		{
			//if (!base_list_empty(&ptrans->packetlist))
			//	TRACE(_T("free packet.front:%u end:%u\r\n"), ((ST_Packet*)base_list_front(&ptrans->packetlist))->tp.seq, ((ST_Packet*)base_list_front(&ptrans->packetlist))->tp.seq);
			//else
			//	TRACE(_T("free packet.NULL\r\n"));
			return;
		}
	}
}

static void TransPullOnRecv(ST_TransPull * ptrans,ST_TransPacket * ptp,int len, uint32_t srcdev)
{
	//printf("pull recv seq:%u len:%d end:%x c: %02x %02x %02x %02x - %02x %02x %02x %02x\r\n", ptp->seq, len, ptp->end, ptp->data[0], ptp->data[1], ptp->data[2], ptp->data[3], ptp->data[len-4], ptp->data[len-3], ptp->data[len-2], ptp->data[len-1]);
	//TransPullSendAck(ptrans, ptp->seq, ptrans->recvseq);

	if (ptrans->recvseq != 0xFFFFFFFE && ptp->seq <= ptrans->recvseq)
	{
		printf("dorp old packet:%u %u\r\n",ptp->seq,ptrans->recvseq);
		return;
	}

	if (!(ptp->end & (1 << 7)) && (ptp->seq + 100) < ptrans->recvseq && (ptrans->recvseq != 0xFFFFFFFE))
	{
		printf("Tran one more again, end:%d ptp->seq:%x  ptrans->recvseq:%x\n", (ptp->end & (1 << 7)), ptp->seq, ptrans->recvseq);
		//判断为新传输
		TransPullNew(ptrans);
	}

	//有重传请求，新的seq， 刷新记录
	if(ptrans->bReReq && ptrans->lastrecvseq < ptp->seq)
	{
		//收到的包不连续， 请求丢失的包
		if (ptrans->lastrecvseq && (ptp->seq != ptrans->lastrecvseq+1))
		{
			//LOGW("Need srcdev:%x to resend  packet:%u but:%u\r\n", srcdev, ptrans->lastrecvseq+1, ptp->seq);

			ST_ReReqPacket sendreq = {0};
			int drop_count = ptp->seq - ptrans->lastrecvseq - 1;
			//目前只支持最大连续10个包重传， 如果超过10个包，改视频帧基本无效了
			for (size_t i = 0; i < drop_count && i < 10; i++)
			{
				sendreq.seqarray[i] = ptrans->lastrecvseq + i + 1;
				sendreq.count = i + 1;
			}

			spi_plc_mgnt_send(srcdev, ptrans->lport+1, (uint8_t*)&sendreq, sizeof(sendreq));
		}
		ptrans->lastrecvseq = ptp->seq;
	}

	BOOL badd = false;
	ST_Packet * padd = (ST_Packet*)malloc(sizeof(ST_Packet) + len);
	if(padd == NULL)
		return;
	padd->len = len;
	memcpy(&padd->tp,ptp,len);
	padd->tick = DPGetTickCount();

	//按seq从小大顺序插入list
	DPLockMutex(ptrans->hMutex);
	if(ptrans->recvseq == 0xFFFFFFFE || padd->tp.seq > ptrans->recvseq)
		badd = TransPullAddList(&(ptrans->packetlist),padd);
	DPUnlockMutex(ptrans->hMutex);
	if(badd && (padd->tp.end & 2)){
		sem_post(&ptrans->frame_sem);
	}
	if(!badd)
		free(padd);
}

// void * transpull_recv_proc(void*param)
// {
// 	ST_TransPull * ptrans = (ST_TransPull*)param;
// 	char * pbuf = (char*)malloc(TRANS_MTU);
// 	struct sockaddr_in from;
// 	int addrlen,recvlen;
// 	printf("+++++transpull_recv_proc++++\r\n");

// 	while(ptrans->btrans)
// 	{
// 		addrlen = sizeof(struct sockaddr_in);
// 		recvlen = recvfrom(ptrans->c, pbuf, TRANS_MTU, 0,(struct sockaddr *)&from,(socklen_t*)&addrlen);
// 		if( recvlen == 0 || recvlen == -1)
// 		{
// 			if(recvlen < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EHOSTUNREACH))
// 				continue;
// 			printf("recvfrom error.%d\r\n",errno);
// 			break;
// 		}

// 		ptrans->nrecv++;
// 		TransPullOnRecv(ptrans,(ST_TransPacket*)pbuf,recvlen);
// 		ptrans->itick = 0;
// 	}

// 	free(pbuf);
// 	printf("-----transpull_recv_proc----\r\n");
// 	return 0;
// }

static unsigned int DeleteFirstPacket(BaseList * plist)
{
	ST_Packet * p;
	while (!base_list_empty(plist))
	{
		p = (ST_Packet*)(base_list_front(plist));
		if (p->tp.end & 1 && !(p->tp.end & (1 << 7)))
			return p->tp.seq - 1;
		printf("del seq:%x \n", p->tp.seq);
		base_list_remove(&p->listnode);
		free(p);
	}
	return 0xFFFFFFFE;
}

static unsigned int DeleteOnePacket(BaseList * plist, int *isend)
{
	//unsigned int ctick = DPGetTickCount();
	ST_Packet * pnext;
	ST_Packet * p = (ST_Packet*)(base_list_front(plist));
	while (true)
	{
		if (p->tp.end & 2)
		{
			*isend = 1;
			break;
		}
		pnext = (ST_Packet*)(base_list_next(&p->listnode));
		if (pnext == (ST_Packet*)(base_list_end(plist)) || p->tp.seq + 1 != pnext->tp.seq)
			break;
		// printf("111 ############[%u]  seq:%u  tick:%d len:%d end:%x c:%02x %02x %02x %02x - %02x %02x %02x %02x\n", ctick, p->tp.seq, p->tick, p->len, p->tp.end,
		// 	p->tp.data[0], p->tp.data[1], p->tp.data[2], p->tp.data[3],
		// 	p->tp.data[p->len-4], p->tp.data[p->len-3], p->tp.data[p->len-2], p->tp.data[p->len-1]);
		base_list_remove(&p->listnode);
		free(p);
		p = pnext;
	}

	// printf("222 ############[%u:%d]  seq:%u  tick:%d len:%d end:%x c:%02x %02x %02x %02x - %02x %02x %02x %02x\n", ctick, *isend, p->tp.seq, p->tick, p->len, p->tp.end,
	// 	p->tp.data[0], p->tp.data[1], p->tp.data[2], p->tp.data[3],
	// 	p->tp.data[p->len-4], p->tp.data[p->len-3], p->tp.data[p->len-2], p->tp.data[p->len-1]);
	unsigned int lastseq = p->tp.seq;
	base_list_remove(&p->listnode);
	free(p);
	return lastseq;
}

static  ST_Packet * FindEnd(BaseList * plist, unsigned int seq,ST_Packet * p)
{
	ST_Packet * pnext;
	ST_Packet * pend = p;
	if (pend == NULL)
	{
		p = (ST_Packet*)(base_list_front(plist));
		if ((seq + 1) != p->tp.seq)
			return NULL;
		pend = p;
	}

	while(true)
	{
		if (pend->tp.end & 2)
			break;
		pnext = (ST_Packet*)(base_list_next(&pend->listnode));
		if (pnext == (ST_Packet*)(base_list_end(plist)) || pend->tp.seq + 1 != pnext->tp.seq)
			break;
		pend = pnext;
	}
	return pend;
}

void * transpull_check_proc(void * param)
{
	ST_TransPull * ptrans = (ST_TransPull*)param;
	unsigned int curtick;
	ST_Packet * pfind = NULL;
	ST_Packet * p;
	printf("+++++transpull_check_proc++++\r\n");
	int isend = 0;
	while (ptrans->btrans)
	{
		sem_wait(&ptrans->frame_sem);
		DPLockMutex(ptrans->hMutex);

		//开始时丢弃非起始分片
		if (ptrans->recvseq == 0xFFFFFFFE)
		{
			ptrans->recvseq = DeleteFirstPacket(&ptrans->packetlist);
			//printf("recv seq:%x\n", ptrans->recvseq);
		}
		if (ptrans->recvseq == 0xFFFFFFFE)
		{
			DPUnlockMutex(ptrans->hMutex);
			DPSleep(50);
			continue;
		}

		curtick = DPGetTickCount();
		//删除500ms未接收完的帧
		while (!base_list_empty(&(ptrans->packetlist)))
		{
			p = (ST_Packet*)(base_list_front(&ptrans->packetlist));
			if (curtick - p->tick < 500)
				break;

			//printf("delete old packet.seq:%u tick:%u %u %u\r\n", p->tp.seq,curtick, p->tick, curtick - p->tick);
			isend = 0;
			ptrans->recvseq = DeleteOnePacket(&ptrans->packetlist, &isend);
			pfind = NULL;
			if (isend)
			{
				//printf("out of while \n");
				break;
			}
		}

		//检查收包完整
		while (!base_list_empty(&(ptrans->packetlist)))
		{
			//printf("FindEnd:%u 0x%x\r\n", ptrans->recvseq, pfind);

			pfind = FindEnd(&(ptrans->packetlist), ptrans->recvseq, pfind);
			if (pfind == NULL)
				break;

			ptrans->recvseq = pfind->tp.seq;

//			printf("recv seq:%u 0x%x %x\r\n", ptrans->recvseq,pfind, pfind->listnode.next);

			if (!(pfind->tp.end & 2))
				break;

			//拼包
			BuildRecvPacket(ptrans, pfind);
			//释放分片
			FreeRecvPacket(ptrans, pfind);

			pfind = NULL;
		}

		DPUnlockMutex(ptrans->hMutex);
		//DPSleep(50);
	}
	printf("-----transpull_check_proc port:0x%x ----\r\n", ptrans->lport);
	return 0;
}

static void on_recv_media(void *param, uint32_t dev, uint8_t *data, uint16_t data_size)
{
	ST_TransPull *ptrans = (ST_TransPull *)param;

	ptrans->nrecv++;
	TransPullOnRecv(ptrans,(ST_TransPacket*)data, data_size, dev);
	ptrans->itick = 0;
}

DPLAN_PUBLIC_API void * transpull_create(short lport,long rip,short rport,long srcip, onRecvCb *cb, void *puserdata, bool bretrans)
{
	printf("transpull_create.lport:%d rip:%x:%d srcip:%x\r\n",lport,(unsigned int)rip,rport,(unsigned int)srcip);

	ST_TransPull * ptrans = (ST_TransPull*)malloc(sizeof(ST_TransPull));
	if(ptrans == NULL)
		return NULL;
	memset(ptrans, 0 , sizeof(ST_TransPull));
	// ptrans->c = CreatUDPSocket(lport);
	// if(ptrans->c == -1)
	// {
	// 	printf("pull create socket fail\r\n");
	// 	free(ptrans);
	// 	return NULL;
	// }
	ptrans->c = -1;

	struct timeval tv;
	tv.tv_sec  = 0;
	tv.tv_usec = 50000;
	setsockopt(ptrans->c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	memset (&ptrans->disaddr, 0, sizeof (struct sockaddr_in));
	ptrans->disaddr.sin_family = AF_INET;
	ptrans->disaddr.sin_port = htons (rport);
	ptrans->disaddr.sin_addr.s_addr = rip;

	sem_init(&ptrans->frame_sem, 0, 0); //无名信号量

	base_list_clear(&ptrans->packetlist);
	ptrans->hMutex = DPCreateMutex();
	ptrans->srcip = srcip;
	ptrans->itick = 0;
	ptrans->recvseq = 0xFFFFFFFE;
	ptrans->lastrecvseq = 0;
	ptrans->bReReq = bretrans;
	ptrans->btrans = true;
	ptrans->cb = cb;
	ptrans->puserdata = puserdata;
	ptrans->nrecv = 0;
	ptrans->nout = 0;
	ptrans->hRecvThread = NULL;//DPCreateThread(204800,"pullrecv", transpull_recv_proc,(void*)ptrans/*, DP_PRIO_NORMAL*/);
	ptrans->hRecvCheckThread = DPCreateThread(204800,"pullcheck", transpull_check_proc,(void*)ptrans/*, DP_PRIO_NORMAL*/);
	ptrans->lport = lport;

	spi_plc_mgnt_register_callbackEx(lport, rip, on_recv_media, ptrans);
	//TransPullSendReq(ptrans);
	return (void*)ptrans;

}

DPLAN_PUBLIC_API void transpull_destroy(void  * p)
{
	ST_TransPull * ptrans = (ST_TransPull*)p;
	printf("destroy0.%d  port:0x%x  devaddr:0x%x\n",ptrans->c, ptrans->lport, ptrans->disaddr.sin_addr.s_addr);
	if(ptrans->btrans == false)
		return;

	//spi_plc_mgnt_unregister_callback(ptrans->lport);
	spi_plc_mgnt_unregister_callbackEx(ptrans->lport, ptrans->disaddr.sin_addr.s_addr);
	ptrans->btrans = false;
	sem_post(&ptrans->frame_sem);

	if(ptrans->hRecvThread)
		DPCloseThread(ptrans->hRecvThread);
	if(ptrans->hRecvCheckThread)
		DPCloseThread(ptrans->hRecvCheckThread);
	sem_destroy(&ptrans->frame_sem);
	// if(ptrans->c!=-1)
	// close(ptrans->c);
	DPDeleteMutex(ptrans->hMutex);
	ST_Packet * ppacket;
	while(!base_list_empty(&(ptrans->packetlist)))
	{
		ppacket = (ST_Packet*)(base_list_begin(&(ptrans->packetlist)));
		base_list_remove(&ppacket->listnode);
		free(ppacket);
	}
	printf("destroy6\r\n");
	free(ptrans);
}

//每秒向源地址发送请求包，并检查本地是否收到包
int transpull_check(void  * p)
{
	ST_TransPull * ptrans = (ST_TransPull*)p;
	printf("transpull.r:%d o:%d t:%d\r\n",ptrans->nrecv,ptrans->nout,ptrans->itick);
	TransPullSendReq(ptrans);
	ptrans->itick++;
	if(ptrans->itick > 4)
		return -1;
	return 0;
}

long transpull_getremote(void * p)
{
	ST_TransPull * ptrans = (ST_TransPull*)p;
	return ptrans->disaddr.sin_addr.s_addr;
}

void transpull_set_restart(void *p)
{
	if (p == NULL)
	{
		printf("invalid handler\n");
		return;
	}
	ST_TransPull *ptrans = (ST_TransPull*)p;
	TransPullNew(ptrans);
}