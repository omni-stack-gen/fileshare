#ifndef __SERVICEFILE_H__
#define __SERVICEFILE_H__

bool InitFileServer(void);
void DeinitFileServer(void);
bool WriteServerFile(const char* name, int len, void* buf);
void DeleteServerFile(const char* name);

#endif //!__SERVICEFILE_H__