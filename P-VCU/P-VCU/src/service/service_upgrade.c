#ifdef ENABLE_V6
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "pthread.h"

#include <cJSON.h>
#include "log.h"
// #include "common/dp_untar.h"
// #include "common/service_upgrade.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <md5-helper.h>
#include <file-helper.h>
#include "dpbase.h"

#define UPGRADE_PATH            "/system/app"
#define UPGRADE_BAK_PATH        "/system/app_bak"
#define UPGRADE_UP_PATH         "/system/app_up"

#define UPGRAED_PACKET_PATH     "/system/update.tar.gz"
#define UPDATE_UNTAR_PATH       "/system/"
#define UPDATE_SYSTEM_PATH      "/system/update_sys"

#define UPDRADE_DOWNFILE_PATH   "/system/update.rar"

int dp_untar(char *filename);

int updata_part(char*part,char*filename)
{
    mtd_info_t mtd_info;
    erase_info_t ei;
    int fd ;
    long long offset;
    int realmtdsize;
    int readlen,filelen,totallen;
    FILE*fp = NULL;
    unsigned char *pbuf = NULL;
    int ret = -1;

    fd = open(part, O_RDWR);
    if(fd < 0)
    {
        printf("open %s fail\n",part);
        return -1;
    }

    if(ioctl(fd, MEMGETINFO, &mtd_info) < 0)   // get the device info
    {
        printf("MEMGETINFO fail\n");
        goto end;
    }

    printf("MTD Type: %x\nMTD total size: %x bytes\nMTD erase size: %x bytes\n",
        mtd_info.type, mtd_info.size, mtd_info.erasesize);

    ei.length = mtd_info.erasesize;   //set the erase block size
    pbuf = (unsigned char*)malloc(ei.length);
    if(pbuf == NULL)
    {
        printf("malloc fail\n");
        goto end;
    }
    printf("read %d\n",ei.length);

    fp = fopen(filename,"rb");
    if(fp == NULL)
    {
        printf("fopen %s fail\n",filename);
        goto end;
    }
    fseek(fp,0,SEEK_END);
    filelen = ftell(fp);
    printf("file len %d\n",filelen);
    fseek(fp,0,SEEK_SET);
    totallen = filelen;

    if(totallen > mtd_info.size)
    {
        printf("file is too big %d %d\n",totallen,mtd_info.size);
        goto end;
    }

    //���������߼��鵽�����ÿ��ӳ�䡣������߼���Ϊ���飬��÷����Ӹ��߼��鿪ʼ����Ŀ�ȫ�ǻ���
    for(ei.start = 0; ei.start < mtd_info.size; ei.start += ei.length)
    {
        //�жϸ�block�ǲ��Ǻõġ�
        offset = (long long)ei.start;
        ret = ioctl(fd,MEMGETBADBLOCK,&offset);
        if(ret < 0)
        {
            printf("MEMGETBADBLOCK fail,param error\n");
            goto end;
        }
        else if(ret > 0)
        {
            printf("this block is bad,behind this block all bad,offset %d\n",ei.start);
            goto eraseend;
        }
        else if(ret == 0)
        {
            printf("block %d is a good block\n",ei.start/ei.length);
        }
        //����
        if(ioctl(fd, MEMERASE, &ei) < 0)
        {
            printf("erase blk fail\n");
            //����ʧ�ܱ�ǻ���
            ret = ioctl(fd,MEMSETBADBLOCK,&offset);
            if(ret < 0)
            {
                printf("MEMSETBADBLOCK fail,this should not happen\n");
                goto end;
            }
        }
    }

    eraseend:
    realmtdsize = ei.start;
    //��Ӧ�ó��������������������ˣ�˵�������ֵ�̫С���ÿ鲻����Ÿ÷��������ݡ�
    if(totallen > realmtdsize)
    {
            printf("good block is too small\n");
            goto end;
    }
    printf("write\n");
    lseek(fd, 0, SEEK_SET);
    for(ei.start = 0;ei.start < realmtdsize; ei.start += ei.length)
    {
        if(totallen >= ei.length)
            readlen = ei.length;
        else
            readlen = totallen;

        memset(pbuf,0xff,ei.length);
        fread(pbuf,1,readlen,fp);
        printf("write %d\n",ei.start);
        if(write(fd, pbuf,ei.length) != ei.length)
        {
            printf("write fail\n");
            goto end;
        }
        printf("write %d end\n",ei.start);
        totallen -= readlen;
        if(totallen == 0)
        {
            ret = 0;
            break;
        }

        printf(".");
    }

    printf("\n");
    printf("write end\n");

    end:
    if(pbuf)
        free(pbuf);

    if(fp)
        fclose(fp);

    close(fd);

    if(ret)
        printf("update fail\n");
    else
        printf("update ok\n");

    return ret;
}

