//
// Created by Harvey on 2017/1/4.
//

#include "strade_share_db.h"

#include "basic/scoped_ptr.h"
#include "strade_share_engine.h"

using namespace strade_logic;

namespace strade_share {

StradeShareDB::StradeShareDB(
    config::FileConfig* config) {

  assert(config->mysql_db_list_.size() >= 2);

  LOG_DEBUG2("db_list size=%d", config->mysql_db_list_.size());
  base::ConnAddr read_conn = config->mysql_db_list_.front();
  config->mysql_db_list_.pop_front();

  base::ConnAddr write_conn = config->mysql_db_list_.front();
  config->mysql_db_list_.pop_front();
  mysql_engine_ = new base_logic::MysqlEngine(read_conn, write_conn, true);
}

StradeShareDB::~StradeShareDB() {
  if (NULL != mysql_engine_) {
    delete mysql_engine_;
    mysql_engine_ = NULL;
  }
}

bool StradeShareDB::FetchAllStockList(std::vector<StockTotalInfo>& stock_vec) {
  static const std::string& SQL =
      "SELECT `code`, `name`, `outstanding`, `totalAssets`, `bvps`, `pb`, `industry` FROM `algo_get_stock_basics`;";
  return mysql_engine_->ReadData<StockTotalInfo>(SQL, stock_vec);
}

bool StradeShareDB::FetchStockHistList(const std::string& stock_code,
                                       std::vector<StockHistInfo>& stock_hist_vec) {
  std::string SQL =
      "SELECT date, open, high, close, low, volume FROM algo_get_hist_data WHERE CODE = '"
          + stock_code + "' ORDER BY DATE DESC LIMIT 30;";
  return mysql_engine_->ReadData<StockHistInfo>(SQL, stock_hist_vec);
}

bool StradeShareDB::WriteData(const std::string& sql) {
  return mysql_engine_->WriteData(sql);
}

} /* namespace strade_share */