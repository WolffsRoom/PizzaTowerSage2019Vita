#ifndef PIZZA_NATIVE_PAUSE_DRAW_H
#define PIZZA_NATIVE_PAUSE_DRAW_H
#include "renderer.h"
#include "pizza_native_pause.h"
#include <stdio.h>
/* Port-owned text is drawn from primitive pixels: no game font or TXTR needed. */
static inline void pauseText(Renderer* r, const char* text, float x, float y, uint32_t color) {
  static const uint8_t letters[26][7] = {
    {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
    {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
    {14,17,16,23,17,17,14},{17,17,17,31,17,17,17},{31,4,4,4,4,4,31},
    {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
  };
  static const uint8_t digits[10][7] = {
    {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
    {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
    {14,17,17,15,1,1,14}
  };
  for (; *text; ++text, x+=18) {
    const uint8_t* glyph = *text>='A' && *text<='Z' ? letters[*text-'A'] :
                            *text>='0' && *text<='9' ? digits[*text-'0'] : NULL;
    if (!glyph) continue;
    for (int row=0; row<7; ++row) for (int col=0; col<5; ++col)
      if (glyph[row] & (1<<(4-col))) r->vtable->drawRectangle(r,x+col*3,y+row*3,
          x+col*3+3,y+row*3+3,color,1,false);
  }
}

static inline void PizzaNativePause_draw(const PizzaNativePause* p, int musicVolume, int sfxVolume, Renderer* r) {
  if (!p->open) return;
  Matrix4f savedWorld=r->gmlMatrices[MATRIX_WORLD], identity;
  Matrix4f_identity(&identity);
  r->vtable->beginGUI(r,960,544,0,0,960,544,RENDER_TARGET_HOST_FRAMEBUFFER);
  r->vtable->setMatrix(r,MATRIX_WORLD,identity);
  r->vtable->gpuResetShader(r);
  r->vtable->gpuSetBlendEnable(r,true);
  r->vtable->gpuSetBlendMode(r,0);
  r->vtable->gpuSetAlphaTestEnable(r,false);
  r->vtable->gpuSetColorWriteEnable(r,true,true,true,true);
  r->vtable->gpuSetFog(r,false,0);
  r->vtable->drawRectangle(r,0,0,960,544,0x000000,0.70f,false);
  r->vtable->drawRectangle(r,140,74,820,470,0x000000,1,false);
  r->vtable->drawRectangle(r,140,74,820,470,0xFFFFFF,1,true);
  pauseText(r,"PIZZA TOWER - PAUSA",235,105,0xFFFFFF);
  if (p->confirmExit) {
    pauseText(r,"SAIR DO JOGO",325,205,0xFFFFFF);
    pauseText(r,"X CONFIRMA",350,270,0xFFFFFF);
    pauseText(r,"CIRCULO CANCELA",305,320,0xFFFFFF);
  } else {
    char music[32],sfx[32];
    snprintf(music,sizeof(music),"MUSICA %d",musicVolume);
    snprintf(sfx,sizeof(sfx),"EFEITOS %d",sfxVolume);
    const char* rows[4]={"CONTINUAR",music,sfx,"SAIR DO JOGO"};
    for (unsigned i=0;i<4;++i) {
      if (i==p->selected) r->vtable->drawRectangle(r,175,166+i*56,785,209+i*56,0x393939,1,false);
      pauseText(r,rows[i],210,178+i*56,0xFFFFFF);
    }
    pauseText(r,"SELECT VOLTAR - X OK",238,411,0xFFFFFF);
    pauseText(r,"ESQ DIR VOLUME",274,441,0xFFFFFF);
  }
  r->vtable->endGUI(r);
  r->vtable->setMatrix(r,MATRIX_WORLD,savedWorld);
}

#endif
