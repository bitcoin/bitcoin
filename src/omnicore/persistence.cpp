/**
 * @file persistence.cpp
 *
 * This file contains file based persistence related functions.
 */

#include <omnicore/persistence.h>

#include <omnicore/dex.h>
#include <omnicore/log.h>
#include <omnicore/mdex.h>
#include <omnicore/rules.h>
#include <omnicore/sp.h>
#include <omnicore/tally.h>
#include <omnicore/utilsbitcoin.h>

#include <chain.h>
#include <fs.h>
#include <hash.h>
#include <validation.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/system.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <stdint.h>

#include <fstream>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace mastercore;

//! Number of "Dev Omni" of the last processed block (needed to save global state)
extern int64_t exodus_prev;

//! Path for file based persistence
extern fs::path pathStateFiles;

enum FILETYPES {
  FILETYPE_BALANCES = 0,
  FILETYPE_OFFERS,
  FILETYPE_ACCEPTS,
  FILETYPE_GLOBALS,
  FILETYPE_CROWDSALES,
  FILETYPE_MDEXORDERS,
  NUM_FILETYPES
};

static char const * const statePrefix[NUM_FILETYPES] = {
    "balances",
    "offers",
    "accepts",
    "globals",
    "crowdsales",
    "mdexorders",
};

static bool is_state_prefix(std::string const &str)
{
    for (int i = 0; i < NUM_FILETYPES; ++i) {
        if (boost::equals(str, statePrefix[i])) {
            return true;
        }
    }

    return false;
}

static int write_msc_balances(std::ofstream& file, CHash256& hasher)
{
    std::unordered_map<std::string, CMPTally>::iterator iter;
    for (iter = mp_tally_map.begin(); iter != mp_tally_map.end(); ++iter) {
        bool emptyWallet = true;

        std::string lineOut = (*iter).first;
        lineOut.append("=");
        CMPTally& curAddr = (*iter).second;
        curAddr.init();
        uint32_t propertyId = 0;
        while (0 != (propertyId = curAddr.next())) {
            int64_t balance = (*iter).second.getMoney(propertyId, BALANCE);
            int64_t sellReserved = (*iter).second.getMoney(propertyId, SELLOFFER_RESERVE);
            int64_t acceptReserved = (*iter).second.getMoney(propertyId, ACCEPT_RESERVE);
            int64_t metadexReserved = (*iter).second.getMoney(propertyId, METADEX_RESERVE);

            // we don't allow 0 balances to read in, so if we don't write them
            // it makes things match up better between persisted state and processed state
            if (0 == balance && 0 == sellReserved && 0 == acceptReserved && 0 == metadexReserved) {
                continue;
            }

            emptyWallet = false;

            lineOut.append(strprintf("%d:%d,%d,%d,%d;",
                    propertyId,
                    balance,
                    sellReserved,
                    acceptReserved,
                    metadexReserved));
        }

        if (false == emptyWallet) {
            // add the line to the hash
            hasher.Write((unsigned char*)lineOut.c_str(), lineOut.length());

            // write the line
            file << lineOut << std::endl;
        }
    }

    return 0;
}

static int write_mp_offers(std::ofstream& file, CHash256& hasher)
{
    OfferMap::const_iterator iter;
    for (iter = my_offers.begin(); iter != my_offers.end(); ++iter) {
        // decompose the key for address
        std::vector<std::string> vstr;
        boost::split(vstr, iter->first, boost::is_any_of("-"), boost::token_compress_on);
        const CMPOffer& offer = iter->second;
        offer.saveOffer(file, vstr[0], hasher);
    }

    return 0;
}

static int write_mp_accepts(std::ofstream& file, CHash256& hasher)
{
    AcceptMap::const_iterator iter;
    for (iter = my_accepts.begin(); iter != my_accepts.end(); ++iter) {
        // decompose the key for address
        std::vector<std::string> vstr;
        boost::split(vstr, iter->first, boost::is_any_of("-+"), boost::token_compress_on);
        const CMPAccept& accept = iter->second;
        accept.saveAccept(file, vstr[0], vstr[2], hasher);
    }

    return 0;
}

