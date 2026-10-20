#!/usr/bin/env python3
"""
dpos_tool.py — FNIRSI DPOS350P firmware utility

Supported commands:
  info     <firmware.iso>                    — show ISO structure info
  verify   <firmware.iso>                    — verify firmware checksum
  patch    <firmware.iso> [output.iso]       — recompute and embed checksum
  extract  <firmware.iso> <out.png>          — extract boot logo as PNG
  replace  <firmware.iso> <logo.png> <out.iso> — replace boot logo and patch checksum

Boot logo format (DPOS350P):
  Pixel format : RGB565, little-endian
  Dimensions   : 529 x 95 pixels (best current estimate — positioning still being investigated)
  Block offset : 0x1000 in ISO (64-byte eGON header + pixel data)

Checksum algorithm:
  Computed over iso[0x1000 : file_size-36] with relative indices.
  9 x big-endian uint32 stored in the last 36 bytes of the ISO.
  See docs/checksum.md for full details.

WARNING:
  Modifying firmware carries the risk of permanently damaging your device.
  Always back up the original ISO and the Winbond NOR flash before proceeding.
  Use at your own risk.

Repository: https://github.com/mkubi/FNIRSI-DPOS350P-Hack
"""

import sys
import struct
import shutil
from pathlib import Path

# ── ISO layout constants ───────────────────────────────────────────────────────
MODEL_SIG       = b'DPOS350P'   # 8-byte model signature at offset 0
LOGO_OFFSET     = 0x001000      # start of eGON.BMP block
LOGO_HDR        = 64            # eGON block header size (bytes)
LOGO_BLOCK      = 0x051000      # end of logo block / start of eGON.EXE
CHECKSUM_SIZE   = 36            # 9 x uint32 BE footer at end of file

# Boot logo pixel dimensions (529x95 confirmed working; exact framebuffer
# layout still under investigation — logo appears slightly off-center)
LOGO_WIDTH      = 529
LOGO_HEIGHT     = 95


# ── Checksum ──────────────────────────────────────────────────────────────────

def compute_checksum(data: bytes) -> list[int]:
    """
    Compute the 9-value FNIRSI firmware checksum.

    Input range: iso[0x1000 : file_size-36]
    Indices are relative (start at 0).

    checkdata[0] = sum of all bytes
    checkdata[1] = sum of indices where (byte & 0x80)
    checkdata[2] = sum of indices where (byte & 0x40)
    ...
    checkdata[8] = sum of indices where (byte & 0x01)

    All values are masked to 32 bits.
    """
    payload = data[0x1000 : len(data) - CHECKSUM_SIZE]
    cd = [0] * 9
    for i, byte in enumerate(payload):
        cd[0] += byte
        mask = 0x80
        for bit in range(1, 9):
            if byte & mask:
                cd[bit] += i
            mask >>= 1
    return [v & 0xFFFFFFFF for v in cd]


def patch_checksum(data: bytes) -> bytes:
    """Recompute checksum and embed it in the last 36 bytes. Returns patched bytes."""
    cd = compute_checksum(data)
    footer = b"".join(struct.pack(">I", v) for v in cd)
    return data[:-CHECKSUM_SIZE] + footer


def verify_checksum(data: bytes) -> bool:
    """Return True if the ISO checksum footer is valid."""
    stored = [
        struct.unpack_from(">I", data, len(data) - CHECKSUM_SIZE + i * 4)[0]
        for i in range(9)
    ]
    return compute_checksum(data) == stored


# ── ISO validation ─────────────────────────────────────────────────────────────

def check_iso(data: bytes) -> bool:
    """Basic sanity check — verify model signature and eGON block markers."""
    ok = True
    if not data[:8].startswith(MODEL_SIG):
        print(f"WARNING: unexpected model signature: {data[:8]}")
        ok = False
    if b'eGON.BMP' not in data[LOGO_OFFSET : LOGO_OFFSET + 16]:
        print(f"WARNING: eGON.BMP marker not found at 0x{LOGO_OFFSET:06x}")
        ok = False
    if b'eGON.EXE' not in data[LOGO_BLOCK : LOGO_BLOCK + 16]:
        print(f"WARNING: eGON.EXE marker not found at 0x{LOGO_BLOCK:06x}")
        ok = False
    return ok


# ── Commands ───────────────────────────────────────────────────────────────────

def cmd_info(iso_path: str):
    """Print ISO structure information."""
    data = open(iso_path, "rb").read()
    check_iso(data)
    cs_ok = verify_checksum(data)
    print(f"File     : {iso_path}")
    print(f"Size     : {len(data)} bytes ({len(data) / 1024:.1f} kB)")
    print(f"Model    : {data[:8].rstrip(b'\\x00').decode('latin1')}")
    print(f"Checksum : {'OK' if cs_ok else 'INVALID'}")
    print()
    print("Known sections:")
    print(f"  0x000000 – 0x001000  header (model signature + padding)")
    print(f"  0x001000 – 0x051000  eGON.BMP (boot logo, RGB565 LE, {LOGO_WIDTH}x{LOGO_HEIGHT}px)")
    print(f"  0x051000 – 0x0e4f10  eGON.EXE (main ARM application)")
    print(f"  0x0e4f10 – 0x0e7832  data tables (waveforms, USB descriptors)")
    print(f"  0x0e7832 – EOF       checksum footer (9x uint32 BE, 36 bytes)")


