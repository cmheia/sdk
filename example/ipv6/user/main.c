/*****************************************************************************
*
* File Name : main.c
*
* Description: main
*
* Copyright (c) 2014 Winner Micro Electronic Design Co., Ltd.
* All rights reserved.
*
* Author : dave
*
* Date : 2014-6-14
*****************************************************************************/
#include "wm_include.h"
#include "map_marco.h"
#include <stdbool.h>

#define USER_TASK_STK_SIZE      512
#define USER_TASK_PRIO          32

#define LED_IO_LIST B_13, B_14, B_15, B_16, B_17, B_18

static u32 user_task_stk[USER_TASK_STK_SIZE];

#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))

int __counter = 0;

const enum tls_io_name led_io_map[] = {
#define XX(name) WM_IO_P##name
    MAP_LIST(XX, LED_IO_LIST),
#undef XX
};

const static char *led_io_name[] = {
#define XX(name) #name
    MAP_LIST(XX, LED_IO_LIST),
#undef XX
};

extern int tls_cmd_join_net(void);

void led_init(void)
{
    int id = 0;
    for (id = 0; ARRAYSIZE(led_io_map) > id; id++) {
        printf("led%d @ %s\r\n", id, led_io_name[id]);
        tls_gpio_cfg(
            led_io_map[id], WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_FLOATING);
    }
}

void led_ctl(int id, bool on)
{
    if (ARRAYSIZE(led_io_map) <= id) {
        id = 0;
    }
    tls_gpio_write(led_io_map[id], on);
//    printf("led%d -> %s\r\n", id, on ? "on" : "off");
}

void ipv6_init(struct netif *netif)
{
    printf("ipv6 on %c%c%d\r\n", netif->name[0], netif->name[1], netif->num);
    printf(__DATE__ " " __TIME__ "\r\n");

    tls_os_time_delay(HZ * 2);
    printf("join_net...");
    printf("%d\r\n", tls_cmd_join_net());
}

void ipv6_app(struct netif *netif)
{
#define PING_VALID_IP6_IDX 1
//#define PING_TARGET_IP6 "fd76:78e1:a097::1"
#define PING_TARGET_IP6 "fd76:78e1:a097:0:e1e6:26fb:aace:a686"

    static int ping_inited = 0;
    int        i;
    char       buf[IP6ADDR_STRLEN_MAX + 1];

    printf("\r\n IPv4 addr\r\n");
    printf("%s", inet_ntop(AF_INET, netif_ip4_addr(netif), buf, sizeof(buf)));
    printf(" / %s",
           inet_ntop(AF_INET, netif_ip4_netmask(netif), buf, sizeof(buf)));
    printf(" -> %s\r\n",
           inet_ntop(AF_INET, netif_ip4_gw(netif), buf, sizeof(buf)));
    printf(" IPv6 addr\r\n");
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        printf("%s (%s)\r\n",
               inet_ntop(AF_INET6, netif_ip_addr6(netif, i), buf, sizeof(buf)),
               get_ip6_addr_state(netif_ip6_addr_state(netif, i)));
    }
    if (!ping_inited
        && ip6_addr_ispreferred(
               netif_ip6_addr_state(netif, PING_VALID_IP6_IDX))) {
        ip_addr_t ip6;
        ping_inited = 1;
        inet_pton(AF_INET6, PING_TARGET_IP6, &ip6);
        ip6.type = IPADDR_TYPE_V6;
//        ping_init(&ip6);
    }
}

void app_task(void *data)
{
    int  id = 0;
    bool on = false;

    struct netif *netif;

    led_init();

    netif = tls_get_netif();

    ipv6_init(netif);

    while (1) {
        for (id = 0; ARRAYSIZE(led_io_map) > id; id++) {
            led_ctl(id, on);
            tls_os_time_delay(HZ / ARRAYSIZE(led_io_map) / 2);
        }
//        printf("%d\r\n", __counter++);
//        ipv6_app(netif);
        on = !on;
    }
}

void UserMain(void)
{
    /* create task */
    tls_os_task_create(NULL,
                       "ipv6",
                       app_task,
                       (void *) 0,
                       (void *) &user_task_stk, /* 任务栈的起始地址 */
                       USER_TASK_STK_SIZE * sizeof(u32), /* 任务栈的大小     */
                       USER_TASK_PRIO,
                       0);
}

