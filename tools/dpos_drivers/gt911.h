#ifndef GT911_H
#define GT911_H

#include <stdint.h>

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t pressed;
    uint8_t points;
} gt911_touch_t;

int gt911_init(void);
int gt911_read(gt911_touch_t *touch);

#endif
