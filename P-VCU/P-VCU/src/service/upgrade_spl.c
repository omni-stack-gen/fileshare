//=====================update spl================
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <mtd/mtd-user.h>


#define SPL_ECC		_IO('S', 1)

struct spl_param_t
{
	unsigned int *buffer;
	unsigned int length;
};

int compound_spl_by_file(char *pfilename, char *partname)
{
    if(!pfilename || !partname)
    {
        printf("filename or partname is null\n");
        return -1;
    }
	int fd;
	unsigned int *buf = NULL;
	FILE*fp = NULL;
	int filesize = 0;
	int ret = -1;
	mtd_info_t mtd_info;
	struct spl_param_t spl_param;
	//int i = 0;

	//setvbuf(stdout, NULL, _IONBF, 0);

	printf("spl filename %s\n",pfilename);


	fd = open("/dev/mtd0", O_RDWR);
	if(fd < 0)
	{
		printf("open /dev/mtd0 fail\n");
		return -1;
	}

	if(ioctl(fd, MEMGETINFO, &mtd_info) < 0)   // get the device info
	{
		printf("MEMGETINFO fail\n");
		goto end;
	}

	printf("MTD Type: %x\nMTD total size: %x bytes\nMTD erase size: %x bytes\n",
	  mtd_info.type, mtd_info.size, mtd_info.erasesize);


	buf = malloc(mtd_info.erasesize);
	if(buf == NULL)
	{
		printf("malloc buf fail\n");
		return -1;
	}

	//spl 必须写一个block,数据不到一个block,填充0xff
	memset(buf,0xff,mtd_info.erasesize);


    fp = fopen(pfilename,"rb");
    if(fp == NULL)
  	{
  		printf("open %s fail\n",pfilename);
  		goto end;
  	}
  	fseek(fp,0,SEEK_END);
  	filesize = ftell(fp);
  	fseek(fp, 0, SEEK_SET);

  	if(fread(buf,1,filesize,fp) != filesize)
	{
		printf("read %s fail\n",pfilename);
		goto end;
	}

	spl_param.buffer = (unsigned int*)buf;
	spl_param.length = filesize;
	if(ioctl(fd, SPL_ECC, &spl_param) < 0)
	{
        printf("SPL_ECC  fail\n");
        goto end;
	}
	fclose(fp);

	fp = fopen(partname,"wb");
    if(fp == NULL)
  	{
  		printf("open %s fail\n",partname);
  		goto end;
  	}
	if(fwrite(buf,1,mtd_info.erasesize,fp) != mtd_info.erasesize)
	{
		printf("write spl.img fail\n");
		goto end;
	}
	fclose(fp);
	fp = NULL;
	//erase one block
	//system("flash_erase /dev/mtd0 0 1");
	//write spl in block0
	//system("nandwrite /dev/mtd0 spl.img -p");
    ret = 0;
  printf("spl ecc end\n");

end:
	if(fp)
		fclose(fp);

	if(buf)
		free(buf);

    if(fd >= 0)
   	 close(fd);

    return ret;
}

#include <utils/time_helper.h>
#include <utils/log.h>

bool ExecuteCmdByShell(char*cmd)
{
    char result_buf[256] = {0};

    // 用于接收命令返回值
    int rc = 0;

    // 执行预先设定的命令，并读出该命令的标准输出
    FILE *fp;
    unsigned int starttick =  time_relative_ms();
    fp = popen(cmd, "r");

    if(NULL == fp)
    {
        perror("popen fial\n");
        return false;
    }

    while(fgets(result_buf, sizeof(result_buf), fp) != NULL)
    {
        // 为了下面输出好看些，把命令返回的换行符去掉
        if('\n' == result_buf[strlen(result_buf) - 1])
        {
            result_buf[strlen(result_buf) - 1] = '\0';
        }

        //printf("output [%s] %u %u \n", result_buf, time_relative_ms(), starttick);

        // if(( time_relative_ms() - starttick) > 3000)
        //     break;
		// if (access(result_buf, F_OK) == 0)
		// {
		// 	printf("current root:%s is exist\n", result_buf);
		// 	break;
		// }
    }

    //等待命令执行完毕并关闭管道及文件指针
    rc = pclose(fp);

    if(0 != rc)
    {
        perror("close fail\n");
        return false;
    }
    else
    {
        printf("cmd:status[%d] returnvalue [%d]", rc, WEXITSTATUS(rc));
    }

	LOGI("cmd: %s , lost time: %u ms\n", cmd, (time_relative_ms() - starttick));
    return true;
}

