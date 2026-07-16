#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/sysinfo.h>

#include <utils/bmem.h>
#include <utils/file_helper.h>
#include <utils/time_helper.h>

#define LOG_TAG "file_helper"
#include <utils/log.h>

#define FILE_COPY_SIZE (16 * 1024)

int file_copy(const char *src_file_path, const char *dest_file_path)
{
	uint64_t tp_now = time_now();

	FILE *dest_fp = NULL, *src_fp = NULL;
	char *buff = NULL;
	int read_cnt, write_cnt, ret = -1, fd;

	buff = bmalloc(FILE_COPY_SIZE);

	if (!buff)
	{
		LOGE("alloc file buffer failed \n");
		goto free_res;
	}

	dest_fp = fopen(dest_file_path, "wb");

	if (dest_fp == NULL)
	{
		LOGE("open %s failed\n", dest_file_path);
		goto free_res;
	}

	src_fp = fopen(src_file_path, "r");

	if (src_fp == NULL)
	{
		LOGE("%s not found\n", src_file_path);
		goto free_res;
	}

	while (1)
	{
		read_cnt = fread(buff, 1, FILE_COPY_SIZE, src_fp);

		if (read_cnt > 0)
		{
			write_cnt = fwrite(buff, 1, read_cnt, dest_fp);

			if (read_cnt != write_cnt)
			{
				LOGE("write %s failed\n", dest_file_path);
				goto free_res;
			}
		}
		else
		{
			break;
		}
	}

	ret = fflush(dest_fp);

	if (ret)
	{
		LOGE("failed to flush file \n");
		goto free_res;
	}

	fd = fileno(dest_fp);

	if (fd == -1)
	{
		LOGE("fileno failed \n");
		goto free_res;
	}

	ret = fsync(fd);

	if (ret)
	{
		LOGE("failed to sync file \n");
	}

	LOGI("file copy [%s] >>> [%s] cost %" PRIu64 "ms\n", src_file_path, dest_file_path, time_now() - tp_now);

free_res:

	if (buff)
	{
		bfree(buff);
	}

	if (dest_fp)
	{
		fclose(dest_fp);
	}

	if (src_fp)
	{
		fclose(src_fp);
	}

	return ret;
}

int file_write(const char *file_path, void *buf, int size)
{
	FILE *flip = NULL;
	int count = 0, fd, ret;

	flip = fopen(file_path, "wb");

	if (flip == NULL)
	{
		LOGE("open %s failed\n", file_path);
		return -1;
	}

	count = fwrite(buf, 1, size, flip);

	if (count != size)
	{
		LOGE("write file(%s) failed, size:%d != write_count:%d \n", file_path, size, count);
		fclose(flip);
		return -1;
	}

	ret = fflush(flip);

	if (ret)
	{
		LOGE("failed to flush file\n");
		goto free_res;
	}

	fd = fileno(flip);

	if (fd == -1)
	{
		LOGE("fileno failed\n");
		goto free_res;
	}

	ret = fsync(fd);

	if (ret)
	{
		LOGE("failed to sync file\n");
	}

free_res:
	fclose(flip);

	return ret;
}

int file_length(const char *file_path)
{
	struct stat buf;

	if (!file_path)
	{
		return -1;
	}

	if (stat(file_path, &buf) != 0)
	{
		return -1;
	}

	return buf.st_size;
}

int file_move(const char *src_file_path, const char *dest_file_path)
{
	int ret;

	ret = file_copy(src_file_path, dest_file_path);

	if (ret)
	{
		LOGE("failed to copy %s to %s\n", src_file_path, dest_file_path);
		return ret;
	}

	ret = remove(src_file_path);

	if (ret)
	{
		LOGE("failed to remove %s\n", src_file_path);
		remove(dest_file_path);
		return ret;
	}

	return 0;
}

int file_read(const char *file_path, void **buf)
{
	FILE *fd;
	int flen;
	void *pdata;

	*buf = NULL;

	fd = fopen(file_path, "rb");

	if (fd == NULL)
	{
		LOGE("open file %s error\n", file_path);
		return 0;
	}

	fseek(fd, 0, SEEK_END);
	flen = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	if (flen <= 0)
	{
		fclose(fd);
		return 0;
	}

	pdata = bzalloc(flen + 1);

	if (pdata == NULL)
	{
		LOGE("bzalloc buf %d error\n", flen);
		fclose(fd);
		return 0;
	}

	if (flen != (int)fread(pdata, 1, flen, fd))
	{
		LOGE("read file %s %d error\n", file_path, flen);
		fclose(fd);
		bfree(pdata);
		return 0;
	}

	fclose(fd);

	*buf = pdata;

	return flen;
}

int file_is_exist(const char *file_path)
{
	struct stat buf;

	if (!file_path)
	{
		return -1;
	}

	if (stat(file_path, &buf) != 0)
	{
		return -1;
	}

	return 0;
}

int dir_is_exist(const char *dir_path)
{
	DIR *dir;
	int ret = -1;

	if (!dir_path)
	{
		return -1;
	}

	dir = opendir(dir_path);

	if (dir != NULL)
	{
		closedir(dir);
		ret = 0;
	}

	return ret;
}

static void _del_dir(const char *path)
{
	DIR *dir;
	char fullpath[PATH_MAX];
	int path_len = strlen(path);

	dir = opendir(path);

	if (dir != NULL)
	{
		struct dirent *dirent;

		do
		{
			struct stat s;

			dirent = readdir(dir);

			if (dirent == NULL)
			{
				break;
			}

			if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
			{
				continue;
			}

			memset(&s, 0, sizeof(struct stat));
			snprintf(fullpath, sizeof(fullpath), "%.*s/%s", path_len, path, dirent->d_name);
			stat(fullpath, &s);

			if (S_ISDIR(s.st_mode))
			{
				_del_dir(fullpath);
			}
			else
			{
				unlink(fullpath);
			}
		} while (dirent != NULL);

		closedir(dir);
		// unlink(path);
		rmdir(path);
	}
}

int file_remove(const char *file_path)
{
	if (!file_path)
	{
		return -1;
	}

	struct stat s;

	if (stat(file_path, &s) == 0)
	{
		if (S_ISDIR(s.st_mode))
		{
			_del_dir(file_path);
		}
		else
		{
			unlink(file_path);
		}
	}
	else
	{
		LOGE("file (%s) does not exist\n", file_path);
		return -1;
	}

	return 0;
}

static int mkdir_r(const char *path, mode_t mode)
{
	char *temp = bstrdup(path);
	char *pos = temp;
	int ret = 0;

	if (!path)
	{
		return -1;
	}

	if (strncmp(temp, "/", 1) == 0)
	{
		pos += 1;
	}
	else if (strncmp(temp, "./", 2) == 0)
	{
		pos += 2;
	}

	for (; *pos != '\0'; ++pos)
	{
		if (*pos == '/')
		{
			*pos = '\0';

			if (-1 == (ret = mkdir(temp, mode)))
			{
				if (errno == EEXIST)
				{
					ret = 0;
				}
				else
				{
					LOGE("failed to mkdir %s: %d\n", temp, errno);
					break;
				}
			}

			*pos = '/';
		}
	}

	if (*(pos - 1) != '/')
	{
		if (-1 == (ret = mkdir(temp, mode)))
		{
			if (errno == EEXIST)
			{
				ret = 0;
			}
			else
			{
				LOGE("failed to mkdir %s: %d\n", temp, errno);
			}
		}
	}

	bfree(temp);
	return ret;
}

int file_create_dir(const char *file_path)
{
	return mkdir_r(file_path, 0775);
}