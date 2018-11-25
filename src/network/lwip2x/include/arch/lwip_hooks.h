#ifndef __LWIP_HOOKS_H
#define __LWIP_HOOKS_H

#include "lwip/opt.h"

#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"

void hook_dhcp4_append_options(struct netif *   netif,
                               struct dhcp *    dhcp,
                               u8_t             state,
                               struct dhcp_msg *msg,
                               u8_t             msg_type,
                               u16_t *          options_len_ptr);

struct netif *
hook_arp_filter_netif(struct pbuf *pbuf, struct netif *netif, u16_t type);

#endif
