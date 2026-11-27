/* SPDX-License-Identifier: MIT */
#include "leviathan-log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int store_super(LeviathanFs *fs)
{
    if (fs == NULL || fs->disk.data == NULL) {
        return -1;
    }
    memcpy(fs->disk.data, &fs->super, sizeof(fs->super));
    return 0;
}

static int load_super(LeviathanFs *fs)
{
    if (fs == NULL || fs->disk.data == NULL) {
        return -1;
    }
    memcpy(&fs->super, fs->disk.data, sizeof(fs->super));
    if (fs->super.magic != LEVIATHAN_MAGIC || fs->super.version != LEVIATHAN_VERSION ||
        fs->super.block_size != fs->disk.block_size ||
        fs->super.blocks != fs->disk.blocks || fs->super.active > 1 ||
        fs->super.area_start[0] >= fs->super.area_end[0] ||
        fs->super.area_start[1] >= fs->super.area_end[1] ||
        fs->super.head < fs->super.area_start[fs->super.active] ||
        fs->super.head > fs->super.area_end[fs->super.active]) {
        return -1;
    }
    return 0;
}

static LeviathanInode *find_inode(LeviathanFs *fs, uint32_t inode)
{
    if (fs == NULL || inode >= LEVIATHAN_MAX_INODES || !fs->inodes[inode].used) {
        return NULL;
    }
    return &fs->inodes[inode];
}

static const LeviathanInode *find_inode_const(const LeviathanFs *fs, uint32_t inode)
{
    if (fs == NULL || inode >= LEVIATHAN_MAX_INODES || !fs->inodes[inode].used) {
        return NULL;
    }
    return &fs->inodes[inode];
}

