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
#include <stdalign.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "rbpf.h"
#include "rbpf/shared.h"
#include "rbpf/shared/utils.h"
#include "embUnit.h"
#include "xtimer.h"

#include "blob/rbpf/fletcher32_rbpf.bin.h"
#include "blob/rbpf/random_num.bin.h"
#include "blob/rbpf/coap.bin.h"

static const alignas(uint16_t) unsigned char wrap_around_data[] =
        "AD3Awn4kb6FtcsyE0RU25U7f55Yncn3LP3oEx9Gl4qr7iDW7I8L6Pbw9jNnh0sE4DmCKuc"
        "d1J8I34vn31W924y5GMS74vUrZQc08805aj4Tf66HgL1cO94os10V2s2GDQ825yNh9Yuq3"
        "QHcA60xl31rdA7WskVtCXI7ruH1A4qaR6Uk454hm401lLmv2cGWt5KTJmr93d3JsGaRRPs"
        "4HqYi4mFGowo8fWv48IcA3N89Z99nf0A0H2R6P0uI4Tir682Of3Rk78DUB2dIGQRRpdqVT"
        "tLhgfET2gUGU65V3edSwADMqRttI9JPVz8JS37g5QZj4Ax56rU1u0m0K8YUs57UYG5645n"
        "byNy4yqxu7";

static uint8_t _rbpf_stack[512];

typedef struct {
    __bpf_shared_ptr(const uint16_t *, data);
    uint32_t words;
} fletcher32_ctx_t;

static rbpf_mem_pool_t _pool;

static void tests_rbpf_allocator(void)
{
    memset(&_pool, 0, sizeof(_pool));
    int handle = rbpf_mem_pool_calloc_handle(&_pool, 20, 1);

    TEST_ASSERT(handle >= 0);

    rbpf_mem_pool_handle_t *phandle = rbpf_mem_pool_find_handle(&_pool, handle);
    TEST_ASSERT(phandle);
    TEST_ASSERT_EQUAL_INT(handle, phandle->num);
    TEST_ASSERT_EQUAL_INT(1, phandle->type);
    TEST_ASSERT_EQUAL_INT(20, phandle->req_size);

    void *obj = rbpf_mem_pool_obj_by_handle(&_pool, handle);
    TEST_ASSERT(phandle->ptr == obj);
}

static void tests_rbpf_header(void)
{
    rbpf_application_t rbpf;
    rbpf_application_setup(&rbpf, _rbpf_stack,
        (void*)fletcher32_rbpf_bin, sizeof(fletcher32_rbpf_bin));

    size_t num_functions = rbpf_application_num_functions(&rbpf);
    TEST_ASSERT_EQUAL_INT(2, num_functions);

    char name_buffer[32];
    ssize_t name_len = rbpf_application_get_function_name(&rbpf, 0, name_buffer, sizeof(name_buffer));

    static const char fletcher32_name[] = "fletcher32";

    TEST_ASSERT_EQUAL_INT(strlen(fletcher32_name), name_len);
    TEST_ASSERT(strcmp(fletcher32_name, name_buffer) == 0);

    static const char other_function_name[] = "second_function";
    name_len = rbpf_application_get_function_name(&rbpf, 1, name_buffer, sizeof(name_buffer));

    TEST_ASSERT_EQUAL_INT(strlen(other_function_name), name_len);
    TEST_ASSERT(strcmp(other_function_name, name_buffer) == 0);
}

static void tests_rbpf_run1(void)
{
    fletcher32_ctx_t ctx = {
        .data = (const uint16_t*)(uintptr_t)wrap_around_data,
        .words = sizeof(wrap_around_data)/2,
    };
    rbpf_application_t rbpf;
    rbpf_mem_region_t region;
    rbpf_application_setup(&rbpf, _rbpf_stack,
        (void*)fletcher32_rbpf_bin, sizeof(fletcher32_rbpf_bin));

    rbpf_memory_region_init(&region,
                   (void*)wrap_around_data, sizeof(wrap_around_data), RBPF_MEM_REGION_READ);
    rbpf_add_region(&rbpf, &region);
    int64_t result = 0;
    int res = rbpf_application_run_ctx(&rbpf, &ctx, sizeof(ctx), &result);

    TEST_ASSERT_EQUAL_INT(0, res);
    TEST_ASSERT_EQUAL_INT(0x5bac8c3d, (uint32_t)result);
}

static void tests_rbpf_run2(void)
{
    rbpf_application_t rbpf;
    rbpf_application_setup(&rbpf, _rbpf_stack,
        (void*)fletcher32_rbpf_bin, sizeof(fletcher32_rbpf_bin));

    int64_t result = 0;
    int res = rbpf_application_run_ctx_name_function(&rbpf, "second_function", NULL, 0, &result);

    TEST_ASSERT_EQUAL_INT(0, res);
    TEST_ASSERT_EQUAL_INT(5, result);
}

static void tests_rbpf_run3(void)
{
    rbpf_application_t rbpf;
    rbpf_application_setup(&rbpf, _rbpf_stack,
        (void*)random_num_bin, sizeof(random_num_bin));

    int64_t result = 0;
    int res = rbpf_application_run_ctx_name_function(&rbpf, "get_random_num", NULL, 0, &result);

    TEST_ASSERT_EQUAL_INT(0, res);
}

static void tests_rbpf_coap(void)
{
    rbpf_application_t rbpf;
    rbpf_application_setup(&rbpf, _rbpf_stack,
        (void*)coap_bin, sizeof(coap_bin));

    int64_t result = 0;
    int res = rbpf_application_run_ctx_name_function(&rbpf, "send_coap", NULL, 0, &result);

    TEST_ASSERT_EQUAL_INT(0, res);
}

Test *tests_bpf(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(tests_rbpf_allocator),
        new_TestFixture(tests_rbpf_header),
        new_TestFixture(tests_rbpf_run1),
        new_TestFixture(tests_rbpf_run2),
        new_TestFixture(tests_rbpf_run3),
        new_TestFixture(tests_rbpf_coap),
    };

    EMB_UNIT_TESTCALLER(bpf_tests, NULL, NULL, fixtures);
    return (Test*)&bpf_tests;
}

int main(void)
{
    TESTS_START();
    TESTS_RUN(tests_bpf());
    TESTS_END();

    return 0;
}
