/*
* Copyright (C) 2020-2022 Artinchip Technology Co. Ltd
*
*  author: <qi.xu@artinchip.com>
*  Desc: mpp_dec_type
*/

#ifndef MPP_DEC_TYPE_H
#define MPP_DEC_TYPE_H

#include <stdint.h>
#include <stddef.h>
// the header file is in dir linux/include/uapi/video
#include <video/mpp_types.h>

#ifndef u8
	typedef uint8_t		u8;
#endif
#ifndef u16
	typedef uint16_t	u16;
#endif
#ifndef u32
	typedef uint32_t	u32;
#endif
#ifndef u64
	typedef uint64_t	u64;
#endif

#ifndef s8
	typedef int8_t		s8;
#endif
#ifndef s16
	typedef int16_t		s16;
#endif
#ifndef s32
	typedef int32_t		s32;
#endif
#ifndef s64
	typedef int64_t		s64;
#endif

/* frame flag */
#define FRAME_FLAG_EOS (0x00000001)

/* packet flag */
#define PACKET_FLAG_EOS		(0x00000001)
#define PACKET_FLAG_EXTRA_DATA	(0x00000002)

/**
 * struct mpp_packet - mpp packet buffer
 * @data: mpp packet virtual address
 * @size: mpp packet buffer size
 * @pts: pts of packet
 * @flags: buffer flags
 */
struct mpp_packet {
	void *data;
	int size;
	long long pts;
	unsigned int flag;
};

enum mpp_codec_type {
	MPP_CODEC_VIDEO_DECODER_H264 = 0x1000,         // decoder
	MPP_CODEC_VIDEO_DECODER_MJPEG,
	MPP_CODEC_VIDEO_DECODER_PNG,

	MPP_CODEC_VIDEO_ENCODER_H264 = 0x2000,         // encoder
};

enum mpp_dec_cmd {
	MPP_DEC_INIT_CMD_SET_BYTESTRESAM_BUFFER_SIZE,        // bytestream buffer size
	MPP_DEC_INIT_CMD_SET_PACKET_COUNT,                   // packet count
	MPP_DEC_INIT_CMD_SET_EXTRA_FRAME_COUNT,              // extra frame count
	MPP_DEC_INIT_CMD_SET_OUTPUT_PIXEL_FORMAT,            // output pixel format
	MPP_DEC_INIT_CMD_SET_EXT_FRAME_ALLOCATOR,            // frame buffer allocator
};

enum mpp_dec_errno {
	// decode
	DEC_ERR_NOT_SUPPORT 			= 0x90000001,

	// if mpp_dec_get_packet return DEC_ERR_NO_EMPTY_PACKET, we should wait a minute then call again
	// it happen in send bitstream fast than decode
	DEC_ERR_NO_EMPTY_PACKET			= 0x90000002, // no packet in empty list

	// if decode return DEC_ERR_NO_READY_PACKET, we should wait a minute then call again
	// it happen in decode faster than send bitstream
	DEC_ERR_NO_READY_PACKET			= 0x90000003, //

	// if decode return DEC_ERR_NO_EMPTY_FRAME, we should wait a minute then call again
	// it happen in decode faster than render
	DEC_ERR_NO_EMPTY_FRAME 			= 0x90000004, //

	// if mpp_dec_get_frame return DEC_ERR_NO_RENDER_FRAME, we should wait a minute then call again
	// it happen in render faster than decode
	DEC_ERR_NO_RENDER_FRAME 		= 0x90000005, //

	DEC_ERR_NULL_PTR			= 0x90000006,

	// if frame manager not create, mpp_dec_get_frame return DEC_ERR_FM_NOT_CREATE.
	// app should wait a minute to get frame
	DEC_ERR_FM_NOT_CREATE			= 0x90000006,
};

#endif
