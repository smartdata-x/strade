//
// Created by Harvey on 2017/1/19.
//

#include "strade_share_timer.h"
#include "logic/stock_util.h"
#include "basic/basic_util.h"

using namespace base_logic;
using namespace strade_share;
using namespace strade_logic;
using namespace stock_logic;

namespace strade_share_logic {

strade_share::SSEngine* StradeShareTimer::ss_engine_ = NULL;
StradeShareTimer::StradeShareTimer() {
  if (!InitParam()) {
    LOG_ERROR("StradeShareTimer init error");
    assert(0);
  }
}

StradeShareTimer* StradeShareTimer::GetInstance() {
  static StradeShareTimer instance;
  return &instance;
}

bool StradeShareTimer::InitParam() {
  strade_share::SSEngine* (* ss_engine)(void);
  std::string strade_share_library = "./strade_share/strade_share.so";
  std::string strade_share_func = "GetStradeShareEngine";
  ss_engine = (strade_share::SSEngine* (*)(void))
      base_logic::SomeUtils::GetLibraryFunction(
          strade_share_library, strade_share_func);
  ss_engine_ = (*ss_engine)();
  if (!ss_engine_) {
    return false;
  }
  ss_engine_->AttachObserver(this);
  return true;
}

void StradeShareTimer::Update(int opcode, void* param) {
  switch (opcode) {
    case REALTIME_MARKET_VALUE_UPDATE: {
      JudgeUpdateTodayHist();
      break;
    }
    default: {
      break;
    }
  }
}

bool StradeShareTimer::OnTimeLoadStockVisit() {
  static const std::string LOAD_STOCK_VISIT_SQL =
      "SELECT stock_code, SUM(COUNT) FROM `stock_visit` GROUP BY stock_code";
  ss_engine_->AddMysqlAsyncJob(2, LOAD_STOCK_VISIT_SQL,
                               OnTimeLoadStockVisitCallback, MYSQL_READ);
  return true;
}

void StradeShareTimer::OnTimeLoadStockVisitCallback(
    int column_num, MYSQL_ROWS_VEC& rows_vec, void* param) {
  bool r = false;
  StockTotalInfo stock_total_info;
  MYSQL_ROWS_VEC::iterator row_iter(rows_vec.begin());
  for (; row_iter != rows_vec.end(); ++row_iter) {
    std::vector<std::string>& column_vec = (*row_iter);
    assert(column_vec.size() == column_num);
    std::string stock_code = column_vec[0];
    if (stock_code.empty()) {
      continue;
    }
    int visit_count = atoi(column_vec[1].c_str());
    r = ss_engine_->GetStockTotalInfoByCode(
        stock_code, stock_total_info);
    if (r) {
      stock_total_info.set_visit_num(visit_count);
    }
  }
}

bool StradeShareTimer::JudgeUpdateTodayHist() {
  bool r = false;
  time_t curr_market_time = ss_engine_->market_time();
  time_t market_close_time = StockUtil::Instance()->market_close_time();
  if (curr_market_time < market_close_time) {
    return false;
  }
  LOG_DEBUG2("curr_market_time=%d, market_close_time=%d",
             curr_market_time, market_close_time);
  // 更新当天历史数据, 写数据库
  StockRealInfo stock_real_info;
  StockHistInfo stock_pre_hist_info;
  STOCKS_MAP stocks_map = ss_engine_->GetAllStockTotalMapCopy();
  STOCKS_MAP::iterator iter(stocks_map.begin());
  std::string today_date = StockUtil::Instance()->get_current_day_str();
  std::vector<StockRealInfo> today_all_stock_hist_vec;
  for (; iter != stocks_map.end(); ++iter) {
    StockTotalInfo& stock_total_info = iter->second;
    r = stock_total_info.GetCurrRealMarketInfo(stock_real_info);
    if (!r) {
      continue;
    }
    StockHistInfo stock_hist_info;
    stock_hist_info.set_code(stock_total_info.get_stock_code());
    stock_hist_info.set_hist_date(today_date);
    stock_hist_info.set_open(stock_real_info.open);
    stock_hist_info.set_high(stock_real_info.high);
    stock_hist_info.set_low(stock_real_info.low);
    stock_hist_info.set_close(stock_real_info.price);
    stock_hist_info.set_volume(stock_real_info.vol);
    double mid_price =
        (stock_real_info.high +
            stock_real_info.low +
            stock_real_info.price) / 3;
    stock_hist_info.set_mid_price(mid_price);

    double day_yield = 0.0;
    r = stock_total_info.GetStockYestodayStockHist(stock_pre_hist_info);
    if (r) {
      double pre_close = stock_pre_hist_info.close();
      day_yield = (stock_hist_info.close() - pre_close) / pre_close;
      stock_hist_info.set_day_yield(day_yield);
    }
    // Add Stock hist data
    stock_total_info.AddStockHistInfoByDate(today_date, stock_hist_info);
    today_all_stock_hist_vec.push_back(stock_real_info);
    if (today_all_stock_hist_vec.size() >= 500) {
      WriteStockCurrHistTODB(today_all_stock_hist_vec);
      today_all_stock_hist_vec.clear();
    }
  }
  WriteStockCurrHistTODB(today_all_stock_hist_vec);
  LOG_MSG2("update today all stock_hist_data finshed, date=%s",
           today_date.c_str());
  return true;
}

void StradeShareTimer::WriteStockCurrHistTODB(
    std::vector<StockRealInfo>& today_hist) {
  if (today_hist.empty()) {
    return;
  }
  std::stringstream ss;
  ss << "REPLACE INFO algo_get_hist_data(";
  ss << "`code`,";
  ss << "`date`,";
  ss << "`open`,";
  ss << "`high`,";
  ss << "`close`,";
  ss << "`low`,";
  ss << "`volume`)";
  ss << " VALUES ";

  std::vector<StockRealInfo>::iterator iter(today_hist.begin());
  for (; iter != today_hist.end(); ++iter) {
    ss << "(" << SerializeStockHistSql(*iter).c_str() << ")";
    ss << ",";
  }
  std::string sql = ss.str();
  sql = sql.substr(0, sql.size() - 1);

  //TODO 此处先改为 test库更新历史数据时通过触发器写入strade库
  //ss_engine_->WriteData(sql);
}

std::string StradeShareTimer::SerializeStockHistSql(
    const StockRealInfo& stock_real_info) {
  std::stringstream ss;
  ss << "'" << stock_real_info.code.c_str() << "',";
  ss << "'" << StockUtil::Instance()->get_current_day_str().c_str() << "',";
  ss << stock_real_info.open << ",";
  ss << stock_real_info.high << ",";
  ss << stock_real_info.close << ",";
  ss << stock_real_info.low << ",";
  ss << stock_real_info.vol << ");";
  return ss.str();
}

void StradeShareTimer::OnTimeTest() {
  //TODO 测试
  return;
  bool r = false;
  std::string stock_code = "000002";
  StockTotalInfo stock_total_info;
  r = ss_engine_->GetStockTotalInfoByCode(stock_code, stock_total_info);
  if (!r) {
    LOG_DEBUG2("%s not exists", stock_code.c_str());
    return;
  }
  LOG_DEBUG2("outstanding=%.2f", stock_total_info.get_outstanding());
  StockRealInfo stock_real_info;
  r = stock_total_info.GetCurrRealMarketInfo(stock_real_info);
  if (!r) {
    LOG_DEBUG("hs300 stock_real_info not exists");
    return;
  }

  LOG_DEBUG2("code=%s, time=%d, price=%.2f",
             stock_real_info.code.c_str(),
             stock_real_info.tradetime,
             stock_real_info.price);

}

} /* namespace strade_share_logic */
