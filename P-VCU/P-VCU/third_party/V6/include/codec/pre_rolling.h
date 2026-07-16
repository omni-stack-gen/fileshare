#ifndef __PRE_ROLLING_H__
#define __PRE_ROLLING_H__
#include <isp460/isp_common.h>

#define MAX_AE_INFO_NODE_NUM            (20)
#define MAX_AWB_INFO_NODE_NUM           (20)
#define MAX_CTRL_INFO_NODE_NUM          (20)
#define PPS_INFO_NUM                    (20)

#define PRG_FLAG_RAWEC_ULOG             (BIT(0))

typedef enum {
	SENSOR_POWER_ALWAYS_ON = 1,
	SENSOR_POWER_WKUP_STANDBY,
	SENSOR_POWER_SLEEP,
	SENSOR_POWER_OFF,
	SENSOR_POWER_PD_STANDBY
} SENSOR_POWER_MODE_E;

typedef enum {
	PRG_NORMAL_MODE = 0,
	PRG_TUNNING_MODE,
	PRG_INVALID_WORK_MODE
} PRG_WORK_MODE;

/*
 * @pic_enc_mode, picture encode mode in pre_rolling system.
 *     =0, Raw, output RawData.
 *     =1, no encode, output YData directly.
 *     =2, JPEG, output jpeg picture.
 *     =3, H264, output h264 picture.
 * @frame_rate, frame rate to share memory in pre-rolling system.
 * @pic_continue_out, indicate whether continue output picture while
 *     pre_rolling exit.
 * @pic_abort, indicate whether PIR event abot pre_rolling process.
 * @main_sensor_preconfig, incidate whether to pre-config main sensor.
 * @sensor_power_mode.
 *     =0, always on.
 *     =1, HW standby.
 *     =2, SW standby.
 *     =3, power off.
 * @standby_time, sensor standby/sleep/power off mode delay time between each frame.
 * @pwr_on_wait_time, sensor power off mode delay time between power on and I2C config
 * @flag, debug flag
 */
struct prg_system_info {
	/**
	 * 编码模式：
	 *   - 0: Reserved
	 *   - 1: RawEc
	 *   - 2: JPEG
	 *   - 3: H264
	 */
	unsigned int pic_enc_mode;
	unsigned int main_sensor_preconfig;     //!< 保留（冷启动时，是否提前配置主sensor）
	unsigned int pic_continue_out;			//!< 唤醒启动过程中，是否继续预存图像
	unsigned int pic_abort;					//!< 唤醒主系统时，是否中止当前帧
	/**
	 * Sensor模式：
	 *   - 0: Reserved
	 *   - 1: Always ON
	 *   - 2: HW standby
	 *   - 3: SW Standby
	 *   - 4: Power Off
	 */
	unsigned int sensor_power_mode;
	unsigned int prg_power_mode;			//!< 保留字段
	unsigned int standby_time;				//!< 定时器周期，Sensor在非Always ON模式时使用
	unsigned int pwr_on_wait_time;			//!< sensor重新上电后等待sensor初始化的时间
	/**
	 * Pre-Rolling server控制信息：BIT0，UART log标志，
	 *    - 0: RAWEC编码时，log信息保存到IRAM；
	 *    - 1: RAWEC编码时，log信息通过串口输出
	 */

	unsigned int flag;
	//unsigned int frame_rate;
};

/*
 * @pic_width/pic_height, picture width & height
 * @phy_base/buf_size, the DDR memory space to store picture data & control information.
 * @pre_data_offset/len, picture data during pre-rolling.
 * @pre_ctrl_offset/len, control information during pre-rolling.
 * @pre_ctrl_num, control information number during pre-rolling.
 * @extra_data_len, picture data during the whole system boot. base is pys_base.
 * @extra_ctrl_offset/len, control information during system boot.
 * @extra_ctrl_number, control information number during system boot.
 */
struct prg_picture_info {
	unsigned int pic_width;				//!< 图像宽度
	unsigned int pic_height;			//!< 图像高度
	unsigned int phy_base;              //!< 图像缓存区的物理起始地址
	unsigned int buf_size;              //!< 图像缓存区大小
	unsigned int pre_data_offset;       //!< 预存的图像数据偏移
	unsigned int pre_data_len;          //!< 预存的图像数据大小
	unsigned int pre_ctrl_offset;       //!< 预存图像的控制信息偏移
	unsigned int pre_ctrl_len;          //!< 预存图像的控制信息大小
	unsigned int pre_ctrl_num;          //!< 预存图像的控制信息数量

