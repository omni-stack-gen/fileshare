#ifdef ENABLE_V6
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <zlib.h>
#include <unistd.h>
#include <errno.h>
#include <log.h>

//#include "common/bmem.h"

/* values used in typeflag field */
#define REGTYPE  '0'            /* regular file */
#define AREGTYPE '\0'           /* regular file */
#define LNKTYPE  '1'            /* link */
#define SYMTYPE  '2'            /* reserved */
#define CHRTYPE  '3'            /* character special */
#define BLKTYPE  '4'            /* block special */
#define DIRTYPE  '5'            /* directory */
#define FIFOTYPE '6'            /* FIFO special */
#define CONTTYPE '7'            /* reserved */

/* GNU tar extensions */

#define GNUTYPE_DUMPDIR  'D'    /* file names from dumped directory */
#define GNUTYPE_LONGLINK 'K'    /* long link name */
#define GNUTYPE_LONGNAME 'L'    /* long file name */
#define GNUTYPE_MULTIVOL 'M'    /* continuation of file from another volume */
#define GNUTYPE_NAMES    'N'    /* file name that does not fit into main hdr */
#define GNUTYPE_SPARSE   'S'    /* sparse file */
#define GNUTYPE_VOLHDR   'V'    /* tape/volume header */

#define BLOCKSIZE 512
#define SHORTNAMESIZE 512
#define REMAINING() {if(outfile != NULL)\
                    {\
                      fclose(outfile);\
                      outfile = NULL;\
                    }

// tar头部 文件信息
struct tar_header
{                               /* byte offset */
  char name[100];               /*   0 */
  char mode[8];                 /* 100 */
  char uid[8];                  /* 108 */
  char gid[8];                  /* 116 */
  char size[12];                /* 124 */
  char mtime[12];               /* 136 */
  char chksum[8];               /* 148 */
  char typeflag;                /* 156 */
  char linkname[100];           /* 157 */
  char magic[6];                /* 257 */
  char version[2];              /* 263 */
  char uname[32];               /* 265 */
  char gname[32];               /* 297 */
  char devmajor[8];             /* 329 */
  char devminor[8];             /* 337 */
  char prefix[155];             /* 345 */
                                /* 500 */
};
// 读取压缩文件的数据
union tar_buffer
{
  char               buffer[BLOCKSIZE];
  struct tar_header  header;
};

struct attr_item
{
  struct attr_item  *next;
  char              *fname;
  int                mode;
  time_t             time;
};


// 保存link信息的list
typedef struct _link_list_t
{
    struct _link_list_t *link;
    char *data;
} linklist_t;

linklist_t *InitList(void)
{
    linklist_t *head = malloc(sizeof(linklist_t));

    head->link = NULL;
    head->data = NULL;

    return head;
}

int AddDataToList(linklist_t *head, void *data)
{
    linklist_t *next = (linklist_t *)malloc(sizeof(linklist_t));

    linklist_t *l = head;

    while (l->link != NULL)
    {
        l = l->link;
    }


    l->link = next;
    next->link = NULL;
    next->data = data;

    return 0;
}

int DelList(linklist_t *head)
{
    linklist_t *next = head->link;
    while (next != NULL)
    {
        if(next->data != NULL)
        {
            free(next->data);
            next->data = NULL;
        }
        head->link = next->link;
        free(next);
        next = head->link;
    }
    free(head);
    head = NULL;

    return 0;
}

void create_links_from_list(linklist_t *head)
{
    linklist_t *list = head->link;
	while (list) {
		char *target;

		target = list->data + strlen(list->data) + 1;
		if (symlink(target, list->data)) {
			/* shared message */
			printf("can't create symlink '%s' to '%s'", list->data, target);
            perror("");
		}
		list = list->link;
	}
    DelList(head);
}

/* convert octal digits to int */
/* on error return -1 */

int getoct (char *p,int width)
{
    int result = 0;
    char c;

    while (width--)
    {
        c = *p++;
        if (c == 0)
        {
            break;
        }
        if (c == ' ')
        {
            continue;
        }
        if (c < '0' || c > '7')
        {
            return -1;
        }
        result = result * 8 + (c - '0');
    }

    return result;
}

