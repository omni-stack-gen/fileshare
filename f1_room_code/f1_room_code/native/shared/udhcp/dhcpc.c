/* dhcpc.c
 *
 * udhcp DHCP client
 *
 * Russ Dill <Russ.Dill@asu.edu> July 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <pthread.h>

#include "dhcpd.h"
#include "dhcpc.h"
#include "options.h"
#include "clientpacket.h"
#include "packet.h"
#include "socket.h"
#include "debug.h"
#include "arpping.h"

#include <net_utils/net_utils.h>
#include <net_utils/ip_config.h>

#include <utils/thread_helper.h>

#define LISTEN_NONE 0
#define LISTEN_KERNEL 1
#define LISTEN_RAW 2

#define DHCPC_INTERFACE             "eth0"
#define DHCPC_TRY_AGAIN_TIMEOUT     (20)
#define DHCPC_TRY_DISCOVER_TIMEOUT  (3)
#define DHCPC_TRY_DISCOVER_RETRIES  (3)

static void dhcp_client_use_default_init_config(dhcp_client_t *client)
{
	client->interface        = bstrdup(DHCPC_INTERFACE);
	client->tryagain_timeout = DHCPC_TRY_AGAIN_TIMEOUT;
	client->discover_timeout = DHCPC_TRY_DISCOVER_TIMEOUT;
	client->discover_retries = DHCPC_TRY_DISCOVER_RETRIES;
}

static const char *dhcp_client_state_to_string(int state)
{
	switch (state)
	{
		case INIT_SELECTING:
			return "init selecting";

		case REQUESTING:
			return "requesting";

		case BOUND:
			return "bound";

		case RENEWING:
			return "renewing";

		case REBINDING:
			return "rebinding";

		case INIT_REBOOT:
			return "init reboot";

		case RENEW_REQUESTED:
			return "renew requested";

		case RELEASED:
			return "released";

		default:
			return "(unknown)";
	}
}

static const char *dhcp_client_event_to_string(dhcp_client_event_t event)
{
	switch (event)
	{
		case DHCPC_EVT_SELECTING:
			return "SELECTING";
		case DHCPC_EVT_REQUESTING:
			return "REQUESTING";
		case DHCPC_EVT_BOUND:
			return "BOUND";
		case DHCPC_EVT_RENEWING:
			return "RENEWING";
		case DHCPC_EVT_REBINDING:
			return "REBINDING";
		case DHCPC_EVT_NAK:
			return "NAK";
		case DHCPC_EVT_LOST:
			return "LOST";
		case DHCPC_EVT_RELEASED:
			return "RELEASED";
		case DHCPC_EVT_STOPPED:
			return "STOPPED";
		case DHCPC_EVT_ERROR:
			return "ERROR";
		default:
			return "UNKNOWN";
	}
}

static int dhcp_client_state_to_event(int state, dhcp_client_event_t *event)
{
	if (!event)
	{
		return -1;
	}

	switch (state)
	{
		case INIT_SELECTING:
			*event = DHCPC_EVT_SELECTING;
			return 0;
		case REQUESTING:
			*event = DHCPC_EVT_REQUESTING;
			return 0;
		case BOUND:
			*event = DHCPC_EVT_BOUND;
			return 0;
		case RENEWING:
			*event = DHCPC_EVT_RENEWING;
			return 0;
		case REBINDING:
			*event = DHCPC_EVT_REBINDING;
			return 0;
		case RELEASED:
			*event = DHCPC_EVT_RELEASED;
			return 0;
		default:
			return -1;
	}
}

static void dhcp_client_emit_event(dhcp_client_t *client,
                                   dhcp_client_event_t event,
                                   const char *reason)
{
	dhcp_client_event_info_t info;

	if (!client || !client->event_handler)
	{
		return;
	}

	memset(&info, 0, sizeof(info));
	info.interface = client->interface;
	memcpy(info.mac, client->arp, sizeof(info.mac));
	info.client_ip = client->requested_ip;
	info.server_ip = client->server_addr;
	info.subnet = client->subnet_addr;
	info.router = client->router_addr;
	info.dns0 = client->dns0_addr;
	info.dns1 = client->dns1_addr;
	info.lease_seconds = client->lease_seconds;
	info.t1_seconds = client->t1_seconds;
	info.t2_seconds = client->t2_seconds;
	info.reason = reason;

	LOGI("dhcp client event=%s interface=%s reason=%s\n",
	     dhcp_client_event_to_string(event),
	     info.interface ? info.interface : "-",
	     reason ? reason : "-");
	client->event_handler(event, &info, client->user_data);
}

static void dhcp_client_clear_lease_info(dhcp_client_t *client)
{
	if (!client)
	{
		return;
	}

	client->requested_ip = 0;
	client->server_addr = 0;
	client->subnet_addr = 0;
	client->router_addr = 0;
	client->dns0_addr = 0;
	client->dns1_addr = 0;
	client->lease_seconds = 0;
	client->t1_seconds = 0;
	client->t2_seconds = 0;
}

static void dhcp_client_state_change(dhcp_client_t *client, int state)
{
	dhcp_client_event_t event;

	if (client->state == state)
	{
		return ;
	}

	LOGI("dhcp client state [%s] >>> [%s] \n", dhcp_client_state_to_string(client->state),
	     dhcp_client_state_to_string(state));

	client->state = state;

	if (dhcp_client_state_to_event(client->state, &event) == 0)
	{
		dhcp_client_emit_event(client, event, "state_change");
	}
}

/* just a little helper */
static void change_mode(dhcp_client_t *client, int new_mode)
{
	DEBUG(LOG_INFO, "entering %s listen mode",
	      new_mode ? (new_mode == 1 ? "kernel" : "raw") : "none");

	if (client->fd >= 0)
	{
		close(client->fd);
		client->fd = -1;
	}

	client->listen_mode = new_mode;
}

