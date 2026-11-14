#include "vita_borders.h"

#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#ifdef PIZZA_NATIVE_GXM
#include "pizza_gxm_platform.h"
typedef unsigned GLuint;
typedef float GLfloat;
#else
#include <vitaGL.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stb_image.h"
#ifndef PIZZA_NATIVE_GXM
#include "vita_gl_quad.h"
#endif

#define BORDER_ROOT "ux0:data/pizzatowersage/borders/"
#define BORDER_LOG "ux0:data/pizzatowersage/pizzatowersage.log"

extern int g_vitaConsoleBordersEnabled;

static const char* borderPath = NULL;
static GLuint borderTexture = 0;
static GLuint previousBorderTexture = 0;
static int previousBorderWidth = 0;
static int previousBorderHeight = 0;
static uint64_t borderTransitionStart = 0;
#define BORDER_TRANSITION_US 350000ULL
static int borderWidth = 0;
static int borderHeight = 0;
static int borderLoadAttempted = 0;
static int borderChapter = 0;
static char borderRoom[96] = {0};
static int chapterMenuSeen = 0;
static int gameplayBordersActive = 0;

static void borderLog(const char* phase) {
    char line[256];
    int length = snprintf(line, sizeof(line), "CONSOLE_BORDER=%s path=%s size=%dx%d\n",
                          phase, borderPath != NULL ? borderPath : "none", borderWidth, borderHeight);
    SceUID fd = sceIoOpen(BORDER_LOG, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666);
    if (fd >= 0) {
        sceIoWrite(fd, line, (SceSize)length);
        sceIoClose(fd);
    }
}

static const char* chapterBorder(int chapter) {
    switch (chapter) {
        case 1: return BORDER_ROOT "border_dw_blue_stars_0.png";
        case 2: return BORDER_ROOT "border_dw_cyber_0.png";
        case 3: return BORDER_ROOT "border_dw_teevie_0.png";
        case 4: return BORDER_ROOT "border_dw_church_a_0.png";
        case 5: return BORDER_ROOT "border_lw_town_0.png";
        default: return NULL;
    }
}

static int loadBorderTexture(void) {
    borderLoadAttempted = 1;
    if (borderPath == NULL) return 0;
    SceUID fd = sceIoOpen(borderPath, SCE_O_RDONLY, 0);
    if (fd < 0) { borderLog("open_failed"); return 0; }
    SceOff fileSize = sceIoLseek(fd, 0, SCE_SEEK_END);
    sceIoLseek(fd, 0, SCE_SEEK_SET);
    if (fileSize <= 0) { sceIoClose(fd); borderLog("empty_file"); return 0; }
    unsigned char* fileData = (unsigned char*)malloc((size_t)fileSize);
    if (fileData == NULL) { sceIoClose(fd); borderLog("file_alloc_failed"); return 0; }
    int bytesRead = sceIoRead(fd, fileData, (unsigned int)fileSize);
    sceIoClose(fd);
    if (bytesRead != fileSize) { free(fileData); borderLog("read_failed"); return 0; }

    int channels = 0;
    unsigned char* pixels = stbi_load_from_memory(fileData, bytesRead, &borderWidth, &borderHeight, &channels, 4);
    free(fileData);
    if (pixels == NULL) { borderLog("decode_failed"); return 0; }

    // The source pack uses 1920x1080 images, but the Vita output is 960x544.
    // Downsample before the GPU upload to reduce each border from about 8 MiB
    // to about 2 MiB of graphics memory.
    if (borderWidth > 960 || borderHeight > 544) {
        const int targetWidth = 960;
        const int targetHeight = 544;
        unsigned char* resized = (unsigned char*)malloc((size_t)targetWidth * targetHeight * 4U);
        if (resized == NULL) {
            stbi_image_free(pixels);
            borderLog("resize_alloc_failed");
            return 0;
        }
        for (int y = 0; y < targetHeight; ++y) {
            int sourceY = (int)((long long)y * borderHeight / targetHeight);
            for (int x = 0; x < targetWidth; ++x) {
                int sourceX = (int)((long long)x * borderWidth / targetWidth);
                const unsigned char* source = pixels + ((size_t)sourceY * borderWidth + sourceX) * 4U;
                unsigned char* target = resized + ((size_t)y * targetWidth + x) * 4U;
                target[0] = source[0];
                target[1] = source[1];
                target[2] = source[2];
                target[3] = source[3];
            }
        }
        stbi_image_free(pixels);
        pixels = resized;
        borderWidth = targetWidth;
        borderHeight = targetHeight;
    }

#ifndef PIZZA_NATIVE_GXM
    GLint oldActiveTex = GL_TEXTURE0, oldTex0 = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &oldActiveTex);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTex0);
    glGenTextures(1, &borderTexture);
    glBindTexture(GL_TEXTURE_2D, borderTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#endif
    // Borders are background art and do not need 32-bit color. Pack in place
    // to halve GPU residency, which is especially important while two textures
    // coexist during a crossfade.
    size_t pixelCount = (size_t)borderWidth * (size_t)borderHeight;
    unsigned short* packed = (unsigned short*)pixels;
    for (size_t i = 0; i < pixelCount; ++i) {
        const unsigned char* src = pixels + i * 4U;
        unsigned short a = src[3] == 0 ? 0 : (unsigned short)((src[3] + 15U) >> 4);
        if (a > 15U) a = 15U;
        packed[i] = (unsigned short)(((unsigned short)(src[0] >> 4) << 12) |
                                     ((unsigned short)(src[1] >> 4) << 8) |
                                     ((unsigned short)(src[2] >> 4) << 4) | a);
    }
#ifdef PIZZA_NATIVE_GXM
    borderTexture=PizzaGxm_image(borderWidth,borderHeight,packed,true);
    free(pixels);
    if(!borderTexture){borderLog("upload_failed");return 0;}
#else
    while (glGetError() != GL_NO_ERROR) {}
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, borderWidth, borderHeight, 0, GL_RGBA,
                 GL_UNSIGNED_SHORT_4_4_4_4, packed);
    free(pixels);
    GLenum err = glGetError();
    glBindTexture(GL_TEXTURE_2D, (GLuint)oldTex0);
    glActiveTexture((GLenum)oldActiveTex);
    if (err != GL_NO_ERROR) {
        glDeleteTextures(1, &borderTexture);
        borderTexture = 0;
        borderLog("upload_failed");
        return 0;
    }
