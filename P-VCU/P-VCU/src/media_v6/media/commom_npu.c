#if defined(SAMPLE_NPU_TEST)
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include "fhhcp/sys.h"
#include "fhhcp/npu.h"
#include "types/vmm_api.h"
#include "npu_com.h"
#include "fh_vpu_mpi.h"
#include "fh_tde_mpi.h"
#include "sample_common.h"

#include "types/bufCtrl.h"
//#include "common.h"

//#include "npu_draw.h"
#include "codec/config.h"

#define RGB_CHN_DEFAULT 1
#define DEFAULT_YC_TIMEOUT 2000
//#define COLOR_FLASSIER 0xE083
#define COLOR_FLASSIER 0xFC00
#define DEFAULT_LINE_W 1
#define ACTURE_LINE_W 4

static pthread_t g_sample_npu_thrd = 0;
static FH_BOOL g_npu_test_start = FH_FALSE;
FH_SINT16 close_nn = 0;
FH_SINT16 wait_time = 0;

FH_SINT16 aiisp_init = 0;
FH_UINT64 time_nnp_s=0,time_nnp_e=0;

FH_UINT32 g_nna_FBPhyAddr = 0;

#if 1
static uint32_t npu_getMdlInputNum(T_TY_ModelDesc *desc){
    return desc->ioDesc.inputNum;
}

static uint32_t npu_getMdlOutputNum(T_TY_ModelDesc *desc){
    return desc->ioDesc.outputNum;
}

static void npu_freeTaskInOutBuf(T_TY_TaskInput inVec[], uint32_t inNum, T_TY_TaskOutput outVec[], uint32_t outNum){
    uint32_t i=0;
    for(; i<inNum; i++){
        npu_freeMem(inVec[i].dataIn.virAddr, inVec[i].dataIn.phyAddr, inVec[i].dataIn.size);
    }
    i=0;
    for(; i<outNum; i++){
        npu_freeMem(outVec[i].dataOut.virAddr, outVec[i].dataOut.phyAddr, outVec[i].dataOut.size);
    }

    free(inVec);
    free(outVec);
}

#ifndef FH_APP_NN_YUV
unsigned long long getus(void)
{
    unsigned long long usec;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    usec = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
    return usec;
}
#endif

static uint32_t npu_prepareModelInOutCache(T_TY_ModelDesc *desc, stNpuOP *npuOp, T_TY_TaskInput inVec[], uint32_t inNum, T_TY_TaskOutput outVec[], uint32_t outNum){
    assert(desc->ioDesc.inputNum == npuOp->inImgNum);
    assert(desc->ioDesc.inputNum == inNum);
    assert(desc->ioDesc.outputNum== outNum);

    int ret = FH_SUCCESS;
    uint32_t idx=0;

    for(; idx<desc->ioDesc.inputNum; idx++){
        T_TY_TaskInput *ptrTyIn = &inVec[idx];
        stImage *ptrImg = &npuOp->inImageVec[idx];
        T_TY_BlobDesc *ptrBlobDesc = &desc->ioDesc.in[idx];

        ptrTyIn->descIn = *ptrBlobDesc;
        if(ptrTyIn->descIn.type == E_TY_BLOB_DATA){
            ptrTyIn->dataIn.size = npu_getBlobSize(ptrBlobDesc);
        }else if(ptrTyIn->descIn.type == E_TY_BLOB_IMAGE_WITH_PRE_PROC){
            ptrTyIn->descIn.type = E_TY_BLOB_IMAGE;
            ptrTyIn->descIn.img.picFormat = ptrImg->fmt;
            ptrTyIn->descIn.img.picWidth  = ptrImg->width;
            ptrTyIn->descIn.img.picHeight = ptrImg->height;

            ptrTyIn->descIn.img.picWidthStride = ptrTyIn->descIn.img.picWidth;  //根据图片实际stride填写
            ptrTyIn->descIn.img.picHeightStride= ptrTyIn->descIn.img.picHeight; //根据图片实际stride填写
            ptrTyIn->descIn.img.roi.x = 0;
            ptrTyIn->descIn.img.roi.y = 0;
            ptrTyIn->descIn.img.roi.width = ptrTyIn->descIn.img.picWidth;
            ptrTyIn->descIn.img.roi.height= ptrTyIn->descIn.img.picHeight;
            ptrTyIn->dataIn.size = npu_getImageSize(ptrTyIn->descIn.img.picWidth, ptrTyIn->descIn.img.picHeight, ptrTyIn->descIn.img.picFormat);
        }else{
            assert(0); //TBD,暂时没有原始模型输入Image的情况
        }

        ret = npu_mallocMem((void**)&ptrTyIn->dataIn.virAddr, &ptrTyIn->dataIn.phyAddr, ptrTyIn->dataIn.size, 8, E_TY_MEM_VMM_CACHED);
        if(ret != FH_SUCCESS)
        {
            return FH_FAILURE;
        }
    }
    idx=0;
    for(; idx<desc->ioDesc.outputNum; idx++){
        T_TY_TaskOutput *out = &outVec[idx];
        out->dataOut.size = npu_getBlobSize(&desc->ioDesc.out[idx]);
        ret = npu_mallocMem((void**)&out->dataOut.virAddr, &out->dataOut.phyAddr, out->dataOut.size, 8, E_TY_MEM_VMM_CACHED);
        if(ret != FH_SUCCESS)
        {
            return FH_FAILURE;
        }
        memset(UINT64_TO_VOIDP(out->dataOut.virAddr), 0, out->dataOut.size);
        npu_flushVmmMem(out->dataOut.virAddr, out->dataOut.phyAddr, out->dataOut.size);
    }

    return ret;
}