/*
0x000000000000-0x000000080000 : "spl"
0x000000080000-0x000000180000 : "uboot"
0x000000180000-0x000000200000 : "logo"
0x000000200000-0x000000280000 : "dtb"
0x000000280000-0x000000900000 : "kernel"
0x000000900000-0x000001100000 : "rootfs"
0x000001100000-0x000001200000 : "private"
0x000001200000-0x000007f80000	: "user"
user分区是可读写的。不支持通过/dev/mtd升级整个分区
private 可以上锁，不要通过/dev/mtd升级。
#./update spl.img                 /dev/mtd0  升级spl
./update  uboot.img               /dev/mtd1  升级uboot
./update  logo.jpg                /dev/mtd2  升级logo
./update  dpchip-v2p-chip-nx5.dtb /dev/mtd3  升级dtb
./update  zImage                  /dev/mtd4  升级内核
./update  rootfs.fex              /dev/mtd5  升级根文件系统
*/

struct system_map
{
    char filename[64];
    char portname[64];
};



int upgrade_spl_by_file(char *pfilename, char *partname);
int update_uboot_by_file(char*filename);
int update_kernel_by_file(char*filename);
int update_root_by_file(char*filename);

struct system_map upguardsystem[] = {
    {.portname = "/dev/mtd0", .filename = UPDATE_SYSTEM_PATH"/u-boot-spl_nand_header.bin" },
    {.portname = "/dev/mtd1", .filename = UPDATE_SYSTEM_PATH"/uboot_nand_header.bin" },     //uboot
    {.portname = "/dev/mtd2", .filename = UPDATE_SYSTEM_PATH"/zImage-dtb" },                 //kernel
    {.portname = "/dev/mtd3", .filename = UPDATE_SYSTEM_PATH"/rootfs_squashfs.img" },  //root
    {.portname = "/dev/mtd4", .filename = UPDATE_SYSTEM_PATH"/ota.img" },
};

static int upgrade_check_sys(void)
{
    for (size_t fexid = 0; fexid < sizeof(upguardsystem)/sizeof(struct system_map); fexid++)
    {
        if (access(upguardsystem[fexid].filename, F_OK) != 0)
            continue;
        if (strcmp(upguardsystem[fexid].portname, "/dev/mtd0")==0)
        {
	        upgrade_spl_by_file(upguardsystem[fexid].filename, upguardsystem[fexid].portname);
            continue;
        }
        else if(strcmp(upguardsystem[fexid].portname, "/dev/mtd1")==0){
            update_uboot_by_file(upguardsystem[fexid].filename);
        }
        else if(strcmp(upguardsystem[fexid].portname, "/dev/mtd2")==0){
            update_kernel_by_file(upguardsystem[fexid].filename);
        }
        else if(strcmp(upguardsystem[fexid].portname, "/dev/mtd3")==0){
            update_root_by_file(upguardsystem[fexid].filename);
        }

        // if(updata_part(upguardsystem[fexid].portname, upguardsystem[fexid].filename))
        // {
        //     printf("sys update %s------>%s fail\n", upguardsystem[fexid].filename, upguardsystem[fexid].portname);
        //     return -1;
        // }
        printf("sys update %s------>%s ok\n", upguardsystem[fexid].filename, upguardsystem[fexid].portname);
    }

    return 0;
}

