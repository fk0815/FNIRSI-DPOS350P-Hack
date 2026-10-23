# DPOS350P FPGA Reverse Engineering Notes

## Scope

These notes distinguish hardware-confirmed behavior, firmware-confirmed behavior and hypotheses. The FPGA work is separate from the already working custom boot chain.

## Original project evidence

Developer screenshots show FPGA modules including `adc.v`, `analog.v`, `clk_div.v`, `clk_inside.v`, `clk_sel.v`, `clock.v`, `gen.v`, `pll_dynamic.v`, `ram_adc.v`, `ram_dac.v`, `sample_trig.v`, `smec.v`, `top.v` and `trans.v`. MCU-side filenames include `adc_phase.c`, `fpga_drv.c`, `sample.c`, `trigger.c`, `vertical.c`, `horizontal_main.c`, `horizontal_zoom.c`, `gen.c` and `factory_check.c`.

These names provide useful search anchors, not a verified register map.

## Confirmed MCU-to-FPGA interface

Static EXE2 analysis and a standalone Custom App confirm a proprietary parallel interface:

```text
PE0..PE3    control signals
PE8..PE15   8-bit bidirectional data bus

PE_CFG0     0x01C20890
PE_CFG1     0x01C20894
PE_DATA     0x01C208A0
```

The PE port is dual-purpose: pinmux function 2 muxes the same pins to the
R11 CSI (camera) input - PE0=CSI_PCLK, PE2=CSI_HSYNC, PE3=CSI_VSYNC,
PE8..PE15=CSI_D4..CSI_D11 (PE4..PE7=CSI_D0..D3 and PE1=CSI_MCLK are unused).
Register access runs in GPIO mode; waveform bulk streaming runs in CSI mode
(confirmed in the 2025-06-19 update image, see `FPGA_Start()` section).

Confirmed EXE2 functions:

```text
main                 0x40034700
FPGA_Start           0x40049188
FPGA byte read       0x400491B8
FPGA write variant A 0x40049214
FPGA write variant B 0x4004927C
FPGA ID probe        0x400492E4
SMEC_Init            0x4004A15C
```

The Custom App reproduced the original identification transaction:

```text
write selector 0x01
read dummy/stale byte 0x00
read A5 A5 5A 5A
```

The original firmware compares the four valid bytes with `0xA5A55A5A`. The hardware result was:

```text
RAW5       00 A5 A5 5A 5A
ID [0..3]  0x00A5A55A
ID [1..4]  0xA5A55A5A
```

This confirms the bus direction switching, command/write path and byte-read path. The minimal transaction did not cause the relay to click.

## `FPGA_Start()` status

The current Custom App reproduces the parallel-bus setup and ID transaction, **not the entire `FPGA_Start()`**.

The original function is synchronous from the point of view of `main()`:

```text
main BL FPGA_Start at 0x40034738
  -> 0x40049188
  -> helper 0x400660C8 with value 8000
  -> bus-related helper 0x40049094
  -> tail call 0x400490B8
       -> 0x4005F1BC
       -> 0x4005F1F0 with 95,000,000
       -> 0x4005F38C with 10,000,000
  -> return to main at 0x4003473C
```

All routines in this trace are now identified (cross-checked against the
2025-06-19 update image, whose FPGA-block addresses are EXE2 + 0x80020):

- `0x400660C8` (+0x80020 = 0x400E60E8) is **not a delay**: it is the R11 CSI
  (camera-input) driver init at MMIO base 0x01CB4000, with the value 8000
  stored in the CSI channel descriptor. `FPGA_Start()` sets up the CSI
  capture port - the waveform streaming path.
- `0x40049094` / `0x400490B8` (0x400C90B4 / 0x400C90D8) set the PE-port role
  (GPIO bus idle, then chain into the clock programming).
- `0x4005F1BC` (0x400DF1DC) configures PE20/PE21 as GPIO for the I2C master.
- `0x4005F1F0` (0x400DF210) programs the MS5351M to 95 MHz and
  `0x4005F38C` (0x400DF3AC) to 10 MHz, via the PE20/PE21 I2C master
  (see "Clock generator").

## Clock generator - confirmed

The MS5351M is programmed by the R11 through a bit-banged I2C master on:

```text
PE20 = SCL
PE21 = SDA
device write address 0xC0 (7-bit 0x60), Si5351-style register protocol
(register 177 = 0x80 PLL reset, multisynth P1/P2/P3 parameter blocks)
```

