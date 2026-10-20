#include "platform.h"

#define SCTLR_DCACHE_ENABLE (1U << 2)

void platform_delay(unsigned int units)
{
    volatile unsigned int i;
    for (i = 0; i < units * 10000U; i++)
        ;
}

static uint32_t read_sctlr(void)
{
    uint32_t value;
    __asm__ volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (value));
    return value;
}

static uint32_t read_ctr(void)
{
    uint32_t value;
    __asm__ volatile ("mrc p15, 0, %0, c0, c0, 1" : "=r" (value));
    return value;
}

void platform_dcache_clean_range(uintptr_t start, size_t bytes)
{
    uint32_t ctr;
    uintptr_t address;
    uintptr_t end;
    uintptr_t line_size;

    if (!(read_sctlr() & SCTLR_DCACHE_ENABLE) || bytes == 0U)
        return;

    ctr = read_ctr();
    line_size = 4U << ((ctr >> 16) & 0xFU);
    address = start & ~(line_size - 1U);
    end = start + bytes;

    while (address < end) {
        __asm__ volatile ("mcr p15, 0, %0, c7, c10, 1"
                          : : "r" (address) : "memory");
        address += line_size;
    }
    platform_dsb();
}
