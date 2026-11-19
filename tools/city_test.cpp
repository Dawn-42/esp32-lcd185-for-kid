#include "Game.h"
#include <vector>
#include <cassert>
#include <cstdio>
int main() {
  Game g;g.tap(240,85);assert(g.scene==Scene::City&&g.animated());g.age=1000;
  assert(g.frameInterval()==50);
  for(unsigned i=0;i<6;++i) {
    const auto &l=PixelCity::landmarks[i];
    g.tap((l.x+l.w/2)*2,(l.y+4)*2);assert(g.cityLandmark==i);
    g.tap(180,244);assert(g.cityLandmark==6);
    assert(PixelCity::selection(i*4000,6)==i);
  }
  g.cityTime=23980;g.tick(20);assert(g.cityTime==0);
  for(unsigned n=0;n<100000;++n)g.tick(20);
  assert(g.age==1000&&g.cityTime<24000); // No fade or clock overflow during long playback.
  g.tap(180,30);assert(g.scene==Scene::Home);g.tap(100,85);assert(g.scene==Scene::Album);
  std::vector<uint16_t> a(360*360+2,0xBEEF),b(a);
  for(unsigned tick=0;tick<480;++tick) {
    for(unsigned lane=0;lane<7;++lane)for(unsigned i=0;i<4;++i) {
      assert(PixelCity::vehiclePhase(tick,lane,i)==PixelCity::vehiclePhase(tick+480,lane,i));
      assert((PixelCity::vehiclePhase(tick,lane,(i+1)%4)+240-PixelCity::vehiclePhase(tick,lane,i))%240==60);
    }
    PixelCity::paintBase(a.data()+1,PixelCity::selection(tick*50,6));PixelCity::paintMotion(a.data()+1,tick*50);
    PixelCity::paintBase(b.data()+1,PixelCity::selection((tick+480)*50,6));PixelCity::paintMotion(b.data()+1,(tick+480)*50);
    assert(a==b&&a.front()==0xBEEF&&a.back()==0xBEEF);
  }
  puts("PASS: six landmark taps, home navigation, bounded clock, 480 periodic frames, vehicle spacing and buffer guards");
}
