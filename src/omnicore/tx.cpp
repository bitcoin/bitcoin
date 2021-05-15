// Master Protocol transaction code

#include <omnicore/tx.h>

#include <omnicore/activation.h>
#include <omnicore/dbfees.h>
#include <omnicore/dbspinfo.h>
#include <omnicore/dbstolist.h>
#include <omnicore/dbtradelist.h>
#include <omnicore/dbtxlist.h>
#include <omnicore/dex.h>
#include <omnicore/log.h>
#include <omnicore/mdex.h>
#include <omnicore/notifications.h>
#include <omnicore/omnicore.h>
#include <omnicore/parsing.h>
#include <omnicore/rules.h>
#include <omnicore/sp.h>
#include <omnicore/sto.h>
#include <omnicore/nftdb.h>
#include <omnicore/utilsbitcoin.h>
#include <omnicore/version.h>

#include <amount.h>
#include <base58.h>
#include <key_io.h>
#include <validation.h>
#include <sync.h>
#include <util/time.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <utility>
#include <vector>

using boost::algorithm::token_compress_on;

using namespace mastercore;

/** Returns a label for the given transaction type. */
std::string mastercore::strTransactionType(uint16_t txType)
{
    switch (txType) {
        case MSC_TYPE_SIMPLE_SEND: return "Simple Send";
        case MSC_TYPE_RESTRICTED_SEND: return "Restricted Send";
        case MSC_TYPE_SEND_TO_OWNERS: return "Send To Owners";
        case MSC_TYPE_SEND_ALL: return "Send All";
        case MSC_TYPE_SEND_NONFUNGIBLE: return "Unique Send";
        case MSC_TYPE_SAVINGS_MARK: return "Savings";
        case MSC_TYPE_SAVINGS_COMPROMISED: return "Savings COMPROMISED";
        case MSC_TYPE_RATELIMITED_MARK: return "Rate-Limiting";
        case MSC_TYPE_AUTOMATIC_DISPENSARY: return "Automatic Dispensary";
        case MSC_TYPE_TRADE_OFFER: return "DEx Sell Offer";
        case MSC_TYPE_METADEX_TRADE: return "MetaDEx trade";
        case MSC_TYPE_METADEX_CANCEL_PRICE: return "MetaDEx cancel-price";
        case MSC_TYPE_METADEX_CANCEL_PAIR: return "MetaDEx cancel-pair";
        case MSC_TYPE_METADEX_CANCEL_ECOSYSTEM: return "MetaDEx cancel-ecosystem";
        case MSC_TYPE_ACCEPT_OFFER_BTC: return "DEx Accept Offer";
        case MSC_TYPE_CREATE_PROPERTY_FIXED: return "Create Property - Fixed";
        case MSC_TYPE_CREATE_PROPERTY_VARIABLE: return "Create Property - Variable";
        case MSC_TYPE_PROMOTE_PROPERTY: return "Promote Property";
        case MSC_TYPE_CLOSE_CROWDSALE: return "Close Crowdsale";
        case MSC_TYPE_CREATE_PROPERTY_MANUAL: return "Create Property - Manual";
        case MSC_TYPE_GRANT_PROPERTY_TOKENS: return "Grant Property Tokens";
        case MSC_TYPE_REVOKE_PROPERTY_TOKENS: return "Revoke Property Tokens";
        case MSC_TYPE_CHANGE_ISSUER_ADDRESS: return "Change Issuer Address";
        case MSC_TYPE_ENABLE_FREEZING: return "Enable Freezing";
        case MSC_TYPE_DISABLE_FREEZING: return "Disable Freezing";
        case MSC_TYPE_FREEZE_PROPERTY_TOKENS: return "Freeze Property Tokens";
        case MSC_TYPE_UNFREEZE_PROPERTY_TOKENS: return "Unfreeze Property Tokens";
        case MSC_TYPE_ADD_DELEGATE: return "Add Delegate";
        case MSC_TYPE_REMOVE_DELEGATE: return "Remove Delegate";
        case MSC_TYPE_ANYDATA: return "Embed any data";
        case MSC_TYPE_NONFUNGIBLE_DATA: return "Set Non-Fungible Token Data";
        case MSC_TYPE_NOTIFICATION: return "Notification";
        case OMNICORE_MESSAGE_TYPE_ALERT: return "ALERT";
        case OMNICORE_MESSAGE_TYPE_DEACTIVATION: return "Feature Deactivation";
        case OMNICORE_MESSAGE_TYPE_ACTIVATION: return "Feature Activation";

        default: return "* unknown type *";
    }
}

/** Helper to convert class number to string. */
static std::string intToClass(int encodingClass)
{
    switch (encodingClass) {
        case OMNI_CLASS_A:
            return "A";
        case OMNI_CLASS_B:
            return "B";
        case OMNI_CLASS_C:
            return "C";
    }

    return "-";
}

/** Checks whether a pointer to the payload is past it's last position. */
bool CMPTransaction::isOverrun(const char* p)
{
    ptrdiff_t pos = (char*) p - (char*) &pkt;
    return (pos > pkt_size);
}

// -------------------- PACKET PARSING -----------------------

/** Parses the packet or payload. */
bool CMPTransaction::interpret_Transaction()
{
    if (!interpret_TransactionType()) {
        PrintToLog("Failed to interpret type and version\n");
        return false;
    }

    switch (type) {
        case MSC_TYPE_SIMPLE_SEND:
            return interpret_SimpleSend();

        case MSC_TYPE_SEND_TO_OWNERS:
            return interpret_SendToOwners();

        case MSC_TYPE_SEND_ALL:
            return interpret_SendAll();

        case MSC_TYPE_SEND_NONFUNGIBLE:
            return interpret_SendNonFungible();

        case MSC_TYPE_TRADE_OFFER:
            return interpret_TradeOffer();

        case MSC_TYPE_ACCEPT_OFFER_BTC:
            return interpret_AcceptOfferBTC();

        case MSC_TYPE_METADEX_TRADE:
            return interpret_MetaDExTrade();

        case MSC_TYPE_METADEX_CANCEL_PRICE:
            return interpret_MetaDExCancelPrice();

        case MSC_TYPE_METADEX_CANCEL_PAIR:
            return interpret_MetaDExCancelPair();

        case MSC_TYPE_METADEX_CANCEL_ECOSYSTEM:
            return interpret_MetaDExCancelEcosystem();

        case MSC_TYPE_CREATE_PROPERTY_FIXED:
            return interpret_CreatePropertyFixed();

        case MSC_TYPE_CREATE_PROPERTY_VARIABLE:
            return interpret_CreatePropertyVariable();

        case MSC_TYPE_CLOSE_CROWDSALE:
            return interpret_CloseCrowdsale();

        case MSC_TYPE_CREATE_PROPERTY_MANUAL:
            return interpret_CreatePropertyManaged();

        case MSC_TYPE_GRANT_PROPERTY_TOKENS:
            return interpret_GrantTokens();

        case MSC_TYPE_REVOKE_PROPERTY_TOKENS:
            return interpret_RevokeTokens();

        case MSC_TYPE_CHANGE_ISSUER_ADDRESS:
            return interpret_ChangeIssuer();

        case MSC_TYPE_ENABLE_FREEZING:
            return interpret_EnableFreezing();

        case MSC_TYPE_DISABLE_FREEZING:
            return interpret_DisableFreezing();

        case MSC_TYPE_FREEZE_PROPERTY_TOKENS:
            return interpret_FreezeTokens();

        case MSC_TYPE_UNFREEZE_PROPERTY_TOKENS:
            return interpret_UnfreezeTokens();

        case OMNICORE_MESSAGE_TYPE_DEACTIVATION:
            return interpret_Deactivation();
            
        case MSC_TYPE_ANYDATA:
            return interpret_AnyData();

        case MSC_TYPE_NONFUNGIBLE_DATA:
            return interpret_NonFungibleData();

        case OMNICORE_MESSAGE_TYPE_ACTIVATION:
            return interpret_Activation();

        case OMNICORE_MESSAGE_TYPE_ALERT:
            return interpret_Alert();
    }

    return false;
}

/** Version and type */
bool CMPTransaction::interpret_TransactionType()
{
    if (pkt_size < 4) {
        return false;
    }
    uint16_t txVersion = 0;
    uint16_t txType = 0;
    memcpy(&txVersion, &pkt[0], 2);
    SwapByteOrder16(txVersion);
    memcpy(&txType, &pkt[2], 2);
    SwapByteOrder16(txType);
    version = txVersion;
    type = txType;

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t------------------------------\n");
        PrintToLog("\t         version: %d, class %s\n", txVersion, intToClass(encodingClass));
        PrintToLog("\t            type: %d (%s)\n", txType, strTransactionType(txType));
    }

    return true;
}

/** Tx 1 */
bool CMPTransaction::interpret_SimpleSend()
{
    if (pkt_size < 16) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    SwapByteOrder64(nValue);
    nNewValue = nValue;

    // Special case: if can't find the receiver -- assume send to self!
    if (receiver.empty()) {
        receiver = sender;
    }

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t           value: %s\n", FormatMP(property, nValue));
    }

    return true;
}

/** Tx 3 */
bool CMPTransaction::interpret_SendToOwners()
{
    int expectedSize = (version == MP_TX_PKT_V0) ? 16 : 20;
    if (pkt_size < expectedSize) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    SwapByteOrder64(nValue);
    nNewValue = nValue;
    if (version > MP_TX_PKT_V0) {
        memcpy(&distribution_property, &pkt[16], 4);
        SwapByteOrder32(distribution_property);
    }

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t             property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t                value: %s\n", FormatMP(property, nValue));
        if (version > MP_TX_PKT_V1) {
            PrintToLog("\t distributionproperty: %d (%s)\n", distribution_property, strMPProperty(distribution_property));
        }
    }

    return true;
}

/** Tx 4 */
bool CMPTransaction::interpret_SendAll()
{
    if (pkt_size < 5) {
        return false;
    }
    memcpy(&ecosystem, &pkt[4], 1);

    property = ecosystem; // provide a hint for the UI, TODO: better handling!

    // Special case: if can't find the receiver -- assume send to self!
    if (receiver.empty()) {
        receiver = sender;
    }

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t       ecosystem: %d\n", (int)ecosystem);
    }

    return true;
}

/** Tx 5 */
bool CMPTransaction::interpret_SendNonFungible()
{
    if (pkt_size < 24) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);
    memcpy(&nonfungible_token_start, &pkt[8], 8);
    SwapByteOrder64(nonfungible_token_start);
    memcpy(&nonfungible_token_end, &pkt[16], 8);
    SwapByteOrder64(nonfungible_token_end);

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t      tokenstart: %d\n", nonfungible_token_start);
        PrintToLog("\t        tokenend: %d\n", nonfungible_token_end);
    }

    return true;
}