static void *dhcp_client_thread(void *args)
{
	os_thread_set_name("dhcpc");

	unsigned char *temp = NULL, *message = NULL;
	unsigned long t1 = 0, t2 = 0;
	unsigned long start = 0, lease = 0;

	fd_set rfds;
	int retval;
	struct timeval tv;
	int len;
	struct dhcpMessage packet;
	struct in_addr temp_addr;
	time_t now;
	int max_fd;

	dhcp_client_t *client = (dhcp_client_t *)args;

	LOGI("dhcp_client_thread start \n");

	client->state = INIT_SELECTING;
	dhcp_client_emit_event(client, DHCPC_EVT_SELECTING, "start");

#ifdef NDEBUG
	net_dev_del_ip_config(client->interface);
#endif

	change_mode(client, LISTEN_RAW);

	// while (!client->quit)
	while (1)
	{
		tv.tv_sec = client->timeout - monotonic_sec();
		tv.tv_usec = 0;

		FD_ZERO(&rfds);

		if (client->listen_mode != LISTEN_NONE && client->fd < 0)
		{
			if (client->listen_mode == LISTEN_KERNEL)
			{
				client->fd = listen_socket(INADDR_ANY, CLIENT_PORT, client->interface);
			}
			else
			{
				client->fd = raw_socket(client->ifindex);
			}

			if (client->fd < 0)
			{
				LOGE("FATAL: couldn't listen on socket, %s\n", strerror(errno));
				dhcp_client_emit_event(client,
				                       DHCPC_EVT_ERROR,
				                       "listen_socket");
				break;
			}
		}

		if (client->fd >= 0)
		{
			FD_SET(client->fd, &rfds);
		}

		FD_SET(client->exit_sockets[0], &rfds);

		if (tv.tv_sec > 0)
		{
			if (client->state != INIT_SELECTING)
			{
				LOGI("%s [%s] Waiting on %ld sec select...\n", client->interface,
				     dhcp_client_state_to_string(client->state), tv.tv_sec);
			}

			max_fd = client->exit_sockets[0] > client->fd ? client->exit_sockets[0] : client->fd;
			retval = select(max_fd + 1, &rfds, NULL, NULL, &tv);
		}
		else
		{
			retval = 0;    /* If we already timed out, fall through */
		}

		now = monotonic_sec();

		if (retval == 0)
		{
			/* timeout dropped to zero */

			switch (client->state)
			{
				case INIT_SELECTING:

					// if (client->packet_num < 3)
					if (!client->discover_retries || client->packet_num < client->discover_retries)
					{
						if (client->packet_num == 0)
						{
							client->xid = random_xid();
						}

						/* send discover packet */
						send_discover(client); /* broadcast */

						// client->timeout = now + ((client->packet_num == 2) ? 4 : 2);
						client->timeout = now + client->discover_timeout;
						client->packet_num++;
					}
					else
					{
						/* wait to try again */
						client->packet_num = 0;
						client->timeout = now + client->tryagain_timeout;

						LOGI("No lease, wait %d sec to try again\n", client->tryagain_timeout);
					}

					break;

				case RENEW_REQUESTED:
				case REQUESTING:
					if (client->packet_num < 3)
					{
						/* send request packet */
						if (client->state == RENEW_REQUESTED)
						{
							send_renew(client);    /* unicast */
						}
						else
						{
							send_selecting(client);    /* broadcast */
						}

						// client->timeout = now + ((client->packet_num == 2) ? 10 : 2);
						client->timeout = now + client->discover_timeout;
						client->packet_num++;
					}
					else
					{
						/* timed out, go back to init state */
						if (client->state == RENEW_REQUESTED)
						{
							net_dev_del_ip_config(client->interface);
							dhcp_client_emit_event(client,
							                       DHCPC_EVT_LOST,
							                       "renew_request_timeout");
						}

						client->timeout = now;
						client->packet_num = 0;
						dhcp_client_clear_lease_info(client);
						dhcp_client_state_change(client, INIT_SELECTING);
						change_mode(client, LISTEN_RAW);
					}

					break;

				case BOUND:
					/* Lease is starting to run out, time to enter renewing state */
					dhcp_client_state_change(client, RENEWING);

					change_mode(client, LISTEN_KERNEL);
					LOGI("Entering renew state \n");

				/* fall right through */
				case RENEWING:

					/* Either set a new T1, or enter REBINDING state */
					if ((t2 - t1) <= (lease / 14400 + 1))
					{
						/* timed out, enter rebinding state */
						dhcp_client_state_change(client, REBINDING);

						client->timeout = now + (t2 - t1);
						LOGI("Entering rebinding state\n");
					}
					else
					{
						/* send a request packet */
						send_renew(client); /* unicast */

						t1 = (t2 - t1) / 2 + t1;
						client->timeout = t1 + start;
					}

					break;

				case REBINDING:

					/* Either set a new T2, or enter INIT state */
					if ((lease - t2) <= (lease / 14400 + 1))
					{
						/* timed out, enter init state */
						LOGI("Lease lost, entering init state \n");
						net_dev_del_ip_config(client->interface);
						dhcp_client_emit_event(client,
						                       DHCPC_EVT_LOST,
						                       "lease_timeout");

						client->timeout = now;
						client->packet_num = 0;
						dhcp_client_clear_lease_info(client);
						dhcp_client_state_change(client, INIT_SELECTING);
						change_mode(client, LISTEN_RAW);
					}
					else
					{
						/* send a request packet */
						send_renew(client); /* broadcast */

						t2 = (lease - t2) / 2 + t2;
						client->timeout = t2 + start;
					}

					break;

				case RELEASED:
					/* yah, I know, *you* say it would never happen */
					client->timeout = 0x7fffffff;
					break;
			}
		}
		else if (retval > 0 && client->listen_mode != LISTEN_NONE && FD_ISSET(client->fd, &rfds))
		{
			/* a packet is ready, read it */

			if (client->listen_mode == LISTEN_KERNEL)
			{
				len = get_packet(&packet, client->fd);
			}
			else
			{
				len = get_raw_packet(&packet, client->fd);
			}

			if (len == -1 && errno != EINTR)
			{
				LOGE("error on read, %s, reopening socket\n", strerror(errno));
				change_mode(client, client->listen_mode); /* just close and reopen */
			}

			if (len < 0)
			{
				continue;
			}

			if (packet.xid != client->xid)
			{
				LOGW("Ignoring XID %lx (our xid is %lx)\n",
				     (unsigned long) packet.xid, (unsigned long)client->xid);
				continue;
			}

			if ((message = get_option(&packet, DHCP_MESSAGE_TYPE)) == NULL)
			{
				LOGW("couldnt get option from packet -- ignoring\n");
				continue;
			}

			switch (client->state)
			{
				case INIT_SELECTING:

					/* Must be a DHCPOFFER to one of our xid's */
					if (*message == DHCPOFFER)
					{
						if ((temp = get_option(&packet, DHCP_SERVER_ID)))
						{
							memcpy(&client->server_addr, temp, 4);
							client->xid = packet.xid;
							client->requested_ip = packet.yiaddr;

							/* enter requesting state */
							dhcp_client_state_change(client, REQUESTING);

							client->timeout = now;
							client->packet_num = 0;
						}
						else
						{
							LOGE("No server ID in message \n");
						}
					}

					break;

				case RENEW_REQUESTED:
				case REQUESTING:
				case RENEWING:
				case REBINDING:
					if (*message == DHCPACK)
					{
						if (!(temp = get_option(&packet, DHCP_LEASE_TIME)))
						{
							LOGE("No lease time with ACK, using 1 hour lease\n");
							lease = 60 * 60;
						}
						else
						{
							memcpy(&lease, temp, 4);
							lease = ntohl(lease);
						}

						// lease = 60;

						/* enter bound state */
						t1 = lease / 2;

						/* little fixed point for n * .875 */
						t2 = (lease * 0x7) >> 3;
						temp_addr.s_addr = packet.yiaddr;

						LOGI("Lease of %s obtained, lease time %ld\n", inet_ntoa(temp_addr), lease);

						start = now;
						client->timeout = t1 + start;
						client->requested_ip = packet.yiaddr;
						client->lease_seconds = (uint32_t)lease;
						client->t1_seconds = (uint32_t)t1;
						client->t2_seconds = (uint32_t)t2;

						// TODO 后面需要优化
						if (client->arpping)
						{
							int rc = arpping(packet.yiaddr, 0, client->arp, client->interface);

							if (rc == 0)
							{
								LOGI("%s belongs to someone\n", inet_ntoa(temp_addr));
							}
						}

#if 0
						int i = 0, num_options;

						for (i = 0; options[i].code; i++)
						{
							if (get_option(&packet, options[i].code))
							{
								num_options++;
								printf("name:%s code:0x%02x\n", options[i].name, options[i].code);
							}
						}

#endif

						ip_static_config_t ip_param;

						memset(&ip_param, 0, sizeof(ip_param));

						// ip地址
						memcpy(&ip_param.ip_addr, &packet.yiaddr, 4);

						// 子网掩码
						if (!(temp = get_option(&packet, DHCP_SUBNET)))
						{
							LOGE("No subnet with ACK\n");
							client->subnet_addr = 0;
						}
						else
						{
							memcpy(&ip_param.netmask, temp, 4);
							memcpy(&client->subnet_addr, temp, 4);
						}

						// 网关
						if (!(temp = get_option(&packet, DHCP_ROUTER)))
						{
							LOGE("No router with ACK\n");
							client->router_addr = 0;
						}
						else
						{
							memcpy(&ip_param.gateway, temp, 4);
							memcpy(&client->router_addr, temp, 4);
						}

						// dns
						if (!(temp = get_option(&packet, DHCP_DNS_SERVER)))
						{
							LOGE("No dns with ACK\n");
							client->dns0_addr = 0;
							client->dns1_addr = 0;
						}
						else
						{
							int dns_length = temp[OPT_LEN - 2];

							// LOGI("dns length:%d \n", dns_length);

							memcpy(&ip_param.dns0, temp, 4);
							memcpy(&client->dns0_addr, temp, 4);

							if (dns_length == 8)
							{
								memcpy(&ip_param.dns1, &temp[4], 4);
								memcpy(&client->dns1_addr, &temp[4], 4);
							}
							else
							{
								client->dns1_addr = 0;
							}
						}

						net_dev_set_static_ip(client->interface, &ip_param);

						dhcp_client_state_change(client, BOUND);

						change_mode(client, LISTEN_NONE);
					}
					else if (*message == DHCPNAK)
					{
						/* return to init state */
						LOGI("Received DHCP NAK\n");

						if (client->state != REQUESTING)
						{
							net_dev_del_ip_config(client->interface);
						}
						dhcp_client_emit_event(client,
						                       DHCPC_EVT_NAK,
						                       "dhcp_nak");

						client->timeout = now;
						client->packet_num = 0;
						dhcp_client_clear_lease_info(client);
						dhcp_client_state_change(client, INIT_SELECTING);
						change_mode(client, LISTEN_RAW);

						sleep(3); /* avoid excessive network traffic */
					}

					break;
					/* case BOUND, RELEASED: - ignore all packets */
			}
		}
		else if (retval > 0 && FD_ISSET(client->exit_sockets[0], &rfds))
		{
			char quit;

			if (read(client->exit_sockets[0], &quit, sizeof(quit)) < 0)
			{
				DEBUG(LOG_ERR, "Could not read exit_sockets: %s", strerror(errno));
			}

			break;
		}
		else
		{
			/* An error occured */
			LOGE("Error on select retval:%d error:%d\n", retval, errno);
			dhcp_client_emit_event(client,
			                       DHCPC_EVT_ERROR,
			                       "select");
		}
	}

	if (client->requested_ip)
	{
		int release_ret = 0;

		if (client->server_addr)
		{
			release_ret = send_release(client);
			LOGI("DHCP release result=%d\n", release_ret);
		}
		dhcp_client_emit_event(client,
		                       DHCPC_EVT_RELEASED,
		                       "stop");
	}
	dhcp_client_emit_event(client,
	                       DHCPC_EVT_STOPPED,
	                       "thread_exit");
	client->state = INIT_SELECTING;
	client->timeout = 0;
	dhcp_client_clear_lease_info(client);
	client->packet_num = 0;

	change_mode(client, LISTEN_NONE);

	LOGI("dhcp_client_thread end \n");

	return NULL;
}

