/*
 * Copyright (C) 2023 Freie Universität Berlin
 * Copyright (C) 2023 Inria
 * Copyright (C) 2023 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    pkg_wamr_user_saul WAMR SAUL bindings
 * @ingroup     pkg_wamr_user
 * @brief       WAMR user SAUL api
 * @{
 *
 * @file
 * @brief Interface definitions for WAMR SAUL functions
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef WAMR_USER_SAUL_H
#define WAMR_USER_SAUL_H


#ifdef __cplusplus
extern "C" {
#endif

int saul_reg_find_nth(int pos);
int saul_reg_find_type(uint8_t type);
float saul_reg_read(int ref);
int saul_reg_write(int ref, float val);

#ifdef __cplusplus
}
#endif
#endif /* WAMR_USER_SAUL_H */

/** @} */
