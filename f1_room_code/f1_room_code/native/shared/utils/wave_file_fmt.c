#include <stdio.h>
#include <stdint.h>

#include "wave_file_fmt.h"

#include <utils/util.h>
#include <utils/file_helper.h>

#define LOG_TAG "wav_file_fmt"
#include <utils/log.h>

int wav_writer_open(wav_writer_t *writer, const char *file_name, wave_fmt_tag_t fmt_tag,
                    int sample_rate, int num_channels)
{
	size_t size;

	wave_hdr_t header = {0};

	memset(writer, 0, sizeof(wav_writer_t));

	writer->file = fopen(file_name, "wb");

	if (!writer->file)
	{
		LOGE("Failed to open WAV file [%s] \n", file_name);
		return -1;
	}

	writer->data_size = 0;
	writer->sample_rate = sample_rate;
	writer->num_channels = num_channels;
	writer->fmt_tag = fmt_tag;

	if (fmt_tag != WAVE_FMT_TAG_PCM)
	{
		writer->bytes_per_sample = 1;
	}
	else
	{
		writer->bytes_per_sample = 2;
	}

	safe_strncpy(writer->file_name, file_name, sizeof(writer->file_name));

	header.riff = RIFF_TAG;
	header.file_len = 0;
	header.wave = WAVE_TAG;

	// fmt 块
	header.fmt = FMT_TAG;
	header.fmt_len = 16;
	header.fmt_tag = writer->fmt_tag;
	header.channel = writer->num_channels;
	header.sample_rate = writer->sample_rate;
	header.bits_per_sample = writer->bytes_per_sample * 8;
	header.bytes_per_sec = writer->sample_rate * writer->num_channels * writer->bytes_per_sample;
	header.block_align = writer->num_channels * writer->bytes_per_sample;

	// data 块
	header.data = DATA_TAG;
	header.data_len = 0;

	if (fmt_tag != WAVE_FMT_TAG_PCM)
	{
		wave_subchunk_t fact_chunk;
		uint32_t tmp = 0;

		fact_chunk.id = FACT_TAG;
		fact_chunk.len = 4;

		/* Write WAVE header without DATA chunk header */
		size = sizeof(wave_hdr_t) - WAVE_DATA_HDR_SIZE;
		fwrite(&header, 1, size, writer->file);

		/* Write FACT chunk if it stores compressed data */
		size = sizeof(fact_chunk);
		fwrite(&fact_chunk, 1, size, writer->file);

		size = 4;
		fwrite(&tmp, 1, size, writer->file);

		/* Write DATA chunk header */
		size = WAVE_DATA_HDR_SIZE;
		fwrite(&header.data, 1, size, writer->file);
	}
	else
	{
		size = sizeof(wave_hdr_t);

		fwrite(&header, 1, size, writer->file);
	}

	return 0;
}

int wav_writer_write(wav_writer_t *writer, const void *data, size_t bytes)
{
	if (!writer->file)
	{
		return -1;
	}

	size_t written = fwrite(data, 1, bytes, writer->file);

	if (written != bytes)
	{
		LOGE("Failed to write WAV data to [%s] \n", writer->file_name);
		return -1;
	}

	writer->data_size += written;

	return 0;
}

int wav_writer_close(wav_writer_t *writer)
{
	enum { FILE_LEN_POS = 4, DATA_LEN_POS = 40 };

	long file_size;
	ssize_t bytes;
	uint32_t wave_file_len;
	uint32_t wave_data_len;
	uint32_t data_len_pos = DATA_LEN_POS;

	if (!writer->file)
	{
		return -1;
	}

	file_size = ftell(writer->file);

	/* Calculate wave fields */
	wave_file_len = (uint32_t)(file_size - 8);
	wave_data_len = (uint32_t)(file_size - sizeof(wave_hdr_t));

	fseek(writer->file, FILE_LEN_POS, SEEK_SET);

	/* Write file_len */
	bytes = sizeof(wave_file_len);
	fwrite(&wave_file_len, 1, bytes, writer->file);

	/* Write samples_len in FACT chunk */
	if (writer->fmt_tag != WAVE_FMT_TAG_PCM)
	{
		enum { SAMPLES_LEN_POS = 44};
		uint32_t wav_samples_len;

		/* Adjust wave_data_len & data_len_pos since there is FACT chunk */
		wave_data_len -= 12;
		data_len_pos += 12;
		wav_samples_len = wave_data_len;

		/* Seek to samples_len field. */
		fseek(writer->file, SAMPLES_LEN_POS, SEEK_SET);

		/* Write samples_len */
		bytes = sizeof(wav_samples_len);
		fwrite(&wav_samples_len, 1, bytes, writer->file);
	}

	/* Seek to data_len field. */
	fseek(writer->file, data_len_pos, SEEK_SET);

	/* Write file_len */
	bytes = sizeof(wave_data_len);
	fwrite(&wave_data_len, 1, bytes, writer->file);

	fclose(writer->file);
	writer->file = NULL;

	LOGI("WAV file [%s] saved. Total samples: %u\n", writer->file_name, writer->data_size / (writer->num_channels * 2));

	return 0;
}

static int read_wav_until(FILE *fp, uint32_t id, wave_subchunk_t *chunk)
{
	for (;;)
	{
		wave_subchunk_t subchunk;
		ssize_t size_read = 8;

		size_read = fread(&subchunk, 1, sizeof(wave_subchunk_t), fp);

		if (size_read != sizeof(wave_subchunk_t))
		{
			return -1;
		}

		*chunk = subchunk;

		/* Break if this is chunk that contains the desired ID */
		if (subchunk.id == id)
		{
			break;
		}

		if (0 != fseek(fp, subchunk.len, SEEK_CUR))
		{
			return -1;
		}
	}

	return 0;
}

