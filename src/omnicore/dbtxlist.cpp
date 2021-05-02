#include <omnicore/dbtxlist.h>

#include <omnicore/activation.h>
#include <omnicore/dbtransaction.h>
#include <omnicore/dex.h>
#include <omnicore/log.h>
#include <omnicore/notifications.h>
#include <omnicore/omnicore.h>
#include <omnicore/tx.h>
#include <omnicore/utilsbitcoin.h>

#include <chain.h>
#include <chainparams.h>
#include <fs.h>
#include <validation.h>
#include <sync.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/system.h>
#include <util/strencodings.h>

#include <leveldb/iterator.h>
#include <leveldb/slice.h>
#include <leveldb/status.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

using mastercore::AddAlert;
using mastercore::CheckAlertAuthorization;
using mastercore::CheckExpiredAlerts;
using mastercore::CheckLiveActivations;
using mastercore::DeleteAlerts;
using mastercore::GetBlockIndex;
using mastercore::isNonMainNet;
using mastercore::pDbTransaction;

CMPTxList::CMPTxList(const fs::path& path, bool fWipe)
{
    leveldb::Status status = Open(path, fWipe);
    PrintToConsole("Loading tx meta-info database: %s\n", status.ToString());
}

CMPTxList::~CMPTxList()
{
    if (msc_debug_persistence) PrintToLog("CMPTxList closed\n");
}

void CMPTxList::recordTX(const uint256 &txid, bool fValid, int nBlock, unsigned int type, uint64_t nValue)
{
    if (!pdb) return;

    // overwrite detection, we should never be overwriting a tx, as that means we have redone something a second time
    // reorgs delete all txs from levelDB above reorg_chain_height
    if (exists(txid)) PrintToLog("LEVELDB TX OVERWRITE DETECTION - %s\n", txid.ToString());

    const std::string key = txid.ToString();
    const std::string value = strprintf("%u:%d:%u:%lu", fValid ? 1 : 0, nBlock, type, nValue);
    leveldb::Status status;

    PrintToLog("%s(%s, valid=%s, block= %d, type= %d, value= %lu)\n",
            __func__, txid.ToString(), fValid ? "YES" : "NO", nBlock, type, nValue);

    status = pdb->Put(writeoptions, key, value);
    ++nWritten;
}

void CMPTxList::recordPaymentTX(const uint256& txid, bool fValid, int nBlock, unsigned int vout, unsigned int propertyId, uint64_t nValue, std::string buyer, std::string seller)
{
    if (!pdb) return;

    // Prep - setup vars
    unsigned int type = 99999999;
    uint64_t numberOfPayments = 1;
    unsigned int paymentNumber = 1;
    uint64_t existingNumberOfPayments = 0;

    // Step 1 - Check TXList to see if this payment TXID exists
    bool paymentEntryExists = exists(txid);

    // Step 2a - If doesn't exist leave number of payments & paymentNumber set to 1
    // Step 2b - If does exist add +1 to existing number of payments and set this paymentNumber as new numberOfPayments
    if (paymentEntryExists) {
        //retrieve old numberOfPayments
        std::vector<std::string> vstr;
        std::string strValue;
        leveldb::Status status = pdb->Get(readoptions, txid.ToString(), &strValue);
        if (status.ok()) {
            // parse the string returned
            boost::split(vstr, strValue, boost::is_any_of(":"), boost::token_compress_on);

            // obtain the existing number of payments
            if (4 <= vstr.size()) {
                existingNumberOfPayments = atoi(vstr[3]);
                paymentNumber = existingNumberOfPayments + 1;
                numberOfPayments = existingNumberOfPayments + 1;
            }
        }
    }

    // Step 3 - Create new/update master record for payment tx in TXList
    const std::string key = txid.ToString();
    const std::string value = strprintf("%u:%d:%u:%lu", fValid ? 1 : 0, nBlock, type, numberOfPayments);
    leveldb::Status status;
    PrintToLog("DEXPAYDEBUG : Writing master record %s(%s, valid=%s, block= %d, type= %d, number of payments= %lu)\n", __func__, txid.ToString(), fValid ? "YES" : "NO", nBlock, type, numberOfPayments);
    status = pdb->Put(writeoptions, key, value);

    // Step 4 - Write sub-record with payment details
    const std::string txidStr = txid.ToString();
    const std::string subKey = STR_PAYMENT_SUBKEY_TXID_PAYMENT_COMBO(txidStr, paymentNumber);
    const std::string subValue = strprintf("%d:%s:%s:%d:%lu", vout, buyer, seller, propertyId, nValue);
    leveldb::Status subStatus;
    PrintToLog("DEXPAYDEBUG : Writing sub-record %s with value %s\n", subKey, subValue);
    subStatus = pdb->Put(writeoptions, subKey, subValue);
}

