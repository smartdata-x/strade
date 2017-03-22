//  Copyright (c) 2015-2015 The restful Authors. All rights reserved.
//  Created on: 2015/11/24 Author: jiaoyongqing
//
#ifndef _NET_OPERATOR_CODE_H_
#define _NET_OPERATOR_CODE_H_

#define STRADE_SOCKET_PATH "/var/www/tmp/stradecorefile"

enum netoperatorcode{

  USER_API = 10001,
  CANDLESTICK_HISTORY = 20001,
  REALINFO_LATESTN = 20002,
  REALINFO_INDEX = 20003,
  REALINFO_TODAY = 20004,

  USER_ACCOUNT_INFO = 30001,
  YIELDS_HISTORY = 30002,
  MODIFY_GROUP_NAME = 30003,
  DELETE_GROUP = 30004,

};


#endif
