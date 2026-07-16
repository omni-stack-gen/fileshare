#ifndef __FB_IOC_H__
#define __FB_IOC_H__

#include <linux/fb.h>
#include "fh_fb_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


#define IOC_TYPE_FYFB       'F'


#define FBIOGET_COLORKEY_FYFB         _IOR(IOC_TYPE_FYFB, 90, FYFB_COLORKEY_S)          /* To obtain the colorkey of an overlay layer */
#define FBIOPUT_COLORKEY_FYFB         _IOW(IOC_TYPE_FYFB, 91, FYFB_COLORKEY_S)          /* To set the colorkey of an overlay layer */
#define FBIOGET_ALPHA_FYFB            _IOR(IOC_TYPE_FYFB, 92, FYFB_ALPHA_S)             /* To get the alpha of an overlay layer */
#define FBIOPUT_ALPHA_FYFB            _IOW(IOC_TYPE_FYFB, 93, FYFB_ALPHA_S)             /* To set the alpha of an overlay layer */
#define FBIOGET_SCREEN_ORIGIN_FYFB    _IOR(IOC_TYPE_FYFB, 94, FYFB_POINT_S)             /* To get the origin of an overlay layer on the screen */
#define FBIOPUT_SCREEN_ORIGIN_FYFB    _IOW(IOC_TYPE_FYFB, 95, FYFB_POINT_S)             /* To set the origin of an overlay layer on the screen */
#define FBIOPUT_SCREENSIZE            _IOW(IOC_TYPE_FYFB, 130, FYFB_SIZE_S*)            /* set the screen output size */
#define FBIOGET_SCREENSIZE            _IOR(IOC_TYPE_FYFB, 131, FYFB_SIZE_S*)            /* get the screen output size */
#define FBIOPUT_SHOW_FYFB             _IOW(IOC_TYPE_FYFB, 101, FH_BOOL)                 /* To set the display state of an overlay layer */
#define FBIOGET_SHOW_FYFB             _IOR(IOC_TYPE_FYFB, 102, FH_BOOL)                 /* To obtain the display state of an overlay layer */
#define FBIOPUT_COMPRESSION_FYFB      _IOW(IOC_TYPE_FYFB, 133, FH_BOOL)                 /*To set the compression status of an overlay layer*/
#define FBIOGET_COMPRESSION_FYFB      _IOR(IOC_TYPE_FYFB, 134, FH_BOOL)                 /*To obtain the compression status of an overlay layer*/


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __FB_IOC_H__ */

