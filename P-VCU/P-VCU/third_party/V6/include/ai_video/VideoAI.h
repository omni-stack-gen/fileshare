#ifndef _VIDEOAI_H_
#define _VIDEOAI_H_


extern void* VideoAI_LLE_Create (char* ModelFileName, int ImageWidth, int ImageHeight, int ImageFormat);
extern int   VideoAI_LLE_Destroy(void* Handle);
extern int   VideoAI_LLE_Process(void* Handle, char* ImageBuffer, int BufferSize);

extern int   VideoAI_NV12_To_RGB3(char* psrc, char *pdst, int width, int height);

#endif//_VIDEOAI_H_
