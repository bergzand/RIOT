/*
 * Copyright (C) 2018 Dylan Laduranty
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup usb_acm Virtual Serial Port
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
#include "usb/plumbum.h"
#include "usb/message.h"

#include "usb/cdc.h"
#include "usb/cdc/acm.h"
#include "periph/uart.h"
#include "board.h"

#include <string.h>

#define ENABLE_DEBUG    (1)
#include "debug.h"

static int event_handler(plumbum_t *plumbum, plumbum_handler_t *handler, uint16_t event, void *arg);
static int _init(plumbum_t *plumbum, plumbum_handler_t *handler);

const plumbum_handler_driver_t cdc_driver = {
    .init = _init,
    .event_handler = event_handler,
};

static size_t _gen_call_mngt_descriptor(plumbum_t *plumbum, void *arg)
{
    (void)arg;
    usb_desc_call_mngt_t mngt;
    /* functional call management descriptor */
    mngt.length = sizeof(usb_desc_call_mngt_t);
    mngt.type = USB_TYPE_DESCRIPTOR_CDC;
    mngt.subtype = 0x1;
    mngt.capabalities = 0;
    mngt.data_if = 1;
    plumbum_put_bytes(plumbum, (uint8_t*)&mngt, sizeof(mngt));
    return sizeof(usb_desc_call_mngt_t);   
}

static size_t _call_mngt_size(plumbum_t *plumbum, void *arg)
{
    (void)plumbum;
    (void)arg;
    return sizeof(usb_desc_call_mngt_t);
}

static size_t _gen_union_descriptor(plumbum_t *plumbum, void *arg)
{
    (void)arg;
    usb_desc_union_t uni;
    /* functional union descriptor */
    uni.length = sizeof(usb_desc_union_t);
    uni.type = USB_TYPE_DESCRIPTOR_CDC;
    uni.subtype = 0x6;
    uni.master_if = 0;
    uni.slave_if = 1;
    plumbum_put_bytes(plumbum, (uint8_t*)&uni, sizeof(uni));
    return sizeof(usb_desc_union_t);   
}

static size_t _union_size(plumbum_t *plumbum, void *arg)
{
    (void)plumbum;
    (void)arg;
    return sizeof(usb_desc_union_t);
}

static size_t _gen_acm_descriptor(plumbum_t *plumbum, void *arg)
{
    (void)arg;
    usb_desc_acm_t acm;
    /* functional cdc acm descriptor */
    acm.length = sizeof(usb_desc_acm_t);
    acm.type = USB_TYPE_DESCRIPTOR_CDC;
    acm.subtype = 0x02;
    acm.capabalities = 0x00;
    plumbum_put_bytes(plumbum, (uint8_t*)&acm, sizeof(acm));
    return sizeof(usb_desc_acm_t);   
}

static size_t _acm_size(plumbum_t *plumbum, void *arg)
{
    (void)plumbum;
    (void)arg;
    return sizeof(usb_desc_acm_t);
}

static size_t _gen_cdc_descriptor(plumbum_t *plumbum, void *arg)
{
    (void)arg;
    usb_desc_cdc_t cdc;
    /* functional cdc descriptor */
    cdc.length = sizeof(usb_desc_cdc_t);
    cdc.bcd_hid = 0x0110;
    cdc.type = USB_TYPE_DESCRIPTOR_CDC;
    cdc.subtype = 0x00;
    plumbum_put_bytes(plumbum, (uint8_t*)&cdc, sizeof(cdc));
    return sizeof(usb_desc_cdc_t);
}

static size_t _cdc_size(plumbum_t *plumbum, void *arg)
{
    (void)plumbum;
    (void)arg;
    return sizeof(usb_desc_cdc_t);
}

static plumbum_cdc_device_t handler;

int cdc_init(plumbum_t *plumbum)
{
    memset(&handler, 0 , sizeof(plumbum_cdc_device_t));
    handler.handler_ctrl.driver = &cdc_driver;
    plumbum_register_event_handler(plumbum, (plumbum_handler_t*)&handler);
    return 0;
}

