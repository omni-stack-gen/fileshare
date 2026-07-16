#ifndef __FH_AES_MPI_H__
#define __FH_AES_MPI_H__
/**AES**/
#include "fh_aes_mpipara.h"

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif /* __cplusplus */
#endif  /* __cplusplus */


/**
 * @brief         打开AES设备
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,aes_errcode} 。
 *
 * @note
 *
 */
FH_SINT32 FH_AES_Open(FH_VOID);

/**
 * @brief         关闭AES设备
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,aes_errcode} 。
 *
 * @note
 *
 */
FH_VOID FH_AES_Close(FH_VOID);

/**
 * @brief         初始化AES模块
 *
 * @param[in]     pAlignmask          对其标记
 * @param[in]     key                 key
 * @param[in]     key_size            key长度
 * @param[in]     alg                 模式
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,aes_errcode} 。
 *
 * @note
 *
 */
FH_SINT32 FH_AES_Init(FH_UINT16 *pAlignmask, const char *key, unsigned int key_size, enum cryptodev_crypto_op_t alg);

/**
 * @brief         去初始化AES模块
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,aes_errcode} 。
 *
 * @note
 *
 */
FH_SINT32 FH_AES_Deinit(FH_VOID);

/**
 * @brief         AES加密
 *
 * @param[in]     iv                     iv
 * @param[in]     plaintext              plaintext
 * @param[in]     ciphertext             ciphertext
 * @param[in]     size                   长度
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,aes_errcode} 。
 *
 * @note
 *
 */
FH_SINT32 FH_AES_Encrypt(const void* iv, const void* plaintext, void* ciphertext, size_t size);

/**
 * @brief         AES解密
 *
 * @param[in]     iv                    iv
 * @param[in]     ciphertext            ciphertext
 * @param[in]     plaintext             plaintext
 * @param[in]     size                  长度
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,aes_errcode} 。
 *
 * @note
 *
 */
FH_SINT32 FH_AES_Decrypt(const void* iv, const void* ciphertext, void* plaintext, size_t size);



#ifdef __cplusplus
 #if __cplusplus
}
 #endif /* __cplusplus */
#endif  /* __cplusplus */

#endif  /* __FH_AES_MPI_H__ */
