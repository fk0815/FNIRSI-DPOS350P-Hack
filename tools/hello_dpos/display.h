#ifndef DISPLAY_H
#define DISPLAY_H

#define CCU_BASE            0x01C20000
#define CCU_PLL_VIDEO_CTRL  (CCU_BASE + 0x0010)
#define CCU_BUS_CLK_GATE0   (CCU_BASE + 0x0060)
#define CCU_BUS_CLK_GATE1   (CCU_BASE + 0x0064)
#define CCU_DE_CLK          (CCU_BASE + 0x0104)
#define CCU_TCON0_CLK       (CCU_BASE + 0x0118)
#define CCU_BUS_SOFT_RST1   (CCU_BASE + 0x02C4)

#define TCON0_BASE          0x01C0C000
#define TCON_CTRL           (TCON0_BASE + 0x000)
#define TCON_INT0           (TCON0_BASE + 0x004)
#define TCON0_CTRL          (TCON0_BASE + 0x040)
#define TCON0_DCLK          (TCON0_BASE + 0x044)
#define TCON0_TIMING_ACTIVE (TCON0_BASE + 0x048)
#define TCON0_TIMING_H      (TCON0_BASE + 0x04C)
#define TCON0_TIMING_V      (TCON0_BASE + 0x050)
#define TCON0_TIMING_SYNC   (TCON0_BASE + 0x054)
#define TCON0_IO_POLARITY   (TCON0_BASE + 0x088)
#define TCON0_IO_TRI        (TCON0_BASE + 0x08C)

#define DE2_BASE            0x01000000
#define DE2_CCU_GATE        (DE2_BASE + 0x00)
#define DE2_CCU_BUS_GATE    (DE2_BASE + 0x04)
#define DE2_CCU_RST         (DE2_BASE + 0x08)
#define DE2_CCU_SEL         (DE2_BASE + 0x10)

#define MIX_BASE            0x01100000
#define GLB_CTL             (MIX_BASE + 0x0000)
#define GLB_DBUFF           (MIX_BASE + 0x0008)
#define GLB_SIZE            (MIX_BASE + 0x000C)
#define BLD_BASE            (MIX_BASE + 0x1000)

/*
 * SPRÁVNE rozlíšenie DPOS350P displeja: 1024 × 600 (IPS)
 * Viditeľné na spec: "Screen resolution: 1024 x 600 pixels"
 *
 * SPL timing table (0x6720) teraz dáva zmysel:
 * [0x04]=1024 = H_ACTIVE   [0x08]=160  = H_BP
 * [0x0c]=163  = H_FP       [0x10]=1    = H_SYNC
 * [0x14]=600  = V_ACTIVE   [0x18]=12   = V_BP
 * [0x1c]=23   = V_FP       [0x20]=1    = V_SYNC
 * [0x44]=8    = DCLK_DIV   [0x3c]=80   = PLL_N
 * [0x40]=5    = PLL_M
 *
 * H_TOTAL = 1024+160+163+1 = 1348
 * V_TOTAL = 600+12+23+1   = 636
 * PLL = 24*(80+1)/(5+1) = 324 MHz
 * DCLK = 324/8 = 40.5 MHz
 * FPS = 40.5M/(1348*636) = 47.2 Hz
 */
#define LCD_WIDTH    1024
#define LCD_HEIGHT   600

#define H_ACTIVE     1024
#define H_BP         160
#define H_FP         163
#define H_SYNC       1
#define H_TOTAL      (H_ACTIVE + H_BP + H_FP + H_SYNC)   /* 1348 */

#define V_ACTIVE     600
#define V_BP         12
#define V_FP         23
#define V_SYNC       1
#define V_TOTAL      (V_ACTIVE + V_BP + V_FP + V_SYNC)   /* 636 */

/* Framebuffer: 1024×600 RGB565 = 1,228,800 bytes */
#define FB_STRIDE        (H_ACTIVE * 2)    /* 2048 bytes/line */
#define FRAMEBUFFER_ADDR  0x43000000

/* RGB565 farby */
#define COLOR_RED    0xF800
#define COLOR_GREEN  0x07E0
#define COLOR_BLUE   0x001F
#define COLOR_WHITE  0xFFFF
#define COLOR_YELLOW 0xFFE0

#endif
