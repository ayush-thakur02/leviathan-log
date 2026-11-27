/* SPDX-License-Identifier: MIT */
#include <stdio.h>

#include "test.h"

int test_disk(void);
int test_fs(void);
int test_crash(void);

int main(void)
{
    int failures = 0;
    RUN_TEST(test_disk);
    RUN_TEST(test_fs);
    RUN_TEST(test_crash);
    (void)printf("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
