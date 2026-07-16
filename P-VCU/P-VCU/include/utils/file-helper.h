#ifndef __FILE_HELPER_H__
#define __FILE_HELPER_H__

#ifdef __cplusplus
extern "C"
{
#endif

int file_copy(char *src_file_path, char *dest_file_path);

int file_write(char *file_path, void *buf, int size);

int file_read(char *file_name, void **buf);

int file_length(char *file_path);

int file_move(char *src_file_path, char *dest_file_path);

int file_remove(char *file_path);

int file_create_dir(const char *file_path);

int file_is_exist(char *file_path);

int dir_is_exist(char *dir_path);

char* file_get_line(FILE *file);

#ifdef __cplusplus
}
#endif

#endif //!__FILE_HELPER_H__
