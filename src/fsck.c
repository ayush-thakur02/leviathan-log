/* SPDX-License-Identifier: MIT */
#include "fsck.h"

#include <stdio.h>
#include <string.h>

#include "log.h"
#include "leviathan-log.h"

static void set_error(char *err, size_t errcap, const char *message)
{
    if (err != NULL && errcap > 0) {
        (void)snprintf(err, errcap, "%s", message);
    }
}

int fsck_run(Disk *disk, int repair, LeviathanFsckReport *report, char *err,
             size_t errcap)
{
    LeviathanSuperblock super;
    uint32_t position;
    uint32_t valid_head;
    uint8_t used[LEVIATHAN_MAX_INODES];
    if (disk == NULL || report == NULL || disk->data == NULL) {
        set_error(err, errcap, "invalid fsck arguments");
        return -1;
    }
    memset(report, 0, sizeof(*report));
    memset(used, 0, sizeof(used));
    memcpy(&super, disk->data, sizeof(super));
    if (super.magic != LEVIATHAN_MAGIC || super.version != LEVIATHAN_VERSION ||
        super.block_size != disk->block_size || super.blocks != disk->blocks ||
        super.active > 1 ||
        super.area_start[super.active] >= super.area_end[super.active] ||
        super.head < super.area_start[super.active] ||
        super.head > super.area_end[super.active]) {
        set_error(err, errcap, "invalid superblock");
        return -1;
    }
    position = super.area_start[super.active];
    valid_head = position;
    while (position < super.head) {
        LeviathanRecordHeader header;
        const uint8_t *name;
        const uint8_t *data;
        int result = tfs_log_next(disk, super.head, &position, &header, &name,
                                  &data);
        if (result <= 0) {
            report->damaged_records += 1;
            break;
        }
        report->records += 1;
        valid_head = position;
        if (header.type == LEVIATHAN_RECORD_CREATE &&
            header.inode < LEVIATHAN_MAX_INODES) {
            used[header.inode] = 1;
        } else if (header.type == LEVIATHAN_RECORD_UNLINK &&
                   header.inode < LEVIATHAN_MAX_INODES) {
            used[header.inode] = 0;
        }
    }
    report->valid_head = valid_head;
    for (uint32_t index = 0; index < LEVIATHAN_MAX_INODES; ++index) {
        report->used_inodes += used[index] ? 1u : 0u;
    }
    if (report->damaged_records > 0 && repair) {
        super.head = valid_head;
        memcpy(disk->data, &super, sizeof(super));
        report->repaired = 1;
    }
    return 0;
}
