#include "vita_gl_quad.h"

#include <psp2/io/fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef PIZZA_MODERN_GL
#include "gl_renderer.h"
#elif !defined(PIZZA_NATIVE_GXM)
#include "gl_legacy_renderer.h"
#endif

#include "stb_image.h"

extern void PizzaVita_logLine(const char* text);

#ifndef PIZZA_NATIVE_GXM

static GLuint g_quadProgram = 0;
static GLuint g_whiteTex = 0;
static GLint g_uInvHalfScreen = -1;
static GLint g_uTex = -1;
static GLint g_uColor = -1;
static bool g_quadFailed = false;
static bool g_inBegin = false;

static struct {
    GLint program;
    GLint vbo;
    GLint ebo;
    GLint fbo;
    GLint viewport[4];
    GLint activeTex;
    GLint tex0;
    GLint eqRgb;
    GLint eqAlpha;
    GLint attrib0Enabled;
    GLint attrib1Enabled;
    GLboolean mask[4];
    GLboolean blend;
    GLboolean depth;
    GLboolean scissor;
    GLboolean stencil;
    GLboolean cull;
} g_savedState;

static GLuint compileQuadShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    if (!shader) return 0;
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char logBuf[256] = {0};
        glGetShaderInfoLog(shader, (GLsizei)sizeof(logBuf) - 1, NULL, logBuf);
        PizzaVita_logLine(logBuf[0] ? logBuf : "VITAGL_QUAD=shader_compile_failed");
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool VitaGlQuad_init(void) {
    if (g_quadProgram != 0 && g_whiteTex != 0) return true;
    if (g_quadFailed) return false;

    /* Two client-side attributes (pixel position + UV), the same pattern as
     * the CRT overlay. A single-attribute VBO-fed program crashed the
     * Vita3K GXM backend right after the first swap. */
    static const char* kVertSrc =
        "attribute vec2 a_pos;\n"
        "attribute vec2 a_uv;\n"
        "uniform vec2 u_invHalfScreen;\n"
        "varying vec2 v_uv;\n"
        "void main() {\n"
        "    vec2 ndc = vec2(a_pos.x * u_invHalfScreen.x - 1.0, 1.0 - a_pos.y * u_invHalfScreen.y);\n"
        "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
        "    v_uv = a_uv;\n"
        "}\n";

    static const char* kFragSrc =
        "precision mediump float;\n"
        "varying vec2 v_uv;\n"
        "uniform sampler2D u_tex;\n"
        "uniform vec4 u_color;\n"
        "void main() {\n"
        "    gl_FragColor = texture2D(u_tex, v_uv) * u_color;\n"
        "}\n";

    GLuint vs = compileQuadShader(GL_VERTEX_SHADER, kVertSrc);
    GLuint fs = vs ? compileQuadShader(GL_FRAGMENT_SHADER, kFragSrc) : 0;
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        g_quadFailed = true;
        PizzaVita_logLine("VITAGL_QUAD=compile_failed");
        return false;
    }

    GLuint prog = glCreateProgram();
    if (!prog) {
        glDeleteShader(vs);
        glDeleteShader(fs);
        g_quadFailed = true;
        return false;
    }
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "a_pos");
    glBindAttribLocation(prog, 1, "a_uv");
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!ok) {
        char logBuf[256] = {0};
        glGetProgramInfoLog(prog, (GLsizei)sizeof(logBuf) - 1, NULL, logBuf);
        PizzaVita_logLine(logBuf[0] ? logBuf : "VITAGL_QUAD=link_failed");
        glDeleteProgram(prog);
        g_quadFailed = true;
        return false;
    }

    g_quadProgram = prog;
    g_uInvHalfScreen = glGetUniformLocation(prog, "u_invHalfScreen");
    g_uTex = glGetUniformLocation(prog, "u_tex");
    g_uColor = glGetUniformLocation(prog, "u_color");

    GLint oldActiveTex = GL_TEXTURE0, oldTex0 = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &oldActiveTex);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTex0);

    static const uint8_t kWhitePx[4] = {255, 255, 255, 255};
    glGenTextures(1, &g_whiteTex);
    glBindTexture(GL_TEXTURE_2D, g_whiteTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, kWhitePx);

    glBindTexture(GL_TEXTURE_2D, (GLuint)oldTex0);
    glActiveTexture((GLenum)oldActiveTex);
    PizzaVita_logLine("VITAGL_QUAD=ready_gles2");
    return true;
}

