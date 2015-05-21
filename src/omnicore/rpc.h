#ifndef OMNICORE_RPC_H
#define OMNICORE_RPC_H

#include "json/json_spirit_value.h"

#include <string>

class uint256;

int populateRPCTransactionObject(const uint256& txid, json_spirit::Object* txobj, std::string filterAddress = "", bool extendedDetails = false, std::string extendedDetailsFilter = "");


#endif // OMNICORE_RPC_H