void CMPTxList::recordMetaDExCancelTX(const uint256& txidMaster, const uint256& txidSub, bool fValid, int nBlock, unsigned int propertyId, uint64_t nValue)
{
    if (!pdb) return;

    // Prep - setup vars
    unsigned int type = 99992104;
    unsigned int refNumber = 1;
    uint64_t existingAffectedTXCount = 0;
    std::string txidMasterStr = txidMaster.ToString() + "-C";

    // Step 1 - Check TXList to see if this cancel TXID exists
    // Step 2a - If doesn't exist leave number of affected txs & ref set to 1
    // Step 2b - If does exist add +1 to existing ref and set this ref as new number of affected
    std::vector<std::string> vstr;
    std::string strValue;
    leveldb::Status status = pdb->Get(readoptions, txidMasterStr, &strValue);
    if (status.ok()) {
        // parse the string returned
        boost::split(vstr, strValue, boost::is_any_of(":"), boost::token_compress_on);

        // obtain the existing affected tx count
        if (4 <= vstr.size()) {
            existingAffectedTXCount = atoi(vstr[3]);
            refNumber = existingAffectedTXCount + 1;
        }
    }

    // Step 3 - Create new/update master record for cancel tx in TXList
    const std::string key = txidMasterStr;
    const std::string value = strprintf("%u:%d:%u:%lu", fValid ? 1 : 0, nBlock, type, refNumber);
    PrintToLog("METADEXCANCELDEBUG : Writing master record %s(%s, valid=%s, block= %d, type= %d, number of affected transactions= %d)\n", __func__, txidMaster.ToString(), fValid ? "YES" : "NO", nBlock, type, refNumber);
    status = pdb->Put(writeoptions, key, value);

    // Step 4 - Write sub-record with cancel details
    const std::string txidStr = txidMaster.ToString() + "-C";
    const std::string subKey = STR_REF_SUBKEY_TXID_REF_COMBO(txidStr, refNumber);
    const std::string subValue = strprintf("%s:%d:%lu", txidSub.ToString(), propertyId, nValue);
    PrintToLog("METADEXCANCELDEBUG : Writing sub-record %s with value %s\n", subKey, subValue);
    status = pdb->Put(writeoptions, subKey, subValue);
    if (msc_debug_txdb) PrintToLog("%s(): store: %s=%s, status: %s\n", __func__, subKey, subValue, status.ToString());
}


/**
 * Records a "send all" sub record.
 */
void CMPTxList::recordSendAllSubRecord(const uint256& txid, int subRecordNumber, uint32_t propertyId, int64_t nValue)
{
    std::string strKey = strprintf("%s-%d", txid.ToString(), subRecordNumber);
    std::string strValue = strprintf("%d:%d", propertyId, nValue);

    leveldb::Status status = pdb->Put(writeoptions, strKey, strValue);
    ++nWritten;
    if (msc_debug_txdb) PrintToLog("%s(): store: %s=%s, status: %s\n", __func__, strKey, strValue, status.ToString());
}


