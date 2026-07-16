#ifndef __FH_COMMON_HDMI_H__
#define __FH_COMMON_HDMI_H__

#include "fh_type.h"
#include "fh_errno.h"
#include "fh_common.h"

#define FY_HDMI_MAX_AUDIO_CAP_COUNT 		16
/**the max audio smprate count*/
#define FY_HDMI_MAX_SAMPE_RATE_NUM		8
/**the max audio bit depth count*/
#define FY_HDMI_MAX_BIT_DEPTH_NUM		6

typedef enum {
	FY_HDMI_VIDEO_FMT_1080P_60 = 0,
	FY_HDMI_VIDEO_FMT_1080P_50,
	FY_HDMI_VIDEO_FMT_1080P_30,
	FY_HDMI_VIDEO_FMT_1080P_25,
	FY_HDMI_VIDEO_FMT_1080P_24,

	FY_HDMI_VIDEO_FMT_1080i_60,
	FY_HDMI_VIDEO_FMT_1080i_50,

	FY_HDMI_VIDEO_FMT_720P_60,
	FY_HDMI_VIDEO_FMT_720P_50,

	FY_HDMI_VIDEO_FMT_576P_50,
	FY_HDMI_VIDEO_FMT_480P_60,			/*10*/

	FY_HDMI_VIDEO_FMT_PAL,				/* B D G H I PAL */
	FY_HDMI_VIDEO_FMT_PAL_N,			/* (N)PAL */
	FY_HDMI_VIDEO_FMT_PAL_Nc,			/* (Nc)PAL */

	FY_HDMI_VIDEO_FMT_NTSC, 			/* (M)NTSC */
	FY_HDMI_VIDEO_FMT_NTSC_J,			/* NTSC-J */
	FY_HDMI_VIDEO_FMT_NTSC_PAL_M,			/* (M)PAL */

	FY_HDMI_VIDEO_FMT_SECAM_SIN,			/**< SECAM_SIN*/
	FY_HDMI_VIDEO_FMT_SECAM_COS,			/**< SECAM_COS*/

	FY_HDMI_VIDEO_FMT_861D_640X480_60,
	FY_HDMI_VIDEO_FMT_VESA_800X600_60,		/*20*/
	FY_HDMI_VIDEO_FMT_VESA_1024X768_60,
	FY_HDMI_VIDEO_FMT_VESA_1280X720_60,
	FY_HDMI_VIDEO_FMT_VESA_1280X800_60,
	FY_HDMI_VIDEO_FMT_VESA_1280X1024_60,
	FY_HDMI_VIDEO_FMT_VESA_1366X768_60,
	FY_HDMI_VIDEO_FMT_VESA_1440X900_60,
	FY_HDMI_VIDEO_FMT_VESA_1440X900_60_RB,
	FY_HDMI_VIDEO_FMT_VESA_1600X900_60_RB,
	FY_HDMI_VIDEO_FMT_VESA_1600X1200_60,
	FY_HDMI_VIDEO_FMT_VESA_1680X1050_60,		/*30*/
	FY_HDMI_VIDEO_FMT_VESA_1920X1080_60,
	FY_HDMI_VIDEO_FMT_VESA_1920X1200_60,
	FY_HDMI_VIDEO_FMT_VESA_2048X1152_60,		/* Currently, not support */

	FY_HDMI_VIDEO_FMT_2560x1440_30,
	FY_HDMI_VIDEO_FMT_2560x1440_60,
	FY_HDMI_VIDEO_FMT_2560x1600_60,
	FY_HDMI_VIDEO_FMT_1920x2160_30,

	FY_HDMI_VIDEO_FMT_3840X2160P_24,		/** 93:3840x2160p @ 24Hz	No Repetition */
	FY_HDMI_VIDEO_FMT_3840X2160P_25,		/** 94:3840x2160p @ 25Hz	No Repetition */
	FY_HDMI_VIDEO_FMT_3840X2160P_30,		/** 95:3840x2160p @ 30Hz	No Repetition */
	FY_HDMI_VIDEO_FMT_3840X2160P_50,		/** 96:3840x2160p @ 50Hz	No Repetition */
	FY_HDMI_VIDEO_FMT_3840X2160P_60,		/** 97:3840x2160p @ 60Hz	No Repetition */

	FY_HDMI_VIDEO_FMT_4096X2160P_24,		/** 98:4096x2160p @ 24Hz	No Repetition */
	FY_HDMI_VIDEO_FMT_4096X2160P_25,		/** 99:4096x2160p @ 25Hz	No Repetition */
	FY_HDMI_VIDEO_FMT_4096X2160P_30,		/** 100:4096x2160p @ 30Hz	No Repetition */
	FY_HDMI_VIDEO_FMT_4096X2160P_50,		/** 101:4096x2160p @ 50Hz	No Repetition */
	FY_HDMI_VIDEO_FMT_4096X2160P_60,		/** 102:4096x2160p @ 60Hz	No Repetition */
	FY_HDMI_VIDEO_FMT_VESA_CUSTOMER_DEFINE,		/** Currently, not support */

	FY_HDMI_VIDEO_FMT_BUTT
} FY_HDMI_VIDEO_FMT_E;