- EXE2 / update-image routines: I2C bit-bang loop around 0x4005F018..0x4005F034
  (0x400DF038..0x400DF054), frequency planner 0x4005F1F0 (0x400DF210, 95 MHz)
  and 0x4005F38C (0x400DF3AC, 10 MHz), PE20/PE21 GPIO setup 0x4005F1BC
  (0x400DF1DC) - all reached from `FPGA_Start()`.
- The updater (EXE1) carries the same driver and programs 125 MHz at boot
  (0x4003659C -> planner 0x40035BDC, bit-send 0x40035AA8, register write
  0x40035B50, PE20/PE21 init 0x40035BA8).
- Which MS5351M output receives 95 MHz vs 10 MHz, and which FPGA block
  consumes each clock, still needs board-level confirmation.

## Unmarked 8-pin device and four-pin header

The former W25N512GV claim is withdrawn. `EF 40 15` belongs to the W25Q16JV NOR, and no independent SPI-NAND read-ID has confirmed a W25N512GV.

Runtime captures on the nearby four-pin header show two-wire, I2C-like traffic, including address-0 decoding and short repeated transfers. Power-on timing appears less conventional. A similar FNIRSI design has been reported to proxy an external device through the FPGA, but applying that architecture to the DPOS350P remains a hypothesis.

Unconfirmed questions:

- Is the FPGA the master of the two-wire bus?
- Is the protocol valid I2C or only I2C-like?
- Is the device memory, a small MCU, calibration storage or an authentication component?
- Does `SMEC_Init()` trigger or consume the traffic?

Firmware reference (confirmed): the R11 drives its own bit-banged I2C master
on PE20/PE21 to device 0x60 - the MS5351M (see "Clock generator"). Whether
the four-pin header carries that bus or a separate FPGA-mastered bus is not
yet resolved: the captured "address-0" decoding does not match 0x60, so the
header traffic may belong to a second, FPGA-side two-wire device. Decoding
the captured address byte against 0xC0/0x60 would settle it.
- Is the FPGA configuration stored internally?

## SMEC

The FPGA project contains `smec.v`, and EXE2 calls `SMEC_Init()` after FAT/UI setup but before power, horizontal, vertical and generator initialization. No expansion of the acronym or functional meaning is currently justified.

Next analysis must inspect `SMEC_Init @ 0x4004A15C` for:

- callers and callees
- use of the known FPGA read/write primitives
- fixed command bytes
- eight-byte loops or buffers
- retries, timeouts and comparisons

Update: `SMEC_Init()` is identified via the 2025-06-19 update image. The
update image's FPGA-block addresses are EXE2 + 0x80020 (constant verified
across the whole primitives block), and 0x4004A15C + 0x80020 = 0x400CA17C
is that image's "full push" routine: state memset (0x2300), shadow clear,
FPGA cmd 0x67+8 (reset/init), 2000-register write as 16-bit values, then the
7-register bulk push (regs 1-7, sizes {1,4,4,4,4,2,4}). `SMEC_Init()` is
thus a full FPGA register-file re-init built on the known low-level
primitives. Direct disassembly of EXE2 to confirm the arithmetic match is
still pending.

## Functional hypotheses

The following are reasonable search guides, not established mappings:

- `HorizontalInit`: timebase, sample rate, depth or acquisition timing
- `VerticalInit`: channel gain/offset, coupling, bandwidth or ADC-facing setup
- `Gen_Init`: waveform-generator and DAC-RAM setup
- FRA: likely composes generator, acquisition and amplitude/phase processing rather than requiring a separate boot-time subsystem

## Next steps

1. done - `FPGA_Start()` and its clock helpers are reconstructed (this file,
   `FPGA_Start()` and "Clock generator" sections)
2. reproduce the complete function in an isolated Custom App
3. mostly done - `SMEC_Init()` identified via the update-image
   cross-reference (see SMEC section); remaining: correlate its commands
   (0x67+8, 16-bit writes, 7-reg bulk) with logic-analyzer captures
4. map callers of the three low-level FPGA primitives
5. build a read-only register/status map before attempting acquisition control
6. analyze `HorizontalInit`, `VerticalInit` and `Gen_Init` only after shared primitives are named
7. capture the CSI streaming path: run the scope in CSI mode (run-state
   nonzero) and scope PE0 (CSI_PCLK) / PE2 (HSYNC) / PE3 (VSYNC) /
   PE8..PE15 during acquisition to confirm the waveform transfer
8. check the bitstream question: no firmware image programs the GW1N-9
   configuration (see 06-nor-boot-update.md in the companion analysis) -
   bitstream updates go through the GOWIN JTAG header only
