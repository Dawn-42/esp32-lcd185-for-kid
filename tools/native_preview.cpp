// Render the actual firmware UI on a virtual LVGL display, no board required.
#include "SceneView.h"
#include <vector>
#include <fstream>
#include <string>
static void flush(lv_disp_drv_t *d,const lv_area_t *,lv_color_t *) { lv_disp_flush_ready(d); }
static void save(const std::string &path,const std::vector<lv_color_t> &pixels) {
  std::ofstream f(path,std::ios::binary); f<<"P6\n360 360\n255\n";
  for(int y=0;y<360;++y) for(int x=0;x<360;++x) {
    lv_color32_t c; c.full=lv_color_to32(pixels[y*360+x]);
    unsigned char rgb[]={c.ch.red,c.ch.green,c.ch.blue};
    if((x-180)*(x-180)+(y-180)*(y-180)>180*180) rgb[0]=rgb[1]=rgb[2]=10;
    f.write(reinterpret_cast<char *>(rgb),3);
  }
}
int main(int argc,char **argv) {
  std::string out=argc>1?argv[1]:"dist";
  lv_init(); static lv_color_t draw[360*10]; static lv_disp_draw_buf_t buf;
  lv_disp_draw_buf_init(&buf,draw,nullptr,360*10);
  static lv_disp_drv_t d; lv_disp_drv_init(&d); d.hor_res=d.ver_res=360; d.draw_buf=&buf; d.flush_cb=flush; lv_disp_drv_register(&d);
  std::vector<lv_color_t> pixels(360*360),base(360*360),previous(360*360);
  viewInit(pixels.data(),base.data(),previous.data());
  auto shot=[&](const char *name,Game &g) { if(g.age<400)g.age=400;viewDraw(g,true);save(out+"/"+name+".ppm",pixels); };
  Game g;g.album=3;shot("01-home",g);
  g.enter(Scene::Cat);g.tap(90,285);g.tap(180,185);shot("02-cat",g);
  g.enter(Scene::Drive);g.startRoute(0);g.stars=2;g.car=145;
  g.items[0]={145,187,Pickup::Star,0,true};g.items[1]={235,155,Pickup::Magnet,0,true};g.items[2]={250,229,Pickup::Rock,0,true};
  shot("03-drive",g);
  g.enter(Scene::Cake);
  g.tap(135,225);g.style=1;g.tap(190,225);g.style=2;g.tap(239,225);shot("04-cake",g);
  g.cakePhase=CakePhase::Party;g.candles=15;g.fireworks=1;g.burst(180,152);g.tick(80);shot("05-birthday",g);
  g.enter(Scene::Cat);g.kitten.begin(PetActivity::Play);g.syncPet();shot("06-cat-ball",g);
  g.kitten.begin(PetActivity::Sleep);g.syncPet();shot("07-cat-sleep",g);
  g.enter(Scene::Drive);shot("08-routes",g);
  g.startRoute(1);g.stars=3;g.items[0]={180,181,Pickup::Pod,0,true};g.items[1]={110,222,Pickup::Fuel,0,true};shot("09-mars",g);
  g.startRoute(2);g.items[0]={180,181,Pickup::Star,0,true};g.items[1]={110,222,Pickup::Star,1,true};shot("10-rings",g);
  g.enter(Scene::Cake);g.cakePhase=CakePhase::Wish;g.candles=1;shot("11-candles",g);
  g.enter(Scene::Album);g.album=63;g.selectedSticker=3;shot("12-album",g);
  g.enter(Scene::Cat);g.kitten.state=PetState{};g.kitten.begin(PetActivity::Room);g.syncPet();shot("13-pet-room",g);
  g.kitten.begin(PetActivity::Bath);g.syncPet();shot("14-pet-bubbles",g);
  for(auto point:{lv_point_t{128,162},lv_point_t{218,168},lv_point_t{139,220},lv_point_t{223,225}})g.tap(point.x,point.y);
  shot("15-pet-rinse",g);
  for(int i=0;i<3;++i)g.tap(180,300);shot("16-pet-dry",g);
  g.kitten.state.wasteMask=7;g.kitten.begin(PetActivity::Litter);g.syncPet();shot("17-pet-litter",g);
  g.kitten.begin(PetActivity::Room);g.kitten.state.growth=60;g.kitten.changed();g.syncPet();shot("18-pet-young",g);
  g.kitten.state.growth=180;g.kitten.changed();g.syncPet();shot("19-pet-adult",g);
  g.kitten.state.satiety=100;g.kitten.begin(PetActivity::Feed);g.syncPet();shot("20-pet-full",g);

}