typedef enum {
	FY_HDMI_AUDIO_FORMAT_CODE_RESERVED	= 0x00,	/**<Audio coding type, refer stream,default type*/
	FY_HDMI_AUDIO_FORMAT_CODE_PCM,			/**<Audio coding PCM type*/
	FY_HDMI_AUDIO_FORMAT_CODE_AC3,			/**<Audio coding AC3 type*/
	FY_HDMI_AUDIO_FORMAT_CODE_MPEG1,		/**<Audio coding MPEG1 type*/
	FY_HDMI_AUDIO_FORMAT_CODE_MP3,			/**<Audio coding MP3 type*/
	FY_HDMI_AUDIO_FORMAT_CODE_MPEG2,		/**<Audio coding MPEG2 type*/
	FY_HDMI_AUDIO_FORMAT_CODE_AAC,			/**<Audio coding AAC type*/
	FY_HDMI_AUDIO_FORMAT_CODE_DTS,			/**<Audio coding DTS type*/
	FY_HDMI_AUDIO_FORMAT_CODE_ATRAC,		/**<Audio coding ATRAC type*/
	FY_HDMI_AUDIO_FORMAT_CODE_ONE_BIT,		/**<Audio coding ONE BIT AUDIO type*/
	FY_HDMI_AUDIO_FORMAT_CODE_DDP,			/**<Audio coding DDPLUS type*/
	FY_HDMI_AUDIO_FORMAT_CODE_DTS_HD,		/**<Audio coding DTS HD type*/
	FY_HDMI_AUDIO_FORMAT_CODE_MAT,			/**<Audio coding MAT type*/
	FY_HDMI_HDMI_AUDIO_FORMAT_CODE_DST,		/**<Audio coding DST type*/
	FY_HDMI_AUDIO_FORMAT_CODE_WMA_PRO,		/**<Audio coding WMA PRO type*/
	FY_HDMI_AUDIO_FORMAT_CODE_BUTT,
}FY_HDMI_AUDIO_FORMAT_CODE_E;

typedef enum {
	FY_HDMI_SAMPLE_RATE_UNKNOWN=0,			/**< unknown sample rate	*/
	FY_HDMI_SAMPLE_RATE_8K	  = 8000,		/**< 8K sample rate 	 */
	FY_HDMI_SAMPLE_RATE_11K   = 11025,		/**< 11.025K sample rate */
	FY_HDMI_SAMPLE_RATE_12K   = 12000,		/**< 12K sample rate	 */
	FY_HDMI_SAMPLE_RATE_16K   = 16000,		/**< 16K sample rate	 */
	FY_HDMI_SAMPLE_RATE_22K   = 22050,		/**< 22.050K sample rate */
	FY_HDMI_SAMPLE_RATE_24K   = 24000,		/**< 24K sample rate	 */
	FY_HDMI_SAMPLE_RATE_32K   = 32000,		/**< 32K sample rate	 */
	FY_HDMI_SAMPLE_RATE_44K   = 44100,		/**< 44.1K sample rate	 */
	FY_HDMI_SAMPLE_RATE_48K   = 48000,		/**< 48K sample rate	 */
	FY_HDMI_SAMPLE_RATE_88K   = 88200,		/**< 88.2K sample rate	 */
	FY_HDMI_SAMPLE_RATE_96K   = 96000,		/**< 96K sample rate	 */
	FY_HDMI_SAMPLE_RATE_176K  = 176400,		/**< 176K sample rate	 */
	FY_HDMI_SAMPLE_RATE_192K  = 192000,		/**< 192K sample rate	 */
	FY_HDMI_SAMPLE_RATE_BUTT,
}FY_HDMI_SAMPLE_RATE_E;

