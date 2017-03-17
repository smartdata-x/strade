//
// Created by Harvey on 2017/1/7.
//

#include "strade_basic_info.h"
#include "logic_unit.h"

namespace strade_logic {

/*
StockRealInfo::StockRealInfo() {
  data_ = new Data();
}

StockRealInfo::StockRealInfo(const StockRealInfo& rhs)
    : data_(rhs.data_) {
  if (NULL != data_) {
    data_->AddRef();
  }
}

StockRealInfo& StockRealInfo::operator=(const StockRealInfo& rhs) {
  if (this == &rhs) {
    return (*this);
  }
  if (NULL == rhs.data_) {
    rhs.data_->AddRef();
  }
  if (NULL != data_) {
    data_->AddRef();
  }
  data_ = rhs.data_;
  return (*this);
}

StockRealInfo::~StockRealInfo() {
  if (NULL != data_) {
    data_->Release();
  }
}
*/

StockHistInfo::StockHistInfo() {
  data_ = new Data();
}

StockHistInfo::StockHistInfo(const StockHistInfo& rhs)
    : data_(rhs.data_) {
  if (NULL != data_) {
    data_->AddRef();
  }
}

StockHistInfo& StockHistInfo::operator=(const StockHistInfo& rhs) {
  if (this == &rhs) {
    return (*this);
  }
  if (NULL != rhs.data_) {
    rhs.data_->AddRef();
  }
  if (NULL != data_) {
    data_->Release();
  }
  data_ = rhs.data_;
  return (*this);
}

StockHistInfo::~StockHistInfo() {
  if (NULL != data_) {
    data_->Release();
  }
}

void StockHistInfo::Deserialize() {
  double qfq_close = 0.0;

  GetString(0, data_->date_);
  GetReal(1, data_->open_);
  GetReal(data_->high_);
  GetReal(3, data_->close_);
  GetReal(4, data_->low_);
  GetReal(5, data_->volume_);

  if (qfq_close > 1) {
    data_->qfq_close_ = qfq_close;
  } else {
    data_->qfq_close_ = data_->close_;
  }
  data_->mid_price_ = (data_->high_ + data_->low_ + data_->close_) / 3;

//  LOG_DEBUG2("date=%s, open=%.2f, high=%.2f, close=%.2f",
//             data_->date_.c_str(),
//             data_->open_,
//             data_->high_,
//             data_->close_);
}

StockTotalInfo::StockTotalInfo() {
  data_ = new Data();
}

StockTotalInfo::StockTotalInfo(const StockTotalInfo& rhs)
    : data_(rhs.data_) {
  if (NULL != data_) {
    data_->AddRef();
  }
}

StockTotalInfo& StockTotalInfo::operator=(const StockTotalInfo& rhs) {
  if (this == &rhs) {
    return (*this);
  }
  if (NULL != rhs.data_) {
    rhs.data_->AddRef();
  }
  if (NULL != data_) {
    data_->Release();
  }
  data_ = rhs.data_;
  return (*this);
}

StockTotalInfo::~StockTotalInfo() {
  if (NULL != data_) {
    data_->Release();
  }
}

void StockTotalInfo::Deserialize() {
  GetString(0, data_->code_);
  GetString(1, data_->name_);
  GetReal(2, data_->outstanding_);
  GetReal(data_->totalAssets_);
  GetReal(data_->bvps_);
  GetReal(data_->pb_);
  GetString(data_->industry_);
  data_->market_value_ = data_->outstanding_ * data_->bvps_ * data_->pb_;
}

void StockTotalInfo::ClearRealMap() {
  data_->stock_real_map_.clear();
}

STOCK_HIST_MAP StockTotalInfo::GetStockHistMap() const {
  return data_->stock_hist_map_;
}

STOCK_REAL_MAP StockTotalInfo::GetStockRealMap() const {
  return data_->stock_real_map_;
}

bool StockTotalInfo::AddStockHistInfoByDate(
    const std::string& date, const StockHistInfo& stock_hist_info) {
//  LOG_DEBUG2("AddStockHistInfoByDate stock_code=%s, date=%s",
//             (data_->code_).c_str(), date.c_str());
  STOCK_HIST_MAP::iterator iter(data_->stock_hist_map_.find(date));
  if (iter == data_->stock_hist_map_.end()) {
    ReplaceStockHistInfo(date, stock_hist_info);
    return true;
  }
  return false;
}

bool StockTotalInfo::AddStockHistVec(
    std::vector<StockHistInfo>& stock_hist_vec) {
  std::vector<StockHistInfo>::iterator iter(stock_hist_vec.begin());
  for (; iter != stock_hist_vec.end(); ++iter) {
    const std::string& date = (*iter).GetHistDate();
    ReplaceStockHistInfo(date, (*iter));
  }
  return true;
}

bool StockTotalInfo::GetStockHistInfoByDate(
    const std::string& date, StockHistInfo& stock_hist_info) {
  STOCK_HIST_MAP::iterator iter(data_->stock_hist_map_.find(date));
  if (iter != data_->stock_hist_map_.end()) {
    stock_hist_info = iter->second;
    return true;
  }
  return false;
}

bool StockTotalInfo::ReplaceStockHistInfo(
    const std::string& date, const StockHistInfo& stock_hist_info) {
  data_->stock_hist_map_[date] = stock_hist_info;
  return true;
}

bool StockTotalInfo::AddStockRealInfoByTime(
    const time_t trade_time, const StockRealInfo& stock_real_info) {
  STOCK_REAL_MAP::iterator iter(data_->stock_real_map_.find(trade_time));
  if (iter == data_->stock_real_map_.end()) {
    ReplaceStockRealInfo(trade_time, stock_real_info);
    return true;
  }
  return false;
}

bool StockTotalInfo::GetStockRealInfoByTradeTime(
    const time_t trade_time, StockRealInfo& stock_real_info) {
  STOCK_REAL_MAP::iterator iter(data_->stock_real_map_.find(trade_time));
  if (iter != data_->stock_real_map_.end()) {
    stock_real_info = iter->second;
    return true;
  }
  return false;
}

bool StockTotalInfo::ReplaceStockRealInfo(
    const time_t trade_time, const StockRealInfo& stock_real_info) {
  data_->stock_real_map_[trade_time] = stock_real_info;
  return true;
}

bool StockTotalInfo::GetCurrRealMarketInfo(StockRealInfo& stock_real_info) {
  if (data_->stock_real_map_.empty()) {
    return false;
  }
  STOCK_REAL_MAP::reverse_iterator iter(
      data_->stock_real_map_.rbegin());
  stock_real_info = iter->second;
  return true;
}

bool StockTotalInfo::GetStockYestodayStockHist(
    StockHistInfo& stock_hist_info) {
  STOCK_HIST_MAP::reverse_iterator riter(data_->stock_hist_map_.rbegin());
  if (riter != data_->stock_hist_map_.rend()) {
    stock_hist_info = riter->second;
    return true;
  }
  return false;
}

} /* namespace strade_logic */
