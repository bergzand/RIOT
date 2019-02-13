/*
 * Copyright (C) 2019 Mesotic SAS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup usb_msc Mass storage
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

static int event_handler(usbus_t *usbus, usbus_handler_t *handler, uint16_t event, void *arg);
static void _init(usbus_t *usbus, usbus_handler_t *handler);

static const usbus_handler_driver_t msc_driver = {
    .init = _init,
    .event_handler = event_handler,
};


static size_t _gen_msc_descriptor(usbus_t *usbus, void *arg)
{
    (void)arg;
    usb_desc_msc_t msc;
    /* functional msc descriptor */
    msc.length = sizeof(usb_desc_msc_t);
    //.bcd_hid = 0x0110;
    msc.type = USB_TYPE_DESCRIPTOR_MSC;
    msc.subtype = 0x00;
    usbus_ctrlslicer_put_bytes(usbus, (uint8_t*)&msc, sizeof(msc));
    return sizeof(usb_desc_msc_t);
}

static size_t _msc_size(usbus_t *usbus, void *arg)
{
    (void)usbus;
    (void)arg;
    return sizeof(usb_desc_msc_t);
}

static usbus_msc_device_t handler;

int msc_init(usbus_t *usbus)
{
    memset(&handler, 0, sizeof(usbus_msc_device_t));
    handler->usbusb = usbus;
    handler.handler_ctrl.driver = &cdc_driver;
    usbus_register_event_handler(usbus, (usbus_handler_t*)&handler);
    return 0;
}

static int _init(usbus_t *usbus, usbus_handler_t *handler)
{
    usbus_msc_device_t *msc = (usbus_msc_device_t*)handler;

    msc->msc_hdr.next = NULL;
    msc->msc_hdr.get_header = _gen_msc_descriptor;
    msc->msc_hdr.get_header_len = _msc_size;
    msc->msc_hdr.arg = msc;

    /* Instantiate interfaces */
    memset(&msc->iface, 0, sizeof(usbus_interface_t));
    /* Configure Interface 0 as control interface */
    msc->iface.class = USB_CLASS_MASS_STORAGE;
    msc->iface.subclass = USB_MSC_SUBCLASS_SCSI_TCS;
    msc->iface.protocol = 80;
    msc->iface.hdr_gen = NULL;
    msc->iface.handler = handler;
    msc->iface.idx = 0;

    /* Create required endpoints */
    usbus_add_endpoint(usbus, &msc->iface, &msc->ep_in, USB_EP_TYPE_BULK, USB_EP_DIR_IN, 64);
    msc->ep_in.interval = 20;
    usbus_add_endpoint(usbus, &msc->iface, &msc->ep_out, USB_EP_TYPE_BULK, USB_EP_DIR_OUT, 64);
    msc->ep_out.interval = 20;
      //  LED0_ON;
    /* Add interfaces to the stack */
    usbus_add_interface(usbus, &msc->iface);
  //  LED0_ON;
    //msc->ep_in.ep->driver->ready(msc->ep_in.ep, 0);
    msc->ep_out.ep->driver->ready(msc->ep_out.ep, 0);
        LED0_ON;
    usbus_enable_endpoint(&msc->ep_in);
    usbus_enable_endpoint(&msc->ep_out);
        LED0_ON;

    return 0;
}

static int _handle_setup(usbus_t *usbus, usbus_handler_t *handler, usb_setup_t *pkt)
{
    (void)handler;
    (void)usbus;
    uint8_t en[1] = {0};
    //DEBUG("Request:0x%x\n", pkt->request);
    switch(pkt->request) {
        case USB_SETUP_REQ_GET_MAX_LUN:
     //DEBUG("Type:0x%x, Request:0x%x Value:0x%x, interface:%d, nb:%d\n",pkt->type, pkt->request, pkt->value, pkt->index, pkt->length);
            usbus_put_bytes(usbus,(uint8_t*)&en,1);
            usbus->in->driver->ready(usbus->in, 1);
            //usbus->out->driver->ready(usbus->out, 1);
           // usbus->in->driver->set(usbus->in, USBOPT_EP_STALL, &en, sizeof(usbopt_enable_t));
            //usbus->out->driver->set(usbus->out, USBOPT_EP_STALL, &en, sizeof(usbopt_enable_t));
            //usbus->in->driver->ready(usbus->in, 0);
            return 0;
        default:
            DEBUG("default handle setup rqt:0x%x\n", pkt->request);
            return -1;
    }
}

static int _handle_tr_complete(usbus_t *usbus, usbus_handler_t *handler, usbdev_ep_t *ep)
{
    (void)ep;
    (void)handler;
    size_t len;
    usbus_msc_device_t *msc = (usbus_msc_device_t*)usbus->handler;
    usbdev_ep_t *out;
    puts("ONLYSHIT");

    /* Retrieve incoming data */
    ep->driver->get( ep, USBOPT_EP_AVAILABLE, &len, sizeof(size_t));
    ep->driver->ready( ep, 0);
    if (len > 0) {
        printf("DATA:%*.*s\n", len, len, (char*) ep->buf);
    }
    
    memset(msc->ep_in.ep->buf, 0, len);
    out = msc->ep_in.ep;
    memcpy(out->buf, ep->buf, len);
    out->driver->ready(out, len);
    return 0;
}

static int event_handler(usbus_t *usbus, usbus_handler_t *handler, uint16_t event, void *arg)
{
    puts("Huh");
    switch(event) {
        
        case USBUS_MSG_TYPE_SETUP_RQ:
            return _handle_setup(usbus, handler, (usb_setup_t*)arg);
        case USBUS_MSG_TYPE_TR_COMPLETE:
            return _handle_tr_complete(usbus, handler, (usbdev_ep_t*)arg);
        default:
            puts("Unhandled event :0x%x\n");
            return -1;
    }
    return 0;
}