std::string CMPTxList::getKeyValue(std::string key)
{
    if (!pdb) return "";
    std::string strValue;
    leveldb::Status status = pdb->Get(readoptions, key, &strValue);
    if (status.ok()) {
        return strValue;
    } else {
        return "";
    }
}

uint256 CMPTxList::findMetaDExCancel(const uint256 txid)
{
    std::vector<std::string> vstr;
    std::string txidStr = txid.ToString();
    leveldb::Slice skey, svalue;
    uint256 cancelTxid;
    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        skey = it->key();
        svalue = it->value();
        std::string svalueStr = svalue.ToString();
        boost::split(vstr, svalueStr, boost::is_any_of(":"), boost::token_compress_on);
        // obtain the existing affected tx count
        if (3 <= vstr.size()) {
            if (vstr[0] == txidStr) {
                delete it;
                cancelTxid.SetHex(skey.ToString());
                return cancelTxid;
            }
        }
    }

    delete it;
    return uint256();
}

/**
 * Returns the number of sub records.
 */
int CMPTxList::getNumberOfSubRecords(const uint256& txid)
{
    int numberOfSubRecords = 0;

    std::string strValue;
    leveldb::Status status = pdb->Get(readoptions, txid.ToString(), &strValue);
    if (status.ok()) {
        std::vector<std::string> vstr;
        boost::split(vstr, strValue, boost::is_any_of(":"), boost::token_compress_on);
        if (4 <= vstr.size()) {
            numberOfSubRecords = boost::lexical_cast<int>(vstr[3]);
        }
    }

    return numberOfSubRecords;
}

int CMPTxList::getNumberOfMetaDExCancels(const uint256 txid)
{
    if (!pdb) return 0;
    int numberOfCancels = 0;
    std::vector<std::string> vstr;
    std::string strValue;
    leveldb::Status status = pdb->Get(readoptions, txid.ToString() + "-C", &strValue);
    if (status.ok()) {
        // parse the string returned
        boost::split(vstr, strValue, boost::is_any_of(":"), boost::token_compress_on);
        // obtain the number of cancels
        if (4 <= vstr.size()) {
            numberOfCancels = atoi(vstr[3]);
        }
    }
    return numberOfCancels;
}

bool CMPTxList::getPurchaseDetails(const uint256 txid, int purchaseNumber, std::string* buyer, std::string* seller, uint64_t* vout, uint64_t* propertyId, uint64_t* nValue)
{
    if (!pdb) return 0;
    std::vector<std::string> vstr;
    std::string strValue;
    leveldb::Status status = pdb->Get(readoptions, txid.ToString() + "-" + std::to_string(purchaseNumber), &strValue);
    if (status.ok()) {
        // parse the string returned
        boost::split(vstr, strValue, boost::is_any_of(":"), boost::token_compress_on);
        // obtain the requisite details
        if (5 == vstr.size()) {
            *vout = atoi(vstr[0]);
            *buyer = vstr[1];
            *seller = vstr[2];
            *propertyId = atoi64(vstr[3]);
            *nValue = boost::lexical_cast<boost::uint64_t>(vstr[4]);
            return true;
        }
    }
    return false;
}

/**
 * Retrieves details about a "send all" record.
 */
bool CMPTxList::getSendAllDetails(const uint256& txid, int subSend, uint32_t& propertyId, int64_t& amount)
{
    std::string strKey = strprintf("%s-%d", txid.ToString(), subSend);
    std::string strValue;
    leveldb::Status status = pdb->Get(readoptions, strKey, &strValue);
    if (status.ok()) {
        std::vector<std::string> vstr;
        boost::split(vstr, strValue, boost::is_any_of(":"), boost::token_compress_on);
        if (2 == vstr.size()) {
            propertyId = boost::lexical_cast<uint32_t>(vstr[0]);
            amount = boost::lexical_cast<int64_t>(vstr[1]);
            return true;
        }
    }
    return false;
}

