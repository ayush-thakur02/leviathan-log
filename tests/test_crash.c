/* SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "disk.h"
#include "fsck.h"
#include "tinyfs.h"
#include "test.h"

static int temporary(char *path, size_t capacity)
{
    return snprintf(path, capacity, "/tmp/tinyfs-crash-%ld.img", (long)getpid()) > 0;
}

int test_crash(void)
{
    TinyFs fs;
    Disk disk;
    TfsFsckReport report;
    char path[128];
    char err[128];
    uint32_t inode;
    uint8_t buffer[32];
    size_t size = 0;
    TEST_ASSERT(temporary(path, sizeof(path)));
    fs_init(&fs);
    TEST_ASSERT(fs_format(&fs, 32, 512) == 0);
    TEST_ASSERT(fs_create(&fs, "stable", &inode) == 0);
    TEST_ASSERT(fs_write(&fs, inode, "durable", 7) == 0);
    TEST_ASSERT(fs_sync(&fs, path) == 0);
    fs.super.head += 16u;
    TEST_ASSERT(fs_sync(&fs, path) == 0);
    fs_unmount(&fs);

    fs_init(&fs);
    TEST_ASSERT(disk_load(&fs.disk, path) == 0);
    TEST_ASSERT(fs_mount(&fs) == 0);
    TEST_ASSERT(fs_read(&fs, inode, buffer, sizeof(buffer), &size) == 0);
    TEST_ASSERT(size == 7 && memcmp(buffer, "durable", 7) == 0);
    fs_unmount(&fs);

    TEST_ASSERT(disk_load(&disk, path) == 0);
    TEST_ASSERT(fsck_run(&disk, 1, &report, err, sizeof(err)) == 0);
    TEST_ASSERT(report.damaged_records > 0);
    TEST_ASSERT(report.repaired == 1);
    TEST_ASSERT(disk_save(&disk, path) == 0);
    disk_free(&disk);

    fs_init(&fs);
    TEST_ASSERT(disk_load(&fs.disk, path) == 0);
    TEST_ASSERT(fs_mount(&fs) == 0);
    TEST_ASSERT(fs_read(&fs, inode, buffer, sizeof(buffer), &size) == 0);
    TEST_ASSERT(size == 7 && memcmp(buffer, "durable", 7) == 0);
    fs_unmount(&fs);
    TEST_ASSERT(remove(path) == 0);
    return 0;
}
