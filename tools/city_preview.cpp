// Export one exact 24-second period from the firmware renderer as PPM frames.
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
  Game g;g.enter(Scene::City);g.age=1000;
  std::vector<unsigned char> rgb(360*360*3);
  for(unsigned frame=0;frame<(argc>1?1:PixelCity::Ticks);++frame) {
    g.cityTime=frame*PixelCity::FrameMs;viewDraw(g,true);lv_refr_now(nullptr);
    for(int y=0;y<360;++y)for(int x=0;x<360;++x) {
      lv_color32_t c;c.full=lv_color_to32(pixels[y*360+x]);
      bool outside=(x-180)*(x-180)+(y-180)*(y-180)>180*180;
      unsigned n=(y*360+x)*3;
      rgb[n]=outside?255:c.ch.red;rgb[n+1]=outside?246:c.ch.green;rgb[n+2]=outside?223:c.ch.blue;
    }
    printf("P6\n360 360\n255\n");
    if(fwrite(rgb.data(),1,rgb.size(),stdout)!=rgb.size())return 2;
  }
}
