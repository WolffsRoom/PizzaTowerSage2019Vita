#include "vita_settings.h"
#include "vita_modern_pause.h"
#include "vita_gl_quad.h"

#include <psp2/io/fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef PIZZA_NATIVE_GXM
#include "pizza_gxm_platform.h"
typedef unsigned GLuint;
typedef float GLfloat;
#else
#include <vitaGL.h>
#endif

#include "stb_image.h"

#include "audio/openal/al_audio_system.h"
#include "runner.h"
#include "vita_borders.h"

#ifndef SETTINGS_PATH
#define SETTINGS_PATH "ux0:data/pizzatowersage/config.ini"
#endif
#define SETTINGS_CATEGORIES 4

static int settingsItemCount(int category) {
  if (category == 0)
    return 1;
  if (category == 1)
    return 4;
  if (category == 3)
    return 4;
  return 2;
}

int g_vitaDisplayOffsetX = 0;
int g_vitaDisplayOffsetY = 0;
int g_vitaDisplayZoom = 100;
int g_vitaPortOverlayFullScreen = 0;
int g_vitaTouchEnabled = 0;
int g_vitaConsoleBordersEnabled = 0;
int g_vitaGraphicsQuality = 2;
static bool g_launcherMode = false;

static void resetDefaults(VitaSettings *s) {
  s->touchEnabled = false;
  s->ptbrEnabled = false;
  s->widescreenEnabled = false;
  s->musicVolume = 10;
  s->sfxVolume = 10;
  s->vsyncEnabled = true;
  s->screenFilterMode = 0;
  s->displayOffsetX = 0;
  s->displayOffsetY = 0;
  s->displayZoom = 100;
  s->graphicsQuality = 2;
}

static void applyDisplaySettings(const VitaSettings *s) {
  g_vitaDisplayOffsetX = g_launcherMode ? 0 : s->displayOffsetX;
  g_vitaDisplayOffsetY = g_launcherMode ? 0 : s->displayOffsetY;
  g_vitaDisplayZoom = g_launcherMode ? 100 : s->displayZoom;
  g_vitaTouchEnabled = s->touchEnabled ? 1 : 0;
  g_vitaConsoleBordersEnabled =
      (!g_launcherMode && s->widescreenEnabled) ? 1 : 0;
  g_vitaGraphicsQuality = s->graphicsQuality;
}