/** Tx 20 */
bool CMPTransaction::interpret_TradeOffer()
{
    int expectedSize = (version == MP_TX_PKT_V0) ? 33 : 34;
    if (pkt_size < expectedSize) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    SwapByteOrder64(nValue);
    nNewValue = nValue;
    memcpy(&amount_desired, &pkt[16], 8);
    memcpy(&blocktimelimit, &pkt[24], 1);
    memcpy(&min_fee, &pkt[25], 8);
    if (version > MP_TX_PKT_V0) {
        memcpy(&subaction, &pkt[33], 1);
    }
    SwapByteOrder64(amount_desired);
    SwapByteOrder64(min_fee);

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t           value: %s\n", FormatMP(property, nValue));
        PrintToLog("\t  amount desired: %s\n", FormatDivisibleMP(amount_desired));
        PrintToLog("\tblock time limit: %d\n", blocktimelimit);
        PrintToLog("\t         min fee: %s\n", FormatDivisibleMP(min_fee));
        if (version > MP_TX_PKT_V0) {
            PrintToLog("\t      sub-action: %d\n", subaction);
        }
    }

    return true;
}

/** Tx 22 */
bool CMPTransaction::interpret_AcceptOfferBTC()
{
    if (pkt_size < 16) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    SwapByteOrder64(nValue);
    nNewValue = nValue;

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t           value: %s\n", FormatMP(property, nValue));
    }

    return true;
}

/** Tx 25 */
bool CMPTransaction::interpret_MetaDExTrade()
{
    if (pkt_size < 28) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    SwapByteOrder64(nValue);
    nNewValue = nValue;
    memcpy(&desired_property, &pkt[16], 4);
    SwapByteOrder32(desired_property);
    memcpy(&desired_value, &pkt[20], 8);
    SwapByteOrder64(desired_value);

    action = CMPTransaction::ADD; // deprecated

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t           value: %s\n", FormatMP(property, nValue));
        PrintToLog("\tdesired property: %d (%s)\n", desired_property, strMPProperty(desired_property));
        PrintToLog("\t   desired value: %s\n", FormatMP(desired_property, desired_value));
    }

    return true;
}

/** Tx 26 */
bool CMPTransaction::interpret_MetaDExCancelPrice()
{
    if (pkt_size < 28) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    SwapByteOrder64(nValue);
    nNewValue = nValue;
    memcpy(&desired_property, &pkt[16], 4);
    SwapByteOrder32(desired_property);
    memcpy(&desired_value, &pkt[20], 8);
    SwapByteOrder64(desired_value);

    action = CMPTransaction::CANCEL_AT_PRICE; // deprecated

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t           value: %s\n", FormatMP(property, nValue));
        PrintToLog("\tdesired property: %d (%s)\n", desired_property, strMPProperty(desired_property));
        PrintToLog("\t   desired value: %s\n", FormatMP(desired_property, desired_value));
    }

    return true;
}

/** Tx 27 */
bool CMPTransaction::interpret_MetaDExCancelPair()
{
    if (pkt_size < 12) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);
    memcpy(&desired_property, &pkt[8], 4);
    SwapByteOrder32(desired_property);

    nValue = 0; // deprecated
    nNewValue = nValue; // deprecated
    desired_value = 0; // deprecated
    action = CMPTransaction::CANCEL_ALL_FOR_PAIR; // deprecated

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\tdesired property: %d (%s)\n", desired_property, strMPProperty(desired_property));
    }

    return true;
}

/** Tx 28 */
bool CMPTransaction::interpret_MetaDExCancelEcosystem()
{
    if (pkt_size < 5) {
        return false;
    }
    memcpy(&ecosystem, &pkt[4], 1);

    property = ecosystem; // deprecated
    desired_property = ecosystem; // deprecated
    nValue = 0; // deprecated
    nNewValue = nValue; // deprecated
    desired_value = 0; // deprecated
    action = CMPTransaction::CANCEL_EVERYTHING; // deprecated

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t       ecosystem: %d\n", (int)ecosystem);
    }

    return true;
}

/** Tx 50 */
bool CMPTransaction::interpret_CreatePropertyFixed()
{
    if (pkt_size < 25) {
        return false;
    }
    const char* p = 11 + (char*) &pkt;
    std::vector<std::string> spstr;
    memcpy(&ecosystem, &pkt[4], 1);
    memcpy(&prop_type, &pkt[5], 2);
    SwapByteOrder16(prop_type);
    memcpy(&prev_prop_id, &pkt[7], 4);
    SwapByteOrder32(prev_prop_id);
    for (int i = 0; i < 5; i++) {
        spstr.push_back(std::string(p));
        p += spstr.back().size() + 1;
    }
    int i = 0;
    memcpy(category, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(category)-1)); i++;
    memcpy(subcategory, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(subcategory)-1)); i++;
    memcpy(name, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(name)-1)); i++;
    memcpy(url, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(url)-1)); i++;
    memcpy(data, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(data)-1)); i++;
    memcpy(&nValue, p, 8);
    SwapByteOrder64(nValue);
    p += 8;
    nNewValue = nValue;

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t       ecosystem: %d\n", ecosystem);
        PrintToLog("\t   property type: %d (%s)\n", prop_type, strPropertyType(prop_type));
        PrintToLog("\tprev property id: %d\n", prev_prop_id);
        PrintToLog("\t        category: %s\n", category);
        PrintToLog("\t     subcategory: %s\n", subcategory);
        PrintToLog("\t            name: %s\n", name);
        PrintToLog("\t             url: %s\n", url);
        PrintToLog("\t            data: %s\n", data);
        PrintToLog("\t           value: %s\n", FormatByType(nValue, prop_type));
    }

    if (isOverrun(p)) {
        PrintToLog("%s(): rejected: malformed string value(s)\n", __func__);
        return false;
    }

    return true;
}

/** Tx 51 */
bool CMPTransaction::interpret_CreatePropertyVariable()
{
    if (pkt_size < 39) {
        return false;
    }
    const char* p = 11 + (char*) &pkt;
    std::vector<std::string> spstr;
    memcpy(&ecosystem, &pkt[4], 1);
    memcpy(&prop_type, &pkt[5], 2);
    SwapByteOrder16(prop_type);
    memcpy(&prev_prop_id, &pkt[7], 4);
    SwapByteOrder32(prev_prop_id);
    for (int i = 0; i < 5; i++) {
        spstr.push_back(std::string(p));
        p += spstr.back().size() + 1;
    }
    int i = 0;
    memcpy(category, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(category)-1)); i++;
    memcpy(subcategory, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(subcategory)-1)); i++;
    memcpy(name, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(name)-1)); i++;
    memcpy(url, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(url)-1)); i++;
    memcpy(data, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(data)-1)); i++;
    memcpy(&property, p, 4);
    SwapByteOrder32(property);
    p += 4;
    memcpy(&nValue, p, 8);
    SwapByteOrder64(nValue);
    p += 8;
    nNewValue = nValue;
    memcpy(&deadline, p, 8);
    SwapByteOrder64(deadline);
    p += 8;
    memcpy(&early_bird, p++, 1);
    memcpy(&percentage, p++, 1);

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t       ecosystem: %d\n", ecosystem);
        PrintToLog("\t   property type: %d (%s)\n", prop_type, strPropertyType(prop_type));
        PrintToLog("\tprev property id: %d\n", prev_prop_id);
        PrintToLog("\t        category: %s\n", category);
        PrintToLog("\t     subcategory: %s\n", subcategory);
        PrintToLog("\t            name: %s\n", name);
        PrintToLog("\t             url: %s\n", url);
        PrintToLog("\t            data: %s\n", data);
        PrintToLog("\tproperty desired: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t tokens per unit: %s\n", FormatByType(nValue, prop_type));
        PrintToLog("\t        deadline: %d\n", deadline);
        PrintToLog("\tearly bird bonus: %d\n", early_bird);
        PrintToLog("\t    issuer bonus: %d\n", percentage);
    }

    if (isOverrun(p)) {
        PrintToLog("%s(): rejected: malformed string value(s)\n", __func__);
        return false;
    }

    return true;
}

/** Tx 53 */
bool CMPTransaction::interpret_CloseCrowdsale()
{
    if (pkt_size < 8) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
    }

    return true;
}

/** Tx 54 */
bool CMPTransaction::interpret_CreatePropertyManaged()
{
    if (pkt_size < 17) {
        return false;
    }
    const char* p = 11 + (char*) &pkt;
    std::vector<std::string> spstr;
    memcpy(&ecosystem, &pkt[4], 1);
    memcpy(&prop_type, &pkt[5], 2);
    SwapByteOrder16(prop_type);
    memcpy(&prev_prop_id, &pkt[7], 4);
    SwapByteOrder32(prev_prop_id);
    for (int i = 0; i < 5; i++) {
        spstr.push_back(std::string(p));
        p += spstr.back().size() + 1;
    }
    int i = 0;
    memcpy(category, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(category)-1)); i++;
    memcpy(subcategory, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(subcategory)-1)); i++;
    memcpy(name, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(name)-1)); i++;
    memcpy(url, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(url)-1)); i++;
    memcpy(data, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(data)-1)); i++;

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t       ecosystem: %d\n", ecosystem);
        PrintToLog("\t   property type: %d (%s)\n", prop_type, strPropertyType(prop_type));
        PrintToLog("\tprev property id: %d\n", prev_prop_id);
        PrintToLog("\t        category: %s\n", category);
        PrintToLog("\t     subcategory: %s\n", subcategory);
        PrintToLog("\t            name: %s\n", name);
        PrintToLog("\t             url: %s\n", url);
        PrintToLog("\t            data: %s\n", data);
    }

    if (isOverrun(p)) {
        PrintToLog("%s(): rejected: malformed string value(s)\n", __func__);
        return false;
    }

    return true;
}

/** Tx 55 */
bool CMPTransaction::interpret_GrantTokens()
{
    if (pkt_size < 16) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    SwapByteOrder64(nValue);
    nNewValue = nValue;

    // Get NFT data, was memo before and previously unused here.
    const char* p = 16 + (char*) &pkt;
    std::string spstr(p);
    memcpy(nonfungible_data, spstr.c_str(), std::min(spstr.length(), sizeof(nonfungible_data)-1));

    // Special case: if can't find the receiver -- assume grant to self!
    if (receiver.empty()) {
        receiver = sender;
    }

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t           value: %s\n", FormatMP(property, nValue));
        PrintToLog("\t            data: %s\n", nonfungible_data);
    }

    return true;
}

/** Tx 56 */
bool CMPTransaction::interpret_RevokeTokens()
{
    if (pkt_size < 16) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    SwapByteOrder64(nValue);
    nNewValue = nValue;

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t           value: %s\n", FormatMP(property, nValue));
    }

    return true;
}

/** Tx 70 */
bool CMPTransaction::interpret_ChangeIssuer()
{
    if (pkt_size < 8) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
    }

    return true;
}

/** Tx 71 */
bool CMPTransaction::interpret_EnableFreezing()
{
    if (pkt_size < 8) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
    }

    return true;
}

/** Tx 72 */
bool CMPTransaction::interpret_DisableFreezing()
{
    if (pkt_size < 8) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
    }

    return true;
}

