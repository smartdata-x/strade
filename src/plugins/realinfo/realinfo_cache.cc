/*
 * realinfo_cache.cc
 *
 *  Created on: 2017年1月12日
 *      Author: pangh
 */
#include "realinfo/realinfo_cache.h"
#include "dic/base_dic_redis_auto.h"
#include "storage/redis_storage_impl.h"
namespace realinfo {
RealInfoCache::RealInfoCache(std::string pool_key) {
  pool_key_ = pool_key;
}

RealInfoCache::~RealInfoCache() {
}

int RealInfoCache::UpdateRealInfoLatestCache(std::string stock_code, std::string value) {
  int rc = 0;
  base_dic::KunDicPool *pool = base_dic::KunDicPool::GetInstance();
  base_dic::AutoDicCommEngine  dic(pool, pool_key_);
  base_storage::DictionaryStorageEngine*  engine = dic.GetDicEngine();
  if (!engine)
    return -1;
  std::string real_key = GetRealInfoLatestKey(stock_code);
  engine->SetValue(real_key.c_str(), real_key.length(), value.c_str(), value.length());
  static_cast<base_storage::RedisStorageEngineImpl*>(engine)->ExpireValue(real_key.c_str(), 30);
  return rc;
}

int RealInfoCache::UpdateRealInfoTodayCache(std::string stock_code, std::string value) {
  int rc = 0;
  base_dic::KunDicPool *pool = base_dic::KunDicPool::GetInstance();
  base_dic::AutoDicCommEngine  dic(pool, pool_key_);
  base_storage::DictionaryStorageEngine*  engine = dic.GetDicEngine();
  if (!engine)
    return -1;
  std::string real_key = GetRealInfoTodayKey(stock_code);
  engine->SetValue(real_key.c_str(), real_key.length(), value.c_str(), value.length());
  static_cast<base_storage::RedisStorageEngineImpl*>(engine)->ExpireValue(real_key.c_str(), 30);
  return rc;
}

int RealInfoCache::UpdateRealInfoIndexCache(std::string value) {
  int rc = 0;
  base_dic::KunDicPool *pool = base_dic::KunDicPool::GetInstance();
  base_dic::AutoDicCommEngine  dic(pool, pool_key_);
  base_storage::DictionaryStorageEngine*  engine = dic.GetDicEngine();
  if (!engine)
    return -1;
  std::string real_key = GetRealInfoIndexKey();
  engine->SetValue(real_key.c_str(), real_key.length(), value.c_str(), value.length());
  static_cast<base_storage::RedisStorageEngineImpl*>(engine)->ExpireValue(real_key.c_str(), 30);
  return rc;
}

int RealInfoCache::GetRealInfoLatestCache(std::string stock_code, std::string *value) {
  int rc = 0;
  base_dic::KunDicPool *pool = base_dic::KunDicPool::GetInstance();
  base_dic::AutoDicCommEngine  dic(pool, pool_key_);
  base_storage::DictionaryStorageEngine*  engine = dic.GetDicEngine();
  if (!engine)
    return -1;
  char *val = NULL;
  size_t len;
  std::string real_key = GetRealInfoLatestKey(stock_code);
  if (engine->GetValue(real_key.c_str(), real_key.length(), &val, &len)) {
    LOG_MSG2("%s", val);
    *value = val;
    free(val);
    return 0;
  }
  return -1;
}

int RealInfoCache::GetRealInfoTodayCache(std::string stock_code, std::string *value) {
  int rc = 0;
  base_dic::KunDicPool *pool = base_dic::KunDicPool::GetInstance();
  base_dic::AutoDicCommEngine  dic(pool, pool_key_);
  base_storage::DictionaryStorageEngine*  engine = dic.GetDicEngine();
  if (!engine)
    return -1;
  char *val = NULL;
  size_t len;
  std::string real_key = GetRealInfoTodayKey(stock_code);
  if (engine->GetValue(real_key.c_str(), real_key.length(), &val, &len)) {
    LOG_MSG2("%s", val);
    *value = val;
    free(val);
    return 0;
  }
  return -1;
}

int RealInfoCache::GetRealInfoIndexCache(std::string *value) {
  int rc = 0;
  base_dic::KunDicPool *pool = base_dic::KunDicPool::GetInstance();
  base_dic::AutoDicCommEngine  dic(pool, pool_key_);
  base_storage::DictionaryStorageEngine*  engine = dic.GetDicEngine();
  if (!engine)
    return -1;
  char *val = NULL;
  size_t len;
  std::string real_key = GetRealInfoIndexKey();
  if (engine->GetValue(real_key.c_str(), real_key.length(), &val, &len)) {
    LOG_MSG2("%s", val);
    *value = val;
    free(val);
    return 0;
  }
  return -1;
}

}  // namespace realinfo {
