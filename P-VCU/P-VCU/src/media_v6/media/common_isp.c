#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "dsp_ext/FH_typedef.h"
#include "fh_vb_mpipara.h"
#include "sample_common.h"
#include "dsp/fh_system_mpi.h"
#include "dsp/fh_vb_mpi.h"
#include "dsp/fh_vpu_mpi.h"
#include "dsp/fh_venc_mpi.h"
#include "vicap/fh_vicap_mpi.h"

#include "dsp/fh_vpu_mpi.h"
#include "isp/isp_common.h"
#include "isp/isp_api.h"
#include "isp/isp_enum.h"
#include "dsp_ext/FHAdv_Isp_mpi_v3.h"
#include "sample_common_isp.h"

static int isp_type = 0;
void ReLoadDayNightParamByMyEnc(unsigned int isDay);
void ReLoadDayNightParam(unsigned int isDay)
{
#if 0 // 建议不使用WDR的， 只有高光、阴影对比明显才适用；一般只有车载，晚上看车大灯才会启动WDR，WDR 本来有蒙蒙的，补光更蒙
    isp_type = isDay == 1 ? 0 : 1;
    ReLoadDayNightParamByMyEnc(isDay);
#endif
}

void let_isp_type(void)
{
    //isp_type = 1 - isp_type;
#if 1
    system("echo 1 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio1/direction");
    system("echo 0 > /sys/class/gpio/gpio1/value");
    usleep(20*1000);
    system("echo 1 > /sys/class/gpio/gpio1/value");
#endif
}


#define ISP_FORMAT_G0      (isp_type == 1 ? FORMAT_WDR_400WP25 : FORMAT_400WP25) //FORMAT_WDR_400WP25 FORMAT_400WP25
#define VI_INPUT_WIDTH_G0  2560
#define VI_INPUT_HEIGHT_G0 1440

FH_SINT32 isp_get_wdr_mode(FH_SINT32 format)
{
    FH_SINT32 wdr_mode;

    wdr_mode = (format & 0x10000) >> 16;

    return wdr_mode;
}

static FH_SINT32 choose_i2c(FH_UINT32 grpid)
{
    switch (grpid)
    {
    case 0:
	//return 0;
//#ifdef CIS0_I2C3
#ifdef FH_APP_CSI0_I2C_NUM
      return FH_APP_CSI0_I2C_NUM;
#else
      return 2;
#endif
    case 1:
        return 1;
    case 2:
        return 0;
    default:
        break;
    }
    return 0;
}

void vicap_mipi_init(int grpid, int mipi)
{
    FH_VICAP_DEV_ATTR_S stDevAttr;
    FH_VICAP_CROP_CFG_S stCropCfg;
    FH_VICAP_VI_ATTR_S stViAttr;
    FH_VICAP_PEAKLOAD_INFO_S peakload = {0};

	memset(&peakload, 0, sizeof(peakload));
	peakload.sizeFpsInfo[0].width = CH0_WIDTH_G0;
	peakload.sizeFpsInfo[0].height = CH0_HEIGHT_G0;
	peakload.sizeFpsInfo[0].fps = 30;
	peakload.sizeFpsInfo[0].pclk = 600000000;
	peakload.sizeFpsInfo[1].width = 0;
	peakload.sizeFpsInfo[1].height = 0;
	peakload.sizeFpsInfo[1].fps = 0;
	peakload.sizeFpsInfo[2].width = 0;
	peakload.sizeFpsInfo[2].height = 0;
	peakload.sizeFpsInfo[2].fps = 0;
	peakload.pclkSet = 0;

	if(isp_get_wdr_mode(ISP_FORMAT_G0))
		peakload.sizeFpsInfo[0].fps *=2;
	FH_VICAP_SetPeakLoad(&peakload);


    stDevAttr.bUsingVb = FH_FALSE;
    stDevAttr.enDataTypeIn = VICAP_DATA_IN_RAW12;

    stDevAttr.enWorkMode = VICAP_WORK_MODE_ONLINE;//VICAP_WORK_MODE_ONLINE;//VICAP_WORK_MODE_OFFLINE;

    stDevAttr.stSize.u16Width = VI_INPUT_WIDTH_G0;
    stDevAttr.stSize.u16Height = VI_INPUT_HEIGHT_G0;
    FH_VICAP_InitViDev(mipi, &stDevAttr);




    stCropCfg.bCutEnable = FH_FALSE;
    FH_VICAP_SetPipeCrop(mipi, VICAP_LFRAME, &stCropCfg);

    stViAttr.enBayerType = VICAP_BAYER_RGGB;

    stViAttr.enWorkMode = VICAP_WORK_MODE_ONLINE;//VICAP_WORK_MODE_ONLINE;//VICAP_WORK_MODE_OFFLINE;

    stViAttr.u8WdrMode = 0;
    if(isp_get_wdr_mode(ISP_FORMAT_G0))
        stViAttr.u8WdrMode = 1;
    stViAttr.stInSize.u16Width = VI_INPUT_WIDTH_G0;
    stViAttr.stInSize.u16Height = VI_INPUT_HEIGHT_G0;
    stViAttr.stOfflineCfg.u8Priority = 0;
    stViAttr.stCropSize.bCutEnable = FH_FALSE;
    stViAttr.stCropSize.stRect.u16X = 0;
    stViAttr.stCropSize.stRect.u16Y = 0;
    stViAttr.stCropSize.stRect.u16Width = 0;
    stViAttr.stCropSize.stRect.u16Height = 0;
    FH_VICAP_SetViAttr(mipi, &stViAttr);

    return;
}