/** Tx 185 */
bool CMPTransaction::interpret_FreezeTokens()
{
    if (pkt_size < 37) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    SwapByteOrder64(nValue);
    nNewValue = nValue;

    /**
        Note, TX185 is a virtual reference transaction type.
              With virtual reference transactions a hash160 in the payload sets the receiver.
              Reference outputs are ignored.
    **/
    unsigned char address_version;
    uint160 address_hash160;
    memcpy(&address_version, &pkt[16], 1);
    memcpy(&address_hash160, &pkt[17], 20);
    receiver = HashToAddress(address_version, address_hash160);
    if (receiver.empty()) {
        return false;
    }
    CTxDestination recAddress = DecodeDestination(receiver);
    if (!IsValidDestination(recAddress)) {
        return false;
    }

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t  value (unused): %s\n", FormatMP(property, nValue));
        PrintToLog("\t         address: %s\n", receiver);
    }

    return true;
}

/** Tx 186 */
bool CMPTransaction::interpret_UnfreezeTokens()
{
    if (pkt_size < 37) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    SwapByteOrder64(nValue);
    nNewValue = nValue;

    /**
        Note, TX186 virtual reference transaction type.
              With virtual reference transactions a hash160 in the payload sets the receiver.
              Reference outputs are ignored.
    **/
    unsigned char address_version;
    uint160 address_hash160;
    memcpy(&address_version, &pkt[16], 1);
    memcpy(&address_hash160, &pkt[17], 20);
    receiver = HashToAddress(address_version, address_hash160);
    if (receiver.empty()) {
        return false;
    }
    CTxDestination recAddress = DecodeDestination(receiver);
    if (!IsValidDestination(recAddress)) {
        return false;
    }

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t  value (unused): %s\n", FormatMP(property, nValue));
        PrintToLog("\t         address: %s\n", receiver);
    }

    return true;
}

/** Tx 200 */
bool CMPTransaction::interpret_AnyData()
{
    if (pkt_size < 4) {
        return false;
    }

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t       data: %s\n", "...");
        PrintToLog("\t   receiver: %s\n", receiver);
    }

    return true;
}

/** Tx 201 */
bool CMPTransaction::interpret_NonFungibleData()
{
    if (pkt_size < 25) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    SwapByteOrder32(property);
    memcpy(&nonfungible_token_start, &pkt[8], 8);
    SwapByteOrder64(nonfungible_token_start);
    memcpy(&nonfungible_token_end, &pkt[16], 8);
    SwapByteOrder64(nonfungible_token_end);
    memcpy(&nonfungible_data_type, &pkt[24], 1);

    const char* p = 25 + (char*) &pkt;
    std::string spstr(p);
    memcpy(nonfungible_data, spstr.c_str(), std::min(spstr.length(), sizeof(nonfungible_data)-1));

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t                property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t nonfungible_token_start: %d\n", nonfungible_token_start);
        PrintToLog("\t   nonfungible_token_end: %d\n", nonfungible_token_end);
        PrintToLog("\t                    data: %s\n", nonfungible_data);
    }

    return true;
}

/** Tx 65533 */
bool CMPTransaction::interpret_Deactivation()
{
    if (pkt_size < 6) {
        return false;
    }
    memcpy(&feature_id, &pkt[4], 2);
    SwapByteOrder16(feature_id);

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t      feature id: %d\n", feature_id);
    }

    return true;
}

/** Tx 65534 */
bool CMPTransaction::interpret_Activation()
{
    if (pkt_size < 14) {
        return false;
    }
    memcpy(&feature_id, &pkt[4], 2);
    SwapByteOrder16(feature_id);
    memcpy(&activation_block, &pkt[6], 4);
    SwapByteOrder32(activation_block);
    memcpy(&min_client_version, &pkt[10], 4);
    SwapByteOrder32(min_client_version);

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t      feature id: %d\n", feature_id);
        PrintToLog("\tactivation block: %d\n", activation_block);
        PrintToLog("\t minimum version: %d\n", min_client_version);
    }

    return true;
}

/** Tx 65535 */
bool CMPTransaction::interpret_Alert()
{
    if (pkt_size < 11) {
        return false;
    }

    memcpy(&alert_type, &pkt[4], 2);
    SwapByteOrder16(alert_type);
    memcpy(&alert_expiry, &pkt[6], 4);
    SwapByteOrder32(alert_expiry);

    const char* p = 10 + (char*) &pkt;
    std::string spstr(p);
    memcpy(alert_text, spstr.c_str(), std::min(spstr.length(), sizeof(alert_text)-1));

    if ((!rpcOnly && msc_debug_packets) || msc_debug_packets_readonly) {
        PrintToLog("\t      alert type: %d\n", alert_type);
        PrintToLog("\t    expiry value: %d\n", alert_expiry);
        PrintToLog("\t   alert message: %s\n", alert_text);
    }

    if (isOverrun(p)) {
        PrintToLog("%s(): rejected: malformed string value(s)\n", __func__);
        return false;
    }

    return true;
}

// ---------------------- CORE LOGIC -------------------------

/**
 * Interprets the payload and executes the logic.
 *
 * @return  0  if the transaction is fully valid
 *         <0  if the transaction is invalid
 */
int CMPTransaction::interpretPacket()
{
    if (rpcOnly) {
        PrintToLog("%s(): ERROR: attempt to execute logic in RPC mode\n", __func__);
        return (PKT_ERROR -1);
    }

    if (!interpret_Transaction()) {
        return (PKT_ERROR -2);
    }

    // Use ::ChainActive()[block] here to avoid locking cs_main after cs_tally below
    CBlockIndex* pindex;
    uint256 blockHash;
    {
        LOCK(cs_main);
        pindex = ::ChainActive()[block];
        blockHash = ::ChainActive()[block]->GetBlockHash();
    }

    LOCK(cs_tally);

    if (isAddressFrozen(sender, property)) {
        PrintToLog("%s(): REJECTED: address %s is frozen for property %d\n", __func__, sender, property);
        return (PKT_ERROR -3);
    }

    switch (type) {
        case MSC_TYPE_SIMPLE_SEND:
            return logicMath_SimpleSend(blockHash);

        case MSC_TYPE_SEND_TO_OWNERS:
            return logicMath_SendToOwners();

        case MSC_TYPE_SEND_ALL:
            return logicMath_SendAll();

        case MSC_TYPE_SEND_NONFUNGIBLE:
            return logicMath_SendNonFungible();

        case MSC_TYPE_TRADE_OFFER:
            return logicMath_TradeOffer();

        case MSC_TYPE_ACCEPT_OFFER_BTC:
            return logicMath_AcceptOffer_BTC();

        case MSC_TYPE_METADEX_TRADE:
            return logicMath_MetaDExTrade();

        case MSC_TYPE_METADEX_CANCEL_PRICE:
            return logicMath_MetaDExCancelPrice();

        case MSC_TYPE_METADEX_CANCEL_PAIR:
            return logicMath_MetaDExCancelPair();

        case MSC_TYPE_METADEX_CANCEL_ECOSYSTEM:
            return logicMath_MetaDExCancelEcosystem();

        case MSC_TYPE_CREATE_PROPERTY_FIXED:
            return logicMath_CreatePropertyFixed(pindex);

        case MSC_TYPE_CREATE_PROPERTY_VARIABLE:
            return logicMath_CreatePropertyVariable(pindex);

        case MSC_TYPE_CLOSE_CROWDSALE:
            return logicMath_CloseCrowdsale(pindex);

        case MSC_TYPE_CREATE_PROPERTY_MANUAL:
            return logicMath_CreatePropertyManaged(pindex);

        case MSC_TYPE_GRANT_PROPERTY_TOKENS:
            return logicMath_GrantTokens(pindex, blockHash);

        case MSC_TYPE_REVOKE_PROPERTY_TOKENS:
            return logicMath_RevokeTokens(pindex);

        case MSC_TYPE_CHANGE_ISSUER_ADDRESS:
            return logicMath_ChangeIssuer(pindex);

        case MSC_TYPE_ENABLE_FREEZING:
            return logicMath_EnableFreezing(pindex);

        case MSC_TYPE_DISABLE_FREEZING:
            return logicMath_DisableFreezing(pindex);

        case MSC_TYPE_FREEZE_PROPERTY_TOKENS:
            return logicMath_FreezeTokens(pindex);

        case MSC_TYPE_UNFREEZE_PROPERTY_TOKENS:
            return logicMath_UnfreezeTokens(pindex);

        case MSC_TYPE_ANYDATA:
            return logicMath_AnyData();

        case MSC_TYPE_NONFUNGIBLE_DATA:
            return logicMath_NonFungibleData();

        case OMNICORE_MESSAGE_TYPE_DEACTIVATION:
            return logicMath_Deactivation();

        case OMNICORE_MESSAGE_TYPE_ACTIVATION:
            return logicMath_Activation();

        case OMNICORE_MESSAGE_TYPE_ALERT:
            return logicMath_Alert();
    }

    return (PKT_ERROR -100);
}

/** Passive effect of crowdsale participation. */
int CMPTransaction::logicHelper_CrowdsaleParticipation(uint256& blockHash)
{
    CMPCrowd* pcrowdsale = getCrowd(receiver);

    // No active crowdsale
    if (pcrowdsale == nullptr) {
        return (PKT_ERROR_CROWD -1);
    }
    // Active crowdsale, but not for this property
    if (pcrowdsale->getCurrDes() != property) {
        return (PKT_ERROR_CROWD -2);
    }

    CMPSPInfo::Entry sp;
    assert(pDbSpInfo->getSP(pcrowdsale->getPropertyId(), sp));
    PrintToLog("INVESTMENT SEND to Crowdsale Issuer: %s\n", receiver);

    // Holds the tokens to be credited to the sender and issuer
    std::pair<int64_t, int64_t> tokens;

    // Passed by reference to determine, if max_tokens has been reached
    bool close_crowdsale = false;

    // Units going into the calculateFundraiser function must match the unit of
    // the fundraiser's property_type. By default this means satoshis in and
    // satoshis out. In the condition that the fundraiser is divisible, but
    // indivisible tokens are accepted, it must account for .0 Div != 1 Indiv,
    // but actually 1.0 Div == 100000000 Indiv. The unit must be shifted or the
    // values will be incorrect, which is what is checked below.
    bool inflateAmount = isPropertyDivisible(property) ? false : true;

    // Calculate the amounts to credit for this fundraiser
    calculateFundraiser(inflateAmount, nValue, sp.early_bird, sp.deadline, blockTime,
            sp.num_tokens, sp.percentage, getTotalTokens(pcrowdsale->getPropertyId()),
            tokens, close_crowdsale);

    if (msc_debug_sp) {
        PrintToLog("%s(): granting via crowdsale to user: %s %d (%s)\n",
                __func__, FormatMP(property, tokens.first), property, strMPProperty(property));
        PrintToLog("%s(): granting via crowdsale to issuer: %s %d (%s)\n",
                __func__, FormatMP(property, tokens.second), property, strMPProperty(property));
    }

    // Update the crowdsale object
    pcrowdsale->incTokensUserCreated(tokens.first);
    pcrowdsale->incTokensIssuerCreated(tokens.second);

    // Data to pass to txFundraiserData
    int64_t txdata[] = {(int64_t) nValue, blockTime, tokens.first, tokens.second};
    std::vector<int64_t> txDataVec(txdata, txdata + sizeof(txdata) / sizeof(txdata[0]));

    // Insert data about crowdsale participation
    pcrowdsale->insertDatabase(txid, txDataVec);

    // Credit tokens for this fundraiser
    if (tokens.first > 0) {
        assert(update_tally_map(sender, pcrowdsale->getPropertyId(), tokens.first, BALANCE));
    }
    if (tokens.second > 0) {
        assert(update_tally_map(receiver, pcrowdsale->getPropertyId(), tokens.second, BALANCE));
    }

    // Number of tokens has changed, update fee distribution thresholds
    NotifyTotalTokensChanged(pcrowdsale->getPropertyId(), block);

    // Close crowdsale, if we hit MAX_TOKENS
    if (close_crowdsale) {
        eraseMaxedCrowdsale(receiver, blockTime, block, blockHash);
    }

    // Indicate, if no tokens were transferred
    if (!tokens.first && !tokens.second) {
        return (PKT_ERROR_CROWD -3);
    }

    return 0;
}