static void saveSettings(const VitaSettings *s) {
  char text[256];
  int length = snprintf(
      text, sizeof(text),
      "touch=%d\nmod=%s\nwidescreen=%d\nmusic_volume=%d\nsfx_volume=%d\nvsync=%"
      "d\ngraphics=%d\nscreen_profile=3\noffset_x=%d\noffset_y=%d\nzoom=%d\nscreen_filter=%d\n",
      s->touchEnabled ? 1 : 0, s->ptbrEnabled ? "PTBR" : "Original",
      s->widescreenEnabled ? 1 : 0, s->musicVolume, s->sfxVolume,
      s->vsyncEnabled ? 1 : 0, s->graphicsQuality, s->displayOffsetX,
      s->displayOffsetY, s->displayZoom, s->screenFilterMode);
  SceUID fd =
      sceIoOpen(SETTINGS_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
  if (fd >= 0) {
    sceIoWrite(fd, text, length);
    sceIoClose(fd);
  }
}

void VitaSettings_load(VitaSettings *s) {
  memset(s, 0, sizeof(*s));
  resetDefaults(s);
  bool migrateScreenProfile = false;
  char text[256] = {0};
  SceUID fd = sceIoOpen(SETTINGS_PATH, SCE_O_RDONLY, 0);
  if (fd >= 0) {
    int read = sceIoRead(fd, text, sizeof(text) - 1);
    sceIoClose(fd);
    if (read > 0)
      text[read] = '\0';
    char *touchSetting = strstr(text, "touch=");
    s->touchEnabled =
        touchSetting != NULL && strstr(touchSetting, "touch=1") == touchSetting;
    s->ptbrEnabled = strstr(text, "mod=PTBR") != NULL;
    s->widescreenEnabled = strstr(text, "widescreen=1") != NULL;
    s->vsyncEnabled = strstr(text, "vsync=0") == NULL;
    char *screenFilter = strstr(text, "screen_filter=");
    if (screenFilter != NULL) sscanf(screenFilter + 14, "%d", &s->screenFilterMode);
    char *musicVolume = strstr(text, "music_volume=");
    char *sfxVolume = strstr(text, "sfx_volume=");
    if (musicVolume != NULL)
      sscanf(musicVolume + 13, "%d", &s->musicVolume);
    if (sfxVolume != NULL)
      sscanf(sfxVolume + 11, "%d", &s->sfxVolume);
    char *offsetX = strstr(text, "offset_x=");
    char *offsetY = strstr(text, "offset_y=");
    char *zoom = strstr(text, "zoom=");
    char *graphics = strstr(text, "graphics=");
    if (offsetX != NULL)
      sscanf(offsetX + 9, "%d", &s->displayOffsetX);
    if (offsetY != NULL)
      sscanf(offsetY + 9, "%d", &s->displayOffsetY);
    if (zoom != NULL)
      sscanf(zoom + 5, "%d", &s->displayZoom);
    if (graphics != NULL)
      sscanf(graphics + 9, "%d", &s->graphicsQuality);
    if (strstr(text, "screen_profile=3") == NULL) {
      s->displayOffsetX = 0;
      s->displayOffsetY = 0;
      s->displayZoom = 100;
      s->widescreenEnabled = false;
      migrateScreenProfile = true;
    }
  }
  if (s->musicVolume < 0)
    s->musicVolume = 0;
  if (s->musicVolume > 10)
    s->musicVolume = 10;
  if (s->sfxVolume < 0)
    s->sfxVolume = 0;
  if (s->sfxVolume > 10)
    s->sfxVolume = 10;
  if (s->displayZoom < 50)
    s->displayZoom = 50;
  if (s->displayZoom > 160)
    s->displayZoom = 160;
  s->graphicsQuality = 2; // v0.40 always preserves native atlas resolution.
  applyDisplaySettings(s);
  if (migrateScreenProfile)
    saveSettings(s);
}

void VitaSettings_save(const VitaSettings *s) {
  saveSettings(s);
}

void VitaSettings_toggleScreenFilter(VitaSettings *s) {
  s->screenFilterMode = s->screenFilterMode ? 0 : 1;
  saveSettings(s);
}

void VitaSettings_toggleTouch(VitaSettings *s) {
  s->touchEnabled = !s->touchEnabled;
  applyDisplaySettings(s);
  saveSettings(s);
}

void VitaSettings_toggleVsync(VitaSettings *s) {
  s->vsyncEnabled = !s->vsyncEnabled;
  saveSettings(s);
}

void VitaSettings_adjustPauseVolume(VitaSettings* s, AudioSystem* audio, unsigned item, int delta) {
  int* value = item==1 ? &s->musicVolume : item==2 ? &s->sfxVolume : NULL;
  if (!value) return;
  *value += delta;
  if (*value<0) *value=0;
  if (*value>10) *value=10;
  VitaSettings_applyAudio(s,audio);
  saveSettings(s);
}

void VitaSettings_drawNativePause(const PizzaNativePause* p, const VitaSettings* s, Renderer* r) {
  VitaModernPause_draw(p, s, r);
}

void VitaSettings_applyAudio(VitaSettings *s, AudioSystem *audio) {
  audio->vtable->setMasterGain(audio, 1.0f);
  AlAudioSystem_setCategoryGains((AlAudioSystem *)audio,
                                 (float)s->musicVolume / 10.0f,
                                 (float)s->sfxVolume / 10.0f);
}

void VitaSettings_setGraphicsQuality(VitaSettings *s, int quality) {
  if (quality < 0) quality = 0;
  if (quality > 2) quality = 2;
  s->graphicsQuality = quality;
  s->qualityNoticeFrames = 150;
  applyDisplaySettings(s);
  saveSettings(s);
}

bool VitaSettings_handleInput(VitaSettings *s, const SceCtrlData *pad,
                              AudioSystem *audio) {
  if (s->inputCooldown > 0)
    s->inputCooldown--;
  uint32_t pressed = pad->buttons & ~s->previousButtons;
  s->previousButtons = pad->buttons;
  bool requestRestart = false;

  if (s->confirmChapterSelect) {
    if (pressed & SCE_CTRL_CROSS) {
      s->confirmChapterSelect = false;
      s->returnToChapterSelect = true;
      s->open = false;
      requestRestart = true;
    } else if (pressed & SCE_CTRL_CIRCLE) {
      s->confirmChapterSelect = false;
      s->inputCooldown = 2;
    }
    return requestRestart;
  }

  if (s->adjustMode) {
    int lx = (int)pad->lx - 128;
    int ly = (int)pad->ly - 128;
    int ry = (int)pad->ry - 128;
    if (lx < -48)
      s->displayOffsetX -= 2;
    if (lx > 48)
      s->displayOffsetX += 2;
    if (ly < -48)
      s->displayOffsetY -= 2;
    if (ly > 48)
      s->displayOffsetY += 2;
    if (ry < -48 && s->displayZoom < 160)
      s->displayZoom++;
    if (ry > 48 && s->displayZoom > 50)
      s->displayZoom--;
    applyDisplaySettings(s);
    if (pressed & SCE_CTRL_CIRCLE) {
      s->displayOffsetX = 0;
      s->displayOffsetY = 0;
      s->displayZoom = 100;
      applyDisplaySettings(s);
    }
    if (pressed & (SCE_CTRL_CROSS | SCE_CTRL_SELECT)) {
      s->adjustMode = false;
      saveSettings(s);
    }
    return false;
  }

  if (pressed & SCE_CTRL_SELECT) {
    if (!s->open && s->debugDevEnabled) {
      s->debugDevEnabled = false;
      s->debugDevChanged = true;
    }
    if (s->open && s->restartOnClose)
      requestRestart = true;
    s->open = !s->open;
  }
  if (!s->open)
    return requestRestart;

  if (pressed & SCE_CTRL_LTRIGGER) {
    s->category = (s->category + SETTINGS_CATEGORIES - 1) % SETTINGS_CATEGORIES;
    s->selected = 0;
  }
  if (pressed & SCE_CTRL_RTRIGGER) {
    s->category = (s->category + 1) % SETTINGS_CATEGORIES;
    s->selected = 0;
  }
  int itemCount = settingsItemCount(s->category);
  if (pressed & SCE_CTRL_UP)
    s->selected = (s->selected + itemCount - 1) % itemCount;
  if (pressed & SCE_CTRL_DOWN)
    s->selected = (s->selected + 1) % itemCount;

  bool activate =
      (pressed & (SCE_CTRL_CROSS | SCE_CTRL_LEFT | SCE_CTRL_RIGHT)) != 0;
  if (activate) {
    if (s->category == 0 && s->selected == 0)
      s->touchEnabled = !s->touchEnabled;
    else if (s->category == 1 && s->selected == 0) {
      s->widescreenEnabled = !s->widescreenEnabled;
      if (s->widescreenEnabled && !VitaBorders_filesAvailable())
        s->borderWarningFrames = 240;
      s->displayOffsetX = 0;
      s->displayOffsetY = 0;
      s->displayZoom = s->widescreenEnabled ? 90 : 100;
      applyDisplaySettings(s);
    } else if (s->category == 1 && s->selected == 1) {
      s->adjustMode = true;
      s->open = false;
    } else if (s->category == 1 && s->selected == 2) {
      if (pressed & SCE_CTRL_LEFT)
        s->graphicsQuality--;
      else
        s->graphicsQuality++;
      if (s->graphicsQuality < 0)
        s->graphicsQuality = 2;
      if (s->graphicsQuality > 2)
        s->graphicsQuality = 0;
      applyDisplaySettings(s);
      s->open = false;
      requestRestart = true;
    } else if (s->category == 1 && s->selected == 3) {
      s->debugDevEnabled = !s->debugDevEnabled;
      s->debugDevChanged = true;
    } else if (s->category == 2) {
      int *value = s->selected == 0 ? &s->sfxVolume : &s->musicVolume;
      if (pressed & SCE_CTRL_LEFT)
        (*value)--;
      else
        (*value)++;
      if (*value < 0)
        *value = 10;
      if (*value > 10)
        *value = 0;
      VitaSettings_applyAudio(s, audio);
    } else if (s->category == 3 && s->selected == 0) {
      s->vsyncEnabled = !s->vsyncEnabled;
    } else if (s->category == 3 && s->selected == 1) {
      s->ptbrEnabled = !s->ptbrEnabled;
      s->restartOnClose = false;
      s->open = false;
      requestRestart = true;
    } else if (s->category == 3 && s->selected == 2) {
      s->confirmChapterSelect = true;
    } else {
      resetDefaults(s);
      applyDisplaySettings(s);
      VitaSettings_applyAudio(s, audio);
    }
    saveSettings(s);
  }
  if (pressed & SCE_CTRL_CIRCLE) {
    s->open = false;
    s->inputCooldown = 2;
    requestRestart = s->restartOnClose;
  }
  return requestRestart;
}

static void drawLabel(Renderer *r, const char *text, float x, float y,
                      uint32_t color) {
  r->vtable->drawTextColor(r, text, x, y, 1.6f, 1.6f, 0.0f, color, color, color,
                           color, 1.0f, -1.0f);
}

static int findSettingsFont(Renderer *r) {
  for (uint32_t i = 0; i < r->dataWin->font.count; ++i) {
    Font *font = &r->dataWin->font.fonts[i];
    if (font->present && font->name != nullptr &&
        strcmp(font->name, "fnt_main") == 0)
      return (int)i;
  }
  return r->dataWin->font.count > 0 ? 0 : -1;
}

static void drawCenteredText(Renderer *r, const char *text, float centerX,
                             float y, float scale, uint32_t color) {
  int oldHalign = r->drawHalign;
  r->drawHalign = 1;
  r->vtable->drawTextColor(r, text, centerX, y, scale, scale, 0.0f, color,
                           color, color, color, 1.0f, -1.0f);
  r->drawHalign = oldHalign;
}

static void drawRightText(Renderer *r, const char *text, float rightX, float y,
                          float scale, uint32_t color) {
  int oldHalign = r->drawHalign;
  r->drawHalign = 2;
  r->vtable->drawTextColor(r, text, rightX, y, scale, scale, 0.0f, color, color,
                           color, color, 1.0f, -1.0f);
  r->drawHalign = oldHalign;
}

static Sprite *findSettingsSprite(Renderer *r, const char *name) {
  for (uint32_t i = 0; i < r->dataWin->sprt.count; ++i) {
    Sprite *sprite = &r->dataWin->sprt.sprites[i];
    if (sprite->present && sprite->name != nullptr &&
        strcmp(sprite->name, name) == 0 && sprite->textureCount > 0 &&
        sprite->tpagIndices[0] >= 0)
      return sprite;
  }
  return nullptr;
}

static void drawDialogBorder(Renderer *r, float left, float top, float right,
                             float bottom) {
  Sprite *corner = findSettingsSprite(r, "spr_textbox_topleft");
  Sprite *edgeTop = findSettingsSprite(r, "spr_textbox_top");
  Sprite *edgeLeft = findSettingsSprite(r, "spr_textbox_left");
  if (corner == nullptr || edgeTop == nullptr || edgeLeft == nullptr ||
      corner->width <= 0 || corner->height <= 0 || edgeTop->width <= 0 ||
      edgeLeft->height <= 0) {
    // Pizza Tower aesthetic: Thick Red and Yellow borders
    r->vtable->drawRectangle(r, left, top, right, bottom, 0x0000FF, 1.0f, true);
    r->vtable->drawRectangle(r, left - 1, top - 1, right + 1, bottom + 1, 0x0000FF, 1.0f, true);
    r->vtable->drawRectangle(r, left - 2, top - 2, right + 2, bottom + 2, 0x0000FF, 1.0f, true);
    
    r->vtable->drawRectangle(r, left + 4, top + 4, right - 4, bottom - 4, 0x00FFFF, 1.0f, true);
    r->vtable->drawRectangle(r, left + 5, top + 5, right - 5, bottom - 5, 0x00FFFF, 1.0f, true);
    r->vtable->drawRectangle(r, left + 6, top + 6, right - 6, bottom - 6, 0x00FFFF, 1.0f, true);
    return;
  }

  float cw = (float)corner->width;
  float ch = (float)corner->height;
  float horizontalScale = (right - left - cw * 2.0f) / (float)edgeTop->width;
  float verticalScale = (bottom - top - ch * 2.0f) / (float)edgeLeft->height;
  r->vtable->drawSprite(r, corner->tpagIndices[0], left, top, 0, 0, 1, 1, 0,
                        0xFFFFFF, 1.0f);
  r->vtable->drawSprite(r, corner->tpagIndices[0], right, top, 0, 0, -1, 1, 0,
                        0xFFFFFF, 1.0f);
  r->vtable->drawSprite(r, corner->tpagIndices[0], left, bottom, 0, 0, 1, -1, 0,
                        0xFFFFFF, 1.0f);
  r->vtable->drawSprite(r, corner->tpagIndices[0], right, bottom, 0, 0, -1, -1,
                        0, 0xFFFFFF, 1.0f);
  r->vtable->drawSprite(r, edgeTop->tpagIndices[0], left + cw, top, 0, 0,
                        horizontalScale, 1, 0, 0xFFFFFF, 1.0f);
  r->vtable->drawSprite(r, edgeTop->tpagIndices[0], left + cw, bottom, 0, 0,
                        horizontalScale, -1, 0, 0xFFFFFF, 1.0f);
  r->vtable->drawSprite(r, edgeLeft->tpagIndices[0], left, top + ch, 0, 0, 1,
                        verticalScale, 0, 0xFFFFFF, 1.0f);
  r->vtable->drawSprite(r, edgeLeft->tpagIndices[0], right, top + ch, 0, 0, -1,
                        verticalScale, 0, 0xFFFFFF, 1.0f);
}

void VitaSettings_draw(VitaSettings *s, Renderer *r) {
  if (!s->open)
    return;
  if (s->borderWarningFrames > 0)
    s->borderWarningFrames--;
  int oldFont = r->drawFont;
  r->drawFont = findSettingsFont(r);

  g_vitaPortOverlayFullScreen = 1;
  r->vtable->beginGUI(r, 960, 544, 0, 0, 960, 544,
                      RENDER_TARGET_HOST_FRAMEBUFFER);
  const float panelLeft = 108.0f, panelTop = 48.0f, panelRight = 852.0f,
              panelBottom = 486.0f;
  r->vtable->drawRectangle(r, panelLeft, panelTop, panelRight, panelBottom,
                           0x000000, 0.96f, false);
  drawDialogBorder(r, panelLeft, panelTop, panelRight, panelBottom);
  drawCenteredText(r, s->ptbrEnabled ? "CONFIGURACOES" : "GAME SETTINGS", 480,
                   79, 1.6f, 0xFFFFFF);

  const char *touch = s->touchEnabled ? (s->ptbrEnabled ? "Ligado" : "On")
                                      : (s->ptbrEnabled ? "Desligado" : "Off");
  const char *mod = s->ptbrEnabled ? "PT-BR" : "Original";
  const char *screen = s->widescreenEnabled
                           ? (s->ptbrEnabled ? "Ligadas" : "On")
                           : (s->ptbrEnabled ? "Desligadas" : "Off");
  char sfxVolume[32], musicVolume[32];
  snprintf(sfxVolume, sizeof(sfxVolume), "%d%%", s->sfxVolume * 10);
  snprintf(musicVolume, sizeof(musicVolume), "%d%%", s->musicVolume * 10);
  const char *categoriesPt[SETTINGS_CATEGORIES] = {"CONTROLES", "TELA", "AUDIO",
                                                   "SISTEMA"};
  const char *categoriesEn[SETTINGS_CATEGORIES] = {"CONTROLS", "SCREEN",
                                                   "AUDIO", "SYSTEM"};
  const char **categories = s->ptbrEnabled ? categoriesPt : categoriesEn;
  for (int i = 0; i < SETTINGS_CATEGORIES; ++i) {
    float centerX = 210.0f + i * 180.0f;
    if (i == s->category) {
      r->vtable->drawRectangle(r, centerX - 70, 126, centerX + 70, 156,
                               0x181818, 1.0f, false);
      r->vtable->drawRectangle(r, centerX - 70, 126, centerX + 70, 156,
                               0x00FFFF, 0.72f, true);
    }
    drawCenteredText(r, categories[i], centerX, 132, 1.15f,
                     i == s->category ? 0x00FFFF : 0x808080);
  }
  char adjustment[64];
  snprintf(adjustment, sizeof(adjustment), "%d,%d  %d%%", s->displayOffsetX,
           s->displayOffsetY, s->displayZoom);
  const char *vsync = s->vsyncEnabled ? (s->ptbrEnabled ? "Ligado" : "On")
                                      : (s->ptbrEnabled ? "Desligado" : "Off");
  const char *graphics =
      s->graphicsQuality == 0
          ? "Original"
          : (s->graphicsQuality == 1 ? (s->ptbrEnabled ? "Medio" : "Medium")
                                     : (s->ptbrEnabled ? "Baixo" : "Low"));
  const char *labelsPt[SETTINGS_CATEGORIES][4] = {
      {"Controles touch", "", "", ""},
      {"Bordas de console", "Ajustar tela", "Graficos", "Debug Dev"},
      {"Efeitos sonoros", "Musica", "", ""},
      {"VSync / 30 FPS", "Mod / Idioma", "Voltar aos capitulos", ""}};
  const char *labelsEn[SETTINGS_CATEGORIES][4] = {
      {"Touch controls", "", "", ""},
      {"Console borders", "Adjust screen", "Graphics", "Debug Dev"},
      {"Sound effects", "Music", "", ""},
      {"VSync / 30 FPS", "Mod / Language", "Chapter Select", ""}};
  const char *values[SETTINGS_CATEGORIES][4] = {
      {touch, "", "", ""},
      {screen, adjustment, graphics, s->debugDevEnabled ? "On" : "Off"},
      {sfxVolume, musicVolume, "", ""},
      {vsync, mod, "", ""}};
  int visibleItems = settingsItemCount(s->category);
  for (int i = 0; i < visibleItems; ++i) {
    float y = visibleItems == 4 ? 184.0f + i * 58.0f
                                : (visibleItems == 3 ? 196.0f + i * 72.0f
                                                     : 214.0f + i * 88.0f);
    if (i == s->selected) {
      r->vtable->drawRectangle(r, 194, y - 8, 766, y + 34, 0x141414, 1.0f,
                               false);
      r->vtable->drawRectangle(r, 194, y - 8, 766, y + 34, 0x404040, 1.0f,
                               true);
    }
    drawLabel(r, i == s->selected ? ">" : " ", 214, y,
              i == s->selected ? 0x00FFFF : 0xFFFFFF);
    const char *label =
        (i == 3 && s->category == 3)
            ? (s->ptbrEnabled ? "Restaurar padrao" : "Restore defaults")
            : (s->ptbrEnabled ? labelsPt[s->category][i]
                              : labelsEn[s->category][i]);
    drawLabel(r, label, 252, y, 0xFFFFFF);
    drawRightText(r, values[s->category][i], 742, y, 1.6f,
                  i == s->selected ? 0x00FFFF : 0xFFFFFF);
  }
  drawCenteredText(r,
                   s->ptbrEnabled ? "L/R: CATEGORIA    X: ALTERAR    O: FECHAR"
                                  : "L/R: CATEGORY    X: CHANGE    O: CLOSE",
                   480, 426, 1.1f, 0x808080);

  if (s->borderWarningFrames > 0) {
    r->vtable->drawRectangle(r, 176, 354, 784, 414, 0x000000, 0.98f, false);
    drawDialogBorder(r, 176, 354, 784, 414);
    drawCenteredText(r,
                     s->ptbrEnabled ? "ARQUIVOS DE BORDA NAO ENCONTRADOS."
                                    : "BORDER FILES WERE NOT FOUND.",
                     480, 372, 1.05f, 0xFFFF00);
  }

  if (s->confirmChapterSelect) {
    r->vtable->drawRectangle(r, 176, 174, 784, 370, 0x000000, 0.98f, false);
    drawDialogBorder(r, 176, 174, 784, 370);
    drawCenteredText(r,
                     s->ptbrEnabled ? "VOLTAR AOS CAPITULOS?"
                                    : "RETURN TO CHAPTER SELECT?",
                     480, 208, 1.35f, 0xFFFFFF);
    drawCenteredText(r,
                     s->ptbrEnabled ? "O PROGRESSO NAO SALVO SERA PERDIDO."
                                    : "UNSAVED PROGRESS WILL BE LOST.",
                     480, 260, 1.05f, 0xFFFF00);
    drawCenteredText(r,
                     s->ptbrEnabled ? "X: CONFIRMAR       O: CANCELAR"
                                    : "X: CONFIRM       O: CANCEL",
                     480, 316, 1.05f, 0xA0A0A0);
  }
  r->vtable->endGUI(r);
  g_vitaPortOverlayFullScreen = 0;
  r->drawFont = oldFont;
}

void VitaSettings_drawDevOverlay(VitaSettings *s, Renderer *r, const char *room,
                                 float fps, uint64_t stepUs, uint64_t audioUs,
                                 uint64_t renderUs, uint64_t gpuBytes,
                                 uint32_t evictions, uint32_t deferred,
                                 uint32_t ramHits) {
  if (!s->debugDevEnabled || s->open)
    return;
  int oldFont = r->drawFont;
  r->drawFont = findSettingsFont(r);
  r->vtable->beginGUI(r, 960, 544, 0, 0, 960, 544,
                      RENDER_TARGET_HOST_FRAMEBUFFER);
  r->vtable->drawRectangle(r, 8, 8, 820, 132, 0x000000, 0.82f, false);
  char line[192];
  snprintf(line, sizeof(line), "DEV  ROOM: %s", room != NULL ? room : "<null>");
  r->vtable->drawTextColor(r, line, 18, 16, 1.25f, 1.25f, 0, 0xFFFFFF, 0xFFFFFF,
                           0xFFFFFF, 0xFFFFFF, 1, -1);
  snprintf(line, sizeof(line),
           "FPS %.1f  STEP %lluus  AUDIO %lluus  RENDER %lluus", fps,
           (unsigned long long)stepUs, (unsigned long long)audioUs,
           (unsigned long long)renderUs);
  r->vtable->drawTextColor(r, line, 18, 54, 1.02f, 1.02f, 0, 0xFFFFFF, 0xFFFFFF,
                           0xFFFFFF, 0xFFFFFF, 1, -1);
  snprintf(line, sizeof(line),
           "GPU CACHE %.1f MiB  EVICT %u  DEFER %u  RAM HIT %u",
           (double)gpuBytes / 1048576.0, evictions, deferred, ramHits);
  r->vtable->drawTextColor(r, line, 18, 92, 1.02f, 1.02f, 0, 0x00FFFF, 0x00FFFF,
                           0x00FFFF, 0x00FFFF, 1, -1);
  r->vtable->endGUI(r);
  r->drawFont = oldFont;
}

typedef struct {
  const char *name;
  const char *path;
  GLuint texture;
  int width;
  int height;
} BundledControl;

static void touchControlLog(const char *phase, const char *name) {
  char line[128];
  int length =
      snprintf(line, sizeof(line), "TOUCH=%s control=%s\n", phase, name);
  SceUID fd = sceIoOpen("ux0:data/pizzatowersage/pizzatowersage.log",
                        SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666);
  if (fd >= 0) {
    sceIoWrite(fd, line, (SceSize)length);
    sceIoClose(fd);
  }
}

static BundledControl bundledControls[] = {
    {"spr_joybase", "app0:assets/touch/spr_joybase_frame0.png", 0, 0, 0},
    {"spr_joystick", "app0:assets/touch/spr_joystick_frame0.png", 0, 0, 0},
    {"spr_control_zkey", "app0:assets/touch/spr_control_zkey_frame0.png", 0, 0,
     0},
    {"spr_control_xkey", "app0:assets/touch/spr_control_xkey_frame0.png", 0, 0,
     0},
    {"spr_control_ckey", "app0:assets/touch/spr_control_ckey_frame0.png", 0, 0,
     0},
};

static bool drawBundledControl(const char *name, float centerX, float centerY,
                               float targetSize, float alpha) {
  BundledControl *control = nullptr;
  for (unsigned i = 0; i < sizeof(bundledControls) / sizeof(bundledControls[0]);
       ++i) {
    if (strcmp(bundledControls[i].name, name) == 0) {
      control = &bundledControls[i];
      break;
    }
  }
  if (control == nullptr)
    return false;
  if (control->texture == 0) {
    touchControlLog("asset_load_begin", name);
#ifdef PIZZA_NATIVE_GXM
    int channels = 0;
    SceUID fd = sceIoOpen(control->path, SCE_O_RDONLY, 0);
    if (fd < 0)
      return false;
    SceOff fileSize = sceIoLseek(fd, 0, SCE_SEEK_END);
    sceIoLseek(fd, 0, SCE_SEEK_SET);
    if (fileSize <= 0) {
      sceIoClose(fd);
      return false;
    }
    unsigned char *fileData = (unsigned char *)malloc((size_t)fileSize);
    if (fileData == nullptr) {
      sceIoClose(fd);
      return false;
    }
    int bytesRead = sceIoRead(fd, fileData, (unsigned int)fileSize);
    sceIoClose(fd);
    if (bytesRead != fileSize) {
      free(fileData);
      return false;
    }
    unsigned char *pixels = stbi_load_from_memory(
        fileData, bytesRead, &control->width, &control->height, &channels, 4);
    free(fileData);
    if (pixels == nullptr)
      return false;
    control->texture=PizzaGxm_image(control->width,control->height,pixels,false);
    stbi_image_free(pixels);
#else
    control->texture = VitaGlQuad_loadPng(control->path, &control->width, &control->height,
                                          false, GL_NEAREST, GL_CLAMP_TO_EDGE);
#endif
    if (control->texture == 0 || control->width <= 0 || control->height <= 0)
      return false;
    touchControlLog("asset_upload_complete", name);
  }
  float largest = control->width > control->height ? (float)control->width
                                                   : (float)control->height;
  float scale = targetSize / largest;
  float halfW = control->width * scale * 0.5f;
  float halfH = control->height * scale * 0.5f;
#ifdef PIZZA_NATIVE_GXM
  PizzaGxm_draw(control->texture,centerX-halfW,centerY-halfH,2*halfW,2*halfH,alpha,false);
#else
  VitaGlQuad_drawTex(control->texture,
                     centerX - halfW, centerY - halfH,
                     2.0f * halfW, 2.0f * halfH,
                     0.0f, 0.0f, 1.0f, 1.0f,
                     1.0f, 1.0f, 1.0f, alpha);
#endif
  return true;
}

void VitaSettings_drawTouchControls(VitaSettings *s, Renderer *r) {
  static bool firstOverlay = true;
  if (!s->touchEnabled || s->open || s->adjustMode)
    return;
  if (firstOverlay)
    touchControlLog("overlay_begin", "all");
#ifndef PIZZA_NATIVE_GXM
  VitaGlQuad_begin(r, 960, 544);
  drawBundledControl("spr_joybase", 155, 420, 205, 0.48f);
  drawBundledControl("spr_joystick", 155 + s->visualStickX * 38.0f,
                     420 + s->visualStickY * 38.0f,
                     125 +
                         (s->visualStickX != 0.0f || s->visualStickY != 0.0f ? 8 : 0),
                     0.68f);
  drawBundledControl("spr_control_zkey", 850, 385 + (s->visualConfirm ? 4 : 0),
                     s->visualConfirm ? 108 : 92, s->visualConfirm ? 0.92f : 0.58f);
  drawBundledControl("spr_control_xkey", 755, 455 + (s->visualCancel ? 4 : 0),
                     s->visualCancel ? 108 : 92, s->visualCancel ? 0.92f : 0.58f);
  drawBundledControl("spr_control_ckey", 755, 340 + (s->visualMenu ? 4 : 0),
                     s->visualMenu ? 108 : 92, s->visualMenu ? 0.92f : 0.58f);
  VitaGlQuad_end(r);
#else
  int oldFont = r->drawFont;
  if (r->drawFont < 0 || (uint32_t)r->drawFont >= r->dataWin->font.count)
    r->drawFont = 0;
  g_vitaPortOverlayFullScreen = 1;
  r->vtable->beginGUI(r, 960, 544, 0, 0, 960, 544,
                      RENDER_TARGET_HOST_FRAMEBUFFER);
  drawBundledControl("spr_joybase", 155, 420, 205, 0.48f);
  drawBundledControl("spr_joystick", 155 + s->visualStickX * 38.0f,
                     420 + s->visualStickY * 38.0f,
                     125 +
                         (s->visualStickX != 0.0f || s->visualStickY != 0.0f ? 8 : 0),
                     0.68f);
  drawBundledControl("spr_control_zkey", 850, 385 + (s->visualConfirm ? 4 : 0),
                     s->visualConfirm ? 108 : 92, s->visualConfirm ? 0.92f : 0.58f);
  drawBundledControl("spr_control_xkey", 755, 455 + (s->visualCancel ? 4 : 0),
                     s->visualCancel ? 108 : 92, s->visualCancel ? 0.92f : 0.58f);
  drawBundledControl("spr_control_ckey", 755, 340 + (s->visualMenu ? 4 : 0),
                     s->visualMenu ? 108 : 92, s->visualMenu ? 0.92f : 0.58f);
  r->vtable->endGUI(r);
  g_vitaPortOverlayFullScreen = 0;
  r->drawFont = oldFont;
#endif
  if (firstOverlay) {
    touchControlLog("overlay_complete", "all");
    firstOverlay = false;
  }
}

void VitaSettings_drawQualityNotice(VitaSettings *s, Renderer *r) {
  if (s->qualityNoticeFrames <= 0) return;
  s->qualityNoticeFrames--;
  const char *quality = s->graphicsQuality == 0 ? "BAIXA" :
                        (s->graphicsQuality == 1 ? "MEDIA" : "ALTA");
  char text[64];
  snprintf(text, sizeof(text), "TEXTURAS: %s", quality);
  int oldFont = r->drawFont;
  if (r->drawFont < 0 || (uint32_t)r->drawFont >= r->dataWin->font.count)
    r->drawFont = 0;
  g_vitaPortOverlayFullScreen = 1;
  r->vtable->beginGUI(r, 960, 544, 0, 0, 960, 544,
                      RENDER_TARGET_HOST_FRAMEBUFFER);
  r->vtable->drawRectangle(r, 350, 18, 610, 62, 0x000000, 0.72f, false);
  drawLabel(r, text, 382, 31, 0xFFFFFF);
  r->vtable->endGUI(r);
  g_vitaPortOverlayFullScreen = 0;
  r->drawFont = oldFont;
}

void VitaSettings_drawCalibration(VitaSettings *s, Renderer *r) {
  if (!s->adjustMode)
    return;
  int gameW = r->runner->renderGameW;
  int gameH = r->runner->renderGameH;
  if (gameW <= 0 || gameH <= 0)
    return;
  int width, height;
  if ((gameW * 544) / gameH < 960) {
    width = (gameW * 544) / gameH;
    height = 544;
  } else {
    width = 960;
    height = (gameH * 960) / gameW;
  }
  width = width * g_vitaDisplayZoom / 100;
  height = height * g_vitaDisplayZoom / 100;
  int x = (960 - width) / 2 + g_vitaDisplayOffsetX;
  int y = (544 - height) / 2 + g_vitaDisplayOffsetY;
  g_vitaPortOverlayFullScreen = 1;
  r->vtable->beginGUI(r, 960, 544, 0, 0, 960, 544,
                      RENDER_TARGET_HOST_FRAMEBUFFER);
  r->vtable->drawRectangle(r, (float)x, (float)y, (float)(x + width - 1),
                           (float)(y + height - 1), 0xFFFFFF, 1.0f, true);
  drawLabel(r,
            s->ptbrEnabled ? "ESQ: MOVER  DIR: ZOOM  X: SALVAR  O: RESET"
                           : "LEFT: MOVE  RIGHT: ZOOM  X: SAVE  O: RESET",
            125, 18, 0xFFFFFF);
  r->vtable->endGUI(r);
  g_vitaPortOverlayFullScreen = 0;
}

void VitaSettings_setTouchVisuals(VitaSettings *s, float stickX, float stickY,
                                  bool confirm, bool cancel, bool menu) {
  if (stickX < -1.0f)
    stickX = -1.0f;
  if (stickX > 1.0f)
    stickX = 1.0f;
  if (stickY < -1.0f)
    stickY = -1.0f;
  if (stickY > 1.0f)
    stickY = 1.0f;
  s->visualStickX += (stickX - s->visualStickX) * 0.82f;
  s->visualStickY += (stickY - s->visualStickY) * 0.82f;
  if (s->visualStickX > -0.02f && s->visualStickX < 0.02f)
    s->visualStickX = 0.0f;
  if (s->visualStickY > -0.02f && s->visualStickY < 0.02f)
    s->visualStickY = 0.0f;
  s->visualConfirm = confirm;
  s->visualCancel = cancel;
  s->visualMenu = menu;
}

void VitaSettings_setLauncherMode(bool launcherMode) {
  g_launcherMode = launcherMode;
  if (launcherMode) {
    g_vitaDisplayOffsetX = 0;
    g_vitaDisplayOffsetY = 0;
    g_vitaDisplayZoom = 100;
  }
}
