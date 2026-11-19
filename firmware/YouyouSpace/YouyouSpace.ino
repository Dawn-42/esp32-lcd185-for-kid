#include "Display_ST77916.h"
#include "LVGL_Driver.h"
#include "I2C_Driver.h"
#include "SceneView.h"
#include "BoardExtras.h"
#include <Preferences.h>
#include "PetSave.h"

Game game;
Preferences progress;
bool viewReady=false,tiltAvailable=false,haveSave=false;
uint32_t lastLogic=0,lastFrame=0,lastTap=0,lastRendered=0,lastSaved=0,saveDue=0;
uint32_t reportAt=0,drawMicros=0,drawCount=0,dirtyPixels=0;
float filteredTilt=0;
uint32_t saveSequence=0,lastStore=0;
bool hasRecord=false;
static void loadProgress() {
  if(!haveSave)return;
  uint8_t a[PetSave::Bytes],b[PetSave::Bytes];
  PetSave::Record r;
  size_t na=progress.getBytes("petA",a,sizeof(a)),nb=progress.getBytes("petB",b,sizeof(b));
  if(PetSave::choose(a,na,b,nb,r)) {
    game.kitten.state=r.pet;game.album=r.album;saveSequence=r.sequence;hasRecord=true;
  } else game.album=progress.getUInt("album",0)&63u; // Migrate v2/v3 album; never erase it.
}
static bool storeProgress() {
  uint8_t data[PetSave::Bytes];
  PetSave::Record record;record.pet=game.kitten.state;record.album=game.album;record.sequence=saveSequence+1;
  PetSave::encode(record,data);
  const char *key=(record.sequence&1)?"petA":"petB";
  if(progress.putBytes(key,data,sizeof(data))!=sizeof(data))return false;
  saveSequence=record.sequence;hasRecord=true;return true;
}

static void onTouch(lv_event_t *event) {
  lv_indev_t *input=lv_indev_get_act();if(!input)return;
  lv_point_t point;lv_indev_get_point(input,&point);
  if(lv_event_get_code(event)==LV_EVENT_PRESSING) {
    game.move(point.x,point.y);return;
  }
  uint32_t now=millis();if(now-lastTap<150)return;lastTap=now;
  Scene previous=game.scene;PetActivity previousActivity=game.kitten.activity;
  game.tap(point.x,point.y);
  if(previous!=game.scene)extrasStop();
  bool leavingCare=previous==Scene::Cat&&(previous!=game.scene||
    (previousActivity!=PetActivity::Room&&game.kitten.activity==PetActivity::Room));
  if(leavingCare&&haveSave&&game.saveRevision!=lastSaved&&storeProgress()) {
    lastSaved=game.saveRevision;lastStore=now;saveDue=0;
  }
}
void setup() {
  Serial.begin(115200);
  I2C_Init();Wire.setTimeOut(15);
  Backlight_Init();LCD_Init();Lvgl_Init();Set_Backlight(65);
  constexpr size_t bytes=360*360*sizeof(lv_color_t);
  void *pixels=heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  void *base=heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  void *previous=heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  viewReady=viewInit(pixels,base,previous);
  if(!viewReady) { Serial.println("Canvas allocation failed: enable OPI PSRAM");return; }
  haveSave=progress.begin("youyou-v2",false);
  loadProgress();
  viewDraw(game,false);lastRendered=game.revision;Lvgl_Loop();
  extrasInit();
  lv_obj_add_event_cb(viewCanvas(),onTouch,LV_EVENT_PRESSED,nullptr);
  lv_obj_add_event_cb(viewCanvas(),onTouch,LV_EVENT_PRESSING,nullptr);
  extrasPlay(Sound::Hello);
  lastLogic=lastFrame=reportAt=millis();
}
void loop() {
  if(!viewReady) { delay(100);return; }
  Lvgl_Loop();
  uint32_t now=millis();
  if(now-lastLogic>=20) {
    // The IMU is needed only while driving. Touch override temporarily suspends reads.
    if(game.scene==Scene::Drive&&game.routeSelected&&!game.won&&game.age>=game.manualUntil) {
      float raw=0;
      tiltAvailable=extrasTilt(raw);
      if(tiltAvailable) {
        filteredTilt+=(raw-filteredTilt)*.25f;
        if(fabsf(filteredTilt)<.035f)filteredTilt=0;
      }
    }
    game.tick(now-lastLogic,filteredTilt,tiltAvailable);lastLogic=now;
  }
  bool changed=lastRendered!=game.revision;
  if(changed||(game.animated()&&now-lastFrame>=game.frameInterval())) {
    uint32_t started=micros();
    viewDraw(game,tiltAvailable);
    drawMicros+=micros()-started;++drawCount;dirtyPixels+=viewStats().pixels;
    lastRendered=game.revision;lastFrame=now;
  }
  extrasPlay(game.takeSound()); // Audio worker owns all blocking SD/I2S writes.
  if(haveSave&&game.saveRevision!=lastSaved) {
    if(!saveDue)saveDue=now+1800;
    if(int32_t(now-saveDue)>=0&&(!hasRecord||now-lastStore>=5000)) {
      if(storeProgress()) { lastSaved=game.saveRevision;lastStore=now; }
      saveDue=0;
    }
  }
  // Aggregate real measurements; do not log per frame or poll battery/RTC.
  if(now-reportAt>=10000) {
    Serial.printf("PERF frames=%lu avg_draw_us=%lu avg_dirty_pixels=%lu free_heap=%u psram=%u\n",
      (unsigned long)drawCount,(unsigned long)(drawCount?drawMicros/drawCount:0),
      (unsigned long)(drawCount?dirtyPixels/drawCount:0),ESP.getFreeHeap(),ESP.getFreePsram());
    drawMicros=drawCount=dirtyPixels=0;reportAt=now;
  }
  delay(2);
}
