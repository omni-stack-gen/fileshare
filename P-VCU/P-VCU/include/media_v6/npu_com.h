/**
* @file utils.h
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef __NPU_COM_H__
#define __NPU_COM_H__
#include <stdio.h>
#include "types/type_def.h"
#include "fhhcp/base.h"
#include "fhhcp/npu.h"

//#define NNP_FACE_DETECT
//AOV 的NN检测唤醒
#define FH_NN_ENABLE
#define NN_CHN_WIDTH 640
#define NN_CHN_HEIGHT 352
#define CH3_YUV_TYPE_G0 VPU_VOMODE_SCAN

#define FACE_SIMILAR_BY_NNP

#ifdef NNP_FACE_DETECT

#ifdef MODEL_768X1024
#define NPU_INPUT_WIDTH    (768)
#define NPU_INPUT_HEIGHT   (1024)
#define IMAGE_PIXEL_FORMAT  E_TY_PIXEL_FORMAT_YUV_SEMIPLANAR_420
#define NPU_MODEL_DETECT_BIN       "facedet_768x1024_nv12.bin"
#else
#define NPU_INPUT_WIDTH    640
#define NPU_INPUT_HEIGHT   352
#define IMAGE_PIXEL_FORMAT  E_TY_PIXEL_FORMAT_YUV_SEMIPLANAR_420
#define NPU_MODEL_DETECT_BIN       "facedet_640x352_nv12.bin"
#endif

#define NPU_MODEL_RECOG_BIN       "facerec_128x128_nv12.bin"
#define NPU_DETECT_WIDTH    NPU_INPUT_WIDTH
#define NPU_DETECT_HEIGHT   NPU_INPUT_HEIGHT

#define NPU_RECOG_WIDTH    128
#define NPU_RECOG_HEIGHT   128

#define EMBEDDING_DIMS 768
#define MATMUL_MAX_K 1024
#define MATMUL_MAX_N 10000
#else
#define NPU_MODEL_BIN       "net_combine.bin"
#define NPU_INPUT_WIDTH    (512)
#define NPU_INPUT_HEIGHT   (288)
#define IMAGE_PIXEL_FORMAT  E_TY_PIXEL_FORMAT_RGB_888_PLANAR
#endif

#define MAX_MDL_IN_NUM (3)
#define MAX_MDL_OUT_NUM (3)
#define MAX_PATH_LEN (1024)

#define NPU_SCREEN_WIDTH  1920
#define NPU_SCREEN_HEIGHT  1080

//#define DET_IMAGE_WIDTH  1920
//#define DET_IMAGE_HEIGHT  1080

#define DEFAULT_W 960
#define DEFAULT_H 1088

#define UINT64_TO_VOIDP(a)    (FH_VOID *)((FH_UINT32)(a))

typedef struct
{
    volatile float x1;     //矩形框x坐标 | [0.0 - 1.0]
    volatile float y1;     //矩形框y坐标 | [0.0 - 1.0]
    volatile float x2;     //矩形框宽度　| [0.0 - 1.0]
    volatile float y2;     //矩形框高度　| [0.0 - 1.0]
}FH_RECT_FLOAT_T;

typedef struct
{
    FH_UINT16 clsType;     //类别索引　| [ ]
    float conf;      //置信度　　| [0.0 - 1.0]
    FH_RECT_FLOAT_T bbox;     //矩形信息  	| 百分比
}FH_DET_BBOX_T;

#define FH_MAX_BBOX_NUM 2000
typedef struct
{
    FH_UINT64 time_stamp;    //矩形信息对应帧的时间戳　| [ ]
    FH_UINT32 boxNum;        //矩形个数　　| [ ]
    FH_DET_BBOX_T *detBBox[FH_MAX_BBOX_NUM]; //具体矩形信息　| [ ]
}FH_FACE_DETECTION_T;



#define DEBUG_LOG(fmt, ...) fprintf(stdout, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#define INFO_LOG(fmt, ...) fprintf(stdout, "[INFO]  " fmt "\n", ##__VA_ARGS__)
#define IMPOTANT_LOG(fmt, ...) fprintf(stdout, "[IMPOT] " fmt "\n", ##__VA_ARGS__)
#define WARN_LOG(fmt, ...) fprintf(stdout, "[WARN]  " fmt "\n", ##__VA_ARGS__)
#define ERROR_LOG(fmt, ...) fprintf(stdout, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#define FATAL_LOG(fmt, ...) fprintf(stdout, "[FATAL] " fmt "\n", ##__VA_ARGS__)

#define SAMPLE_NNA_PRT(fmt,...)   \
    do {\
        printf("[%s]-%d: "fmt "\n", __FUNCTION__, __LINE__, ## __VA_ARGS__);\
       }while(0)

#define SAMPLE_NNA_CHECK_EXPR_GOTO(Expr, Label, fmt, ...)                    \
do{																				  \
	if(Expr)                                                                      \
	{                                                                             \
		SAMPLE_NNA_PRT(fmt,## __VA_ARGS__);                               \
		goto Label;                                                               \
	}                                                                             \
}while(0)
/****
*Expr is true,return void
*/
#define SAMPLE_NNA_CHECK_EXPR_RET_VOID(Expr,Level,Msg, ...)					     \
do{                                                                              \
	if(Expr)                                                                     \
	{                                                                            \
		SAMPLE_NNA_PRT(Level,Msg, ##__VA_ARGS__);                              \
		return;                                                                  \
	}                                                                            \
}while(0)
/****
*Expr is true,return Ret
*/
#define SAMPLE_NNA_CHECK_EXPR_RET(Expr,Ret,Msg, ...)					     \
do{                                                                              \
	if(Expr)                                                                     \
	{                                                                            \
		SAMPLE_NNA_PRT(Msg, ##__VA_ARGS__);                              \
		return Ret;                                                              \
	}                                                                            \
}while(0)

typedef struct{
    char path[MAX_PATH_LEN];
    uint32_t width;
    uint32_t height;
    E_TY_PixelFormat fmt;
}stImage;

typedef struct{
    char mdlPath[MAX_PATH_LEN];
    stImage inImageVec[MAX_MDL_IN_NUM];
    uint32_t inImgNum;
    char *outPath;
}stNpuOP;

typedef struct fh_nna_info
{
    T_TY_MemSegmentInfo mdlMemInfo;
    T_TY_ModelDesc mdlDesc;
    TY_NPU_MODEL_HANDLE mdlHdl;
    T_TY_MemSegmentInfo tskMemInfo;
    TY_NPU_TASK_HANDLE tskHdl;
    T_TY_TaskInput *taskInVec;
    T_TY_TaskOutput *taskOutVec;
    stNpuOP npuOp;
    FH_UINT32 mdlInNum;
    FH_UINT32 mdlOutNum;
} FY_NPU_INFO;

FH_VOID sample_nn_change_modle(FH_VOID);
void npu_printMemSegments(T_TY_MemSegmentInfo *ptrMemInfo);
int32_t npu_freeMem(uint64_t virAddr, uint64_t phyAddr, uint32_t size);
int32_t npu_mallocMem(void **ppVirAddr, uint64_t *ptrPhyAddr, uint32_t size, uint32_t align, E_TY_MemAllocType type);
int32_t npu_flushVmmMem(uint64_t virAddr, uint64_t phyAddr, uint32_t size);
int32_t npu_flushCacheMemSeg(T_TY_MemSegmentInfo *ptrMemInfo);
int32_t npu_freeMemSegments(T_TY_MemSegmentInfo *ptrMemInfo);
int32_t npu_allocMemSegments(T_TY_MemSegmentInfo *ptrMemInfo);
int32_t npu_readFile(T_TY_Mem *ptrMem, char *path);
int32_t npu_freeImageBuf(T_TY_Image *ptrImg);
int32_t npu_CheckPathIsFile(char *filePath);
int32_t npu_GetFileSize(char *path);
int32_t npu_ReadBinFile(char *filePath, char **ppInputBuff, uint32_t *ptrFileSize);
void npu_printBlob(T_TY_BlobDesc *blob);
uint32_t npu_getBlobSize(T_TY_BlobDesc *blob);
int32_t npu_getImageSize(uint32_t width, uint32_t height, E_TY_PixelFormat fmt);
FH_SINT32 sample_npu_test_start();
FH_SINT32 sample_npu_test_stop();
FH_SINT32 npu_test_init();
void nnp_postprocess_nms(float *data, FH_FACE_DETECTION_T *pstResult);
void nnp_postprocess_free_result(FH_FACE_DETECTION_T *pstResult);
int __gosd_font_init(void);
void  __gosd_font_cleanup(void);
int __vpu_gosd_draw_text(unsigned char *bitmap, int width, int height, char *word);

#endif //__NPU_COM_H__
//#pragma once
