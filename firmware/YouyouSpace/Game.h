#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "PetModel.h"
#include "PixelCity.h"

enum class Scene { Home, Cat, Drive, Cake, Album, City };
enum class Sound { None, Hello, Meow, Star, Birthday, Stop, Bump, Purr, FeedHelp, BathHelp, PlayHelp, LitterHelp, SleepHelp };
enum class PetMode { Calm, Fed, Play, Sleep };
enum class CakePhase { Decorate, Light, Wish, Party };
enum class Pickup { Star, Rock, Magnet, Fuel, Pod };
struct Item { float x=180,y=0; Pickup kind=Pickup::Star; unsigned color=0; bool active=false; };
struct Spark { float x=0,y=0,vx=0,vy=0; uint16_t life=0; unsigned color=0; };
struct Topping { int16_t x=0,y=0; unsigned style=0; };

// Fixed storage: simulation and input handlers never allocate.
struct Game {
  Scene scene=Scene::Home;
  uint32_t cityTime=0;
  unsigned cityLandmark=PixelCity::LandmarkCount,cityRide=0,cityCooldown=0;
  Sound sound=Sound::None;
  uint32_t memoryUntil=0;
  unsigned memorySeen=0,memoryGift=0;
  PetModel kitten;
  uint32_t petSeen=0,petSaveSeen=0,petMessageUntil=0;
  PetSignal petMessage=PetSignal::None;
  CakePhase cakePhase=CakePhase::Decorate;
  uint32_t age=0,affection=0,manualUntil=0,magnetUntil=0,slowUntil=0;
  uint32_t feedbackUntil=0,revision=1,saveRevision=0,randomState=20261123;
  unsigned pats=0,careMask=0,ballHits=0,album=0,selectedSticker=0;
  unsigned stars=0,route=0,spawnCount=0,spawnClock=0,candles=0,style=0,toppingCount=0,fireworks=0;
  float car=180,target=180;
  bool routeSelected=false,won=false,newSticker=false;
  int lastRubX=180,lastRubY=180;
  uint32_t lastRub=0;
  Item items[4];
  Spark sparks[20];
  Topping toppings[10];
  uint32_t candleTouch[4]={};