static int write_globals_state(std::ofstream& file, CHash256& hasher)
{
    uint32_t nextSPID = pDbSpInfo->peekNextSPID(OMNI_PROPERTY_MSC);
    uint32_t nextTestSPID = pDbSpInfo->peekNextSPID(OMNI_PROPERTY_TMSC);
    std::string lineOut = strprintf("%d,%d,%d",
            exodus_prev,
            nextSPID,
            nextTestSPID);

    // add the line to the hash
    hasher.Write((unsigned char*)lineOut.c_str(), lineOut.length());

    // write the line
    file << lineOut << std::endl;

    return 0;
}

static int write_mp_crowdsales(std::ofstream& file, CHash256& hasher)
{
    for (CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it) {
        // decompose the key for address
        const CMPCrowd& crowd = it->second;
        crowd.saveCrowdSale(file, it->first, hasher);
    }

    return 0;
}

static int write_mp_metadex(std::ofstream &file, CHash256& hasher)
{
    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        md_PricesMap& prices = my_it->second;
        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
            md_Set& indexes = (it->second);
            for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it) {
                const CMPMetaDEx& meta = *it;
                meta.saveOffer(file, hasher);
            }
        }
    }

    return 0;
}

static int input_msc_balances_string(const std::string& s)
{
    // "address=propertybalancedata"
    std::vector<std::string> addrData;
    boost::split(addrData, s, boost::is_any_of("="), boost::token_compress_on);
    if (addrData.size() != 2) return -1;

    std::string strAddress = addrData[0];

    // split the tuples of properties
    std::vector<std::string> vProperties;
    boost::split(vProperties, addrData[1], boost::is_any_of(";"), boost::token_compress_on);

    std::vector<std::string>::const_iterator iter;
    for (iter = vProperties.begin(); iter != vProperties.end(); ++iter) {
        if ((*iter).empty()) {
            continue;
        }

        // "propertyid:balancedata"
        std::vector<std::string> curProperty;
        boost::split(curProperty, *iter, boost::is_any_of(":"), boost::token_compress_on);
        if (curProperty.size() != 2) return -1;

        // "balance,sellreserved,acceptreserved,metadexreserved"
        std::vector<std::string> curBalance;
        boost::split(curBalance, curProperty[1], boost::is_any_of(","), boost::token_compress_on);
        if (curBalance.size() != 4) return -1;

        uint32_t propertyId = boost::lexical_cast<uint32_t>(curProperty[0]);

        int64_t balance = boost::lexical_cast<int64_t>(curBalance[0]);
        int64_t sellReserved = boost::lexical_cast<int64_t>(curBalance[1]);
        int64_t acceptReserved = boost::lexical_cast<int64_t>(curBalance[2]);
        int64_t metadexReserved = boost::lexical_cast<int64_t>(curBalance[3]);

        if (balance) update_tally_map(strAddress, propertyId, balance, BALANCE);
        if (sellReserved) update_tally_map(strAddress, propertyId, sellReserved, SELLOFFER_RESERVE);
        if (acceptReserved) update_tally_map(strAddress, propertyId, acceptReserved, ACCEPT_RESERVE);
        if (metadexReserved) update_tally_map(strAddress, propertyId, metadexReserved, METADEX_RESERVE);
    }

    return 0;
}

// seller-address, offer_block, amount, property, desired BTC , property_desired, fee, blocktimelimit
// 13z1JFtDMGTYQvtMq5gs4LmCztK3rmEZga,299076,76375000,1,6415500,0,10000,6
static int input_mp_offers_string(const std::string& s)
{
    std::vector<std::string> vstr;
    boost::split(vstr, s, boost::is_any_of(" ,="), boost::token_compress_on);

    if (9 != vstr.size()) return -1;

    int i = 0;

    std::string sellerAddr = vstr[i++];
    int offerBlock = boost::lexical_cast<int>(vstr[i++]);
    int64_t amountOriginal = boost::lexical_cast<int64_t>(vstr[i++]);
    uint32_t prop = boost::lexical_cast<uint32_t>(vstr[i++]);
    int64_t btcDesired = boost::lexical_cast<int64_t>(vstr[i++]);
    uint32_t prop_desired = boost::lexical_cast<uint32_t>(vstr[i++]);
    int64_t minFee = boost::lexical_cast<int64_t>(vstr[i++]);
    uint8_t blocktimelimit = boost::lexical_cast<unsigned int>(vstr[i++]); // lexical_cast can't handle char!
    uint256 txid = uint256S(vstr[i++]);

    // TODO: should this be here? There are usually no sanity checks..
    if (OMNI_PROPERTY_BTC != prop_desired) return -1;

    const std::string combo = STR_SELLOFFER_ADDR_PROP_COMBO(sellerAddr, prop);
    CMPOffer newOffer(offerBlock, amountOriginal, prop, btcDesired, minFee, blocktimelimit, txid);

    if (!my_offers.insert(std::make_pair(combo, newOffer)).second) return -1;

    return 0;
}

