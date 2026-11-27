/* SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "disk.h"
#include "test.h"

int test_disk(void)
{
    Disk disk;
    uint8_t block[512];
    uint8_t readback[512];
    char path[128];
    TEST_ASSERT(disk_init(&disk, 8, 512) == 0);
    for (size_t index = 0; index < sizeof(block); ++index) {
        block[index] = (uint8_t)(index % 251u);
    }
    TEST_ASSERT(disk_write(&disk, 1, block) == 0);
    TEST_ASSERT(disk_read(&disk, 1, readback) == 0);
    TEST_ASSERT(memcmp(block, readback, sizeof(block)) == 0);
    TEST_ASSERT(disk_write(&disk, 8, block) != 0);
    TEST_ASSERT(disk_read(&disk, 9, readback) != 0);
    TEST_ASSERT(snprintf(path, sizeof(path), "/tmp/leviathan-log-disk-%ld.img",
                         (long)getpid()) > 0);
    TEST_ASSERT(disk_save(&disk, path) == 0);
    disk_free(&disk);
    TEST_ASSERT(disk_load(&disk, path) == 0);
    TEST_ASSERT(disk.blocks == 8 && disk.block_size == 512);
    TEST_ASSERT(disk_read(&disk, 1, readback) == 0);
    TEST_ASSERT(memcmp(block, readback, sizeof(block)) == 0);
    disk_free(&disk);
    TEST_ASSERT(remove(path) == 0);
    return 0;
}
