/* serverpacket.c
 *
 * Constuct and send DHCP server packets
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

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "packet.h"
#include "debug.h"
#include "dhcpd.h"
#include "options.h"
#include "leases.h"

/* send a packet to giaddr using the kernel ip stack */
static int send_packet_to_relay(dhcp_server_t *server, struct dhcpMessage *payload)
{
	DEBUG(LOG_INFO, "Forwarding packet to relay");

	return kernel_packet(payload, server->server_nip, SERVER_PORT,
	                     payload->giaddr, SERVER_PORT);
}


/* send a packet to a specific arp address and ip address by creating our own ip packet */
static int send_packet_to_client(dhcp_server_t *server, struct dhcpMessage *payload, int force_broadcast)
{
	unsigned char *chaddr;
	u_int32_t ciaddr;

	if (force_broadcast)
	{
		DEBUG(LOG_INFO, "broadcasting packet to client (NAK)");
		ciaddr = INADDR_BROADCAST;
		chaddr = MAC_BCAST_ADDR;
	}
	else if (payload->ciaddr)
	{
		DEBUG(LOG_INFO, "unicasting packet to client ciaddr");
		ciaddr = payload->ciaddr;
		chaddr = payload->chaddr;
	}
	else if (ntohs(payload->flags) & BROADCAST_FLAG)
	{
		DEBUG(LOG_INFO, "broadcasting packet to client (requested)");
		ciaddr = INADDR_BROADCAST;
		chaddr = MAC_BCAST_ADDR;
	}
	else
	{
		DEBUG(LOG_INFO, "unicasting packet to client yiaddr");
		ciaddr = payload->yiaddr;
		chaddr = payload->chaddr;
	}

	return raw_packet(payload, server->server_nip, SERVER_PORT,
	                  ciaddr, CLIENT_PORT, chaddr, server->ifindex);
}


/* send a dhcp packet, if force broadcast is set, the packet will be broadcast to the client */
static int send_packet(dhcp_server_t *server, struct dhcpMessage *payload, int force_broadcast)
{
	int ret;

	if (payload->giaddr)
	{
		ret = send_packet_to_relay(server, payload);
	}
	else
	{
		ret = send_packet_to_client(server, payload, force_broadcast);
	}

	return ret;
}


static void init_packet(dhcp_server_t *server, struct dhcpMessage *packet, struct dhcpMessage *oldpacket, char type)
{
	init_header(packet, type);
	packet->xid = oldpacket->xid;
	memcpy(packet->chaddr, oldpacket->chaddr, 16);
	packet->flags = oldpacket->flags;
	packet->giaddr = oldpacket->giaddr;
	packet->ciaddr = oldpacket->ciaddr;
	add_simple_option(packet->options, DHCP_SERVER_ID, server->server_nip);
}


/* add in the bootp options */
static void add_bootp_options(dhcp_server_t *server, struct dhcpMessage *packet)
{
	packet->siaddr = server->siaddr;

	if (server->sname)
	{
		strncpy((char *)packet->sname, server->sname, sizeof(packet->sname) - 1);
	}

	if (server->boot_file)
	{
		strncpy((char *)packet->file, server->boot_file, sizeof(packet->file) - 1);
	}
}


