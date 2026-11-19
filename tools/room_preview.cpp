// A 36-second child-flow demo: need, feeding, flowers, call, city souvenir and bath.
#include "SceneView.h"
#include <vector>
#include <cstdio>
static void flush(lv_disp_drv_t *d,const lv_area_t *,lv_color_t *) { lv_disp_flush_ready(d); }
int main(int argc,char **) {
  lv_init();static lv_color_t draw[360*36];static lv_disp_draw_buf_t buf;
  lv_disp_draw_buf_init(&buf,draw,nullptr,360*36);
  static lv_disp_drv_t d;lv_disp_drv_init(&d);d.hor_res=d.ver_res=360;d.draw_buf=&buf;d.flush_cb=flush;lv_disp_drv_register(&d);
  std::vector<lv_color_t> pixels(360*360),base(360*360),previous(360*360);
  if(!viewInit(pixels.data(),base.data(),previous.data()))return 1;
  Game g;g.enter(Scene::Cat);g.age=1000;g.kitten.state.wasteMask=0;
  if(argc>1) { g.kitten.state.completedMask=15;g.kitten.state.growth=60;g.album=18; }
  std::vector<unsigned char> rgb(360*360*3);
  for(unsigned frame=0;frame<(argc>1?1u:600u);++frame) {
    if(argc==1) {
      if(frame==25)g.tap(275,89);
      if(frame==45||frame==65||frame==85)g.tap(180,238);
      if(frame==110)g.tap(180,310);
      if(frame==145)g.tap(90,250);
      if(frame==225)g.tap(g.kitten.walk.x,g.kitten.walk.y-g.kitten.roomSize()/2);
      if(frame==260)g.enter(Scene::City);
      if(frame==280)g.tap(180,300);
      if(frame==325)g.tap(180,40);
      if(frame==340)g.tap(180,140);
      if(frame==370)g.tap(150,285);
      const int bx[]={128,218,139,223},by[]={162,168,220,225};
      for(unsigned i=0;i<4;++i)if(frame==395+i*12)g.tap(bx[i],by[i]);
      if(frame==450||frame==460||frame==470||frame==480||frame==490||frame==500)g.tap(180,310);
      if(frame==525)g.tap(180,310);
    }
    g.tick(60);viewDraw(g,true);lv_refr_now(nullptr);
    for(int y=0;y<360;++y)for(int x=0;x<360;++x) {
      lv_color32_t c;c.full=lv_color_to32(pixels[y*360+x]);
      bool outside=(x-180)*(x-180)+(y-180)*(y-180)>180*180;unsigned n=(y*360+x)*3;
      rgb[n]=outside?255:c.ch.red;rgb[n+1]=outside?246:c.ch.green;rgb[n+2]=outside?223:c.ch.blue;
    }
    printf("P6\n360 360\n255\n");if(fwrite(rgb.data(),1,rgb.size(),stdout)!=rgb.size())return 2;
  }
}
