//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2016/10/13 Author: zjc

#include <algorithm>

#include "stock_logic.h"
#include "stock_db.h"
#include "logic/logic_comm.h"
#include "logic/stock_util.h"
#include "strade_share/strade_share_engine.h"

#define DEFAULT_CONFIG_PATH     "./plugins/stock/stock_config.xml"
#define SHARE_LIB_PATH          "./strade_share/strade_share.so"
#define SHARE_LIB_HANDLER       "GetStradeShareEngine"

#define TIMER_SIMULATE  (DataEngine::MAX_TYPE)

namespace stock_logic {

using strade_share::SSEngine;

SSEngine* engine = NULL;
StockLogic* StockLogic::instance_ = NULL;
const char* const StockLogic::hkey_prefix_[DataEngine::MAX_TYPE] = {
    "sh_stock_",
    "sz_stock_"
};

StockLogic::StockLogic() {
  if (!Init())
    assert(0);
}

StockLogic::~StockLogic() {

}

bool StockLogic::Init() {
  config::FileConfig* conf = config::FileConfig::GetFileConfig();
  if (NULL == conf) {
    return false;
  }
  std::string path = DEFAULT_CONFIG_PATH;
  if (!conf->LoadConfig(path)) {
    return false;
  }

  db_ = new StockDB(conf);
  redis_.InitParam(conf->redis_list_);

  for (size_t i = 0; i < DataEngine::MAX_TYPE; ++i) {
    data_[i].Init((DataEngine::Type)i, hkey_prefix_[i],
                  db_, &redis_);
  }

  SSEngine* (* handler)(void);
  handler = (strade_share::SSEngine* (*)(void))
      base_logic::SomeUtils::GetLibraryFunction(
          SHARE_LIB_PATH, SHARE_LIB_HANDLER);
  engine = (*handler)();
  if (engine == NULL)
    assert(0);
  engine->Init();
  LOG_DEBUG2("engine: %p", engine);
  return true;
}

StockLogic*
StockLogic::GetInstance() {
  if (instance_ == NULL)
    instance_ = new StockLogic();
  return instance_;
}

void StockLogic::FreeInstance() {
  delete instance_;
  instance_ = NULL;
}

bool StockLogic::OnStockConnect(struct server *srv, const int socket) {
  return true;
}

bool StockLogic::OnStockMessage(struct server *srv, const int socket,
                                const void *msg, const int len) {
  return true;
}

bool StockLogic::OnStockClose(struct server *srv, const int socket) {
  return true;
}

bool StockLogic::OnBroadcastConnect(struct server *srv, const int socket,
                                    const void *msg, const int len) {
  return true;
}

bool StockLogic::OnBroadcastMessage(struct server *srv, const int socket,
                                    const void *msg, const int len) {
  return true;
}

bool StockLogic::OnBroadcastClose(struct server *srv, const int socket) {
  return true;
}

bool StockLogic::OnIniTimer(struct server *srv) {
  srv->add_time_task(srv, "stock", DataEngine::SH, 5, -1);
  srv->add_time_task(srv, "stock", DataEngine::SZ, 5, -1);
#ifdef DEBUG_TEST
  srv->add_time_task(srv, "stock", TIMER_SIMULATE, 30, -1);
#endif
  return true;
}

bool StockLogic::OnTimeout(struct server *srv, char *id, int opcode, int time) {
  switch (opcode) {
    case DataEngine::SH:
      StartSHEngine();
      break;
    case DataEngine::SZ:
      StartSZEngine();
      break;
    case TIMER_SIMULATE:
      Simulate();
      break;
    default:
      break;
  }
  return true;
}

void StockLogic::StartSHEngine() {
  data_[DataEngine::SH].OnTime();
}

void StockLogic::StartSZEngine() {
  data_[DataEngine::SZ].OnTime();
}

void StockLogic::Simulate() {
  data_[DataEngine::SH].Simulate();
  data_[DataEngine::SZ].Simulate();
}
} /* namespace stock_logic */
