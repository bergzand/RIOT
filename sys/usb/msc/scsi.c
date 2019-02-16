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

#define VENDOR_ID "RIOT-OS"
#define PRODUCT_ID "RIOT_MSC_DISK"
#define PRODUCT_REV " 1.0"

void _scsi_inquiry(usbus_handler_t *handler, usbdev_ep_t *ep) {

    (void)ep;
    usbus_msc_device_t *msc = (usbus_msc_device_t*)handler;
    msc_inquiry_pkt_t pkt;
    size_t len = sizeof(msc_inquiry_pkt_t);
    memset(&pkt, 0, len);

    /* prepare pkt response */
    pkt.removable = 0x80;
    pkt.version = 0x0001;
    pkt.length = len - 4;
    pkt.tmp[0] = 0x80;

    memcpy(&pkt.vendor_id, VENDOR_ID, 8);
    memcpy(&pkt.product_id, PRODUCT_ID, 16);
    memcpy(&pkt.product_rev, PRODUCT_REV, 4);

    /* copy into ep buffer */
    memcpy(msc->ep_in.ep->buf, &pkt, len);
    usbdev_ep_ready(msc->ep_in.ep, len);
    return;
}

int scsi_process_cmd(usbus_t *usbus, usbus_handler_t *handler, usbdev_ep_t *ep, size_t len) {
    (void)usbus;

    if (len == sizeof(msc_cbw_buf_t)) {
        puts("Command Block Wrapper");
    }
    else {
        printf("error receiving, ep->len:%d should be %d\n",len,sizeof(msc_cbw_buf_t));
        return -1;
    }

    /* store data into specific struct */
    msc_cbw_buf_t *cbw = (msc_cbw_buf_t*) ep->buf;

    /* Check Command Block signature */
    if (cbw->signature != SCSI_CBW_SIGNATURE) {
        printf("Invalid CBW signature:0x%lx, abort", cbw->signature);
        //return -1;
    }

    switch(cbw->CB[0]) {
        case SCSI_TEST_UNIT_READY:
            puts("TODO: SCSI_TEST_UNIT_READY");
            break;
        case SCSI_REQUEST_SENSE:
            puts("TODO: SCSI_REQUEST_SENSE");
            break;
        case SCSI_FORMAT_UNIT:
            puts("TODO: SCSI_FORMAT_UNIT");
            break;
        case SCSI_INQUIRY:
            _scsi_inquiry(handler, ep);
            puts("TODO: SCSI_INQUIRY");
            break;
        case SCSI_START_STOP_UNIT:
            puts("TODO: SCSI_START_STOP_UNIT");
            break;
        case SCSI_MEDIA_REMOVAL:
            puts("TODO: SCSI_MEDIA_REMOVAL");
            break;
        case SCSI_MODE_SELECT6:
            puts("TODO: SCSI_MODE_SELECT6");
            break;
        case SCSI_MODE_SENSE6:
            puts("TODO: SCSI_MODE_SENSE6");
            break;
        case SCSI_MODE_SELECT10:
            puts("TODO: SCSI_MODE_SELECT10");
            break;
        case SCSI_MODE_SENSE10:
            puts("TODO: SCSI_MODE_SENSE10");
            break;
        case SCSI_READ_FORMAT_CAPACITIES:
            puts("TODO: SCSI_READ_FORMAT_CAPACITIES");
            break;
        case SCSI_READ_CAPACITY:
            puts("TODO: SCSI_READ_CAPACITY");
            break;
        case SCSI_READ10:
            puts("TODO: SCSI_READ10");
            break;
        case SCSI_WRITE10:
            puts("TODO: SCSI_WRITE10");
            break;
        case SCSI_VERIFY10:
            puts("TODO: SCSI_VERIFY10");
            break;
        default:
            printf("Unhandled SCSI command:0x%x", cbw->CB[0]);
    }
    return 0;
}