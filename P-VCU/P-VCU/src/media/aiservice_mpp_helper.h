#ifndef __AISERVICE_MPP_HELPER_H__
#define __AISERVICE_MPP_HELPER_H__

#include <stdio.h>
#include <plat_type.h>
#include <media/mm_comm_region.h>
#include <media/mpi_region.h>
#include "awnn/awnn_det2.h"

#define _CHECK_RET( ret )  do { \
        if( SUCCESS != ret ) { \
            printf("Error no %d.\n", ret); \
            return ret; \
        } \
    } while(0)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef int (*npu_callback_t)(void * userdata,unsigned char *pBuffer, int vipp, void *pRunParam);

ERRORTYPE mpp_helper_init(void);
ERRORTYPE mpp_helper_uninit(void);
ERRORTYPE mpp_helper_start_npu(int isp, int vipp, int vipp_buf_num, int width, int height, int fps, int format);
ERRORTYPE mpp_helper_stop_npu(int isp, int vipp);
BOOL mpp_helper_do_npu(int vipp,unsigned char *pBuffer, int width,int height, void *pRunParam, npu_callback_t npu_callback,void * userdata);

BOOL mpp_helper_getframe(int vipp,VIDEO_FRAME_INFO_S * pFrameInfo);
void mpp_helper_releaseframe(int vipp,VIDEO_FRAME_INFO_S * pFrameInfo);

// typedef struct
// {
//     int region_hdl_base;
//     int old_num_of_boxes;
// }region_info_t;

// void paint_object_detect_region(int vipp,region_info_t * pinfo,BBoxResults_t *res,float scale_w,float scale_h);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __AISERVICE_MPP_HELPER_H__ */
