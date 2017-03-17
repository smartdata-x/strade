//
// Created by Harvey on 2017/1/11.
//

#ifndef STRADE_COMM_HEAD_H_H
#define STRADE_COMM_HEAD_H_H
#include "../net/operator_code.h"
namespace strade_logic {

// 共享数据通知信号
enum STRADE_SHARE_SIGNAL {

  // 实时行情更新
  REALTIME_MARKET_VALUE_UPDATE  = 0,

};

} /* namespace strade_logic */

#endif //STRADE_COMM_HEAD_H_H
