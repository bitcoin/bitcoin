#ifndef OMNICORE_RPCVALUES_H
#define OMNICORE_RPCVALUES_H

class CPubKey;
class CTransaction;
struct CMutableTransaction;
struct PrevTxsEntry;

#include "json/json_spirit_value.h"

#include <stdint.h>
#include <string>
#include <vector>

std::string ParseAddress(const json_spirit::Value& value);
std::string ParseAddressOrEmpty(const json_spirit::Value& value);
std::string ParseAddressOrWildcard(const json_spirit::Value& value);
uint32_t ParsePropertyId(const json_spirit::Value& value);
int64_t ParseAmount(const json_spirit::Value& value, bool isDivisible);
int64_t ParseAmount(const json_spirit::Value& value, int propertyType);
uint8_t ParseDExPaymentWindow(const json_spirit::Value& value);
int64_t ParseDExFee(const json_spirit::Value& value);
uint8_t ParseDExAction(const json_spirit::Value& value);
uint8_t ParseEcosystem(const json_spirit::Value& value);
uint16_t ParsePropertyType(const json_spirit::Value& value);
uint32_t ParsePreviousPropertyId(const json_spirit::Value& value);
std::string ParseText(const json_spirit::Value& value);
int64_t ParseDeadline(const json_spirit::Value& value);
uint8_t ParseEarlyBirdBonus(const json_spirit::Value& value);
uint8_t ParseIssuerBonus(const json_spirit::Value& value);
uint8_t ParseMetaDExAction(const json_spirit::Value& value);
CTransaction ParseTransaction(const json_spirit::Value& value);
CMutableTransaction ParseMutableTransaction(const json_spirit::Value& value);
CPubKey ParsePubKeyOrAddress(const json_spirit::Value& value);
uint32_t ParseOutputIndex(const json_spirit::Value& value);
/** Parses previous transaction outputs. */
std::vector<PrevTxsEntry> ParsePrevTxs(const json_spirit::Value& value);


#endif // OMNICORE_RPCVALUES_H
