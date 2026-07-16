#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <linux/input.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/sysinfo.h>

#include "dpbase.h"

void DPChmodFile(const char* pfile, char * type)
{
	if(NULL != pfile && NULL != type)
	{
		char pCommand[512] = {0};
		sprintf(pCommand, "chmod %s %s", type, pfile);
		system(pCommand);
	}
}

void DPDeleteFile(const char* pfile)
{
	if(NULL != pfile)
		remove(pfile);
}

void DPMoveFile(const char* dstfile, const char* srcfile)
{
	if(NULL != dstfile && NULL != srcfile)
	{
		char pCommand[512] = {0};
		sprintf(pCommand, "mv %s %s", srcfile, dstfile);
		system(pCommand);
	}
}

void DPCopyFile(const char* dstfile, const char* srcfile)
{
	char pCommand[512] = {0};
	sprintf(pCommand, "cp -r %s %s", srcfile, dstfile);
	system(pCommand);
}

void* DPFindFirstFile(char* dir, char* pfile)
{
	DIR * pDir;
	struct dirent *entry;

	if((pDir = opendir(dir)) == NULL)
	{
		return NULL;
	}

	entry = readdir(pDir);
	if(entry == NULL)
	{
		closedir(pDir);
		return NULL;
	}

	strcpy(pfile, entry->d_name);
	return pDir;
}

bool DPFindNextFile(void* hFind, char* pfile)
{
	DIR* pDir = (DIR*)hFind;
    struct dirent *entry;

	entry = readdir(pDir);
	if(entry == NULL)
	{
		return false;
	}

	strcpy(pfile, entry->d_name);
	return true;
}

void DPFindClose(void* hFind)
{
	closedir((DIR *)hFind);
}

int DPGetFileAttributes(const char* filename, int* filelen)
{
	struct stat attributes;
	int ret = stat(filename, &attributes);
	if((ret != -1)
		&& (filelen != NULL))
	{
		*filelen = attributes.st_size;
	}
	return ret;
}

bool CheckAndCreateDir(const char *cDir)
{
	DIR* pDir;

    if((pDir = opendir(cDir)) == NULL)
    {
		mkdir(cDir, 777);
    }
	else
		closedir(pDir);
	return true;
}

unsigned long long CheckSpareSpace(const char* dir)
{
#ifdef _DEBUG
	return 0x1000000;
#endif

	char dirname[32];
	struct statfs diskInfo;

	if(strstr(dir, "app") != NULL)
		strcpy(dirname, "/app/");
	else if(strstr(dir, "FlashDev")!=NULL)
		strcpy(dirname, "/FlashDev/");
	else if(strstr(dir, "UserDev")!=NULL)
		strcpy(dirname, "/UserDev/");

	statfs(dirname, &diskInfo);
	unsigned long long totalsize = diskInfo.f_bsize * diskInfo.f_blocks;
	unsigned long long freeDisk = diskInfo.f_bfree * diskInfo.f_bsize;
	printf("total = %llu KB Free =  %llu KB \n", totalsize>>10, freeDisk>>10);

	if(freeDisk > 204800)
		freeDisk -= 204800;
	else
		freeDisk = 0;

	return (unsigned long long)freeDisk;
}

unsigned int DumpMemory(unsigned int* total, unsigned int* left)
{
    FILE *fd;
    fd = fopen("/proc/meminfo", "r");
    char buf[256] = {0};
    char tmp[256] = {0};

    unsigned int MemTotal = 0;
    unsigned int MemFree = 0;
    unsigned int MemAvailable = 0;
    unsigned int Buffers = 0;
    unsigned int Cached = 0;

    if (fd != NULL)
    {
        if (fgets(buf, sizeof(buf), fd))
            sscanf(buf, "%s %d %s", tmp, &MemTotal, tmp);

        if (fgets(buf, sizeof(buf), fd))
            sscanf(buf, "%s %d %s", tmp, &MemFree, tmp);

        if (fgets(buf, sizeof(buf), fd))
            sscanf(buf, "%s %d %s", tmp, &MemAvailable, tmp);

        if (fgets(buf, sizeof(buf), fd))
            sscanf(buf, "%s %d %s", tmp, &Buffers, tmp);

        if (fgets(buf, sizeof(buf), fd))
            sscanf(buf, "%s %d %s", tmp, &Cached, tmp);

        fclose(fd);

        *total = MemTotal * 1024;
        *left = MemFree * 1024 + Buffers * 1024 + Cached * 1024;
    }
    else
    {
        *total = 1;
        *left = 1;
    }

    return 0;
}


unsigned int GetFreeMemory()
{
	system("free >> /Windows/memory.dat");
	FILE* pFile = fopen("/Windows/memory.dat", "rb");
	if(pFile)
	{
		char buf[1024], temp[256];
		unsigned int dwUsed, dwFree;

		fgets(buf, 1024, pFile);
		fgets(buf, 1024, pFile);
		fgets(buf, 1024, pFile);
		sscanf(buf, "%s %s %d %d", temp, temp, &dwUsed, &dwFree);
		fclose(pFile);

		system("rm /Windows/memory.dat");
		return dwFree;
	}
	return 0;
}

