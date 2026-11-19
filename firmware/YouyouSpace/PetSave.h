#pragma once
#include "PetModel.h"
#include <stddef.h>
namespace PetSave {
constexpr size_t Words=24,Bytes=Words*4;
struct Record { PetState pet;uint32_t album=0,sequence=0; };
inline uint32_t hash(const uint8_t *data,size_t n) {
  uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^data[i])*16777619u;return h;
}
inline void put(uint8_t *b,uint32_t v) { for(unsigned i=0;i<4;++i)b[i]=v>>(8*i); }
inline uint32_t get(const uint8_t *b) { return uint32_t(b[0])|uint32_t(b[1])<<8|uint32_t(b[2])<<16|uint32_t(b[3])<<24; }
inline bool newer(uint32_t a,uint32_t b) { return int32_t(a-b)>0; }
inline void encode(const Record &r,uint8_t *out) {
  const auto &p=r.pet;
  uint32_t w[Words]={0x34504159u,1,r.sequence,r.album,p.satiety,p.clean,p.mood,p.energy,p.growth,
    p.activeSeconds,p.wasteMask,p.pendingMeals,p.digestSeconds,p.completedMask,p.totalFeeds,p.totalBaths,
    p.totalPlayHits,p.totalScoops,p.lastReward[0],p.lastReward[1],p.lastReward[2],p.lastReward[3],p.goodSeconds,0};
  for(unsigned i=0;i<Words-1;++i)put(out+4*i,w[i]);
  put(out+Bytes-4,hash(out,Bytes-4));
}
inline bool decode(const uint8_t *data,size_t length,Record &out) {
  if(!data||length!=Bytes||get(data)!=0x34504159u||get(data+4)!=1||get(data+Bytes-4)!=hash(data,Bytes-4))return false;
  uint32_t w[Words];for(unsigned i=0;i<Words;++i)w[i]=get(data+4*i);
  for(unsigned i=4;i<=7;++i)if(w[i]<20||w[i]>100)return false;
  if(w[3]>63||w[8]>240||w[10]>7||w[11]>3||w[12]>=150||w[13]>63||w[22]>=60)return false;
  for(unsigned i=18;i<=21;++i)if(w[i]!=UINT32_MAX&&w[i]>w[9])return false;
  Record r;r.sequence=w[2];r.album=w[3];auto &p=r.pet;
  p.satiety=w[4];p.clean=w[5];p.mood=w[6];p.energy=w[7];p.growth=w[8];p.activeSeconds=w[9];
  p.wasteMask=w[10];p.pendingMeals=w[11];p.digestSeconds=w[12];p.completedMask=w[13];
  p.totalFeeds=w[14];p.totalBaths=w[15];p.totalPlayHits=w[16];p.totalScoops=w[17];
  for(unsigned i=0;i<4;++i)p.lastReward[i]=w[18+i];
  p.goodSeconds=w[22];out=r;return true;
}
inline bool choose(const uint8_t *a,size_t na,const uint8_t *b,size_t nb,Record &out) {
  Record first,second;bool va=decode(a,na,first),vb=decode(b,nb,second);
  if(!va&&!vb)return false;
  out=va&&(!vb||newer(first.sequence,second.sequence))?first:second;return true;
}
}
