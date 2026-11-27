/* SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "leviathan-log.h"

static double now_ms(void)
{
    struct timespec value;
    (void)clock_gettime(CLOCK_MONOTONIC, &value);
    return (double)value.tv_sec * 1000.0 + (double)value.tv_nsec / 1.0e6;
}

int main(void)
{
    LeviathanFs fs;
    char path[128];
    uint8_t data[200];
    uint8_t buffer[256];
    double start;
    double write_ms;
    double read_ms;
    double compact_ms;
    fs_init(&fs);
    if (fs_format(&fs, 512, 512) != 0) {
        return 1;
    }
    (void)snprintf(path, sizeof(path), "/tmp/leviathan-log-bench-%ld.img",
                   (long)getpid());
    for (size_t index = 0; index < sizeof(data); ++index) {
        data[index] = (uint8_t)(index % 251u);
    }
    start = now_ms();
    for (uint32_t index = 0; index < 16; ++index) {
        char name[32];
        uint32_t inode;
        (void)snprintf(name, sizeof(name), "file-%u", index);
        if (fs_create(&fs, name, &inode) != 0 ||
            fs_write(&fs, inode, data, sizeof(data)) != 0) {
            return 1;
        }
    }
    write_ms = now_ms() - start;
    start = now_ms();
    for (uint32_t index = 0; index < 16; ++index) {
        size_t size = 0;
        if (fs_read(&fs, index, buffer, sizeof(buffer), &size) != 0 ||
            size != sizeof(data)) {
            return 1;
        }
    }
    read_ms = now_ms() - start;
    start = now_ms();
    if (fs_compact(&fs) != 0) {
        return 1;
    }
    compact_ms = now_ms() - start;
    (void)printf("operation,files,median_ms\n");
    (void)printf("write,16,%.4f\n", write_ms / 16.0);
    (void)printf("read,16,%.4f\n", read_ms / 16.0);
    (void)printf("compact,16,%.4f\n", compact_ms);
    fs_unmount(&fs);
    (void)remove(path);
    return 0;
}
