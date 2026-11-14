#ifndef PIZZA_VITA_PAUSE_POLICY_H
#define PIZZA_VITA_PAUSE_POLICY_H

#include <stdbool.h>

typedef struct PizzaVitaPauseState {
    bool physicalWasDown;
} PizzaVitaPauseState;

typedef struct PizzaVitaPauseSignal {
    bool physicalEdge;
    bool needsFallback;
    bool diverged;
} PizzaVitaPauseSignal;

static inline PizzaVitaPauseSignal PizzaVitaPausePolicy_update(
    PizzaVitaPauseState* state,
    bool physicalStart,
    bool gamepadPressed,
    bool bindingPressed,
    bool playerKeyStart
) {
    PizzaVitaPauseSignal signal = {0};
    signal.physicalEdge = physicalStart && !state->physicalWasDown;
    signal.needsFallback = signal.physicalEdge && !bindingPressed && !playerKeyStart;
    signal.diverged = signal.physicalEdge &&
        (!gamepadPressed || (!bindingPressed && !playerKeyStart));
    state->physicalWasDown = physicalStart;
    return signal;
}

#endif
