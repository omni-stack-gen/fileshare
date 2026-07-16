#ifndef MYCONFIG_H
#define MYCONFIG_H

/*
一键：P-VCU-D1
两键：P-VCU-D2
四键：P-VCU-D4
四键：P-VCU-C0（支持夜视、微波人感）

V1.1版：1个按键：0.164V
        2个按键：0.3V
        4个按键：0.415V
*/
#ifdef ENABLE_V1
		#define MODEL_NAME "P-VCU-A0"
#elif ENABLE_V6
        #define MODEL_NAME "P-VCU-C0"
#endif


#define PLC_DOOR_P_VCU_D1   1       //一键
#define PLC_DOOR_P_VCU_D2   2       //两键
#define PLC_DOOR_P_VCU_D4   4       //四键

/*OTA升级相关结构体 */
typedef  struct _upgrade_info
{
    int     updatetype;
    char    softCo[32];
    char    filemd5[64];
    char    softver[32];
    int     havereg;
    int     endiantype;
} upgrade_info;

//获取PLC设备类型(通过ADC判断的)
int get_plc_dev_type(void);

#endif