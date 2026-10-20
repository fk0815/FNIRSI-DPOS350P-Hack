#include "display.h"
#include "gpio.h"
#include "platform.h"

#define CCU_BASE              0x01C20000U
#define CCU_PLL_VIDEO_CTRL    (CCU_BASE + 0x0010U)
#define CCU_BUS_CLK_GATE0     (CCU_BASE + 0x0060U)
#define CCU_BUS_CLK_GATE1     (CCU_BASE + 0x0064U)
#define CCU_DE_CLK            (CCU_BASE + 0x0104U)
#define CCU_TCON0_CLK         (CCU_BASE + 0x0118U)
#define CCU_BUS_SOFT_RST1     (CCU_BASE + 0x02C4U)

#define TCON_BASE             0x01C0C000U
#define TCON_CTRL             (TCON_BASE + 0x000U)
#define TCON_INT0             (TCON_BASE + 0x004U)
#define TCON0_CTRL            (TCON_BASE + 0x040U)
#define TCON0_DCLK            (TCON_BASE + 0x044U)
#define TCON0_TIMING_ACTIVE   (TCON_BASE + 0x048U)
#define TCON0_TIMING_H        (TCON_BASE + 0x04CU)
#define TCON0_TIMING_V        (TCON_BASE + 0x050U)
#define TCON0_TIMING_SYNC     (TCON_BASE + 0x054U)
#define TCON0_IO_POLARITY     (TCON_BASE + 0x088U)
#define TCON0_IO_TRI          (TCON_BASE + 0x08CU)

#define DE2_BASE              0x01000000U
#define DE2_CCU_GATE          (DE2_BASE + 0x00U)
#define DE2_CCU_BUS_GATE      (DE2_BASE + 0x04U)
#define DE2_CCU_RST           (DE2_BASE + 0x08U)
#define DE2_CCU_SEL           (DE2_BASE + 0x10U)
#define MIX_BASE              0x01100000U
#define GLB_CTL               (MIX_BASE + 0x0000U)
#define GLB_DBUFF             (MIX_BASE + 0x0008U)
#define GLB_SIZE              (MIX_BASE + 0x000CU)
#define BLD_BASE              (MIX_BASE + 0x1000U)
#define MIX_CH0_BASE          (MIX_BASE + 0x2000U)
#define MIX_CH1_BASE          (MIX_BASE + 0x3000U)
#define MIX_CH2_BASE          (MIX_BASE + 0x4000U)
#define MIX_CH3_BASE          (MIX_BASE + 0x5000U)

#define FB0_ADDR              0x43000000U
#define FB1_ADDR              0x43200000U
#define FB_STRIDE             (DISPLAY_WIDTH * 2U)
#define FB_SIZE               (FB_STRIDE * DISPLAY_HEIGHT)

#define H_BP 160U
#define H_FP 163U
#define H_SYNC 1U
#define H_TOTAL (DISPLAY_WIDTH + H_BP + H_FP + H_SYNC)
#define V_BP 12U
#define V_FP 23U
#define V_SYNC 1U
#define V_TOTAL (DISPLAY_HEIGHT + V_BP + V_FP + V_SYNC)

static uintptr_t front_address = FB0_ADDR;
static uintptr_t back_address = FB1_ADDR;

static void clear_words(uintptr_t address, unsigned int bytes)
{
    volatile uint32_t *p = (volatile uint32_t *)address;
    unsigned int i;
    for (i = 0; i < bytes / 4U; i++)
        p[i] = 0U;
}

static void display_gpio_init(void)
{
    unsigned int pin;
    uint32_t value = mmio_read32(CCU_BUS_CLK_GATE0);
    mmio_write32(CCU_BUS_CLK_GATE0, value & ~(1U << 20));

    for (pin = 1U; pin <= 21U; pin++) {
        if (pin != 12U)
            gpio_set_function(GPIO_PORT_D, pin, 2U);
    }
}

static void display_clock_init(void)
{
    uint32_t value;
    mmio_write32(CCU_PLL_VIDEO_CTRL,
                 (1U << 31) | (1U << 24) | (80U << 8) | 5U);
    while (!(mmio_read32(CCU_PLL_VIDEO_CTRL) & (1U << 28)))
        ;
    mmio_write32(CCU_DE_CLK, 1U << 31);
    mmio_write32(CCU_TCON0_CLK, 1U << 31);
    value = mmio_read32(CCU_BUS_SOFT_RST1);
    mmio_write32(CCU_BUS_SOFT_RST1, value | (1U << 12) | (1U << 4));
    platform_delay(1U);
    value = mmio_read32(CCU_BUS_CLK_GATE1);
    mmio_write32(CCU_BUS_CLK_GATE1, value | (1U << 12) | (1U << 4));
}

