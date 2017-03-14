//Copyright (c) 2016-2017 The strade Authors. All rights reserved.
//Created on: 2017/01/09 Author: tangtao

#include "yields/yields_init.h"

#include <signal.h>

#include "core/common.h"
#include "core/plugins.h"
#include "logic/logic_comm.h"
#include "yields/yields_logic.h"

struct yieldsplugin {
  char *id;
  char *name;
  char *version;
  char *provider;
};

static void* OnYieldsStart() {
  signal(SIGPIPE, SIG_IGN);
  struct yieldsplugin *yields = (struct yieldsplugin*)calloc(1, \
                                   sizeof(struct yieldsplugin));

  yields->id = "yields";

  yields->name = "yields";

  yields->version = "1.0.0";

  yields->provider = "tangtao";

  return yields;
}

static handler_t OnYieldsShutdown(struct server *srv, void *pd) {
  return HANDLER_GO_ON;
}

static handler_t OnYieldsConnect(struct server *srv, int fd, void *data, int len) {
  yields::YieldsLogic::GetInstance()->OnYieldsConnect(srv, fd);

  return HANDLER_GO_ON;
}

static handler_t OnYieldsMessage(struct server *srv, int fd, void *data, int len) {
  bool r = yields::YieldsLogic::GetInstance()->OnYieldsMessage(srv,  fd,  data, len);
  if (true == r) {
    return HANDLER_FINISHED;
  } else {
    return HANDLER_GO_ON;
  }
}

static handler_t OnYieldsClose(struct server *srv, int fd) {
  yields::YieldsLogic::GetInstance()->OnYieldsClose(srv, fd);

  return HANDLER_GO_ON;
}

static handler_t OnUnknow(struct server *srv, int fd, void *data, int len) {
  return HANDLER_GO_ON;
}

static handler_t OnBroadcastConnect(struct server *srv, int fd, void *data, int len) {
  yields::YieldsLogic::GetInstance()->OnBroadcastConnect(srv, fd, data, len);

  return HANDLER_GO_ON;
}

static handler_t OnBroadcastClose(struct server *srv, int fd) {
  yields::YieldsLogic::GetInstance()->OnBroadcastClose(srv, fd);

  return HANDLER_GO_ON;
}

static handler_t OnBroadcastMessage(struct server *srv, int fd, void *data, int len) {
  yields::YieldsLogic::GetInstance()->OnBroadcastMessage(srv, fd, data, len);

  return HANDLER_GO_ON;
}

static handler_t OnIniTimer(struct server *srv) {
  yields::YieldsLogic::GetInstance()->OnIniTimer(srv);

  return HANDLER_GO_ON;
}

static handler_t OnTimeOut(struct server *srv, char *id, int opcode, int time) {
  yields::YieldsLogic::GetInstance()->OnTimeout(srv, id, opcode, time);

  return HANDLER_GO_ON;
}

int yields_plugin_init(struct plugin *pl) {
  pl->init = OnYieldsStart;

  pl->clean_up = OnYieldsShutdown;

  pl->connection = OnYieldsConnect;

  pl->connection_close = OnYieldsClose;

  pl->connection_close_srv = OnBroadcastClose;

  pl->connection_srv = OnBroadcastConnect;

  pl->handler_init_time = OnIniTimer;

  pl->handler_read = OnYieldsMessage;

  pl->handler_read_srv = OnBroadcastMessage;

  pl->handler_read_other = OnUnknow;

  pl->time_msg = OnTimeOut;

  pl->data = NULL;

  return 0;
}

