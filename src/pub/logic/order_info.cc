//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2017/1/10 Author: zjc

#include "order_info.h"

#include <stdlib.h>

#include <vector>
#include <sstream>

#include "message.h"
#include "comm_head.h"
#include "logic/logic_comm.h"
#include "strade_share/strade_share_engine.h"
#include "logic/strade_basic_info.h"
#include "logic/user_engine.h"
#include "logic/stock_util.h"

namespace strade_user {

using strade_share::STOCKS_MAP;
using strade_share::STOCK_REAL_MAP;
using strade_logic::StockTotalInfo;
using stock_logic::StockUtil;

SSEngine* OrderInfo::engine_ = NULL;

OrderInfo::OrderInfo() {
  data_ = new Data();
}

OrderInfo::OrderInfo(UserId user_id,
                     OrderId order_id,
                     OrderType type) {
  data_ = new Data();
  data_->user_id_ = user_id;
  data_->id_ = order_id;
  data_->type_ = type;
  stale_ = false;
}

REFCOUNT_DEFINE(OrderInfo)

std::string OrderInfo::GetUserOrderSql(UserId user_id) {
  std::ostringstream oss;
  oss << "SELECT `id`, `userId`, `groupId`, `stock`, `price`, "
      << "`lossOrProfitPrice`, `tradeType`, `status`, `count`, "
      << "`needFunds`, UNIX_TIMESTAMP(`tradeTime`), `tradePrice`, `tradeCount`, "
      << "`stampDuty`, `commission`, `transferFee`, `type`, "
      << "`amount`, `profit`, `availableCapital`, UNIX_TIMESTAMP(`createTime`) "
      << "FROM `delegation_record`"
      << "WHERE "
      << "`userId` = " << user_id;
  return oss.str();
}

std::string OrderInfo::GetPendingOrderSql(UserId user_id) {
  return std::string("");
}

std::string OrderInfo::GetFinishedOrderSql(UserId user_id) {
  return std::string("");
}

void OrderInfo::Init(const SubmitOrderReq& req) {
  data_->group_id_ = req.group_id;
  data_->code_ = req.code;
  data_->op_ = req.op;
  data_->order_price_ = req.order_price;
  data_->order_num_ = req.order_nums;
  data_->expected_price_ = req.expected_price;
}

void OrderInfo::Init(const OrderInfo& order) {
  data_->group_id_ = order.group_id();
  data_->code_ = order.code();
  data_->op_ = SELL;
  data_->order_price_ = order.expected_price();
  data_->order_num_ = order.deal_num();
}

void OrderInfo::Deserialize() {
  int t = 0;
  data_->initialized_ = true;

  GetInteger(ID, data_->id_);
  GetInteger(USER_ID, data_->user_id_);
  GetInteger(GROUP_ID, data_->group_id_);
  GetString(STOCK, data_->code_);
  GetReal(ORDER_PRICE, data_->order_price_);
  GetReal(EXPECTED_PRICE, data_->expected_price_);

  GetInteger(ORDER_OPERATION, t);
  data_->op_ = (OrderOperation) t;

  GetInteger(ORDER_STATUS, t);
  data_->status_ = (OrderStatus) t;

  GetInteger(ORDER_COUNT, data_->order_num_);
  GetReal(FROZEN, data_->frozen_);

  GetInteger(ORDER_TYPE, t);
  data_->type_ = (OrderType) t;

  if (FINISHED != data_->status_) {
    return;
  }

  if (GetInteger(DEAL_TIME, t)) {
    data_->deal_time_ = t;
  }

  GetReal(DEAL_PRICE, data_->deal_price_);
  GetInteger(DEAL_COUNT, data_->deal_num_);
  GetReal(STAMP_DUTY, data_->stamp_duty_);
  GetReal(COMMISSION, data_->commission_);
  GetReal(TRANSFER_FEE, data_->transfer_fee_);
  GetReal(AMOUNT, data_->amount_);
  GetReal(PROFIT, data_->profit_);
  GetReal(AVAILABLE_CAPITAL, data_->available_capital_);

  if (GetInteger(CREATE_TIME, t)) {
    data_->create_time_ = t;
  }
}

bool OrderInfo::InitPendingOrder(MYSQL_ROW row) {
  return true;
}

bool OrderInfo::InitFinishedOrder(MYSQL_ROW row) {
  return true;
}

void OrderInfo::Update(int opcode, void* param) {
  engine_ = static_cast<SSEngine*>(param);
  assert(NULL != param);
  switch (opcode) {
    case strade_logic::REALTIME_MARKET_VALUE_UPDATE: {
      OnStockUpdate();
      break;
    }
    default:break;
  }
}

bool OrderInfo::MakeADeal(double price) {
  // check trading time
#ifndef DEBUG_TEST
  StockUtil* util = StockUtil::Instance();
  if (!util->is_trading_time()) {
    return false;
  }
#endif
  if (!can_deal(price)) {
    return false;
  }

  // now can remove from observer list
  stale_ = true;

  data_->status_ = FINISHED;
  data_->deal_time_ = time(NULL);
  data_->deal_price_ = price;
  data_->deal_num_ = data_->order_num_;

  double amount = data_->deal_price_ * data_->deal_num_;

  // TODO 佣金计算
  // double commission = data_->amount_ * COMMISSION_RATE;
  double commission = amount * COMMISSION_RATE;
  data_->commission_ = ROUND_COMMISSION(commission);
  data_->amount_ += data_->commission_;

  if (IS_SH_CODE(data_->code_)) {
    data_->transfer_fee_ = TRANSFER_FEE(data_->deal_num_);
  }
  data_->amount_ += data_->transfer_fee_;

  if (SELL == data_->op_) {
    data_->stamp_duty_ = amount * STAMP_DUTY_RATE;
  }

//  data_->amount_ = amount +
//      data_->commission_ +
//      data_->transfer_fee_ +
//      data_->stamp_duty_;

  //TODO 成交额不加费用
  data_->amount_ = amount;

  UserEngine* engine = UserEngine::GetUserEngine();
  UserInfo* user = engine->GetUser(data_->user_id_);
  assert(NULL != user);
  user->OnOrderDone(this);

  return true;
}

void OrderInfo::OnStockUpdate() {
  // check trading time
  bool r = false;
#ifndef DEBUG_TEST
  StockUtil* util = StockUtil::Instance();
  if (!util->is_trading_time()) {
    LOG_ERROR("不在交易时间");
    return;
  }
#endif
  if (FINISHED == data_->op_) {
    stale_ = true;
    LOG_ERROR2("fatal error: finished order, user_id:%d, group_id:%d, "
                   "order_id:%d, code:%s, num:%d, create_time:%d, deal_time:%d",
               data_->user_id_, data_->group_id_, data_->id_, data_->code_.data(),
               data_->order_num_, data_->create_time_, data_->deal_time_);
    return;
  }

  StockTotalInfo stock_total_info;
  r = engine_->GetStockTotalInfoByCode(data_->code_, stock_total_info);
  if (!r) {
    LOG_ERROR2("stock:%s NOT EXIST", data_->code_.data());
    return;
  }

  STOCK_REAL_MAP stock = stock_total_info.GetStockRealMap();
  STOCK_REAL_MAP::reverse_iterator deal_it = stock.rend();
  for (STOCK_REAL_MAP::reverse_iterator rit = stock.rbegin();
       stock.rend() != rit; ++rit) {
    if (rit->first >= data_->create_time_
        && can_deal(rit->second.price)) {
      LOG_MSG2("stock=%s, curr_price=%.2f, order_price=%.2f, status=%d, success deal",
               rit->second.code.c_str(), rit->second.price, data_->order_price_, data_->op_);
      deal_it = rit;
    }
  }

  // cannot deal
  if (stock.rend() == deal_it) {
    return;
  }

  // can make a deal
  MakeADeal(deal_it->second.price);
}

void OrderInfo::OnOrderCancel() {
  data_->status_ = CANCEL;
  stale_ = true;
}

} /* namespace strade_user */
