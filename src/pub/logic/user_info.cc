//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2017/1/10 Author: zjc

#include "user_info.h"

#include <stdlib.h>

#include <sstream>

#include "logic/logic_comm.h"
#include "strade_share/strade_share_engine.h"

namespace strade_user {

using strade_logic::StockTotalInfo;
using strade_logic::StockRealInfo;
using strade_share::STOCK_REAL_MAP;

const char UserInfo::kGetAllUserInfoSql[] =
    "SELECT `userId`, `userName`, `password`, `platformId`, `userLevel`, `email`, `phone`, `availableCapital`, `frozenCapital` FROM `user_info`";

SSEngine* UserInfo::engine_ = NULL;
UserInfo::UserInfo() {
  data_ = new Data();
}

REFCOUNT_DEFINE(UserInfo)

void UserInfo::Deserialize() {
  GetInteger(ID, data_->id_);
  GetString(NAME, data_->name_);
  GetString(PASSWORD, data_->password_);
  GetInteger(PLATFORM_ID, data_->platform_id_);
  GetInteger(USER_LEVEL, data_->level_);
  GetString(EMAIL, data_->email_);
  GetString(PHONE, data_->phone_);

  data_->initialized_ = true;
  LOG_DEBUG2("init user:%d, name:%s, passwd:%s, platform_id:%d, user_level:%d, "
                 "email:%s, phone:%s",
             data_->id_, data_->name_.data(), data_->password_.data(), data_->platform_id_,
             data_->level_, data_->email_.data(), data_->phone_.data());
}

bool UserInfo::Init() {
  return data_->initialized_ &&
      InitStockGroup() &&
      InitStockPosition() &&
      InitOrder();
}

bool UserInfo::InitStockGroup() {
  std::vector<StockGroup> sgs;
  SSEngine* engine = GetStradeShareEngine();
  if (!engine->ReadData(StockGroup::GetUserGroupSql(data_->id_), sgs)) {
    LOG_ERROR2("init user:%s stock group error", data_->name_.data());
    return false;
  }

//  bool find_default = false;
  for (size_t i = 0; i < sgs.size(); ++i) {
    if (!(sgs[i].initialized() && sgs[i].InitStockList())) {
      continue;
    }
    data_->stock_group_list_.push_back(sgs[i]);
//    if (sgs[i].status() == StockGroup::DEFAULT) {
//      find_default = true;
//      data_->default_gid_ = sgs[i].id();
//    }
  }

//  if (!find_default) {
//    std::string name = "DEFAULT";
//    GroupId group_id = StockGroup::CreateGroup(data_->id_, name, StockGroup::DEFAULT);
//    if (INVALID_GROUPID == group_id) {
//      LOG_ERROR2("user:%s create DEFAULT group error", data_->name_.data());
//      return false;
//    }
//
//    data_->default_gid_ = group_id;
//    StockGroup g(data_->id_, group_id, name);
//    data_->stock_group_list_.push_back(g);
//  }

  LOG_MSG2("user:%s init %d stock groups", data_->name_.data(), sgs.size());
  return true;
}

bool UserInfo::InitStockPosition() {
  SSEngine* engine = GetStradeShareEngine();
  std::vector<GroupStockPosition> rows;
  std::string sql = GroupStockPosition::GetGroupStockPositionSql(data_->id_);
  if (!engine->ReadData(sql, rows)) {
    LOG_ERROR2("init user:%s group stock position error", data_->name_.data());
    return false;
  }

  for (size_t i = 0; i < rows.size(); ++i) {
    rows[i].set_user_id(data_->id_);
    if (!(rows[i].initialized() && rows[i].InitFakeStockPosition())) {
      continue;
    }
    data_->stock_position_list_.push_back(rows[i]);
  }
  LOG_MSG2("user:%s init %d group stock position",
           data_->name_.data(), rows.size());
  return true;
}

bool UserInfo::InitOrder() {
  SSEngine* engine = GetStradeShareEngine();
  std::vector<OrderInfo> orders;
  std::string sql = OrderInfo::GetUserOrderSql(data_->id_);
  LOG_DEBUG2("GetUserOrderSql sql=%s", sql.data());
  if (!engine->ReadData(sql, orders)) {
    LOG_ERROR2("init user:%s orders error", data_->name_.data());
    return false;
  }

  for (size_t i = 0; i < orders.size(); ++i) {
    OrderInfo& o = orders[i];
    if (!o.initialized()) {
      LOG_ERROR("init order error");
      continue;
    }
    data_->order_list_.push_back(new OrderInfo(o));

    if (o.status() == PENDING) {
      size_t last = data_->order_list_.size() - 1;
      if (o.operation() == SELL) {
        GroupStockPosition* g = GetGroupStockPosition(o.group_id(), o.code());
        assert(NULL != g);
        assert(g->Delegate(o.order_num()));
      }
      engine->AttachObserver(data_->order_list_[last]);
    }
  }

  LOG_MSG2("user:%s init %d orders",
           data_->name_.data(), orders.size());

  BindOrder();
  return true;
}

// holding_record 与 delegation_record 绑定对应
void UserInfo::BindOrder() {
  for (size_t i = 0; i < data_->stock_position_list_.size(); ++i) {
    FakeStockPositionList& fps =
        data_->stock_position_list_[i].data_->fake_stock_position_list_;
    for (size_t j = 0; j < fps.size(); ++j) {
      OrderId order_id = fps[j].order_id();
      OrderInfo* p = NULL;
      for (size_t k = 0; k < data_->order_list_.size(); ++k) {
        if (data_->order_list_[k]->id() == order_id) {
          p = data_->order_list_[k];
          break;
        }
      }
      assert(NULL != p);
      fps[j].BindOrder(p);
    }
  }
}

Status::State UserInfo::CreateGroup(const std::string& name,
                                    StockCodeList& code_list,
                                    GroupId* id) {
  base_logic::WLockGd lock(data_->lock_);

  for (size_t i = 0; i < data_->stock_group_list_.size(); ++i) {
    if (name == data_->stock_group_list_[i].name()) {
      LOG_ERROR2("user:%s create group error: group_name:%s exist",
                 data_->name_.data(), name.data());
      return Status::GROUP_NAME_ALREADAY_EXIST;
    }
  }

  GroupId group_id = StockGroup::CreateGroup(data_->id_, name);
  if (INVALID_GROUPID == group_id) {
    return Status::MYSQL_ERROR;
  }

  StockGroup g(data_->id_, group_id, name);
  g.AddStocks(code_list);
  data_->stock_group_list_.push_back(g);

  *id = group_id;
  LOG_MSG2("group_id=%d, init_capitial=%.2f,available_capitial=%.2f",
           g.id(), g.init_capital(), g.available_capital());
  return Status::SUCCESS;
}

StockGroup* UserInfo::GetGroup(GroupId group_id) {
  base_logic::RLockGd lock(data_->lock_);

  for (size_t i = 0; i < data_->stock_group_list_.size(); ++i) {
    if (group_id == data_->stock_group_list_[i].id()) {
      return &data_->stock_group_list_[i];
    }
  }
  return NULL;
}

StockGroup* UserInfo::GetGroupWithNonLock(GroupId group_id) {
  for (size_t i = 0; i < data_->stock_group_list_.size(); ++i) {
    if (group_id == data_->stock_group_list_[i].id()) {
      return &data_->stock_group_list_[i];
    }
  }
  return NULL;
}

Status::State UserInfo::AddStock(GroupId group_id, StockCodeList& code_list) {
  base_logic::WLockGd lock(data_->lock_);

//  if (0 == group_id) {
//    group_id = data_->default_gid_;
//  }

  StockGroup* g = GetGroupWithNonLock(group_id);
  if (NULL == g) {
    LOG_ERROR2("user:%s add stock error: group_id:%d not exist",
               data_->name_.data(), group_id);
    return Status::GROUP_NOT_EXIST;
  }

  if (!g->AddStocks(code_list)) {
    return Status::MYSQL_ERROR;
  }

  return Status::SUCCESS;
}

Status::State UserInfo::DelStock(GroupId group_id, StockCodeList& code_list) {
  base_logic::WLockGd lock(data_->lock_);

//  if (0 == group_id) {
//    group_id = data_->default_gid_;
//  }

  StockGroup* g = GetGroupWithNonLock(group_id);
  if (NULL == g) {
    LOG_ERROR2("user:%s del stock error: group_id:%d not exist",
               data_->name_.data(), group_id);
    return Status::GROUP_NOT_EXIST;
  }

  if (!g->DelStocks(code_list)) {
    return Status::MYSQL_ERROR;
  }

  return Status::SUCCESS;
}

Status::State UserInfo::GetGroupStock(GroupId group_id, StockCodeList& stocks) {
  base_logic::RLockGd lock(data_->lock_);

//  if (0 == group_id) {
//    group_id = data_->default_gid_;
//  }

  StockGroup* g = GetGroupWithNonLock(group_id);
  if (NULL == g) {
    LOG_ERROR2("user:%s get group stock error: group_id:%d not exist",
               data_->name_.data(), group_id);
    return Status::GROUP_NOT_EXIST;
  }

  stocks = g->stocks();
  return Status::SUCCESS;
}

GroupStockPositionList UserInfo::GetHoldingStocks() {
  base_logic::RLockGd lock(data_->lock_);

  return data_->stock_position_list_;
}

OrderList UserInfo::FindOrders(const OrderFilterList& filters) {
  base_logic::RLockGd lock(data_->lock_);

  OrderList orders;
  for (size_t i = 0; i < data_->order_list_.size(); ++i) {
    bool filter = true;
    OrderInfo* curr_order = data_->order_list_[i];
//    LOG_MSG2("\n order_info= \n %s", curr_order->ToString().c_str());
    for (size_t j = 0; j < filters.size(); ++j) {
      if (filters[j]->filter(*curr_order)) {
        filter = false;
        break;
      }
    }
    if (filter) {
      orders.push_back(curr_order);
    }

  }
  LOG_MSG2("user_id:%d,total_orders:%d,filter_order:%d",
             data_->id_, data_->order_list_.size(), orders.size() );
  return orders;


//  OrderList orders;
//  for (size_t i = 0; i < data_->order_list_.size(); ++i) {
//    bool filter = false;
//    for (size_t j = 0; j < filters.size(); ++j) {
//      if (!filters[j]->filter(*data_->order_list_[i])) {
//        filter = true;
//        break;
//      }
//    }
//    if (filter) {
//      orders.push_back(data_->order_list_[i]);
//    }
//  }
//  return orders;
}

GroupStockPositionList UserInfo::GetAllGroupStockPosition() {
  base_logic::RLockGd lock(data_->lock_);

  return data_->stock_position_list_;
}

GroupStockPosition* UserInfo::GetGroupStockPosition(GroupId group_id,
                                                    const std::string& code) {
  base_logic::RLockGd lock(data_->lock_);

  StockGroup* g = GetGroupWithNonLock(group_id);
  if (NULL == g) {
    LOG_ERROR2("user:%s get stock position error: group_id:%d not exist",
               data_->name_.data(), group_id);
    return NULL;
  }

  for (size_t i = 0; i < data_->stock_position_list_.size(); ++i) {
    if (data_->stock_position_list_[i].group_id() == group_id
        && data_->stock_position_list_[i].code() == code) {
      return &data_->stock_position_list_[i];
    }
  }
  return NULL;
}

GroupStockPosition* UserInfo::GetGroupStockPositionWithNonLock(
    GroupId group_id,
    const std::string& code) {
  StockGroup* g = GetGroupWithNonLock(group_id);
  if (NULL == g) {
    LOG_ERROR2("user:%s get stock position error: group_id:%d not exist",
               data_->name_.data(), group_id);
    return NULL;
  }

  for (size_t i = 0; i < data_->stock_position_list_.size(); ++i) {
    if (data_->stock_position_list_[i].group_id() == group_id
        && data_->stock_position_list_[i].code() == code) {
      return &data_->stock_position_list_[i];
    }
  }
  return NULL;
}

GroupStockPositionList UserInfo::GetGroupStockPosition(GroupId group_id) {
  base_logic::RLockGd lock(data_->lock_);

  StockGroup* g = GetGroupWithNonLock(group_id);
  if (NULL == g) {
    LOG_ERROR2("user:%s get stock position error: group_id:%d not exist",
               data_->name_.data(), group_id);
    return GroupStockPositionList();
  }

  GroupStockPositionList l;
  for (size_t i = 0; i < data_->stock_position_list_.size(); ++i) {
    if (data_->stock_position_list_[i].group_id() == group_id) {
      l.push_back(data_->stock_position_list_[i]);
    }
  }
  return l;
}

Status::State UserInfo::OnBuyOrder(SubmitOrderReq& req, double* frozen) {
  // check available capital
  double need = 0.0;
  need = req.order_price * req.order_nums;

  // 佣金
  double commission = need * COMMISSION_RATE;
  int round_commission = ROUND_COMMISSION(commission);

  int transfer_fee = 0;
  if (IS_SH_CODE(req.code)) {
    transfer_fee = TRANSFER_FEE(req.order_nums);
  }

  need += round_commission + transfer_fee;

  LOG_DEBUG2("stock=%s, 印花税=%.2f, 过户费=%.2f",
             req.code.c_str(), round_commission, transfer_fee);

  StockGroup* g = GetGroupWithNonLock(req.group_id);
  assert(NULL != g);
  if (need > g->available_capital()) {
    LOG_ERROR2("available capital not enough, "
                   "available_capital:%lf, need:%lf",
               g->available_capital(), need);
    return Status::CAPITAL_NOT_ENOUGH;
  }

  *frozen = need;
  return Status::SUCCESS;
}

Status::State UserInfo::OnSellOrder(SubmitOrderReq& req) {
  GroupStockPosition* p = GetGroupStockPositionWithNonLock(req.group_id, req.code);
  if (NULL == p) {
    LOG_ERROR2("user:%s submit order error: no stock:%s position",
               data_->name_.data(), req.code.data());
    return Status::NO_HOLDING_STOCK;
  }

  if (!p->Delegate(req.order_nums)) {
    LOG_ERROR2("user:%s submit order error: "
                   "current stock:%s count:%d less than order count:%d",
               data_->name_.data(), req.code.data(), p->count(), req.order_nums);
    req.order_nums = p->available();
    p->Delegate(req.order_nums);
  }
  return Status::SUCCESS;
}

SubmitOrderRes UserInfo::SubmitOrder(SubmitOrderReq& req) {
  WlockThreadrw(data_->lock_);

//  if (0 == req.group_id) {
//    req.group_id = data_->default_gid_;
//  }

  // step 1 获取组合
  SubmitOrderRes res;
  res.order_id = -1;
  res.status.state = Status::SUCCESS;
  StockGroup* g = GetGroupWithNonLock(req.group_id);
  if (NULL == g) {
    UnlockThreadrw(data_->lock_);
    LOG_ERROR2("user:%s submit order error: group_id:%d not exist",
               data_->name_.data(), req.group_id);
    res.status.state = Status::GROUP_NOT_EXIST;
    return res;
  }

//  if (!g->exist_stock(req.code)) {
//    LOG_ERROR2("user:%s submit order error: stock:%s not in group:%s",
//               data_->name_.data(), req.code.data(), g->name().data());
//    return Status::STOCK_NOT_IN_GROUP;
//  }

  // step 2 判断股票是否支持交易
  bool r = false;
  StockTotalInfo stock_total_info;
  r = engine_->GetStockTotalInfoByCode(req.code, stock_total_info);
  if (!r) {
    UnlockThreadrw(data_->lock_);
    LOG_ERROR2("stock:%s NOT EXIST", req.code.data());
    res.status.state = Status::STOCK_NOT_EXIST;
    return res;
  }
//  STOCK_REAL_MAP stock = engine_->GetStockRealInfoMapCopy(req.code);
//  if (stock.empty()) {
//    UnlockThreadrw(data_->lock_);
//    LOG_ERROR2("stock:%s NOT EXIST", req.code.data());
//    res.status.state = Status::STOCK_NOT_EXIST;
//    return res;
//  }

  Status::State status;
  double frozen = 0.0;

  // step 3 判断当前交易是否非法
  if (BUY == req.op) {
    status = OnBuyOrder(req, &frozen);
  } else if (SELL == req.op) {
    status = OnSellOrder(req);
  }

  // step 4 如果非法直接返回
  if (Status::SUCCESS != status) {
    UnlockThreadrw(data_->lock_);
    res.status.state = status;
    return res;
  }

  // step 5 插入mysql委托记录表，更新组合可用资金和冻结资金
  // insert into mysql
  // 1. insert delegation_record table
  // 2. update available capital and frozen capital
  std::ostringstream oss;
  oss << "CALL `proc_InsertDelegation`("
      << data_->id_ << ","
      << req.group_id << ","
      << "'" << req.code << "'" << ","
      << req.order_price << ","
      << req.expected_price << ","
      << (int) req.op << ","
      << req.order_nums << ","
      << frozen << ")";
  MYSQL_ROWS_VEC row;
  if (!engine_->ExcuteStorage(1, oss.str(), row)) {
    UnlockThreadrw(data_->lock_);
    LOG_ERROR2("user:%s submit order error: mysql error",
               data_->name_.data());
    res.status.state = Status::MYSQL_ERROR;
    return res;
  }
  assert(!row.empty() && !row[0].empty());
  OrderId order_id = atoi(row[0][0].data());

  // step 6 如果是委买修改用户资金
  // frozen commitment
  if (BUY == req.op) {
    g->OnDelegateBuyOrderDelegate(frozen);
  }

  // step 7 插入用户委托表
  data_->order_list_.push_back(new OrderInfo(data_->id_, order_id));
  OrderInfo* order = data_->order_list_[data_->order_list_.size() - 1];
  order->Init(req);
  order->set_frozen(frozen);

  LOG_MSG2("user:%s new order:%d, code:%s, count:%d, frozen=%.2f",
           data_->name_.data(), order_id, req.code.data(), req.order_nums, frozen);

  res.order_id = order_id;

  // step 8 判断交易能否成功
  // check can make a deal
  StockRealInfo stock_real_info;
  r = stock_total_info.GetCurrRealMarketInfo(stock_real_info);
  if (r && order->can_deal(stock_real_info.price)) {
    UnlockThreadrw(data_->lock_);
    order->MakeADeal(stock_real_info.price);
    return res;
  }

//  if (order->can_deal(stock.rbegin()->second.price)) {
//    UnlockThreadrw(data_->lock_);
//    order->MakeADeal(stock.rbegin()->second.price);
//    return res;
//  }

  // step 9 当前不能成交， 等待后续行情
  // cannot make a deal now, register callback
  engine_->AttachObserver(order);
  UnlockThreadrw(data_->lock_);
  return res;
}

bool UserInfo::OnBuyOrderDone(OrderInfo* order) {
  StockGroup* g = GetGroupWithNonLock(order->group_id());
  assert(g != NULL);

  // 调整冻结资金
  double curr_cost = order->amount() + order->commission() + order->transfer_fee();
  g->OnBuyOrderDone(order->frozen(), curr_cost);

  order->set_available_capital(g->available_capital());

  // update mysql
  std::ostringstream oss;
  oss << "CALL proc_BuyOrderDone("
      << order->id() << ","
      << order->deal_price() << ","
      << order->deal_num() << ","
      << order->stamp_duty() << ","
      << order->commission() << ","
      << order->transfer_fee() << ","
      << order->amount() << ","
      << order->available_capital() << ")";

  MYSQL_ROWS_VEC row;
//  SSEngine* engine = GetStradeShareEngine();
  std::string sql = oss.str();
  if (!engine_->ExcuteStorage(1, sql, row)) {
    LOG_ERROR2("user:%s update buy order mysql error", data_->name_.data());
    return false;
  }

  LOG_MSG2("order_buy_down, excute sql=%s", sql.c_str());
  // add new FakeStockPosition
//  assert(!row.empty());
//  StockPositionId id = atoi(row[0][0].data());

  //TODO 待优化
  row.clear();
  static const std::string SELECT_MAX_ID_SQL = "SELECT MAX(holdingId) FROM holding_record";
  if (!engine_->ExcuteStorage(1, SELECT_MAX_ID_SQL, row)) {
    LOG_ERROR2("user:%s update buy order mysql error", data_->name_.data());
    return false;
  }

  // add new FakeStockPosition
  assert(!row.empty() && !row[0].empty());
  StockPositionId id = atoi(row[0][0].data());

  FakeStockPosition fp(id, order);
  GroupStockPosition* gp =
      GetGroupStockPositionWithNonLock(order->group_id(), order->code());
  if (NULL == gp) {
    GroupStockPosition p(data_->id_, order->group_id(), order->code());
    data_->stock_position_list_.push_back(p);
    gp = &p;
  }
  gp->AddFakeStockPosition(fp);
  LOG_MSG2("group_id:%d, available:%d,holdingId:%d",
           gp->group_id(), gp->available(), id);

  //TODO 自动止损先去掉
  return true;

  // check auto order
  if (order->expected_price() < 1.0) {
    return true;
  }

  // automatic generate order
  oss.str("");
  oss << "CALL `proc_InsertDelegation`("
      << data_->id_ << ","
      << order->group_id() << ","
      << "'" << order->code() << "'" << ","
      << order->expected_price() << ","
      << 0 << ","
      << (int) SELL << ","
      << order->deal_num() << ","
      << 0 << ")";
  row.clear();
  if (!engine_->ExcuteStorage(1, oss.str(), row)) {
    LOG_ERROR2("user:%s AUTO generate order error: mysql error",
               data_->name_.data());
    return false;
  }
  assert(!row.empty());
  OrderId order_id = atoi(row[0][0].data());
  LOG_MSG2("user:%s new order:%d, code:%s, count:%d",
           data_->name_.data(), order_id, order->code().data(), order->deal_num());

  data_->order_list_.push_back(new OrderInfo(data_->id_, order_id, OrderInfo::AUTO_ORDER));
  OrderInfo* new_order = data_->order_list_[data_->order_list_.size() - 1];
  new_order->Init(*order);

  gp->Delegate(order->deal_num());
  engine_->AttachObserver(new_order);
  return true;
}

bool UserInfo::OnSellOrderDone(OrderInfo* order) {
  StockGroup* g = GetGroupWithNonLock(order->group_id());
  assert(g != NULL);
  // update user available capital
  double profit = order->deal_price() * order->deal_num();
  profit -= order->stamp_duty();
  profit -= order->transfer_fee();
  profit -= order->commission();
  g->OnSellOrderDone(profit);
  order->set_available_capital(g->available_capital());

  LOG_MSG2("OnSell Total:%.2f", profit);

  // pick FakeStockPosition
  FakeStockPositionList fp_list;
  GroupStockPosition* gp =
      GetGroupStockPositionWithNonLock(order->group_id(), order->code());
  if (NULL == gp) {
    LOG_ERROR2("user:%s not find stock:%s group:%d position",
               data_->name_.data(), order->code().data(), order->group_id());
    return false;
  }
  gp->OnOrderDone(order->deal_num(), fp_list);

  // calculate profit
  double cost = 0.0;
  std::ostringstream oss;
  for (size_t i = 0; i < fp_list.size(); ++i) {
    OrderInfo* order = fp_list[i].order();
    int remain = fp_list[i].last_count() - fp_list[i].count();
    fp_list[i].set_last_count(fp_list[i].count());
    double ratio = (1.0 * remain / order->deal_num());
    double curr_fp_amount = 0.0;
    double curr_fp_commission = 0.0;
    double curr_fp_transfee = 0.0;
    if (remain > 0) {
      curr_fp_amount = order->amount() * ratio;
      curr_fp_commission = order->commission() * ratio;
      curr_fp_transfee = order->transfer_fee() * ratio;
    } else {
      curr_fp_amount = order->amount();
      curr_fp_commission = order->commission();
      curr_fp_transfee = order->transfer_fee();
    }
    LOG_MSG2("remain:%d, ratio:%.2f, curr_fp_amount:%.2f,curr_fp_commission:%.2f,curr_fp_transfee:%.2f",
             remain, ratio, curr_fp_amount, curr_fp_commission, curr_fp_transfee);
    cost += curr_fp_amount + curr_fp_commission + curr_fp_transfee;
    oss << fp_list[i].id() << ":" << fp_list[i].count() << ",";
  }

  profit -= cost;
  order->set_profit(profit);
  LOG_MSG2("OnSell Cost:%.2f, Profit:%.2f", cost, profit);

  std::string h = oss.str();
  oss.str("");
  h.erase(h.size() - 1);

  oss << "CALL `proc_SellOrderDone`("
      << order->id() << ","
      << order->deal_price() << ","
      << order->deal_num() << ","
      << order->stamp_duty() << ","
      << order->commission() << ","
      << order->transfer_fee() << ","
      << order->profit() << ","
      << order->available_capital() << ","
      << "'" << h << "')";

  LOG_MSG2("proc_SellOrderDone: %s", oss.str().data());
//  SSEngine* engine = GetStradeShareEngine();
  if (!engine_->WriteData(oss.str())) {
    LOG_ERROR2("user:%s update sell order mysql error", data_->name_.data());
    return false;
  }
  return true;
}

void UserInfo::OnOrderDone(OrderInfo* order) {
  base_logic::WLockGd lock(data_->lock_);

  // update stock position info

  // update items:
  // 1. update user frozen capital
  // 2. update user_info record_deal delegation_record hold_postion

  if (BUY == order->operation()) {
    OnBuyOrderDone(order);
  }

  if (SELL == order->operation()) {
    OnSellOrderDone(order);
  }
}

Status::State UserInfo::OnCancelBuyOrder(const OrderInfo* order) {
  base_logic::WLockGd lock(data_->lock_);

  // step 1 update user available capital and frozen capital
  StockGroup* g = GetGroupWithNonLock(order->group_id());
  assert(g != NULL);

  // step 2 update mysql
  // 1. update delegation_record
  // 2. update user available and frozen capital
  std::ostringstream oss;
  oss << "CALL proc_CancelBuyDelegation(" << order->id() << ")";
  if (!engine_->WriteData(oss.str())) {
    LOG_ERROR2("user:%s cancel buy order mysql error", data_->name_.data());
    return Status::MYSQL_ERROR;
  }

  // step 3 更新内存
  g->OnCancelBuyOrder(order->frozen());

  return Status::SUCCESS;
}

Status::State UserInfo::OnCancelSellOrder(const OrderInfo* order) {
  base_logic::WLockGd lock(data_->lock_);

  // step 1 获取组合
  GroupStockPosition* p =
      GetGroupStockPositionWithNonLock(order->group_id(), order->code());
  assert(p != NULL);

  // step 2 update delegation_record status
  std::ostringstream oss;
  oss << "CALL proc_CancelSaleDelegation(" << order->id() << ")";
  if (!engine_->WriteData(oss.str())) {
    LOG_ERROR2("user:%s cancel sell order mysql error", data_->name_.data());
    return Status::MYSQL_ERROR;
  }

  // step 3 修改内存
  p->OnOrderCancel(order->order_num());
  return Status::SUCCESS;
}

Status::State UserInfo::CancleOrder(OrderInfo* order) {
  if (NULL == order) {
    return Status::FAILED;
  }

  Status::State status;
  order->OnOrderCancel();
  if (BUY == order->operation()) {
    status = OnCancelBuyOrder(order);
  }

  if (SELL == order->operation()) {
    status = OnCancelSellOrder(order);
  }
  return status;
}

Status::State UserInfo::OnCancelOrder(OrderId order_id) {
  Status::State status;
  OrderList::iterator order = data_->order_list_.end();
  for (OrderList::iterator it = data_->order_list_.begin();
       data_->order_list_.end() != it; ++it) {
    if ((*it)->id() == order_id) {
      order = it;
      break;
    }
  }

  if (data_->order_list_.end() == order) {
    return Status::ORDER_NOT_EXIST;
  }

  if ((*order)->status() != PENDING) {
    return Status::FAILED;
  }

  LOG_DEBUG2("user:%s cancle order:%d", data_->name_.data(), (*order)->id());
  status = CancleOrder(*order);

  // TODO remove order
  //  delete *order;
  //  data_->order_list_.erase(order);
  return status;
}

Status::State UserInfo::OnModifyInitCapital(GroupId group_id, double capital) {
  base_logic::WLockGd lock(data_->lock_);

  StockGroup* g = GetGroupWithNonLock(group_id);
  if (NULL == g) {
    return Status::GROUP_NOT_EXIST;
  }

  if (capital < 0.0) {
    return Status::FAILED;
  }

  double curr_init_captial = g->init_capital() + capital;
  double curr_available_capital = g->available_capital() + capital;
  std::ostringstream oss;
  oss << "UPDATE `group_info` "
      << "SET `initCapital` = " << curr_init_captial << ","
      << "`availableCapital` = " << curr_available_capital
      << " WHERE `groupId` = " << g->id() << " AND `userId` = " << data_->id_;
  if (!engine_->WriteData(oss.str())) {
    LOG_ERROR2("user:%s modify init capital mysql error", data_->name_.data());
    return Status::MYSQL_ERROR;
  }
  g->add_init_capital(capital);
  LOG_DEBUG2("user_id=%d, group_id=%d, init_capital=%.2f, available_captial=%.2f",
             data_->id_, g->id(), g->init_capital(), g->available_capital());
  return Status::SUCCESS;
}

void UserInfo::OnCloseMarket() {
  OrderList tmp;
  for (size_t i = 0; i < data_->order_list_.size(); ++i) {
    if (data_->order_list_[i]->status() == PENDING) {
      CancleOrder(data_->order_list_[i]);
      delete data_->order_list_[i];
    } else {
      tmp.push_back(data_->order_list_[i]);
    }
  }

  // remove from cache
  data_->order_list_.swap(tmp);
}

} /* namespace strade_user */
