#ifndef __VIDEO_ENCODE_H__
#define __VIDEO_ENCODE_H__

#if 0
#define BITMAPFILE_960X540_1 "/mnt/app/960x540_1.bmp"
#define BITMAPFILE_960X540_2 "/mnt/app/960x540_2.bmp"
#define BITMAPFILE_1920X1080_1 "/mnt/app/1920x1080_1.bmp"
#define BITMAPFILE_1920X1080_2 "/mnt/app/1920x1080_2.bmp"
#else
#define BITMAPFILE_960X540_1 "/mnt/V1/pack/pack_dpv1_fd/app/960x540_1.bmp"
#define BITMAPFILE_960X540_2 "/mnt/V1/pack/pack_dpv1_fd/app/960x540_2.bmp"
#define BITMAPFILE_1920X1080_1 "/mnt/V1/pack/pack_dpv1_fd/app/1920x1080_1.bmp"
#define BITMAPFILE_1920X1080_2 "/mnt/V1/pack/pack_dpv1_fd/app/1920x1080_2.bmp"
#endif

int video_enc_start(int devip);
void video_enc_stop();
void video_enc_printf();
void video_mode(int hight);
void video_idr(int channal);
int video_catch_jpeg(int chn,unsigned char **jpegbuffer,unsigned int *jpegsize);

void video_md_setlevel(int level);
void video_md_enable(bool benable);
void video_md_enable_bk(bool benable);
void videe_md_getstatus(int * mvnum,int * mdarea);

// >200 夜晚 <=200  白天
int video_check_night();
void video_switch_night(bool bnight);

void video_set_wdr(int wdr);

bool video_md_check_enable(void);
void video_md_set_param(void *mvsp);
void video_md_print_param(void);
void video_md_set_alarm_delay(void);

void video_draw2mainstream(void);
void video_draw2substream(void);
void video_draw2substream2_nocache(void);
void video_draw2mainstream_destroy(void);
void video_draw2substream_destroy(void);

void video_draw_destroy(int nVEncChn, int nOSDIdx);

void video_crop_switch(bool bcrop);
#endif