/* send a DHCP OFFER to a DHCP DISCOVER */
int sendOffer(dhcp_server_t *server, struct dhcpMessage *oldpacket)
{
	struct dhcpMessage packet;
	struct dhcpOfferedAddr *lease = NULL;
	u_int32_t req_align, lease_time_align = server->lease;
	unsigned char *req, *lease_time;
	struct option_set *curr;
	struct in_addr addr;
	unsigned int gateway = 0, netmask = 0;

	init_packet(server, &packet, oldpacket, DHCPOFFER);

	/* ADDME: if static, short circuit */
	/* the client is in our lease/offered table */
	if ((lease = find_lease_by_chaddr(server, oldpacket->chaddr)))
	{
		if (!lease_expired(lease))
		{
			lease_time_align = lease->expires - monotonic_sec();
		}

		packet.yiaddr = lease->yiaddr;

		/* Or the client has a requested ip */
	}
	else if ((req = get_option(oldpacket, DHCP_REQUESTED_IP)) &&

	         /* Don't look here (ugly hackish thing to do) */
	         memcpy(&req_align, req, 4) &&

	         /* and the ip is in the lease range */
	         ntohl(req_align) >= ntohl(server->start_ip) &&
	         ntohl(req_align) <= ntohl(server->end_ip) &&

	         /* and its not already taken/offered */ /* ADDME: check that its not a static lease */
	         ((!(lease = find_lease_by_yiaddr(server, req_align)) ||

	           /* or its taken, but expired */ /* ADDME: or maybe in here */
	           lease_expired(lease))))
	{
		packet.yiaddr = req_align; /* FIXME: oh my, is there a host using this IP? */

		/* otherwise, find a free IP */ /*ADDME: is it a static lease? */
	}
	else
	{
		packet.yiaddr = find_address(server,  0);

		/* try for an expired lease */
		if (!packet.yiaddr)
		{
			packet.yiaddr = find_address(server,  1);
		}
	}

	if (!packet.yiaddr)
	{
		LOGW("no IP addresses to give -- OFFER abandoned\n");
		return -1;
	}

	if (!add_lease(server, packet.chaddr, packet.yiaddr, server->offer_time))
	{
		LOGW("lease pool is full -- OFFER abandoned\n");
		return -1;
	}

	if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME)))
	{
		memcpy(&lease_time_align, lease_time, 4);
		lease_time_align = ntohl(lease_time_align);

		if (lease_time_align > server->lease)
		{
			lease_time_align = server->lease;
		}
	}

	/* Make sure we aren't just using the lease time from the previous offer */
	if (lease_time_align < server->min_lease)
	{
		lease_time_align = server->lease;
	}

	/* ADDME: end of short circuit */
	add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(lease_time_align));
	add_simple_option(packet.options, DHCP_SUBNET, server->server_netmask);
	add_simple_option(packet.options, DHCP_ROUTER, server->server_gateway);
	add_simple_option(packet.options, DHCP_DNS_SERVER, server->server_dns);

	curr = server->options;

	while (curr)
	{
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
		{
			add_option_string(packet.options, curr->data);
		}

		curr = curr->next;
	}

	add_bootp_options(server, &packet);

	addr.s_addr = packet.yiaddr;
	LOGI("sending OFFER of %s\n", inet_ntoa(addr));
	return send_packet(server, &packet, 0);
}


int sendNAK(dhcp_server_t *server, struct dhcpMessage *oldpacket)
{
	struct dhcpMessage packet;

	init_packet(server, &packet, oldpacket, DHCPNAK);

	DEBUG(LOG_INFO, "sending NAK");
	return send_packet(server, &packet, 1);
}


int sendACK(dhcp_server_t *server, struct dhcpMessage *oldpacket, u_int32_t yiaddr)
{
	struct dhcpMessage packet;
	struct option_set *curr;
	unsigned char *lease_time;
	u_int32_t lease_time_align = server->lease;
	struct in_addr addr;
	unsigned int gateway = 0, netmask = 0;

	init_packet(server, &packet, oldpacket, DHCPACK);
	packet.yiaddr = yiaddr;

	if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME)))
	{
		memcpy(&lease_time_align, lease_time, 4);
		lease_time_align = ntohl(lease_time_align);

		if (lease_time_align > server->lease)
		{
			lease_time_align = server->lease;
		}
		else if (lease_time_align < server->min_lease)
		{
			lease_time_align = server->lease;
		}
	}

	add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(lease_time_align));
	add_simple_option(packet.options, DHCP_SUBNET, server->server_netmask);
	add_simple_option(packet.options, DHCP_ROUTER, server->server_gateway);
	add_simple_option(packet.options, DHCP_DNS_SERVER, server->server_dns);

	curr = server->options;

	while (curr)
	{
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
		{
			add_option_string(packet.options, curr->data);
		}

		curr = curr->next;
	}

	add_bootp_options(server, &packet);

	addr.s_addr = packet.yiaddr;
	if (!add_lease(server, packet.chaddr, packet.yiaddr, lease_time_align))
	{
		return -1;
	}

	dhcp_server_emit_lease_event(server,
	                             DHCPD_LEASE_EVENT_BOUND,
	                             packet.chaddr,
	                             packet.yiaddr,
	                             lease_time_align);

	LOGI("sending ACK to %s\n", inet_ntoa(addr));

	if (send_packet(server, &packet, 0) < 0)
	{
		return -1;
	}

	return 0;
}


int send_inform(dhcp_server_t *server, struct dhcpMessage *oldpacket)
{
	struct dhcpMessage packet;
	struct option_set *curr;

	init_packet(server, &packet, oldpacket, DHCPACK);

	curr = server->options;

	while (curr)
	{
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
		{
			add_option_string(packet.options, curr->data);
		}

		curr = curr->next;
	}

	add_bootp_options(server, &packet);

	return send_packet(server, &packet, 0);
}

