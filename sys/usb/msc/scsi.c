/*
 * Copyright (C) 2019 Mesotic SAS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup usb_scsi Mass storage
 * @{
 * @file
 *
 * @author  Dylan Laduranty <dylan.laduranty@mesotic.com>
 * @}
 */

#include "thread.h"
#include "kernel_types.h"
#include "msg.h"
#include "mutex.h"
#include "usb/usbus.h"
#include "usb/descriptor.h"
#include "usb/usbopt.h"

#include "usb/msc/msc.h"
#include "usb/msc/scsi.h"
#include "board.h"

#include <string.h>

#define ENABLE_DEBUG    (1)
#include "debug.h"


int scsi_process_cmd(usbus_t *usbus, usbus_handler_t *handler, usbdev_ep_t *ep,size_t len) {
    (void)usbus;
    (void)handler;
    /* store data into specific struct */
    msc_cbw_buf_t *cbw = (msc_cbw_buf_t*) ep->buf;
    if (len == sizeof(msc_cbw_buf_t)) {
        puts("Command Block Wrapper");

    }
    else {
        printf("error receiving, ep->len:%d should be %d\n",len,sizeof(msc_cbw_buf_t));
    }
    switch(cbw->CB[0]) {
        case SCSI_TEST_UNIT_READY:
            puts("TODO: SCSI_TEST_UNIT_READY");
            break;
        case SCSI_REQUEST_SENSE:
            break;
        case SCSI_FORMAT_UNIT:
            break;
        case SCSI_INQUIRY:
            puts("TODO: SCSI_INQUIRY");
            break;
        case SCSI_START_STOP_UNIT:
            break;
        case SCSI_MEDIA_REMOVAL:
            break;
        case SCSI_MODE_SELECT6:
            break;
        case SCSI_MODE_SENSE6:
            break;
        case SCSI_MODE_SELECT10:
            break;
        case SCSI_MODE_SENSE10:
            break;
        case SCSI_READ_FORMAT_CAPACITIES:
            break;
        case SCSI_READ_CAPACITY:
            break;
        case SCSI_READ10:
            break;
        case SCSI_WRITE10:
            break;
        case SCSI_VERIFY10:
            break;
        default:
            puts("Unhandled SCSI command");
    }
    return 0;
}