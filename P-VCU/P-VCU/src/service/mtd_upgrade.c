#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>

#include <md5.h>
#include <log.h>

#ifdef ENABLE_V6

#include <types/vmm_api.h>
#endif //ENABLE_V6

int OTA_burnboot0(const char *img_path);
int OTA_burnuboot(const char *img_path);

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define MBR_SIZE (1 * 1024)
#define MBR_MAGIC "dpower"
#define MBR_MAGIC_EX "DPOWER"
#define MBR_MAX_PART_COUNT 15
#define MBR_RESERVED (MBR_SIZE - 58 - (MBR_MAX_PART_COUNT * sizeof(partition_info_t)))

/* partition information */
typedef struct partition_info
{
	char name[32];
	char mtd_path[16];
	uint32_t size;
	uint8_t reserved[12];
} __attribute__((packed)) partition_info_t;

/* mbr information */
typedef struct mbr_info
{
	uint8_t magic[6];
	uint8_t md5[16];
	char softver[32];
	uint32_t partition_count;
	partition_info_t array[MBR_MAX_PART_COUNT];
	uint8_t reserved[MBR_RESERVED];
} __attribute__((packed)) mbr_info_t;

static int upgrade_partition(partition_info_t *partition, uint8_t *image_data)
{
	int partition_fd = -1;
	int ret = -1;

	uint32_t total_len = 0, write_len = 0, percent = 0, left_len = 0, cp_len = 0;
	uint8_t *pbuf = NULL;

	left_len = total_len = partition->size;

	mtd_info_t mtd_info;
	memset(&mtd_info, 0, sizeof(mtd_info));

	erase_info_t erase_info;
	memset(&erase_info, 0, sizeof(erase_info));

	partition_fd = open(partition->mtd_path, O_RDWR);

	if (partition_fd < 0)
	{
		printf("open %s fail \n", partition->mtd_path);
		goto end;
	}

	/* get the device info */
	if (ioctl(partition_fd, MEMGETINFO, &mtd_info) != 0)
	{
		printf("MEMGETINFO fail %s \n", strerror(errno));
		goto end;
	}

	printf("MTD Type:  %d \n", mtd_info.type);
	printf("MTD flags: %d\n", mtd_info.flags);
	printf("MTD total size: 0x%x bytes\n", mtd_info.size);
	printf("MTD erase_info size: 0x%x bytes\n", mtd_info.erasesize);
	printf("MTD writesize size: 0x%x bytes\n", mtd_info.writesize);
	printf("MTD oobsize size: 0x%x bytes\n", mtd_info.oobsize);

	/* set the erase_info block size */
	erase_info.length = mtd_info.erasesize;

	pbuf = (uint8_t *)malloc(erase_info.length);

	if (pbuf == NULL)
	{
		printf("mymalloc fail %s \n", strerror(errno));
		goto end;
	}

	// LOGI("read 0x%x bytes \n", erase_info.length);

	if (total_len > mtd_info.size)
	{
		printf("data is too big %d %d \n", total_len, mtd_info.size);
		goto end;
	}

	printf("upgrade %s ====> %s waiting...\n", partition->name, partition->mtd_path);

	for (erase_info.start = 0; erase_info.start < mtd_info.size;
		 erase_info.start += erase_info.length)
	{
		if (ioctl(partition_fd, MEMERASE, &erase_info) != 0)
		{
			printf("erase_info blk fail \n");
			goto end;
		}

		if (left_len >= erase_info.length)
		{
			cp_len = erase_info.length;
		}
		else
		{
			cp_len = left_len;
		}

		memset(pbuf, 0xFF, erase_info.length);

		memcpy(pbuf, image_data + write_len, cp_len);

		if (write(partition_fd, pbuf, erase_info.length) != (ssize_t)erase_info.length)
		{
			printf("write %s fail %s \n", partition->mtd_path, strerror(errno));
			goto end;
		}

		write_len += cp_len;

		left_len -= cp_len;

		percent = (write_len * 100) / total_len;

		printf("totallen %d Writelen %d leftlen %d %d%% \n", total_len, write_len,
			  left_len, percent);

		if (left_len == 0)
		{
			ret = 0;
			break;
		}
	}

	ret = 0;

end:

	if (partition_fd >= 0)
	{
		close(partition_fd);
		partition_fd = -1;
	}

	if (pbuf)
	{
		free(pbuf);
		pbuf = NULL;
	}

	printf("upgrade %s ====> %s %s \n", partition->name, partition->mtd_path,
		  ret == 0 ? "successful" : "fail");

	return ret;
}

static void write_tmp_file(const char * file_path, uint8_t * buff, uint32_t size)
{
    FILE * fb = fopen(file_path, "wb");

    fwrite(buff, size, 1, fb);
    fflush(fb);

    fclose(fb);
}

