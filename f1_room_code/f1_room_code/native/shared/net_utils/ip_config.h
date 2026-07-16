/* ip_config.h */

#ifndef __IP_CONFIG_H__
#define __IP_CONFIG_H__

/**
 * enum ip_assignment - IP assignment way.
 * @IP_ASSIGN_STATIC: use statically configured IP settings.
 * @IP_ASSIGN_DHCP: use dynamically configured IP settings.
 * @IP_ASSIGN_UNASSIGNED: no IP details are assigned.
 */
typedef enum ip_assignment
{
	IP_ASSIGN_STATIC = 0,
	IP_ASSIGN_DHCP = 1,
	IP_ASSIGN_UNASSIGNED = 2,
} ip_assignment_t;

/**
 * enum ip_gset_status - IP get and set status.
 * @GSET_PARAM_ERR: invalid parameters.
 * @SET_IP_ERR: failed to set IP Address.
 * @SET_NETMASK_ERR: failed to set netmask.
 * @SET_GATEWAY_ERR: failed to set gateway.
 * @SET_DNS_ERR: failed to set dns.
 * @GET_IP_ERR: failed to get IP Address.
 * @GET_NETMASK_ERR: failed to get netmask.
 * @GET_GATEWAY_ERR: failed to get gateway.
 *
 * Note: failed to get dns is not regard as an error.
 */
typedef enum ip_gset_status
{
	GSET_PARAM_ERR = (1 << 0),
	SET_IP_ERR = (1 << 1),
	SET_NETMASK_ERR = (1 << 2),
	SET_GATEWAY_ERR = (1 << 3),
	SET_DNS_ERR = (1 << 4),
	GET_IP_ERR = (1 << 5),
	GET_NETMASK_ERR = (1 << 6),
	GET_GATEWAY_ERR = (1 << 7),
} ip_gset_status;

/**
 * enum dhcp_status - DHCP status.
 * @DHCP_ERROR: DHCP error.
 * @DHCP_ABORT: DHCP abort.
 * @DHCP_TIMEOUT: DHCP timeout.
 * @DHCP_SUCCESS: DHCP success.
 */
typedef enum dhcp_status
{
	DHCP_ERROR = -3,
	DHCP_ABORT = -2,
	DHCP_TIMEOUT = -1,
	DHCP_SUCCESS = 0,
} dhcp_status_t;

/**
 * struct ip_static_config - static IP configuration.
 * @ip_addr: IP Address, big-endian.
 * @netmask: netmask, big-endian.
 * @gateway: gate way, big-endian.
 * @dns0: first choice dns, big-endian.
 * @dns1: additional dns, big-endian.
 */
typedef struct ip_static_config
{
	unsigned char ip_addr[4];
	unsigned char netmask[4];
	unsigned char gateway[4];
	unsigned char dns0[4];
	unsigned char dns1[4];
} ip_static_config_t;

/**
 * struct ip_config - IP configuration.
 * @ip: static IP configuration.
 * @assign: IP assignment way.
 */
typedef struct ip_config
{
	ip_static_config_t ip;
	ip_assignment_t assign;
} ip_config_t;

/**
 * dhcp_status_handler - DHCP status handler.
 * @status: DHCP status.
 */
typedef void (*dhcp_status_handler)(const char *dev_name, dhcp_status_t status, void *user_data);

/**
 * net_dev_del_ip_config - net device delete IP config.
 * @dev_name: device name.
 *
 * return 0 if success; otherwise failed.
 */
extern int net_dev_del_ip_config(const char *dev_name);

/**
 * net_dev_set_static_ip - set static IP configuration.
 * @dev_name: device name.
 * @ip_config: static IP configuration.
 *
 * return 0 if success; otherwise failed.
 */
extern int net_dev_set_static_ip(const char *dev_name,
                                 const ip_static_config_t *ip_config);

/**
 * net_dev_get_static_ip - get static IP configuration.
 * @dev_name: device name.
 * @ip_config: static IP configuration.
 *
 * return 0 if success; otherwise failed.
 */
extern int net_dev_get_static_ip(const char *dev_name,
                                 ip_static_config_t *ip_config);

/**
 * net_dev_change_priority - net device change priority.
 * @dev_name: device name.
 *
 * return 0 if success; otherwise failed.
 */
extern int net_dev_change_priority(const char *dev_name);

/**
 * net_dev_get_ip_addr - net device get ip address.
 * @dev_name: device name.
 * @ip_addr: ip address.
 *
 * return 0 if success; otherwise failed.
 */
extern int net_dev_get_ip_addr(const char *dev_name, unsigned long *ip_addr);

#endif /* __IP_CONFIG_H__ */
