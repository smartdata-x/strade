//
// Created by Harvey on 2017/1/11.
//

#include "subject.h"

#include "observer.h"

namespace strade_logic {

Subject::Subject() {

}

Subject::~Subject() {

}

void Subject::Attach(Observer* observer) {
  this->m_lst.push_back(observer);
  LOG_MSG2("new observe attach, curr total observe size:%d", m_lst.size());
}

void Subject::Detach(Observer* observer) {
  std::list<Observer*>::iterator iter;
  iter = find(m_lst.begin(), m_lst.end(), observer);
  if (iter != m_lst.end()) {
    m_lst.erase(iter);
  }
}

void Subject::Notify(int opcode, void* param) {
  typedef std::list<Observer*>::iterator ITER_TYPE;
  ITER_TYPE iter = this->m_lst.begin();
  for (; iter != m_lst.end(); ) {
    ITER_TYPE iter_tmp(iter);
    ++iter;

    if ((*iter_tmp)->stale()) {
      m_lst.erase(iter_tmp);
      LOG_DEBUG("order observer detach");
      continue;
    }
    (*iter_tmp)->Update(opcode, param);
  }
}

} /* namespace strade_logic */
