#include <stdio.h>
#include <stdint.h>

#include "wave_file_fmt.h"

#define LOG_TAG "wav_file_fmt"
#include <utils/log.h>

static void pack32be(unsigned long value, unsigned char **buffer, unsigned long *buffer_size)
{
    if (buffer && *buffer && buffer_size && *buffer_size >= 4)
    {
        unsigned char *pbuf = *buffer;
        pbuf[0] = (unsigned char)((value & 0xFF000000) >> 24);
        pbuf[1] = (unsigned char)((value & 0x00FF0000) >> 16);
        pbuf[2] = (unsigned char)((value & 0x0000FF00) >> 8);
        pbuf[3] = (unsigned char)(value & 0x000000FF);

        *buffer += 4;
        *buffer_size -= 4;
    }
}

static void pack32le(unsigned long value, unsigned char **buffer, unsigned long *buffer_size)
{
    if (buffer && *buffer && buffer_size && *buffer_size >= 4)
    {
        unsigned char *pbuf = *buffer;

        pbuf[0] = (unsigned char)(value & 0x000000FF);
        pbuf[1] = (unsigned char)((value & 0x0000FF00) >> 8);
        pbuf[2] = (unsigned char)((value & 0x00FF0000) >> 16);
        pbuf[3] = (unsigned char)((value & 0xFF000000) >> 24);

        *buffer += 4;
        *buffer_size -= 4;
    }
}

static void pack16le(unsigned short value, unsigned char **buffer, unsigned long *buffer_size)
{
    if (buffer && *buffer && buffer_size && *buffer_size >= 2)
    {
        unsigned char *pbuf = *buffer;

        pbuf[0] = (unsigned char)(value & 0x000000FF);
        pbuf[1] = (unsigned char)((value & 0x0000FF00) >> 8);

        *buffer += 2;
        *buffer_size -= 2;
    }
}

int read_wave_info(FILE **fd, wave_hdr_t *wave_hdr)
{
    int bret = 0;

    unsigned int size_to_read = sizeof(wave_hdr_t) - 8;

    int status = 1;

    fseek(*fd, 0, SEEK_SET);

    if (size_to_read != fread(wave_hdr, 1, size_to_read, *fd))
    {
        goto end;
    }

    if (wave_hdr->riff != RIFF_TAG || wave_hdr->wave != WAVE_TAG || wave_hdr->fmt != FMT_TAG)
    {
        LOGE("unknow wave format riff:0x%08x wave:0x%08x fmt:0x%08x \n", wave_hdr->riff, wave_hdr->wave, wave_hdr->fmt);
        goto end;
    }

    switch (wave_hdr->fmt_tag)
    {
        case WAVE_FMT_TAG_PCM:
            {
                if (wave_hdr->bits_per_sample != 16 || wave_hdr->block_align != (2 * wave_hdr->channel))
                    status = 0;
            }
        break;
        // case WAVE_FMT_TAG_ALAW:
        // case WAVE_FMT_TAG_ULAW:
        //     {
        //         if(wave_hdr->bits_per_sample != 8 ||
        //             wave_hdr->block_align != wave_hdr->channel)
        //             status = 0;
        //     }
        //     break;
        default:
            LOGE("unsupport wav format tag :%d \n", wave_hdr->fmt_tag);
            status = 0;
            break;
    }

    if (!status)
        goto end;

    if (wave_hdr->fmt_len > 16)
    {
        size_to_read = wave_hdr->fmt_len - 16;

        fseek(*fd, size_to_read, SEEK_CUR);
    }

    for (;;)
    {
        wave_subchunk_t subchunk;

        size_t size_read = 8;

        size_read = fread(&subchunk, 1, sizeof(wave_subchunk_t), *fd);

        if (size_read != sizeof(wave_subchunk_t))
            goto end;

        if (subchunk.id == DATA_TAG)
        {
            wave_hdr->data = DATA_TAG;
            wave_hdr->data_len = subchunk.len;
            break;
        }

        if (0 != fseek(*fd, subchunk.len, SEEK_CUR))
            goto end;
    }

    bret = 1;

    LOGD("wav file info: %d Hz, %d channels, %d bits, %.3f seconds, datalen %d\n", wave_hdr->sample_rate, wave_hdr->channel,
         wave_hdr->bits_per_sample, (float)(wave_hdr->data_len / wave_hdr->block_align) / wave_hdr->sample_rate, wave_hdr->data_len);

end:
    return bret;
}