int sample_npu_test_init(FY_NPU_INFO *npu_info)
{
	int32_t tyRet;
    T_TY_TaskCfgParam tskCfg;
    T_TY_ModelCfgParam mdlCfg;

#ifdef FH_APP_NN_YUV
	while(!aiisp_init){
		usleep(1000*20);
	}
#else
	tyRet = TY_NPU_SysInit();
    SAMPLE_NNA_CHECK_EXPR_RET(tyRet != TY_NPU_SUCCESS, FH_FAILURE, "TY_NPU_SysInit fail, errCode:%d", tyRet);
#endif
#if 0
	uint32_t loglvl = 4;
	tyRet = TY_SDK_SetLogLevel(loglvl);
	if(TY_NPU_SUCCESS != tyRet){
		PRINTF_LOG(ERROR_LEVEL, "TY_SDK_SetLogLevel fail, errCode:%d", tyRet);
		TY_NPU_SysExit();
		return FH_FAILURE;
	}
#endif

	char *envMdlPath = "./res/";
	if(NULL == envMdlPath){
		SAMPLE_NNA_PRT("please set models path and data_in path");
		TY_NPU_SysExit();
		return FH_FAILURE;
	}
#ifdef NNP_FACE_DETECT
    printf("########## 进入人脸检测 ##########  \n");
#else
    printf("########## 进入人形检测 ##########  \n");
#endif
    /*========== 1. 分配加载模型文件的内存 =========*/
	strncpy(npu_info->npuOp.mdlPath, envMdlPath, MAX_PATH_LEN);
    printf("######################################\n");
    printf("######################################\n");
    printf("######################################\n");
    printf("############ 加载模型 #################\n");
    printf("model path:%s\n", NPU_MODEL_BIN);
    printf("######################################\n");
    printf("######################################\n");
    printf("######################################\n");
    printf("######################################\n");
	strncpy(npu_info->npuOp.mdlPath + strlen(npu_info->npuOp.mdlPath), NPU_MODEL_BIN, MAX_PATH_LEN - strlen(npu_info->npuOp.mdlPath));
	npu_info->mdlMemInfo.segNum = 1;
	npu_info->mdlMemInfo.memInfo[0].allocInfo.alignByteSize = 128;
	npu_info->mdlMemInfo.memInfo[0].allocInfo.allocType  = E_TY_MEM_VMM_CACHED;
	npu_info->mdlMemInfo.memInfo[0].allocInfo.shareType  = E_MEM_EXCLUSIVED;
	npu_info->mdlMemInfo.memInfo[0].allocInfo.size = npu_GetFileSize(npu_info->npuOp.mdlPath);

	tyRet = npu_allocMemSegments(&npu_info->mdlMemInfo);
	if(FH_SUCCESS != tyRet){
		SAMPLE_NNA_PRT("npu_allocMemSegments failed");
		TY_NPU_SysExit();
		return FH_FAILURE;
	}
    // 1.1 读取模型文件
	tyRet = npu_readFile(&npu_info->mdlMemInfo.memInfo[0].mem, npu_info->npuOp.mdlPath);
	if(FH_SUCCESS != tyRet){
		SAMPLE_NNA_PRT("read model file(%s) error", npu_info->npuOp.mdlPath);
		TY_NPU_SysExit();
		npu_freeMemSegments(&npu_info->mdlMemInfo);
		return FH_FAILURE;
	}

	npu_flushVmmMem(npu_info->mdlMemInfo.memInfo[0].mem.virAddr, npu_info->mdlMemInfo.memInfo[0].mem.phyAddr, npu_info->mdlMemInfo.memInfo[0].mem.size);
    printf("Creating detection model!\n");

	tyRet = TY_NPU_CreateModelFromPhyMem(&npu_info->mdlMemInfo, &mdlCfg, &npu_info->mdlMemInfo, &npu_info->mdlDesc, &npu_info->mdlHdl);
	if(TY_NPU_SUCCESS != tyRet){
		SAMPLE_NNA_PRT("TY_NPU_CreateModel fail, errCode:%d", tyRet);
		TY_NPU_SysExit();
		npu_freeMemSegments(&npu_info->mdlMemInfo);
		return FH_FAILURE;
	}
    printf("Create detection model success!\n");


    /*========== 2. 获取task需要的内存信息,分配task内存并创建task =========*/
	tyRet = TY_NPU_GetTaskMemSize(npu_info->mdlHdl, &tskCfg, &npu_info->tskMemInfo);
	if(TY_NPU_SUCCESS != tyRet){
		SAMPLE_NNA_PRT("TY_NPU_GetTaskMemSize fail, errCode:%d", tyRet);
		TY_NPU_ReleaseModel(npu_info->mdlHdl);
		TY_NPU_SysExit();
		npu_freeMemSegments(&npu_info->mdlMemInfo);
		return FH_FAILURE;
	}

	npu_allocMemSegments(&npu_info->tskMemInfo);
	tyRet = TY_NPU_CreateTask(npu_info->mdlHdl, &tskCfg, &npu_info->tskMemInfo, &npu_info->tskHdl);
	if(TY_NPU_SUCCESS != tyRet){
		SAMPLE_NNA_PRT("TY_NPU_CreateTask fail, errCode:%d", tyRet);
		TY_NPU_ReleaseModel(npu_info->mdlHdl);
		TY_NPU_SysExit();
		npu_freeMemSegments(&npu_info->tskMemInfo);
		npu_freeMemSegments(&npu_info->mdlMemInfo);
		return FH_FAILURE;
	}

	npu_info->npuOp.inImgNum = 1;
	npu_info->npuOp.inImageVec[0].width = NPU_INPUT_WIDTH;
	npu_info->npuOp.inImageVec[0].height= NPU_INPUT_HEIGHT;
	npu_info->npuOp.inImageVec[0].fmt   = IMAGE_PIXEL_FORMAT;

	uint32_t mdlInNum = npu_getMdlInputNum(&npu_info->mdlDesc);
	uint32_t mdlOutNum = npu_getMdlOutputNum(&npu_info->mdlDesc);

	npu_info->taskInVec = malloc(sizeof(T_TY_TaskInput) * mdlInNum);
	npu_info->taskOutVec = malloc(sizeof(T_TY_TaskOutput) * mdlOutNum);
	if((NULL == npu_info->taskInVec) || (NULL == npu_info->taskOutVec)){
		SAMPLE_NNA_PRT( "malloc input and output error, inNum %d, outNum:%d", mdlInNum, mdlOutNum);
		TY_NPU_ReleaseTask(npu_info->tskHdl);
		TY_NPU_ReleaseModel(npu_info->mdlHdl);
		TY_NPU_SysExit();
		npu_freeMemSegments(&npu_info->tskMemInfo);
		npu_freeMemSegments(&npu_info->mdlMemInfo);
		if(npu_info->taskInVec != NULL) free(npu_info->taskInVec);
		if(npu_info->taskOutVec!= NULL) free(npu_info->taskOutVec);
		return FH_FAILURE;
	}
#ifdef FH_APP_NN_YUV
	nnp_c2_pp_init(NPU_INPUT_WIDTH, NPU_INPUT_HEIGHT);
#endif
//	prepareModelInOutCache(&mdlDesc, &npuOp, taskInVec, mdlInNum, taskOutVec, mdlOutNum);
    return 0;
}