void *dhcp_client_create(const char *interface)
{
	dhcp_client_t *client = NULL;

	client = bmalloc(sizeof(dhcp_client_t));

	if (client == NULL)
	{
		LOGE("malloc failed!\n");
		goto __exit;
	}

	memset(client, 0, sizeof(dhcp_client_t));

	dhcp_client_use_default_init_config(client);

	client->fd = -1;
	client->exit_sockets[0] = client->exit_sockets[1] = -1;

	client->quit = true;

	if (interface)
	{
		bfree(client->interface);

		client->interface = bstrdup(interface);
	}

	if (read_interface(client->interface, &client->ifindex,
	                   NULL, client->arp) < 0)
	{
		LOGE("read_interface failed!\n");
		goto __exit;
	}

	client->clientid = bmalloc(6 + 3);
	client->clientid[OPT_CODE] = DHCP_CLIENT_ID;
	client->clientid[OPT_LEN] = 7;
	client->clientid[OPT_DATA] = 1;
	memcpy(client->clientid + 3, client->arp, 6);

	return client;

__exit:

	if (client)
	{
		if (client->interface)
		{
			bfree(client->interface);
			client->interface = NULL;
		}

		if (client->clientid)
		{
			bfree(client->clientid);
			client->clientid = NULL;
		}

		bfree(client);
	}

	return NULL;
}

