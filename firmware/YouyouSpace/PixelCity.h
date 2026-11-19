#pragma once
#include <stdint.h>
// A 180x180 logical pixel scene, drawn at exactly 2x into the existing RGB565 canvas.
// No textures, decoder, new frame buffers or per-frame allocations.
namespace PixelCity {
constexpr unsigned Width=180,Height=180,Scale=2,FrameMs=50,LoopMs=24000,Ticks=LoopMs/FrameMs;
constexpr unsigned LandmarkCount=6;
struct Landmark { const char *name;int x,y,w,h; };
static const Landmark landmarks[]={
  {"平安金融中心",69,40,17,71},
  {"京基100",92,52,17,59},
  {"地王大厦",112,60,18,51},
  {"华润春笋",134,59,25,52},
  {"深圳市民中心",49,100,57,17},
  {"深圳湾春茧",22,90,35,21}
};
constexpr uint16_t rgb(uint32_t c) { return ((c>>19)&31)<<11|((c>>10)&63)<<5|((c>>3)&31); }
struct Canvas {
  uint16_t *p;
  void box(int x,int y,int w,int h,uint32_t color) {
    int x0=x<0?0:x,y0=y<0?0:y,x1=x+w>180?180:x+w,y1=y+h>180?180:y+h;
    if(x0>=x1||y0>=y1)return;
    uint16_t c=rgb(color);
    for(int yy=y0*2;yy<y1*2;++yy) {
      uint16_t *row=p+yy*360+x0*2;
      for(int xx=x0*2;xx<x1*2;++xx)*row++=c;
    }
  }
  void line(int x,int y,int endX,int endY,uint32_t c) {
    int dx=endX>x?endX-x:x-endX,dy=endY>y?y-endY:endY-y;
    int sx=x<endX?1:-1,sy=y<endY?1:-1,e=dx+dy;
    for(;;) {
      box(x,y,1,1,c);
      if(x==endX&&y==endY)break;
      int d=e*2;
      if(d>=dy) { e+=dy;x+=sx; }
      if(d<=dx) { e+=dx;y+=sy; }
    }
  }
  void oval(int x,int y,int rx,int ry,uint32_t c,bool outline=false) {
    for(int yy=-ry;yy<=ry;++yy)for(int xx=-rx;xx<=rx;++xx) {
      int d=xx*xx*ry*ry+yy*yy*rx*rx,limit=rx*rx*ry*ry;
      if(d<=limit&&(!outline||d>(rx-1)*(rx-1)*(ry-1)*(ry-1)))box(x+xx,y+yy,1,1,c);
    }
  }
};
inline unsigned selection(uint32_t clock,unsigned pinned) {
  return pinned<LandmarkCount?pinned:(clock%LoopMs)/4000;
}
inline int hit(int screenX,int screenY) {
  int x=screenX/2,y=screenY/2;
  // Foreground civic roof takes precedence over tower foundations.
  const unsigned order[]={4,5,0,1,2,3};
  for(unsigned i:order) {
    const auto &b=landmarks[i];
    if(x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+b.h)return int(i);
  }
  return -1;
}
inline void windows(Canvas &c,int x,int y,int w,int h,unsigned seed) {
  for(int yy=y;yy<y+h;yy+=4)for(int xx=x;xx<x+w;xx+=4) {
    unsigned v=(xx*7+yy*11+seed*17)%11;
    c.box(xx,yy,2,2,v<4?0xFFF5CA:v<8?0x8DD1DA:0xA3BDD8);
  }
}
inline void palm(Canvas &c,int x,int y) {
  c.box(x,y-9,2,10,0xC8947B);
  c.line(x,y-9,x-6,y-12,0x71CFA5);c.line(x,y-9,x+6,y-12,0x71CFA5);
  c.line(x,y-9,x-5,y-7,0x4BB88F);c.line(x,y-9,x+5,y-7,0x4BB88F);
  c.line(x,y-10,x,y-15,0xACEDBF);
}
inline int bridgeY(int x) { return 156-x/9; }
inline void bridge(Canvas &c) {
  for(int x=4;x<177;++x) {
    int y=bridgeY(x);
    c.box(x,y+5,1,3,0x9EAAC6);
    c.box(x,y,1,7,0xD2CBE9);
    c.box(x,y+1,1,1,0xFFF7DD);
    c.box(x,y+2,1,4,0xA5A6CD);
    c.box(x,y+6,1,1,0xE7E5F5);
    if(x%14<7)c.box(x,y+4,1,1,0xFFF0B0);
  }
}
inline void paintBase(uint16_t *pixels,unsigned selected) {
  Canvas c{pixels};
  static const uint32_t sky[]={0xA9DEEF,0xB7E6F2,0xC6EDF2,0xD8F2ED,0xE4F4E9,0xF4F3DF,0xFFF0DB};
  for(int y=0;y<180;++y)c.box(0,y,180,1,y<112?sky[y*7/112]:0xC4E7DA);
  // Sunny candy palette; clouds are separate moving sprites.
  c.oval(33,50,13,13,0xFFE69A);c.box(19,52,29,2,0xFFD37F);c.box(20,58,25,2,0xFFC683);
  c.box(28,47,2,2,0xA37B65);c.box(36,47,2,2,0xA37B65);
  c.box(31,52,5,1,0xC08A6D);c.box(32,53,3,1,0xC08A6D);
  c.box(25,51,3,1,0xFFBC9D);c.box(39,51,3,1,0xFFBC9D);
  for(int i=0;i<24;++i) {
    int x=(i*47+13)%180,y=(i*23+7)%72;
    if((x<60||x>120)&&y<55)c.box(x,y,i%6?1:2,1,i%3?0xFFF7D9:0xFFFFFF);
  }
  // Mountain silhouette behind the skyline.
  for(int x=0;x<180;++x) {
    int h=9+(x*7%21)/3;c.box(x,111-h,1,h,0xB7D9CE);
  }
  for(int i=0;i<18;++i) {
    int x=i*11-5,h=10+(i*17)%28,w=7+(i%3);
    c.box(x,111-h,w,h,0xBBDAE6);c.box(x,111-h,1,h,0xDFF0EE);
    windows(c,x+2,114-h,w-3,h-4,i);
  }
  // Bay and stepped reflections.
  for(int y=111;y<128;++y)c.box(0,y,180,1,y<117?0x90D9DF:0xAFE8E2);
  for(int x=7;x<180;x+=11) {
    c.box(x,114,5,1,0xFFF2CE);c.box(x+2,118,3,1,0xD8F5E9);
  }
  // Ping An Finance Center: tapered, ribbed needle crown.
  uint32_t edge=selected==0?0xFF8B9E:0xE8FFF6;
  c.line(77,40,77,47,edge);
  for(int y=47;y<111;++y) {
    int half=1+(y-47)*7/64;
    c.box(77-half,y,half*2+1,1,0x77BBCF);
    c.box(77-half,y,1,1,edge);c.box(77+half,y,1,1,0xA6DFE3);
    if(y%4==0)c.box(77-half+2,y,half*2-3,1,0xD6F5EF);
  }
  c.line(76,49,76,110,0xF2FFF4);c.line(79,57,81,110,0x549EB9);
  // KK100: curved crown and tall blue glass facade.
  edge=selected==1?0xFF8B9E:0xE9FFF6;
  for(int y=53;y<111;++y) {
    int inset=y<62?(62-y)/3:0;
    c.box(93+inset,y,14-inset,1,0x8CBFD9);c.box(93+inset,y,1,1,edge);c.box(106,y,1,1,edge);
    if(y%4==0)c.box(96,y,8,1,0xDDF3F8);
  }
  c.line(100,54,104,60,0xFFF7DE);c.line(101,62,101,110,0x6CA3C4);
  // Diwang: green stepped crown and twin antennae.
  edge=selected==2?0xFF8B9E:0xF0FFE3;
  c.line(116,60,116,68,edge);c.line(124,60,124,68,edge);
  c.box(114,69,13,42,0x7FC4AC);c.box(117,65,7,5,0x9EDBC0);
  c.box(112,74,17,3,0xAFE5C9);c.box(114,69,1,42,edge);c.box(126,69,1,42,edge);
  for(int y=79;y<109;y+=4)c.box(116,y,9,1,0xF2F6CC);
  c.line(120,69,120,110,0x5BAF9D);
  // China Resources "Spring Bamboo": rounded tapered body and diagonal lattice.
  edge=selected==3?0xFF8B9E:0xF2F8FF;
  for(int y=61;y<111;++y) {
    int d=y-61,half=d<9?1+d/2:d>40?9-(d-40)/2:10;
    c.box(146-half,y,half*2+1,1,0xA8BDE3);
    c.box(146-half,y,1,1,edge);c.box(146+half,y,1,1,edge);
    if(y%4==0)c.box(146-half+2,y,half*2-3,1,0xDDE6FA);
    for(int x=146-half+2;x<146+half-1;++x)if((x+y)%6==0||(x-y+180)%6==0)c.box(x,y,1,1,0xF9F2FF);
  }
  c.box(146,59,1,3,edge);
  // Shenzhen Bay Sports Center "Spring Cocoon": a low white woven shell.
  edge=selected==5?0xFF8B9E:0xFFFBE7;
  c.oval(38,102,16,10,edge);c.oval(38,103,14,8,0x9BD4D4);
  for(int y=95;y<109;++y)for(int x=24;x<53;++x)
    if((x-38)*(x-38)*64+(y-103)*(y-103)*196<12544&&((x+y)%4==0||(x-y+180)%4==0))
      c.box(x,y,1,1,0xF9FFF1);
  c.box(22,109,34,3,0x82B6C4);
  // Shenzhen Civic Center: broad red roof shaped like spread wings.
  c.box(54,108,47,8,0xFFE2B6);c.box(58,109,39,5,0x9AD0DC);
  for(int x=59;x<98;x+=6)c.box(x,108,2,8,0xFFF1CF);
  const int rx[]={49,61,77,94,105},ry[]={103,101,106,101,103};
  for(int i=0;i<4;++i) {
    c.line(rx[i],ry[i],rx[i+1],ry[i+1],0xFF99A0);
    c.line(rx[i],ry[i]+1,rx[i+1],ry[i+1]+1,0xE7788A);
    if(selected==4)c.line(rx[i],ry[i]-1,rx[i+1],ry[i+1]-1,0xFF8B9E);
  }
  c.box(51,115,52,2,0xC7DDCE);
  // Promenade and palms.
  c.box(0,123,180,3,0xBDDCCB);c.box(0,125,180,1,0xEFF6DC);
  palm(c,13,123);palm(c,165,123);
  // Three stacked roads with six lanes. All lamps / markings stay cached.
  for(int road=0;road<3;++road) {
    int y=128+road*20;c.box(0,y,180,17,0xB1B8D7);
    c.box(0,y,180,1,0xF9F7E4);c.box(0,y+16,180,1,0x929DBE);
    for(int x=0;x<180;x+=12)c.box(x+2,y+8,6,1,0xFFF5CA);
    for(int x=18;x<180;x+=43) { c.box(x,y-7,1,7,0x85A3B6);c.box(x-2,y-7,5,1,0xFFF2B3); }
  }
  c.box(31,147,3,19,0xB6B8D4);c.box(138,135,3,30,0xB6B8D4);
  bridge(c);
}
inline unsigned vehiclePhase(unsigned tick,unsigned lane,unsigned index) {
  static const unsigned speeds[]={1,1,2,2,3,3,2};
  return (tick*speeds[lane]+index*60+lane*19)%240;
}
inline void vehicle(Canvas &c,int x,int y,unsigned kind,unsigned colorIndex,bool right,unsigned tick) {
  static const uint32_t colors[]={0xFF8F9D,0x78D9C1,0xFFD16F,0xB09BE8,0xFFB99C,0x68C5E1};
  uint32_t body=colors[colorIndex%6],glass=0x5388AB;
  int w=kind==2?21:kind==3?15:13,h=kind>=2?7:6;
  c.box(x+1,y+h,w-1,1,0x8E9CB8);c.box(x,y+2,w, h-2,body);
  if(kind==2) {
    c.box(x+1,y,w-2,2,0xFFF0CD);
    for(int i=0;i<4;++i)c.box(x+2+i*4,y+2,3,2,glass);
    c.box(x+2,y+5,w-4,1,0x62B3B6);
  } else if(kind==3) {
    c.box(x+2,y,w-5,3,body);c.box(x+w-4,y+1,3,3,glass);
    c.box(x+3,y+2,4,1,0xFFEBD2);
  } else {
    c.box(x+3,y,w-6,3,body);c.box(x+4,y+1,4,2,glass);
    c.box(x+1,y+3,w-2,1,0xFFF8E8);
    if(kind==1) { c.box(x+5,y-1,3,1,0xFFE491);c.box(x+3,y+4,1,1,0x5F7899);c.box(x+5,y+4,1,1,0x5F7899); }
  }
  c.box(x+2,y+h-1,3,2,0x5B7190);c.box(x+w-5,y+h-1,3,2,0x5B7190);
  if((tick/2)%2) { c.box(x+3,y+h,1,1,0xC8D9E7);c.box(x+w-4,y+h,1,1,0xC8D9E7); }
  c.box(right?x+w-1:x,y+3,1,2,0xFFFBE0);c.box(right?x:x+w-1,y+4,1,1,0xEF7B98);
}
inline void cloud(Canvas &c,int x,int y) {
  c.box(x+4,y,15,2,0xFFF7E6);c.box(x,y+2,25,3,0xFFFEF5);
  c.box(x+3,y+5,19,1,0xF2C9CB);
}
inline void paintMotion(uint16_t *pixels,uint32_t milliseconds) {
  Canvas c{pixels};unsigned tick=(milliseconds%LoopMs)/FrameMs;
  cloud(c,int((tick/2+15)%240)-30,21);cloud(c,int((tick/2+138)%240)-30,28);
  for(unsigned i=0;i<8;++i) {
    int x=int((tick/4+i*17)%120)+30;
    c.box(x,118+(i%3)*2,4+i%3,1,i%2?0xDBFFF2:0xF8FFE7);
  }
  for(unsigned lane=0;lane<6;++lane)for(unsigned i=0;i<4;++i) {
    bool right=lane%2==0;unsigned phase=vehiclePhase(tick,lane,i);
    int x=right?int(phase)-30:210-int(phase),y=130+(lane/2)*20+(lane%2)*8;
    vehicle(c,x,y,(lane+i)%4,lane*3+i,right,tick);
  }
  // Elevated ramp occludes the cars beneath it, then carries its own stream.
  bridge(c);
  for(unsigned i=0;i<4;++i) {
    int x=int(vehiclePhase(tick,6,i))-30;
    vehicle(c,x,bridgeY(x)-4,i%3,i+1,true,tick);
  }
}
}
