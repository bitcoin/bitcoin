#include <omnicore/dbstolist.h>

#include <omnicore/log.h>
#include <omnicore/sp.h>
#include <omnicore/walletutils.h>

#include <fs.h>
#include <interfaces/wallet.h>
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
#include <stdint.h>

#include <string>
#include <vector>

using mastercore::IsMyAddress;
using mastercore::isPropertyDivisible;

CMPSTOList::CMPSTOList(const fs::path& path, bool fWipe)
{
    leveldb::Status status = Open(path, fWipe);
    PrintToConsole("Loading send-to-owners database: %s\n", status.ToString());
}

CMPSTOList::~CMPSTOList()
{
    if (msc_debug_persistence) PrintToLog("CMPSTOList closed\n");
}

void CMPSTOList::getRecipients(const uint256 txid, std::string filterAddress, UniValue* recipientArray, uint64_t* total, uint64_t* numRecipients, interfaces::Wallet* iWallet)
{
    if (!pdb) return;

    bool filter = true; //default
    bool filterByWallet = true; //default
    bool filterByAddress = false; //default

    if (filterAddress == "*") filter = false;
    if ((filterAddress != "") && (filterAddress != "*")) {
        filterByWallet = false;
        filterByAddress = true;
    }

    // iterate through SDB, dropping all records where key is not filterAddress (if filtering)
    int count = 0;

    // the fee is variable based on version of STO - provide number of recipients and allow calling function to work out fee
    *numRecipients = 0;

    leveldb::Slice skey, svalue;
    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        skey = it->key();
        std::string recipientAddress = skey.ToString();
        svalue = it->value();
        std::string strValue = svalue.ToString();
        // see if txid is in the data
        size_t txidMatch = strValue.find(txid.ToString());
        if (txidMatch != std::string::npos) {
            ++*numRecipients;
            // the txid exists inside the data, this address was a recipient of this STO, check filter and add the details
            if (filter) {
                if (((filterByAddress) && (filterAddress == recipientAddress)) || ((filterByWallet) && (IsMyAddress(recipientAddress, iWallet)))) {
                } else {
                    continue;
                } // move on if no filter match (but counter still increased for fee)
            }
            std::vector<std::string> vstr;
            boost::split(vstr, strValue, boost::is_any_of(","), boost::token_compress_on);
            for (uint32_t i = 0; i < vstr.size(); i++) {
                std::vector<std::string> svstr;
                boost::split(svstr, vstr[i], boost::is_any_of(":"), boost::token_compress_on);
                if (4 == svstr.size()) {
                    if (svstr[0] == txid.ToString()) {
                        //add data to array
                        uint64_t amount = 0;
                        uint64_t propertyId = 0;
                        try {
                            amount = boost::lexical_cast<uint64_t>(svstr[3]);
                            propertyId = boost::lexical_cast<uint64_t>(svstr[2]);
                        } catch (const boost::bad_lexical_cast &e) {
                            PrintToLog("DEBUG STO - error in converting values from leveldb\n");
                            delete it;
                            return; //(something went wrong)
                        }
                        UniValue recipient(UniValue::VOBJ);
                        recipient.pushKV("address", recipientAddress);
                        if (isPropertyDivisible(propertyId)) {
                            recipient.pushKV("amount", FormatDivisibleMP(amount));
                        } else {
                            recipient.pushKV("amount", FormatIndivisibleMP(amount));
                        }
                        *total += amount;
                        recipientArray->push_back(recipient);
                        ++count;
                    }
                }
            }
        }
    }

    delete it;
    return;
}

std::string CMPSTOList::getMySTOReceipts(std::string filterAddress, interfaces::Wallet &iWallet)
{
    if (!pdb) return "";
    std::string mySTOReceipts = "";
    leveldb::Slice skey, svalue;
    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        skey = it->key();
        std::string recipientAddress = skey.ToString();
        if (!IsMyAddress(recipientAddress, &iWallet)) continue; // not ours, not interested
        if ((!filterAddress.empty()) && (filterAddress != recipientAddress)) continue; // not the filtered address
        // ours, get info
        svalue = it->value();
        std::string strValue = svalue.ToString();
        // break into individual receipts
        std::vector<std::string> vstr;
        boost::split(vstr, strValue, boost::is_any_of(","), boost::token_compress_on);
        for (uint32_t i = 0; i < vstr.size(); i++) {
            // add to array
            std::vector<std::string> svstr;
            boost::split(svstr, vstr[i], boost::is_any_of(":"), boost::token_compress_on);
            if (4 == svstr.size()) {
                size_t txidMatch = mySTOReceipts.find(svstr[0]);
                if (txidMatch == std::string::npos) mySTOReceipts += svstr[0] + ":" + svstr[1] + ":" + recipientAddress + ":" + svstr[2] + ",";
            }
        }
    }
    delete it;
    // above code will leave a trailing comma - strip it
    if (mySTOReceipts.size() > 0) mySTOReceipts.resize(mySTOReceipts.size() - 1);
    return mySTOReceipts;
}