int dhcp_client_setopt(void *handle, dhcp_client_option_t option, void *data)
{
	int ret = 0;

	dhcp_client_t *client = (dhcp_client_t *)handle;

	if (client == NULL || data == NULL)
	{
		return -1;
	}

	if (option >= DHCPC_OPT_MAX)
	{
		return -1;
	}

	switch (option)
	{
		case DHCPC_OPT_INTERFACE:
			{
				if (client->interface)
				{
					bfree(client->interface);
					client->interface = NULL;
				}

				if (client->clientid)
				{
					bfree(client->clientid);
					client->clientid = NULL;
				}

				client->interface = bstrdup((const char *)data);

				ret = read_interface(client->interface, &client->ifindex,
				                     NULL, client->arp);

				if (ret == 0)
				{
					client->clientid = bmalloc(6 + 3);
					client->clientid[OPT_CODE] = DHCP_CLIENT_ID;
					client->clientid[OPT_LEN] = 7;
					client->clientid[OPT_DATA] = 1;
					memcpy(client->clientid + 3, client->arp, 6);
				}
			}
			break;

		case DHCPC_OPT_HOSTNAME:
			{
				if (client->hostname)
				{
					bfree(client->hostname);
					client->hostname = NULL;
				}

				int opt_len = 0;

				const char *opt_str = (const char *)data;

				opt_len = strlen(opt_str);

				opt_len = opt_len > 255 ? 255 : opt_len;

				client->hostname = bmalloc(opt_len + 2);
				client->hostname[OPT_CODE] = DHCP_HOST_NAME;
				client->hostname[OPT_LEN] = opt_len;
				strncpy(client->hostname + 2, opt_str, opt_len);
			}
			break;

		case DHCPC_OPT_TRY_AGAIN_TIMEOUT:
			{
				client->tryagain_timeout = *(int *)data;
			}
			break;

		case DHCPC_OPT_TRY_DISCOVER_TIMEOUT:
			{
				client->discover_timeout = *(int *)data;
			}
			break;

		case DHCPC_OPT_TRY_DISCOVER_RETRIES:
			{
				client->discover_retries = *(int *)data;
			}
			break;

		case DHCPC_OPT_EVENT_HANDLER:
			{
				client->event_handler = (dhcp_client_event_handler_t)data;
			}
			break;

		case DHCPC_OPT_USER_DATA:
			{
				client->user_data = data;
			}
			break;

		case DHCPC_OPT_ARPPING:
			{
				client->arpping = *(bool *)data;
			}
			break;

		default:
			LOGE("unknow dhcp client option:%d \n", option);
			break;
	}

	return ret;
}