// seller-address, property, buyer-address, amount, fee, block
// 13z1JFtDMGTYQvtMq5gs4LmCztK3rmEZga,1, 148EFCFXbk2LrUhEHDfs9y3A5dJ4tttKVd,100000,11000,299126
// 13z1JFtDMGTYQvtMq5gs4LmCztK3rmEZga,1,1Md8GwMtWpiobRnjRabMT98EW6Jh4rEUNy,50000000,11000,299132
static int input_mp_accepts_string(const std::string& s)
{
    int nBlock;
    unsigned char blocktimelimit;
    std::vector<std::string> vstr;
    boost::split(vstr, s, boost::is_any_of(" ,="), boost::token_compress_on);
    int64_t amountRemaining, amountOriginal, offerOriginal, btcDesired;
    unsigned int prop;
    std::string sellerAddr, buyerAddr, txidStr;
    int i = 0;

    if (10 != vstr.size()) return -1;

    sellerAddr = vstr[i++];
    prop = boost::lexical_cast<unsigned int>(vstr[i++]);
    buyerAddr = vstr[i++];
    nBlock = atoi(vstr[i++]);
    amountRemaining = boost::lexical_cast<int64_t>(vstr[i++]);
    amountOriginal = boost::lexical_cast<uint64_t>(vstr[i++]);
    blocktimelimit = atoi(vstr[i++]);
    offerOriginal = boost::lexical_cast<int64_t>(vstr[i++]);
    btcDesired = boost::lexical_cast<int64_t>(vstr[i++]);
    txidStr = vstr[i++];

    const std::string combo = STR_ACCEPT_ADDR_PROP_ADDR_COMBO(sellerAddr, buyerAddr, prop);
    CMPAccept newAccept(amountOriginal, amountRemaining, nBlock, blocktimelimit, prop, offerOriginal, btcDesired, uint256S(txidStr));
    if (my_accepts.insert(std::make_pair(combo, newAccept)).second) {
        return 0;
    } else {
        return -1;
    }
}

// exodus_prev
static int input_globals_state_string(const std::string& s)
{
    int64_t exodusPrev;
    uint32_t nextSPID, nextTestSPID;
    std::vector<std::string> vstr;
    boost::split(vstr, s, boost::is_any_of(" ,="), boost::token_compress_on);
    if (3 != vstr.size()) return -1;

    int i = 0;
    exodusPrev = boost::lexical_cast<int64_t>(vstr[i++]);
    nextSPID = boost::lexical_cast<uint32_t>(vstr[i++]);
    nextTestSPID = boost::lexical_cast<uint32_t>(vstr[i++]);

    exodus_prev = exodusPrev;
    pDbSpInfo->init(nextSPID, nextTestSPID);
    return 0;
}

