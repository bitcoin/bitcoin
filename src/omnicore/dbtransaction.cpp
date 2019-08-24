#include <omnicore/dbtransaction.h>

#include <omnicore/errors.h>
#include <omnicore/log.h>

#include <fs.h>
#include <uint256.h>
#include <tinyformat.h>

#include <leveldb/status.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

COmniTransactionDB::COmniTransactionDB(const fs::path& path, bool fWipe)
{
    leveldb::Status status = Open(path, fWipe);
    PrintToConsole("Loading master transactions database: %s\n", status.ToString());
}

COmniTransactionDB::~COmniTransactionDB()
{
    if (msc_debug_persistence) PrintToLog("COmniTransactionDB closed\n");
}

/**
 * Retrieves the serialized transaction details from the DB. 
 */
std::vector<std::string> COmniTransactionDB::FetchTransactionDetails(const uint256& txid)
{
    assert(pdb);
    std::string strValue;
    std::vector<std::string> vTransactionDetails;

    leveldb::Status status = pdb->Get(readoptions, txid.ToString(), &strValue);
    if (status.ok()) {
        std::vector<std::string> vStr;
        boost::split(vStr, strValue, boost::is_any_of(":"), boost::token_compress_on);
        if (vStr.size() == 2) {
            vTransactionDetails.push_back(vStr[0]);
            vTransactionDetails.push_back(vStr[1]);
        } else {
            PrintToLog("ERROR: Entry (%s) found in OmniTXDB with unexpected number of attributes!\n", txid.GetHex());
        }
    } else {
        PrintToLog("ERROR: Entry (%s) could not be loaded from OmniTXDB!\n", txid.GetHex());
    }

    return vTransactionDetails;
}

/**
 * Stores position in block and validation result for a transaction.
 */
void COmniTransactionDB::RecordTransaction(const uint256& txid, uint32_t posInBlock, int processingResult)
{
    assert(pdb);

    const std::string key = txid.ToString();
    const std::string value = strprintf("%d:%d", posInBlock, processingResult);

    leveldb::Status status = pdb->Put(writeoptions, key, value);
    ++nWritten;
}

/**
 * Returns the position of a transaction in a block.
 */
uint32_t COmniTransactionDB::FetchTransactionPosition(const uint256& txid)
{
    uint32_t posInBlock = 999999; // setting an initial arbitrarily high value will ensure transaction is always "last" in event of bug/exploit

    std::vector<std::string> vTransactionDetails = FetchTransactionDetails(txid);
    if (vTransactionDetails.size() == 2) {
        posInBlock = boost::lexical_cast<uint32_t>(vTransactionDetails[0]);
    }

    return posInBlock;
}

/**
 * Returns the reason why a transaction is invalid.
 */
std::string COmniTransactionDB::FetchInvalidReason(const uint256& txid)
{
    int processingResult = -999999;

    std::vector<std::string> vTransactionDetails = FetchTransactionDetails(txid);
    if (vTransactionDetails.size() == 2) {
        processingResult = boost::lexical_cast<int>(vTransactionDetails[1]);
    }

    return error_str(processingResult);
}
