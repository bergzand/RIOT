/*
 * Copyright (C) 2020 Inria
 * Copyright (C) 2020 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       Tests bpf virtual machine
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 *
 * @}
 */
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "bpf.h"
#include "bpf/shared.h"
#include "embUnit.h"
#include "xtimer.h"
#include "random.h"

#include "blob/bpf/memcpy_bpf.bin.h"
#define TEST_DATA_LEN       128

static uint8_t _bpf_stack[512];

static uint8_t src_data[TEST_DATA_LEN];
static uint8_t dst_data[TEST_DATA_LEN];

typedef struct {
    __bpf_shared_ptr(const uint8_t *, src);
    __bpf_shared_ptr(uint8_t *, dst);
    uint32_t len;
} memcpy_ctx_t;

static void _init(void)
{
    bpf_init();
    random_bytes(src_data, TEST_DATA_LEN);
}

static void tests_bpf_run1(void)
{
    memcpy_ctx_t ctx = {
        .src = src_data,
        .dst = dst_data,
        .len = TEST_DATA_LEN,
    };
    bpf_t bpf = {
        .application = memcpy_bpf_bin,
        .application_len = sizeof(memcpy_bpf_bin),
        .stack = _bpf_stack,
        .stack_size = sizeof(_bpf_stack),
    };
    bpf_mem_region_t read_region, write_region;
    printf("bpf context size: %u, memory region size: %u\n", (unsigned)sizeof(bpf_t), (unsigned)sizeof(bpf_mem_region_t));
    bpf_setup(&bpf);

    bpf_add_region(&bpf, &read_region,
                   (void*)src_data, TEST_DATA_LEN, BPF_MEM_REGION_READ);
    bpf_add_region(&bpf, &write_region,
                   (void*)dst_data, TEST_DATA_LEN, BPF_MEM_REGION_WRITE);
    int64_t result = 0;
    uint32_t start = xtimer_now_usec();
    int res = 0;
    for (unsigned i = 0; i < 1000; i++) {
        res = bpf_execute_ctx(&bpf, &ctx, sizeof(ctx), &result);
    }
    uint32_t stop = xtimer_now_usec();

    TEST_ASSERT_EQUAL_INT(0, res);
    TEST_ASSERT_EQUAL_INT(0, memcmp(src_data, dst_data, TEST_DATA_LEN));
    printf("duration: %"PRIu32" us -> %"PRIu32" us/exec\n",
           (stop - start), (stop - start)/1000);
}

Test *tests_bpf(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(tests_bpf_run1),
    };

    EMB_UNIT_TESTCALLER(bpf_tests, _init, NULL, fixtures);
    return (Test*)&bpf_tests;
}

int main(void)
{
    TESTS_START();
    TESTS_RUN(tests_bpf());
    TESTS_END();

    return 0;
}
