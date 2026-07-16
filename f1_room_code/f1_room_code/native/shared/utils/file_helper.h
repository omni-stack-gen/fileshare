#ifndef __FILE_HELPER_H__
#define __FILE_HELPER_H__

#ifdef __cplusplus
extern "C"
{
#endif

int file_copy(const char *src_file_path, const char *dest_file_path);

int file_write(const char *file_path, void *buf, int size);

int file_read(const char *file_path, void **buf);

int file_length(const char *file_path);

int file_move(const char *src_file_path, const char *dest_file_path);

int file_remove(const char *file_path);

int file_create_dir(const char *file_path);

int file_is_exist(const char *file_path);

int dir_is_exist(const char *dir_path);

#ifdef __cplusplus
}
#endif

#endif //!__FILE_HELPER_H__