int CMPTxList::getMPTransactionCountTotal()
{
    int count = 0;
    leveldb::Slice skey, svalue;
    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        skey = it->key();
        if (skey.ToString().length() == 64) {
            ++count;
        } //extra entries for cancels and purchases are more than 64 chars long
    }
    delete it;
    return count;
}

int CMPTxList::getMPTransactionCountBlock(int block)
{
    int count = 0;
    leveldb::Slice skey, svalue;
    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        skey = it->key();
        svalue = it->value();
        if (skey.ToString().length() == 64) {
            std::string strValue = svalue.ToString();
            std::vector<std::string> vstr;
            boost::split(vstr, strValue, boost::is_any_of(":"), boost::token_compress_on);
            if (4 == vstr.size()) {
                if (atoi(vstr[1]) == block) {
                    ++count;
                }
            }
        }
    }
    delete it;
    return count;
}

/** Returns a list of all Omni transactions in the given block range. */
int CMPTxList::GetOmniTxsInBlockRange(int blockFirst, int blockLast, std::set<uint256>& retTxs)
{
    int count = 0;
    leveldb::Iterator* it = NewIterator();
    
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        const leveldb::Slice& sKey = it->key();
        const leveldb::Slice& sValue = it->value();

        if (sKey.ToString().length() == 64) {
            const std::string& strValue = sValue.ToString();
            std::vector<std::string> vStr;
            boost::split(vStr, strValue, boost::is_any_of(":"), boost::token_compress_on);
            if (4 == vStr.size()) {
                int blockCurrent = atoi(vStr[1]);
                if (blockCurrent >= blockFirst && blockCurrent <= blockLast) {
                    retTxs.insert(uint256S(sKey.ToString()));                    
                    ++count;
                }
            }
        }
    }

    delete it;
    return count;
}

/*
 * Gets the DB version from txlistdb
 *
 * Returns the current version
 */
int CMPTxList::getDBVersion()
{
    std::string strValue;
    int verDB = 0;

    leveldb::Status status = pdb->Get(readoptions, "dbversion", &strValue);
    if (status.ok()) {
        verDB = boost::lexical_cast<uint64_t>(strValue);
    }

    if (msc_debug_txdb) PrintToLog("%s(): dbversion %s status %s, line %d, file: %s\n", __func__, strValue, status.ToString(), __LINE__, __FILE__);

    return verDB;
}

/*
 * Sets the DB version for txlistdb
 *
 * Returns the current version after update
 */
int CMPTxList::setDBVersion()
{
    std::string verStr = boost::lexical_cast<std::string>(DB_VERSION);
    leveldb::Status status = pdb->Put(writeoptions, "dbversion", verStr);

    if (msc_debug_txdb) PrintToLog("%s(): dbversion %s status %s, line %d, file: %s\n", __func__, verStr, status.ToString(), __LINE__, __FILE__);

    return getDBVersion();
}

std::pair<int64_t,int64_t> CMPTxList::GetNonFungibleGrant(const uint256& txid)
{
    std::string strKey = strprintf("%s-UG", txid.ToString());
    std::string strValue;
    leveldb::Status status = pdb->Get(readoptions, strKey, &strValue);
    if (status.ok()) {
        std::vector<std::string> vstr;
        boost::split(vstr, strValue, boost::is_any_of("-"), boost::token_compress_on);
        if (2 == vstr.size()) {
            return std::make_pair(boost::lexical_cast<int64_t>(vstr[0]), boost::lexical_cast<int64_t>(vstr[1]));
        }
    }
    return std::make_pair(0,0);
}

void CMPTxList::RecordNonFungibleGrant(const uint256& txid, int64_t start, int64_t end)
{
    assert(pdb);

    const std::string key = txid.ToString() + "-UG";
    const std::string value = strprintf("%d-%d", start, end);

    leveldb::Status status = pdb->Put(writeoptions, key, value);
    PrintToLog("%s(): Writing Non-Fungible Grant range %s:%d-%d (%s), line %d, file: %s\n", __FUNCTION__, key, start, end, status.ToString(), __LINE__, __FILE__);
}

