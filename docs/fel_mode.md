# FEL Mode

Allwinner V3s supports FEL mode — a USB-based bootloader for reading/writing memory and flash without any running firmware.

## Setup (Windows)

1. Install [xfel v1.4.0](https://github.com/xboot/xfel/releases)
2. Connect oscilloscope via USB-C
3. Install **libusbK** driver via [Zadig](https://zadig.akeo.ie/) for the FEL USB device

## Entering FEL Mode

Short **SPI CS (PC2)** to **GND** using the SPI flash header at power-on:

```
SPI flash header:
[ GND    ] [ GND   ]
[ 3.3V   ] [ RESET ]
[ SPI CS ]←short to GND at power-on
[ MISO   ] [ MOSI  ]
```

The wire can be released immediately after the device powers on — FEL stays active via USB. The oscilloscope screen remains dark; check with `xfel version`.

## Verified xfel Commands

```bash
# Confirm FEL mode and chip identity
xfel version
# → DRAM: 64M V3s

# Read chip SID (unique device ID)
xfel sid
# → 12c00001740046207905431c34490f50

# Dump full Winbond W25Q16JV (2MB)
xfel spinor read 0 0x200000 winbond_backup.bin

# Write to Winbond (careful!)
xfel spinor write 0x58000 new_logo_section.bin

# Detect SPI NAND (FPGA flash — read not supported by xfel)
xfel spinand
# → ID=0xef401500 (Winbond W25N512GV)

# Initialize DDR RAM
xfel ddr v3s  # v3s profile works for R11

# Write binary to RAM
xfel write 0x40000000 payload.bin

# Hexdump RAM
xfel hexdump 0x40000000 0x40
```

## Memory Map (RAM)

| Address | Content |
|---------|---------|
| 0x40000000 | Main application load address |
| 0x40000000 | Updater load address (same) |
| 0x4001f424 | "Found new firmware!" string (updater) |
| 0x400234d8 | "DPOS350P" string (updater) |
| 0x48000000 | ISO written here during FEL experiments |

## SDC2 Controller

The SD-NAND (CSNP1GCR01-B0W) is on the SDC2 controller at `0x01C11000`.  
In FEL mode this register reads `0x00000000` — SDC2 is not initialized.  
Accessing SD-NAND from FEL requires a custom ARM payload to initialize SDC2.

## Notes

- The `xfel spinand` command detects the W25N512GV but cannot read or write it
- The 4-pin header on the PCB does **not** appear to be UART (see hardware.md)
- UART0 (PB8/PB9, chip pins 54/55) is not broken out to any header
