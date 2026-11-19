#pragma once
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include "PetWalk.h"
enum class PetActivity { Room, Feed, Bath, Play, Litter, Sleep };
enum class PetSignal { None, Stroke, Bite, Bubble, Rinse, Dry, Ball, Scoop, Rest, Complete, Full, Tired, Grow, Hint, Invite };
struct PetState {
  uint32_t satiety=58,clean=55,mood=58,energy=70,growth=0,activeSeconds=0;
  uint32_t wasteMask=1,pendingMeals=0,digestSeconds=0,completedMask=0;
  uint32_t totalFeeds=0,totalBaths=0,totalPlayHits=0,totalScoops=0,goodSeconds=0;
  uint32_t lastReward[4]={UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX};
  unsigned stage() const { return growth>=180?2:growth>=60?1:0; }
  unsigned wasteCount() const { return (wasteMask&1)+((wasteMask>>1)&1)+((wasteMask>>2)&1); }
  bool wellKept() const { return satiety>=55&&clean>=60&&mood>=55&&energy>=45&&wasteCount()<=1; }
};
struct PetModel {
  PetState state;
  PetWalk walk;
  unsigned roomSize() const { return 88+(state.growth>180?180:state.growth)*20/180; }
  bool hitCat(int x,int y) const {
    return fabsf(x-walk.x)<roomSize()*(walk.stand>.5f?.64f:.43f)&&fabsf(y-(walk.y-roomSize()/2))<roomSize()*.47f;
  }
  PetActivity activity=PetActivity::Room;
  PetSignal signal=PetSignal::None;
  uint32_t revision=1,saveRevision=0,clockMs=0,millisecond=0,lastStroke=UINT32_MAX-500;
  unsigned progress=0,phase=0,spots=0,strokes=0;
  bool done=false,eligible=false,earned=false,called=false;
  PetSignal feedback=PetSignal::None;
  uint32_t feedbackAt=0,attentionUntil=0,inviteClock=0;
  int feedbackX=180,feedbackY=210;
  PetActivity need() const {
    if(state.energy<40)return PetActivity::Sleep;
    if(state.wasteMask)return PetActivity::Litter;
    if(state.satiety<65)return PetActivity::Feed;
    if(state.clean<65)return PetActivity::Bath;
    if(state.mood<70)return PetActivity::Play;
    return PetActivity::Room;
  }
  void respond(PetSignal event,int x=180,int y=210) {
    signal=feedback=event;feedbackAt=clockMs;feedbackX=x;feedbackY=y;
  }
  uint32_t feedbackAge() const { return clockMs-feedbackAt; }
  static bool actionHit(int x,int y) { return x>=108&&x<=252&&y>=289&&y<337; }
  void acceptNeed() { if(need()==PetActivity::Room)stroke();else begin(need()); }

