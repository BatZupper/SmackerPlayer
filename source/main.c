#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "smacker.h"
#include "smk2avi.h"
#include <SDL3/SDL.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define AUDIO_BUFFER_SIZE (1024*1024) // 1 MB audio buffer

// ring buffer audio
typedef struct {
    unsigned char data[AUDIO_BUFFER_SIZE];
    size_t read_pos;
    size_t write_pos;
    size_t size;
} RingBuffer;

static void rb_init(RingBuffer *rb){
    rb->read_pos=rb->write_pos=rb->size=0;
}

static size_t rb_write(RingBuffer *rb, const unsigned char *src, size_t len){
    size_t written=0;
    while(written < len && rb->size < AUDIO_BUFFER_SIZE){
        rb->data[rb->write_pos] = src[written];
        rb->write_pos = (rb->write_pos+1) % AUDIO_BUFFER_SIZE;
        rb->size++;
        written++;
    }
    return written;
}

static size_t rb_read(RingBuffer *rb, unsigned char *dst, size_t len){
    size_t read=0;
    while(read < len && rb->size > 0){
        dst[read] = rb->data[rb->read_pos];
        rb->read_pos = (rb->read_pos+1) % AUDIO_BUFFER_SIZE;
        rb->size--;
        read++;
    }
    return read;
}

typedef struct {
    RingBuffer *rb;
} AudioState;

static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    AudioState* state = (AudioState*)pDevice->pUserData;
    if(state == NULL) return;

    ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
    ma_uint32 bytesToWrite  = frameCount * bytesPerFrame;
    unsigned char *out = (unsigned char*)pOutput;

    size_t got = rb_read(state->rb, out, bytesToWrite);
    if(got < bytesToWrite){
        // empty buffer -> complete with silence
        memset(out+got, 0, bytesToWrite-got);
    }

    (void)pInput;
}

int main(int argc, char **argv){
    if(argc<2){
        fprintf(stderr,"Usage: %s file.smk\n",argv[0]);
        return 1;
    }

    smk movie = smk_open_file(argv[1], SMK_MODE_DISK);
    if(!movie){ fprintf(stderr,"Cannot open %s\n",argv[1]); return 1; }

    unsigned long w=0,h=0;
    smk_info_video(movie,&w,&h,NULL);

    unsigned long cur=0, frames=0;
    double usf=0.0;
    smk_info_all(movie,&cur,&frames,&usf);

    // audio info
    unsigned char track_mask=0;
    unsigned char channels[7]={0};
    unsigned char bitdepth[7]={0};
    unsigned long rate[7]={0};
    smk_info_audio(movie,&track_mask,channels,bitdepth,rate);

    int track=-1;
    for(int i=0;i<7;i++){
        if(track_mask & (1<<i)){ track=i; break; }
    }
    if(track<0){
        fprintf(stderr,"No audio track\n");
        smk_close(movie);
        return 1;
    }
    printf("Audio: %lu Hz, %u ch, %u bits\n", rate[track], channels[track], bitdepth[track]);

    smk_enable_video(movie,1);
    smk_enable_audio(movie,track,1);
    smk_first(movie);

    // init SDL video
    if(SDL_Init(SDL_INIT_VIDEO)!= true){
        fprintf(stderr,"SDL init error: %s\n",SDL_GetError());
        smk_close(movie);
        return 1;
    }
    SDL_Window *win=SDL_CreateWindow("Smacker Player",(int)w,(int)h,0);
    SDL_Renderer *ren=SDL_CreateRenderer(win,NULL);
    SDL_Texture *tex=SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, (int)w,(int)h);

    // init audio
    RingBuffer rb;
    rb_init(&rb);
    AudioState state={.rb=&rb};

    ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = (bitdepth[track]==16)? ma_format_s16 : ma_format_u8;
    cfg.playback.channels = channels[track];
    cfg.sampleRate        = rate[track];
    cfg.dataCallback      = data_callback;
    cfg.pUserData         = &state;

    ma_device device;
    if(ma_device_init(NULL,&cfg,&device)!=MA_SUCCESS){
        fprintf(stderr,"Failed to init audio device\n");
        smk_close(movie);
        return 1;
    }
    ma_device_start(&device);

    size_t frame_pixels=w*h;
    uint8_t *rgb_buf=malloc(frame_pixels*3);

    double ms_per_frame = (usf>0.0)? usf/1000.0 : 40.0;

    int running=1;
    while(running){
        // events
        SDL_Event ev;
        while(SDL_PollEvent(&ev)){
            if(ev.type==SDL_EVENT_QUIT) running=0;
            if(ev.type==SDL_EVENT_KEY_DOWN){
                if(ev.key.key==SDLK_ESCAPE || ev.key.key==SDLK_Q) running=0;
                else if(ev.key.key ==SDLK_E) ConvertToAvi(argv[1]);
            }
        }

        // video
        const unsigned char *palette=smk_get_palette(movie);
        const unsigned char *frame_buf=smk_get_video(movie);
        if(frame_buf && palette){
            for(unsigned long i=0;i<frame_pixels;i++){
                unsigned char idx=frame_buf[i];
                rgb_buf[i*3+0]=palette[idx*3+0];
                rgb_buf[i*3+1]=palette[idx*3+1];
                rgb_buf[i*3+2]=palette[idx*3+2];
            }
            SDL_UpdateTexture(tex,NULL,rgb_buf,(int)(w*3));
            SDL_RenderClear(ren);
            SDL_RenderTexture(ren,tex,NULL,NULL);
            SDL_RenderPresent(ren);
        }

        // queue audio in the ring buffer
        const unsigned char *chunk = smk_get_audio(movie,track);
        unsigned long size = smk_get_audio_size(movie,track);
        if(chunk && size>0){
            rb_write(&rb, chunk, size);
        }

        char res=smk_next(movie);
        if(res==SMK_DONE || res==SMK_ERROR) running=0;

        SDL_Delay((Uint32)ms_per_frame);
    }

    free(rgb_buf);
    ma_device_uninit(&device);
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    smk_close(movie);
    return 0;
}