#include "lwip/opt.h"

#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#include "lwip/prot/ethernet.h"

#include "hexdump.h"

#include <string.h>

/*
 * Concatenate an option type and length field to the outgoing
 * DHCP message.
 *
 */
static u16_t dhcp_option(u16_t options_out_len,
                         u8_t *options,
                         u8_t  option_type,
                         u8_t  option_len)
{
    LWIP_ASSERT(
        "dhcp_option: options_out_len + 2 + option_len <= DHCP_OPTIONS_LEN",
        options_out_len + 2U + option_len <= DHCP_OPTIONS_LEN);
    options[options_out_len++] = option_type;
    options[options_out_len++] = option_len;
    return options_out_len;
}

/*
 * Concatenate a single byte to the outgoing DHCP message.
 *
 */
static u16_t dhcp_option_byte(u16_t options_out_len, u8_t *options, u8_t value)
{
    LWIP_ASSERT("dhcp_option_byte: options_out_len < DHCP_OPTIONS_LEN",
                options_out_len < DHCP_OPTIONS_LEN);
    options[options_out_len++] = value;
    return options_out_len;
}

#ifdef TLS_CONFIG_DHCP_OPTION60
void hook_dhcp4_append_options(struct netif *   netif,
                               struct dhcp *    dhcp,
                               u8_t             state,
                               struct dhcp_msg *msg,
                               u8_t             msg_type,
                               u16_t *          options_len_ptr)
{
    size_t optlen = strlen(TLS_CONFIG_DHCP_OPTION60);
    if (optlen > 0) {
        size_t      len;
        const char *p = TLS_CONFIG_DHCP_OPTION60;
        u16_t options_out_len = *options_len_ptr;
        /* Shrink len to available bytes (need 2 bytes for OPTION_US
           and 1 byte for trailer) */
        size_t available = DHCP_OPTIONS_LEN - options_out_len - 3;
        LWIP_ASSERT("DHCP: vid is too long!", optlen <= available);
        len = LWIP_MIN(optlen, available);
        LWIP_ASSERT("DHCP: vid is too long!", len <= 0xFF);
        options_out_len = dhcp_option(
            options_out_len, msg->options, DHCP_OPTION_US, (u8_t) len);
        while (len--) {
            options_out_len = dhcp_option_byte(options_out_len, msg->options, *p++);
        }
        *options_len_ptr = options_out_len;
    }
}
#endif

struct netif *
hook_arp_filter_netif(struct pbuf *p, struct netif *netif, u16_t type)
{
    char *pkt_type = NULL;

    const struct eth_hdr *ethhdr = p->payload;

    switch (type) {
    case ETHTYPE_ARP:
        pkt_type = "ARP";
        break;
    case ETHTYPE_IP:
        if (IP_PROTO_ICMP == IPH_PROTO((struct ip_hdr *) &ethhdr[1])) {
            pkt_type = "ICMP";
        }
        break;
    case ETHTYPE_IPV6:
        pkt_type = "IPv6";
        break;
    default:
        pkt_type = NULL;
        break;
    }

    if (pkt_type) {
        char if_name[8];

        sprintf(if_name, "%c%c%d", netif->name[0], netif->name[1], netif->num);
        LWIP_DEBUGF(LWIP_DBG_ON,
                    ("%s packet (%" U16_F "/%" U16_F ")\n",
                     pkt_type,
                     p->len,
                     p->tot_len));
        hexdump(if_name, &ethhdr->dest, p->len - ETH_PAD_SIZE);
    }

    return netif;
}