#endif
    // PNG decoding/uploading can itself take longer than the transition. Start
    // the visible crossfade only after the replacement texture is actually ready.
    if (previousBorderTexture != 0) borderTransitionStart = sceKernelGetProcessTimeWide();
    borderLog("loaded");
    return 1;
}

void VitaBorders_init(int chapter) {
    borderChapter = chapter;
    // Never show chapter art over VitaGL, initialization, opening logos or the
    // chapter title/menu. Gameplay explicitly unlocks it after PLACE_MENU.
    borderPath = NULL;
    borderLoadAttempted = 0;
    chapterMenuSeen = 0;
    gameplayBordersActive = 0;
    borderLog(chapter > 0 ? "waiting_for_gameplay" : "launcher_disabled");
}

static const char* roomBorder(const char* room) {
    if (room == NULL) return NULL;
    // Borders belong behind gameplay only. Native save/configuration screens
    // and the chapter selector already provide their own complete backdrop.
    if (strstr(room, "MENU") != NULL || strstr(room, "menu") != NULL ||
        strstr(room, "CHAPTER_SELECT") != NULL || strstr(room, "chapter_select") != NULL ||
        strstr(room, "SELECT") != NULL || strstr(room, "select") != NULL) {
        if (strstr(room, "PLACE_MENU") != NULL) {
            chapterMenuSeen = 1;
            gameplayBordersActive = 0;
        }
        return NULL;
    }
    if (!gameplayBordersActive) {
        if (!chapterMenuSeen) return NULL;
        // The first regular room reached after the chapter menu marks the
        // actual game session. Openings before that point stay borderless.
        gameplayBordersActive = 1;
        borderLog("gameplay_enabled");
    }
    // Chapter 5 opens in Hometown. Its intro, Kris/Toriel house, yard and city
    // all use the Light World town surround; Garden is selected only by rooms
    // whose names explicitly identify that Dark World area.
    if (borderChapter == 5 &&
        (strstr(room, "intro") != NULL || strstr(room, "kris") != NULL ||
         strstr(room, "torhouse") != NULL || strstr(room, "hallway") != NULL ||
         strstr(room, "town") != NULL || strstr(room, "city") != NULL ||
         strstr(room, "home") != NULL || strstr(room, "house") != NULL))
        return BORDER_ROOT "border_lw_town_0.png";
    if (strstr(room, "titan") != NULL) return BORDER_ROOT "border_dw_titan_eyes_0.png";
    if (strstr(room, "cliff") != NULL) return BORDER_ROOT "border_dw_garden_cliff_0.png";
    if (strstr(room, "garden") != NULL) return BORDER_ROOT "border_dw_garden_0.png";
    if (strstr(room, "church") != NULL) return BORDER_ROOT "border_dw_church_a_0.png";
    if (strstr(room, "mansion") != NULL) return BORDER_ROOT "border_dw_mansion_0.png";
    if (strstr(room, "green") != NULL) return BORDER_ROOT "border_dw_green_room_0.png";
    if (strstr(room, "teevie") != NULL || strstr(room, "tv_") != NULL) return BORDER_ROOT "border_dw_teevie_0.png";
    if (strstr(room, "cyber") != NULL) return BORDER_ROOT "border_dw_cyber_0.png";
    if (strstr(room, "city") != NULL) return BORDER_ROOT "border_dw_city_0.png";
    if (strstr(room, "castle") != NULL || strstr(room, "town") != NULL) {
        return borderChapter >= 2 ? BORDER_ROOT "border_dw_castletown_0.png" : BORDER_ROOT "border_lw_town_0.png";
    }
    if (strstr(room, "home") != NULL || strstr(room, "house") != NULL || strstr(room, "school") != NULL || strstr(room, "class") != NULL)
        return BORDER_ROOT "border_lw_town_morning_0.png";
    return chapterBorder(borderChapter);
}

