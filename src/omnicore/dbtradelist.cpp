#include <omnicore/dbtradelist.h>

#include <omnicore/log.h>
#include <omnicore/mdex.h>
#include <omnicore/sp.h>

#include <amount.h>
#include <fs.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <tinyformat.h>

#include <univalue.h>

#include <leveldb/iterator.h>
#include <leveldb/slice.h>
#include <leveldb/status.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <stddef.h>

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

using mastercore::isPropertyDivisible;

CMPTradeList::CMPTradeList(const fs::path& path, bool fWipe)
{
    leveldb::Status status = Open(path, fWipe);
    PrintToConsole("Loading trades database: %s\n", status.ToString());
}

CMPTradeList::~CMPTradeList()
{
    if (msc_debug_persistence) PrintToLog("CMPTradeList closed\n");
}

void CMPTradeList::recordMatchedTrade(const uint256& txid1, const uint256& txid2, const std::string& address1, const std::string& address2, uint32_t prop1, uint32_t prop2, int64_t amount1, int64_t amount2, int blockNum, int64_t fee)
{
    if (!pdb) return;
    const std::string key = txid1.ToString() + "+" + txid2.ToString();
    const std::string value = strprintf("%s:%s:%u:%u:%lu:%lu:%d:%d", address1, address2, prop1, prop2, amount1, amount2, blockNum, fee);
    leveldb::Status status = pdb->Put(writeoptions, key, value);
    ++nWritten;
    if (msc_debug_tradedb) PrintToLog("%s: %s\n", __func__, status.ToString());
}

void CMPTradeList::recordNewTrade(const uint256& txid, const std::string& address, uint32_t propertyIdForSale, uint32_t propertyIdDesired, int blockNum, int blockIndex)
{
    if (!pdb) return;
    std::string strValue = strprintf("%s:%d:%d:%d:%d", address, propertyIdForSale, propertyIdDesired, blockNum, blockIndex);
    leveldb::Status status = pdb->Put(writeoptions, txid.ToString(), strValue);
    ++nWritten;
    if (msc_debug_tradedb) PrintToLog("%s: %s\n", __func__, status.ToString());
}

/**
 * This function deletes records of trades above/equal to a specific block from the trade database.
 *
 * Returns the number of records changed.
 */
int CMPTradeList::deleteAboveBlock(int blockNum)
{
    leveldb::Slice skey, svalue;
    unsigned int count = 0;
    std::vector<std::string> vstr;
    int block = 0;
    unsigned int n_found = 0;
    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        skey = it->key();
        svalue = it->value();
        ++count;
        std::string strvalue = it->value().ToString();
        boost::split(vstr, strvalue, boost::is_any_of(":"), boost::token_compress_on);
        if (7 == vstr.size()) block = atoi(vstr[6]); // trade matches have 7 tokens, key is txid+txid, only care about block
        if (5 == vstr.size()) block = atoi(vstr[3]); // trades have 5 tokens, key is txid, only care about block
        if (block >= blockNum) {
            ++n_found;
            PrintToLog("%s() DELETING FROM TRADEDB: %s=%s\n", __func__, skey.ToString(), svalue.ToString());
            pdb->Delete(writeoptions, skey);
        }
    }
    
    delete it;

    PrintToLog("%s(%d); tradedb n_found= %d\n", __func__, blockNum, n_found);

    return n_found;
}

void CMPTradeList::printStats()
{
    PrintToLog("CMPTradeList stats: tWritten= %d , tRead= %d\n", nWritten, nRead);
}

void CMPTradeList::printAll()
{
    int count = 0;
    leveldb::Slice skey, svalue;
    leveldb::Iterator* it = NewIterator();

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        skey = it->key();
        svalue = it->value();
        ++count;
        PrintToConsole("entry #%8d= %s:%s\n", count, skey.ToString(), svalue.ToString());
    }

    delete it;
}

