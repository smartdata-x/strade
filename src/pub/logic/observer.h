//
// Created by Harvey on 2017/1/11.
//

#ifndef STRADE_OBSERVER_H
#define STRADE_OBSERVER_H

#include "logic/logic_unit.h"
#include "subject.h"

namespace strade_logic {

class Observer {
 public:
  Observer();
  virtual ~Observer();
  virtual void Init();
  bool stale() const { return stale_; }
  virtual void Update(int opcode, void* param=NULL) = 0;
 protected:
  bool stale_;
};

} /* namespace strade_logic */

#endif //STRADE_OBSERVER_H
