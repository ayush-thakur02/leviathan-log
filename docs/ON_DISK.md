# tinyfs on-disk format

All integers are native-endian in this first release. Images use 512-byte blocks.

## 1. Superblock

Block 0 contains a packed `TfsSuperblock`:

```text
magic         0x54465331 ("TFS1")
version       1
block_size    bytes per block
blocks        total blocks
area_start[2] byte offsets of the two log areas
area_end[2]   exclusive byte offsets
active        0 or 1
head          committed append offset in the active area
seq           next record sequence number
```

The remainder of block 0 is zero. Areas split the usable blocks after block 0; for even
splits the first half is area 0 and the second half is area 1.

## 2. Record

```text
magic      u32  0x54465352 ("TFSR")
seq        u32
type       u8   1=CREATE, 2=WRITE, 3=UNLINK
reserved   u8[3]
inode      u32
name_len   u16  (CREATE only; must be < 32)
data_len   u16  (WRITE only; must be <= 4096)
crc        u32  CRC32 of name+data payload
```

The payload follows immediately:

```text
name[name_len]
data[data_len]
zero padding to an 8-byte boundary
```

`head` advances by the aligned record size. A record is committed when the superblock with
the updated head is written.

## 3. Replay

Mount starts at the active area's start and reads records while `position < head`:

- magic mismatch ends replay (zero or torn tail);
- a record extending past `head` is treated as damage;
- a CRC mismatch is damage;
- otherwise CREATE/WRITE/UNLINK are applied to the in-memory inode table.

The sequence number is informational; replay does not require contiguous sequence numbers.

## 4. Compaction

`fs_compact` uses the inactive area:

1. Set `head` to the inactive start and `seq` to the current sequence.
2. For every live inode, append CREATE and WRITE records.
3. Once all records fit, update the superblock's active area, head, and sequence.
4. The old area remains untouched, so a crash before step 3 leaves the old state valid.

If the inactive area is too small, compaction fails without switching.

## 5. fsck

`fsck_run` validates the superblock and scans records up to `head`. It counts valid and
damaged records and tracks live inode IDs from CREATE/UNLINK records. With `--repair`, it
sets `head` to the last valid record boundary and writes the superblock back. It does not
invent missing data.

## 6. In-memory limits

```text
TFS_MAX_INODES 32
TFS_MAX_NAME   32 bytes (including NUL)
TFS_MAX_FILE   4096 bytes
```

Names are unique. The inode number is the slot index. File data is replaced by each WRITE
record; there are no offsets, appends, or sparse holes.

## 7. Failure model

- A crash before the superblock head update loses the uncommitted record.
- A crash during compaction before the active-area switch leaves the old area valid.
- A crash after the switch uses the new complete area.
- A torn tail is ignored by mount and can be truncated by fsck.
- Corrupted committed records before the tail cause replay to stop; later records are not
  recovered.
