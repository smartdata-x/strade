#include "realinfo/realinfo_today.h"

namespace realinfo {

bool RealInfoToday::RemoveDuplicate(std::list<StockRealInfo> *today) {

    time_t start = GetTradeStart();
    std::list<StockRealInfo>::iterator it = today->begin();
    while (it != today->end()) {
      if (it->ts < start) {
        today->erase(it++);
        continue;
      }
      if (it != today->begin()) {
        std::list<StockRealInfo>::iterator former = it;
        former--;
        if (it->time == former->time) {
          today->erase(former);
        }
      }
      it++;
    }
    return true;
}

}  // namespace realinfo
