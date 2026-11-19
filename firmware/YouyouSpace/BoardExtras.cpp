#include "BoardExtras.h"
#include <Arduino.h>
#include <Wire.h>
#include <SD_MMC.h>
#include <ESP_I2S.h>
#include "es8311.h"
#include <freertos/queue.h>
#include <freertos/task.h>

namespace {
I2SClass speaker;
bool audioReady=false, sdReady=false, imuReady=false;
File wav;
uint32_t wavRemaining=0, samplesRemaining=0;
uint32_t phase=0,phaseStep=0;
float frequency=0;
int16_t sineTable[256];
QueueHandle_t audioCommands=nullptr;
void audioWorker(void *);
unsigned note=0, noteSamples=0;
bool birthday=false,playingBirthday=false;
// Traditional public-domain birthday melody, played softly if no recording exists.
const float tune[]={392,392,440,392,523.25,493.88,392,392,440,392,587.33,523.25,
                    392,392,783.99,659.25,523.25,493.88,440,698.46,698.46,659.25,523.25,587.33,523.25};
const unsigned beats[]={1,1,2,2,2,4,1,1,2,2,2,4,1,1,2,2,2,2,4,1,1,2,2,2,4};
bool readReg(uint8_t reg,uint8_t *b,size_t n) {
  Wire.beginTransmission(0x6B); Wire.write(reg);
  if(Wire.endTransmission(false)!=0) return false;
  if(Wire.requestFrom(uint8_t(0x6B),uint8_t(n))!=n) return false;
  for(size_t i=0;i<n;++i) b[i]=Wire.read();
  return true;
}
bool writeReg(uint8_t reg,uint8_t v) {
  Wire.beginTransmission(0x6B); Wire.write(reg); Wire.write(v); return Wire.endTransmission()==0;
}
uint16_t u16(const uint8_t *b) { return b[0]|uint16_t(b[1])<<8; }
uint32_t u32(const uint8_t *b) { return u16(b)|uint32_t(u16(b+2))<<16; }
bool openWav(const char *path) {
  if(!sdReady) return false;
  wav=SD_MMC.open(path);
  if(!wav) return false;
  uint8_t h[12];
  bool valid=wav.read(h,12)==12 && !memcmp(h,"RIFF",4) && !memcmp(h+8,"WAVE",4);
  bool fmt=false;
  while(valid && wav.available()) {
    uint8_t chunk[8]; if(wav.read(chunk,8)!=8) break;
    uint32_t size=u32(chunk+4), pos=wav.position();
    if(size>wav.size()-pos) break;
    if(!memcmp(chunk,"fmt ",4)) {
      uint8_t f[16];
      if(size<16 || wav.read(f,16)!=16) break;
      fmt=u16(f)==1 && u16(f+2)==1 && u32(f+4)==16000 && u16(f+12)==2 && u16(f+14)==16;
    } else if(!memcmp(chunk,"data",4)) {
      if(!fmt || !size || size%2) break;
      wavRemaining=size; return true;
    }
    if(!wav.seek(pos+size+(size&1))) break;
  }
  Serial.printf("Unsupported WAV: %s (need PCM16 mono 16000Hz)\n",path);
  wav.close(); return false;
}
}
void extrasInit() {
  uint8_t id=0;
  if(readReg(0,&id,1) && id==0x05) {
    // QMI8658: auto-increment, +/-2g, 125Hz accelerometer, gyro disabled.
    imuReady=writeReg(0x02,0x60) && writeReg(0x08,0x00) && writeReg(0x03,0x06) && writeReg(0x08,0x01);
  }
  Serial.printf("IMU: %s\n",imuReady?"ready":"unavailable; touch steering enabled");
  SD_MMC.setPins(15,14,16);
  sdReady=SD_MMC.begin("/sdcard",true,false,20000);
  Serial.printf("SD: %s\n",sdReady?"ready":"optional; using built-in sounds");
  speaker.setPins(48,38,47,-1,2);
  if(!speaker.begin(I2S_MODE_STD,16000,I2S_DATA_BIT_WIDTH_16BIT,I2S_SLOT_MODE_MONO,I2S_STD_SLOT_LEFT)) return;
  es8311_handle_t codec=es8311_create(I2C_NUM_0,ES8311_ADDRRES_0);
  if(!codec) return;
  es8311_clock_config_t clock={};
  clock.mclk_from_mclk_pin=true; clock.mclk_frequency=16000*256; clock.sample_frequency=16000;
  if(es8311_init(codec,&clock,ES8311_RESOLUTION_16,ES8311_RESOLUTION_16)!=ESP_OK) return;
  if(es8311_voice_volume_set(codec,45,nullptr)!=ESP_OK) return;
  es8311_microphone_config(codec,false);
  pinMode(9,OUTPUT); digitalWrite(9,HIGH);
  for(unsigned i=0;i<256;++i)sineTable[i]=int16_t(sinf(i*6.2831853f/256)*3500);
  audioCommands=xQueueCreate(1,sizeof(Sound));
  if(!audioCommands)return;
  audioReady=true;
  if(xTaskCreatePinnedToCore(audioWorker,"youyou-audio",6144,nullptr,1,nullptr,0)!=pdPASS) {
    audioReady=false;vQueueDelete(audioCommands);audioCommands=nullptr;return;
  }
  Serial.println("Audio: ready");
}
bool extrasTilt(float &tilt) {
  if(!imuReady) return false;
  uint8_t v[2]; if(!readReg(0x35,v,2)) return false;
  tilt=int16_t(u16(v))/16384.f;
  return true;
}
static void stopAudio() {
  if(wav) wav.close();
  birthday=playingBirthday=false; samplesRemaining=0; wavRemaining=0;
}
static void startAudio(Sound s) {
  if(!audioReady || s==Sound::None) return;
  stopAudio();
  playingBirthday=s==Sound::Birthday;
  const char *path=s==Sound::Hello?"/youyou/hello.wav":s==Sound::Meow?"/youyou/meow.wav":s==Sound::Birthday?"/youyou/birthday.wav":
    s==Sound::FeedHelp?"/youyou/feed.wav":s==Sound::BathHelp?"/youyou/bath.wav":
    s==Sound::PlayHelp?"/youyou/play.wav":s==Sound::LitterHelp?"/youyou/litter.wav":
    s==Sound::SleepHelp?"/youyou/sleep.wav":nullptr;
  if(path && openWav(path)) return;
  phase=0; birthday=s==Sound::Birthday; note=0; noteSamples=0;
  frequency=s==Sound::Bump?196:s==Sound::Purr?262:s==Sound::Meow?660:s==Sound::Hello?523.25:1046.5;
  phaseStep=uint32_t(frequency*(4294967296.0/16000));
  samplesRemaining=s==Sound::Purr?6400:s==Sound::Bump?1800:s==Sound::Meow?4400:2400;
}
static void audioPump() {
  if(!audioReady) return;
  int16_t buffer[256]={};
  if(wav && wavRemaining) {
    unsigned n=wavRemaining>sizeof(buffer)?sizeof(buffer):wavRemaining;
    size_t got=wav.read(reinterpret_cast<uint8_t *>(buffer),n);
    if(!got) { wav.close(); wavRemaining=0; return; }
    wavRemaining-=got;
    // Limit recording volume, too. Keep a signed waveform.
    for(unsigned i=0;i<got/2;++i) buffer[i]/=2;
    speaker.write(reinterpret_cast<uint8_t *>(buffer),got);
    if(!wavRemaining) wav.close();
    return;
  }
  if(!samplesRemaining && !birthday) return;
  for(unsigned i=0;i<256;++i) {
    if(birthday) {
      if(note>=sizeof(tune)/sizeof(tune[0])) { birthday=false; samplesRemaining=0; break; }
      if(noteSamples==0) { frequency=tune[note]; phaseStep=uint32_t(frequency*(4294967296.0/16000)); samplesRemaining=beats[note]*3200; noteSamples=samplesRemaining; }
    }
    if(samplesRemaining) {
      int32_t sample=sineTable[phase>>24];
      if(samplesRemaining<200)sample=sample*samplesRemaining/200;
      if(birthday&&samplesRemaining<500)sample=0;
      buffer[i]=int16_t(sample);phase+=phaseStep;
      --samplesRemaining;
      if(birthday && !samplesRemaining) { ++note; noteSamples=0; }
    }
  }
  speaker.write(reinterpret_cast<uint8_t *>(buffer),sizeof(buffer));
}

namespace {
void audioWorker(void *) {
  // SD, decoder state and I2S are exclusively owned by this task after setup.
  // A one-slot mailbox coalesces rapid effects instead of growing a backlog.
  for(;;) {
    Sound command;
    bool active=wavRemaining||samplesRemaining||birthday;
    if(xQueueReceive(audioCommands,&command,active?0:portMAX_DELAY)==pdTRUE) {
      if(command==Sound::Stop)stopAudio();
      else if(!(playingBirthday&&active&&command==Sound::Star))startAudio(command);
    }
    audioPump();
  }
}
}
void extrasPlay(Sound s) {
  if(audioReady&&s!=Sound::None)xQueueOverwrite(audioCommands,&s);
}
void extrasStop() {
  Sound s=Sound::Stop;
  if(audioReady)xQueueOverwrite(audioCommands,&s);
}