#define SENSOR_CREATE_MULTI(n) Sensor_Create##_##n
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(gc4653_mipi)();
#if !defined(ENABLE_AOV)
struct isp_sensor_if *start_sensor(char *SensorName, int grpid)
{
    int ret;
    struct isp_sensor_if *sensor = NULL;
    //sensor = SENSOR_CREATE_MULTI(gc4653_mipi);
    sensor = Sensor_Create_gc4653_mipi();
    //if (0/*sensor && grpid == FH_APP_GRP_ID*/)
    if (sensor && grpid == FH_APP_GRP_ID)
    {
        ret = FHAdv_Isp_SensorInit(grpid, sensor);
        if (ret)
        {
            printf("Error: FHAdv_Isp_SensorInit failed, ret = %d\n", ret);
        }
    }

    if(sensor == NULL)
        printf("sensor %s not support yet!\n", SensorName? SensorName :"4653");

    return sensor;
}
#endif
void LoadDayNightParam(int u32Id, unsigned int isDay)
{
    //int wdr_mode = isp_get_wdr_mode(u32Id);
    //char filename[64] = {'\0'};
    char *fileName = "./hex/gc4653_mipi_attr.hex";//"./hex/ovos02k_mipi_wdr_attr.hex"
    ISP_PARAM_CONFIG param;


    // if(!strcmp(g_isp_info[u32Id].sensor_name, "gc4653_mipi")){
    //     fileName = "./hex/gc4653_mipi_attr.hex";
    // }
    if (access(fileName, F_OK))
    {
        fileName = "./gc4653_mipi_attr.hex";
        if (access(fileName, F_OK))
        {
            fileName = "/system/hex/gc4653_mipi_attr.hex";
        }
    }

    if(isp_get_wdr_mode(ISP_FORMAT_G0))
    {
        printf("isp_get_wdr_mode = %d\n", isp_get_wdr_mode(ISP_FORMAT_G0));
        fileName = "/system/hex/gc4653_mipi_wdr_attr.hex";
        if (access(fileName, F_OK))
            fileName = "./gc4653_mipi_wdr_attr.hex";
    }

    //GetSensorHexName(isDay,wdr_mode, filename, u32Id);
    FILE *param_file;
    API_ISP_GetBinAddr(u32Id, &param);
    param_file = fopen(fileName,"rb");
    if(param_file == NULL)
    {
        LOG_PRT("open file failed! %s\n", fileName);
        return ;
    }
    LOG_PRT("[irc] open file! %s\n", fileName);
    LOG_PRT("API_ISP_GetBinAddr size = %d isp_grp = %d\n", param.u32BinSize, u32Id);

    char *isp_param_buff = malloc(sizeof(unsigned int)*param.u32BinSize);
    fread(isp_param_buff,1,param.u32BinSize,param_file);
    fclose(param_file);
    API_ISP_LoadIspParam(u32Id, isp_param_buff);
    free(isp_param_buff);
    isp_param_buff = NULL;
}



