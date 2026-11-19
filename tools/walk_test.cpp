#include "Game.h"
#include "PetSave.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
int main() {
  Game g;g.enter(Scene::Cat);auto &p=g.kitten;
  float minX=180,maxX=180,minY=247,maxY=247;
  for(unsigned n=0;n<100000;++n) {
    float beforeX=p.walk.x,beforeY=p.walk.y;
    g.tick(20);
    assert(p.walk.x>=90&&p.walk.x<=258&&p.walk.y>=236&&p.walk.y<=256);
    assert(hypotf(p.walk.x-beforeX,p.walk.y-beforeY)<=.55f);
    minX=fminf(minX,p.walk.x);maxX=fmaxf(maxX,p.walk.x);
    minY=fminf(minY,p.walk.y);maxY=fmaxf(maxY,p.walk.y);
  }
  assert(maxX-minX>100&&maxY-minY>12);
  p.walk.x=220;p.walk.y=247;g.tap(90,253);
  assert(p.walk.targetX==90&&p.walk.marker>0);
  for(unsigned n=0;n<150;++n)g.tick(20);
  assert(p.walk.x<160);
  auto strokes=p.strokes;g.tap(p.walk.x,p.walk.y-p.roomSize()/2);
  assert(p.strokes==strokes+1&&!p.walk.moving);
  auto x=p.walk.x;g.tick(60);assert(p.walk.x==x);
  // Moving a happy cat does not touch persistent state or save revisions.
  PetModel clean;clean.state.mood=100;auto saved=clean.saveRevision;
  PetSave::Record r;r.pet=clean.state;uint8_t a[PetSave::Bytes],b[PetSave::Bytes];PetSave::encode(r,a);
  PetModel control=clean;clean.walk.go(90,250,true);
  for(int i=0;i<400;++i) { clean.tick(20);control.tick(20,false); }
  r.pet=control.state;PetSave::encode(r,a);
  r.pet=clean.state;PetSave::encode(r,b);assert(!memcmp(a,b,sizeof(a))&&clean.saveRevision==saved);
  x=p.walk.x;g.enter(Scene::Home);for(int n=0;n<200;++n)g.tick(20);assert(p.walk.x==x);
  for(auto activity:{PetActivity::Feed,PetActivity::Bath,PetActivity::Litter,PetActivity::Sleep}) {
    p.begin(activity);x=p.walk.x;for(int n=0;n<100;++n)g.tick(20);assert(p.walk.x==x);
  }
  g.enter(Scene::Cat);p.begin(PetActivity::Play);x=p.walk.x;
  for(int n=0;n<100;++n)g.tick(20);assert(fabsf(p.walk.x-x)>1);
  g.tap(p.ballX(),p.ballY());assert(p.progress==1);
  p.begin(PetActivity::Room);g.tap(90,285);assert(p.activity==PetActivity::Feed);
  PetWalk cadence;cadence.go(258,250);
  unsigned framesSeen=0;
  for(unsigned n=0;n<180;++n) {
    float px=cadence.x,py=cadence.y,phase=cadence.phase,v=cadence.speed;
    cadence.tick(20,false,180,26);
    float distance=hypotf(cadence.x-px,cadence.y-py);
    if(cadence.moving) {
      float delta=cadence.phase-phase;if(delta<0)delta+=1;
      assert(fabsf(delta-distance/26)<.0001f);
      framesSeen|=1u<<cadence.frame();
    }
    assert(cadence.speed>=0&&cadence.speed<=27);
    assert(cadence.speed<=v+1.701f);
  }
  assert(framesSeen==255);
  cadence.go(90,247);bool turned=false;
  for(unsigned n=0;n<80;++n) {
    bool facing=cadence.left;cadence.tick(20);
    if(cadence.left!=facing) { assert(cadence.speed==0);turned=true; }
  }
  assert(turned&&cadence.left);
  cadence.rest();float frozen=cadence.phase;
  for(unsigned n=0;n<50;++n)cadence.tick(20);
  assert(cadence.phase==frozen&&!cadence.moving&&cadence.stand==0);
  puts("PASS: distance-synced 8-frame cadence, acceleration, stationary turn/stop, 100000-step roam bounds/speed/coverage, floor call, moving hitbox, stroke pause, hidden/stationary activities, ball chase and unchanged save record");
}