bool GetInfoByShell(char*cmd, char *pbuf)
{
    char result_buf[256] = {0};

	bool bret = false;
    // 用于接收命令返回值
    int rc = 0;

    // 执行预先设定的命令，并读出该命令的标准输出
    FILE *fp;
    unsigned int starttick =  time_relative_ms();
    fp = popen(cmd, "r");

    if(NULL == fp)
    {
        perror("popen fial\n");
        return false;
    }

    while(fgets(result_buf, sizeof(result_buf), fp) != NULL)
    {
        // 为了下面输出好看些，把命令返回的换行符去掉
        if('\n' == result_buf[strlen(result_buf) - 1])
        {
            result_buf[strlen(result_buf) - 1] = '\0';
        }

        //printf("output [%s] %u %u \n", result_buf, time_relative_ms(), starttick);

        // if(( time_relative_ms() - starttick) > 3000)
        //     break;
		// if (access(result_buf, F_OK) == 0)
		// {
		// 	printf("current root:%s is exist\n", result_buf);
		// 	break;
		// }

		if(pbuf)
		{
			strncpy(pbuf, result_buf, 256);
			bret = true;
			break;
		}
    }

    //等待命令执行完毕并关闭管道及文件指针
    rc = pclose(fp);

    if(0 != rc)
    {
        perror("close fail\n");
        return false;
    }
    else
    {
        printf("cmd:status[%d] returnvalue [%d]", rc, WEXITSTATUS(rc));
    }

	LOGI("cmd: %s , lost time: %u ms\n", cmd, (time_relative_ms() - starttick));
    return bret;
}

//==================update ubboot ====================
/*
#!/bin/sh
#update first uboot
flash_erase /dev/mtd1 0 16
nandwrite /dev/mtd1 uboot_nand_header.bin -p
#update second uboot
flash_erase /dev/mtd1 0x200000 16
nandwrite -s 0x200000 /dev/mtd1 uboot_nand_header.bin -p
*/

int update_uboot_by_file(char*filename)
{
	//int ret = -1;
	if(access(filename, F_OK) != 0)
	{
		printf("file %s not exist\n",filename);
		return -1;
	}

	//update first uboot
	char strcmd[512] = {0};
	if(!ExecuteCmdByShell("/system/mtdutils/flash_erase /dev/mtd1 0 16")){
		LOGW("flash_erase /dev/mtd1 0 16 fail\n");
		return -1;
	}

	sprintf(strcmd,"/system/mtdutils/nandwrite /dev/mtd1 %s -p",filename);
	if(!ExecuteCmdByShell(strcmd)){
		LOGW("nandwrite /dev/mtd1 %s -p fail\n",filename);
		return -1;
	}

	//update second uboot
	if(!ExecuteCmdByShell("/system/mtdutils/flash_erase /dev/mtd1 0x200000 16")){
		LOGW("flash_erase /dev/mtd1 0x200000 16 fail\n");
		return -1;
	}

	sprintf(strcmd,"/system/mtdutils/nandwrite -s 0x200000 /dev/mtd1 %s -p",filename);
	if(!ExecuteCmdByShell(strcmd)){
		LOGW("nandwrite /dev/mtd1 %s -p fail\n",filename);
		return -1;
	}


	LOGI("update uboot success\n");
	return 0;
}



