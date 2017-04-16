/* Digistump Oak (ESP8266-based MCU) sketch to load WAV from SPIFFS and plays it via Adafruit's I2S MAX98357 3W Mono Amp Breakout board.

MAX08357 connections:
   * VIN - 2.7 to 5.5V
   * LRC - pin 0 (GPIO2/TX1)
   * BCLK - pin 3 (GPIO3/RX0)
   * DIN - pin 6 (GPIO15)
   * GND

Adapted from bbx10's very useful SFX-I2S-web-trigger code */

#include <WAV_SPIFFS.h>


void setup() {
  Particle.begin();
  toggleParticleReportStatus();   //turn on verbose reporting to Particle
  
  if (!SPIFFS.begin()) {
    Particle.println("SPIFFS.begin() failed");
    return;
  }

  wav_startPlayingFile("/R2D2A.WAV");
  while(wav_loop());
  wav_startPlayingFile("/SYNTH.WAV");
  while(wav_loop());
  wav_startPlayingFile("/WOLF.WAV");
  while(wav_loop());
  wav_startPlayingFile("/GUITAR.WAV");
  while(wav_loop());
  wav_startPlayingFile("/ORCHESTR.WAV");
  while(wav_loop());
}

void loop() {
}



