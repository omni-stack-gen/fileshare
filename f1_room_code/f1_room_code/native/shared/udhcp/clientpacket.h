#ifndef _CLIENTPACKET_H
#define _CLIENTPACKET_H

uint32_t random_xid(void);
int send_discover(dhcp_client_t *client);
int send_selecting(dhcp_client_t *client);
int send_renew(dhcp_client_t *client);
int send_release(dhcp_client_t *client);
int get_raw_packet(struct dhcpMessage *payload, int fd);

#endif
