#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#define DISPLAY_WIDTH  1024U
#define DISPLAY_HEIGHT 600U

#define COLOR_RED      0xF800U
#define COLOR_GREEN    0x07E0U
#define COLOR_BLUE     0x001FU
#define COLOR_WHITE    0xFFFFU
#define COLOR_YELLOW   0xFFE0U
#define COLOR_BLACK    0x0000U

int display_init(void);
uint16_t *display_backbuffer(void);
void display_fill_back(uint16_t color);
void display_swap(void);

#endif