int upgrade_image_with_buffer(uint8_t *image_data, size_t image_length)
{
	int ret = -1;

	uint32_t i = 0;

	mbr_info_t *mbr = (mbr_info_t *)image_data;

	uint8_t *ptr = (uint8_t *)(mbr + 1);

	for (i = 0; i < mbr->partition_count; ++i)
	{
        if (strcmp(mbr->array[i].mtd_path, "ota_boot0") == 0)
        {
            write_tmp_file("/tmp/boot0.fex", ptr, mbr->array[i].size);
#ifdef ENABLE_V1
            OTA_burnboot0("/tmp/boot0.fex");
#endif //#ifdef ENABLE_V1
        }
        else if (strcmp(mbr->array[i].mtd_path, "ota_uboot") == 0)
        {
            write_tmp_file("/tmp/uboot.fex", ptr, mbr->array[i].size);
#ifdef ENABLE_V1
		    OTA_burnuboot("/tmp/uboot.fex");
#endif //#ifdef ENABLE_V1
        }
        else
        {
    		upgrade_partition(&mbr->array[i], ptr);
        }

		ptr += mbr->array[i].size;
	}

	ret = 0;

	return ret;
}

int ota_upgrade_image_with_buffer(uint8_t *image_data, size_t image_length)
{
	int ret = -1;

	mbr_info_t *mbr = NULL;

	uint8_t md5_buffer[16];
	memset(md5_buffer, 0, sizeof(md5_buffer));

	md5_ctx_t ctx;
	memset(&ctx, 0, sizeof(ctx));

	if (image_length <= sizeof(mbr_info_t))
	{
		LOGW("check ota image file length error \n");
		goto end;
	}

	mbr = (mbr_info_t *)image_data;

	if (memcmp(mbr->magic, MBR_MAGIC, 6) != 0 && memcmp(mbr->magic, MBR_MAGIC_EX, 6) != 0)
	{
		LOGW("check ota image magic error, magic is %s \n", mbr->magic);
		goto end;
	}

	md5_begin(&ctx);

	md5_hash(image_data + sizeof(mbr_info_t),
			 image_length - sizeof(mbr_info_t),
			 &ctx);

	md5_end(md5_buffer, &ctx);

	if (memcmp(mbr->md5, md5_buffer, 16) != 0)
	{
		LOGW("check ota image md5 error \n");
		goto end;
	}

	upgrade_image_with_buffer(image_data, image_length);

	ret = 0;

end:

	if (image_data)
	{
		free(image_data);
		image_data = NULL;
	}

	return ret;
}

/**
 * @brief 升级分区程序
 * @param [分区名] part         /dev/mtd4               /dev/mtd5
 * @param [升级文件] filename   rootfs_squashfs.img     system_squashfs.img
 * @return int
 */
int V6_updata_part(char*part,char*filename)
{
    if (!part || !filename)
    {
        LOGW("NX5_updata_part part:%p  filename:%p \n",part, filename);
        return -1;
    }

	mtd_info_t mtd_info;
    erase_info_t ei;
    int fd ;
    //int blkcnt;
    //int i;
    //long long offset;
    //int realmtdsize;
    unsigned int readlen,filelen,totallen;
        FILE*fp = NULL;
    size_t  writelen = 0, UpdatePrecent = 0;
    unsigned char *pbuf = NULL;
    int ret = -1;

    fd = open(part, O_RDWR);
    if(fd < 0)
    {
        LOGW("open %s fail\n",part);
        return -1;
    }

    if(ioctl(fd, MEMGETINFO, &mtd_info) < 0)   // get the device info
    {
        LOGW("MEMGETINFO fail\n");
        goto end;
    }

    LOGI("MTD Type: %x\nMTD total size: %x bytes\nMTD erase size: %x bytes\n",
        mtd_info.type, mtd_info.size, mtd_info.erasesize);

    ei.length = mtd_info.erasesize;   //set the erase block size
    pbuf = (unsigned char*)malloc(ei.length);
    if(pbuf == NULL)
    {
        LOGE("malloc fail\n");
        goto end;
    }
    LOGI("read %d\n",ei.length);

    fp = fopen(filename,"rb");
    if(fp == NULL)
    {
        LOGW("fopen %s fail\n",filename);
        goto end;
    }
    fseek(fp,0,SEEK_END);
    filelen = ftell(fp);
    LOGW("Updata file len %d\n",filelen);
    fseek(fp,0,SEEK_SET);
    totallen = filelen;

    if(totallen > mtd_info.size)
    {
        LOGE("Update file : %s is too big %d %d\n", filename,totallen,mtd_info.size);
        goto end;
    }

    for(ei.start = 0; ei.start < mtd_info.size; ei.start += ei.length)
    {
        //擦除
        if(ioctl(fd, MEMERASE, &ei) < 0)
        {
            LOGW("erase blk fail\n");
            goto end;
        }

        if(totallen >= ei.length)
            readlen = ei.length;
        else
            readlen = totallen;

        //memset(pbuf,0x00,ei.length);
        memset(pbuf,0xff,ei.length);
        if(fread(pbuf,1,readlen,fp) != readlen)
        {
            LOGW("read %s fail\n",filename);
            goto end;
        }
        //printf("write %d\n",ei.start);
        if((unsigned int)write(fd, pbuf,ei.length) != ei.length)
        {
            LOGW("write fail\n");
            goto end;
        }
        //printf("write %d end\n",ei.start);
        totallen -= readlen;

        writelen += readlen;


        //LOG(LOGINFO, "filelen:%d  totallen:%d   %f", filelen, totallen, (float)totallen/filelen);
        UpdatePrecent = (writelen * 100 ) / filelen;

        LOGI("totallen %d Writelen %d leftlen %d %d%% \n", filelen, writelen, totallen, UpdatePrecent);

        //DPPostMessage(MSG_PRIVATE, UPGRATE_APPID, 1, UpdatePrecent);
        if(totallen == 0)
        {
            ret = 0;
            break;
        }

    }

end:
	if(pbuf)
   	free(pbuf);

    if(fp)
    	fclose(fp);

    close(fd);

    if(ret)
    	LOGI("update fail\n");
    else
    	LOGI("update ok\n");

    // if (ret != 0)
    // {
    //     DPPostMessage(MSG_PRIVATE, UPGRATE_APPID, 0, UPDATE_FILE_ERR);
    // }
    // else
    // {
    //     DPPostMessage(MSG_PRIVATE, UPGRATE_APPID, 0, UPDATE_FILE_OK);
    // }

    return ret;
}


