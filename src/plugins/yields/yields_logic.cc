//Copyright (c) 2016-2017 The strade Authors. All rights reserved.
//Created on: 2017/01/09 Author: tangtao

#include "yields/yields_logic.h"

#include <assert.h>

#include <string>
#include <map>
#include <cmath>
#include <iostream>

#include "logic/logic_comm.h"
#include "logic/logic_unit.h"
#include "config/config.h"
#include "json/json.h"
#include "net/http_data.h"
//#include "baseconfig/base_config.h"
#include "yields/db_mysql.h"
#include "logic/stock_util.h"
#include "logic/user_info.h"
#include "logic/stock_group.h"

#define UPDATE_YIELDS_INFO  30001

namespace yields {

YieldsLogic *YieldsLogic::instance_ = NULL;

YieldsLogic::YieldsLogic() {
  if (!Init()) {
    assert(0);
  }
}

YieldsLogic::~YieldsLogic() {
}

bool YieldsLogic::Init() {
  /*
  bool r = false;
  std::string path = DEFAULT_CONFIG_PATH;
  config::FileConfig *config = config::FileConfig::GetFileConfig();
  if (NULL == config) {
    return false;
  }
  r = config->LoadConfig(path);
  */

  strade_share::SSEngine* (* ss_engine)(void);
  std::string strade_share_library = "./strade_share/strade_share.so";
  std::string strade_share_func = "GetStradeShareEngine";
  ss_engine = (strade_share::SSEngine* (*)(void))
  base_logic::SomeUtils::GetLibraryFunction(strade_share_library, strade_share_func);
  ss_engine_ = (*ss_engine)();
  if (ss_engine_ == NULL)
    assert(0);
  //ss_engine_->Init();

  //db_mysql_->Init(ss_engine_, &config->mysql_db_list_);

  return true;
}

bool YieldsLogic::OnYieldsConnect(struct server *srv, const int socket) {
  return true;
}

bool YieldsLogic::OnYieldsMessage(struct server *srv,     \
                                     const int socket,    \
                                      const void *msg,    \
                                       const int len) {
  bool r = true;
  do {
    const char *packet_stream = reinterpret_cast<const char*>(msg);
    if (NULL == packet_stream) {
      r = false;
      break;
    }
    std::string http_str(packet_stream, len);
    std::string error_str;
    int error_code = 0;

    scoped_ptr<base_logic::ValueSerializer> serializer(\
      base_logic::ValueSerializer::Create(base_logic::IMPL_HTTP, &http_str));

    NetBase *value = (NetBase*)\
      (serializer.get()->Deserialize(&error_code, &error_str));

    if (NULL == value) {
      error_code = STRUCT_ERROR;
      //send_error(error_code, socket);
      r = true;
    }

    scoped_ptr<RecvPacketBase> packet(new RecvPacketBase(value));
    LOG_DEBUG2("message %s", "yields plugin");
    uint32 type = packet->GetType();

    switch (type) {
      case USER_ACCOUNT_INFO: {
        r = OnRequestGroupAccountInfo(srv, socket, value);
        break;
      }

      case YIELDS_HISTORY: {
        r = OnRequestYieldsHistory(srv, socket, value);
        break;
      }

      default: {
        r = false;
        break;
	  }
    }
  } while (0);

  return r;
}

bool YieldsLogic::OnYieldsClose(struct server *srv, const int socket) {
  return true;
}

bool YieldsLogic::OnBroadcastConnect(struct server *srv,  \
                                        const int socket, \
                                         const void *msg, \
                                           const int len) {
}

bool YieldsLogic::OnBroadcastMessage(struct server *srv,  \
                                        const int socket, \
                                         const void *msg, \
                                           const int len) {
  return true;
}

bool YieldsLogic::OnBroadcastClose(struct server *srv, const int socket) {
  return true;
}

bool YieldsLogic::OnIniTimer(struct server *srv) {
  srv->add_time_task(srv, "yields", UPDATE_YIELDS_INFO, 60*60, -1);
  return true;
}

bool YieldsLogic::OnTimeout(struct server *srv, char *id, int opcode, int time) {
  bool r = true;
  switch (opcode) {
    case UPDATE_YIELDS_INFO: {
      r = OnUpdateYields();
      break;
    }
 }

  return r;
}

bool YieldsLogic::OnUpdateYields() {
  /*
  需求修改为：1.除去当前持仓这个组合  2.计算每个组合剩余持仓股票的收益率（用股票当天的（收盘价-购买价）*数量），不考虑买入时各项费用
  通过当前持仓
  */
  //update between 23:00~23:59
  if(stock_logic::StockUtil::Instance()->hour() < 23) {
    return true;
  }
  //update at trading day
  if(!(stock_logic::StockUtil::Instance()->is_trading_day())) {
    return true;
  }
  /*
  engine_ = GetEngine();
  if (NULL == engine_) {
    LOG_ERROR("DbMysql::UpdateYields, engine_ == NULL");
    return false;
  }
  */

  std::map<uint32, strade_user::UserInfo> user_map;
  user_map = ss_engine_->GetUserMap(); //获取所有用户
  std::string date = stock_logic::StockUtil::Instance()->get_current_day_str();
  const double EPSINON = 0.000001;
  //更新所有用户下所有组合的当日收益率
  for(std::map<uint32, strade_user::UserInfo>::iterator it_1 = user_map.begin(); \
      it_1 != user_map.end(); ++it_1) { //遍历所有用户
    strade_user::UserInfo user_info = it_1->second;
    strade_user::StockGroupList group_list = user_info.GetAllGroups();
    for(std::vector<strade_user::StockGroup>::iterator it_2 = group_list.begin(); \
        it_2 != group_list.end(); ++it_2) { //遍历该用户下的所有组合
      double profit_or_loss_for_holding = 0.0;
      double stock_buying_costs = 0.0;
	  strade_user::GroupStockPositionList group_position_list = user_info.GetGroupStockPosition(it_2->id());
      for(std::vector<strade_user::GroupStockPosition>::iterator it_3 = group_position_list.begin(); \
          it_3 != group_position_list.end(); ++it_3) { //遍历该组合下所有股票
        std::string stock_code = it_3->code();
        int stock_num = it_3->count();
        double stock_bought_price = it_3->cost();
        strade_logic::StockRealInfo stock_real_info;
        ss_engine_->GetStockCurrRealMarketInfo(stock_code, stock_real_info);
        profit_or_loss_for_holding += (stock_real_info.close - stock_bought_price) * stock_num;
        stock_buying_costs += stock_bought_price * stock_num;
      }

      double today_yields = 0.0;
      if(stock_buying_costs < EPSINON && stock_buying_costs > -EPSINON) {
        today_yields = 0.0;
      } else {
        today_yields = profit_or_loss_for_holding / stock_buying_costs;
      }

      std::stringstream sql;
      uint32 user_id = it_1->first;
      uint32 group_id = it_2->id();
      std::string group_name = it_2->name();
      sql << "call proc_UpdateYields(" << user_id << "," << group_id  \
          << ",'" << group_name << "','"<< date << "'," << today_yields << ")";

      bool r = ss_engine_->WriteData(sql.str());
      if (!r) {
        LOG_ERROR2("DbMysql::UpdateYields,sql:%s", sql.str().c_str());
        return false;
      }
    }
  }

  //更新‘沪深300’当日收益率
  std::string stock_code = "hs300";
  strade_logic::StockRealInfo stock_real_info;
  ss_engine_->GetStockCurrRealMarketInfo(stock_code, stock_real_info);
  double today_yields = 0.0;
  if(stock_real_info.open < EPSINON && stock_real_info.open > -EPSINON) {
    today_yields = 0.0;
  } else {
    today_yields = (stock_real_info.close - stock_real_info.open)/stock_real_info.open;
  }

  std::stringstream sql;
  sql << "call proc_UpdateYields(" << 0 << "," << 0  \
      << ",'" << date << "'," << today_yields << ")";

  bool r = ss_engine_->WriteData(sql.str());
  if (!r) {
    LOG_ERROR2("DbMysql::UpdateYields,sql:%s", sql.str().c_str());
    return false;
  }

}

}  //namespace yields
