/*
 * Copyright (c) 2020 Koen Zandberg <koen@bergzand.net>
 * Copyright (c) 2020 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_nrf52_common
 * @{
 *
 * @file        qspi.c
 * @brief       Low-level QSPI peripheral implementation
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 *
 * @}
 */

#include <string.h>

#include "cpu.h"
#include "periph_conf.h"
#include "periph/gpio.h"
#include "periph/qspi.h"
#include "macros/units.h"
#include "bitarithm.h"
#include "mutex.h"

static mutex_t _qspi_mutex = MUTEX_INIT;

static inline NRF_QSPI_Type *dev(qspi_t bus)
{
    return qspi_config[bus].dev;
}

static inline void _wait_rdy(qspi_t bus)
{
    while (!(dev(bus)->EVENTS_READY)) {}
    dev(bus)->EVENTS_READY = 0;
}

//static void _enable(qspi_t bus)
//{
//    (void)bus;
////    dev(bus)->ENABLE = 1;
////    dev(bus)->TASKS_ACTIVATE = 1;
////    _wait_rdy(bus);
//}

//static void _disable(qspi_t bus)
//{
//    dev(bus)->ENABLE = 0;
//    dev(bus)->TASKS_DEACTIVATE = 1;
//    _wait_rdy(bus);
//}

static unsigned _divider(uint32_t freq)
{
    /* bounds check */
    if (freq > MHZ(32)) {
        return 0;
    }
    unsigned res =((MHZ(32) / freq) - 1) << QSPI_IFCONFIG1_SCKFREQ_Pos;
    /* More bounds check */
    return res > QSPI_IFCONFIG1_SCKFREQ_Msk ? QSPI_IFCONFIG1_SCKFREQ_Msk : res;
}

void qspi_cmd_read(qspi_t bus, uint8_t command, void *response, size_t len)
{
    bool lfmode = (len > 8);
    do {
        uint32_t additional = 0;
        if (lfmode) {
            if (len < 8) {
                /* Stop */
                additional |= QSPI_CINSTRCONF_LFSTOP_Msk;
            }
            /* Add lfmode */
            additional |= QSPI_CINSTRCONF_LFEN_Msk;
        }
        unsigned transfer_len = len > 8 ? 8 : len;
        dev(bus)->CINSTRCONF = (transfer_len + 1) << QSPI_CINSTRCONF_LENGTH_Pos |
                               QSPI_CINSTRCONF_LIO2_Msk |
                               QSPI_CINSTRCONF_LIO3_Msk |
                               command |
                               additional;
        _wait_rdy(bus);
        uint32_t cmd_data[2] = { dev(bus)->CINSTRDAT0, dev(bus)->CINSTRDAT1 };
        memcpy(response, cmd_data, transfer_len);
        response += transfer_len;
        len -= transfer_len;
    } while (len);

}

void qspi_cmd_write(qspi_t bus, uint8_t command, const void *data, size_t len)
{
    assert(len <= 8);

    /* TODO: handle long command writes */

    uint32_t cmd_data[2] = { 0 };
    memcpy(cmd_data, data, len);

    dev(bus)->CINSTRDAT0 = cmd_data[0];
    dev(bus)->CINSTRDAT1 = cmd_data[1];

    uint32_t instruction = ((len + 1) << QSPI_CINSTRCONF_LENGTH_Pos) |
                           command;
    dev(bus)->CINSTRCONF = instruction;


    _wait_rdy(bus);
}

void qspi_read(qspi_t bus, uint32_t addr, void *data, size_t len)
{
    assert((len % 4) == 0);
    assert(((uintptr_t)data % 4) == 0);
    assert((addr % 4) == 0);

    dev(bus)->READ.DST = (uint32_t)data;
    dev(bus)->READ.SRC = addr;
    dev(bus)->READ.CNT = len;

    dev(bus)->TASKS_READSTART = 1;

    _wait_rdy(bus);
}