int updata_mtd_part_by_fd(char*part, FILE*pfile, unsigned int filelen)
{
    if (!part || !pfile || filelen == 0)
    {
        LOGW("NX5_updata_part part:%p  pfile:%p filelen:%d\n",part, pfile, filelen);
        return -1;
    }

	mtd_info_t mtd_info;
    erase_info_t ei;
    int fd ;

    unsigned int readlen,totallen;
    size_t  writelen = 0, UpdatePrecent = 0;
    unsigned char *pbuf = NULL;
    int ret = -1;

    fd = open(part, O_RDWR);
    if(fd < 0)
    {
        LOGW("open %s fail\n",part);
        return -1;
    }

    if(ioctl(fd, MEMGETINFO, &mtd_info) < 0)   // get the device info
    {
        LOGW("MEMGETINFO fail\n");
        goto end;
    }

    LOGI("MTD Type: %x\nMTD total size: %x bytes\nMTD erase size: %x bytes\n\n",
        mtd_info.type, mtd_info.size, mtd_info.erasesize);

    ei.length = mtd_info.erasesize;   //set the erase block size
    pbuf = (unsigned char*)malloc(ei.length);
    if(pbuf == NULL)
    {
        LOGE("malloc fail\n");
        goto end;
    }
    LOGI("read %d\n",ei.length);

    LOGW("Updata part:%s file len %d\n", part,filelen);
    totallen = filelen;

    if(totallen > mtd_info.size)
    {
        LOGE("Update part : %s is too big %d %d\n", part,totallen,mtd_info.size);
        goto end;
    }

    for(ei.start = 0; ei.start < mtd_info.size; ei.start += ei.length)
    {
        //擦除
        if(ioctl(fd, MEMERASE, &ei) < 0)
        {
            LOGW("erase blk fail\n");
            goto end;
        }

        if(totallen >= ei.length)
            readlen = ei.length;
        else
            readlen = totallen;

        //memset(pbuf,0x00,ei.length);
        memset(pbuf,0xff,ei.length);
        if(fread(pbuf,1,readlen,pfile) != readlen)
        {
            LOGW("read fail\n");
            goto end;
        }
        if((unsigned int)write(fd, pbuf,ei.length) != ei.length)
        {
            LOGW("write fail\n");
            goto end;
        }
        totallen -= readlen;
        writelen += readlen;

        UpdatePrecent = (writelen * 100 ) / filelen;

        LOGI("totallen %d Writelen %d leftlen %d %d%% \n", filelen, writelen, totallen, UpdatePrecent);

        if(totallen == 0)
        {
            ret = 0;
            break;
        }
    }

end:
	if(pbuf)
   		free(pbuf);

    close(fd);

    if(ret)
    	LOGI("update fail\n");
    else
    	LOGI("update ok\n");

    return ret;
}


