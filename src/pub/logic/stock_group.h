//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2017/1/10 Author: zjc

#ifndef SRC_PUB_LOGIC_STOCK_GROUP_H_
#define SRC_PUB_LOGIC_STOCK_GROUP_H_

#include <string>
#include <vector>

#include <mysql.h>

#include "macros.h"
#include "user_defined_types.h"
#include "dao/abstract_dao.h"
#include "logic/logic_comm.h"

namespace strade_share {
class SSEngine;
}
using strade_share::SSEngine;

namespace strade_user {

// 组合的历史收益率
struct GroupHistYields {

  double yield_;
};

typedef std::map<std::string, GroupHistYields> GROUP_HIST_YIELD_MAP;
class StockGroup : public base_logic::AbstractDao {
 public:
  static SSEngine* engine_;
  enum Status {
    INVALID,
    VALID,
    DEFAULT
  };

  static std::string GetUserGroupSql(UserId user_id);
  static std::string GetGroupStockSql(GroupId group_id);
  static GroupId CreateGroup(UserId user_id,
                             const std::string& name,
                             Status status = VALID);
 public:
  StockGroup();
  StockGroup(UserId user_id, GroupId id, const std::string& name);
  REFCOUNT_DECLARE(StockGroup);
 public:
  bool Init(const MYSQL_ROW row);
  bool InitStockList();
  bool AddStocks(StockCodeList& stocks);
  bool DelStocks(StockCodeList& stocks);
  StockCodeList stocks() const { return data_->stock_list_; }
  bool exist_stock(const std::string& code) const {
    return data_->stock_set_.count(code);
  }
  bool OnDelegateBuyOrderDelegate(double delegate_capital) {
    if (data_->available_capital_ >= delegate_capital) {
      data_->available_capital_ -= delegate_capital;
      data_->frozen_capital_ += delegate_capital;
      return true;
    }
    return false;
  }
  void OnCancelBuyOrder(double frozen) {
    data_->available_capital_ += frozen;
    data_->frozen_capital_ -= frozen;
  }
  void OnBuyOrderDone(double frozen, double amount) {
    LOG_MSG2("pre order down, available_capital:%.2f",
             data_->available_capital_);
    data_->frozen_capital_ -= frozen;
    data_->available_capital_ = data_->available_capital_ + frozen - amount;

    LOG_MSG2("after order down, available_capital:%.2f",
             data_->available_capital_);
//    data_->available_capital_ += frozen - amount;
  }

  void OnSellOrderDone(double amount) {
    data_->available_capital_ += amount;
  }

  GROUP_HIST_YIELD_MAP GetGroupAllHistYieldMap() const {
    return data_->group_hist_yield_map_;
  }

  bool AddGroupHistYield(const std::string& date, GroupHistYields& yield) {
    data_->group_hist_yield_map_.insert(GROUP_HIST_YIELD_MAP::value_type(date, yield));
    return true;
  }

 private:
  void Deserialize();

 public:
  void set_id(UserId id) { data_->id_ = id; }
  UserId id() const { return data_->id_; }

  void set_name(const std::string& name) { data_->name_ = name; }
  std::string name() const { return data_->name_; }
  Status status() const { return data_->status_; }
  bool initialized() const { return data_->initialized_; }
  void add_init_capital(double capital) {
    data_->init_capital_ += capital;
    data_->available_capital_ += capital;
  }
  double init_capital() const { return data_->init_capital_; }
  double frozen_capital() const { return data_->frozen_capital_; }
  double available_capital() const { return data_->available_capital_; }

 public:
  class Data {
   public:
    Data()
        : refcount_(1),
          id_(0),
          user_id_(0),
          status_(VALID),
          init_capital_(0.0),
          available_capital_(0.0),
          frozen_capital_(0.0),
          initialized_(false) {
    }

   public:
    GroupId id_;
    UserId user_id_;
    std::string name_;
    Status status_;
    bool initialized_;
    double init_capital_;
    double available_capital_;  // 可用资金
    double frozen_capital_;     // 冻结资金

    StockCodeList stock_list_;
    StockCodeSet stock_set_;

    // 组合的历史收益率
    GROUP_HIST_YIELD_MAP group_hist_yield_map_;

    void AddRef() {
      __sync_fetch_and_add(&refcount_, 1);
    }

    void Release() {
      __sync_fetch_and_sub(&refcount_, 1);
      if (!refcount_)
        delete this;
    }

   private:
    int refcount_;
  };
 private:
  Data* data_;
};

typedef std::vector<StockGroup> StockGroupList;

} /* namespace strade_user */

#endif /* SRC_PUB_LOGIC_STOCK_GROUP_H_ */
