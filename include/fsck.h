/* SPDX-License-Identifier: MIT */
#ifndef TINYFS_FSCK_H
#define TINYFS_FSCK_H

#include <stddef.h>
#include <stdint.h>

#include "disk.h"

typedef struct {
    uint32_t valid_head;
    uint32_t records;
    uint32_t damaged_records;
    uint32_t used_inodes;
    int repaired;
} TfsFsckReport;

int fsck_run(Disk *disk, int repair, TfsFsckReport *report, char *err,
             size_t errcap);

#endif /* TINYFS_FSCK_H */
