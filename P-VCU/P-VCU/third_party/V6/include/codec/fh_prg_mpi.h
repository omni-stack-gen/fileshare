#ifndef __FH_PRG_MPI_H__
#define __FH_PRG_MPI_H__

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "fh_system_mpi.h"
#include "vmm_api.h"
#include "fh_vb_mpi.h"
#include <camif/fh_camif_mpi.h>
#include <fh_venc_mpipara.h>
#include <dsp/fh_jpege_mpi.h>
#include <dsp/fh_jpege_mpipara.h>

#define RPC_CMD_ID                      (0xC0000000L)
#define RPC_CUSTOMER_CMD_ID(cmdid)      ((FH_SINT32)( (RPC_CMD_ID) | ((FH_ID_APP) << 16 ) | (cmdid) ))
#define RPC_CUSTOMER_CMD_GETMMZ             RPC_CUSTOMER_CMD_ID(1)
#define RPC_CUSTOMER_CMD_RLSMMZ             RPC_CUSTOMER_CMD_ID(2)
#define RPC_CUSTOMER_CMD_SNS_ONOFF          RPC_CUSTOMER_CMD_ID(3)
#define RPC_CUSTOMER_CMD_MEDIA_START        RPC_CUSTOMER_CMD_ID(4)
#define RPC_CUSTOMER_CMD_MEDIA_STOP         RPC_CUSTOMER_CMD_ID(5)
#define RPC_CUSTOMER_CMD_SET_PRG_SENSOR     RPC_CUSTOMER_CMD_ID(6)

typedef enum {
	PRG_SENSOR_POWER_ALWAYS_ON = 1,
	PRG_SENSOR_POWER_WKUP_STANDBY,
	PRG_SENSOR_POWER_SLEEP,
	PRG_SENSOR_POWER_OFF,
	PRG_SENSOR_POWER_PD_STANDBY
} PRG_SENSOR_POWER_MODE_E;

typedef struct prg_system_info_s {
    /**
     * 编码模式：
     *   - 0: Reserved
     *   - 1: RawEc
     *   - 2: JPEG
     *   - 3: H264
     */
    unsigned int pic_enc_mode;
    unsigned int main_sensor_preconfig;     //!< 保留（冷启动时，是否提前配置主sensor）
    unsigned int pic_continue_out;          //!< 唤醒启动过程中，是否继续预存图像
    unsigned int pic_abort;                 //!< 唤醒主系统时，是否中止当前帧
    /**
     * Sensor模式：
     *   - 0: Reserved
     *   - 1: Always ON
     *   - 2: HW standby
     *   - 3: SW Standby
     *   - 4: Power Off
     */
    unsigned int sensor_power_mode;
    unsigned int prg_power_mode;            //!< 保留字段
    unsigned int standby_time;              //!< 定时器周期，Sensor在非Always ON模式时使用
    unsigned int pwr_on_wait_time;          //!< sensor重新上电后等待sensor初始化的时间
    /**
     * Pre-Rolling server控制信息：BIT0，UART log标志，
     *    - 0: RAWEC编码时，log信息保存到IRAM；
     *    - 1: RAWEC编码时，log信息通过串口输出
     */

    unsigned int flag;
}PRG_SYSTEM_INFO_S;

typedef struct prg_config_dsp_info
{
    Camif_Port_Type_E port_type;
    unsigned int prg_addr;
    unsigned int prg_size; 
    char image_filepath[64];                //pre-rolling image filepath
    void* camif_sensor_config;              //camif_sensor_config
    unsigned int camif_sensor_config_len;   //camif_sensor_config length
    PRG_SYSTEM_INFO_S prg_cfg;
}PRG_CONFIG_DSP_INFO_S;

typedef struct prg_config_isp_info
{
    int ae_enable;
    int awb_enable;
    int blc_enable;
    char hex_filepath[64];         //hex filepath
}PRG_CONFIG_ISP_INFO_S;

/***************
@name:   fh_prg_mpi_set_dsp_info
@brief:  用来设置dsp相关参数，然后进入休眠，开启pre-rolling
@param:
PRG_CONFIG_DSP_INFO_S* dsp_info

@return 0:success others: fail
***************/
int fh_prg_mpi_set_dsp_info(PRG_CONFIG_DSP_INFO_S* dsp_info);

/***************
@name:   fh_prg_mpi_set_isp_info
@brief:  用来设置isp相关参数,必须在prg_config_set_dsp_info前调用
@param:
PRG_CONFIG_ISP_INFO_S* isp_info

@return 0:success others: fail
***************/

int fh_prg_mpi_set_isp_info(PRG_CONFIG_ISP_INFO_S* isp_info);


/***************
@name:   fh_prg_mpi_get_image_count
@brief:  获取pre-rolling录制帧数
@param:
unsigned int addr     pre-rolling预录数据buffer地址
unsigned int size     pre-rolling预录数据buffer size

@return 小于0: 获取失败  大于等于0: 实际帧数
***************/

int fh_prg_mpi_get_image_count(unsigned int addr, unsigned int size);

/***************
@name:   fh_prg_mpi_dump_image
@brief:  dump pre_rolling的预录数据到程序运行的当前路径
@param:
unsigned int addr     pre-rolling预录数据buffer地址
unsigned int size     pre-rolling预录数据buffer size

@return 非0: 失败  等于0: 成功
***************/

int fh_prg_mpi_dump_image(unsigned int addr, unsigned int size);

#endif

