#ifndef TRAN_AUDIO_H
#define TRAN_AUDIO_H

#include <stdint.h>


enum {
    AUDIO_TRAN_SEND = 1,
    AUDIO_TRAN_RECV,
    AUDIO_TRAN_SEND_AND_RECV,
    AUDIO_TRAN_BUTT
};

void *PlcAudioTrans_Start(uint32_t lport, uint32_t rdev, uint32_t rport, uint8_t trandirection, uint32_t play_volume);

void PlcAudioTrans_SetAoVolume(void*param, int volume);

void PlcAudioTrans_Stop(void*param);

void Audio_Trans_Ai_Noise_onoff(void*param, bool onoff);
#endif /* TRAN_AUDIO_H */