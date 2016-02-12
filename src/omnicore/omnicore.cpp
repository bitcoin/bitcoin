/**
 * @file omnicore.cpp
 *
 * This file contains the core of Omni Core.
 */

#include "omnicore/omnicore.h"

#include "omnicore/activation.h"
#include "omnicore/consensushash.h"
#include "omnicore/convert.h"
#include "omnicore/dex.h"
#include "omnicore/encoding.h"
#include "omnicore/errors.h"
#include "omnicore/log.h"
#include "omnicore/mdex.h"
#include "omnicore/notifications.h"
#include "omnicore/pending.h"
#include "omnicore/persistence.h"
#include "omnicore/rules.h"
#include "omnicore/script.h"
#include "omnicore/seedblocks.h"
#include "omnicore/sp.h"
#include "omnicore/tally.h"
#include "omnicore/tx.h"
#include "omnicore/utils.h"
#include "omnicore/utilsbitcoin.h"
#include "omnicore/version.h"
#include "omnicore/walletcache.h"
#include "omnicore/wallettxs.h"

#include "base58.h"
#include "chainparams.h"
#include "coincontrol.h"
#include "coins.h"
#include "core_io.h"
#include "init.h"
#include "main.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "script/standard.h"
#include "sync.h"
#include "tinyformat.h"
#include "uint256.h"
#include "ui_interface.h"
#include "util.h"
#include "utilstrencodings.h"
#include "utiltime.h"
#ifdef ENABLE_WALLET
#include "wallet.h"
#include "wallet_ismine.h"
#endif

#include <boost/algorithm/string.hpp>
#include <boost/exception/to_string.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <openssl/sha.h>

#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"

#include "leveldb/db.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

using boost::algorithm::token_compress_on;
using boost::to_string;

using json_spirit::Array;
using json_spirit::Pair;
using json_spirit::Object;
using json_spirit::write_string;

using leveldb::Iterator;
using leveldb::Slice;
using leveldb::Status;

using std::endl;
using std::make_pair;
using std::map;
using std::ofstream;
using std::pair;
using std::string;
using std::vector;

using namespace mastercore;

CCriticalSection cs_tally;

static string exodus_address = "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P";

static const string exodus_mainnet = "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P";
static const string exodus_testnet = "mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv";
static const string getmoney_testnet = "moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP";

static int nWaterlineBlock = 0;

//! Available balances of wallet properties
std::map<uint32_t, int64_t> global_balance_money;
//! Reserved balances of wallet propertiess
std::map<uint32_t, int64_t> global_balance_reserved;
//! Vector containing a list of properties relative to the wallet
std::set<uint32_t> global_wallet_property_list;

/**
 * Used to indicate, whether to automatically commit created transactions.
 *
 * Can be set with configuration "-autocommit" or RPC "setautocommit_OMNI".
 */
bool autoCommit = true;

//! Number of "Dev MSC" of the last processed block
static int64_t exodus_prev = 0;

static boost::filesystem::path MPPersistencePath;

static int mastercoreInitialized = 0;

static int reorgRecoveryMode = 0;
static int reorgRecoveryMaxHeight = 0;

CMPTxList *mastercore::p_txlistdb;
CMPTradeList *mastercore::t_tradelistdb;
CMPSTOList *mastercore::s_stolistdb;

// indicate whether persistence is enabled at this point, or not
// used to write/read files, for breakout mode, debugging, etc.
static bool writePersistence(int block_now)
{
  // if too far away from the top -- do not write
  if (GetHeight() > (block_now + MAX_STATE_HISTORY)) return false;

  return true;
}

std::string mastercore::strMPProperty(uint32_t propertyId)
{
    std::string str = "*unknown*";

    // test user-token
    if (0x80000000 & propertyId) {
        str = strprintf("Test token: %d : 0x%08X", 0x7FFFFFFF & propertyId, propertyId);
    } else {
        switch (propertyId) {
            case OMNI_PROPERTY_BTC: str = "BTC";
                break;
            case OMNI_PROPERTY_MSC: str = "OMNI";
                break;
            case OMNI_PROPERTY_TMSC: str = "TOMNI";
                break;
            default:
                str = strprintf("SP token: %d", propertyId);
        }
    }

    return str;
}

std::string FormatDivisibleShortMP(int64_t n)
{
    int64_t n_abs = (n > 0 ? n : -n);
    int64_t quotient = n_abs / COIN;
    int64_t remainder = n_abs % COIN;
    std::string str = strprintf("%d.%08d", quotient, remainder);
    // clean up trailing zeros - good for RPC not so much for UI
    str.erase(str.find_last_not_of('0') + 1, std::string::npos);
    if (str.length() > 0) {
        std::string::iterator it = str.end() - 1;
        if (*it == '.') {
            str.erase(it);
        }
    } //get rid of trailing dot if non decimal
    return str;
}

std::string FormatDivisibleMP(int64_t n, bool fSign)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    int64_t n_abs = (n > 0 ? n : -n);
    int64_t quotient = n_abs / COIN;
    int64_t remainder = n_abs % COIN;
    std::string str = strprintf("%d.%08d", quotient, remainder);

    if (!fSign) return str;

    if (n < 0)
        str.insert((unsigned int) 0, 1, '-');
    else
        str.insert((unsigned int) 0, 1, '+');
    return str;
}

std::string mastercore::FormatIndivisibleMP(int64_t n)
{
    return strprintf("%d", n);
}

std::string FormatShortMP(uint32_t property, int64_t n)
{
    if (isPropertyDivisible(property)) {
        return FormatDivisibleShortMP(n);
    } else {
        return FormatIndivisibleMP(n);
    }
}

std::string FormatMP(uint32_t property, int64_t n, bool fSign)
{
    if (isPropertyDivisible(property)) {
        return FormatDivisibleMP(n, fSign);
    } else {
        return FormatIndivisibleMP(n);
    }
}

std::string FormatByType(int64_t amount, uint16_t propertyType)
{
    if (propertyType & MSC_PROPERTY_TYPE_INDIVISIBLE) {
        return FormatIndivisibleMP(amount);
    } else {
        return FormatDivisibleMP(amount);
    }
}

OfferMap mastercore::my_offers;
AcceptMap mastercore::my_accepts;

CMPSPInfo *mastercore::_my_sps;
CrowdMap mastercore::my_crowds;

// this is the master list of all amounts for all addresses for all properties, map is sorted by Bitcoin address
std::map<std::string, CMPTally> mastercore::mp_tally_map;

CMPTally* mastercore::getTally(const std::string& address)
{
    std::map<std::string, CMPTally>::iterator it = mp_tally_map.find(address);

    if (it != mp_tally_map.end()) return &(it->second);

    return (CMPTally *) NULL;
}

// look at balance for an address
int64_t getMPbalance(const std::string& address, uint32_t propertyId, TallyType ttype)
{
    int64_t balance = 0;
    if (TALLY_TYPE_COUNT <= ttype) {
        return 0;
    }
    if (ttype == ACCEPT_RESERVE && propertyId > OMNI_PROPERTY_TMSC) {
        // ACCEPT_RESERVE is always empty, except for MSC and TMSC
        return 0; 
    }

    LOCK(cs_tally);
    const std::map<std::string, CMPTally>::iterator my_it = mp_tally_map.find(address);
    if (my_it != mp_tally_map.end()) {
        balance = (my_it->second).getMoney(propertyId, ttype);
    }

    return balance;
}

int64_t getUserAvailableMPbalance(const std::string& address, uint32_t propertyId)
{
    int64_t money = getMPbalance(address, propertyId, BALANCE);
    int64_t pending = getMPbalance(address, propertyId, PENDING);

    if (0 > pending) {
        return (money + pending); // show the decrease in available money
    }

    return money;
}

bool mastercore::isTestEcosystemProperty(uint32_t propertyId)
{
    if ((OMNI_PROPERTY_TMSC == propertyId) || (TEST_ECO_PROPERTY_1 <= propertyId)) return true;

    return false;
}

bool mastercore::isMainEcosystemProperty(uint32_t propertyId)
{
    if ((OMNI_PROPERTY_BTC != propertyId) && !isTestEcosystemProperty(propertyId)) return true;

    return false;
}

std::string mastercore::getTokenLabel(uint32_t propertyId)
{
    std::string tokenStr;
    if (propertyId < 3) {
        if (propertyId == 1) {
            tokenStr = " OMNI";
        } else {
            tokenStr = " TOMNI";
        }
    } else {
        tokenStr = strprintf(" SPT#%d", propertyId);
    }
    return tokenStr;
}

// get total tokens for a property
// optionally counts the number of addresses who own that property: n_owners_total
int64_t mastercore::getTotalTokens(uint32_t propertyId, int64_t* n_owners_total)
{
    int64_t prev = 0;
    int64_t owners = 0;
    int64_t totalTokens = 0;

    LOCK(cs_tally);

    CMPSPInfo::Entry property;
    if (false == _my_sps->getSP(propertyId, property)) {
        return 0; // property ID does not exist
    }

    if (!property.fixed || n_owners_total) {
        for (std::map<std::string, CMPTally>::const_iterator it = mp_tally_map.begin(); it != mp_tally_map.end(); ++it) {
            const CMPTally& tally = it->second;

            totalTokens += tally.getMoney(propertyId, BALANCE);
            totalTokens += tally.getMoney(propertyId, SELLOFFER_RESERVE);
            totalTokens += tally.getMoney(propertyId, ACCEPT_RESERVE);
            totalTokens += tally.getMoney(propertyId, METADEX_RESERVE);

            if (prev != totalTokens) {
                prev = totalTokens;
                owners++;
            }
        }
    }

    if (property.fixed) {
        totalTokens = property.num_tokens; // only valid for TX50
    }

    if (n_owners_total) *n_owners_total = owners;

    return totalTokens;
}

