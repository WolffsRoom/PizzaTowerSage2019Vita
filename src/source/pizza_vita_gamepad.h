#ifndef PIZZA_VITA_GAMEPAD_H
#define PIZZA_VITA_GAMEPAD_H

#include <stdbool.h>
#include <string.h>

#include "runner_gamepad.h"
#include "runner_keyboard.h"

typedef struct PizzaVitaGamepadInput {
    float leftX;
    float leftY;
    bool up;
    bool down;
    bool left;
    bool right;
    bool cross;
    bool circle;
    bool square;
    bool triangle;
    bool leftShoulder;
    bool rightShoulder;
    bool start;
    bool select;
} PizzaVitaGamepadInput;

static inline void PizzaVitaGamepad_setButton(GamepadSlot* slot, int index, bool down) {
    bool wasDown = slot->buttonDown[index];
    slot->buttonDownPrev[index] = wasDown;
    slot->buttonDown[index] = down;
    slot->buttonPressed[index] = down && !wasDown;
    slot->buttonReleased[index] = !down && wasDown;
    slot->buttonValue[index] = down ? 1.0f : 0.0f;
}

static inline void PizzaVitaGamepad_apply(RunnerGamepadState* gamepads,
                                           const PizzaVitaGamepadInput* input,
                                           bool consumeSelect) {
    GamepadSlot* slot = &gamepads->slots[0];
    slot->connectedPrev = slot->connected;
    slot->connected = true;
    slot->jid = 0;
    if (slot->description[0] == '\0') {
        strcpy(slot->description, "PS Vita");
        strcpy(slot->guid, "psvita");
        slot->deadzone = 0.25f;
        slot->triggerThreshold = 0.5f;
    }
    gamepads->connectedCount = 1;

    PizzaVitaGamepad_setButton(slot, 0, input->cross);
    PizzaVitaGamepad_setButton(slot, 1, input->circle);
    PizzaVitaGamepad_setButton(slot, 2, input->square);
    PizzaVitaGamepad_setButton(slot, 3, input->triangle);
    PizzaVitaGamepad_setButton(slot, 4, input->leftShoulder);
    PizzaVitaGamepad_setButton(slot, 5, input->rightShoulder);
    PizzaVitaGamepad_setButton(slot, 8, input->select && !consumeSelect);
    PizzaVitaGamepad_setButton(slot, 9, input->start);
    PizzaVitaGamepad_setButton(slot, 12, input->up);
    PizzaVitaGamepad_setButton(slot, 13, input->down);
    PizzaVitaGamepad_setButton(slot, 14, input->left);
    PizzaVitaGamepad_setButton(slot, 15, input->right);
    slot->axisValue[0] = input->leftX;
    slot->axisValue[1] = input->leftY;
    slot->axisValue[2] = 0.0f;
    slot->axisValue[3] = 0.0f;
}

static inline void PizzaVitaGamepad_applyExclusive(RunnerGamepadState* gamepads,
        RunnerKeyboardState* keyboard, const PizzaVitaGamepadInput* input,
        bool consumeSelect) {
    /* Physical Vita controls have only one device identity. Do not manufacture
       keyboard release edges either: those also switch the game's input mode. */
    memset(keyboard->keyDown, 0, sizeof(keyboard->keyDown));
    memset(keyboard->keyPressed, 0, sizeof(keyboard->keyPressed));
    memset(keyboard->keyReleased, 0, sizeof(keyboard->keyReleased));
    keyboard->lastKey = 0;
    keyboard->lastChar[0] = '\0';
    PizzaVitaGamepad_apply(gamepads, input, consumeSelect);
}

#endif
