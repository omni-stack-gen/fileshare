#ifndef __common_h__
#define __common_h__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "pthread.h"
#include "time.h"
#include <signal.h>
#include <fcntl.h>
#include <stddef.h>
#include <config.h>
#include <sys/prctl.h>
#include "types/type_def.h"
#include "types/bufCtrl.h"

// #include <libdbi_over_tcp/include/dbi_over_tcp.h>
// #include <librtsp/include/librtsp.h>
// #include <libdmc/include/libdmc.h>
// #include <libdmc/include/libdmc_pes.h>
// #include <libdmc/include/libdmc_rtsp.h>
// #include <libdmc/include/libdmc_record_raw.h>
// #include <libdmc/include/libdmc_http_mjpeg.h>

// #include <libpes/include/libpes.h>
// #include <librtsp/include/librtsp.h>
// #include <libscaler/include/fh_scaler.h>
// #include <librect_merge_by_gaus/include/rect_merge_by_gaus.h>
// #include <libmisc/include/libmisc.h>
// #ifdef ISP460
// #include <isp/include/sample_common_isp460.h>
// #else
// #include <isp/include/sample_common_isp.h>
// #endif
// #include <dsp/include/sample_common_dsp.h>
// #include <overlay/sample_overlay.h>
// #include <motion_and_cover/sample_md_cd.h>
// #include <bgm/include/sample_bgm.h>
// #include <venc/sample_venc.h>
//#include <fh_nn_detect/sample_nna_detect.h>
// #include <isp_strategy/sample_isp.h>
// #include <af_demo/sample_af.h>
// #include <ivs_demo/sample_ivs.h>
// #include <abandon_detect/sample_abandon_detect.h>

// #include <bind/include/sample_bind.h>
// #include <dual_camera/include/dual_camera.h>
// #include <vou/include/sample_common_vou.h>
// #include <jpege/include/sample_common_jpege.h>
// #include <vgs/include/sample_common_vgs.h>
// #include <vdec/include/sample_common_vdec.h>
// #ifdef FH_APP_NN_YUV
// #include <ty_nn_detect/include/sample_nna_detect.h>
// #endif

// #include <npu/include/npu_com.h>


// #include "FHAdv_OSD_mpi.h"

#define ALIGN_UP(addr, edge)   ((addr + edge - 1) & ~(edge - 1)) /* 数据结构对齐定义 */
#define ALIGN_BACK(addr, edge) ((edge) * (((addr) / (edge))))
#define ALIGNTO(addr, edge) ((addr + edge - 1) & ~(edge - 1))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define CLIP(x, min, max) MAX((min), MIN((x), (max)))

#define ISP_PROC "/proc/driver/isp"
#define VPU_PROC "/proc/driver/vpu"
#define BGM_PROC "/proc/driver/bgm"
#define ENC_PROC "/proc/driver/enc"
#define JPEG_PROC "/proc/driver/jpeg"
#define TRACE_PROC "/proc/driver/trace"

#define WR_PROC_DEV(device,cmd)  do{ \
    int _tmp_fd; \
    _tmp_fd = open(device, O_RDWR, 0); \
    if(_tmp_fd >= 0) { \
        write(_tmp_fd, cmd, sizeof(cmd)); \
        close(_tmp_fd); \
    } \
}while(0)


#define SAMPLE_DEBUG
#ifdef SAMPLE_DEBUG
#define LOG_PRT(msg, ...)   \
    do {\
          printf("[%s:%d]: " msg "\n",  __func__, __LINE__, ##__VA_ARGS__); \
       }while(0)
#else
#define LOG_PRT(msg, ...) do{}while(0)
#endif


#define DMC_MEDIA_TYPE_H264   	(1<<8)  /*H264*/
#define DMC_MEDIA_TYPE_H265    	(1<<9)  /*H265*/
#define DMC_MEDIA_TYPE_JPEG    	(1<<10) /*JPEG*/
#define DMC_MEDIA_TYPE_AUDIO   	(1<<11) /*Audio*/
#define DMC_MEDIA_TYPE_MJPEG    (1<<12) /*motion JPEG*/

/*DMC_MEDIA_TYPE_AUDIO: 媒体子类型*/
#define DMC_MEDIA_SUBTYPE_PCM  (1<<0)
#define DMC_MEDIA_SUBTYPE_ALAW (1<<1)
#define DMC_MEDIA_SUBTYPE_ULAW (1<<2)
#define DMC_MEDIA_SUBTYPE_AAC  (1<<3)

/*DMC_MEDIA_TYPE_H264/DMC_MEDIA_TYPE_H265: 媒体子类型*/
#define DMC_MEDIA_SUBTYPE_IFRAME  (1<<0)
#define DMC_MEDIA_SUBTYPE_PFRAME  (1<<1)
#define DMC_MEDIA_SUBTYPE_BFRAME  (1<<2)

#endif /*__common_h__*/