// 打开WAV文件
int wav_reader_open(wav_reader_t *reader, const char *file_name)
{
	int status = 0;

	int file_size = 0;

	wave_hdr_t header;
	wave_subchunk_t chunk;

	uint32_t size_read = 0, size_to_read = 0;

	memset(&header, 0, sizeof(header));
	memset(&chunk, 0, sizeof(chunk));
	memset(reader, 0, sizeof(wav_reader_t));

	file_size = file_length(file_name);

	if (file_size <= (int)(sizeof(wave_hdr_t)))
	{
		LOGE("[%s] Not a valid wave file \n", file_name);
		return -1;
	}

	reader->file = fopen(file_name, "rb");

	if (!reader->file)
	{
		LOGE("Error opening file: %s\n", file_name);
		goto __exit;
	}

	/* Read the RIFF file header only. */
	size_read = WAVE_RIFF_HDR_SIZE;

	if (size_read != fread(&header, 1, size_read, reader->file))
	{
		LOGE("Error reading WAV riff chunk\n");
		goto __exit;
	}

	if (header.riff != RIFF_TAG ||
	    header.wave != WAVE_TAG)
	{
		LOGE("actual value|expected riff=%x|%x, wave=%x|%x \n",
		     header.riff, RIFF_TAG, header.wave, WAVE_TAG);
		goto __exit;
	}

	/* Read the WAVE file until we find 'fmt ' chunk. */
	status = read_wav_until(reader->file, FMT_TAG, &chunk);

	if (status != 0)
	{
		LOGE("Error reading WAVE fmt chunk \n");
		goto __exit;
	}

	memcpy(&header.fmt, &chunk, sizeof(chunk));

	/* Read the rest of `fmt ` chunk. */
	size_read = WAVE_FMT_HDR_SIZE - sizeof(chunk);

	if (size_read != fread(&header.fmt_tag, 1, size_read, reader->file))
	{
		LOGE("Error reading WAV header\n");
		goto __exit;
	}

	/* Validate format and its attributes (i.e: bits per sample, block align) */
	switch (header.fmt_tag)
	{
		case WAVE_FMT_TAG_PCM:
			{
				if (header.bits_per_sample != 16 ||
				    header.block_align != (2 * header.channel))
				{
					status = -1;
				}
			}
			break;

		case WAVE_FMT_TAG_ALAW:
		case WAVE_FMT_TAG_ULAW:
			{
				if (header.bits_per_sample != 8 ||
				    header.block_align != header.channel)
				{
					status = -1;
				}
			}
			break;

		default:
			{
				LOGE("unsupport wav format tag :%d \n", header.fmt_tag);

				status = -1;
			}
			break;
	}

	if (status != 0)
	{
		goto __exit;
	}

	/* If length of fmt_header is greater than 16, skip the remaining
	 * fmt header data.
	 */
	if (header.fmt_len > 16)
	{
		size_to_read = header.fmt_len - 16;

		fseek(reader->file, size_to_read, SEEK_CUR);
	}

	/* Read the WAVE file until we find 'data ' chunk */
	status = read_wav_until(reader->file, DATA_TAG, &chunk);

	if (status != 0)
	{
		LOGE("Error reading WAVE data chunk \n");
		goto __exit;
	}

	memcpy(&header.data, &chunk, sizeof(chunk));

	reader->fmt_tag = header.fmt_tag;
	reader->channel = header.channel;
	reader->sample_rate = header.sample_rate;
	reader->bits_per_sample = header.bits_per_sample;
	reader->data_len = header.data_len;

	reader->file_size = file_size;
	reader->data_start = ftell(reader->file);
	reader->data_position = 0;

	LOGI("wav file info tag [%d] %d Hz, %d channels, %d bits, %.3f seconds, datalen %d\n",
	     header.fmt_tag, header.sample_rate, header.channel, header.bits_per_sample,
	     (float)(header.data_len / header.block_align) / header.sample_rate, header.data_len);

	return 0;

__exit:

	if (reader->file)
	{
		fclose(reader->file);
		reader->file = NULL;
	}

	return -1;
}

// 读取音频数据
size_t wav_reader_read(wav_reader_t *reader, void *buffer, size_t buffer_size)
{
	if (!reader->file)
	{
		return 0;
	}

	if (buffer_size == 0)
	{
		return 0;
	}

	// 计算可读取的最大字节数
	size_t max_bytes_to_read = reader->data_len - reader->data_position;

	if (max_bytes_to_read == 0)
	{
		return 0;
	}

	// 限制读取大小为缓冲区大小
	size_t bytes_to_read = max_bytes_to_read < buffer_size ? max_bytes_to_read : buffer_size;

	// 定位到当前位置
	fseek(reader->file, reader->data_start + reader->data_position, SEEK_SET);

	// 读取原始数据
	size_t bytes_read = fread(buffer, 1, bytes_to_read, reader->file);

	// 更新位置
	reader->data_position += bytes_read;

	return bytes_read;
}

// 跳转到指定位置
int wav_reader_seek(wav_reader_t *reader, uint32_t position)
{
	if (!reader->file)
	{
		return -1;
	}

	if (position > reader->data_len)
	{
		position = reader->data_len;
	}

	reader->data_position = position;

	return 0;
}

// 关闭WAV文件
void wav_reader_close(wav_reader_t *reader)
{
	if (reader->file)
	{
		fclose(reader->file);
		reader->file = NULL;
	}
}