void VitaGlQuad_shutdown(void) {
    if (g_whiteTex != 0) {
        glDeleteTextures(1, &g_whiteTex);
        g_whiteTex = 0;
    }
    if (g_quadProgram != 0) {
        glDeleteProgram(g_quadProgram);
        g_quadProgram = 0;
    }
    g_quadFailed = false;
    g_inBegin = false;
}

VitaGlTex VitaGlQuad_loadPng(const char* path, int* outW, int* outH,
                             bool rgba4444, GLint filter, GLint wrap) {
    if (outW) *outW = 0;
    if (outH) *outH = 0;
    if (!path) return 0;

    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
    if (fd < 0) return 0;
    SceOff size = sceIoLseek(fd, 0, SCE_SEEK_END);
    sceIoLseek(fd, 0, SCE_SEEK_SET);
    if (size <= 0) {
        sceIoClose(fd);
        return 0;
    }
    unsigned char* data = (unsigned char*)malloc((size_t)size);
    if (!data) {
        sceIoClose(fd);
        return 0;
    }
    int got = sceIoRead(fd, data, (unsigned int)size);
    sceIoClose(fd);
    if (got != (int)size) {
        free(data);
        return 0;
    }

    int w = 0, h = 0, ch = 0;
    unsigned char* pixels = stbi_load_from_memory(data, got, &w, &h, &ch, 4);
    free(data);
    if (!pixels || w <= 0 || h <= 0) {
        stbi_image_free(pixels);
        return 0;
    }

    GLint oldActiveTex = GL_TEXTURE0, oldTex0 = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &oldActiveTex);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTex0);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

    while (glGetError() != GL_NO_ERROR) {}
    if (rgba4444) {
        size_t n = (size_t)w * (size_t)h;
        uint16_t* packed = (uint16_t*)pixels;
        for (size_t i = 0; i < n; ++i) {
            const uint8_t* s = pixels + i * 4U;
            uint16_t a = s[3] == 0 ? 0 : (uint16_t)((s[3] + 15U) >> 4);
            if (a > 15U) a = 15U;
            packed[i] = (uint16_t)(((uint16_t)(s[0] >> 4) << 12) |
                                   ((uint16_t)(s[1] >> 4) << 8) |
                                   ((uint16_t)(s[2] >> 4) << 4) | a);
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                     GL_UNSIGNED_SHORT_4_4_4_4, packed);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, pixels);
    }
    GLenum err = glGetError();
    stbi_image_free(pixels);

    glBindTexture(GL_TEXTURE_2D, (GLuint)oldTex0);
    glActiveTexture((GLenum)oldActiveTex);

    if (err != GL_NO_ERROR) {
        glDeleteTextures(1, &tex);
        return 0;
    }

    if (outW) *outW = w;
    if (outH) *outH = h;
    return tex;
}

void VitaGlQuad_deleteTex(VitaGlTex* tex) {
    if (tex && *tex != 0) {
        glDeleteTextures(1, tex);
        *tex = 0;
    }
}