// 递归创建文件目录
int stamakedir(char *newdir)
{
    char *buffer = strdup(newdir);
    char *p;
    int  len = strlen(buffer);

    if (len <= 0)
    {
        free(buffer);
        return 0;
    }

    if (buffer[len-1] == '/')
    {
        buffer[len-1] = '\0';
    }

    if (mkdir(buffer, 0755) == 0)
    {
        free(buffer);
        return 1;
    }

    p = buffer+1;
    while (1)
    {
        char hold;

        while(*p && *p != '\\' && *p != '/')
        {
            p++;
        }

        hold = *p;
        *p = 0;
        if ((mkdir(buffer, 0755) == -1) && (errno == ENOENT))
        {
            fprintf(stderr,"makedir: Couldn't create directory %s\n",buffer);
            free(buffer);
            return 0;
        }
        if (hold == 0)
        {
           break;
        }
        *p++ = hold;
    }

    free(buffer);
    return 1;
}

// 推送文件属性
void push_attr(struct attr_item **list,char *fname,int mode,time_t time)
{
    struct attr_item *item;

    item = (struct attr_item *)malloc(sizeof(struct attr_item));
    if (item == NULL)
    {
        perror("Out of memory");
    }
    item->fname = strdup(fname);
    item->mode  = mode;
    item->time  = time;
    item->next  = *list;
    *list       = item;
}

/* convert time_t to string */
/* use the "YYYY/MM/DD hh:mm:ss" format */

char *strtime (time_t *t)
{
  struct tm   *local;
  static char result[64];

  local = localtime(t);
  snprintf(result, sizeof(result),"%4d/%02d/%02d %02d:%02d:%02d",
          local->tm_year+1900, local->tm_mon+1, local->tm_mday,
          local->tm_hour, local->tm_min, local->tm_sec);
  return result;
}

// 设置文件时间
int setfiletime (char *fname,time_t ftime)
{
    struct utimbuf settime;

    settime.actime = settime.modtime = ftime;
    return utime(fname,&settime);
}

// 还原文件属性
void restore_attr(struct attr_item **list)
{
  struct attr_item *item, *prev;

  for (item = *list; item != NULL; )
    {
      setfiletime(item->fname,item->time);
      chmod(item->fname,item->mode);
      prev = item;
      item = item->next;
      free(prev);
    }
  *list = NULL;
}

