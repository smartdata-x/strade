/*
 * realinfo_pro.h
 *
 *  Created on: 2017年1月12日
 *      Author: pangh
 */

#ifndef REALINFO_REALINFO_PRO_H_
#define REALINFO_REALINFO_PRO_H_
#include <list>
#include <string>
#include "net/http_data.h"

namespace realinfo {

enum RealInfoRequestInterval {
  REALINFO_REQUEST_MONTH,
  REALINFO_REQUEST_WEEK,
  REALINFO_REQUEST_DAY
};

struct StockDealInfo {
  double price;
  double vol;
};

struct StockRealInfo {
  double now_price;
  double open_price;
  double avg_price;
  double yesterday_close_price;
  double high_price;
  double low_price;
  double limit_up;
  double limit_down;
  double turnover_rate;
  double change_rate;
  double change_price;
  double  amount;  
  double vol;
  std::string name;
  std::string time;
  time_t ts;
};

struct StockDealNInfo {
  std::list<StockDealInfo> buy;
  std::list<StockDealInfo> sell;
};

struct MarketIndexInfo {
  double sh_index;
  double sz_index;
  double hs_index;
};

class SendRealInfoLatestProtocol {
 public:
  explicit SendRealInfoLatestProtocol() {
    result_.reset(new NetBase());
  }
  ~SendRealInfoLatestProtocol() {
    DeleteGroup(L"sell", result_.get());
    DeleteGroup(L"buy", result_.get());
    DeleteGroup(L"index_info", result_.get());
    DeleteGroup(L"today", result_.get());
  }

  void DeleteGroup(std::wstring key, base_logic::DictionaryValue* root) {
    base_logic::ListValue* listvalue;
    bool r = root->GetList(key, &listvalue);
    if (!r) {
      return;
    }
    while (listvalue->GetSize()) {
      base_logic::Value* result_value;
      listvalue->Remove(0, &result_value);
      base_logic::DictionaryValue* dvalue =
          static_cast<base_logic::DictionaryValue*>(result_value);
      delete result_value;
    }

  }

  void AddStockRealInfo(const StockRealInfo &real_info) {
    base_logic::DictionaryValue* info_value =
        new base_logic::DictionaryValue();
    info_value->SetReal(L"now_price", GetSendPrice(real_info.now_price));
    info_value->SetReal(L"open_price", GetSendPrice(real_info.open_price));
    info_value->SetReal(L"yesterday_close_price", GetSendPrice(real_info.yesterday_close_price));
    info_value->SetReal(L"high_price", GetSendPrice(real_info.high_price));
    info_value->SetReal(L"low_price", GetSendPrice(real_info.low_price));
    info_value->SetReal(L"limit_up", GetSendPrice(real_info.limit_up));
    info_value->SetReal(L"limit_down", GetSendPrice(real_info.limit_down));
    info_value->SetReal(L"turnover_rate", real_info.turnover_rate);
    info_value->SetReal(L"vol", real_info.vol);
    result_->Set(L"real_info", info_value); 
  }

  void AddGroup(std::wstring group_name,
                const std::list<StockDealInfo> &result,
                base_logic::DictionaryValue* root) {
    std::list<StockDealInfo>::const_iterator it = result.begin();
    base_logic::ListValue* list = new base_logic::ListValue();
    while (it != result.end()) {
      base_logic::DictionaryValue* info_value =
          new base_logic::DictionaryValue();
      info_value->SetReal(L"price", GetSendPrice(it->price));
      info_value->SetReal(L"vol", it->vol);
      list->Append((base_logic::Value*) (info_value));
      it++;
    }

    root->Set(group_name, list);
  }
  
  void AddTodayRealInfoGroup(std::wstring group_name,
                             const std::list<StockRealInfo> &result,
                             base_logic::DictionaryValue* root) {

    std::list<StockRealInfo>::const_iterator it = result.begin();
    base_logic::ListValue* list = new base_logic::ListValue();
    while (it != result.end()) {
      base_logic::DictionaryValue* info_value =
          new base_logic::DictionaryValue();
      info_value->SetString(L"time", it->time);
      info_value->SetReal(L"price", GetSendPrice(it->now_price));
      info_value->SetReal(L"avg_price", GetSendPrice(it->avg_price));
      info_value->SetReal(L"vol", GetSendPrice(it->vol));
      list->Append((base_logic::Value*) (info_value));
      it++;
    }
    root->Set(group_name, list);
}

  void AddIndexGroup(std::wstring group_name,
                     const std::list<StockRealInfo> &result,
                     base_logic::DictionaryValue* root) {
    std::list<StockRealInfo>::const_iterator it = result.begin();
    base_logic::ListValue* list = new base_logic::ListValue();
    while (it != result.end()) {
      base_logic::DictionaryValue* info_value =
          new base_logic::DictionaryValue();
      info_value->SetReal(L"price", GetSendPrice(it->now_price));
      info_value->SetString(L"name", it->name);
      info_value->SetReal(L"up_percent", GetSendPrice(it->change_rate));
      info_value->SetReal(L"up_price", GetSendPrice(it->change_price));
      list->Append((base_logic::Value*) (info_value));
      it++;
    }

    root->Set(group_name, list);
  }

  std::string GetJson() {
    std::string json;
    scoped_ptr <base_logic::ValueSerializer> serializer(
        base_logic::ValueSerializer::Create(base_logic::IMPL_JSONP));
    bool r = serializer->Serialize(*result_, &json);
    if (!r)
      return "";
    return json;
  }

  void set_latest_info(const StockDealNInfo &deal_info, const StockRealInfo &real_info) {
    deal_info_ = deal_info;
    real_info_ = real_info;
    now_price_ = real_info.now_price;
    AddGroup(L"sell", deal_info_.sell, result_.get());
    AddGroup(L"buy",  deal_info_.buy, result_.get());
    AddStockRealInfo(real_info_);
    SetCommonInfo(1);
  }
  
  void SetIndexInfo(const std::list<StockRealInfo> &info_list) {
    AddIndexGroup(L"index_info", info_list, result_.get());
    SetCommonInfo(1);
  }

  void SetTodayRealInfo(const std::list<StockRealInfo> &info_list) {
    AddTodayRealInfoGroup(L"today", info_list, result_.get());
    SetCommonInfo(1);
  }

  void SetCommonInfo(int status) {
    result_->SetInteger(L"status", status);
    result_->SetBigInteger(L"timestamp", time(NULL));
  }
  
  void SetErrorState(int status, std::string error) {
    SetCommonInfo(status);
    result_->SetString(L"error", error);
  }  

  double GetSendPrice(double price) {
    return price;
  }

 private:
  scoped_ptr<NetBase> result_;
  StockDealNInfo  deal_info_;
  StockRealInfo   real_info_;
  double now_price_;
};

}  // namespace realinfo

#endif /* REALINFO_REALINFO_PRO_H_ */
