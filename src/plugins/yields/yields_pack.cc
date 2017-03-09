//Copyright (c) 2016-2017 The strade Authors. All rights reserved.
//Created on: 2017/01/09 Author: tangtao

#include "yields/yields_pack.h"
#include "yields/yields_info.h"

namespace yields {

SendYieldsPack::SendYieldsPack() {
  Init();
}

void SendYieldsPack::Init() {
  yields_list_pt_.reset(new base_logic::ListValue);
}

NetBase* SendYieldsPack::release() {
 this->set_status(1);
 head_->Set("result", yields_list_pt_.release());

 return head_.get();
}

void SendYieldsPack::AddYieldsInfo(NetBase *nb) {
  if (NULL != nb) { 
    yields_list_pt_->Append(nb);
  }
}

void SendYieldsPack::GroupAccountInfoToSendPack(GroupAccountInfo* group_account_info_pt) {
  do {
    NetBase *nb_pt = new NetBase;
    if (NULL == nb_pt) {
      break;
    }

    nb_pt->SetBigInteger("group_id", group_account_info_pt->get_group_id());
    nb_pt->SetReal("total_assets", group_account_info_pt->get_total_assets());
    nb_pt->SetReal("yields_of_day", group_account_info_pt->get_yields_of_day());
    nb_pt->SetReal("total_yields", group_account_info_pt->get_total_yields());
    nb_pt->SetReal("usable_assets", group_account_info_pt->get_usable_assets());
    nb_pt->SetReal("profit_or_loss", group_account_info_pt->get_profit_or_loss());
    nb_pt->SetReal("stock_value", group_account_info_pt->get_stock_value());
    nb_pt->SetReal("holding_yields", group_account_info_pt->get_holding_yields());

    this->AddYieldsInfo(nb_pt);
  } while(0);
}

void SendYieldsPack::YieldsHistoryToSendPack(std::vector<YieldsHistoryInfo*> *vec_yields_history_pt) {
  for (std::vector<YieldsHistoryInfo*>::iterator it = \
        vec_yields_history_pt->begin(); \
        it != vec_yields_history_pt->end(); ++it) {
    NetBase *nb_pt = new NetBase;
    if (NULL == nb_pt) {
      continue;
    }

    YieldsHistoryInfo *yields_history_info_pt = *it;
    nb_pt->SetBigInteger("group_id", yields_history_info_pt->get_group_id());
    nb_pt->SetString("group_name", yields_history_info_pt->get_group_name());
    nb_pt->SetString("date", yields_history_info_pt->get_date());
    nb_pt->SetReal("yields", yields_history_info_pt->get_yields());

    this->AddYieldsInfo(nb_pt);

    delete *it;
    *it = NULL;
  }
}

}  // namespace yields