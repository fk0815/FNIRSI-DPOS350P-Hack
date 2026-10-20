#include "gt911.h"
#include "gpio.h"
#include "platform.h"

#define GT911_ADDR_WRITE 0xBAU
#define GT911_ADDR_READ  0xBBU
#define GT911_REG_STATUS 0x814EU
#define GT911_REG_POINT1 0x8150U

#define GT_INT_PORT GPIO_PORT_E
#define GT_INT_PIN  22U
#define GT_RST_PORT GPIO_PORT_E
#define GT_RST_PIN  24U
#define GT_SDA_PORT GPIO_PORT_G
#define GT_SDA_PIN  6U
#define GT_SCL_PORT GPIO_PORT_G
#define GT_SCL_PIN  7U

/* Fast delay dedicated to bit-banged GT911 I2C.
 * Do NOT use platform_delay() here: it is millisecond-scale on this target. */
static inline void i2c_delay(void)
{
    volatile unsigned int i;
    for (i = 0U; i < 32U; i++)
        __asm__ volatile ("nop");
}
static void scl(int high) { gpio_write(GT_SCL_PORT, GT_SCL_PIN, high); i2c_delay(); }
static void sda_output(void) { gpio_set_output(GT_SDA_PORT, GT_SDA_PIN); }
static void sda_input(void) { gpio_set_input(GT_SDA_PORT, GT_SDA_PIN); }
static void sda(int high) { gpio_write(GT_SDA_PORT, GT_SDA_PIN, high); i2c_delay(); }

static void i2c_start(void)
{
    sda_output(); sda(1); scl(1); sda(0); scl(0);
}

static void i2c_stop(void)
{
    sda_output(); sda(0); scl(1); sda(1);
}

static int i2c_write_byte(uint8_t value)
{
    unsigned int bit;
    sda_output();
    for (bit = 0; bit < 8U; bit++) {
        sda((value & 0x80U) != 0U);
        scl(1); scl(0);
        value <<= 1;
    }
    sda_input();
    scl(1);
    bit = (unsigned int)gpio_read(GT_SDA_PORT, GT_SDA_PIN);
    scl(0);
    sda_output();
    return bit == 0U ? 0 : -1;
}

static uint8_t i2c_read_byte(int acknowledge)
{
    unsigned int bit;
    uint8_t value = 0U;
    sda_input();
    for (bit = 0; bit < 8U; bit++) {
        value <<= 1;
        scl(1);
        if (gpio_read(GT_SDA_PORT, GT_SDA_PIN)) value |= 1U;
        scl(0);
    }
    sda_output();
    sda(acknowledge ? 0 : 1);
    scl(1); scl(0); sda(1);
    return value;
}

static int gt911_write_reg(uint16_t reg, const uint8_t *data, unsigned int length)
{
    unsigned int i;
    i2c_start();
    if (i2c_write_byte(GT911_ADDR_WRITE) ||
        i2c_write_byte((uint8_t)(reg >> 8)) ||
        i2c_write_byte((uint8_t)reg)) {
        i2c_stop(); return -1;
    }
    for (i = 0; i < length; i++) {
        if (i2c_write_byte(data[i])) { i2c_stop(); return -1; }
    }
    i2c_stop();
    return 0;
}

static int gt911_read_reg(uint16_t reg, uint8_t *data, unsigned int length)
{
    unsigned int i;
    i2c_start();
    if (i2c_write_byte(GT911_ADDR_WRITE) ||
        i2c_write_byte((uint8_t)(reg >> 8)) ||
        i2c_write_byte((uint8_t)reg)) {
        i2c_stop(); return -1;
    }
    i2c_start();
    if (i2c_write_byte(GT911_ADDR_READ)) { i2c_stop(); return -1; }
    for (i = 0; i < length; i++)
        data[i] = i2c_read_byte(i + 1U < length);
    i2c_stop();
    return 0;
}

int gt911_init(void)
{
    gpio_set_output(GT_SDA_PORT, GT_SDA_PIN);
    gpio_set_output(GT_SCL_PORT, GT_SCL_PIN);
    gpio_write(GT_SDA_PORT, GT_SDA_PIN, 1);
    gpio_write(GT_SCL_PORT, GT_SCL_PIN, 1);

    gpio_set_output(GT_INT_PORT, GT_INT_PIN);
    gpio_set_output(GT_RST_PORT, GT_RST_PIN);
    gpio_write(GT_INT_PORT, GT_INT_PIN, 0);
    gpio_write(GT_RST_PORT, GT_RST_PIN, 0);
    platform_delay(40U);
    gpio_write(GT_INT_PORT, GT_INT_PIN, 1);
    platform_delay(40U);
    gpio_write(GT_RST_PORT, GT_RST_PIN, 1);
    platform_delay(40U);
    gpio_set_input(GT_INT_PORT, GT_INT_PIN);
    return 0;
}

int gt911_read(gt911_touch_t *touch)
{
    uint8_t status;
    uint8_t point[4];
    uint8_t clear = 0U;

    if (!touch) return -1;
    if (gt911_read_reg(GT911_REG_STATUS, &status, 1U)) return -1;

    touch->points = status & 0x0FU;
    touch->pressed = 0U;
    if ((status & 0x80U) && touch->points != 0U) {
        if (gt911_read_reg(GT911_REG_POINT1, point, 4U)) return -1;
        touch->x = (uint16_t)point[0] | ((uint16_t)point[1] << 8);
        touch->y = (uint16_t)point[2] | ((uint16_t)point[3] << 8);
        if (touch->x >= 1024U) touch->x = 1023U;
        if (touch->y >= 600U) touch->y = 599U;
        touch->pressed = 1U;
    }
    return gt911_write_reg(GT911_REG_STATUS, &clear, 1U);
}