/** Tx 0 */
int CMPTransaction::logicMath_SimpleSend(uint256& blockHash)
{
    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_SEND -22);
    }

    if (isPropertyNonFungible(property)) {
        PrintToLog("%s(): rejected: property %d is of type non-fungible\n", __func__, property);
        return (PKT_ERROR_TOKENS -27);
    }

    if (nValue <= 0 || MAX_INT_8_BYTES < nValue) {
        PrintToLog("%s(): rejected: value out of range or zero: %d", __func__, nValue);
        return (PKT_ERROR_SEND -23);
    }

    if (!IsPropertyIdValid(property)) {
        PrintToLog("%s(): rejected: property %d does not exist\n", __func__, property);
        return (PKT_ERROR_SEND -24);
    }

    int64_t nBalance = GetTokenBalance(sender, property, BALANCE);
    if (nBalance < (int64_t) nValue) {
        PrintToLog("%s(): rejected: sender %s has insufficient balance of property %d [%s < %s]\n",
                __func__,
                sender,
                property,
                FormatMP(property, nBalance),
                FormatMP(property, nValue));
        return (PKT_ERROR_SEND -25);
    }

    // ------------------------------------------

    // Move the tokens
    assert(update_tally_map(sender, property, -nValue, BALANCE));
    assert(update_tally_map(receiver, property, nValue, BALANCE));

    // Is there an active crowdsale running from this recipient?
    logicHelper_CrowdsaleParticipation(blockHash);

    return 0;
}

/** Tx 3 */
int CMPTransaction::logicMath_SendToOwners()
{
    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_STO -22);
    }

    if (isPropertyNonFungible(property)) {
        PrintToLog("%s(): rejected: property %d is of type non-fungible\n", __func__, property);
        return (PKT_ERROR_TOKENS -27);
    }

    if (nValue <= 0 || MAX_INT_8_BYTES < nValue) {
        PrintToLog("%s(): rejected: value out of range or zero: %d\n", __func__, nValue);
        return (PKT_ERROR_STO -23);
    }

    if (!IsPropertyIdValid(property)) {
        PrintToLog("%s(): rejected: property %d does not exist\n", __func__, property);
        return (PKT_ERROR_STO -24);
    }

    if (version > MP_TX_PKT_V0) {
        if (!IsPropertyIdValid(distribution_property)) {
            PrintToLog("%s(): rejected: distribution property %d does not exist\n", __func__, distribution_property);
            return (PKT_ERROR_STO -24);
        }
    }

    int64_t nBalance = GetTokenBalance(sender, property, BALANCE);
    if (nBalance < (int64_t) nValue) {
        PrintToLog("%s(): rejected: sender %s has insufficient balance of property %d [%s < %s]\n",
                __func__,
                sender,
                FormatMP(property, nBalance),
                FormatMP(property, nValue),
                property);
        return (PKT_ERROR_STO -25);
    }

    // ------------------------------------------

    uint32_t distributeTo = (version == MP_TX_PKT_V0) ? property : distribution_property;
    OwnerAddrType receiversSet = STO_GetReceivers(sender, distributeTo, nValue);
    uint64_t numberOfReceivers = receiversSet.size();

    // make sure we found some owners
    if (numberOfReceivers <= 0) {
        PrintToLog("%s(): rejected: no other owners of property %d [owners=%d <= 0]\n", __func__, distributeTo, numberOfReceivers);
        return (PKT_ERROR_STO -26);
    }

    // determine which property the fee will be paid in
    uint32_t feeProperty = isTestEcosystemProperty(property) ? OMNI_PROPERTY_TMSC : OMNI_PROPERTY_MSC;
    int64_t feePerOwner = (version == MP_TX_PKT_V0) ? TRANSFER_FEE_PER_OWNER : TRANSFER_FEE_PER_OWNER_V1;
    int64_t transferFee = feePerOwner * numberOfReceivers;
    PrintToLog("\t    Transfer fee: %s %s\n", FormatDivisibleMP(transferFee), strMPProperty(feeProperty));

    // enough coins to pay the fee?
    if (feeProperty != property) {
        int64_t nBalanceFee = GetTokenBalance(sender, feeProperty, BALANCE);
        if (nBalanceFee < transferFee) {
            PrintToLog("%s(): rejected: sender %s has insufficient balance of property %d to pay for fee [%s < %s]\n",
                    __func__,
                    sender,
                    feeProperty,
                    FormatMP(property, nBalanceFee),
                    FormatMP(property, transferFee));
            return (PKT_ERROR_STO -27);
        }
    } else {
        // special case check, only if distributing MSC or TMSC -- the property the fee will be paid in
        int64_t nBalanceFee = GetTokenBalance(sender, feeProperty, BALANCE);
        if (nBalanceFee < ((int64_t) nValue + transferFee)) {
            PrintToLog("%s(): rejected: sender %s has insufficient balance of %d to pay for amount + fee [%s < %s + %s]\n",
                    __func__,
                    sender,
                    feeProperty,
                    FormatMP(property, nBalanceFee),
                    FormatMP(property, nValue),
                    FormatMP(property, transferFee));
            return (PKT_ERROR_STO -28);
        }
    }

    // ------------------------------------------

    assert(update_tally_map(sender, feeProperty, -transferFee, BALANCE));
    if (version == MP_TX_PKT_V0) {
        // v0 - do not credit the subtracted fee to any tally (ie burn the tokens)
    } else {
        // v1 - credit the subtracted fee to the fee cache
        pDbFeeCache->AddFee(feeProperty, block, transferFee);
    }

    // split up what was taken and distribute between all holders
    int64_t sent_so_far = 0;
    for (OwnerAddrType::reverse_iterator it = receiversSet.rbegin(); it != receiversSet.rend(); ++it) {
        const std::string& address = it->second;

        int64_t will_really_receive = it->first;
        sent_so_far += will_really_receive;

        // real execution of the loop
        assert(update_tally_map(sender, property, -will_really_receive, BALANCE));
        assert(update_tally_map(address, property, will_really_receive, BALANCE));

        // add to stodb
        pDbStoList->recordSTOReceive(address, txid, block, property, will_really_receive);

        if (sent_so_far != (int64_t)nValue) {
            PrintToLog("sent_so_far= %14d, nValue= %14d, n_owners= %d\n", sent_so_far, nValue, numberOfReceivers);
        } else {
            PrintToLog("SendToOwners: DONE HERE\n");
        }
    }

    // sent_so_far must equal nValue here
    assert(sent_so_far == (int64_t)nValue);

    // Number of tokens has changed, update fee distribution thresholds
    if (version == MP_TX_PKT_V0) NotifyTotalTokensChanged(OMNI_PROPERTY_MSC, block); // fee was burned

    return 0;
}

/** Tx 4 */
int CMPTransaction::logicMath_SendAll()
{
    if (!IsTransactionTypeAllowed(block, ecosystem, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                ecosystem,
                block);
        return (PKT_ERROR_SEND_ALL -22);
    }

    if (isPropertyNonFungible(property)) {
        PrintToLog("%s(): rejected: property %d is of type non-fungible\n", __func__, property);
        return (PKT_ERROR_TOKENS -27);
    }

    // ------------------------------------------

    CMPTally* ptally = getTally(sender);
    if (ptally == nullptr) {
        PrintToLog("%s(): rejected: sender %s has no tokens to send\n", __func__, sender);
        return (PKT_ERROR_SEND_ALL -54);
    }

    uint32_t propertyId = ptally->init();
    int numberOfPropertiesSent = 0;

    while (0 != (propertyId = ptally->next())) {
        // only transfer tokens in the specified ecosystem
        if (ecosystem == OMNI_PROPERTY_MSC && isTestEcosystemProperty(propertyId)) {
            continue;
        }
        if (ecosystem == OMNI_PROPERTY_TMSC && isMainEcosystemProperty(propertyId)) {
            continue;
        }

        // do not transfer tokens from a frozen property
        if (isAddressFrozen(sender, propertyId)) {
            PrintToLog("%s(): sender %s is frozen for property %d - the property will not be included in processing.\n", __func__, sender, propertyId);
            continue;
        }

        int64_t moneyAvailable = ptally->getMoney(propertyId, BALANCE);
        if (moneyAvailable > 0) {
            ++numberOfPropertiesSent;
            assert(update_tally_map(sender, propertyId, -moneyAvailable, BALANCE));
            assert(update_tally_map(receiver, propertyId, moneyAvailable, BALANCE));
            pDbTransactionList->recordSendAllSubRecord(txid, numberOfPropertiesSent, propertyId, moneyAvailable);
        }
    }

    if (!numberOfPropertiesSent) {
        PrintToLog("%s(): rejected: sender %s has no tokens to send\n", __func__, sender);
        return (PKT_ERROR_SEND_ALL -55);
    }

    nNewValue = numberOfPropertiesSent;

    return 0;
}

