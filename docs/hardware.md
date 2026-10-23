# Hardware

This document separates confirmed findings from open hypotheses.

## Confirmed components

### Allwinner R11 / V3s-compatible SoC

- ARM Cortex-A7, physically marked R11
- FEL ID `0x00168100`
- 64 MiB DDR2 at `0x40000000`
- `xfel ddr v3s` works on the tested unit
- UART0 PB8/PB9 is not known to be exposed on a header

### GOWIN FPGA

- GW1N-9 family, marked `GW1N-LV9MG256C5/I5`
- used for acquisition-related logic and generator-related functions
- exact configuration-storage path remains unconfirmed

### W25Q16JV SPI NOR

- 2 MiB, 3.3 V, SOP-8
- JEDEC `EF 40 15`
- connected to the Allwinner SPI0 path
- contains BT0/SPL, updater, logo, main ARM application and preserved device data

### CSNP1GCR01-B0W managed NAND

- SD-protocol managed NAND
- connected to **SDC0** through **PF0..PF5**
- tested unit contains a partition of type `0x06` beginning at LBA 63
- tested filesystem is FAT16
- used by the original USB/update environment and by the Custom App loader

### Display and touch

- 1024x600 RGB565 panel
- Goodix GT911 touch controller
- GT911: PG6 SDA, PG7 SCL, PE22 INT, PE24 RESET, address `0x5D`

## Unmarked 8-pin device near the FPGA

The device was previously documented as a Winbond W25N512GV SPI NAND containing the FPGA bitstream. That identification is **not confirmed** and must not be treated as fact.

Why the old identification was withdrawn:

- `EF 40 15` is the confirmed JEDEC ID of the 2 MiB W25Q16JV NOR
- `xfel spinand` returning those bytes does not prove a second SPI-NAND device
- the chip marking is absent
- activity observed on the nearby four-pin header is two-wire and I2C-like during normal operation

Observed by another researcher:

- two signal lines plus power/ground on the nearby header
- runtime traffic that can be partly decoded as address 0 with short repeated transfers
- power-on traffic with less conventional timing

Still unconfirmed:

- device type and exact pinout
- whether the FPGA is the bus master
- whether the traffic is standards-compliant I2C
- whether the device stores configuration, calibration or authentication data
- any relationship with `SMEC_Init()` or `smec.v`

The GW1N family is non-volatile, so internal FPGA configuration storage is a viable hypothesis, not a confirmed conclusion.

## W25Q16JV map

```text
0x000000..0x0067FF  eGON.BT0 / original or custom Stage 1
0x018000..0x03C7FF  original EXE1 / USB updater
0x058000             original logo block region
0x0A8000             original EXE2 header/application region
0x1DB000..0x1EAFFF  custom Stage 2 slot
0x1EB000..0x1EEFFF  calibration/configuration, preserve
0x1F0000..0x1FDFFF  metadata/version/configuration, preserve
```

Offsets not explicitly allocated must be preserved. All-`0xFF` data is not sufficient evidence that a region is generally free.

## Debug headers

### SPI/FEL header

The SPI header exposes the W25Q16 signals and reset. Shorting SPI CS to ground during power-on forces FEL on the tested unit. Keep a full backup before any write.

### FPGA JTAG header

A nearby 2x3 header is believed to expose GOWIN JTAG signals, but package-pin claims and tool compatibility should remain qualified until a complete trace and successful transaction are documented.

### Four-pin header near FPGA

Not UART. Current captures show two-wire I2C-like activity during normal operation. See the unmarked-device section above.

## Original EXE2 startup flow

Confirmed call sequence around the application startup:

```text
sys_clock_init
sys_mmu_init
System_Init
AllInfoInit
spi_flash_init
GetSysSavedInfo
LanguageInit
FPGA_Start
GUI_Init
GT911_Init
Win_Init
UI_ColorInit
FatFsInit
SMEC_Init
PowerInit
HorizontalInit
VerticalInit
HardCostTimeInfoInit
Gen_Init
CheckAndUpdateNewSystem
FactoryCheck
SysConfigExec
```

Key confirmed addresses:

```text
main                    0x40034700
FPGA_Start              0x40049188
GUI_Init                0x400344E0
GT911_Init              0x40049A74
SMEC_Init               0x4004A15C
CheckAndUpdateNewSystem 0x4004F134
FactoryCheck            0x40051CF0
SysConfigExec           0x4003E708
```

The earlier address table shifted several names by one call and is superseded by this sequence.
