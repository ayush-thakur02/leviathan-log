/* SPDX-License-Identifier: MIT */
#include "disk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int disk_init(Disk *disk, uint32_t blocks, uint32_t block_size)
{
    if (disk == NULL || blocks < 4 || block_size < 128) {
        return -1;
    }
    if (block_size != 0 && (size_t)blocks > SIZE_MAX / block_size) {
        return -1;
    }
    disk->data = (uint8_t *)calloc((size_t)blocks * block_size, 1);
    if (disk->data == NULL) {
        return -1;
    }
    disk->blocks = blocks;
    disk->block_size = block_size;
    return 0;
}

void disk_free(Disk *disk)
{
    if (disk == NULL) {
        return;
    }
    free(disk->data);
    memset(disk, 0, sizeof(*disk));
}

int disk_write(Disk *disk, uint32_t block, const void *data)
{
    if (disk == NULL || data == NULL || block >= disk->blocks) {
        return -1;
    }
    memcpy(disk->data + (size_t)block * disk->block_size, data,
           disk->block_size);
    return 0;
}

int disk_read(const Disk *disk, uint32_t block, void *data)
{
    if (disk == NULL || data == NULL || block >= disk->blocks) {
        return -1;
    }
    memcpy(data, disk->data + (size_t)block * disk->block_size,
           disk->block_size);
    return 0;
}

int disk_save(const Disk *disk, const char *path)
{
    FILE *file;
    size_t size;
    if (disk == NULL || path == NULL) {
        return -1;
    }
    size = (size_t)disk->blocks * disk->block_size;
    file = fopen(path, "wb");
    if (file == NULL) {
        return -1;
    }
    if (fwrite(disk->data, 1, size, file) != size) {
        fclose(file);
        return -1;
    }
    return fclose(file) == 0 ? 0 : -1;
}

int disk_load(Disk *disk, const char *path)
{
    FILE *file;
    long length;
    if (disk == NULL || path == NULL) {
        return -1;
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        return -1;
    }
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) <= 0) {
        fclose(file);
        return -1;
    }
    rewind(file);
    memset(disk, 0, sizeof(*disk));
    disk->data = (uint8_t *)malloc((size_t)length);
    if (disk->data == NULL || fread(disk->data, 1, (size_t)length, file) !=
                                  (size_t)length) {
        free(disk->data);
        memset(disk, 0, sizeof(*disk));
        fclose(file);
        return -1;
    }
    fclose(file);
    disk->block_size = 512;
    if ((size_t)length < 4 * disk->block_size ||
        (size_t)length % disk->block_size != 0) {
        free(disk->data);
        memset(disk, 0, sizeof(*disk));
        return -1;
    }
    disk->blocks = (uint32_t)((size_t)length / disk->block_size);
    return 0;
}
