#pragma once
#include <stdint.h>
#include <math.h>
// Transient motion, separate from the NVS pet record. Phase advances by distance.
struct PetWalk {
  float x=180,y=247,targetX=180,targetY=247;
  float speed=0,phase=0,stand=0,dirX=1,dirY=0;
  uint32_t seed=70423,pause=900,stepClock=0,marker=0,turn=0,standingTime=0;
  bool moving=false,left=false;
  static float bound(float v,float lo,float hi) { return v<lo?lo:v>hi?hi:v; }
  unsigned random(unsigned n) { seed=seed*1664525u+1013904223u;return (seed>>8)%n; }
  static float approach(float value,float goal,float amount) {
    return value<goal?fminf(goal,value+amount):fmaxf(goal,value-amount);
  }
  void go(float xx,float yy,bool called=false) {
    targetX=bound(xx,90,258);targetY=bound(yy,236,256);pause=0;
    if(called)marker=1700;
  }
  void rest() { targetX=x;targetY=y;pause=1400;moving=false;speed=0;turn=0;standingTime=0; }
  unsigned frame() const { return unsigned(phase*8)&7; }
  void tick(uint32_t dt,bool chase=false,float ball=180,float stride=26) {
    if(dt>80)dt=80;float seconds=dt*.001f;
    stepClock=(stepClock+dt)%2400;marker=marker>dt?marker-dt:0;
    if(chase) { targetX=bound(ball,90,258);targetY=249;pause=0; }
    if(pause) {
      pause=pause>dt?pause-dt:0;moving=false;speed=0;
      standingTime+=dt;if(standingTime>350)stand=approach(stand,0,seconds/0.24f);
      return;
    }
    float dx=targetX-x,dy=targetY-y,dist=sqrtf(dx*dx+dy*dy);
    if(dist<.35f) {
      speed=0;moving=false;
      if(!chase) {
        pause=700+random(1700);standingTime=0;
        targetX=90+random(169);targetY=236+random(21);
      }
      return;
    }
    stand=approach(stand,1,seconds/.22f);
    if(stand<1) { moving=false;return; } // Rise before the first step.
    bool desired=dx<-4?true:dx>4?false:left;
    if(turn) {
      turn=turn>dt?turn-dt:0;moving=false;speed=0;
      if(!turn)left=desired;
      return;
    }
    bool reversing=desired!=left;
    float wanted=reversing?0:fminf(chase?42.f:27.f,sqrtf(2*85.f*dist));
    float previousSpeed=speed;
    speed=approach(speed,wanted,(wanted<speed?120.f:85.f)*seconds);
    if(reversing&&speed<.1f) { turn=240;moving=false;return; }
    // During braking the feet continue in the previous direction until stopped.
    if(!reversing) { dirX=dx/dist;dirY=dy/dist; }
    float travel=(previousSpeed+speed)*.5f*seconds;
    if(!reversing&&travel>=dist)travel=dist;
    float oldX=x,oldY=y;
    x=bound(x+dirX*travel,90,258);y=bound(y+dirY*travel,236,256);
    float actual=hypotf(x-oldX,y-oldY);
    phase+=actual/fmaxf(12,stride);phase-=floorf(phase);
    moving=actual>.002f;
    if(!reversing&&travel>=dist) { speed=0;moving=false; }
  }
};
