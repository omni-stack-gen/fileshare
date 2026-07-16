#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#include "dsp/fh_ai_mpi.h"
#include "dsp/fh_ao_mpi.h"
#include "dsp/fh_adec_mpi.h"
#include "dsp/fh_aenc_mpi.h"
#include "types/vmm_api.h"

#include <utils/circlebuf.h>
#include <utils/log.h>
#include <utils/util.h>
#include <utils/bmem.h>
#include <utils/time_helper.h>

#define ID_RIFF		0x46464952
#define ID_WAVE		0x45564157
#define ID_FMT		0x20746d66
#define ID_DATA		0x61746164

typedef struct {
    unsigned int riff_id;
    unsigned int riff_sz;
    unsigned int riff_fmt;
    unsigned int fmt_id;
    unsigned int fmt_sz;
    unsigned short audio_format;
    unsigned short num_channels;
    unsigned int sample_rate;
    unsigned int byte_rate;
    unsigned short block_align;
    unsigned short bits_per_sample;
    unsigned int data_id;
    unsigned int data_sz;
} wav_header_t;

typedef struct
{
	FILE*			pFile;
	FH_BOOL			bSave;
	FH_UINT32		u32RdSize;
	FH_UINT32		u32FsRate;
	FH_UINT32		u32BitWidth;
	FH_UINT32		u32SndMode;
	FH_UINT32		u32PhyAddr;
	FH_UINT32		u32ExpID;
	FH_VOID*		pVirAddr;
}AUDIO_TST_FILE_S;

typedef struct
{
	FH_BOOL			bUse;
	FH_SINT32		s32Dev;
	FH_SINT32		s32Chn;
}AUDIO_TST_DEV_S;

typedef struct
{
	FH_SINT32		s32Type;
	pthread_t		stPid;
	volatile FH_BOOL	bStart;
	AUDIO_TST_DEV_S		stAiDev;
	AUDIO_TST_DEV_S		stAoDev;
	AUDIO_TST_DEV_S		stAencDev;
	AUDIO_TST_DEV_S		stAdecDev;
	AUDIO_TST_FILE_S	stAiFile;
	AUDIO_TST_FILE_S	stAoFile;
	AUDIO_TST_FILE_S	stAencFile;
	AUDIO_TST_FILE_S	stAdecFile;
	PAYLOAD_TYPE_E		enType;
}AUDIO_TST_S;


typedef struct _ADTS_HEADER_s
{
	FH_UINT16	syncWord:12;
	FH_UINT16	id:1;
	FH_UINT16	layer:2;
	FH_UINT16	protect_absent:1;
	FH_UINT16	profile:2;
	FH_UINT16	sample_freq:4;
	FH_UINT16	private:1;
	FH_UINT16	channel_config:3;
	FH_UINT16	origin_copy:1;
	FH_UINT16	home:1;
	FH_UINT16	copyright_indentify_b:1;
	FH_UINT16	copyright_indentify_s:1;
	FH_UINT16	aac_frame_len:13;
	FH_UINT16	adts_buffer:11;
	FH_UINT16	number_of_rawdata:2;
}ADTS_HEADER_S;