// addr,propertyId,nValue,property_desired,deadline,early_bird,percentage,txid
static int input_mp_crowdsale_string(const std::string& s)
{
    std::vector<std::string> vstr;
    boost::split(vstr, s, boost::is_any_of(" ,"), boost::token_compress_on);

    if (9 > vstr.size()) return -1;

    unsigned int i = 0;

    std::string sellerAddr = vstr[i++];
    uint32_t propertyId = boost::lexical_cast<uint32_t>(vstr[i++]);
    int64_t nValue = boost::lexical_cast<int64_t>(vstr[i++]);
    uint32_t property_desired = boost::lexical_cast<uint32_t>(vstr[i++]);
    int64_t deadline = boost::lexical_cast<int64_t>(vstr[i++]);
    uint8_t early_bird = boost::lexical_cast<unsigned int>(vstr[i++]); // lexical_cast can't handle char!
    uint8_t percentage = boost::lexical_cast<unsigned int>(vstr[i++]); // lexical_cast can't handle char!
    int64_t u_created = boost::lexical_cast<int64_t>(vstr[i++]);
    int64_t i_created = boost::lexical_cast<int64_t>(vstr[i++]);

    CMPCrowd newCrowdsale(propertyId, nValue, property_desired, deadline, early_bird, percentage, u_created, i_created);

    // load the remaining as database pairs
    while (i < vstr.size()) {
        std::vector<std::string> entryData;
        boost::split(entryData, vstr[i++], boost::is_any_of("="), boost::token_compress_on);
        if (2 != entryData.size()) return -1;

        std::vector<std::string> valueData;
        boost::split(valueData, entryData[1], boost::is_any_of(";"), boost::token_compress_on);

        std::vector<int64_t> vals;
        for (std::vector<std::string>::const_iterator it = valueData.begin(); it != valueData.end(); ++it) {
            vals.push_back(boost::lexical_cast<int64_t>(*it));
        }

        uint256 txHash = uint256S(entryData[0]);
        newCrowdsale.insertDatabase(txHash, vals);
    }

    if (!my_crowds.insert(std::make_pair(sellerAddr, newCrowdsale)).second) {
        return -1;
    }

    return 0;
}

// address, block, amount for sale, property, amount desired, property desired, subaction, idx, txid, amount remaining
static int input_mp_mdexorder_string(const std::string& s)
{
    std::vector<std::string> vstr;
    boost::split(vstr, s, boost::is_any_of(" ,="), boost::token_compress_on);

    if (10 != vstr.size()) return -1;

    int i = 0;

    std::string addr = vstr[i++];
    int block = boost::lexical_cast<int>(vstr[i++]);
    int64_t amount_forsale = boost::lexical_cast<int64_t>(vstr[i++]);
    uint32_t property = boost::lexical_cast<uint32_t>(vstr[i++]);
    int64_t amount_desired = boost::lexical_cast<int64_t>(vstr[i++]);
    uint32_t desired_property = boost::lexical_cast<uint32_t>(vstr[i++]);
    uint8_t subaction = boost::lexical_cast<unsigned int>(vstr[i++]); // lexical_cast can't handle char!
    unsigned int idx = boost::lexical_cast<unsigned int>(vstr[i++]);
    uint256 txid = uint256S(vstr[i++]);
    int64_t amount_remaining = boost::lexical_cast<int64_t>(vstr[i++]);

    CMPMetaDEx mdexObj(addr, block, property, amount_forsale, desired_property,
            amount_desired, txid, idx, subaction, amount_remaining);

    if (!MetaDEx_INSERT(mdexObj)) return -1;

    return 0;
}

static int write_state_file(const CBlockIndex* pBlockIndex, int what)
{
    fs::path path = pathStateFiles / strprintf("%s-%s.dat", statePrefix[what], pBlockIndex->GetBlockHash().ToString());
    const std::string strFile = path.string();

    std::ofstream file;
    file.open(strFile.c_str());

    CHash256 hasher;

    int result = 0;

    switch (what) {
        case FILETYPE_BALANCES:
            result = write_msc_balances(file, hasher);
            break;

        case FILETYPE_OFFERS:
            result = write_mp_offers(file, hasher);
            break;

        case FILETYPE_ACCEPTS:
            result = write_mp_accepts(file, hasher);
            break;

        case FILETYPE_GLOBALS:
            result = write_globals_state(file, hasher);
            break;

        case FILETYPE_CROWDSALES:
            result = write_mp_crowdsales(file, hasher);
            break;

        case FILETYPE_MDEXORDERS:
            result = write_mp_metadex(file, hasher);
            break;
    }

    // generate and write the double hash of all the contents written
    uint256 hash;
    hasher.Finalize(hash.begin());
    file << "!" << hash.ToString() << std::endl;

    file.flush();
    file.close();
    return result;
}

