/*
 * File      : sample_vlcview.c
 * This file is part of SOCs BSP for RT-Thread distribution.
 *
 * Copyright (c) 2017 Shanghai Fullhan Microelectronics Co., Ltd.
 * All rights reserved
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Visit http://www.fullhan.com to get contact with Fullhan.
 *
 * Change Logs:
 * Date           Author       Notes
 */
//#include <sample_common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dsp_ext/FH_typedef.h>
#include "vlcview.h"
#include "sample_common_isp.h"
#include "npu_com.h"

//开了NPU的宏，才能使用NPU的功能
#if defined(SAMPLE_NPU_TEST)
#define NN_ENABLE_G0
#endif //#if defined(SAMPLE_NPU_TEST)

extern FH_SINT32 sample_common_media_sys_init(FH_VOID);
extern FH_SINT32 sample_common_media_sys_exit(FH_VOID);
FH_SINT32 sample_common_start_vpu(FH_VOID);
FH_SINT32 sample_common_start_enc(FH_VOID);
extern FH_SINT32 sample_common_stop_enc(FH_VOID);
FH_SINT32 sample_common_start_isp(FH_VOID);

extern FH_SINT32 sample_common_start_bind(FH_VOID);
extern FH_SINT32 sample_common_stop_bind(FH_VOID);

extern FH_SINT32 sample_common_start_get_stream(FH_VOID);
extern FH_SINT32 sample_common_stop_get_stream(FH_VOID);

extern FH_SINT32 sample_common_dmc_init(FH_CHAR *dst_ip, FH_UINT32 port);
extern FH_SINT32 sample_common_dmc_deinit(FH_VOID);
static FH_SINT32 g_sample_running = 0;

extern FH_SINT32 sample_nn_chan_create(FH_VOID);

static int g_is_media_inited = 0;

void let_isp_type(void);
int aov_video_exit(void);

//异步调用编码
#define ENCODE_VIDEO_RUN_BY_MY  0

#if ENCODE_VIDEO_RUN_BY_MY
FH_SINT32 Encode_video_start(void);
int Encode_video_stop(void);
#endif //ENCODE_VIDEO_RUN_BY_MY

void Isp_run_type(int isonoff)
{
#if ENCODE_VIDEO_RUN_BY_MY
        if(isonoff==1)
        {
            let_isp_type();
            Encode_video_start();
        }
        else{
            Encode_video_stop();
        }
    return ;
#endif //ENCODE_VIDEO_RUN_BY_MY
    if (!g_sample_running)
    {
        printf("vlcview is not running!\n");
        return ;
    }
    //return ;
    if (isonoff)
    {
        if(g_is_media_inited)
        {
            return;
        }
        /* code */

        let_isp_type();
        //1.
        //sample_common_media_sys_init();
        //2.
        //sample_common_start_vpu();


        //3.
        sample_common_start_enc();
        //8

        sample_common_start_isp();

        //10
        sample_common_start_bind();

        //11.
        //sample_common_dmc_init(NULL, 0);

        //12
        sample_common_start_get_stream();

        //13.
        sample_common_isp_run();

#if defined(NN_ENABLE_G0)
        sample_npu_test_start();
#endif
#if SAMPLE_ISP_RUN_ENABLE
        //18
        sample_isp_start();
#endif //SAMPLE_ISP_RUN_ENABLE
        g_is_media_inited = 1;
#ifdef ENABLE_AOV
    // void aov_creat(void);
    // aov_creat();
    int aov_ready(void);
    aov_ready();
#endif //#ifdef ENABLE_AOV
    }
    else if(g_is_media_inited)
    {
#ifdef ENABLE_AOV
        aov_video_exit();
#endif //#ifdef ENABLE_AOV
#if defined(NN_ENABLE_G0)
        sample_npu_test_stop();
        printf("=====sample_npu_test_stop=====\n");
#endif
        sample_common_stop_get_stream();
	    sample_common_stop_bind();
#if SAMPLE_ISP_RUN_ENABLE
        sample_isp_stop();
#endif //SAMPLE_ISP_RUN_ENABLE
        sample_common_stop_enc();

        sample_common_isp_stop();

        g_is_media_inited = 0;
    }
}