bool CMPTxList::exists(const uint256 &txid)
{
    if (!pdb) return false;

    std::string strValue;
    leveldb::Status status = pdb->Get(readoptions, txid.ToString(), &strValue);

    if (!status.ok()) {
        if (status.IsNotFound()) return false;
    }

    return true;
}

bool CMPTxList::getTX(const uint256 &txid, std::string& value)
{
    leveldb::Status status = pdb->Get(readoptions, txid.ToString(), &value);
    ++nRead;

    if (status.ok()) {
        return true;
    }

    return false;
}

// call it like so (variable # of parameters):
// int block = 0;
// ...
// uint64_t nNew = 0;
//
// if (getValidMPTX(txid, &block, &type, &nNew)) // if true -- the TX is a valid MP TX
//
bool CMPTxList::getValidMPTX(const uint256& txid, int* block, unsigned int* type, uint64_t* nAmended)
{
    std::string result;
    int validity = 0;

    if (msc_debug_txdb) PrintToLog("%s()\n", __func__);

    if (!pdb) return false;

    if (!getTX(txid, result)) return false;

    // parse the string returned, find the validity flag/bit & other parameters
    std::vector<std::string> vstr;
    boost::split(vstr, result, boost::is_any_of(":"), boost::token_compress_on);

    if (msc_debug_txdb) PrintToLog("%s() size=%lu : %s\n", __func__, vstr.size(), result);

    if (1 <= vstr.size()) validity = atoi(vstr[0]);

    if (block) {
        if (2 <= vstr.size()) *block = atoi(vstr[1]);
        else *block = 0;
    }

    if (type) {
        if (3 <= vstr.size()) *type = atoi(vstr[2]);
        else *type = 0;
    }

    if (nAmended) {
        if (4 <= vstr.size()) *nAmended = boost::lexical_cast<boost::uint64_t>(vstr[3]);
        else nAmended = 0;
    }

    if (msc_debug_txdb) printStats();

    if ((int) 0 == validity) return false;

    return true;
}

std::set<int> CMPTxList::GetSeedBlocks(int startHeight, int endHeight)
{
    std::set<int> setSeedBlocks;

    if (!pdb) return setSeedBlocks;

    leveldb::Iterator* it = NewIterator();

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string itData = it->value().ToString();
        std::vector<std::string> vstr;
        boost::split(vstr, itData, boost::is_any_of(":"), boost::token_compress_on);
        if (4 != vstr.size()) continue; // unexpected number of tokens
        int block = atoi(vstr[1]);
        if (block >= startHeight && block <= endHeight) {
            setSeedBlocks.insert(block);
        }
    }

    delete it;

    return setSeedBlocks;
}