FH_SINT32 dump_vpss_frame_data(FH_VOID *addr, int size)
{
    FILE *file;

    SAMPLE_NNA_CHECK_EXPR_GOTO(addr == FH_NULL, exit, "address is null\n");

    file = fopen("rgb_512x288.rgb", "wb");

    if (file == NULL)
    {
        SAMPLE_NNA_PRT("open rgb_512x288.rgb err\n");
        return FH_FAILURE;
    }

    fwrite(addr, size, 1, file);

    fflush(file);
    fclose(file);

exit:
    return FH_FAILURE;
}

#ifndef NNP_FACE_DETECT
static FH_SINT32 npu_check_parmas(RECT_S *rect, int chn_width, int chn_height)
{
    FH_SINT32 ret = 0;

    if (rect->s32X < 0 || rect->s32Y  < 0 || rect->u32Width <= 0 || rect->u32Height <= 0 ||
		rect->s32X >= chn_width || rect->s32Y >= chn_height)
    {
        ret = FH_FAILURE;
    }

    if ((rect->s32X + rect->u32Width) > chn_width) // fix over size
    {
        rect->u32Width = chn_width - rect->s32X - 1;
    }
    if ((rect->s32Y + rect->u32Height) > chn_height)
    {
        rect->u32Height = chn_height - rect->s32Y  - 1;
    }

    return ret;
}
#endif

