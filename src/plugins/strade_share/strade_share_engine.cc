//
// Created by Harvey on 2017/1/4.
//

#include "strade_share_engine.h"

#ifdef KUN_REDIS_POOL
#include "dic/base_dic_redis_auto.h"
#endif

#define DEFAULT_CONFIG_PATH  "./strade_share/stradeshare_config.xml"

strade_share::SSEngine* GetStradeShareEngine(void) {
  return strade_share::SSEngineImpl::GetInstance();
}

namespace strade_share {

SSEngineImpl* SSEngineImpl::instance_ = NULL;
pthread_mutex_t SSEngineImpl::mutex_ = PTHREAD_MUTEX_INITIALIZER;
SSEngineImpl::SSEngineImpl() {
  if (!InitParam()) {
    LOG_ERROR("StradeShareEngineImpl Init error");
    assert(0);
  }
}

SSEngineImpl::~SSEngineImpl() {
  if (NULL != strade_share_db_) {
    delete strade_share_db_;
    strade_share_db_ = NULL;
  }
  DeinitThreadrw(lock_);
}

SSEngineImpl* SSEngineImpl::GetInstance() {
  if (NULL == instance_) {
    pthread_mutex_lock(&mutex_);
    if (NULL == instance_) {
      instance_ = new SSEngineImpl();
    }
    pthread_mutex_unlock(&mutex_);
  }
  return instance_;
}

bool SSEngineImpl::Init() {
  LOG_DEBUG("SSEngine init begin");
  user_engine_ = UserEngine::GetUserEngine();
  if (!user_engine_->Init()) {
    LOG_ERROR("init user engine error");
    return false;
  }
  LOG_DEBUG("SSEngine init end");
  return true;
}

bool SSEngineImpl::InitParam() {
  InitThreadrw(&lock_);
  bool r = false;
  std::string path = DEFAULT_CONFIG_PATH;
  config::FileConfig* config = config::FileConfig::GetFileConfig();
  if (config == NULL) {
    return false;
  }
  r = config->LoadConfig(path);
  if (!r) {
    return false;
  }
  strade_share_db_ = new StradeShareDB(config);
  LoadAllStockBasicInfo();
#ifdef KUN_REDIS_POOL
  assert(base_dic::KunDicPool::GetInstance()->Init(
      config->redis_list_, 100));
#endif
  return true;
}

void SSEngineImpl::AttachObserver(strade_logic::Observer* observer) {
  if (NULL != observer) {
    base_logic::WLockGd lk(lock_);
    this->Attach(observer);
  }
}

void SSEngineImpl::LoadAllStockBasicInfo() {
  base_logic::WLockGd lk(lock_);
  bool r = false;
  std::vector<strade_logic::StockTotalInfo> stock_vec;
  strade_share_db_->FetchAllStockList(stock_vec);
  std::vector<strade_logic::StockTotalInfo>::iterator iter(stock_vec.begin());
  for (; iter != stock_vec.end(); ++iter) {
    strade_logic::StockTotalInfo stock_total_info = (*iter);
    std::vector<strade_logic::StockHistInfo> stock_hist_vec;
    std::string stock_code = stock_total_info.get_stock_code();
    r = strade_share_db_->FetchStockHistList(stock_code, stock_hist_vec);
    if (r) {
      LOG_DEBUG2("stock_code=%s, load stock_hist_data size=%d",
                 stock_code.c_str(), stock_hist_vec.size());
    }
    stock_total_info.AddStockHistVec(stock_hist_vec);
    AddStockTotalInfoNonblock(stock_total_info);
  }
  LOG_DEBUG2("LoadAllStockInfo size=%d",
             share_cache_.stocks_map_.size());
}

void SSEngineImpl::UpdateStockRealMarketData(
    time_t market_time,
    REAL_MARKET_DATA_VEC& stocks_market_data) {

  {
    base_logic::WLockGd lk(lock_);
    int total_count = 0;
    strade_logic::StockTotalInfo stock_total_info;
    REAL_MARKET_DATA_VEC::const_iterator iter(stocks_market_data.begin());
    for (; iter != stocks_market_data.end(); ++iter) {
      const strade_logic::StockRealInfo& stock_real_info = *iter;
      const std::string& stock_code = stock_real_info.code;
      if (!GetStockTotalNonBlock(stock_code, stock_total_info)) {
        LOG_ERROR2("UpdateStockRealMarketData stock_code=%s, not exists!!!!",
                   stock_code.c_str());
        continue;
      }
      stock_total_info.AddStockRealInfoByTime(market_time, stock_real_info);
      ++total_count;
    }
    market_time_ = market_time;
    LOG_DEBUG2("total_count=%d, current_time=%d",
               total_count, market_time);
  }

  // 通知所有需要实时行情数据的观察者
  this->Notify(strade_logic::REALTIME_MARKET_VALUE_UPDATE, (void*)this);
}

bool SSEngineImpl::UpdateStockHistInfoByDate(const std::string& stock_code,
                                             const std::string& date,
                                             strade_logic::StockHistInfo& stock_hist_info) {
  base_logic::WLockGd lk(lock_);
  strade_logic::StockTotalInfo stock_total_info;
  if (!GetStockTotalNonBlock(stock_code, stock_total_info)) {
    return false;
  }
  return stock_total_info.AddStockHistInfoByDate(date, stock_hist_info);
}

bool SSEngineImpl::ClearOldRealTradeMap() {
  base_logic::WLockGd lk(lock_);
  STOCKS_MAP::iterator iter(share_cache_.stocks_map_.begin());
  for (; iter != share_cache_.stocks_map_.end(); ++iter) {
    iter->second.ClearRealMap();
  }
  return true;
}

bool SSEngineImpl::UpdateStockHistDataVec(
    const std::string& stock_code,
    STOCK_HIST_DATA_VEC& stock_vec) {
  base_logic::WLockGd lk(lock_);
  strade_logic::StockTotalInfo stock_total_info;
  if (!GetStockTotalNonBlock(stock_code, stock_total_info)) {
    return false;
  }
  stock_total_info.AddStockHistVec(stock_vec);
  return true;
}

bool SSEngineImpl::AddStockTotalInfoNonblock(
    const strade_logic::StockTotalInfo& stock_total_info) {
  const std::string& stock_code = stock_total_info.get_stock_code();
  share_cache_.stocks_map_[stock_code] = stock_total_info;
  return true;
}

bool SSEngineImpl::AddStockTotalInfoBlock(
    const strade_logic::StockTotalInfo& stock_total_info) {
  base_logic::WLockGd lk(lock_);
  AddStockTotalInfoNonblock(stock_total_info);
  return true;
}

STOCKS_MAP SSEngineImpl::GetAllStockTotalMapCopy() {
  base_logic::RLockGd lk(lock_);
  return share_cache_.stocks_map_;
}

STOCK_HIST_MAP SSEngineImpl::GetStockHistMapByCodeCopy(
    const std::string& stock_code) {
  base_logic::RLockGd lk(lock_);
  strade_logic::StockTotalInfo stock_total_info;
  if (!GetStockTotalNonBlock(stock_code, stock_total_info)) {
    return STOCK_HIST_MAP();
  }
  return stock_total_info.GetStockHistMap();
}

STOCK_REAL_MAP SSEngineImpl::GetStockRealInfoMapCopy(
    const std::string& stock_code) {
  base_logic::RLockGd lk(lock_);
  strade_logic::StockTotalInfo stock_total_info;
  if (!GetStockTotalNonBlock(stock_code, stock_total_info)) {
    return STOCK_REAL_MAP();
  }
  return stock_total_info.GetStockRealMap();
}

bool SSEngineImpl::GetStockTotalInfoByCode(
    const std::string& stock_code,
    strade_logic::StockTotalInfo& stock_total_info) {
  base_logic::RLockGd lk(lock_);
  return GetStockTotalNonBlock(stock_code, stock_total_info);
}

bool SSEngineImpl::GetStockHistInfoByDate(
    const std::string& stock_code,
    const std::string& date,
    strade_logic::StockHistInfo& stock_hist_info) {
  base_logic::WLockGd lk(lock_);
  strade_logic::StockTotalInfo stock_total_info;
  if (!GetStockTotalNonBlock(stock_code, stock_total_info)) {
    return false;
  }
  return stock_total_info.GetStockHistInfoByDate(date, stock_hist_info);
}

bool SSEngineImpl::GetStockRealMarketDataByTime(
    const std::string& stock_code,
    const time_t& time,
    strade_logic::StockRealInfo& stock_real_info) {
  base_logic::WLockGd lk(lock_);
  strade_logic::StockTotalInfo stock_total_info;
  if (!GetStockTotalNonBlock(stock_code, stock_total_info)) {
    return false;
  }
  return stock_total_info.GetStockRealInfoByTradeTime(time, stock_real_info);
}

bool SSEngineImpl::GetStockCurrRealMarketInfo(
    const std::string& stock_code,
    strade_logic::StockRealInfo& stock_real_info) {
  base_logic::WLockGd lk(lock_);
  strade_logic::StockTotalInfo stock_total_info;
  if (!GetStockTotalNonBlock(stock_code, stock_total_info)) {
    return false;
  }
  return stock_total_info.GetCurrRealMarketInfo(stock_real_info);
}

bool SSEngineImpl::ReadDataRows(
    int column_num, const std::string& sql, MYSQL_ROWS_VEC& rows_vec) {
  return strade_share_db_->ReadDataRows(column_num, sql, rows_vec);
}

bool SSEngineImpl::WriteData(const std::string& sql) {
  return strade_share_db_->WriteData(sql);
}

bool SSEngineImpl::ExcuteStorage(
    int column_num, const std::string& sql, MYSQL_ROWS_VEC& rows_vec) {
  return strade_share_db_->ExcuteStorage(column_num, sql, rows_vec);
}

bool SSEngineImpl::AddMysqlAsyncJob(int column_num,
                                    const std::string& sql,
                                    MysqlCallback callback,
                                    base_logic::MYSQL_JOB_TYPE type,
                                    void* param) {
  return strade_share_db_->AddAsyncMysqlJob(column_num, sql, callback, type, param);
}

bool SSEngineImpl::OnCloseMarket() {
  user_engine_->OnCloseMarket();
  return true;
}

} /* namespace strade_share */
