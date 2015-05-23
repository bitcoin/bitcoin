#ifndef OMNICORE_RPCVALUES_H
#define OMNICORE_RPCVALUES_H

#include "json/json_spirit_value.h"

#include <stdint.h>
#include <string>

std::string ParseAddress(const json_spirit::Value& value);
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


#endif // OMNICORE_RPCVALUES_H
