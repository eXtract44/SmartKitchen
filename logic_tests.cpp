#include "logic.h"
#include <initializer_list>
constexpr bool allAlarmCombinations() {
  for(unsigned n=0;n<64;n++) {
    const bool w[3]={bool(n&1),bool(n&2),bool(n&4)};
    if(Kitchen::alarms(w,n&8,n&16,n&32)!=n) return false;
    if(Kitchen::buzzer(0,n!=0)!=(n!=0)) return false;
    if(Kitchen::buzzer(350,n!=0)) return false;
  }
  return true;
}
constexpr bool waterBounds() {
  for(int wet: {1700,1650,1900}) {
    int previous=100;
    for(int raw=0;raw<=4095;raw++) {
      int p=Kitchen::waterPercent(raw,wet);
      if(p<0||p>100||p>previous) return false;
      previous=p;
    }
    if(Kitchen::waterPercent(wet,wet)!=100||Kitchen::waterPercent(4095,wet)!=0) return false;
  }
  return true;
}
static_assert(allAlarmCombinations(),"No normal sensor may cancel another alarm");
static_assert(waterBounds(),"ADC mapping must be bounded and monotonic");
static_assert(!Kitchen::hysteresis(false,9,10,5)&&Kitchen::hysteresis(false,10,10,5));
static_assert(Kitchen::hysteresis(true,6,10,5)&&!Kitchen::hysteresis(true,5,10,5));
static_assert(Kitchen::elapsed(20,0xfffffff0u,36),"millis overflow");
static_assert(!Kitchen::elapsed(20,0xfffffff0u,37),"millis overflow boundary");
static_assert(Kitchen::nightHour(23,22,7)&&Kitchen::nightHour(6,22,7)&&!Kitchen::nightHour(7,22,7));
static_assert(!Kitchen::nightHour(12,7,7));
constexpr bool debounceTest() {
  Kitchen::WaterLatch w;
  if(w.update(50,100)||w.update(0,300)||w.update(50,400)||w.update(50,899)) return false;
  if(!w.update(50,900)||!w.update(6,950)||!w.update(5,1000)||!w.update(5,1499)) return false;
  return !w.update(5,1500);
}
static_assert(debounceTest(),"Noise must not trigger or clear wet alarm");
static_assert(Kitchen::combinedStars(200,200,200,200)==5);
static_assert(Kitchen::combinedStars(500,200,200,200)==1);
static_assert(Kitchen::combinedStars(200,500,200,200)==1,"No averaging can hide either channel");
static_assert(Kitchen::combinedStars(200,200,0,200)==0,"No rating without a baseline");
static_assert(Kitchen::airStars(19)==5 && Kitchen::airStars(20)==4 && Kitchen::airStars(40)==3);
static_assert(Kitchen::airStars(75)==2 && Kitchen::airStars(125)==1);