static FH_SINT32 sample_isp_init(FH_UINT32 grpid)
{
    FH_SINT32 ret;
    ISP_MEM_INIT stMemInit = {0};
    Sensor_Init_t initConf = {0};
    ISP_VI_ATTR_S sensor_vi_attr = {0};

    LOG_PRT("grpid = %d\n",grpid);

	FH_SINT32 csi = 0;//choose_csi(grpid);

	vicap_mipi_init(grpid, csi);
    //printf("start API_ISP_MemInit, isp_max_width = %d, isp_max_height = %d\n", g_isp_info[grpid].isp_max_width, g_isp_info[grpid].isp_max_height);

    stMemInit.enOfflineWorkMode = ISP_OFFLINE_MODE_DISABLE;

    stMemInit.stPicConf.u32Width = CH0_WIDTH_G0;
    stMemInit.stPicConf.u32Height = CH0_HEIGHT_G0;
    stMemInit.enLut2dWorkMode = ISP_LUT2D_BYPASS;

    printf("enOfflineWorkMode:%d u32Width:%d u32Height:%d enLut2dWorkMode:%d\n", stMemInit.enOfflineWorkMode, stMemInit.stPicConf.u32Width, stMemInit.stPicConf.u32Height, stMemInit.enLut2dWorkMode);
    // if(stMemInit.enLut2dWorkMode == ISP_LUT2D_OFFLINE)
    // {
    //     stMemInit.enIspOutMode = ISP_OUT_TO_DDR;
    // }
    // else
    {
        stMemInit.enIspOutMode = ISP_OUT_TO_VPU;
    }

    ret = API_ISP_MemInit(grpid, &stMemInit);
    if (ret)
    {
        LOG_PRT("Error(%d - %x): API_ISP_MemInit (grpid):(%d)!\n", ret, ret, grpid);
        return ret;
    }

    //只初始化一次就行
    static struct isp_sensor_if *psensor = NULL;
#if 0
    g_isp_info[grpid].sensor = start_sensor(g_isp_info[grpid].sensor_name, grpid);
    psensor = g_isp_info[grpid].sensor;
#else
    if(!psensor)
    {
#ifdef ENABLE_AOV
    //create sensor 需要再arc端实现该函数
    #define SENSOR_NAME "gc4653_mipi"
    psensor = Sensor_Create(SENSOR_NAME);
#else
    psensor = start_sensor("gc4653_mipi", grpid);
#endif
    }
#endif
    if (!psensor)
    {
        LOG_PRT("Error(%d - %x): start_sensor (grpid):(%d)!\n", ret, ret, grpid);
        return -1;
    }

    ret = API_ISP_SensorRegCb(grpid, 0, psensor);
    if (ret)
    {
        LOG_PRT("Error(%d - %x): API_ISP_SensorRegCb (grpid):(%d)!\n", ret, ret, grpid);
        return ret;
    }

    initConf.u8CsiDeviceId = csi;
    initConf.u8CciDeviceId = choose_i2c(grpid);
// #if defined DUAL_CAMERA_DEMO
//     initConf.bGrpSync = FH_TRUE; //拼接需要同步
// #else
    initConf.bGrpSync = FH_FALSE;
// #endif
	LOG_PRT("start API_ISP_SensorInit. i2c %d-%d\r\n", initConf.u8CsiDeviceId, initConf.u8CciDeviceId);
    ret = API_ISP_SensorInit(grpid, &initConf);
    if (ret)
    {
        LOG_PRT("Error(%d - %x): API_ISP_SensorInit (grpid):(%d)!\n", ret, ret, grpid);
        return ret;
    }

    // 下载配置到sensor
	//LOG_PRT("start API_ISP_SetSensorFmt: 0x%x\n", g_isp_info[grpid].isp_format);

    ret = API_ISP_SetSensorFmt(grpid, ISP_FORMAT_G0);
    if (ret)
    {
        LOG_PRT("Error(%d - %x): API_ISP_SetSensorFmt (grpid):(%d)!\n", ret, ret, grpid);
        return ret;
    }

    // if(!strcmp(g_isp_info[grpid].sensor_name, "imx415_mipi"))
    // {
    //     printf("imx415_mipi set csi :%d\n", csi);
    //     if(csi==0)
    //        FH_SYS_SetReg(0x10300268, 0x3);
    //     else if(csi==1)
    //         FH_SYS_SetReg(0x10300264, 0x3);
    // }

    // 启动sensor输出
     LOG_PRT("start API_ISP_SensorKick\r\n");
    ret = API_ISP_SensorKick(grpid);
    if (ret)
    {
        LOG_PRT("Error(%d - %x): API_ISP_SensorKick (grpid):(%d)!\n", ret, ret, grpid);
        return ret;
    }

    // 初始化ISP硬件寄存器
    LOG_PRT("start API_ISP_Init\r\n");
    ret = API_ISP_Init(grpid);
    if (ret)
    {
        LOG_PRT("Error(%d - %x): API_ISP_Init (grpid):(%d)!\n", ret, ret, grpid);
        return ret;
    }

    // 3 根据实际使用，配置vicap是离线模式还是在线模式
     LOG_PRT("start API_ISP_GetViAttr\r\n");
    ret = API_ISP_GetViAttr(grpid, &sensor_vi_attr);
    if (ret)
    {
        LOG_PRT("Error(%d - %x): API_ISP_GetViAttr (grpid):(%d)!\n", ret, ret, grpid);
        return ret;
    }


// #ifdef CONFIG_VICAP_OFFLINE_MODE
//     src.obj_id = FH_OBJ_VICAP;
//     src.dev_id = grpid;
//     src.chn_id = 0;
//     dst.obj_id = FH_OBJ_ISP;
//     dst.dev_id = grpid;
//     dst.chn_id = 0;

//     ret = FH_SYS_Bind(src, dst);
//     if (ret)
//     {
//         LOG_PRT("Error(%d - %x): FH_SYS_BindVicap2Isp (grpid):(%d)!\n", ret, ret, grpid);
//         return -1;
//     }
//     LOG_PRT("VICAP[%d] bind to ISP[%d] success\n", grpid, grpid);
// #endif
    LoadDayNightParam(grpid, 1);


// #if defined(FH_APP_USING_IRCUT_G0) || defined(FH_APP_USING_IRCUT_G1) || defined(FH_APP_USING_IRCUT_G2)
//     sample_SmartIR_init(g_isp_info[grpid].sensor->name, grpid);
// #endif

    return 0;
}

