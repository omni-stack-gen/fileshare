#ifndef H26X_AGG_H
#define H26X_AGG_H

#include <stdint.h>
#include <stdbool.h>

#include "resize_buf.h"

// Codec ID 定义
#define H26X_CODEC_H264 0
#define H26X_CODEC_HEVC 1

// H264 NALU 类型
enum
{
	H264_NAL_SLICE     = 1,
	H264_NAL_IDR_SLICE = 5,
	H264_NAL_SEI       = 6,
	H264_NAL_SPS       = 7,
	H264_NAL_PPS       = 8,
};

// H265 NALU 类型
enum
{
	H265_NAL_SLICE_TRAIL_N = 0,
	H265_NAL_VPS = 32,
	H265_NAL_SPS = 33,
	H265_NAL_PPS = 34,
};

// 状态掩码 (记录当前 Buffer 收集到的 NALU 种类)
#define H26X_BIT_SLICE 0x01
#define H26X_BIT_IDR   0x02
#define H26X_BIT_PPS   0x04
#define H26X_BIT_SPS   0x08
#define H26X_BIT_VPS   0x10
#define H26X_BIT_SEI   0x20

// 提取 NALU 类型的宏 (传入的是去除 Start Code 后的 NALU Header 字节)
#define H264_NAL_TYPE(header)  ((header) & 0x1F)
#define H265_NAL_TYPE(header)  (((header) & 0x7F) >> 1)

typedef struct h26x_agg
{
	int              codec_id;

	resize_buf_t     in_rb;     // 内部用于零碎拼接的 Buffer
	resize_buf_t     out_rb;    // 用于向外输出完整帧的 Buffer (外部只读，无需释放)

	int64_t          pts;

	int              frame_bits;
	bool             frame_corrupted;
	bool             wait_keyframe;

	bool             output_corrupted;  // 是否允许输出残缺帧/花屏帧

	uint8_t          *extradata;
	int              extradata_size;
} h26x_agg_t;

/**
 * @brief 初始化拼帧器
 */
void h26x_agg_init(h26x_agg_t *agg, int codec_id, bool output_corrupted, const uint8_t *extra, int extra_size);

/**
 * @brief 销毁拼帧器并释放内存
 */
void h26x_agg_deinit(h26x_agg_t *agg);

/**
 * @brief 核心处理函数 (完全通用化，不依赖任何特定框架)
 *
 * @param agg             拼帧器上下文
 * @param data            输入的 NALU 数据 (必须包含 00 00 01 或 00 00 00 01 起始码)
 * @param size            数据长度
 * @param pts             时间戳
 * @param is_corrupted    当前这块数据是否发生了网络丢包截断
 * @param is_end_of_frame 是否是当前帧的最后一块数据 (如 RTP 的 Marker Bit)
 * @param out_frame       输出: 完整的帧数据指针 (生命周期维持到下一次调用该函数前)
 * @param out_size        输出: 完整的帧数据长度
 * @param out_pts         输出: 完整的帧时间戳
 * @return true           拼装成功，有一帧完整数据输出
 * @return false          正在拼装中，或者丢弃了无效数据，暂无输出
 */
bool h26x_agg_process_nalu(h26x_agg_t *agg,
                           const uint8_t *data, int size, int64_t pts,
                           bool is_corrupted, bool is_end_of_frame,
                           uint8_t **out_frame, int *out_size, int64_t *out_pts);

#endif // H26X_AGG_H