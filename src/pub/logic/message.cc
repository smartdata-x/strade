//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2017/1/9 Author: zjc

#include "message.h"

#include <stdlib.h>
#include <sstream>

#include "logic/base_values.h"
#include "logic/logic_comm.h"
#include "basic/basic_util.h"

#define OSS_WRITE(x)        \
  oss << "\t\t" << #x << " = " << x << std::endl

namespace strade_user {

bool ReqHead::StartDeserialize(DictionaryValue& dict) {
  return ReqHead::Deserialize(dict) && Deserialize(dict);
}

bool ReqHead::Deserialize(DictionaryValue& dict) {
  int64 t;
  if (!dict.GetBigInteger(L"type", &t)) {
    LOG_ERROR("NOT FIND type");
    return false;
  }
  type = t;

  if (!dict.GetBigInteger(L"opcode", &t)) {
    LOG_ERROR("NOT FIND opcode");
    return false;
  }
  opcode = t;

  if (!dict.GetBigInteger(L"user_id", &t)) {
    LOG_ERROR("NOT FIND user_id");
    return false;
  }
  user_id = t;

  if (!dict.GetString(L"token", &token)) {
    LOG_ERROR("NOT FIND token");
    return false;
  }
  return true;
}

void ReqHead::StartDump(std::ostringstream& oss) {
  ReqHead::Dump(oss);
  Dump(oss);
}

void ReqHead::Dump(std::ostringstream& oss) {
  oss << "\n\t\t--------- Head ---------" << std::endl;
  OSS_WRITE(opcode);
  OSS_WRITE(user_id);
  OSS_WRITE(token);
}

///////////////////////////////////////////////////////////////////////////////
bool ResHead::StartSerialize(DictionaryValue& dict) {
  return ResHead::Serialize(dict) && Serialize(dict);
}

bool ResHead::Serialize(DictionaryValue& dict) {
  return status.Serialize(dict);
}
///////////////////////////////////////////////////////////////////////////////
bool CreateGroupReq::Deserialize(DictionaryValue& dict) {
  std::string str;
  if (!dict.GetString(L"group_name", &str)) {
    LOG_ERROR("NOT FIND group_name");
    return false;
  }

  if (!base::BasicUtil::UrlDecode(str, group_name)) {
    LOG_ERROR("group_name Decode error");
    return false;
  }

  if (!dict.GetString(L"code_list", &str)) {
    LOG_ERROR("NOT FIND code_list");
    return false;
  }

  std::string code;
  std::istringstream iss(str);
  while (std::getline(iss, code, ',')) {
    if (!code.empty()) {
      code_list.push_back(code);
    }
  }
  return true;
}

void CreateGroupReq::Dump(std::ostringstream& oss) {
  oss << "\t\t--------- CreateGroupReq ---------" << std::endl;
  OSS_WRITE(group_name);
  oss << "\t\tcode_list = ";
  for (size_t i = 0; i < code_list.size(); ++i) {
    oss << code_list[i] << ",";
  }
}

bool CreateGroupRes::Serialize(DictionaryValue& dict) {
  dict.SetBigInteger(L"group_id", group_id);
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool AddStockReq::Deserialize(DictionaryValue& dict) {
  int64 t;
  if (!dict.GetBigInteger(L"group_id", &t)) {
    LOG_ERROR("NOT FIND group_id");
    return false;
  }
  group_id = t;

  std::string str;
  if (!dict.GetString(L"code_list", &str)) {
    LOG_ERROR("NOT FIND code_list");
    return false;
  }

  std::string code;
  std::istringstream iss(str);
  while (std::getline(iss, code, ',')) {
    if (!code.empty()) {
      code_list.push_back(code);
    }
  }
  return true;
}

void AddStockReq::Dump(std::ostringstream& oss) {
  oss << "\t\t--------- AddStockReq ---------" << std::endl;
  OSS_WRITE(group_id);
  oss << "\t\tcode_list = ";
  for (size_t i = 0; i < code_list.size(); ++i) {
    oss << code_list[i] << ",";
  }
}
///////////////////////////////////////////////////////////////////////////////
bool DelStockReq::Deserialize(DictionaryValue& dict) {
  int64 t;
  if (!dict.GetBigInteger(L"group_id", &t)) {
    LOG_ERROR("NOT FIND group_id");
    return false;
  }
  group_id = t;

  std::string str;
  if (!dict.GetString(L"code_list", &str)) {
    LOG_ERROR("NOT FIND code_list");
    return false;
  }

  std::string code;
  std::istringstream iss(str);
  while (std::getline(iss, code, ',')) {
    if (!code.empty()) {
      code_list.push_back(code);
    }
  }
  return true;
}

void DelStockReq::Dump(std::ostringstream& oss) {
  oss << "\t\t--------- DelStockReq ---------" << std::endl;
  OSS_WRITE(group_id);
  oss << "\t\tcode_list = ";
  for (size_t i = 0; i < code_list.size(); ++i) {
    oss << code_list[i] << ",";
  }
}
///////////////////////////////////////////////////////////////////////////////
bool QueryGroupReq::Deserialize(DictionaryValue& dict) {
  return true;
}

void QueryGroupReq::Dump(std::ostringstream& oss) {
  oss << "\t\t--------- QueryGroupReq ---------" << std::endl;
}

bool QueryGroupsRes::GroupInfo::Serialize(DictionaryValue& dict) {
  dict.SetBigInteger(L"id", id);
  dict.SetString(L"name", name);
  return true;
}

bool QueryGroupsRes::Serialize(DictionaryValue& dict) {
  ListValue* unit_list = new ListValue();
  for (size_t i = 0; i < group_list.size(); ++i) {
    DictionaryValue* unit = new DictionaryValue();
    group_list[i].Serialize(*unit);
    unit_list->Append(unit);
  }
  dict.Set(L"group_list", unit_list);
  return true;
}
///////////////////////////////////////////////////////////////////////////////
bool QueryStocksReq::Deserialize(DictionaryValue& dict) {
  int64 t;
  if (!dict.GetBigInteger(L"group_id", &t)) {
    LOG_ERROR("NOT FIND group_id");
    return false;
  }
  group_id = t;
  return true;
}

void QueryStocksReq::Dump(std::ostringstream& oss) {
  oss << "\t\t--------- QueryStocksReq ---------" << std::endl;
  OSS_WRITE(group_id);
}

bool QueryStocksRes::StockInfo::Serialize(DictionaryValue& dict) {
  dict.SetString(L"code", code);
  dict.SetString(L"name", name);
  dict.SetBigInteger(L"visit_heat", visit_heat);
  dict.SetReal(L"price", price);
  dict.SetReal(L"change", change);
  dict.SetBigInteger(L"volume", volume);
  dict.SetString(L"industry", industry);
  return true;
}

bool QueryStocksRes::Serialize(DictionaryValue& dict) {
  ListValue* unit_list = new ListValue();
  for (size_t i = 0; i < stock_list.size(); ++i) {
    DictionaryValue* unit = new DictionaryValue();
    stock_list[i].Serialize(*unit);
    unit_list->Append(unit);
  }
  dict.Set(L"stock_list", unit_list);
  return true;
}
///////////////////////////////////////////////////////////////////////////////
bool QueryHoldingStocksReq::Deserialize(DictionaryValue& dict) {
  int64 t;
  if (!dict.GetBigInteger(L"group_id", &t)) {
    LOG_ERROR("NOT FIND group_id");
    return false;
  }
  group_id = t;
  return true;
}

void QueryHoldingStocksReq::Dump(std::ostringstream& oss) {
  oss << "\t\t--------- QueryHoldingStocksReq ---------" << std::endl;
  OSS_WRITE(group_id);
}

bool QueryHoldingStocksRes::StockInfo::Serialize(DictionaryValue& dict) {
  dict.SetString(L"code", code);
  dict.SetBigInteger(L"holding", holding);
  dict.SetBigInteger(L"available", available);
  dict.SetReal(L"cost", cost);
  dict.SetReal(L"price", price);
  dict.SetReal(L"market_value", market_value);
  dict.SetReal(L"profit", profit);
  dict.SetReal(L"profit_ratio", profit_ratio);
  dict.SetReal(L"position", position);
  return true;
}

bool QueryHoldingStocksRes::Serialize(DictionaryValue& dict) {
  ListValue* unit_list = new ListValue();
  for (size_t i = 0; i < stock_list.size(); ++i) {
    DictionaryValue* unit = new DictionaryValue();
    stock_list[i].Serialize(*unit);
    unit_list->Append(unit);
  }
  dict.Set(L"stock_list", unit_list);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool QueryTodayOrdersReq::Deserialize(DictionaryValue& dict) {
  int64 t;
  if (!dict.GetBigInteger(L"group_id", &t)) {
    LOG_ERROR("NOT FIND gruop_id");
    return false;
  }
  group_id = t;
  return true;
}

void QueryTodayOrdersReq::Dump(std::ostringstream& oss) {
  oss << "\t\t--------- QueryTodayOrdersReq ---------" << std::endl;
  OSS_WRITE(group_id);
}

bool QueryTodayOrdersRes::OrderInfo::Serialize(DictionaryValue& dict) {
  dict.SetBigInteger(L"id", id);
  dict.SetString(L"code", code);
  dict.SetString(L"name", name);
  dict.SetBigInteger(L"order_operation", op);
  dict.SetReal(L"order_price", order_price);
  dict.SetBigInteger(L"order_nums", order_nums);
  dict.SetBigInteger(L"order_time", order_time);
  dict.SetBigInteger(L"status", status);
  return true;
}

bool QueryTodayOrdersRes::Serialize(DictionaryValue& dict) {
  ListValue* unit_list = new ListValue();
  for (size_t i = 0; i < order_list.size(); ++i) {
    DictionaryValue* unit = new DictionaryValue();
    order_list[i].Serialize(*unit);
    unit_list->Append(unit);
  }
  dict.Set(L"order_list", unit_list);
  return true;
}
///////////////////////////////////////////////////////////////////////////////
bool QueryTodayFinishedOrdersReq::Deserialize(DictionaryValue& dict) {
  int64 t;
  if (!dict.GetBigInteger(L"group_id", &t)) {
    LOG_ERROR("NOT FIND gruop_id");
    return false;
  }
  group_id = t;
  return true;
}

void QueryTodayFinishedOrdersReq::Dump(std::ostringstream& oss) {
  oss << "\t\t--------- QueryTodayFinishedOrdersReq ---------" << std::endl;
  OSS_WRITE(group_id);
}

bool QueryTodayFinishedOrdersRes::OrderInfo::Serialize(DictionaryValue& dict) {
  dict.SetString(L"code", code);
  dict.SetBigInteger(L"order_operation", op);
  dict.SetReal(L"order_price", order_price);
  dict.SetBigInteger(L"order_nums", order_nums);
  dict.SetReal(L"amount", amount);
  dict.SetBigInteger(L"order_time", order_time);
  return true;
}

bool QueryTodayFinishedOrdersRes::Serialize(DictionaryValue& dict) {
  ListValue* unit_list = new ListValue();
  for (size_t i = 0; i < order_list.size(); ++i) {
    DictionaryValue* unit = new DictionaryValue();
    order_list[i].Serialize(*unit);
    unit_list->Append(unit);
  }
  dict.Set(L"order_list", unit_list);
  return true;
}
///////////////////////////////////////////////////////////////////////////////
bool QueryHistoryFinishedOrdersReq::Deserialize(DictionaryValue& dict) {
  if (!dict.GetString(L"begin_time", &begin_time)) {
    LOG_ERROR("NOT FIND begin_time");
    return false;
  }
  if (!dict.GetString(L"end_time", &end_time)) {
    LOG_ERROR("NOT FIND end_time");
    return false;
  }
  int64 t;
  if (!dict.GetBigInteger(L"group_id", &t)) {
    LOG_ERROR("NOT FIND group_id");
    return false;
  }
  group_id = t;
  return true;
}

void QueryHistoryFinishedOrdersReq::Dump(std::ostringstream& oss) {
  oss << "\t\t--------- QueryHistoryFinishedOrdersReq ---------" << std::endl;
  OSS_WRITE(begin_time);
  OSS_WRITE(end_time);
  OSS_WRITE(group_id);
}

bool QueryHistoryFinishedOrdersRes::OrderInfo::Serialize(DictionaryValue& dict) {
  dict.SetString(L"code", code);
  dict.SetBigInteger(L"order_operation", op);
  dict.SetReal(L"order_price", order_price);
  dict.SetBigInteger(L"order_nums", order_nums);
  dict.SetReal(L"amount", amount);
  dict.SetBigInteger(L"order_time", order_time);
  return true;
}

bool QueryHistoryFinishedOrdersRes::Serialize(DictionaryValue& dict) {
  ListValue* unit_list = new ListValue();
  for (size_t i = 0; i < order_list.size(); ++i) {
    DictionaryValue* unit = new DictionaryValue();
    order_list[i].Serialize(*unit);
    unit_list->Append(unit);
  }
  dict.Set(L"order_list", unit_list);
  return true;
}
///////////////////////////////////////////////////////////////////////////////
bool QueryStatementReq::Deserialize(DictionaryValue& dict) {
  if (!dict.GetString(L"begin_time", &begin_time)) {
    LOG_ERROR("NOT FIND begin_time");
    return false;
  }

  if (!dict.GetString(L"end_time", &end_time)) {
    LOG_ERROR("NOT FIND end_time");
    return false;
  }

  int64 t;
  if (!dict.GetBigInteger(L"group_id", &t)) {
    LOG_ERROR("NOT FIND group_id");
    return false;
  }
  group_id = t;
  return true;
}

void QueryStatementReq::Dump(std::ostringstream& oss) {
  oss << "\t\t--------- QueryStatementReq ---------" << std::endl;
  OSS_WRITE(group_id);
  OSS_WRITE(begin_time);
  OSS_WRITE(end_time);
}

bool QueryStatementRes::StatementRecord::Serialize(DictionaryValue& dict) {
  dict.SetString(L"code", code);
  dict.SetBigInteger(L"order_operation", op);
  dict.SetReal(L"order_price", order_price);
  dict.SetBigInteger(L"order_nums", order_nums);
  dict.SetReal(L"commission", commission);
  dict.SetReal(L"stamp_duty", stamp_duty);
  dict.SetReal(L"transfer_fee", transfer_fee);
  dict.SetReal(L"amount", amount);
  dict.SetReal(L"available_capital", available_capital);
  return true;
}

bool QueryStatementRes::Serialize(DictionaryValue& dict) {
  ListValue* unit_list = new ListValue();
  for (size_t i = 0; i < statement_list.size(); ++i) {
    DictionaryValue* unit = new DictionaryValue();
    statement_list[i].Serialize(*unit);
    unit_list->Append(unit);
  }
  dict.Set(L"statement_list", unit_list);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool SubmitOrderReq::Deserialize(DictionaryValue& dict) {
  int64 t;
  std::string str;
  if (!dict.GetBigInteger(L"group_id", &t)) {
    LOG_ERROR("NOT FIND group_id");
    return false;
  }
  group_id = t;

  if (!dict.GetString(L"code", &code)) {
    LOG_ERROR("NOT FIND code");
    return false;
  }
  code.erase(code.size()-1);

  if (!dict.GetReal(L"order_price", &order_price)) {
    LOG_ERROR("NOT FIND order_price");
    return false;
  }

  if (!dict.GetReal(L"expected_price", &expected_price)) {
    LOG_ERROR("NOT FIND expected_price");
    return false;
  }

  if (!dict.GetBigInteger(L"order_nums", &t)) {
    LOG_ERROR("NOT FIND order_nums");
    return false;
  }
  order_nums = t;

  if (!dict.GetBigInteger(L"order_operation", &t)) {
    LOG_ERROR("NOT FIND order_operation");
    return false;
  }
  op = (OrderOperation)t;
  return true;
}

void SubmitOrderReq::Dump(std::ostringstream& oss) {
  oss << "\t\t--------- SubmitOrderReq ---------" << std::endl;
  OSS_WRITE(group_id);
  OSS_WRITE(code);
  OSS_WRITE(order_price);
  OSS_WRITE(expected_price);
  OSS_WRITE(order_nums);
  OSS_WRITE((int)op);
}

bool SubmitOrderRes::Serialize(DictionaryValue& dict) {
  dict.SetBigInteger(L"order_id", order_id);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool GroupStockHoldingReq::Deserialize(DictionaryValue& dict) {
  int64 t;
  if (!dict.GetBigInteger(L"group_id", &t)) {
    LOG_ERROR("NOT FIND group_id");
    return false;
  }
  group_id = t;

  return true;
}

void GroupStockHoldingReq::Dump(std::ostringstream& oss) {
  oss << "\t\t--------- GroupStockHoldingReq ---------" << std::endl;
  OSS_WRITE(group_id);
}

bool GroupStockHoldingRes::StockInfo::Serialize(DictionaryValue& dict) {
  dict.SetString(L"code", code);
  dict.SetString(L"name", name);
  dict.SetBigInteger(L"holding", holding);
  return true;
}

bool GroupStockHoldingRes::Serialize(DictionaryValue& dict) {
  ListValue* unit_list = new ListValue();
  for (size_t i = 0; i < stock_list.size(); ++i) {
    DictionaryValue* unit = new DictionaryValue();
    stock_list[i].Serialize(*unit);
    unit_list->Append(unit);
  }
  dict.Set(L"stock_list", unit_list);
  return true;
}
///////////////////////////////////////////////////////////////////////////////
bool AvailableStockCountReq::Deserialize(DictionaryValue& dict) {
  int64 t;
  if (!dict.GetBigInteger(L"group_id", &t)) {
    LOG_ERROR("NOT FIND group_id");
    return false;
  }
  group_id = t;

  if (!dict.GetString(L"code", &code)) {
    LOG_ERROR("NOT FIND code");
    return false;
  }

  // erase last ','
  code.erase(code.size()-1);
  return true;
}

void AvailableStockCountReq::Dump(std::ostringstream& oss) {
  oss << "\t\t--------- AvailableStockCountReq ---------" << std::endl;
  OSS_WRITE(group_id);
  OSS_WRITE(code);
}

bool AvailableStockCountRes::Serialize(DictionaryValue& dict) {
  dict.SetString(L"code", code);
  dict.SetString(L"name", name);
  dict.SetBigInteger(L"count", count);
  return true;
}
///////////////////////////////////////////////////////////////////////////////
bool CancelOrderReq::Deserialize(DictionaryValue& dict) {
  int64 t;
  if (!dict.GetBigInteger(L"order_id", &t)) {
    LOG_ERROR("NOT FIND order_id");
    return false;
  }
  order_id = t;
  return true;
}

void CancelOrderReq::Dump(std::ostringstream& oss) {
  oss << "\t\t--------- CancelOrder ---------" << std::endl;
  OSS_WRITE(order_id);
}

///////////////////////////////////////////////////////////////////////////////
bool ProfitAndLossOrderNumReq::Deserialize(DictionaryValue& dict) {
  int64 t;
  if (!dict.GetBigInteger(L"group_id", &t)) {
    LOG_ERROR("NOT FIND group_id");
    return false;
  }
  group_id = t;
  return true;
}

void ProfitAndLossOrderNumReq::Dump(std::ostringstream& oss) {
  oss << "\t\t--------- ProfitAndLossOrderNumReq ---------" << std::endl;
  OSS_WRITE(group_id);
}

bool ProfitAndLossOrderNumRes::Serialize(DictionaryValue& dict) {
  dict.SetBigInteger(L"profit_num", profit_num);
  dict.SetBigInteger(L"loss_num", loss_num);
  return true;
}
///////////////////////////////////////////////////////////////////////////////

bool ModifyInitCapitalReq::Deserialize(DictionaryValue& dict) {
  int64 t;
  std::string str;
  if (!dict.GetBigInteger(L"group_id", &t)) {
    LOG_ERROR("NOT FIND group_id");
    return false;
  }
  group_id = t;

  if (!dict.GetReal(L"capital", &capital)) {
    LOG_ERROR("NOT FIND capital");
    return false;
  }
  return true;
}

void ModifyInitCapitalReq::Dump(std::ostringstream& oss) {
  oss << "\t\t--------- AddInitCapital ---------" << std::endl;
  OSS_WRITE(group_id);
  OSS_WRITE(capital);
}

bool ModifyInitCapitalRes::Serialize(DictionaryValue& dict) {
  dict.SetReal(L"capital", capital);
  return true;
}
///////////////////////////////////////////////////////////////////////////////

std::string Status::to_string() {
  switch (state) {
    case SUCCESS:
      return "success";
    case FAILED:
      return "failed";
    case ERROR_MSG:
      return "error msg";
    case UNKNOWN_OPCODE:
      return "UNKNOWN opcode";
    case USER_NOT_EXIST:
      return "user not exist";
    case INVALID_TOKEN:
      return "invalid token";
    case GROUP_NAME_ALREADAY_EXIST:
      return "group name already exist";
    case MYSQL_ERROR:
      return "mysql error";
    case GROUP_NOT_EXIST:
      return "group not exist";
    case STOCK_NOT_IN_GROUP:
      return "stock not in group";
    case STOCK_NOT_EXIST:
      return "stock not exit";
    case CAPITAL_NOT_ENOUGH:
      return "capital not enough";
    case NO_HOLDING_STOCK:
      return "no holding stock";
    case ORDER_NOT_EXIST:
      return "order not exist";
    case NOT_IN_ORDER_TIME:
      return "not in order time";
    default:
      return "UNKNOWN error";
  }
}

bool Status::Serialize(DictionaryValue& dict) {
  int64 t = state;
  dict.SetBigInteger(L"status", t);
  dict.SetString(L"msg", to_string());
  return true;
}

}

