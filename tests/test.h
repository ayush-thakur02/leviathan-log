/* SPDX-License-Identifier: MIT */
#ifndef TINYFS_TEST_H
#define TINYFS_TEST_H

#include <stdio.h>
#include <string.h>

#define TEST_ASSERT(condition)                                                \
    do {                                                                      \
        if (!(condition)) {                                                   \
            (void)fprintf(stderr, "  %s:%d assertion failed: %s\n", __FILE__, \
                          __LINE__, #condition);                              \
            return 1;                                                         \
        }                                                                     \
    } while (0)

#define TEST_ASSERT_STR_EQ(actual, expected)                                   \
    do {                                                                      \
        if (strcmp((actual), (expected)) != 0) {                              \
            (void)fprintf(stderr, "  %s:%d expected %s, got %s\n", __FILE__,  \
                          __LINE__, (expected), (actual));                    \
            return 1;                                                         \
        }                                                                     \
    } while (0)

#define RUN_TEST(test_function)                                               \
    do {                                                                      \
        int test_status = (test_function)();                                  \
        (void)printf("%s %s\n", test_status == 0 ? "ok  " : "FAIL",           \
                     #test_function);                                         \
        failures += test_status != 0;                                         \
    } while (0)

#endif /* TINYFS_TEST_H */
