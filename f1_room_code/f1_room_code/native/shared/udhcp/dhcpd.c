/* dhcpd.c
 *
 * udhcp Server
 * Copyright (C) 1999 Matthew Ramsay <matthewr@moreton.com.au>
 *          Chris Trew <ctrew@moreton.com.au>
 *
 * Rewrite by Russ Dill <Russ.Dill@asu.edu> July 2001
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

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "debug.h"
#include "dhcpd.h"
#include "arpping.h"
#include "socket.h"
#include "options.h"
#include "leases.h"
#include "packet.h"
#include "serverpacket.h"

#include <utils/bmem.h>
#include <utils/thread_helper.h>

#include <net_utils/ip_config.h>

#define DHCPD_ADDR_START         "192.168.1.100"
#define DHCPD_ADDR_END           "192.168.1.104"
#define DHCPD_INTERFACE          "eth0"
#define DHCPD_REMAIN             "yes"
#define DHCPD_MAX_LEASES         (254)
#define DHCPD_AUTO_TIME          (7200)
#define DHCPD_DECLINE_TIME       (3600)
#define DHCPD_CONFLICT_TIME      (3600)
#define DHCPD_OFFER_TIME         (60)
#define DHCPD_MIN_LEASE          (60)
#define DHCPD_SIADDR             "0.0.0.0"
#define DHCPD_SNAME              "IOT"

static void dhcp_server_use_default_init_config(dhcp_server_t *server)
{
	server->start_ip      = inet_addr(DHCPD_ADDR_START);
	server->end_ip        = inet_addr(DHCPD_ADDR_END);
	server->siaddr        = inet_addr(DHCPD_SIADDR);
	server->interface     = bstrdup(DHCPD_INTERFACE);
	server->remaining     = strcasecmp("yes", DHCPD_REMAIN) ? 0 : 1;
	server->max_leases    = DHCPD_MAX_LEASES;
	server->auto_time     = DHCPD_AUTO_TIME;
	server->decline_time  = DHCPD_DECLINE_TIME;
	server->conflict_time = DHCPD_CONFLICT_TIME;
	server->offer_time    = DHCPD_OFFER_TIME;
	server->min_lease     = DHCPD_MIN_LEASE;
	server->lease         = LEASE_TIME;
	server->sname         = NULL;
}

void dhcp_server_emit_lease_event(dhcp_server_t *server,
                                  dhcp_server_lease_event_t event,
                                  const uint8_t *chaddr,
                                  uint32_t yiaddr,
                                  uint32_t lease_seconds)
{
	dhcp_server_lease_event_info_t info;

	if (!server || !server->event_handler || !chaddr)
	{
		return;
	}

	memset(&info, 0, sizeof(info));
	info.interface = server->interface;
	memcpy(info.chaddr, chaddr, sizeof(info.chaddr));
	info.yiaddr = yiaddr;
	info.server_nip = server->server_nip;
	info.lease_seconds = lease_seconds;

	server->event_handler(event, &info, server->user_data);
}

static void *dhcp_server_thread(void *arg)
{
	os_thread_set_name("dhcpd");

	dhcp_server_t *server = (dhcp_server_t *)arg;

	fd_set rfds;
	struct timeval tv;
	int bytes, retval;
	struct dhcpMessage packet;
	unsigned char *msg_type;
	unsigned char *server_id, *requested;
	uint32_t server_id_align, requested_align;
	struct dhcpOfferedAddr *lease;
	int max_fd;

	DEBUG(LOG_INFO, "dhcp server started");

	while (1)  /* loop until universe collapses */
	{
		if (server->fd < 0)
		{
			if ((server->fd = listen_socket(INADDR_ANY, SERVER_PORT, server->interface)) < 0)
			{
				LOGE("FATAL: couldn't create server socket, %s\n", strerror(errno));
				goto exit_server;
			}
		}

		FD_ZERO(&rfds);
		FD_SET(server->fd, &rfds);
		FD_SET(server->exit_sockets[0], &rfds);

		max_fd = server->exit_sockets[0] > server->fd ? server->exit_sockets[0] : server->fd;

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		retval = select(max_fd + 1, &rfds, NULL, NULL, &tv);

		if (retval == 0)
		{
			continue;
		}
		else if (retval < 0 && errno != EINTR)
		{
			LOGE("Error on select retval:%d error:%d\n", retval, errno);
			continue;
		}

		if (FD_ISSET(server->exit_sockets[0], &rfds))
		{
			char quit;

			if (read(server->exit_sockets[0], &quit, sizeof(quit)) < 0)
			{
				DEBUG(LOG_ERR, "Could not read exit_sockets: %s", strerror(errno));
			}

			break;
		}

		if (FD_ISSET(server->fd, &rfds))
		{
			if ((bytes = get_packet(&packet, server->fd)) < 0)   /* this waits for a packet - idle */
			{
				if (bytes == -1 && errno != EINTR)
				{
					LOGE("error on read, %s, reopening socket \n", strerror(errno));
					close(server->fd);
					server->fd = -1;
				}

				continue;
			}

			if (packet.hlen != 6)
			{
				DEBUG(LOG_ERR, "MAC length != 6, ignoring packet");
				continue;
			}

			if (packet.op != BOOTREQUEST)
			{
				DEBUG(LOG_ERR, "not a REQUEST, ignoring packet");
				continue;
			}

			msg_type = get_option(&packet, DHCP_MESSAGE_TYPE);

			if (!msg_type || msg_type[0] < DHCP_MINTYPE || msg_type[0] > DHCP_MAXTYPE)
			{
				DEBUG(LOG_ERR, "no or bad message type option, ignoring packet");
				continue;
			}

			/* ADDME: look for a static lease */
			lease = find_lease_by_chaddr(server, packet.chaddr);

			DEBUG(LOG_INFO, "MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
			      packet.chaddr[0], packet.chaddr[1],
			      packet.chaddr[2], packet.chaddr[3],
			      packet.chaddr[4], packet.chaddr[5]);

			switch (msg_type[0])
			{
				case DHCPDISCOVER:
					DEBUG(LOG_INFO, "received DISCOVER");

					if (sendOffer(server, &packet) < 0)
					{
						DEBUG(LOG_ERR, "send OFFER failed");
					}

					break;

				case DHCPREQUEST:
					DEBUG(LOG_INFO, "received REQUEST");
					requested = get_option(&packet, DHCP_REQUESTED_IP);
					server_id = get_option(&packet, DHCP_SERVER_ID);

					if (requested)
					{
						memcpy(&requested_align, requested, 4);
						DEBUG(LOG_INFO, "requeste id = %s", inet_ntoa(*(struct in_addr *)&requested_align));
					}

					if (server_id)
					{
						memcpy(&server_id_align, server_id, 4);
						DEBUG(LOG_INFO, "server id = %s", inet_ntoa(*(struct in_addr *)&server_id_align));
					}

					if (lease)   /*ADDME: or static lease */
					{
						if (server_id)
						{
							/* SELECTING State */
							DEBUG(LOG_INFO, "server_id = %08x", ntohl(server_id_align));

							if (server_id_align == server->server_nip && requested &&
							    requested_align == lease->yiaddr)
							{
								sendACK(server, &packet, lease->yiaddr);
							}
						}
						else
						{
							if (requested)
							{
								/* INIT-REBOOT State */
								if (lease->yiaddr == requested_align)
								{
									sendACK(server, &packet, lease->yiaddr);
								}
								else
								{
									sendNAK(server, &packet);
								}
							}
							else
							{
								/* RENEWING or REBINDING State */
								if (lease->yiaddr == packet.ciaddr)
								{
									sendACK(server, &packet, lease->yiaddr);
								}
								else
								{
									/* don't know what to do!!!! */
									sendNAK(server, &packet);
								}
							}
						}

						/* what to do if we have no record of the server */
					}
					else if (server_id)
					{
						/* SELECTING State */

					}
					else if (requested)
					{
						/* INIT-REBOOT State */
						if ((lease = find_lease_by_yiaddr(server, requested_align)))
						{
							if (lease_expired(lease))
							{
								/* probably best if we drop this lease */
								memset(lease->chaddr, 0, 16);
								/* make some contention for this address */
							}
							else
							{
								sendNAK(server, &packet);
							}
						}
						else
						{
							sendNAK(server, &packet);
						}
					}
					else
					{
						/* RENEWING or REBINDING State */
					}

					break;

				case DHCPDECLINE:
					DEBUG(LOG_INFO, "received DECLINE");

					if (lease)
					{
						memset(lease->chaddr, 0, 16);
						lease->expires = monotonic_sec() + server->decline_time;
						dhcp_server_emit_lease_event(server,
						                             DHCPD_LEASE_EVENT_DECLINED,
						                             packet.chaddr,
						                             lease->yiaddr,
						                             server->decline_time);
					}

					break;

				case DHCPRELEASE:
					DEBUG(LOG_INFO, "received RELEASE");

					if (lease)
					{
						lease->expires = monotonic_sec();
						dhcp_server_emit_lease_event(server,
						                             DHCPD_LEASE_EVENT_RELEASED,
						                             packet.chaddr,
						                             lease->yiaddr,
						                             0);
					}

					break;

				case DHCPINFORM:
					DEBUG(LOG_INFO, "received INFORM");
					send_inform(server, &packet);
					break;

				default:
					DEBUG(LOG_WARNING, "unsupported DHCP message (%02x) -- ignoring", msg_type[0]);
			}
		}
	}

