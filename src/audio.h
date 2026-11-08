#ifndef AUDIO_H
#define AUDIO_H
#include <orbis/libkernel.h>
#include <orbis/SceAudioOut.h>
#include <orbis/pthread.h>
#include <orbis/SceUserService.h>
#include <math.h>
#include <stdint.h>

#define AUD_LEN   1024
#define AUD_FREQ  48000
#define AUD_VOICES 12
#define AUD_PI    3.14159265f

enum { V_SQUARE, V_NOISE, V_SINE };

typedef struct { int on; int type; float f, ph, amp, dec; } Voice;

static Voice  audVoices[AUD_VOICES];
static int16_t audBuf[AUD_LEN * 2];
static int    audHandle = -1;
static volatile int audDrone;
static volatile int audMusic = 1;
static uint32_t audRng = 0x9E3779B9u;
static pthread_t audThreadId;
static float  audMusT;
static int    audMusStep;

static inline float audNoise(void){
  audRng ^= audRng << 13; audRng ^= audRng >> 17; audRng ^= audRng << 5;
  return ((float)(audRng & 0xFFFF) / 32768.0f) - 1.0f;
}

static void sfx(int type, float f, float amp, float dec){
  for(int i = 0; i < AUD_VOICES; i++){
    if(audVoices[i].on) continue;
    audVoices[i].on = 1; audVoices[i].type = type; audVoices[i].f = f;
    audVoices[i].ph = 0.0f; audVoices[i].amp = amp; audVoices[i].dec = dec;
    return;
  }
}

static const float AUD_MOTIF[8] = {220.0f,261.6f,329.6f,392.0f,329.6f,261.6f,196.0f,164.8f};

static void* audThreadFn(void* arg){
  (void)arg;
  float dronePh = 0.0f, tremPh = 0.0f;
  for(;;){
    for(int i = 0; i < AUD_LEN; i++){
      float mix = 0.0f;
      float dAmp = audDrone == 2 ? 0.085f : audDrone == 1 ? 0.050f : 0.018f;
      dronePh += 2.0f * AUD_PI * 55.0f / AUD_FREQ; if(dronePh > 2.0f*AUD_PI) dronePh -= 2.0f*AUD_PI;
      tremPh  += 2.0f * AUD_PI * 0.35f / AUD_FREQ; if(tremPh  > 2.0f*AUD_PI) tremPh  -= 2.0f*AUD_PI;
      mix += sinf(dronePh) * dAmp * (0.72f + 0.28f * sinf(tremPh));

      if(audMusic){
        audMusT += 1.0f / AUD_FREQ;
        if(audMusT > 0.42f){
          audMusT = 0.0f;
          audMusStep = (audMusStep + 1) & 7;
          sfx(V_SQUARE, AUD_MOTIF[audMusStep], audDrone ? 0.035f : 0.018f, 0.99965f);
        }
      }

      for(int v = 0; v < AUD_VOICES; v++){
        Voice* o = &audVoices[v];
        if(!o->on) continue;
        float s = (o->type == V_NOISE) ? audNoise() : sinf(o->ph);
        if(o->type == V_SQUARE) s = (s > 0.0f ? 0.62f : -0.62f);
        o->ph += 2.0f * AUD_PI * o->f / AUD_FREQ;
        if(o->ph > 2.0f*AUD_PI) o->ph -= 2.0f*AUD_PI;
        s *= o->amp;
        o->amp *= o->dec;
        if(o->amp < 0.0004f){ o->on = 0; o->amp = 0.0f; }
        mix += s;
      }

      if(mix > 1.0f) mix = 1.0f;
      if(mix < -1.0f) mix = -1.0f;
      int16_t q = (int16_t)(mix * 30000.0f);
      audBuf[i * 2] = q; audBuf[i * 2 + 1] = q;
    }
    if(audHandle >= 0) sceAudioOutOutput(audHandle, audBuf);
    else sceKernelUsleep(20000);
  }
  return NULL;
}

static void audInit(void){
  audHandle = sceAudioOutOpen(SCE_USER_SERVICE_USER_ID_SYSTEM,
                              SCE_AUDIO_OUT_PORT_TYPE_MAIN, 0,
                              AUD_LEN, AUD_FREQ,
                              SCE_AUDIO_OUT_PARAM_FORMAT_S16_STEREO);
  if(audHandle < 0) return;
  scePthreadCreate(&audThreadId, NULL, audThreadFn, NULL, "wishwood_aud");
}

#endif
