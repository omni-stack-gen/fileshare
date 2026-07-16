#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "tuya_cloud_types.h"
#include "tkl_media.h"
#include "tkl_video_enc.h"

/**
* @brief video encode set osd
*
* @param[in] vi_chn: vi channel number
* @param[in] venc_chn: venc channel number
* @param[in] idr_type: request idr type
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_venc_set_IDR(TKL_VI_CHN_E vi_chn, TKL_VENC_CHN_E venc_chn,  TKL_VENC_IDR_E idr_type)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief video settings format
*
* @param[in] vi_chn: vi channel number
* @param[in] venc_chn: venc channel number
* @param[in] pformat: format config
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_venc_set_format(TKL_VI_CHN_E vi_chn, TKL_VENC_CHN_E venc_chn, TKL_VENC_FORMAT_T *pformat)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief video get format
*
* @param[in] vi_chn: vi channel number
* @param[in] venc_chn: venc channel number
* @param[out] pformat: format config
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_venc_get_format(TKL_VI_CHN_E vi_chn, TKL_VENC_CHN_E venc_chn, TKL_VENC_FORMAT_T *pformat)
{
    return OPRT_NOT_SUPPORTED;
}


/**
 * @brief video time callback
 *        Used to set osd time
 * @param[in] cb: time callback api
 *
 * @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
 */
OPERATE_RET tkl_venc_set_time_cb(TKL_VENC_TIME_CB cb)
{
    return OPRT_NOT_SUPPORTED;
}


void tuya_wifi_protect_ap_scan_mgnt_cb(UCHAR_T *buf, UINT_T len)
{
    return;
}