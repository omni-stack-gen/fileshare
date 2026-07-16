#ifndef __SOUND_PLAY_H__
#define __SOUND_PLAY_H__

#include <common_defs.h>

#define PLAY_FOREVER            (UINT32_MAX)

#if 1
#ifdef ENABLE_V6
#define SOUND_PATH              "/system/sound/"
#else
#define SOUND_PATH              "/mnt/app/sound/"
#endif //ENABLE_V6
#else
#define SOUND_PATH              "/mnt/nfs/V1/code/PLC_Door/output/sound/"
#endif

#define KEYPAD_INDEX                    SOUND_PATH"keyvol.wav"
#define RING_MP3                        SOUND_PATH"ring.mp3" //SOUND_PATH"ring.mp3" // "/tmp/ring.mp3"//
#define OFFLINE_MP3                     SOUND_PATH"dev_offline_e.mp3"
#define UNLOCK_MP3                      SOUND_PATH"unlock_e.mp3"
#define BEEP_WAV                        SOUND_PATH"beep4.wav"
#define UNLOCK_MP3_PATH                 SOUND_PATH"unlock_s/"

#define UPGRADED_TIPS                   SOUND_PATH"upgraded.mp3"              // 设备已升级，请放心使用.mp3
#define UPGRADING_TIPS                  SOUND_PATH"upgrading.mp3"             // 升级中设备无法使用，请稍候.mp3
#define UPGRAD_START_TIPS               SOUND_PATH"upgarde_start.mp3"         // 正在升级，请勿断电.mp3

#define CONNECT_IOT_FAIL_TIPS           SOUND_PATH"connect_iot_fail.mp3"      // 登录服务器连接异常，请重试.mp3
#define LOGIN_SUCCESS_TIPS              SOUND_PATH"login_success.mp3"         // 登录成功，设备可正常操作.mp3
#define SN_NOT_REG_TIPS                 SOUND_PATH"sn_not_registered.mp3"     // 请先注册设备.mp3
#define MODEL_MISMATCH_TIPS             SOUND_PATH"model_mismatch.mp3"        // 设备类型不符，请重新选择.mp3

#define CALL_FAIL_TIPS                  SOUND_PATH"call_fail.mp3"             // 呼叫失败，请稍后再试.mp3
#define NO_GET_TX_SECRET_TIPS           SOUND_PATH"no_get_tx_secret.mp3"      // 未获取音视频认证信息，请稍后再试.mp3
#define NO_LOGIN_TX_SERVER_TIPS         SOUND_PATH"no_login_tx_server.mp3"    // 未登录音视频服务器，请稍后再试.mp3
#define CALL_MODE_MISMATCH_TIPS         SOUND_PATH"call_mode_mismatch.mp3"    // 呼叫失败，呼叫模式不匹配，请检查配置.mp3
#define CALL_OFFLINE_TIPS               SOUND_PATH"call_offline.mp3"          // 呼叫失败，无法连接，请确保设备在线.mp3
#define CALL_REMOTE_OFFLINE_TIPS        SOUND_PATH"call_remote_offline.mp3"   // 呼叫失败，无法建立呼叫，请检查对方状态.mp3
#define ALARM_ELECTROMOBILE_TIPS        SOUND_PATH"alarm_tips_have_electro_mobile.mp3"   // 电瓶车语音提示.mp3

#ifdef __cplusplus
extern "C"
{
#endif

int sound_play_mgnt_init(bool control_speaker);

int sound_play_start(char *file_name, uint32_t times, uint32_t volume, bool sync);

int sound_play_stop(int play_id);

int sound_play_set_volume(int play_id, int volume);

int sound_play_tips(char *file_name, bool sync);

int sound_play_speaker_en(bool enable);


int sound_play_ring_start(int volume);

void sound_play_ring_stop(int play_id);

#ifdef __cplusplus
}
#endif

#endif /* __SOUND_PLAY_H__ */
