//
// Created by Harvey on 2017/1/19.
//

#ifndef STRADE_STRADE_SHARE_TIMER_H
#define STRADE_STRADE_SHARE_TIMER_H

#include "strade_share/strade_share_engine.h"
#include "storage/mysql_engine.h"
#include "logic/observer.h"

using strade_logic::StockRealInfo;

namespace strade_share_logic {

class StradeShareTimer : public strade_logic::Observer{
 private:
  StradeShareTimer();

 public:
  static StradeShareTimer* GetInstance();
  bool InitParam();

  bool OnTimeLoadStockVisit();

  void OnTimeTest();

 private:
  virtual void Update(int opcode, void* param);
  bool JudgeUpdateTodayHist();
  void WriteStockCurrHistTODB(std::vector<StockRealInfo>& today_hist);

  std::string SerializeStockHistSql(const StockRealInfo& stock_real_info);

 private:
  static void OnTimeLoadStockVisitCallback(
      int column_num, MYSQL_ROWS_VEC& rows_vec, void* param);

 private:
  static strade_share::SSEngine* ss_engine_;
};


} /* namespace strade_share_logic */

#endif //STRADE_STRADE_SHARE_TIMER_H