/** Tx 5 */
int CMPTransaction::logicMath_SendNonFungible()
{
    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_SEND -22);
    }

    if (!isPropertyNonFungible(property)) {
        PrintToLog("%s(): rejected: property %d is not of type non-fungible\n", __func__, property);
        return (PKT_ERROR_TOKENS -27);
    }

    if (nonfungible_token_start <= 0 || MAX_INT_8_BYTES < nonfungible_token_start) {
        PrintToLog("%s(): rejected: non-fungible token range start value out of range or zero: %d", __func__, nonfungible_token_start);
        return (PKT_ERROR_SEND -23);
    }

    if (nonfungible_token_end <= 0 || MAX_INT_8_BYTES < nonfungible_token_end) {
        PrintToLog("%s(): rejected: non-fungible token range end value out of range or zero: %d", __func__, nonfungible_token_end);
        return (PKT_ERROR_SEND -23);
    }

    if (nonfungible_token_start > nonfungible_token_end) {
        PrintToLog("%s(): rejected: non-fungible token range start value: %d is less than or equal to range end value: %d", __func__, nonfungible_token_start, nonfungible_token_end);
        return (PKT_ERROR_SEND -23);
    }

    int64_t amount = (nonfungible_token_end - nonfungible_token_start) + 1;
    if (amount <= 0) {
        PrintToLog("%s(): rejected: non-fungible token range amount out of range or zero: %d", __func__, amount);
        return (PKT_ERROR_SEND -23);
    }

    if (!pDbSpInfo->hasSP(property)) {
        PrintToLog("%s(): rejected: property %d does not exist\n", __func__, property);
        return (PKT_ERROR_SEND -24);
    }

    int64_t nBalance = GetAvailableTokenBalance(sender, property);
    if (nBalance < amount) {
        PrintToLog("%s(): rejected: sender %s has insufficient balance of property %d [%s < %s]\n",
                __func__,
                sender,
                property,
                FormatMP(property, nBalance),
                FormatMP(property, amount));
        return (PKT_ERROR_SEND -25);
    }

    std::string rangeStartOwner = pDbNFT->GetNonFungibleTokenOwner(property, nonfungible_token_start);
    std::string rangeEndOwner = pDbNFT->GetNonFungibleTokenOwner(property, nonfungible_token_end);
    bool contiguous = pDbNFT->IsRangeContiguous(property, nonfungible_token_start, nonfungible_token_end);
    if (rangeStartOwner != sender || rangeEndOwner != sender || !contiguous) {
        PrintToLog("%s(): rejected: sender %s does not own the range being sent\n",
                __func__,
                sender);
        return (PKT_ERROR_SEND -26);
    }

    // ------------------------------------------

    // Special case: if can't find the receiver -- assume send to self!
    if (receiver.empty()) {
        receiver = sender;
    }

    // Move the tokens
    assert(update_tally_map(sender, property, -amount, BALANCE));
    assert(update_tally_map(receiver, property, amount, BALANCE));
    bool success = pDbNFT->MoveNonFungibleTokens(property,nonfungible_token_start,nonfungible_token_end,sender,receiver);
    assert(success);

    return 0;
}

/** Tx 20 */
int CMPTransaction::logicMath_TradeOffer()
{
    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_TRADEOFFER -22);
    }

    if (isPropertyNonFungible(property)) {
        PrintToLog("%s(): rejected: property %d is of type non-fungible\n", __func__, property);
        return (PKT_ERROR_TOKENS -27);
    }

    if (MAX_INT_8_BYTES < nValue) {
        PrintToLog("%s(): rejected: value out of range or zero: %d\n", __func__, nValue);
        return (PKT_ERROR_TRADEOFFER -23);
    }

    // Ensure only OMNI and TOMNI are allowed, when the DEx is not yet free
    if (!IsFeatureActivated(FEATURE_FREEDEX, block)) {
        if (OMNI_PROPERTY_TMSC != property && OMNI_PROPERTY_MSC != property) {
            PrintToLog("%s(): rejected: property for sale %d must be OMN or TOMN\n", __func__, property);
            return (PKT_ERROR_TRADEOFFER -47);
        }
    }

    // ------------------------------------------

    int rc = PKT_ERROR_TRADEOFFER;

    // figure out which Action this is based on amount for sale, version & etc.
    switch (version)
    {
        case MP_TX_PKT_V0:
        {
            if (0 != nValue) {
                if (!DEx_offerExists(sender, property)) {
                    rc = DEx_offerCreate(sender, property, nValue, block, amount_desired, min_fee, blocktimelimit, txid, &nNewValue);
                } else {
                    rc = DEx_offerUpdate(sender, property, nValue, block, amount_desired, min_fee, blocktimelimit, txid, &nNewValue);
                }
            } else {
                // what happens if nValue is 0 for V0 ?  ANSWER: check if exists and it does -- cancel, otherwise invalid
                if (DEx_offerExists(sender, property)) {
                    rc = DEx_offerDestroy(sender, property);
                } else {
                    PrintToLog("%s(): rejected: sender %s has no active sell offer for property: %d\n", __func__, sender, property);
                    rc = (PKT_ERROR_TRADEOFFER -49);
                }
            }

            break;
        }

        case MP_TX_PKT_V1:
        {
            if (DEx_offerExists(sender, property)) {
                if (CANCEL != subaction && UPDATE != subaction) {
                    PrintToLog("%s(): rejected: sender %s has an active sell offer for property: %d\n", __func__, sender, property);
                    rc = (PKT_ERROR_TRADEOFFER -48);
                    break;
                }
            } else {
                // Offer does not exist
                if (NEW != subaction) {
                    PrintToLog("%s(): rejected: sender %s has no active sell offer for property: %d\n", __func__, sender, property);
                    rc = (PKT_ERROR_TRADEOFFER -49);
                    break;
                }
            }

            switch (subaction) {
                case NEW:
                    rc = DEx_offerCreate(sender, property, nValue, block, amount_desired, min_fee, blocktimelimit, txid, &nNewValue);
                    break;

                case UPDATE:
                    rc = DEx_offerUpdate(sender, property, nValue, block, amount_desired, min_fee, blocktimelimit, txid, &nNewValue);
                    break;

                case CANCEL:
                    rc = DEx_offerDestroy(sender, property);
                    break;

                default:
                    rc = (PKT_ERROR -999);
                    break;
            }
            break;
        }

        default:
            rc = (PKT_ERROR -500); // neither V0 nor V1
            break;
    };

    return rc;
}

/** Tx 22 */
int CMPTransaction::logicMath_AcceptOffer_BTC()
{
    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (DEX_ERROR_ACCEPT -22);
    }

    if (isPropertyNonFungible(property)) {
        PrintToLog("%s(): rejected: property %d is of type non-fungible\n", __func__, property);
        return (PKT_ERROR_TOKENS -27);
    }

    if (nValue <= 0 || MAX_INT_8_BYTES < nValue) {
        PrintToLog("%s(): rejected: value out of range or zero: %d\n", __func__, nValue);
        return (DEX_ERROR_ACCEPT -23);
    }

    // ------------------------------------------

    // the min fee spec requirement is checked in the following function
    int rc = DEx_acceptCreate(sender, receiver, property, nValue, block, tx_fee_paid, &nNewValue);

    return rc;
}

/** Tx 25 */
int CMPTransaction::logicMath_MetaDExTrade()
{
    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_METADEX -22);
    }

    if (isPropertyNonFungible(property) || isPropertyNonFungible(desired_property)) {
        PrintToLog("%s(): rejected: property %d or %d is of type non-fungible\n", __func__, property, desired_property);
        return (PKT_ERROR_TOKENS -27);
    }

    if (property == desired_property) {
        PrintToLog("%s(): rejected: property for sale %d and desired property %d must not be equal\n",
                __func__,
                property,
                desired_property);
        return (PKT_ERROR_METADEX -29);
    }

    if (isTestEcosystemProperty(property) != isTestEcosystemProperty(desired_property)) {
        PrintToLog("%s(): rejected: property for sale %d and desired property %d not in same ecosystem\n",
                __func__,
                property,
                desired_property);
        return (PKT_ERROR_METADEX -30);
    }

    if (!IsPropertyIdValid(property)) {
        PrintToLog("%s(): rejected: property for sale %d does not exist\n", __func__, property);
        return (PKT_ERROR_METADEX -31);
    }

    if (!IsPropertyIdValid(desired_property)) {
        PrintToLog("%s(): rejected: desired property %d does not exist\n", __func__, desired_property);
        return (PKT_ERROR_METADEX -32);
    }

    if (nNewValue <= 0 || MAX_INT_8_BYTES < nNewValue) {
        PrintToLog("%s(): rejected: amount for sale out of range or zero: %d\n", __func__, nNewValue);
        return (PKT_ERROR_METADEX -33);
    }

    if (desired_value <= 0 || MAX_INT_8_BYTES < desired_value) {
        PrintToLog("%s(): rejected: desired amount out of range or zero: %d\n", __func__, desired_value);
        return (PKT_ERROR_METADEX -34);
    }

    if (!IsFeatureActivated(FEATURE_TRADEALLPAIRS, block)) {
        // Trading non-Omni pairs is not allowed before trading all pairs is activated
        if ((property != OMNI_PROPERTY_MSC) && (desired_property != OMNI_PROPERTY_MSC) &&
            (property != OMNI_PROPERTY_TMSC) && (desired_property != OMNI_PROPERTY_TMSC)) {
            PrintToLog("%s(): rejected: one side of a trade [%d, %d] must be OMN or TOMN\n", __func__, property, desired_property);
            return (PKT_ERROR_METADEX -35);
        }
    }

    int64_t nBalance = GetTokenBalance(sender, property, BALANCE);
    if (nBalance < (int64_t) nNewValue) {
        PrintToLog("%s(): rejected: sender %s has insufficient balance of property %d [%s < %s]\n",
                __func__,
                sender,
                property,
                FormatMP(property, nBalance),
                FormatMP(property, nNewValue));
        return (PKT_ERROR_METADEX -25);
    }

    // ------------------------------------------

    pDbTradeList->recordNewTrade(txid, sender, property, desired_property, block, tx_idx);
    int rc = MetaDEx_ADD(sender, property, nNewValue, block, desired_property, desired_value, txid, tx_idx);
    return rc;
}

/** Tx 26 */
int CMPTransaction::logicMath_MetaDExCancelPrice()
{
    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_METADEX -22);
    }

    if (isPropertyNonFungible(property) || isPropertyNonFungible(desired_property)) {
        PrintToLog("%s(): rejected: property %d or %d is of type non-fungible\n", __func__, property, desired_property);
        return (PKT_ERROR_TOKENS -27);
    }

    if (property == desired_property) {
        PrintToLog("%s(): rejected: property for sale %d and desired property %d must not be equal\n",
                __func__,
                property,
                desired_property);
        return (PKT_ERROR_METADEX -29);
    }

    if (isTestEcosystemProperty(property) != isTestEcosystemProperty(desired_property)) {
        PrintToLog("%s(): rejected: property for sale %d and desired property %d not in same ecosystem\n",
                __func__,
                property,
                desired_property);
        return (PKT_ERROR_METADEX -30);
    }

    if (!IsPropertyIdValid(property)) {
        PrintToLog("%s(): rejected: property for sale %d does not exist\n", __func__, property);
        return (PKT_ERROR_METADEX -31);
    }

    if (!IsPropertyIdValid(desired_property)) {
        PrintToLog("%s(): rejected: desired property %d does not exist\n", __func__, desired_property);
        return (PKT_ERROR_METADEX -32);
    }

    if (nNewValue <= 0 || MAX_INT_8_BYTES < nNewValue) {
        PrintToLog("%s(): rejected: amount for sale out of range or zero: %d\n", __func__, nNewValue);
        return (PKT_ERROR_METADEX -33);
    }

    if (desired_value <= 0 || MAX_INT_8_BYTES < desired_value) {
        PrintToLog("%s(): rejected: desired amount out of range or zero: %d\n", __func__, desired_value);
        return (PKT_ERROR_METADEX -34);
    }

    // ------------------------------------------

    int rc = MetaDEx_CANCEL_AT_PRICE(txid, block, sender, property, nNewValue, desired_property, desired_value);

    return rc;
}

