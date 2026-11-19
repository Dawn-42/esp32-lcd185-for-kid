#include "SceneView.h"
#include "DirtyTiles.h"
#include "ArtAssets.h"
#include "PetAssets.h"
#include "WalkAssets.h"
#include "Motion.h"
#include "PixelRoom.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
LV_FONT_DECLARE(youyou_font);
LV_FONT_DECLARE(youyou_small);
static_assert(sizeof(lv_color_t)==2,"This renderer requires RGB565");
static lv_obj_t *canvas;
static uint16_t *pixels,*background,*previous;
static uint64_t cachedKey=UINT64_MAX;
static ViewStats stats;
static bool wasEntering=false;
static constexpr uint32_t palette[]={0xFFD36B,0xFF92B3,0x8CE1DE};
static constexpr uint32_t BG=0x0D1830, INK=0x25344A, WHITE=0xFFF4DA;
static lv_color_t color(uint32_t c) { return lv_color_hex(c); }
static void rect(int x,int y,int w,int h,uint32_t c,int r=0) {
  if(w<=0||h<=0)return;
  lv_draw_rect_dsc_t d; lv_draw_rect_dsc_init(&d);
  d.bg_color=color(c); d.bg_opa=LV_OPA_COVER; d.radius=r;
  lv_canvas_draw_rect(canvas,x,y,w,h,&d);
}
static void circle(int x,int y,int r,uint32_t c) { rect(x-r,y-r,r*2,r*2,c,r); }
static void line(int x,int y,int xx,int yy,uint32_t c,int width=3) {
  lv_point_t p[2]={{(lv_coord_t)x,(lv_coord_t)y},{(lv_coord_t)xx,(lv_coord_t)yy}};
  lv_draw_line_dsc_t d; lv_draw_line_dsc_init(&d); d.color=color(c); d.width=width;
  d.round_start=d.round_end=1; lv_canvas_draw_line(canvas,p,2,&d);
}
static void triangle(int x,int y,int xx,int yy,int xxx,int yyy,uint32_t c) {
  lv_point_t p[3]={{(lv_coord_t)x,(lv_coord_t)y},{(lv_coord_t)xx,(lv_coord_t)yy},{(lv_coord_t)xxx,(lv_coord_t)yyy}};
  lv_draw_rect_dsc_t d; lv_draw_rect_dsc_init(&d); d.bg_color=color(c); d.bg_opa=LV_OPA_COVER;
  lv_canvas_draw_polygon(canvas,p,3,&d);
}
static void text(const char *s,int y,uint32_t c=WHITE,int x=0,int w=360) {
  lv_draw_label_dsc_t d; lv_draw_label_dsc_init(&d);
  d.color=color(c); d.font=&youyou_font; d.align=LV_TEXT_ALIGN_CENTER;
  lv_canvas_draw_text(canvas,x,y,w,&d,s);
}
static void smallText(const char *str,int y,uint32_t c=WHITE,int x=0,int w=360) {
  lv_draw_label_dsc_t d;lv_draw_label_dsc_init(&d);
  d.color=color(c);d.font=&youyou_small;d.align=LV_TEXT_ALIGN_CENTER;
  lv_canvas_draw_text(canvas,x,y,w,&d,str);
}
static void star(int x,int y,int r,uint32_t c) {
  static const int ux[]={0,26,95,43,59,0,-59,-43,-95,-26};
  static const int uy[]={-100,-36,-31,14,81,45,81,14,-31,-36};
  for(int i=0;i<10;++i) {
    int j=(i+1)%10;
    triangle(x,y,x+ux[i]*r/100,y+uy[i]*r/100,x+ux[j]*r/100,y+uy[j]*r/100,c);
  }
}
static void heart(int x,int y,int r) {
  circle(x-r/2,y,r/2,0xFF819D); circle(x+r/2,y,r/2,0xFF819D);
  triangle(x-r,y,x+r,y,x,y+r+2,0xFF819D);
}
static void sprite(const lv_img_dsc_t &img,int x,int y,unsigned opacity=255,unsigned zoom=256,int angle=0) {
  lv_draw_img_dsc_t d;lv_draw_img_dsc_init(&d);
  d.opa=opacity;d.zoom=zoom;d.angle=angle;d.antialias=1;
  d.pivot.x=img.header.w/2;d.pivot.y=img.header.h/2;
  lv_canvas_draw_img(canvas,x-img.header.w/2,y-img.header.h/2,&img,&d);
}
static void softCircle(int x,int y,int r,uint32_t c,unsigned opacity) {
  lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.radius=r;
  d.bg_color=color(c);d.bg_opa=opacity;lv_canvas_draw_rect(canvas,x-r,y-r,r*2,r*2,&d);
}
static void halo(int x,int y,int r,uint32_t c) {
  softCircle(x,y,r,c,5);softCircle(x,y,r*2/3,c,8);softCircle(x,y,r/3,c,12);
}
static void shadow(int x,int y,int w,int h) {
  lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.radius=h/2;
  d.bg_color=color(0x020A1B);d.bg_opa=75;
  lv_canvas_draw_rect(canvas,x-w/2,y-h/2,w,h,&d);
}
static void cat(int x,int y,int r,bool happy,bool,int =0) {
  const lv_img_dsc_t &art=happy?art_cat_badge:art_cat_icon;
  unsigned zoom=unsigned((r*2+12)*256/art.header.w);
  sprite(art,x,y,255,zoom);
}
static void confetti(uint32_t t) {
  uint32_t colors[]={0xFFD36B,0x80E0D0,0xFF8EA8,0xB6A0F9};
  for(int i=0;i<24;++i) {
    int x=50+(i*67)%255,y=115+(int(t/14)+i*39)%145;
    if((x-180)*(x-180)+(y-180)*(y-180)<160*160) rect(x,y,5,9,colors[i%4],2);
  }
}