FH_SINT32 nna_draw_box(FH_UINT32 pAddr, FH_SINT32 mw, FH_SINT32 mh, RECT_S *box, FH_SINT32 box_num, FH_UINT32 color)
{
	if(close_nn==0){
		if(box_num == 0) wait_time++;
		else wait_time = 0;
		if(wait_time != 0 && wait_time != 4){
			return 0;
		}
		wait_time = 0;
	}
	FH_SINT32 ret;
	FH_SINT32 num_add;
	TDE2_SURFACE_S stDst;
    TDE2_RECT_S stDstRect;
	FH_SINT32 tde_handle;
    tde_handle = FH_TDE2_BeginJob();
	stDst.enColorFmt = TDE2_COLOR_FMT_ARGB1555;
    stDst.u32Width = mw;
    stDst.u32Height = mh;
    stDst.u32Stride = mw*2;
    stDst.u32PhyAddr = pAddr;
    stDst.bAlphaMax255 = 1;
    stDst.bAlphaExt1555 = 0;
    stDst.u8Alpha0 = 0;
    stDst.u8Alpha1 = 255;

	//clear screen
	stDstRect.s32Xpos = 0;
    stDstRect.s32Ypos = 0;
    stDstRect.u32Width = mw;
    stDstRect.u32Height = mh;
//	printf("[fill 0]%d-%d-%d-%d\n",stDstRect.s32Xpos,stDstRect.s32Ypos,stDstRect.u32Width,stDstRect.u32Height);


	ret = FH_TDE2_QuickFill(tde_handle, &stDst, &stDstRect, 0x0000);
	ret = FH_TDE2_EndJob(tde_handle, 1, 1, 500);

	if(box_num == 0)
	{
//		printf("have no detection result\n");
		goto exit;
	}
//	num_add = box_num%7==0 ? 7 : box_num%7;
    num_add = box_num;
	if (ret)
    {
        goto error;
    }
	for(; num_add>0; num_add--)
	{
    tde_handle = FH_TDE2_BeginJob();
		FH_SINT32 x=box[box_num-num_add].s32X, y=box[box_num-num_add].s32Y;
		FH_UINT32 w=box[box_num-num_add].u32Width, h=box[box_num-num_add].u32Height;
//		printf("[acture]x %d y %d w %d h %d num %d\n",x,y,w,h,num_add);

		if(x>mw)
		{
			x=mw;
		}
		if(y>mh)
		{
			y=mh;
		}

		if((x+w)>mw)
		{
			printf("[nna] width error(%d -- %d)\n",x,w);
			goto error;
		}
		else if((y+h)>mh)
		{
			h -= (y+h) - mh;

		}
		if(w<=0||h<=0)
		{
			continue;
		}

	    stDstRect.s32Xpos = x;
	    stDstRect.s32Ypos = y;
	    stDstRect.u32Width = w>ACTURE_LINE_W?ACTURE_LINE_W:DEFAULT_LINE_W;
	    stDstRect.u32Height = h;
//		printf("[fill 1]%d-%d-%d-%d\n",stDstRect.s32Xpos,stDstRect.s32Ypos,stDstRect.u32Width,stDstRect.u32Height);
		ret = FH_TDE2_QuickFill(tde_handle, &stDst, &stDstRect, color);
		if (ret)
	    {
	        goto error;
	    }
		stDstRect.s32Xpos = x;
	    stDstRect.s32Ypos = y;
	    stDstRect.u32Width = w;
	    stDstRect.u32Height = h>ACTURE_LINE_W?ACTURE_LINE_W:DEFAULT_LINE_W;
//		printf("[fill 2]%d-%d-%d-%d\n",stDstRect.s32Xpos,stDstRect.s32Ypos,stDstRect.u32Width,stDstRect.u32Height);
		ret = FH_TDE2_QuickFill(tde_handle, &stDst, &stDstRect, color);
		if (ret)
	    {
	        goto error;
	    }
		stDstRect.s32Xpos = w>ACTURE_LINE_W*2?((x+w)-ACTURE_LINE_W):((x+w)-DEFAULT_LINE_W);
	    stDstRect.s32Ypos = y;
	    stDstRect.u32Width = w>ACTURE_LINE_W*2?ACTURE_LINE_W:DEFAULT_LINE_W;
	    stDstRect.u32Height = h;
//		printf("[fill 3]%d-%d-%d-%d\n",stDstRect.s32Xpos,stDstRect.s32Ypos,stDstRect.u32Width,stDstRect.u32Height);
		ret = FH_TDE2_QuickFill(tde_handle, &stDst, &stDstRect, color);
		if (ret)
	    {
	        goto error;
	    }
		stDstRect.s32Xpos = x;
	    stDstRect.s32Ypos = h>ACTURE_LINE_W*2?((y+h)-ACTURE_LINE_W):((y+h)-DEFAULT_LINE_W);
	    stDstRect.u32Width = w;
	    stDstRect.u32Height = h>ACTURE_LINE_W*2?ACTURE_LINE_W:DEFAULT_LINE_W;;
//		printf("[fill 4]%d-%d-%d-%d\n",stDstRect.s32Xpos,stDstRect.s32Ypos,stDstRect.u32Width,stDstRect.u32Height);
		ret = FH_TDE2_QuickFill(tde_handle, &stDst, &stDstRect, color);
		if (ret)
	    {
	        goto error;
	    }
    	ret = FH_TDE2_EndJob(tde_handle, 1, 1, 500);

	}
exit:

#if 0
	ret = FH_TDE2_EndJob(tde_handle, 1, 1, 500);
    if (ret)
    {
        printf("FH_TDE2_EndJob failed! ret:0x%x(%d)\n",ret,ret);
    }
#endif
    return ret;

error:
    printf("[nna] FH_TDE2_QuickFill failed! ret:0x%x(%d)\n",ret,ret);
    FH_TDE2_CancelJob(tde_handle);
	return FH_FAILURE;
}
//int stop_updata = 0;

int vpu_gosd_update_box(int grpid, int chn, int index, RECT_S *box, int box_num)
{
//	printf("vpu_gosd_update_box\n ");
    int ret = 0;
	//get the current osd
	FH_VPU_LOGOV2 logo;
	memset(&logo, 0, sizeof(logo));
	logo.logo_idx = index;
	ret = FH_VPSS_GetChnGraphV2(grpid, chn, &logo);
	SAMPLE_NNA_CHECK_EXPR_GOTO(ret, exit, "FH_VPSS_GetChnGraphV2 failed! ret = 0x%x", ret);

	//allocate the memory, generate the bitmap
	int bitmap_w = logo.logo_cfg.logo_size.u32Width; /* 位图的宽 */
	int bitmap_h = logo.logo_cfg.logo_size.u32Height; /* 位图的高 */
	FH_UINT32 u32PhyAddr = 0;

	/* get video buffer block form common pool */
	u32PhyAddr = logo.logo_addr;
//	printf("********* index = %d  w = %d h = %d phyaddr = %d *********\n", logo.logo_idx, bitmap_w, bitmap_h, u32PhyAddr);

	if (u32PhyAddr == 0) {
		printf("ERR: SetUserPic FH_VPSS_GetChnGraphV2 logo get phyaddr failed!\n");
		ret = -__LINE__;
		goto exit;
	}
//	RECT_S boxt[1] = {{160,160,640,640},{320,320,640,640}};
//	box_num = 1;
//	if(stop_updata == 0)
//	{
//	sample_isp_pq_ae(bitmap_w, bitmap_h, box, box_num);
//	for (int box_sum = box_num; box_sum>=0;){
	nna_draw_box(u32PhyAddr, bitmap_w, bitmap_h, box, box_num, COLOR_FLASSIER);
//		box_sum = 0;
//	}
//		stop_updata = 1;
//	}
exit:

	return ret;

}

