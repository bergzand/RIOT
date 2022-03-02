/*
 * Copyright (C) 2022 Inria
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
 * @brief       Pytorch package test
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 *
 * @}
 */

#include "embUnit.h"

static void test_pytorch_1(void)
{
    return;
}

Test *tests_pytorch(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_pytorch_1),
    };

    EMB_UNIT_TESTCALLER(pytorch_tests, NULL, NULL, fixtures);
    return (Test*)&pytorch_tests;
}

int main(void)
{
    TESTS_START();
    TESTS_RUN(tests_pytorch());
    TESTS_END();

    return 0;
}