/** Tx 27 */
int CMPTransaction::logicMath_MetaDExCancelPair()
{
    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_METADEX -22);
    }

    if (isPropertyNonFungible(property) || isPropertyNonFungible(desired_property)) {
        PrintToLog("%s(): rejected: property %d or %d is of type non-fungible\n", __func__, property, desired_property);
        return (PKT_ERROR_TOKENS -27);
    }

    if (property == desired_property) {
        PrintToLog("%s(): rejected: property for sale %d and desired property %d must not be equal\n",
                __func__,
                property,
                desired_property);
        return (PKT_ERROR_METADEX -29);
    }

    if (isTestEcosystemProperty(property) != isTestEcosystemProperty(desired_property)) {
        PrintToLog("%s(): rejected: property for sale %d and desired property %d not in same ecosystem\n",
                __func__,
                property,
                desired_property);
        return (PKT_ERROR_METADEX -30);
    }

    if (!IsPropertyIdValid(property)) {
        PrintToLog("%s(): rejected: property for sale %d does not exist\n", __func__, property);
        return (PKT_ERROR_METADEX -31);
    }

    if (!IsPropertyIdValid(desired_property)) {
        PrintToLog("%s(): rejected: desired property %d does not exist\n", __func__, desired_property);
        return (PKT_ERROR_METADEX -32);
    }

    // ------------------------------------------

    int rc = MetaDEx_CANCEL_ALL_FOR_PAIR(txid, block, sender, property, desired_property);

    return rc;
}

/** Tx 28 */
int CMPTransaction::logicMath_MetaDExCancelEcosystem()
{
    if (!IsTransactionTypeAllowed(block, ecosystem, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_METADEX -22);
    }

    if (OMNI_PROPERTY_MSC != ecosystem && OMNI_PROPERTY_TMSC != ecosystem) {
        PrintToLog("%s(): rejected: invalid ecosystem: %d\n", __func__, ecosystem);
        return (PKT_ERROR_METADEX -21);
    }

    int rc = MetaDEx_CANCEL_EVERYTHING(txid, block, sender, ecosystem);

    return rc;
}

/** Tx 50 */
int CMPTransaction::logicMath_CreatePropertyFixed(CBlockIndex* pindex)
{
    if (pindex == nullptr) {
        PrintToLog("%s(): ERROR: block %d not in the active chain\n", __func__, block);
        return (PKT_ERROR_SP -20);
    }
    uint256 blockHash = pindex->GetBlockHash();

    if (OMNI_PROPERTY_MSC != ecosystem && OMNI_PROPERTY_TMSC != ecosystem) {
        PrintToLog("%s(): rejected: invalid ecosystem: %d\n", __func__, (uint32_t) ecosystem);
        return (PKT_ERROR_SP -21);
    }

    if (!IsTransactionTypeAllowed(block, ecosystem, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_SP -22);
    }

    if (nValue <= 0 || MAX_INT_8_BYTES < nValue) {
        PrintToLog("%s(): rejected: value out of range or zero: %d\n", __func__, nValue);
        return (PKT_ERROR_SP -23);
    }

    if (MSC_PROPERTY_TYPE_INDIVISIBLE != prop_type && MSC_PROPERTY_TYPE_DIVISIBLE != prop_type) {
        PrintToLog("%s(): rejected: invalid property type: %d\n", __func__, prop_type);
        return (PKT_ERROR_SP -36);
    }

    if ('\0' == name[0]) {
        PrintToLog("%s(): rejected: property name must not be empty\n", __func__);
        return (PKT_ERROR_SP -37);
    }

    // ------------------------------------------

    CMPSPInfo::Entry newSP;
    newSP.issuer = sender;
    newSP.updateIssuer(block, tx_idx, sender);
    newSP.txid = txid;
    newSP.prop_type = prop_type;
    newSP.num_tokens = nValue;
    newSP.category.assign(category);
    newSP.subcategory.assign(subcategory);
    newSP.name.assign(name);
    newSP.url.assign(url);
    newSP.data.assign(data);
    newSP.fixed = true;
    newSP.creation_block = blockHash;
    newSP.update_block = newSP.creation_block;

    const uint32_t propertyId = pDbSpInfo->putSP(ecosystem, newSP);
    assert(propertyId > 0);
    assert(update_tally_map(sender, propertyId, nValue, BALANCE));

    NotifyTotalTokensChanged(propertyId, block);

    return 0;
}

/** Tx 51 */
int CMPTransaction::logicMath_CreatePropertyVariable(CBlockIndex* pindex)
{
    if (pindex == nullptr) {
        PrintToLog("%s(): ERROR: block %d not in the active chain\n", __func__, block);
        return (PKT_ERROR_SP -20);
    }
    uint256 blockHash = pindex->GetBlockHash();

    if (OMNI_PROPERTY_MSC != ecosystem && OMNI_PROPERTY_TMSC != ecosystem) {
        PrintToLog("%s(): rejected: invalid ecosystem: %d\n", __func__, (uint32_t) ecosystem);
        return (PKT_ERROR_SP -21);
    }

    if (IsFeatureActivated(FEATURE_SPCROWDCROSSOVER, block)) {
    /**
     * Ecosystem crossovers shall not be allowed after the feature was enabled.
     */
    if (isTestEcosystemProperty(ecosystem) != isTestEcosystemProperty(property)) {
        PrintToLog("%s(): rejected: ecosystem %d of tokens to issue and desired property %d not in same ecosystem\n",
                __func__,
                ecosystem,
                property);
        return (PKT_ERROR_SP -50);
    }
    }

    if (!IsTransactionTypeAllowed(block, ecosystem, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_SP -22);
    }

    if (nValue <= 0 || MAX_INT_8_BYTES < nValue) {
        PrintToLog("%s(): rejected: value out of range or zero: %d\n", __func__, nValue);
        return (PKT_ERROR_SP -23);
    }

    if (!IsPropertyIdValid(property)) {
        PrintToLog("%s(): rejected: property %d does not exist\n", __func__, property);
        return (PKT_ERROR_SP -24);
    }

    if (MSC_PROPERTY_TYPE_INDIVISIBLE != prop_type && MSC_PROPERTY_TYPE_DIVISIBLE != prop_type) {
        PrintToLog("%s(): rejected: invalid property type: %d\n", __func__, prop_type);
        return (PKT_ERROR_SP -36);
    }

    if ('\0' == name[0]) {
        PrintToLog("%s(): rejected: property name must not be empty\n", __func__);
        return (PKT_ERROR_SP -37);
    }

    if (!deadline || (int64_t) deadline < blockTime) {
        PrintToLog("%s(): rejected: deadline must not be in the past [%d < %d]\n", __func__, deadline, blockTime);
        return (PKT_ERROR_SP -38);
    }

    if (nullptr != getCrowd(sender)) {
        PrintToLog("%s(): rejected: sender %s has an active crowdsale\n", __func__, sender);
        return (PKT_ERROR_SP -39);
    }

    // ------------------------------------------

    CMPSPInfo::Entry newSP;
    newSP.issuer = sender;
    newSP.updateIssuer(block, tx_idx, sender);
    newSP.txid = txid;
    newSP.prop_type = prop_type;
    newSP.num_tokens = nValue;
    newSP.category.assign(category);
    newSP.subcategory.assign(subcategory);
    newSP.name.assign(name);
    newSP.url.assign(url);
    newSP.data.assign(data);
    newSP.fixed = false;
    newSP.property_desired = property;
    newSP.deadline = deadline;
    newSP.early_bird = early_bird;
    newSP.percentage = percentage;
    newSP.creation_block = blockHash;
    newSP.update_block = newSP.creation_block;

    const uint32_t propertyId = pDbSpInfo->putSP(ecosystem, newSP);
    assert(propertyId > 0);
    my_crowds.insert(std::make_pair(sender, CMPCrowd(propertyId, nValue, property, deadline, early_bird, percentage, 0, 0)));

    PrintToLog("CREATED CROWDSALE id: %d value: %d property: %d\n", propertyId, nValue, property);

    return 0;
}

/** Tx 53 */
int CMPTransaction::logicMath_CloseCrowdsale(CBlockIndex* pindex)
{
    if (pindex == nullptr) {
        PrintToLog("%s(): ERROR: block %d not in the active chain\n", __func__, block);
        return (PKT_ERROR_SP -20);
    }
    uint256 blockHash = pindex->GetBlockHash();

    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_SP -22);
    }

    if (isPropertyNonFungible(property)) {
        PrintToLog("%s(): rejected: property %d is of type non-fungible\n", __func__, property);
        return (PKT_ERROR_TOKENS -27);
    }

    if (!IsPropertyIdValid(property)) {
        PrintToLog("%s(): rejected: property %d does not exist\n", __func__, property);
        return (PKT_ERROR_SP -24);
    }

    CrowdMap::iterator it = my_crowds.find(sender);
    if (it == my_crowds.end()) {
        PrintToLog("%s(): rejected: sender %s has no active crowdsale\n", __func__, sender);
        return (PKT_ERROR_SP -40);
    }

    const CMPCrowd& crowd = it->second;
    if (property != crowd.getPropertyId()) {
        PrintToLog("%s(): rejected: property identifier mismatch [%d != %d]\n", __func__, property, crowd.getPropertyId());
        return (PKT_ERROR_SP -41);
    }

    // ------------------------------------------

    CMPSPInfo::Entry sp;
    assert(pDbSpInfo->getSP(property, sp));

    int64_t missedTokens = GetMissedIssuerBonus(sp, crowd);

    sp.historicalData = crowd.getDatabase();
    sp.update_block = blockHash;
    sp.close_early = true;
    sp.timeclosed = blockTime;
    sp.txid_close = txid;
    sp.missedTokens = missedTokens;

    assert(pDbSpInfo->updateSP(property, sp));
    if (missedTokens > 0) {
        assert(update_tally_map(sp.issuer, property, missedTokens, BALANCE));
    }
    my_crowds.erase(it);

    if (msc_debug_sp) PrintToLog("CLOSED CROWDSALE id: %d=%X\n", property, property);

    return 0;
}

