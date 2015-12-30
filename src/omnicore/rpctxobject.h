#ifndef OMNICORE_RPCTXOBJECT_H
#define OMNICORE_RPCTXOBJECT_H

#include "json/json_spirit_value.h"
#include <string>

class uint256;
class CMPTransaction;
class CTransaction;

int populateRPCTransactionObject(const uint256& txid, json_spirit::Object& txobj, std::string filterAddress = "", bool extendedDetails = false, std::string extendedDetailsFilter = "");
int populateRPCTransactionObject(const CTransaction& tx, const uint256& blockHash, json_spirit::Object& txobj, std::string filterAddress = "", bool extendedDetails = false, std::string extendedDetailsFilter = "", int blockHeight = 0);

void populateRPCTypeInfo(CMPTransaction& mp_obj, json_spirit::Object& txobj, uint32_t txType, bool extendedDetails, std::string extendedDetailsFilter);

void populateRPCTypeSimpleSend(CMPTransaction& omniObj, json_spirit::Object& txobj);
void populateRPCTypeSendToOwners(CMPTransaction& omniObj, json_spirit::Object& txobj, bool extendedDetails, std::string extendedDetailsFilter);
void populateRPCTypeSendAll(CMPTransaction& omniObj, json_spirit::Object& txobj);
void populateRPCTypeTradeOffer(CMPTransaction& omniObj, json_spirit::Object& txobj);
void populateRPCTypeMetaDExTrade(CMPTransaction& omniObj, json_spirit::Object& txobj, bool extendedDetails);
void populateRPCTypeMetaDExCancelPrice(CMPTransaction& omniObj, json_spirit::Object& txobj, bool extendedDetails);
void populateRPCTypeMetaDExCancelPair(CMPTransaction& omniObj, json_spirit::Object& txobj, bool extendedDetails);
void populateRPCTypeMetaDExCancelEcosystem(CMPTransaction& omniObj, json_spirit::Object& txobj, bool extendedDetails);
void populateRPCTypeAcceptOffer(CMPTransaction& omniObj, json_spirit::Object& txobj);
void populateRPCTypeCreatePropertyFixed(CMPTransaction& omniObj, json_spirit::Object& txobj);
void populateRPCTypeCreatePropertyVariable(CMPTransaction& omniObj, json_spirit::Object& txobj);
void populateRPCTypeCreatePropertyManual(CMPTransaction& omniObj, json_spirit::Object& txobj);
void populateRPCTypeCloseCrowdsale(CMPTransaction& omniObj, json_spirit::Object& txobj);
void populateRPCTypeGrant(CMPTransaction& omniObj, json_spirit::Object& txobj);
void populateRPCTypeRevoke(CMPTransaction& omniOobj, json_spirit::Object& txobj);
void populateRPCTypeChangeIssuer(CMPTransaction& omniObj, json_spirit::Object& txobj);
void populateRPCTypeActivation(CMPTransaction& omniObj, json_spirit::Object& txobj);

void populateRPCExtendedTypeSendToOwners(const uint256 txid, std::string extendedDetailsFilter, json_spirit::Object& txobj);
void populateRPCExtendedTypeMetaDExTrade(const uint256& txid, uint32_t propertyIdForSale, int64_t amountForSale, json_spirit::Object& txobj);
void populateRPCExtendedTypeMetaDExCancel(const uint256& txid, json_spirit::Object& txobj);

int populateRPCDExPurchases(const CTransaction& wtx, json_spirit::Array& purchases, std::string filterAddress);
int populateRPCSendAllSubSends(const uint256& txid, json_spirit::Array& subSends);

bool showRefForTx(uint32_t txType);

#endif // OMNICORE_RPCTXOBJECT_H