/**
 * This function deletes records of STO receivers above/equal to a specific block from the STO database.
 *
 * Returns the number of records changed.
 */
int CMPSTOList::deleteAboveBlock(int blockNum)
{
    unsigned int n_found = 0;
    std::vector<std::string> vecSTORecords;
    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string newValue;
        std::string oldValue = it->value().ToString();
        bool needsUpdate = false;
        boost::split(vecSTORecords, oldValue, boost::is_any_of(","), boost::token_compress_on);
        for (uint32_t i = 0; i < vecSTORecords.size(); i++) {
            std::vector<std::string> vecSTORecordFields;
            boost::split(vecSTORecordFields, vecSTORecords[i], boost::is_any_of(":"), boost::token_compress_on);
            if (4 != vecSTORecordFields.size()) continue;
            if (atoi(vecSTORecordFields[1]) < blockNum) {
                newValue += vecSTORecords[i].append(","); // STO before the reorg, add data back to new value string
            } else {
                needsUpdate = true;
            }
        }
        if (needsUpdate) { // rewrite record with existing key and new value
            ++n_found;
            leveldb::Status status = pdb->Put(writeoptions, it->key().ToString(), newValue);
            PrintToLog("DEBUG STO - rewriting STO data after reorg\n");
            PrintToLog("STODBDEBUG : %s(): %s, line %d, file: %s\n", __FUNCTION__, status.ToString(), __LINE__, __FILE__);
        }
    }

    PrintToLog("%s(%d); stodb updated records= %d\n", __FUNCTION__, blockNum, n_found);

    delete it;

    return (n_found);
}

void CMPSTOList::printStats()
{
    PrintToLog("CMPSTOList stats: tWritten= %d , tRead= %d\n", nWritten, nRead);
}

void CMPSTOList::printAll()
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

bool CMPSTOList::exists(std::string address)
{
    if (!pdb) return false;

    std::string strValue;
    leveldb::Status status = pdb->Get(readoptions, address, &strValue);

    if (!status.ok()) {
        if (status.IsNotFound()) return false;
    }

    return true;
}

void CMPSTOList::recordSTOReceive(std::string address, const uint256 &txid, int nBlock, unsigned int propertyId, uint64_t amount)
{
    if (!pdb) return;

    bool addressExists = exists(address);
    if (addressExists) {
        // retrieve existing record
        std::vector<std::string> vstr;
        std::string strValue;
        leveldb::Status status = pdb->Get(readoptions, address, &strValue);
        if (status.ok()) {
            // add details to record
            // see if we are overwriting (check)
            size_t txidMatch = strValue.find(txid.ToString());
            if (txidMatch != std::string::npos) PrintToLog("STODEBUG : Duplicating entry for %s : %s\n", address, txid.ToString());

            const std::string key = address;
            const std::string newValue = strprintf("%s:%d:%u:%lu,", txid.ToString(), nBlock, propertyId, amount);
            strValue += newValue;
            // write updated record
            leveldb::Status status;
            if (pdb) {
                status = pdb->Put(writeoptions, key, strValue);
                PrintToLog("STODBDEBUG : %s(): %s, line %d, file: %s\n", __FUNCTION__, status.ToString(), __LINE__, __FILE__);
            }
        }
    } else {
        const std::string key = address;
        const std::string value = strprintf("%s:%d:%u:%lu,", txid.ToString(), nBlock, propertyId, amount);
        leveldb::Status status;
        if (pdb) {
            status = pdb->Put(writeoptions, key, value);
            PrintToLog("STODBDEBUG : %s(): %s, line %d, file: %s\n", __FUNCTION__, status.ToString(), __LINE__, __FILE__);
        }
    }
}
