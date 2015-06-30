#include "omnicore/rpcvalues.h"

#include "omnicore/parse_string.h"

#include "base58.h"
#include "rpcprotocol.h"

#include "json/json_spirit_value.h"

#include <string>

using mastercore::StrToInt64;

std::string ParseAddress(const json_spirit::Value& value)
{
    CBitcoinAddress address(value.get_str());
    if (!address.IsValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }
    return address.ToString();
}

uint32_t ParsePropertyId(const json_spirit::Value& value)
{
    int64_t propertyId = value.get_int64();
    if (propertyId < 1 || 4294967295LL < propertyId) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier is out of range");
    }
    return static_cast<uint32_t>(propertyId);
}

int64_t ParseAmount(const json_spirit::Value& value, bool isDivisible)
{
    int64_t amount = mastercore::StrToInt64(value.get_str(), isDivisible);
    if (amount < 1) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
    }
    return amount;
}

int64_t ParseAmount(const json_spirit::Value& value, int propertyType)
{
    bool fDivisible = (propertyType == 2);  // 1 = indivisible, 2 = divisible
    return ParseAmount(value, fDivisible);
}

uint8_t ParseDExPaymentWindow(const json_spirit::Value& value)
{
    int64_t blocks = value.get_int64();
    if (blocks < 1 || 255 < blocks) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Payment window must be within 1-255 blocks");
    }
    return static_cast<uint8_t>(blocks);
}

int64_t ParseDExFee(const json_spirit::Value& value)
{
    int64_t fee = StrToInt64(value.get_str(), true);  // BTC is divisible
    if (fee < 0) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Mininmum accept fee must be positive");
    }
    return fee;
}

uint8_t ParseDExAction(const json_spirit::Value& value)
{
    int64_t action = value.get_int64();
    if (action <= 0 || 3 < action) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid action (1 = new, 2 = update, 3 = cancel only)");
    }
    return static_cast<uint8_t>(action);
}

uint8_t ParseEcosystem(const json_spirit::Value& value)
{
    int64_t ecosystem = value.get_int64();
    if (ecosystem < 1 || 2 < ecosystem) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid ecosystem (1 = main, 2 = test only)");
    }
    return static_cast<uint8_t>(ecosystem);
}

uint16_t ParsePropertyType(const json_spirit::Value& value)
{
    int64_t propertyType = value.get_int64();
    if (propertyType < 1 || 2 < propertyType) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid property type (1 = indivisible, 2 = divisible only)");
    }
    return static_cast<uint16_t>(propertyType);
}

uint32_t ParsePreviousPropertyId(const json_spirit::Value& value)
{
    int64_t previousId = value.get_int64();
    if (previousId != 0) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Property appends/replaces are not yet supported");
    }
    return static_cast<uint32_t>(previousId);
}

std::string ParseText(const json_spirit::Value& value)
{
    std::string text = value.get_str();
    if (text.size() > 255) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Text must not be longer than 255 characters");
    }
    return text;
}

int64_t ParseDeadline(const json_spirit::Value& value)
{
    int64_t deadline = value.get_int64();
    if (deadline < 0) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Deadline must be positive");
    }
    return deadline;
}

uint8_t ParseEarlyBirdBonus(const json_spirit::Value& value)
{
    int64_t percentage = value.get_int64();
    if (percentage < 0 || 255 < percentage) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Early bird bonus must be in the range of 0-255 percent per week");
    }
    return static_cast<uint8_t>(percentage);
}

uint8_t ParseIssuerBonus(const json_spirit::Value& value)
{
    int64_t percentage = value.get_int64();
    if (percentage < 0 || 255 < percentage) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Bonus for issuer must be in the range of 0-255 percent");
    }
    return static_cast<uint8_t>(percentage);
}

uint8_t ParseMetaDExAction(const json_spirit::Value& value)
{
    int64_t action = value.get_int64();
    if (action <= 0 || 4 < action) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid action (1, 2, 3, 4 only)");
    }
    return static_cast<uint8_t>(action);
}

