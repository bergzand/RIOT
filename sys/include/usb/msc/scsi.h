/*
 * Copyright (C) 2019 Mesotic SAS
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for
 * more details.
 */

#ifndef USB_SCSI_H
#define USB_SCSI_H

#ifdef __cplusplus
extern "c" {
#endif

#define USB_SETUP_REQ_GET_MAX_LUN       0xFE
#define USB_SETUP_REQ_RESET             0xFF

/* SCSI Commands */
#define SCSI_TEST_UNIT_READY            0x00
#define SCSI_REQUEST_SENSE              0x03
#define SCSI_FORMAT_UNIT                0x04
#define SCSI_INQUIRY                    0x12
#define SCSI_MODE_SELECT6               0x15
#define SCSI_MODE_SENSE6                0x1A
#define SCSI_START_STOP_UNIT            0x1B
#define SCSI_MEDIA_REMOVAL              0x1E
#define SCSI_READ_FORMAT_CAPACITIES     0x23
#define SCSI_READ_CAPACITY              0x25
#define SCSI_READ10                     0x28
#define SCSI_WRITE10                    0x2A
#define SCSI_VERIFY10                   0x2F
#define SCSI_MODE_SELECT10              0x55
#define SCSI_MODE_SENSE10               0x5A

#define SCSI_CBW_SIGNATURE              0x55534243
#define SCSI_CSW_SIGNATURE              0x55534253

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t removable;
    uint16_t version;
    uint8_t length;
    uint8_t tmp[3];
    uint8_t vendor_id[8];
    uint8_t product_id[16];
    uint8_t product_rev[4];
} msc_inquiry_pkt_t;
/* Bulk-only Command Block Wrapper */
typedef struct __attribute__((packed)) {
    uint32_t signature;
    uint32_t tag;
    uint32_t data_len;
    uint8_t  flags;
    uint8_t  lun;
    uint8_t  CB_len;
    uint8_t  CB[16];
} msc_cbw_buf_t;

/* Bulk-only Command Status Wrapper */
typedef struct __attribute__((packed)) {
    uint32_t signature;
    uint32_t tag;
    uint32_t data_left;
    uint8_t  status;
} msc_csw_buf_t;

int mass_storage_init(usbus_t *usbus);

int scsi_process_cmd(usbus_t *usbus, usbus_handler_t *handler, usbdev_ep_t *ep, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* USB_SCSI_H */
/** @} */