static int _init(plumbum_t *plumbum, plumbum_handler_t *handler)
{
    plumbum_cdc_device_t *cdc = (plumbum_cdc_device_t*)handler;

    cdc->call_mngt_hdr.next = NULL;
    cdc->call_mngt_hdr.gen_hdr = _gen_call_mngt_descriptor;
    cdc->call_mngt_hdr.hdr_len = _call_mngt_size;
    cdc->call_mngt_hdr.arg = cdc;

    cdc->union_hdr.next = &cdc->call_mngt_hdr;
    cdc->union_hdr.gen_hdr = _gen_union_descriptor;
    cdc->union_hdr.hdr_len = _union_size;
    cdc->union_hdr.arg = cdc;

    cdc->acm_hdr.next = &cdc->union_hdr;
    cdc->acm_hdr.gen_hdr = _gen_acm_descriptor;
    cdc->acm_hdr.hdr_len = _acm_size;
    cdc->acm_hdr.arg = cdc;

    cdc->cdc_hdr.next = &cdc->acm_hdr;
    cdc->cdc_hdr.gen_hdr = _gen_cdc_descriptor;
    cdc->cdc_hdr.hdr_len = _cdc_size;
    cdc->cdc_hdr.arg = cdc;

    /* Instantiate interfaces */
    memset(&cdc->iface_ctrl, 0, sizeof(plumbum_interface_t));
    memset(&cdc->iface_data, 0, sizeof(plumbum_interface_t));
    /* Configure Interface 0 as control interface */
    cdc->iface_ctrl.class = USB_CLASS_CDC_CONTROL ;
    cdc->iface_ctrl.subclass = USB_CDC_SUBCLASS_ACM;
    cdc->iface_ctrl.protocol = USB_CDC_PROTOCOL_NONE;
    cdc->iface_ctrl.hdr_gen = &cdc->cdc_hdr;
    cdc->iface_ctrl.handler = handler;
    cdc->iface_ctrl.idx = 0;
    /* Configure second interface to handle data endpoint */
    cdc->iface_data.class = USB_CLASS_CDC_DATA ;
    cdc->iface_data.subclass = USB_CDC_SUBCLASS_NONE;
    cdc->iface_data.protocol = USB_CDC_PROTOCOL_NONE;
    cdc->iface_data.hdr_gen = NULL;
    cdc->iface_data.handler = handler;
    cdc->iface_data.idx = 1;

    /* Create required endpoints */
    plumbum_add_endpoint(plumbum, &cdc->iface_ctrl, &cdc->ep_ctrl, USB_EP_TYPE_INTERRUPT, USB_EP_DIR_IN, 8);
    cdc->ep_ctrl.interval = 255;
    plumbum_add_endpoint(plumbum, &cdc->iface_data, &cdc->ep_data_in, USB_EP_TYPE_BULK, USB_EP_DIR_IN, 64);
    cdc->ep_data_in.interval = 0;
    plumbum_add_endpoint(plumbum, &cdc->iface_data, &cdc->ep_data_out, USB_EP_TYPE_BULK, USB_EP_DIR_OUT, 64);
    cdc->ep_data_out.interval = 0;
    /* Add interfaces to the stack */
    plumbum_add_interface(plumbum, &cdc->iface_data);
    plumbum_add_interface(plumbum, &cdc->iface_ctrl);

    cdc->ep_data_in.ep->driver->ready(cdc->ep_data_in.ep, 0);
    cdc->ep_data_out.ep->driver->ready(cdc->ep_data_out.ep, 0);
    cdc->ep_ctrl.ep->driver->ready(cdc->ep_ctrl.ep, 0);
    plumbum_enable_endpoint(&cdc->ep_data_in);
    plumbum_enable_endpoint(&cdc->ep_data_out);
    plumbum_enable_endpoint(&cdc->ep_ctrl);
   
    return 0;
}

static int _handle_setup(plumbum_t *plumbum, plumbum_handler_t *handler, usb_setup_t *pkt)
{
    (void)handler;
    DEBUG("Request:0x%x\n", pkt->request);
    switch(pkt->request) {
        case USB_SETUP_REQ_SET_LINE_CODING:
            DEBUG("Value:0x%x, interface:%d, len:%d\n",pkt->value, pkt->index, pkt->length);
            plumbum->in->driver->ready(plumbum->in, 0);
            return 0;
        case USB_SETUP_REQ_SET_CONTROL_LINE_STATE:
            DEBUG("Value:0x%x, interface:%d, nb:%d\n",pkt->value, pkt->index, pkt->length);
            plumbum->in->driver->ready(plumbum->in, 0);
            return 0;
        default:
            DEBUG("default handle setup rqt:0x%x\n", pkt->request);
            return -1;
    }
}

static int _handle_tr_complete(plumbum_t *plumbum, plumbum_handler_t *handler, usbdev_ep_t *ep)
{
    (void)ep;
    (void)handler;
    size_t len;
    plumbum_cdc_device_t *cdc = (plumbum_cdc_device_t*)plumbum->handler;
    usbdev_ep_t *out;

    /* Retrieve incoming data */
    ep->driver->get( ep, USBOPT_EP_AVAILABLE, &len, sizeof(size_t));
    ep->driver->ready( ep, 0);
    if (len > 0) {
        printf("DATA:%*.*s\n", len, len, (char*) ep->buf);
    }
    
    memset(cdc->ep_data_in.ep->buf, 0, len);
    out = cdc->ep_data_in.ep;
    memcpy(out->buf, ep->buf, len);
    out->driver->ready(out, len);
    return 0;
}

static int event_handler(plumbum_t *plumbum, plumbum_handler_t *handler, uint16_t event, void *arg)
{
    switch(event) {
        case PLUMBUM_MSG_TYPE_SETUP_RQ:
            return _handle_setup(plumbum, handler, (usb_setup_t*)arg);
        case PLUMBUM_MSG_TYPE_TR_COMPLETE:
            return _handle_tr_complete(plumbum, handler, (usbdev_ep_t*)arg);
        default:
            puts("Unhandled event :0x%x\n");
            return -1;
    }
    return 0;
}

