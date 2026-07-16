/**
 * @file DBIDCard.h
 * @brief 小门口机ID卡数据库接口声明
 * @author shihuaming (shihuaming@d-power.com.cn)
 * @version 1.0.0
 * @date 2020-02-22
 * @copyright Copyright(c) 2016 - 2020 Digital Power Inc.
 */

#ifndef _DBICCARD_H_
#define _DBICCARD_H_

#define		USER_CARD		0x1
#define		PATROL_CARD		0x2
#define		LOCAL_USER_CARD	0x3

typedef struct
{
	int				        idtype;
	unsigned long long		idcode;
	unsigned long long		roomid;
	unsigned int			ExpDate;
}CardList_T;

void ResetICCardDB(void);
bool DelICCard(unsigned long long cno);
bool DelICCardByRoomAndID(unsigned long long cno, unsigned long long roomid);
bool AddICCard(CardList_T* cardInfo);
bool CheckICCard(unsigned long long cno, CardList_T *cardinfo);
void InitICCardDB(void);

int GetICCardVersion(void);
int GetICCardcardcount(void);

int GetICCardData(CardList_T** pCard, int * cardcount);
bool SyncAllICCard(int count, CardList_T* cardInfo, unsigned int version);

#endif //_DBICCARD_H_
