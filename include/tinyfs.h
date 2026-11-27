/* SPDX-License-Identifier: MIT */
#ifndef TINYFS_TINYFS_H
#define TINYFS_TINYFS_H

#include <stddef.h>
#include <stdint.h>

#include "disk.h"
#include "log.h"

#define TFS_MAGIC 0x54465331u
#define TFS_VERSION 1u
#define TFS_MAX_INODES 32

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t block_size;
    uint32_t blocks;
    uint32_t area_start[2];
    uint32_t area_end[2];
    uint32_t active;
    uint32_t head;
    uint32_t seq;
} __attribute__((packed)) TfsSuperblock;

typedef struct {
    int used;
    uint32_t inode;
    char name[TFS_MAX_NAME];
    uint8_t *data;
    size_t size;
} TfsInode;

typedef struct {
    Disk disk;
    TfsSuperblock super;
    TfsInode inodes[TFS_MAX_INODES];
    int mounted;
} TinyFs;

void fs_init(TinyFs *fs);
int fs_format(TinyFs *fs, uint32_t blocks, uint32_t block_size);
int fs_mount(TinyFs *fs);
void fs_unmount(TinyFs *fs);
int fs_create(TinyFs *fs, const char *name, uint32_t *inode);
int fs_write(TinyFs *fs, uint32_t inode, const void *data, size_t size);
int fs_read(const TinyFs *fs, uint32_t inode, void *buffer, size_t capacity,
            size_t *size);
int fs_unlink(TinyFs *fs, uint32_t inode);
int fs_compact(TinyFs *fs);
int fs_lookup(const TinyFs *fs, const char *name, uint32_t *inode);
const TfsInode *fs_inode(const TinyFs *fs, uint32_t inode);
int fs_sync(TinyFs *fs, const char *path);

#endif /* TINYFS_TINYFS_H */
