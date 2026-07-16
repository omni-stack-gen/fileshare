#ifndef _MIPI_API_H_
#define _MIPI_API_H_


enum FRQ_RANGE
{
    R_80_89 = 15,
    R_90_99 = 15,
    R_100_109 = 15,
    R_109_119 = 15,
    R_120_139 = 15,
    R_140_149 = 12,
    R_150_159 = 12,
    R_160_179 = 12,
    R_180_199 = 12,
    R_200_249 = 12,
    R_250_299 = 12,
    R_300_399 = 10,
    R_350_499 = 10,
    R_400_499 = 10,
    R_500_599 = 10,
    R_600_699 = 10,
    R_700_799 = 10,
    R_800_899 = 10,
    R_900_999 = 8,
    R_1000_1099 = 8,
    R_1100_1199 = 8,
    R_1200_1299 = 8,
    R_1300_1399 = 8,
    R_1400_1499 = 8,
    R_1500_1999 = 8,
    R_2000_2499 = 8,
};

enum SENSOR_MODE
{
    NON_WDR=0,
    NOT_SONY_WDR_USE_WDR=1,
    SONY_WDR_NOT_USE_WDR=2,
    SONY_WDR_USE_WDR=3,
};

enum RAW_TYPE
{
    RAW10=1,
    RAW12=0,
};

struct mipi_conf
{
    int mipi_id;
    int frq_range;
    int sensor_mode;
    int raw_type;
    int lf_vc_id;
    int sf_vc_id;
    int lane_num;

};

int mipi_init(struct mipi_conf *config);
int mipi_deinit(int mipi_id);
int FH_VICAP_Get_Phy_Lane(unsigned int *num);

/**
* FH_MIPI_Version
*@brief 按SDK的统一格式定义的版本信息
*@param [in] print_enable:是否打印版本信息
*@param [out] 无
*@return 是否成功
* - RETURN_OK(0):  成功
* - 其他：失败,错误详见错误号
*/
char *FH_MIPI_Version(unsigned int print_enable);
#endif
