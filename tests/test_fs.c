/* SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "disk.h"
#include "leviathan-log.h"
#include "test.h"

static int temporary(char *path, size_t capacity)
{
    return snprintf(path, capacity, "/tmp/leviathan-log-fs-%ld.img", (long)getpid()) > 0;
}

int test_fs(void)
{
    LeviathanFs fs;
    char path[128];
    uint32_t inode;
    uint32_t found;
    uint8_t buffer[64];
    size_t size = 0;
    TEST_ASSERT(temporary(path, sizeof(path)));
    fs_init(&fs);
    TEST_ASSERT(fs_format(&fs, 32, 512) == 0);
    TEST_ASSERT(fs_create(&fs, "hello", &inode) == 0);
    TEST_ASSERT(fs_write(&fs, inode, "world", 5) == 0);
    TEST_ASSERT(fs_read(&fs, inode, buffer, sizeof(buffer), &size) == 0);
    TEST_ASSERT(size == 5 && memcmp(buffer, "world", 5) == 0);
    TEST_ASSERT(fs_lookup(&fs, "hello", &found) == 0 && found == inode);
    TEST_ASSERT(fs_sync(&fs, path) == 0);
    fs_unmount(&fs);

    fs_init(&fs);
    TEST_ASSERT(disk_load(&fs.disk, path) == 0);
    TEST_ASSERT(fs_mount(&fs) == 0);
    TEST_ASSERT(fs_lookup(&fs, "hello", &found) == 0 && found == inode);
    TEST_ASSERT(fs_read(&fs, inode, buffer, sizeof(buffer), &size) == 0);
    TEST_ASSERT(size == 5 && memcmp(buffer, "world", 5) == 0);
    {
        uint32_t previous_active = fs.super.active;
        TEST_ASSERT(fs_compact(&fs) == 0);
        TEST_ASSERT(fs.super.active != previous_active);
        TEST_ASSERT(fs_sync(&fs, path) == 0);
    }
    fs_unmount(&fs);

    fs_init(&fs);
    TEST_ASSERT(disk_load(&fs.disk, path) == 0);
    TEST_ASSERT(fs_mount(&fs) == 0);
    TEST_ASSERT(fs_read(&fs, inode, buffer, sizeof(buffer), &size) == 0);
    TEST_ASSERT(size == 5 && memcmp(buffer, "world", 5) == 0);
    TEST_ASSERT(fs_unlink(&fs, inode) == 0);
    TEST_ASSERT(fs_lookup(&fs, "hello", &found) != 0);
    TEST_ASSERT(fs_sync(&fs, path) == 0);
    fs_unmount(&fs);
    TEST_ASSERT(remove(path) == 0);
    return 0;
}
