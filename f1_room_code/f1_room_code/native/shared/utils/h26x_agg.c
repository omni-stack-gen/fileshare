#include "h26x_agg.h"

#include <stdlib.h>
#include <string.h>

#include <utils/bmem.h>

#define LOG_TAG "h26x_agg"
#include <utils/log.h>

void h26x_agg_init(h26x_agg_t *agg, int codec_id, bool output_corrupted, const uint8_t *extra, int extra_size)
{
	memset(agg, 0, sizeof(h26x_agg_t));
	agg->codec_id = codec_id;

	resize_buf_init(&agg->in_rb, 1024 * 128);
	resize_buf_init(&agg->out_rb, 1024 * 256);

	agg->wait_keyframe = true;
	agg->output_corrupted = output_corrupted;

	if (extra && extra_size > 0)
	{
		agg->extradata = (uint8_t *)bmalloc(extra_size);
		memcpy(agg->extradata, extra, extra_size);
		agg->extradata_size = extra_size;
	}
}

void h26x_agg_deinit(h26x_agg_t *agg)
{
	resize_buf_free(&agg->in_rb);
	resize_buf_free(&agg->out_rb);

	if (agg->extradata)
	{
		bfree(agg->extradata);
	}

	memset(agg, 0, sizeof(h26x_agg_t));
}

static void h26x_agg_reset_frame(h26x_agg_t *agg)
{
	agg->in_rb.size = 0;
	agg->frame_bits = 0;
	agg->frame_corrupted = false;
}

static const uint8_t *find_startcode_internal(const uint8_t *p, const uint8_t *end)
{
	const uint8_t *a = p + 4 - ((intptr_t)p & 3);

	for (end -= 3; p < a && p < end; p++)
	{
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
		{
			return p;
		}
	}

	for (end -= 3; p < end; p += 4)
	{
		uint32_t x = *(const uint32_t *)p;

		if ((x - 0x01010101) & (~x) & 0x80808080)
		{
			if (p[1] == 0)
			{
				if (p[0] == 0 && p[2] == 1)
				{
					return p;
				}

				if (p[2] == 0 && p[3] == 1)
				{
					return p + 1;
				}
			}

			if (p[3] == 0)
			{
				if (p[2] == 0 && p[4] == 1)
				{
					return p + 2;
				}

				if (p[4] == 0 && p[5] == 1)
				{
					return p + 3;
				}
			}
		}
	}

	for (end += 3; p < end; p++)
	{
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
		{
			return p;
		}
	}

	return end + 3;
}

static const uint8_t *find_startcode(const uint8_t *p, const uint8_t *end)
{
	const uint8_t *out = find_startcode_internal(p, end);

	if (p < out && out < end && !out[-1])
	{
		out--;
	}

	return out;
}

