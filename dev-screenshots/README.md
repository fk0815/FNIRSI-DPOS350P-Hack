# Developer Screenshots

Screenshots preserved from public DPOS350P development/forum material
for reverse-engineering reference.

## v3s-source-tree-main.png

Shows part of the original V3s application source tree and the visible
initialization sequence from main().

Notable visible items include:

- fpga_drv.c
- sample.c
- trigger.c
- vertical.c
- horizontal_main.c
- gen.c
- factory_check.c
- update.c
- usb_trans.c

Visible initialization calls include:

- FPGA_Start()
- GUI_Init()
- GT911_Init()
- FatFsInit()
- SMEC_Init()
- HorizontalInit()
- VerticalInit()
- Gen_Init()
- CheckAndUpdateNewSystem()
- FactoryCheck()
- SysConfigExec()

One source filename between save_sys_info.c and spectrum.c was
intentionally obscured in the original screenshot. The filename is unknown.

## fpga-verilog-project-tree.png

Shows part of the original FPGA project tree.

Visible Verilog modules include:

- adc.v
- analog.v
- clk_div.v
- clk_inside.v
- clk_sel.v
- clkdiv4/clkdiv4.v
- clock.v
- gen.v
- hclk_en.v
- ip_ram/ram_adc.v
- pll_dynamic.v
- ram_dac/ram_dac.v
- sample_trig.v
- smec.v
- top.v
- trans.v

Visible constraint files:

- fpga_project.cst
- fpga_project.sdc

## Notes

These screenshots are reference evidence only. Information inferred from
them should be independently verified against firmware or hardware where
possible.

Original source/forum URL:
TODO

Archived:
2026-10-22
