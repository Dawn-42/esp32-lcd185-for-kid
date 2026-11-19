#include "SceneView.h"
#include "DirtyTiles.h"
#include <vector>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <chrono>
static std::vector<lv_color_t> screen(360*360);
static unsigned flushed=0;
static void flush(lv_disp_drv_t *d,const lv_area_t *a,lv_color_t *p) {
  for(int y=a->y1;y<=a->y2;++y) {
    memcpy(screen.data()+y*360+a->x1,p,(a->x2-a->x1+1)*2);
    p+=a->x2-a->x1+1;
  }
  flushed+=lv_area_get_size(a);lv_disp_flush_ready(d);
}
int main() {
  lv_init();static lv_color_t draw[360*36];static lv_disp_draw_buf_t buf;
  lv_disp_draw_buf_init(&buf,draw,nullptr,360*36);
  static lv_disp_drv_t d;lv_disp_drv_init(&d);d.hor_res=d.ver_res=360;d.draw_buf=&buf;d.flush_cb=flush;lv_disp_drv_register(&d);
  std::vector<lv_color_t> pixels(360*360),base(360*360),old(360*360),reference(360*360);
  assert(viewInit(pixels.data(),base.data(),old.data()));
  Game g;viewDraw(g,true);lv_refr_now(nullptr);
  assert(memcmp(screen.data(),pixels.data(),pixels.size()*2)==0);
  unsigned builds=viewStats().staticBuilds;flushed=0;
  viewDraw(g,true);lv_refr_now(nullptr);
  assert(viewStats().pixels==0&&flushed==0&&viewStats().staticBuilds==builds);
  puts("PASS: identical frame requests zero changed pixels and zero display transfers");

  auto equivalent=[&](Game &state) {
    viewDraw(state,true);reference=pixels;lv_refr_now(nullptr);
    assert(memcmp(screen.data(),pixels.data(),pixels.size()*2)==0);
    viewInvalidateCache();viewDraw(state,true);
    if(memcmp(reference.data(),pixels.data(),pixels.size()*2)!=0) {
      for(int y=0;y<360;++y)for(int x=0;x<360;++x)
        if(reference[y*360+x].full!=pixels[y*360+x].full) {
          fprintf(stderr,"CACHE MISMATCH scene=%u age=%u at %d,%d\n",unsigned(state.scene),state.age,x,y);
          abort();
        }
    }
    lv_refr_now(nullptr);
  };
  for(Scene scene:{Scene::Cat,Scene::Drive,Scene::Cake,Scene::Album,Scene::City}) {
    g.enter(scene);
    if(scene==Scene::Drive)g.startRoute(2);
    for(int n=0;n<140;++n) {
      if(scene==Scene::Cat) {
        if(n==3)g.kitten.begin(PetActivity::Feed);
        if(n>=4&&n<7)g.tap(180,180);
        if(n==10)g.kitten.begin(PetActivity::Bath);
        if(n==11)g.tap(128,162);if(n==12)g.tap(218,168);if(n==13)g.tap(139,220);if(n==14)g.tap(223,225);
        if(n>=16&&n<22)g.tap(180,300);
        if(n==30)g.kitten.begin(PetActivity::Play);
        if(n>=33&&n<38)g.tap(g.kitten.ballX(),g.kitten.ballY());
        if(n==45) { g.kitten.state.wasteMask=7;g.kitten.begin(PetActivity::Litter); }
        if(n>=46&&n<49)g.tap(120+(n-46)*60,218);
        if(n==55)g.kitten.begin(PetActivity::Sleep);
        if(n==85) { g.kitten.state.growth=60;g.kitten.begin(PetActivity::Room); }
        if(n==100) { g.kitten.state.growth=180;g.kitten.changed(); }
        if(n==115)g.tap(180,200);
        if(n==125) { g.kitten.state.satiety=100;g.kitten.begin(PetActivity::Feed); }
      }
      if(scene==Scene::City&&n==20)g.tap(180,300);
      if(scene==Scene::Cake) {
        if(n<10)g.tap(110+n*14,230);
        if(n==10)g.tap(180,325);
        if(n>=20&&n<24)g.tap(120+(n-20)*40,155);
        if(n>=30&&n<34)g.tap(120+(n-30)*40,155);
        if(n==40||n==90)g.tap(85+n,130+n/3);
      }
      g.tick(50,float(n%80-40)/30,true);equivalent(g);
    }
  }
  g.enter(Scene::Cat);g.age=1000;
  for(unsigned n=0;n<350;++n) {
    if(n==80)g.tap(90,253);
    if(n==170)g.tap(g.kitten.walk.x,g.kitten.walk.y-g.kitten.roomSize()/2);
    if(n==200)g.kitten.state.growth=180;
    g.tick(60);equivalent(g);
  }
  g.enter(Scene::Cat);g.age=1000;
  for(unsigned mask:{0u,1u,2u,4u,15u,32u}) {
    g.kitten.state.completedMask=mask;equivalent(g);
    for(unsigned album:{0u,2u,4u,8u,16u,63u}) { g.album=album;equivalent(g); }
  }
  g.enter(Scene::City);g.age=1000;
  for(unsigned n=0;n<PixelCity::Ticks+2;++n) { g.tick(50);equivalent(g); }
  puts("PASS: incremental frames equal full recomposition; actual LVGL partial flushes reconstruct every frame");

  g=Game{};g.enter(Scene::Drive);g.startRoute(0);g.age=400;viewDraw(g,true);lv_refr_now(nullptr);
  unsigned frames=240;uint64_t transfer=0,dirty=0;
  builds=viewStats().staticBuilds;
  auto started=std::chrono::steady_clock::now();
  for(unsigned n=0;n<frames;++n) {
    g.tick(33,float(int(n%80)-40)/70,true);flushed=0;
    viewDraw(g,true);dirty+=viewStats().pixels;lv_refr_now(nullptr);transfer+=flushed;
  }
  auto us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count();
  double ratio=double(transfer)/(frames*129600);
  assert(ratio<.75);
  printf("BENCH drive frames=%u avg_changed_pixels=%llu avg_actual_flush_pixels=%llu full_frame_ratio=%.3f host_frame_us=%lld static_rebuilds=%u\n",
    frames,(unsigned long long)(dirty/frames),(unsigned long long)(transfer/frames),ratio,(long long)(us/frames),viewStats().staticBuilds-builds);
  g=Game{};g.enter(Scene::City);g.age=1000;viewDraw(g,true);lv_refr_now(nullptr);
  transfer=dirty=0;frames=PixelCity::Ticks;builds=viewStats().staticBuilds;
  started=std::chrono::steady_clock::now();
  for(unsigned n=0;n<frames;++n) {
    g.tick(50);flushed=0;viewDraw(g,true);dirty+=viewStats().pixels;
    lv_refr_now(nullptr);transfer+=flushed;
  }
  us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count();
  ratio=double(transfer)/(frames*129600);assert(ratio<.70);
  printf("BENCH city frames=%u avg_changed_pixels=%llu avg_actual_flush_pixels=%llu full_frame_ratio=%.3f host_frame_us=%lld static_rebuilds=%u\n",
    frames,(unsigned long long)(dirty/frames),(unsigned long long)(transfer/frames),ratio,(long long)(us/frames),viewStats().staticBuilds-builds);

  g=Game{};g.enter(Scene::Cat);g.age=1000;viewDraw(g,true);lv_refr_now(nullptr);
  transfer=dirty=0;frames=480;builds=viewStats().staticBuilds;
  started=std::chrono::steady_clock::now();
  for(unsigned n=0;n<frames;++n) {
    g.tick(60);flushed=0;viewDraw(g,true);dirty+=viewStats().pixels;
    lv_refr_now(nullptr);transfer+=flushed;
  }
  us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count();
  ratio=double(transfer)/(frames*129600);assert(ratio<.75);
  printf("BENCH room frames=%u avg_changed_pixels=%llu avg_actual_flush_pixels=%llu full_frame_ratio=%.3f host_frame_us=%lld static_rebuilds=%u\n",
    frames,(unsigned long long)(dirty/frames),(unsigned long long)(transfer/frames),ratio,(long long)(us/frames),viewStats().staticBuilds-builds);

}
