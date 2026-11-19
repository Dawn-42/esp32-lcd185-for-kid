#pragma once
#include <stdint.h>
#include <math.h>
namespace Motion {
inline float outCubic(float t) { t=fmaxf(0,fminf(1,t));float a=1-t;return 1-a*a*a; }
inline unsigned blink(uint32_t age) {
  unsigned t=age%4200;
  return t>=2100&&t<2170?1:t>=2170&&t<2290?2:t>=2290&&t<2360?1:0;
}
inline float breathe(uint32_t age,bool asleep=false) { return sinf(age/(asleep?820.f:610.f)); }
inline float response(uint32_t age,uint32_t until,uint32_t duration) {
  if(age>=until||until<duration)return 0;
  float elapsed=float(age-(until-duration));
  return sinf(elapsed*.021f)*expf(-elapsed/210.f);
}
inline unsigned entryOpacity(uint32_t age) {
  return age>=320?0:unsigned((1-outCubic(age/320.f))*255);
}
}