void VitaGlQuad_begin(Renderer* renderer, int fbW, int fbH) {
    if (g_inBegin) return;
    if (!VitaGlQuad_init()) return;
    if (renderer != NULL && renderer->vtable != NULL && renderer->vtable->flush != NULL) {
        renderer->vtable->flush(renderer);
    }

    g_savedState.blend = glIsEnabled(GL_BLEND);
    g_savedState.depth = glIsEnabled(GL_DEPTH_TEST);
    g_savedState.scissor = glIsEnabled(GL_SCISSOR_TEST);
    g_savedState.stencil = glIsEnabled(GL_STENCIL_TEST);
    g_savedState.cull = glIsEnabled(GL_CULL_FACE);
    glGetIntegerv(GL_CURRENT_PROGRAM, &g_savedState.program);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &g_savedState.vbo);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &g_savedState.ebo);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &g_savedState.fbo);
    glGetIntegerv(GL_VIEWPORT, g_savedState.viewport);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &g_savedState.activeTex);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &g_savedState.tex0);
    glGetIntegerv(GL_BLEND_EQUATION, &g_savedState.eqRgb);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &g_savedState.eqAlpha);
    glGetBooleanv(GL_COLOR_WRITEMASK, g_savedState.mask);
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &g_savedState.attrib0Enabled);
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &g_savedState.attrib1Enabled);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbW, fbH);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

#ifdef __vita__
    glBindVertexArray(0);
#endif
    glUseProgram(g_quadProgram);
    if (g_uInvHalfScreen >= 0) {
        glUniform2f(g_uInvHalfScreen,
                    fbW > 0 ? 2.0f / (float)fbW : (2.0f / 960.0f),
                    fbH > 0 ? 2.0f / (float)fbH : (2.0f / 544.0f));
    }
    if (g_uTex >= 0) glUniform1i(g_uTex, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    g_inBegin = true;
}

void VitaGlQuad_end(Renderer* renderer) {
    if (!g_inBegin) return;
    g_inBegin = false;

    if (!g_savedState.attrib1Enabled) glDisableVertexAttribArray(1);
    if (!g_savedState.attrib0Enabled) glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)g_savedState.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)g_savedState.ebo);
    glUseProgram((GLuint)g_savedState.program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (GLuint)g_savedState.tex0);
    glActiveTexture((GLenum)g_savedState.activeTex);
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)g_savedState.fbo);
    glViewport(g_savedState.viewport[0], g_savedState.viewport[1],
               g_savedState.viewport[2], g_savedState.viewport[3]);
    glBlendEquationSeparate((GLenum)g_savedState.eqRgb, (GLenum)g_savedState.eqAlpha);

    if (renderer != NULL && renderer->vtable != NULL &&
        renderer->vtable->gpuGetBlendMode != NULL) {
        int32_t blendMode = renderer->vtable->gpuGetBlendMode(renderer);
#if defined(PIZZA_MODERN_GL)
        if (blendMode == bm_complex) {
            GLRenderer* gl = (GLRenderer*)renderer;
            renderer->vtable->gpuSetBlendModeExt(renderer, gl->currentSFactor,
                gl->currentDFactor, gl->currentSFactorAlpha, gl->currentDFactorAlpha);
        } else {
            renderer->vtable->gpuSetBlendMode(renderer, blendMode);
        }
#else
        if (blendMode == bm_complex) {
            GLLegacyRenderer* legacy = (GLLegacyRenderer*)renderer;
            renderer->vtable->gpuSetBlendModeExt(renderer, legacy->currentSFactor,
                legacy->currentDFactor, legacy->currentSFactorAlpha, legacy->currentDFactorAlpha);
        } else {
            renderer->vtable->gpuSetBlendMode(renderer, blendMode);
        }
#endif
    }
    glBlendEquationSeparate((GLenum)g_savedState.eqRgb, (GLenum)g_savedState.eqAlpha);
    glColorMask(g_savedState.mask[0], g_savedState.mask[1],
                g_savedState.mask[2], g_savedState.mask[3]);
    if (!g_savedState.blend) glDisable(GL_BLEND);
    else glEnable(GL_BLEND);
    if (g_savedState.depth) glEnable(GL_DEPTH_TEST);
    if (g_savedState.scissor) glEnable(GL_SCISSOR_TEST);
    if (g_savedState.stencil) glEnable(GL_STENCIL_TEST);
    if (g_savedState.cull) glEnable(GL_CULL_FACE);
}