typedef enum {
	FY_HDMI_BIT_DEPTH_UNKNOWN =0,			/**< unknown bit width	*/
	FY_HDMI_BIT_DEPTH_8  = 8,			/**< 8 bits width  */
	FY_HDMI_BIT_DEPTH_16 = 16,			/**< 16 bits width */
	FY_HDMI_BIT_DEPTH_18 = 18,			/**< 18 bits width */
	FY_HDMI_BIT_DEPTH_20 = 20,			/**< 20 bits width */
	FY_HDMI_BIT_DEPTH_24 = 24,			/**< 24 bits width */
	FY_HDMI_BIT_DEPTH_32 = 32,			/**< 32 bits width */
	FY_HDMI_BIT_DEPTH_BUTT,
}FY_HDMI_BIT_DEPTH_E;

typedef enum {
	FY_HDMI_AUDIO_SPEAKER_FL_FR,
	FY_HDMI_AUDIO_SPEAKER_LFE,
	FY_HDMI_AUDIO_SPEAKER_FC,
	FY_HDMI_AUDIO_SPEAKER_RL_RR,
	FY_HDMI_AUDIO_SPEAKER_RC,
	FY_HDMI_AUDIO_SPEAKER_FLC_FRC,
	FY_HDMI_AUDIO_SPEAKER_RLC_RRC,
	FY_HDMI_AUDIO_SPEAKER_FLW_FRW,
	FY_HDMI_AUDIO_SPEAKER_FLH_FRH,
	FY_HDMI_AUDIO_SPEAKER_TC,
	FY_HDMI_AUDIO_SPEAKER_FCH,
	FY_HDMI_AUDIO_SPEAKER_BUTT,
} FY_HDMI_AUDIO_SPEAKER_E;

typedef struct {
	FH_UINT32			u32VFB;
	FH_UINT32			u32VBB;
	FH_UINT32			u32VACT;
	FH_UINT32			u32HFB;
	FH_UINT32			u32HBB;
	FH_UINT32			u32HACT;
	FH_UINT32			u32VPW;
	FH_UINT32			u32HPW;
	FH_UINT32			bIDV;
	FH_UINT32			bIHS;
	FH_UINT32			bIVS;
	FH_UINT32			u32ImageWidth;
	FH_UINT32			u32ImageHeight;
	FH_UINT32			u32AspectRationW;
	FH_UINT32			u32AspectRationH;
	FH_UINT32			u32Interlace;
	FH_UINT32			u32PixelClk;
} FY_HDMI_TIMING_INFO_S;

typedef struct {
	FH_UINT32			u32DetTimingNum;
	FY_HDMI_TIMING_INFO_S		astDetTiming[10];
}FY_HDMI_DET_TIMING_S;

typedef struct{
	FY_HDMI_AUDIO_FORMAT_CODE_E	enAudFmtCode;					/**<  Audio coding type*/
	FY_HDMI_SAMPLE_RATE_E		enSupportSampleRate[FY_HDMI_MAX_SAMPE_RATE_NUM];/**< Audio sample rate*/
	FH_UINT8			u8AudChannel;					/**< Aud Channel of the coding type*/
	FY_HDMI_BIT_DEPTH_E		bSupportBitDepth[FY_HDMI_MAX_BIT_DEPTH_NUM];	/**< sample bit depth,when audio format code is FY_UNF_EDID_AUDIO_FORMAT_CODE_PCM*/
	FH_UINT32			u32SupportBitDepthNum;				/**< sample bit depth Num,when audio format code is FY_UNF_EDID_AUDIO_FORMAT_CODE_PCM*/
	FH_UINT32			u32MaxBitRate;					/**< enter max bit rate,when audio format code is FY_UNF_EDID_AUDIO_FORMAT_CODE_AC3 - FY_UNF_EDID_AUDIO_FORMAT_CODE_ATRAC**/
}FY_HDMI_AUDIO_INFO_S;

