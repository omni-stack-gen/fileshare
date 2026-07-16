#ifndef _DBLANGUAGE_H_
#define _DBLANGUAGE_H_

#include <stdint.h>

typedef struct
{
	uint16_t				roomid;
	uint16_t				languageid;           /*1：阿拉伯语 2: 德语 3.俄语 4.法语 5.韩语6:日语 7:土耳其语 8:西班牙语 9:希伯来语 10:英文*/
}LanguageList_T;


bool DelLanguage(uint16_t roomid);
bool AddLanguage(LanguageList_T* languageInfo);
bool CheckLanguage(uint16_t roomid, LanguageList_T *languageinfo);

void InitLanguageDB(void);
void ResetLanguageDB(void);

#endif // _DBLANGUAGE_H_
