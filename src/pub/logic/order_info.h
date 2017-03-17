//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2017/1/10 Author: zjc

#ifndef SRC_PUB_LOGIC_ORDER_INFO_H_
#define SRC_PUB_LOGIC_ORDER_INFO_H_

#include <time.h>

#include <string>
#include <vector>

#include <mysql.h>

#include "macros.h"
#include "user_defined_types.h"
#include "basic/basictypes.h"
#include "observer.h"
#include "dao/abstract_dao.h"
#include "logic/message.h"

namespace strade_share {
class SSEngine;
}

namespace strade_user {

using strade_share::SSEngine;
struct SubmitOrderReq;

class OrderInfo : public strade_logic::Observer,
                  public base_logic::AbstractDao {
 public:
  static SSEngine* engine_;
  enum {
    ID,
    USER_ID,
    GROUP_ID,
    STOCK,
    ORDER_PRICE,
    EXPECTED_PRICE,
    ORDER_OPERATION,
    ORDER_STATUS,
    ORDER_COUNT,
    FROZEN,
    DEAL_TIME,
    DEAL_PRICE,
    DEAL_COUNT,
    STAMP_DUTY,
    COMMISSION,
    TRANSFER_FEE,
    ORDER_TYPE,
    AMOUNT,
    PROFIT,
    AVAILABLE_CAPITAL,
    CREATE_TIME
  };

  enum OrderType {
    USER_ORDER,
    AUTO_ORDER
  };

  OrderInfo();
  OrderInfo(UserId user_id, OrderId order_id, OrderType type = USER_ORDER);
  REFCOUNT_DECLARE(OrderInfo);
 public:
  static std::string GetPendingOrderSql(UserId user_id);
  static std::string GetFinishedOrderSql(UserId user_id);
  static std::string GetUserOrderSql(UserId user_id);

 public:
  void Init(const SubmitOrderReq& req);
  // call while automatic generate orer
  void Init(const OrderInfo& order);
  bool InitPendingOrder(MYSQL_ROW row);
  bool InitFinishedOrder(MYSQL_ROW row);

  bool MakeADeal(double price);
  void OnOrderCancel();
 private:
  void Deserialize();
  void Update(int opcode, void* param=NULL);
  void OnStockUpdate();
 public:
  OrderId id() const { return data_->id_; }
  GroupId group_id() const { return data_->group_id_; }
  std::string code() const { return data_->code_; }
  void set_operation(OrderOperation op) { data_->op_ = op; }
  OrderOperation operation() const { return data_->op_; }

  void set_status(OrderStatus status) { data_->status_ = status; }
  OrderStatus status() const { return data_->status_; }

  void set_frozen(double frozen) { data_->frozen_ = frozen; }
  double frozen() const { return data_->frozen_; }

  time_t create_time() const { return data_->create_time_; }
  time_t deal_time() const { return data_->deal_time_; }

  uint32 order_num() const { return data_->order_num_; }
  double order_price() const { return data_->order_price_; }

  uint32 deal_num() const { return data_->deal_num_; }
  double deal_price() const { return data_->deal_price_; }

  double stamp_duty() const { return data_->stamp_duty_; }
  double commission() const { return data_->commission_; }
  double transfer_fee() const { return data_->transfer_fee_; }
  double amount() const { return data_->amount_; }
  double expected_price() const { return data_->expected_price_; }

  void set_available_capital(double available_capital) {
    data_->available_capital_ = available_capital;
  }
  double available_capital() const { return data_->available_capital_; }

  void set_profit(double profit) { data_->profit_ = profit; }
  double profit() const { return data_->profit_; }

  bool can_deal(double price) const {
    if (BUY == data_->op_) {
      return data_->order_price_ >= price;
    } else {
      return data_->order_price_ <= price;
    }
  }

  bool initialized() const { return data_->initialized_; }

  const std::string& ToString() const {
    using std::endl;
    std::ostringstream oss;
    OSS_WRITE(data_->user_id_);
    OSS_WRITE(data_->group_id_);
    OSS_WRITE(data_->op_);
    OSS_WRITE(data_->status_);
    OSS_WRITE(data_->create_time_);
    OSS_WRITE(data_->order_price_);
    OSS_WRITE(data_->order_num_);
    OSS_WRITE(data_->deal_time_);
    OSS_WRITE(data_->deal_price_);
    OSS_WRITE(data_->deal_num_);
    return oss.str();
  }
 private:
  class Data {
   public:
    Data()
        : refcount_(1),
          user_id_(0),
          group_id_(INVALID_GROUPID),
          type_(USER_ORDER),
          op_(BUY),
          status_(PENDING),
          create_time_(time(NULL)),
          order_price_(0.0),
          order_num_(0),
          frozen_(0.0),
          deal_time_(-1),
          deal_price_(0.0),
          deal_num_(0),
          commission_(0.0),
          transfer_fee_(0.0),
          stamp_duty_(0.0),
          amount_(0.0),
          profit_(0.0),
          available_capital_(0.0),
          initialized_(false) {
    }

   public:
    bool initialized_;
    OrderId id_;
    UserId user_id_;
    GroupId group_id_;
    std::string code_;
    OrderType type_;
    OrderOperation op_;
    OrderStatus status_;

    time_t create_time_;
    double order_price_;
    double expected_price_;
    uint32 order_num_;
    double frozen_;

    time_t deal_time_;
    double deal_price_;
    uint32 deal_num_;
    double commission_;
    double transfer_fee_;
    double stamp_duty_;
    double amount_;
    double profit_;
    double available_capital_;

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

typedef std::vector<OrderInfo*> OrderList;
} /* namespace strade_user */

#endif /* SRC_PUB_LOGIC_ORDER_INFO_H_ */