/** Tx 54 */
int CMPTransaction::logicMath_CreatePropertyManaged(CBlockIndex* pindex)
{
    if (pindex == nullptr) {
        PrintToLog("%s(): ERROR: block %d not in the active chain\n", __func__, block);
        return (PKT_ERROR_SP -20);
    }
    uint256 blockHash = pindex->GetBlockHash();

    if (OMNI_PROPERTY_MSC != ecosystem && OMNI_PROPERTY_TMSC != ecosystem) {
        PrintToLog("%s(): rejected: invalid ecosystem: %d\n", __func__, (uint32_t) ecosystem);
        return (PKT_ERROR_SP -21);
    }

    if (!IsTransactionTypeAllowed(block, ecosystem, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_SP -22);
    }

    if (MSC_PROPERTY_TYPE_INDIVISIBLE != prop_type && MSC_PROPERTY_TYPE_DIVISIBLE != prop_type && MSC_PROPERTY_TYPE_NONFUNGIBLE != prop_type) {
        PrintToLog("%s(): rejected: invalid property type: %d\n", __func__, prop_type);
        return (PKT_ERROR_SP -36);
    }

    if (MSC_PROPERTY_TYPE_NONFUNGIBLE == prop_type && !IsFeatureActivated(FEATURE_NONFUNGIBLE, block)) {
        PrintToLog("%s(): rejected: non-fungible tokens are not yet activated (property type: %d)\n", __func__, prop_type);
        return (PKT_ERROR_SP -22);
    }

    if ('\0' == name[0]) {
        PrintToLog("%s(): rejected: property name must not be empty\n", __func__);
        return (PKT_ERROR_SP -37);
    }

    // ------------------------------------------

    CMPSPInfo::Entry newSP;
    newSP.issuer = sender;
    newSP.updateIssuer(block, tx_idx, sender);
    newSP.txid = txid;
    newSP.prop_type = prop_type;
    newSP.category.assign(category);
    newSP.subcategory.assign(subcategory);
    newSP.name.assign(name);
    newSP.url.assign(url);
    newSP.data.assign(data);
    if (prop_type != MSC_PROPERTY_TYPE_NONFUNGIBLE) {
        newSP.unique = false;
    } else {
        newSP.unique = true;
    }
    newSP.fixed = false;
    newSP.manual = true;
    newSP.creation_block = blockHash;
    newSP.update_block = newSP.creation_block;

    uint32_t propertyId = pDbSpInfo->putSP(ecosystem, newSP);
    assert(propertyId > 0);

    if (prop_type != MSC_PROPERTY_TYPE_NONFUNGIBLE) {
        PrintToLog("CREATED MANUAL PROPERTY id: %d admin: %s\n", propertyId, sender);
    } else {
        PrintToLog("CREATED MANUAL PROPERTY WITH NON-FUNGIBLE TOKENS id: %d admin: %s\n", propertyId, sender);
    }

    return 0;
}

/** Tx 55 */
int CMPTransaction::logicMath_GrantTokens(CBlockIndex* pindex, uint256& blockHash)
{
    if (pindex == nullptr) {
        PrintToLog("%s(): ERROR: block %d not in the active chain\n", __func__, block);
        return (PKT_ERROR_SP -20);
    }
    uint256 pindexBlockHash = pindex->GetBlockHash();

    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_TOKENS -22);
    }

    if (nValue <= 0 || MAX_INT_8_BYTES < nValue) {
        PrintToLog("%s(): rejected: value out of range or zero: %d\n", __func__, nValue);
        return (PKT_ERROR_TOKENS -23);
    }

    if (!IsPropertyIdValid(property)) {
        PrintToLog("%s(): rejected: property %d does not exist\n", __func__, property);
        return (PKT_ERROR_TOKENS -24);
    }

    CMPSPInfo::Entry sp;
    assert(pDbSpInfo->getSP(property, sp));

    if (!sp.manual) {
        PrintToLog("%s(): rejected: property %d is not managed\n", __func__, property);
        return (PKT_ERROR_TOKENS -42);
    }

    if (sender != sp.getIssuer(block)) {
        PrintToLog("%s(): rejected: sender %s is not issuer of property %d [issuer=%s]\n", __func__, sender, property, sp.issuer);
        return (PKT_ERROR_TOKENS -43);
    }

    int64_t nTotalTokens = getTotalTokens(property);
    if (nValue > (MAX_INT_8_BYTES - nTotalTokens)) {
        PrintToLog("%s(): rejected: no more than %s tokens can ever exist [%s + %s > %s]\n",
                __func__,
                FormatMP(property, MAX_INT_8_BYTES),
                FormatMP(property, nTotalTokens),
                FormatMP(property, nValue),
                FormatMP(property, MAX_INT_8_BYTES));
        return (PKT_ERROR_TOKENS -44);
    }

    // ------------------------------------------

    std::vector<int64_t> dataPt;
    dataPt.push_back(nValue);
    dataPt.push_back(0);
    sp.historicalData.insert(std::make_pair(txid, dataPt));
    sp.update_block = pindexBlockHash;

    // Persist the number of granted tokens
    assert(pDbSpInfo->updateSP(property, sp));

    // Move the tokens
    if (sp.unique) {
        std::pair<int64_t,int64_t> grantedRange = pDbNFT->CreateNonFungibleTokens(property, nValue, receiver, nonfungible_data);
        assert(grantedRange.first > 0);
        assert(grantedRange.second > 0);
        assert(grantedRange.second >= grantedRange.first);
        pDbTransactionList->RecordNonFungibleGrant(txid, grantedRange.first, grantedRange.second);
        PrintToLog("%s(): non-fungible: granted range %d to %d of property %d to %s\n", __func__, grantedRange.first, grantedRange.second, property, receiver);
    }
    assert(update_tally_map(receiver, property, nValue, BALANCE));

    /**
     * As long as the feature to disable the side effects of "granting tokens"
     * is not activated, "granting tokens" can trigger crowdsale participations.
     */
    if (!IsFeatureActivated(FEATURE_GRANTEFFECTS, block)) {
        // Is there an active crowdsale running from this recipient?
        logicHelper_CrowdsaleParticipation(blockHash);
    }

    NotifyTotalTokensChanged(property, block);

    return 0;
}

/** Tx 56 */
int CMPTransaction::logicMath_RevokeTokens(CBlockIndex* pindex)
{
    if (pindex == nullptr) {
        PrintToLog("%s(): ERROR: block %d not in the active chain\n", __func__, block);
        return (PKT_ERROR_TOKENS -20);
    }
    uint256 blockHash = pindex->GetBlockHash();

    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_TOKENS -22);
    }

    if (isPropertyNonFungible(property)) {
        PrintToLog("%s(): rejected: property %d is of type non-fungible\n", __func__, property);
        return (PKT_ERROR_TOKENS -27);
    }

    if (nValue <= 0 || MAX_INT_8_BYTES < nValue) {
        PrintToLog("%s(): rejected: value out of range or zero: %d\n", __func__, nValue);
        return (PKT_ERROR_TOKENS -23);
    }

    if (!IsPropertyIdValid(property)) {
        PrintToLog("%s(): rejected: property %d does not exist\n", __func__, property);
        return (PKT_ERROR_TOKENS -24);
    }

    CMPSPInfo::Entry sp;
    assert(pDbSpInfo->getSP(property, sp));

    if (!sp.manual) {
        PrintToLog("%s(): rejected: property %d is not managed\n", __func__, property);
        return (PKT_ERROR_TOKENS -42);
    }

    int64_t nBalance = GetTokenBalance(sender, property, BALANCE);
    if (nBalance < (int64_t) nValue) {
        PrintToLog("%s(): rejected: sender %s has insufficient balance of property %d [%s < %s]\n",
                __func__,
                sender,
                property,
                FormatMP(property, nBalance),
                FormatMP(property, nValue));
        return (PKT_ERROR_TOKENS -25);
    }

    // ------------------------------------------

    std::vector<int64_t> dataPt;
    dataPt.push_back(0);
    dataPt.push_back(nValue);
    sp.historicalData.insert(std::make_pair(txid, dataPt));
    sp.update_block = blockHash;

    assert(update_tally_map(sender, property, -nValue, BALANCE));
    assert(pDbSpInfo->updateSP(property, sp));

    NotifyTotalTokensChanged(property, block);

    return 0;
}

/** Tx 70 */
int CMPTransaction::logicMath_ChangeIssuer(CBlockIndex* pindex)
{
    if (pindex == nullptr) {
        PrintToLog("%s(): ERROR: block %d not in the active chain\n", __func__, block);
        return (PKT_ERROR_TOKENS -20);
    }
    uint256 blockHash = pindex->GetBlockHash();

    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_TOKENS -22);
    }

    if (isPropertyNonFungible(property)) {
        PrintToLog("%s(): rejected: property %d is of type non-fungible\n", __func__, property);
        return (PKT_ERROR_TOKENS -27);
    }

    if (!IsPropertyIdValid(property)) {
        PrintToLog("%s(): rejected: property %d does not exist\n", __func__, property);
        return (PKT_ERROR_TOKENS -24);
    }

    CMPSPInfo::Entry sp;
    assert(pDbSpInfo->getSP(property, sp));

    if (sender != sp.getIssuer(block)) {
        PrintToLog("%s(): rejected: sender %s is not issuer of property %d [issuer=%s]\n", __func__, sender, property, sp.issuer);
        return (PKT_ERROR_TOKENS -43);
    }

    if (nullptr != getCrowd(sender)) {
        PrintToLog("%s(): rejected: sender %s has an active crowdsale\n", __func__, sender);
        return (PKT_ERROR_TOKENS -39);
    }

    if (receiver.empty()) {
        PrintToLog("%s(): rejected: receiver is empty\n", __func__);
        return (PKT_ERROR_TOKENS -45);
    }

    if (nullptr != getCrowd(receiver)) {
        PrintToLog("%s(): rejected: receiver %s has an active crowdsale\n", __func__, receiver);
        return (PKT_ERROR_TOKENS -46);
    }

    // ------------------------------------------

    sp.updateIssuer(block, tx_idx, receiver);

    sp.issuer = receiver;
    sp.update_block = blockHash;

    assert(pDbSpInfo->updateSP(property, sp));

    return 0;
}

/** Tx 71 */
int CMPTransaction::logicMath_EnableFreezing(CBlockIndex* pindex)
{
    if (pindex == nullptr) {
        PrintToLog("%s(): ERROR: block %d not in the active chain\n", __func__, block);
        return (PKT_ERROR_TOKENS -20);
    }

    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_TOKENS -22);
    }

    if (!IsPropertyIdValid(property)) {
        PrintToLog("%s(): rejected: property %d does not exist\n", __func__, property);
        return (PKT_ERROR_TOKENS -24);
    }

    CMPSPInfo::Entry sp;
    assert(pDbSpInfo->getSP(property, sp));

    if (!sp.manual) {
        PrintToLog("%s(): rejected: property %d is not managed\n", __func__, property);
        return (PKT_ERROR_TOKENS -42);
    }

    if (sender != sp.getIssuer(block)) {
        PrintToLog("%s(): rejected: sender %s is not issuer of property %d [issuer=%s]\n", __func__, sender, property, sp.issuer);
        return (PKT_ERROR_TOKENS -43);
    }

    if (isFreezingEnabled(property, block)) {
        PrintToLog("%s(): rejected: freezing is already enabled for property %d\n", __func__, property);
        return (PKT_ERROR_TOKENS -49);
    }

    int liveBlock = 0;
    if (!IsFeatureActivated(FEATURE_FREEZENOTICE, block)) {
        liveBlock = block;
    } else {
        const CConsensusParams& params = ConsensusParams();
        liveBlock = params.OMNI_FREEZE_WAIT_PERIOD + block;
    }

    enableFreezing(property, liveBlock);

    return 0;
}