/**
 * @brief 删除升级文件
 * @return int
 */
int NX5_CheckAndDelFile(void)
{
    int ret = 0;
    //1.删除残存的升级文件
    if(access(UPGRADE_UP_PATH, F_OK) == 0 && file_remove(UPGRADE_UP_PATH))
    {
        LOGW("file_remove "UPGRADE_UP_PATH" fail\n");
    }
    if(access(UPGRADE_BAK_PATH, F_OK) == 0 && file_remove(UPGRADE_BAK_PATH))
    {
        LOGW("file_remove "UPGRADE_BAK_PATH" fail\n");
    }
    if (access(UPDATE_SYSTEM_PATH, F_OK) == 0 && file_remove(UPDATE_SYSTEM_PATH))
    {
        LOGW("file_remove "UPDATE_SYSTEM_PATH" fail\n");
    }
    if (access(UPDATE_SYSTEM_PATH, F_OK) == 0 && file_remove(UPGRAED_PACKET_PATH))
    {
        LOGW("file_remove "UPDATE_SYSTEM_PATH" fail\n");
    }
    return ret;
}

static int file_number = 0;
// static int one_process = 0;
// static int last_line = 0;

void check_process_callback(int line)
{
    // if(line - last_line > one_process)
    // {
    //     last_line = line;
    //     //write_event_to_lua("app_upgrade", "%s %d", "check_process", line * 100 / file_number);
    //     printf("check_process_callback %d\n", line * 100 / file_number);
    // }
    // else if(line == -1)
    // {
    //     printf("check_process_callback end\n");
    //     //write_event_to_lua("app_upgrade", "%s %d", "check_process", 100);
    // }
}


int NAND_FLASH_UpGrade(void)
{
    //1.删除残存的升级文件
    if(file_remove(UPGRADE_UP_PATH))
    {
        LOGW("file_remove "UPGRADE_UP_PATH" fail\n");
    }
    if(file_remove(UPGRADE_BAK_PATH))
    {
        LOGW("file_remove "UPGRADE_BAK_PATH" fail\n");
    }
    if (file_remove(UPDATE_SYSTEM_PATH))
    {
        LOGW("file_remove "UPDATE_SYSTEM_PATH" fail\n");
    }

    //2.解压升级压缩包
    if((file_number = dp_untar(UPGRAED_PACKET_PATH)) < 0)
    {
        LOGW("dp_untar "UPGRAED_PACKET_PATH" fail\n");
        goto fail;
    }
    //刷新缓冲区，防止此处重启文件系统时丢失数据
    sync();
    //3.检查md5
    if (md5sum_check("/system/check.md5", check_process_callback) != 0)
    {
        LOGW("md5sum_check fail\n");
        file_remove(UPGRADE_UP_PATH);
        goto fail;
    }

    //4.备份原系统文件
    if(rename(UPGRADE_PATH, UPGRADE_BAK_PATH))
    {
        LOGW("rename "UPGRADE_PATH" fail\n");
        file_remove(UPGRADE_UP_PATH);
        goto fail;
    }

    //5.更新最新系统文件
    if(rename(UPGRADE_UP_PATH, UPGRADE_PATH))
    {
        LOGW("rename "UPGRADE_UP_PATH" fail\n");
        file_remove(UPGRADE_UP_PATH);
        rename(UPGRADE_BAK_PATH, UPGRADE_PATH);
        goto fail;
    }

    // 检查是否有系统升级
    upgrade_check_sys();

fail:
    file_remove(UPDATE_SYSTEM_PATH);
    file_remove(UPGRADE_BAK_PATH);
    file_remove(UPGRAED_PACKET_PATH);
    return 1;
}


void Clear_Upgrade_File(void)
{
    file_remove(UPGRADE_UP_PATH);
    file_remove(UPGRADE_BAK_PATH);
    file_remove(UPDATE_SYSTEM_PATH);
    file_remove(UPGRAED_PACKET_PATH); //DOWNLOAD_IMAGE_PATH
}

#endif //#ifdef ENABLE_V6