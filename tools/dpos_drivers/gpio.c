#include "gpio.h"

static uintptr_t gpio_cfg_address(uint32_t port_offset, unsigned int pin)
{
    return PIO_BASE + port_offset + (pin / 8U) * 4U;
}

static uintptr_t gpio_data_address(uint32_t port_offset)
{
    return PIO_BASE + port_offset + 0x10U;
}

void gpio_set_function(uint32_t port_offset, unsigned int pin,
                       unsigned int function)
{
    uintptr_t address = gpio_cfg_address(port_offset, pin);
    unsigned int shift = (pin % 8U) * 4U;
    uint32_t value = mmio_read32(address);

    value &= ~(0x7U << shift);
    value |= (function & 0x7U) << shift;
    mmio_write32(address, value);
}

void gpio_set_output(uint32_t port_offset, unsigned int pin)
{
    gpio_set_function(port_offset, pin, 1U);
}

void gpio_set_input(uint32_t port_offset, unsigned int pin)
{
    gpio_set_function(port_offset, pin, 0U);
}

void gpio_write(uint32_t port_offset, unsigned int pin, int high)
{
    uintptr_t address = gpio_data_address(port_offset);
    uint32_t value = mmio_read32(address);

    if (high)
        value |= 1U << pin;
    else
        value &= ~(1U << pin);
    mmio_write32(address, value);
}

int gpio_read(uint32_t port_offset, unsigned int pin)
{
    return (int)((mmio_read32(gpio_data_address(port_offset)) >> pin) & 1U);
}