bool CMPTradeList::getMatchingTrades(const uint256& txid, uint32_t propertyId, UniValue& tradeArray, int64_t& totalSold, int64_t& totalReceived)
{
    if (!pdb) return false;

    int count = 0;
    totalReceived = 0;
    totalSold = 0;

    std::vector<std::string> vstr;
    std::string txidStr = txid.ToString();
    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        // search key to see if this is a matching trade
        std::string strKey = it->key().ToString();
        std::string strValue = it->value().ToString();
        std::string matchTxid;
        size_t txidMatch = strKey.find(txidStr);
        if (txidMatch == std::string::npos) continue; // no match

        // sanity check key is the correct length for a matched trade
        if (strKey.length() != 129) continue;

        // obtain the txid of the match
        if (txidMatch == 0) {
            matchTxid = strKey.substr(65, 64);
        } else {
            matchTxid = strKey.substr(0, 64);
        }

        // ensure correct amount of tokens in value string
        boost::split(vstr, strValue, boost::is_any_of(":"), boost::token_compress_on);
        if (vstr.size() != 8) {
            PrintToLog("TRADEDB error - unexpected number of tokens in value (%s)\n", strValue);
            continue;
        }

        // decode the details from the value string
        std::string address1 = vstr[0];
        std::string address2 = vstr[1];
        uint32_t prop1 = boost::lexical_cast<uint32_t>(vstr[2]);
        uint32_t prop2 = boost::lexical_cast<uint32_t>(vstr[3]);
        int64_t amount1 = boost::lexical_cast<int64_t>(vstr[4]);
        int64_t amount2 = boost::lexical_cast<int64_t>(vstr[5]);
        int blockNum = atoi(vstr[6]);
        int64_t tradingFee = boost::lexical_cast<int64_t>(vstr[7]);

        std::string strAmount1 = FormatMP(prop1, amount1);
        std::string strAmount2 = FormatMP(prop2, amount2);
        std::string strTradingFee = FormatMP(prop2, tradingFee);
        std::string strAmount2PlusFee = FormatMP(prop2, amount2 + tradingFee);

        // populate trade object and add to the trade array, correcting for orientation of trade
        UniValue trade(UniValue::VOBJ);
        trade.pushKV("txid", matchTxid);
        trade.pushKV("block", blockNum);
        if (prop1 == propertyId) {
            trade.pushKV("address", address1);
            trade.pushKV("amountsold", strAmount1);
            trade.pushKV("amountreceived", strAmount2);
            trade.pushKV("tradingfee", strTradingFee);
            totalReceived += amount2;
            totalSold += amount1;
        } else {
            trade.pushKV("address", address2);
            trade.pushKV("amountsold", strAmount2PlusFee);
            trade.pushKV("amountreceived", strAmount1);
            trade.pushKV("tradingfee", FormatMP(prop1, 0)); // not the liquidity taker so no fee for this participant - include attribute for standardness
            totalReceived += amount1;
            totalSold += amount2;
        }
        tradeArray.push_back(trade);
        ++count;
    }

    // clean up
    delete it;
    if (count) {
        return true;
    } else {
        return false;
    }
}


// obtains a vector of txids where the supplied address participated in a trade (needed for gettradehistory_MP)
// optional property ID parameter will filter on propertyId transacted if supplied
// sorted by block then index
void CMPTradeList::getTradesForAddress(const std::string& address, std::vector<uint256>& vecTransactions, uint32_t propertyIdFilter)
{
    if (!pdb) return;

    std::map<std::string, uint256> mapTrades;
    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string strKey = it->key().ToString();
        std::string strValue = it->value().ToString();
        std::vector<std::string> vecValues;
        if (strKey.size() != 64) continue; // only interested in trades
        uint256 txid = uint256S(strKey);
        size_t addressMatch = strValue.find(address);
        if (addressMatch == std::string::npos) continue;
        boost::split(vecValues, strValue, boost::is_any_of(":"), boost::token_compress_on);
        if (vecValues.size() != 5) {
            PrintToLog("TRADEDB error - unexpected number of tokens in value (%s)\n", strValue);
            continue;
        }
        uint32_t propertyIdForSale = boost::lexical_cast<uint32_t>(vecValues[1]);
        uint32_t propertyIdDesired = boost::lexical_cast<uint32_t>(vecValues[2]);
        int64_t blockNum = boost::lexical_cast<uint32_t>(vecValues[3]);
        int64_t txIndex = boost::lexical_cast<uint32_t>(vecValues[4]);
        if (propertyIdFilter != 0 && propertyIdFilter != propertyIdForSale && propertyIdFilter != propertyIdDesired) continue;
        std::string sortKey = strprintf("%06d%010d", blockNum, txIndex);
        mapTrades.insert(std::make_pair(sortKey, txid));
    }
    delete it;

    for (std::map<std::string, uint256>::iterator it = mapTrades.begin(); it != mapTrades.end(); it++) {
        vecTransactions.push_back(it->second);
    }
}

static bool CompareTradePair(const std::pair<int64_t, UniValue>& firstJSONObj, const std::pair<int64_t, UniValue>& secondJSONObj)
{
    return firstJSONObj.first > secondJSONObj.first;
}

