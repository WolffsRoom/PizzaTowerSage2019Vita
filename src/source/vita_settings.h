#ifndef pizzatower_VITA_SETTINGS_H
#define pizzatower_VITA_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>
#include <psp2/ctrl.h>

#include "audio_system.h"
#include "renderer.h"
#include "pizza_native_pause.h"

typedef struct {
    bool open;
    bool touchEnabled;
    bool ptbrEnabled;
    bool widescreenEnabled;
    bool adjustMode;
    bool restartOnClose;
    bool returnToChapterSelect;
    bool confirmChapterSelect;
    int musicVolume;
    int sfxVolume;
    bool vsyncEnabled;
    int screenFilterMode;
    bool debugDevEnabled;
    bool debugDevChanged;
    int graphicsQuality;
    int displayOffsetX;
    int displayOffsetY;
    int displayZoom;
    float visualStickX;
    float visualStickY;
    bool visualConfirm;
    bool visualCancel;
    bool visualMenu;
    int category;
    int selected;
    int inputCooldown;
    int borderWarningFrames;
    uint32_t previousButtons;
    int qualityNoticeFrames;
} VitaSettings;

extern int g_vitaGraphicsQuality;

void VitaSettings_load(VitaSettings* settings);
void VitaSettings_save(const VitaSettings* settings);
void VitaSettings_applyAudio(VitaSettings* settings, AudioSystem* audio);
void VitaSettings_toggleScreenFilter(VitaSettings* settings);
void VitaSettings_toggleTouch(VitaSettings* settings);
void VitaSettings_toggleVsync(VitaSettings* settings);
bool VitaSettings_handleInput(VitaSettings* settings, const SceCtrlData* pad, AudioSystem* audio);
void VitaSettings_draw(VitaSettings* settings, Renderer* renderer);
void VitaSettings_drawTouchControls(VitaSettings* settings, Renderer* renderer);
void VitaSettings_drawCalibration(VitaSettings* settings, Renderer* renderer);
void VitaSettings_drawDevOverlay(VitaSettings* settings, Renderer* renderer,
                                 const char* room, float fps, uint64_t stepUs,
                                 uint64_t audioUs, uint64_t renderUs,
                                 uint64_t gpuBytes, uint32_t evictions,
                                 uint32_t deferred, uint32_t ramHits);
void VitaSettings_setTouchVisuals(VitaSettings* settings, float stickX, float stickY,
                                  bool confirm, bool cancel, bool menu);
void VitaSettings_setLauncherMode(bool launcherMode);
void VitaSettings_setGraphicsQuality(VitaSettings* settings, int quality);
void VitaSettings_drawQualityNotice(VitaSettings* settings, Renderer* renderer);
void VitaSettings_drawNativePause(const PizzaNativePause* pause, const VitaSettings* settings, Renderer* renderer);
void VitaSettings_adjustPauseVolume(VitaSettings* settings, AudioSystem* audio, unsigned item, int delta);

#endif