static void prune_state_files(const CBlockIndex* topIndex)
{
    // build a set of blockHashes for which we have any state files
    std::set<uint256> statefulBlockHashes;

    fs::directory_iterator dIter(pathStateFiles);
    fs::directory_iterator endIter;
    for (; dIter != endIter; ++dIter) {
        std::string fName = dIter->path().empty() ? "<invalid>" : (*--dIter->path().end()).string();
        if (false == fs::is_regular_file(dIter->status())) {
            // skip funny business
            PrintToLog("Non-regular file found in persistence directory : %s\n", fName);
            continue;
        }

        std::vector<std::string> vstr;
        boost::split(vstr, fName, boost::is_any_of("-."), boost::token_compress_on);
        if (vstr.size() == 3 &&
                is_state_prefix(vstr[0]) &&
                boost::equals(vstr[2], "dat")) {
            uint256 blockHash;
            blockHash.SetHex(vstr[1]);
            statefulBlockHashes.insert(blockHash);
        } else {
            PrintToLog("None state file found in persistence directory : %s\n", fName);
        }
    }

    // for each blockHash in the set, determine the distance from the given block
    std::set<uint256>::const_iterator iter;
    for (iter = statefulBlockHashes.begin(); iter != statefulBlockHashes.end(); ++iter) {
        // look up the CBlockIndex for height info
        CBlockIndex const *curIndex = GetBlockIndex(*iter);

        // if we have nothing int the index, or this block is too old..
        if (nullptr == curIndex || (((topIndex->nHeight - curIndex->nHeight) > MAX_STATE_HISTORY)
                && (curIndex->nHeight % STORE_EVERY_N_BLOCK != 0))) {
            if (msc_debug_persistence) {
                if (curIndex) {
                    PrintToLog("State from Block:%s is no longer need, removing files (age-from-tip: %d)\n", (*iter).ToString(), topIndex->nHeight - curIndex->nHeight);
                } else {
                    PrintToLog("State from Block:%s is no longer need, removing files (not in index)\n", (*iter).ToString());
                }
            }

            // destroy the associated files!
            std::string strBlockHash = iter->ToString();
            for (int i = 0; i < NUM_FILETYPES; ++i) {
                fs::path path = pathStateFiles / strprintf("%s-%s.dat", statePrefix[i], strBlockHash);
                fs::remove(path);
            }
        }
    }
}

/**
 * @return The block height at which the state is persisted every block.
 */
static int GetWrapModeHeight()
{
    if (MainNet()) {
        return DONT_STORE_MAINNET_STATE_UNTIL;
    } else {
        return 0;
    }
}

/**
 * Indicates whether persistence is enabled and the state is stored.
 */
bool IsPersistenceEnabled(int blockHeight)
{
    static int nWarpModeHeight = GetWrapModeHeight();

    int nMinHeight = nWarpModeHeight;

    if (nWarpModeHeight == 0) {
        nMinHeight = GetHeight();
    }

    // if too far away from the top -- do not write
    if (nMinHeight > (blockHeight + MAX_STATE_HISTORY)
            && (blockHeight % STORE_EVERY_N_BLOCK != 0)) {
        return false;
    }

    return true;
}

/**
 * Stores the in-memory state in files.
 */
int PersistInMemoryState(const CBlockIndex* pBlockIndex)
{
    // write the new state as of the given block
    write_state_file(pBlockIndex, FILETYPE_BALANCES);
    write_state_file(pBlockIndex, FILETYPE_OFFERS);
    write_state_file(pBlockIndex, FILETYPE_ACCEPTS);
    write_state_file(pBlockIndex, FILETYPE_GLOBALS);
    write_state_file(pBlockIndex, FILETYPE_CROWDSALES);
    write_state_file(pBlockIndex, FILETYPE_MDEXORDERS);

    // clean-up the directory
    prune_state_files(pBlockIndex);

    pDbSpInfo->setWatermark(pBlockIndex->GetBlockHash());

    return 0;
}

/**
 * Loads and retrieves state from a file.
 */
