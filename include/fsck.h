/* SPDX-License-Identifier: MIT */
#ifndef LEVIATHAN_LOG_FSCK_H
#define LEVIATHAN_LOG_FSCK_H

#include <stddef.h>
#include <stdint.h>

#include "disk.h"

typedef struct {
    uint32_t valid_head;
    uint32_t records;
    uint32_t damaged_records;
    uint32_t used_inodes;
    int repaired;
} LeviathanFsckReport;

int fsck_run(Disk *disk, int repair, LeviathanFsckReport *report, char *err,
             size_t errcap);

#endif /* LEVIATHAN_LOG_FSCK_H */