// return true if everything is ok
bool mastercore::update_tally_map(const std::string& who, uint32_t propertyId, int64_t amount, TallyType ttype)
{
    if (0 == amount) {
        PrintToLog("%s(%s, %u=0x%X, %+d, ttype=%d) ERROR: amount to credit or debit is zero\n", __func__, who, propertyId, propertyId, amount, ttype);
        return false;
    }
    if (ttype >= TALLY_TYPE_COUNT) {
        PrintToLog("%s(%s, %u=0x%X, %+d, ttype=%d) ERROR: invalid tally type\n", __func__, who, propertyId, propertyId, amount, ttype);
        return false;
    }
    
    bool bRet = false;
    int64_t before = 0;
    int64_t after = 0;

    LOCK(cs_tally);

    before = getMPbalance(who, propertyId, ttype);

    std::map<std::string, CMPTally>::iterator my_it = mp_tally_map.find(who);
    if (my_it == mp_tally_map.end()) {
        // insert an empty element
        my_it = (mp_tally_map.insert(std::make_pair(who, CMPTally()))).first;
    }

    CMPTally& tally = my_it->second;
    bRet = tally.updateMoney(propertyId, amount, ttype);

    after = getMPbalance(who, propertyId, ttype);
    if (!bRet) {
        assert(before == after);
        PrintToLog("%s(%s, %u=0x%X, %+d, ttype=%d) ERROR: insufficient balance (=%d)\n", __func__, who, propertyId, propertyId, amount, ttype, before);
    }
    if (msc_debug_tally && (exodus_address != who || msc_debug_exo)) {
        PrintToLog("%s(%s, %u=0x%X, %+d, ttype=%d): before=%d, after=%d\n", __func__, who, propertyId, propertyId, amount, ttype, before, after);
    }

    return bRet;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// some old TODOs
//  6) verify large-number calculations (especially divisions & multiplications)
//  9) build in consesus checks with the masterchain.info & masterchest.info -- possibly run them automatically, daily (?)
// 10) need a locking mechanism between Core & Qt -- to retrieve the tally, for instance, this and similar to this: LOCK(wallet->cs_wallet);
//

/**
 * Calculates and updates the "development mastercoins".
 *
 * For every 10 MSC sold during the Exodus period, 1 additional "Dev MSC" was generated,
 * which are being awarded to the Exodus address slowly over the years.
 *
 * @see The "Dev MSC" specification:
 * https://github.com/OmniLayer/spec#development-mastercoins-dev-msc-previously-reward-mastercoins
 *
 * Note:
 * If timestamps are out of order, then previously vested "Dev MSC" are not voided.
 *
 * @param nTime  The timestamp of the block to update the "Dev MSC" for
 * @return The number of "Dev MSC" generated
 */
static int64_t calculate_and_update_devmsc(unsigned int nTime)
{
    // do nothing if before end of fundraiser
    if (nTime < 1377993874) return 0;

    // taken mainly from msc_validate.py: def get_available_reward(height, c)
    int64_t devmsc = 0;
    int64_t exodus_delta = 0;
    // spec constants:
    const int64_t all_reward = 5631623576222;
    const double seconds_in_one_year = 31556926;
    const double seconds_passed = nTime - 1377993874; // exodus bootstrap deadline
    const double years = seconds_passed / seconds_in_one_year;
    const double part_available = 1 - pow(0.5, years);
    const double available_reward = all_reward * part_available;

    devmsc = rounduint64(available_reward);
    exodus_delta = devmsc - exodus_prev;

    if (msc_debug_exo) PrintToLog("devmsc=%d, exodus_prev=%d, exodus_delta=%d\n", devmsc, exodus_prev, exodus_delta);

    // skip if a block's timestamp is older than that of a previous one!
    if (0 > exodus_delta) return 0;

    // sanity check that devmsc isn't an impossible value
    if (devmsc > all_reward || 0 > devmsc) {
        PrintToLog("%s(): ERROR: insane number of Dev OMNI (nTime=%d, exodus_prev=%d, devmsc=%d)\n", __func__, nTime, exodus_prev, devmsc);
        return 0;
    }

    if (exodus_delta > 0) {
        update_tally_map(exodus_address, OMNI_PROPERTY_MSC, exodus_delta, BALANCE);
        exodus_prev = devmsc;
    }

    return exodus_delta;
}

uint32_t mastercore::GetNextPropertyId(bool maineco)
{
    if (maineco) {
        return _my_sps->peekNextSPID(1);
    } else {
        return _my_sps->peekNextSPID(2);
    }
}

void CheckWalletUpdate(bool forceUpdate)
{
    if (!WalletCacheUpdate()) {
        // no balance changes were detected that affect wallet addresses, signal a generic change to overall Omni state
        if (!forceUpdate) {
            uiInterface.OmniStateChanged();
            return;
        }
    }
#ifdef ENABLE_WALLET
    LOCK(cs_tally);

    // balance changes were found in the wallet, update the global totals and signal a Omni balance change
    global_balance_money.clear();
    global_balance_reserved.clear();

    // populate global balance totals and wallet property list - note global balances do not include additional balances from watch-only addresses
    for (std::map<std::string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
        // check if the address is a wallet address (including watched addresses)
        std::string address = my_it->first;
        int addressIsMine = IsMyAddress(address);
        if (!addressIsMine) continue;
        // iterate only those properties in the TokenMap for this address
        my_it->second.init();
        uint32_t propertyId;
        while (0 != (propertyId = (my_it->second).next())) {
            // add to the global wallet property list
            global_wallet_property_list.insert(propertyId);
            // check if the address is spendable (only spendable balances are included in totals)
            if (addressIsMine != ISMINE_SPENDABLE) continue;
            // work out the balances and add to globals
            global_balance_money[propertyId] += getUserAvailableMPbalance(address, propertyId);
            global_balance_reserved[propertyId] += getMPbalance(address, propertyId, SELLOFFER_RESERVE);
            global_balance_reserved[propertyId] += getMPbalance(address, propertyId, METADEX_RESERVE);
            global_balance_reserved[propertyId] += getMPbalance(address, propertyId, ACCEPT_RESERVE);
        }
    }
    // signal an Omni balance change
    uiInterface.OmniBalanceChanged();
#endif
}

/**
 * Executes Exodus crowdsale purchases.
 *
 * @return True, if it was a valid purchase
 */
static bool TXExodusFundraiser(const CTransaction& tx, const std::string& sender, int64_t amountInvested, int nBlock, unsigned int nTime)
{
    const int secondsPerWeek = 60 * 60 * 24 * 7;
    const CConsensusParams& params = ConsensusParams();

    if (nBlock >= params.GENESIS_BLOCK && nBlock <= params.LAST_EXODUS_BLOCK) {
        int deadlineTimeleft = params.exodusDeadline - nTime;
        double bonusPercentage = params.exodusBonusPerWeek * deadlineTimeleft / secondsPerWeek;
        double bonus = 1.0 + std::max(bonusPercentage, 0.0);

        int64_t amountGenerated = round(params.exodusReward * amountInvested * bonus);
        if (amountGenerated > 0) {
            PrintToLog("Exodus Fundraiser tx detected, tx %s generated %s\n", tx.GetHash().ToString(), FormatDivisibleMP(amountGenerated));

            assert(update_tally_map(sender, OMNI_PROPERTY_MSC, amountGenerated, BALANCE));
            assert(update_tally_map(sender, OMNI_PROPERTY_TMSC, amountGenerated, BALANCE));

            return true;
        }
    }

    return false;
}

/**
 * Returns the encoding class, used to embed a payload.
 *
 *   0 None
 *   1 Class A (p2pkh)
 *   2 Class B (multisig)
 *   3 Class C (op-return)
 */
int mastercore::GetEncodingClass(const CTransaction& tx, int nBlock)
{
    bool hasExodus = false;
    bool hasMultisig = false;
    bool hasOpReturn = false;
    bool hasMoney = false;

    for (unsigned int n = 0; n < tx.vout.size(); ++n) {
        const CTxOut& output = tx.vout[n];

        txnouttype outType;
        if (!GetOutputType(output.scriptPubKey, outType)) {
            continue;
        }
        if (!IsAllowedOutputType(outType, nBlock)) {
            continue;
        }

        if (outType == TX_PUBKEYHASH) {
            CTxDestination dest;
            if (ExtractDestination(output.scriptPubKey, dest)) {
                CBitcoinAddress address(dest);
                if (address == ExodusAddress()) {
                    hasExodus = true;
                }
                if (address == ExodusCrowdsaleAddress(nBlock)) {
                    hasMoney = true;
                }
            }
        }
        if (outType == TX_MULTISIG) {
            hasMultisig = true;
        }
        if (outType == TX_NULL_DATA) {
            // Ensure there is a payload, and the first pushed element equals,
            // or starts with the "omni" marker
            std::vector<std::string> scriptPushes;
            if (!GetScriptPushes(output.scriptPubKey, scriptPushes)) {
                continue;
            }
            if (!scriptPushes.empty()) {
                std::vector<unsigned char> vchMarker = GetOmMarker();
                std::vector<unsigned char> vchPushed = ParseHex(scriptPushes[0]);
                if (vchPushed.size() < vchMarker.size()) {
                    continue;
                }
                if (std::equal(vchMarker.begin(), vchMarker.end(), vchPushed.begin())) {
                    hasOpReturn = true;
                }
            }
        }
    }

    if (hasOpReturn) {
        return OMNI_CLASS_C;
    }
    if (hasExodus && hasMultisig) {
        return OMNI_CLASS_B;
    }
    if (hasExodus || hasMoney) {
        return OMNI_CLASS_A;
    }

    return NO_MARKER;
}

// TODO: move
CCoinsView mastercore::viewDummy;
CCoinsViewCache mastercore::view(&viewDummy);

//! Guards coins view cache
CCriticalSection mastercore::cs_tx_cache;

static unsigned int nCacheHits = 0;
static unsigned int nCacheMiss = 0;

/**
 * Fetches transaction inputs and adds them to the coins view cache.
 *
 * @param tx[in]  The transaction to fetch inputs for
 * @return True, if all inputs were successfully added to the cache
 */
static bool FillTxInputCache(const CTransaction& tx)
{
    LOCK(cs_tx_cache);
    static unsigned int nCacheSize = GetArg("-omnitxcache", 500000);

    if (view.GetCacheSize() > nCacheSize) {
        PrintToLog("%s(): clearing cache before insertion [size=%d, hit=%d, miss=%d]\n",
                __func__, view.GetCacheSize(), nCacheHits, nCacheMiss);
        view.Flush();
    }

    for (std::vector<CTxIn>::const_iterator it = tx.vin.begin(); it != tx.vin.end(); ++it) {
        const CTxIn& txIn = *it;
        unsigned int nOut = txIn.prevout.n;
        CCoinsModifier coins = view.ModifyCoins(txIn.prevout.hash);

        if (coins->IsAvailable(nOut)) {
            ++nCacheHits;
            continue;
        } else {
            ++nCacheMiss;
        }

        CTransaction txPrev;
        uint256 hashBlock;
        if (!GetTransaction(txIn.prevout.hash, txPrev, hashBlock, true)) {
            return false;
        }

        if (nOut >= coins->vout.size()) {
            coins->vout.resize(nOut+1);
        }
        coins->vout[nOut].scriptPubKey = txPrev.vout[nOut].scriptPubKey;
        coins->vout[nOut].nValue = txPrev.vout[nOut].nValue;
    }

    return true;
}

// idx is position within the block, 0-based
// int msc_tx_push(const CTransaction &wtx, int nBlock, unsigned int idx)
// INPUT: bRPConly -- set to true to avoid moving funds; to be called from various RPC calls like this
// RETURNS: 0 if parsed a MP TX
// RETURNS: < 0 if a non-MP-TX or invalid
// RETURNS: >0 if 1 or more payments have been made
static int parseTransaction(bool bRPConly, const CTransaction& wtx, int nBlock, unsigned int idx, CMPTransaction& mp_tx, unsigned int nTime)
{
    assert(bRPConly == mp_tx.isRpcOnly());
    mp_tx.Set(wtx.GetHash(), nBlock, idx, nTime);

    // ### CLASS IDENTIFICATION AND MARKER CHECK ###
    int omniClass = GetEncodingClass(wtx, nBlock);

    if (omniClass == NO_MARKER) {
        return -1; // No Exodus/Omni marker, thus not a valid Omni transaction
    }

    if (!bRPConly || msc_debug_parser_readonly) {
        PrintToLog("____________________________________________________________________________________________________________________________________\n");
        PrintToLog("%s(block=%d, %s idx= %d); txid: %s\n", __FUNCTION__, nBlock, DateTimeStrFormat("%Y-%m-%d %H:%M:%S", nTime), idx, wtx.GetHash().GetHex());
    }

    // Add previous transaction inputs to the cache
    if (!FillTxInputCache(wtx)) {
        PrintToLog("%s() ERROR: failed to get inputs for %s\n", __func__, wtx.GetHash().GetHex());
        return -101;
    }

    assert(view.HaveInputs(wtx));

    // ### SENDER IDENTIFICATION ###
    std::string strSender;

    if (omniClass != OMNI_CLASS_C)
    {
        // OLD LOGIC - collect input amounts and identify sender via "largest input by sum"
        std::map<std::string, int64_t> inputs_sum_of_values;

        for (unsigned int i = 0; i < wtx.vin.size(); ++i) {
            if (msc_debug_vin) PrintToLog("vin=%d:%s\n", i, wtx.vin[i].scriptSig.ToString());

            const CTxIn& txIn = wtx.vin[i];
            const CTxOut& txOut = view.GetOutputFor(txIn);

            assert(!txOut.IsNull());

            CTxDestination source;
            txnouttype whichType;
            if (!GetOutputType(txOut.scriptPubKey, whichType)) {
                return -104;
            }
            if (!IsAllowedInputType(whichType, nBlock)) {
                return -105;
            }
            if (ExtractDestination(txOut.scriptPubKey, source)) { // extract the destination of the previous transaction's vout[n] and check it's allowed type
                CBitcoinAddress addressSource(source);
                inputs_sum_of_values[addressSource.ToString()] += txOut.nValue;
            }
            else return -106;
        }

        int64_t nMax = 0;
        for (std::map<std::string, int64_t>::iterator it = inputs_sum_of_values.begin(); it != inputs_sum_of_values.end(); ++it) { // find largest by sum
            int64_t nTemp = it->second;
            if (nTemp > nMax) {
                strSender = it->first;
                if (msc_debug_exo) PrintToLog("looking for The Sender: %s , nMax=%lu, nTemp=%d\n", strSender, nMax, nTemp);
                nMax = nTemp;
            }
        }
    }
    else
    {
        // NEW LOGIC - the sender is chosen based on the first vin

        // determine the sender, but invalidate transaction, if the input is not accepted
        {
            unsigned int vin_n = 0; // the first input
            if (msc_debug_vin) PrintToLog("vin=%d:%s\n", vin_n, wtx.vin[vin_n].scriptSig.ToString());

            const CTxIn& txIn = wtx.vin[vin_n];
            const CTxOut& txOut = view.GetOutputFor(txIn);

            assert(!txOut.IsNull());

            txnouttype whichType;
            if (!GetOutputType(txOut.scriptPubKey, whichType)) {
                return -108;
            }
            if (!IsAllowedInputType(whichType, nBlock)) {
                return -109;
            }
            CTxDestination source;
            if (ExtractDestination(txOut.scriptPubKey, source)) {
                strSender = CBitcoinAddress(source).ToString();
            }
            else return -110;
        }
    }

    int64_t inAll = view.GetValueIn(wtx);
    int64_t outAll = wtx.GetValueOut();
    int64_t txFee = inAll - outAll; // miner fee

    if (!strSender.empty()) {
        if (msc_debug_verbose) PrintToLog("The Sender: %s : fee= %s\n", strSender, FormatDivisibleMP(txFee));
    } else {
        PrintToLog("The sender is still EMPTY !!! txid: %s\n", wtx.GetHash().GetHex());
        return -5;
    }

    // ### DATA POPULATION ### - save output addresses, values and scripts
    std::string strReference;
    unsigned char single_pkt[MAX_PACKETS * PACKET_SIZE];
    unsigned int packet_size = 0;
    std::vector<std::string> script_data;
    std::vector<std::string> address_data;
    std::vector<int64_t> value_data;

    for (unsigned int n = 0; n < wtx.vout.size(); ++n) {
        txnouttype whichType;
        if (!GetOutputType(wtx.vout[n].scriptPubKey, whichType)) {
            continue;
        }
        if (!IsAllowedOutputType(whichType, nBlock)) {
            continue;
        }
        CTxDestination dest;
        if (ExtractDestination(wtx.vout[n].scriptPubKey, dest)) {
            CBitcoinAddress address(dest);
            if (!(address == ExodusAddress())) {
                // saving for Class A processing or reference
                GetScriptPushes(wtx.vout[n].scriptPubKey, script_data);
                address_data.push_back(address.ToString());
                value_data.push_back(wtx.vout[n].nValue);
                if (msc_debug_parser_data) PrintToLog("saving address_data #%d: %s:%s\n", n, address.ToString(), wtx.vout[n].scriptPubKey.ToString());
            }
        }
    }
    if (msc_debug_parser_data) PrintToLog(" address_data.size=%lu\n script_data.size=%lu\n value_data.size=%lu\n", address_data.size(), script_data.size(), value_data.size());

    // ### CLASS A PARSING ###
    if (omniClass == OMNI_CLASS_A) {
        std::string strScriptData;
        std::string strDataAddress;
        std::string strRefAddress;
        unsigned char dataAddressSeq = 0xFF;
        unsigned char seq = 0xFF;
        int64_t dataAddressValue = 0;
        for (unsigned k = 0; k < script_data.size(); ++k) { // Step 1, locate the data packet
            std::string strSub = script_data[k].substr(2,16); // retrieve bytes 1-9 of packet for peek & decode comparison
            seq = (ParseHex(script_data[k].substr(0,2)))[0]; // retrieve sequence number
            if ("0000000000000001" == strSub || "0000000000000002" == strSub) { // peek & decode comparison
                if (strScriptData.empty()) { // confirm we have not already located a data address
                    strScriptData = script_data[k].substr(2*1,2*PACKET_SIZE_CLASS_A); // populate data packet
                    strDataAddress = address_data[k]; // record data address
                    dataAddressSeq = seq; // record data address seq num for reference matching
                    dataAddressValue = value_data[k]; // record data address amount for reference matching
                    if (msc_debug_parser_data) PrintToLog("Data Address located - data[%d]:%s: %s (%s)\n", k, script_data[k], address_data[k], FormatDivisibleMP(value_data[k]));
                } else { // invalidate - Class A cannot be more than one data packet - possible collision, treat as default (BTC payment)
                    strDataAddress.clear(); //empty strScriptData to block further parsing
                    if (msc_debug_parser_data) PrintToLog("Multiple Data Addresses found (collision?) Class A invalidated, defaulting to BTC payment\n");
                    break;
                }
            }
        }
        if (!strDataAddress.empty()) { // Step 2, try to locate address with seqnum = DataAddressSeq+1 (also verify Step 1, we should now have a valid data packet)
            unsigned char expectedRefAddressSeq = dataAddressSeq + 1;
            for (unsigned k = 0; k < script_data.size(); ++k) { // loop through outputs
                seq = (ParseHex(script_data[k].substr(0,2)))[0]; // retrieve sequence number
                if ((address_data[k] != strDataAddress) && (address_data[k] != exodus_address) && (expectedRefAddressSeq == seq)) { // found reference address with matching sequence number
                    if (strRefAddress.empty()) { // confirm we have not already located a reference address
                        strRefAddress = address_data[k]; // set ref address
                        if (msc_debug_parser_data) PrintToLog("Reference Address located via seqnum - data[%d]:%s: %s (%s)\n", k, script_data[k], address_data[k], FormatDivisibleMP(value_data[k]));
                    } else { // can't trust sequence numbers to provide reference address, there is a collision with >1 address with expected seqnum
                        strRefAddress.clear(); // blank ref address
                        if (msc_debug_parser_data) PrintToLog("Reference Address sequence number collision, will fall back to evaluating matching output amounts\n");
                        break;
                    }
                }
            }
            std::vector<int64_t> ExodusValues;
            for (unsigned int n = 0; n < wtx.vout.size(); ++n) {
                CTxDestination dest;
                if (ExtractDestination(wtx.vout[n].scriptPubKey, dest)) {
                    if (CBitcoinAddress(dest) == ExodusAddress()) {
                        ExodusValues.push_back(wtx.vout[n].nValue);
                    }
                }
            }
            if (strRefAddress.empty()) { // Step 3, if we still don't have a reference address, see if we can locate an address with matching output amounts
                for (unsigned k = 0; k < script_data.size(); ++k) { // loop through outputs
                    if ((address_data[k] != strDataAddress) && (address_data[k] != exodus_address) && (dataAddressValue == value_data[k])) { // this output matches data output, check if matches exodus output
                        for (unsigned int exodus_idx = 0; exodus_idx < ExodusValues.size(); exodus_idx++) {
                            if (value_data[k] == ExodusValues[exodus_idx]) { //this output matches data address value and exodus address value, choose as ref
                                if (strRefAddress.empty()) {
                                    strRefAddress = address_data[k];
                                    if (msc_debug_parser_data) PrintToLog("Reference Address located via matching amounts - data[%d]:%s: %s (%s)\n", k, script_data[k], address_data[k], FormatDivisibleMP(value_data[k]));
                                } else {
                                    strRefAddress.clear();
                                    if (msc_debug_parser_data) PrintToLog("Reference Address collision, multiple potential candidates. Class A invalidated, defaulting to BTC payment\n");
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        } // end if (!strDataAddress.empty())
        if (!strRefAddress.empty()) {
            strReference = strRefAddress; // populate expected var strReference with chosen address (if not empty)
        }
        if (strRefAddress.empty()) {
            strDataAddress.clear(); // last validation step, if strRefAddress is empty, blank strDataAddress so we default to BTC payment
        }
        if (!strDataAddress.empty()) { // valid Class A packet almost ready
            if (msc_debug_parser_data) PrintToLog("valid Class A:from=%s:to=%s:data=%s\n", strSender, strReference, strScriptData);
            packet_size = PACKET_SIZE_CLASS_A;
            memcpy(single_pkt, &ParseHex(strScriptData)[0], packet_size);
        } else {
            if ((!bRPConly || msc_debug_parser_readonly) && msc_debug_parser_dex) {
                PrintToLog("!! sender: %s , receiver: %s\n", strSender, strReference);
                PrintToLog("!! this may be the BTC payment for an offer !!\n");
            }
        }
    }
    // ### CLASS B / CLASS C PARSING ###
    if ((omniClass == OMNI_CLASS_B) || (omniClass == OMNI_CLASS_C)) {
        if (msc_debug_parser_data) PrintToLog("Beginning reference identification\n");
        bool referenceFound = false; // bool to hold whether we've found the reference yet
        bool changeRemoved = false; // bool to hold whether we've ignored the first output to sender as change
        unsigned int potentialReferenceOutputs = 0; // int to hold number of potential reference outputs
        for (unsigned k = 0; k < address_data.size(); ++k) { // how many potential reference outputs do we have, if just one select it right here
            const std::string& addr = address_data[k];
            if (msc_debug_parser_data) PrintToLog("ref? data[%d]:%s: %s (%s)\n", k, script_data[k], addr, FormatIndivisibleMP(value_data[k]));
            if (addr != exodus_address) {
                ++potentialReferenceOutputs;
                if (1 == potentialReferenceOutputs) {
                    strReference = addr;
                    referenceFound = true;
                    if (msc_debug_parser_data) PrintToLog("Single reference potentially id'd as follows: %s \n", strReference);
                } else { //as soon as potentialReferenceOutputs > 1 we need to go fishing
                    strReference.clear(); // avoid leaving strReference populated for sanity
                    referenceFound = false;
                    if (msc_debug_parser_data) PrintToLog("More than one potential reference candidate, blanking strReference, need to go fishing\n");
                }
            }
        }
        if (!referenceFound) { // do we have a reference now? or do we need to dig deeper
            if (msc_debug_parser_data) PrintToLog("Reference has not been found yet, going fishing\n");
            for (unsigned k = 0; k < address_data.size(); ++k) {
                const std::string& addr = address_data[k];
                if (addr != exodus_address) { // removed strSender restriction, not to spec
                    if (addr == strSender && !changeRemoved) {
                        changeRemoved = true; // per spec ignore first output to sender as change if multiple possible ref addresses
                        if (msc_debug_parser_data) PrintToLog("Removed change\n");
                    } else {
                        strReference = addr; // this may be set several times, but last time will be highest vout
                        if (msc_debug_parser_data) PrintToLog("Resetting strReference as follows: %s \n ", strReference);
                    }
                }
            }
        }
        if (msc_debug_parser_data) PrintToLog("Ending reference identification\nFinal decision on reference identification is: %s\n", strReference);

        // ### CLASS B SPECIFC PARSING ###
        if (omniClass == OMNI_CLASS_B) {
            std::vector<std::string> multisig_script_data;

            // ### POPULATE MULTISIG SCRIPT DATA ###
            for (unsigned int i = 0; i < wtx.vout.size(); ++i) {
                txnouttype whichType;
                std::vector<CTxDestination> vDest;
                int nRequired;
                if (msc_debug_script) PrintToLog("scriptPubKey: %s\n", HexStr(wtx.vout[i].scriptPubKey));
                if (!ExtractDestinations(wtx.vout[i].scriptPubKey, whichType, vDest, nRequired)) {
                    continue;
                }
                if (whichType == TX_MULTISIG) {
                    if (msc_debug_script) {
                        PrintToLog(" >> multisig: ");
                        BOOST_FOREACH(const CTxDestination& dest, vDest) {
                            PrintToLog("%s ; ", CBitcoinAddress(dest).ToString());
                        }
                        PrintToLog("\n");
                    }
                    // ignore first public key, as it should belong to the sender
                    // and it be used to avoid the creation of unspendable dust
                    GetScriptPushes(wtx.vout[i].scriptPubKey, multisig_script_data, true);
                }
            }

            // The number of packets is limited to MAX_PACKETS,
            // which allows, at least in theory, to add 1 byte
            // sequence numbers to each packet.

            // Transactions with more than MAX_PACKET packets
            // are not invalidated, but trimmed.

            unsigned int nPackets = multisig_script_data.size();
            if (nPackets > MAX_PACKETS) {
                nPackets = MAX_PACKETS;
                PrintToLog("limiting number of packets to %d [extracted=%d]\n", nPackets, multisig_script_data.size());
            }

            // ### PREPARE A FEW VARS ###
            std::string strObfuscatedHashes[1+MAX_SHA256_OBFUSCATION_TIMES];
            PrepareObfuscatedHashes(strSender, strObfuscatedHashes);
            unsigned char packets[MAX_PACKETS][32];
            unsigned int mdata_count = 0;  // multisig data count

            // ### DEOBFUSCATE MULTISIG PACKETS ###
            for (unsigned int k = 0; k < nPackets; ++k) {
                assert(mdata_count < MAX_PACKETS);
                assert(mdata_count < MAX_SHA256_OBFUSCATION_TIMES);

                std::vector<unsigned char> hash = ParseHex(strObfuscatedHashes[mdata_count+1]);
                std::vector<unsigned char> packet = ParseHex(multisig_script_data[k].substr(2*1,2*PACKET_SIZE));
                for (unsigned int i = 0; i < packet.size(); i++) { // this is a data packet, must deobfuscate now
                    packet[i] ^= hash[i];
                }
                memcpy(&packets[mdata_count], &packet[0], PACKET_SIZE);
                ++mdata_count;

                if (msc_debug_parser_data) {
                    CPubKey key(ParseHex(multisig_script_data[k]));
                    CKeyID keyID = key.GetID();
                    std::string strAddress = CBitcoinAddress(keyID).ToString();
                    PrintToLog("multisig_data[%d]:%s: %s\n", k, multisig_script_data[k], strAddress);
                }
                if (msc_debug_parser) {
                    if (!packet.empty()) {
                        std::string strPacket = HexStr(packet.begin(), packet.end());
                        PrintToLog("packet #%d: %s\n", mdata_count, strPacket);
                    }
                }
            }
            packet_size = mdata_count * (PACKET_SIZE - 1);
            assert(packet_size <= sizeof(single_pkt));

            // ### FINALIZE CLASS B ###
            for (unsigned int m = 0; m < mdata_count; ++m) { // now decode mastercoin packets
                if (msc_debug_parser) PrintToLog("m=%d: %s\n", m, HexStr(packets[m], PACKET_SIZE + packets[m], false));

                // check to ensure the sequence numbers are sequential and begin with 01 !
                if (1 + m != packets[m][0]) {
                    if (msc_debug_spec) PrintToLog("Error: non-sequential seqnum ! expected=%d, got=%d\n", 1+m, packets[m][0]);
                }

                memcpy(m*(PACKET_SIZE-1)+single_pkt, 1+packets[m], PACKET_SIZE-1); // now ignoring sequence numbers for Class B packets
            }
        }

        // ### CLASS C SPECIFIC PARSING ###
        if (omniClass == OMNI_CLASS_C) {
            std::vector<std::string> op_return_script_data;

            // ### POPULATE OP RETURN SCRIPT DATA ###
            for (unsigned int n = 0; n < wtx.vout.size(); ++n) {
                txnouttype whichType;
                if (!GetOutputType(wtx.vout[n].scriptPubKey, whichType)) {
                    continue;
                }
                if (!IsAllowedOutputType(whichType, nBlock)) {
                    continue;
                }
                if (whichType == TX_NULL_DATA) {
                    // only consider outputs, which are explicitly tagged
                    std::vector<std::string> vstrPushes;
                    if (!GetScriptPushes(wtx.vout[n].scriptPubKey, vstrPushes)) {
                        continue;
                    }
                    // TODO: maybe encapsulate the following sort of messy code
                    if (!vstrPushes.empty()) {
                        std::vector<unsigned char> vchMarker = GetOmMarker();
                        std::vector<unsigned char> vchPushed = ParseHex(vstrPushes[0]);
                        if (vchPushed.size() < vchMarker.size()) {
                            continue;
                        }
                        if (std::equal(vchMarker.begin(), vchMarker.end(), vchPushed.begin())) {
                            size_t sizeHex = vchMarker.size() * 2;
                            // strip out the marker at the very beginning
                            vstrPushes[0] = vstrPushes[0].substr(sizeHex);
                            // add the data to the rest
                            op_return_script_data.insert(op_return_script_data.end(), vstrPushes.begin(), vstrPushes.end());

                            if (msc_debug_parser_data) {
                                PrintToLog("Class C transaction detected: %s parsed to %s at vout %d\n", wtx.GetHash().GetHex(), vstrPushes[0], n);
                            }
                        }
                    }
                }
            }
            // ### EXTRACT PAYLOAD FOR CLASS C ###
            for (unsigned int n = 0; n < op_return_script_data.size(); ++n) {
                if (!op_return_script_data[n].empty()) {
                    assert(IsHex(op_return_script_data[n])); // via GetScriptPushes()
                    std::vector<unsigned char> vch = ParseHex(op_return_script_data[n]);
                    unsigned int payload_size = vch.size();
                    if (packet_size + payload_size > MAX_PACKETS * PACKET_SIZE) {
                        payload_size = MAX_PACKETS * PACKET_SIZE - packet_size;
                        PrintToLog("limiting payload size to %d byte\n", packet_size + payload_size);
                    }
                    if (payload_size > 0) {
                        memcpy(single_pkt+packet_size, &vch[0], payload_size);
                        packet_size += payload_size;
                    }
                    if (MAX_PACKETS * PACKET_SIZE == packet_size) {
                        break;
                    }
                }
            }
        }
    }

    // ### SET MP TX INFO ###
    if (msc_debug_verbose) PrintToLog("single_pkt: %s\n", HexStr(single_pkt, packet_size + single_pkt));
    mp_tx.Set(strSender, strReference, 0, wtx.GetHash(), nBlock, idx, (unsigned char *)&single_pkt, packet_size, omniClass, (inAll-outAll));

    // TODO: the following is a bit aweful
    // Provide a hint for DEx payments
    if (omniClass == OMNI_CLASS_A && packet_size == 0) {
        return 1;
    }

    return 0;
}

/**
 * Provides access to parseTransaction in read-only mode.
 */
int ParseTransaction(const CTransaction& tx, int nBlock, unsigned int idx, CMPTransaction& mptx, unsigned int nTime)
{
    return parseTransaction(true, tx, nBlock, idx, mptx, nTime);
}

/**
 * Handles potential DEx payments.
 *
 * Note: must *not* be called outside of the transaction handler, and it does not
 * check, if a transaction marker exists.
 *
 * @return True, if valid
 */
static bool HandleDExPayments(const CTransaction& tx, int nBlock, const std::string& strSender)
{
    int count = 0;

    for (unsigned int n = 0; n < tx.vout.size(); ++n) {
        CTxDestination dest;
        if (ExtractDestination(tx.vout[n].scriptPubKey, dest)) {
            CBitcoinAddress address(dest);
            if (address == ExodusAddress()) {
                continue;
            }
            std::string strAddress = address.ToString();
            if (msc_debug_parser_dex) PrintToLog("payment #%d %s %s\n", count, strAddress, FormatIndivisibleMP(tx.vout[n].nValue));

            // check everything and pay BTC for the property we are buying here...
            if (0 == DEx_payment(tx.GetHash(), n, strAddress, strSender, tx.vout[n].nValue, nBlock)) ++count;
        }
    }

    return (count > 0);
}

/**
 * Handles potential Exodus crowdsale purchases.
 *
 * Note: must *not* be called outside of the transaction handler, and it does not
 * check, if a transaction marker exists.
 *
 * @return True, if it was a valid purchase
 */
static bool HandleExodusPurchase(const CTransaction& tx, int nBlock, const std::string& strSender, unsigned int nTime)
{
    int64_t amountInvested = 0;

    for (unsigned int n = 0; n < tx.vout.size(); ++n) {
        CTxDestination dest;
        if (ExtractDestination(tx.vout[n].scriptPubKey, dest)) {
            if (CBitcoinAddress(dest) == ExodusCrowdsaleAddress(nBlock)) {
                amountInvested = tx.vout[n].nValue;
                break; // TODO: maybe sum all values
            }
        }
    }

    if (0 < amountInvested) {
        return TXExodusFundraiser(tx, strSender, amountInvested, nBlock, nTime);
    }

    return false;
}

/**
 * Reports the progress of the initial transaction scanning.
 *
 * The progress is printed to the console, written to the debug log file, and
 * the RPC status, as well as the splash screen progress label, are updated.
 *
 * @see msc_initial_scan()
 */
class ProgressReporter
{
private:
    const CBlockIndex* m_pblockFirst;
    const CBlockIndex* m_pblockLast;
    const int64_t m_timeStart;

    /** Returns the estimated remaining time in milliseconds. */
    int64_t estimateRemainingTime(double progress) const
    {
        int64_t timeSinceStart = GetTimeMillis() - m_timeStart;

        double timeRemaining = 3600000.0; // 1 hour
        if (progress > 0.0 && timeSinceStart > 0) {
            timeRemaining = (100.0 - progress) / progress * timeSinceStart;
        }

        return static_cast<int64_t>(timeRemaining);
    }

    /** Converts a time span to a human readable string. */
    std::string remainingTimeAsString(int64_t remainingTime) const
    {
        int64_t secondsTotal = 0.001 * remainingTime;
        int64_t hours = secondsTotal / 3600;
        int64_t minutes = secondsTotal / 60;
        int64_t seconds = secondsTotal % 60;

        if (hours > 0) {
            return strprintf("%d:%02d:%02d hours", hours, minutes, seconds);
        } else if (minutes > 0) {
            return strprintf("%d:%02d minutes", minutes, seconds);
        } else {
            return strprintf("%d seconds", seconds);
        }
    }

public:
    ProgressReporter(const CBlockIndex* pblockFirst, const CBlockIndex* pblockLast)
    : m_pblockFirst(pblockFirst), m_pblockLast(pblockLast), m_timeStart(GetTimeMillis())
    {
    }

    /** Prints the current progress to the console and notifies the UI. */
    void update(const CBlockIndex* pblockNow) const
    {
        int nLastBlock = m_pblockLast->nHeight;
        int nCurrentBlock = pblockNow->nHeight;
        unsigned int nFirst = m_pblockFirst->nChainTx;
        unsigned int nCurrent = pblockNow->nChainTx;
        unsigned int nLast = m_pblockLast->nChainTx;

        double dProgress = 100.0 * (nCurrent - nFirst) / (nLast - nFirst);
        int64_t nRemainingTime = estimateRemainingTime(dProgress);

        std::string strProgress = strprintf(
                "Still scanning.. at block %d of %d. Progress: %.2f %%, about %s remaining..\n",
                nCurrentBlock, nLastBlock, dProgress, remainingTimeAsString(nRemainingTime));
        std::string strProgressUI = strprintf(
                "Still scanning.. at block %d of %d.\nProgress: %.2f %% (about %s remaining)",
                nCurrentBlock, nLastBlock, dProgress, remainingTimeAsString(nRemainingTime));

        PrintToConsole(strProgress);
        uiInterface.InitMessage(strProgressUI);
    }
};

/**
 * Scans the blockchain for meta transactions.
 *
 * It scans the blockchain, starting at the given block index, to the current
 * tip, much like as if new block were arriving and being processed on the fly.
 *
 * Every 30 seconds the progress of the scan is reported.
 *
 * In case the current block being processed is not part of the active chain, or
 * if a block could not be retrieved from the disk, then the scan stops early.
 * Likewise, global shutdown requests are honored, and stop the scan progress.
 *
 * @see mastercore_handler_block_begin()
 * @see mastercore_handler_tx()
 * @see mastercore_handler_block_end()
 *
 * @param nFirstBlock[in]  The index of the first block to scan
 * @return An exit code, indicating success or failure
 */
static int msc_initial_scan(int nFirstBlock)
{
    int nTimeBetweenProgressReports = GetArg("-omniprogressfrequency", 30);  // seconds
    int64_t nNow = GetTime();
    unsigned int nTxsTotal = 0;
    unsigned int nTxsFoundTotal = 0;
    int nBlock = 999999;
    const int nLastBlock = GetHeight();

    // this function is useless if there are not enough blocks in the blockchain yet!
    if (nFirstBlock < 0 || nLastBlock < nFirstBlock) return -1;
    PrintToConsole("Scanning for transactions in block %d to block %d..\n", nFirstBlock, nLastBlock);

    // used to print the progress to the console and notifies the UI
    ProgressReporter progressReporter(chainActive[nFirstBlock], chainActive[nLastBlock]);

    // check if using seed block filter should be disabled
    bool seedBlockFilterEnabled = GetBoolArg("-omniseedblockfilter", true);

    for (nBlock = nFirstBlock; nBlock <= nLastBlock; ++nBlock)
    {
        if (ShutdownRequested()) {
            PrintToLog("Shutdown requested, stop scan at block %d of %d\n", nBlock, nLastBlock);
            break;
        }

        CBlockIndex* pblockindex = chainActive[nBlock];
        if (NULL == pblockindex) break;
        std::string strBlockHash = pblockindex->GetBlockHash().GetHex();

        if (msc_debug_exo) PrintToLog("%s(%d; max=%d):%s, line %d, file: %s\n",
            __FUNCTION__, nBlock, nLastBlock, strBlockHash, __LINE__, __FILE__);

        if (GetTime() >= nNow + nTimeBetweenProgressReports) {
            progressReporter.update(pblockindex);
            nNow = GetTime();
        }

        unsigned int nTxNum = 0;
        unsigned int nTxsFoundInBlock = 0;
        mastercore_handler_block_begin(nBlock, pblockindex);

        if (!seedBlockFilterEnabled || !SkipBlock(nBlock)) {
            CBlock block;
            if (!ReadBlockFromDisk(block, pblockindex)) break;

            BOOST_FOREACH(const CTransaction&tx, block.vtx) {
                if (mastercore_handler_tx(tx, nBlock, nTxNum, pblockindex)) ++nTxsFoundInBlock;
                ++nTxNum;
            }
        }

        nTxsFoundTotal += nTxsFoundInBlock;
        nTxsTotal += nTxNum;
        mastercore_handler_block_end(nBlock, pblockindex, nTxsFoundInBlock);
    }

    if (nBlock < nLastBlock) {
        PrintToConsole("Scan stopped early at block %d of block %d\n", nBlock, nLastBlock);
    }

    PrintToConsole("%d transactions processed, %d meta transactions found\n", nTxsTotal, nTxsFoundTotal);

    return 0;
}

int input_msc_balances_string(const std::string& s)
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
int input_mp_offers_string(const std::string& s)
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
    uint256 txid = uint256(vstr[i++]);

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
int input_mp_accepts_string(const string &s)
{
  int nBlock;
  unsigned char blocktimelimit;
  std::vector<std::string> vstr;
  boost::split(vstr, s, boost::is_any_of(" ,="), token_compress_on);
  uint64_t amountRemaining, amountOriginal, offerOriginal, btcDesired;
  unsigned int prop;
  string sellerAddr, buyerAddr, txidStr;
  int i = 0;

  if (10 != vstr.size()) return -1;

  sellerAddr = vstr[i++];
  prop = boost::lexical_cast<unsigned int>(vstr[i++]);
  buyerAddr = vstr[i++];
  nBlock = atoi(vstr[i++]);
  amountRemaining = boost::lexical_cast<uint64_t>(vstr[i++]);
  amountOriginal = boost::lexical_cast<uint64_t>(vstr[i++]);
  blocktimelimit = atoi(vstr[i++]);
  offerOriginal = boost::lexical_cast<uint64_t>(vstr[i++]);
  btcDesired = boost::lexical_cast<uint64_t>(vstr[i++]);
  txidStr = vstr[i++];

  const string combo = STR_ACCEPT_ADDR_PROP_ADDR_COMBO(sellerAddr, buyerAddr, prop);
  CMPAccept newAccept(amountOriginal, amountRemaining, nBlock, blocktimelimit, prop, offerOriginal, btcDesired, uint256(txidStr));
  if (my_accepts.insert(std::make_pair(combo, newAccept)).second) {
    return 0;
  } else {
    return -1;
  }
}

// exodus_prev
int input_globals_state_string(const string &s)
{
  uint64_t exodusPrev;
  unsigned int nextSPID, nextTestSPID;
  std::vector<std::string> vstr;
  boost::split(vstr, s, boost::is_any_of(" ,="), token_compress_on);
  if (3 != vstr.size()) return -1;

  int i = 0;
  exodusPrev = boost::lexical_cast<uint64_t>(vstr[i++]);
  nextSPID = boost::lexical_cast<unsigned int>(vstr[i++]);
  nextTestSPID = boost::lexical_cast<unsigned int>(vstr[i++]);

  exodus_prev = exodusPrev;
  _my_sps->init(nextSPID, nextTestSPID);
  return 0;
}

// addr,propertyId,nValue,property_desired,deadline,early_bird,percentage,txid
int input_mp_crowdsale_string(const std::string& s)
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

        uint256 txHash(entryData[0]);
        newCrowdsale.insertDatabase(txHash, vals);
    }

    if (!my_crowds.insert(std::make_pair(sellerAddr, newCrowdsale)).second) {
        return -1;
    }

    return 0;
}

// address, block, amount for sale, property, amount desired, property desired, subaction, idx, txid, amount remaining
int input_mp_mdexorder_string(const std::string& s)
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
    uint256 txid = uint256(vstr[i++]);
    int64_t amount_remaining = boost::lexical_cast<int64_t>(vstr[i++]);

    CMPMetaDEx mdexObj(addr, block, property, amount_forsale, desired_property,
            amount_desired, txid, idx, subaction, amount_remaining);

    if (!MetaDEx_INSERT(mdexObj)) return -1;

    return 0;
}

static int msc_file_load(const string &filename, int what, bool verifyHash = false)
{
  int lines = 0;
  int (*inputLineFunc)(const string &) = NULL;

  SHA256_CTX shaCtx;
  SHA256_Init(&shaCtx);

  switch (what)
  {
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

  if (msc_debug_persistence)
  {
    LogPrintf("Loading %s ... \n", filename);
    PrintToLog("%s(%s), line %d, file: %s\n", __FUNCTION__, filename, __LINE__, __FILE__);
  }

  std::ifstream file;
  file.open(filename.c_str());
  if (!file.is_open())
  {
    if (msc_debug_persistence) LogPrintf("%s(%s): file not found, line %d, file: %s\n", __FUNCTION__, filename, __LINE__, __FILE__);
    return -1;
  }

  int res = 0;

  std::string fileHash;
  while (file.good())
  {
    std::string line;
    std::getline(file, line);
    if (line.empty() || line[0] == '#') continue;

    // remove \r if the file came from Windows
    line.erase( std::remove( line.begin(), line.end(), '\r' ), line.end() ) ;

    // record and skip hashes in the file
    if (line[0] == '!') {
      fileHash = line.substr(1);
      continue;
    }

    // update hash?
    if (verifyHash) {
      SHA256_Update(&shaCtx, line.c_str(), line.length());
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
    // generate and wite the double hash of all the contents written
    uint256 hash1;
    SHA256_Final((unsigned char*)&hash1, &shaCtx);
    uint256 hash2;
    SHA256((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);

    if (false == boost::iequals(hash2.ToString(), fileHash)) {
      PrintToLog("File %s loaded, but failed hash validation!\n", filename);
      res = -1;
    }
  }

  PrintToLog("%s(%s), loaded lines= %d, res= %d\n", __FUNCTION__, filename, lines, res);
  LogPrintf("%s(): file: %s , loaded lines= %d, res= %d\n", __FUNCTION__, filename, lines, res);

  return res;
}

static char const * const statePrefix[NUM_FILETYPES] = {
    "balances",
    "offers",
    "accepts",
    "globals",
    "crowdsales",
    "mdexorders",
};

// returns the height of the state loaded
static int load_most_relevant_state()
{
  int res = -1;
  // check the SP database and roll it back to its latest valid state
  // according to the active chain
  uint256 spWatermark;
  if (!_my_sps->getWatermark(spWatermark)) {
    //trigger a full reparse, if the SP database has no watermark
    return -1;
  }

  CBlockIndex const *spBlockIndex = GetBlockIndex(spWatermark);
  if (NULL == spBlockIndex) {
    //trigger a full reparse, if the watermark isn't a real block
    return -1;
  }

  while (NULL != spBlockIndex && false == chainActive.Contains(spBlockIndex)) {
    int remainingSPs = _my_sps->popBlock(spBlockIndex->GetBlockHash());
    if (remainingSPs < 0) {
      // trigger a full reparse, if the levelDB cannot roll back
      return -1;
    } /*else if (remainingSPs == 0) {
      // potential optimization here?
    }*/
    spBlockIndex = spBlockIndex->pprev;
    if (spBlockIndex != NULL) {
        _my_sps->setWatermark(spBlockIndex->GetBlockHash());
    }
  }

  // prepare a set of available files by block hash pruning any that are
  // not in the active chain
  std::set<uint256> persistedBlocks;
  boost::filesystem::directory_iterator dIter(MPPersistencePath);
  boost::filesystem::directory_iterator endIter;
  for (; dIter != endIter; ++dIter) {
    if (false == boost::filesystem::is_regular_file(dIter->status()) || dIter->path().empty()) {
      // skip funny business
      continue;
    }

    std::string fName = (*--dIter->path().end()).string();
    std::vector<std::string> vstr;
    boost::split(vstr, fName, boost::is_any_of("-."), token_compress_on);
    if (  vstr.size() == 3 &&
          boost::equals(vstr[2], "dat")) {
      uint256 blockHash;
      blockHash.SetHex(vstr[1]);
      CBlockIndex *pBlockIndex = GetBlockIndex(blockHash);
      if (pBlockIndex == NULL || false == chainActive.Contains(pBlockIndex)) {
        continue;
      }

      // this is a valid block in the active chain, store it
      persistedBlocks.insert(blockHash);
    }
  }

  // using the SP's watermark after its fixed-up as the tip
  // walk backwards until we find a valid and full set of persisted state files
  // for each block we discard, roll back the SP database
  // Note: to avoid rolling back all the way to the genesis block (which appears as if client is hung) abort after MAX_STATE_HISTORY attempts
  CBlockIndex const *curTip = spBlockIndex;
  int abortRollBackBlock;
  if (curTip != NULL) abortRollBackBlock = curTip->nHeight - (MAX_STATE_HISTORY+1);
  while (NULL != curTip && persistedBlocks.size() > 0 && curTip->nHeight > abortRollBackBlock) {
    if (persistedBlocks.find(spBlockIndex->GetBlockHash()) != persistedBlocks.end()) {
      int success = -1;
      for (int i = 0; i < NUM_FILETYPES; ++i) {
        boost::filesystem::path path = MPPersistencePath / strprintf("%s-%s.dat", statePrefix[i], curTip->GetBlockHash().ToString());
        const std::string strFile = path.string();
        success = msc_file_load(strFile, i, true);
        if (success < 0) {
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
    if (0 > _my_sps->popBlock(curTip->GetBlockHash())) {
      // trigger a full reparse, if the levelDB cannot roll back
      return -1;
    }
    curTip = curTip->pprev;
    if (curTip != NULL) {
        _my_sps->setWatermark(curTip->GetBlockHash());
    }
  }

  if (persistedBlocks.size() == 0) {
    // trigger a reparse if we exhausted the persistence files without success
    return -1;
  }

  // return the height of the block we settled at
  return res;
}

static int write_msc_balances(std::ofstream& file, SHA256_CTX* shaCtx)
{
    std::map<std::string, CMPTally>::iterator iter;
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
            SHA256_Update(shaCtx, lineOut.c_str(), lineOut.length());

            // write the line
            file << lineOut << endl;
        }
    }

    return 0;
}

static int write_mp_offers(ofstream &file, SHA256_CTX *shaCtx)
{
  OfferMap::const_iterator iter;
  for (iter = my_offers.begin(); iter != my_offers.end(); ++iter) {
    // decompose the key for address
    std::vector<std::string> vstr;
    boost::split(vstr, (*iter).first, boost::is_any_of("-"), token_compress_on);
    CMPOffer const &offer = (*iter).second;
    offer.saveOffer(file, shaCtx, vstr[0]);
  }


  return 0;
}

static int write_mp_metadex(ofstream &file, SHA256_CTX *shaCtx)
{
  for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it)
  {
    md_PricesMap & prices = my_it->second;
    for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it)
    {
      md_Set & indexes = (it->second);
      for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it)
      {
        CMPMetaDEx meta = *it;
        meta.saveOffer(file, shaCtx);
      }
    }
  }

  return 0;
}

static int write_mp_accepts(ofstream &file, SHA256_CTX *shaCtx)
{
  AcceptMap::const_iterator iter;
  for (iter = my_accepts.begin(); iter != my_accepts.end(); ++iter) {
    // decompose the key for address
    std::vector<std::string> vstr;
    boost::split(vstr, (*iter).first, boost::is_any_of("-+"), token_compress_on);
    CMPAccept const &accept = (*iter).second;
    accept.saveAccept(file, shaCtx, vstr[0], vstr[1]);
  }

  return 0;
}

static int write_globals_state(ofstream &file, SHA256_CTX *shaCtx)
{
  unsigned int nextSPID = _my_sps->peekNextSPID(OMNI_PROPERTY_MSC);
  unsigned int nextTestSPID = _my_sps->peekNextSPID(OMNI_PROPERTY_TMSC);
  std::string lineOut = strprintf("%d,%d,%d",
    exodus_prev,
    nextSPID,
    nextTestSPID);

  // add the line to the hash
  SHA256_Update(shaCtx, lineOut.c_str(), lineOut.length());

  // write the line
  file << lineOut << endl;

  return 0;
}

static int write_mp_crowdsales(std::ofstream& file, SHA256_CTX* shaCtx)
{
    for (CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it) {
        // decompose the key for address
        const CMPCrowd& crowd = it->second;
        crowd.saveCrowdSale(file, shaCtx, it->first);
    }

    return 0;
}

static int write_state_file( CBlockIndex const *pBlockIndex, int what )
{
  boost::filesystem::path path = MPPersistencePath / strprintf("%s-%s.dat", statePrefix[what], pBlockIndex->GetBlockHash().ToString());
  const std::string strFile = path.string();

  std::ofstream file;
  file.open(strFile.c_str());

  SHA256_CTX shaCtx;
  SHA256_Init(&shaCtx);

  int result = 0;

  switch(what) {
  case FILETYPE_BALANCES:
    result = write_msc_balances(file, &shaCtx);
    break;

  case FILETYPE_OFFERS:
    result = write_mp_offers(file, &shaCtx);
    break;

  case FILETYPE_ACCEPTS:
    result = write_mp_accepts(file, &shaCtx);
    break;

  case FILETYPE_GLOBALS:
    result = write_globals_state(file, &shaCtx);
    break;

  case FILETYPE_CROWDSALES:
      result = write_mp_crowdsales(file, &shaCtx);
      break;

  case FILETYPE_MDEXORDERS:
      result = write_mp_metadex(file, &shaCtx);
      break;
  }

  // generate and wite the double hash of all the contents written
  uint256 hash1;
  SHA256_Final((unsigned char*)&hash1, &shaCtx);
  uint256 hash2;
  SHA256((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
  file << "!" << hash2.ToString() << endl;

  file.flush();
  file.close();
  return result;
}

static bool is_state_prefix( std::string const &str )
{
  for (int i = 0; i < NUM_FILETYPES; ++i) {
    if (boost::equals(str,  statePrefix[i])) {
      return true;
    }
  }

  return false;
}

static void prune_state_files( CBlockIndex const *topIndex )
{
  // build a set of blockHashes for which we have any state files
  std::set<uint256> statefulBlockHashes;

  boost::filesystem::directory_iterator dIter(MPPersistencePath);
  boost::filesystem::directory_iterator endIter;
  for (; dIter != endIter; ++dIter) {
    std::string fName = dIter->path().empty() ? "<invalid>" : (*--dIter->path().end()).string();
    if (false == boost::filesystem::is_regular_file(dIter->status())) {
      // skip funny business
      PrintToLog("Non-regular file found in persistence directory : %s\n", fName);
      continue;
    }

    std::vector<std::string> vstr;
    boost::split(vstr, fName, boost::is_any_of("-."), token_compress_on);
    if (  vstr.size() == 3 &&
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
    if (NULL == curIndex || (topIndex->nHeight - curIndex->nHeight) > MAX_STATE_HISTORY ) {
     if (msc_debug_persistence)
     {
      if (curIndex) {
        PrintToLog("State from Block:%s is no longer need, removing files (age-from-tip: %d)\n", (*iter).ToString(), topIndex->nHeight - curIndex->nHeight);
      } else {
        PrintToLog("State from Block:%s is no longer need, removing files (not in index)\n", (*iter).ToString());
      }
     }

      // destroy the associated files!
      std::string strBlockHash = iter->ToString();
      for (int i = 0; i < NUM_FILETYPES; ++i) {
        boost::filesystem::path path = MPPersistencePath / strprintf("%s-%s.dat", statePrefix[i], strBlockHash);
        boost::filesystem::remove(path);
      }
    }
  }
}

int mastercore_save_state( CBlockIndex const *pBlockIndex )
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

    _my_sps->setWatermark(pBlockIndex->GetBlockHash());

    return 0;
}

/**
 * Clears the state of the system.
 */
void clear_all_state()
{
    LOCK2(cs_tally, cs_pending);

    // Memory based storage
    mp_tally_map.clear();
    my_offers.clear();
    my_accepts.clear();
    my_crowds.clear();
    metadex.clear();
    my_pending.clear();
    ResetConsensusParams();
    ClearActivations();
    ClearAlerts();

    // LevelDB based storage
    _my_sps->Clear();
    p_txlistdb->Clear();
    s_stolistdb->Clear();
    t_tradelistdb->Clear();
    assert(p_txlistdb->setDBVersion() == DB_VERSION); // new set of databases, set DB version
    exodus_prev = 0;
}

/**
 * Global handler to initialize Omni Core.
 *
 * @return An exit code, indicating success or failure
 */
int mastercore_init()
{
    LOCK(cs_tally);

    if (mastercoreInitialized) {
        // nothing to do
        return 0;
    }

    PrintToConsole("Initializing Omni Core v%s [%s]\n", OmniCoreVersion(), Params().NetworkIDString());

    PrintToLog("\nInitializing Omni Core v%s [%s]\n", OmniCoreVersion(), Params().NetworkIDString());
    PrintToLog("Startup time: %s\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()));
    PrintToLog("Build date: %s, based on commit: %s\n", BuildDate(), BuildCommit());

    InitDebugLogLevels();
    ShrinkDebugLog();

    if (isNonMainNet()) {
        exodus_address = exodus_testnet;
    }

    // check for --autocommit option and set transaction commit flag accordingly
    if (!GetBoolArg("-autocommit", true)) {
        PrintToLog("Process was started with --autocommit set to false. "
                "Created Omni transactions will not be committed to wallet or broadcast.\n");
        autoCommit = false;
    }

    // check for --startclean option and delete MP_ folders if present
    bool startClean = false;
    if (GetBoolArg("-startclean", false)) {
        PrintToLog("Process was started with --startclean option, attempting to clear persistence files..\n");
        try {
            boost::filesystem::path persistPath = GetDataDir() / "MP_persist";
            boost::filesystem::path txlistPath = GetDataDir() / "MP_txlist";
            boost::filesystem::path tradePath = GetDataDir() / "MP_tradelist";
            boost::filesystem::path spPath = GetDataDir() / "MP_spinfo";
            boost::filesystem::path stoPath = GetDataDir() / "MP_stolist";
            if (boost::filesystem::exists(persistPath)) boost::filesystem::remove_all(persistPath);
            if (boost::filesystem::exists(txlistPath)) boost::filesystem::remove_all(txlistPath);
            if (boost::filesystem::exists(tradePath)) boost::filesystem::remove_all(tradePath);
            if (boost::filesystem::exists(spPath)) boost::filesystem::remove_all(spPath);
            if (boost::filesystem::exists(stoPath)) boost::filesystem::remove_all(stoPath);
            PrintToLog("Success clearing persistence files in datadir %s\n", GetDataDir().string());
            startClean = true;
        } catch (const boost::filesystem::filesystem_error& e) {
            PrintToLog("Failed to delete persistence folders: %s\n", e.what());
            PrintToConsole("Failed to delete persistence folders: %s\n", e.what());
        }
    }

    t_tradelistdb = new CMPTradeList(GetDataDir() / "MP_tradelist", fReindex);
    s_stolistdb = new CMPSTOList(GetDataDir() / "MP_stolist", fReindex);
    p_txlistdb = new CMPTxList(GetDataDir() / "MP_txlist", fReindex);
    _my_sps = new CMPSPInfo(GetDataDir() / "MP_spinfo", fReindex);

    MPPersistencePath = GetDataDir() / "MP_persist";
    TryCreateDirectory(MPPersistencePath);

    bool wrongDBVersion = (p_txlistdb->getDBVersion() != DB_VERSION);

    ++mastercoreInitialized;

    nWaterlineBlock = load_most_relevant_state();
    bool noPreviousState = (nWaterlineBlock <= 0);

    if (startClean) {
        assert(p_txlistdb->setDBVersion() == DB_VERSION); // new set of databases, set DB version
    } else if (wrongDBVersion) {
        nWaterlineBlock = -1; // force a clear_all_state and parse from start
    }

    if (nWaterlineBlock > 0) {
        PrintToConsole("Loading persistent state: OK [block %d]\n", nWaterlineBlock);
    } else {
        std::string strReason = "unknown";
        if (wrongDBVersion) strReason = "client version changed";
        if (noPreviousState) strReason = "no usable previous state found";
        if (startClean) strReason = "-startclean parameter used";
        PrintToConsole("Loading persistent state: NONE (%s)\n", strReason);
    }

    if (nWaterlineBlock < 0) {
        // persistence says we reparse!, nuke some stuff in case the partial loads left stale bits
        clear_all_state();
    }

    // legacy code, setting to pre-genesis-block
    int snapshotHeight = ConsensusParams().GENESIS_BLOCK - 1;

    if (nWaterlineBlock < snapshotHeight) {
        nWaterlineBlock = snapshotHeight;
        exodus_prev = 0;
    }

    // advance the waterline so that we start on the next unaccounted for block
    nWaterlineBlock += 1;

    // collect the real Exodus balances available at the snapshot time
    // redundant? do we need to show it both pre-parse and post-parse?  if so let's label the printfs accordingly
    if (msc_debug_exo) {
        int64_t exodus_balance = getMPbalance(exodus_address, OMNI_PROPERTY_MSC, BALANCE);
        PrintToLog("Exodus balance at start: %s\n", FormatDivisibleMP(exodus_balance));
    }

    // load feature activation messages from txlistdb and process them accordingly
    p_txlistdb->LoadActivations(nWaterlineBlock);

    // load all alerts from levelDB (and immediately expire old ones)
    p_txlistdb->LoadAlerts(nWaterlineBlock);

    // initial scan
    msc_initial_scan(nWaterlineBlock);

    // display Exodus balance
    int64_t exodus_balance = getMPbalance(exodus_address, OMNI_PROPERTY_MSC, BALANCE);
    PrintToLog("Exodus balance after initialization: %s\n", FormatDivisibleMP(exodus_balance));

    PrintToConsole("Exodus balance: %s OMNI\n", FormatDivisibleMP(exodus_balance));
    PrintToConsole("Omni Core initialization completed\n");

    return 0;
}

/**
 * Global handler to shut down Omni Core.
 *
 * In particular, the LevelDB databases of the global state objects are closed
 * properly.
 *
 * @return An exit code, indicating success or failure
 */
int mastercore_shutdown()
{
    LOCK(cs_tally);

    if (p_txlistdb) {
        delete p_txlistdb;
        p_txlistdb = NULL;
    }
    if (t_tradelistdb) {
        delete t_tradelistdb;
        t_tradelistdb = NULL;
    }
    if (s_stolistdb) {
        delete s_stolistdb;
        s_stolistdb = NULL;
    }
    if (_my_sps) {
        delete _my_sps;
        _my_sps = NULL;
    }

    PrintToLog("\nOmni Core shutdown completed\n");
    PrintToLog("Shutdown time: %s\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()));

    PrintToConsole("Omni Core shutdown completed\n");

    return 0;
}

/**
 * This handler is called for every new transaction that comes in (actually in block parsing loop).
 *
 * @return True, if the transaction was an Exodus purchase, DEx payment or a valid Omni transaction
 */
bool mastercore_handler_tx(const CTransaction& tx, int nBlock, unsigned int idx, const CBlockIndex* pBlockIndex)
{
    LOCK(cs_tally);

    if (!mastercoreInitialized) {
        mastercore_init();
    }

    // clear pending, if any
    // NOTE1: Every incoming TX is checked, not just MP-ones because:
    // if for some reason the incoming TX doesn't pass our parser validation steps successfuly, I'd still want to clear pending amounts for that TX.
    // NOTE2: Plus I wanna clear the amount before that TX is parsed by our protocol, in case we ever consider pending amounts in internal calculations.
    PendingDelete(tx.GetHash());

    // we do not care about parsing blocks prior to our waterline (empty blockchain defense)
    if (nBlock < nWaterlineBlock) return false;
    int64_t nBlockTime = pBlockIndex->GetBlockTime();

    CMPTransaction mp_obj;
    mp_obj.unlockLogic();

    bool fFoundTx = false;
    int pop_ret = parseTransaction(false, tx, nBlock, idx, mp_obj, nBlockTime);

    if (pop_ret >= 0) {
        assert(mp_obj.getEncodingClass() != NO_MARKER);
        assert(mp_obj.getSender().empty() == false);

        fFoundTx |= HandleExodusPurchase(tx, nBlock, mp_obj.getSender(), nBlockTime);
    }

    if (pop_ret > 0) {
        assert(mp_obj.getEncodingClass() == OMNI_CLASS_A);
        assert(mp_obj.getPayload().empty() == true);

        fFoundTx |= HandleDExPayments(tx, nBlock, mp_obj.getSender());
    }

    if (0 == pop_ret) {
        int interp_ret = mp_obj.interpretPacket();
        if (interp_ret) PrintToLog("!!! interpretPacket() returned %d !!!\n", interp_ret);

        // Only structurally valid transactions get recorded in levelDB
        // PKT_ERROR - 2 = interpret_Transaction failed, structurally invalid payload
        if (interp_ret != PKT_ERROR - 2) {
            bool bValid = (0 <= interp_ret);
            p_txlistdb->recordTX(tx.GetHash(), bValid, nBlock, mp_obj.getType(), mp_obj.getNewAmount());
        }
        fFoundTx |= (interp_ret == 0);
    }

    if (fFoundTx && msc_debug_consensus_hash_every_transaction) {
        uint256 consensusHash = GetConsensusHash();
        PrintToLog("Consensus hash for transaction %s: %s\n", tx.GetHash().GetHex(), consensusHash.GetHex());
    }

    return fFoundTx;
}

/**
 * Determines, whether it is valid to use a Class C transaction for a given payload size.
 *
 * @param nDataSize The length of the payload
 * @return True, if Class C is enabled and the payload is small enough
 */
bool mastercore::UseEncodingClassC(size_t nDataSize)
{
    size_t nTotalSize = nDataSize + GetOmMarker().size(); // Marker "omni"
    bool fDataEnabled = GetBoolArg("-datacarrier", true);
    int nBlockNow = GetHeight();
    if (!IsAllowedOutputType(TX_NULL_DATA, nBlockNow)) {
        fDataEnabled = false;
    }
    return nTotalSize <= nMaxDatacarrierBytes && fDataEnabled;
}

// This function requests the wallet create an Omni transaction using the supplied parameters and payload
int mastercore::ClassAgnosticWalletTXBuilder(const std::string& senderAddress, const std::string& receiverAddress, const std::string& redemptionAddress,
                          int64_t referenceAmount, const std::vector<unsigned char>& data, uint256& txid, std::string& rawHex, bool commit)
{
#ifdef ENABLE_WALLET
    if (pwalletMain == NULL) return MP_ERR_WALLET_ACCESS;

    // Determine the class to send the transaction via - default is Class C
    int omniTxClass = OMNI_CLASS_C;
    if (!UseEncodingClassC(data.size())) omniTxClass = OMNI_CLASS_B;

    // Prepare the transaction - first setup some vars
    CCoinControl coinControl;
    txid = 0;
    CWalletTx wtxNew;
    int64_t nFeeRet = 0;
    std::string strFailReason;
    std::vector<std::pair<CScript, int64_t> > vecSend;
    CReserveKey reserveKey(pwalletMain);

    // Next, we set the change address to the sender
    CBitcoinAddress addr = CBitcoinAddress(senderAddress);
    coinControl.destChange = addr.Get();

    // Select the inputs
    if (0 > SelectCoins(senderAddress, coinControl, referenceAmount)) { return MP_INPUTS_INVALID; }

    // Encode the data outputs
    switch(omniTxClass) {
        case OMNI_CLASS_B: { // declaring vars in a switch here so use an expicit code block
            CPubKey redeemingPubKey;
            const std::string& sAddress = redemptionAddress.empty() ? senderAddress : redemptionAddress;
            if (!AddressToPubKey(sAddress, redeemingPubKey)) {
                return MP_REDEMP_BAD_VALIDATION;
            }
            if (!OmniCore_Encode_ClassB(senderAddress,redeemingPubKey,data,vecSend)) { return MP_ENCODING_ERROR; }
        break; }
        case OMNI_CLASS_C:
            if(!OmniCore_Encode_ClassC(data,vecSend)) { return MP_ENCODING_ERROR; }
        break;
    }

    // Then add a paytopubkeyhash output for the recipient (if needed) - note we do this last as we want this to be the highest vout
    if (!receiverAddress.empty()) {
        CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(receiverAddress).Get());
        vecSend.push_back(std::make_pair(scriptPubKey, 0 < referenceAmount ? referenceAmount : GetDustThreshold(scriptPubKey)));
    }

    // Now we have what we need to pass to the wallet to create the transaction, perform some checks first

    if (!coinControl.HasSelected()) return MP_ERR_INPUTSELECT_FAIL;

    // Ask the wallet to create the transaction (note mining fee determined by Bitcoin Core params)
    if (!pwalletMain->CreateTransaction(vecSend, wtxNew, reserveKey, nFeeRet, strFailReason, &coinControl)) { return MP_ERR_CREATE_TX; }

    // If this request is only to create, but not commit the transaction then display it and exit
    if (!commit) {
        rawHex = EncodeHexTx(wtxNew);
        return 0;
    } else {
        // Commit the transaction to the wallet and broadcast)
        PrintToLog("%s():%s; nFeeRet = %lu, line %d, file: %s\n", __FUNCTION__, wtxNew.ToString(), nFeeRet, __LINE__, __FILE__);
        if (!pwalletMain->CommitTransaction(wtxNew, reserveKey)) return MP_ERR_COMMIT_TX;
        txid = wtxNew.GetHash();
        return 0;
    }
#else
    return MP_ERR_WALLET_ACCESS;
#endif
}

std::set<int> CMPTxList::GetSeedBlocks(int startHeight, int endHeight)
{
    std::set<int> setSeedBlocks;

    if (!pdb) return setSeedBlocks;

    Iterator* it = NewIterator();

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

void CMPTxList::LoadActivations(int blockHeight)
{
    if (!pdb) return;

    Slice skey, svalue;
    Iterator* it = NewIterator();

    PrintToLog("Loading feature activations from levelDB\n");

    std::vector<std::pair<int64_t, uint256> > loadOrder;

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string itData = it->value().ToString();
        std::vector<std::string> vstr;
        boost::split(vstr, itData, boost::is_any_of(":"), token_compress_on);
        if (4 != vstr.size()) continue; // unexpected number of tokens
        if (atoi(vstr[2]) != OMNICORE_MESSAGE_TYPE_ACTIVATION || atoi(vstr[0]) != 1) continue; // we only care about valid activations
        uint256 txid(it->key().ToString());;
        loadOrder.push_back(std::make_pair(atoi(vstr[1]), txid));
    }

    std::sort (loadOrder.begin(), loadOrder.end());

    for (std::vector<std::pair<int64_t, uint256> >::iterator it = loadOrder.begin(); it != loadOrder.end(); ++it) {
        uint256 hash = (*it).second;
        uint256 blockHash = 0;
        CTransaction wtx;
        CMPTransaction mp_obj;

        if (!GetTransaction(hash, wtx, blockHash, true)) {
            PrintToLog("ERROR: While loading activation transaction %s: tx in levelDB but does not exist.\n", hash.GetHex());
            continue;
        }
        if ((0 == blockHash) || (NULL == GetBlockIndex(blockHash))) {
            PrintToLog("ERROR: While loading activation transaction %s: failed to retrieve block hash.\n", hash.GetHex());
            continue;
        }
        CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
        if (NULL == pBlockIndex) {
            PrintToLog("ERROR: While loading activation transaction %s: failed to retrieve block index.\n", hash.GetHex());
            continue;
        }
        int blockHeight = pBlockIndex->nHeight;
        if (0 != ParseTransaction(wtx, blockHeight, 0, mp_obj)) {
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
    if (mapArgs.count("-omniactivationallowsender") || mapArgs.count("-omniactivationignoresender")) {
        AddAlert("omnicore", ALERT_CLIENT_VERSION_EXPIRY, std::numeric_limits<uint32_t>::max(),
                 "Authorization for feature activation has been modified.  Data provided by this client should not be trusted.");
    }
}

void CMPTxList::LoadAlerts(int blockHeight)
{
    if (!pdb) return;
    Slice skey, svalue;
    Iterator* it = NewIterator();

    std::vector<std::pair<int64_t, uint256> > loadOrder;

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string itData = it->value().ToString();
        std::vector<std::string> vstr;
        boost::split(vstr, itData, boost::is_any_of(":"), token_compress_on);
        if (4 != vstr.size()) continue; // unexpected number of tokens
        if (atoi(vstr[2]) != OMNICORE_MESSAGE_TYPE_ALERT || atoi(vstr[0]) != 1) continue; // not a valid alert
        uint256 txid(it->key().ToString());;
        loadOrder.push_back(std::make_pair(atoi(vstr[1]), txid));
    }

    std::sort (loadOrder.begin(), loadOrder.end());

    for (std::vector<std::pair<int64_t, uint256> >::iterator it = loadOrder.begin(); it != loadOrder.end(); ++it) {
        uint256 txid = (*it).second;
        uint256 blockHash = 0;
        CTransaction wtx;
        CMPTransaction mp_obj;
        if (!GetTransaction(txid, wtx, blockHash, true)) {
            PrintToLog("ERROR: While loading alert %s: tx in levelDB but does not exist.\n", txid.GetHex());
            continue;
        }
        if (0 != ParseTransaction(wtx, blockHeight, 0, mp_obj)) {
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
        CBlockIndex* pBlockIndex = chainActive[blockHeight-1];
        if (pBlockIndex != NULL) {
            blockTime = pBlockIndex->GetBlockTime();
        }
    }
    if (blockTime > 0) {
        CheckExpiredAlerts(blockHeight, blockTime);
    }
}

uint256 CMPTxList::findMetaDExCancel(const uint256 txid)
{
  std::vector<std::string> vstr;
  string txidStr = txid.ToString();
  Slice skey, svalue;
  uint256 cancelTxid;
  Iterator* it = NewIterator();
  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
      skey = it->key();
      svalue = it->value();
      string svalueStr = svalue.ToString();
      boost::split(vstr, svalueStr, boost::is_any_of(":"), token_compress_on);
      // obtain the existing affected tx count
      if (3 <= vstr.size())
      {
          if (vstr[0] == txidStr) { delete it; cancelTxid.SetHex(skey.ToString()); return cancelTxid; }
      }
  }

  delete it;
  return 0;
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

    Status status = pdb->Get(readoptions, "dbversion", &strValue);
    if (status.ok()) {
        verDB = boost::lexical_cast<uint64_t>(strValue);
    }

    if (msc_debug_txdb) PrintToLog("%s(): dbversion %s status %s, line %d, file: %s\n", __FUNCTION__, strValue, status.ToString(), __LINE__, __FILE__);

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
    Status status = pdb->Put(writeoptions, "dbversion", verStr);

    if (msc_debug_txdb) PrintToLog("%s(): dbversion %s status %s, line %d, file: %s\n", __FUNCTION__, verStr, status.ToString(), __LINE__, __FILE__);

    return getDBVersion();
}

int CMPTxList::getNumberOfMetaDExCancels(const uint256 txid)
{
    if (!pdb) return 0;
    int numberOfCancels = 0;
    std::vector<std::string> vstr;
    string strValue;
    Status status = pdb->Get(readoptions, txid.ToString() + "-C", &strValue);
    if (status.ok())
    {
        // parse the string returned
        boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);
        // obtain the number of cancels
        if (4 <= vstr.size())
        {
            numberOfCancels = atoi(vstr[3]);
        }
    }
    return numberOfCancels;
}

/**
 * Returns the number of sub records.
 */
int CMPTxList::getNumberOfSubRecords(const uint256& txid)
{
    int numberOfSubRecords = 0;

    std::string strValue;
    Status status = pdb->Get(readoptions, txid.ToString(), &strValue);
    if (status.ok()) {
        std::vector<std::string> vstr;
        boost::split(vstr, strValue, boost::is_any_of(":"), boost::token_compress_on);
        if (4 <= vstr.size()) {
            numberOfSubRecords = boost::lexical_cast<int>(vstr[3]);
        }
    }

    return numberOfSubRecords;
}

int CMPTxList::getMPTransactionCountTotal()
{
    int count = 0;
    Slice skey, svalue;
    Iterator* it = NewIterator();
    for(it->SeekToFirst(); it->Valid(); it->Next())
    {
        skey = it->key();
        if (skey.ToString().length() == 64) { ++count; } //extra entries for cancels and purchases are more than 64 chars long
    }
    delete it;
    return count;
}

int CMPTxList::getMPTransactionCountBlock(int block)
{
    int count = 0;
    Slice skey, svalue;
    Iterator* it = NewIterator();
    for(it->SeekToFirst(); it->Valid(); it->Next())
    {
        skey = it->key();
        svalue = it->value();
        if (skey.ToString().length() == 64)
        {
            string strValue = svalue.ToString();
            std::vector<std::string> vstr;
            boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);
            if (4 == vstr.size())
            {
                if (atoi(vstr[1]) == block) { ++count; }
            }
        }
    }
    delete it;
    return count;
}

string CMPTxList::getKeyValue(string key)
{
    if (!pdb) return "";
    string strValue;
    Status status = pdb->Get(readoptions, key, &strValue);
    if (status.ok()) { return strValue; } else { return ""; }
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

bool CMPTxList::getPurchaseDetails(const uint256 txid, int purchaseNumber, string *buyer, string *seller, uint64_t *vout, uint64_t *propertyId, uint64_t *nValue)
{
    if (!pdb) return 0;
    std::vector<std::string> vstr;
    string strValue;
    Status status = pdb->Get(readoptions, txid.ToString()+"-"+to_string(purchaseNumber), &strValue);
    if (status.ok())
    {
        // parse the string returned
        boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);
        // obtain the requisite details
        if (5 == vstr.size())
        {
            *vout = atoi(vstr[0]);
            *buyer = vstr[1];
            *seller = vstr[2];
            *propertyId = atoi(vstr[3]);
            *nValue = boost::lexical_cast<boost::uint64_t>(vstr[4]);;
            return true;
        }
    }
    return false;
}

void CMPTxList::recordMetaDExCancelTX(const uint256 &txidMaster, const uint256 &txidSub, bool fValid, int nBlock, unsigned int propertyId, uint64_t nValue)
{
  if (!pdb) return;

       // Prep - setup vars
       unsigned int type = 99992104;
       unsigned int refNumber = 1;
       uint64_t existingAffectedTXCount = 0;
       string txidMasterStr = txidMaster.ToString() + "-C";

       // Step 1 - Check TXList to see if this cancel TXID exists
       // Step 2a - If doesn't exist leave number of affected txs & ref set to 1
       // Step 2b - If does exist add +1 to existing ref and set this ref as new number of affected
       std::vector<std::string> vstr;
       string strValue;
       Status status = pdb->Get(readoptions, txidMasterStr, &strValue);
       if (status.ok())
       {
           // parse the string returned
           boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);

           // obtain the existing affected tx count
           if (4 <= vstr.size())
           {
               existingAffectedTXCount = atoi(vstr[3]);
               refNumber = existingAffectedTXCount + 1;
           }
       }

       // Step 3 - Create new/update master record for cancel tx in TXList
       const string key = txidMasterStr;
       const string value = strprintf("%u:%d:%u:%lu", fValid ? 1:0, nBlock, type, refNumber);
       PrintToLog("METADEXCANCELDEBUG : Writing master record %s(%s, valid=%s, block= %d, type= %d, number of affected transactions= %d)\n", __FUNCTION__, txidMaster.ToString(), fValid ? "YES":"NO", nBlock, type, refNumber);
       if (pdb)
       {
           status = pdb->Put(writeoptions, key, value);
           PrintToLog("METADEXCANCELDEBUG : %s(): %s, line %d, file: %s\n", __FUNCTION__, status.ToString(), __LINE__, __FILE__);
       }

       // Step 4 - Write sub-record with cancel details
       const string txidStr = txidMaster.ToString() + "-C";
       const string subKey = STR_REF_SUBKEY_TXID_REF_COMBO(txidStr, refNumber);
       const string subValue = strprintf("%s:%d:%lu", txidSub.ToString(), propertyId, nValue);
       Status subStatus;
       PrintToLog("METADEXCANCELDEBUG : Writing sub-record %s with value %s\n", subKey, subValue);
       if (pdb)
       {
           subStatus = pdb->Put(writeoptions, subKey, subValue);
           PrintToLog("METADEXCANCELDEBUG : %s(): %s, line %d, file: %s\n", __FUNCTION__, subStatus.ToString(), __LINE__, __FILE__);
       }
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

void CMPTxList::recordPaymentTX(const uint256 &txid, bool fValid, int nBlock, unsigned int vout, unsigned int propertyId, uint64_t nValue, string buyer, string seller)
{
  if (!pdb) return;

       // Prep - setup vars
       unsigned int type = 99999999;
       uint64_t numberOfPayments = 1;
       unsigned int paymentNumber = 1;
       uint64_t existingNumberOfPayments = 0;

       // Step 1 - Check TXList to see if this payment TXID exists
       bool paymentEntryExists = p_txlistdb->exists(txid);

       // Step 2a - If doesn't exist leave number of payments & paymentNumber set to 1
       // Step 2b - If does exist add +1 to existing number of payments and set this paymentNumber as new numberOfPayments
       if (paymentEntryExists)
       {
           //retrieve old numberOfPayments
           std::vector<std::string> vstr;
           string strValue;
           Status status = pdb->Get(readoptions, txid.ToString(), &strValue);
           if (status.ok())
           {
               // parse the string returned
               boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);

               // obtain the existing number of payments
               if (4 <= vstr.size())
               {
                   existingNumberOfPayments = atoi(vstr[3]);
                   paymentNumber = existingNumberOfPayments + 1;
                   numberOfPayments = existingNumberOfPayments + 1;
               }
           }
       }

       // Step 3 - Create new/update master record for payment tx in TXList
       const string key = txid.ToString();
       const string value = strprintf("%u:%d:%u:%lu", fValid ? 1:0, nBlock, type, numberOfPayments); 
       Status status;
       PrintToLog("DEXPAYDEBUG : Writing master record %s(%s, valid=%s, block= %d, type= %d, number of payments= %lu)\n", __FUNCTION__, txid.ToString(), fValid ? "YES":"NO", nBlock, type, numberOfPayments);
       if (pdb)
       {
           status = pdb->Put(writeoptions, key, value);
           PrintToLog("DEXPAYDEBUG : %s(): %s, line %d, file: %s\n", __FUNCTION__, status.ToString(), __LINE__, __FILE__);
       }

       // Step 4 - Write sub-record with payment details
       const string txidStr = txid.ToString();
       const string subKey = STR_PAYMENT_SUBKEY_TXID_PAYMENT_COMBO(txidStr, paymentNumber);
       const string subValue = strprintf("%d:%s:%s:%d:%lu", vout, buyer, seller, propertyId, nValue);
       Status subStatus;
       PrintToLog("DEXPAYDEBUG : Writing sub-record %s with value %s\n", subKey, subValue);
       if (pdb)
       {
           subStatus = pdb->Put(writeoptions, subKey, subValue);
           PrintToLog("DEXPAYDEBUG : %s(): %s, line %d, file: %s\n", __FUNCTION__, subStatus.ToString(), __LINE__, __FILE__);
       }
}

void CMPTxList::recordTX(const uint256 &txid, bool fValid, int nBlock, unsigned int type, uint64_t nValue)
{
  if (!pdb) return;

  // overwrite detection, we should never be overwriting a tx, as that means we have redone something a second time
  // reorgs delete all txs from levelDB above reorg_chain_height
  if (p_txlistdb->exists(txid)) PrintToLog("LEVELDB TX OVERWRITE DETECTION - %s\n", txid.ToString());

const string key = txid.ToString();
const string value = strprintf("%u:%d:%u:%lu", fValid ? 1:0, nBlock, type, nValue);
Status status;

  PrintToLog("%s(%s, valid=%s, block= %d, type= %d, value= %lu)\n",
   __FUNCTION__, txid.ToString(), fValid ? "YES":"NO", nBlock, type, nValue);

  if (pdb)
  {
    status = pdb->Put(writeoptions, key, value);
    ++nWritten;
    if (msc_debug_txdb) PrintToLog("%s(): %s, line %d, file: %s\n", __FUNCTION__, status.ToString(), __LINE__, __FILE__);
  }
}

bool CMPTxList::exists(const uint256 &txid)
{
  if (!pdb) return false;

string strValue;
Status status = pdb->Get(readoptions, txid.ToString(), &strValue);

  if (!status.ok())
  {
    if (status.IsNotFound()) return false;
  }

  return true;
}

bool CMPTxList::getTX(const uint256 &txid, string &value)
{
Status status = pdb->Get(readoptions, txid.ToString(), &value);

  ++nRead;

  if (status.ok())
  {
    return true;
  }

  return false;
}

void CMPTxList::printStats()
{
  PrintToLog("CMPTxList stats: nWritten= %d , nRead= %d\n", nWritten, nRead);
}

void CMPTxList::printAll()
{
int count = 0;
Slice skey, svalue;
  Iterator* it = NewIterator();

  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
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

  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
    skey = it->key();
    svalue = it->value();

    ++count;

    string strvalue = it->value().ToString();

    // parse the string returned, find the validity flag/bit & other parameters
    boost::split(vstr, strvalue, boost::is_any_of(":"), token_compress_on);

    // only care about the block number/height here
    if (2 <= vstr.size())
    {
      block = atoi(vstr[1]);

      if ((starting_block <= block) && (block <= ending_block))
      {
        ++n_found;
        PrintToLog("%s() DELETING: %s=%s\n", __FUNCTION__, skey.ToString(), svalue.ToString());
        if (bDeleteFound) pdb->Delete(writeoptions, skey);
      }
    }
  }

  PrintToLog("%s(%d, %d); n_found= %d\n", __FUNCTION__, starting_block, ending_block, n_found);

  delete it;

  return (n_found);
}

// MPSTOList here
std::string CMPSTOList::getMySTOReceipts(string filterAddress)
{
  if (!pdb) return "";
  string mySTOReceipts = "";
  Slice skey, svalue;
  Iterator* it = NewIterator();
  for(it->SeekToFirst(); it->Valid(); it->Next()) {
      skey = it->key();
      string recipientAddress = skey.ToString();
      if(!IsMyAddress(recipientAddress)) continue; // not ours, not interested
      if((!filterAddress.empty()) && (filterAddress != recipientAddress)) continue; // not the filtered address
      // ours, get info
      svalue = it->value();
      string strValue = svalue.ToString();
      // break into individual receipts
      std::vector<std::string> vstr;
      boost::split(vstr, strValue, boost::is_any_of(","), token_compress_on);
      for(uint32_t i = 0; i<vstr.size(); i++) {
          // add to array
          std::vector<std::string> svstr;
          boost::split(svstr, vstr[i], boost::is_any_of(":"), token_compress_on);
          if(4 == svstr.size()) {
              size_t txidMatch = mySTOReceipts.find(svstr[0]);
              if(txidMatch==std::string::npos) mySTOReceipts += svstr[0]+":"+svstr[1]+":"+recipientAddress+":"+svstr[2]+",";
          }
      }
  }
  delete it;
  // above code will leave a trailing comma - strip it
  if (mySTOReceipts.size() > 0) mySTOReceipts.resize(mySTOReceipts.size()-1);
  return mySTOReceipts;
}

void CMPSTOList::getRecipients(const uint256 txid, string filterAddress, Array *recipientArray, uint64_t *total, uint64_t *stoFee)
{
  if (!pdb) return;

  bool filter = true; //default
  bool filterByWallet = true; //default
  bool filterByAddress = false; //default

  if (filterAddress == "*") filter = false;
  if ((filterAddress != "") && (filterAddress != "*")) { filterByWallet = false; filterByAddress = true; }

  // iterate through SDB, dropping all records where key is not filterAddress (if filtering)
  int count = 0;

  // ugly way to do this, really we should store the fee used but for now since we know it is
  // always num_addresses * 0.00000001 MSC we can recalculate it on the fly
  *stoFee = 0;

  Slice skey, svalue;
  Iterator* it = NewIterator();
  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
      skey = it->key();
      string recipientAddress = skey.ToString();
      svalue = it->value();
      string strValue = svalue.ToString();
      // see if txid is in the data
      size_t txidMatch = strValue.find(txid.ToString());
      if(txidMatch!=std::string::npos)
      {
          ++*stoFee;
          // the txid exists inside the data, this address was a recipient of this STO, check filter and add the details
          if(filter)
          {
              if( ( (filterByAddress) && (filterAddress == recipientAddress) ) || ( (filterByWallet) && (IsMyAddress(recipientAddress)) ) )
              { } else { continue; } // move on if no filter match (but counter still increased for fee)
          }
          std::vector<std::string> vstr;
          boost::split(vstr, strValue, boost::is_any_of(","), token_compress_on);
          for(uint32_t i = 0; i<vstr.size(); i++)
          {
              std::vector<std::string> svstr;
              boost::split(svstr, vstr[i], boost::is_any_of(":"), token_compress_on);
              if(4 == svstr.size())
              {
                  if(svstr[0] == txid.ToString())
                  {
                      //add data to array
                      uint64_t amount = 0;
                      uint64_t propertyId = 0;
                      try
                      {
                          amount = boost::lexical_cast<uint64_t>(svstr[3]);
                          propertyId = boost::lexical_cast<uint64_t>(svstr[2]);
                      } catch (const boost::bad_lexical_cast &e)
                      {
                          PrintToLog("DEBUG STO - error in converting values from leveldb\n");
                          delete it;
                          return; //(something went wrong)
                      }
                      Object recipient;
                      recipient.push_back(Pair("address", recipientAddress));
                      if(isPropertyDivisible(propertyId))
                      {
                         recipient.push_back(Pair("amount", FormatDivisibleMP(amount)));
                      }
                      else
                      {
                         recipient.push_back(Pair("amount", FormatIndivisibleMP(amount)));
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

bool CMPSTOList::exists(string address)
{
  if (!pdb) return false;

  string strValue;
  Status status = pdb->Get(readoptions, address, &strValue);

  if (!status.ok())
  {
    if (status.IsNotFound()) return false;
  }

  return true;
}

void CMPSTOList::recordSTOReceive(string address, const uint256 &txid, int nBlock, unsigned int propertyId, uint64_t amount)
{
  if (!pdb) return;

  bool addressExists = s_stolistdb->exists(address);
  if (addressExists)
  {
      //retrieve existing record
      std::vector<std::string> vstr;
      string strValue;
      Status status = pdb->Get(readoptions, address, &strValue);
      if (status.ok())
      {
          // add details to record
          // see if we are overwriting (check)
          size_t txidMatch = strValue.find(txid.ToString());
          if(txidMatch!=std::string::npos) PrintToLog("STODEBUG : Duplicating entry for %s : %s\n",address,txid.ToString());

          const string key = address;
          const string newValue = strprintf("%s:%d:%u:%lu,", txid.ToString(), nBlock, propertyId, amount);
          strValue += newValue;
          // write updated record
          Status status;
          if (pdb)
          {
              status = pdb->Put(writeoptions, key, strValue);
              PrintToLog("STODBDEBUG : %s(): %s, line %d, file: %s\n", __FUNCTION__, status.ToString(), __LINE__, __FILE__);
          }
      }
  }
  else
  {
      const string key = address;
      const string value = strprintf("%s:%d:%u:%lu,", txid.ToString(), nBlock, propertyId, amount);
      Status status;
      if (pdb)
      {
          status = pdb->Put(writeoptions, key, value);
          PrintToLog("STODBDEBUG : %s(): %s, line %d, file: %s\n", __FUNCTION__, status.ToString(), __LINE__, __FILE__);
      }
  }
}

void CMPSTOList::printAll()
{
  int count = 0;
  Slice skey, svalue;
  Iterator* it = NewIterator();

  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
    skey = it->key();
    svalue = it->value();
    ++count;
    PrintToConsole("entry #%8d= %s:%s\n", count, skey.ToString(), svalue.ToString());
  }

  delete it;
}

void CMPSTOList::printStats()
{
  PrintToLog("CMPSTOList stats: tWritten= %d , tRead= %d\n", nWritten, nRead);
}

/**
 * This function deletes records of STO receivers above a specific block from the STO database.
 *
 * Returns the number of records changed.
 *
 * NOTE: The blockNum parameter is inclusive, so deleteAboveBlock(1000) will delete records in block 1000 and above.
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
      for (uint32_t i = 0; i<vecSTORecords.size(); i++) {
          std::vector<std::string> vecSTORecordFields;
          boost::split(vecSTORecordFields, vecSTORecords[i], boost::is_any_of(":"), boost::token_compress_on);
          if (4 != vecSTORecordFields.size()) continue;
          if (atoi(vecSTORecordFields[1]) <= blockNum) {
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

// MPTradeList here
bool CMPTradeList::getMatchingTrades(const uint256& txid, uint32_t propertyId, Array& tradeArray, int64_t& totalSold, int64_t& totalReceived)
{
  if (!pdb) return false;

  int count = 0;
  totalReceived = 0;
  totalSold = 0;

  std::vector<std::string> vstr;
  string txidStr = txid.ToString();
  leveldb::Iterator* it = NewIterator();
  for(it->SeekToFirst(); it->Valid(); it->Next()) {
      // search key to see if this is a matching trade
      std::string strKey = it->key().ToString();
      std::string strValue = it->value().ToString();
      std::string matchTxid;
      size_t txidMatch = strKey.find(txidStr);
      if (txidMatch == std::string::npos) continue; // no match

      // sanity check key is the correct length for a matched trade
      if (strKey.length() != 129) continue;

      // obtain the txid of the match
      if (txidMatch==0) { matchTxid = strKey.substr(65,64); } else { matchTxid = strKey.substr(0,64); }

      // ensure correct amount of tokens in value string
      boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);
      if (vstr.size() != 7) {
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
      std::string strAmount1 = FormatMP(prop1, amount1);
      std::string strAmount2 = FormatMP(prop2, amount2);

      // populate trade object and add to the trade array, correcting for orientation of trade
      Object trade;
      trade.push_back(Pair("txid", matchTxid));
      trade.push_back(Pair("block", blockNum));
      if (prop1 == propertyId) {
          trade.push_back(Pair("address", address1));
          trade.push_back(Pair("amountsold", strAmount1));
          trade.push_back(Pair("amountreceived", strAmount2));
          totalReceived += amount2;
          totalSold += amount1;
      } else {
          trade.push_back(Pair("address", address2));
          trade.push_back(Pair("amountsold", strAmount2));
          trade.push_back(Pair("amountreceived", strAmount1));
          totalReceived += amount1;
          totalSold += amount2;
      }
      tradeArray.push_back(trade);
      ++count;
  }

  // clean up
  delete it;
  if (count) { return true; } else { return false; }
}

bool CompareTradePair(const std::pair<int64_t, Object>& firstJSONObj, const std::pair<int64_t, Object>& secondJSONObj)
{
    return firstJSONObj.first > secondJSONObj.first;
}

// obtains an array of matching trades with pricing and volume details for a pair sorted by blocknumber
void CMPTradeList::getTradesForPair(uint32_t propertyIdSideA, uint32_t propertyIdSideB, Array& responseArray, uint64_t count)
{
  if (!pdb) return;
  leveldb::Iterator* it = NewIterator();
  std::vector<std::pair<int64_t,Object> > vecResponse;
  bool propertyIdSideAIsDivisible = isPropertyDivisible(propertyIdSideA);
  bool propertyIdSideBIsDivisible = isPropertyDivisible(propertyIdSideB);
  for(it->SeekToFirst(); it->Valid(); it->Next()) {
      std::string strKey = it->key().ToString();
      std::string strValue = it->value().ToString();
      std::vector<std::string> vecKeys;
      std::vector<std::string> vecValues;
      uint256 sellerTxid = 0, matchingTxid = 0;
      std::string sellerAddress, matchingAddress;
      int64_t amountReceived = 0, amountSold = 0;
      if (strKey.size() != 129) continue; // only interested in matches
      boost::split(vecKeys, strKey, boost::is_any_of("+"), boost::token_compress_on);
      boost::split(vecValues, strValue, boost::is_any_of(":"), boost::token_compress_on);
      if (vecKeys.size() != 2 || vecValues.size() != 7) {
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

      Object trade;
      trade.push_back(Pair("block", blockNum));
      trade.push_back(Pair("unitprice", unitPriceStr));
      trade.push_back(Pair("inverseprice", inversePriceStr));
      trade.push_back(Pair("sellertxid", sellerTxid.GetHex()));
      trade.push_back(Pair("selleraddress", sellerAddress));
      if (propertyIdSideAIsDivisible) {
          trade.push_back(Pair("amountsold", FormatDivisibleMP(amountSold)));
      } else {
          trade.push_back(Pair("amountsold", FormatIndivisibleMP(amountSold)));
      }
      if (propertyIdSideBIsDivisible) {
          trade.push_back(Pair("amountreceived", FormatDivisibleMP(amountReceived)));
      } else {
          trade.push_back(Pair("amountreceived", FormatIndivisibleMP(amountReceived)));
      }
      trade.push_back(Pair("matchingtxid", matchingTxid.GetHex()));
      trade.push_back(Pair("matchingaddress", matchingAddress));
      vecResponse.push_back(make_pair(blockNum, trade));
  }

  // sort the response most recent first before adding to the array
  std::sort(vecResponse.begin(), vecResponse.end(), CompareTradePair);
  uint64_t processed = 0;
  for (std::vector<std::pair<int64_t,Object> >::iterator it = vecResponse.begin(); it != vecResponse.end(); ++it) {
      responseArray.push_back(it->second);
      processed++;
      if (processed >= count) break;
  }
  std::reverse(responseArray.begin(), responseArray.end());
  delete it;
}

// obtains a vector of txids where the supplied address participated in a trade (needed for gettradehistory_MP)
// optional property ID parameter will filter on propertyId transacted if supplied
// sorted by block then index
void CMPTradeList::getTradesForAddress(std::string address, std::vector<uint256>& vecTransactions, uint32_t propertyIdFilter)
{
  if (!pdb) return;
  std::map<std::string,uint256> mapTrades;
  leveldb::Iterator* it = NewIterator();
  for(it->SeekToFirst(); it->Valid(); it->Next()) {
      std::string strKey = it->key().ToString();
      std::string strValue = it->value().ToString();
      std::vector<std::string> vecValues;
      if (strKey.size() != 64) continue; // only interested in trades
      uint256 txid(strKey);
      size_t addressMatch = strValue.find(address);
      if (addressMatch == std::string::npos) continue;
      boost::split(vecValues, strValue, boost::is_any_of(":"), token_compress_on);
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
  for (std::map<std::string,uint256>::iterator it = mapTrades.begin(); it != mapTrades.end(); it++) {
      vecTransactions.push_back(it->second);
  }
}

void CMPTradeList::recordNewTrade(const uint256& txid, const std::string& address, uint32_t propertyIdForSale, uint32_t propertyIdDesired, int blockNum, int blockIndex)
{
  if (!pdb) return;
  std::string strValue = strprintf("%s:%d:%d:%d:%d", address, propertyIdForSale, propertyIdDesired, blockNum, blockIndex);
  Status status = pdb->Put(writeoptions, txid.ToString(), strValue);
  ++nWritten;
  if (msc_debug_tradedb) PrintToLog("%s(): %s\n", __FUNCTION__, status.ToString());
}

void CMPTradeList::recordMatchedTrade(const uint256 txid1, const uint256 txid2, string address1, string address2, unsigned int prop1, unsigned int prop2, uint64_t amount1, uint64_t amount2, int blockNum)
{
  if (!pdb) return;
  const string key = txid1.ToString() + "+" + txid2.ToString();
  const string value = strprintf("%s:%s:%u:%u:%lu:%lu:%d", address1, address2, prop1, prop2, amount1, amount2, blockNum);
  Status status;
  if (pdb)
  {
    status = pdb->Put(writeoptions, key, value);
    ++nWritten;
    if (msc_debug_tradedb) PrintToLog("%s(): %s\n", __FUNCTION__, status.ToString());
  }
}

// delete any trades after blockNum
int CMPTradeList::deleteAboveBlock(int blockNum)
{
  leveldb::Slice skey, svalue;
  unsigned int count = 0;
  std::vector<std::string> vstr;
  int block = 0;
  unsigned int n_found = 0;
  leveldb::Iterator* it = NewIterator();
  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
    skey = it->key();
    svalue = it->value();
    ++count;
    string strvalue = it->value().ToString();
    boost::split(vstr, strvalue, boost::is_any_of(":"), token_compress_on);
    if (7 == vstr.size()) block = atoi(vstr[6]); // trade matches have 7 tokens, key is txid+txid, only care about block
    if (5 == vstr.size()) block = atoi(vstr[3]); // trades have 5 tokens, key is txid, only care about block
    if (block >= blockNum) {
        ++n_found;
        PrintToLog("%s() DELETING FROM TRADEDB: %s=%s\n", __FUNCTION__, skey.ToString(), svalue.ToString());
        pdb->Delete(writeoptions, skey);
    }
  }

  PrintToLog("%s(%d); tradedb n_found= %d\n", __FUNCTION__, blockNum, n_found);

  delete it;

  return (n_found);
}

void CMPTradeList::printStats()
{
  PrintToLog("CMPTradeList stats: tWritten= %d , tRead= %d\n", nWritten, nRead);
}

int CMPTradeList::getMPTradeCountTotal()
{
    int count = 0;
    Slice skey, svalue;
    Iterator* it = NewIterator();
    for(it->SeekToFirst(); it->Valid(); it->Next())
    {
        ++count;
    }
    delete it;
    return count;
}

void CMPTradeList::printAll()
{
  int count = 0;
  Slice skey, svalue;
  Iterator* it = NewIterator();

  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
    skey = it->key();
    svalue = it->value();
    ++count;
    PrintToConsole("entry #%8d= %s:%s\n", count, skey.ToString(), svalue.ToString());
  }

  delete it;
}

// global wrapper, block numbers are inclusive, if ending_block is 0 top of the chain will be used
bool mastercore::isMPinBlockRange(int starting_block, int ending_block, bool bDeleteFound)
{
  if (!p_txlistdb) return false;

  if (0 == ending_block) ending_block = GetHeight(); // will scan 'til the end

  return p_txlistdb->isMPinBlockRange(starting_block, ending_block, bDeleteFound);
}

// call it like so (variable # of parameters):
// int block = 0;
// ...
// uint64_t nNew = 0;
//
// if (getValidMPTX(txid, &block, &type, &nNew)) // if true -- the TX is a valid MP TX
//
bool mastercore::getValidMPTX(const uint256 &txid, int *block, unsigned int *type, uint64_t *nAmended)
{
string result;
int validity = 0;

  if (msc_debug_txdb) PrintToLog("%s()\n", __FUNCTION__);

  if (!p_txlistdb) return false;

  if (!p_txlistdb->getTX(txid, result)) return false;

  // parse the string returned, find the validity flag/bit & other parameters
  std::vector<std::string> vstr;
  boost::split(vstr, result, boost::is_any_of(":"), token_compress_on);

  if (msc_debug_txdb) PrintToLog("%s() size=%lu : %s\n", __FUNCTION__, vstr.size(), result);

  if (1 <= vstr.size()) validity = atoi(vstr[0]);

  if (block)
  {
    if (2 <= vstr.size()) *block = atoi(vstr[1]);
    else *block = 0;
  }

  if (type)
  {
    if (3 <= vstr.size()) *type = atoi(vstr[2]);
    else *type = 0;
  }

  if (nAmended)
  {
    if (4 <= vstr.size()) *nAmended = boost::lexical_cast<boost::uint64_t>(vstr[3]);
    else nAmended = 0;
  }

  if (msc_debug_txdb) p_txlistdb->printStats();

  if ((int)0 == validity) return false;

  return true;
}

int mastercore_handler_block_begin(int nBlockPrev, CBlockIndex const * pBlockIndex)
{
    LOCK(cs_tally);

    if (reorgRecoveryMode > 0) {
        reorgRecoveryMode = 0; // clear reorgRecovery here as this is likely re-entrant

        p_txlistdb->isMPinBlockRange(pBlockIndex->nHeight, reorgRecoveryMaxHeight, true); // inclusive
        t_tradelistdb->deleteAboveBlock(pBlockIndex->nHeight - 1); // deleteAboveBlock functions are non-inclusive (>blocknum not >=blocknum)
        s_stolistdb->deleteAboveBlock(pBlockIndex->nHeight - 1);
        reorgRecoveryMaxHeight = 0;

        nWaterlineBlock = ConsensusParams().GENESIS_BLOCK - 1;

        int best_state_block = load_most_relevant_state();
        if (best_state_block < 0) {
            // unable to recover easily, remove stale stale state bits and reparse from the beginning.
            clear_all_state();
        } else {
            nWaterlineBlock = best_state_block;
        }

        // clear the global wallet property list, perform a forced wallet update and tell the UI that state is no longer valid, and UI views need to be reinit
        global_wallet_property_list.clear();
        CheckWalletUpdate(true);
        uiInterface.OmniStateInvalidated();

        if (nWaterlineBlock < nBlockPrev) {
            // scan from the block after the best active block to catch up to the active chain
            msc_initial_scan(nWaterlineBlock + 1);
        }
    }

    // handle any features that go live with this block
    CheckLiveActivations(pBlockIndex->nHeight);

    eraseExpiredCrowdsale(pBlockIndex);

    return 0;
}

// called once per block, after the block has been processed
// TODO: consolidate into *handler_block_begin() << need to adjust Accept expiry check.............
// it performs cleanup and other functions
int mastercore_handler_block_end(int nBlockNow, CBlockIndex const * pBlockIndex,
        unsigned int countMP)
{
    LOCK(cs_tally);

    if (!mastercoreInitialized) {
        mastercore_init();
    }

    // for every new received block must do:
    // 1) remove expired entries from the accept list (per spec accept entries are
    //    valid until their blocklimit expiration; because the customer can keep
    //    paying BTC for the offer in several installments)
    // 2) update the amount in the Exodus address
    int64_t devmsc = 0;
    unsigned int how_many_erased = eraseExpiredAccepts(nBlockNow);

    if (how_many_erased) {
        PrintToLog("%s(%d); erased %u accepts this block, line %d, file: %s\n",
            __FUNCTION__, how_many_erased, nBlockNow, __LINE__, __FILE__);
    }

    // calculate devmsc as of this block and update the Exodus' balance
    devmsc = calculate_and_update_devmsc(pBlockIndex->GetBlockTime());

    if (msc_debug_exo) {
        int64_t balance = getMPbalance(exodus_address, OMNI_PROPERTY_MSC, BALANCE);
        PrintToLog("devmsc for block %d: %d, Exodus balance: %d\n", nBlockNow, devmsc, FormatDivisibleMP(balance));
    }

    // check the alert status, do we need to do anything else here?
    CheckExpiredAlerts(nBlockNow, pBlockIndex->GetBlockTime());

    // transactions were found in the block, signal the UI accordingly
    if (countMP > 0) CheckWalletUpdate(true);

    // calculate and print a consensus hash if required
    if (msc_debug_consensus_hash_every_block) {
        uint256 consensusHash = GetConsensusHash();
        PrintToLog("Consensus hash for block %d: %s\n", nBlockNow, consensusHash.GetHex());
    }

    // request checkpoint verification
    bool checkpointValid = VerifyCheckpoint(nBlockNow, pBlockIndex->GetBlockHash());
    if (!checkpointValid) {
        // failed checkpoint, can't be trusted to provide valid data - shutdown client
        const std::string& msg = strprintf("Shutting down due to failed checkpoint for block %d (hash %s)\n", nBlockNow, pBlockIndex->GetBlockHash().GetHex());
        PrintToLog(msg);
        if (!GetBoolArg("-overrideforcedshutdown", false)) AbortNode(msg, msg);
    } else {
        // save out the state after this block
        if (writePersistence(nBlockNow)) {
            mastercore_save_state(pBlockIndex);
        }
    }

    return 0;
}

int mastercore_handler_disc_begin(int nBlockNow, CBlockIndex const * pBlockIndex)
{
    LOCK(cs_tally);

    reorgRecoveryMode = 1;
    reorgRecoveryMaxHeight = (pBlockIndex->nHeight > reorgRecoveryMaxHeight) ? pBlockIndex->nHeight: reorgRecoveryMaxHeight;
    return 0;
}

int mastercore_handler_disc_end(int nBlockNow, CBlockIndex const * pBlockIndex)
{
    LOCK(cs_tally);

    return 0;
}

/**
 * Returns the Exodus address.
 *
 * Main network:
 *   1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P
 *
 * Test network:
 *   mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv
 *
 * @return The Exodus address
 */
const CBitcoinAddress ExodusAddress()
{
    if (isNonMainNet()) {
        static CBitcoinAddress testAddress(exodus_testnet);
        return testAddress;
    } else {
        static CBitcoinAddress mainAddress(exodus_mainnet);
        return mainAddress;
    }
}

/**
 * Returns the Exodus crowdsale address.
 *
 * Main network:
 *   1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P
 *
 * Test network:
 *   mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv (for blocks <  270775)
 *   moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP (for blocks >= 270775)
 *
 * @return The Exodus fundraiser address
 */
const CBitcoinAddress ExodusCrowdsaleAddress(int nBlock)
{
    if (MONEYMAN_TESTNET_BLOCK <= nBlock && isNonMainNet()) {
        static CBitcoinAddress moneyAddress(getmoney_testnet);
        return moneyAddress;
    }
    else if (MONEYMAN_REGTEST_BLOCK <= nBlock && RegTest()) {
        static CBitcoinAddress moneyAddress(getmoney_testnet);
        return moneyAddress;
    }

    return ExodusAddress();
}

/**
 * @return The marker for class C transactions.
 */
const std::vector<unsigned char> GetOmMarker()
{
    static unsigned char pch[] = {0x6f, 0x6d, 0x6e, 0x69}; // Hex-encoded: "omni"

    return std::vector<unsigned char>(pch, pch + sizeof(pch) / sizeof(pch[0]));
}