exit_server:

	if (server->fd >= 0)
	{
		close(server->fd);
		server->fd = -1;
	}

	DEBUG(LOG_INFO, "dhcp server stoped");

	return NULL;
}

void *dhcp_server_create(void)
{
	dhcp_server_t *server = NULL;

	ip_static_config_t ip_param;

	server = bmalloc(sizeof(dhcp_server_t));

	if (server == NULL)
	{
		LOGE("malloc failed!\n");
		goto __exit;
	}

	memset(server, 0, sizeof(dhcp_server_t));

	dhcp_server_use_default_init_config(server);

	server->fd = -1;
	server->exit_sockets[0] = server->exit_sockets[1] = -1;

	server->quit = true;

#if 0

	if (read_interface(server->interface, &server->ifindex,
	                   &server->server_nip, server->arp) < 0)
	{
		LOGE("read_interface failed!\n");
		goto __exit;
	}

	memset(&ip_param, 0, sizeof(ip_param));

	net_dev_get_static_ip(server->interface, &ip_param);

	memcpy(&server->server_netmask, ip_param.netmask, sizeof(server->server_netmask));
	memcpy(&server->server_gateway, ip_param.gateway, sizeof(server->server_gateway));
	memcpy(&server->server_dns, ip_param.gateway, sizeof(server->server_gateway));
#endif

	return server;

__exit:

	if (server)
	{
		if (server->interface)
		{
			bfree(server->interface);
			server->interface = NULL;
		}

		bfree(server);
	}

	return NULL;
}

