# Display Initialization (Bare-metal)

Working bare-metal display and touch driver for FNIRSI DPOS350P (Allwinner R11/V3s).
See `tools/dpos_drivers/` for the complete modular implementation.

## Display specs

| Parameter | Value |
|-----------|-------|
| Module | HT070-RGB-50PIN-03 (HTdisplay) |
| Resolution | **1024 × 600** (IPS) |
| Interface | RGB565 parallel, 50-pin FFC |
| Touch | GT911 capacitive, separate smaller FFC |

## Timing (from SPL timing table at Winbond 0x6720)

| | H | V |
|-|---|---|
| Active | 1024 | 600 |
| Back porch | 160 | 12 |
| Front porch | 163 | 23 |
| Sync width | 1 | 1 |
| **Total** | **1348** | **636** |

PLL_VIDEO = 324 MHz (N=80, M=5) · DCLK = 324/8 = **40.5 MHz**

## Port D pinmux

PD pins 1–21 (excluding PD12) set to LCD function (function 2):

```c
for (pin = 1; pin <= 21; pin++)
    if (pin != 12)
        gpio_set_function(GPIO_PORT_D, pin, 2);
```

## GT911 touch controller

Hardware mapping confirmed by PCB continuity tracing and original firmware analysis:

| Signal | Pin |
|--------|-----|
| SDA | PG6 |
| SCL | PG7 |
| INT | PE22 |
| RST | PE24 |
| I2C address | 0x5D (write 0xBA / read 0xBB) |

Uses bit-banged I2C. Status register: `0x814E`, first touch point: `0x8150`.

## Framebuffers

Double-buffered to eliminate tearing. D-cache must be flushed before each swap
since DE2 reads directly from physical DRAM while the ARM CPU caches writes.

```
FB0 = 0x43000000   (1024 × 600 × 2 = 1,228,800 bytes)
FB1 = 0x43200000
```

`display_swap()` updates the DE2 layer address atomically and flips the buffers.

## CCU setup

```c
// PLL_VIDEO = 24*(N+1)/(M+1) = 24*81/6 = 324 MHz
CCU_PLL_VIDEO_CTRL = (1<<31)|(1<<24)|(80<<8)|5;
CCU_DE_CLK    = (1<<31);   // source = PLL_VIDEO, M=0
CCU_TCON0_CLK = (1<<31);
CCU_BUS_SOFT_RST1 |= (1<<12)|(1<<4);   // DE + TCON0
CCU_BUS_CLK_GATE1 |= (1<<12)|(1<<4);
```

## DE2 setup

```c
// DE2 internal CCU: reset → enable clocks → release
// Mixer channel 2 (MIXER_BASE + 0x4000):
CH2[0x00] = 0x00000A01;  // RGB565, enable
CH2[0x04] = (599<<16)|1023;  // size: 1024×600
CH2[0x08] = 0;               // coord: (0,0)
CH2[0x0C] = 2048;            // stride: 1024*2 bytes
CH2[0x10] = front_address;   // framebuffer
CH2[0x88] = (599<<16)|1023;  // ovl_size

// Blender route: pipe 0 → channel 2
BLD_BASE[0x80] = 2;
BLD_BASE[0x00] = (1<<8);          // pipe 0 enable
BLD_BASE[0x04] = 0xFF000000;      // fill color (opaque black)
BLD_BASE[0x08] = (599<<16)|1023;  // pipe 0 insize
BLD_BASE[0x8C] = (599<<16)|1023;  // output size
```

## TCON0 setup

```c
TCON0_DCLK          = (0xF<<28) | 8;         // 324/8 = 40.5 MHz
TCON0_TIMING_ACTIVE = (599<<16) | 1023;       // 1024×600
TCON0_TIMING_H      = (1347<<16) | (163+1-1); // HTotal-1 | HFP+HSW-1
TCON0_TIMING_V      = (636<<17)  | (23+1-1);  // VTotal*2 | VFP+VSW-1
TCON0_TIMING_SYNC   = (0<<16)    | 0;         // VSW-1 | HSW-1
TCON0_IO_POLARITY   = 0x17000000;
TCON0_IO_TRI        = 0;
```

## Register map

```
CCU_BASE         = 0x01C20000
  PLL_VIDEO_CTRL = +0x0010   BUS_CLK_GATE1 = +0x0064
  DE_CLK         = +0x0104   BUS_SOFT_RST1 = +0x02C4
  TCON0_CLK      = +0x0118

DE2_BASE         = 0x01000000
  CCU_GATE       = +0x00     CCU_BUS_GATE  = +0x04
  CCU_RST        = +0x08     CCU_SEL       = +0x10

MIXER_BASE       = 0x01100000
  GLB_CTL        = +0x0000   GLB_DBUFF     = +0x0008
  GLB_SIZE       = +0x000C
  BLD_PIPE_CTL   = +0x1000   BLD_ROUTE_CTL = +0x1080
  BLD_OUTPUT_SIZE= +0x108C   BLD_MODE0     = +0x1090
  CH2_BASE       = +0x4000

TCON0_BASE       = 0x01C0C000
  TCON_CTRL      = +0x000    TCON0_CTRL    = +0x040
  TCON0_DCLK     = +0x044    TCON0_TIMING_ACTIVE = +0x048
  TCON0_TIMING_H = +0x04C    TCON0_TIMING_V      = +0x050
  TCON0_TIMING_SYNC = +0x054 TCON0_IO_POL  = +0x088
  TCON0_IO_TRI   = +0x08C

PIO_BASE         = 0x01C20800
  PORT_B = +0x24  PORT_C = +0x48  PORT_D = +0x6C
  PORT_E = +0x90  PORT_G = +0xD8
```
