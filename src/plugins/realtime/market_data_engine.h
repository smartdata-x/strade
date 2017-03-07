//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2016/10/13 Author: zjc

#ifndef SRC_PLUGINS_REALTIME_MARKET_DATA_ENGINE_H_
#define SRC_PLUGINS_REALTIME_MARKET_DATA_ENGINE_H_

#include <string>
#include <set>

#include "market_time_manager.h"
#include "mem_pool.h"
#include "realtime_code_info.h"
#include "thread/base_thread_lock.h"

namespace base_logic {
class RedisController;
}
using base_logic::RedisController;

namespace stock_logic {

class DataEngine {
 public:
  typedef std::map<std::string, StockRealInfo*> CodeMap;
  typedef std::map<int, CodeMap> DataMap;
  typedef std::vector<std::string> StringArray;
  typedef std::vector<StringArray> StringArray2;
  typedef std::vector<int> TimeList;
  typedef std::vector<StockRealInfo> CodeInfoArray;
  typedef std::set<int> TimeSet;
  enum Type {
    SH,
    SZ,
    MAX_TYPE
  };

  struct Option {
    Type type;
    std::string hkey_prefix;
    StockDB* db;
    RedisController* redis;
  };

 public:
  void Simulate();
  void Init(Type type, const std::string& hkey_prefix,
            StockDB* db, RedisController* redis);
  DataEngine();
  ~DataEngine();
  void OnTime();
  int market_time() const { return current_->mtime_set.market_time(); }

  bool is_work_time() const {
    time_t now = time(NULL);
    struct tm* pt = localtime(&now);
    int t = pt->tm_hour * 100 + pt->tm_min;
    if (t >= START_WORK_TIME && t <= END_WORK_TIME) {
      return true;
    }
    return false;
  }

  std::string string_type() const {
    switch (type_) {
      case SH:
        return "SH";
      case SZ:
        return "SZ";
      default:
        return "UNKNOWN";
    }
  }

 private:
  void ClearDataMap(DataMap& data_map);

  void switch_to_prev_buffer() {
    if (--current_ < &data_[0]) {
      current_ = &data_[BUFFER_DAYS-1];
    }
  }

  void switch_to_next_buffer() {
    ++current_;
    if (current_ > &data_[BUFFER_DAYS-1]) {
      current_ = &data_[0];
    }
  }

  bool FillBuffer(const std::string& trade_date);

  bool Update();
  bool FilterOutOldData(const StringArray& all_times, StringArray& new_times);
  bool Append(int market_time, const CodeInfoArray& data);

  void Reset();

  bool FetchRawData(const std::string& date,
                 const std::string& market_time, CodeInfoArray& res);
  bool SplitLine(const std::string& info, StringArray2& res);
  bool SplitField(const std::string& line, StringArray& res);

  void ProcessIndex(StockRealInfo& info);
 private:

  const static char LINE_DELIM;
  const static char FIELD_DELIM;

  const static char kShIndex[];           // 上证指数
  const static char kShSz300[];           // 沪深300
  const static char kSzComponentIndex[];  // 深证成指
  const static char kGEMIndex[];          // 创业板指

  const static int CODE_COUNT = 2000;
  const static int BUFFER_DAYS = 1;
  const static int MEM_POOL_SIZE = CODE_COUNT * sizeof(StockRealInfo);
  const static int START_WORK_TIME = 915;
  const static int END_WORK_TIME = 2030;

  const static int SECONDS_PER_TICK = 5;
  const static int TIMEOUT = 5 * 60;

  struct Data {
    std::string trade_date;
    int last_mtime;
    int tick;
    bool update_yield;
    MemPool mem_pool;
    DataMap data_map;
    TimeSet time_set;
    MarketTimeManager mtime_set;
    Data()
        : tick(0),
          last_mtime(-1),
          update_yield(false),
          mem_pool(MEM_POOL_SIZE) {}

    void reset() {
      // 清内存池之前, 需要先调用析构函数,由于析构函数是空的,因此此处省略
      mem_pool.Reset();
      data_map.clear();
      time_set.clear();
      mtime_set.reset();
      last_mtime = -1;
      tick = 0;
      update_yield = false;
    }

    bool timeout() {
      if (tick * SECONDS_PER_TICK > TIMEOUT) {
        return true;
      }
      return false;
    }
  };
 private:
  Type type_;
  std::string hkey_prefix_;

  bool initialized_;
  StockDB* db_;
  RedisController* redis_;

  threadrw_t *lock_;

  Data data_[BUFFER_DAYS];
  Data* current_;
};

} /* namespace stock_logic */

#endif /* SRC_PLUGINS_REALTIME_MARKET_DATA_ENGINE_H_ */
