//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2016/10/13 Author: zjc

#include "market_data_engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sstream>

#include "logic/stock_util.h"
#include "logic/logic_comm.h"
#include "storage/redis_controller.h"
#include "strade_share/strade_share_engine.h"

namespace stock_logic {

extern strade_share::SSEngine* engine;

const char DataEngine::LINE_DELIM = ';';
const char DataEngine::FIELD_DELIM = ',';

const char DataEngine::kShIndex[] = "000001";
const char DataEngine::kShSz300[] = "000300";
const char DataEngine::kSzComponentIndex[] = "399001";
const char DataEngine::kGEMIndex[] = "399006";

DataEngine::DataEngine()
    : type_(SH),
      lock_(NULL),
      db_(NULL),
      redis_(NULL),
      initialized_(false),
      current_(&data_[0]) {
  InitThreadrw(&lock_);
#ifdef DEBUG_TEST
  LOG_MSG("DEBUG_TEST FOR DataEngine");
#endif
}

DataEngine::~DataEngine() {
  DeinitThreadrw(lock_);
}

void DataEngine::Init(Type type, const std::string& hkey_prefix,
          StockDB* db, RedisController* redis) {
  type_ = type;
  hkey_prefix_ = hkey_prefix;
  db_ = db;
  redis_ = redis;

  char str_date[10];
  int buf_days = BUFFER_DAYS;
  StockUtil* util = StockUtil::Instance();
  int date = util->Date();
  if (util->is_trading_day()) --buf_days;
  for (size_t i = 0; i < buf_days; ++i) {
    date = util->PreTradeDate(date);
    sprintf(str_date, "%d", date);
    current_->trade_date = str_date;
    LOG_MSG2("init %d data", date);
    if (!FillBuffer(str_date)) {
      LOG_ERROR2("init %s data", date);
    }
    switch_to_prev_buffer();
  }
  switch_to_prev_buffer();

  LOG_MSG2("RealtimeCodeInfo size: %d, mem_pool_size: %d",
           sizeof(StockRealInfo), MEM_POOL_SIZE);
}

void DataEngine::ClearDataMap(DataMap& data_map) {
  for (DataMap::iterator it = data_map.begin();
      data_map.end() != it; ++it) {
    CodeMap &code_map = it->second;
    for (CodeMap::iterator it2 = code_map.begin();
        it2 != code_map.end(); ++it2) {
      it2->second->~StockRealInfo();
    }
  }
}

bool DataEngine::SplitLine(const std::string& info, StringArray2& res) {
  if (info.empty()) {
    return false;
  }

  size_t start = 0;
  for (size_t i = 0; i < info.size(); ++i) {
    if (LINE_DELIM == info[i]) {
      StringArray v;
      std::string line(&info[start], &info[i]);
//      LOG_DEBUG2("line: %s", line.c_str());
      SplitField(line, v);
      res.push_back(v);
      start = i+1;
    }
  }
  return true;
}

bool DataEngine::SplitField(const std::string& line, StringArray& res) {
  std::stringstream ss(line);
  std::string field;
  StockRealInfo code;
  res.clear();

  size_t start = 0;
  for (size_t i = 0; i < line.size(); ++i) {
    if (FIELD_DELIM == line[i]) {
      res.push_back(std::string(&line[start], &line[i]));
      start = i+1;
    }
  }

  return true;
}

bool DataEngine::FilterOutOldData(const StringArray& all_times,
                               StringArray& new_times) {
  new_times.clear();
  for (size_t i = 0; i < all_times.size(); ++i) {
    if (all_times[i].empty()) {
      continue;
    }

    int t = atoi(all_times[i].c_str());
//    if (!current_->mtime_set.exist(t)) {
    if (0 == current_->time_set.count(t)) {
      current_->time_set.insert(t);
      new_times.push_back(all_times[i]);
    }
  }
  return !new_times.empty();
}

void DataEngine::ProcessIndex(StockRealInfo& info) {
  if (SH == type_) {
    if (kShIndex == info.code) {
      info.code = "sh" + info.code;
    } else if (kShSz300 == info.code) {
      info.code = "hs300";
    }
  } else if (SZ == type_) {
    if (kSzComponentIndex == info.code
        || kGEMIndex == info.code) {
      info.code = "sh" + info.code;
    }
  }
}

bool DataEngine::FetchRawData(const std::string& date,
                                     const std::string& market_time,
                              CodeInfoArray& res) {
  std::string raw_data;
  std::string hkey = hkey_prefix_ + date;
  if (!redis_->GetHashElement(hkey, market_time, raw_data)) {
    LOG_ERROR2("fetch data error, hkeys: %s, market_time: %s",
        hkey.c_str(), market_time.c_str());
    return false;
  }

  StringArray2 fields;
  SplitLine(raw_data, fields);

  res.clear();
  res.reserve(fields.size());

  for (size_t i = 0; i < fields.size(); ++i) {
    StockRealInfo code;
    if (code.Deserialize(fields[i])) {
      ProcessIndex(code);
      res.push_back(code);
    }
  }
  return !res.empty();
}

bool DataEngine::FillBuffer(const std::string& trade_date) {
  StringArray time_list;
  std::string hkey = hkey_prefix_ + trade_date;
  if (!redis_->GetAllHashFields(hkey, time_list)) {
    LOG_ERROR2("get time list error: %s", hkey.c_str());
    return false;
  }

  CodeInfoArray data;
  for (size_t i = 0; i < time_list.size(); ++i) {
    if (!FetchRawData(trade_date, time_list[i], data)
        || data.empty()) {
      continue;
    }

    LOG_MSG2("build time: %s", time_list[i].c_str());
    int t = atoi(time_list[i].c_str());
    current_->mtime_set.Add(t);

    CodeMap& code_map = current_->data_map[t];
    for (size_t j = 0; j < data.size(); ++j) {
//      LOG_DEBUG2("construct code: %s", data[j].code.c_str());
      StockRealInfo* code = current_->mem_pool.Construct(data[j]);
//      RealtimeCodeInfo* code = new RealtimeCodeInfo(data[i]);
      code_map.insert(CodeMap::value_type(code->code, code));
    }
  }
  return true;
}

bool DataEngine::Append(int market_time, const CodeInfoArray& data) {
  CodeMap& code_map = current_->data_map[market_time];
  LOG_DEBUG2("time: %d, data size: %d", market_time, data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    StockRealInfo* code = current_->mem_pool.Construct(data[i]);
//    RealtimeCodeInfo* code = new RealtimeCodeInfo(data[i]);
    code_map.insert(CodeMap::value_type(code->code, code));
  }
  return true;
}

bool DataEngine::Update() {
  StringArray all_times;
  std::string hkey = hkey_prefix_ + current_->trade_date;
  if (!redis_->GetAllHashFields(hkey, all_times)) {
    LOG_ERROR2("get market time error: %s", hkey.c_str());
    return false;
  }

  StringArray new_times;
  if (!FilterOutOldData(all_times, new_times)) {
    return false;
  }

  CodeInfoArray data;
  StockUtil* util = StockUtil::Instance();
  for (size_t i = 0; i < new_times.size(); ++i) {
    if (!FetchRawData(current_->trade_date, new_times[i], data)
        || data.empty()) {
      continue;
    }
    int t = atoi(new_times[i].c_str());
    time_t ts = util->to_timestamp(t);
#ifdef DEBUG_TEST
    Append(ts, data);
#else
    engine->UpdateStockRealMarketData(ts, data);
#endif
  }
  return true;
}

void DataEngine::OnTime() {
  base_logic::WLockGd lock(lock_);

  ++current_->tick;
  if (!(StockUtil::Instance()->is_trading_day()
      && is_work_time())) {
    initialized_ = false;
    return ;
  }

  if (!initialized_) {
    initialized_ = true;
    char str_date[10];
    switch_to_next_buffer();
    current_->reset();
    StockUtil* util = StockUtil::Instance();
    int today = util->Date();
    sprintf(str_date, "%d", today);
    current_->trade_date = str_date;
    LOG_MSG2("new trade day: %d", today);
  }
  Update();
}

void DataEngine::Simulate() {
  base_logic::WLockGd lock(lock_);
  static time_t last = 1488504600;
  CodeMap* code_map;
  DataMap::iterator it = current_->data_map.begin();
  for (; current_->data_map.end() != it; ++it) {
    if(it->first > last) {
      last = it->first;
      code_map = &(it->second);
      break;
    }
  }
  if (current_->data_map.end() == it) {
       return ;
  }
  CodeInfoArray data;
  CodeMap::iterator iter(code_map->begin());
  for(; iter != code_map->end(); ++iter) {
    data.push_back(*(iter->second));
  }
  engine->UpdateStockRealMarketData(last, data);
}

} /* namespace stock_logic */
