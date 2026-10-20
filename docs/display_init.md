# Display Initialization (Bare-metal)

Working bare-metal display init for FNIRSI DPOS350P (Allwinner R11/V3s).

## Display specs

| Parameter | Value |
|-----------|-------|
| Module | HT070-RGB-50PIN-03 (HTdisplay) |
| Resolution | **1024 × 600** (IPS) |
| Interface | RGB565 parallel, 50-pin FFC |
| Touch | GT911 capacitive (separate smaller FFC) |

## Timing (from SPL timing table at Winbond 0x6720)

| | H | V |
|-|---|---|
| Active | 1024 | 600 |
| Back porch | 160 | 12 |
| Front porch | 163 | 23 |
| Sync width | 1 | 1 |
| **Total** | **1348** | **636** |

PLL_VIDEO = 324 MHz (N=80, M=5) · DCLK = 324/8 = **40.5 MHz** · ~47 fps

## Init sequence

### 1. Port D pinmux (LCD function)

```c
writel(0x22222222, 0x01C2086C);  // PD_CFG0: PD0-PD7
writel(0x22222222, 0x01C20870);  // PD_CFG1: PD8-PD15
writel(0x22222222, 0x01C20874);  // PD_CFG2: PD16-PD23
```

### 2. CCU clocks

```c
// PLL_VIDEO = 24*(N+1)/(M+1) = 324 MHz
CCU_PLL_VIDEO_CTRL = (1<<31)|(1<<24)|(80<<8)|(5<<0);
while (!(CCU_PLL_VIDEO_CTRL & (1<<28)));

CCU_DE_CLK    = (1<<31)|(0<<24)|(0<<0);  // source=PLL_VIDEO
CCU_TCON0_CLK = (1<<31)|(0<<24)|(0<<0);
CCU_BUS_SOFT_RST1 |= (1<<12)|(1<<4);     // de-assert DE+TCON0 reset
CCU_BUS_CLK_GATE1 |= (1<<12)|(1<<4);     // enable DE+TCON0 AHB clocks
```

### 3. DE2 internal CCU

```c
DE2_CCU_RST      &= ~1;   // assert Mixer0 reset
DE2_CCU_BUS_GATE |=  1;   // Mixer0 bus clock on
DE2_CCU_GATE     |=  1;   // Mixer0 module clock on
DE2_CCU_RST      |=  1;   // de-assert reset
DE2_CCU_SEL       =  0;   // Mixer0 → TCON0
```

### 4. DE2 Mixer

```c
#define ACTIVE ((600-1)<<16|(1024-1))   // = 0x025703FF

// Global
GLB_SIZE = ACTIVE;   // mixer output = 1024×600
GLB_CTL  = 1;

// Blender
BLD_PIPE_CTL   |= (1<<8);               // enable pipe 0
BLD_ROUTE_CTL   = (BLD_ROUTE_CTL & ~0xF) | 2;  // pipe 0 → channel 2
BLD_OUTPUT_SIZE = ACTIVE;
BLD_MODE0       = 0x03010301;

// Channel 2 (MIXER_BASE + 0x4000)
CH2[0x00] = 0x0a01;       // attr: RGB565, enable (format=10, no UI_SEL)
CH2[0x04] = ACTIVE;       // size: 1024×600
CH2[0x08] = 0;            // coord: (0, 0)
CH2[0x0C] = 2048;         // stride: 1024 * 2 bytes
CH2[0x10] = 0x43000000;   // framebuffer address
CH2[0x88] = ACTIVE;       // ovl_size

GLB_DBUFF = 1;            // commit
```

### 5. TCON0

```c
TCON_CTRL           = 0;
TCON_INT0           = 0;
TCON0_CTRL          = (1<<31)|(30<<4);    // enable, clk_delay=30
TCON0_DCLK          = (0xF<<28)|(8<<0);   // enable, 324/8=40.5 MHz
TCON0_TIMING_ACTIVE = ACTIVE;             // 1024×600
TCON0_TIMING_H      = (1347<<16)|160;     // HTotal-1, HBP+HSW-1
TCON0_TIMING_V      = (635<<16)|12;       // VTotal-1, VBP+VSW-1
TCON0_TIMING_SYNC   = 0;                  // HSW=1, VSW=1
TCON0_IO_POLARITY   = 0;
TCON0_IO_TRI        = 0;
TCON_CTRL           = (1<<31);            // enable TCON
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
  CH2_BASE       = +0x4000   (attr/size/coord/stride/laddr at +0/4/8/C/10, ovl at +0x88)

TCON0_BASE       = 0x01C0C000
  TCON_CTRL      = +0x000    TCON0_CTRL    = +0x040
  TCON0_DCLK     = +0x044    TCON0_TIMING_ACTIVE = +0x048
  TCON0_TIMING_H = +0x04C    TCON0_TIMING_V      = +0x050
  TCON0_TIMING_SYNC = +0x054 TCON0_IO_POL  = +0x088
  TCON0_IO_TRI   = +0x08C

PIO_BASE         = 0x01C20800
  PD_CFG0        = +0x6C     PD_CFG1       = +0x70
  PD_CFG2        = +0x74
```

## Full example

See `tools/hello_dpos/` — complete working bare-metal demo (solid color cycling).
