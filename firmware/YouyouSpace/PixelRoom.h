#pragma once
#include "PixelCity.h"
// Warm 180x180 pixel room, composed into the existing RGB565 buffers.
namespace PixelRoom {
using PixelCity::Canvas;
inline void plant(Canvas &c,int x,int y,int sway=0) {
  c.box(x-3,y,8,6,0xE7A99C);c.box(x-4,y,10,1,0xFFD6B5);
  c.box(x,y-11,1,11,0x75AA87);
  c.box(x-5+sway,y-9,5,3,0x88C9A3);c.box(x+1+sway,y-12,5,3,0xA1DBAD);
  c.box(x-3+sway,y-15,4,3,0x75B795);
}
inline void paintBase(uint16_t *p) {
  Canvas c{p};c.box(0,0,180,180,0xFFF5DF);
  c.box(0,69,180,37,0xF6E4D1);
  for(int x=0;x<180;x+=8)c.box(x,69,2,37,0xFBEEDB);
  c.box(0,101,180,5,0xD5B69D);c.box(0,102,180,1,0xFFEFD5);
  c.box(0,106,180,74,0xEBC9A7);
  for(int y=107;y<180;y+=12) {
    c.box(0,y,180,1,0xDDB796);
    for(int x=(y/12)%2?8:28;x<180;x+=42) {
      c.box(x,y+1,1,11,0xE0BC9A);c.box(x+5,y+4,12,1,0xF4D6B2);
    }
  }
  // Window with a deep cream sill, peach curtains and a blue garden view.
  c.box(22,71,39,29,0xD5B59C);c.box(24,73,35,24,0xA9DDE7);
  c.box(24,90,35,7,0xB4D9AB);c.box(24,94,35,3,0x94C99C);
  c.box(40,73,2,24,0xFFF8E6);c.box(24,85,35,2,0xFFF8E6);
  c.box(20,70,43,2,0xA6847C);c.box(19,98,45,3,0xFFEFDA);
  for(int i=0;i<3;++i) { c.box(21+i*2,72,2,25-i*2,0xEFB7AD);c.box(57+i*2,72,2,21+i*2,0xEFB7AD); }
  c.box(25,72,1,21,0xFFD8C4);c.box(59,72,1,21,0xFFD8C4);
  // Sofa and cushions, behind the walkable foreground.
  c.box(14,101,28,12,0xD8ACAD);c.box(16,99,24,11,0xF3CAC0);
  c.box(14,105,4,9,0xE8B8B3);c.box(38,105,4,9,0xE8B8B3);
  c.box(20,103,8,6,0xFFF0CE);c.box(29,104,7,5,0xAFD6CC);
  c.box(18,114,2,3,0xB78D7C);c.box(36,114,2,3,0xB78D7C);
  // A tiny shelf: books, toy car, a cat portrait and a plant.
  c.box(120,74,38,3,0xCDA78C);c.box(123,67,4,7,0xA6CADB);
  c.box(128,66,3,8,0xF0B9AB);c.box(132,68,4,6,0xC3B6DE);

  c.box(91,72,13,16,0xD5AE92);c.box(93,74,9,12,0xFFFAE9);

  c.box(119,95,35,2,0xCCA88D);
  // Cushioned sleeping nook, beside the sleep button.
  c.box(130,109,27,7,0xC3BCD9);c.box(132,106,23,6,0xDDD3ED);
  c.box(134,108,19,4,0xF7E9DF);c.box(135,109,4,2,0xF0C4BC);
  // Pixel woven rug and its little fringes.
  c.box(47,113,83,17,0xD4AAB0);c.box(48,112,81,17,0xF0CCD1);
  c.box(51,114,75,13,0xFCE5DC);c.box(54,116,69,9,0xB9DCD0);
  for(int x=49;x<130;x+=4) { c.box(x,110,1,2,0xF5D8C8);c.box(x,129,1,2,0xF5D8C8); }
  for(int x=58;x<123;x+=8)c.box(x,120,3,1,0xE8F0D7);
  // A basket of toys in the corner.
  c.box(19,121,15,8,0xD5AE84);c.box(18,121,17,2,0xFFE2AF);

}
inline void paintMemories(uint16_t *p,unsigned flags) {
  Canvas c{p};
  if(flags&1) { // a space trip leaves a moon poster
    c.box(70,72,13,17,0xB9C4DD);c.box(71,73,11,15,0x7F9AB7);
    c.oval(77,79,3,3,0xFFF1BA);c.oval(79,78,3,3,0x7F9AB7);
    c.box(73,84,1,1,0xFFF8E3);c.box(80,82,1,1,0xFFF8E3);
  }
  if(flags&2) {
    c.box(139,70,10,3,0x9ECFC6);c.box(141,68,5,2,0xBDE9DA);
    c.box(140,73,2,2,0x7E879A);c.box(146,73,2,2,0x7E879A);
  }
  if(flags&4) {
    c.line(156,92,154,103,0xBC94A0);c.line(164,87,161,103,0xBC94A0);
    c.oval(156,87,4,6,0xEBA6BA);c.oval(164,82,4,6,0xB1C8E6);
  }
  if(flags&8) {
    c.box(95,78,5,5,0xB1BFC5);c.box(95,76,1,3,0x9AA8B2);c.box(99,76,1,3,0x9AA8B2);
    c.box(96,80,1,1,0x4B7A82);c.box(99,80,1,1,0x4B7A82);c.box(97,83,1,1,0xEFA1A8);
  }
  if(flags&16) { c.oval(25,119,4,4,0xA3CFD9);c.line(22,119,28,119,0xFFF2CE); }
  if(flags&32) {
    c.box(110,91,6,4,0xE8BAAE);c.box(112,85,1,6,0x72B494);
    c.box(110,84,5,2,0xF1AABC);c.box(112,82,1,6,0xF1AABC);c.box(112,84,1,2,0xFFF0B1);
  }
  if(flags&64) { c.box(63,91,14,8,0xE8CBB2);c.box(64,92,12,6,0xFFF4DA);c.box(66,94,6,2,0x8CC9C0);c.box(72,93,3,4,0x8CC9C0);c.box(67,94,1,1,0x518E94); }
}
inline void paintMotion(uint16_t *p,uint32_t clock) {
  Canvas c{p};unsigned t=(clock%12000)/100;
  // Window view only: clouds never paint over the frame or curtains.
  int x=12+int(t/2);
  auto cloudStrip=[&](int xx,int y,int w) {
    int lo=xx<24?24:xx,hi=xx+w>59?59:xx+w;
    if(hi>lo)c.box(lo,y,hi-lo,2,0xFFFEED);
  };
  cloudStrip(x,77,6);cloudStrip(x-2,79,10);
  c.box(40,73,2,24,0xFFF8E6);
  c.box(44,87,3,2,(t/12)%2?0xFFF2BF:0xF8DFBD);
  plant(c,124,89,(t/8)%2?1:0);
  // A sunlit patch glimmers on the floor without a full-scene alpha overlay.
  c.box(70,108,13,1,(t/10)%2?0xFFF1CB:0xFAE7C4);
  c.box(73,109,13,1,0xF6DFB8);
}
}
