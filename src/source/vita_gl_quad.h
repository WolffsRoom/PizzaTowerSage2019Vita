#ifndef VITA_GL_QUAD_H
#define VITA_GL_QUAD_H

#include <stdbool.h>
#include <stdint.h>
#include "renderer.h"

#ifdef PIZZA_NATIVE_GXM
typedef unsigned VitaGlTex;
#ifndef GL_NEAREST
#define GL_NEAREST 0x2600
#endif
#ifndef GL_LINEAR
#define GL_LINEAR 0x2601
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GLint
typedef int GLint;
#endif
#else
#include <vitaGL.h>
typedef GLuint VitaGlTex;
#endif

bool VitaGlQuad_init(void);
void VitaGlQuad_shutdown(void);

VitaGlTex VitaGlQuad_loadPng(const char* path, int* outW, int* outH,
                             bool rgba4444, GLint filter, GLint wrap);
void VitaGlQuad_deleteTex(VitaGlTex* tex);

void VitaGlQuad_begin(Renderer* renderer, int fbW, int fbH);
void VitaGlQuad_end(Renderer* renderer);

void VitaGlQuad_drawTex(VitaGlTex tex,
                        float x, float y, float w, float h,
                        float u0, float v0, float u1, float v1,
                        float r, float g, float b, float a);
void VitaGlQuad_drawRect(float x, float y, float w, float h,
                         float r, float g, float b, float a);
void VitaGlQuad_drawOutline(float x, float y, float w, float h, float thickness,
                            float r, float g, float b, float a);

#endif

