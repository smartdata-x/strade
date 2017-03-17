//
// Created by Harvey on 2017/1/11.
//

#include "observer.h"

namespace strade_logic {

Observer::Observer() {
  Init();
}

Observer::~Observer() {

}

void Observer::Init() {
  stale_ = false;
}

} /* namespace strade_logic */