void CMPTxList::LoadAlerts(int blockHeight)
{
    if (!pdb) return;
    leveldb::Slice skey, svalue;
    leveldb::Iterator* it = NewIterator();

    std::vector<std::pair<int64_t, uint256> > loadOrder;

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string itData = it->value().ToString();
        std::vector<std::string> vstr;
        boost::split(vstr, itData, boost::is_any_of(":"), boost::token_compress_on);
        if (4 != vstr.size()) continue; // unexpected number of tokens
        if (atoi(vstr[2]) != OMNICORE_MESSAGE_TYPE_ALERT || atoi(vstr[0]) != 1) continue; // not a valid alert
        uint256 txid = uint256S(it->key().ToString());
        loadOrder.push_back(std::make_pair(atoi(vstr[1]), txid));
    }

    std::sort(loadOrder.begin(), loadOrder.end());

    for (std::vector<std::pair<int64_t, uint256> >::iterator it = loadOrder.begin(); it != loadOrder.end(); ++it) {
        uint256 txid = (*it).second;
        uint256 blockHash;
        CTransactionRef wtx;
        CMPTransaction mp_obj;
        if (!GetTransaction(txid, wtx, Params().GetConsensus(), blockHash)) {
            PrintToLog("ERROR: While loading alert %s: tx in levelDB but does not exist.\n", txid.GetHex());
            continue;
        }
        CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
        int currentBlockHeight = pBlockIndex->nHeight;
        if (currentBlockHeight > blockHeight) {
            // skipping, because it's in the future
            continue;
        }
        if (0 != ParseTransaction(*wtx, blockHeight, 0, mp_obj)) {
            PrintToLog("ERROR: While loading alert %s: failed ParseTransaction.\n", txid.GetHex());
            continue;
        }
        if (!mp_obj.interpret_Transaction()) {
            PrintToLog("ERROR: While loading alert %s: failed interpret_Transaction.\n", txid.GetHex());
            continue;
        }
        if (OMNICORE_MESSAGE_TYPE_ALERT != mp_obj.getType()) {
            PrintToLog("ERROR: While loading alert %s: levelDB type mismatch, not an alert.\n", txid.GetHex());
            continue;
        }
        if (!CheckAlertAuthorization(mp_obj.getSender())) {
            PrintToLog("ERROR: While loading alert %s: sender is not authorized to send alerts.\n", txid.GetHex());
            continue;
        }

        if (mp_obj.getAlertType() == 65535) { // set alert type to FFFF to clear previously sent alerts
            DeleteAlerts(mp_obj.getSender());
        } else {
            AddAlert(mp_obj.getSender(), mp_obj.getAlertType(), mp_obj.getAlertExpiry(), mp_obj.getAlertMessage());
        }
    }

    delete it;
    int64_t blockTime = 0;
    {
        LOCK(cs_main);
        CBlockIndex* pBlockIndex = ::ChainActive()[blockHeight - 1];
        if (pBlockIndex != nullptr) {
            blockTime = pBlockIndex->GetBlockTime();
        }
    }
    if (blockTime > 0) {
        CheckExpiredAlerts(blockHeight, blockTime);
    }
}

void CMPTxList::LoadActivations(int blockHeight)
{
    if (!pdb) return;

    leveldb::Slice skey, svalue;
    leveldb::Iterator* it = NewIterator();

    PrintToLog("Loading feature activations from levelDB\n");

    std::vector<std::pair<int64_t, uint256> > loadOrder;

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string itData = it->value().ToString();
        std::vector<std::string> vstr;
        boost::split(vstr, itData, boost::is_any_of(":"), boost::token_compress_on);
        if (4 != vstr.size()) continue; // unexpected number of tokens
        if (atoi(vstr[2]) != OMNICORE_MESSAGE_TYPE_ACTIVATION || atoi(vstr[0]) != 1) continue; // we only care about valid activations
        uint256 txid = uint256S(it->key().ToString());
        loadOrder.push_back(std::make_pair(atoi(vstr[1]), txid));
    }

    std::sort(loadOrder.begin(), loadOrder.end());

    for (std::vector<std::pair<int64_t, uint256> >::iterator it = loadOrder.begin(); it != loadOrder.end(); ++it) {
        uint256 hash = (*it).second;
        uint256 blockHash;
        CTransactionRef wtx;
        CMPTransaction mp_obj;

        if (!GetTransaction(hash, wtx, Params().GetConsensus(), blockHash)) {
            PrintToLog("ERROR: While loading activation transaction %s: tx in levelDB but does not exist.\n", hash.GetHex());
            continue;
        }
        if (blockHash.IsNull() || (nullptr == GetBlockIndex(blockHash))) {
            PrintToLog("ERROR: While loading activation transaction %s: failed to retrieve block hash.\n", hash.GetHex());
            continue;
        }
        CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
        if (nullptr == pBlockIndex) {
            PrintToLog("ERROR: While loading activation transaction %s: failed to retrieve block index.\n", hash.GetHex());
            continue;
        }
        int currentBlockHeight = pBlockIndex->nHeight;
        if (currentBlockHeight > blockHeight) {
            // skipping, because it's in the future
            continue;
        }
        if (0 != ParseTransaction(*wtx, currentBlockHeight, 0, mp_obj)) {
            PrintToLog("ERROR: While loading activation transaction %s: failed ParseTransaction.\n", hash.GetHex());
            continue;
        }
        if (!mp_obj.interpret_Transaction()) {
            PrintToLog("ERROR: While loading activation transaction %s: failed interpret_Transaction.\n", hash.GetHex());
            continue;
        }
        if (OMNICORE_MESSAGE_TYPE_ACTIVATION != mp_obj.getType()) {
            PrintToLog("ERROR: While loading activation transaction %s: levelDB type mismatch, not an activation.\n", hash.GetHex());
            continue;
        }
        mp_obj.unlockLogic();
        if (0 != mp_obj.interpretPacket()) {
            PrintToLog("ERROR: While loading activation transaction %s: non-zero return from interpretPacket\n", hash.GetHex());
            continue;
        }
    }
    delete it;
    CheckLiveActivations(blockHeight);

    // This alert never expires as long as custom activations are used
    if (gArgs.IsArgSet("-omniactivationallowsender") || gArgs.IsArgSet("-omniactivationignoresender")) {
        AddAlert("omnicore", ALERT_CLIENT_VERSION_EXPIRY, std::numeric_limits<uint32_t>::max(),
                "Authorization for feature activation has been modified.  Data provided by this client should not be trusted.");
    }
}