static FH_SINT32 nna_draw_rect_ext(FY_NPU_INFO *nna_info, FH_UINT32 mdlOutNum, FH_SINT32 width, FH_SINT32 height, int grpid)
{
	FH_SINT32 ret = 0;
    FH_SINT32 i = 0;
#ifndef NNP_FACE_DETECT
    FH_FLOAT *box_infor = NULL;
    FH_FLOAT *score = NULL;
//	FH_SINT32 *picture;
	FH_UINT32 box_size;
//	FILE* pic;
//	FH_VPU_STREAM_ADV frameinfo;
	RECT_S box_test;
	uint32_t nums = nna_info->taskOutVec[1].dataOut.size / sizeof(float);
#endif
    FH_SINT32 box_id = 0;
    RECT_S *box;

#ifdef NNP_FACE_DETECT
    FH_FACE_DETECTION_T result;

	float *paOut[6];
	int idx;
#ifdef FH_APP_NN_YUV
	for(idx=0; idx<mdlOutNum; idx++){
	 	paOut[idx] = (float *)nna_info->taskOutVec[idx].dataOut.virAddr;
		npu_flushVmmMem(nna_info->taskOutVec[idx].dataOut.virAddr,
						nna_info->taskOutVec[idx].dataOut.phyAddr,
						nna_info->taskOutVec[idx].dataOut.size);
//	 	printf("taskOutVec[%d].dataOut.phyAddr = [%08llx], size=[%d]\n\n", idx, nna_info->taskOutVec[idx].dataOut.phyAddr, nna_info->taskOutVec[idx].dataOut.size);
	}

	nnp_c2_pp_nms(paOut, &result);
#else
    nnp_postprocess_nms((float *)nna_info->taskOutVec->dataOut.virAddr, &result);
#endif
	box = malloc(sizeof(RECT_S) * result.boxNum);

    for (i = 0; i < result.boxNum; i++)
    {
    	box[i].s32X = ALIGNTO((FH_UINT32)(result.detBBox[i]->bbox.x1 * width), 2);
    	box[i].s32Y = ALIGNTO((FH_UINT32)(result.detBBox[i]->bbox.y1 * height), 2);
    	box[i].u32Width = ALIGNTO((FH_UINT32)((result.detBBox[i]->bbox.x2 - result.detBBox[i]->bbox.x1) * width), 2);
    	box[i].u32Height = ALIGNTO((FH_UINT32)((result.detBBox[i]->bbox.y2 - result.detBBox[i]->bbox.y1) * height), 2);
    }
    box_id = result.boxNum;

	ret = vpu_gosd_update_box(grpid, 0, 2, box, box_id);
	free(box);

    // free result
    nnp_postprocess_free_result(&result);

#else
    if (mdlOutNum == 2)
    {
//    	memset(&frameinfo,0,sizeof(FH_VPU_STREAM_ADV));
        box_infor = (FH_FLOAT *)(FH_UINT32)(nna_info->taskOutVec[0].dataOut.virAddr);
        score = (FH_FLOAT *)(FH_UINT32)(nna_info->taskOutVec[1].dataOut.virAddr);
		box_size = 100;
		if (box_size == 0)
			goto exit;
		box = malloc(sizeof(RECT_S) * box_size);
//		memset(&box, 0, sizeof(RECT_S) * box_size);
        box_id = 0;

        for (i = 0; i < nums; i++)
        {
            if (*score > 0.7)
            {
                box_test.s32X = ALIGNTO((FH_SINT32)(box_infor[1] * width), 2);
                box_test.s32Y = ALIGNTO((FH_SINT32)(box_infor[0] * height), 2);
                box_test.u32Width = ALIGNTO((FH_UINT32)((box_infor[3] - box_infor[1]) * width), 2);
                box_test.u32Height = ALIGNTO((FH_UINT32)((box_infor[2] - box_infor[0]) * height), 2);

                if (!npu_check_parmas(&box_test, width, height)) // over size check
                {
//                    SAMPLE_NNA_PRT("detect box idx[%d], pos[%d-%d-%d-%d], score=[%f]\n", box_id, box_test.s32X, box_test.s32Y, box_test.u32Width, box_test.u32Height, *score);
					box[box_id] = box_test;
					box_id++;
                }
            }
            box_infor += 4; // addr offset
            score += 1;
        }
        printf("detect box num[%d] width[%d] height[%d] grpid[%d]\n", box_id, width, height, grpid);
        for (size_t i = 0; i < box_id; i++)
        {
            printf("[%d]------------ x:%d y:%d w:%d h:%d\n", i, box[i].s32X, box[i].s32Y, box[i].u32Width, box[i].u32Height);
        }

		ret = vpu_gosd_update_box(grpid, 0, 2, box, box_id);
		free(box);
    }
exit:

#endif
    return ret;
}

FH_VOID sample_nn_change_modle(FH_VOID)
{
	close_nn = close_nn==0?1:0;
	if(close_nn == 1)
	{
		RECT_S *box = NULL;
		vpu_gosd_update_box(0, 0, 2, box, 0);
	}

	printf("nn_modle is %s\n", close_nn==0?"opened":"closed");
}


