#include "Game.h"
#include "PetSave.h"
#include <cassert>
#include <cstdio>
static void wash(PetModel &p) {
  p.begin(PetActivity::Bath);
  int x[]={128,218,139,223},y[]={162,168,220,225};
  for(int i=0;i<4;++i)p.tap(x[i],y[i]);
  for(int i=0;i<6;++i)p.tap(180,310);
}
int main() {
  PetModel p;p.state.energy=30;assert(p.need()==PetActivity::Sleep);
  p.state.energy=80;assert(p.need()==PetActivity::Litter);
  p.state.wasteMask=0;assert(p.need()==PetActivity::Feed);
  p.state.satiety=80;assert(p.need()==PetActivity::Bath);
  p.state.clean=80;assert(p.need()==PetActivity::Play);
  p.state.mood=80;assert(p.need()==PetActivity::Room);
  p.tap(275,89);assert(p.strokes==1);
  p.state.satiety=58;p.tap(275,89);assert(p.activity==PetActivity::Feed);
  auto revision=p.saveRevision;
  for(int n=0;n<1000;++n)p.tap(70,140);
  assert(p.progress==0&&p.state.totalFeeds==0&&p.saveRevision==revision&&p.feedback==PetSignal::Hint);
  p.tap(180,185);p.tap(180,238);p.tap(180,310);
  assert(p.progress==3&&p.done&&p.earned&&p.state.growth==6&&p.feedback==PetSignal::Bite);
  p.begin(PetActivity::Bath);
  int x[]={128,218,139,223},y[]={162,168,220,225};
  for(int i=0;i<4;++i)p.tap(x[i],y[i]);
  assert(p.phase==1);p.tap(70,140);assert(p.progress==0);
  for(int n=0;n<3;++n)p.tap(180,180);
  assert(p.phase==2);p.tap(70,200);assert(p.progress==0);
  for(int n=0;n<3;++n)p.tap(180,230);
  assert(p.done&&p.earned);
  p.state.clean=100;auto growth=p.state.growth;wash(p);
  assert(p.done&&!p.earned&&p.state.growth==growth);
  p.state.satiety=100;p.begin(PetActivity::Feed);p.tap(70,140);assert(p.feedback==PetSignal::Full&&p.progress==0);p.tap(180,310);assert(p.activity==PetActivity::Play);
  p.state.energy=20;p.tap(180,310);assert(p.activity==PetActivity::Sleep);
  PetModel ball;ball.begin(PetActivity::Play);ball.tap(ball.ballX(),ball.ballY());
  assert(ball.feedback==PetSignal::Ball);ball.tick(100);
  PetModel sameClock;sameClock.clockMs=ball.clockMs;
  assert(ball.ballY()<sameClock.ballY());
  PetModel called;called.tap(90,250);assert(called.called);
  bool arrived=false;
  for(int n=0;n<500;++n) {
    called.tick(20);
    if(!called.called&&called.attentionUntil>called.clockMs) { arrived=true;break; }
  }
  assert(arrived&&!called.walk.moving&&called.feedback==PetSignal::Invite);
  Game voice;voice.enter(Scene::Cat);voice.takeSound();voice.tap(275,89);
  assert(voice.takeSound()==Sound::LitterHelp);voice.tap(180,40);assert(voice.takeSound()==Sound::Stop);
  voice.tap(90,285);assert(voice.takeSound()==Sound::FeedHelp);voice.tap(70,140);assert(voice.takeSound()==Sound::None);
  Game g;g.enter(Scene::City);g.tap(180,300);
  assert(g.cityRide&&g.kitten.state.completedMask&32&&g.memories()&2);
  revision=g.saveRevision;for(int n=0;n<40;++n)g.tick(20);
  g.tap(180,300);assert(g.saveRevision==revision);
  PetSave::Record r;r.pet=g.kitten.state;uint8_t bytes[PetSave::Bytes];PetSave::encode(r,bytes);
  PetSave::Record loaded;assert(PetSave::decode(bytes,sizeof(bytes),loaded)&&loaded.pet.completedMask&32);
  PetSave::put(bytes+13*4,64);PetSave::put(bytes+PetSave::Bytes-4,PetSave::hash(bytes,PetSave::Bytes-4));
  assert(!PetSave::decode(bytes,sizeof(bytes),loaded));
  g.album=18;g.kitten.state.completedMask=15;assert(g.memories()==127);
  g=Game{};g.enter(Scene::Drive);assert(g.routeSelected&&g.route==0);
  g.album=2;g.enter(Scene::Drive);assert(!g.routeSelected);
  g.enter(Scene::Cat);g.tap(90,285);g.tap(180,40);
  assert(g.scene==Scene::Cat&&g.kitten.activity==PetActivity::Room);
  g.tap(180,40);assert(g.scene==Scene::Home);
  puts("PASS: need priority, icon invitation, object-only care, valid alternative buttons, reward explanation, ball kick, call/arrival eye contact, persistent souvenir and first drive guidance");
}
