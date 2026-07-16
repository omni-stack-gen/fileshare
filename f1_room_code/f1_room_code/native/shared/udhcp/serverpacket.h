#ifndef _SERVERPACKET_H
#define _SERVERPACKET_H


int sendOffer(dhcp_server_t *server, struct dhcpMessage *oldpacket);
int sendNAK(dhcp_server_t *server, struct dhcpMessage *oldpacket);
int sendACK(dhcp_server_t *server, struct dhcpMessage *oldpacket, u_int32_t yiaddr);
int send_inform(dhcp_server_t *server, struct dhcpMessage *oldpacket);


#endif
