/* SPDX-License-Identifier: MIT */
#ifndef LEVIATHAN_LOG_LOG_H
#define LEVIATHAN_LOG_LOG_H

#include <stddef.h>
#include <stdint.h>

#include "disk.h"

#define LEVIATHAN_RECORD_MAGIC 0x54465352u
#define LEVIATHAN_MAX_NAME 32
#define LEVIATHAN_MAX_FILE 4096

typedef enum {
    LEVIATHAN_RECORD_CREATE = 1,
    LEVIATHAN_RECORD_WRITE = 2,
    LEVIATHAN_RECORD_UNLINK = 3
} LeviathanRecordType;

typedef struct {
    uint32_t magic;
    uint32_t seq;
    uint8_t type;
    uint8_t reserved[3];
    uint32_t inode;
    uint16_t name_len;
    uint16_t data_len;
    uint32_t crc;
} __attribute__((packed)) LeviathanRecordHeader;

uint32_t tfs_crc32(const void *data, size_t length);
int tfs_log_append(Disk *disk, uint32_t area_start, uint32_t area_end,
                   uint32_t *head, uint32_t seq, LeviathanRecordType type,
                   uint32_t inode, const char *name, const uint8_t *data,
                   size_t data_len);
int tfs_log_next(const Disk *disk, uint32_t area_end, uint32_t *position,
                 LeviathanRecordHeader *header, const uint8_t **name,
                 const uint8_t **data);

#endif /* LEVIATHAN_LOG_LOG_H */
