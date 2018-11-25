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
    printf("led%d -> %s\r\n", id, on ? "on" : "off");
}

void blink_task(void *data)
{
    int  id = 0;
    bool on = false;

    led_init();

    while (1) {
        for (id = 0; ARRAYSIZE(led_io_map) > id; id++) {
            led_ctl(id, on);
            tls_os_time_delay(HZ / ARRAYSIZE(led_io_map) / 2);
        }
        printf("%d\r\n", __counter++);
        on = !on;
    }
}

void empty_fn(void)
{
    ;
}

void UserMain(void)
{
    empty_fn();
    printf("blinky!\r\n");
    printf(__DATE__ " " __TIME__ "\r\n");
    printf("GO!\r\n");
    empty_fn();

    /* create task */
    tls_os_task_create(NULL,
                       "blinky",
                       blink_task,
                       (void *) 0,
                       (void *) &user_task_stk,
                       USER_TASK_STK_SIZE * sizeof(u32),
                       USER_TASK_PRIO,
                       0);
}
