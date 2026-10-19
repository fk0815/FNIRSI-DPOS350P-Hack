# Firmware Checksum Algorithm

## Overview

The DPOS350P updater validates the firmware ISO using a custom 9-value checksum stored in the last 36 bytes of the file. If validation fails the ISO is silently deleted with no error message.

This algorithm is the same family as used in the FNIRSI 1013D/1014D, identified by [pecostm32](https://github.com/pecostm32/FNIRSI-1013D-1014D-Hack). The DPOS350P variant differs in the input range.

## Algorithm

```
Input:   iso_file[0x1000 : file_size - 36]
Indices: relative, starting from 0

checkdata[0] = Σ byte
checkdata[1] = Σ index  for every byte where (byte & 0x80)
checkdata[2] = Σ index  for every byte where (byte & 0x40)
checkdata[3] = Σ index  for every byte where (byte & 0x20)
checkdata[4] = Σ index  for every byte where (byte & 0x10)
checkdata[5] = Σ index  for every byte where (byte & 0x08)
checkdata[6] = Σ index  for every byte where (byte & 0x04)
checkdata[7] = Σ index  for every byte where (byte & 0x02)
checkdata[8] = Σ index  for every byte where (byte & 0x01)

All values are masked to 32 bits (uint32 overflow wraps).
Stored as 9 × big-endian uint32 in the last 36 bytes of the file.
```

The first `0x1000` bytes (8-byte `DPOS350P` signature + zero padding) are **skipped** — checksum computation starts at file offset `0x1000`.

## Reference Implementation (Python)

```python
import struct

def compute_checksum(data: bytes) -> list[int]:
    """Compute 9-value checksum. data = full ISO bytes."""
    payload = data[0x1000 : len(data) - 36]
    cd = [0] * 9
    for i, byte in enumerate(payload):
        cd[0] += byte
        bf = 0x80
        for bit in range(1, 9):
            if byte & bf:
                cd[bit] += i
            bf >>= 1
    return [v & 0xFFFFFFFF for v in cd]


def patch_checksum(data: bytes) -> bytes:
    """Recompute and embed checksum footer. Returns patched ISO bytes."""
    cd = compute_checksum(data)
    footer = b"".join(struct.pack(">I", v) for v in cd)
    return data[:-36] + footer


def verify_checksum(data: bytes) -> bool:
    """Return True if the ISO checksum is valid."""
    stored = [
        struct.unpack_from(">I", data, len(data) - 36 + i * 4)[0]
        for i in range(9)
    ]
    return compute_checksum(data) == stored
```

## Usage

```bash
# Verify an existing ISO
python3 tools/dpos_tool.py verify firmware.iso

# Patch checksum after modifying an ISO
python3 tools/dpos_tool.py patch modified.iso output.iso
```

## Notes

- The same 36-byte footer is stored in Winbond NOR at `0x13e832` after a successful update
- The `0x4eeb0104` value we initially identified as a "magic number" is simply `checkdata[0]` (byte sum) read as little-endian — it is not a magic constant
- Despite the 32-byte payload looking like a SHA-256 hash (entropy ≈ 4.94), it is this custom algorithm