void DPUnloadFile(const char* filename)
{
	printf("fuser -k /sdcard\r\n");
	system("fuser -k /sdcard");
	usleep(500);
	printf("umount /sdcard\r\n");
	system("umount /sdcard");
	usleep(500);
	printf("rm -rf /sdcard\r\n");
	system("rm -rf /sdcard");
	usleep(500);
	printf("sync\r\n");
	sync();
	usleep(1000 * 50);
	printf("DPUnloadFile over\r\n");
	return;

	char pCommand[128];
	sprintf(pCommand, "fuser -k %s", filename);
	printf("cmd %s\r\n", pCommand);
	system(pCommand);
	DPSleep(1000);

	sprintf(pCommand, "umount -l %s", filename);
	printf("cmd %s\r\n", pCommand);
	system(pCommand);
	DPSleep(1000);

	printf("cmd sync\r\n");
	sync();
	DPSleep(1000);
	printf("DPUnloadFile %s\r\n", filename);
}

bool DPWriteFile(const char* filename, char* pdata, int len)
{
	bool ret = false;
	int fd = open(filename, O_CREAT | O_RDWR | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if(-1 == fd)
	{
		perror(filename);
		return false;
	}

	if(len == write(fd, pdata, len))
	{
		fdatasync(fd);
		ret = true;
	}
	close(fd);
	return ret;
}

void DPFlush(FILE* pFile)
{
	//unsigned int tick = DPGetTickCount();
	fflush(pFile);
	fsync(fileno(pFile));
	//printf("dpflush cost %dms\r\n", DPGetTickCount() - tick);
}

int BReadFile(char *filename, char **buf)
{
    FILE *tf;
    int flen;
    char *pdata;
    *buf = NULL;

    tf = fopen(filename, "rb");

    if(tf == NULL)
    {
        printf("open file %s error\n", filename);
        return 0;
    }

    fseek(tf, 0, SEEK_END);
    flen = ftell(tf);
    fseek(tf, 0, SEEK_SET);

    if(flen <= 0)
    {
        fclose(tf);
        return false;
    }

    pdata = (char *)malloc(flen + 1);

    if(pdata == NULL)
    {
        printf("malloc buf %d error\n", flen);
        fclose(tf);
        return 0;
    }

    if(flen != (int)fread(pdata, 1, flen, tf))
    {
        printf("read file %d error\n", flen);
        fclose(tf);
        free(pdata);
        return 0;
    }

    fclose(tf);
    pdata[flen] = 0;
    *buf = pdata;
    return flen;
}

void FindFileFromDirectory(char *dir, void (*func)(char *directory, char *fileName, void *param), void *param)
{
    char fileName[256];
    void* hFind = DPFindFirstFile(dir, fileName);
    if (hFind != NULL)
    {
        do
        {
            func(dir, fileName, param);
        } while (DPFindNextFile(hFind, fileName));
        DPFindClose(hFind);
    }
}

bool DPCopyFileEx(const char *dstfile, const char *srcfile)
{
    bool ret = false;
    FILE *pSrcFile = NULL;
    FILE *pDstFile = NULL;
    char *pbuf = NULL;
    unsigned int len = 0;

    do
    {
        if (dstfile == NULL || srcfile == NULL)
            break;

        pSrcFile = fopen(srcfile, "rb");
        if (pSrcFile == NULL)
            break;

        fseek(pSrcFile, 0, SEEK_END);
        len = ftell(pSrcFile);
        fseek(pSrcFile, 0, SEEK_SET);

        if (len <= 0)
        {
            fclose(pSrcFile);
            return false;
        }

        pbuf = (char *)malloc(len);
        if (pbuf == NULL)
            break;

        if (len != fread(pbuf, 1, len, pSrcFile))
            break;

        pDstFile = fopen(dstfile, "wb");
        if (NULL == pDstFile)
            break;

        if (len != fwrite(pbuf, 1, len, pDstFile))
            break;

        ret = true;
    } while (0);

    if (pSrcFile)
        fclose(pSrcFile);
    if (pDstFile)
        fclose(pDstFile);
    if (pbuf)
        free(pbuf);

    return ret;
}


//===================================================================================================//
#define SYS_BLK_MMCBLK0         "/sys/block/mmcblk1/device"
#define DEV_MMCBLK0             "/dev/mmcblk1 "
#define DEV_MMCBLK0P1           "/dev/mmcblk1p1 "
#define SYSTEM_MOUNTS           "/proc/self/mounts"

enum sdcard_key_index
{
    SD_NAME = 0,
    SD_CID,
    SD_CSD,
    SD_SCR,
    SD_SERIAL,
    SD_MANFID,
    SD_OEMID,
    SD_DATE,
};

struct sdcard_desc
{
    enum sdcard_key_index index;
    char dev[32];
    char desc[64];
};

static struct sdcard_desc sdcard_desc_list[] = {
    {SD_NAME, "name"},
    {SD_CID, "cid"},
    {SD_CSD, "csd"},
    {SD_SCR, "scr"},
    {SD_SERIAL, "serial"},
    {SD_MANFID, "manfid"},
    {SD_OEMID, "oemid"},
    {SD_DATE, "date"},
};

static ssize_t file_read_path(const char *path, void *data, size_t len)
{
    int fd = open(path, O_RDONLY, 0666);
    if (fd == -1)
    {
        return -1;
    }
    int n;
    char *p = (char *)data;
    size_t left = len;
    size_t step = 1024 * 1024;
    int cnt = 0;

    while (left > 0)
    {
        if (left < step)
            step = left;
        n = read(fd, (void *)p, step);
        if (n > 0)
        {
            p += n;
            left -= n;
            continue;
        }
        else
        {
            if (n == 0)
            {
                break;
            }
            else
            {
                if (errno == EINTR || errno == EAGAIN)
                {
                    if (++cnt > 3)
                        break;
                    continue;
                }
            }
        }
    }
    close(fd);
    return (len - left);
}

int sdcard_get_info(struct sdcard_info *info)
{
    int i = 0;
    uint64_t free_space = 0;
    char path[128];
    char buf[2048];
    int ret = 0;

    do
    {
        if (!info)
        {
            printf("invalid paraments!\n");
            ret = -1;
            break;
        }

        memset(info, 0, sizeof(*info));

        if (-1 == access(SYS_BLK_MMCBLK0, F_OK | W_OK | R_OK))
        {
            printf("sdcard does not insert\n");
            info->is_insert = false;
            break;
        }
        else
        {
            info->is_insert = true;
        }

        for (i = 0; i < (int)ARRAY_SIZE(sdcard_desc_list); i++)
        {
            snprintf(path, sizeof(path), "%s/%s", SYS_BLK_MMCBLK0, sdcard_desc_list[i].dev);

            if (-1 == file_read_path(path, sdcard_desc_list[i].desc, sizeof(sdcard_desc_list[i].desc)))
            {
                continue;
            }

            size_t str_end = strlen(sdcard_desc_list[i].desc);

            if (sdcard_desc_list[i].desc[str_end - 1] == '\n')
            {
                sdcard_desc_list[i].desc[str_end - 1] = '\0';
            }
        }

        file_read_path(SYSTEM_MOUNTS, buf, sizeof(buf));

        char *pStart = strstr(buf, DEV_MMCBLK0);
        if(pStart == NULL)
        {
            pStart = strstr(buf, DEV_MMCBLK0P1);
            if(pStart == NULL)
            {
                printf("sdcard insert, but mount failed, please format sdcard!");
                info->is_mounted = false;
                break;
            }
            else
            {
                pStart += strlen(DEV_MMCBLK0P1) + 1;
                char *pEnd = strstr(pStart, " vfat rw");
                if(pEnd != NULL)
                {
                    info->mount_point[0] = '/';
                    strncpy(&info->mount_point[1], pStart, pEnd - pStart);
                }
                else
                {
                    printf("sdcard insert, but mount failed, please format sdcard!");
                    info->is_mounted = false;
                    break;
                }
            }
        }
        else
        {
            pStart += strlen(DEV_MMCBLK0) + 1;
            char *pEnd = strstr(pStart, " vfat rw");
            if(pEnd != NULL)
            {
                info->mount_point[0] = '/';
                strncpy(&info->mount_point[1], pStart, pEnd - pStart);
            }
            else
            {
                printf("sdcard insert, but mount failed, please format sdcard!");
                info->is_mounted = false;
                break;
            }
        }

        info->is_mounted = true;

        struct statfs disk_statfs;
        if (-1 == statfs(info->mount_point, &disk_statfs))
        {
            printf("get disk statfs failed: %d\n", errno);
            break;
        }
        free_space = ((uint64_t)disk_statfs.f_bsize * (uint64_t)disk_statfs.f_bfree);
        info->capacity = ((uint64_t)disk_statfs.f_bsize * (uint64_t)disk_statfs.f_blocks);
        strcpy(info->name, sdcard_desc_list[SD_NAME].desc);
        strcpy(info->serial, sdcard_desc_list[SD_SERIAL].desc);
        strcpy(info->manfid, sdcard_desc_list[SD_MANFID].desc);
        strcpy(info->oemid, sdcard_desc_list[SD_OEMID].desc);
        strcpy(info->date, sdcard_desc_list[SD_DATE].desc);
        info->used_size = info->capacity - free_space;
    } while (0);
    return ret;
}