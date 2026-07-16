/* net_utils.h */

#ifndef __NET_UTILS_H__
#define __NET_UTILS_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * net_dev_open - open net device.
 * @dev_name: device name.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_open(const char *dev_name);

/**
 * net_dev_close - close net device.
 * @dev_name: device name.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_close(const char *dev_name);

/**
 * net_dev_is_open - check if net device is opened.
 * @dev_name: device name.
 *
 * return 0 if device is closed; 1 if device is opened;
 * otherwise some error happened.
 */
int net_dev_is_open(const char *dev_name);

/**
 * net_dev_set_mac_addr - set net device MAC address.
 * @dev_name: device name.
 * @mac_addr: MAC address.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_set_mac_addr(const char *dev_name, const unsigned char *mac_addr);

/**
 * net_dev_get_mac_addr - get net device MAC address.
 * @dev_name: device name.
 * @mac_addr: MAC address buffer.
 *
 * return 0 if success; otherwise failed.
 */
int net_dev_get_mac_addr(const char *dev_name, unsigned char *mac_addr);

#ifdef __cplusplus
}
#endif

#endif /* __NET_UTILS_H__ */
