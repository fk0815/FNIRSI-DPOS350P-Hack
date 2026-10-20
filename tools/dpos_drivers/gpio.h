#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include "platform.h"

void gpio_set_function(uint32_t port_offset, unsigned int pin,
                       unsigned int function);
void gpio_set_output(uint32_t port_offset, unsigned int pin);
void gpio_set_input(uint32_t port_offset, unsigned int pin);
void gpio_write(uint32_t port_offset, unsigned int pin, int high);
int gpio_read(uint32_t port_offset, unsigned int pin);

#endif
