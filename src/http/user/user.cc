
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>
#include <sstream>

// #include "client/linux/handler/exception_handler.h"

//#if defined (FCGI_STD)
//#include <fcgi_stdio.h>
//#elif defined(FCGI_PLUS)
//#include <fcgio.h>
//#include <fcgi_config.h>
//#endif

#include "fcgimodule/fcgimodule.h"
#include "logic/comm_head.h"
#include "net/operator_code.h"
// 设置请求类型



int main(int agrc, char* argv[]) {
  fcgi_module::FcgiModule fcgi_client;
  fcgi_client.Init(STRADE_SOCKET_PATH, USER_API, 5);
  fcgi_client.Run();
  fcgi_client.Close();
  return 0;
}