static volatile FH_SINT32 g_isp_start = 0;
FH_SINT32 sample_common_start_isp(FH_VOID)
{
    FH_UINT32 ret;
    FH_UINT32 grpid = 0;
    if (g_isp_start)
    {
        printf("sample_isp_init is already running\n");
        return 0;
    }


    FH_VICAP_STITCH_GRP_ATTR_S stStitchConf = {0};


    memset(&stStitchConf, 0, sizeof(stStitchConf));

    FH_VICAP_SetStitchGrpAttr(&stStitchConf);

    //for (grpid = 0; grpid < MAX_GRP_NUM; grpid++)
    {
        //printf("sample_common_start_isp grpid:%d  enable:%d \n", grpid, g_isp_info[grpid].enable);
        //if (g_isp_info[grpid].enable)
        {
            ret = sample_isp_init(grpid);
            if (ret)
            {
                LOG_PRT("Error(%d - %x): sample_isp_init (grp):(%d)!\n", ret, ret, grpid);
                return -1;
            }
        }
    }

    g_isp_start = 1;
    return 0;
}





static FH_BOOL bStop;
static FH_BOOL running;
FH_VOID *sample_isp_proc(FH_VOID *arg)
{
    //struct dev_isp_info *isp_info = (struct dev_isp_info *)arg;
    char name[20];

    sprintf(name, "demo_isp%d", 0);
    prctl(PR_SET_NAME, name);
    running = 1;

    while (!bStop)
    {
        API_ISP_Run(0);


        usleep(10000);
    }

    bStop = 0;
    running = 0;
    return NULL;
}


FH_SINT32 sample_common_isp_run(FH_VOID)
{
    FH_UINT32 ret;
    pthread_t isp_thread[MAX_GRP_NUM]={0};
    pthread_attr_t attr[MAX_GRP_NUM]={0};
    struct sched_param param[MAX_GRP_NUM]={0};
    FH_UINT32 grpid = 0;

    //for (grpid = 0; grpid < MAX_GRP_NUM; grpid++)
    {
        //if (g_isp_info[grpid].enable)
        {
            //g_isp_info[grpid].bStop = 0;
            pthread_attr_init(&attr[grpid]);
            pthread_attr_setdetachstate(&attr[grpid], PTHREAD_CREATE_DETACHED);
            pthread_attr_setstacksize(&attr[grpid], 8 * 1024);
#ifdef __RTTHREAD_OS__
            param.sched_priority = 130;
#endif
            pthread_attr_setschedparam(&attr[grpid], &param[grpid]);
            ret = pthread_create(&isp_thread[grpid], &attr[grpid], sample_isp_proc, NULL);
            if (ret != 0)
            {
                LOG_PRT("Error: create ISP%d thread failed!\n", grpid);
                return -1;
            }
        }
    }

    return ret;
}

