#include "Game.h"
#include "Motion.h"
#include <cassert>
#include <cstdio>
static void collect(Game &g,Pickup kind,unsigned color=0) {
  for(auto &i:g.items)i.active=false;
  g.spawnClock=0;g.items[0]={g.car,250,kind,color,true};g.tick(20);
}
int main() {
  assert(Motion::blink(2200)==2&&Motion::entryOpacity(0)==255&&Motion::entryOpacity(320)==0);
  Game g;g.tap(180,80);
  assert(g.scene==Scene::Album);
  g.tap(180,40);g.tap(180,140);
  assert(g.scene==Scene::Cat);
  g.tap(90,285);
  assert(g.kitten.activity==PetActivity::Feed);
  for(int i=0;i<3;++i)g.tap(180,180);
  assert(g.kitten.done&&g.kitten.state.totalFeeds==1&&g.saveRevision>0);
  g.tap(180,40);
  assert(g.scene==Scene::Cat&&g.kitten.activity==PetActivity::Room);
  g.tap(180,40);
  assert(g.scene==Scene::Home);
  auto growth=g.kitten.state.growth;g.enter(Scene::Cat);
  assert(g.kitten.state.growth==growth);
  g.kitten.state.completedMask=15;g.kitten.state.totalPlayHits=5;g.kitten.signal=PetSignal::Complete;g.syncPet();
  assert((g.album&33)==33);
  g.enter(Scene::Drive);g.tap(90,175);
  assert(g.routeSelected&&g.route==0);
  g.tap(85,250);g.tick(20,1,true);
  assert(g.target==85);
  for(int i=0;i<95;++i)g.tick(20,1,true);
  assert(g.target==282);
  g.startRoute(0);collect(g,Pickup::Rock);
  assert(g.stars==0&&g.slowUntil>g.age);
  collect(g,Pickup::Magnet);
  assert(g.magnetUntil>g.age);
  for(int i=0;i<5;++i)collect(g,Pickup::Star);
  assert(g.won&&(g.album&2));g.tap(230,300);
  assert(g.route==1);
  for(int i=0;i<3;++i)collect(g,Pickup::Fuel);
  collect(g,Pickup::Fuel);
  assert(g.stars==3&&!g.won);
  collect(g,Pickup::Pod);
  assert(g.won&&(g.album&4));
  g.startRoute(2);collect(g,Pickup::Star,1);
  assert(g.stars==0);
  for(int i=0;i<5;++i)collect(g,Pickup::Star,g.wantedColor());
  assert(g.won&&(g.album&8));
  for(int i=0;i<20000;++i) {
    g.tick(20,float((i%100)-50)/50,true);
    if(i%25==0)g.tap((i*29)%360,240);
    if(g.won)g.startRoute((g.route+1)%3);
    assert(g.car>=78&&g.car<=282);
  }
  g.enter(Scene::Cake);
  for(int i=0;i<30;++i)g.tap(110+i%10*10,230);
  assert(g.toppingCount==10);
  g.tap(180,325);
  for(int pass=0;pass<2;++pass)for(int i=0;i<4;++i)g.tap(120+i*40,155);
  assert(g.cakePhase==CakePhase::Party&&g.takeSound()==Sound::Birthday&&(g.album&16));
  g.tap(180,160);
  assert(g.fireworks==1);
  for(int i=0;i<400;++i)g.tick(20);
  assert(!g.animated());
  g.tap(180,320);
  assert(g.cakePhase==CakePhase::Decorate&&g.takeSound()==Sound::Stop);
  assert(g.album==63);
  g.enter(Scene::Home);for(int i=0;i<8;++i)g.tick(50);
  assert(!g.animated());
  puts("PASS: pet navigation/persistent state/stickers, three missions, 20000-step driving, birthday and replay");
}