int dhcp_client_start(void *handle)
{
	int ret = 0;

	dhcp_client_t *client = (dhcp_client_t *)handle;

	if (client == NULL)
	{
		return -1;
	}

	if (client->quit)
	{
		client->quit = false;

		ret = socketpair(AF_UNIX, SOCK_STREAM, 0, client->exit_sockets);

		if (ret != 0)
		{
			LOGE("create socketpair failed! \n");
			goto __exit;
		}

		ret = pthread_create(&client->thread,
		                     NULL,
		                     dhcp_client_thread,
		                     client);

		if (ret != 0)
		{
			LOGE("create thread failed! \n");
			goto __exit;
		}
	}

	return ret;

__exit:

	dhcp_client_stop(client);

	return ret;
}

int dhcp_client_stop(void *handle)
{
	int ret = 0;

	dhcp_client_t *client = (dhcp_client_t *)handle;

	if (client == NULL)
	{
		return -1;
	}

	client->quit = true;

	if (client->thread)
	{
		write(client->exit_sockets[1], "q", 1);

		pthread_join(client->thread, NULL);
		client->thread = 0;
	}

	if (client->exit_sockets[0] >= 0)
	{
		close(client->exit_sockets[0]);
		client->exit_sockets[0] = -1;
	}

	if (client->exit_sockets[1] >= 0)
	{
		close(client->exit_sockets[1]);
		client->exit_sockets[1] = -1;
	}

	return 0;
}