int dhcp_server_setopt(void *handle, dhcp_server_option_t option, void *data)
{
	int ret = 0;

	dhcp_server_t *server = (dhcp_server_t *)handle;

	if (server == NULL || data == NULL)
	{
		return -1;
	}

	if (option >= DHCPD_OPT_MAX)
	{
		return -1;
	}

	switch (option)
	{
		case DHCPD_OPT_INTERFACE:
			{
				if (server->interface)
				{
					bfree(server->interface);
					server->interface = NULL;
				}

				server->interface = bstrdup((const char *)data);

				ret = read_interface(server->interface, &server->ifindex,
				                     &server->server_nip, server->arp);

#if 0
				ip_static_config_t ip_param;

				memset(&ip_param, 0, sizeof(ip_param));

				net_dev_get_static_ip(server->interface, &ip_param);

				memcpy(&server->server_netmask, ip_param.netmask, sizeof(server->server_netmask));
				memcpy(&server->server_gateway, ip_param.gateway, sizeof(server->server_gateway));
				memcpy(&server->server_dns, ip_param.gateway, sizeof(server->server_gateway));
#endif
			}
			break;

		case DHCPD_OPT_SERVER_IP:
			{
				server->server_nip = inet_addr((const char *)data);
			}
			break;

		case DHCPD_OPT_START_IP:
			{
				server->start_ip = inet_addr((const char *)data);
			}
			break;

		case DHCPD_OPT_END_IP:
			{
				server->end_ip = inet_addr((const char *)data);
			}
			break;

		case DHCPD_OPT_SUBNET:
			{
				server->server_netmask = inet_addr((const char *)data);
			}
			break;

		case DHCPD_OPT_ROUTER:
			{
				server->server_gateway = inet_addr((const char *)data);
			}
			break;

		case DHCPD_OPT_DNS:
			{
				server->server_dns = inet_addr((const char *)data);
			}
			break;

		case DHCPD_OPT_MAX_LEASE_TIME:
			{
				server->lease = *(uint32_t *)data;
			}
			break;

		case DHCPD_OPT_EVENT_HANDLER:
			{
				server->event_handler = (dhcp_server_event_handler_t)data;
			}
			break;

		case DHCPD_OPT_USER_DATA:
			{
				server->user_data = data;
			}
			break;

		default:
			LOGE("unknow dhcp server option:%d \n", option);
			break;
	}

	return ret;
}