FH_VOID* sample_npu_detect_thread(FH_VOID* pAtr)
{
	FH_SINT32 ret;
	FH_SINT32 chn;
    FH_SINT32 vpssGrp;
    FH_UINT32 u32ChnNum;
    FY_NPU_INFO npu_info;
    E_TY_NpuID npuId = 0;
    FH_UINT32 mdlInNum = 0;
    FH_UINT32 mdlOutNum = 0;
    FH_UINT8 *pPhyAddr[3] = {NULL};//, *pGaddr = NULL, *pBaddr = NULL;
    FH_UINT32 u32VpssFrameWidth, u32VpssFrameHeight;
    FH_VPU_STREAM_ADV frameinfo;
    FH_UINT32 handle_lock;
    FH_UINT32 u32ComponentSize;
    FH_UINT32 u32Cnt = 0;
    FH_UINT32 i;
	FH_UINT32 frame_id = 0;

    memset(&npu_info, 0, sizeof(FY_NPU_INFO));

    u32ChnNum = 1;
    ret = sample_npu_test_init(&npu_info);
    SAMPLE_NNA_CHECK_EXPR_GOTO(ret != FH_SUCCESS, exit_0, "sample_pp_npu_init failed\n");

    mdlInNum = npu_getMdlInputNum(&npu_info.mdlDesc);
    mdlOutNum = npu_getMdlOutputNum(&npu_info.mdlDesc);

    ret = npu_prepareModelInOutCache(&npu_info.mdlDesc, &npu_info.npuOp, npu_info.taskInVec, mdlInNum, npu_info.taskOutVec, mdlOutNum);
    SAMPLE_NNA_CHECK_EXPR_GOTO(ret != FH_SUCCESS, exit_0, "npu_prepareModelInOutCache error\n");
    printf("begin nna detection \n");
	sleep(1);
	time_nnp_s = getus();
	while(g_npu_test_start)
	{
		if (close_nn)
		{
			usleep(500*1000);
			continue;
		}
#if 0
		if ((ret = FH_VPSS_LockChnFrameAdv(0,RGB_CHN_DEFAULT,&frameinfo,DEFAULT_YC_TIMEOUT, &handle_lock)) != FH_SUCCESS)
            {
                printf("[nna]FH_VPSS_LockChnFrameAdv error: 0x%x ,grp %d -- chn %d\n", ret,vpssGrp,RGB_CHN_DEFAULT);
                usleep(500);
                continue;
       }
		if ((ret = FH_VPSS_UnlockChnFrameAdv(0, RGB_CHN_DEFAULT, &frameinfo, handle_lock))!= FH_SUCCESS)
            {
                SAMPLE_NNA_PRT("FH_VPSS_UnlockChnFrameAdv error: 0x%x ,grp %d -- chn %d\n", ret,vpssGrp,RGB_CHN_DEFAULT);
       }
	usleep(1*1000);
#else
    	for(chn=0;chn<u32ChnNum;chn++)
		{
            /*==== 1. get src data from vpu====*/
            memset(&frameinfo,0,sizeof(FH_VPU_STREAM_ADV));
#if(defined(VIDEO_GRP0))
			vpssGrp = 0;
#elif(defined(VIDEO_GRP1))
			vpssGrp = 1;
#elif(defined(VIDEO_GRP2))
			vpssGrp = 2;
#endif

            if ((ret = FH_VPSS_LockChnFrameAdv(vpssGrp,RGB_CHN_DEFAULT,&frameinfo,DEFAULT_YC_TIMEOUT, &handle_lock)) != FH_SUCCESS)
            {
                printf("[nna]FH_VPSS_LockChnFrameAdv error: 0x%x ,grp %d -- chn %d\n", ret,vpssGrp,RGB_CHN_DEFAULT);
                usleep(500);
                continue;
            }

            u32VpssFrameWidth =  frameinfo.size.u32Width;
            u32VpssFrameHeight =  frameinfo.size.u32Height;
            u32ComponentSize = u32VpssFrameWidth*u32VpssFrameHeight;
            //printf("vpssGrp:%d \n", vpssGrp);
            SAMPLE_NNA_PRT("u32VpssFrame w*h = [%d*%d]\n", u32VpssFrameWidth, u32VpssFrameHeight);

#ifdef NNP_FACE_DETECT
            pPhyAddr[0] = FH_SYS_Mmap(frameinfo.frm_scan.luma.data.base, u32ComponentSize);
            pPhyAddr[1] = FH_SYS_Mmap(frameinfo.frm_scan.chroma.data.base, u32ComponentSize/2);
            if(pPhyAddr[0] == NULL || pPhyAddr[1] == NULL)
            {
                SAMPLE_NNA_PRT("FY_MPI_SYS_MmapCache failed， R/G/B vaddr[%p/%p/%p]\n", pPhyAddr[0], pPhyAddr[1]);
                goto exit_0;
            }
#if 1//def USE_OLD_MODEL
            memcpy((void *)npu_info.taskInVec[0].dataIn.virAddr,pPhyAddr[0], u32ComponentSize);
            memcpy((void *)npu_info.taskInVec[0].dataIn.virAddr+u32ComponentSize, pPhyAddr[1], u32ComponentSize/2);
            if(u32Cnt == 0)
                dump_vpss_frame_data(UINT64_TO_VOIDP(npu_info.taskInVec[0].dataIn.virAddr), u32ComponentSize*3/2);
            u32Cnt++;
#else
            npu_info.taskInVec[0].dataIn.phyAddr = frameinfo.stVFrame.u32PhyAddr[0];
            npu_info.taskInVec[0].dataIn.virAddr = frameinfo.stVFrame.u32PhyAddr[0];
#endif
#else
            pPhyAddr[0] = FH_SYS_Mmap(frameinfo.frm_rrggbb.r_data.data.base, u32ComponentSize);
            pPhyAddr[1] = FH_SYS_Mmap(frameinfo.frm_rrggbb.g_data.data.base, u32ComponentSize);
            pPhyAddr[2] = FH_SYS_Mmap(frameinfo.frm_rrggbb.b_data.data.base, u32ComponentSize);
            if(pPhyAddr[0] == NULL || pPhyAddr[1] == NULL || pPhyAddr[2] == NULL)
            {
                SAMPLE_NNA_PRT("FY_MPI_SYS_MmapCache failed， R/G/B vaddr[%p/%p/%p]\n", pPhyAddr[0], pPhyAddr[1], pPhyAddr[2]);
                goto exit_0;
            }
#if 1//def USE_OLD_MODEL
            memcpy(UINT64_TO_VOIDP(npu_info.taskInVec[0].dataIn.virAddr),pPhyAddr[0], u32ComponentSize);
            memcpy(UINT64_TO_VOIDP(npu_info.taskInVec[0].dataIn.virAddr+u32ComponentSize), pPhyAddr[1], u32ComponentSize);
            memcpy(UINT64_TO_VOIDP(npu_info.taskInVec[0].dataIn.virAddr+u32ComponentSize*2), pPhyAddr[2], u32ComponentSize);

            if(u32Cnt == 0)
                dump_vpss_frame_data(UINT64_TO_VOIDP(npu_info.taskInVec[0].dataIn.virAddr), u32ComponentSize*3);
            u32Cnt++;
#else
            npu_info.taskInVec[0].dataIn.phyAddr = frameinfo.stVFrame.u32PhyAddr[0];
            npu_info.taskInVec[0].dataIn.virAddr = frameinfo.stVFrame.u32PhyAddr[0];
#endif
#endif
            //flush output buffer
            for(i=0; i<mdlOutNum; i++)
                npu_flushVmmMem(npu_info.taskOutVec[i].dataOut.virAddr, npu_info.taskOutVec[i].dataOut.phyAddr, npu_info.taskOutVec[i].dataOut.size);

//            flushVmmMem(npu_info.taskInVec[0].dataIn.virAddr, npu_info.taskInVec[0].dataIn.phyAddr, npu_info.taskInVec[0].dataIn.size);
#ifdef FH_APP_NN_YUV
			ret = TY_NPU_ForwardPriority(npu_info.tskHdl, npuId, mdlInNum, npu_info.taskInVec, mdlOutNum, npu_info.taskOutVec, 0);
#else
			ret = TY_NPU_Forward(npu_info.tskHdl, npuId, mdlInNum, npu_info.taskInVec, mdlOutNum, npu_info.taskOutVec);
#endif
			if(ret != FH_SUCCESS)
            {
                SAMPLE_NNA_PRT("TY_NPU_Forward failed, ret=[%#x]\n", ret);
            }

#ifdef NNP_FACE_DETECT
            FH_SYS_Munmap((FH_VOID *)pPhyAddr[0], u32ComponentSize);
            FH_SYS_Munmap((FH_VOID *)pPhyAddr[1], u32ComponentSize/2);
#else
            FH_SYS_Munmap((FH_VOID *)pPhyAddr[0], u32ComponentSize);
            FH_SYS_Munmap((FH_VOID *)pPhyAddr[1], u32ComponentSize);
            FH_SYS_Munmap((FH_VOID *)pPhyAddr[2], u32ComponentSize);
#endif
        	if(TY_NPU_SUCCESS != ret){
                if ((ret = FH_VPSS_UnlockChnFrameAdv(vpssGrp, RGB_CHN_DEFAULT, &frameinfo, handle_lock))!= FH_SUCCESS)
                {
                    SAMPLE_NNA_PRT("FH_VPSS_UnlockChnFrameAdv error: 0x%x ,grp %d -- chn %d\n", ret,vpssGrp,RGB_CHN_DEFAULT);
                }
                usleep(500);
                SAMPLE_NNA_PRT("TY_NPU_Forward failed \n");

                continue;
        	}

            if ((ret = FH_VPSS_UnlockChnFrameAdv(vpssGrp, RGB_CHN_DEFAULT, &frameinfo, handle_lock))!= FH_SUCCESS)
            {
                SAMPLE_NNA_PRT("FH_VPSS_UnlockChnFrameAdv error: 0x%x ,grp %d -- chn %d\n", ret,vpssGrp,RGB_CHN_DEFAULT);
            }
#if(defined(VIDEO_GRP0))
            nna_draw_rect_ext(&npu_info, mdlOutNum, CH0_WIDTH_G0, CH0_HEIGHT_G0, vpssGrp);
#elif(defined(VIDEO_GRP1))
            nna_draw_rect_ext(&npu_info, mdlOutNum, CH0_WIDTH_G1, CH0_HEIGHT_G1, vpssGrp);
#elif(defined(VIDEO_GRP2))
            nna_draw_rect_ext(&npu_info, mdlOutNum, CH0_WIDTH_G2, CH0_HEIGHT_G2, vpssGrp);
#endif

    	}
		frame_id++;
		if (frame_id % 100 == 0)
		{
			time_nnp_e = getus();
			printf("[detect]FRAME 100 use %lldms,FPS:%lld\n", (time_nnp_e - time_nnp_s) / 1000, 1000 * 1000 * 100 / (time_nnp_e - time_nnp_s));
			time_nnp_s = time_nnp_e;
		}
//        usleep(50*1000);
#endif

	}

exit_0:
    if (npu_info.tskHdl != NULL)
    {
        ret = TY_NPU_ReleaseTask(npu_info.tskHdl);
        if (ret)
        {
            printf("Error[FH_NN]: TY_NPU_ReleaseTask failed, ret = %x\n", ret);
        }
    }

    if (npu_info.mdlHdl != NULL)
    {
        ret = TY_NPU_ReleaseModel(npu_info.mdlHdl);
        if (ret)
        {
            printf("Error[FH_NN]: TY_NPU_ReleaseModel failed, ret = %x\n", ret);
        }
    }

    TY_NPU_SysExit();

    if (npu_info.taskInVec != NULL && npu_info.taskOutVec != NULL)
    {
        npu_freeMemSegments(&npu_info.tskMemInfo);
        npu_freeMemSegments(&npu_info.mdlMemInfo);
        npu_freeTaskInOutBuf(npu_info.taskInVec, mdlInNum, npu_info.taskOutVec, mdlOutNum);
    }
	for(chn=0;chn<u32ChnNum;chn++)
	{
		FH_VPU_LOGOV2 logo;
		logo.logo_idx = 2;
		ret = FH_VPSS_GetChnGraphV2(vpssGrp, chn, &logo);
		FH_SYS_VmmFree(logo.logo_addr);
	}

	return FH_NULL;
}

    #endif //#if 0
