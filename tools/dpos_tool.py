#!/usr/bin/env python3
"""
DPOS350P firmware tool
- extract_logo  : vytiahne bootovacie logo ako PNG
- replace_logo  : vlozi vlastne PNG logo do .iso
- info          : zobrazi info o iso subore

Pouzitie:
  python3 dpos_tool.py info        firmware.iso
  python3 dpos_tool.py extract     firmware.iso logo_out.png
  python3 dpos_tool.py replace     firmware.iso moje_logo.png custom.iso

Logo parametre:
  Format   : RGB565, little-endian
  Stride   : 793 pixelov (sirka riadku v datach)
  Max vyska: ~63 riadkov (viac mozno zobrazi, neoverene)
  Offset v .iso: 0x1000 + 64 bajtov hlavicka
  Logo sa zobrazuje na modrom pozadi, ide o hornu cast obrazovky.

Bezpecnost:
  - Tento nastroj meni LEN logo blok (0x1000-0x51000).
  - ARM kod, FPGA, kalibracia a data su nedotknuté.
  - Vzdy si zalohuj povodny .iso pred pouzitim!
  - Verzia firmveru: DPOS350P-FNIRSI-2025619.iso
"""

import sys, struct, shutil
from pathlib import Path

LOGO_OFFSET  = 0x1000        # zaciatok eGON.BMP bloku
LOGO_HDR     = 64            # velkost hlavicky pred pixel datami
LOGO_STRIDE  = 793           # pixelov na riadok (RGB565)
LOGO_BLOCK   = 0x51000       # koniec logo bloku (zaciatok eGON.EXE)
MODEL_SIG    = b'DPOS350P'   # podpis modelu na bajte 0

def check_iso(data):
    if data[:8] != MODEL_SIG + b'\x00' * 0:
        pass
    if data[:8] != (MODEL_SIG + b'\x00'*8)[:8]:
        if data[:8] != MODEL_SIG + b'\x00\x00\x00\x00\x00\x00\x00\x00':
            pass
    sig = data[:8]
    if not sig.startswith(MODEL_SIG):
        print('VAROVANIE: podpis modelu nekorektny:', sig)
        return False
    if b'eGON.BMP' not in data[0x1000:0x1010]:
        print('VAROVANIE: eGON.BMP marker nenadeny na 0x1000')
        return False
    if b'eGON.EXE' not in data[0x51000:0x51010]:
        print('VAROVANIE: eGON.EXE marker nenadeny na 0x51000')
        return False
    return True

def cmd_info(iso_path):
    data = open(iso_path,'rb').read()
    ok = check_iso(data)
    print(f'Subor     : {iso_path}')
    print(f'Velkost   : {len(data)} bajtov ({len(data)//1024} kB)')
    print(f'Platny    : {"ANO" if ok else "NIE - overte subor"}')
    print(f'Model     : {data[:8].decode("latin1").rstrip(chr(0))}')
    print()
    print('Sekcie:')
    print(f'  0x000000 - 0x001000 : hlavicka (podpis modelu)')
    print(f'  0x001000 - 0x051000 : eGON.BMP (logo, RGB565 stride=793)')
    print(f'  0x051000 - 0x0de000 : eGON.EXE (ARM aplikacia)')
    print(f'  0x0de000 - 0x0e4f10 : datove tabulky')
    print(f'  0x0e4f10 - koniec   : generatorova sekcia + USB deskriptory')
    print()
    # velkost logo pixel dat
    logo_px = (LOGO_BLOCK - LOGO_OFFSET - LOGO_HDR) // 2
    h = logo_px // LOGO_STRIDE
    print(f'Logo      : {LOGO_STRIDE}x{h} px max (RGB565)')

def cmd_extract(iso_path, out_png):
    from PIL import Image
    import numpy as np
    data = open(iso_path,'rb').read()
    if not check_iso(data):
        print('Varovanie: subor moze byt neplatny, pokracujem...')
    buf = data[LOGO_OFFSET + LOGO_HDR : LOGO_BLOCK]
    stride = LOGO_STRIDE
    npx = len(buf) // 2
    h = npx // stride
    arr = np.frombuffer(buf[:h*stride*2], dtype='<u2').reshape(h, stride)
    # RGB565 -> RGB888
    r = ((arr >> 11) & 0x1f) << 3
    g = ((arr >> 5)  & 0x3f) << 2
    b = (arr & 0x1f) << 3
    rgb = np.stack([r,g,b], axis=2).astype(np.uint8)
    img = Image.fromarray(rgb)
    # orez na neprazdnu cast
    rowsum = rgb.sum(axis=(1,2))
    nz = np.where(rowsum > 0)[0]
    if len(nz):
        img = img.crop((0, nz.min(), stride, nz.max()+1))
    img.save(out_png)
    print(f'Logo extrahovane: {img.size[0]}x{img.size[1]} px -> {out_png}')

