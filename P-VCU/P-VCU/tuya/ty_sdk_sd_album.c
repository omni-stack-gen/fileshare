#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <dirent.h>

#include "tal_system.h"
#include "plog.h"
#include "tuya_iot_config.h"
#include "tuya_ipc_api.h"
#include "tuya_ipc_media.h"
#include "tuya_ring_buffer.h"
#include "tuya_ipc_p2p.h"
#include "tuya_ipc_stream_storage.h"
#include "tuya_ipc_media_stream_event.h"
#include "ty_sdk_common.h"
#include "tuya_ipc_album.h"


/* album info */
extern CHAR_T s_raw_path[128];
#define ABS_PATH_LEN 128
#define CMD_LEN 1024
static char s_test_pic_thumbNail[ABS_PATH_LEN] = "thumbnail/pic_thumbnail.jpg";
static char s_test_mp4_thumbNail[ABS_PATH_LEN] = "thumbnail/mp4_thumbnail.jpg";
static char s_raw_emergency_path[ABS_PATH_LEN] = { 0 };
static char s_raw_emergency_thumbnail[ABS_PATH_LEN] = { 0 };
static char s_emergency_path[ABS_PATH_LEN] = { 0 };
static char s_emergency_thunbnail[ABS_PATH_LEN] = { 0 };
static int emerge_album_path_inited = 0;

/* Callback functions for transporting events */
int tuya_ipc_stor_album_cb(int event, void* args)
{
    debug("p2p rev event cb=[%d] ", event);
    switch (event)
    {
        /* 内容结构：
        C2C_ALBUM_INDEX_HEAD
        C2C_ALBUM_INDEX_ITEM
        C2C_ALBUM_INDEX_ITEM
        C2C_ALBUM_INDEX_ITEM
        */
        case MEDIA_STREAM_ALBUM_QUERY: /* query album */
        {
            C2C_QUERY_ALBUM_REQ* pSrcType = (C2C_QUERY_ALBUM_REQ*)args;

            // NOTICE: pIndexFile malloc/free in SDK
            int ret = tuya_ipc_album_query_by_name(pSrcType->albumName, pSrcType->channel, &pSrcType->fileLen, (SS_ALBUM_INDEX_HEAD_T**)&(pSrcType->pIndexFile));
            if (0 != ret) {
                error("err %d", ret);
            }

            if (pSrcType->pIndexFile) {
                C2C_ALBUM_INDEX_HEAD* ptmp = (C2C_ALBUM_INDEX_HEAD*)pSrcType->pIndexFile;
                debug("get album items %d", ptmp->itemCount);
            }
            break;
        }
        case MEDIA_STREAM_ALBUM_DOWNLOAD_START: /* start download album */
        {
            C2C_CMD_IO_CTRL_ALBUM_DOWNLOAD_START* pSrcType = (C2C_CMD_IO_CTRL_ALBUM_DOWNLOAD_START*)args;
            debug("start download\n");
            SS_DOWNLOAD_STATUS_E status = 0;
            SS_ALBUM_DOWNLOAD_START_INFO_T strStarInfo = { 0 };
            strStarInfo.session_id = pSrcType->channel;
            memcpy(strStarInfo.album_name, pSrcType->albumName, strlen(pSrcType->albumName));
            strStarInfo.file_count = pSrcType->fileTotalCnt;
            strStarInfo.thumbnail = pSrcType->thumbnail;
            strStarInfo.p_file_info_arr = (SS_FILE_PATH_T*)pSrcType->pFileInfoArr;
            int ret = tuya_ipc_album_set_download_status(SS_DL_START, &strStarInfo);
            if (0 != ret) {
                error("err %d", ret);
            }
            break;
        }
        case MEDIA_STREAM_ALBUM_DOWNLOAD_CANCEL: // cancel album
        {
            // thread do nothing...
            C2C_ALBUM_DOWNLOAD_CANCEL* pSrcType = (C2C_ALBUM_DOWNLOAD_CANCEL*)args;
            SS_ALBUM_DOWNLOAD_START_INFO_T strStarInfo = { 0 };
            strStarInfo.session_id = pSrcType->channel;
            memcpy(strStarInfo.album_name, pSrcType->albumName, strlen(pSrcType->albumName));
            int ret = tuya_ipc_album_set_download_status(SS_DL_CANCLE, &strStarInfo);
            if (0 != ret) {
                error("err %d", ret);
            }
            error("ok %d", ret);

            break;
        }
        case MEDIA_STREAM_ALBUM_DELETE: //delete
        {
            C2C_CMD_IO_CTRL_ALBUM_DELETE* pSrcType = (C2C_CMD_IO_CTRL_ALBUM_DELETE*)args;
            int ret = tuya_ipc_album_delete_by_file_info(pSrcType->channel, pSrcType->albumName, pSrcType->fileNum, (SS_FILE_PATH_T*)pSrcType->pFileInfoArr);
            if (0 != ret) {
                error("err %d", ret);
                tuya_ipc_delete_video_finish_v2(pSrcType->channel, TUYA_DOWNLOAD_ALBUM, 0);
            }
            break;
        }
        default:
            return -1;
            break;
    }
    return 0;
}


/*
  本地相册文件名要求；
  ipc_emergency_record/
        A.mp4
        B.jpg
        thumbnail/
              A.jpg
              B.jpg
  缩略图文件名和源文件文件名必须相同(除后缀外);
  源文件的文件名不可重名;

  本地相册列表交互ver1:
  1.user获取某相册路径、某相册缩略图路径，
  2.user在相册路径写入源数据（mp4/图片）
  3.user在缩略图路径写入对应缩略图
  4.user通过tuya_ipc_album_write_file_info，更新文件信息
*/

void album_file_init()
{
    if (0 == emerge_album_path_inited) {
        emerge_album_path_inited = 1;
        tuya_ipc_album_get_path(TUYA_IPC_ALBUM_EMERAGE_FILE, s_emergency_path, s_emergency_thunbnail);
        debug("get album file path %s \nthumbNail %s\n", s_emergency_path, s_emergency_thunbnail);

        snprintf(s_raw_emergency_path, ABS_PATH_LEN, "%s/resource/ipc_emergency_record", s_raw_path);
        snprintf(s_raw_emergency_thumbnail, ABS_PATH_LEN, "%s/resource/ipc_emergency_record/thumbnail", s_raw_path);
    }

}

void album_file_put()
{
    album_file_init();

    CHAR_T cmd[CMD_LEN];
    CONST CHAR_T* video_name = "1601278626_58_ch2.mp4";
    memset(cmd, 0, sizeof(cmd));

    // Emergency video generation, this step needs to be designed by yourself
    snprintf(cmd, CMD_LEN, "cp -rf %s/* %s/", s_raw_emergency_path, s_emergency_path);
    debug("cmd:%s\n",cmd);
    system(cmd);

    // write file info
    ALBUM_FILE_INFO_T strInfo;
    memset(&strInfo, 0, sizeof(ALBUM_FILE_INFO_T));
    strInfo.channel = 2;
    strInfo.type = SS_DATA_TYPE_VIDOE;
    memcpy(&strInfo.file_name, video_name, strlen(video_name));
    strInfo.create_time = 1601278626;
    strInfo.duration = 58;
    tuya_ipc_album_write_file_info(TUYA_IPC_ALBUM_EMERAGE_FILE, &strInfo);

    return;
}

