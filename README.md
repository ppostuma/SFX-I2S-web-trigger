# Play WAV from SPIFFS
Play WAV sounds from SPIFFS to an I2S device using the Digistump Oak
Uses wavspiffs.h code as per bbx10's SFX-I2S-web-trigger

This is a simpler version of the original code. Unlike to original, web triggering has been removed. I've also taken most of the functions from the example sketch espi2s.ino and moved them into the wavspiffs files. These I've renamed to WAV_SPIFFS in order to keep them distinct from the originals. I've also increased the audio buffer for more even playback.

An example sketch using the Digistump Oak is included.

Important note regarding the Oak's core_esp8266_i2s implementation, as of this writing: the pin declaration for I2S in Oak's core_esp8266_i2s.c, line 222, is incorrect:

    `pinMode(esp8266_gpioToPin[5], FUNCTION_1); //I2SO_BCK (SCLK)`

should read

    `pinMode(esp8266_gpioToPin[15], FUNCTION_1); //I2SO_BCK (SCLK)`

(I've already logged a pull request for this one.) Similar declaration on line 244 is correct.

Pin mappings:  LRC - Oak pin 0 (ESP8266 pin 2)
               BCLK - Oak pin 6 (ESP8266 pin 15)
               DIN - Oak pin 3 (ESP8266 pin 3)
               
WAV files should be mono (number of channels = 1) and contain uncompressed 16-bit PCM audio. Samples rates 8000, 11025, 22050, and 44100 have been verified to work. Included test WAV files were generated using Audacity; files created using Festival have previously been reported to work.

Please note that audio decoded directly from the ESP8266 won't be as clean as that achieved by a dedicated decoder board. You won't get as much storage from SPIFFS as boards with on-board storage. You won't be able to play compressed files like MP3s or Oggs so you'll be very limited in playback time. What you do get is a very small sized, simple, powered solution.

## Hardware components

* [Adafruit I2S 3W Class D Amplifier Breakout - MAX98357A](https://www.adafruit.com/products/3006)
This small, dirt-cheap board converts the digital audio data from the ESP8266 I2S controller to analog (mono) audio and amplifies the signal to drive a speaker.

* [Digistump Oak](http://digistump.com/category/22)

* 1+ Watt, 4-8 Ohm speaker

* power source for Oak and breakout board

## Connection Diagram

Adafruit I2S DAC |Digistump Oak      | Description
-----------------|-------------------|-------------
LRC              |P0                 | Left/Right audio
BCLK             |P3                 | I2S Clock
DIN              |P6                 | I2S Data
GAIN             |not connected      | 9 dB gain
SD               |not connected      | Stereo average
GND              |GND                | Ground
Vin              |not connected      | 3.3-5V battery power

Note that I don't recommend driving the breakout board using the Oak's VCC pin, though you could try. Power output would be limited, and I suspect you'll get cleaner audio powering it directly.

## Software components

The FS library handles SPIFFS file storage; the I2S library handles output to the MAX98357 breakout board. Both are part of the Oak's core code.