/*
PARTITION_A="kernelA"
PARTITION_B="kernelB"

kernel_part=$(fw_printenv kernel_partition 2>/dev/null | awk -F '=' '{print $2}')
echo "current kernel part $kernel_part"

if [ "$kernel_part" = "$PARTITION_A" ]; then
    upgrade_partition="$PARTITION_B"
elif [ "$kernel_part" = "$PARTITION_B" ]; then
    upgrade_partition="$PARTITION_A"
else
    echo "error"
    exit 2
fi

if [ "$upgrade_partition" = "$PARTITION_B" ];then
ubirmvol /dev/ubi0 -N kernelB
ubimkvol /dev/ubi0 -n 3 -N kernelB -s 6MiB -t static
ubiupdatevol /dev/ubi0_3 zImage-dtb
fw_setenv kernel_partition kernelB
fi


if [ "$upgrade_partition" = "$PARTITION_A" ];then
ubirmvol /dev/ubi0 -N kernelA
ubimkvol /dev/ubi0 -n 2 -N kernelA -s 6MiB -t static
ubiupdatevol /dev/ubi0_2 zImage-dtb
fw_setenv kernel_partition kernelA
fi
*/
int update_kernel_by_file(char*filename)
{
	const char PARTITION_A[64]="kernelA";
	const char PARTITION_B[64]="kernelB";
	char upgrade_partition[64] = {0};

	char kernel_part[256] = {0};
	if(!GetInfoByShell("fw_printenv kernel_partition 2>/dev/null | awk -F '=' '{print $2}'", kernel_part))
	{
		LOGW("get kernel_partition fail\n");
		return -1;
	}

	LOGI("current kernel part %s\n", kernel_part);
	if(strcmp(kernel_part, PARTITION_A) == 0)
	{
		strcpy(upgrade_partition, PARTITION_B);
	}
	else if(strcmp(kernel_part, PARTITION_B) == 0)
	{
		strcpy(upgrade_partition, PARTITION_A);
	}
	else
	{
		LOGW("error\n");
		return -1;
	}

	LOGI("upgrade partition %s\n", upgrade_partition);

	char strcmd[512] = {0};
	//1.
	sprintf(strcmd, "/system/mtdutils/ubirmvol /dev/ubi0 -N %s", upgrade_partition);
	if(!ExecuteCmdByShell(strcmd)){
		LOGW("cmd: %s fail\n", strcmd);
		return -1;
	}
	//2.
	sprintf(strcmd, "/system/mtdutils/ubimkvol /dev/ubi0 -n %u -N %s -s 6MiB -t static", strcmp(upgrade_partition, PARTITION_A)==0?2:3, upgrade_partition);
	if(!ExecuteCmdByShell(strcmd)){
		LOGW("cmd: %s fail\n", strcmd);
		//return -1;
	}

	//3.
	sprintf(strcmd, "/system/mtdutils/ubiupdatevol /dev/ubi0_%u %s", strcmp(upgrade_partition, PARTITION_A)==0?2:3, filename);
	if(!ExecuteCmdByShell(strcmd)){
		LOGW("cmd: %s fail\n", strcmd);
		return -1;
	}

	//4.
	sprintf(strcmd, "fw_setenv kernel_partition %s", upgrade_partition);
	if(!ExecuteCmdByShell(strcmd)){
		LOGW("cmd: %s fail\n", strcmd);
		return -1;
	}

	LOGI("update %s kernel success\n", upgrade_partition);
	return 0;
}

/**
PARTITION_A="/dev/ubiblock0_4"
PARTITION_B="/dev/ubiblock0_5"

current_root=$(sed -n 's/.*\broot=\([^ ]*\).*\/\1/p' /proc/cmdline)
echo $current_root

if [ "$current_root" = "$PARTITION_A" ]; then
    upgrade_partition="$PARTITION_B"
elif [ "$current_root" = "$PARTITION_B" ]; then
    upgrade_partition="$PARTITION_A"
else
    echo "error"
    exit 2
fi

if [ "$upgrade_partition" = "$PARTITION_B" ];then
ubirmvol /dev/ubi0 -N rootfsB
ubimkvol /dev/ubi0 -n 5 -N rootfsB -s 3MiB -t static
ubiupdatevol /dev/ubi0_5 rootfs_squashfs.img
fw_setenv root_partition 0,5
fw_setenv nand_root /dev/ubiblock0_5
fi


if [ "$upgrade_partition" = "$PARTITION_A" ];then
ubirmvol /dev/ubi0 -N rootfsA
ubimkvol /dev/ubi0 -n 4 -N rootfsA -s 3MiB -t static
ubiupdatevol /dev/ubi0_4 rootfs_squashfs.img
fw_setenv root_partition 0,4
fw_setenv nand_root /dev/ubiblock0_4
fi
**/