int RestoreInMemoryState(const std::string& filename, int what, bool verifyHash)
{
    int lines = 0;
    int (*inputLineFunc)(const std::string&) = nullptr;

    CHash256 hasher;

    switch (what) {
        case FILETYPE_BALANCES:
            mp_tally_map.clear();
            inputLineFunc = input_msc_balances_string;
            break;

        case FILETYPE_OFFERS:
            my_offers.clear();
            inputLineFunc = input_mp_offers_string;
            break;

        case FILETYPE_ACCEPTS:
            my_accepts.clear();
            inputLineFunc = input_mp_accepts_string;
            break;

        case FILETYPE_GLOBALS:
            inputLineFunc = input_globals_state_string;
            break;

        case FILETYPE_CROWDSALES:
            my_crowds.clear();
            inputLineFunc = input_mp_crowdsale_string;
            break;

        case FILETYPE_MDEXORDERS:
            // FIXME
            // memory leak ... gotta unallocate inner layers first....
            // TODO
            // ...
            metadex.clear();
            inputLineFunc = input_mp_mdexorder_string;
            break;

        default:
            return -1;
    }

    if (msc_debug_persistence) {
        LogPrintf("Loading %s ... \n", filename);
        PrintToLog("%s(%s), line %d, file: %s\n", __FUNCTION__, filename, __LINE__, __FILE__);
    }

    std::ifstream file;
    file.open(filename.c_str());
    if (!file.is_open()) {
        if (msc_debug_persistence) LogPrintf("%s(%s): file not found, line %d, file: %s\n", __FUNCTION__, filename, __LINE__, __FILE__);
        return -1;
    }

    int res = 0;

    std::string fileHash;
    while (file.good()) {
        std::string line;
        std::getline(file, line);
        if (line.empty() || line[0] == '#') continue;

        // remove \r if the file came from Windows
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        // record and skip hashes in the file
        if (line[0] == '!') {
            fileHash = line.substr(1);
            continue;
        }

        // update hash?
        if (verifyHash) {
            hasher.Write((unsigned char*)line.c_str(), line.length());
        }

        if (inputLineFunc) {
            if (inputLineFunc(line) < 0) {
                res = -1;
                break;
            }
        }

        ++lines;
    }

    file.close();

    if (verifyHash && res == 0) {
        // generate and write the double hash of all the contents written
        uint256 hash;
        hasher.Finalize(hash.begin());

        if (false == boost::iequals(hash.ToString(), fileHash)) {
            PrintToLog("File %s loaded, but failed hash validation!\n", filename);
            res = -1;
        }
    }

    PrintToLog("%s(%s), loaded lines= %d, res= %d\n", __FUNCTION__, filename, lines, res);
    LogPrintf("%s(): file: %s , loaded lines= %d, res= %d\n", __FUNCTION__, filename, lines, res);

    return res;
}

/**
 * Loads and restores the latest state. Returns -1 if reparse is required.
 */
