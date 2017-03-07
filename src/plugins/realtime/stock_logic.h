//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2016/10/13 Author: zjc

#ifndef SRC_PLUGINS_REALTIME_STOCK_LOGIC_H_
#define SRC_PLUGINS_REALTIME_STOCK_LOGIC_H_

#include "market_data_engine.h"
#include "storage/redis_controller.h"
#include "core/common.h"

namespace base_logic {
class RedisController;
}

namespace stock_logic {

class StockDB;

class StockLogic {
 public:
  StockLogic();
  virtual ~StockLogic();

 private:
  static StockLogic *instance_;

 public:
  static StockLogic *GetInstance();
  static void FreeInstance();

 public:

  bool OnStockConnect(struct server *srv, const int socket);

  bool OnStockMessage(struct server *srv, const int socket, const void *msg,
                      const int len);

  bool OnStockClose(struct server *srv, const int socket);

  bool OnBroadcastConnect(struct server *srv, const int socket,
                          const void *data, const int len);

  bool OnBroadcastMessage(struct server *srv, const int socket, const void *msg,
                          const int len);

  bool OnBroadcastClose(struct server *srv, const int socket);

  bool OnIniTimer(struct server *srv);

  bool OnTimeout(struct server *srv, char* id, int opcode, int time);
 private:
  void StartSHEngine();
  void StartSZEngine();
  void Simulate();
 private:
  const static char* const hkey_prefix_[DataEngine::MAX_TYPE];

  bool Init();
 private:
  StockDB* db_;
  RedisController redis_;
  DataEngine data_[DataEngine::MAX_TYPE];
};

} /* namespace stock_logic */

#endif /* SRC_PLUGINS_REALTIME_STOCK_LOGIC_H_ */
