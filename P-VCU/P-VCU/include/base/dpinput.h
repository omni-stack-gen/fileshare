#ifndef _INPUT_H_
#define _INPUT_H_

#include <stdbool.h>

#define	MIN_KEYVALUE		1
#define	MAX_KEYVALUE		5

#define	LONG_PRESSED		0x80
#define	BUTTON_1			1
#define	BUTTON_2			2
#define	BUTTON_3			3
#define	BUTTON_4			4
#define	BUTTON_5			5

#define BUTTON_LONGPRESS_1	(BUTTON_1 | LONG_PRESSED)
#define BUTTON_LONGPRESS_2	(BUTTON_2 | LONG_PRESSED)
#define BUTTON_LONGPRESS_3	(BUTTON_3 | LONG_PRESSED)
#define BUTTON_LONGPRESS_4	(BUTTON_4 | LONG_PRESSED)
#define BUTTON_LONGPRESS_5	(BUTTON_5 | LONG_PRESSED)

#define	BUTTON_LEFT			0
#define	BUTTON_UP			1
#define	BUTTON_RIGHT		2
#define	BUTTON_DOWN			3
#define	BUTTON_PREV			4
#define	BUTTON_NEXT			5
#define	BUTTON_ENTER		6
#define	BUTTON_PAGEUP		7
#define	BUTTON_PAGEDOWN		8
#define	BUTTON_MAX			9

// #define BTN_PREV			2
// #define BTN_NEXT			3
// #define BTN_ENTER			4
// #define BTN_RETURN			5

#define HARDKBD_MESSAGE  		10001		// 按键的原始消息
#define	KBD_MESSAGE				10002
#define	KBD_DOWN				0
#define	KBD_UP					1
#define KBD_CTRL				2			// 键盘控件消息

#define TOUCH_RAW_MESSAGE		10003		// 触摸屏的原始消息，用于屏幕校正或黑屏
#define TOUCH_DOWN				0			// 某个按键被按下
#define TOUCH_VALID				1			// 某个按键按下后移动触摸点
#define TOUCH_UP				2			// 被按下的按键被释放
#define	TOUCH_MOVEOUT			3			// 被按下的按键被移出


void DPCreateInputEvent(void);

#endif //!_INPUT_H_