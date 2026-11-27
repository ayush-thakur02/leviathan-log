# leviathan-log

A log-structured filesystem image tool in C11: CRC-checked append records, two log areas,
explicit compaction, crash-tail recovery, and fsck. Files are stored in log records; there
is no separate data-block allocator yet.

## Why this exists

Filesystem metadata is usually hidden by the kernel. This project isolates the on-disk
format and recovery path:

- a superblock with two alternating log areas;
- append-only CREATE/WRITE/UNLINK records;
- CRC32 per record;
- replay on mount, stopping at a torn tail;
- active-area compaction into the inactive area;
- an fsck scan that can truncate a damaged tail.

## Design overview

```text
superblock (block 0)
  area[0]  blocks 1..mid
  area[1]  blocks mid..end
  active, head, seq

record = header + name + data + padding + CRC32
```

- A record header contains magic, sequence, type, inode, name/data lengths, and payload
  CRC32.
- `fs_create` appends a CREATE record, then updates in-memory state.
- `fs_write` appends a WRITE record with the file bytes. A later write replaces the
  in-memory file and remains recoverable from the log.
- `fs_unlink` appends an UNLINK record.
- The superblock's `head` is written after a record is complete. A crash before the
  superblock update leaves the record outside the committed region.
- `fs_compact` writes CREATE+WRITE records for every live inode into the inactive area,
  then atomically switches the active area. A crash before the switch leaves the old area
  valid; a crash after it leaves the new area complete.
- Mount replays the active area up to `head`. A damaged record stops replay; files from
  earlier records remain available. `leviathan-log fsck --repair` truncates `head` to the last
  valid record.

## Build

Requirements: a C11 compiler and GNU make.

```bash
make
make test
make bench
```

Analysis targets: `make asan`, `make ubsan`, `make valgrind`.

### Docker

```bash
docker build -t leviathan-log .
docker run --rm leviathan-log --help
```

## Usage

Images are raw 512-byte-block files.

```bash
# Format a 64-block image
bin/leviathan-log format --image disk.img --blocks 64

# Store and read a file
bin/leviathan-log put --image disk.img --name greeting --value "hello leviathan-log"
bin/leviathan-log get --image disk.img --name greeting

# List files
bin/leviathan-log ls --image disk.img

# Compact the active log into the inactive area
bin/leviathan-log compact --image disk.img

# Check and repair a damaged tail
bin/leviathan-log fsck --image disk.img --repair
```

Example output is **illustrative** and was not produced by running the code:

```text
formatted=disk.img blocks=64 block_size=512
name=greeting inode=0 bytes=11
hello leviathan-log
0 greeting 11
compacted active_area=1 head=...
records=2 damaged=0 valid_head=... used_inodes=1 repaired=0
```

## Tests

```bash
make test
```

Coverage includes:

- block device read/write/save/load and bounds checks;
- format, create, write, read, lookup, sync, remount, compact, and unlink;
- compaction switching active areas while preserving content;
- a torn-tail simulation: mount ignores an incomplete record, fsck repairs the head, and
  the earlier file remains readable;
- disk image size/block validation.

Tests were authored but not executed in the authoring environment; no pass claim is made.

## Benchmarks

```bash
make bench
```

The benchmark creates 16 files of 200 bytes in a 512-block image, times writes, reads, and
compaction, and prints CSV. Results depend on hardware and compiler flags; no numbers are
checked in.

## Limitations

- At most 32 files, 32-byte names, and 4 KiB per file.
- No directories, permissions, timestamps, or hard links.
- File content lives in the log; there is no data-block allocator or free-space bitmap.
- Only 512-byte blocks are accepted by the CLI image loader.
- Mount builds the entire inode table in memory.
- Compaction rewrites all live data and is O(total live bytes).
- fsck repairs only the committed head; it does not reconstruct missing records.
- No concurrency, locking, or OS block-device integration.

## Future work

- Data-block allocation and indirect blocks.
- Directories and path lookup.
- A real write-ahead journal with checkpoint barriers.
- Sparse files and checksums for metadata blocks.
- A FUSE or loopback adapter for host testing.

## Project status

Initial implementation. Source, tests, benchmarks, documentation, and container files were
authored without building, running, or mounting the filesystem. CI compiles and tests on
push. All example output is illustrative.

## License

MIT. See [LICENSE](LICENSE).