  static uint32_t raise(uint32_t v,uint32_t amount) { return v+amount>100?100:v+amount; }
  static uint32_t lower(uint32_t v,uint32_t amount) { return v>20+amount?v-amount:20; }
  float ballX() const { unsigned p=(clockMs/6)%360;return 90+(p<180?p:360-p); }
  float ballY() const { return 201-fabsf(sinf(clockMs/280.f))*35-(feedback==PetSignal::Ball&&feedbackAge()<400?14*sinf(feedbackAge()*3.1415926f/400):0); }
  void changed(bool persistent=true) { ++revision;if(persistent)++saveRevision; }
  void grow(unsigned amount) {
    unsigned before=state.stage();state.growth=state.growth+amount>240?240:state.growth+amount;
    if(before!=state.stage())signal=PetSignal::Grow;
  }
  void reward(unsigned category,unsigned amount) {
    uint32_t before=state.growth;
    bool ready=state.lastReward[category]==UINT32_MAX||state.activeSeconds-state.lastReward[category]>=180;
    if(eligible&&ready) { grow(amount);state.lastReward[category]=state.activeSeconds; }
    earned=state.growth>before;
  }
  void begin(PetActivity next) {
    uint32_t previousMask=state.completedMask;
    activity=next;progress=phase=spots=0;done=false;eligible=false;earned=false;called=false;
    signal=feedback=PetSignal::None;inviteClock=0;attentionUntil=0;
    if(next==PetActivity::Feed)eligible=state.satiety<85;
    if(next==PetActivity::Bath)eligible=state.clean<85;
    if(next==PetActivity::Play)eligible=state.mood<90&&state.energy>=25;
    if(next==PetActivity::Litter)eligible=state.wasteMask!=0;
    if(next==PetActivity::Sleep) { signal=PetSignal::Rest;state.completedMask|=16; }
    if(next==PetActivity::Feed&&!eligible)respond(PetSignal::Full);
    if(next==PetActivity::Play&&state.energy<=25)respond(PetSignal::Tired);
    changed(state.completedMask!=previousMask);
  }
  void stroke() {
    if(clockMs-lastStroke<450)return;
    walk.rest();
    lastStroke=clockMs;++strokes;uint32_t before=state.mood;state.mood=raise(state.mood,2);
    attentionUntil=clockMs+1600;called=false;respond(PetSignal::Stroke,walk.x,walk.y-roomSize()/2);changed(state.mood!=before);
  }
  void finish(unsigned category,unsigned xp) {
    done=true;state.completedMask|=1u<<category;signal=PetSignal::Complete;
    reward(category,xp);changed();
  }
  void tap(int x,int y) {
    if(activity==PetActivity::Room) {
      if(y>=258&&y<324&&x>=56&&x<304) {
        unsigned i=x<118?0:x<180?1:x<242?2:3;
        begin(i==0?PetActivity::Feed:i==1?PetActivity::Bath:i==2?PetActivity::Play:PetActivity::Litter);return;
      }
      if(x>=242&&x<=306&&y>=68&&y<=111) { acceptNeed();return; }
      if(hitCat(x,y)) { stroke();return; }
      if(x>=253&&x<313&&y>=155&&y<218) { begin(PetActivity::Sleep);return; }
      else if(x>=65&&x<=295&&y>=214&&y<258) { walk.go(x,y,true);called=true; }
      return;
    }
    if(activity==PetActivity::Sleep&&x>=125&&x<=235&&y>=143&&y<=255) { begin(PetActivity::Room);return; }
    if((activity==PetActivity::Feed&&!eligible)||(activity==PetActivity::Play&&state.energy<=25)) {
      if(actionHit(x,y)||(x>=253&&x<=313&&y>=148&&y<=218)) {
        begin(activity==PetActivity::Feed?PetActivity::Play:PetActivity::Sleep);return;
      }
    }
    bool action=actionHit(x,y);
    if(action) {
      if(done||activity==PetActivity::Sleep||activity==PetActivity::Litter||
         (activity==PetActivity::Feed&&!eligible)||
         (activity==PetActivity::Play&&state.energy<=25)) { begin(PetActivity::Room);return; }
      if(activity==PetActivity::Bath&&phase==0&&spots!=15)return;
      // The large action button is a second generous target in feed/rinse/dry.
    } else if(x<65||x>295||y<132||y>266)return;
    if(done)return;
    if(activity==PetActivity::Feed) {
      if(!eligible) { respond(PetSignal::Full);changed(false);return; }
      if(!action&&!((x>=128&&x<=232&&y>=211&&y<=270)||(x>=145&&x<=215&&y>=157&&y<=211))) {
        respond(PetSignal::Hint,180,245);changed(false);return;
      }
      if(progress==0) {
        if(!state.pendingMeals)state.digestSeconds=0;
        state.pendingMeals=state.pendingMeals<3?state.pendingMeals+1:3;
      }
      ++progress;state.satiety=raise(state.satiety,8);state.energy=raise(state.energy,1);
      respond(PetSignal::Bite,180,238);changed();
      if(progress>=3) {
        ++state.totalFeeds;finish(0,6);
      }
    } else if(activity==PetActivity::Bath) {
      if(phase==0) {
        unsigned before=spots;
        const int sx[]={128,218,139,223},sy[]={162,168,220,225};
        for(unsigned i=0;i<4;++i)if(abs(x-sx[i])<=35&&abs(y-sy[i])<=33) {
          unsigned bit=1u<<i;
          if(!(spots&bit)) { spots|=bit;respond(PetSignal::Bubble,sx[i],sy[i]);changed(false); }
          break;
        }
        if(spots==15) { phase=1;progress=0;changed(false); }
        else if(spots==before) {
          for(unsigned i=0;i<4;++i)if(!(spots&(1u<<i))) { respond(PetSignal::Hint,sx[i],sy[i]);changed(false);break; }
        }
      } else {
        bool target=phase==1?(x>=112&&x<=248&&y>=136&&y<=254):(x>=130&&x<=230&&y>=208&&y<=262);
        if(!action&&!target) { respond(PetSignal::Hint,180,phase==1?180:235);changed(false);return; }
        ++progress;respond(phase==1?PetSignal::Rinse:PetSignal::Dry,x,y);changed(false);
        if(progress>=3) {
          if(phase==1) { phase=2;progress=0; }
          else {
            state.clean=raise(state.clean,40);state.mood=raise(state.mood,8);++state.totalBaths;
            finish(1,8);
          }
        }
      }
    } else if(activity==PetActivity::Play) {
      if(state.energy<=25) { respond(PetSignal::Tired);changed(false);return; }
      if(fabsf(x-ballX())<44&&fabsf(y-ballY())<44) {
        ++progress;++state.totalPlayHits;state.mood=raise(state.mood,5);state.energy=lower(state.energy,2);
        respond(PetSignal::Ball,x,y);changed();if(progress>=5)finish(2,6);
      } else { respond(PetSignal::Hint,ballX(),ballY());changed(false); }
    } else if(activity==PetActivity::Litter&&y>=170&&y<=262) {
      unsigned slot=x<150?0:x<210?1:2,bit=1u<<slot;
      if((state.wasteMask&bit)&&abs(x-int(120+slot*60))<=28&&abs(y-218)<=36) {
        state.wasteMask&=~bit;++state.totalScoops;state.clean=raise(state.clean,6);state.mood=raise(state.mood,3);
        respond(PetSignal::Scoop,120+slot*60,218);changed();if(!state.wasteMask)finish(3,6);
      }
    }
  }
  void tick(uint32_t dt,bool visible=true) {
    if(visible&&(activity==PetActivity::Room||activity==PetActivity::Play))
      walk.tick(dt,activity==PetActivity::Play&&!done,ballX(),roomSize()*.28f);
    clockMs+=dt;millisecond+=dt;
    if(visible&&activity==PetActivity::Room) {
      inviteClock+=dt;
      if(called&&fabsf(walk.x-walk.targetX)<1&&fabsf(walk.y-walk.targetY)<1) {
        called=false;walk.rest();attentionUntil=clockMs+2200;respond(PetSignal::Invite);changed(false);
      } else if(inviteClock>=8000) {
        inviteClock=0;
        if(!called&&walk.pause&&clockMs-lastStroke>2500) {
          walk.rest();attentionUntil=clockMs+1800;respond(PetSignal::Invite);changed(false);
        }
      }
    }
    while(millisecond>=1000) {
      millisecond-=1000;++state.activeSeconds;
      bool visible=false;
      if(activity==PetActivity::Sleep&&state.activeSeconds%3==0) {
        uint32_t before=state.energy;state.energy=raise(state.energy,1);visible|=before!=state.energy;
        if(before!=state.energy)++saveRevision;
      }
      if(state.activeSeconds%60==0) {
        state.satiety=lower(state.satiety,2);
        state.clean=lower(state.clean,state.wasteMask?3:1);
        state.mood=lower(state.mood,state.wasteMask?2:1);
        if(activity!=PetActivity::Sleep)state.energy=lower(state.energy,1);
        visible=true;++saveRevision;
      }
      if(state.pendingMeals) {
        if(++state.digestSeconds>=150) {
          if(state.wasteMask!=7) {
            for(unsigned i=0;i<3;++i)if(!(state.wasteMask&(1u<<i))) { state.wasteMask|=1u<<i;break; }
            --state.pendingMeals;
          }
          state.digestSeconds=0;visible=true;++saveRevision;
        }
      }
      if(state.wellKept()) {
        if(++state.goodSeconds>=60) {
          state.goodSeconds=0;unsigned before=state.growth;grow(1);
          visible|=before!=state.growth;++saveRevision;
        }
      } else state.goodSeconds=0;
      if(visible)++revision;
    }
  }
  PetSignal takeSignal() { auto s=signal;signal=PetSignal::None;return s; }
};