static void fish(int x,int y,int r,uint32_t c) {
  rect(x-r,y-r/2,2*r,r,c,r/2);
  triangle(x+r-2,y,x+r+r/2,y-r/2,x+r+r/2,y+r/2,c);
  circle(x-r/2,y-1,2,INK);
}
static void planet(int x,int y,int r,unsigned type,bool locked=false) {
  const lv_img_dsc_t *large[]={&art_moon,&art_mars,&art_saturn};
  const lv_img_dsc_t *small[]={&art_moon_badge,&art_mars_badge,&art_saturn_badge};
  const lv_img_dsc_t &img=*(r<26?small[type%3]:large[type%3]);
  sprite(img,x,y,locked?55:255);
}
static void carIcon(int x,int y,uint32_t) { sprite(art_rover_icon,x,y); }
static void panel(int x,int y,int w,int h,uint32_t c,int radius=20) {
  lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);
  d.radius=radius;d.bg_color=color(c);
  d.border_width=1;d.border_color=color(0xFFFFFF);d.border_opa=75;
  d.shadow_width=9;d.shadow_ofs_y=4;d.shadow_color=color(0x050D22);d.shadow_opa=85;
  lv_canvas_draw_rect(canvas,x,y,w,h,&d);
}
static void button(int x,int y,int w,const char *label,uint32_t c) {
  panel(x,y,w,40,c,19);text(label,y+4,INK,x,w);
}
static void backButton(bool room) {
  panel(140,19,80,40,0xF4DBC8,17);
  if(room) {
    circle(158,40,5,0x70999B);
    circle(151,30,2,0x70999B);circle(157,28,2,0x70999B);circle(164,30,2,0x70999B);
  } else {
    triangle(149,36,158,27,167,36,0x70999B);rect(152,35,12,12,0x70999B,1);rect(157,40,3,7,0xFFF4DA);
  }
  smallText(room?"小窝":"首页",24,INK,171,44);
}
static void needIcon(PetActivity a,int x,int y,unsigned zoom=150) {
  if(a==PetActivity::Room) { heart(x,y-3,8);return; }
  const auto &icon=a==PetActivity::Feed?pet_food_icon:a==PetActivity::Bath?pet_bath_icon:
    a==PetActivity::Play?pet_play_icon:a==PetActivity::Litter?pet_litter_icon:art_moon_badge;
  sprite(icon,x,y,255,zoom);
}
static const char *needLabel(PetActivity a) {
  return a==PetActivity::Feed?"想吃小鱼":a==PetActivity::Bath?"想洗香香":a==PetActivity::Play?"想玩小球":
    a==PetActivity::Litter?"一起清理":a==PetActivity::Sleep?"想睡觉啦":"想要摸摸";
}
static void growthFlower(int x,int y,bool lit) {
  uint32_t petal=lit?0xE8A9B1:0xE9DCC9;
  circle(x-3,y,2,petal);circle(x+3,y,2,petal);circle(x,y-3,2,petal);circle(x,y+3,2,petal);
  circle(x,y,2,lit?0xF2C36F:0xCBBDA8);
}
static void space(const Game &g) {
  uint32_t tint=g.scene==Scene::Cake?0x47324F:g.scene==Scene::Cat?0x303F60:
    g.scene==Scene::Drive&&g.routeSelected?(g.route==1?0x44384E:0x253C58):0x2B3E60;
  uint32_t shades[256];
  for(int i=0;i<256;++i) {
    unsigned r=8+((tint>>16)-8)*i/255;
    unsigned g=19+(((tint>>8)&255)-19)*i/255;
    unsigned b=42+((tint&255)-42)*i/255;
    shades[i]=(r<<16)|(g<<8)|b;
  }
  static const uint8_t bayer[4][4]={{0,8,2,10},{12,4,14,6},{3,11,1,9},{15,7,13,5}};
  // Static radial sky is composed only when the scene or its UI state changes.
  for(int y=0;y<360;++y)for(int x=0;x<360;++x) {
    int dx=x-172,dy=y-142;int level=255-(dx*dx+dy*dy)/230;
    uint32_t c=shades[level<0?0:level];unsigned d=bayer[y&3][x&3];
    unsigned r=((c>>16)+(d>>1))>>3;
    unsigned gr=(((c>>8)&255)+(d>>2))>>2;
    unsigned b=((c&255)+(d>>1))>>3;
    pixels[y*360+x]=(r<<11)|(gr<<5)|b;
  }
  for(int i=0;i<34;++i) {
    int x=(i*83+19)%360,y=(i*47+31)%360;
    if(i%7==0)halo(x,y,5,0xC6D0E3);
    circle(x,y,i%5==0?2:1,i%3?0x8196B9:0xEBD8AA);
  }
}
static const char *routes[]={"月亮采星","火星救援","彩环寻宝"};
static const char *stickerNames[]={"小A好朋友","月亮探险家","火星救援队","彩环收藏家","生日魔法师","追球小高手"};
static uint64_t baseKey(const Game &g,bool tilt) {
  if(g.scene==Scene::City)return uint64_t(Scene::City)|(uint64_t(PixelCity::selection(g.cityTime,g.cityLandmark))<<3);
  if(g.scene==Scene::Cat) {
    const auto &k=g.kitten;const auto &p=k.state;
    return uint64_t(Scene::Cat)|uint64_t(p.satiety)<<3|uint64_t(p.clean)<<10|
      uint64_t(p.mood)<<17|uint64_t(p.energy)<<24|uint64_t(p.growth)<<31|
      uint64_t(k.activity)<<39|uint64_t(k.progress)<<42|uint64_t(k.phase)<<45|
      uint64_t(k.spots)<<47|uint64_t(k.done)<<51|uint64_t(k.eligible)<<52|uint64_t(p.wasteMask)<<53|uint64_t(g.memories())<<56|uint64_t(k.earned)<<63;
  }
  // Only fields that affect static pixels belong here. Time never invalidates it.
  return uint64_t(g.scene)|uint64_t(g.album)<<3|uint64_t(g.kitten.activity)<<9|
    uint64_t(g.careMask)<<11|uint64_t(g.route)<<14|uint64_t(g.routeSelected)<<16|
    uint64_t(g.won)<<17|uint64_t(g.stars)<<18|uint64_t(g.cakePhase)<<23|
    uint64_t(g.style)<<25|uint64_t(g.selectedSticker)<<27|uint64_t(tilt)<<30|
    uint64_t(g.newSticker)<<31|uint64_t(g.scene==Scene::Cat?g.kitten.revision:0)<<32;
}
static void drawBase(const Game &g,bool tilt) {
  if(g.scene==Scene::City) {
    PixelCity::paintBase(pixels,PixelCity::selection(g.cityTime,g.cityLandmark));return;
  }
  if(g.scene==Scene::Cat) { PixelRoom::paintBase(pixels);PixelRoom::paintMemories(pixels,g.memories()); } else space(g);
  if(g.scene==Scene::Home) {
    text("又又和小A",29);
    char label[48];snprintf(label,sizeof(label),"星球贴纸 %u/6",g.stickerCount());
    panel(65,70,110,34,0xB6A0F9,12);smallText(label,73,INK,65,110);
    panel(185,70,110,34,0xA3DBD5,12);smallText("像素深圳",73,INK,185,110);
    panel(65,115,230,58,0xEED7BB,22);sprite(pet_baby,105,142,255,100);text("小A的家",127,INK,143,135);
    panel(65,187,230,58,0xA3DBD5,22);carIcon(105,216,INK);text("星球探险",199,INK,143,135);
    panel(65,259,230,58,0xE6B5CA,22);sprite(art_cake_icon,105,289);text("生日派对",271,INK,143,135);
    return;
  }
  backButton(g.scene==Scene::Cat&&g.kitten.activity!=PetActivity::Room);
  if(g.scene==Scene::Cat) {
    const auto &k=g.kitten;const auto &p=k.state;char label[96];
    if(k.activity==PetActivity::Room) {
      const char *ages[]={"幼崽","小猫","成猫"};
      snprintf(label,sizeof(label),"小A · %s",ages[p.stage()]);text(label,73,0x477E82,65,175);
      panel(246,70,56,40,0xF5DCC2,13);needIcon(k.need(),274,89,170);
      unsigned within=p.stage()==0?p.growth:p.stage()==1?p.growth-60:60;
      unsigned goal=p.stage()==1?120:60;
      unsigned petals=(within*6+goal-1)/goal;
      for(unsigned i=0;i<6;++i)growthFlower(119+i*21,105,i<petals);
      const char *names[]={"肚肚","干净","开心","精神"};
      unsigned values[]={p.satiety,p.clean,p.mood,p.energy};
      for(int i=0;i<4;++i) {
        int x=63+i*62;smallText(names[i],112,values[i]<50?0xB97482:0x547D86,x,48);
        rect(x+3,132,42,4,0xE7D8C8,2);
        rect(x+3,132,42*values[i]/100,4,values[i]<50?0xD99AAA:0x72B9AA,2);
      }

      const lv_img_dsc_t *icons[]={&pet_food_icon,&pet_bath_icon,&pet_play_icon,&pet_litter_icon};
      const char *labels[]={"喂鱼","洗澡","玩耍","铲屎"};
      uint32_t colors[]={0xF2D9B9,0xD4C2E9,0xA3DBD5,p.wasteMask?0xE6B5CAu:0xB9C4D8u};
      for(int i=0;i<4;++i) {
        int x=57+i*62;panel(x,261,58,59,colors[i],16);
        sprite(*icons[i],x+29,279,255,205);smallText(labels[i],299,INK,x,58);
      }
      if(p.wasteMask) { circle(288,267,7,0xD78AA2);snprintf(label,sizeof(label),"%u",p.wasteCount());smallText(label,254,WHITE,278,20); }
      sprite(art_moon_badge,279,181,180,170);smallText("睡觉",205,0x7F739A,252,55);
      smallText(needLabel(k.need()),321,0x547D86);
    } else {
      const char *title=k.activity==PetActivity::Feed?"小鱼开饭啦":k.activity==PetActivity::Bath?"给小A洗香香":
        k.activity==PetActivity::Play?"陪小A玩一会":k.activity==PetActivity::Litter?"一起铲屎啦":"小A睡得香香的";
      text(k.done?"小A谢谢又又":title,74,0x477E82);
      const char *buttonLabel="回小窝";
      if(k.activity==PetActivity::Feed) {
        if(!k.eligible) { smallText("吃饱啦，去玩一会吧",111,0xBD875D);buttonLabel="去玩球"; }
        else { snprintf(label,sizeof(label),"点鱼喂一口 %u/3",k.progress);smallText(label,111,INK); }
        if(!k.done&&k.eligible)buttonLabel="喂一口";
      } else if(k.activity==PetActivity::Bath) {
        unsigned bubbles=(k.spots&1)+((k.spots>>1)&1)+((k.spots>>2)&1)+((k.spots>>3)&1);
        if(k.phase==0)snprintf(label,sizeof(label),"点四个泡泡 %u/4",bubbles);
        else snprintf(label,sizeof(label),k.phase==1?"点水滴冲洗 %u/3":"轻轻擦干 %u/3",k.progress);
        smallText(k.done?"洗得香香啦":label,111,INK);
        if(!k.done)buttonLabel=k.phase==0?"点泡泡":k.phase==1?"冲一冲":"擦一擦";
      } else if(k.activity==PetActivity::Play) {
        snprintf(label,sizeof(label),"点中小球 %u/5",k.progress);
        smallText(p.energy<=25?"有点累啦，先睡一会":label,111,INK);
        if(p.energy<=25)buttonLabel="去睡觉";
      } else if(k.activity==PetActivity::Litter) {
        snprintf(label,sizeof(label),"还剩 %u 块，点便便铲走",p.wasteCount());
        smallText(p.wasteMask?label:"猫砂盆干干净净啦",111,INK);
      } else {
        snprintf(label,sizeof(label),"精神 %u/100，慢慢恢复",p.energy);smallText(label,111,INK);
        buttonLabel="轻轻叫醒";
      }
      button(114,292,132,buttonLabel,k.done?0xBD875D:0xA3DBD5);
      if(k.done)smallText(k.earned?"成长花开啦！":k.activity==PetActivity::Bath&&!k.eligible?"已经香香的啦，也很开心":"这次陪伴，小A也很开心",264,0x7F8672);
    }
  } else if(g.scene==Scene::Drive) {
    if(!g.routeSelected) {
      text("今天去哪里？",79,0x8CE1DE);
      for(unsigned i=0;i<3;++i) { int x=90+i*90;planet(x,176,28,i);text(i==0?"月亮":i==1?"火星":"彩环",218,WHITE,x-43,86); }
      text("采星  救援  找颜色",267,0xFFD36B);
      text("点点星球就出发",303);
    } else {
      text(routes[g.route],77,palette[g.route]);
      sprite(g.route==0?art_moon:g.route==1?art_mars:art_saturn,307,175,95);
      line(125,134,56,304,0x56718E,1);line(235,134,304,304,0x56718E,1);
      halo(180,285,105,g.route==1?0xD79494:0x88C6D2);
      for(unsigned i=0;i<g.goal();++i)star(132+i*24,119,7,i<g.stars?0xFFD36B:0x405271);
      if(g.route==2&&!g.won) { circle(269,119,9,palette[g.wantedColor()]); }
      text(tilt?"倾斜或拖动小车":"左右拖动小车",310);
      if(g.won) {
        rect(72,159,216,94,0xFFD36B,23);text("任务完成啦！",172,INK);
        text(g.newSticker?"收到一张新贴纸":"又又太棒啦",208,INK);
        button(78,281,97,"选星球",0xF3A9BF);button(185,281,97,"下一站",0x8CE1DE);
      }
    }
  } else if(g.scene==Scene::Cake) {
    const char *title=g.cakePhase==CakePhase::Decorate?"一起装饰蛋糕":g.cakePhase==CakePhase::Light?"点亮四根蜡烛":g.cakePhase==CakePhase::Wish?"许个愿再点灭":"又又，生日快乐！";
    text(title,77,0xFFD36B);
    halo(180,224,105,0xF9C7AC);sprite(art_cake,180,223);
    panel(164,237,32,31,0xFFF0CE,13);text("4",235,0x8B5D64,164,32);
    if(g.cakePhase==CakePhase::Decorate) {
      button(70,267,68,"草莓",g.style==0?0xFFD36B:0xF3A9BF);
      button(146,267,68,"星星",g.style==1?0xFFD36B:0x8CE1DE);
      button(222,267,68,"小鱼",g.style==2?0xFFD36B:0xFFDAB0);
      button(119,310,122,"点蜡烛",0x8CE1DE);
      text("点蛋糕放装饰",111,WHITE);
    } else if(g.cakePhase==CakePhase::Party) {
      button(116,302,128,"再做一个",0x8CE1DE);
      text("点点天空放烟花",270);
    } else text(g.cakePhase==CakePhase::Light?"一根一根点亮它":"点灭蜡烛有惊喜",301);
  } else if(g.scene==Scene::Album) {
    char label[48];snprintf(label,sizeof(label),"又又的贴纸 %u/6",g.stickerCount());text(label,77,0xFFD36B);
    for(unsigned i=0;i<6;++i) {
      int x=100+i%3*80,y=i<3?157:244;bool got=g.album&(1u<<i);
      circle(x,y,34,i==g.selectedSticker?0x526781:0x263954);
      if(i>=1&&i<=3)planet(x,y,23,i-1,!got);
      else if(i==0) { if(got)sprite(art_cat_badge,x,y);else text("?",y-15,0x64758E,x-25,50); }
      else if(i==4) { if(got)sprite(art_cake_icon,x,y);else text("?",y-15,0x64758E,x-25,50); }
      else { if(got)cat(x,y,20,true,false);else text("?",y-15,0x64758E,x-25,50); }
    }
    text(stickerNames[g.selectedSticker],291,g.album&(1u<<g.selectedSticker)?WHITE:0x879BB8);
    const char *hint=g.selectedSticker==0?"喂鱼玩球再睡觉":g.selectedSticker==5?"追到小球三次":g.selectedSticker==4?"完成生日派对":"完成对应星球任务";
    if(!(g.album&(1u<<g.selectedSticker)))text(hint,320,0x879BB8);
  }
}
static void particles(const Game &g) {
  for(const auto &p:g.sparks)if(p.life&&p.x>48&&p.x<310&&p.y>115&&p.y<265)
    { softCircle(int(p.x),int(p.y),5,palette[p.color],35);
      circle(int(p.x),int(p.y),p.life>500?2:1,palette[p.color]); }
}
static void drawMoving(const Game &g) {
  if(g.scene==Scene::City) {
    PixelCity::paintMotion(pixels,g.cityTime);
    backButton(false);
    smallText("像素深圳",58,0x427C90);
    if(g.cityRide) {
      PixelCity::Canvas c{pixels};int x=-24+int((1800-g.cityRide)*228/1800);
      PixelCity::vehicle(c,x,132,2,1,true,g.cityTime/50);
      c.box(x+3,131,4,4,0xDEE6E3);c.box(x+3,130,1,2,0x9EAAB2);c.box(x+6,130,1,2,0x9EAAB2);
      c.box(x+4,132,1,1,0x508783);c.box(x+6,132,1,1,0x508783);
    }
    rect(102,320,156,24,0xFFF6DF,0);smallText("点道路叫小A",320,0x427C90);
    rect(64,232,232,23,0xFFF6DF,0);
    smallText(PixelCity::landmarks[PixelCity::selection(g.cityTime,g.cityLandmark)].name,232,0x427C90);
    return;
  }
  if(g.scene==Scene::Cat) {
    PixelRoom::paintMotion(pixels,g.kitten.clockMs);
    const auto &k=g.kitten;const auto &p=k.state;
    bool sleeping=k.activity==PetActivity::Sleep;
    bool attentive=k.activity==PetActivity::Room&&k.clockMs<k.attentionUntil;
    unsigned stage=p.stage();
    const lv_img_dsc_t *neutral[]={&pet_baby,&pet_young,&pet_adult};
    const lv_img_dsc_t *image=neutral[stage];
    unsigned blink=Motion::blink(k.clockMs);
    if(stage==0&&blink)image=blink==1?&pet_half:&pet_blink;
    bool stroking=k.strokes&&k.clockMs-k.lastStroke<1200;
    if(stroking)image=k.clockMs-k.lastStroke<260?&pet_stretch:&pet_happy;
    if(k.activity==PetActivity::Room&&p.clean<45)image=&pet_dirty;
    if(k.activity==PetActivity::Feed)image=k.done||!k.eligible||(k.feedback==PetSignal::Bite&&k.feedbackAge()<650)?&pet_happy:neutral[stage];
    if(k.activity==PetActivity::Bath)image=k.done?&pet_happy:k.phase<2?&pet_bath:&pet_happy;
    if(k.activity==PetActivity::Play)image=k.done?&pet_happy:&pet_play;
    if(sleeping)image=&pet_sleep;
    unsigned size=100+(p.growth>180?180:p.growth)*34/180;
    if(k.activity!=PetActivity::Room)size+=12;
    float breath=Motion::breathe(k.clockMs,sleeping);
    bool roaming=k.activity==PetActivity::Room||k.activity==PetActivity::Play;
    if(roaming)size=k.roomSize()+(k.activity==PetActivity::Play?6:0);
    int cx=roaming?int(k.walk.x):180;
    int bob=0;
    int cy=(roaming?int(k.walk.y)-int(size/2):202)+int(breath*1.8f)-bob;
    if(k.activity==PetActivity::Room&&k.walk.marker) {
      int x=k.walk.targetX,y=k.walk.targetY;
      rect(x-7,y-2,14,2,0xF3A7AC);rect(x-2,y-6,4,10,0xF3A7AC);
    }
    if(k.activity==PetActivity::Litter) {
      sprite(*neutral[stage],104,166,255,110);sprite(pet_tray,180,222);
      for(unsigned i=0;i<3;++i)if(p.wasteMask&(1u<<i)) {
        int x=120+i*60;
        rect(x-13,214,26,13,0xA87861,6);rect(x-10,207,20,12,0xB4866C,6);
        triangle(x-7,210,x+8,210,x+2,198,0xC69778);
        softCircle(x-3,209,3,0xF9DABB,55);
      }
    } else {
      int feet=roaming?int(k.walk.y):250;
      lv_draw_rect_dsc_t sd;lv_draw_rect_dsc_init(&sd);sd.radius=6;
      sd.bg_color=color(0xA18F83);sd.bg_opa=48;
      lv_canvas_draw_rect(canvas,cx-size/3,feet-8,size*2/3,9,&sd);
      if(roaming&&k.walk.stand>0&&!stroking) {
        unsigned weight=unsigned(k.walk.stand*255);
        if(weight<255)sprite(*image,cx,cy,255-weight,size*256/144);
        if(weight) {
          const lv_img_dsc_t *frame;
          if(attentive)frame=walk_turn[1];
          else if(k.walk.turn) {
            unsigned elapsed=240-k.walk.turn;
            unsigned pose=elapsed<80?(k.walk.left?2:0):elapsed<160?1:(k.walk.left?0:2);
            frame=walk_turn[pose];
          } else if(!k.walk.moving)frame=walk_stand[k.walk.left?1:0];
          else frame=(k.walk.left?walk_left:walk_right)[k.walk.frame()];
          unsigned zoom=size*307/128; // side silhouette is longer than a seated cat
          int centerY=feet-int((frame->header.h/2-4)*zoom/256);
          sprite(*frame,cx,centerY,weight,zoom);
        }
      } else sprite(*image,cx,cy,255,size*256/144+int(breath));
    }
    if(k.activity==PetActivity::Room) {
      unsigned pulse=(k.clockMs/350)%2;
      if(pulse) { softCircle(274,89,19,0xFFF4C9,50);needIcon(k.need(),274,89,170); }
      if(attentive) { heart(cx+28,cy-24,6);needIcon(k.need(),cx-30,cy-18,115); }
      if(g.age<g.memoryUntil) {
        rect(94,320,172,24,0xF6E7C8,8);smallText("看看小A的纪念物",320,0x8F7770);
        int mx=g.memoryGift&2?294:g.memoryGift&1?154:g.memoryGift&4?320:g.memoryGift&8?195:g.memoryGift&32?223:g.memoryGift&16?49:136;
        int my=g.memoryGift&2?156:g.memoryGift&1?164:g.memoryGift&4?183:g.memoryGift&8?174:g.memoryGift&32?181:g.memoryGift&16?236:195;
        if((k.clockMs/250)%2)star(mx,my,5,0xE8BD70);
      }
    }
    if(k.activity==PetActivity::Feed) {
      rect(148,240,64,21,0xFFF2D6,9);rect(145,237,70,9,0x99CBC6,4);
      if(k.progress<3)fish(180,238,18-4*k.progress,0x66B7B6);
      if(k.feedback==PetSignal::Bite&&k.feedbackAge()<600) {
        float t=fminf(1,k.feedbackAge()/260.f);
        int x=k.feedbackX+(180-k.feedbackX)*t,y=238+(cy+2-238)*t;
        if(t<1)fish(x,y,6,0x66B7B6);else heart(218,cy-14-int((k.feedbackAge()-260)/50),6);
      }
      for(unsigned i=0;i<3;++i)circle(163+i*17,279,4,i<k.progress?0xE6AA86:0xC5D0CB);
      if(!k.eligible) { needIcon(PetActivity::Play,279,177,215);line(226,228,256,194,0xCC9BA7,2); }
    }
    if(k.activity==PetActivity::Play&&p.energy<=25)needIcon(PetActivity::Sleep,279,177,200);
    if(k.activity==PetActivity::Bath&&!k.done) {
      if(k.phase==0) {
        const int x[]={128,218,139,223},y[]={162,168,220,225};
        for(unsigned i=0;i<4;++i)if(!(k.spots&(1u<<i))) {
          int bob=int(sinf(k.clockMs/300.f+i)*2);
          softCircle(x[i],y[i]+bob,14,0xABD7E8,125);
          softCircle(x[i]-4,y[i]-5+bob,5,0xFFFFFF,175);
          softCircle(x[i]+4,y[i]+5+bob,7,0xD5B5E7,80);
        }
      } else if(k.phase==1) {
        for(int i=0;i<9;++i) {
          int y=139+(int(k.clockMs/8)+i*19)%102,x=139+(i%3)*40;
          line(x,y,x-2,y+7,0xA1DFEF,3);
        }
      } else {
        int x=180+int(sinf(k.clockMs/170.f)*22);
        panel(x-21,223,42,22,0xD6BEE7,8);
      }
    }
    if(k.activity==PetActivity::Play&&!k.done) {
      int x=k.ballX(),y=k.ballY();
      halo(x,y,24,0xEAB3CF);circle(x,y,16,0xCB86B1);circle(x-2,y-3,13,0xEDB4CF);
      softCircle(x-6,y-7,4,0xFFFFFF,150);line(x-7,y+3,x+8,y,0xFFF0DF,2);
    }
    if(k.feedbackAge()<450) {
      int x=k.feedbackX,y=k.feedbackY;unsigned t=k.feedbackAge();
      if(k.feedback==PetSignal::Bubble||k.feedback==PetSignal::Ball) {
        int r=8+t/25;
        line(x-r,y,x-r-4,y,0xE9ADBC,2);line(x+r,y,x+r+4,y,0xE9ADBC,2);
        line(x,y-r,x,y-r-4,0xF2C975,2);line(x,y+r,x,y+r+4,0xF2C975,2);
      } else if(k.feedback==PetSignal::Scoop) {
        int sy=y-int(t/30);rect(x-14,sy,26,9,0xA2D2CD,3);line(x+8,sy,x+24,sy-18,0x96B9B1,4);
        star(x-20,sy-8,5,0xE8C56F);
      } else if(k.feedback==PetSignal::Hint) {
        softCircle(x,y,24,0xE9C78F,65);circle(x,y,3,0xD8949E);
      }
    }
    if(sleeping)smallText("z  Z",151-int((k.clockMs%1800)/250),0x86749D,230,58);
    if(stroking&&k.activity==PetActivity::Room) {
      heart(cx-30,cy-25-int(k.clockMs%1000)/80,7);heart(cx+31,cy-22-int(k.clockMs%1000)/90,6);
    }
    if(g.age<g.petMessageUntil&&g.petMessage==PetSignal::Grow) {
      panel(78,211,204,34,0xF1D79F,15);smallText("小A长大一点啦！",216,INK);
    }
    particles(g);
  } else if(g.scene==Scene::Drive&&g.routeSelected) {
    if(g.won) { particles(g);return; }
    for(int i=0;i<7;++i) {
      int y=137+(i*25+int(g.age/16))%166;int spread=(y-130)*3/5;
      line(180-spread,y,180-spread-3,y+7,0x6B829D,2);
      line(180+spread,y,180+spread+3,y+7,0x6B829D,2);
    }
    for(const auto &i:g.items)if(i.active) {
      int x=i.x,y=i.y;
      if(i.kind==Pickup::Rock) { triangle(x-14,y+12,x,y-14,x+14,y+12,0xF1A779);line(x-9,y+3,x+9,y+3,WHITE,3); }
      else if(i.kind==Pickup::Magnet) {
        rect(x-12,y-14,24,28,0xB6A0F9,8);rect(x-5,y-15,10,18,BG,4);
        rect(x-12,y-14,7,8,WHITE);rect(x+5,y-14,7,8,WHITE);
      } else if(i.kind==Pickup::Fuel) { rect(x-11,y-13,22,26,0x8CE1DE,5);text("+",y-18,INK,x-15,30); }
      else if(i.kind==Pickup::Pod)cat(x,y,15,false,true);
      else {
        uint32_t c=g.route==2?palette[i.color]:0xFFD36B;
        halo(x,y,23,c);star(x,y,14,c);circle(x-2,y-4,2,WHITE);
      }
    }
    int x=g.car;int bounce=int(sinf(g.age/105.f)*1.6f);
    if(g.age<g.magnetUntil) {
      halo(x,255,55,0xB6A0F9);
      for(int k=0;k<3;++k) { float a=g.age/430.f+k*2.094f;circle(x+cosf(a)*43,253+sinf(a)*27,3,0xC9B8FF); }
    }
    shadow(x,293,78,12);
    const auto &rover=g.target-g.car<-6?art_rover_left:g.target-g.car>6?art_rover_right:art_rover_front;
    sprite(rover,x,255+bounce);
    if(g.age<g.slowUntil)text("咚！慢慢开",142,0xFFD36B);
    else if(g.age<g.magnetUntil)text("磁铁吸星星",142,0xB6A0F9);
    else if(g.age<g.feedbackUntil&&g.route==2)text("找同色星星",142,0xF3A9BF);
    particles(g);
  } else if(g.scene==Scene::Cake) {
    for(unsigned j=0;j<g.toppingCount;++j) {
      const auto &t=g.toppings[j];
      if(t.style==0) { heart(t.x,t.y,7);line(t.x-3,t.y-3,t.x+3,t.y-3,0x8CE1DE,3); }
      else if(t.style==1)star(t.x,t.y,8,0xFFD36B);
      else fish(t.x,t.y,7,0x8CE1DE);
    }
    if(g.cakePhase!=CakePhase::Decorate)for(unsigned i=0;i<4;++i) {
      int x=120+i*40;rect(x-6,170,12,42,i%2?0x8CE1DE:0xF3A9BF,4);
      line(x-5,184,x+4,177,WHITE,2);
      bool lit=g.cakePhase==CakePhase::Light?(g.candles&(1u<<i)):
        g.cakePhase==CakePhase::Wish?!(g.candles&(1u<<i)):false;
      if(lit) {
        int f=int(sinf(g.age/100.f+i*1.7f)*2);
        halo(x,153+f,19,0xFFA74E);
        circle(x,155+f,7,0xFFB84E);
        triangle(x-6,153+f,x+6,153+f,x+2,137+f,0xFFD67F);
        circle(x,157+f,3,0xFFF7D9);
      } else if((g.cakePhase==CakePhase::Wish||g.cakePhase==CakePhase::Party)&&
                (g.candles&(1u<<i))&&g.age-g.candleTouch[i]<700) {
        unsigned elapsed=g.age-g.candleTouch[i];
        for(int j=0;j<3;++j)softCircle(x+int(sinf(elapsed/140.f+j)*3),
          156-int(elapsed/24)-j*5,2+elapsed/220,0xCBD3DF,(700-elapsed)/12);
      }
    }
    if(g.cakePhase==CakePhase::Party) {
      if(!g.fireworks&&g.age<6000)confetti(g.age);
      particles(g);
      if(g.fireworks%3==1)sprite(art_cat_badge,180,153+int(sinf(g.age/250.f)*3),255);
      else if(g.fireworks%3==2)sprite(art_rocket,180,151+int(sinf(g.age/230.f)*5),255);
      else if(g.fireworks)sprite(art_rover_icon,180,157,255);
    }
  }
}
bool viewInit(void *buf,void *base,void *old) {
  if(!buf||!base||!old)return false;
  pixels=static_cast<uint16_t *>(buf);background=static_cast<uint16_t *>(base);previous=static_cast<uint16_t *>(old);
  memset(previous,0,360*360*2);cachedKey=UINT64_MAX;stats={};wasEntering=false;
  lv_obj_clean(lv_scr_act());lv_obj_set_style_bg_color(lv_scr_act(),color(BG),0);
  canvas=lv_canvas_create(lv_scr_act());lv_canvas_set_buffer(canvas,buf,360,360,LV_IMG_CF_TRUE_COLOR);
  lv_obj_center(canvas);lv_obj_add_flag(canvas,LV_OBJ_FLAG_CLICKABLE);lv_obj_clear_flag(canvas,LV_OBJ_FLAG_SCROLLABLE);
  return true;
}
lv_obj_t *viewCanvas() { return canvas; }
ViewStats viewStats() { return stats; }
void viewInvalidateCache() { cachedKey=UINT64_MAX; }
void viewDraw(const Game &g,bool tiltAvailable) {
  lv_disp_t *disp=lv_obj_get_disp(canvas);
  // Canvas primitives invalidate the whole object. Suppress those requests,
  // then submit only changed tile spans after composing a complete frame.
  lv_disp_enable_invalidation(disp,false);
  uint64_t key=baseKey(g,tiltAvailable);
  if(key!=cachedKey) {
    drawBase(g,tiltAvailable);memcpy(background,pixels,360*360*2);
    cachedKey=key;++stats.staticBuilds;
  } else {
    // All animation fits this playfield; navigation and toolbars stay cached.
    if(g.age<320||wasEntering)memcpy(pixels,background,360*360*2);
    else if(g.scene==Scene::City) {
      memcpy(pixels,background,360*88*2);
      memcpy(pixels+360*228,background+360*228,360*132*2);
    }
    else if(g.scene==Scene::Cat) {
      memcpy(pixels+360*65,background+360*65,360*295*2);
    }
    else for(int y=100;y<360;++y)memcpy(pixels+y*360,background+y*360,360*2);
  }
  drawMoving(g);
  // Pickups disappear under the dashboard, rather than covering its instruction.
  if(g.scene==Scene::Drive&&g.routeSelected)
    for(int y=307;y<360;++y)memcpy(pixels+y*360,background+y*360,360*2);
  unsigned fade=Motion::entryOpacity(g.age);
  if(fade) {
    lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);
    d.bg_color=color((g.scene==Scene::City||g.scene==Scene::Cat)?0xFFF6DF:0x081226);d.bg_opa=fade;
    lv_canvas_draw_rect(canvas,0,0,360,360,&d);
  }
  wasEntering=g.age<320;
  lv_disp_enable_invalidation(disp,true);
  stats.spans=0;
  stats.pixels=dirtyTiles(pixels,previous,[&](const DirtySpan &s) {
    lv_area_t area{(lv_coord_t)s.x1,(lv_coord_t)s.y1,(lv_coord_t)s.x2,(lv_coord_t)s.y2};
    lv_obj_invalidate_area(canvas,&area);++stats.spans;
  });
}
