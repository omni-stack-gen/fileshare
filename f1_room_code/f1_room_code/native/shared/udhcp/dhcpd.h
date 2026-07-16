/* dhcpd.h */
#ifndef _DHCPD_H
#define _DHCPD_H

#include <stdbool.h>
#include <stdint.h>

#include <netinet/ip.h>
#include <netinet/udp.h>

#include "libbb_udhcp.h"

/************************************/
/* Defaults _you_ may want to tweak */
/************************************/

/* the period of time the client is allowed to use that address */
#define LEASE_TIME              (60*60*24*10) /* 10 days of seconds */

/* where to find the DHCP server configuration file */
#define DHCPD_CONF_FILE         "/etc/udhcpd.conf"

/*****************************************************************/
/* Do not modify below here unless you know what you are doing!! */
/*****************************************************************/

/* DHCP protocol -- see RFC 2131 */
#define SERVER_PORT     67
#define CLIENT_PORT     68

#define DHCP_MAGIC      0x63825363

/* DHCP option codes (partial list) */
#define DHCP_PADDING        0x00
#define DHCP_SUBNET     0x01
#define DHCP_TIME_OFFSET    0x02
#define DHCP_ROUTER     0x03
#define DHCP_TIME_SERVER    0x04
#define DHCP_NAME_SERVER    0x05
#define DHCP_DNS_SERVER     0x06
#define DHCP_LOG_SERVER     0x07
#define DHCP_COOKIE_SERVER  0x08
#define DHCP_LPR_SERVER     0x09
#define DHCP_HOST_NAME      0x0c
#define DHCP_BOOT_SIZE      0x0d
#define DHCP_DOMAIN_NAME    0x0f
#define DHCP_SWAP_SERVER    0x10
#define DHCP_ROOT_PATH      0x11
#define DHCP_IP_TTL     0x17
#define DHCP_MTU        0x1a
#define DHCP_BROADCAST      0x1c
#define DHCP_NTP_SERVER     0x2a
#define DHCP_WINS_SERVER    0x2c
#define DHCP_REQUESTED_IP   0x32
#define DHCP_LEASE_TIME     0x33
#define DHCP_OPTION_OVER    0x34
#define DHCP_MESSAGE_TYPE   0x35
#define DHCP_SERVER_ID      0x36
#define DHCP_PARAM_REQ      0x37
#define DHCP_MESSAGE        0x38
#define DHCP_MAX_SIZE       0x39
#define DHCP_T1         0x3a
#define DHCP_T2         0x3b
#define DHCP_VENDOR     0x3c
#define DHCP_CLIENT_ID      0x3d

#define DHCP_END        0xFF

#define BOOTREQUEST     1
#define BOOTREPLY       2

#define ETH_10MB        1
#define ETH_10MB_LEN        6

/* DHCP_MESSAGE_TYPE values */
#define DHCPDISCOVER            1 /* client -> server */
#define DHCPOFFER               2 /* client <- server */
#define DHCPREQUEST             3 /* client -> server */
#define DHCPDECLINE             4 /* client -> server */
#define DHCPACK                 5 /* client <- server */
#define DHCPNAK                 6 /* client <- server */
#define DHCPRELEASE             7 /* client -> server */
#define DHCPINFORM              8 /* client -> server */
#define DHCP_MINTYPE DHCPDISCOVER
#define DHCP_MAXTYPE DHCPINFORM

#define BROADCAST_FLAG      0x8000

#define OPTION_FIELD        0
#define FILE_FIELD      1
#define SNAME_FIELD     2

/* miscellaneous defines */
#define MAC_BCAST_ADDR      (unsigned char *) "\xff\xff\xff\xff\xff\xff"
#define OPT_CODE 0
#define OPT_LEN 1
#define OPT_DATA 2

struct option_set
{
	unsigned char *data;
	struct option_set *next;
};

typedef enum dhcp_server_option
{
	DHCPD_OPT_INTERFACE = 0,

	DHCPD_OPT_SERVER_IP,

	DHCPD_OPT_START_IP,

	DHCPD_OPT_END_IP,

	DHCPD_OPT_SUBNET,

	DHCPD_OPT_ROUTER,

	DHCPD_OPT_DNS,

	DHCPD_OPT_MAX_LEASE_TIME,

	DHCPD_OPT_EVENT_HANDLER,

	DHCPD_OPT_USER_DATA,

	DHCPD_OPT_MAX
} dhcp_server_option_t;

typedef enum dhcp_server_lease_event
{
	DHCPD_LEASE_EVENT_BOUND = 0,
	DHCPD_LEASE_EVENT_RELEASED,
	DHCPD_LEASE_EVENT_DECLINED,
	DHCPD_LEASE_EVENT_EXPIRED
} dhcp_server_lease_event_t;

typedef struct dhcp_server_lease_event_info
{
	const char *interface;
	uint8_t chaddr[6];
	uint32_t yiaddr;       /* network order */
	uint32_t server_nip;   /* network order */
	uint32_t lease_seconds;
} dhcp_server_lease_event_info_t;

typedef void (*dhcp_server_event_handler_t)(
	dhcp_server_lease_event_t event,
	const dhcp_server_lease_event_info_t *info,
	void *user_data);

typedef struct dhcp_server
{
	char *interface;        /* The name of the interface to use */
	int ifindex;            /* Index number of the interface to use */
	u_int32_t server_nip;   /* Our IP, in network order */
	u_int32_t server_netmask;   /* Our NETMASK, in network order */
	u_int32_t server_gateway;   /* Our GATEWAY, in network order */

	u_int32_t server_dns;      /* Our dns, in network order */
	u_int32_t start_ip;        /* Start address of leases, network order */
	u_int32_t end_ip;          /* End of leases, network order */

	struct option_set *options; /* List of DHCP options loaded from the config file */
	unsigned char arp[6];       /* Our arp address */
	u_int32_t lease;        /* lease time in seconds (host order) */
	u_int32_t max_leases;   /* maximum number of leases (including reserved address) */
	char remaining;         /* should the lease file be interpreted as lease time remaining, or
                     * as the time the lease expires */
	u_int32_t auto_time;    /* how long should udhcpd wait before writing a config file.
                     * if this is zero, it will only write one on SIGUSR1 */
	u_int32_t decline_time;     /* how long an address is reserved if a client returns a
                         * decline message */
	u_int32_t conflict_time;    /* how long an arp conflict offender is leased for */
	u_int32_t offer_time;   /* how long an offered address is reserved */
	u_int32_t min_lease;    /* minimum lease a client can request*/

	char *lease_file;
	char *pidfile;
	char *notify_file;      /* What to run whenever leases are written */

	u_int32_t siaddr;       /* next server bootp option */
	char *sname;            /* bootp server name */
	char *boot_file;        /* bootp boot file option */

	int fd;
	int exit_sockets[2];
	struct dhcpOfferedAddr *leases;
	unsigned char blank_chaddr[16];
	dhcp_server_event_handler_t event_handler;
	void *user_data;

	volatile bool quit;
	pthread_t thread;
} dhcp_server_t;

void dhcp_server_emit_lease_event(dhcp_server_t *server,
                                  dhcp_server_lease_event_t event,
                                  const uint8_t *chaddr,
                                  uint32_t yiaddr,
                                  uint32_t lease_seconds);

void *dhcp_server_create(void);

int dhcp_server_setopt(void *handle, dhcp_server_option_t option, void *data);

int dhcp_server_start(void *handle);

int dhcp_server_stop(void *handle);

int dhcp_server_destroy(void **handle);

#endif
