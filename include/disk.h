/* SPDX-License-Identifier: MIT */
#ifndef LEVIATHAN_LOG_DISK_H
#define LEVIATHAN_LOG_DISK_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *data;
    uint32_t blocks;
    uint32_t block_size;
} Disk;

int disk_init(Disk *disk, uint32_t blocks, uint32_t block_size);
void disk_free(Disk *disk);
int disk_write(Disk *disk, uint32_t block, const void *data);
int disk_read(const Disk *disk, uint32_t block, void *data);
int disk_save(const Disk *disk, const char *path);
int disk_load(Disk *disk, const char *path);

#endif /* LEVIATHAN_LOG_DISK_H */
