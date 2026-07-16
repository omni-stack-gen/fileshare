/* dhcpc.h */
#ifndef _DHCPC_H
#define _DHCPC_H

#include "libbb_udhcp.h"

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#define INIT_SELECTING  0
#define REQUESTING  1
#define BOUND       2
#define RENEWING    3
#define REBINDING   4
#define INIT_REBOOT 5
#define RENEW_REQUESTED 6
#define RELEASED    7

typedef enum dhcp_client_event
{
	DHCPC_EVT_SELECTING = 0,
	DHCPC_EVT_REQUESTING,
	DHCPC_EVT_BOUND,
	DHCPC_EVT_RENEWING,
	DHCPC_EVT_REBINDING,
	DHCPC_EVT_NAK,
	DHCPC_EVT_LOST,
	DHCPC_EVT_RELEASED,
	DHCPC_EVT_STOPPED,
	DHCPC_EVT_ERROR
} dhcp_client_event_t;

typedef struct dhcp_client_event_info
{
	const char *interface;
	unsigned char mac[6];
	uint32_t client_ip;     /* network order */
	uint32_t server_ip;     /* network order */
	uint32_t subnet;        /* network order */
	uint32_t router;        /* network order */
	uint32_t dns0;          /* network order */
	uint32_t dns1;          /* network order */
	uint32_t lease_seconds;
	uint32_t t1_seconds;
	uint32_t t2_seconds;
	const char *reason;
} dhcp_client_event_info_t;

typedef void (*dhcp_client_event_handler_t)(
	dhcp_client_event_t event,
	const dhcp_client_event_info_t *info,
	void *user_data);

typedef enum dhcp_client_option
{
	DHCPC_OPT_INTERFACE = 0,

	DHCPC_OPT_HOSTNAME,

	DHCPC_OPT_TRY_AGAIN_TIMEOUT,

	DHCPC_OPT_TRY_DISCOVER_TIMEOUT,

	DHCPC_OPT_TRY_DISCOVER_RETRIES,

	DHCPC_OPT_EVENT_HANDLER,

	DHCPC_OPT_USER_DATA,

	DHCPC_OPT_ARPPING,

	DHCPC_OPT_MAX
} dhcp_client_option_t;

typedef struct dhcp_client
{
	int state;
	uint32_t requested_ip; /* = 0 */
	uint32_t server_addr;
	uint32_t subnet_addr;
	uint32_t router_addr;
	uint32_t dns0_addr;
	uint32_t dns1_addr;
	uint32_t lease_seconds;
	uint32_t t1_seconds;
	uint32_t t2_seconds;
	int packet_num; /* = 0 */
	int fd;
	int listen_mode;

	int exit_sockets[2];

	uint32_t xid;
	long timeout; /* must be signed */

	int tryagain_timeout;		/* Optional Wait if lease is not obtained (default 20) */
	int discover_timeout;		/* Optional Pause between packets (default 3) */
	int discover_retries;		/* Optional Send up to N discover packets (default 3) */

	char *interface;        /* The name of the interface to use (default eth0) */
	unsigned char *clientid;    /* Optional client id to use */
	char *hostname;    			/* Optional hostname to use */
	int ifindex;            /* Index number of the interface to use */
	unsigned char arp[6];	/* Our arp address */

	dhcp_client_event_handler_t event_handler;
	void *user_data;

	volatile bool arpping;

	volatile bool quit;
	pthread_t thread;
} dhcp_client_t;

void *dhcp_client_create(const char *interface);

int dhcp_client_setopt(void *handle, dhcp_client_option_t option, void *data);

int dhcp_client_start(void *handle);

int dhcp_client_stop(void *handle);

int dhcp_client_destroy(void **handle);

#ifdef DHCPC_ENABLE_TEST_HOOKS
void dhcp_client_test_prepare(
	dhcp_client_t *client,
	const char *interface,
	const unsigned char mac[6],
	dhcp_client_event_handler_t handler,
	void *user_data);

void dhcp_client_test_set_lease(
	dhcp_client_t *client,
	uint32_t client_ip,
	uint32_t server_ip,
	uint32_t subnet,
	uint32_t router,
	uint32_t dns0,
	uint32_t dns1,
	uint32_t lease_seconds,
	uint32_t t1_seconds,
	uint32_t t2_seconds);

void dhcp_client_test_emit_event(
	dhcp_client_t *client,
	dhcp_client_event_t event,
	const char *reason);

void dhcp_client_test_emit_nak_then_selecting(dhcp_client_t *client);

void dhcp_client_test_emit_lost_then_selecting(dhcp_client_t *client);

void dhcp_client_test_emit_stopped(dhcp_client_t *client);
#endif

#endif