	unsigned int ae_ctrl_offset;        //!< 预存图像的AE信息偏移
	unsigned int ae_ctrl_num;           //!< 预存图像的AE信息数量
	unsigned int awb_ctrl_offset;       //!< 预存图像的AWB信息偏移
	unsigned int awb_ctrl_num;          //!< 预存图像的AWB信息数量

	unsigned int extra_data_offset;     //!< 唤醒启动过程中，预存图像数据的偏移
	unsigned int extra_data_len;        //!< 唤醒启动过程中，预存图像数据的大小
	unsigned int extra_ctrl_offset;     //!< 唤醒启动过程中，预存图像的控制信息偏移
	unsigned int extra_ctrl_len;        //!< 唤醒启动过程中，预存图像的控制信息大小
	unsigned int extra_ctrl_num;        //!< 唤醒启动过程中，预存图像的控制信息数量
	unsigned int extra_ae_ctrl_offset;  //!< 唤醒启动过程中，预存图像的AE信息偏移
	unsigned int extra_ae_ctrl_num;     //!< 唤醒启动过程中，预存图像的AE信息数量
	unsigned int extra_awb_ctrl_offset; //!< 唤醒启动过程中，预存图像的AWB信息偏移
	unsigned int extra_awb_ctrl_num;    //!< 唤醒启动过程中，预存图像的AWB信息数量
	};

/*
 * @frame_id, picture id
 * @frame_addr, picture phy addr
 * @frame_size, picture real size
 * @pps, rawec picture pps info, This will be left blank when h264 and jpeg mode
 */
typedef struct {
	unsigned int frame_id;			//!< 帧ID
	unsigned int frame_addr;		//!< 当前帧所在的位置（物理地址）
	unsigned int frame_size;		//!< 当前帧的数据大小
	unsigned int PPS_0;				//!< 仅Raw EC模式有效，Camif RawEC压缩配置
	unsigned int PPS_1;				//!< 仅Raw EC模式有效，Camif RawEC压缩配置
	unsigned int PPS_2;				//!< 仅Raw EC模式有效，Camif RawEC压缩配置
	unsigned int PPS_3;				//!< 仅Raw EC模式有效，Camif RawEC压缩配置
	unsigned int PPS_4;				//!< 仅Raw EC模式有效，Camif RawEC压缩配置
	unsigned int PPS_5;             //!< 仅Raw EC模式有效，Camif RawEC压缩配置
	unsigned int PPS_6;             //!< 仅Raw EC模式有效，Camif RawEC压缩配置
	unsigned int PPS_7;             //!< 仅Raw EC模式有效，Camif RawEC压缩配置
	unsigned int PPS_8;				//!< 仅Raw EC模式有效，Camif RawEC压缩配置
	unsigned int PPS_9;             //!< 仅Raw EC模式有效，Camif RawEC压缩配置
	unsigned int PPS_10;            //!< 仅Raw EC模式有效，Camif RawEC压缩配置
	unsigned int PPS_11;            //!< 仅Raw EC模式有效，Camif RawEC压缩配置
	unsigned int PPS_12;            //!< 仅Raw EC模式有效，Camif RawEC压缩配置
	unsigned int PPS_13;            //!< 仅Raw EC模式有效，Camif RawEC压缩配置
	unsigned int PPS_14;            //!< 仅Raw EC模式有效，Camif RawEC压缩配置
	unsigned int PPS_15;            //!< 仅Raw EC模式有效，Camif RawEC压缩配置
	unsigned int STAT_0;			//!< 仅Raw EC模式有效，Camif RawEC编码状态
	unsigned int STAT_1;			//!< 仅Raw EC模式有效，Camif RawEC编码状态
	unsigned int STAT_2;			//!< 仅Raw EC模式有效，Camif RawEC编码状态
	unsigned int STAT_3;			//!< 仅Raw EC模式有效，Camif RawEC编码状态
	unsigned int time_stamp_hight;	//!< 当前帧的时间戳高32bit
	unsigned int time_stamp_low;	//!< 当前帧的时间戳低32bit
}Prg_Pps_Info_Node_S;

