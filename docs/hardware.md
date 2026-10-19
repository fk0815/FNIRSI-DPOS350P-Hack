# Hardware

## PCB Components

### Allwinner V3s (SoC)
- ARM Cortex-A7, confirmed via FEL (`xfel version` → ID=0x00168100)
- RAM: DDR from 0x40000000, ~64MB
- System clock: 1.152 GHz
- Chip SID: `12c00001740046207905431c34490f50`
- UART0: PB8 (pin 54, TX) / PB9 (pin 55, RX) — not broken out to any header

### GOWIN GW1N-LV9MG256C5/I5 (FPGA)
- GW1N-9 series, 8640 LUT, BGA256, 0.5mm pitch, 8×8mm
- Handles ADC sampling, FFT, signal generator
- Bitstream stored on W25N512GV (SPI NAND)
- Pinout reference: GOWIN document UG114 (MG256 package)

### Winbond W25Q16JV (SPI NOR, 2MB)
- Package: SOP-8, 3.3V
- Contains: BT0/SPL, updater, boot logo, main ARM app, config/calibration
- Accessible in-circuit via SPI header or via FEL (`xfel spinor`)

### Winbond W25N512GV (SPI NAND, 64MB)
- Package: WSON-8, 3.3V (marking abraded off PCB)
- Contains: GOWIN FPGA bitstream
- xfel detects as ID=0xef401500 but does not support read/write

### CSNP1GCR01-B0W (SD-NAND, 1GB)
- LGA-8, SD protocol, connected to V3s SDC2 controller (PC0–PC3)
- FAT32 filesystem
- Holds firmware ISO during update process

---

## Winbond W25Q16JV Flash Map

| Offset | Size | Content |
|--------|------|---------|
| 0x00000 | 26624B | BT0/SPL (eGON.BT0, checksum 0x05d89244 ✓) |
| 0x08000 | — | Empty |
| 0x18000 | 149504B | Updater (eGON.EXE, loads to RAM 0x40000000) |
| 0x58000 | — | Boot logo (eGON.BMP) |
| 0xa8000 | 612352B | Main application (eGON.EXE) |
| 0x13e832 | 36B | Firmware checksum footer (mirrors ISO footer) |
| 0x1eb000 | — | ADC calibration data |
| 0x1f3000 | — | Wear-leveling index / version tables |
| 0x1f5000 | — | Migration magic values (5a5555a5 / a5aa555a …) |

---

## Debug Headers

### SPI Flash Header (6-pin 2×3)

Used for: in-circuit Winbond read/write, FEL entry.

```
[ GND    ] [ GND   ]   ← top row (towards PCB edge)
[ 3.3V  ] [ RESET  ]   P56 VCC-IO3 / P94
[ SPI CS ] [ SPI CLK]  PC2 (P60)   / PC1 (P59)
[ MISO  ] [ MOSI   ]   PC0 (P58)   / PC3 (P61)
```

**FEL entry:** Short SPI CS to GND at power-on.  
Wire can be released immediately after power-on; FEL stays active via USB.

**T48 in-circuit read:** Connect MISO/CLK/CS/MOSI to T48, hold RESET low.

### FPGA JTAG Header (6-pin 2×3)

Used for: GowinEDA or openFPGALoader bitstream read/write.

```
[ GND   ] [ GND   ]   ← top row
[ 3.3V  ] [ 1.1V  ]   P56 VCC-IO3 / P69 VDD-SYS
[ TCK   ] [ TDI   ]   GOWIN pin 17 / pin 19
[ TDO   ] [ TMS   ]   GOWIN pin 20 / pin 16
```

Pin 18 (SCLK) is not connected.  
Pinout verified against GOWIN LQ176 package table (pins 16/17/19/20 = TMS/TCK/TDI/TDO).  
Requires FT2232H or FT232H adapter (MPSSE capable) for openFPGALoader.

### 4-pin Header (unconfirmed)

- GND + 3.3V (stable from battery)
- Two pins measuring 1.5–2.6V during boot
- Suspected USB D+/D− (both pins show identical output at any baud rate)
- Not UART — further investigation needed

---

## Boot Sequence

```
BT0/SPL (Winbond 0x00000)
  → sys_clock_init (1.152 GHz)
  → sys_mmu_init
  → System_Init
  → spi_flash_init (SPI0)
  → Load main app (Winbond 0xa8000 → RAM 0x40000000)
  → Jump to main app

Main app (RAM 0x40000000)
  → FPGA_Start
  → GUI_Init
  → GT911_Init (touch)
  → while(1) { TP_ScanMain, Key_ControlHandle, Win_DisplayHandle }
```

Bare-metal application — no Linux, no RTOS.

---

## Key Function Addresses (RAM)

| Function | Address |
|----------|---------|
| main() init | 0x40034700 |
| sys_clock_init | 0x4004282c |
| FPGA_Start | 0x400344e0 |
| GUI_Init | 0x40049a74 |
| GT911_Init | 0x4003d454 |
| CheckAndUpdateNewSystem | 0x40051cf0 |
| FactoryCheck | 0x4003e708 |