void qspi_erase(qspi_t bus, uint8_t command, uint32_t address)
{
    assert((address % 4) == 0);
    dev(bus)->ERASE.PTR = address;
    if (command == SFLASH_CMD_ERASE_SECTOR) {
        dev(bus)->ERASE.LEN = 0;
    }
    else if (command == SFLASH_CMD_ERASE_BLOCK) {
        dev(bus)->ERASE.LEN = 1;
    }
    else if (command == SFLASH_CMD_ERASE_CHIP) {
        dev(bus)->ERASE.LEN = 2;
    }
    else {
        /* No other option supported */
        assert(false);
    }

    dev(bus)->TASKS_ERASESTART = 1;

    _wait_rdy(bus);
}

void qspi_write(qspi_t bus, uint32_t addr, const void *data, size_t len)
{
    assert(((uintptr_t)data % 4) == 0);
    assert((len % 4) == 0);
    assert((addr % 4) == 0);
    dev(bus)->WRITE.DST = addr;
    dev(bus)->WRITE.SRC = (uint32_t)data;
    dev(bus)->WRITE.CNT = len;

    dev(bus)->TASKS_WRITESTART = 1;

    _wait_rdy(bus);
}

void qspi_acquire(qspi_t bus)
{
    (void)bus;

    mutex_lock(&_qspi_mutex);

    //_enable(bus);
}

void qspi_release(qspi_t bus)
{
    (void)bus;

    mutex_unlock(&_qspi_mutex);

    //_disable(bus);
}

void qspi_init(qspi_t bus)
{
    const qspi_conf_t *conf = &qspi_config[bus];
    static const gpio_mode_t mode = GPIO_MODE(1, 1, 0, 3); /**< High drive configuration */

    gpio_init(conf->sclk, mode);
    gpio_init(conf->cs, mode);
    gpio_init(conf->io0, mode);
    gpio_init(conf->io1, mode);
    gpio_init(conf->io2, GPIO_OUT);
    gpio_init(conf->io3, GPIO_OUT);

    dev(bus)->PSEL.SCK = conf->sclk;
    dev(bus)->PSEL.CSN = conf->cs;
    dev(bus)->PSEL.IO0 = conf->io0;
    dev(bus)->PSEL.IO1 = conf->io1;
    //dev(bus)->PSEL.IO2 = conf->io2;
    //dev(bus)->PSEL.IO3 = conf->io3;

    gpio_set(conf->io2);
    gpio_set(conf->io3);

}

void qspi_configure(qspi_t bus, qspi_mode_t mode, uint8_t addrlen, uint32_t clk_hz)
{
    assert(addrlen == 3 || addrlen == 4);
    assert(mode == QSPI_MODE_0 || mode == QSPI_MODE_3);
    assert(clk_hz <= CLOCK_CORECLOCK/2);

    dev(bus)->ADDRCONF |= QSPI_ADDRCONF_WREN_Pos;
    dev(bus)->IFCONFIG1 = _divider(clk_hz) |
                          (mode == QSPI_MODE_3 ? QSPI_IFCONFIG1_SPIMODE_Msk : 0) |
                          5; /* TODO: sane value for the high period */

    /* make sure to keep the memory peripheral in sync */
    /* set addr len needs write enable                 */
    //qspi_cmd_write(bus, SFLASH_CMD_WRITE_ENABLE, NULL, 0);

    unsigned command = SFLASH_CMD_3_BYTE_ADDR;

    if (addrlen == 3) {
        dev(bus)->IFCONFIG0 &= ~QSPI_IFCONFIG0_ADDRMODE_Msk;
    }

    if (addrlen == 4) {
        dev(bus)->IFCONFIG0 |= QSPI_IFCONFIG0_ADDRMODE_Msk;
        command = SFLASH_CMD_4_BYTE_ADDR;
    }
    dev(bus)->ENABLE = 1;
    dev(bus)->TASKS_ACTIVATE = 1;
    _wait_rdy(bus);
    qspi_cmd_write(bus, command, NULL, 0);
}
