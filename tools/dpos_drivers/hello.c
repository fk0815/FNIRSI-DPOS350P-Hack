#include "display.h"
#include "gt911.h"

void main(void)
{
    static const uint16_t colors[] = {
        COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_YELLOW, COLOR_WHITE
    };
    gt911_touch_t touch;
    unsigned int color = 0U;
    unsigned int was_pressed = 0U;

    display_init();
    display_fill_back(colors[color]);
    display_swap();
    gt911_init();

    for (;;) {
        if (gt911_read(&touch) == 0) {
            if (touch.pressed && !was_pressed) {
                color = (color + 1U) % 5U;
                display_fill_back(colors[color]);
                display_swap();
            }
            was_pressed = touch.pressed;
        }
    }
}