typedef struct {
	FH_BOOL				bConnected;					/**< Whether the devices are connected*/
	FH_BOOL				bSupportHdmi;					/**< Whether the HDMI is supported by the device. If the HDMI is not supported by the device, the device is DVI*/
	FH_BOOL				bIsSinkPowerOn;					/**< Whether the sink device is powered on. */
	FH_BOOL				bIsRealEDID;					/**< Whether the EDID obtains the flag from the sink device.FY_TRUE: The EDID information is correctly read.FY_FASE: default settings*/
	FY_HDMI_VIDEO_FMT_E		enNativeVideoFormat;				/**< Physical resolution of the display device*/
	FH_BOOL				bVideoFmtSupported[FY_HDMI_VIDEO_FMT_BUTT];	/**< Video capability set.FY_TRUE: This display format is supported.FY_FALSE: This display format is not supported*/
	FH_BOOL				bSupportYCbCr;					/**< Whether the YCBCR display is supported.FY_TRUE: The YCBCR display is supported.FY_FALSE: Only red-green-blue (RGB) is supported*/
	FH_BOOL				bSupportxvYCC601;				/**< Whether the xvYCC601 color format is supported*/
	FH_BOOL				bSupportxvYCC709;				/**< Whether the xvYCC709 color format is supported*/
	FH_UINT8			u8MDBit;					/**< Transfer profile supported by xvYCC601. 1: P0; 2: P1; 4: P2*/
	FH_UINT32			u32AudioInfoNum;				/**< Num of audio Info*/
	FY_HDMI_AUDIO_INFO_S		stAudioInfo[FY_HDMI_MAX_AUDIO_CAP_COUNT];	/**< Audio Info.For details, see Table 37 in EIA-CEA-861-D*/
	FH_BOOL				bSpeaker[FY_HDMI_AUDIO_SPEAKER_BUTT];		/**< Speaker position. For details, see the definition of SpeakerDATABlock in EIA-CEA-861-D*/
	FH_UINT8			u8IDManufactureName[4];				/**< Device vendor flag*/
	FH_UINT32			u32IDProductCode;				/**< Device ID.*/
	FH_UINT32			u32IDSerialNumber;				/**< Device sequence number*/
	FH_UINT32			u32WeekOfManufacture;				/**< Device production data (week)*/
	FH_UINT32			u32YearOfManufacture;				/**< Set the production data (year)*/
	FH_UINT8			u8Version;					/**< Device version number*/
	FH_UINT8			u8Revision;					/**< Device sub version number*/
	FH_UINT8			u8EDIDExternBlockNum;				/**< EDID extended block number*/
	FH_BOOL				bIsPhyAddrValid;				/**< Valid flag of the consumer electronics control (CEC) physical address*/
	FH_UINT8			u8PhyAddr_A;					/**< CEC physical address A*/
	FH_UINT8			u8PhyAddr_B;					/**< CEC physical address B*/
	FH_UINT8			u8PhyAddr_C;					/**< CEC physical address C*/
	FH_UINT8			u8PhyAddr_D;					/**< CEC physical address D*/
	FH_BOOL				bSupportDVIDual;				/**< Whether to support the DVI dual-link operation*/
	FH_BOOL				bSupportDeepColorYCBCR444;			/**< Whether to support the YCBCR 4:4:4 deep-color mode*/
	FH_BOOL				bSupportDeepColor30Bit;				/**< Whether to support the deep-color 30-bit mode*/
	FH_BOOL				bSupportDeepColor36Bit;				/**< Whether to support the deep-color 36-bit mode*/
	FH_BOOL				bSupportDeepColor48Bit;				/**< Whether to support the deep-color 48-bit mode*/
	FH_BOOL				bSupportAI;					/**< Whether to support the Supports_AI mode*/
	FH_UINT32			u32MaxTMDSClock;				/**< Maximum TMDS clock*/
	FH_BOOL				bI_Latency_Fields_Present;			/**< Delay flag bit*/
	FH_BOOL				bLatency_Fields_Present;			/**< whether Video_Latency and Audio_Latency fields are present*/
	FH_BOOL				bHDMI_Video_Present;				/**< Special video format*/
	FH_UINT8			u8Video_Latency;				/**< Video delay*/
	FH_UINT8			u8Audio_Latency;				/**< Audio delay*/
	FH_UINT8			u8Interlaced_Video_Latency;			/**< Video delay in interlaced video mode*/
	FH_UINT8			u8Interlaced_Audio_Latency;			/**< Audio delay in interlaced video mode*/
	FH_BOOL				bSupportY420DC30Bit;				/**< Whether to support the YUV420 deep-color 30-bit mode*/
	FH_BOOL				bSupportY420DC36Bit;				/**< Whether to support the YUV420 deep-color 36-bit mode*/
	FH_BOOL				bSupportY420DC48Bit;				/**< Whether to support the YUV420 deep-color 48-bit mode*/
	FH_BOOL				bSupportHdmi_2_0;				/**< Whether to support HDMI2.0*/
	FH_BOOL				bSupportY420Format[FY_HDMI_VIDEO_FMT_BUTT];	/**< the format support YUV420*/
	FH_BOOL				bOnlySupportY420Format[FY_HDMI_VIDEO_FMT_BUTT]; /**< the format only support YUV420*/
	FY_HDMI_DET_TIMING_S		stDetailedTiming;				/**< Detailed timing infomation*/
} VO_HDMI_SINK_S;
#endif /* End of #ifndef __FY_COMMON_HDMI_H__ */
