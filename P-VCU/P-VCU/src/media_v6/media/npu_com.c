#if defined(SAMPLE_NPU_TEST)
/**
* @file utils.cpp
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include "fhhcp/base.h"
#include "npu_com.h"
#define USEFULLHAN
#ifdef USEMOLCHIP
#include "mpi_sys.h"
#endif
#ifdef USEFULLHAN
#include "types/vmm_api.h"
#endif
// Fullhan vmm free interface will free all vmm memory, only really free at the last call
static uint32_t gVmmRefCnt = 0;

int32_t npu_freeMem(uint64_t virAddr, uint64_t phyAddr, uint32_t size){

    gVmmRefCnt--;

#ifdef USEFULLHAN
        FH_SYS_VmmFreeOne64(phyAddr);
#endif
#ifdef USEMOLCHIP
        FY_MPI_SYS_MmzFree(phyAddr, (void*)virAddr);
#endif

    return 0;
}

int32_t npu_flushVmmMem(uint64_t virAddr, uint64_t phyAddr, uint32_t size){
//    int ret = FH_SUCCESS;

#ifdef USEFULLHAN
    //uintprt_t addr = virAddr;
    int32_t ret = FH_SYS_VmmFlushCache64(phyAddr, (void*)(uint32_t)virAddr, size);
#endif
#ifdef USEMOLCHIP
    int32_t ret = FY_MPI_SYS_MmzFlushCache(phyAddr, (void*)virAddr, size);
#endif
    return ret;
}

int32_t npu_mallocMem(void **ppVirAddr, uint64_t *ptrPhyAddr, uint32_t size, uint32_t align, E_TY_MemAllocType type){

    int ret = FH_SUCCESS;

    switch(type){
        case E_TY_MEM_VMM_NO_CACHED:
            #ifdef USEFULLHAN
            ret = FH_SYS_VmmAllocEx64(ptrPhyAddr, ppVirAddr, "IVE", "anonymous", size, align);
            if(ret != FH_SUCCESS)
            {
                SAMPLE_NNA_PRT("malloc size(%d) failed, ret=[%#x]\n", size, ret);
                return FH_FAILURE;
            }
            #endif
            #ifdef USEMOLCHIP
            ret = FY_MPI_SYS_MmzAlloc(ptrPhyAddr, ppVirAddr, "IVE", "anonymous", size);
            if(ret != FH_SUCCESS)
            {
                SAMPLE_NNA_PRT("FY_MPI_SYS_MmzAlloc size(%d) failed, ret=[%#x]\n", size, ret);
                return FH_FAILURE;
            }
            #endif
            gVmmRefCnt++;

        break;
        case E_TY_MEM_VMM_CACHED:

            #ifdef USEFULLHAN
            ret = FH_SYS_VmmAllocEx_Cached64(ptrPhyAddr, ppVirAddr, "TY_NPU", "anonymous", size, align);
            if(ret != FH_SUCCESS)
            {
                SAMPLE_NNA_PRT("FH_SYS_VmmAllocEx_Cached64 size(%d) failed, ret=[%#x]\n", size, ret);
                return FH_FAILURE;
            }
            ret = FH_SYS_VmmFlushCache64(*ptrPhyAddr, *ppVirAddr, size);
            if(ret != FH_SUCCESS)
            {
                SAMPLE_NNA_PRT("FH_SYS_VmmFlushCache64 cache failed, ret=[%#x]\n", ret);
                return FH_FAILURE;
            }
            #endif

            #ifdef USEMOLCHIP
            ret = FY_MPI_SYS_MmzAlloc_Cached(ptrPhyAddr, ppVirAddr, "TY_NPU", "anonymous", size);
            if(ret != FH_SUCCESS)
            {
                SAMPLE_NNA_PRT("FY_MPI_SYS_MmzAlloc_Cached size(%d) failed, ret=[%#x]\n", size, ret);
                return FH_FAILURE;
            }
            ret = FY_MPI_SYS_MmzFlushCache(*ptrPhyAddr, *ppVirAddr, size);
            if(ret != FH_SUCCESS)
            {
                SAMPLE_NNA_PRT("FY_MPI_SYS_MmzFlushCache failed, ret=[%#x]\n", ret);
                return FH_FAILURE;
            }
            #endif
            gVmmRefCnt++;

        break;
        case E_TY_MEM_MALLOC:
            posix_memalign(ppVirAddr, align, size);
            *ptrPhyAddr= (uint64_t)((uint32_t)(*ppVirAddr));
        break;
        default:
            ERROR_LOG("allocType error, %d", type);
            *ppVirAddr = NULL;
            *ptrPhyAddr= 0;
        break;
    }
    return ret;
}

int32_t npu_freeMemSegments(T_TY_MemSegmentInfo *ptrMemInfo){
    int32_t idx=0;
    for(; idx<ptrMemInfo->segNum; idx++){
        npu_freeMem(ptrMemInfo->memInfo[idx].mem.virAddr, ptrMemInfo->memInfo[idx].mem.phyAddr, ptrMemInfo->memInfo[idx].mem.size);
    }
    return 0;
}

int32_t npu_allocMemSegments(T_TY_MemSegmentInfo *ptrMemInfo){
    int32_t idx=0;
    int ret = FH_SUCCESS;

    for(; idx<ptrMemInfo->segNum; idx++){
        void **ppVirAddr = (void**)&ptrMemInfo->memInfo[idx].mem.virAddr;
        uint64_t *ptrPhyAddr = &ptrMemInfo->memInfo[idx].mem.phyAddr;
        uint32_t size = ptrMemInfo->memInfo[idx].allocInfo.size;
        uint32_t align= ptrMemInfo->memInfo[idx].allocInfo.alignByteSize;
        ret = npu_mallocMem(ppVirAddr, ptrPhyAddr, size, align, ptrMemInfo->memInfo[idx].allocInfo.allocType);
        if(ret != FH_SUCCESS)
        {
            SAMPLE_NNA_PRT("npu_mallocMem failed\n");
            return FH_FAILURE;
        }

        ptrMemInfo->memInfo[idx].mem.size = size;
    }
    return ret;
}

int32_t npu_getImageSize(uint32_t width, uint32_t height, E_TY_PixelFormat fmt){
    int32_t size =0;
    switch(fmt){
        case E_TY_PIXEL_FORMAT_U8C1:
            size = width * height * 1;
            break;
        case E_TY_PIXEL_FORMAT_U16C1:
            size = width * height * 2;
            break;
        case E_TY_PIXEL_FORMAT_YUV_SEMIPLANAR_420:
            size = width * height * 3 /2;
            break;
        case E_TY_PIXEL_FORMAT_RGB_888_PLANAR:
            size = width * height * 3;
            break;
        default:
            size =0;
            ERROR_LOG("not supported format %d", fmt);
            break;
    }
    return size;
}

int32_t npu_readFile(T_TY_Mem *ptrMem, char *path){
    int32_t fileLen = npu_GetFileSize(path);
    if(fileLen != ptrMem->size){
        ERROR_LOG("image size error, file size:%d, required:%d", fileLen, ptrMem->size);
        return -1;
    }

    FILE *fp =fopen(path, "rb");
    if(fp == NULL)
    {
        SAMPLE_NNA_PRT("open file %s failed", path);
        return FH_FAILURE;
    }

    fread((void*)(uint32_t)ptrMem->virAddr, ptrMem->size, 1, fp);
    SAMPLE_NNA_PRT("load file to addr:%p, size %d, path:%s\n", (void*)(uint32_t)ptrMem->virAddr, ptrMem->size, path);
    return FH_SUCCESS;
}

int32_t npu_writeFile(T_TY_Mem *ptrMem, char *path){
    FILE *fp =fopen(path, "wb");

    if(fp == NULL)
    {
        SAMPLE_NNA_PRT("open file %s failed", path);
        return FH_FAILURE;
    }

    fwrite((void*)(uint32_t)ptrMem->virAddr, 1, ptrMem->size, fp);
    SAMPLE_NNA_PRT("write addr:%p, size %d, to path:%s\n", (void*)(size_t)ptrMem->virAddr, ptrMem->size, path);
    return 0;
}

void npu_printBlob(T_TY_BlobDesc *blob){
    switch(blob->type){
        case E_TY_BLOB_DATA:
            SAMPLE_NNA_PRT("BLOB Tensor");
            SAMPLE_NNA_PRT("tensor name:%s", blob->tensor.name);
            SAMPLE_NNA_PRT("tensor dataType:%d", blob->tensor.dataType);
            SAMPLE_NNA_PRT("tensor numDims:%d", blob->tensor.numDims);
            SAMPLE_NNA_PRT("tensor dims:[%lld, %lld, %lld, %lld, %lld]",blob->tensor.dims[0],
                                                                 blob->tensor.dims[1],
                                                                 blob->tensor.dims[2],
                                                                 blob->tensor.dims[3],
                                                                 blob->tensor.dims[4]);
            break;
        case E_TY_BLOB_IMAGE:
            SAMPLE_NNA_PRT("BLOB Image");
            break;
        case E_TY_BLOB_IMAGE_WITH_PRE_PROC:
            SAMPLE_NNA_PRT("BLOB Image with pre proc");
        default:
            break;
    }
}

uint32_t npu_getBlobSize(T_TY_BlobDesc *blob){
    int32_t byteUnitVec[] ={4, 2, 1, 4, 1, 2, 4, 8, 8, 1}; //对应E_TY_DataType
    uint32_t blobByteSize = 0;
    int32_t unitSize = byteUnitVec[blob->tensor.dataType];
    int64_t unitNum  = 1;
    int32_t idx=0;
    switch(blob->type){
        case E_TY_BLOB_DATA:
            idx=0;
            for(; idx<blob->tensor.numDims; idx++){
                unitNum *= blob->tensor.dims[idx];
            }
            blobByteSize = unitSize * unitNum;
            break;
        case E_TY_BLOB_IMAGE:
            blobByteSize = 0;
            break;
        default:
            blobByteSize = 0;
            break;
    }
    return blobByteSize;
}

int32_t npu_GetFileSize(char *path){
    struct stat statbuf;
    int ret = stat(path, &statbuf);
    if(0 != ret){
        ERROR_LOG("read file error, path:%s\n", path);
        return 0;
    }else{
        return statbuf.st_size;
    }
}
#endif //#if defined(SAMPLE_NPU_TEST)