  static float clamp(float x,float a,float b) { return fmaxf(a,fminf(b,x)); }
  unsigned random(unsigned n) { randomState=randomState*1664525u+1013904223u;return (randomState>>8)%n; }
  unsigned stickerCount() const { unsigned n=0;for(unsigned i=0;i<6;++i)n+=(album>>i)&1;return n; }
  unsigned memories() const {
    unsigned m=0;const auto &p=kitten.state;
    if(album&14)m|=1; // space poster
    if((album&14)||(p.completedMask&32))m|=2; // souvenir car
    if(album&16)m|=4; // birthday balloons
    if((p.completedMask&15)==15)m|=8; // care photo
    if(p.completedMask&4)m|=16; // toy ball
    if(p.completedMask&2)m|=32; // flowers
    if(p.completedMask&1)m|=64; // fish keepsake
    return m;
  }
  void checkMemories() {
    if(scene!=Scene::Cat||kitten.activity!=PetActivity::Room)return;
    unsigned m=memories(),fresh=m&~memorySeen;
    if(fresh) { memoryGift=fresh;memoryUntil=age+2200; }
    memorySeen=m;
  }
  static Sound help(PetActivity a) {
    return a==PetActivity::Feed?Sound::FeedHelp:a==PetActivity::Bath?Sound::BathHelp:
      a==PetActivity::Play?Sound::PlayHelp:a==PetActivity::Litter?Sound::LitterHelp:
      a==PetActivity::Sleep?Sound::SleepHelp:Sound::Meow;
  }
  unsigned goal() const { return route==1?4:5; }
  unsigned wantedColor() const { return stars%3; }
  float ballX() const { unsigned p=(age/5)%400;return 80+(p<200?p:400-p); }
  float ballY() const { return 202-fabsf(sinf(age/260.f))*40; }
  bool animated() const {
    if(age<320)return true;
    bool effects=false;for(const auto &p:sparks)effects|=p.life!=0;
    return scene==Scene::City||scene==Scene::Cat||(scene==Scene::Drive&&routeSelected&&(!won||effects))||
      (scene==Scene::Cake&&(cakePhase==CakePhase::Wish||
      (cakePhase==CakePhase::Light&&candles)||
      (cakePhase==CakePhase::Party&&((age<6000&&!fireworks)||effects))));
  }
  unsigned frameInterval() const {
    if(age<320)return 33;
    if(scene==Scene::Drive&&routeSelected)return 33;
    if(scene==Scene::City)return PixelCity::FrameMs;
    if(scene==Scene::Cat)return kitten.activity==PetActivity::Sleep?100:60;
    if(scene==Scene::Cake)return cakePhase==CakePhase::Party?50:100;
    return 1000;
  }
  void burst(float x,float y,unsigned palette=0) {
    for(unsigned i=0;i<20;++i) {
      float a=i*6.2831853f/20,v=25+random(55);
      sparks[i]={x,y,cosf(a)*v,sinf(a)*v-20,uint16_t(700+random(400)),(i+palette)%3};
    }
  }
  void unlock(unsigned bit) {
    if(album&(1u<<bit))return;
    album|=1u<<bit;++saveRevision;newSticker=true;
  }
  void care(unsigned bit) { careMask|=bit;if(careMask==7)unlock(0); }
  void enter(Scene next) {
    cityTime=0;cityLandmark=PixelCity::LandmarkCount;cityRide=cityCooldown=0;
    scene=next;age=0;won=false;newSticker=false;routeSelected=false;
    petMessage=PetSignal::None;petMessageUntil=0;memoryUntil=0;memoryGift=0;
    car=target=180;stars=0;spawnCount=spawnClock=0;
    manualUntil=magnetUntil=slowUntil=feedbackUntil=affection=0;
    if(next==Scene::Cat&&kitten.activity!=PetActivity::Sleep)kitten.begin(PetActivity::Room);
    cakePhase=CakePhase::Decorate;candles=toppingCount=fireworks=0;
    for(auto &i:items)i.active=false;
    for(auto &p:sparks)p.life=0;
    for(auto &t:candleTouch)t=0;
    sound=next==Scene::Cat?Sound::Meow:Sound::Stop;++revision;
    if(next==Scene::Drive&&!(album&14))startRoute(0);
  }
  void startRoute(unsigned r) {
    route=r%3;routeSelected=true;stars=0;won=false;
    car=target=180;spawnClock=800;spawnCount=0;magnetUntil=slowUntil=manualUntil=0;
    for(auto &i:items)i.active=false;
    sound=Sound::Star;++revision;
  }
  void syncPet() {
    if(petSeen!=kitten.revision) { petSeen=kitten.revision;++revision; }
    if(petSaveSeen!=kitten.saveRevision) { petSaveSeen=kitten.saveRevision;++saveRevision; }
    auto event=kitten.takeSignal();
    if(event==PetSignal::None)return;
    petMessage=event;petMessageUntil=age+(event==PetSignal::Grow?3000:1200);
    if(scene!=Scene::Cat)return;
    if(event==PetSignal::Stroke||event==PetSignal::Bite)sound=Sound::Meow;
    else if(event==PetSignal::Rest||event==PetSignal::Invite)sound=Sound::Purr;
    else if(event!=PetSignal::Full&&event!=PetSignal::Tired&&event!=PetSignal::Hint)sound=Sound::Star;
    if(event==PetSignal::Complete||event==PetSignal::Grow||event==PetSignal::Scoop)burst(180,175,1);
    if((kitten.state.completedMask&15)==15)unlock(0);
    if(kitten.state.totalPlayHits>=5)unlock(5);
  }
  void steer(int x) { target=clamp(x,78,282);manualUntil=age+1800; }
  void move(int x,int y) {
    if(scene==Scene::Drive&&routeSelected&&!won&&y>74)steer(x);
    // All care actions also work by tapping; movement is optional extra petting.
    if(scene==Scene::Cat&&kitten.activity==PetActivity::Room&&kitten.hitCat(x,y)) {
      if(abs(x-lastRubX)+abs(y-lastRubY)>35) {
        kitten.stroke();syncPet();lastRubX=x;lastRubY=y;
      }
    }
  }
  void tap(int x,int y) {
    if(x<0||x>=360||y<0||y>=360)return;
    if(scene!=Scene::Home&&x>=140&&x<=220&&y>=15&&y<=69) {
      if(scene==Scene::Cat&&kitten.activity!=PetActivity::Room) {
        kitten.begin(PetActivity::Room);syncPet();sound=Sound::Stop;
      } else enter(Scene::Home);
      return;
    }
    if(scene==Scene::Home) {
      if(x>=65&&x<=295&&y>=63&&y<105) { enter(x<=180?Scene::Album:Scene::City);return; }
      if(x<65||x>295)return;
      if(y>=110&&y<173)enter(Scene::Cat);
      else if(y>=181&&y<245)enter(Scene::Drive);
      else if(y>=253&&y<321)enter(Scene::Cake);
    } else if(scene==Scene::City) {
      if(y>=266&&y<=335&&x>=55&&x<=305) {
        if(!cityCooldown&&!cityRide) {
          cityRide=1800;cityCooldown=700;
          if(!(kitten.state.completedMask&32)) { kitten.state.completedMask|=32;kitten.changed();syncPet(); }
          sound=Sound::Meow;++revision;
        }
        return;
      }
      int hit=PixelCity::hit(x,y);
      if(y>=232&&y<=262)cityLandmark=PixelCity::LandmarkCount;
      else if(hit>=0)cityLandmark=unsigned(hit);
      ++revision;
    } else if(scene==Scene::Cat) {
      auto before=kitten.activity;kitten.tap(x,y);syncPet();
      if(before!=kitten.activity)sound=kitten.activity==PetActivity::Room?Sound::Stop:help(kitten.activity);
    } else if(scene==Scene::Drive) {
      if(!routeSelected) {
        if(y>=131&&y<235&&x>=50&&x<310)startRoute(x<135?0:x<225?1:2);
      } else if(won) {
        if(y>=272&&y<327) {
          if(x<180) { routeSelected=false;++revision; }
          else startRoute((route+1)%3);
        }
      } else if(y>=75)steer(x);
    } else if(scene==Scene::Cake) {
      if(cakePhase==CakePhase::Decorate) {
        if(y>=266&&y<298&&x>=70&&x<290) { style=(x-70)/74;++revision; }
        else if(y>=304&&y<344&&x>=115&&x<=245) { cakePhase=CakePhase::Light;candles=0;++revision; }
        else if(x>=90&&x<=270&&y>=211&&y<=260) {
          unsigned slot=toppingCount<10?toppingCount++:random(10);
          toppings[slot]={int16_t(clamp(x,102,258)),int16_t(clamp(y,219,251)),style};
          sound=Sound::Star;++revision;
        }
      } else if(cakePhase==CakePhase::Light||cakePhase==CakePhase::Wish) {
        if(y>=123&&y<=207&&x>=100&&x<260) {
          unsigned bit=1u<<((x-100)/40);
          if(!(candles&bit)) {
            candles|=bit;candleTouch[(x-100)/40]=age;sound=Sound::Star;++revision;
            if(candles==15) {
              if(cakePhase==CakePhase::Light) { cakePhase=CakePhase::Wish;candles=0; }
              else { cakePhase=CakePhase::Party;unlock(4);sound=Sound::Birthday;burst(180,150); }
            }
          }
        }
      } else {
        if(y>290&&x>=110&&x<=250) { enter(Scene::Cake);return; }
        if(y>75&&y<285) { ++fireworks;burst(x,y,fireworks);++revision; }
      }
    } else if(scene==Scene::Album) {
      if(x>=60&&x<300&&y>=118&&y<285) {
        selectedSticker=(y<205?0:3)+(x<140?0:x<220?1:2);sound=Sound::Star;++revision;
      }
    }
  }
  void spawn() {
    for(auto &i:items)if(!i.active) {
      i.active=true;i.y=145;i.x=96+random(169);i.color=random(3);
      unsigned phase=spawnCount++%7;
      i.kind=phase==3?Pickup::Rock:phase==5?Pickup::Magnet:Pickup::Star;
      if(route==1&&i.kind==Pickup::Star)i.kind=stars>=3?Pickup::Pod:Pickup::Fuel;
      if(route==2&&phase%3!=2)i.color=wantedColor();
      return;
    }
  }
  void tick(uint32_t dt,float tilt=0,bool haveTilt=false) {
    bool wasAnimated=animated();
    if(dt>80)dt=80;
    if(scene==Scene::City) {
      cityTime=(cityTime+dt)%PixelCity::LoopMs;
      cityRide=cityRide>dt?cityRide-dt:0;cityCooldown=cityCooldown>dt?cityCooldown-dt:0;
      age=age<1000?age+dt:1000;
    } else age+=dt;
    kitten.tick(dt,scene==Scene::Cat);syncPet();checkMemories();
    for(auto &p:sparks)if(p.life) {
      if(p.life<=dt) { p.life=0;continue; }
      p.life-=dt;p.x+=p.vx*dt/1000;p.y+=p.vy*dt/1000;p.vy+=45*dt/1000.f;
    }
    if(scene!=Scene::Drive||!routeSelected||won) {
      if(wasAnimated&&!animated())++revision;
      return;
    }
    if(haveTilt&&age>manualUntil)target=clamp(180+tilt*210,78,282);
    car+=(target-car)*fminf(1,dt/100.f);
    spawnClock+=dt;
    if(spawnClock>=1050) { spawnClock-=1050;spawn(); }
    float speed=(route==0?0.063f:0.077f)*(age<slowUntil?.5f:1);
    for(auto &i:items)if(i.active) {
      i.y+=dt*speed;
      bool collectible=i.kind!=Pickup::Rock;
      if(age<magnetUntil&&collectible&&i.y>180)i.x+=(car-i.x)*fminf(1,dt/160.f);
      if(i.y>=246&&i.y<=293&&fabsf(i.x-car)<37) {
        i.active=false;++revision;
        if(i.kind==Pickup::Rock) { slowUntil=age+1100;feedbackUntil=age+1100;sound=Sound::Bump;burst(car,265,1); }
        else if(i.kind==Pickup::Magnet) { magnetUntil=age+6000;sound=Sound::Star;burst(car,240,2); }
        else if((route==2&&i.color!=wantedColor())||(route==1&&stars>=3&&i.kind!=Pickup::Pod))feedbackUntil=age+1200;
        else {
          ++stars;sound=Sound::Star;burst(car,235,i.color);
          if(stars>=goal()) {
            won=true;unlock(1+route);for(auto &o:items)o.active=false;break;
          }
        }
      } else if(i.y>316)i.active=false;
    }
    if(wasAnimated&&!animated())++revision;
  }
  Sound takeSound() { Sound s=sound;sound=Sound::None;return s; }
};