static void de2_init(void)
{
    const uint32_t active = ((DISPLAY_HEIGHT - 1U) << 16) |
                            (DISPLAY_WIDTH - 1U);

    mmio_write32(DE2_CCU_RST, mmio_read32(DE2_CCU_RST) & ~1U);
    platform_delay(1U);
    mmio_write32(DE2_CCU_RST, mmio_read32(DE2_CCU_RST) | 1U);
    mmio_write32(DE2_CCU_BUS_GATE, mmio_read32(DE2_CCU_BUS_GATE) | 1U);
    mmio_write32(DE2_CCU_GATE, mmio_read32(DE2_CCU_GATE) | 1U);
    mmio_write32(DE2_CCU_SEL, mmio_read32(DE2_CCU_SEL) & ~1U);

    clear_words(MIX_CH0_BASE, 0x8CU);
    clear_words(MIX_CH1_BASE, 0x8CU);
    clear_words(MIX_CH2_BASE, 0x8CU);
    clear_words(MIX_CH3_BASE, 0x8CU);

    mmio_write32(GLB_CTL, 1U);
    mmio_write32(GLB_DBUFF, 0U);
    mmio_write32(MIX_BASE + 0x0004U, 0U);
    mmio_write32(GLB_SIZE, active);

    clear_words(BLD_BASE, 0x100U);
    mmio_write32(BLD_BASE + 0x80U, 2U);
    mmio_write32(BLD_BASE + 0x00U, 1U << 8);
    mmio_write32(BLD_BASE + 0x8CU, active);
    mmio_write32(BLD_BASE + 0x90U, 0x03010301U);
    mmio_write32(BLD_BASE + 0x94U, 0x03010301U);
    mmio_write32(BLD_BASE + 0x04U, 0xFF000000U);
    mmio_write32(BLD_BASE + 0x08U, active);

    mmio_write32(MIX_CH2_BASE + 0x00U, 0x00000A01U);
    mmio_write32(MIX_CH2_BASE + 0x04U, active);
    mmio_write32(MIX_CH2_BASE + 0x08U, 0U);
    mmio_write32(MIX_CH2_BASE + 0x0CU, FB_STRIDE);
    mmio_write32(MIX_CH2_BASE + 0x10U, (uint32_t)front_address);
    mmio_write32(MIX_CH2_BASE + 0x88U, active);
    mmio_write32(GLB_DBUFF, 1U);
}

static void tcon_init(void)
{
    uint32_t start_delay = ((V_FP + V_BP + V_SYNC) & 0x1FU) << 4;
    mmio_write32(TCON_CTRL, 0U);
    mmio_write32(TCON_INT0, 0U);
    mmio_write32(TCON0_CTRL, 0U);
    mmio_write32(TCON0_DCLK, (0xFU << 28) | 8U);
    mmio_write32(TCON0_TIMING_ACTIVE,
                 ((DISPLAY_HEIGHT - 1U) << 16) | (DISPLAY_WIDTH - 1U));
    mmio_write32(TCON0_TIMING_H,
                 ((H_TOTAL - 1U) << 16) | (H_FP + H_SYNC - 1U));
    mmio_write32(TCON0_TIMING_V,
                 (V_TOTAL << 17) | (V_FP + V_SYNC - 1U));
    mmio_write32(TCON0_TIMING_SYNC,
                 ((V_SYNC - 1U) << 16) | (H_SYNC - 1U));
    mmio_write32(TCON0_IO_POLARITY, 0x17000000U);
    mmio_write32(TCON0_IO_TRI, 0U);
    mmio_write32(TCON0_CTRL, (1U << 31) | start_delay);
    mmio_write32(TCON_CTRL, 1U << 31);
}

int display_init(void)
{
    display_gpio_init();
    display_clock_init();
    display_fill_back(COLOR_BLACK);
    platform_dcache_clean_range(back_address, FB_SIZE);
    front_address = back_address;
    back_address = FB0_ADDR;
    de2_init();
    tcon_init();
    return 0;
}

uint16_t *display_backbuffer(void)
{
    return (uint16_t *)back_address;
}

void display_fill_back(uint16_t color)
{
    volatile uint16_t *fb = (volatile uint16_t *)back_address;
    unsigned int i;
    for (i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++)
        fb[i] = color;
}

void display_swap(void)
{
    uintptr_t old_front;
    platform_dcache_clean_range(back_address, FB_SIZE);
    platform_dsb();
    mmio_write32(MIX_CH2_BASE + 0x10U, (uint32_t)back_address);
    mmio_write32(GLB_DBUFF, 1U);
    platform_dsb();
    old_front = front_address;
    front_address = back_address;
    back_address = old_front;
}