bool CMPTxList::LoadFreezeState(int blockHeight)
{
    assert(pdb);

    std::vector<std::pair<std::string, uint256> > loadOrder;
    int txnsLoaded = 0;
    leveldb::Iterator* it = NewIterator();
    PrintToLog("Loading freeze state from levelDB\n");

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string itData = it->value().ToString();
        std::vector<std::string> vstr;
        boost::split(vstr, itData, boost::is_any_of(":"), boost::token_compress_on);
        if (4 != vstr.size()) continue;
        uint16_t txtype = atoi(vstr[2]);
        if (txtype != MSC_TYPE_FREEZE_PROPERTY_TOKENS && txtype != MSC_TYPE_UNFREEZE_PROPERTY_TOKENS &&
                txtype != MSC_TYPE_ENABLE_FREEZING && txtype != MSC_TYPE_DISABLE_FREEZING) continue;
        if (atoi(vstr[0]) != 1) continue; // invalid, ignore
        uint256 txid = uint256S(it->key().ToString());
        int txPosition = pDbTransaction->FetchTransactionPosition(txid);
        std::string sortKey = strprintf("%06d%010d", atoi(vstr[1]), txPosition);
        loadOrder.push_back(std::make_pair(sortKey, txid));
    }

    delete it;

    std::sort(loadOrder.begin(), loadOrder.end());

    for (std::vector<std::pair<std::string, uint256> >::iterator it = loadOrder.begin(); it != loadOrder.end(); ++it) {
        uint256 hash = (*it).second;
        uint256 blockHash;
        CTransactionRef wtx;
        CMPTransaction mp_obj;
        if (!GetTransaction(hash, wtx, Params().GetConsensus(), blockHash)) {
            PrintToLog("ERROR: While loading freeze transaction %s: tx in levelDB but does not exist.\n", hash.GetHex());
            return false;
        }
        if (blockHash.IsNull() || (nullptr == GetBlockIndex(blockHash))) {
            PrintToLog("ERROR: While loading freeze transaction %s: failed to retrieve block hash.\n", hash.GetHex());
            return false;
        }
        CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
        if (nullptr == pBlockIndex) {
            PrintToLog("ERROR: While loading freeze transaction %s: failed to retrieve block index.\n", hash.GetHex());
            return false;
        }
        int currentBlockHeight = pBlockIndex->nHeight;
        if (currentBlockHeight > blockHeight) {
            // skipping, because it's in the future
            continue;
        }
        if (0 != ParseTransaction(*wtx, currentBlockHeight, 0, mp_obj)) {
            PrintToLog("ERROR: While loading freeze transaction %s: failed ParseTransaction.\n", hash.GetHex());
            return false;
        }
        if (!mp_obj.interpret_Transaction()) {
            PrintToLog("ERROR: While loading freeze transaction %s: failed interpret_Transaction.\n", hash.GetHex());
            return false;
        }
        if (MSC_TYPE_FREEZE_PROPERTY_TOKENS != mp_obj.getType() && MSC_TYPE_UNFREEZE_PROPERTY_TOKENS != mp_obj.getType() &&
                MSC_TYPE_ENABLE_FREEZING != mp_obj.getType() && MSC_TYPE_DISABLE_FREEZING != mp_obj.getType()) {
            PrintToLog("ERROR: While loading freeze transaction %s: levelDB type mismatch, not a freeze transaction.\n", hash.GetHex());
            return false;
        }
        mp_obj.unlockLogic();
        if (0 != mp_obj.interpretPacket()) {
            PrintToLog("ERROR: While loading freeze transaction %s: non-zero return from interpretPacket\n", hash.GetHex());
            return false;
        }
        txnsLoaded++;
    }

    if (blockHeight > 497000 && !isNonMainNet()) {
        assert(txnsLoaded >= 2); // sanity check against a failure to properly load the freeze state
    }

    return true;
}

