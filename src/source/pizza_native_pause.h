#ifndef PIZZA_NATIVE_PAUSE_H
#define PIZZA_NATIVE_PAUSE_H
#include <stdbool.h>
#include <stdint.h>

enum {
    PIZZA_PAUSE_ITEM_RESUME = 0,
    PIZZA_PAUSE_ITEM_MUSIC = 1,
    PIZZA_PAUSE_ITEM_SFX = 2,
    PIZZA_PAUSE_ITEM_LANG = 3,
    PIZZA_PAUSE_ITEM_CRT = 4,
    PIZZA_PAUSE_ITEM_TOUCH = 5,
    PIZZA_PAUSE_ITEM_VSYNC = 6,
    PIZZA_PAUSE_ITEM_EXIT = 7,
    PIZZA_PAUSE_ITEM_COUNT = 8
};

typedef struct {
    bool open, confirmExit;
    unsigned selected;
    uint32_t previousButtons;
    uint32_t tick;
    int32_t knobAnimFrames;
} PizzaNativePause;

enum {
    PIZZA_PAUSE_START = 1,
    PIZZA_PAUSE_UP = 2,
    PIZZA_PAUSE_DOWN = 4,
    PIZZA_PAUSE_CONFIRM = 8,
    PIZZA_PAUSE_CANCEL = 16,
    PIZZA_PAUSE_LEFT = 32,
    PIZZA_PAUSE_RIGHT = 64
};

/* Returns true only for a separately confirmed exit. Held START cannot reopen. */
static inline bool PizzaNativePause_update(PizzaNativePause* s, uint32_t buttons,
                                           bool canOpen) {
    uint32_t pressed = buttons & ~s->previousButtons;
    s->previousButtons = buttons;
    if (s->open) {
        s->tick++;
        if (s->knobAnimFrames > 0) s->knobAnimFrames--;
    }
    if (pressed & PIZZA_PAUSE_START) {
        if (s->open || canOpen) {
            s->open = !s->open;
            s->confirmExit = false;
            s->selected = 0;
            s->knobAnimFrames = 0;
        }
        return false;
    }
    if (!s->open) return false;
    if (s->confirmExit) {
        if (pressed & PIZZA_PAUSE_CANCEL) {
            s->confirmExit = false;
            return false;
        }
        return (pressed & PIZZA_PAUSE_CONFIRM) != 0;
    }
    if (pressed & PIZZA_PAUSE_CANCEL) {
        s->confirmExit = false;
        s->open = false;
        return false;
    }
    if (pressed & PIZZA_PAUSE_UP) s->selected = (s->selected + PIZZA_PAUSE_ITEM_COUNT - 1U) % PIZZA_PAUSE_ITEM_COUNT;
    if (pressed & PIZZA_PAUSE_DOWN) s->selected = (s->selected + 1U) % PIZZA_PAUSE_ITEM_COUNT;
    if ((pressed & (PIZZA_PAUSE_LEFT | PIZZA_PAUSE_RIGHT)) &&
        (s->selected == PIZZA_PAUSE_ITEM_MUSIC || s->selected == PIZZA_PAUSE_ITEM_SFX)) {
        s->knobAnimFrames = 14;
    }
    if (pressed & PIZZA_PAUSE_CONFIRM) {
        if (s->selected == PIZZA_PAUSE_ITEM_RESUME) s->open = false;
        else if (s->selected == PIZZA_PAUSE_ITEM_EXIT) s->confirmExit = true;
    }
    return false;
}
#endif
