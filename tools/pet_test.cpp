#include "PetModel.h"
#include "PetSave.h"
#include <cassert>
#include <cstring>
#include <cstdio>
static void bites(PetModel &p) { p.begin(PetActivity::Feed);for(int i=0;i<3;++i)p.tap(180,185); }
static void bath(PetModel &p) {
  p.begin(PetActivity::Bath);
  int x[]={128,218,139,223},y[]={162,168,220,225};
  p.tap(x[0],y[0]);p.tap(x[0],y[0]);
  assert(p.spots==1);
  for(int i=1;i<4;++i)p.tap(x[i],y[i]);
  assert(p.phase==1);
  for(int i=0;i<3;++i)p.tap(180,300);
  assert(p.phase==2);
  for(int i=0;i<3;++i)p.tap(180,300);
  assert(p.done);
}
int main() {
  PetModel p;
  PetModel content;content.state.mood=100;content.stroke();assert(content.saveRevision==0);
  content.state.completedMask=16;content.begin(PetActivity::Sleep);assert(content.saveRevision==0);
  PetModel snack;snack.begin(PetActivity::Feed);snack.tap(180,180);
  snack.begin(PetActivity::Room);assert(snack.state.pendingMeals==1&&snack.state.growth==0);
  assert(p.state.stage()==0);
  bites(p);
  assert(p.done&&p.progress==3&&p.state.growth==6&&p.state.pendingMeals==1);
  auto xp=p.state.growth;bites(p);
  assert(p.state.growth==xp&&p.state.satiety==100);
  p.begin(PetActivity::Feed);
  assert(!p.eligible);
  p.tap(180,180);
  assert(p.state.satiety==100&&p.state.growth==xp);
  bath(p);
  assert(p.state.clean==95&&p.state.totalBaths==1);
  xp=p.state.growth;bath(p);
  assert(p.state.growth==xp); // Clean already: fun still works, no farming.
  p.begin(PetActivity::Play);
  for(int i=0;i<5;++i)p.tap(p.ballX(),p.ballY());
  assert(p.done&&p.state.totalPlayHits==5&&p.state.energy<80);
  p.state.wasteMask=7;p.begin(PetActivity::Litter);
  p.tap(120,218);p.tap(120,218);
  assert(p.state.wasteMask==6&&p.state.totalScoops==1);
  p.tap(180,218);p.tap(240,218);
  assert(p.done&&p.state.wasteMask==0&&p.state.totalScoops==3);
  assert((p.state.completedMask&15)==15&&p.state.growth<60); // One care cycle cannot instantly grow up.
  xp=p.state.growth;p.begin(PetActivity::Litter);p.tap(120,218);
  assert(p.state.growth==xp);
  p.state.energy=30;p.begin(PetActivity::Sleep);p.tick(6000);
  assert(p.state.energy==32);p.tap(180,300);
  assert(p.activity==PetActivity::Room);
  p.state.pendingMeals=1;p.state.digestSeconds=0;p.tick(150000);
  assert(p.state.wasteMask&&p.state.pendingMeals==0);
  p.state.wasteMask=7;p.state.pendingMeals=3;p.tick(450000);
  assert(p.state.wasteMask==7&&p.state.pendingMeals==3&&p.state.digestSeconds<150);
  PetModel growing;growing.state.satiety=growing.state.clean=growing.state.mood=growing.state.energy=90;
  growing.state.wasteMask=0;growing.state.growth=59;growing.state.goodSeconds=59;growing.tick(1000);
  assert(growing.state.stage()==1&&growing.takeSignal()==PetSignal::Grow);
  growing.state.growth=179;growing.state.goodSeconds=59;growing.tick(1000);
  assert(growing.state.stage()==2&&growing.state.growth==180);
  growing.grow(1000);
  assert(growing.state.growth==240);
  PetModel neglected;neglected.state.clean=20;neglected.tick(600000);
  assert(neglected.state.growth==0&&neglected.state.clean>=20);
  PetSave::Record r;r.pet=p.state;r.album=37;r.sequence=7;
  uint8_t a[PetSave::Bytes],b[PetSave::Bytes],copy[PetSave::Bytes];PetSave::Record loaded;
  PetSave::encode(r,a);
  assert(PetSave::decode(a,sizeof(a),loaded));
  PetSave::encode(loaded,copy);
  assert(!memcmp(a,copy,sizeof(a)));
  for(size_t i=0;i<sizeof(a);++i) {
    memcpy(copy,a,sizeof(a));copy[i]^=1;
  assert(!PetSave::decode(copy,sizeof(copy),loaded));
  }
  r.sequence=8;r.pet.growth=180;PetSave::encode(r,b);
  assert(PetSave::choose(a,sizeof(a),b,sizeof(b),loaded)&&loaded.sequence==8);
  b[13]^=1;
  assert(PetSave::choose(a,sizeof(a),b,sizeof(b),loaded)&&loaded.sequence==7);
  assert(!PetSave::choose(nullptr,0,b,sizeof(b),loaded));
  r.sequence=UINT32_MAX;PetSave::encode(r,a);r.sequence=0;PetSave::encode(r,b);
  assert(PetSave::choose(a,sizeof(a),b,sizeof(b),loaded)&&loaded.sequence==0);
  // A self-consistent but impossible state is rejected, too.
  PetSave::put(b+4*4,101);PetSave::put(b+PetSave::Bytes-4,PetSave::hash(b,PetSave::Bytes-4));
  assert(!PetSave::decode(b,sizeof(b),loaded));

  PetModel endurance;
  for(unsigned n=0;n<100000;++n) {
    if(n%200==0)endurance.begin(PetActivity((n/200)%6));
    endurance.tick(20);endurance.tap(70+(n*29)%220,140+(n*31)%180);
    const auto &s=endurance.state;
    assert(s.satiety>=20&&s.satiety<=100&&s.clean>=20&&s.clean<=100);
    assert(s.mood>=20&&s.mood<=100&&s.energy>=20&&s.energy<=100);
    assert(s.wasteMask<=7&&s.pendingMeals<=3&&s.growth<=240&&s.goodSeconds<60);
    if(n%100==0) {
      PetSave::Record record;record.pet=s;PetSave::encode(record,a);
      assert(PetSave::decode(a,sizeof(a),loaded));
    }
  }
  puts("PASS: feed limits, 3-step bath, play, litter, sleep, digestion, balanced growth, reward cooldown, 100000-step bounds, save corruption/fallback/roundtrip/sequence wrap");
}
