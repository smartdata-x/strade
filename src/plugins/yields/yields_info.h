//Copyright (c) 2016-2017 The strade Authors. All rights reserved.
//Created on: 2017/01/09 Author: tangtao

#ifndef _STRADE_SRC_PLUGINS_YIELDS_YIELDS_INFO_H_
#define _STRADE_SRC_PLUGINS_YIELDS_YIELDS_INFO_H_

#include <string>
#include <vector>
#include <map>

#include "basic/basictypes.h"
#include "strade_share/strade_share_engine.h"

#define INITIAL_ASSETS    500000

namespace yields {

class GroupAccountInfo {
 public:
  GroupAccountInfo();
  ~GroupAccountInfo() {}

  uint32 get_group_id() {return gid_;}
  double get_total_assets() {return total_assets_;}
  double get_yields_of_day() {return yields_of_day_;}
  double get_total_yields() {return total_yields_;}
  double get_usable_assets() {return usable_assets_;}
  double get_profit_or_loss() {return profit_or_loss_;}
  double get_stock_value() {return stock_value_;}
  double get_holding_yields() {return holding_yields_;}

  void set_group_id(uint32 group_id) {gid_ = group_id;}
  void set_total_assets(double total_assets) {total_assets_ = total_assets;}
  void set_yields_of_day(double yields_of_day) {yields_of_day_ = yields_of_day;}
  void set_total_yields(double total_yields) {total_yields_ = total_yields;}
  void set_usable_assets(double usable_assets) {usable_assets_ = usable_assets;}
  void set_profit_or_loss(double profit_or_loss) {profit_or_loss_ = profit_or_loss;}
  void set_stock_value(double stock_value) {stock_value_ = stock_value;}
  void set_holding_yields(double holding_yields) {holding_yields_ = holding_yields;}

  bool fetch_group_account_info(strade_share::SSEngine* ss_engine, uint32 user_id, uint32 group_id);

 private:
  uint32 gid_;
  double total_assets_;
  double yields_of_day_; 
  double total_yields_;
  double usable_assets_;
  double profit_or_loss_;
  double stock_value_;
  double holding_yields_;
};

class YieldsHistoryInfo {
 public:
  YieldsHistoryInfo();
  ~YieldsHistoryInfo() {}

  uint32 get_group_id() {return gid_;}
  std::string get_group_name() {return gname_;}
  std::string get_date() {return date_;}
  double get_yields() {return yields_;}

  void set_group_id(uint32 group_id) {gid_ = group_id;}
  void set_group_name(std::string group_name) {gname_ = group_name;}
  void set_date(std::string date) {date_ = date;}
  void set_yields(double yields) {yields_ = yields;}

 private:
  uint32 gid_;
  std::string gname_;
  std::string date_;
  double yields_;
};

}  // namespace yields

#endif  // _STRADE_SRC_PLUGINS_YIELDS_YIELDS_INFO_H_