FH_SINT32 sample_common_isp_stop(FH_VOID)
{
    FH_UINT32 grpid = 0;

    //for (grpid = 0; grpid < MAX_GRP_NUM; grpid++)
    {
        //if (g_isp_info[grpid].enable)
        {
            if (bStop == 0)
            {
                bStop = 1;
                while (running)
                {
                    usleep(20 * 1000);
                }

#if defined(FH_APP_USING_IRCUT_G0) || defined(FH_APP_USING_IRCUT_G1) || defined(FH_APP_USING_IRCUT_G2)
                sample_SmartIR_deinit(grpid);
#endif
#ifdef CONFIG_VICAP_OFFLINE_MODE
                sample_common_unbind_isp(grpid);
#endif
                FH_VICAP_Exit(grpid);
                API_ISP_Exit(grpid);
            }
        }
    }

    g_isp_start = 0;
    return 0;
}

//===============================

static volatile FH_SINT32 g_stop_isp = 0;
static volatile FH_SINT32 g_isp_runnig = 0;

#if SAMPLE_ISP_RUN_ENABLE

#ifdef FH_APP_ISP_MIRROR_FLIP
static FH_VOID SetMirrorAndFlip(FH_VOID)
{
    //int time_wait;

    //time_wait = 5;
#ifdef FH_APP_ISP_MIRROR
    printf("[ISP_Strategy] set Mirror.\n");
    FHAdv_Isp_SetMirrorAndflip(FH_APP_GRP_ID, FH_APP_GRP_ID, 1, 0);
#endif //FH_APP_ISP_MIRROR
    //sleep(time_wait);
#ifdef FH_APP_ISP_FLIP
    //printf("[ISP_Strategy] set Flip.\n");
    FHAdv_Isp_SetMirrorAndflip(FH_APP_GRP_ID, FH_APP_GRP_ID, 1, 1);
#endif //FH_APP_ISP_FLIP

}
#endif

static FH_VOID* sample_isp_thread(FH_VOID *args)
{
    int cmd = 0;
    int k;

    prctl(PR_SET_NAME, "demo_adv_isp");
    while (!g_stop_isp)
    {
#ifdef FH_APP_ISP_MIRROR_FLIP
        if (cmd == 0 && 0)
        {
            SetMirrorAndFlip();
        }
#endif


        if (++cmd > 4)
            cmd = 0;

        for (k=0; k<20 && !g_stop_isp; k++)
        {
            usleep(100*1000);
        }
    }

    g_isp_runnig = 0;

    printf("*******************  sample_isp_thread exit\n");
    return FH_NULL;
}

FH_SINT32 sample_isp_start(FH_VOID)
{
    FH_SINT32 ret;

    pthread_t isp_thread;
    pthread_attr_t attr;

    printf("sample_isp_start\n");
    if (g_isp_runnig)
    {
        printf("Isp strategy is already running\n");
        return 0;
    }

    ret = FHAdv_Isp_Init(FH_APP_GRP_ID);
    if (ret)
    {
        printf("Error: isp_strategy init failed!\n");
        return ret;
    }

    g_isp_runnig = 1;
    g_stop_isp   = 0;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 10 * 1024);
    printf("sample_isp_start: pthread_attr_setstacksize %d\n", 10 * 1024);
    ret = pthread_create(&isp_thread, &attr, sample_isp_thread, FH_NULL);
    if (ret)
    {
        printf("Error: Create isp_strategy thread failed!\n");
        g_isp_runnig = 0;
    }

    return ret;
}
#endif //515,5: #if SAMPLE_ISP_RUN_ENABLE
FH_VOID sample_isp_stop(FH_VOID)
{
    g_stop_isp   = 1;
    while (g_isp_runnig)
    {
        usleep(40 * 1000);
    }
}
