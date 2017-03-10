// author: pangh
// date: Mon Jun 20 16:50:31 2016

#include "realinfo_init.h"
#include <signal.h>
#include <stdlib.h>
#include "core/common.h"
#include "core/plugins.h"
#include "realinfo/realinfo_logic.h"

struct RealInfoplugin {
  char* id;
  char* name;
  char* version;
  char* provider;
};

static void *OnRealInfoStart() {
  signal(SIGPIPE, SIG_IGN);
  struct RealInfoplugin* realinfo = (struct RealInfoplugin*) calloc(
      1, sizeof(struct RealInfoplugin));
  realinfo->id = "realinfo";
  realinfo->name = "realinfo";
  realinfo->version = "1.0.0";
  realinfo->provider = "pangh";

  realinfo::RealInfoLogic::GetInstance();
  return realinfo;
}

static handler_t OnRealInfoShutdown(struct server* srv, void* pd) {
  return HANDLER_GO_ON;
}

static handler_t OnRealInfoConnect(struct server *srv, int fd, void *data,
                                      int len) {
  return HANDLER_GO_ON;
}

static handler_t OnRealInfoMessage(struct server *srv, int fd, void *data,
                                      int len) {
  if (realinfo::RealInfoLogic::GetInstance()->OnRealInfoMessage(srv,
                                                                         fd,
                                                                         data,
                                                                         len)) {
    return HANDLER_FINISHED;
  }
  return HANDLER_GO_ON;
}

static handler_t OnRealInfoClose(struct server *srv, int fd) {

  return HANDLER_GO_ON;
}

static handler_t OnUnknow(struct server *srv, int fd, void *data, int len) {
  return HANDLER_GO_ON;
}

static handler_t OnBroadcastConnect(struct server* srv, int fd, void *data,
                                    int len) {

  return HANDLER_GO_ON;
}

static handler_t OnBroadcastClose(struct server* srv, int fd) {

  return HANDLER_GO_ON;
}

static handler_t OnBroadcastMessage(struct server* srv, int fd, void *data,
                                    int len) {

  return HANDLER_GO_ON;
}

static handler_t OnIniTimer(struct server* srv) {
  realinfo::RealInfoLogic::GetInstance()->OnIniTimer(srv);
  return HANDLER_GO_ON;
}

static handler_t OnTimeOut(struct server* srv, char* id, int opcode, int time) {
  realinfo::RealInfoLogic::GetInstance()->OnTimeout(srv, id, opcode,
                                                          time);

  return HANDLER_GO_ON;
}

int realinfo_plugin_init(struct plugin *pl) {
  pl->init = OnRealInfoStart;
  pl->clean_up = OnRealInfoShutdown;
  pl->connection = OnRealInfoConnect;
  pl->connection_close = OnRealInfoClose;
  pl->connection_close_srv = OnBroadcastClose;
  pl->connection_srv = OnBroadcastConnect;
  pl->handler_init_time = OnIniTimer;
  pl->handler_read = OnRealInfoMessage;
  pl->handler_read_srv = OnBroadcastMessage;
  pl->handler_read_other = OnUnknow;
  pl->time_msg = OnTimeOut;
  pl->data = NULL;
  return 0;
}

