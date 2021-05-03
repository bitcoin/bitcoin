// This file serves to provide payload creation functions.

#include <omnicore/createpayload.h>

#include <omnicore/log.h>
#include <omnicore/parsing.h>

#include <base58.h>

#include <stdint.h>
#include <string>
#include <vector>

/**
 * Pushes bytes to the end of a vector.
 */
#define PUSH_BACK_BYTES(vector, value)\
    vector.insert(vector.end(), reinterpret_cast<unsigned char *>(&(value)),\
    reinterpret_cast<unsigned char *>(&(value)) + sizeof((value)));

/**
 * Pushes bytes to the end of a vector based on a pointer.
 */
#define PUSH_BACK_BYTES_PTR(vector, ptr, size)\
    vector.insert(vector.end(), reinterpret_cast<unsigned char *>((ptr)),\
    reinterpret_cast<unsigned char *>((ptr)) + (size));

/**
 * Returns a vector of bytes containing the version and hash160 for an address.
 */
static std::vector<unsigned char> AddressToBytes(const std::string& address)
{
    std::vector<unsigned char> addressBytes;
    bool success = DecodeBase58(address, addressBytes, 21 + 4);
    if (!success) {
        PrintToLog("ERROR: failed to decode address %s.\n", address);
    }
    if (addressBytes.size() == 25) {
        addressBytes.resize(21); // truncate checksum
    } else {
        PrintToLog("ERROR: unexpected size from DecodeBase58 when decoding address %s.\n", address);
    }

    return addressBytes;
}

std::vector<unsigned char> CreatePayload_SimpleSend(uint32_t propertyId, uint64_t amount)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 0;
    uint16_t messageVer = 0;
    SwapByteOrder16(messageType);
    SwapByteOrder16(messageVer);
    SwapByteOrder32(propertyId);
    SwapByteOrder64(amount);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyId);
    PUSH_BACK_BYTES(payload, amount);

    return payload;
}

std::vector<unsigned char> CreatePayload_SendAll(uint8_t ecosystem)
{
    std::vector<unsigned char> payload;
    uint16_t messageVer = 0;
    uint16_t messageType = 4;
    SwapByteOrder16(messageVer);
    SwapByteOrder16(messageType);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, ecosystem);

    return payload;
}

std::vector<unsigned char> CreatePayload_SendNonFungible(uint32_t propertyId, uint64_t tokenStart, uint64_t tokenEnd)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 5;
    uint16_t messageVer = 0;
    SwapByteOrder16(messageType);
    SwapByteOrder16(messageVer);
    SwapByteOrder32(propertyId);
    SwapByteOrder64(tokenStart);
    SwapByteOrder64(tokenEnd);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyId);
    PUSH_BACK_BYTES(payload, tokenStart);
    PUSH_BACK_BYTES(payload, tokenEnd);

    return payload;
}

std::vector<unsigned char> CreatePayload_SetNonFungibleData(uint32_t propertyId, uint64_t tokenStart, uint64_t tokenEnd, uint8_t issuer, std::string& data)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 201;
    uint16_t messageVer = 0;
    SwapByteOrder16(messageType);
    SwapByteOrder16(messageVer);
    SwapByteOrder32(propertyId);
    SwapByteOrder64(tokenStart);
    SwapByteOrder64(tokenEnd);
    if (data.size() > 255) data = data.substr(0,255);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyId);
    PUSH_BACK_BYTES(payload, tokenStart);
    PUSH_BACK_BYTES(payload, tokenEnd);
    PUSH_BACK_BYTES(payload, issuer);
    payload.insert(payload.end(), data.begin(), data.end());
    payload.push_back('\0');

    return payload;
}

std::vector<unsigned char> CreatePayload_DExSell(uint32_t propertyId, uint64_t amountForSale, uint64_t amountDesired, uint8_t timeLimit, uint64_t minFee, uint8_t subAction)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 20;
    uint16_t messageVer = 1;
    SwapByteOrder16(messageType);
    SwapByteOrder16(messageVer);
    SwapByteOrder32(propertyId);
    SwapByteOrder64(amountForSale);
    SwapByteOrder64(amountDesired);
    SwapByteOrder64(minFee);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyId);
    PUSH_BACK_BYTES(payload, amountForSale);
    PUSH_BACK_BYTES(payload, amountDesired);
    PUSH_BACK_BYTES(payload, timeLimit);
    PUSH_BACK_BYTES(payload, minFee);
    PUSH_BACK_BYTES(payload, subAction);

    return payload;
}