FH_SINT32 _vlcview_exit(FH_VOID)
{
#ifdef ENABLE_AOV
    aov_video_exit();
#endif //#ifdef ENABLE_AOV
#if ENCODE_VIDEO_RUN_BY_MY
    return Encode_video_stop();
#endif //ENCODE_VIDEO_RUN_BY_MY
    if (!g_sample_running)
    {
        printf("vlcview is not running!\n");
        return 0;
    }
    printf("=====vlc start exit=====\n");
#if 1
    //sample_common_stop_vou();
    //printf("=====sample_common_stop_vou=====\n");


#ifdef FH_APP_USING_COOLVIEW
    //sample_common_stop_coolview(0);
    printf("=====sample_common_stop_coolview=====\n");
#endif
#if defined(NN_ENABLE_G0)
        sample_npu_test_stop();
        printf("=====sample_npu_test_stop=====\n");
#endif
    //提前退出获取编码， 要不然FH_VENC_GetChnStream_Timeout 获取超时10秒
    sample_common_stop_get_stream();
	sample_common_stop_bind();
	printf("=====sample_common_stop_bind=====\n");
#if SAMPLE_ISP_RUN_ENABLE
    sample_isp_stop();
    printf("=====sample_isp_stop=====\n");
#endif //SAMPLE_ISP_RUN_ENABLE
    //sample_common_stop_send_code_stream();
    printf("=====sample_common_stop_send_code_stream=====\n");
    //sample_common_stop_send_stream();
    printf("=====%llu sample_common_stop_send_stream=====\n", time(NULL));
    sample_common_stop_get_stream();
    printf("=====%llu sample_common_stop_get_stream=====\n", time(NULL));
    //sample_common_stop_mjpeg_gend_stream();
    //printf("=====sample_common_stop_mjpeg_gend_stream=====\n");
    //sample_common_stop_vou();
    //printf("=====sample_common_stop_vou=====\n");
    //sample_common_stop_vgs();
    //printf("=====sample_common_stop_vgs=====\n");
    //sample_common_stop_vdec();
    //printf("=====sample_common_stop_vdec=====\n");
    sample_common_stop_enc();
    printf("=====sample_common_stop_enc=====\n");
    sample_common_dmc_deinit();
    printf("=====sample_common_dmc_deinit=====\n");
    sample_common_isp_stop();
    printf("=====sample_common_isp_stop=====\n");
    sample_common_media_sys_exit();
    static int index = 0;
    index++;
    printf("=====sample_common_media_sys_exit, %d=====\n", index);
#endif
    g_sample_running = 0;
    g_is_media_inited = 0;
    return 0;
}