void update_wav_info(FILE **fd)
{
    unsigned char tmp[4];
    unsigned char *ptmp = NULL;
    unsigned long tmp_len = 0;

    unsigned long file_size = 0;
    unsigned long riff_size = 0;
    unsigned long data_size = 0;

    /*
    * Compute the RIFF chunk size and the
    * data chunk size from the size of the file.
    */
    fseek(*fd, 0, SEEK_END);

    file_size = (unsigned long)ftell(*fd);
    riff_size = file_size - 8;
    data_size = file_size - sizeof(wave_hdr_t);

    /* Seek to the RIFF chunk size */
    fseek(*fd, 4, SEEK_SET);

    /* Set up the packing buffer */
    ptmp = &tmp[0];
    tmp_len = sizeof(ptmp);

    /* Pack the RIFF chunk size */
    pack32le(riff_size, &ptmp, &tmp_len);

    /* Write out the buffer */
    fwrite(&tmp[0], 1, 4, *fd);

    /* Seek to the beginning of the data chunk size */
    fseek(*fd, 40, SEEK_SET);

    /* Set up the packing buffer */
    ptmp = &tmp[0];
    tmp_len = sizeof(ptmp);

    /* Pack the data chunk size */
    pack32le(data_size, &ptmp, &tmp_len);

    /* Write out the buffer */
    fwrite(&tmp[0], 1, 4, *fd);
}

int write_wave_info(FILE **fd, int samplerate, int samplebit, int channels)
{
    wave_hdr_t hdr;

    memset(&hdr, 0, sizeof(hdr));

    unsigned char *wav_hdr = (unsigned char *)&hdr;

    fseek(*fd, 0, SEEK_SET);

    unsigned long hdr_len = sizeof(hdr);

    // Pack the RIFF four cc
    pack32be(TAG_FOURCC_RIFF, &wav_hdr, &hdr_len);
    // Pack 0 for the RIFF chunk size - update in update_wav_info.
    pack32le(0, &wav_hdr, &hdr_len);
    // Pack the WAVE four cc
    pack32be(TAG_FOURCC_WAVE, &wav_hdr, &hdr_len);
    // Pack the 'fmt ' subchunk four cc
    pack32be(TAG_FOURCC_fmt, &wav_hdr, &hdr_len);
    // Pack the fmt subchunk size
    pack32le(0x10, &wav_hdr, &hdr_len);
    // Pack the audio format
    pack16le((unsigned short)WAVE_FMT_TAG_PCM, &wav_hdr, &hdr_len);
    // Pack the number of channels
    pack16le((unsigned short)channels, &wav_hdr, &hdr_len);
    // Pack the sample rate
    pack32le((unsigned long)samplerate, &wav_hdr, &hdr_len);
    // Pack the byte rate
    pack32le((unsigned long)(channels * (samplebit / 8) * samplerate), &wav_hdr, &hdr_len);
    // Pack the block align
    pack16le((unsigned short)(channels * (samplebit / 8)), &wav_hdr, &hdr_len);
    // Pack the bits per sample
    pack16le((unsigned short)samplebit, &wav_hdr, &hdr_len);
    // Pack the 'data' subchunk four cc
    pack32be(TAG_FOURCC_data, &wav_hdr, &hdr_len);
    // Pack the data subchunk size (0 for now - update in update_wav_info)
    pack32le(0, &wav_hdr, &hdr_len);

    fwrite(&hdr, 1, sizeof(hdr), *fd);

    LOGD("riff:0x%08x wave:0x%08x fmt:0x%08x data:0x%08x\n", hdr.riff, hdr.wave, hdr.fmt, hdr.data);

    return 1;
}