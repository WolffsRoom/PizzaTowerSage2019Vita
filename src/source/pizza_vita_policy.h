#ifndef PIZZA_VITA_POLICY_H
#define PIZZA_VITA_POLICY_H

#include <stdint.h>
#include <stdbool.h>

#define PIZZA_VITA_CPU_CLOCK_MHZ 444
#define PIZZA_VITA_BUS_CLOCK_MHZ 222
#define PIZZA_VITA_GPU_CLOCK_MHZ 222
#define PIZZA_VITA_XBAR_CLOCK_MHZ 166

typedef enum PizzaVitaButton {
    PIZZA_VITA_BUTTON_CROSS,
    PIZZA_VITA_BUTTON_CIRCLE,
    PIZZA_VITA_BUTTON_SQUARE,
    PIZZA_VITA_BUTTON_TRIANGLE,
    PIZZA_VITA_BUTTON_START,
    PIZZA_VITA_BUTTON_SELECT
} PizzaVitaButton;

typedef struct PizzaVitaQualityChordState {
    uint8_t pendingFrames;
    bool selectWasDown;
    bool selectDelivered;
    bool leftWasDown;
    bool rightWasDown;
    bool chordConsumed;
} PizzaVitaQualityChordState;

typedef struct PizzaVitaQualityChordResult {
    int delta;
    bool deliverSelect;
    bool consumeSelect;
} PizzaVitaQualityChordResult;

static inline PizzaVitaQualityChordResult PizzaVitaPolicy_updateQualityChord(
    PizzaVitaQualityChordState* state,
    bool selectDown,
    bool leftShoulderDown,
    bool rightShoulderDown
) {
    PizzaVitaQualityChordResult result = {0, false, false};
    bool selectPressed = selectDown && !state->selectWasDown;
    bool leftPressed = leftShoulderDown && !state->leftWasDown;
    bool rightPressed = rightShoulderDown && !state->rightWasDown;

    if (selectPressed) {
        state->pendingFrames = 2;
        state->selectDelivered = false;
        state->chordConsumed = false;
    }

    if (selectDown && !state->selectDelivered && !state->chordConsumed &&
        (leftPressed || rightPressed)) {
        result.delta = rightPressed ? 1 : -1;
        result.consumeSelect = true;
        state->pendingFrames = 0;
        state->chordConsumed = true;
    } else if (!selectPressed && selectDown && !state->selectDelivered && !state->chordConsumed &&
               state->pendingFrames > 0) {
        state->pendingFrames--;
        result.consumeSelect = true;
        if (state->pendingFrames == 0) {
            result.deliverSelect = true;
            state->selectDelivered = true;
            result.consumeSelect = false;
        }
    }

    if (!selectDown) {
        state->pendingFrames = 0;
        state->selectDelivered = false;
        state->chordConsumed = false;
    }

    state->selectWasDown = selectDown;
    state->leftWasDown = leftShoulderDown;
    state->rightWasDown = rightShoulderDown;
    return result;
}

static inline uint32_t PizzaVitaPolicy_targetFrameMicros(void) {
    return 1000000U / 60U;
}

static inline uint64_t PizzaVitaPolicy_deliveredFrameMicros(uint64_t workMicros,
                                                             uint64_t waitMicros) {
    return workMicros + waitMicros;
}

static inline double PizzaVitaPolicy_fps(uint64_t workMicros,
                                         uint64_t waitMicros) {
    uint64_t delivered = PizzaVitaPolicy_deliveredFrameMicros(workMicros, waitMicros);
    return delivered > 0 ? 1000000.0 / (double)delivered : 0.0;
}

static inline int PizzaVitaPolicy_keyForButton(PizzaVitaButton button) {
    switch (button) {
        case PIZZA_VITA_BUTTON_CROSS: return 'Z';
        case PIZZA_VITA_BUTTON_CIRCLE:
        case PIZZA_VITA_BUTTON_SQUARE: return 'X';
        case PIZZA_VITA_BUTTON_TRIANGLE: return 'C';
        case PIZZA_VITA_BUTTON_START: return 0x1B;
        case PIZZA_VITA_BUTTON_SELECT: return 0x0D;
        default: return 0;
    }
}

#endif