static int inode_name_exists(const LeviathanFs *fs, const char *name)
{
    for (size_t index = 0; index < LEVIATHAN_MAX_INODES; ++index) {
        if (fs->inodes[index].used &&
            strcmp(fs->inodes[index].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

static void apply_create(LeviathanFs *fs, uint32_t inode, const char *name,
                         size_t name_len)
{
    if (inode >= LEVIATHAN_MAX_INODES) {
        return;
    }
    LeviathanInode *target = &fs->inodes[inode];
    free(target->data);
    memset(target, 0, sizeof(*target));
    target->used = 1;
    target->inode = inode;
    if (name_len >= LEVIATHAN_MAX_NAME) {
        name_len = LEVIATHAN_MAX_NAME - 1u;
    }
    memcpy(target->name, name, name_len);
    target->name[name_len] = '\0';
}

static void apply_write(LeviathanFs *fs, uint32_t inode, const uint8_t *data,
                        size_t data_len)
{
    LeviathanInode *target = find_inode(fs, inode);
    if (target == NULL || data_len > LEVIATHAN_MAX_FILE) {
        return;
    }
    uint8_t *copy = NULL;
    if (data_len > 0) {
        copy = (uint8_t *)malloc(data_len);
        if (copy == NULL) {
            return;
        }
        memcpy(copy, data, data_len);
    }
    free(target->data);
    target->data = copy;
    target->size = data_len;
}

static void apply_unlink(LeviathanFs *fs, uint32_t inode)
{
    LeviathanInode *target = find_inode(fs, inode);
    if (target == NULL) {
        return;
    }
    free(target->data);
    memset(target, 0, sizeof(*target));
}

static int replay(LeviathanFs *fs, int report_errors)
{
    uint32_t position = fs->super.area_start[fs->super.active];
    uint32_t limit = fs->super.head;
    while (position < limit) {
        LeviathanRecordHeader header;
        const uint8_t *name;
        const uint8_t *data;
        int result = tfs_log_next(&fs->disk, limit, &position, &header, &name,
                                  &data);
        if (result <= 0) {
            if (report_errors && result < 0) {
                return -1;
            }
            break;
        }
        if (header.type == LEVIATHAN_RECORD_CREATE) {
            char name_buffer[LEVIATHAN_MAX_NAME];
            size_t name_len = header.name_len;
            if (name_len >= LEVIATHAN_MAX_NAME) {
                continue;
            }
            memcpy(name_buffer, name, name_len);
            name_buffer[name_len] = '\0';
            apply_create(fs, header.inode, name_buffer, name_len);
        } else if (header.type == LEVIATHAN_RECORD_WRITE) {
            apply_write(fs, header.inode, data, header.data_len);
        } else if (header.type == LEVIATHAN_RECORD_UNLINK) {
            apply_unlink(fs, header.inode);
        }
        fs->super.seq = header.seq + 1u;
    }
    return 0;
}

void fs_init(LeviathanFs *fs)
{
    if (fs != NULL) {
        memset(fs, 0, sizeof(*fs));
    }
}

int fs_format(LeviathanFs *fs, uint32_t blocks, uint32_t block_size)
{
    uint32_t usable;
    uint32_t half;
    if (fs == NULL || blocks < 4) {
        return -1;
    }
    fs_init(fs);
    if (disk_init(&fs->disk, blocks, block_size) != 0) {
        return -1;
    }
    usable = blocks - 1u;
    half = usable / 2u;
    if (half == 0) {
        disk_free(&fs->disk);
        return -1;
    }
    memset(&fs->super, 0, sizeof(fs->super));
    fs->super.magic = LEVIATHAN_MAGIC;
    fs->super.version = LEVIATHAN_VERSION;
    fs->super.block_size = block_size;
    fs->super.blocks = blocks;
    fs->super.area_start[0] = block_size;
    fs->super.area_end[0] = (1u + half) * block_size;
    fs->super.area_start[1] = fs->super.area_end[0];
    fs->super.area_end[1] = blocks * block_size;
    fs->super.active = 0;
    fs->super.head = fs->super.area_start[0];
    fs->super.seq = 1;
    if (store_super(fs) != 0) {
        disk_free(&fs->disk);
        return -1;
    }
    fs->mounted = 1;
    return 0;
}

int fs_mount(LeviathanFs *fs)
{
    if (fs == NULL || fs->disk.data == NULL) {
        return -1;
    }
    if (load_super(fs) != 0) {
        return -1;
    }
    for (size_t index = 0; index < LEVIATHAN_MAX_INODES; ++index) {
        free(fs->inodes[index].data);
        memset(&fs->inodes[index], 0, sizeof(fs->inodes[index]));
    }
    if (replay(fs, 0) != 0) {
        return -1;
    }
    fs->mounted = 1;
    return 0;
}

void fs_unmount(LeviathanFs *fs)
{
    if (fs == NULL) {
        return;
    }
    for (size_t index = 0; index < LEVIATHAN_MAX_INODES; ++index) {
        free(fs->inodes[index].data);
    }
    disk_free(&fs->disk);
    memset(fs, 0, sizeof(*fs));
}

static int append(LeviathanFs *fs, LeviathanRecordType type, uint32_t inode,
                  const char *name, const uint8_t *data, size_t data_len)
{
    uint32_t head = fs->super.head;
    if (!fs->mounted) {
        return -1;
    }
    if (tfs_log_append(&fs->disk, fs->super.area_start[fs->super.active],
                       fs->super.area_end[fs->super.active], &head,
                       fs->super.seq, type, inode, name, data, data_len) != 0) {
        return -1;
    }
    fs->super.head = head;
    fs->super.seq += 1u;
    return store_super(fs);
}

int fs_create(LeviathanFs *fs, const char *name, uint32_t *inode)
{
    size_t length;
    uint32_t slot = LEVIATHAN_MAX_INODES;
    if (fs == NULL || name == NULL || name[0] == '\0') {
        return -1;
    }
    length = strlen(name);
    if (length >= LEVIATHAN_MAX_NAME || inode_name_exists(fs, name)) {
        return -1;
    }
    for (uint32_t index = 0; index < LEVIATHAN_MAX_INODES; ++index) {
        if (!fs->inodes[index].used) {
            slot = index;
            break;
        }
    }
    if (slot == LEVIATHAN_MAX_INODES) {
        return -1;
    }
    if (append(fs, LEVIATHAN_RECORD_CREATE, slot, name, NULL, 0) != 0) {
        return -1;
    }
    apply_create(fs, slot, name, length);
    if (inode != NULL) {
        *inode = slot;
    }
    return 0;
}

int fs_write(LeviathanFs *fs, uint32_t inode, const void *data, size_t size)
{
    if (fs == NULL || find_inode(fs, inode) == NULL || size > LEVIATHAN_MAX_FILE ||
        (size > 0 && data == NULL)) {
        return -1;
    }
    if (append(fs, LEVIATHAN_RECORD_WRITE, inode, NULL, (const uint8_t *)data, size) !=
        0) {
        return -1;
    }
    apply_write(fs, inode, (const uint8_t *)data, size);
    return 0;
}

int fs_read(const LeviathanFs *fs, uint32_t inode, void *buffer, size_t capacity,
            size_t *size)
{
    const LeviathanInode *target = find_inode_const(fs, inode);
    if (target == NULL || (capacity < target->size && target->size > 0) ||
        (target->size > 0 && buffer == NULL)) {
        return -1;
    }
    if (target->size > 0) {
        memcpy(buffer, target->data, target->size);
    }
    if (size != NULL) {
        *size = target->size;
    }
    return 0;
}

int fs_unlink(LeviathanFs *fs, uint32_t inode)
{
    if (fs == NULL || find_inode(fs, inode) == NULL) {
        return -1;
    }
    if (append(fs, LEVIATHAN_RECORD_UNLINK, inode, NULL, NULL, 0) != 0) {
        return -1;
    }
    apply_unlink(fs, inode);
    return 0;
}

int fs_compact(LeviathanFs *fs)
{
    uint32_t inactive;
    uint32_t head;
    uint32_t seq;
    if (fs == NULL || !fs->mounted) {
        return -1;
    }
    inactive = fs->super.active == 0 ? 1u : 0u;
    head = fs->super.area_start[inactive];
    seq = fs->super.seq;
    for (uint32_t inode = 0; inode < LEVIATHAN_MAX_INODES; ++inode) {
        const LeviathanInode *target = find_inode_const(fs, inode);
        if (target == NULL) {
            continue;
        }
        if (tfs_log_append(&fs->disk, fs->super.area_start[inactive],
                           fs->super.area_end[inactive], &head, seq++,
                           LEVIATHAN_RECORD_CREATE, inode, target->name, NULL, 0) !=
            0) {
            return -1;
        }
        if (tfs_log_append(&fs->disk, fs->super.area_start[inactive],
                           fs->super.area_end[inactive], &head, seq++,
                           LEVIATHAN_RECORD_WRITE, inode, NULL, target->data,
                           target->size) != 0) {
            return -1;
        }
    }
    fs->super.active = inactive;
    fs->super.head = head;
    fs->super.seq = seq;
    return store_super(fs);
}

int fs_lookup(const LeviathanFs *fs, const char *name, uint32_t *inode)
{
    if (fs == NULL || name == NULL) {
        return -1;
    }
    for (uint32_t index = 0; index < LEVIATHAN_MAX_INODES; ++index) {
        if (fs->inodes[index].used &&
            strcmp(fs->inodes[index].name, name) == 0) {
            if (inode != NULL) {
                *inode = index;
            }
            return 0;
        }
    }
    return -1;
}

const LeviathanInode *fs_inode(const LeviathanFs *fs, uint32_t inode)
{
    return find_inode_const(fs, inode);
}

int fs_sync(LeviathanFs *fs, const char *path)
{
    if (fs == NULL || path == NULL) {
        return -1;
    }
    return disk_save(&fs->disk, path);
}