void VitaBorders_updateRoom(const char* roomName) {
    const char* next = roomBorder(roomName);
    if (roomName != NULL && strcmp(borderRoom, roomName) == 0 && next == borderPath) return;
    snprintf(borderRoom, sizeof(borderRoom), "%s", roomName != NULL ? roomName : "<null>");
    if (next == borderPath) return;
    if (previousBorderTexture != 0) 
#ifdef PIZZA_NATIVE_GXM
    PizzaGxm_delete(previousBorderTexture);
#else
    glDeleteTextures(1, &previousBorderTexture);
#endif

    previousBorderTexture = borderTexture;
    previousBorderWidth = borderWidth;
    previousBorderHeight = borderHeight;
    borderTexture = 0;
    borderWidth = borderHeight = 0;
    borderLoadAttempted = 0;
    borderPath = next;
    borderTransitionStart = sceKernelGetProcessTimeWide();
    borderLog("room_selected");
}

#ifdef PIZZA_NATIVE_GXM
static void drawBorderTexture(GLuint tex,int w,int h,float alpha,int opaque){
    PizzaGxm_draw(tex,0,0,(float)w,(float)h,alpha,opaque!=0);
}
#else
static void drawBorderTexture(GLuint texture, int windowW, int windowH, float alpha, int opaque) {
    (void)opaque;
    if (texture == 0 || alpha <= 0.0f) return;
    VitaGlQuad_begin(NULL, windowW, windowH);
    VitaGlQuad_drawTex(texture, 0.0f, 0.0f, (float)windowW, (float)windowH,
                       0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, alpha);
    VitaGlQuad_end(NULL);
}
#endif

void VitaBorders_draw(int windowW, int windowH) {
    if (!g_vitaConsoleBordersEnabled) return;
    if (borderPath != NULL && borderTexture == 0 && !borderLoadAttempted) loadBorderTexture();

    float progress = 1.0f;
    if (borderTransitionStart != 0) {
        uint64_t elapsed = sceKernelGetProcessTimeWide() - borderTransitionStart;
        progress = elapsed >= BORDER_TRANSITION_US ? 1.0f : (float)elapsed / (float)BORDER_TRANSITION_US;
    }
    if (previousBorderTexture != 0) {
        // True crossfade when a replacement exists; fade to black when entering
        // a menu where borders are deliberately disabled.
        drawBorderTexture(previousBorderTexture, windowW, windowH,
                          borderTexture != 0 ? 1.0f : 1.0f - progress,
                          borderTexture != 0);
    }
    if (borderTexture != 0) drawBorderTexture(borderTexture, windowW, windowH, progress, previousBorderTexture == 0 && progress >= 1.0f);

    if (progress >= 1.0f && previousBorderTexture != 0) {
        
#ifdef PIZZA_NATIVE_GXM
    PizzaGxm_delete(previousBorderTexture);
#else
    glDeleteTextures(1, &previousBorderTexture);
#endif

        previousBorderTexture = 0;
        previousBorderWidth = previousBorderHeight = 0;
        borderTransitionStart = 0;
    }
}

int VitaBorders_filesAvailable(void) {
    const char* required = chapterBorder(borderChapter);
    if (required == NULL) return 0;
    SceIoStat stat;
    return sceIoGetstat(required, &stat) >= 0 && stat.st_size > 0;
}

void VitaBorders_shutdown(void) {
    if (borderTexture != 0) 
#ifdef PIZZA_NATIVE_GXM
    PizzaGxm_delete(borderTexture);
#else
    glDeleteTextures(1, &borderTexture);
#endif

    if (previousBorderTexture != 0) 
#ifdef PIZZA_NATIVE_GXM
    PizzaGxm_delete(previousBorderTexture);
#else
    glDeleteTextures(1, &previousBorderTexture);
#endif

    borderTexture = 0;
    previousBorderTexture = 0;
}
