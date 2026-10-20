#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stddef.h>

#define PIO_BASE 0x01C20800U

#define GPIO_PORT_B 0x24U
#define GPIO_PORT_C 0x48U
#define GPIO_PORT_D 0x6CU
#define GPIO_PORT_E 0x90U
#define GPIO_PORT_G 0xD8U

static inline void mmio_write32(uintptr_t address, uint32_t value)
{
    *(volatile uint32_t *)address = value;
}

static inline uint32_t mmio_read32(uintptr_t address)
{
    return *(volatile uint32_t *)address;
}

static inline void platform_dsb(void)
{
    __asm__ volatile ("dsb sy" ::: "memory");
}

static inline void platform_isb(void)
{
    __asm__ volatile ("isb sy" ::: "memory");
}

void platform_delay(unsigned int units);
void platform_dcache_clean_range(uintptr_t start, size_t bytes);

#endif
