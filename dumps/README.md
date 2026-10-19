# Dumps

Firmware binaries are **not** included in this repository for licensing reasons.

## What to Back Up (and How)

### 1. Winbond W25Q16JV (2MB NOR) — strongly recommended

Via FEL (easiest, no soldering):
```bash
xfel ddr v3s
xfel spinor read 0 0x200000 winbond_backup.bin
```

Via T48 programmer (in-circuit via SPI header):
- Connect MISO/CLK/CS/MOSI to T48 SPI header pins
- Hold RESET low during read
- Read as W25Q16JV

**MD5 of known-good dump:** verify your dump matches before modifying anything.

### 2. Winbond W25N512GV (64MB SPI NAND) — FPGA bitstream

xfel cannot read this chip. Options:
- Desolder and read with T48 (`W25N512GV @WSON8`, 3.3V)
- Read via GOWIN JTAG header using openFPGALoader + FT232H

### 3. SD-NAND CSNP1GCR01-B0W (1GB) — FAT32 filesystem

Accessible as USB mass storage when device is running (Menu → USB Connect).  
Raw dump requires an ARM payload to initialize the SDC2 controller via FEL.

## Checksums of Original Firmware ISO

```
File:   DPOS350P-FNIRSI-2025619.iso
Size:   948310 bytes
MD5:    (add your hash here)
SHA256: (add your hash here)
```

Verify before modifying:
```bash
md5sum DPOS350P-FNIRSI-2025619.iso
sha256sum DPOS350P-FNIRSI-2025619.iso
```
