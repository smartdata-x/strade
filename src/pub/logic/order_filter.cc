//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2017/1/11 Author: zjc

#include "order_filter.h"
#include "order_info.h"

namespace strade_user {

bool OrderFilter::filter(const OrderInfo& order) {
  if (0 == group_id_
      || order.group_id() == group_id_)
    return false;
  return true;
}

bool OrderOperationFilter::filter(const OrderInfo& order) {
  if (order.operation() == op_) {
    return false;
  }
  return true;
}

bool OrderStatusFilter::filter(const OrderInfo& order) {
  if (order.status() == status_) {
    return false;
  }
  return true;
}

bool OrderCreateTimeFilter::filter(const OrderInfo& order) {
  if (order.create_time() >= begin_time_
      && order.create_time() <= end_time_) {
    return false;
  }
  return true;
}

bool OrderDealTimeFilter::filter(const OrderInfo& order) {
  if (order.deal_time() >= begin_time_
      && order.deal_time() <= end_time_) {
    return false;
  }
  return true;
}

bool OrderProfitFilter::filter(const OrderInfo& order) {
  if(order.profit() > 0.0) {
    return false;
  }
  return true;
}

bool OrderLossFilter::filter(const OrderInfo& order) {
  if (order.profit() < 0.0) {
    return false;
  }
  return true;
}
} /* namespace strade_user */
