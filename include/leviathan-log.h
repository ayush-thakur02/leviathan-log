/* SPDX-License-Identifier: MIT */
#ifndef LEVIATHAN_LOG_LEVIATHAN_LOG_H
#define LEVIATHAN_LOG_LEVIATHAN_LOG_H

#include <stddef.h>
#include <stdint.h>

#include "disk.h"
#include "log.h"

#define LEVIATHAN_MAGIC 0x54465331u
#define LEVIATHAN_VERSION 1u
#define LEVIATHAN_MAX_INODES 32

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
} __attribute__((packed)) LeviathanSuperblock;

typedef struct {
    int used;
    uint32_t inode;
    char name[LEVIATHAN_MAX_NAME];
    uint8_t *data;
    size_t size;
} LeviathanInode;

typedef struct {
    Disk disk;
    LeviathanSuperblock super;
    LeviathanInode inodes[LEVIATHAN_MAX_INODES];
    int mounted;
} LeviathanFs;

void fs_init(LeviathanFs *fs);
int fs_format(LeviathanFs *fs, uint32_t blocks, uint32_t block_size);
int fs_mount(LeviathanFs *fs);
void fs_unmount(LeviathanFs *fs);
int fs_create(LeviathanFs *fs, const char *name, uint32_t *inode);
int fs_write(LeviathanFs *fs, uint32_t inode, const void *data, size_t size);
int fs_read(const LeviathanFs *fs, uint32_t inode, void *buffer, size_t capacity,
            size_t *size);
int fs_unlink(LeviathanFs *fs, uint32_t inode);
int fs_compact(LeviathanFs *fs);
int fs_lookup(const LeviathanFs *fs, const char *name, uint32_t *inode);
const LeviathanInode *fs_inode(const LeviathanFs *fs, uint32_t inode);
int fs_sync(LeviathanFs *fs, const char *path);

#endif /* LEVIATHAN_LOG_LEVIATHAN_LOG_H */
