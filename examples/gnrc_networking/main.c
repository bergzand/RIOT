/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Example application for demonstrating the RIOT network stack
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

#include "shell.h"
#include "msg.h"
#include "sam_usb.h"
#include "usb/usbman.h"
#include "cpu.h"

#define USBMAN_STACKSIZE           (THREAD_STACKSIZE_DEFAULT)
#define USBMAN_PRIO                (THREAD_PRIORITY_MAIN - 6)
#define USBMAN_TNAME               "usb"

extern const usbdev_driver_t driver;
static char _stack[USBMAN_STACKSIZE];
static sam0_common_usb_t usbdev;

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

extern int udp_cmd(int argc, char **argv);

static const shell_command_t shell_commands[] = {
    { "udp", "send data over UDP and listen on UDP ports", udp_cmd },
    { NULL, NULL, NULL }
};

int main(void)
{
    usbdev.usbdev.driver = &driver;
    usbman_create(_stack, USBMAN_STACKSIZE, USBMAN_PRIO,
                   USBMAN_TNAME, &usbdev.usbdev );
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("RIOT network stack example application");

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
