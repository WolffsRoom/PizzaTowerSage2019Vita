#include "vita_modern_pause.h"
#include "vita_gl_quad.h"
#include "vita_modern_font.h"

#include <math.h>
#include <stdio.h>

#ifndef PIZZA_NATIVE_GXM

static bool g_assetsLoaded = false;
static VitaGlTex g_bgTex = 0;
static VitaGlTex g_fontTex = 0;
static VitaGlTex g_sliderTex = 0;
static VitaGlTex g_knobIdleTex = 0;
static VitaGlTex g_knobYellTex = 0;
static VitaGlTex g_glyphDpadTex = 0;
static VitaGlTex g_glyphCrossTex = 0;
static VitaGlTex g_glyphCircleTex = 0;
static VitaGlTex g_glyphSelectTex = 0;
static VitaGlTex g_glyphStartTex = 0;

static void ensureAssetsLoaded(void) {
    if (g_assetsLoaded) return;
    g_assetsLoaded = true;
    g_bgTex = VitaGlQuad_loadPng("app0:assets/ui/options_bg.png", NULL, NULL,
                                 false, GL_NEAREST, GL_CLAMP_TO_EDGE);
    g_fontTex = VitaGlQuad_loadPng("app0:assets/ui/options_font.png", NULL, NULL,
                                   false, GL_NEAREST, GL_CLAMP_TO_EDGE);
    g_sliderTex = VitaGlQuad_loadPng("app0:assets/ui/options_slider.png", NULL, NULL,
                                     false, GL_NEAREST, GL_CLAMP_TO_EDGE);
    g_knobIdleTex = VitaGlQuad_loadPng("app0:assets/ui/options_slider_knob_0.png", NULL, NULL,
                                       false, GL_NEAREST, GL_CLAMP_TO_EDGE);
    g_knobYellTex = VitaGlQuad_loadPng("app0:assets/ui/options_slider_knob_1.png", NULL, NULL,
                                       false, GL_NEAREST, GL_CLAMP_TO_EDGE);
    g_glyphDpadTex = VitaGlQuad_loadPng("app0:assets/ps4_glyphs/dpad.png", NULL, NULL,
                                        false, GL_NEAREST, GL_CLAMP_TO_EDGE);
    g_glyphCrossTex = VitaGlQuad_loadPng("app0:assets/ps4_glyphs/cross.png", NULL, NULL,
                                         false, GL_NEAREST, GL_CLAMP_TO_EDGE);
    g_glyphCircleTex = VitaGlQuad_loadPng("app0:assets/ps4_glyphs/circle.png", NULL, NULL,
                                          false, GL_NEAREST, GL_CLAMP_TO_EDGE);
    g_glyphSelectTex = VitaGlQuad_loadPng("app0:assets/ps4_glyphs/select.png", NULL, NULL,
                                          false, GL_NEAREST, GL_CLAMP_TO_EDGE);
    g_glyphStartTex = VitaGlQuad_loadPng("app0:assets/ps4_glyphs/start.png", NULL, NULL,
                                         false, GL_NEAREST, GL_CLAMP_TO_EDGE);
}

static float measureTextWidth(const char* text, float scale) {
    if (!text) return 0.0f;
    float w = 0.0f;
    const char* p = text;
    while (*p) {
        uint32_t cp = vita_utf8_next(&p);
        if (cp == ' ') {
            w += 14.0f * scale;
            continue;
        }
        int idx = vita_modern_font_lookup(cp);
        if (idx < 0 || idx >= VITA_MODERN_FONT_GLYPH_COUNT) {
            w += 14.0f * scale;
            continue;
        }
        w += g_vitaModernGlyphs[idx].advance * scale;
    }
    return w;
}

static void drawModernText(const char* text, float x, float y, float scale,
                           float r, float g, float b, float a) {
    if (!text || g_fontTex == 0) return;
    float curX = x;
    const char* p = text;
    while (*p) {
        uint32_t cp = vita_utf8_next(&p);
        if (cp == ' ') {
            curX += 14.0f * scale;
            continue;
        }
        int idx = vita_modern_font_lookup(cp);
        if (idx < 0 || idx >= VITA_MODERN_FONT_GLYPH_COUNT) {
            curX += 14.0f * scale;
            continue;
        }
        const VitaModernGlyph* gl = &g_vitaModernGlyphs[idx];
        VitaGlQuad_drawTex(g_fontTex,
                           curX, y + gl->yoff * scale,
                           gl->w * scale, gl->h * scale,
                           gl->u0, gl->v0, gl->u1, gl->v1,
                           r, g, b, a);
        curX += gl->advance * scale;
    }
}

