#ifndef SRC_REALINFO_REALINFO_TODAY_H
#define SRC_REALINFO_REALINFO_TODAY_H
#include "realinfo_pro.h"

namespace realinfo {

class RealInfoToday {
 public:
  static time_t GetTradeStart() {
    struct tm start;
    time_t ts = time(NULL);
    localtime_r(&ts, &start);
    start.tm_hour = 9;
    start.tm_min = 30;
    start.tm_sec = 0;
    return mktime(&start);
  }  

  static std::string BuildDate(time_t ts) {
    struct tm start;
    localtime_r(&ts, &start);
    char buf[64];
    snprintf(buf, sizeof(buf), "%02d:%02d", start.tm_hour, start.tm_min);
    return buf;
  }
  
  static void CalculateVol(std::list<StockRealInfo> *today) {
    std::list<StockRealInfo>::reverse_iterator it = today->rbegin();
    while (it != today->rend()) {
      std::list<StockRealInfo>::reverse_iterator tmp = it;
      tmp++;
      if (tmp != today->rend())
        it->vol = it->vol - tmp->vol;
      it++;
    }
  }  
 
  static bool RemoveDuplicate(std::list<StockRealInfo> *today); 

};

} // namespace
#endif
