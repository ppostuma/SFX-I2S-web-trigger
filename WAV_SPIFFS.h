#ifndef __WAVFILE_H__

#include <FS.h>
#include <i2s.h>

typedef struct wavFILE_s {
    File f;
} wavFILE_t;

typedef struct wavProperties_s {
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
} wavProperties_t;

#ifdef __cplusplus
extern "C" {
#endif

void toggleParticleReportStatus();

int wavOpen(const char *wavname, wavFILE_t *, wavProperties_t *wavProps);
int wavRead(wavFILE_t *wf, void *buffer, size_t buflen);
int wavClose(wavFILE_t *wf);

bool ICACHE_FLASH_ATTR i2s_write_lr_nb(int16_t left, int16_t right);
void wav_stopPlaying();
bool wav_loop();
void wav_startPlayingFile(const char *wavfilename);

#ifdef __cplusplus
}
#endif

#endif
#define __WAVFILE_H__   1
