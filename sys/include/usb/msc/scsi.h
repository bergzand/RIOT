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

#define USB_SETUP_REQ_GET_MAX_LUN 0xFE


int mass_storage_init(usbus_t *usbus);

#ifdef __cplusplus
}
#endif

#endif /* USB_SCSI_H */
/** @} */