
#ifndef __TUYAMAIN_H__
#define __TUYAMAIN_H__
#include <stdbool.h>
#ifdef __cplusplus
extern "C"
{
#endif

bool StartTuyaServer(void);
void StopTuyaServer(void);

bool StartTuyaVideo(void);
void SendTuyVideo(char *buf, int bufsize, int type, unsigned long long int pts);
void StopTuyaVideo(void);
bool StartTuyaAudio(void);
void SendTuyaAudio(char *buf, int bufsize, unsigned long long int pts);
void StopTuyaAudio(void);


int Tuya_Start_Media_Stream(void);
int Tuya_Stop_Media_Stream(void);

int Tuya_Start_Audio_Stream(void);
int Tuya_Stop_Audio_Stream(void);
#ifdef __cplusplus
}
#endif

#endif  //!__TUYAMAIN_H__