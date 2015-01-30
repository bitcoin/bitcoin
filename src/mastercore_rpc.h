#ifndef _MASTERCORE_RPC
#define _MASTERCORE_RPC

#include "json/json_spirit_value.h"

#include <string>

class uint256;

int populateRPCTransactionObject(const uint256& txid, json_spirit::Object* txobj, std::string filterAddress);

#endif // #ifndef _MASTERCORE_RPC