FH_SINT32 _vlcview(FH_CHAR *dst_ip, FH_UINT32 port)
{
#if ENCODE_VIDEO_RUN_BY_MY
    let_isp_type();
    return Encode_video_start();
#endif //ENCODE_VIDEO_RUN_BY_MY
    FH_SINT32 ret = -1;

    if (g_sample_running)
    {
        printf("vlcview is running!\n");
        return 0;
    }
    let_isp_type();
    g_sample_running = 1;
    /******************************************
      step  1: init media platform
     ******************************************/
    ret = sample_common_media_sys_init();
    if (ret != 0)
    {
        printf("Error(%d - %x): sample_common_media_sys_init!\n", ret, ret);
        goto err_exit;
    }
    /******************************************
      step  2: init vpu vi and open vpu
     ******************************************/

    ret = sample_common_start_vpu();
    if (ret != 0)
    {
        printf("Error(%d - %x): sample_common_start_vpu!\n", ret, ret);
        goto err_exit;
    }


    /******************************************
      step  3: init enc
     ******************************************/
    ret = sample_common_start_enc();
    if (ret != 0)
    {
        printf("Error(%d - %x): sample_common_start_enc!\n", ret, ret);
        goto err_exit;
    }
    /************************************************
     step  4: if use smart encode type, open bgm
    ************************************************/

    /************************************************
        step  5: start vgs
    ************************************************/
    //ret = sample_common_start_vgs();
    if (ret != 0)
    {
        printf("Error(%d - %x): sample_common_start_vgs!\n", ret, ret);
        goto err_exit;
    }
    /************************************************
        step  6: start vdec
    ************************************************/
    //TODO ret = sample_common_start_vdec();
    if (ret != 0)
    {
        printf("Error(%d - %x): sample_common_start_vdec!\n", ret, ret);
        goto err_exit;
    }
    /************************************************
        step  7: start vou
    ************************************************/
    //TODO ret = sample_common_start_vou();
    if (ret != 0)
    {
        printf("Error(%d - %x): sample_common_start_vou!\n", ret, ret);
        goto err_exit;
    }
    /************************************************
        step  8: start isp
    ************************************************/
    ret = sample_common_start_isp();

    if (ret)
    {
        printf("Error(%d - %x): sample_common_start_isp!\n", ret, ret);
        goto err_exit;
    }

    /******************************************
      step  9: start init mjpeg
    ******************************************/
    //TODO ret = sample_common_start_mjpeg();
    // if (ret != 0)
    // {
    //     printf("Error(%d - %x): sample_common_start_mjpeg!\n", ret, ret);
    //     goto err_exit;
    // }

#if 0
    ret = sample_common_start_jpege();
    if(ret != 0)
    {
        printf("Error(%d - %x): sample_common_start_jpege!\n", ret, ret);
        goto err_exit;
    }
#endif

    /************************************************
        step  10: start bind
    ************************************************/
    ret = sample_common_start_bind();
    if (ret)
    {
        goto err_exit;
    }

#if defined(FH_NN_ENABLE)
    sample_nn_chan_create();
#endif
    /************************************************
      step  11: init data-manage-center which is used to dispatch stream...
     ************************************************/
    sample_common_dmc_init(dst_ip, port);
    /******************************************
      step  12: start get stream
     ******************************************/
    sample_common_start_get_stream();
    /******************************************
      step  13: isp send stream
     ******************************************/
    sample_common_isp_run();

	/************************************************
	  step	14: send yuv stream, then send to data manager..
	************************************************/
	//TODO sample_common_start_send_stream();

    /************************************************
      step  15: send code stream, then send to data manager..
    ************************************************/
    //TODO sample_common_start_send_code_stream();
#ifdef FH_APP_USING_COOLVIEW
    /******************************************
      step  16: open coolview
     ******************************************/
    //sample_common_start_coolview(NULL);
#endif
    /******************************************
      step  17: do nn human detect
     ******************************************/
#ifdef FH_APP_NN_YUV
	//nn_yuv_start();
#endif

#if defined(NN_ENABLE_G0)
    sample_npu_test_start();
#endif


    //TODO sample_common_start_mjpeg_gend_stream();

#ifdef FH_APP_OPEN_IVS
    //sample_ivs_start();
#endif

#ifdef FH_APP_OPEN_OVERLAY
    /******************************************
      step  14: add overlayer
     ******************************************/
    //sample_overlay_start();
#endif

#if defined(FH_APP_OPEN_MOTION_DETECT) ||      \
    defined(FH_APP_OPEN_RECT_MOTION_DETECT) || \
    defined(FH_APP_OPEN_COVER_DETECT)

    /******************************************
      step  17: do motion detect and cover detect
     ******************************************/
    //sample_md_cd_start();
#endif

#ifdef FH_APP_OPEN_VENC
    /******************************************
      step  18: open venc functions
     ******************************************/
    //sample_venc_start();
#endif

#if SAMPLE_ISP_RUN_ENABLE
    /******************************************
      step  18: open isp strategy functions
     ******************************************/
    sample_isp_start();
#endif //SAMPLE_ISP_RUN_ENABLE

#ifdef FH_APP_OPEN_AF
    //sample_af_start();
#endif

#ifdef FH_APP_OPEN_ABANDON_DETECT
    //sample_aod_start();
#endif


    printf("[vlcview] start successful! ------build at(%s-%s)\n", __DATE__, __TIME__);

    g_is_media_inited = 1;

#ifdef ENABLE_AOV
    // void aov_creat(void);
    // aov_creat();
    int aov_ready(void);
    aov_ready();
#endif //#ifdef ENABLE_AOV
    return 0;

err_exit:
    _vlcview_exit();
    return -1;
}