static int __vpu_gosd_draw_box(int grpid, int chn, int index)
{
    int ret = 0;
    FH_VPU_CHN_CONFIG stChnAttr;

    ret = FH_VPSS_GetChnAttr(grpid, chn, &stChnAttr);

    //allocate the memory, generate the bitmap
    int bitmap_w = ALIGNTO(stChnAttr.vpu_chn_size.u32Width - 15, 16); /* 位图的宽 */
    int bitmap_h = stChnAttr.vpu_chn_size.u32Height; /* 位图的高 */

    FH_UINT32 u32PhyAddr;
    FH_UINT8 *pVirAddr;
    int size = bitmap_w * bitmap_h * 2;
//	printf("*******size = %d  w = %d  h = %d*********\n", size, bitmap_w, bitmap_h);

	ret = FH_SYS_VmmAlloc(&u32PhyAddr,(FH_VOID **)&pVirAddr, "npu-gosd", NULL, size);
    if (ret) {
        printf("FH_SYS_VmmAlloc failed! ret = 0x%x\n", ret);
		goto exit;
    }

	memset(pVirAddr, 0, size);

    //set the gosd
    FH_VPU_LOGOV2 logo;
    memset(&logo, 0, sizeof(logo));
    logo.logo_enable = 1;
    logo.logo_idx    = index;
    logo.logo_pixdepth = 16;  // ARGB1555
    logo.logo_startpos.u32X = 0;
    logo.logo_startpos.u32Y = 0;
    logo.stride_value = bitmap_w * 2;
    logo.logo_addr = u32PhyAddr;
    logo.logo_cfg.global_alpha_en = 1;
    logo.logo_cfg.global_alpha = 255;
    logo.logo_cfg.rgb16_type = 2;
    logo.logo_cfg.logo_size.u32Width = bitmap_w;
    logo.logo_cfg.logo_size.u32Height = bitmap_h;

    ret = FH_VPSS_SetChnGraphV2(grpid, chn, &logo);
    if (ret) {
        printf("FH_VPSS_SetChnGraphV2 fsailed! ret = 0x%x\n", ret);
		goto exit;
    }

exit:

    return ret;
}

