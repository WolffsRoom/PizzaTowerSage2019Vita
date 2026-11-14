/* Bounded detached file reads. No DataWin, VM or GPU access on the worker. */
#include "pizza_texture_stream.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef __vita__
#include "vita_thread_policy.h"
#endif
#define STREAM_SLOTS 16
#define STREAM_PATH 256
enum { EMPTY, QUEUED, READING, READY };
typedef struct {
    char path[STREAM_PATH]; uint32_t generation;
    size_t bytes; void* data; int state; bool cancelled;
} StreamJob;
struct PizzaTextureStream {
    pthread_mutex_t mutex; pthread_cond_t wake;
    pthread_t thread;
    bool stopping,unlocked,workerReady; size_t budget,reserved;
    StreamJob jobs[STREAM_SLOTS];
};
static void releaseJob(PizzaTextureStream* s,StreamJob* j) {
    s->reserved-=j->bytes; free(j->data); memset(j,0,sizeof(*j));
}
static void* streamWorker(void* arg) {
    PizzaTextureStream* s=arg;
    pthread_mutex_lock(&s->mutex);
#ifdef __vita__
    /* pthread creation installs the TLS/cancellation state used by cond_wait. */
    s->unlocked=VitaThread_applyBackground();
#endif
    s->workerReady=true;
    pthread_cond_broadcast(&s->wake);
    while(!s->stopping) {
        StreamJob* j=NULL;
        for(int i=0;i<STREAM_SLOTS;i++) if(s->jobs[i].state==QUEUED) {j=&s->jobs[i];break;}
        if(!j) { pthread_cond_wait(&s->wake,&s->mutex); continue; }
        j->state=READING;
        pthread_mutex_unlock(&s->mutex);
        void* data=malloc(j->bytes);
        FILE* f=data?fopen(j->path,"rb"):NULL;
        if(f)setvbuf(f,NULL,_IOFBF,64U*1024U);
        bool ok=f && fread(data,1,j->bytes,f)==j->bytes && fgetc(f)==EOF && !ferror(f);
        if(f)fclose(f);
        if(!ok){free(data);data=NULL;}
        pthread_mutex_lock(&s->mutex);
        j->data=data;
        if(s->stopping||j->cancelled)releaseJob(s,j);
        else j->state=READY; /* failed reads publish NULL, consumed as fallback */
        pthread_cond_broadcast(&s->wake);
    }
    pthread_mutex_unlock(&s->mutex);
    return NULL;
}
PizzaTextureStream* PizzaTextureStream_create(size_t budget) {
    PizzaTextureStream* s=calloc(1,sizeof(*s)); if(!s)return NULL;
    s->budget=budget?budget:8U*1024U*1024U;
    if(pthread_mutex_init(&s->mutex,NULL)!=0){free(s);return NULL;}
    if(pthread_cond_init(&s->wake,NULL)!=0){pthread_mutex_destroy(&s->mutex);free(s);return NULL;}
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr,128*1024);
    bool started=pthread_create(&s->thread,&attr,streamWorker,s)==0;
    pthread_attr_destroy(&attr);
    if(!started){pthread_cond_destroy(&s->wake);pthread_mutex_destroy(&s->mutex);free(s);return NULL;}
    pthread_mutex_lock(&s->mutex);
    while(!s->workerReady)pthread_cond_wait(&s->wake,&s->mutex);
    pthread_mutex_unlock(&s->mutex);
    return s;
}
bool PizzaTextureStream_request(PizzaTextureStream* s,const char* path,uint32_t gen) {
    if(!s||!path||strlen(path)>=STREAM_PATH)return false;
    struct stat st;
    if(stat(path,&st)!=0||st.st_size<=0||(uint64_t)st.st_size>s->budget)return false;
    pthread_mutex_lock(&s->mutex);
    StreamJob* slot=NULL;
    for(int i=0;i<STREAM_SLOTS;i++){
        StreamJob* j=&s->jobs[i];
        if(j->state!=EMPTY && j->generation==gen && !strcmp(j->path,path)){
            pthread_mutex_unlock(&s->mutex);return false;
        }
        if(j->state==EMPTY&&!slot)slot=j;
    }
    size_t bytes=(size_t)st.st_size;
    if(!slot||s->stopping||bytes>s->budget-s->reserved){pthread_mutex_unlock(&s->mutex);return false;}
    strcpy(slot->path,path);slot->bytes=bytes;slot->generation=gen;slot->state=QUEUED;
    s->reserved+=bytes;pthread_cond_signal(&s->wake);pthread_mutex_unlock(&s->mutex);return true;
}
bool PizzaTextureStream_take(PizzaTextureStream* s,const char* path,uint32_t gen,PizzaTextureBuffer* out) {
    if(!s||!path||!out)return false;
    pthread_mutex_lock(&s->mutex);
    for(int i=0;i<STREAM_SLOTS;i++){
        StreamJob* j=&s->jobs[i];
        if(j->state==READY&&!j->cancelled&&j->generation==gen&&!strcmp(path,j->path)){
            out->data=j->data;out->size=j->data?j->bytes:0;j->data=NULL;
            releaseJob(s,j);pthread_mutex_unlock(&s->mutex);return true;
        }
    }
    pthread_mutex_unlock(&s->mutex);return false;
}
void PizzaTextureStream_cancel(PizzaTextureStream* s,uint32_t gen) {
    if(!s)return;
    pthread_mutex_lock(&s->mutex);
    for(int i=0;i<STREAM_SLOTS;i++)if(s->jobs[i].state!=EMPTY&&s->jobs[i].generation==gen){
        if(s->jobs[i].state==READING)s->jobs[i].cancelled=true;
        else releaseJob(s,&s->jobs[i]);
    }
    pthread_mutex_unlock(&s->mutex);
}
bool PizzaTextureStream_waitTake(PizzaTextureStream*s,const char*path,uint32_t gen,PizzaTextureBuffer*out){
    if(!s||!path||!out)return false;
    pthread_mutex_lock(&s->mutex);
    for(;;){
        StreamJob*found=NULL;
        for(int i=0;i<STREAM_SLOTS;i++){
            StreamJob*j=&s->jobs[i];
            if(j->state!=EMPTY&&!j->cancelled&&j->generation==gen&&!strcmp(j->path,path)){found=j;break;}
        }
        if(!found||s->stopping){pthread_mutex_unlock(&s->mutex);return false;}
        if(found->state==READY){
            out->data=found->data;out->size=found->data?found->bytes:0;found->data=NULL;
            releaseJob(s,found);pthread_mutex_unlock(&s->mutex);return true;
        }
        pthread_cond_wait(&s->wake,&s->mutex);
    }
}
void PizzaTextureStream_cancelAndWait(PizzaTextureStream*s,uint32_t gen){
    if(!s)return;
    PizzaTextureStream_cancel(s,gen);
    pthread_mutex_lock(&s->mutex);
    for(;;){
        bool running=false;
        for(int i=0;i<STREAM_SLOTS;i++)if(s->jobs[i].state==READING&&s->jobs[i].generation==gen)running=true;
        if(!running)break;
        pthread_cond_wait(&s->wake,&s->mutex);
    }
    pthread_mutex_unlock(&s->mutex);
}
void PizzaTextureStream_forget(PizzaTextureStream* s,const char* path,uint32_t gen){
    if(!s||!path)return;
    pthread_mutex_lock(&s->mutex);
    for(int i=0;i<STREAM_SLOTS;i++){
        StreamJob*j=&s->jobs[i];
        if(j->state==EMPTY||j->generation!=gen||strcmp(j->path,path))continue;
        if(j->state==READING)j->cancelled=true;
        else releaseJob(s,j);
    }
    pthread_mutex_unlock(&s->mutex);
}
void PizzaTextureStream_drain(PizzaTextureStream*s){
    if(!s)return;
    pthread_mutex_lock(&s->mutex);
    for(int i=0;i<STREAM_SLOTS;i++){
        StreamJob*j=&s->jobs[i];
        if(j->state==READING)j->cancelled=true;
        else if(j->state!=EMPTY)releaseJob(s,j);
    }
    for(;;){
        bool reading=false;
        for(int i=0;i<STREAM_SLOTS;i++)if(s->jobs[i].state==READING)reading=true;
        if(!reading)break;
        pthread_cond_wait(&s->wake,&s->mutex);
    }
    pthread_mutex_unlock(&s->mutex);
}
size_t PizzaTextureStream_reserved(PizzaTextureStream* s){
    if(!s)return 0;
    pthread_mutex_lock(&s->mutex);
    size_t n=s->reserved;
    pthread_mutex_unlock(&s->mutex);
    return n;
}
bool PizzaTextureStream_unlocked(PizzaTextureStream* s){return s&&s->unlocked;}
void PizzaTextureStream_destroy(PizzaTextureStream* s){
    if(!s)return;
    pthread_mutex_lock(&s->mutex);s->stopping=true;pthread_cond_signal(&s->wake);pthread_mutex_unlock(&s->mutex);
    pthread_join(s->thread,NULL);
    for(int i=0;i<STREAM_SLOTS;i++)if(s->jobs[i].state!=EMPTY)releaseJob(s,&s->jobs[i]);
    pthread_cond_destroy(&s->wake);pthread_mutex_destroy(&s->mutex);free(s);
}