int update_root_by_file(char*filename)
{
	const char PARTITION_A[64]="/dev/ubiblock0_4";
	const char PARTITION_B[64]="/dev/ubiblock0_5";

	char current_root[256] = {0};
	char upgrade_partition[256] = {0};
	char upgrade_type = 0;
	if(!GetInfoByShell("sed -n 's/.*\\broot=\\([^ ]*\\).*/\\1/p' /proc/cmdline", current_root))
	{
		LOGW("get current_root fail\n");
		return -1;
	}

	if(access(current_root, F_OK) != 0)
	{
		LOGW("current_root %s not exist\n", current_root);
		return -1;
	}

	LOGI("current root partition %s\n", current_root);
	if (strcmp(current_root, PARTITION_A) == 0)
	{
		strcpy(upgrade_partition, PARTITION_B);
		upgrade_type = 1;
	}
	else if (strcmp(current_root, PARTITION_B) == 0)
	{
		strcpy(upgrade_partition, PARTITION_A);
		upgrade_type = 0;
	}
	else
	{
		LOGW("error\n");
		return -1;
	}

	char strcmd[512] = {0};
	//1.
	sprintf(strcmd, "/system/mtdutils/ubirmvol /dev/ubi0 -N %s", upgrade_type==0?"rootfsA":"rootfsB");
	if(!ExecuteCmdByShell(strcmd)){
		LOGW("cmd: %s fail\n", strcmd);
		return -1;
	}

	//2.
	sprintf(strcmd, "/system/mtdutils/ubimkvol /dev/ubi0 -n %u -N %s -s 3MiB -t static", upgrade_type==0?4:5, upgrade_type==0?"rootfsA":"rootfsB");
	if(!ExecuteCmdByShell(strcmd)){
		LOGW("cmd: %s fail\n", strcmd);
		return -1;
	}

	//3.
	sprintf(strcmd, "/system/mtdutils/ubiupdatevol /dev/ubi0_%u %s", upgrade_type==0?4:5, filename);
	if(!ExecuteCmdByShell(strcmd)){
		LOGW("cmd: %s fail\n", strcmd);
		return -1;
	}

	//4.
	sprintf(strcmd, "fw_setenv root_partition 0,%u", upgrade_type==0?4:5);
	if(!ExecuteCmdByShell(strcmd)){
		LOGW("cmd: %s fail\n", strcmd);
		return -1;
	}

	//5.
	sprintf(strcmd, "fw_setenv nand_root /dev/ubiblock0_%u", upgrade_type==0?4:5);
	if(!ExecuteCmdByShell(strcmd)){
		LOGW("cmd: %s fail\n", strcmd);
		return -1;
	}


	LOGI("updata root partition %s success\n", upgrade_partition);
	return 0;
}

int upgrade_spl_by_file(char *pfilename, char *partname)
{
	#define SPL_BIN_PATHNAME "/tmp/spl.bin"

	if(compound_spl_by_file(pfilename, SPL_BIN_PATHNAME)!=0)
	{
		LOGW("compound_spl_by_file fail\n");
		return -1;
	}

	if(access(SPL_BIN_PATHNAME, F_OK) != 0)
	{
		LOGW("SPL_BIN_PATHNAME %s not exist\n", SPL_BIN_PATHNAME);
		return -1;
	}

	char strcmd[512] = {0};
	int offsetarray[3] = {0, 0x20000, 0x20000};
	int indexarray[3] = {1, 1, 6};
	for (int i = 0; i < 3; i++)
	{
		//1.
		sprintf(strcmd, "/system/mtdutils/flash_erase /dev/mtd0 0x%x %u", offsetarray[i], indexarray[i]);
		if(!ExecuteCmdByShell(strcmd)){
			LOGW("cmd: %s fail\n", strcmd);
			return -1;
		}

		//2.
		sprintf(strcmd, "/system/mtdutils/nandwrite -s 0x%x  /dev/mtd0 %s", 0x20000*i, SPL_BIN_PATHNAME);
		if(!ExecuteCmdByShell(strcmd)){
			LOGW("cmd: %s fail\n", strcmd);
			return -1;
		}
	}

	return 0;
}