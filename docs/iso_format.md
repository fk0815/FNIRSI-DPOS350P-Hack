# ISO Container Format

The firmware update file (`DPOS350P-FNIRSI-2025619.iso`) is **not** an ISO 9660 image despite the `.iso` extension. It is a custom FNIRSI binary container using eGON-style block headers.

## Update Process

1. Switch oscilloscope to USB storage mode via Menu → USB Connect
2. Copy ISO to root of the USB drive (keep the original filename)
3. Safely eject, reboot — updater in Winbond detects the file and flashes it
4. On success: file is deleted, device reboots into new firmware
5. On checksum failure: file is silently deleted, nothing is flashed

## Container Layout

| Offset | Size | Block | Content |
|--------|------|-------|---------|
| 0x000000 | 0x001000 | Header | 8-byte model signature `DPOS350P` + zero padding |
| 0x001000 | 0x050000 | eGON.BMP | Boot logo (RGB565, 529×95px) |
| 0x051000 | 0x093f10 | eGON.EXE | Main ARM application |
| 0x0e4f10 | 0x02922 | Data | Waveform tables, USB descriptors |
| 0x0e7832 | 36B | Footer | Firmware checksum (9× big-endian uint32) |

Total file size: 948,310 bytes (0xe7856)

## eGON Block Header (64 bytes)

Each block starts with a 64-byte eGON header:

```
Bytes  0– 3:  ARM branch instruction (0xEA000006 — branch past header)
Bytes  4–11:  Block magic ("eGON.BMP" or "eGON.EXE")
Bytes 12–15:  Block checksum (uint32 LE)
Bytes 16–19:  Block total size in bytes (uint32 LE, includes this 64B header)
Bytes 20–63:  Additional metadata (format-specific)
```

## Boot Logo Block (eGON.BMP)

```
Block offset in ISO:  0x001000
Block size:           0x050000 = 327,680 bytes
Header size:          64 bytes
Pixel data size:      100,800 bytes (from size field: 100,864 − 64)
Pixel format:         RGB565, little-endian
Image dimensions:     529 × 95 pixels
Remaining space:      zero-padded to end of block
```

The logo is displayed centered on the 1024×600 display with the device's blue background filling the sides.

**Encoding example (Python):**

```python
import numpy as np
from PIL import Image

img = Image.open("logo.png").resize((529, 95)).convert("RGB")
arr = np.array(img, dtype=np.uint16)
r = (arr[:,:,0] >> 3) & 0x1F
g = (arr[:,:,1] >> 2) & 0x3F
b = (arr[:,:,2] >> 3) & 0x1F
rgb565 = ((r << 11) | (g << 5) | b).astype(np.uint16)
pixel_bytes = rgb565.tobytes()  # little-endian, ready to write
```

## Checksum Footer

The last 36 bytes of the ISO file are the checksum footer.
See [checksum.md](checksum.md) for the full algorithm.

```
Offset 0xe7832:  checkdata[0]  (uint32 BE) — sum of all bytes
Offset 0xe7836:  checkdata[1]  (uint32 BE) — sum of indices where bit 7 set
...
Offset 0xe784e:  checkdata[8]  (uint32 BE) — sum of indices where bit 0 set
```

The same 36 bytes are also stored in Winbond NOR at offset `0x13e832` after a successful update.
