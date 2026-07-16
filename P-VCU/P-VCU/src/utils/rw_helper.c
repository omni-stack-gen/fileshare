#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>

#include <utils/rw_helper.h>
#include <utils/bmem.h>

#define LOG_TAG "rw_helper"
#include <utils/log.h>

#define RW_TYPE_UNKNOWN 0
#define RW_TYPE_STDIO   1
#define RW_TYPE_MEM     2

typedef struct rw_ops
{
	int (*seek)(rw_fd_t *fd, int offset, int whence);
	int (*read)(rw_fd_t *fd, void *ptr, int size, int nmemb);
	int (*write)(rw_fd_t *fd, const void *ptr, int size, int nmemb);
	int (*close)(rw_fd_t *fd);
	int (*eof)(rw_fd_t *fd);
} rw_ops_t;

struct rw_fd
{
	uint32_t type;

	const rw_ops_t *ops;

	union
	{
		struct
		{
			int autoclose;
			FILE *fp;
		} stdio;

		struct
		{
			uint8_t *start_pos;
			uint8_t *cur_pos;
			uint8_t *end_pos;
		} mem;
	} u;
};

static int mem_seek(rw_fd_t *fd, int offset, int whence);
static int mem_read(rw_fd_t *fd, void *ptr, int size, int nmemb);
static int mem_write(rw_fd_t *fd, const void *ptr, int size, int nmemb);
static int mem_close(rw_fd_t *fd);
static int mem_eof(rw_fd_t *fd);

static int stdio_seek(rw_fd_t *fd, int offset, int whence);
static int stdio_read(rw_fd_t *fd, void *ptr, int size, int nmemb);
static int stdio_write(rw_fd_t *fd, const void *ptr, int size, int nmemb);
static int stdio_close(rw_fd_t *fd);
static int stdio_eof(rw_fd_t *fd);

static rw_fd_t *rw_alloc(void);
static void rw_free(rw_fd_t *fd);

static const rw_ops_t rw_stdio_ops =
{
	stdio_seek,
	stdio_read,
	stdio_write,
	stdio_close,
	stdio_eof,
};

static const rw_ops_t rw_mem_ops =
{
	mem_seek,
	mem_read,
	mem_write,
	mem_close,
	mem_eof,
};

static int mem_seek(rw_fd_t *fd, int offset, int whence)
{
	uint8_t *new_pos;

	switch (whence)
	{
		case SEEK_SET:
			new_pos = fd->u.mem.start_pos + offset;
			break;

		case SEEK_CUR:
			new_pos = fd->u.mem.cur_pos + offset;
			break;

		case SEEK_END:
			new_pos = fd->u.mem.end_pos + offset;
			break;

		default:
			LOGE("Unknown value:%d \n", whence);
			return -1;
	}

	if (new_pos < fd->u.mem.start_pos)
	{
		new_pos = fd->u.mem.start_pos;
	}

	if (new_pos > fd->u.mem.end_pos)
	{
		new_pos = fd->u.mem.end_pos;
	}

	fd->u.mem.cur_pos = new_pos;

	return ((fd->u.mem.cur_pos - fd->u.mem.start_pos) == offset) ? 0 : -1;
}

static int mem_read(rw_fd_t *fd, void *ptr, int size, int nmemb)
{
	if ((fd->u.mem.cur_pos + (nmemb * size)) > fd->u.mem.end_pos)
	{
		nmemb = (fd->u.mem.end_pos - fd->u.mem.cur_pos) / size;
	}

	memcpy(ptr, fd->u.mem.cur_pos, nmemb * size);

	fd->u.mem.cur_pos += nmemb * size;

	return nmemb;
}

static int mem_write(rw_fd_t *fd, const void *ptr, int size, int nmemb)
{
	if ((fd->u.mem.cur_pos + (nmemb * size)) > fd->u.mem.end_pos)
	{
		nmemb = (fd->u.mem.end_pos - fd->u.mem.cur_pos) / size;
	}

	memcpy(fd->u.mem.cur_pos, ptr, nmemb * size);

	fd->u.mem.cur_pos += nmemb * size;

	return nmemb;
}