std::vector<unsigned char> CreatePayload_DExAccept(uint32_t propertyId, uint64_t amount)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 22;
    uint16_t messageVer = 0;
    SwapByteOrder16(messageType);
    SwapByteOrder16(messageVer);
    SwapByteOrder32(propertyId);
    SwapByteOrder64(amount);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyId);
    PUSH_BACK_BYTES(payload, amount);

    return payload;
}

std::vector<unsigned char> CreatePayload_SendToOwners(uint32_t propertyId, uint64_t amount, uint32_t distributionProperty)
{
    bool v0 = (propertyId == distributionProperty) ? true : false;

    std::vector<unsigned char> payload;

    uint16_t messageType = 3;
    uint16_t messageVer = (v0) ? 0 : 1;
    SwapByteOrder16(messageType);
    SwapByteOrder16(messageVer);
    SwapByteOrder32(propertyId);
    SwapByteOrder64(amount);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyId);
    PUSH_BACK_BYTES(payload, amount);
    if (!v0) {
        SwapByteOrder32(distributionProperty);
        PUSH_BACK_BYTES(payload, distributionProperty);
    }

    return payload;
}

std::vector<unsigned char> CreatePayload_IssuanceFixed(uint8_t ecosystem, uint16_t propertyType, uint32_t previousPropertyId, std::string category,
                                                       std::string subcategory, std::string name, std::string url, std::string data, uint64_t amount)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 50;
    uint16_t messageVer = 0;
    SwapByteOrder16(messageVer);
    SwapByteOrder16(messageType);
    SwapByteOrder16(propertyType);
    SwapByteOrder32(previousPropertyId);
    SwapByteOrder64(amount);
    if (category.size() > 255) category = category.substr(0,255);
    if (subcategory.size() > 255) subcategory = subcategory.substr(0,255);
    if (name.size() > 255) name = name.substr(0,255);
    if (url.size() > 255) url = url.substr(0,255);
    if (data.size() > 255) data = data.substr(0,255);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, ecosystem);
    PUSH_BACK_BYTES(payload, propertyType);
    PUSH_BACK_BYTES(payload, previousPropertyId);
    payload.insert(payload.end(), category.begin(), category.end());
    payload.push_back('\0');
    payload.insert(payload.end(), subcategory.begin(), subcategory.end());
    payload.push_back('\0');
    payload.insert(payload.end(), name.begin(), name.end());
    payload.push_back('\0');
    payload.insert(payload.end(), url.begin(), url.end());
    payload.push_back('\0');
    payload.insert(payload.end(), data.begin(), data.end());
    payload.push_back('\0');
    PUSH_BACK_BYTES(payload, amount);

    return payload;
}

std::vector<unsigned char> CreatePayload_IssuanceVariable(uint8_t ecosystem, uint16_t propertyType, uint32_t previousPropertyId, std::string category,
                                                          std::string subcategory, std::string name, std::string url, std::string data, uint32_t propertyIdDesired,
                                                          uint64_t amountPerUnit, uint64_t deadline, uint8_t earlyBonus, uint8_t issuerPercentage)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 51;
    uint16_t messageVer = 0;
    SwapByteOrder16(messageVer);
    SwapByteOrder16(messageType);
    SwapByteOrder16(propertyType);
    SwapByteOrder32(previousPropertyId);
    SwapByteOrder32(propertyIdDesired);
    SwapByteOrder64(amountPerUnit);
    SwapByteOrder64(deadline);
    if (category.size() > 255) category = category.substr(0,255);
    if (subcategory.size() > 255) subcategory = subcategory.substr(0,255);
    if (name.size() > 255) name = name.substr(0,255);
    if (url.size() > 255) url = url.substr(0,255);
    if (data.size() > 255) data = data.substr(0,255);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, ecosystem);
    PUSH_BACK_BYTES(payload, propertyType);
    PUSH_BACK_BYTES(payload, previousPropertyId);
    payload.insert(payload.end(), category.begin(), category.end());
    payload.push_back('\0');
    payload.insert(payload.end(), subcategory.begin(), subcategory.end());
    payload.push_back('\0');
    payload.insert(payload.end(), name.begin(), name.end());
    payload.push_back('\0');
    payload.insert(payload.end(), url.begin(), url.end());
    payload.push_back('\0');
    payload.insert(payload.end(), data.begin(), data.end());
    payload.push_back('\0');
    PUSH_BACK_BYTES(payload, propertyIdDesired);
    PUSH_BACK_BYTES(payload, amountPerUnit);
    PUSH_BACK_BYTES(payload, deadline);
    PUSH_BACK_BYTES(payload, earlyBonus);
    PUSH_BACK_BYTES(payload, issuerPercentage);

    return payload;
}