static void drawModernTextShadow(const char* text, float x, float y, float scale,
                                 float r, float g, float b, float a) {
    drawModernText(text, x + 2.0f, y + 2.0f, scale, 0.0f, 0.0f, 0.0f, a * 0.55f);
    drawModernText(text, x, y, scale, r, g, b, a);
}

static void drawSlider(float x, float y, int value, bool selected, bool yelling) {
    const float trackW = 223.0f;
    const float trackH = 31.0f;
    float tint = selected ? 1.0f : 0.62f;
    if (value < 0) value = 0;
    if (value > 10) value = 10;

    if (g_sliderTex != 0) {
        VitaGlQuad_drawTex(g_sliderTex, x, y, trackW, trackH,
                           0.0f, 0.0f, 1.0f, 1.0f, tint, tint, tint, 1.0f);
    } else {
        VitaGlQuad_drawRect(x, y + 10.0f, trackW, 10.0f, 0.2f, 0.2f, 0.2f, 0.9f);
    }

    float t = (float)value / 10.0f;
    float knobCx = x + 16.0f + t * (trackW - 32.0f);
    float knobCy = y + trackH * 0.5f;
    VitaGlTex knobTex = (selected && yelling && g_knobYellTex != 0) ? g_knobYellTex : g_knobIdleTex;
    if (knobTex != 0) {
        float knobSize = selected ? 50.0f : 44.0f;
        VitaGlQuad_drawTex(knobTex,
                           knobCx - knobSize * 0.5f, knobCy - knobSize * 0.5f,
                           knobSize, knobSize,
                           0.0f, 0.0f, 1.0f, 1.0f,
                           selected ? 1.0f : 0.72f,
                           selected ? 1.0f : 0.72f,
                           selected ? 1.0f : 0.72f,
                           1.0f);
    }
}

