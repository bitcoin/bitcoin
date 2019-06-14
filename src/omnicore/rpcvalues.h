#ifndef OMNICORE_RPCVALUES_H
#define OMNICORE_RPCVALUES_H

class CPubKey;
class CTransaction;
struct CMutableTransaction;
struct PrevTxsEntry;

#include <univalue.h>

#include <stdint.h>
#include <string>
#include <vector>

std::string ParseAddress(const UniValue& value);
std::string ParseAddressOrEmpty(const UniValue& value);
std::string ParseAddressOrWildcard(const UniValue& value);
uint32_t ParsePropertyId(const UniValue& value);
int64_t ParseAmount(const UniValue& value, bool isDivisible);
int64_t ParseAmount(const UniValue& value, int propertyType);
uint8_t ParseDExPaymentWindow(const UniValue& value);
int64_t ParseDExFee(const UniValue& value);
uint8_t ParseDExAction(const UniValue& value);
uint8_t ParseEcosystem(const UniValue& value);
uint16_t ParsePropertyType(const UniValue& value);
uint32_t ParsePreviousPropertyId(const UniValue& value);
std::string ParseText(const UniValue& value);
int64_t ParseDeadline(const UniValue& value);
uint8_t ParseEarlyBirdBonus(const UniValue& value);
uint8_t ParseIssuerBonus(const UniValue& value);
uint8_t ParseMetaDExAction(const UniValue& value);
CTransaction ParseTransaction(const UniValue& value);
CMutableTransaction ParseMutableTransaction(const UniValue& value);
CPubKey ParsePubKeyOrAddress(const UniValue& value);
uint32_t ParseOutputIndex(const UniValue& value);
/** Parses previous transaction outputs. */
std::vector<PrevTxsEntry> ParsePrevTxs(const UniValue& value);


#endif // OMNICORE_RPCVALUES_H
