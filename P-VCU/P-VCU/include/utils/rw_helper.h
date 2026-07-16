#ifndef __RW_HELPER_H__
#define __RW_HELPER_H__

#include <stdio.h>
#include <stdint.h>

typedef struct rw_fd rw_fd_t;

rw_fd_t *rw_open_from_file(const char *file, const char *mode);
rw_fd_t *rw_open_from_fp(FILE *fp, int autoclose);
rw_fd_t *rw_open_from_mem(void *mem, size_t size);

int rw_seek(rw_fd_t *fd, int offset, int whence);
int rw_read(rw_fd_t *fd, void *ptr, int size, int nmemb);
int rw_write(rw_fd_t *fd, const void *ptr, int size, int nmemb);
int rw_close(rw_fd_t *fd);
int rw_eof(rw_fd_t *fd);

#endif //!__RW_HELPER_H__