bool CMPTxList::CheckForFreezeTxs(int blockHeight)
{
    assert(pdb);

    leveldb::Iterator* it = NewIterator();

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string itData = it->value().ToString();
        std::vector<std::string> vstr;
        boost::split(vstr, itData, boost::is_any_of(":"), boost::token_compress_on);
        if (4 != vstr.size()) continue;
        int block = atoi(vstr[1]);
        if (block < blockHeight) continue;
        uint16_t txtype = atoi(vstr[2]);
        if (txtype == MSC_TYPE_FREEZE_PROPERTY_TOKENS || txtype == MSC_TYPE_UNFREEZE_PROPERTY_TOKENS ||
                txtype == MSC_TYPE_ENABLE_FREEZING || txtype == MSC_TYPE_DISABLE_FREEZING) {
            delete it;
            return true;
        }
    }

    delete it;
    return false;
}

void CMPTxList::printStats()
{
    PrintToLog("CMPTxList stats: nWritten= %d , nRead= %d\n", nWritten, nRead);
}

void CMPTxList::printAll()
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

// figure out if there was at least 1 Master Protocol transaction within the block range, or a block if starting equals ending
// block numbers are inclusive
// pass in bDeleteFound = true to erase each entry found within the block range
bool CMPTxList::isMPinBlockRange(int starting_block, int ending_block, bool bDeleteFound)
{
    leveldb::Slice skey, svalue;
    unsigned int count = 0;
    std::vector<std::string> vstr;
    int block;
    unsigned int n_found = 0;

    leveldb::Iterator* it = NewIterator();

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        skey = it->key();
        svalue = it->value();

        ++count;

        std::string strvalue = it->value().ToString();

        // parse the string returned, find the validity flag/bit & other parameters
        boost::split(vstr, strvalue, boost::is_any_of(":"), boost::token_compress_on);

        // only care about the block number/height here
        if (2 <= vstr.size()) {
            block = atoi(vstr[1]);

            if ((starting_block <= block) && (block <= ending_block)) {
                ++n_found;
                PrintToLog("%s() DELETING: %s=%s\n", __func__, skey.ToString(), svalue.ToString());
                if (bDeleteFound) pdb->Delete(writeoptions, skey);
            }
        }
    }

    PrintToLog("%s(%d, %d); n_found= %d\n", __func__, starting_block, ending_block, n_found);

    delete it;

    return (n_found);
}
