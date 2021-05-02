#ifndef BITCOIN_OMNICORE_RPCTXOBJECT_H
#define BITCOIN_OMNICORE_RPCTXOBJECT_H

#include <univalue.h>

#include <string>

class uint256;
class CMPTransaction;
class CTransaction;

namespace interfaces {
class Wallet;
} // namespace interfaces

int populateRPCTransactionObject(const uint256& txid, UniValue& txobj, std::string filterAddress = "", bool extendedDetails = false, std::string extendedDetailsFilter = "", interfaces::Wallet* iWallet = nullptr);
int populateRPCTransactionObject(const CTransaction& tx, const uint256& blockHash, UniValue& txobj, std::string filterAddress = "", bool extendedDetails = false, std::string extendedDetailsFilter = "", int blockHeight = 0, interfaces::Wallet* iWallet = nullptr);

void populateRPCTypeInfo(CMPTransaction& mp_obj, UniValue& txobj, uint32_t txType, bool extendedDetails, std::string extendedDetailsFilter, int confirmations, interfaces::Wallet* iWallet = nullptr);

void populateRPCTypeSimpleSend(CMPTransaction& omniObj, UniValue& txobj);
void populateRPCTypeSendToOwners(CMPTransaction& omniObj, UniValue& txobj, bool extendedDetails, std::string extendedDetailsFilter, interfaces::Wallet* iWallet = nullptr);
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
void populateRPCTypeSendNonFungible(CMPTransaction& omniObj, UniValue& txobj);
void populateRPCTypeSetNonFungibleData(CMPTransaction& omniObj, UniValue& txobj);
void populateRPCTypeChangeIssuer(CMPTransaction& omniObj, UniValue& txobj);
void populateRPCTypeActivation(CMPTransaction& omniObj, UniValue& txobj);
void populateRPCTypeEnableFreezing(CMPTransaction& omniObj, UniValue& txobj);
void populateRPCTypeDisableFreezing(CMPTransaction& omniObj, UniValue& txobj);
void populateRPCTypeFreezeTokens(CMPTransaction& omniObj, UniValue& txobj);
void populateRPCTypeUnfreezeTokens(CMPTransaction& omniObj, UniValue& txobj);
void populateRPCTypeAnyData(CMPTransaction& omniObj, UniValue& txobj);

void populateRPCExtendedTypeSendToOwners(const uint256 txid, std::string extendedDetailsFilter, UniValue& txobj, uint16_t version, interfaces::Wallet* iWallet = nullptr);
void populateRPCExtendedTypeGrantNonFungible(CMPTransaction& omniObj, UniValue& txobj);
void populateRPCExtendedTypeMetaDExTrade(const uint256& txid, uint32_t propertyIdForSale, int64_t amountForSale, UniValue& txobj);
void populateRPCExtendedTypeMetaDExCancel(const uint256& txid, UniValue& txobj);

int populateRPCDExPurchases(const CTransaction& wtx, UniValue& purchases, std::string filterAddress, interfaces::Wallet* iWallet = nullptr);
int populateRPCSendAllSubSends(const uint256& txid, UniValue& subSends);

bool showRefForTx(uint32_t txType);

#endif // BITCOIN_OMNICORE_RPCTXOBJECT_H