/** Tx 72 */
int CMPTransaction::logicMath_DisableFreezing(CBlockIndex* pindex)
{
    if (pindex == nullptr) {
        PrintToLog("%s(): ERROR: block %d not in the active chain\n", __func__, block);
        return (PKT_ERROR_TOKENS -20);
    }

    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_TOKENS -22);
    }

    if (!IsPropertyIdValid(property)) {
        PrintToLog("%s(): rejected: property %d does not exist\n", __func__, property);
        return (PKT_ERROR_TOKENS -24);
    }

    CMPSPInfo::Entry sp;
    assert(pDbSpInfo->getSP(property, sp));

    if (!sp.manual) {
        PrintToLog("%s(): rejected: property %d is not managed\n", __func__, property);
        return (PKT_ERROR_TOKENS -42);
    }

    if (sender != sp.getIssuer(block)) {
        PrintToLog("%s(): rejected: sender %s is not issuer of property %d [issuer=%s]\n", __func__, sender, property, sp.issuer);
        return (PKT_ERROR_TOKENS -43);
    }

    if (!isFreezingEnabled(property, block)) {
        PrintToLog("%s(): rejected: freezing is not enabled for property %d\n", __func__, property);
        return (PKT_ERROR_TOKENS -47);
    }

    disableFreezing(property);

    return 0;
}

/** Tx 185 */
int CMPTransaction::logicMath_FreezeTokens(CBlockIndex* pindex)
{
    if (pindex == nullptr) {
        PrintToLog("%s(): ERROR: block %d not in the active chain\n", __func__, block);
        return (PKT_ERROR_TOKENS -20);
    }

    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_TOKENS -22);
    }

    if (!IsPropertyIdValid(property)) {
        PrintToLog("%s(): rejected: property %d does not exist\n", __func__, property);
        return (PKT_ERROR_TOKENS -24);
    }

    CMPSPInfo::Entry sp;
    assert(pDbSpInfo->getSP(property, sp));

    if (!sp.manual) {
        PrintToLog("%s(): rejected: property %d is not managed\n", __func__, property);
        return (PKT_ERROR_TOKENS -42);
    }

    if (sender != sp.getIssuer(block)) {
        PrintToLog("%s(): rejected: sender %s is not issuer of property %d [issuer=%s]\n", __func__, sender, property, sp.issuer);
        return (PKT_ERROR_TOKENS -43);
    }

    if (!isFreezingEnabled(property, block)) {
        PrintToLog("%s(): rejected: freezing is not enabled for property %d\n", __func__, property);
        return (PKT_ERROR_TOKENS -47);
    }

    if (isAddressFrozen(receiver, property)) {
        PrintToLog("%s(): rejected: address %s is already frozen for property %d\n", __func__, receiver, property);
        return (PKT_ERROR_TOKENS -50);
    }

    freezeAddress(receiver, property);

    return 0;
}

/** Tx 186 */
int CMPTransaction::logicMath_UnfreezeTokens(CBlockIndex* pindex)
{
    if (pindex == nullptr) {
        PrintToLog("%s(): ERROR: block %d not in the active chain\n", __func__, block);
        return (PKT_ERROR_TOKENS -20);
    }

    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_TOKENS -22);
    }

    if (!IsPropertyIdValid(property)) {
        PrintToLog("%s(): rejected: property %d does not exist\n", __func__, property);
        return (PKT_ERROR_TOKENS -24);
    }

    CMPSPInfo::Entry sp;
    assert(pDbSpInfo->getSP(property, sp));

    if (!sp.manual) {
        PrintToLog("%s(): rejected: property %d is not managed\n", __func__, property);
        return (PKT_ERROR_TOKENS -42);
    }

    if (sender != sp.getIssuer(block)) {
        PrintToLog("%s(): rejected: sender %s is not issuer of property %d [issuer=%s]\n", __func__, sender, property, sp.issuer);
        return (PKT_ERROR_TOKENS -43);
    }

    if (!isFreezingEnabled(property, block)) {
        PrintToLog("%s(): rejected: freezing is not enabled for property %d\n", __func__, property);
        return (PKT_ERROR_TOKENS -47);
    }

    if (!isAddressFrozen(receiver, property)) {
        PrintToLog("%s(): rejected: address %s is not frozen for property %d\n", __func__, receiver, property);
        return (PKT_ERROR_TOKENS -48);
    }

    unfreezeAddress(receiver, property);

    return 0;
}

/** Tx 200 */
int CMPTransaction::logicMath_AnyData()
{
    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_ANYDATA -22);
    }

    return 0;
}

/** Tx 201 */
int CMPTransaction::logicMath_NonFungibleData()
{
    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR_SEND -22);
    }

    if (!isPropertyNonFungible(property)) {
        PrintToLog("%s(): rejected: property %d is not of type non-fungible\n", __func__, property);
        return (PKT_ERROR_TOKENS -27);
    }

    if (nonfungible_token_start <= 0 || MAX_INT_8_BYTES < nonfungible_token_start) {
        PrintToLog("%s(): rejected: non-fungible token range start value out of range or zero: %d", __func__, nonfungible_token_start);
        return (PKT_ERROR_SEND -23);
    }

    if (nonfungible_token_end <= 0 || MAX_INT_8_BYTES < nonfungible_token_end) {
        PrintToLog("%s(): rejected: non-fungible token range end value out of range or zero: %d", __func__, nonfungible_token_end);
        return (PKT_ERROR_SEND -23);
    }

    if (nonfungible_token_start > nonfungible_token_end) {
        PrintToLog("%s(): rejected: non-fungible token range start value: %d is less than or equal to range end value: %d", __func__, nonfungible_token_start, nonfungible_token_end);
        return (PKT_ERROR_SEND -23);
    }

    int64_t amount = (nonfungible_token_end - nonfungible_token_start) + 1;
    if (amount <= 0) {
        PrintToLog("%s(): rejected: non-fungible token range amount out of range or zero: %d", __func__, amount);
        return (PKT_ERROR_SEND -23);
    }

    if (!pDbSpInfo->hasSP(property)) {
        PrintToLog("%s(): rejected: property %d does not exist\n", __func__, property);
        return (PKT_ERROR_SEND -24);
    }

    NonFungibleStorage type = nonfungible_data_type ? NonFungibleStorage::IssuerData : NonFungibleStorage::HolderData;

    if (type == NonFungibleStorage::HolderData)
    {
        std::string rangeStartOwner = pDbNFT->GetNonFungibleTokenOwner(property, nonfungible_token_start);
        std::string rangeEndOwner = pDbNFT->GetNonFungibleTokenOwner(property, nonfungible_token_end);
        bool contiguous = pDbNFT->IsRangeContiguous(property, nonfungible_token_start, nonfungible_token_end);
        if (rangeStartOwner != sender || rangeEndOwner != sender || !contiguous) {
            PrintToLog("%s(): rejected: sender %s does not own the range data is being set on\n",
                    __func__,
                    sender);
            return (PKT_ERROR_SEND -26);
        }
    }
    else
    {
        CMPSPInfo::Entry sp;
        pDbSpInfo->getSP(property, sp);
        if (sp.getIssuer(block) != sender) {
            PrintToLog("%s(): rejected: sender %s is not the issuer of property %d\n",
                    __func__,
                    sender,
                    property);
        }
    }

    // ------------------------------------------

    pDbNFT->ChangeNonFungibleTokenData(property, nonfungible_token_start, nonfungible_token_end, nonfungible_data, type);

    return 0;
}


/** Tx 65533 */
int CMPTransaction::logicMath_Deactivation()
{
    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR -22);
    }

    // is sender authorized
    bool authorized = CheckDeactivationAuthorization(sender);

    PrintToLog("\t          sender: %s\n", sender);
    PrintToLog("\t      authorized: %s\n", authorized);

    if (!authorized) {
        PrintToLog("%s(): rejected: sender %s is not authorized to deactivate features\n", __func__, sender);
        return (PKT_ERROR -51);
    }

    // authorized, request feature deactivation
    bool DeactivationSuccess = DeactivateFeature(feature_id, block);

    if (!DeactivationSuccess) {
        PrintToLog("%s(): DeactivateFeature failed\n", __func__);
        return (PKT_ERROR -54);
    }

    // successful deactivation - did we deactivate the MetaDEx?  If so close out all trades
    if (feature_id == FEATURE_METADEX) {
        MetaDEx_SHUTDOWN();
    }
    if (feature_id == FEATURE_TRADEALLPAIRS) {
        MetaDEx_SHUTDOWN_ALLPAIR();
    }

    return 0;
}

/** Tx 65534 */
int CMPTransaction::logicMath_Activation()
{
    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR -22);
    }

    // is sender authorized - temporarily use alert auths but ## TO BE MOVED TO FOUNDATION P2SH KEY ##
    bool authorized = CheckActivationAuthorization(sender);

    PrintToLog("\t          sender: %s\n", sender);
    PrintToLog("\t      authorized: %s\n", authorized);

    if (!authorized) {
        PrintToLog("%s(): rejected: sender %s is not authorized for feature activations\n", __func__, sender);
        return (PKT_ERROR -51);
    }

    // authorized, request feature activation
    bool activationSuccess = ActivateFeature(feature_id, activation_block, min_client_version, block);

    if (!activationSuccess) {
        PrintToLog("%s(): ActivateFeature failed to activate this feature\n", __func__);
        return (PKT_ERROR -54);
    }

    return 0;
}

/** Tx 65535 */
int CMPTransaction::logicMath_Alert()
{
    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        PrintToLog("%s(): rejected: type %d or version %d not permitted for property %d at block %d\n",
                __func__,
                type,
                version,
                property,
                block);
        return (PKT_ERROR -22);
    }

    // is sender authorized?
    bool authorized = CheckAlertAuthorization(sender);

    PrintToLog("\t          sender: %s\n", sender);
    PrintToLog("\t      authorized: %s\n", authorized);

    if (!authorized) {
        PrintToLog("%s(): rejected: sender %s is not authorized for alerts\n", __func__, sender);
        return (PKT_ERROR -51);
    }

    if (alert_type == ALERT_CLIENT_VERSION_EXPIRY && OMNICORE_VERSION < alert_expiry) {
        // regular alert keys CANNOT be used to force a client upgrade on mainnet - at least 3 signatures from board/devs are required
        if (sender == "34kwkVRSvFVEoUwcQSgpQ4ZUasuZ54DJLD" || isNonMainNet()) {
            std::string msgText = "Client upgrade is required!  Shutting down due to unsupported consensus state!";
            PrintToLog(msgText);
            PrintToConsole(msgText);
            if (!gArgs.GetBoolArg("-overrideforcedshutdown", false)) {
                fs::path persistPath = GetDataDir() / "MP_persist";
                if (fs::exists(persistPath)) fs::remove_all(persistPath); // prevent the node being restarted without a reparse after forced shutdown
                AbortNode(msgText, msgText);
            }
        }
    }

    if (alert_type == 65535) { // set alert type to FFFF to clear previously sent alerts
        DeleteAlerts(sender);
    } else {
        AddAlert(sender, alert_type, alert_expiry, alert_text);
    }

    // we have a new alert, fire a notify event if needed
    DoWarning(alert_text);

    return 0;
}
