#include "omnicore_pending.h"

#include "mastercore.h"
#include "mastercore_log.h"

#include "uint256.h"

#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"

#include <string>

using json_spirit::Object;
using json_spirit::Pair;
using json_spirit::Value;
using json_spirit::write_string;

using namespace mastercore;

/**
 * Adds a transaction to the pending map using supplied parameters
 */
void PendingAdd(const uint256& txid, const std::string& sendingAddress, const std::string& refAddress, uint16_t type, uint32_t propertyId, int64_t amount, uint32_t propertyIdDesired, int64_t amountDesired, int64_t action)
{
    Object txobj;
    std::string amountStr, amountDStr;
    bool divisible;
    bool divisibleDesired;
    txobj.push_back(Pair("txid", txid.GetHex()));
    txobj.push_back(Pair("sendingaddress", sendingAddress));
    if (!refAddress.empty()) txobj.push_back(Pair("referenceaddress", refAddress));
    txobj.push_back(Pair("confirmations", 0));
    txobj.push_back(Pair("type", c_strMasterProtocolTXType(type)));
    switch (type) {
        case MSC_TYPE_SIMPLE_SEND:
            txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
            divisible = isPropertyDivisible(propertyId);
            txobj.push_back(Pair("divisible", divisible));
            if (divisible) { amountStr = FormatDivisibleMP(amount); } else { amountStr = FormatIndivisibleMP(amount); }
            txobj.push_back(Pair("amount", amountStr));
        break;
        case MSC_TYPE_SEND_TO_OWNERS:
            txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
            divisible = isPropertyDivisible(propertyId);
            txobj.push_back(Pair("divisible", divisible));
            if (divisible) { amountStr = FormatDivisibleMP(amount); } else { amountStr = FormatIndivisibleMP(amount); }
            txobj.push_back(Pair("amount", amountStr));
        break;
        case MSC_TYPE_TRADE_OFFER:
            amountStr = FormatDivisibleMP(amount); // both amount for sale (either TMSC or MSC) and BTC desired are always divisible
            amountDStr = FormatDivisibleMP(amountDesired);
            txobj.push_back(Pair("amountoffered", amountStr));
            txobj.push_back(Pair("propertyidoffered", (uint64_t)propertyId));
            txobj.push_back(Pair("btcamountdesired", amountDStr));
            txobj.push_back(Pair("action", action));
        break;
        case MSC_TYPE_METADEX:
            divisible = isPropertyDivisible(propertyId);
            divisibleDesired = isPropertyDivisible(propertyIdDesired);
            if (divisible) { amountStr = FormatDivisibleMP(amount); } else { amountStr = FormatIndivisibleMP(amount); }
            if (divisibleDesired) { amountDStr = FormatDivisibleMP(amountDesired); } else { amountDStr = FormatIndivisibleMP(amountDesired); }
            txobj.push_back(Pair("amountoffered", amountStr));
            txobj.push_back(Pair("propertyidoffered", (uint64_t)propertyId));
            txobj.push_back(Pair("propertyidofferedisdivisible", divisible));
            txobj.push_back(Pair("amountdesired", amountDStr));
            txobj.push_back(Pair("propertyiddesired", (uint64_t)propertyIdDesired));
            txobj.push_back(Pair("propertyiddesiredisdivisible", divisibleDesired));
            txobj.push_back(Pair("action", action));
        break;
    }
    std::string txDesc = write_string(Value(txobj), false);
    CMPPending pending;
    if (msc_debug_pending) file_log("%s(%s,%s,%s,%d,%u,%ld,%u,%ld,%d,%s)\n", __FUNCTION__, txid.GetHex(), sendingAddress, refAddress,
                                        type, propertyId, amount, propertyIdDesired, amountDesired, action, txDesc);
    if (update_tally_map(sendingAddress, propertyId, -amount, PENDING)) {
        pending.src = sendingAddress;
        pending.amount = amount;
        pending.prop = propertyId;
        pending.desc = txDesc;
        pending.type = type;
        my_pending.insert(std::make_pair(txid, pending));
    }
}

/**
 * Deletes a transaction from the pending map and credits the amount back to the pending tally for the address
 *
 * NOTE: this is currently called for every bitcoin transaction prior to running through the parser
 */
void PendingDelete(const uint256& txid)
{
    PendingMap::iterator it = my_pending.find(txid);
    if (it != my_pending.end()) {
        CMPPending *p_pending = &(it->second);
        int64_t src_amount = getMPbalance(p_pending->src, p_pending->prop, PENDING);
        if (msc_debug_pending) file_log("%s(%s): amount=%d\n", __FUNCTION__, txid.GetHex(), src_amount);
        if (src_amount) update_tally_map(p_pending->src, p_pending->prop, p_pending->amount, PENDING);
        my_pending.erase(it);
    }
}

