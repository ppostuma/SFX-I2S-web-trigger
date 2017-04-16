/*****************************************************************************
  The MIT License (MIT)

  Copyright (c) 2016 by bbx10node@gmail.com
  Modified by postuma for the Digistump Oak and the Particle IoT platform

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 **************************************************************************/

/*
 * Read WAV/RIFF audio files. Parses header and returns properties. This is
 * mostly designed for uncompressed PCM audio. The WAV files must be stored
 * in the ESP8266 SPIFFS file system.
 */

#include "WAV_SPIFFS.h"

#define CCCC(c1, c2, c3, c4)    ((c4 << 24) | (c3 << 16) | (c2 << 8) | c1)

bool particleReportStatus = 0;       //FALSE: report errors only. TRUE: verbose reporting

void toggleParticleReportStatus() {
     particleReportStatus = (particleReportStatus) ? 0 : 1;
}

static int readuint32(wavFILE_t *wf, uint32_t *ui32)
{
    int rc;

    rc = wf->f.read((uint8_t *)ui32, sizeof(*ui32));
    //Particle.printf("readuint32 rc=%d val=0x%X\r\n", rc, *ui32);
    return rc;
}

int wavOpen(const char *wavname, wavFILE_t *wf, wavProperties_t *wavProps)
{
    typedef enum headerState_e {
        HEADER_INIT, HEADER_RIFF, HEADER_FMT, HEADER_DATA
    } headerState_t;
    headerState_t state = HEADER_INIT;
    uint32_t chunkID, chunkSize;
    int retval;

    wf->f = SPIFFS.open(wavname, "r");
    if (!wf->f) {
        retval = -1;
        goto closeExit;
    }
    Particle.println("SPIFFS.open ok");

    while (state != HEADER_DATA) {
        if (readuint32(wf, &chunkID) != 4) {
            retval = -2;
            goto closeExit;
        }
        if (readuint32(wf, &chunkSize) != 4) {
            retval = -3;
            goto closeExit;
        }
        switch (chunkID) {
            case CCCC('R', 'I', 'F', 'F'):
                if (readuint32(wf, &chunkID) != 4) {
                    retval = -4;
                    goto closeExit;
                }
                if (chunkID != CCCC('W', 'A', 'V', 'E')) {
                    retval = -5;
                    goto closeExit;
                }
                state = HEADER_RIFF;
                Particle.printf("RIFF %d\r\n", chunkSize);
                break;

            case CCCC('f', 'm', 't', ' '):
                if (wf->f.read((uint8_t *)wavProps, sizeof(*wavProps)) != sizeof(*wavProps)) {
                    retval = -6;
                    goto closeExit;
                }
                state = HEADER_FMT;
                Serial.printf("fmt  %d\r\n", chunkSize);
                if (chunkSize > sizeof(*wavProps)) {
                    wf->f.seek(chunkSize - sizeof(*wavProps), SeekCur);
                }
                break;

            case CCCC('d', 'a', 't', 'a'):
                state = HEADER_DATA;
                Serial.printf("data %d\r\n", chunkSize);
                break;
            default:
                Serial.printf("%08X %d\r\n", chunkID, chunkSize);
                if (!wf->f.seek(chunkSize, SeekCur)) {
                    retval = -7;
                    goto closeExit;
                }
        }
    }
    if (state == HEADER_DATA) return 0;
    retval = -8;
    closeExit:
    wf->f.close();
    return retval;
}

int wavRead(wavFILE_t *wf, void *buffer, size_t buflen)
{
    return wf->f.read((uint8_t *)buffer, buflen);
}

int wavClose(wavFILE_t *wf)
{
    wf->f.close();
    return 0;
}

/*The following moved in from the original demo sketch espi.ino

I have removed function wav_setup() since wav_startPlayingFile() already does this initialization, appropriately, for each file played
I have removed function wav_playing() since it isn't used in any of the original code, and I2S_WAV.playing can be used directly

I have modified function wav_loop() - it returns a bool value of 1 if still playing. Using while(wav_loop()) allows the file to play through to the end. If conditional code is attached to this statement, then other code could be run between iterations, and wav_stopPlaying() could be called to stop play
*/

struct I2S_status_s {
  wavFILE_t wf;
  int16_t buffer[5120];           //was 512. Hoping that a larger buffer will smooth playback, without resorting to external RAM. Assigning > 5120 causes Oak to fail, despite only 88% of available memory being reported as used
  int bufferlen;
  int buffer_index;
  int playing;
} I2S_WAV;

// Non-blocking I2S write for left and right 16-bit PCM
bool ICACHE_FLASH_ATTR i2s_write_lr_nb(int16_t left, int16_t right) {
  int sample = right & 0xFFFF;
  sample = sample << 16;
  sample |= left & 0xFFFF;
  return i2s_write_sample_nb(sample);
}

void wav_stopPlaying() {
  i2s_end();
  I2S_WAV.playing = false;
  wavClose(&I2S_WAV.wf);
}

bool wav_loop() {
  bool i2s_full = false;
  int rc;

  while (I2S_WAV.playing && !i2s_full) {
    while (I2S_WAV.buffer_index < I2S_WAV.bufferlen) {
      int16_t pcm = I2S_WAV.buffer[I2S_WAV.buffer_index];
      if (i2s_write_lr_nb(pcm, pcm)) {
        I2S_WAV.buffer_index++;
      }
      else {
        i2s_full = true;
        break;
      }
      if ((I2S_WAV.buffer_index & 0x3F) == 0) yield();
    }
    if (i2s_full) break;

    rc = wavRead(&I2S_WAV.wf, I2S_WAV.buffer, sizeof(I2S_WAV.buffer));
    if (rc > 0) {
      I2S_WAV.bufferlen = rc / sizeof(I2S_WAV.buffer[0]);
      I2S_WAV.buffer_index = 0;
    }
    else {
      if (particleReportStatus) Particle.println("Play stopped");
      wav_stopPlaying();
      return 0;
    }
  }
  return 1;
}

void wav_startPlayingFile(const char *wavfilename) {
  wavProperties_t wProps;
  int rc;

  if (particleReportStatus) Particle.printf("Playing WAV %s\r\n", wavfilename);
  i2s_begin();
  rc = wavOpen(wavfilename, &I2S_WAV.wf, &wProps);
  if (rc != 0) {
    Particle.println("wavOpen failed");
    return;
  }
  if (particleReportStatus) {
    Particle.printf("audioFormat %d\r\n", wProps.audioFormat);
    Particle.printf("numChannels %d\r\n", wProps.numChannels);
    Particle.printf("sampleRate %d\r\n", wProps.sampleRate);
    Particle.printf("byteRate %d\r\n", wProps.byteRate);
    Particle.printf("blockAlign %d\r\n", wProps.blockAlign);
    Particle.printf("bitsPerSample %d\r\n", wProps.bitsPerSample);
  }
  
  i2s_set_rate(wProps.sampleRate);
  I2S_WAV.bufferlen = -1;
  I2S_WAV.buffer_index = 0;
  I2S_WAV.playing = true;
  wav_loop();
}