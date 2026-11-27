/* SPDX-License-Identifier: MIT */
#include "log.h"

#include <stdio.h>
#include <string.h>

static uint32_t crc_table[256];
static int crc_ready;

static void crc_init(void)
{
    for (uint32_t index = 0; index < 256; ++index) {
        uint32_t value = index;
        for (int bit = 0; bit < 8; ++bit) {
            value = (value & 1u) != 0 ? (value >> 1u) ^ 0xedb88320u
                                      : value >> 1u;
        }
        crc_table[index] = value;
    }
    crc_ready = 1;
}

uint32_t tfs_crc32(const void *data, size_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xffffffffu;
    if (!crc_ready) {
        crc_init();
    }
    for (size_t index = 0; index < length; ++index) {
        crc = crc_table[(crc ^ bytes[index]) & 0xffu] ^ (crc >> 8u);
    }
    return crc ^ 0xffffffffu;
}

static uint32_t align8(uint32_t value)
{
    return (value + 7u) & ~7u;
}

int tfs_log_append(Disk *disk, uint32_t area_start, uint32_t area_end,
                   uint32_t *head, uint32_t seq, TfsRecordType type,
                   uint32_t inode, const char *name, const uint8_t *data,
                   size_t data_len)
{
    size_t name_len = name != NULL ? strlen(name) : 0;
    uint32_t total;
    TfsRecordHeader header;
    uint8_t *destination;
    if (disk == NULL || head == NULL || name_len >= TFS_MAX_NAME ||
        data_len > TFS_MAX_FILE || area_start >= area_end || *head < area_start ||
        *head > area_end) {
        return -1;
    }
    total = align8((uint32_t)(sizeof(TfsRecordHeader) + name_len + data_len));
    if (total > area_end - *head) {
        return -1;
    }
    memset(&header, 0, sizeof(header));
    header.magic = TFS_RECORD_MAGIC;
    header.seq = seq;
    header.type = (uint8_t)type;
    header.inode = inode;
    header.name_len = (uint16_t)name_len;
    header.data_len = (uint16_t)data_len;
    header.crc = tfs_crc32(data, data_len);
    destination = disk->data + *head;
    memset(destination, 0, total);
    memcpy(destination, &header, sizeof(header));
    if (name_len > 0) {
        memcpy(destination + sizeof(header), name, name_len);
    }
    if (data_len > 0) {
        memcpy(destination + sizeof(header) + name_len, data, data_len);
    }
    *head += total;
    return 0;
}

int tfs_log_next(const Disk *disk, uint32_t limit, uint32_t *position,
                 TfsRecordHeader *header, const uint8_t **name,
                 const uint8_t **data)
{
    const uint8_t *base;
    uint32_t total;
    if (disk == NULL || position == NULL || header == NULL || name == NULL ||
        data == NULL || *position > limit) {
        return -1;
    }
    if (*position + sizeof(TfsRecordHeader) > limit) {
        return 0;
    }
    base = disk->data + *position;
    memcpy(header, base, sizeof(*header));
    if (header->magic != TFS_RECORD_MAGIC) {
        return 0;
    }
    total = (uint32_t)(sizeof(*header) + header->name_len + header->data_len);
    if (header->name_len >= TFS_MAX_NAME || header->data_len > TFS_MAX_FILE ||
        *position + total > limit) {
        return -1;
    }
    if (tfs_crc32(base + sizeof(*header) + header->name_len, header->data_len) !=
        header->crc) {
        return -1;
    }
    *name = base + sizeof(*header);
    *data = base + sizeof(*header) + header->name_len;
    *position += align8(total);
    return 1;
}