/*
 * @info
 * @valid, picture valid flag.
 *     =1, picture is complete.
 *     =0, picture is not complete.
 */
struct prg_ctrl_info_node {
	Prg_Pps_Info_Node_S info;      	//!< 帧信息
	unsigned int valid;				//!< 帧信息有效标志：0: 无效; 1:有效

};

typedef struct {
	unsigned int rGain;			//<! 红色像素增益。
	unsigned int gGain;         //<! 绿色像素增益。
	unsigned int bGain;         //<! 蓝色像数增益。
	unsigned int CT;            //<! 色温。
	unsigned int framId;        //<! 帧ID。
}awb_info_t;

typedef struct {
	unsigned int exposureTime;  //<! 曝光事件。
	unsigned int aGain;         //<! 模拟增益。
	unsigned int dGain;         //<! 数字增益。
	unsigned int EV;            //<! 曝光值。
	unsigned int framId;        //<! 帧ID。
}ae_info_t;

struct prg_buffer {
	unsigned int base;			//!< 起始地址
	unsigned int size;			//!< 大小
};

struct prg_image_info {
	unsigned int phy_base;  //!< 起始地址
	unsigned int size;      //!< 大小
};

struct prg_sensor_info {
	Sensor_Param sensor;
	void *hex_virt;
	unsigned int hex_len;
};

struct ram_log_header {
	unsigned int magic_number;
	unsigned int loop_cnt;
	unsigned int top;
	unsigned int reserved;
};

/**
 * @brief      根据指定的模式，配置Pre-Rolling系统参数。
 *
 * @param[in]  mode  0:正常模式, >0:调试模式
 * @param      info  Pre-Rolling系统配置参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败。
 */
int pre_rolling_init(int mode, struct prg_system_info *info);


/**
 * @brief      释放Pre-Rolling占用的系统资源，并将系统参数复制到IRAM空间。
 *
 * @param[in]  mode  0:正常模式, >0:调试模式
 *
 * @retval  0              成功
 * @retval  "非0"          失败。
 *
 */
int pre_rolling_deinit(int mode);

/**
 * @brief      获取Pre-Rolling server预存的图像信息
 *
 * @param      image  接收Pre-Rolling图像帧信息的地址
 *
 * @retval  0              成功
 * @retval  "非0"          失败。
 */
int pre_rolling_get_frame(struct prg_picture_info *image);

/**
 * @brief      获取Pre-Rolling系统参数
 *
 * @param      info   接收Pre-Rolling系统参数的地址
 *
 * @retval  0              成功
 * @retval  "非0"          失败。
 */
int pre_rolling_get_system_info(struct prg_system_info *info);

/**
 * @brief      配置Pre-Rolling系统参数
 *
 * @param      info  接收Pre-Rolling系统参数的地址
 *
 * @retval  0              成功
 * @retval  "非0"          失败。
 */
int pre_rolling_set_system_info(struct prg_system_info *info);

/**
 * @brief      获取Pre-Rolling Server执行空间信息
 *
 * @param      image  镜像执行空间信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败。
 */
int pre_rolling_get_image_info(struct prg_image_info *image);

/**
 * @brief      设置Sensor参数
 *
 * @param      sensor  Sensor parameter，详细信息
 * @param      hex     Sensor参数
 * @param[in]  len     Sensor参数大小
 *
 * @retval  0              成功
 * @retval  "非0"          失败。
 */
int pre_rolling_set_sensor_info(Sensor_Param *sensor, void *hex, unsigned int len);

/**
 * @brief      获取Pre-Rolling server执行状态
 *
 * @param      state  Pre-Rolling server执行结果
 *
 * @retval  0              成功
 * @retval  "非0"          失败。
 */
int pre_rolling_get_prg_state(unsigned int *state);

/**
 * @brief      设置VENC编码器PingPong  Buffer
 *
 * @retval  0              成功
 * @retval  "非0"          失败。
 */
int pre_rolling_set_ppbuf(void);

/**
 * @brief      启动Pre-Rolling server程序
 *
 * @retval  0              成功
 * @retval  "非0"          失败。
 */
int pre_rolling_start(void);

/**
 * @brief      设置DDR里哪片空间是pre rolling可以用
 *
 * @param      buffer  The buffer
 *
 * @retval  0              成功
 * @retval  "非0"          失败。
 */
int pre_rolling_buffer_set(struct prg_buffer *buffer);

#endif
