#ifndef _DTLN_TR_H_
#define _DTLN_TR_H_

// #ifdef ENABLE_NS_NN
#define DTLN_NEED_LENGTH    256//300
// #else
//#define DTLN_NEED_LENGTH    512//300
// #endif //ENABLE_NS_NN

void *dtln_create_handle(void);
typedef void *(*DtlnCreateHandle)(void);

int dtln_destroy_handle(void*phandle);
typedef int (*DtlnDestroyHandle)(void*phandle);

int dtln_train_by_handle(void*phandle, short *pindata, short *pplbdata, int indatasize, short *poutdata);
typedef int (*DtlnTrainByHandle)(void*phandle, short *pindata, short *pplbdata, int indatasize, short *poutdata);



void *AEC_create_handle(void);
int AEC_destroy_handle(void*phandle);
int AEC_train_by_handle(void*phandle, short *pindata, short *pplbdata, int indatasize, short *poutdata);


//单降噪模块， 回采数据无效
void *NS_create_handle(void);
int NS_destroy_handle(void*phandle);
int NS_train_by_handle(void*phandle, short *pindata, short *pplbdata, int indatasize, short *poutdata);

#endif //_DTLN_TR_H_