int decompression_targz(char *filename, char *dscpath)
{
    int ret = -1;
    if(filename == NULL)
    {
        return ret;
    }

    union  tar_buffer buffer;
    gzFile fd;
    linklist_t *head = InitList();

    fd = gzopen(filename, "rb");
    // 读取的数据长度
    int len = -1;
    // 错误编号
    int err;
    // 是否获取头部信息
    int getheader = 1;

    int remaining = -1;

    int tarmode;
    // 文件创建时间
    time_t tartime;
    // 文件路径
    char fname[BLOCKSIZE];

    char fnamecopy[SHORTNAMESIZE];

    FILE   *outfile = NULL;

    struct attr_item *attributes = NULL;

    bool endread = false;

    if(fd == NULL)
    {
        printf("DecompressionTarGz : Couldn't gzopen %s : %s\n", filename, strerror(errno));
        return ret;
    }
    else
    {
        printf("start zlib tar file : %s\n", filename);
    }

    printf("    date      time     size                       file\n"
        " ---------- -------- --------- -------------------------------------\n");

    int file_number = 0;

    while(1)
    {
        // 读取压缩文件的数据
        len = gzread(fd, &buffer, BLOCKSIZE);
        if(len < 0)
        {
            perror(gzerror(fd, &err));
            goto readerr;

        }
        // 如果大小
        if(len != BLOCKSIZE)
        {
            // 最后一块数据且不与BLOCKSIZE大小一样
            endread = true;
            remaining = 0;
        }
        if(getheader >= 1)
        {
            // 如果是超出文件，则退出
            if (len == 0 || buffer.header.name[0] == 0)
            {
                endread = true;
                break;
            }
            //
            tarmode = getoct(buffer.header.mode, 8);
            // 创建时间
            tartime = (time_t)getoct(buffer.header.mtime, 12);
            if(tarmode == -1 || tartime == (time_t)-1)
            {
                endread = true;
                buffer.header.name[0] = 0;
            }

            if(getheader == 1)
            {
                // 保存文件名
                sprintf(fname, "%s%s", dscpath, buffer.header.name);
                // 最后一位清零，保证fname的完整性
                if(fname[SHORTNAMESIZE - 1] != 0)
                {
                    fname[SHORTNAMESIZE - 1] = 0;
                }
            }
            else
            {
                //文件路径最长不得超过 SHORTNAMESIZE
                if (strncmp(fname, buffer.header.name, SHORTNAMESIZE-1) != 0)
                {
                    perror("bad long name");
                }
                getheader = 1;
            }

            // 文件类型
            switch(buffer.header.typeflag)
            {
                case DIRTYPE :
                        printf(" %s     <dir> %s\n", strtime(&tartime), fname);
                        stamakedir(fname);
                        push_attr(&attributes, fname, tarmode, tartime);
                    break;
                case REGTYPE:
                case AREGTYPE:
                    remaining = getoct(buffer.header.size,12);
                    if(remaining == -1)
                    {
                        endread = true;
                        break;
                    }
                    // 设置路径
                    sprintf(fnamecopy, "%s%s", dscpath, buffer.header.name);
                    if(fnamecopy[SHORTNAMESIZE - 1] != 0)
                    {
                        fnamecopy[SHORTNAMESIZE - 1] = 0;
                    }
                    // 打印信息
                    file_number++;
                    printf(" %s %9d %s\n", strtime(&tartime), remaining, fnamecopy);
                    // 打开并创建文件
                    outfile = fopen(fnamecopy, "wb");
                    if(outfile == NULL)
                    {
                        printf("create %s fail\n", fnamecopy);
                        goto readerr;
                    }
                    getheader = 0;
                    break;
                case GNUTYPE_LONGLINK:
                case GNUTYPE_LONGNAME:
                    remaining = getoct(buffer.header.size,12);
                    if (remaining < 0 || remaining >= BLOCKSIZE)
                    {
                        endread = true;
                        break;
                    }
                    len = gzread(fd, fname, BLOCKSIZE);
                    if (len < 0)
                    {
                        perror(gzerror(fd, &err));
                    }
                    if (fname[BLOCKSIZE-1] != 0 || (int)strlen(fname) > remaining)
                    {
                        endread = true;
                        break;
                    }
                    getheader = 2;
                default:
                    printf(" %s     <---> %s\n",strtime(&tartime),fname);
                    break;
            }
            if(strcmp(buffer.header.linkname, "") != 0)
            {
                char *linkmsg = malloc(strlen(buffer.header.name) + 3 + strlen(buffer.header.linkname) + strlen(dscpath));
                printf(" %s     %s%s%s\n",strtime(&tartime), buffer.header.name, "--->", buffer.header.linkname);
                sprintf(linkmsg, "%s%s%c%s%c", dscpath, buffer.header.name, '\0', buffer.header.linkname, '\0');
                AddDataToList(head, linkmsg);
            }
        }
        else
        {
            unsigned int bytes = (remaining > BLOCKSIZE) ? BLOCKSIZE : remaining;
            if(outfile != NULL)
            {
                if(fwrite(&buffer, sizeof(char), bytes, outfile) != bytes)
                {
                    fprintf(stderr, "DecompressionTarGz: Error writing %s -- skipping\n", fname);
                    fclose(outfile);
                    outfile = NULL;
                    remove(fname);
                }
            }
            remaining -= bytes;
        }
        if(remaining == 0)
        {
            getheader = 1;
            if(outfile != NULL)
            {
                fclose(outfile);
                outfile = NULL;
                push_attr(&attributes,fname,tarmode,tartime);
            }
        }

        if(endread)
        {
            break;
        }
    }

    restore_attr(&attributes);
    ret = 0;
readerr:

    if(gzclose(fd) != Z_OK)
    {
        perror("fail gzclose\n");
        return -1;
    }

    printf("==========file_number:%d\n", file_number);

    create_links_from_list(head);
    return 0;
}

// #include "common/dp_file.h"
// #include "common/service_upgrade.h"
#define UPDATE_UNTAR_PATH       "/system/"
#define UPGRADE_UP_PATH         "/system/app_up"
int dp_untar(char *filename)
{
    // 解压
    if(decompression_targz(filename, UPDATE_UNTAR_PATH) != 0)
    {
        //file_remove(filename);
        //file_remove(UPGRADE_UP_PATH);
        // 删除已解压的部分
        printf("Update Fail\n");
        return -1;
    }

    // 删除成功
    //file_remove(filename);

    return 0;
}


#endif