def cmd_verify(iso_path: str):
    """Verify firmware checksum."""
    data = open(iso_path, "rb").read()
    ok = verify_checksum(data)
    print(f"Checksum: {'OK' if ok else 'INVALID'}")
    sys.exit(0 if ok else 1)


def cmd_patch(iso_path: str, out_path: str = None):
    """Recompute and embed checksum footer."""
    if out_path is None:
        out_path = iso_path.replace(".iso", "_patched.iso")
    data = open(iso_path, "rb").read()
    patched = patch_checksum(data)
    open(out_path, "wb").write(patched)
    print(f"Checksum patched → {out_path}")


def cmd_extract(iso_path: str, out_png: str):
    """Extract boot logo from ISO as PNG."""
    try:
        from PIL import Image
        import numpy as np
    except ImportError:
        print("ERROR: Pillow and numpy required: pip install pillow numpy")
        sys.exit(1)

    data = open(iso_path, "rb").read()
    check_iso(data)

    # Read pixel data from logo block (skip 64-byte eGON header)
    raw = data[LOGO_OFFSET + LOGO_HDR : LOGO_BLOCK]
    total_px = len(raw) // 2
    h = min(LOGO_HEIGHT, total_px // LOGO_WIDTH)

    arr = (
        np.frombuffer(raw[: h * LOGO_WIDTH * 2], dtype="<u2")
        .reshape(h, LOGO_WIDTH)
    )
    # Decode RGB565 → RGB888
    r = ((arr >> 11) & 0x1F) * 255 // 31
    g = ((arr >>  5) & 0x3F) * 255 // 63
    b = ( arr        & 0x1F) * 255 // 31
    rgb = np.stack([r, g, b], axis=2).astype(np.uint8)

    img = Image.fromarray(rgb)
    img.save(out_png)
    print(f"Logo extracted: {img.width}x{img.height}px → {out_png}")


def cmd_replace(iso_path: str, logo_png: str, out_iso: str):
    """Replace boot logo in ISO and recompute checksum."""
    try:
        from PIL import Image
        import numpy as np
    except ImportError:
        print("ERROR: Pillow and numpy required: pip install pillow numpy")
        sys.exit(1)

    data = bytearray(open(iso_path, "rb").read())
    check_iso(bytes(data))

    # Load and resize logo to target dimensions
    img = Image.open(logo_png).convert("RGB").resize(
        (LOGO_WIDTH, LOGO_HEIGHT), Image.LANCZOS
    )
    arr = np.array(img, dtype=np.uint16)
    r   = (arr[:, :, 0] >> 3) & 0x1F
    g   = (arr[:, :, 1] >> 2) & 0x3F
    b   = (arr[:, :, 2] >> 3) & 0x1F
    rgb565 = ((r << 11) | (g << 5) | b).astype(np.uint16)

    # Build pixel buffer — pad remaining space with zeros
    buf_size = LOGO_BLOCK - LOGO_OFFSET - LOGO_HDR
    buf = bytearray(buf_size)
    pixel_bytes = rgb565.tobytes()
    buf[: len(pixel_bytes)] = pixel_bytes

    # Write pixel data into ISO (preserve eGON header)
    px_start = LOGO_OFFSET + LOGO_HDR
    data[px_start : px_start + buf_size] = buf

    # Recompute and embed checksum
    patched = patch_checksum(bytes(data))
    open(out_iso, "wb").write(patched)

    cs_ok = verify_checksum(patched)
    print(f"Logo replaced : {logo_png} ({LOGO_WIDTH}x{LOGO_HEIGHT}px)")
    print(f"Output        : {out_iso}")
    print(f"Checksum      : {'OK' if cs_ok else 'ERROR — something went wrong'}")
    print()
    print("WARNING: Always back up the original ISO and Winbond NOR flash")
    print("         before flashing to the device. Proceed at your own risk.")


# ── Entry point ────────────────────────────────────────────────────────────────

def usage():
    print(__doc__)
    sys.exit(1)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        usage()

    cmd = sys.argv[1]
    args = sys.argv[2:]

    if   cmd == "info"    and len(args) == 1: cmd_info(*args)
    elif cmd == "verify"  and len(args) == 1: cmd_verify(*args)
    elif cmd == "patch"   and len(args) in (1, 2): cmd_patch(*args)
    elif cmd == "extract" and len(args) == 2: cmd_extract(*args)
    elif cmd == "replace" and len(args) == 3: cmd_replace(*args)
    else: usage()