int upgrade_check_by_magic(char*pfilename)
{
	int ret = -1;
	FILE *fp = NULL;

	unsigned int filelen = 0;
	mbr_info_t mbr = {0};
	uint8_t md5_buffer[16];

	do
	{
		//1.打开文件
		fp = fopen(pfilename, "rb");
		if (fp == NULL)
		{
			LOGW("fopen %s fail,errno is %d\n", pfilename, errno);
			break;
		}

		//2.获取文件长度
		fseek(fp, 0, SEEK_END);
		filelen = ftell(fp);
		LOGI("Updata file len %u\n", filelen);
		fseek(fp, 0, SEEK_SET);

		//3.初始化md5计算器
		memset(md5_buffer, 0, sizeof(md5_buffer));

		md5_ctx_t ctx;
		memset(&ctx, 0, sizeof(ctx));

		//4.读取MBR信息
		if (fread(&mbr, 1, sizeof(mbr), fp) != sizeof(mbr))
		{
			LOGW("read mbr_info_t sizo fail \n");
			break;
		}

		//5.校验MBR信息
		if (memcmp(mbr.magic, MBR_MAGIC, 6) != 0 && memcmp(mbr.magic, MBR_MAGIC_EX, 6) != 0)
		{
			LOGW("check ota image magic error, magic is %s \n", mbr.magic);
			break;;
		}

		if (strstr(mbr.softver, "V6")==NULL)
		{
			LOGW("check ota image is V6 version:%s, not support \n", mbr.softver);
			break;
		}

		//6.计算文件MD5
		char buf[256];
		md5_begin(&ctx);
		do
		{
			int len = fread(buf, 1, sizeof(buf), fp);
			if (!len)
				break;

			md5_hash(buf, len, &ctx);
			ret += len;
		} while (1);

		md5_end(md5_buffer, &ctx);

		if (memcmp(mbr.md5, md5_buffer, 16) != 0)
		{
			LOGW("check ota image md5 error \n");
			break;
		}

		filelen = ftell(fp);
		fseek(fp, sizeof(mbr), SEEK_SET);
		filelen = ftell(fp);

		for (int i = 0; i < mbr.partition_count; ++i)
		{
			LOGI("partition %d name:%s  path: %s, size: %d, start: %lu\n", i, mbr.array[i].name, mbr.array[i].mtd_path, mbr.array[i].size, ftell(fp));
			if(strncmp(mbr.array[i].name, "UpdateCco.bin", 13) == 0)
			{
				LOGI("find UpdateCco.bin partition\n");
				//通过文件描述符升级PLC固件
				int goblin_plc_upgrade_firmware_by_fd(FILE *pfd, uint32_t file_size);
				int goblin_plc_reboot(void);
				goblin_plc_upgrade_firmware_by_fd(fp, mbr.array[i].size);
				goblin_plc_reboot();
			}
			else
				updata_mtd_part_by_fd(mbr.array[i].mtd_path, fp, mbr.array[i].size);
		}

		ret = 0;
		LOGI("Upgrade success\n");
	} while (0);

	//关闭打开的文件
	if(fp)
		fclose(fp);

	return ret;
}

int V6_ota_upgrade_image(char *pfilename)
{
	int ret = -1;

	//如果分区升级不成功，就按system分区升级
	if(upgrade_check_by_magic(pfilename) != 0)
	{
		LOGW("upgrade_check_by_magic fail\n");
		//V6_updata_part("/dev/mtd4", pfilename);
	}
	else
		ret = 0;
	// printf("\nplease press twice ENTER to exit this sample\n");
	// getchar();
    // getchar();

	return ret;
}

int ota_upgrade_image(char *pfilename)
{
#ifdef ENABLE_SPINAAD
	int NAND_FLASH_UpGrade(void);
	return NAND_FLASH_UpGrade();
#endif
#ifdef ENABLE_V6
	return V6_ota_upgrade_image(pfilename);
#endif
	FILE *fp = NULL;
	unsigned char *pbuf = NULL;
	fp = fopen(pfilename, "rb");
	unsigned long filelen = 0;
	if (fp == NULL)
	{
		printf("fopen %s fail,errno is %d\n", pfilename, errno);
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	filelen = ftell(fp);
	printf("Updata file len %ld\n", filelen);
	fseek(fp, 0, SEEK_SET);
	pbuf = (unsigned char *)malloc(filelen);
	if (pbuf == NULL)
	{
		printf("mymalloc fail\n");
		fclose(fp);
		return -1;
	}
	unsigned long readlen = filelen;
	if (fread(pbuf, 1, readlen, fp) != readlen)
	{
		fclose(fp);
		free(pbuf);
		return -1;
	}
	fclose(fp);
	return ota_upgrade_image_with_buffer(pbuf, readlen);
}