int LoadMostRelevantInMemoryState()
{
    int res = -1;
    uint256 spWatermark;
    {
        LOCK(cs_tally);
        PrintToLog("Trying to load most relevant state into memory..\n");
        // check the SP database and roll it back to its latest valid state
        // according to the active chain
        if (!pDbSpInfo->getWatermark(spWatermark)) {
            // trigger a full reparse, if the SP database has no watermark
            PrintToLog("Failed to load historical state: SP database has no watermark\n");
            return -1;
        }

    }

    // Try and get watermark block
    CBlockIndex const *spBlockIndex = GetBlockIndex(spWatermark);

    // Watermark block not found.
    if (nullptr == spBlockIndex)
    {
        PrintToLog("spWatermark not found: %s\n", spWatermark.ToString());

        // Try and load an historical state
        fs::directory_iterator dIter(pathStateFiles);
        fs::directory_iterator endIter;
        std::map<int, const CBlockIndex*> foundBlocks;

        for (; dIter != endIter; ++dIter) {
            if (false == fs::is_regular_file(dIter->status()) || dIter->path().empty()) {
                // skip funny business
                continue;
            }

            std::string fName = (*--dIter->path().end()).string();
            std::vector<std::string> vstr;
            boost::split(vstr, fName, boost::is_any_of("-."), boost::token_compress_on);
            if (vstr.size() == 3 && boost::equals(vstr[2], "dat")) {
                uint256 blockHash;
                blockHash.SetHex(vstr[1]);
                CBlockIndex *pBlockIndex = GetBlockIndex(blockHash);
                if (pBlockIndex == nullptr) {
                    continue;
                }

                // Add to found blocks
                foundBlocks.emplace(pBlockIndex->nHeight, pBlockIndex);
            }
        }

        // Was unable to find valid previous state, full reparse required.
        if (foundBlocks.empty()) {
            PrintToLog("Failed to load historical state: watermark isn't a real block\n");
            return -1;
        }

        spBlockIndex = foundBlocks.rbegin()->second;
        pDbSpInfo->setWatermark(spBlockIndex->GetBlockHash());

        PrintToLog("Watermark not found. New one set from state files: %s\n", spBlockIndex->GetBlockHash().ToString());
    }

    std::set<uint256> persistedBlocks;
    {
        LOCK2(cs_main, cs_tally);

        PrintToLog("Rolling back blocks to active chain.\n");

        while (nullptr != spBlockIndex && false == ::ChainActive().Contains(spBlockIndex)) {
            int remainingSPs = pDbSpInfo->popBlock(spBlockIndex->GetBlockHash());
            if (remainingSPs < 0) {
                // trigger a full reparse, if the levelDB cannot roll back
                PrintToLog("Failed to load historical state: no valid state found after rolling back SP database\n");
                return -1;
            }

            spBlockIndex = spBlockIndex->pprev;
            if (spBlockIndex != nullptr) {
                pDbSpInfo->setWatermark(spBlockIndex->GetBlockHash());
            }
        }

        // prepare a set of available files by block hash pruning any that are
        // not in the active chain
        fs::directory_iterator dIter(pathStateFiles);
        fs::directory_iterator endIter;
        for (; dIter != endIter; ++dIter) {
            if (false == fs::is_regular_file(dIter->status()) || dIter->path().empty()) {
                // skip funny business
                continue;
            }

            std::string fName = (*--dIter->path().end()).string();
            std::vector<std::string> vstr;
            boost::split(vstr, fName, boost::is_any_of("-."), boost::token_compress_on);
            if (vstr.size() == 3 &&
                    boost::equals(vstr[2], "dat")) {
                uint256 blockHash;
                blockHash.SetHex(vstr[1]);
                CBlockIndex *pBlockIndex = GetBlockIndex(blockHash);
                if (pBlockIndex == nullptr || false == ::ChainActive().Contains(pBlockIndex)) {
                    continue;
                }

                // this is a valid block in the active chain, store it
                persistedBlocks.insert(blockHash);
            }
        }
    }

    {
        LOCK(cs_tally);
        // using the SP's watermark after its fixed-up as the tip
        // walk backwards until we find a valid and full set of persisted state files
        // for each block we discard, roll back the SP database
        CBlockIndex const *curTip = spBlockIndex;
        int abortRollBackBlock = 9999999;
        if (curTip != nullptr) {
            abortRollBackBlock = ConsensusParams().GENESIS_BLOCK - 1;
        }
        while (nullptr != curTip && persistedBlocks.size() > 0 && curTip->nHeight > abortRollBackBlock ) {
            if (persistedBlocks.find(curTip->GetBlockHash()) != persistedBlocks.end()) {
                int success = -1;
                for (int i = 0; i < NUM_FILETYPES; ++i) {
                    fs::path path = pathStateFiles / strprintf("%s-%s.dat", statePrefix[i], curTip->GetBlockHash().ToString());
                    const std::string strFile = path.string();
                    success = RestoreInMemoryState(strFile, i, true);
                    if (success < 0) {
                        PrintToConsole("Found a state inconsistency at block height %d. "
                                "Reverting up to %d blocks.. this may take a few minutes.\n",
                                curTip->nHeight, (curTip->nHeight - abortRollBackBlock - 1));
                        break;
                    }
                }

                if (success >= 0) {
                    res = curTip->nHeight;
                    break;
                }

                // remove this from the persistedBlock Set
                persistedBlocks.erase(spBlockIndex->GetBlockHash());
            }

            // go to the previous block
            if (pDbSpInfo->popBlock(curTip->GetBlockHash()) <= 0) {
                // trigger a full reparse, if the levelDB cannot roll back
                PrintToLog("Failed to load historical state: no valid state found after rolling back SP database (2)\n");
                return -1;
            }
            curTip = curTip->pprev;
            spBlockIndex = curTip;
            if (curTip != nullptr) {
                pDbSpInfo->setWatermark(curTip->GetBlockHash());
            }
        }
    }

    if (persistedBlocks.size() == 0) {
        // trigger a reparse if we exhausted the persistence files without success
        PrintToLog("Failed to load historical state: no valid state found after exhausting persistence files\n");
        return -1;
    }

    // return the height of the block we settled at
    return res;
}