static int mem_close(rw_fd_t *fd)
{
	if (fd)
	{
		rw_free(fd);
	}

	return 0;
}

static int mem_eof(rw_fd_t *fd)
{
	if (fd->u.mem.cur_pos >= fd->u.mem.end_pos)
	{
		return 1;
	}

	return 0;
}

static int stdio_seek(rw_fd_t *fd, int offset, int whence)
{
	return fseek(fd->u.stdio.fp, offset, whence);
}

static int stdio_read(rw_fd_t *fd, void *ptr, int size, int nmemb)
{
	return fread(ptr, size, nmemb, fd->u.stdio.fp);
}

static int stdio_write(rw_fd_t *fd, const void *ptr, int size, int nmemb)
{
	return fwrite(ptr, size, nmemb, fd->u.stdio.fp);
}

static int stdio_close(rw_fd_t *fd)
{
	if (fd)
	{
		if (fd->u.stdio.autoclose)
		{
			fclose(fd->u.stdio.fp);
		}

		rw_free(fd);
	}

	return 0;
}

static int stdio_eof(rw_fd_t *fd)
{
	return feof(fd->u.stdio.fp);
}

static rw_fd_t *rw_alloc(void)
{
	rw_fd_t *fd;

	fd = (rw_fd_t *)bmalloc(sizeof(rw_fd_t));

	if (fd == NULL)
	{
		LOGE("Out of memory\n");
	}
	else
	{
		memset(fd, 0, sizeof(rw_fd_t));
	}

	return fd;
}

static void rw_free(rw_fd_t *fd)
{
	bfree(fd);
}

rw_fd_t *rw_open_from_file(const char *file, const char *mode)
{
	FILE *fp = NULL;
	rw_fd_t *fd = NULL;

	fp = fopen(file, mode);

	if (fp == NULL)
	{
		LOGE("open %s fail\n", file);
	}
	else
	{
		fd = rw_open_from_fp(fp, 1);
	}

	return fd;
}

rw_fd_t *rw_open_from_fp(FILE *fp, int autoclose)
{
	rw_fd_t *fd = NULL;

	fd = rw_alloc();

	if (fd != NULL)
	{
		fd->type = RW_TYPE_STDIO;
		fd->ops = &rw_stdio_ops;

		fd->u.stdio.fp = fp;
		fd->u.stdio.autoclose = autoclose;
	}

	return fd;
}

rw_fd_t *rw_open_from_mem(void *mem, size_t size)
{
	rw_fd_t *fd = NULL;

	fd = rw_alloc();

	if (fd != NULL)
	{
		fd->type = RW_TYPE_MEM;
		fd->ops = &rw_mem_ops;

		fd->u.mem.start_pos = (uint8_t *)mem;
		fd->u.mem.cur_pos = fd->u.mem.start_pos;
		fd->u.mem.end_pos = fd->u.mem.start_pos + size;
	}

	return fd;
}

int rw_seek(rw_fd_t *fd, int offset, int whence)
{
	if (fd == NULL)
	{
		return -1;
	}

	return fd->ops->seek(fd, offset, whence);
}

int rw_read(rw_fd_t *fd, void *ptr, int size, int nmemb)
{
	if (fd == NULL)
	{
		return -1;
	}

	return fd->ops->read(fd, ptr, size, nmemb);
}

int rw_write(rw_fd_t *fd, const void *ptr, int size, int nmemb)
{
	if (fd == NULL)
	{
		return -1;
	}

	return fd->ops->write(fd, ptr, size, nmemb);
}

int rw_close(rw_fd_t *fd)
{
	if (fd == NULL)
	{
		return -1;
	}

	return fd->ops->close(fd);
}

int rw_eof(rw_fd_t *fd)
{
	if (fd == NULL)
	{
		return -1;
	}

	return fd->ops->eof(fd);
}