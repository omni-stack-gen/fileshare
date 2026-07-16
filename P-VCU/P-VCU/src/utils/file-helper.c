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

#include <file-helper.h>
#include <bmem.h>

#define LOG_TAG "file-helper"
#include <log.h>

#define FILE_COPY_SIZE (16 * 1024)

int file_copy(char *src_file_path, char *dest_file_path)
{
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
        LOGE("%s not found\n",src_file_path);
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
            break;
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
        LOGE("failed to sync file \n");

free_res:
    if (buff)
        bfree(buff);
    if (dest_fp)
        fclose(dest_fp);
    if (src_fp)
        fclose(src_fp);

    return ret;
}

int file_write(char *file_path, void *buf, int size)
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
        LOGE("failed to sync file\n");

free_res:
    fclose(flip);

    return ret;
}

int file_length(char *file_path)
{
    FILE *fp = NULL;
    int file_len;

    fp = fopen(file_path, "rb");
    if (fp == NULL)
    {
        LOGE("open %s failed\n", file_path);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    file_len = ftell(fp);
    if (file_len < 0)
    {
        LOGE("get %s file length error\n", file_path);
        return -1;
    }
    fseek(fp, 0, SEEK_SET);

    return file_len;
}

int file_move(char *src_file_path, char *dest_file_path)
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

int file_read(char *file_name, void **buf)
{
    FILE *fd;
    int flen;
    void *pdata;

    *buf = NULL;

    fd = fopen(file_name, "rb");

    if(fd == NULL)
    {
        LOGE("open file %s error\n", file_name);
        return 0;
    }

    fseek(fd, 0, SEEK_END);
    flen = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    if(flen <= 0)
    {
        fclose(fd);
        return 0;
    }

    pdata = bzalloc(flen + 1);

    if(pdata == NULL)
    {
        LOGE("bzalloc buf %d error\n", flen);
        fclose(fd);
        return 0;
    }

    if(flen != (int)fread(pdata, 1, flen, fd))
    {
        LOGE("read file %s %d error\n", file_name, flen);
        fclose(fd);
        bfree(pdata);
        return 0;
    }

    fclose(fd);

    *buf = pdata;

    return flen;
}

int file_is_exist(char *file_path)
{
    if (!file_path)
		return -1;

    struct stat sb;

    if (stat(file_path, &sb) != 0)
        return -1;

    return 0;
}

int dir_is_exist(char *dir_path)
{
	DIR *dir;
	int ret = -1;

	if (!dir_path)
		return -1;

	dir = opendir(dir_path);

    if (dir != NULL)
    {
		closedir(dir);
		ret = 0;
	}

	return ret;
}

static void _del_dir(const char* path)
{
	DIR *dir;
	char fullpath[PATH_MAX];

	dir = opendir(path);

    if (dir != NULL)
	{
		struct dirent* dirent;

        do
		{
			struct stat s;

			dirent = readdir(dir);
			if (dirent == NULL)
				break;

            if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
                continue;

            memset(&s, 0, sizeof(struct stat));
			// snprintf(fullpath, sizeof(fullpath), "%s/%s", path, dirent->d_name);
            int path_len = strlen(path);
			snprintf(fullpath, sizeof(fullpath), "%.*s/%s", path_len, path, dirent->d_name);
			stat(fullpath, &s);

			if( S_ISDIR(s.st_mode))
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

int file_remove(char *file_path)
{
	if (!file_path)
		return -1;

    struct stat s;

    if(stat(file_path, &s) == 0)
    {
        if( S_ISDIR(s.st_mode))
            _del_dir(file_path);
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


static char* file_get_chunk_from_file(FILE *file, size_t *end)
{
	int ch;
	size_t idx = 0;
	char *linebuf = NULL;

	while ((ch = getc(file)) != EOF)
    {
		/* grow the line buffer as necessary */
		if (!(idx & 0xff))
        {
			if (idx == ((size_t)-1) - 0xff)
				LOGE("too long\n");

			linebuf = brealloc(linebuf, idx + 0x100);
		}

		linebuf[idx++] = (char) ch;

		if (ch == '\0')
			break;

		if (end && ch == '\n')
			break;
	}

	if (end)
		*end = idx;

	if (linebuf) {
		// huh, does fgets discard prior data on error like this?
		// I don't think so....
		//if (ferror(file)) {
		//	free(linebuf);
		//	return NULL;
		//}
		linebuf = brealloc(linebuf, idx + 1);

		linebuf[idx] = '\0';
	}

	return linebuf;
}

char* file_get_line(FILE *file)
{
	size_t i;
	char *c = file_get_chunk_from_file(file, &i);

	if (i && c[--i] == '\n')
		c[i] = '\0';

	return c;
}
