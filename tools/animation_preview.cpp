// Three independent pet saves: infant / young / adult. This does not accelerate growth.
#include "SceneView.h"
#include <vector>
#include <cstdio>
#include <cmath>
static void flush(lv_disp_drv_t *d,const lv_area_t *,lv_color_t *) { lv_disp_flush_ready(d); }
int main() {
  lv_init();static lv_color_t draw[360*36];static lv_disp_draw_buf_t buf;
  lv_disp_draw_buf_init(&buf,draw,nullptr,360*36);
  static lv_disp_drv_t d;lv_disp_drv_init(&d);d.hor_res=d.ver_res=360;d.draw_buf=&buf;d.flush_cb=flush;lv_disp_drv_register(&d);
  std::vector<lv_color_t> pixels(360*360),base(360*360),previous(360*360);
  if(!viewInit(pixels.data(),base.data(),previous.data()))return 1;
  Game scenes[3];unsigned ages[]={0,60,180};
  for(unsigned i=0;i<3;++i) { scenes[i].enter(Scene::Cat);scenes[i].age=400;scenes[i].kitten.state.growth=ages[i]; }
  scenes[2].kitten.state.wasteMask=7;
  std::vector<lv_color_t> held[3];for(auto &p:held)p.resize(360*360);
  uint32_t revisions[3]={},lastRender[3]={};
  std::vector<unsigned char> rgb(1080*360*3);
  const int sx[]={128,218,139,223},sy[]={162,168,220,225};
  for(unsigned frame=0;frame<1000;++frame) {
    unsigned time=frame*20;
    for(auto &g:scenes)g.tick(20);
    auto &baby=scenes[0];auto &young=scenes[1];auto &adult=scenes[2];
    if(frame==100) { baby.tap(90,285);young.tap(150,285);adult.tap(270,285); }
    if(frame==140||frame==180||frame==220)baby.tap(180,185);
    if(frame==260||frame==600)baby.tap(180,300);
    if(frame==320)baby.tap(210,285);
    if(frame==360||frame==410||frame==460||frame==510||frame==560)
      baby.tap(baby.kitten.ballX(),baby.kitten.ballY());
    if(frame==700)baby.tap(279,180);
    for(unsigned i=0;i<4;++i)if(frame==140+i*35)young.tap(sx[i],sy[i]);
    if(frame==300||frame==340||frame==380||frame==440||frame==480||frame==520)young.tap(180,300);
    if(frame==560)young.tap(180,300);
    if(frame==650)young.tap(180,185);
    if(frame==750)young.tap(279,180);
    for(unsigned i=0;i<3;++i)if(frame==150+i*60)adult.tap(120+i*60,218);
    if(frame==330||frame==600)adult.tap(180,300);
    if(frame==430)adult.tap(90,285);
    if(frame==460||frame==500||frame==540)adult.tap(180,185);
    if(frame==700)adult.tap(180,190);
    if(frame==780)adult.tap(279,180);
    for(unsigned i=0;i<3;++i) {
      auto &g=scenes[i];
      if(frame==0||g.revision!=revisions[i]||(g.animated()&&time-lastRender[i]>=g.frameInterval())) {
        viewDraw(g,true);lv_refr_now(nullptr);held[i]=pixels;
        lastRender[i]=time;revisions[i]=g.revision;
      }
    }
    for(unsigned y=0;y<360;++y)for(unsigned panel=0;panel<3;++panel)for(unsigned x=0;x<360;++x) {
      lv_color32_t c;c.full=lv_color_to32(held[panel][y*360+x]);
      unsigned n=(y*1080+panel*360+x)*3;
      bool outside=(int(x)-180)*(int(x)-180)+(int(y)-180)*(int(y)-180)>180*180;
      rgb[n]=outside?9:c.ch.red;rgb[n+1]=outside?14:c.ch.green;rgb[n+2]=outside?25:c.ch.blue;
    }
    printf("P6\n1080 360\n255\n");
    if(fwrite(rgb.data(),1,rgb.size(),stdout)!=rgb.size())return 2;
  }
}