int dhcp_client_destroy(void **handle)
{
	int ret = 0;

	if (handle == NULL || *handle == NULL)
	{
		return -1;
	}

	dhcp_client_t *client = *(dhcp_client_t **)handle;

	if (client->quit == false)
	{
		dhcp_client_stop(client);
	}

	if (client->interface)
	{
		bfree(client->interface);
		client->interface = NULL;
	}

	if (client->clientid)
	{
		bfree(client->clientid);
		client->clientid = NULL;
	}

	if (client->hostname)
	{
		bfree(client->hostname);
		client->hostname = NULL;
	}

	bfree(client);
	*handle = NULL;

	return 0;
}

#ifdef DHCPC_ENABLE_TEST_HOOKS
void dhcp_client_test_prepare(
	dhcp_client_t *client,
	const char *interface,
	const unsigned char mac[6],
	dhcp_client_event_handler_t handler,
	void *user_data)
{
	if (!client)
	{
		return;
	}

	memset(client, 0, sizeof(*client));
	client->interface = (char *)interface;
	client->event_handler = handler;
	client->user_data = user_data;
	client->state = INIT_SELECTING;
	client->quit = true;
	client->fd = -1;
	client->exit_sockets[0] = -1;
	client->exit_sockets[1] = -1;
	if (mac)
	{
		memcpy(client->arp, mac, 6);
	}
}

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
	uint32_t t2_seconds)
{
	if (!client)
	{
		return;
	}

	client->requested_ip = client_ip;
	client->server_addr = server_ip;
	client->subnet_addr = subnet;
	client->router_addr = router;
	client->dns0_addr = dns0;
	client->dns1_addr = dns1;
	client->lease_seconds = lease_seconds;
	client->t1_seconds = t1_seconds;
	client->t2_seconds = t2_seconds;
}