std::vector<unsigned char> CreatePayload_IssuanceManaged(uint8_t ecosystem, uint16_t propertyType, uint32_t previousPropertyId, std::string category,
                                                       std::string subcategory, std::string name, std::string url, std::string data)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 54;
    uint16_t messageVer = 0;
    SwapByteOrder16(messageVer);
    SwapByteOrder16(messageType);
    SwapByteOrder16(propertyType);
    SwapByteOrder32(previousPropertyId);
    if (category.size() > 255) category = category.substr(0,255);
    if (subcategory.size() > 255) subcategory = subcategory.substr(0,255);
    if (name.size() > 255) name = name.substr(0,255);
    if (url.size() > 255) url = url.substr(0,255);
    if (data.size() > 255) data = data.substr(0,255);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, ecosystem);
    PUSH_BACK_BYTES(payload, propertyType);
    PUSH_BACK_BYTES(payload, previousPropertyId);
    payload.insert(payload.end(), category.begin(), category.end());
    payload.push_back('\0');
    payload.insert(payload.end(), subcategory.begin(), subcategory.end());
    payload.push_back('\0');
    payload.insert(payload.end(), name.begin(), name.end());
    payload.push_back('\0');
    payload.insert(payload.end(), url.begin(), url.end());
    payload.push_back('\0');
    payload.insert(payload.end(), data.begin(), data.end());
    payload.push_back('\0');

    return payload;
}

std::vector<unsigned char> CreatePayload_CloseCrowdsale(uint32_t propertyId)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 53;
    uint16_t messageVer = 0;
    SwapByteOrder16(messageType);
    SwapByteOrder16(messageVer);
    SwapByteOrder32(propertyId);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyId);

    return payload;
}

std::vector<unsigned char> CreatePayload_Grant(uint32_t propertyId, uint64_t amount, std::string info)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 55;
    uint16_t messageVer = 0;
    SwapByteOrder16(messageType);
    SwapByteOrder16(messageVer);
    SwapByteOrder32(propertyId);
    SwapByteOrder64(amount);
    if (info.size() > 255) info = info.substr(0,255);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyId);
    PUSH_BACK_BYTES(payload, amount);
    payload.insert(payload.end(), info.begin(), info.end());
    payload.push_back('\0');

    return payload;
}


std::vector<unsigned char> CreatePayload_Revoke(uint32_t propertyId, uint64_t amount, std::string memo)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 56;
    uint16_t messageVer = 0;
    SwapByteOrder16(messageType);
    SwapByteOrder16(messageVer);
    SwapByteOrder32(propertyId);
    SwapByteOrder64(amount);
    if (memo.size() > 255) memo = memo.substr(0,255);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyId);
    PUSH_BACK_BYTES(payload, amount);
    payload.insert(payload.end(), memo.begin(), memo.end());
    payload.push_back('\0');

    return payload;
}

std::vector<unsigned char> CreatePayload_ChangeIssuer(uint32_t propertyId)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 70;
    uint16_t messageVer = 0;
    SwapByteOrder16(messageType);
    SwapByteOrder16(messageVer);
    SwapByteOrder32(propertyId);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyId);

    return payload;
}

std::vector<unsigned char> CreatePayload_EnableFreezing(uint32_t propertyId)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 71;
    uint16_t messageVer = 0;
    SwapByteOrder16(messageType);
    SwapByteOrder16(messageVer);
    SwapByteOrder32(propertyId);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyId);

    return payload;
}

std::vector<unsigned char> CreatePayload_DisableFreezing(uint32_t propertyId)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 72;
    uint16_t messageVer = 0;
    SwapByteOrder16(messageType);
    SwapByteOrder16(messageVer);
    SwapByteOrder32(propertyId);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyId);

    return payload;
}

std::vector<unsigned char> CreatePayload_FreezeTokens(uint32_t propertyId, uint64_t amount, const std::string& address)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 185;
    uint16_t messageVer = 0;
    SwapByteOrder16(messageType);
    SwapByteOrder16(messageVer);
    SwapByteOrder32(propertyId);
    SwapByteOrder64(amount);
    std::vector<unsigned char> addressBytes = AddressToBytes(address);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyId);
    PUSH_BACK_BYTES(payload, amount);
    payload.insert(payload.end(), addressBytes.begin(), addressBytes.end());

    return payload;
}

std::vector<unsigned char> CreatePayload_UnfreezeTokens(uint32_t propertyId, uint64_t amount, const std::string& address)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 186;
    uint16_t messageVer = 0;
    SwapByteOrder16(messageType);
    SwapByteOrder16(messageVer);
    SwapByteOrder32(propertyId);
    SwapByteOrder64(amount);
    std::vector<unsigned char> addressBytes = AddressToBytes(address);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyId);
    PUSH_BACK_BYTES(payload, amount);
    payload.insert(payload.end(), addressBytes.begin(), addressBytes.end());

    return payload;
}