bool h26x_agg_process_nalu(h26x_agg_t *agg,
                           const uint8_t *data, int size, int64_t pts,
                           bool is_corrupted, bool is_eof,
                           uint8_t **out_frame, int *out_size, int64_t *out_pts)
{
	if (!data || size < 4)
	{
		return false;
	}

	bool has_output = false;

	// =========================================================
	// 1. 全量扫描当前块的 NALU 属性
	// =========================================================
	bool is_idr = false;
	bool is_slice = false;
	bool is_param = false;

	const uint8_t *nal_start, *nal_end;
	const uint8_t *end = data + size;

	nal_start = find_startcode(data, end);
	nal_end = NULL;

	while (nal_end != end)
	{
		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
		{
			break;
		}

		if (agg->codec_id == H26X_CODEC_H264)
		{
			int nalu_type = H264_NAL_TYPE(nal_start[0]);

			if (nalu_type == H264_NAL_SPS || nalu_type == H264_NAL_PPS)
			{
				is_param = true;
			}
			else if (nalu_type == H264_NAL_IDR_SLICE)
			{
				is_idr = true;
			}
			else if (nalu_type >= 1 && nalu_type <= 4)
			{
				is_slice = true;
			}
		}
		else
		{
			int nalu_type = H265_NAL_TYPE(nal_start[0]);

			if (nalu_type == H265_NAL_VPS || nalu_type == H265_NAL_SPS || nalu_type == H265_NAL_PPS)
			{
				is_param = true;
			}
			else if (nalu_type >= 16 && nalu_type <= 21)
			{
				is_idr = true;
			}
			else if (nalu_type >= 0 && nalu_type <= 15)
			{
				is_slice = true;
			}
		}

		if (is_slice || is_idr)
		{
			break;
		}

		nal_end = find_startcode(nal_start, end);

		if (!nal_end)
		{
			nal_end = end;
		}

		nal_start = nal_end;
	}

	// =========================================================
	// 2. 隐式边界探测 (PTS 跳变结算)
	// =========================================================
	if (agg->in_rb.size > 0 && pts != agg->pts)
	{
		bool has_idr = (agg->frame_bits & H26X_BIT_IDR) != 0;
		bool has_slice = (agg->frame_bits & H26X_BIT_SLICE) != 0;

		// 核心修复：输出判定必须结合 wait_keyframe 状态！
		// 只有包含关键帧，或者 (包含普通P帧 且 我们没有在等待关键帧) 时，才是合法输出。
		bool valid_to_output = has_idr || (has_slice && !agg->wait_keyframe);

		// LOGI("Implicit Boundary: old_pts=%lld, new_pts=%lld, "
		//       "frame_bits=0x%x, wait_keyframe=%d, valid_to_output=%d\n",
		//       (long long)agg->pts, (long long)pts, agg->frame_bits, agg->wait_keyframe,
		// 	  valid_to_output);

		if (valid_to_output)
		{
			if (!agg->frame_corrupted || agg->output_corrupted)
			{
				if (agg->frame_corrupted)
				{
					LOGW("Frame corrupted, but outputting it anyway for error concealment (Smearing).\n");
				}

				resize_buf_copy(&agg->out_rb, agg->in_rb.buf, agg->in_rb.size);
				*out_frame = agg->out_rb.buf;
				*out_size  = agg->out_rb.size;
				*out_pts   = agg->pts;
				has_output = true;

				if (has_idr)
				{
					agg->wait_keyframe = false;
				}
			}
			else
			{
				LOGW("Frame corrupted, dropping it.\n");
				// 如果丢弃了残缺帧，说明参考链断裂，必须重新等待关键帧
				agg->wait_keyframe = true;
			}


			// 结算完毕，清空缓冲迎接新帧
			h26x_agg_reset_frame(agg);
		}
		else
		{
			if (has_slice)
			{
				// 说明这是在 wait_keyframe 状态下缓冲到的孤立 P 帧，直接丢弃，绝不能顺延！
				LOGW("Invalid isolated P-frame while waiting for I-frame. Resetting buffer!\n");
				h26x_agg_reset_frame(agg);
			}
			else
			{
				// 只有 SPS/PPS 等参数集，不包含 Slice，将 PTS 顺延吸附到新一帧头部
				// LOGD("Frame only contains params (SPS/PPS). Extending PTS to %lld.\n", (long long)pts);
				agg->pts = pts;
			}
		}
	}

	// =========================================================
	// 3. 终极门神：等待关键帧时的【绝对隔离】
	// =========================================================
	if (agg->wait_keyframe)
	{
		if (is_slice && !is_idr)
		{
			return has_output;
		}

		if (!is_idr && !is_param)
		{
			return has_output;
		}
	}

	// =========================================================
	// 4. 记录当前帧状态
	// =========================================================
	if (is_idr)
	{
		agg->frame_bits |= H26X_BIT_IDR;
	}

	if (is_slice)
	{
		agg->frame_bits |= H26X_BIT_SLICE;
	}

	if (is_param)
	{
		agg->frame_bits |= H26X_BIT_SPS;
	}

	if (is_corrupted)
	{
		agg->frame_corrupted = true;
	}

	// =========================================================
	// 5. 首帧强行注入 ExtraData
	// =========================================================
	if (is_idr && !(agg->frame_bits & H26X_BIT_SPS) && agg->extradata_size > 0)
	{
		if (agg->in_rb.size == 0) // 确保注入到绝对开头
		{
			resize_buf_append(&agg->in_rb, agg->extradata, agg->extradata_size);
			agg->frame_bits |= H26X_BIT_SPS;
		}
	}

	// =========================================================
	// 6. 追加数据
	// =========================================================
	if (agg->in_rb.size == 0)
	{
		agg->pts = pts;
	}

	resize_buf_append(&agg->in_rb, data, size);

	// =========================================================
	// 7. 显式边界结算 (is_eof)
	// =========================================================
	if (is_eof && !has_output)
	{
		// 如果不允许输出残缺帧，执行拦截和丢弃
		if (agg->frame_corrupted && !agg->output_corrupted)
		{
			agg->wait_keyframe = true;
			h26x_agg_reset_frame(agg);
			return false;
		}

		// 同理，显式边界处也采用收紧后的安全判定条件
		bool has_idr = (agg->frame_bits & H26X_BIT_IDR) != 0;
		bool has_slice = (agg->frame_bits & H26X_BIT_SLICE) != 0;
		bool valid_to_output = has_idr || (has_slice && !agg->wait_keyframe);

		if (valid_to_output)
		{
			if (agg->frame_corrupted)
			{
				LOGW("Outputting corrupted EOF frame (Smearing).\n");
			}

			resize_buf_copy(&agg->out_rb, agg->in_rb.buf, agg->in_rb.size);
			*out_frame = agg->out_rb.buf;
			*out_size  = agg->out_rb.size;
			*out_pts   = agg->pts;
			has_output = true;

			if (has_idr)
			{
				agg->wait_keyframe = false;
			}

			h26x_agg_reset_frame(agg);
		}
	}

	return has_output;
}