// obtains an array of matching trades with pricing and volume details for a pair sorted by blocknumber
void CMPTradeList::getTradesForPair(uint32_t propertyIdSideA, uint32_t propertyIdSideB, UniValue& responseArray, uint64_t count)
{
    if (!pdb) return;
    leveldb::Iterator* it = NewIterator();
    std::vector<std::pair<int64_t, UniValue> > vecResponse;
    bool propertyIdSideAIsDivisible = isPropertyDivisible(propertyIdSideA);
    bool propertyIdSideBIsDivisible = isPropertyDivisible(propertyIdSideB);
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string strKey = it->key().ToString();
        std::string strValue = it->value().ToString();
        std::vector<std::string> vecKeys;
        std::vector<std::string> vecValues;
        uint256 sellerTxid, matchingTxid;
        std::string sellerAddress, matchingAddress;
        int64_t amountReceived = 0, amountSold = 0;
        if (strKey.size() != 129) continue; // only interested in matches
        boost::split(vecKeys, strKey, boost::is_any_of("+"), boost::token_compress_on);
        boost::split(vecValues, strValue, boost::is_any_of(":"), boost::token_compress_on);
        if (vecKeys.size() != 2 || vecValues.size() != 8) {
            PrintToLog("TRADEDB error - unexpected number of tokens (%s:%s)\n", strKey, strValue);
            continue;
        }
        uint32_t tradePropertyIdSideA = boost::lexical_cast<uint32_t>(vecValues[2]);
        uint32_t tradePropertyIdSideB = boost::lexical_cast<uint32_t>(vecValues[3]);
        if (tradePropertyIdSideA == propertyIdSideA && tradePropertyIdSideB == propertyIdSideB) {
            sellerTxid.SetHex(vecKeys[1]);
            sellerAddress = vecValues[1];
            amountSold = boost::lexical_cast<int64_t>(vecValues[4]);
            matchingTxid.SetHex(vecKeys[0]);
            matchingAddress = vecValues[0];
            amountReceived = boost::lexical_cast<int64_t>(vecValues[5]);
        } else if (tradePropertyIdSideB == propertyIdSideA && tradePropertyIdSideA == propertyIdSideB) {
            sellerTxid.SetHex(vecKeys[0]);
            sellerAddress = vecValues[0];
            amountSold = boost::lexical_cast<int64_t>(vecValues[5]);
            matchingTxid.SetHex(vecKeys[1]);
            matchingAddress = vecValues[1];
            amountReceived = boost::lexical_cast<int64_t>(vecValues[4]);
        } else {
            continue;
        }

        rational_t unitPrice(amountReceived, amountSold);
        rational_t inversePrice(amountSold, amountReceived);
        if (!propertyIdSideAIsDivisible) unitPrice = unitPrice / COIN;
        if (!propertyIdSideBIsDivisible) inversePrice = inversePrice / COIN;
        std::string unitPriceStr = xToString(unitPrice); // TODO: not here!
        std::string inversePriceStr = xToString(inversePrice);

        int64_t blockNum = boost::lexical_cast<int64_t>(vecValues[6]);

        UniValue trade(UniValue::VOBJ);
        trade.pushKV("block", blockNum);
        trade.pushKV("unitprice", unitPriceStr);
        trade.pushKV("inverseprice", inversePriceStr);
        trade.pushKV("sellertxid", sellerTxid.GetHex());
        trade.pushKV("selleraddress", sellerAddress);
        if (propertyIdSideAIsDivisible) {
            trade.pushKV("amountsold", FormatDivisibleMP(amountSold));
        } else {
            trade.pushKV("amountsold", FormatIndivisibleMP(amountSold));
        }
        if (propertyIdSideBIsDivisible) {
            trade.pushKV("amountreceived", FormatDivisibleMP(amountReceived));
        } else {
            trade.pushKV("amountreceived", FormatIndivisibleMP(amountReceived));
        }
        trade.pushKV("matchingtxid", matchingTxid.GetHex());
        trade.pushKV("matchingaddress", matchingAddress);
        vecResponse.push_back(std::make_pair(blockNum, trade));
    }

    // sort the response most recent first before adding to the array
    std::sort(vecResponse.begin(), vecResponse.end(), CompareTradePair);
    uint64_t processed = 0;
    for (std::vector<std::pair<int64_t, UniValue> >::iterator it = vecResponse.begin(); it != vecResponse.end(); ++it) {
        responseArray.push_back(it->second);
        processed++;
        if (processed >= count) break;
    }

    std::vector<UniValue> responseArrayValues = responseArray.getValues();
    std::reverse(responseArrayValues.begin(), responseArrayValues.end());
    responseArray.clear();
    for (std::vector<UniValue>::iterator it = responseArrayValues.begin(); it != responseArrayValues.end(); ++it) {
        responseArray.push_back(*it);
    }

    delete it;
}

int CMPTradeList::getMPTradeCountTotal()
{
    int count = 0;
    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        ++count;
    }
    delete it;
    return count;
}
