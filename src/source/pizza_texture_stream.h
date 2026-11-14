#ifndef PIZZA_TEXTURE_STREAM_H
#define PIZZA_TEXTURE_STREAM_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct PizzaTextureStream PizzaTextureStream;
typedef struct PizzaTextureBuffer { void* data; size_t size; } PizzaTextureBuffer;
PizzaTextureStream* PizzaTextureStream_create(size_t budget);
void PizzaTextureStream_destroy(PizzaTextureStream*);
bool PizzaTextureStream_request(PizzaTextureStream*,const char* path,uint32_t generation);
bool PizzaTextureStream_take(PizzaTextureStream*,const char* path,uint32_t generation,PizzaTextureBuffer*);
bool PizzaTextureStream_waitTake(PizzaTextureStream*,const char* path,uint32_t generation,PizzaTextureBuffer*);
void PizzaTextureStream_cancelAndWait(PizzaTextureStream*,uint32_t generation);
void PizzaTextureStream_drain(PizzaTextureStream*);
void PizzaTextureStream_cancel(PizzaTextureStream*,uint32_t generation);
void PizzaTextureStream_forget(PizzaTextureStream*,const char* path,uint32_t generation);
size_t PizzaTextureStream_reserved(PizzaTextureStream*);
bool PizzaTextureStream_unlocked(PizzaTextureStream*);
#endif
