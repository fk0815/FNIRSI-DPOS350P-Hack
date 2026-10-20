#include "display.h"

#define PIO_BASE    0x01C20800
#define PC_CFG0     (PIO_BASE + 0x48)
#define PC_DATA     (PIO_BASE + 0x58)
#define PD_CFG0     (PIO_BASE + 0x6C)
#define PD_CFG1     (PIO_BASE + 0x70)
#define PD_CFG2     (PIO_BASE + 0x74)
#define PB_CFG0     (PIO_BASE + 0x24)
#define PB_DATA     (PIO_BASE + 0x34)
#define PE_CFG0     (PIO_BASE + 0x90)
#define PE_DATA     (PIO_BASE + 0xA0)

static inline void     writel(unsigned int v, unsigned int a) { *(volatile unsigned int*)a = v; }
static inline unsigned readl (unsigned int a)                 { return *(volatile unsigned int*)a; }
static void delay(unsigned int n) { volatile unsigned int i; for(i=0;i<n*10000;i++); }

static void gpio_init(void)
{
    unsigned int v;
    v = readl(CCU_BUS_CLK_GATE0); writel(v&~(1U<<20), CCU_BUS_CLK_GATE0);
    v = readl(PC_CFG0); v&=~0xFFFF; v|=0x1111; writel(v, PC_CFG0);
    writel(0x00, PC_DATA);
    writel(0x22222222, PD_CFG0);
    writel(0x22222222, PD_CFG1);
    writel(0x22222222, PD_CFG2);
    v = readl(PB_CFG0); v=(v&0xFF000000)|0x00111111; writel(v, PB_CFG0);
    writel(0xFF, PB_DATA);
    v = readl(PE_CFG0); v=(v&0xFFFF0000)|0x00001111; writel(v, PE_CFG0);
    writel(0x0F, PE_DATA);
    delay(100);
}
static void set_pin(int p) { writel(readl(PC_DATA)|(1<<p), PC_DATA); }

static void ccu_init(void)
{
    unsigned int v;
    /* PLL_VIDEO = 24*(N+1)/(M+1) = 24*81/6 = 324 MHz (from SPL timing table) */
    writel((1U<<31)|(1U<<24)|(80U<<8)|(5U<<0), CCU_PLL_VIDEO_CTRL);
    while (!(readl(CCU_PLL_VIDEO_CTRL) & (1U<<28)));
    writel((1U<<31)|(0U<<24)|(0U<<0), CCU_DE_CLK);
    writel((1U<<31)|(0U<<24)|(0U<<0), CCU_TCON0_CLK);
    v = readl(CCU_BUS_SOFT_RST1); writel(v|(1U<<12)|(1U<<4), CCU_BUS_SOFT_RST1); delay(1);
    v = readl(CCU_BUS_CLK_GATE1); writel(v|(1U<<12)|(1U<<4), CCU_BUS_CLK_GATE1);
}

static void de2_init(void)
{
    /* Active display area: 1024×600 pixels, RGB565 (2 bytes/pixel) */
    unsigned int active = ((LCD_HEIGHT-1)<<16)|(LCD_WIDTH-1); /* (599<<16)|1023 */
    unsigned int ch, i;

    /* DE2 internal clock controller */
    writel(readl(DE2_CCU_RST)&~1U, DE2_CCU_RST); delay(1);
    writel(readl(DE2_CCU_BUS_GATE)|1U, DE2_CCU_BUS_GATE);
    writel(readl(DE2_CCU_GATE)|1U, DE2_CCU_GATE); delay(1);
    writel(readl(DE2_CCU_RST)|1U, DE2_CCU_RST); delay(1);
    writel(0, DE2_CCU_SEL);

    /* Clear all channels */
    for (ch=0; ch<4; ch++) {
        volatile unsigned int *p = (volatile unsigned int*)(MIX_BASE+0x2000+ch*0x1000);
        for (i=0; i<0x200; i++) p[i]=0;
    }

    /* Global: mixer output size = active area */
    writel(active, GLB_SIZE);
    writel(1U, GLB_CTL);

    /* Blender */
    writel(readl(BLD_BASE+0x00)|(1U<<8), BLD_BASE+0x00);  /* enable pipe 0 */
    writel((readl(BLD_BASE+0x80)&~0xF)|2U, BLD_BASE+0x80); /* route pipe 0 to channel 2 */
    writel(active, BLD_BASE+0x8C);   /* blender output = active area */
    writel(0x03010301, BLD_BASE+0x90);

    /* Channel 2 (MIXER+0x4000) — matches SPL:
     * attr=0x0a01 (RGB565, enable, no UI_SEL), size=active area, stride=2048 */
#define CH2 (MIX_BASE + 0x4000)
    writel(0x00000a01, CH2+0x00);    /* format 10 = RGB565, layer enable */
    writel(active,     CH2+0x04);    /* layer size = active area */
    writel(0,          CH2+0x08);    /* layer position: top-left */
    writel(FB_STRIDE,  CH2+0x0C);    /* line stride: 1024 pixels * 2 bytes */
    writel(FRAMEBUFFER_ADDR, CH2+0x10);
    writel(active,     CH2+0x88);    /* ovl size */

    writel(1U, GLB_DBUFF);
    delay(1);
}

static void tcon0_init(void)
{
    /*
     * TCON0 timing for 1024x600 display (from SPL):
     * H: active=1024, BP=160, FP=163, SW=1, total=1348
     * V: active=600,  BP=12,  FP=23,  SW=1, total=636
     * DCLK = 324 MHz / 8 = 40.5 MHz, ~47 fps
     */
    unsigned int ht  = H_TOTAL - 1;              /* 1347 */
    unsigned int hbp = H_BP + H_SYNC - 1;        /* 160  */
    unsigned int vt  = V_TOTAL - 1;              /* 635  */
    unsigned int vbp = V_BP + V_SYNC - 1;        /* 12   */

    writel(0, TCON_CTRL);
    writel(0, TCON_INT0);
    writel((1U<<31)|(30U<<4), TCON0_CTRL);
    /* pixel clock: enable (0xF<<28), divisor=8 → 40.5 MHz */
    writel((0xFU<<28)|(6U<<0), TCON0_DCLK);  /* 324/6=54MHz → 63fps */
    /* active display area */
    writel(((LCD_HEIGHT-1)<<16)|(LCD_WIDTH-1), TCON0_TIMING_ACTIVE);
    writel((ht<<16)|hbp, TCON0_TIMING_H);
    writel((vt<<16)|vbp, TCON0_TIMING_V);
    writel(((V_SYNC-1)<<16)|(H_SYNC-1), TCON0_TIMING_SYNC);
    writel(0, TCON0_IO_POLARITY);
    writel(0, TCON0_IO_TRI);
    writel((1U<<31), TCON_CTRL);
    delay(10);
}

static void fill_rgb565(unsigned short color)
{
    volatile unsigned short *fb = (volatile unsigned short*)FRAMEBUFFER_ADDR;
    unsigned int i;
    for (i=0; i < LCD_WIDTH * LCD_HEIGHT; i++)
        fb[i] = color;
}

void main(void)
{
    unsigned short colors[] = {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_YELLOW, COLOR_WHITE};
    unsigned int i = 0;

    gpio_init();
    ccu_init();    set_pin(0);
    de2_init();    set_pin(1);
    tcon0_init();  set_pin(2);
    set_pin(3);

    while (1) {
        fill_rgb565(colors[i % 5]);
        i++;
        delay(2000);
    }
}
