//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2017/1/9/ Author: zjc

#ifndef SRC_PLUGINS_USER_USER_LOGIC_H_
#define SRC_PLUGINS_USER_USER_LOGIC_H_

#include "core/common.h"

namespace base_logic {
class Value;
class ListValue;
class DictionaryValue;
}

using base_logic::Value;
using base_logic::ListValue;
using base_logic::DictionaryValue;

namespace strade_user {

class ResHead;
class Status;

class UserLogic {
 public:
  UserLogic();
  virtual ~UserLogic();

 private:
  static UserLogic *instance_;

 public:
  static UserLogic *GetInstance();
  static void FreeInstance();

 public:

  bool OnUserConnect(struct server *srv, const int socket);

  bool OnUserMessage(struct server *srv, const int socket, const void *msg,
                      const int len);

  bool OnUserClose(struct server *srv, const int socket);

  bool OnBroadcastConnect(struct server *srv, const int socket,
                          const void *data, const int len);

  bool OnBroadcastMessage(struct server *srv, const int socket, const void *msg,
                          const int len);

  bool OnBroadcastClose(struct server *srv, const int socket);

  bool OnIniTimer(struct server *srv);

  bool OnTimeout(struct server *srv, char* id, int opcode, int time);
 private:
  DictionaryValue* HttpDeserialize(const char* msg, int len);

  void OnCreateGroup(int socket, DictionaryValue& dict);
  void OnAddStock(int socket, DictionaryValue& dict);
  void OnDelStock(int socket, DictionaryValue& dict);
  void OnQueryGroup(int socket, DictionaryValue& dict);
  void OnQueryStock(int socket, DictionaryValue& dict);
  void OnQueryGroupHoldingStock(int socket, DictionaryValue& dict);
  void OnQueryHoldingStock(int socket, DictionaryValue& dict);
  void OnQueryTodayOrder(int socket, DictionaryValue& dict);
  void OnQueryTodayFinishedOrder(int socket, DictionaryValue& dict);
  void OnQueryHistoryFinishedOrder(int socket, DictionaryValue& dict);
  void OnQueryStatement(int socket, DictionaryValue& dict);
  void OnSubmitOrder(int socket, DictionaryValue& dict);
  void OnAvailableStockCount(int socket, DictionaryValue& dict);
  void OnCancelOrder(int socket, DictionaryValue& dict);
  void OnProfitAndLossOrderNum(int socket, DictionaryValue& dict);
  void OnModifyInitCapital(int socket, DictionaryValue& dict);
  void OnSubmitMultiOrder(int socket, DictionaryValue& dict);
  void OnModifyGroupName(int socket, DictionaryValue& dict);
  void OnDelGroup(int socket, DictionaryValue& dict);

  void ProcessClose();

  bool SendResponse(int socket, Status& status);
  bool SendResponse(int socket, ResHead& msg);
};

} /* namespace strade_user */

#endif /* SRC_PLUGINS_USER_USER_LOGIC_H_ */
