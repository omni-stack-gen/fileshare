#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>

#include <utils/mq.h>
#include <utils/wave_file_fmt.h>
#include <utils/time_helper.h>
#include <utils/bmem.h>
#include <utils/circlebuf.h>
#include <utils/rw_helper.h>

#include <sound/keytone_mgnt.h>
#include <sound/sound_play.h>
#include <AudioCore.h>
#define LOG_TAG "service_keytone"
#include <utils/log.h>

#define	BLOCKTIME         (20)     //每个缓冲数据块的时长（毫秒）
#define	PLAYBLOCKS        (4)      //播放最大缓冲数据块数

#define KEYTONE_LOCK_ON()                                       \
	do                                                          \
	{                                                           \
		pthread_mutex_lock(&keytone_mutex);                     \
	} while (0)

#define KEYTONE_LOCK_OFF()                                      \
	do                                                          \
	{                                                           \
		pthread_mutex_unlock(&keytone_mutex);                   \
	} while (0)


typedef struct keytone_mgnt
{
    rw_fd_t *fd;
    uint8_t *data;
    int stream_id;
    int count;
} keytone_mgnt_t;

static keytone_mgnt_t *g_keytone_mgnt = NULL;

static pthread_mutex_t keytone_mutex = PTHREAD_MUTEX_INITIALIZER;

static int audio_player_callback(char *data_buffer, int data_size, void *opaque)
{
    keytone_mgnt_t *mgnt = (keytone_mgnt_t *)opaque;

    int ret = 0;

    KEYTONE_LOCK_ON();

    if (mgnt->count > 0)
    {
#if 0
        if (!rw_eof(mgnt->fd))
        {
            int nread = rw_read(mgnt->fd, data_buffer, 1, data_size);

            if (nread < data_size)
            {
                memset(&data_buffer[nread], 0, data_size - nread);
            }
        }
        else
        {
            mgnt->count--;

            rw_seek(mgnt->fd, 0, SEEK_SET);

            if (mgnt->count > 0)
            {
                rw_read(mgnt->fd, data_buffer, 1, data_size);
            }
            else
            {
                memset(data_buffer, 0, data_size);
            }
        }
#else
        int nread = rw_read(mgnt->fd, data_buffer, 1, data_size);

        if (nread == 0)
        {
            // decrease keytone count and seek to start pos read again
            mgnt->count--;

            rw_seek(mgnt->fd, 0, SEEK_SET);

            if (mgnt->count > 0)
            {
                rw_read(mgnt->fd, data_buffer, 1, data_size);
            }
            else
            {
                memset(data_buffer, 0, data_size);
            }
        }
        else if (nread < data_size)
        {
            memset(&data_buffer[nread], 0, data_size - nread);
        }
#endif
    }
    else
    {
        memset(data_buffer, 0, data_size);
    }

    ret = data_size;

    KEYTONE_LOCK_OFF();

    return ret;
}

static int init_audio_player(keytone_mgnt_t *mgnt)
{

    wave_hdr_t wave_hdr;

    FILE *fp = NULL;

    int samples_pre_block = 0, stream_id = 0;

    fp = fopen(KEYPAD_INDEX, "rb");

    if (!fp)
    {
        LOGE("open %s fail \n", KEYPAD_INDEX);
        return -1;
    }

    if (!read_wave_info(&fp, &wave_hdr))
    {
        LOGE("%s read_wave_info fail \n", KEYPAD_INDEX);
        fclose(fp);
        return -1;
    }

    mgnt->data = bmalloc(wave_hdr.data_len);

    fread(mgnt->data, 1, wave_hdr.data_len, fp);

    mgnt->fd = rw_open_from_mem(mgnt->data, wave_hdr.data_len);

    fclose(fp);

    samples_pre_block = BLOCKTIME * wave_hdr.sample_rate / 1000;

    stream_id = Audio_Player_Open();

    Audio_Player_SetDevice(stream_id, "default");
    Audio_Player_SetFormat(stream_id, wave_hdr.channel, wave_hdr.bits_per_sample, wave_hdr.sample_rate);
    Audio_Player_SetCache(stream_id, PLAYBLOCKS, samples_pre_block);
    Audio_Player_SetMode(stream_id, false, audio_player_callback, mgnt);
    Audio_Player_SetVolume(stream_id, 100);
    Audio_Player_Start(stream_id);

    mgnt->stream_id = stream_id;

    return 0;
}

int keytone_mgnt_init(void)
{
    int ret = -1;

    if (!g_keytone_mgnt)
    {
        g_keytone_mgnt = (keytone_mgnt_t *)bzalloc(sizeof(keytone_mgnt_t));

        if (!g_keytone_mgnt)
            goto fail;

        ret = init_audio_player(g_keytone_mgnt);

        if (ret != 0)
            goto fail;

        ret = 0;
    }

    return ret;

fail:
    if (g_keytone_mgnt)
    {
        if (g_keytone_mgnt->stream_id != 0)
        {

            Audio_Player_Stop(g_keytone_mgnt->stream_id);
            Audio_Player_Close(g_keytone_mgnt->stream_id);

            g_keytone_mgnt->stream_id = 0;
        }

        if (g_keytone_mgnt->data)
        {
            bfree(g_keytone_mgnt->data);
            g_keytone_mgnt->data = NULL;
        }

        if (g_keytone_mgnt->fd)
        {
            rw_close(g_keytone_mgnt->fd);
            g_keytone_mgnt->fd = NULL;
        }

        bfree(g_keytone_mgnt);
        g_keytone_mgnt = NULL;
    }

    return -1;
}

void keytone_mgnt_deinit(void)
{
    if (g_keytone_mgnt)
    {
        if (g_keytone_mgnt->stream_id != 0)
        {
            Audio_Player_Stop(g_keytone_mgnt->stream_id);
            Audio_Player_Close(g_keytone_mgnt->stream_id);
            g_keytone_mgnt->stream_id = 0;
        }

        if (g_keytone_mgnt->data)
        {
            bfree(g_keytone_mgnt->data);
            g_keytone_mgnt->data = NULL;
        }

        if (g_keytone_mgnt->fd)
        {
            rw_close(g_keytone_mgnt->fd);
            g_keytone_mgnt->fd = NULL;
        }

        bfree(g_keytone_mgnt);
        g_keytone_mgnt = NULL;
    }
}

int play_keytone(int volume)
{
    int ret = -1;

    KEYTONE_LOCK_ON();

    if (g_keytone_mgnt && g_keytone_mgnt->stream_id != 0)
    {
        if (g_keytone_mgnt->stream_id != 0)
        {
            Audio_Player_SetVolume(g_keytone_mgnt->stream_id, volume);
        }
        if (g_keytone_mgnt->count == 0)
        {
            rw_seek(g_keytone_mgnt->fd, 0, SEEK_SET);
        }

        g_keytone_mgnt->count++;
    }
    else
    {
        sound_play_start(KEYPAD_INDEX, 1, 60, true);
    }

    KEYTONE_LOCK_OFF();

    return ret;
}