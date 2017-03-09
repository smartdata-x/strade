//Copyright (c) 2016-2017 The strade Authors. All rights reserved.
//Created on: 2017/01/09 Author: tangtao

#include "yields/db_mysql.h"

#include <assert.h>
#include <stdio.h>
#include <mysql/mysql.h>
#include <sstream> 

#include "base/db/base_db_mysql_auto.h"
#include "logic/stock_util.h"
#include "logic/user_info.h"
#include "logic/stock_group.h"

namespace yields {

DbMysql::DbMysql() {
  error_info_ = 0;
  //engine_ = base_db::MysqlDBPool::DBConnectionPop();
}

DbMysql::~DbMysql() {
  //base_db::MysqlDBPool::DBConnectionPush(engine_);
}

void DbMysql::Dest() {
  base_db::MysqlDBPool::Dest();
}

void DbMysql::Init(strade_share::SSEngine* ss_engine, std::list<base::ConnAddr>* const addrlist) {
  //ss_engine_ = ss_engine;
  base_db::MysqlDBPool::Init(*addrlist);
}

base_storage::DBStorageEngine* DbMysql::GetEngine() {
  error_info_ = 0;
  int num = 0;
  while (num < 5) {
    if (engine_) {
      engine_->Release();
      if (!engine_->CheckConnect()) {
        LOG_DEBUG("lost connection");
        if (!engine_->Connections(base_db::MysqlDBPool::addrlist_)) {
          engine_ = NULL;
        }
      }
    }
    if (engine_ != NULL) break;
    usleep(100);
    ++num;
  }

  if (engine_ == NULL) {
    LOG_ERROR("GetConnection Error");
    return NULL;
  }

  return engine_;
}

bool DbMysql::RequestYieldsHistory(const uint32 user_id, \
       strade_share::SSEngine* ss_engine, \
       std::vector<YieldsHistoryInfo*> *vec_yields_history_info_pt) {
  if (NULL == vec_yields_history_info_pt) {
    LOG_ERROR("DbMysql::RequestYieldsHistory, vec_yields_history_info_pt == NULL");
    return false; 
  }

  /*
  engine_ = GetEngine();
  if (NULL == engine_) {
    LOG_ERROR("DbMysql::RequestYieldsHistory, engine_ == NULL");
    return false;
  }
  */

  std::stringstream sql;
  sql << "call proc_QueryYieldsHistory(" << user_id << ")"; //其中另外添加沪深300的收益率信息

  /*
  bool r = engine_->SQLExec(sql.str().c_str());
  if (!r) {
    LOG_ERROR2("DbMysql::RequestYieldsHistory,sql:%s", sql.str().c_str());
    return false;
  }
  */
  //MYSQL_ROW ret_row;
  //int record_num = engine_->RecordCount();
  MYSQL_ROWS_VEC rows_vec;
  int colum_num = 3;
  bool r = ss_engine->ExcuteStorage(colum_num, sql.str(), rows_vec);
  if (!r) {
    LOG_ERROR2("DbMysql::RequestYieldsHistory,sql:%s", sql.str().c_str());
    return false;
  }

  int record_num = rows_vec.size();

  uint32 tmp_int = 0;
  double tmp_double = 0.0;
  std::string tmp_str;

  for (int i = 0; i < record_num; ++i) {
    //ret_row = *(reinterpret_cast<MYSQL_ROW*>(engine_->FetchRows()->proc));
    YieldsHistoryInfo *yields_history_info_pt = new YieldsHistoryInfo();
    if (NULL == yields_history_info_pt) {
      continue;
    }

    //group_id
    std::stringstream ss_tmp;
    ss_tmp << rows_vec[i][0];
    ss_tmp >> tmp_int;
    yields_history_info_pt->set_group_id(tmp_int);

    //group_name
    tmp_str = rows_vec[i][1];
    yields_history_info_pt->set_group_name(tmp_str);

    //date
    tmp_str = rows_vec[i][2];
    yields_history_info_pt->set_date(tmp_str);

    //yields
    ss_tmp << rows_vec[i][2];
    ss_tmp >> tmp_double;
    yields_history_info_pt->set_yields(tmp_double);

    vec_yields_history_info_pt->push_back(yields_history_info_pt);
  }

  return true;
}

bool DbMysql::RequestTodayDealRecord(const uint32 group_id,             \
                                     strade_share::SSEngine* ss_engine, \
                                     double& profit_or_loss_for_deal,   \
                                     double& stock_cost_for_deal) {
  /*
  该函数只取当日该组合卖出记录
  */

  /*
  engine_ = GetEngine();
  if (NULL == engine_) {
    LOG_ERROR("DbMysql::RequestTodayDealRecordByUser, engine_ == NULL");
    return false;
  }
  */

  if(stock_logic::StockUtil::Instance()->hour() < 9) {
    profit_or_loss_for_deal = 0.0;
    stock_cost_for_deal = 0.0;
    return true;
  }

  time_t current_time = time(NULL);
  int64 time_stamp = (current_time - 16*60*60)*1000;  //Get a point in time which before stock deal time.
  std::stringstream sql;
  sql << "call proc_QueryTodayDealRecordByGroupId(" << group_id << "," << time_stamp <<")";

  /*
  bool r = engine_->SQLExec(sql.str().c_str());
  if (!r) {
    LOG_ERROR2("DbMysql::RequestTodayDealRecordByUser,sql:%s", sql.str().c_str());
    return false;
  }
  MYSQL_ROW ret_row;
  int record_num = engine_->RecordCount();
  */

  MYSQL_ROWS_VEC rows_vec;
  int colum_num = 0;
  bool r = ss_engine->ExcuteStorage(colum_num, sql.str(), rows_vec);
  if (!r) {
    LOG_ERROR2("DbMysql::RequestTodayDealRecordByUser,sql:%s", sql.str().c_str());
    return false;
  }
  int record_num = rows_vec.size();

  int64 stock_trade_num = 0;
  double stock_trade_price = 0.0;
  std::string stock_code;

  for (int i = 0; i < record_num; ++i) {
    //ret_row = *(reinterpret_cast<MYSQL_ROW*>(engine_->FetchRows()->proc));

    //stock_code
    stock_code = rows_vec[i][0];

    //trade_price
    std::stringstream ss_tmp;
    ss_tmp << rows_vec[i][1];
    ss_tmp >> stock_trade_price;

    //trade_num
    ss_tmp << rows_vec[i][2];
    ss_tmp >> stock_trade_num;

    strade_logic::StockRealInfo stock_real_info;
    ss_engine->GetStockCurrRealMarketInfo(stock_code, stock_real_info);
    double stock_open_price = stock_real_info.open;
	stock_cost_for_deal += stock_open_price * stock_trade_num;
    profit_or_loss_for_deal += (stock_trade_price - stock_open_price) * stock_trade_num;
  }

  return true;
}

}  //namespace yields