static int __npu_det_init(int grpid, int chan_vpu, int index)
{
	int ret;

    ret = FH_TDE2_Open();
    if(ret < 0)
    {
        printf("open tde failed errno:%d\n", ret);
        return FH_FAILURE;
    }
//	printf("***********before __vpu_gosd_draw_box*********\n");
	__vpu_gosd_draw_box(grpid, chan_vpu, index);


    return FH_SUCCESS;

}


FH_SINT32 sample_npu_test_start()
{
    int ret = FH_SUCCESS;

#if(defined(VIDEO_GRP0))
	ret = __npu_det_init(0,0,2);
#elif(defined(VIDEO_GRP1))
	ret = __npu_det_init(1,0,2);
#elif(defined(VIDEO_GRP2))
	ret = __npu_det_init(2,0,2);
#endif


	if( !g_npu_test_start )
	{
        g_npu_test_start = FH_TRUE;
		ret = pthread_create(&g_sample_npu_thrd, 0, sample_npu_detect_thread, NULL);
	}
 	return ret;

}
FH_SINT32 sample_npu_test_stop()
{
	FH_TDE2_Close();
	if(g_npu_test_start)
	{
		g_npu_test_start = FH_FALSE;

        if(g_sample_npu_thrd)
    		pthread_join(g_sample_npu_thrd, NULL);
	}
	return FH_SUCCESS;
}

#endif //#if defined(SAMPLE_NPU_TEST)