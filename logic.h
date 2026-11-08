#pragma once
#include <stdint.h>

namespace Kitchen {
constexpr uint32_t WARMUP_MS = 180000;
constexpr int airRise(int raw,int baseline) { return baseline>8 && raw>baseline?(raw-baseline)*100/baseline:0; }
constexpr int airStars(int rise) { return rise<20?5:rise<40?4:rise<75?3:rise<125?2:1; }
constexpr int combinedStars(int a,int b,int baseA,int baseB) {
  return baseA<=8||baseB<=8?0:airStars(airRise(a,baseA)>airRise(b,baseB)?airRise(a,baseA):airRise(b,baseB));
}
constexpr bool elapsed(uint32_t now, uint32_t then, uint32_t interval) { return uint32_t(now-then)>=interval; }
constexpr int waterPercent(int raw, int wet) {
  int n=(4095-raw)*100/(4095-wet);
  return n<0?0:n>100?100:n;
}
constexpr bool hysteresis(bool active, int value, int on, int off) { return active?value>off:value>=on; }
constexpr uint16_t alarms(const bool water[3], bool hot, bool humid, bool gas) {
  return (water[0]?1:0)|(water[1]?2:0)|(water[2]?4:0)|(hot?8:0)|(humid?16:0)|(gas?32:0);
}
constexpr bool buzzer(uint32_t now, bool active) { return active && now%2000<350; }
constexpr bool nightHour(int hour, int from, int until) {
  return from==until?false:from<until?(hour>=from && hour<until):(hour>=from || hour<until);
}
struct WaterLatch {
  bool active=false,pending=false;
  uint32_t changedAt=0;
  constexpr bool update(int percent,uint32_t now) {
    bool target=hysteresis(active,percent,10,5);
    if(target!=pending) { pending=target; changedAt=now; }
    if(elapsed(now,changedAt,500)) active=target;
    return active;
  }
};
}