void dhcp_client_test_emit_event(
	dhcp_client_t *client,
	dhcp_client_event_t event,
	const char *reason)
{
	dhcp_client_emit_event(client, event, reason);
}

void dhcp_client_test_emit_nak_then_selecting(dhcp_client_t *client)
{
	if (!client)
	{
		return;
	}

	dhcp_client_emit_event(client, DHCPC_EVT_NAK, "test_nak");
	dhcp_client_clear_lease_info(client);
	client->state = REQUESTING;
	dhcp_client_state_change(client, INIT_SELECTING);
}

void dhcp_client_test_emit_lost_then_selecting(dhcp_client_t *client)
{
	if (!client)
	{
		return;
	}

	dhcp_client_emit_event(client, DHCPC_EVT_LOST, "test_lost");
	dhcp_client_clear_lease_info(client);
	client->state = REBINDING;
	dhcp_client_state_change(client, INIT_SELECTING);
}

void dhcp_client_test_emit_stopped(dhcp_client_t *client)
{
	if (!client)
	{
		return;
	}

	if (client->requested_ip)
	{
		dhcp_client_emit_event(client, DHCPC_EVT_RELEASED, "test_stop");
	}
	dhcp_client_emit_event(client, DHCPC_EVT_STOPPED, "test_stop");
}
#endif