#define AUDIO_PRT(fmt,...)   \
	do {\
		printf("[%s]#%d "fmt"\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
	}while(0)

#define SAMPLE_PS		25
#define SAMPLE_BITS		AUDIO_BIT_WIDTH_16
#define GET_RATION(x,y)		x/y
#define AI_CHN_MODE		AIO_MODE_I2S_SLAVE
#define AO_CHN_MODE		AIO_MODE_I2S_SLAVE
#define AI_SAMPLE_RATE		AUDIO_SAMPLE_RATE_8000
#define AO_SAMPLE_RATE		AUDIO_SAMPLE_RATE_8000
#define HDMI_SAMPLE_RATE	AUDIO_SAMPLE_RATE_48000
#define A_FRM_SIZE(fs,bit,num)	(fs/num)*(bit/8)
#define AAC_FRAME_SIZE		2048

static AUDIO_TST_S g_stAudTest={0};
static int isDimcStero=FH_FALSE;
// static int isCodecStereo=FH_FALSE;
static int foffset = 0;

static bool iswavfile = false;

static struct circlebuf audio_packet;
static pthread_mutex_t audio_mutex;

static int g_dwAoVolume = 0x80;

FH_VOID SAMPLE_ParseAacHeader(FH_UINT8* pData,FH_VOID* pHeader)
{
	ADTS_HEADER_S* p=(ADTS_HEADER_S*)pHeader;
	FH_UINT16 temp=0;

	p->syncWord = pData[0];
	p->syncWord <<= 4;
	p->syncWord |= (pData[1]>>4)&0xF;
	//p->syncWord = pData[0]<<4 | (pData[1]>>4)&0xF;
	p->id = (pData[1]>>3)&0x1;
	p->layer = (pData[1]>>1)&0x3;
	p->protect_absent = pData[1]&0x1;
	p->profile = (pData[2]>>6)&0x3;
	p->sample_freq = (pData[2]>>2)&0xF;
	p->private = (pData[2]>>1)&0x1;
	//p->channel_config = (pData[2]&0x1)<<2 | (pData[3]>>6)&0x3;
	p->channel_config = pData[2]&0x1;
	p->channel_config <<= 2;
	p->channel_config |= (pData[3]>>6)&0x3;
	p->origin_copy = (pData[3]>>5)&0x1;
	p->home = (pData[3]>>4)&0x1;
	p->copyright_indentify_b = (pData[3]>>3)&0x1;
	p->copyright_indentify_s = (pData[3]>>2)&0x1;
	//p->aac_frame_len = (pData[3]&0x3)<<11 | pData[4]<<3 | (pData[5]>>5)&0x3;
	temp = pData[3]&0x3;
	temp <<= 11;
	p->aac_frame_len = temp;
	temp = pData[4];
	temp <<= 3;
	p->aac_frame_len |= temp;
	p->aac_frame_len |= (pData[5]>>5)&0x7;
	//p->adts_buffer = (pData[5]&0x1F)<<6 | (pData[6]>>2)&0x3F;
	p->adts_buffer = pData[5]&0x1F;
	p->adts_buffer <<= 6;
	p->adts_buffer |= (pData[6]>>2)&0x3F;
	p->number_of_rawdata = pData[6]&0x3;

	//printf("[%s]syw 0x%x abs %d sample %d len %d\n",__func__,p->syncWord,p->protect_absent,p->sample_freq,p->aac_frame_len);
}

void *SAMPLE_AUDIO_Proc(void *ptr)
{
	FH_SINT32 s32Ret=0;
	AUDIO_TST_S* pAtr=(AUDIO_TST_S*)ptr;
	AUDIO_FRAME_S stFrame={0};
	AUDIO_STREAM_S stream={0};
	static int cnt=1000;
	ADTS_HEADER_S adts={0};

	//AUDIO_PRT("Dev%d-Chn%d bEn%d enter", pAtr->stAiDev.s32Dev,pAtr->stAiDev.s32Chn,pAtr->bStart);
	while(pAtr->bStart)
	{
		memset(&stFrame,0,sizeof(stFrame));
		memset(&stream,0,sizeof(stream));
		if(FH_TRUE == pAtr->stAiDev.bUse)
		{
            uint32_t time_ms_last = time_relative_ms();
			/* get frame from ai chn */
			s32Ret = FH_AI_GetChnFrame(pAtr->stAiDev.s32Dev, pAtr->stAiDev.s32Chn, &stFrame, NULL, 1000);
            uint32_t time_ms = time_relative_ms();
            usleep(20*1000);
			if (FH_SUCCESS != s32Ret )
			{
				AUDIO_PRT("Dev%d-Chn%d failed with %#x!", pAtr->stAiDev.s32Dev,pAtr->stAiDev.s32Chn,s32Ret);
				usleep(10*1000);
				continue;
			}
			if((!isDimcStero) && IS_DMIC_ID(pAtr->stAiDev.s32Dev))
			{
				AUDIO_FRAME_S stFrame1;
				s32Ret = FH_AI_GetChnFrame(pAtr->stAiDev.s32Dev, 1, &stFrame1, NULL, 1000);
				if (FH_SUCCESS != s32Ret )
				{
					AUDIO_PRT("Dev%d-Chn%d failed with %#x isDimcStero %d!", pAtr->stAiDev.s32Dev,1,s32Ret,isDimcStero);
				}
			}

			AUDIO_PRT("<%u:%u>Dev%d-Chn%d bSave%d u32Len=%d!",time_ms, time_ms-time_ms_last, pAtr->stAiDev.s32Dev,pAtr->stAiDev.s32Chn,pAtr->stAiFile.bSave,stFrame.u32Len);
			if(FH_TRUE == pAtr->stAiFile.bSave)
			{
				//AUDIO_PRT("[DATA] len=0x%x,data=%x-%x\n",stFrame.u32Len,((char*)stFrame.pVirAddr)[0],((char*)stFrame.pVirAddr)[1]);
				/* save audio stream to file */
				s32Ret = fwrite(stFrame.pVirAddr, 1, stFrame.u32Len, pAtr->stAiFile.pFile);
			}
		}

		/* send frame to encoder */
		if(FH_TRUE == pAtr->stAencDev.bUse)
		{
			s32Ret = FH_AENC_SendFrame(pAtr->stAencDev.s32Dev,&stFrame,40);
			if(s32Ret != FH_SUCCESS)
			{
				continue;
			}
			s32Ret = FH_AENC_GetStream(pAtr->stAencDev.s32Dev,&stream,40);
			if(s32Ret != FH_SUCCESS)
			{
				continue;
			}
			if (FH_TRUE == pAtr->stAencFile.bSave)
			{
				/* save audio stream to file */
				s32Ret = fwrite(stream.pStream, 1, stream.u32Len, pAtr->stAencFile.pFile);
			}
		}

		if(pAtr->stAdecFile.pFile && pAtr->stAdecFile.u32RdSize)
		{
			FH_UINT32 u32ReadLen=0;

			if(!pAtr->stAdecFile.pVirAddr)
			{
				s32Ret = FH_SYS_VmmAlloc(&pAtr->stAdecFile.u32PhyAddr, &pAtr->stAdecFile.pVirAddr, "SMP_ADEC_FILE", NULL,pAtr->stAdecFile.u32RdSize);
				if(s32Ret != FH_SUCCESS)
				{
					AUDIO_PRT("Dev%d-Chn%d failed with %#x!", pAtr->stAdecDev.s32Dev,pAtr->stAdecDev.s32Chn,s32Ret);
					usleep(10*1000);
					continue;
				}
			}
			stream.pStream = pAtr->stAdecFile.pVirAddr;
			stream.u32PhyAddr = pAtr->stAdecFile.u32PhyAddr;
			stream.u32Len = pAtr->stAdecFile.u32RdSize;
			stream.enFsRate = pAtr->stAdecFile.u32FsRate;
			stream.enBitwidth = pAtr->stAdecFile.u32BitWidth;
			stream.enSoundmode = pAtr->stAdecFile.u32SndMode;
			u32ReadLen = fread(stream.pStream, 1, stream.u32Len, pAtr->stAdecFile.pFile);
			if (u32ReadLen <= 0)
			{
				fseek(pAtr->stAdecFile.pFile, 0, SEEK_SET);/*read file again*/
				//printf("\033[32m[read-end]u32Len %d\033[0m\n",u32ReadLen);
				continue;
			}
			if(g_stAudTest.enType == PT_AAC)
			{
				fseek(pAtr->stAdecFile.pFile, -u32ReadLen, SEEK_CUR);
				SAMPLE_ParseAacHeader(stream.pStream,&adts);
				if(adts.syncWord != 0xFFF)
				{
					printf("%s: not adts audio!\n", __FUNCTION__);
					continue;
				}
				if(adts.aac_frame_len>pAtr->stAdecFile.u32RdSize && pAtr->stAdecFile.pVirAddr)
				{
					FH_SYS_VmmFree(pAtr->stAdecFile.u32PhyAddr);
					pAtr->stAdecFile.pVirAddr = FH_NULL;
					s32Ret = FH_SYS_VmmAlloc(&pAtr->stAdecFile.u32PhyAddr, &pAtr->stAdecFile.pVirAddr, "SAMPLE_ADEC_FILE", NULL,adts.aac_frame_len);
					if(s32Ret != FH_SUCCESS)
					{
						AUDIO_PRT("Dev%d-Chn%d failed with %#x!", pAtr->stAdecDev.s32Dev,pAtr->stAdecDev.s32Chn,s32Ret);
						usleep(10*1000);
						continue;
					}
					stream.pStream = pAtr->stAdecFile.pVirAddr;
					stream.u32PhyAddr = pAtr->stAdecFile.u32PhyAddr;
				}

				if(stream.pStream)
				{
					u32ReadLen = fread(stream.pStream, 1, adts.aac_frame_len, pAtr->stAdecFile.pFile);
					if (u32ReadLen <= 0)
					{
						continue;
					}
				}
			}
			stream.u32Len = u32ReadLen;
		}

		if(pAtr->stAdecDev.bUse)
		{
			FH_UINT8* p;
			//stream.pStream = stFrame.pVirAddr;
			//stream.u32Len = stFrame.u32Len;
			s32Ret = FH_ADEC_SendStream(pAtr->stAdecDev.s32Dev, &stream, 40);
			if(s32Ret != FH_SUCCESS)
			{
				continue;
			}
			s32Ret = FH_ADEC_GetFrame(pAtr->stAdecDev.s32Dev,&stFrame,40);
			if(s32Ret != FH_SUCCESS)
			{
				continue;
			}
			p = (FH_UINT8*)stFrame.pVirAddr;if(p){}
			//AUDIO_PRT("[DATA] len=0x%x,snd=%d,data=%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",stFrame.u32Len,stFrame.enSoundmode,p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
		}

		if(pAtr->stAoFile.pFile && pAtr->stAoFile.u32RdSize)
		{
			FH_UINT32 u32ReadLen;

			if(!pAtr->stAoFile.pVirAddr)
			{
				s32Ret = FH_SYS_VmmAlloc(&pAtr->stAoFile.u32PhyAddr, &pAtr->stAoFile.pVirAddr, "SAMPLE_AO_FILE", NULL,pAtr->stAoFile.u32RdSize);
				if(s32Ret != FH_SUCCESS)
				{
					AUDIO_PRT("Dev%d-Chn%d failed with %#x!", pAtr->stAoDev.s32Dev,pAtr->stAoDev.s32Chn,s32Ret);
					usleep(10*1000);
					continue;
				}
			}
			stFrame.pVirAddr = pAtr->stAoFile.pVirAddr;
			stFrame.u32PhyAddr = pAtr->stAoFile.u32PhyAddr;
			stFrame.u32Len = pAtr->stAoFile.u32RdSize;
			stFrame.enFsRate = pAtr->stAoFile.u32FsRate;
			stFrame.enBitwidth = pAtr->stAoFile.u32BitWidth;
			stFrame.enSoundmode = pAtr->stAoFile.u32SndMode;
			u32ReadLen = fread(stFrame.pVirAddr, 1, stFrame.u32Len, pAtr->stAoFile.pFile);
			printf("\033[32m[read-end]u32Len %d\033[0m\n",u32ReadLen);
			if (u32ReadLen <= 0)
			{
				fseek(pAtr->stAoFile.pFile, 0, SEEK_SET);/*read file again*/
				//printf("\033[32m[read-end]u32Len %d\033[0m\n",u32ReadLen);
				continue;
			}
		}

		/* send frame to ao */
		if(FH_TRUE == pAtr->stAoDev.bUse && cnt)
		{
		replay:
			if(pAtr->bStart)
			{
				s32Ret = FH_AO_SendFrame(pAtr->stAoDev.s32Dev, pAtr->stAoDev.s32Chn, &stFrame, 0);
				if (FH_SUCCESS != s32Ret )
				{
					//AUDIO_PRT("Dev%d-Chn%d failed with %#x!\n",pAtr->stAoDev.s32Dev, pAtr->stAoDev.s32Chn, s32Ret);
					usleep(10*1000);
					goto replay;
				}
			}
			else
			{
				break;
			}
			//cnt--;
		}

		if(FH_TRUE == pAtr->stAiDev.bUse)
		{
			/* finally release the stream */
			s32Ret = FH_AI_ReleaseChnFrame(pAtr->stAiDev.s32Dev, pAtr->stAiDev.s32Chn, &stFrame, NULL);
			if(FH_SUCCESS != s32Ret )
			{
				AUDIO_PRT("Dev%d-Chn%d failed with %#x!\n", pAtr->stAiDev.s32Dev, pAtr->stAiDev.s32Chn, s32Ret);
			}
		}
	}
	return NULL;
}

void *AUDIO_In_Proc(void *ptr)
{
	FH_SINT32 s32Ret=0;
	AUDIO_TST_S* pAtr=(AUDIO_TST_S*)ptr;
	AUDIO_FRAME_S stFrame={0};

	//AUDIO_PRT("Dev%d-Chn%d bEn%d enter", pAtr->stAiDev.s32Dev,pAtr->stAiDev.s32Chn,pAtr->bStart);
	while(pAtr->bStart)
	{
        memset(&stFrame,0,sizeof(stFrame));

        uint32_t time_ms_last = time_relative_ms();
        /* get frame from ai chn */
        s32Ret = FH_AI_GetChnFrame(pAtr->stAiDev.s32Dev, pAtr->stAiDev.s32Chn, &stFrame, NULL, 1000);
        uint32_t time_ms = time_relative_ms();
        //usleep(20*1000);
        if (FH_SUCCESS != s32Ret )
        {
            AUDIO_PRT("Dev%d-Chn%d failed with %#x!", pAtr->stAiDev.s32Dev,pAtr->stAiDev.s32Chn,s32Ret);
            usleep(10*1000);
            continue;
        }

        AUDIO_PRT("<%u:%u>===Dev%d-Chn%d bSave%d u32Len=%d!",time_ms, time_ms-time_ms_last, pAtr->stAiDev.s32Dev,pAtr->stAiDev.s32Chn,pAtr->stAiFile.bSave,stFrame.u32Len);
        if(FH_TRUE == pAtr->stAiFile.bSave)
        {
            //AUDIO_PRT("[DATA] len=0x%x,data=%x-%x\n",stFrame.u32Len,((char*)stFrame.pVirAddr)[0],((char*)stFrame.pVirAddr)[1]);
            /* save audio stream to file */
            s32Ret = fwrite(stFrame.pVirAddr, 1, stFrame.u32Len, pAtr->stAiFile.pFile);

            pthread_mutex_lock(&audio_mutex);
            circlebuf_push_back(&audio_packet, stFrame.pVirAddr, stFrame.u32Len);
            pthread_mutex_unlock(&audio_mutex);
        }

        /* finally release the stream */
        s32Ret = FH_AI_ReleaseChnFrame(pAtr->stAiDev.s32Dev, pAtr->stAiDev.s32Chn, &stFrame, NULL);
        if(FH_SUCCESS != s32Ret )
        {
            AUDIO_PRT("Dev%d-Chn%d failed with %#x!\n", pAtr->stAiDev.s32Dev, pAtr->stAiDev.s32Chn, s32Ret);
        }
    }

    return NULL;
}

void *AUDIO_Out_Proc(void *ptr)
{
	FH_SINT32 s32Ret=0;
	AUDIO_TST_S* pAtr=(AUDIO_TST_S*)ptr;
	AUDIO_FRAME_S stFrame={0};

    while(pAtr->bStart)
	{
        FH_UINT32 u32ReadLen = 0;

        if(!pAtr->stAoFile.pVirAddr)
        {
            s32Ret = FH_SYS_VmmAlloc(&pAtr->stAoFile.u32PhyAddr, &pAtr->stAoFile.pVirAddr, "SAMPLE_AO_FILE", NULL,pAtr->stAoFile.u32RdSize);
            if(s32Ret != FH_SUCCESS)
            {
                AUDIO_PRT("Dev%d-Chn%d failed with %#x!", pAtr->stAoDev.s32Dev,pAtr->stAoDev.s32Chn,s32Ret);
                usleep(10*1000);
                continue;
            }
        }
        stFrame.pVirAddr = pAtr->stAoFile.pVirAddr;
        stFrame.u32PhyAddr = pAtr->stAoFile.u32PhyAddr;
        stFrame.u32Len = pAtr->stAoFile.u32RdSize;
        stFrame.enFsRate = pAtr->stAoFile.u32FsRate;
        stFrame.enBitwidth = pAtr->stAoFile.u32BitWidth;
        stFrame.enSoundmode = pAtr->stAoFile.u32SndMode;
#if 1
        pthread_mutex_lock(&audio_mutex);
        //printf("===========audio_packet.size:%d \n", audio_packet.size);
        if (audio_packet.size >= stFrame.u32Len)
		{
			circlebuf_pop_front(&audio_packet, stFrame.pVirAddr, stFrame.u32Len);
            u32ReadLen = stFrame.u32Len;
        }
        else if (pAtr->stAoFile.pFile)
        {
            u32ReadLen = fread(stFrame.pVirAddr, 1, stFrame.u32Len, pAtr->stAoFile.pFile);
        }

        pthread_mutex_unlock(&audio_mutex);
#else
        u32ReadLen = fread(stFrame.pVirAddr, 1, stFrame.u32Len, pAtr->stAoFile.pFile);
#endif
        //printf("fread data len:%d\n",u32ReadLen);
        if (u32ReadLen <= 0)
        {
            if(pAtr->stAoFile.pFile)
                fseek(pAtr->stAoFile.pFile, 0, SEEK_SET);/*read file again*/
            //printf("\033[32m[read-end]u32Len %d\033[0m\n",u32ReadLen);
            usleep(10*1000);
            continue;
        }

        //memset(stFrame.pVirAddr, 0, stFrame.u32Len);
        if(FH_TRUE == pAtr->stAoDev.bUse)
		{
            static unsigned int tindex = 0;
            tindex++;
		replay:
			if(pAtr->bStart)
			{
				s32Ret = FH_AO_SendFrame(pAtr->stAoDev.s32Dev, pAtr->stAoDev.s32Chn, &stFrame, 0);
				if (FH_SUCCESS != s32Ret )
				{
					//AUDIO_PRT("<%u> Dev%d-Chn%d failed with %#x!\n", tindex,pAtr->stAoDev.s32Dev, pAtr->stAoDev.s32Chn, s32Ret);
					usleep(10*1000);
					goto replay;
				}
			}
			else
			{
				break;
			}
		}
    }

    return NULL;
}

FH_SINT32 SAMPLE_AUDIO_CreatTrd(FH_VOID *ptr)
{
	AUDIO_TST_S* pAtr=(AUDIO_TST_S*)ptr;

	pAtr->bStart = FH_TRUE;
	pthread_create(&pAtr->stPid, 0, SAMPLE_AUDIO_Proc, pAtr);
	return FH_SUCCESS;
}

FH_SINT32 SAMPLE_AUDIO_In_CreatTrd(FH_VOID *ptr)
{
	AUDIO_TST_S* pAtr=(AUDIO_TST_S*)ptr;

	pAtr->bStart = FH_TRUE;
	pthread_create(&pAtr->stPid, 0, AUDIO_In_Proc, pAtr);
	return FH_SUCCESS;
}

FH_SINT32 SAMPLE_AUDIO_Out_CreatTrd(FH_VOID *ptr)
{
	AUDIO_TST_S* pAtr=(AUDIO_TST_S*)ptr;

	pAtr->bStart = FH_TRUE;
	pthread_create(&pAtr->stPid, 0, AUDIO_Out_Proc, pAtr);
	return FH_SUCCESS;
}

FH_SINT32 SAMPLE_AUDIO_DestoryTrd(FH_VOID *ptr)
{
	AUDIO_TST_S* pAtr=(AUDIO_TST_S*)ptr;

	if(pAtr->bStart)
	{
		pAtr->bStart = FH_FALSE;
		pthread_join(pAtr->stPid, 0);
	}
	if(pAtr->stAoFile.pVirAddr)
	{
		FH_SYS_VmmFree(pAtr->stAoFile.u32PhyAddr);
		pAtr->stAoFile.pVirAddr=FH_NULL;
	}
	return FH_SUCCESS;
}



/******************************************************************************
* function : Open Ai File
******************************************************************************/
FILE * SAMPLE_AUDIO_OpenAiFile(AUDIO_DEV AiDevID, AI_CHN AiChn)
{
	FILE *pfd;
	FH_CHAR aszFileName[128]="\0";

	/* create file for save stream*/
	sprintf(aszFileName, "audio_dev%d_chn%d.pcm",AiDevID, AiChn);
	pfd = fopen(aszFileName, "w+");
	if (NULL == pfd)
	{
		AUDIO_PRT("open file %s failed", aszFileName);
		return NULL;
	}
	AUDIO_PRT("open wav:%d file  \"%s\" OK", iswavfile, aszFileName);
    if(iswavfile)
	{
		wav_header_t wav_header;
		wav_header.riff_id = ID_RIFF;
	    wav_header.riff_sz = 0;
	    wav_header.riff_fmt = ID_WAVE;
	    wav_header.fmt_id = ID_FMT;
	    wav_header.fmt_sz = 16;
	    wav_header.audio_format = 1;
	    wav_header.num_channels = AUDIO_CHN_NUM_1;
	    wav_header.sample_rate = 8000;
	    wav_header.bits_per_sample = 16;
	    wav_header.byte_rate = (wav_header.bits_per_sample / 8) * wav_header.num_channels * wav_header.sample_rate;
	    wav_header.block_align = wav_header.num_channels * (wav_header.bits_per_sample / 8);
	    wav_header.data_id = ID_DATA;
		fwrite(&wav_header, 1, sizeof(wav_header_t), pfd);
		foffset = ftell(pfd);
	}
	return pfd;
}

/******************************************************************************
* function : Open Ao File
******************************************************************************/
FILE * SAMPLE_AUDIO_OpenAoFile(AUDIO_DEV AoDevID, AO_CHN AoChn)
{
	FILE *pfd;
	FH_CHAR aszFileName[128]="\0";

	/* create file for save stream*/
	printf("\033[33m Enter File Name:\033[0m");
	//scanf("%s", aszFileName);
    sprintf(aszFileName, "audio_dev%d_chn%d.pcm",AoDevID, AoChn);
	pfd = fopen(aszFileName, "r");
	if (NULL == pfd)
	{
		AUDIO_PRT("open file %s failed", aszFileName);
		return NULL;
	}
	AUDIO_PRT("open file \"%s\" OK", aszFileName);

	return pfd;
}

FH_SINT32 SAMPLE_AUDIO_CodecToFile(int type)
{
	FH_SINT32 s32Ret=FH_SUCCESS;
	AUDIO_DEV AiDevId=DEV_ACW0;
	AIO_ATTR_S stAttr={0};
	AI_CHN	AiChn=0;
	AUDIO_CHN_PARAM_S stPara={0};
	FILE* pFD = FH_NULL;
	AUDIO_TST_S* pAtr=(AUDIO_TST_S*)&g_stAudTest;
    memset(pAtr, 0, sizeof(AUDIO_TST_S));
	AUDIO_SAVE_FILE_INFO_S stFileInfo={0};

	memset(&stAttr,0,sizeof(AIO_ATTR_S));
	stAttr.fs_rate = AI_SAMPLE_RATE;
	stAttr.chn_num = AUDIO_CHN_NUM_1;
	stAttr.chn_width = SAMPLE_BITS;
	stAttr.is_slave = AIO_MODE_I2S_SLAVE;
	stAttr.data_format = AUDIO_SOUND_MODE_MONO;
	stAttr.io_type = SELECT_TYPE_AI;
	stAttr.u32DmaNum = DEFAULT_AI_CACHE_NUM;
	stAttr.u32DmaSize = A_FRM_SIZE(AI_SAMPLE_RATE, SAMPLE_BITS, SAMPLE_PS);
	s32Ret = FH_AI_SetPubAttr(AiDevId, IOC_SET_PUBATTR, (FH_VOID*)&stAttr);
	if(s32Ret != FH_SUCCESS)
	{
		AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
		goto exit;
	}
	s32Ret = FH_AI_Enable(AiDevId);
	if(s32Ret != FH_SUCCESS)
	{
		AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
		goto exit;
	}
	stPara.u32FrmNum = DEFAULT_AI_CACHE_NUM;
	stPara.u32FrmSize = A_FRM_SIZE(AI_SAMPLE_RATE, SAMPLE_BITS, SAMPLE_PS);
	s32Ret = FH_AI_SetChnParam(AiDevId,AiChn,&stPara);
	if(s32Ret != FH_SUCCESS)
	{
		AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
		goto exit;
	}
	s32Ret = FH_AI_EnableChn(AiDevId,AiChn);
	if(s32Ret != FH_SUCCESS)
	{
		AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
		goto exit;
	}
	{
        VOLUME_PARAM_S stPara;
        //?����������������?��?�㝝����� 24db
        stPara.s32Chn = AiChn;
        stPara.stType = VOLUME_TYPE_ANA;
        stPara.u32Vol = 32;//val[0~31];//0: ������1:-12db��..., 31:24db����� 1.2db��
        FH_AI_SetVolume(AiDevId, &stPara);
        //�����������������?���������������?��?����?��� 6db
        stPara.s32Chn = AiChn;
        stPara.stType = VOLUME_TYPE_DIG;
        stPara.u32Vol = 0x7f;//val[0x1B~0x7F];//0x1B:-114db, ..., 0x7F�4�9+6db����� 1.2db��
        FH_AI_SetVolume(AiDevId, &stPara);
	}

	pFD = SAMPLE_AUDIO_OpenAiFile(AiDevId,AiChn);
	if(pFD == FH_NULL)
	{
		AUDIO_PRT("failed! pFD[%p]\n", pFD);
		goto exit;
	}
	pAtr->stAiFile.pFile = pFD;
	pAtr->stAiFile.bSave = FH_TRUE;
	pAtr->stAiDev.s32Dev=AiDevId;
	pAtr->stAiDev.s32Chn=AiChn;
	pAtr->stAiDev.bUse = FH_TRUE;

	stFileInfo.bEnable = FH_FALSE;
	sprintf(stFileInfo.aFileName,"ai%d_chn%d.pcm",AiDevId,AiChn);
	sprintf(stFileInfo.aFilePath,"/mnt/");
	stFileInfo.u32FileSize = 1000000;
	//FH_AI_SaveFile(AiDevId,AiChn,&stFileInfo);
    if(type == 1)
    {
        s32Ret = SAMPLE_AUDIO_CreatTrd((FH_VOID*)pAtr);
    }
    else{
        s32Ret = SAMPLE_AUDIO_In_CreatTrd((FH_VOID*)pAtr);
    }
	if (s32Ret != FH_SUCCESS)
	{
		AUDIO_PRT("Create get stream thread failed! s32Ret[%#x]\n", s32Ret);
		goto exit;
	}

	printf("\nplease press twice ENTER to exit this sample\n");
	getchar();
	getchar();
    if(iswavfile)
	{
		wav_header_t wav_header;
		wav_header.riff_id = ID_RIFF;
	    wav_header.riff_sz = 0;
	    wav_header.riff_fmt = ID_WAVE;
	    wav_header.fmt_id = ID_FMT;
	    wav_header.fmt_sz = 16;
	    wav_header.audio_format = 1;
	    wav_header.num_channels = AUDIO_CHN_NUM_1;
	    wav_header.sample_rate = 8000;
	    wav_header.bits_per_sample = 16;
	    wav_header.byte_rate = (wav_header.bits_per_sample / 8) * wav_header.num_channels * wav_header.sample_rate;
	    wav_header.block_align = wav_header.num_channels * (wav_header.bits_per_sample / 8);
	    wav_header.data_id = ID_DATA;

		wav_header.data_sz = ftell(pFD) - foffset;
		wav_header.riff_sz = wav_header.data_sz + sizeof(wav_header_t) - 8;
		fseek(pFD, 0, SEEK_SET);
		fwrite(&wav_header, 1, sizeof(wav_header_t), pFD);
		fclose(pFD);
	}
exit:
	s32Ret = SAMPLE_AUDIO_DestoryTrd(pAtr);
	if (s32Ret != FH_SUCCESS)
	{
		AUDIO_PRT("Destory get stream thread failed! s32Ret[%#x]\n", s32Ret);
	}
	s32Ret = FH_AI_DisableChn(AiDevId, AiChn);
	if (FH_SUCCESS != s32Ret)
	{
		AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
	}

	s32Ret = FH_AI_Disable(AiDevId);
	if (FH_SUCCESS != s32Ret)
	{
		AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
	}

	if(pFD){fclose(pFD);}
	return s32Ret;
}



FH_SINT32 SAMPLE_AUDIO_FileToCodec(int type)
{
	FH_SINT32 s32Ret=FH_SUCCESS;
	AUDIO_DEV AoDevId=DEV_ACW0;
	AIO_ATTR_S stAttr={0};
	AO_CHN	AoChn=0;
	AUDIO_CHN_PARAM_S stPara={0};
	FILE* pFD = FH_NULL;
	AUDIO_TST_S* pAtr=(AUDIO_TST_S*)&g_stAudTest;
    memset(pAtr, 0, sizeof(AUDIO_TST_S));
	AUDIO_SAVE_FILE_INFO_S stFileInfo={0};

	pFD = SAMPLE_AUDIO_OpenAoFile(AoDevId,AoChn);
	if(pFD == FH_NULL)
	{
		AUDIO_PRT("failed! pFD[%p]\n", pFD);
		goto exit;
	}

	memset(&stAttr,0,sizeof(AIO_ATTR_S));
	stAttr.fs_rate = AO_SAMPLE_RATE;
	stAttr.chn_num = AUDIO_CHN_NUM_1;
	stAttr.chn_width = SAMPLE_BITS;
	stAttr.is_slave = AIO_MODE_I2S_SLAVE;
	stAttr.data_format = AUDIO_SOUND_MODE_MONO;
	stAttr.io_type = SELECT_TYPE_AO;
	stAttr.u32DmaNum = DEFAULT_AI_CACHE_NUM;
	stAttr.u32DmaSize = A_FRM_SIZE(stAttr.fs_rate, stAttr.chn_width, SAMPLE_PS);
	s32Ret = FH_AO_SetPubAttr(AoDevId, IOC_SET_PUBATTR, (FH_VOID*)&stAttr);
	if(s32Ret != FH_SUCCESS)
	{
		AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
		goto exit;
	}
	s32Ret = FH_AO_Enable(AoDevId);
	if(s32Ret != FH_SUCCESS)
	{
		AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
		goto exit;
	}


	stPara.u32FrmNum = DEFAULT_AO_CACHE_NUM;
	stPara.u32FrmSize = A_FRM_SIZE(stAttr.fs_rate, stAttr.chn_width, SAMPLE_PS);
	s32Ret = FH_AO_SetChnParam(AoDevId,AoChn,&stPara);
	if(s32Ret != FH_SUCCESS)
	{
		AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
		goto exit;
	}
	s32Ret = FH_AO_EnableChn(AoDevId,AoChn);
	if(s32Ret != FH_SUCCESS)
	{
		AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
		goto exit;
	}

	{
		VOLUME_PARAM_S stPara_vol;
		//?����������
		stPara_vol.s32Chn = AoChn;
		stPara_vol.stType = VOLUME_TYPE_ANA;
		stPara_vol.u32Vol = 0;//[0�4�9+0db/1�4�9+2db/2�4�9+4db/3�4�9+6db], ?��? 3��6db
		FH_AO_SetVolume(AoDevId, &stPara_vol);
		//������������
		stPara_vol.s32Chn = AoChn;
		stPara_vol.stType = VOLUME_TYPE_DIG;
		stPara_vol.u32Vol = g_dwAoVolume;//�����???�?�� 0x80����������������??� 1/128 ����
		FH_AO_SetVolume(AoDevId, &stPara_vol);

        printf("AoDevId %d AoChn %d g_dwAoVolume %x\n", AoDevId, AoChn, g_dwAoVolume);
	}

	stFileInfo.bEnable = FH_FALSE;
	sprintf(stFileInfo.aFileName,"ao%d_chn%d.pcm",AoDevId,AoChn);
	sprintf(stFileInfo.aFilePath,"/mnt/");
	stFileInfo.u32FileSize = 1000000;
	FH_AO_SaveFile(AoDevId,AoChn,&stFileInfo);
	pAtr->stAoFile.pFile = pFD;
	pAtr->stAoFile.u32RdSize = stPara.u32FrmSize;
	pAtr->stAoFile.u32FsRate = stAttr.fs_rate;
	pAtr->stAoFile.u32BitWidth = stAttr.chn_width;
	pAtr->stAoFile.u32SndMode = stAttr.data_format;
	pAtr->stAoDev.s32Dev=AoDevId;
	pAtr->stAoDev.s32Chn=AoChn;
	pAtr->stAoDev.bUse = FH_TRUE;
    if(type==3){
	    s32Ret = SAMPLE_AUDIO_CreatTrd((FH_VOID*)pAtr);
    }
    else{
        s32Ret = SAMPLE_AUDIO_Out_CreatTrd((FH_VOID*)pAtr);
    }
	if (s32Ret != FH_SUCCESS)
	{
		AUDIO_PRT("Create get stream thread failed! s32Ret[%#x]\n", s32Ret);
		goto exit;
	}

	printf("\nplease press twice ENTER to exit this sample\n");
	getchar();
	getchar();
exit:
	s32Ret = SAMPLE_AUDIO_DestoryTrd(pAtr);
	if (s32Ret != FH_SUCCESS)
	{
		AUDIO_PRT("Destory get stream thread failed! s32Ret[%#x]\n", s32Ret);
	}
	s32Ret = FH_AO_DisableChn(AoDevId, AoChn);
	if (FH_SUCCESS != s32Ret)
	{
		AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
	}

	s32Ret = FH_AO_Disable(AoDevId);
	if (FH_SUCCESS != s32Ret)
	{
		AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
	}

	if(pFD){fclose(pFD);}
	return s32Ret;
}

FH_SINT32 SAMPLE_AUDIO_AI_And_AO(int type)
{
    FH_SINT32 s32Ret=FH_SUCCESS;
    static AUDIO_TST_S g_stAudIn={0};
    static AUDIO_TST_S g_stAudOut={0};

    bool bai_use = false;
    bool bao_use = false;
    if(type == 66){
        bai_use = true;
        bao_use = true;
    }else if (type == 67)
    {
        /* code */
        bao_use = true;
    }
    else if (type == 68)
    {
        /* code */
        bai_use = true;
    }
    else
    {
        AUDIO_PRT("error type %d\n", type);
        return -1;
    }

    do
    {
        if(!bai_use)
            break;
        AUDIO_DEV AiDevId=DEV_ACW0;
        AIO_ATTR_S stAttr={0};
        AI_CHN	AiChn=0;
        AUDIO_CHN_PARAM_S stPara={0};
        FILE* pFD = FH_NULL;
        AUDIO_TST_S* pAtr=(AUDIO_TST_S*)&g_stAudIn;
        memset(pAtr, 0, sizeof(AUDIO_TST_S));

        memset(&stAttr,0,sizeof(AIO_ATTR_S));
        stAttr.fs_rate = AI_SAMPLE_RATE;
        stAttr.chn_num = AUDIO_CHN_NUM_1;
        stAttr.chn_width = SAMPLE_BITS;
        stAttr.is_slave = AIO_MODE_I2S_SLAVE;
        stAttr.data_format = AUDIO_SOUND_MODE_MONO;
        stAttr.io_type = SELECT_TYPE_AI;
        stAttr.u32DmaNum = DEFAULT_AI_CACHE_NUM;
        stAttr.u32DmaSize = A_FRM_SIZE(AI_SAMPLE_RATE, SAMPLE_BITS, SAMPLE_PS);
        s32Ret = FH_AI_SetPubAttr(AiDevId, IOC_SET_PUBATTR, (FH_VOID*)&stAttr);
        if(s32Ret != FH_SUCCESS)
        {
            AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
            break;;
        }
        s32Ret = FH_AI_Enable(AiDevId);
        if(s32Ret != FH_SUCCESS)
        {
            AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
            break;
        }
        stPara.u32FrmNum = DEFAULT_AI_CACHE_NUM;
        stPara.u32FrmSize = A_FRM_SIZE(AI_SAMPLE_RATE, SAMPLE_BITS, SAMPLE_PS);
        s32Ret = FH_AI_SetChnParam(AiDevId,AiChn,&stPara);
        if(s32Ret != FH_SUCCESS)
        {
            AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
            break;
        }
        s32Ret = FH_AI_EnableChn(AiDevId,AiChn);
        if(s32Ret != FH_SUCCESS)
        {
            AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
            break;
        }

        {
            VOLUME_PARAM_S stPara;
            //?����������������?��?�㝝����� 24db
            stPara.s32Chn = AiChn;
            stPara.stType = VOLUME_TYPE_ANA;
            stPara.u32Vol = 32;//val[0~31];//0: ������1:-12db��..., 31:24db����� 1.2db��
            FH_AI_SetVolume(AiDevId, &stPara);
            //�����������������?���������������?��?����?��� 6db
            stPara.s32Chn = AiChn;
            stPara.stType = VOLUME_TYPE_DIG;
            stPara.u32Vol = 0x7f;//val[0x1B~0x7F];//0x1B:-114db, ..., 0x7F�4�9+6db����� 1.2db��
            FH_AI_SetVolume(AiDevId, &stPara);
        }

            pFD = SAMPLE_AUDIO_OpenAiFile(AiDevId,AiChn);
            if(pFD == FH_NULL)
            {
                AUDIO_PRT("failed! pFD[%p]\n", pFD);
                break;;
            }
            pAtr->stAiFile.pFile = pFD;
            pAtr->stAiFile.bSave = FH_TRUE;
            pAtr->stAiDev.s32Dev=AiDevId;
            pAtr->stAiDev.s32Chn=AiChn;
            pAtr->stAiDev.bUse = FH_TRUE;

            s32Ret = SAMPLE_AUDIO_In_CreatTrd((FH_VOID*)pAtr);

            if (s32Ret != FH_SUCCESS)
            {
                AUDIO_PRT("Create get stream thread failed! s32Ret[%#x]\n", s32Ret);
                break;
            }
    }while (0);

    //??
    do
    {
        if(!bao_use)
            break;
        AUDIO_DEV AoDevId=DEV_ACW0;
        AIO_ATTR_S stAttr={0};
        AO_CHN	AoChn=0;
        AUDIO_CHN_PARAM_S stPara={0};
        FILE* pFD = FH_NULL;
        AUDIO_TST_S* pAtr=(AUDIO_TST_S*)&g_stAudOut;
        memset(pAtr, 0, sizeof(AUDIO_TST_S));
        AUDIO_SAVE_FILE_INFO_S stFileInfo={0};

        if(type != 66){
            pFD = SAMPLE_AUDIO_OpenAoFile(AoDevId,AoChn);
            if(pFD == FH_NULL)
            {
                AUDIO_PRT("failed! pFD[%p]\n", pFD);
                break;
            }
        }

        memset(&stAttr,0,sizeof(AIO_ATTR_S));
        stAttr.fs_rate = AO_SAMPLE_RATE;
        stAttr.chn_num = AUDIO_CHN_NUM_1;
        stAttr.chn_width = SAMPLE_BITS;
        stAttr.is_slave = AIO_MODE_I2S_SLAVE;
        stAttr.data_format = AUDIO_SOUND_MODE_MONO;
        stAttr.io_type = SELECT_TYPE_AO;
        stAttr.u32DmaNum = DEFAULT_AI_CACHE_NUM;
        stAttr.u32DmaSize = A_FRM_SIZE(stAttr.fs_rate, stAttr.chn_width, SAMPLE_PS);
        s32Ret = FH_AO_SetPubAttr(AoDevId, IOC_SET_PUBATTR, (FH_VOID*)&stAttr);
        if(s32Ret != FH_SUCCESS)
        {
            AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
            break;;
        }
        s32Ret = FH_AO_Enable(AoDevId);
        if(s32Ret != FH_SUCCESS)
        {
            AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
            break;
        }

        stPara.u32FrmNum = DEFAULT_AO_CACHE_NUM;
        stPara.u32FrmSize = A_FRM_SIZE(stAttr.fs_rate, stAttr.chn_width, SAMPLE_PS);
        s32Ret = FH_AO_SetChnParam(AoDevId,AoChn,&stPara);
        if(s32Ret != FH_SUCCESS)
        {
            AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
            break;
        }
        s32Ret = FH_AO_EnableChn(AoDevId,AoChn);
        if(s32Ret != FH_SUCCESS)
        {
            AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
            break;
        }

        {
            VOLUME_PARAM_S stPara_vol;
            //?����������
            stPara_vol.s32Chn = AoChn;
            stPara_vol.stType = VOLUME_TYPE_ANA;
            stPara_vol.u32Vol = 0;//[0�4�9+0db/1�4�9+2db/2�4�9+4db/3�4�9+6db], ?��? 3��6db
            FH_AO_SetVolume(AoDevId, &stPara_vol);
            //������������
            stPara_vol.s32Chn = AoChn;
            stPara_vol.stType = VOLUME_TYPE_DIG;
            stPara_vol.u32Vol = g_dwAoVolume;//�����???�?�� 0x80����������������??� 1/128 ����

            printf("AoDevId:%d, AoChn:%d, stPara_vol.u32Vol:%x\n", AoDevId, AoChn, stPara_vol.u32Vol);
            FH_AO_SetVolume(AoDevId, &stPara_vol);
        }

        stFileInfo.bEnable = FH_FALSE;
        sprintf(stFileInfo.aFileName,"ao%d_chn%d.pcm",AoDevId,AoChn);
        sprintf(stFileInfo.aFilePath,"/mnt/");
        stFileInfo.u32FileSize = 1000000;
        FH_AO_SaveFile(AoDevId,AoChn,&stFileInfo);
        pAtr->stAoFile.pFile = pFD;
        pAtr->stAoFile.u32RdSize = stPara.u32FrmSize;
        pAtr->stAoFile.u32FsRate = stAttr.fs_rate;
        pAtr->stAoFile.u32BitWidth = stAttr.chn_width;
        pAtr->stAoFile.u32SndMode = stAttr.data_format;
        pAtr->stAoDev.s32Dev=AoDevId;
        pAtr->stAoDev.s32Chn=AoChn;
        pAtr->stAoDev.bUse = FH_TRUE;

        s32Ret = SAMPLE_AUDIO_Out_CreatTrd((FH_VOID*)pAtr);

        if (s32Ret != FH_SUCCESS)
        {
            AUDIO_PRT("Create get stream thread failed! s32Ret[%#x]\n", s32Ret);
            break;
        }
    } while (0);

    printf("\nplease press twice ENTER to exit this sample\n");
	getchar();
	getchar();

    if(bai_use)
    {
        s32Ret = SAMPLE_AUDIO_DestoryTrd(&g_stAudIn);
        if (s32Ret != FH_SUCCESS)
        {
            AUDIO_PRT("Destory get stream thread failed! s32Ret[%#x]\n", s32Ret);
        }
        s32Ret = FH_AI_DisableChn(g_stAudIn.stAiDev.s32Dev, g_stAudIn.stAiDev.s32Chn);
        if (FH_SUCCESS != s32Ret)
        {
            AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
        }

        s32Ret = FH_AI_Disable(g_stAudIn.stAiDev.s32Dev);
        if (FH_SUCCESS != s32Ret)
        {
            AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
        }

        if(g_stAudIn.stAiFile.pFile){
            fclose(g_stAudIn.stAiFile.pFile);
        }
    }

    if(bao_use)
    {
        s32Ret = SAMPLE_AUDIO_DestoryTrd(&g_stAudOut);
        if (s32Ret != FH_SUCCESS)
        {
            AUDIO_PRT("Destory get stream thread failed! s32Ret[%#x]\n", s32Ret);
        }
        s32Ret = FH_AO_DisableChn(g_stAudOut.stAoDev.s32Dev, g_stAudOut.stAoDev.s32Chn);
        if (FH_SUCCESS != s32Ret)
        {
            AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
        }

        s32Ret = FH_AO_Disable(g_stAudOut.stAoDev.s32Dev);
        if (FH_SUCCESS != s32Ret)
        {
            AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
        }

        if(g_stAudOut.stAoFile.pFile){fclose(g_stAudOut.stAoFile.pFile);}
    }
    return s32Ret;
}

void fh_audio_test(int type, int volume)
{
    printf("fh_audio_test type %d voluem:%x\n", type, volume);
    pthread_mutex_init(&audio_mutex, NULL);
    circlebuf_init(&audio_packet);

    circlebuf_reserve(&audio_packet, 16000);
    switch (type)
    {
    case 1:
    case 11:
        {
            FH_SINT32 s32Ret = SAMPLE_AUDIO_CodecToFile(type);
            if (s32Ret != FH_SUCCESS)
            {
                AUDIO_PRT("failed! s32Ret[%#x]\n", s32Ret);
            }
        }
        break;
    case 2:
        {
            iswavfile = iswavfile ? false : true;
        }
    case 3:
    case 33:
        {
            SAMPLE_AUDIO_FileToCodec(type);
        }
        break;
    case 66:
    case 67:
    case 68:
        {
            if (volume != 999)
            {
                g_dwAoVolume = volume;
                printf("g_dwAoVolume %x\n", g_dwAoVolume);
            }
            SAMPLE_AUDIO_AI_And_AO(type);
        }
    default:
        break;
    }

    printf("fh_audio_test end, audio pack size:%d\n", audio_packet.size);
    pthread_mutex_destroy(&audio_mutex);
    circlebuf_free(&audio_packet);
}