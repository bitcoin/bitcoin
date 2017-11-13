#ifndef OMNICORE_RPCTXOBJECT_H
#define OMNICORE_RPCTXOBJECT_H

#include <univalue.h>

#include <string>

class uint256;
class CMPTransaction;
class CTransaction;

int populateRPCTransactionObject(const uint256& txid, UniValue& txobj, std::string filterAddress = "", bool extendedDetails = false, std::string extendedDetailsFilter = "");
int populateRPCTransactionObject(const CTransaction& tx, const uint256& blockHash, UniValue& txobj, std::string filterAddress = "", bool extendedDetails = false, std::string extendedDetailsFilter = "", int blockHeight = 0);

void populateRPCTypeInfo(CMPTransaction& mp_obj, UniValue& txobj, uint32_t txType, bool extendedDetails, std::string extendedDetailsFilter, int confirmations);

void populateRPCTypeSimpleSend(CMPTransaction& omniObj, UniValue& txobj);
void populateRPCTypeSendToOwners(CMPTransaction& omniObj, UniValue& txobj, bool extendedDetails, std::string extendedDetailsFilter);
void populateRPCTypeSendAll(CMPTransaction& omniObj, UniValue& txobj, int confirmations);
void populateRPCTypeTradeOffer(CMPTransaction& omniObj, UniValue& txobj);
void populateRPCTypeMetaDExTrade(CMPTransaction& omniObj, UniValue& txobj, bool extendedDetails);
void populateRPCTypeMetaDExCancelPrice(CMPTransaction& omniObj, UniValue& txobj, bool extendedDetails);
void populateRPCTypeMetaDExCancelPair(CMPTransaction& omniObj, UniValue& txobj, bool extendedDetails);
void populateRPCTypeMetaDExCancelEcosystem(CMPTransaction& omniObj, UniValue& txobj, bool extendedDetails);
void populateRPCTypeAcceptOffer(CMPTransaction& omniObj, UniValue& txobj);
void populateRPCTypeCreatePropertyFixed(CMPTransaction& omniObj, UniValue& txobj, int confirmations);
void populateRPCTypeCreatePropertyVariable(CMPTransaction& omniObj, UniValue& txobj, int confirmations);
void populateRPCTypeCreatePropertyManual(CMPTransaction& omniObj, UniValue& txobj, int confirmations);
void populateRPCTypeCloseCrowdsale(CMPTransaction& omniObj, UniValue& txobj);
void populateRPCTypeGrant(CMPTransaction& omniObj, UniValue& txobj);
void populateRPCTypeRevoke(CMPTransaction& omniOobj, UniValue& txobj);
void populateRPCTypeChangeIssuer(CMPTransaction& omniObj, UniValue& txobj);
void populateRPCTypeActivation(CMPTransaction& omniObj, UniValue& txobj);

void populateRPCExtendedTypeSendToOwners(const uint256 txid, std::string extendedDetailsFilter, UniValue& txobj, uint16_t version);
void populateRPCExtendedTypeMetaDExTrade(const uint256& txid, uint32_t propertyIdForSale, int64_t amountForSale, UniValue& txobj);
void populateRPCExtendedTypeMetaDExCancel(const uint256& txid, UniValue& txobj);

int populateRPCDExPurchases(const CTransaction& wtx, UniValue& purchases, std::string filterAddress);
int populateRPCSendAllSubSends(const uint256& txid, UniValue& subSends);

bool showRefForTx(uint32_t txType);

#endif // OMNICORE_RPCTXOBJECT_H
