/*
 * fh_af_ctrl.h
 *
 *  Created on: 2020-12-5
 *      Author: dongky341
 */

#ifndef __FH_AF_CTRL_H_
#define __FH_AF_CTRL_H_

#ifdef _cplusplus
#if _cplusplus
extern "C"{
#endif
#endif

#include "isp/isp_common.h"

typedef enum
{
    AF_HOLD             = 0, /* stay */
    AF_LENS_RESET       = 1, /* reset focus len */
    AF_LENS_MOVE_ONLY   = 2, /* move focus len*/
    AF_MOVE_ZOOM        = 3, /* move zoom len and trigger autofocus by zfmap */
    AF_AUTOFOCUS        = 4, /* autofocus by algorithm */
    AF_UNDO             = 5, /* return zoom len to last status and trigger autofocus */
    AF_GENERATE_MAPFILE = 6, /* generate zfmap. If zfmap file doesn't exist, run this macro first */
    AF_TEST             = 7  /* only to test */
}FH_AF_CTRL_e;

typedef enum
{
    AF_DEBUG_OFF       = 0x0, /* disable debug */
    AF_DEBUG_POS       = 0x1, /* show focus and zoom lens' position */
    AF_DEBUG_STAGES    = 0x2, /* show autofocus details */
    AF_DEBUG_ALL       = 0xf  /* enable all debug print */
}FH_AF_DEBUG_LEVEL_e;

typedef enum
{
    AF_MOTOR_PWM = 0, /* motiviated by pwm */
    AF_MOTOR_STM = 1, /* motiviated by step motor */
}FH_AF_MOTOR_TYPE_e;

typedef struct
{
    int id;   /* lens id  0:focus, 1:zoom */
    int dir;  /* move dir 0:backward, 1:forward*/
    int step; /* move step, should be greater than 0 */
}FH_AF_CFG_t;

typedef struct
{
    int zoom_range;
    int focus_range;
    int zoom_reserve_range;
    int focus_reserve_range;
    int inv_error;
    int freq_low;
    int freq_mid;
    int freq_high;

    int zoom_pwm[4]; /* A- A+ B+ B- */
    int focus_pwm[4]; /* A- A+ B- B+ */
    FH_AF_MOTOR_TYPE_e type;
}FH_AF_MOTOR_CFG_t;

typedef struct
{
    int gainthre;
    int frmW;
    int frmH;
    int reserved[2];
}FH_AF_INTER_CFG_t;

typedef struct
{
    ISP_AF_FILTER afFilter;
    ISP_AF_STAT_CFG afStat;
    int (*sharpness_cb)(int grpid);
}FH_AF_EXTER_CFG_t;

typedef struct
{
    int mode; /* 0: built-in strategy, 1: external strategy*/
    char mapPth[20];
    union
    {
        FH_AF_INTER_CFG_t interCfg;
        FH_AF_EXTER_CFG_t exterCfg;
    };

}FH_AF_STG_CFG_t;


/*
*@ af system init
*@return 0: success
*/
int FH_AF_Init(int grpid, FH_AF_MOTOR_CFG_t *motorcfg, FH_AF_STG_CFG_t *cfg);

/*
*@ af control, can be used as ioctl(), refer to AF_CTRL_e
*/
int FH_AF_Ctrl(int grpid, FH_AF_CTRL_e cmd, void *args);

/*
*@ af frame rate control, must call it once per frame
*/
void FH_AF_FrameRun(int grpid);

/*
*@ set af debug level, refer to AF_DEBUG_LEVEL_e
*/
int FH_AF_SetDebugLevel(int grpid, FH_AF_DEBUG_LEVEL_e level);

/*
*@ uninit af system
*/
void FH_AF_Exit(int grpid);

/*
*@ print version
*/
void FH_AF_PrintVersion(void);

#ifdef _cpluspluc
#if _cplusplus
}
#endif
#endif
#endif /* __FH_AF_CTRL_H_ */
