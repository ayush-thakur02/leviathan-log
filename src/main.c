/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "fsck.h"
#include "leviathan-log.h"

static void usage(void)
{
    (void)printf(
        "leviathan-log - log-structured filesystem image tool\n"
        "\n"
        "USAGE:\n"
        "  leviathan-log format --image PATH --blocks N [--block-size 512]\n"
        "  leviathan-log put --image PATH --name NAME --value TEXT\n"
        "  leviathan-log get --image PATH --name NAME\n"
        "  leviathan-log ls --image PATH\n"
        "  leviathan-log compact --image PATH\n"
        "  leviathan-log fsck --image PATH [--repair]\n"
        "\n"
        "The image is a raw block device; the default block size is 512 bytes.\n");
}

static const char *argument(int argc, char **argv, const char *name)
{
    for (int index = 1; index + 1 < argc; ++index) {
        if (strcmp(argv[index], name) == 0) {
            return argv[index + 1];
        }
    }
    return NULL;
}

static int has_flag(int argc, char **argv, const char *name)
{
    for (int index = 1; index < argc; ++index) {
        if (strcmp(argv[index], name) == 0) {
            return 1;
        }
    }
    return 0;
}

static int load(const char *path, LeviathanFs *fs)
{
    if (disk_load(&fs->disk, path) != 0) {
        (void)fprintf(stderr, "error: cannot load %s\n", path);
        return -1;
    }
    if (fs_mount(fs) != 0) {
        (void)fprintf(stderr, "error: cannot mount %s\n", path);
        disk_free(&fs->disk);
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage();
        return 1;
    }
    const char *command = argv[1];
    const char *image = argument(argc, argv, "--image");
    if (image == NULL) {
        (void)fprintf(stderr, "error: --image PATH is required\n");
        return 1;
    }
    if (strcmp(command, "format") == 0) {
        const char *blocks_text = argument(argc, argv, "--blocks");
        const char *block_size_text = argument(argc, argv, "--block-size");
        uint32_t blocks = blocks_text != NULL ? (uint32_t)strtoul(blocks_text, NULL, 10) : 0;
        uint32_t block_size =
            block_size_text != NULL ? (uint32_t)strtoul(block_size_text, NULL, 10) : 512;
        if (block_size != 512) {
            (void)fprintf(stderr, "error: this image format currently requires 512-byte blocks\n");
            return 1;
        }
        LeviathanFs fs;
        fs_init(&fs);
        if (blocks == 0 || fs_format(&fs, blocks, block_size) != 0 ||
            fs_sync(&fs, image) != 0) {
            (void)fprintf(stderr, "error: format failed\n");
            fs_unmount(&fs);
            return 1;
        }
        (void)printf("formatted=%s blocks=%u block_size=%u\n", image, blocks, block_size);
        fs_unmount(&fs);
        return 0;
    }
    if (strcmp(command, "fsck") == 0) {
        Disk disk;
        LeviathanFsckReport report;
        char err[128];
        if (disk_load(&disk, image) != 0) {
            (void)fprintf(stderr, "error: cannot load image\n");
            return 1;
        }
        if (fsck_run(&disk, has_flag(argc, argv, "--repair"), &report, err,
                     sizeof(err)) != 0) {
            (void)fprintf(stderr, "error: %s\n", err);
            disk_free(&disk);
            return 1;
        }
        (void)printf(
            "records=%u damaged=%u valid_head=%u used_inodes=%u repaired=%d\n",
            report.records, report.damaged_records, report.valid_head,
            report.used_inodes, report.repaired);
        if (report.repaired && disk_save(&disk, image) != 0) {
            (void)fprintf(stderr, "error: cannot save repaired image\n");
            disk_free(&disk);
            return 1;
        }
        disk_free(&disk);
        return 0;
    }
    LeviathanFs fs;
    fs_init(&fs);
    if (load(image, &fs) != 0) {
        return 1;
    }
    if (strcmp(command, "put") == 0) {
        const char *name = argument(argc, argv, "--name");
        const char *value = argument(argc, argv, "--value");
        uint32_t inode;
        if (name == NULL || value == NULL) {
            (void)fprintf(stderr, "error: --name and --value are required\n");
            fs_unmount(&fs);
            return 1;
        }
        if (fs_lookup(&fs, name, &inode) != 0 &&
            fs_create(&fs, name, &inode) != 0) {
            (void)fprintf(stderr, "error: cannot create %s\n", name);
            fs_unmount(&fs);
            return 1;
        }
        if (fs_write(&fs, inode, value, strlen(value)) != 0 ||
            fs_sync(&fs, image) != 0) {
            (void)fprintf(stderr, "error: cannot write %s\n", name);
            fs_unmount(&fs);
            return 1;
        }
        (void)printf("name=%s inode=%u bytes=%zu\n", name, inode, strlen(value));
        fs_unmount(&fs);
        return 0;
    }
    if (strcmp(command, "get") == 0) {
        const char *name = argument(argc, argv, "--name");
        uint32_t inode;
        uint8_t buffer[LEVIATHAN_MAX_FILE];
        size_t size = 0;
        if (name == NULL || fs_lookup(&fs, name, &inode) != 0 ||
            fs_read(&fs, inode, buffer, sizeof(buffer), &size) != 0) {
            (void)fprintf(stderr, "error: cannot read %s\n", name != NULL ? name : "");
            fs_unmount(&fs);
            return 1;
        }
        (void)fwrite(buffer, 1, size, stdout);
        (void)fputc('\n', stdout);
        fs_unmount(&fs);
        return 0;
    }
    if (strcmp(command, "ls") == 0) {
        for (uint32_t inode = 0; inode < LEVIATHAN_MAX_INODES; ++inode) {
            const LeviathanInode *entry = fs_inode(&fs, inode);
            if (entry != NULL) {
                (void)printf("%u %s %zu\n", inode, entry->name, entry->size);
            }
        }
        fs_unmount(&fs);
        return 0;
    }
    if (strcmp(command, "compact") == 0) {
        if (fs_compact(&fs) != 0 || fs_sync(&fs, image) != 0) {
            (void)fprintf(stderr, "error: compaction failed\n");
            fs_unmount(&fs);
            return 1;
        }
        (void)printf("compacted active_area=%u head=%u\n", fs.super.active,
                     fs.super.head);
        fs_unmount(&fs);
        return 0;
    }
    (void)fprintf(stderr, "error: unknown command %s\n", command);
    usage();
    fs_unmount(&fs);
    return 1;
}