void VitaGlQuad_drawTex(VitaGlTex tex,
                        float x, float y, float w, float h,
                        float u0, float v0, float u1, float v1,
                        float r, float g, float b, float a) {
    if (tex == 0 || a <= 0.0f) return;
    bool autoWrap = !g_inBegin;
    if (autoWrap) VitaGlQuad_begin(NULL, 960, 544);
    if (!g_inBegin) return;

    glBindTexture(GL_TEXTURE_2D, tex);
    if (g_uColor >= 0) glUniform4f(g_uColor, r, g, b, a);
    /* vitaGL copies client arrays into per-frame GPU memory at draw time,
     * so a stack array is safe here. Layout: x, y, u, v. */
    const GLfloat verts[16] = {
        x,     y,     u0, v0,
        x + w, y,     u1, v0,
        x,     y + h, u0, v1,
        x + w, y + h, u1, v1
    };
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * (GLsizei)sizeof(GLfloat), verts);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * (GLsizei)sizeof(GLfloat), verts + 2);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (autoWrap) VitaGlQuad_end(NULL);
}

void VitaGlQuad_drawRect(float x, float y, float w, float h,
                         float r, float g, float b, float a) {
    if (w <= 0.0f || h <= 0.0f || a <= 0.0f) return;
    VitaGlQuad_drawTex(g_whiteTex, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, r, g, b, a);
}

void VitaGlQuad_drawOutline(float x, float y, float w, float h, float t,
                            float r, float g, float b, float a) {
    if (t <= 0.0f || w <= 0.0f || h <= 0.0f || a <= 0.0f) return;
    VitaGlQuad_drawRect(x, y, w, t, r, g, b, a);
    VitaGlQuad_drawRect(x, y + h - t, w, t, r, g, b, a);
    VitaGlQuad_drawRect(x, y + t, t, h - 2.0f * t, r, g, b, a);
    VitaGlQuad_drawRect(x + w - t, y + t, t, h - 2.0f * t, r, g, b, a);
}

#else

bool VitaGlQuad_init(void) { return false; }
void VitaGlQuad_shutdown(void) {}
VitaGlTex VitaGlQuad_loadPng(const char* path, int* outW, int* outH,
                             bool rgba4444, GLint filter, GLint wrap) {
    (void)path; (void)outW; (void)outH; (void)rgba4444; (void)filter; (void)wrap;
    return 0;
}
void VitaGlQuad_deleteTex(VitaGlTex* tex) { if (tex) *tex = 0; }
void VitaGlQuad_begin(Renderer* renderer, int fbW, int fbH) { (void)renderer; (void)fbW; (void)fbH; }
void VitaGlQuad_end(Renderer* renderer) { (void)renderer; }
void VitaGlQuad_drawTex(VitaGlTex tex, float x, float y, float w, float h,
                        float u0, float v0, float u1, float v1,
                        float r, float g, float b, float a) {
    (void)tex; (void)x; (void)y; (void)w; (void)h;
    (void)u0; (void)v0; (void)u1; (void)v1;
    (void)r; (void)g; (void)b; (void)a;
}
void VitaGlQuad_drawRect(float x, float y, float w, float h, float r, float g, float b, float a) {
    (void)x; (void)y; (void)w; (void)h; (void)r; (void)g; (void)b; (void)a;
}
void VitaGlQuad_drawOutline(float x, float y, float w, float h, float thickness, float r, float g, float b, float a) {
    (void)x; (void)y; (void)w; (void)h; (void)thickness; (void)r; (void)g; (void)b; (void)a;
}

#endif