std::vector<unsigned char> CreatePayload_MetaDExTrade(uint32_t propertyIdForSale, uint64_t amountForSale, uint32_t propertyIdDesired, uint64_t amountDesired)
{
    std::vector<unsigned char> payload;

    uint16_t messageType = 25;
    uint16_t messageVer = 0;

    SwapByteOrder16(messageVer);
    SwapByteOrder16(messageType);
    SwapByteOrder32(propertyIdForSale);
    SwapByteOrder64(amountForSale);
    SwapByteOrder32(propertyIdDesired);
    SwapByteOrder64(amountDesired);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyIdForSale);
    PUSH_BACK_BYTES(payload, amountForSale);
    PUSH_BACK_BYTES(payload, propertyIdDesired);
    PUSH_BACK_BYTES(payload, amountDesired);

    return payload;
}

std::vector<unsigned char> CreatePayload_MetaDExCancelPrice(uint32_t propertyIdForSale, uint64_t amountForSale, uint32_t propertyIdDesired, uint64_t amountDesired)
{
    std::vector<unsigned char> payload;

    uint16_t messageType = 26;
    uint16_t messageVer = 0;

    SwapByteOrder16(messageVer);
    SwapByteOrder16(messageType);
    SwapByteOrder32(propertyIdForSale);
    SwapByteOrder64(amountForSale);
    SwapByteOrder32(propertyIdDesired);
    SwapByteOrder64(amountDesired);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyIdForSale);
    PUSH_BACK_BYTES(payload, amountForSale);
    PUSH_BACK_BYTES(payload, propertyIdDesired);
    PUSH_BACK_BYTES(payload, amountDesired);

    return payload;
}

std::vector<unsigned char> CreatePayload_MetaDExCancelPair(uint32_t propertyIdForSale, uint32_t propertyIdDesired)
{
    std::vector<unsigned char> payload;

    uint16_t messageType = 27;
    uint16_t messageVer = 0;

    SwapByteOrder16(messageVer);
    SwapByteOrder16(messageType);
    SwapByteOrder32(propertyIdForSale);
    SwapByteOrder32(propertyIdDesired);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, propertyIdForSale);
    PUSH_BACK_BYTES(payload, propertyIdDesired);

    return payload;
}

std::vector<unsigned char> CreatePayload_MetaDExCancelEcosystem(uint8_t ecosystem)
{
    std::vector<unsigned char> payload;

    uint16_t messageType = 28;
    uint16_t messageVer = 0;

    SwapByteOrder16(messageVer);
    SwapByteOrder16(messageType);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, ecosystem);

    return payload;
}

std::vector<unsigned char> CreatePayload_AnyData(const std::vector<unsigned char>& data)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 0;
    uint16_t messageType = 200;

    SwapByteOrder16(messageVer);
    SwapByteOrder16(messageType);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    payload.insert(payload.end(), data.begin(), data.end());

    return payload;
}

std::vector<unsigned char> CreatePayload_DeactivateFeature(uint16_t featureId)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 65535;
    uint16_t messageType = 65533;

    SwapByteOrder16(messageVer);
    SwapByteOrder16(messageType);
    SwapByteOrder16(featureId);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, featureId);

    return payload;
}

std::vector<unsigned char> CreatePayload_ActivateFeature(uint16_t featureId, uint32_t activationBlock, uint32_t minClientVersion)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 65535;
    uint16_t messageType = 65534;

    SwapByteOrder16(messageVer);
    SwapByteOrder16(messageType);
    SwapByteOrder16(featureId);
    SwapByteOrder32(activationBlock);
    SwapByteOrder32(minClientVersion);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, featureId);
    PUSH_BACK_BYTES(payload, activationBlock);
    PUSH_BACK_BYTES(payload, minClientVersion);

    return payload;
}

std::vector<unsigned char> CreatePayload_OmniCoreAlert(uint16_t alertType, uint32_t expiryValue, const std::string& alertMessage)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 65535;
    uint16_t messageVer = 65535;

    SwapByteOrder16(messageVer);
    SwapByteOrder16(messageType);
    SwapByteOrder16(alertType);
    SwapByteOrder32(expiryValue);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, alertType);
    PUSH_BACK_BYTES(payload, expiryValue);
    payload.insert(payload.end(), alertMessage.begin(), alertMessage.end());
    payload.push_back('\0');

    return payload;
}

#undef PUSH_BACK_BYTES
#undef PUSH_BACK_BYTES_PTR
