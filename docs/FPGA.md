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

The later routines appear related to programmable clock setup, but the target device, output routing and exact semantics remain to be confirmed before reproducing the complete function.

## Clock generator

An MS5351M with a 25 MHz reference has been identified near the FPGA subsystem. The startup values 95 MHz and 10 MHz make clock-generator control a strong hypothesis. Still unknown:

- which routine talks to the MS5351M
- physical SDA/SCL routing
- which output receives each frequency
- whether clock programming has any relay or analog-side effect

## Unmarked 8-pin device and four-pin header

The former W25N512GV claim is withdrawn. `EF 40 15` belongs to the W25Q16JV NOR, and no independent SPI-NAND read-ID has confirmed a W25N512GV.

Runtime captures on the nearby four-pin header show two-wire, I2C-like traffic, including address-0 decoding and short repeated transfers. Power-on timing appears less conventional. A similar FNIRSI design has been reported to proxy an external device through the FPGA, but applying that architecture to the DPOS350P remains a hypothesis.

Unconfirmed questions:

- Is the FPGA the master of the two-wire bus?
- Is the protocol valid I2C or only I2C-like?
- Is the device memory, a small MCU, calibration storage or an authentication component?
- Does `SMEC_Init()` trigger or consume the traffic?
- Is the FPGA configuration stored internally?

## SMEC

The FPGA project contains `smec.v`, and EXE2 calls `SMEC_Init()` after FAT/UI setup but before power, horizontal, vertical and generator initialization. No expansion of the acronym or functional meaning is currently justified.

Next analysis must inspect `SMEC_Init @ 0x4004A15C` for:

- callers and callees
- use of the known FPGA read/write primitives
- fixed command bytes
- eight-byte loops or buffers
- retries, timeouts and comparisons

## Functional hypotheses

The following are reasonable search guides, not established mappings:

- `HorizontalInit`: timebase, sample rate, depth or acquisition timing
- `VerticalInit`: channel gain/offset, coupling, bandwidth or ADC-facing setup
- `Gen_Init`: waveform-generator and DAC-RAM setup
- FRA: likely composes generator, acquisition and amplitude/phase processing rather than requiring a separate boot-time subsystem

## Next steps

1. fully reconstruct `FPGA_Start()` and its clock-related helpers
2. reproduce the complete function in an isolated Custom App
3. disassemble `SMEC_Init()` and correlate commands with logic-analyzer captures
4. map callers of the three low-level FPGA primitives
5. build a read-only register/status map before attempting acquisition control
6. analyze `HorizontalInit`, `VerticalInit` and `Gen_Init` only after shared primitives are named