def cmd_replace(iso_path, logo_png, out_iso):
    from PIL import Image
    import numpy as np
    # nacitaj povodny iso
    data = bytearray(open(iso_path,'rb').read())
    if not check_iso(bytes(data)):
        print('Varovanie: zdrojovy subor moze byt neplatny')
    # nacitaj logo
    img = Image.open(logo_png).convert('RGB')
    w, h = img.size
    if w > LOGO_STRIDE:
        print(f'Logo je sirsi nez stride ({w} > {LOGO_STRIDE}), orezavam.')
        img = img.crop((0,0,LOGO_STRIDE,h))
        w = LOGO_STRIDE
    arr = np.array(img)
    r = (arr[:,:,0] >> 3).astype(np.uint16)
    g = (arr[:,:,1] >> 2).astype(np.uint16)
    b_c = (arr[:,:,2] >> 3).astype(np.uint16)
    rgb565 = ((r << 11) | (g << 5) | b_c)
    # poskladaj data: kazdy riadok ma presne LOGO_STRIDE pixelov
    max_h = (LOGO_BLOCK - LOGO_OFFSET - LOGO_HDR) // (LOGO_STRIDE * 2)
    if h > max_h:
        print(f'Logo je vyssi nez max ({h} > {max_h}), orezavam.')
        rgb565 = rgb565[:max_h,:]
        h = max_h
    # vytvor buffer: pad na plnu sirku stride, zvysok nuly
    buf_size = LOGO_BLOCK - LOGO_OFFSET - LOGO_HDR
    buf = bytearray(buf_size)
    for y in range(h):
        row = rgb565[y,:w].astype('<u2').tobytes()
        dst = y * LOGO_STRIDE * 2
        buf[dst:dst+len(row)] = row
    # vloz do iso - pixel data
    px_start = LOGO_OFFSET + LOGO_HDR
    data[px_start : px_start + buf_size] = buf

    # DOLEZITE: zachovaj magic oblast tesne pred eGON.EXE (0x50ff0-0x51000)
    # Tato oblast obsahuje magic hodnotu 0xaa555a5a ktoru updater overuje.
    # Ak ju vymazeme, updater odmietne .iso. Kopirujeme z povodneho suboru.
    orig = open(iso_path, 'rb').read()
    MAGIC_START = 0x50ff0
    MAGIC_END   = LOGO_BLOCK           # = 0x51000
    data[MAGIC_START:MAGIC_END] = orig[MAGIC_START:MAGIC_END]

    # zapis
    shutil.copy2(iso_path, out_iso) if out_iso != iso_path else None
    with open(out_iso, 'wb') as f:
        f.write(data)
    print(f'Hotovo: {logo_png} ({w}x{h}) vlozene do {out_iso}')
    print(f'Povodny subor: {iso_path} (nedotknuty)')
    print()
    print('UPOZORNENIE: vzdy si zalohuj povodny .iso pred nahratim do pristroja!')

if __name__=='__main__':
    if len(sys.argv) < 2 or sys.argv[1] not in ('info','extract','replace'):
        print(__doc__)
        sys.exit(1)
    cmd = sys.argv[1]
    if cmd == 'info' and len(sys.argv)==3:
        cmd_info(sys.argv[2])
    elif cmd == 'extract' and len(sys.argv)==4:
        cmd_extract(sys.argv[2], sys.argv[3])
    elif cmd == 'replace' and len(sys.argv)==5:
        cmd_replace(sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        print(__doc__)
        sys.exit(1)


# ──────────────────────────────────────────────────────────────────────────────
# CHECKSUM — DPOS350P firmware update checksum (discovered 2026-10-19)
# Algorithm: process iso[0x1000 : file_size-36] with relative indices,
#            9× big-endian uint32 stored in last 36 bytes of ISO.
# ──────────────────────────────────────────────────────────────────────────────

def compute_checksum(data: bytes) -> list[int]:
    """Compute 9-value checksum over iso[0x1000:file_size-36].
    data = raw ISO bytes (full file)."""
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
    """Recompute and write checksum footer into ISO bytes."""
    import struct
    cd = compute_checksum(data)
    footer = b"".join(struct.pack(">I", v) for v in cd)
    assert len(footer) == 36
    return data[:-36] + footer


def verify_checksum(data: bytes) -> bool:
    """Return True if ISO checksum is valid."""
    import struct
    stored = [struct.unpack_from(">I", data, len(data) - 36 + i * 4)[0]
              for i in range(9)]
    computed = compute_checksum(data)
    return computed == stored


if __name__ == "__main__":
    import sys, os

    cmd = sys.argv[1] if len(sys.argv) > 1 else "help"

    if cmd == "verify" and len(sys.argv) == 3:
        data = open(sys.argv[2], "rb").read()
        ok = verify_checksum(data)
        print(f"Checksum: {'OK' if ok else 'INVALID'}")
        sys.exit(0 if ok else 1)

    elif cmd == "patch" and len(sys.argv) in (3, 4):
        src = sys.argv[2]
        dst = sys.argv[3] if len(sys.argv) == 4 else src.replace(".iso", "_patched.iso")
        data = open(src, "rb").read()
        patched = patch_checksum(data)
        open(dst, "wb").write(patched)
        print(f"Patched → {dst}")

    else:
        print("Usage:")
        print("  dpos_tool.py verify  <file.iso>")
        print("  dpos_tool.py patch   <file.iso> [output.iso]")
        print("  dpos_tool.py info    <file.iso>")
        print("  dpos_tool.py extract <file.iso> <section> <out>")
        print("  dpos_tool.py replace <file.iso> <section> <in> <out>")
