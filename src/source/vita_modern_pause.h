#ifndef VITA_MODERN_PAUSE_H
#define VITA_MODERN_PAUSE_H

#include "renderer.h"
#include "pizza_native_pause.h"
#include "vita_settings.h"

void VitaModernPause_draw(const PizzaNativePause* pause,
                          const VitaSettings* settings,
                          Renderer* renderer);
void VitaModernPause_shutdown(void);

#endif