void VitaModernPause_draw(const PizzaNativePause* pause,
                          const VitaSettings* settings,
                          Renderer* renderer) {
    if (!pause || !pause->open || !settings) return;

    VitaGlQuad_begin(renderer, 960, 544);
    ensureAssetsLoaded();

    /* Background */
    if (g_bgTex != 0) {
        int ox = (int)((pause->tick / 2U) % 400U);
        int oy = (int)((pause->tick / 3U) % 400U);
        for (int ty = -oy; ty < 544; ty += 400) {
            for (int tx = -ox; tx < 960; tx += 400) {
                VitaGlQuad_drawTex(g_bgTex, (float)tx, (float)ty, 400.0f, 400.0f,
                                   0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.98f);
            }
        }
    } else {
        VitaGlQuad_drawRect(0.0f, 0.0f, 960.0f, 544.0f, 0.80f, 0.52f, 0.04f, 0.96f);
    }

    /* Subtle top/bottom vignette strips for readability */
    VitaGlQuad_drawRect(0.0f, 0.0f, 960.0f, 76.0f, 0.0f, 0.0f, 0.0f, 0.36f);
    VitaGlQuad_drawRect(0.0f, 486.0f, 960.0f, 58.0f, 0.0f, 0.0f, 0.0f, 0.52f);

    /* Header title */
    bool isPt = settings->ptbrEnabled;
    const char* title = isPt ? "CONFIGURAÇÕES" : "GAME SETTINGS";
    float titleScale = 0.88f;
    float titleW = measureTextWidth(title, titleScale);
    drawModernTextShadow(title, (960.0f - titleW) * 0.5f, 16.0f, titleScale,
                         1.0f, 0.96f, 0.72f, 1.0f);

    /* Menu options */
    const char* labelsPt[PIZZA_PAUSE_ITEM_COUNT] = {
        "CONTINUAR",
        "MÚSICA",
        "EFEITOS",
        "IDIOMA",
        "FILTRO CRT",
        "CONTROLES TOUCH",
        "VSYNC",
        "SAIR DO JOGO"
    };

    const char* labelsEn[PIZZA_PAUSE_ITEM_COUNT] = {
        "CONTINUE",
        "MUSIC",
        "SFX",
        "LANGUAGE",
        "CRT FILTER",
        "TOUCH CONTROLS",
        "VSYNC",
        "EXIT GAME"
    };

    const char** labels = isPt ? labelsPt : labelsEn;

    const float rowScale = 0.72f;
    const float listTopY = 88.0f;
    const float rowStep = 48.0f;
    const float labelX = 116.0f;
    const float rightEdgeX = 838.0f;
    const float sliderX = 615.0f;
    float bob = sinf((float)pause->tick * 0.14f) * 2.5f;

    for (unsigned i = 0; i < PIZZA_PAUSE_ITEM_COUNT; ++i) {
        bool sel = (i == pause->selected) && !pause->confirmExit;
        float rowY = listTopY + (float)i * rowStep + (sel ? bob : 0.0f);
        float drawX = labelX + (sel ? 10.0f : 0.0f);

        float cr = sel ? 1.0f : 0.58f;
        float cg = sel ? 1.0f : 0.58f;
        float cb = sel ? 1.0f : 0.58f;
        if (i == PIZZA_PAUSE_ITEM_EXIT) {
            cr = sel ? 1.0f : 0.62f;
            cg = sel ? 0.52f : 0.45f;
            cb = sel ? 0.48f : 0.45f;
        }

        drawModernTextShadow(labels[i], drawX, rowY, rowScale, cr, cg, cb, 1.0f);

        if (i == PIZZA_PAUSE_ITEM_MUSIC || i == PIZZA_PAUSE_ITEM_SFX) {
            int vol = (i == PIZZA_PAUSE_ITEM_MUSIC) ? settings->musicVolume : settings->sfxVolume;
            char valStr[16];
            snprintf(valStr, sizeof(valStr), "%d", vol);
            float valScale = 0.62f;
            float valW = measureTextWidth(valStr, valScale);
            drawModernTextShadow(valStr, sliderX - 24.0f - valW, rowY + 6.0f, valScale,
                                 cr, cg, cb, 1.0f);
            drawSlider(sliderX, rowY + 8.0f, vol, sel, pause->knobAnimFrames > 0);
        } else if (i == PIZZA_PAUSE_ITEM_LANG) {
            const char* langStr = settings->ptbrEnabled ? "PORTUGUÊS" : "ENGLISH";
            float sw = measureTextWidth(langStr, rowScale);
            float vr = sel ? 1.0f : 0.85f;
            float vg = sel ? 0.92f : 0.75f;
            float vb = sel ? 0.35f : 0.35f;
            drawModernTextShadow(langStr, rightEdgeX - sw, rowY, rowScale, vr, vg, vb, 1.0f);
        } else if (i == PIZZA_PAUSE_ITEM_CRT ||
                   i == PIZZA_PAUSE_ITEM_TOUCH ||
                   i == PIZZA_PAUSE_ITEM_VSYNC) {
            bool on = false;
            if (i == PIZZA_PAUSE_ITEM_CRT) on = (settings->screenFilterMode != 0);
            else if (i == PIZZA_PAUSE_ITEM_TOUCH) on = settings->touchEnabled;
            else on = settings->vsyncEnabled;

            const char* stateStr = on ? (isPt ? "LIGADO" : "ON") : (isPt ? "DESLIGADO" : "OFF");
            float sw = measureTextWidth(stateStr, rowScale);
            float vr = on ? (sel ? 0.55f : 0.42f) : cr;
            float vg = on ? (sel ? 1.0f : 0.68f) : cg;
            float vb = on ? (sel ? 0.55f : 0.42f) : cb;
            drawModernTextShadow(stateStr, rightEdgeX - sw, rowY, rowScale, vr, vg, vb, 1.0f);
        }
    }

    /* Footer bar with PS4 glyph hints + version tag */
    {
        float hintScale = 0.48f;
        float textH = VITA_MODERN_FONT_CELL_HEIGHT * hintScale;
        float hintTextY = 515.0f - textH * 0.5f - 2.0f;
        float x = 34.0f;

        const char* hintNav = isPt ? "NAVEGAR" : "NAVIGATE";
        const char* hintSel = isPt ? "SELECIONAR" : "SELECT";
        const char* hintBack = isPt ? "VOLTAR" : "BACK";
        const char* hintClose = isPt ? "FECHAR" : "CLOSE";

        if (g_glyphDpadTex != 0) {
            VitaGlQuad_drawTex(g_glyphDpadTex, x, 501.0f, 28.0f, 28.0f,
                               0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
            x += 34.0f;
        }
        drawModernTextShadow(hintNav, x, hintTextY, hintScale, 0.95f, 0.95f, 0.95f, 1.0f);
        x += measureTextWidth(hintNav, hintScale) + 22.0f;

        if (g_glyphCrossTex != 0) {
            VitaGlQuad_drawTex(g_glyphCrossTex, x, 502.0f, 26.0f, 26.0f,
                               0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
            x += 32.0f;
        }
        drawModernTextShadow(hintSel, x, hintTextY, hintScale, 0.95f, 0.95f, 0.95f, 1.0f);
        x += measureTextWidth(hintSel, hintScale) + 22.0f;

        if (g_glyphCircleTex != 0) {
            VitaGlQuad_drawTex(g_glyphCircleTex, x, 502.0f, 26.0f, 26.0f,
                               0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
            x += 32.0f;
        }
        drawModernTextShadow(hintBack, x, hintTextY, hintScale, 0.95f, 0.95f, 0.95f, 1.0f);
        x += measureTextWidth(hintBack, hintScale) + 22.0f;

        if (g_glyphSelectTex != 0) {
            VitaGlQuad_drawTex(g_glyphSelectTex, x, 505.0f, 52.0f, 25.0f,
                               0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
            x += 58.0f;
        } else if (g_glyphStartTex != 0) {
            VitaGlQuad_drawTex(g_glyphStartTex, x, 505.0f, 52.0f, 25.0f,
                               0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
            x += 58.0f;
        }
        drawModernTextShadow(hintClose, x, hintTextY, hintScale, 0.95f, 0.95f, 0.95f, 1.0f);

        const char* ver = "V0.51";
        float verScale = 0.52f;
        float verW = measureTextWidth(ver, verScale);
        drawModernTextShadow(ver, 932.0f - verW, 501.0f, verScale,
                             0.95f, 0.85f, 0.45f, 1.0f);
    }

    /* Exit confirmation modal */
    if (pause->confirmExit) {
        VitaGlQuad_drawRect(0.0f, 0.0f, 960.0f, 544.0f, 0.0f, 0.0f, 0.0f, 0.72f);

        const float bx = 210.0f, by = 152.0f, bw = 540.0f, bh = 232.0f;
        VitaGlQuad_drawRect(bx, by, bw, bh, 0.08f, 0.04f, 0.02f, 0.96f);
        VitaGlQuad_drawOutline(bx - 4.0f, by - 4.0f, bw + 8.0f, bh + 8.0f, 4.0f,
                               1.0f, 0.65f, 0.0f, 1.0f);
        VitaGlQuad_drawOutline(bx, by, bw, bh, 2.0f, 1.0f, 1.0f, 1.0f, 0.95f);

        const char* qTitle = isPt ? "CONFIRMAR SAÍDA?" : "CONFIRM EXIT?";
        float qTitleScale = 0.82f;
        float qTitleW = measureTextWidth(qTitle, qTitleScale);
        drawModernTextShadow(qTitle, (960.0f - qTitleW) * 0.5f, by + 32.0f, qTitleScale,
                             1.0f, 1.0f, 1.0f, 1.0f);

        const char* qSub = isPt ? "DESEJA FECHAR O JOGO?" : "DO YOU WANT TO QUIT?";
        float qSubScale = 0.54f;
        float qSubW = measureTextWidth(qSub, qSubScale);
        drawModernTextShadow(qSub, (960.0f - qSubW) * 0.5f, by + 98.0f, qSubScale,
                             1.0f, 0.88f, 0.52f, 1.0f);

        float optScale = 0.64f;
        const char* yesStr = isPt ? "SIM" : "YES";
        const char* noStr = isPt ? "NÃO" : "NO";
        float simW = measureTextWidth(yesStr, optScale);
        float naoW = measureTextWidth(noStr, optScale);
        float blockW = (32.0f + 10.0f + simW) + 56.0f + (32.0f + 10.0f + naoW);
        float cx = (960.0f - blockW) * 0.5f;
        float optY = by + 162.0f;

        if (g_glyphCrossTex != 0) {
            VitaGlQuad_drawTex(g_glyphCrossTex, cx, optY + 2.0f, 30.0f, 30.0f,
                               0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        }
        drawModernTextShadow(yesStr, cx + 38.0f, optY, optScale, 1.0f, 0.55f, 0.48f, 1.0f);
        cx += 38.0f + simW + 56.0f;

        if (g_glyphCircleTex != 0) {
            VitaGlQuad_drawTex(g_glyphCircleTex, cx, optY + 2.0f, 30.0f, 30.0f,
                               0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        }
        drawModernTextShadow(noStr, cx + 38.0f, optY, optScale, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    VitaGlQuad_end(renderer);
}

void VitaModernPause_shutdown(void) {
    VitaGlQuad_deleteTex(&g_bgTex);
    VitaGlQuad_deleteTex(&g_fontTex);
    VitaGlQuad_deleteTex(&g_sliderTex);
    VitaGlQuad_deleteTex(&g_knobIdleTex);
    VitaGlQuad_deleteTex(&g_knobYellTex);
    VitaGlQuad_deleteTex(&g_glyphDpadTex);
    VitaGlQuad_deleteTex(&g_glyphCrossTex);
    VitaGlQuad_deleteTex(&g_glyphCircleTex);
    VitaGlQuad_deleteTex(&g_glyphSelectTex);
    VitaGlQuad_deleteTex(&g_glyphStartTex);
    g_assetsLoaded = false;
}

#else

void VitaModernPause_draw(const PizzaNativePause* pause,
                          const VitaSettings* settings,
                          Renderer* renderer) {
    (void)pause;
    (void)settings;
    (void)renderer;
}

void VitaModernPause_shutdown(void) {}

#endif