int dhcp_server_start(void *handle)
{
	int ret = 0;

	dhcp_server_t *server = (dhcp_server_t *)handle;

	if (server == NULL)
	{
		return -1;
	}

	if (server->quit)
	{
		server->quit = false;

		if (ntohl(server->start_ip) > ntohl(server->end_ip))
		{
			LOGE("bad start/end IP range start_ip:0x%x end_ip:0x%x \n", server->start_ip, server->end_ip);
			goto __exit;
		}

		server->max_leases = ntohl(server->end_ip) - ntohl(server->start_ip) + 1;

		server->leases = bmalloc(sizeof(struct dhcpOfferedAddr) * server->max_leases);

		if (server->leases == NULL)
		{
			LOGE("malloc fail \n");
			goto __exit;
		}

		memset(server->leases, 0, sizeof(struct dhcpOfferedAddr) * server->max_leases);

		ret = socketpair(AF_UNIX, SOCK_STREAM, 0, server->exit_sockets);

		if (ret != 0)
		{
			LOGE("create socketpair failed! \n");
			goto __exit;
		}

		ret = pthread_create(&server->thread,
		                     NULL,
		                     dhcp_server_thread,
							 server);

		if (ret != 0)
		{
			LOGE("create thread failed! \n");
			goto __exit;
		}
	}

	return ret;

__exit:

	dhcp_server_stop(server);

	return ret;
}

int dhcp_server_stop(void *handle)
{
	int ret = 0;

	dhcp_server_t *server = (dhcp_server_t *)handle;

	if (server == NULL)
	{
		return -1;
	}

	server->quit = true;

	if (server->thread)
	{
		if (server->exit_sockets[1] >= 0)
		{
			write(server->exit_sockets[1], "q", 1);
		}

		pthread_join(server->thread, NULL);
		server->thread = 0;
	}

	if (server->exit_sockets[0] >= 0)
	{
		close(server->exit_sockets[0]);
		server->exit_sockets[0] = -1;
	}

	if (server->exit_sockets[1] >= 0)
	{
		close(server->exit_sockets[1]);
		server->exit_sockets[1] = -1;
	}

	if (server->leases)
	{
		bfree(server->leases);
		server->leases = NULL;
	}

	return 0;
}

int dhcp_server_destroy(void **handle)
{
	int ret = 0;

	if (handle == NULL || *handle == NULL)
	{
		return -1;
	}

	dhcp_server_t *server = *(dhcp_server_t **)handle;

	if (server->quit == false)
	{
		dhcp_server_stop(server);
	}

	if (server->interface)
	{
		bfree(server->interface);
		server->interface = NULL;
	}

	bfree(server);
	*handle = NULL;

	return 0;
}
