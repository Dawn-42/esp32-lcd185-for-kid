#pragma once
#include <stdint.h>
#include <string.h>
struct DirtySpan { int x1,y1,x2,y2; };
// At most 12 invalidations, comfortably below LVGL's default invalidation limit.
template<class Emit>
unsigned dirtyTiles(const uint16_t *current,uint16_t *previous,Emit emit) {
  unsigned pixels=0;
  for(int ty=0;ty<12;++ty) {
    int first=12,last=-1;
    for(int tx=0;tx<12;++tx) {
      bool changed=false;
      for(int y=ty*30;y<(ty+1)*30&&!changed;++y)
        changed=memcmp(current+y*360+tx*30,previous+y*360+tx*30,60)!=0;
      if(changed) { if(tx<first)first=tx;last=tx; }
    }
    if(last<0)continue;
    DirtySpan span{first*30,ty*30,(last+1)*30-1,(ty+1)*30-1};
    for(int y=span.y1;y<=span.y2;++y)
      memcpy(previous+y*360+span.x1,current+y*360+span.x1,(span.x2-span.x1+1)*2);
    pixels+=(span.x2-span.x1+1)*30;
    emit(span);
  }
  return pixels;
}
