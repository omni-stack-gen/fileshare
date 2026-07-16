#ifndef __FY_DEFINES_H__
#define __FY_DEFINES_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#define MAX_VMM_NAME_LEN 32
#define MMAX_MMB_NAME_LEN 16

/* For JPEG */
#define JPEGE_MAX_WIDTH   4096
#define JPEGE_MAX_HEIGHT  3072
#define JPEGE_MAX_CHN_NUM  8

/* For VDEC */
#define VDEC_MAX_CHN_NUM        16
#define MAX_VDEC_CHN            VDEC_MAX_CHN_NUM

/* for VGS */
#define VGS_MAX_CHN_NUM     8
#define VGS_MAX_PTH_NUM     3

/* for VOU  */
#define VO_MAX_DEV_NUM                  2                   /* max dev num */
#define VO_MAX_LAYER_NUM                3                   /* max layer num */
#define VHD_MAX_CHN_NUM                 16                  /* max VHD chn num */
#define VO_MAX_CHN_NUM                  VHD_MAX_CHN_NUM     /* max chn num */
#define VO_MAX_GRAPHICS_LAYER_NUM       4                   /* max gfx layer num */
#define VO_MAX_WBC_NUM                  1                   /* max wbc dev num */

/* for VPSS */
#define VPSS_MAX_WIDTH              4096
#define VPSS_MAX_HEIGHT             2160
#define VPSS_MAX_GRP_NUM            8
#define VPSS_MAX_PHY_CHN_NUM        5
#define VPSS_MAX_EXT_CHN_NUM        5
#define VPSS_MAX_CHN_NUM            (VPSS_MAX_PHY_CHN_NUM + VPSS_MAX_EXT_CHN_NUM)

/* For VENC */
#define VENC_MAX_CHN_NUM    (8)
#define VENC_MIN_WIDTH      (64)
#define VENC_MIN_HEIGTH     (64)
#define VENC_MAX_WIDTH      (4096)
#define VENC_MAX_HEIGTH     (4096)
#define SEI_MAX_SIZE_BYTE	(65536)

/* For Venc + Jpege*/
#define ENC_MAX_CHN_NUM    (VENC_MAX_CHN_NUM + 8)




/* VB size calculate for compressed frame.
	[param input]
		w: 	width
		h: 	height
		fmt:	pixel format, 0: SP420, 1: SP422
		z:	compress mode, 0: no compress, 1: default compress
	[param output]
		size: vb blk size

 */
#define VB_W_ALIGN		16
#define VB_H_ALIGN		32
#define VB_ALIGN(x, a)	((a) * (((x) + (a) - 1) / (a)))

#define VDU_ALIGN4(_x)              (((_x)+0x03)&~0x03)
#define VDU_ALIGN8(_x)              (((_x)+0x07)&~0x07)
#define VDU_ALIGN16(_x)             (((_x)+0x0f)&~0x0f)
#define VDU_ALIGN32(_x)             (((_x)+0x1f)&~0x1f)
#define VDU_ALIGN64(_x)             (((_x)+0x3f)&~0x3f)
#define VDU_ALIGN128(_x)            (((_x)+0x7f)&~0x7f)
#define VDU_ALIGN256(_x)            (((_x)+0xff)&~0xff)
#define VDU_ALIGN512(_x)            (((_x)+0x1ff)&~0x1ff)
#define VDU_ALIGN1024(_x)           (((_x)+0x3ff)&~0x3ff)
#define VDU_ALIGN2048(_x)           (((_x)+0x7ff)&~0x7ff)
#define VDU_ALIGN4096(_x)           (((_x)+0xfff)&~0xfff)
#define VDU_ALIGN16384(_x)          (((_x)+0x3fff)&~0x3fff)

/* VB size calculate for compressed frame.
	[param input]
		w: 	width
		h: 	height
		fmt:	pixel format, 0: SP420, 1: SP422
		z:	compress mode, 0: no compress, 1: default compress
	[param output]
		size: vb blk size
 */
#define VB_HEADER_STRIDE    16

#define VB_PIC_Y_HEADER_SIZE(Width, Height, size)\
	do{\
            size = VB_HEADER_STRIDE * (Height);\
	}while(0)

#define VB_PIC_HEADER_SIZE(Width, Height, Type, size)\
	do{\
            if (PIXEL_FMT_YUV_SEMIPLANAR_422 == Type)\
            {\
		        size = VB_HEADER_STRIDE * (Height) * 2;\
            }\
            else\
            {\
                size = (VB_HEADER_STRIDE * (Height) * 3) >> 1;\
            }\
	}while(0)


#define VB_PIC_BLK_SIZE(Width, Height, Type, size)\
	do{\
			unsigned int u32AlignWidth;\
			unsigned int u32AlignHeight;\
			unsigned int __mvSize = 0;\
			if (Type==PT_H264 || Type==PT_H265) \
			{\
				u32AlignWidth = VDU_ALIGN64(Width);\
				u32AlignHeight= VDU_ALIGN64(Height);\
				size = (u32AlignWidth * u32AlignHeight * 3 )>> 1;\
				if(Type==PT_H264) \
					__mvSize  =	0; \
				else if(Type==PT_H265) \
					__mvSize  =	((size*5)>>6); \
				__mvSize = VDU_ALIGN64(__mvSize); \
				size +=  __mvSize;\
			}\
			else\
			{\
				u32AlignWidth = VDU_ALIGN16(Width);\
				u32AlignHeight= VDU_ALIGN16(Height);\
				size = (u32AlignWidth * u32AlignHeight * 3) >> 1;\
			}\
			size = VDU_ALIGN4096(size); \
	}while(0)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __FY_DEFINES_H__ */

