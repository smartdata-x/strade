//Copyright (c) 2016-2017 The strade Authors. All rights reserved.
//Created on: 2017/01/09 Author: tangtao

#include <string>
#include <vector>

#include "yields/db_mysql.h"
#include "yields/yields_info.h"
#include "logic/user_info.h"
#include "logic/order_info.h"


namespace yields {

GroupAccountInfo::GroupAccountInfo():
  gid_(0),
  total_assets_(0.0),
  //yields_of_day_(0.0),
  total_yields_(0.0),
  usable_assets_(0.0),
  //profit_or_loss_(0.0),
  stock_value_(0.0),
  holding_yields_(0.0){
}

bool GroupAccountInfo::fetch_group_account_info(strade_share::SSEngine* ss_engine, uint32 user_id, uint32 group_id) {
  /*
  新需求改为：
  1.每个组合作为一个独立的账户
  2.当日收益率：当前持仓股票（当前价-买入价）/买入价，不考虑税费等
  3.总收益率：（当前总资产-初始资产）/初始资产
  4.净值：1+总收益率
  */
  if(ss_engine == NULL) {
    return false;
  }
  strade_user::UserInfo* user_info = ss_engine->GetUser(user_id);
  if(user_info == NULL) {
    return false;
  }
  strade_user::StockGroup* stock_group_ptr = user_info->GetGroup(group_id);
  if(stock_group_ptr == NULL) {
    return false;
  }

  gid_ = group_id;
  usable_assets_ = stock_group_ptr->available_capital();

  //stock_value
  strade_user::GroupStockPositionList group_position_list = user_info->GetGroupStockPosition(group_id);

  //double profit_or_loss_for_holding = 0.0;
  //double stock_cost_for_holding = 0.0;
  for(std::vector<strade_user::GroupStockPosition>::iterator it = group_position_list.begin(); \
      it != group_position_list.end(); ++it) {
    std::string stock_code = it->code();
    int stock_num = it->count();
    strade_logic::StockRealInfo stock_real_info;
    ss_engine->GetStockCurrRealMarketInfo(stock_code, stock_real_info);
    double stock_bought_price = it->cost();
    //stock_cost_for_holding += stock_bought_price * stock_num;  
    //profit_or_loss_for_holding = (stock_real_info.price - stock_bought_price) * stock_num;
    stock_value_ += stock_real_info.price * stock_num;
  }

  //total_assets
  double frozen_assets = stock_group_ptr->frozen_capital();
  int temp = usable_assets_ + frozen_assets;
  total_assets_ = temp + stock_value_;

  /*
  //profit_or_loss
  double profit_or_loss_for_deal = 0.0;
  double stock_cost_for_deal = 0.0;
  DbMysql mysql;
  mysql.RequestTodayDealRecord(group_id, ss_engine, profit_or_loss_for_deal, stock_cost_for_deal);
  profit_or_loss_ = profit_or_loss_for_deal + profit_or_loss_for_holding;

  //yields_of_day
  double stock_total_cost = stock_cost_for_deal + stock_cost_for_holding;
  const double EPSINON = 0.000001;
  if (stock_total_cost < EPSINON && stock_total_cost > -EPSINON) {
    yields_of_day_ = 0.0;
  } else {
    yields_of_day_ = profit_or_loss_/stock_total_cost;
  }
  */

  double initial_assets = stock_group_ptr->init_capital();

  //holding_yields 持仓收益
  double holding_yields_ = total_assets_ - initial_assets;

  //total_yields
  total_yields_ = holding_yields_ / initial_assets;

  return true;
}

YieldsHistoryInfo::YieldsHistoryInfo():
  gid_(0),
  gname_(""),
  date_(""),
  yields_(0.0) {
}

}    // namespace yields