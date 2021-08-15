/**
 * @file omnicore.cpp
 *
 * This file contains the core of Omni Core.
 */

#include <omnicore/omnicore.h>

#include <omnicore/activation.h>
#include <omnicore/consensushash.h>
#include <omnicore/convert.h>
#include <omnicore/dbbase.h>
#include <omnicore/dbfees.h>
#include <omnicore/dbspinfo.h>
#include <omnicore/dbstolist.h>
#include <omnicore/dbtradelist.h>
#include <omnicore/dbtransaction.h>
#include <omnicore/dbtxlist.h>
#include <omnicore/dex.h>
#include <omnicore/log.h>
#include <omnicore/mdex.h>
#include <omnicore/notifications.h>
#include <omnicore/parsing.h>
#include <omnicore/pending.h>
#include <omnicore/persistence.h>
#include <omnicore/rules.h>
#include <omnicore/script.h>
#include <omnicore/seedblocks.h>
#include <omnicore/sp.h>
#include <omnicore/tally.h>
#include <omnicore/tx.h>
#include <omnicore/utilsbitcoin.h>
#include <omnicore/utilsui.h>
#include <omnicore/version.h>
#include <omnicore/walletcache.h>
#include <omnicore/walletutils.h>

#include <base58.h>
#include <chainparams.h>
#include <coins.h>
#include <core_io.h>
#include <fs.h>
#include <key_io.h>
#include <init.h>
#include <validation.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/standard.h>
#include <shutdown.h>
#include <sync.h>
#include <tinyformat.h>
#include <uint256.h>
#include <ui_interface.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <util/time.h>
#ifdef ENABLE_WALLET
#include <wallet/ismine.h>
#include <wallet/wallet.h>
#endif

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

using namespace mastercore;

//! Global lock for state objects
CCriticalSection cs_tally;

//! Mainnet Exodus address
static const std::string exodus_mainnet = "3CXhBWrUw9Ls34nDGfE3TDAvEyeDRqbEV5";
//! Testnet Exodus address
static const std::string exodus_testnet = "2NEadRvxtx81rq7e1eKNaXq92RkPBkk4b5U";

//! Exodus address (changes based on network)
static std::string exodus_address = exodus_mainnet;

static int nWaterlineBlock = 0;

/**
 * Used to indicate, whether to automatically commit created transactions.
 *
 * Can be set with configuration "-autocommit" or RPC "setautocommit_OMNI".
 */
bool autoCommit = true;

//! Number of "Dev Omni" of the last processed block
int64_t exodus_prev = 0;

//! Path for file based persistence
fs::path pathStateFiles;

//! Flag to indicate whether Omni Core was initialized
static int mastercoreInitialized = 0;

//! Flag to indicate whether there was a block reorganisatzion
static int reorgRecoveryMode = 0;
//! Block height to recover from after a block reorganization
static int reorgRecoveryMaxHeight = 0;

//! LevelDB based storage for currencies, smart properties and tokens
CMPSPInfo* mastercore::pDbSpInfo;
//! LevelDB based storage for transactions, with txid as key and validity bit, and other data as value
CMPTxList* mastercore::pDbTransactionList;
//! LevelDB based storage for the MetaDEx trade history
CMPTradeList* mastercore::pDbTradeList;
//! LevelDB based storage for STO recipients
CMPSTOList* mastercore::pDbStoList;
//! LevelDB based storage for storing Omni transaction validation and position in block data
COmniTransactionDB* mastercore::pDbTransaction;
//! LevelDB based storage for the MetaDEx fee cache
COmniFeeCache* mastercore::pDbFeeCache;
//! LevelDB based storage for the MetaDEx fee distributions
COmniFeeHistory* mastercore::pDbFeeHistory;

//! In-memory collection of DEx offers
OfferMap mastercore::my_offers;
//! In-memory collection of DEx accepts
AcceptMap mastercore::my_accepts;
//! In-memory collection of active crowdsales
CrowdMap mastercore::my_crowds;

//! Set containing properties that have freezing enabled
std::set<std::pair<uint32_t,int> > setFreezingEnabledProperties;
//! Set containing addresses that have been frozen
std::set<std::pair<std::string,uint32_t> > setFrozenAddresses;

//! In-memory collection of all amounts for all addresses for all properties
std::unordered_map<std::string, CMPTally> mastercore::mp_tally_map;

// Only needed for GUI:

//! Available balances of wallet properties
std::map<uint32_t, int64_t> global_balance_money;
//! Reserved balances of wallet propertiess
std::map<uint32_t, int64_t> global_balance_reserved;
//! Vector containing a list of properties relative to the wallet
std::set<uint32_t> global_wallet_property_list;

std::string mastercore::strMPProperty(uint32_t propertyId)
{
    std::string str = "*unknown*";

    // test user-token
    if (0x80000000 & propertyId) {
        str = strprintf("Test token: %d : 0x%08X", 0x7FFFFFFF & propertyId, propertyId);
    } else {
        switch (propertyId) {
            case OMNI_PROPERTY_BTC: str = "QTC";
                break;
            case OMNI_PROPERTY_MSC: str = "OMN";
                break;
            case OMNI_PROPERTY_TMSC: str = "TOMN";
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

std::string FormatIndivisibleMP(int64_t n)
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

CMPTally* mastercore::getTally(const std::string& address)
{
    std::unordered_map<std::string, CMPTally>::iterator it = mp_tally_map.find(address);

    if (it != mp_tally_map.end()) return &(it->second);

    return static_cast<CMPTally*>(nullptr);
}

// look at balance for an address
int64_t GetTokenBalance(const std::string& address, uint32_t propertyId, TallyType ttype)
{
    int64_t balance = 0;
    if (TALLY_TYPE_COUNT <= ttype) {
        return 0;
    }

    LOCK(cs_tally);
    const std::unordered_map<std::string, CMPTally>::iterator my_it = mp_tally_map.find(address);
    if (my_it != mp_tally_map.end()) {
        balance = (my_it->second).getMoney(propertyId, ttype);
    }

    return balance;
}

int64_t GetAvailableTokenBalance(const std::string& address, uint32_t propertyId)
{
    int64_t money = GetTokenBalance(address, propertyId, BALANCE);
    int64_t pending = GetTokenBalance(address, propertyId, PENDING);

    if (0 > pending) {
        return (money + pending); // show the decrease in available money
    }

    return money;
}

int64_t GetReservedTokenBalance(const std::string& address, uint32_t propertyId)
{
    int64_t nReserved = 0;
    nReserved += GetTokenBalance(address, propertyId, ACCEPT_RESERVE);
    nReserved += GetTokenBalance(address, propertyId, METADEX_RESERVE);
    nReserved += GetTokenBalance(address, propertyId, SELLOFFER_RESERVE);

    return nReserved;
}

int64_t GetFrozenTokenBalance(const std::string& address, uint32_t propertyId)
{
    int64_t frozen = 0;

    if (isAddressFrozen(address, propertyId)) {
        frozen = GetTokenBalance(address, propertyId, BALANCE);
    }

    return frozen;
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

void mastercore::ClearFreezeState()
{
    // Should only ever be called in the event of a reorg
    setFreezingEnabledProperties.clear();
    setFrozenAddresses.clear();
}

void mastercore::PrintFreezeState()
{
    PrintToLog("setFrozenAddresses state:\n");
    for (std::set<std::pair<std::string,uint32_t> >::iterator it = setFrozenAddresses.begin(); it != setFrozenAddresses.end(); it++) {
        PrintToLog("  %s:%d\n", (*it).first, (*it).second);
    }
    PrintToLog("setFreezingEnabledProperties state:\n");
    for (std::set<std::pair<uint32_t,int> >::iterator it = setFreezingEnabledProperties.begin(); it != setFreezingEnabledProperties.end(); it++) {
        PrintToLog("  %d:%d\n", (*it).first, (*it).second);
    }
}

void mastercore::enableFreezing(uint32_t propertyId, int liveBlock)
{
    setFreezingEnabledProperties.insert(std::make_pair(propertyId, liveBlock));
    assert(isFreezingEnabled(propertyId, liveBlock));
    PrintToLog("Freezing for property %d will be enabled at block %d.\n", propertyId, liveBlock);
}

void mastercore::disableFreezing(uint32_t propertyId)
{
    int liveBlock = 0;
    for (std::set<std::pair<uint32_t,int> >::iterator it = setFreezingEnabledProperties.begin(); it != setFreezingEnabledProperties.end(); it++) {
        if (propertyId == (*it).first) {
            liveBlock = (*it).second;
        }
    }
    assert(liveBlock > 0);

    setFreezingEnabledProperties.erase(std::make_pair(propertyId, liveBlock));
    PrintToLog("Freezing for property %d has been disabled.\n", propertyId);

    // When disabling freezing for a property, all frozen addresses for that property will be unfrozen!
    for (std::set<std::pair<std::string,uint32_t> >::iterator it = setFrozenAddresses.begin(); it != setFrozenAddresses.end(); ) {
        if ((*it).second == propertyId) {
            PrintToLog("Address %s has been unfrozen for property %d.\n", (*it).first, propertyId);
            it = setFrozenAddresses.erase(it);
            assert(!isAddressFrozen((*it).first, (*it).second));
        } else {
            it++;
        }
    }

    assert(!isFreezingEnabled(propertyId, liveBlock));
}

bool mastercore::isFreezingEnabled(uint32_t propertyId, int block)
{
    for (std::set<std::pair<uint32_t,int> >::iterator it = setFreezingEnabledProperties.begin(); it != setFreezingEnabledProperties.end(); it++) {
        uint32_t itemPropertyId = (*it).first;
        int itemBlock = (*it).second;
        if (propertyId == itemPropertyId && block >= itemBlock) {
            return true;
        }
    }
    return false;
}

void mastercore::freezeAddress(const std::string& address, uint32_t propertyId)
{
    setFrozenAddresses.insert(std::make_pair(address, propertyId));
    assert(isAddressFrozen(address, propertyId));
    PrintToLog("Address %s has been frozen for property %d.\n", address, propertyId);
}

void mastercore::unfreezeAddress(const std::string& address, uint32_t propertyId)
{
    setFrozenAddresses.erase(std::make_pair(address, propertyId));
    assert(!isAddressFrozen(address, propertyId));
    PrintToLog("Address %s has been unfrozen for property %d.\n", address, propertyId);
}

bool mastercore::isAddressFrozen(const std::string& address, uint32_t propertyId)
{
    if (setFrozenAddresses.find(std::make_pair(address, propertyId)) != setFrozenAddresses.end()) {
        return true;
    }
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
    if (false == pDbSpInfo->getSP(propertyId, property)) {
        return 0; // property ID does not exist
    }

    if (!property.fixed || n_owners_total) {
        for (std::unordered_map<std::string, CMPTally>::const_iterator it = mp_tally_map.begin(); it != mp_tally_map.end(); ++it) {
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
        int64_t cachedFee = pDbFeeCache->GetCachedAmount(propertyId);
        totalTokens += cachedFee;
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

    if (ttype == BALANCE && amount < 0) {
        assert(!isAddressFrozen(who, propertyId)); // for safety, this should never fail if everything else is working properly.
    }

    before = GetTokenBalance(who, propertyId, ttype);

    std::unordered_map<std::string, CMPTally>::iterator my_it = mp_tally_map.find(who);
    if (my_it == mp_tally_map.end()) {
        // insert an empty element
        my_it = (mp_tally_map.insert(std::make_pair(who, CMPTally()))).first;
    }

    CMPTally& tally = my_it->second;
    bRet = tally.updateMoney(propertyId, amount, ttype);

    after = GetTokenBalance(who, propertyId, ttype);
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
static int64_t calculate_and_update_devmsc(unsigned int nTime, int block)
{
    const unsigned int exodus_bootstrap_deadline = 1588291200; // 05/01/2020 00:00:00

    // do nothing if before end of fundraiser
    if (nTime < exodus_bootstrap_deadline) return 0;

    // taken mainly from msc_validate.py: def get_available_reward(height, c)
    int64_t devmsc = 0;
    int64_t exodus_delta = 0;
    // spec constants:
    const int64_t all_reward = 5631623576222;
    const double seconds_in_one_year = 31556926;
    const double seconds_passed = nTime - exodus_bootstrap_deadline;
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

    NotifyTotalTokensChanged(OMNI_PROPERTY_MSC, block);

    return exodus_delta;
}

uint32_t mastercore::GetNextPropertyId(bool maineco)
{
    if (!pDbSpInfo)
        return 0;

    if (maineco) {
        return pDbSpInfo->peekNextSPID(1);
    } else {
        return pDbSpInfo->peekNextSPID(2);
    }
}

// Perform any actions that need to be taken when the total number of tokens for a property ID changes
void NotifyTotalTokensChanged(uint32_t propertyId, int block)
{
    if (IsFeatureActivated(FEATURE_FEES, block)) {
        pDbFeeCache->UpdateDistributionThresholds(propertyId);
        pDbFeeCache->EvalCache(propertyId, block);
    }
}

void CheckWalletUpdate(bool forceUpdate)
{
#ifdef ENABLE_WALLET
    if (!HasWallets()) {
        return;
    }
#endif

    // because the wallet balance cache is *only* used by the UI, it's not needed,
    // when the daemon is running without UI.
    if (!fQtMode) {
        return;
    }

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
    for (std::unordered_map<std::string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
        // check if the address is a wallet address (including watched addresses)
        std::string address = my_it->first;
        int addressIsMine = IsMyAddressAllWallets(address, false, ISMINE_SPENDABLE);
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
            global_balance_money[propertyId] += GetAvailableTokenBalance(address, propertyId);
            global_balance_reserved[propertyId] += GetTokenBalance(address, propertyId, SELLOFFER_RESERVE);
            global_balance_reserved[propertyId] += GetTokenBalance(address, propertyId, METADEX_RESERVE);
            global_balance_reserved[propertyId] += GetTokenBalance(address, propertyId, ACCEPT_RESERVE);
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

//! Guards marker cache
static CCriticalSection cs_marker_cache;

//! Cache for potential Omni Layer transactions
static std::set<uint256> setMarkerCache GUARDED_BY(cs_marker_cache);

void TryToAddToMarkerCache(const CTransactionRef &tx)
{
    if (mastercore::GetEncodingClass(*tx, 0) == OMNI_CLASS_C) {
        LOCK(cs_marker_cache);
        setMarkerCache.insert(tx->GetHash());
    }
}

/** Removes transaction from marker cache. */
void RemoveFromMarkerCache(const uint256& txHash)
{
    LOCK(cs_marker_cache);
    setMarkerCache.erase(txHash);
}

/** Checks, if transaction is in marker cache. */
bool IsInMarkerCache(const uint256& txHash)
{
    LOCK(cs_marker_cache);
    return (setMarkerCache.find(txHash) != setMarkerCache.end());
}

/**
 * Returns the encoding class, used to embed a payload.
 *
 *   0 None
 *   1 Class A (p2pkh)          Deprected
 *   2 Class B (multisig)       Deprected
 *   3 Class C (op-return)      Support
 */
int mastercore::GetEncodingClass(const CTransaction& tx, int nBlock)
{
    if (nBlock != 0 && nBlock < ConsensusParams().GENESIS_BLOCK)
        return NO_MARKER;

    bool hasOpReturn = false;
    for (unsigned int n = 0; n < tx.vout.size(); ++n) {
        const CTxOut& out = tx.vout[n];
        if (out.scriptPubKey.size() < 2 || out.scriptPubKey[0] != OP_RETURN)
            continue;

        txnouttype outType;
        if (!GetOutputType(out.scriptPubKey, outType)) {
            continue;
        }
        if (nBlock > 0 && !IsAllowedOutputType(outType, nBlock)) {
            break;
        }
        if (outType == TX_NULL_DATA) {
            // Ensure there is a payload, and the first pushed element equals,
            // or starts with the "omni" marker
            std::vector<std::string> scriptPushes;
            if (GetScriptPushes(out.scriptPubKey, scriptPushes) && !scriptPushes.empty()) {
                std::vector<unsigned char> vchMarker = GetOmMarker();
                std::vector<unsigned char> vchPushed = ParseHex(scriptPushes[0]);
                if (vchPushed.size() >= vchMarker.size() && std::equal(vchMarker.begin(), vchMarker.end(), vchPushed.begin())) {
                    hasOpReturn = true;
                }
            }

            break;
        }
    }

    if (hasOpReturn) {
        return OMNI_CLASS_C;
    }

    return NO_MARKER;
}

//! Guards coins view cache
static CCriticalSection cs_viewcache;

// TODO: move
static CCoinsView _viewDummy;
static CCoinsViewCache g_viewcache(&_viewDummy);

static unsigned int nCacheHits = 0;
static unsigned int nCacheMiss = 0;

/**
 * Fetches transaction inputs and adds them to the coins view cache.
 *
 *
 * @param tx[in]  The transaction to fetch inputs for
 * @return True, if all inputs were successfully added to the cache
 */
static bool FillTxInputCache(const CTransaction& tx, const std::shared_ptr<std::map<COutPoint, Coin>> removedCoins, CCoinsViewCache &view)
{
    static unsigned int nCacheSize = gArgs.GetArg("-omnitxcache", 500000);

    if (view.GetCacheSize() > nCacheSize) {
        PrintToLog("%s(): clearing cache before insertion [size=%d, hit=%d, miss=%d]\n",
                __func__, view.GetCacheSize(), nCacheHits, nCacheMiss);
        view.Flush();
    }

    for (std::vector<CTxIn>::const_iterator it = tx.vin.begin(); it != tx.vin.end(); ++it) {
        const CTxIn& txIn = *it;
        unsigned int nOut = txIn.prevout.n;
        const Coin& coin = view.AccessCoin(txIn.prevout);

        if (!coin.IsSpent()) {
            ++nCacheHits;
            continue;
        } else {
            ++nCacheMiss;
        }

        CTransactionRef txPrev;
        uint256 hashBlock;
        Coin newcoin;
        if (removedCoins && removedCoins->find(txIn.prevout) != removedCoins->end()) {
            newcoin = removedCoins->find(txIn.prevout)->second;
        } else if (GetTransaction(txIn.prevout.hash, txPrev, Params().GetConsensus(), hashBlock)) {
            newcoin.out.scriptPubKey = txPrev->vout[nOut].scriptPubKey;
            newcoin.out.nValue = txPrev->vout[nOut].nValue;
            newcoin.nHeight = 1;
            if (CBlockIndex *pindex = LookupBlockIndex(hashBlock))
                newcoin.nHeight = pindex->nHeight;
        } else {
            return false;
        }

        view.AddCoin(txIn.prevout, std::move(newcoin), true);
    }

    return true;
}

// idx is position within the block, 0-based
// int msc_tx_push(const CTransaction &wtx, int nBlock, unsigned int idx)
// INPUT: bRPConly -- set to true to avoid moving funds; to be called from various RPC calls like this
// RETURNS: 0 if parsed a MP TX
// RETURNS: < 0 if a non-MP-TX or invalid
// RETURNS: >0 if 1 or more payments have been made
static int parseTransaction(bool bRPConly, const CTransaction& wtx, int nBlock, unsigned int idx, CMPTransaction& mp_tx, unsigned int nTime,
    CCoinsViewCache &view, const std::shared_ptr<std::map<COutPoint, Coin>> removedCoins)
{
    assert(bRPConly == mp_tx.isRpcOnly());
    mp_tx.Set(wtx.GetHash(), nBlock, idx, nTime);

    // ### CLASS IDENTIFICATION AND MARKER CHECK ###
    int omniClass = GetEncodingClass(wtx, nBlock);
    if (omniClass == NO_MARKER) {
        return -1; // No Exodus/Omni marker, thus not a valid Omni transaction
    }
    assert(omniClass == OMNI_CLASS_C);

    if (!bRPConly || msc_debug_parser_readonly) {
        PrintToLog("____________________________________________________________________________________________________________________________________\n");
        PrintToLog("%s(block=%d, %s idx= %d); txid: %s\n", __FUNCTION__, nBlock, FormatISO8601DateTime(nTime), idx, wtx.GetHash().GetHex());
    }

    // ### SENDER IDENTIFICATION ###
    std::string strSender;
    {
        // Add previous transaction inputs to the cache
        if (!FillTxInputCache(wtx, removedCoins, view)) {
            PrintToLog("%s() ERROR: failed to get inputs for %s\n", __func__, wtx.GetHash().GetHex());
            return -101;
        }

        assert(view.HaveInputs(wtx));

        // NEW LOGIC - the sender is chosen based on the first vin

        // determine the sender, but invalidate transaction, if the input is not accepted
        {
            unsigned int vin_n = 0; // the first input
            if (msc_debug_vin) PrintToLog("vin=%d:%s\n", vin_n, ScriptToAsmStr(wtx.vin[vin_n].scriptSig));

            const CTxIn& txIn = wtx.vin[vin_n];
            const Coin& coin = view.AccessCoin(txIn.prevout);
            const CTxOut& txOut = coin.out;

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
                strSender = EncodeDestination(source);
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
            if (!(dest == ExodusAddress())) {
                // saving for Class A processing or reference
                GetScriptPushes(wtx.vout[n].scriptPubKey, script_data);
                address_data.push_back(EncodeDestination(dest));
                value_data.push_back(wtx.vout[n].nValue);
                if (msc_debug_parser_data) PrintToLog("saving address_data #%d: %s:%s\n", n, EncodeDestination(dest), ScriptToAsmStr(wtx.vout[n].scriptPubKey));
            }
        }
    }
    if (msc_debug_parser_data) PrintToLog(" address_data.size=%lu\n script_data.size=%lu\n value_data.size=%lu\n", address_data.size(), script_data.size(), value_data.size());

    // ### CLASS C PARSING ###
    {
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

        // ### CLASS C SPECIFIC PARSING ###
        {
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
                    if (GetScriptPushes(wtx.vout[n].scriptPubKey, vstrPushes) && !vstrPushes.empty()) {
                        std::vector<unsigned char> vchMarker = GetOmMarker();
                        std::vector<unsigned char> vchPushed = ParseHex(vstrPushes[0]);
                        if (vchPushed.size() >= vchMarker.size() && std::equal(vchMarker.begin(), vchMarker.end(), vchPushed.begin())) {
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

                    break;
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

    return 0;
}

/**
 * Provides access to parseTransaction in read-only mode.
 */
int ParseTransaction(const CTransaction& tx, int nBlock, unsigned int idx, CMPTransaction& mptx, unsigned int nTime, CCoinsViewCache *pView)
{
    //! Lock lock lock ...
    LOCK2(cs_main, ::mempool.cs);
    
    if (pView) {
        return parseTransaction(true, tx, nBlock, idx, mptx, nTime, *pView, nullptr);
    } else {
        LOCK2(cs_tally, cs_viewcache);
        return parseTransaction(true, tx, nBlock, idx, mptx, nTime, g_viewcache, nullptr);
    }
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
            if (dest == ExodusAddress()) {
                continue;
            }
            std::string strAddress = EncodeDestination(dest);
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
            if (dest == ExodusCrowdsaleAddress(nBlock)) {
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
        int64_t minutes = (secondsTotal / 60) % 60;
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
    AssertLockHeld(cs_main);

    int nTimeBetweenProgressReports = gArgs.GetArg("-omniprogressfrequency", 30);  // seconds
    int64_t nNow = GetTime();
    unsigned int nTxsTotal = 0;
    unsigned int nTxsFoundTotal = 0;
    int nBlock = 999999;
    const int nLastBlock = GetHeight();

    // this function is useless if there are not enough blocks in the blockchain yet!
    if (nFirstBlock < 0 || nLastBlock < nFirstBlock) return -1;
    PrintToConsole("Scanning for transactions in block %d to block %d..\n", nFirstBlock, nLastBlock);

    const CBlockIndex* pFirstBlock = ::ChainActive()[nFirstBlock];
    const CBlockIndex* pLastBlock = ::ChainActive()[nLastBlock];

    ProgressReporter progressReporter(pFirstBlock, pLastBlock);

    // check if using seed block filter should be disabled
    bool seedBlockFilterEnabled = gArgs.GetBoolArg("-omniseedblockfilter", true);

    for (nBlock = nFirstBlock; nBlock <= nLastBlock; ++nBlock)
    {
        if (ShutdownRequested()) {
            PrintToLog("Shutdown requested, stop scan at block %d of %d\n", nBlock, nLastBlock);
            break;
        }

        CBlockIndex* pblockindex = ::ChainActive()[nBlock];

        if (nullptr == pblockindex) break;
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
            if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())) break;

            for(const auto tx : block.vtx) {
                if (mastercore_handler_tx(*tx, nBlock, nTxNum, pblockindex, nullptr)) ++nTxsFoundInBlock;
                ++nTxNum;
            }
        }

        nTxsFoundTotal += nTxsFoundInBlock;
        nTxsTotal += nTxNum;
        {
            LOCK(mempool.cs);
            mastercore_handler_block_end(nBlock, pblockindex, nTxsFoundInBlock);
        }
    }

    if (nBlock < nLastBlock) {
        PrintToConsole("Scan stopped early at block %d of block %d\n", nBlock, nLastBlock);
    }

    PrintToConsole("%d new transactions processed, %d meta transactions found\n", nTxsTotal, nTxsFoundTotal);

    return 0;
}

/**
 * Clears the state of the system.
 */
void clear_all_state()
{
    PrintToLog("Clearing all state..\n");
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
    ClearFreezeState();

    // LevelDB based storage
    pDbSpInfo->Clear();
    pDbTransactionList->Clear();
    pDbStoList->Clear();
    pDbTradeList->Clear();
    pDbTransaction->Clear();
    pDbFeeCache->Clear();
    pDbFeeHistory->Clear();
    assert(pDbTransactionList->setDBVersion() == DB_VERSION); // new set of databases, set DB version
    exodus_prev = 0;
}

static void RewindDBsAndState(int nHeight, int nBlockPrev = 0, bool fInitialParse = false) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    int nWaterline;
    bool reorgContainsFreeze;
    {
        LOCK(cs_tally);
        // Check if any freeze related transactions would be rolled back - if so wipe the state and startclean
        reorgContainsFreeze = pDbTransactionList->CheckForFreezeTxs(nHeight);

        // NOTE: The blockNum parameter is inclusive, so deleteAboveBlock(1000) will delete records in block 1000 and above.
        pDbTransactionList->isMPinBlockRange(nHeight, reorgRecoveryMaxHeight, true);
        pDbTradeList->deleteAboveBlock(nHeight);
        pDbStoList->deleteAboveBlock(nHeight);
        pDbFeeCache->RollBackCache(nHeight);
        pDbFeeHistory->RollBackHistory(nHeight);
        reorgRecoveryMaxHeight = 0;

        nWaterlineBlock = ConsensusParams().GENESIS_BLOCK - 1;
    }

    if (reorgContainsFreeze && !fInitialParse) {
       PrintToConsole("Reorganization containing freeze related transactions detected, forcing a reparse...\n");
       clear_all_state(); // unable to reorg freezes safely, clear state and reparse
    } else {
        int best_state_block = LoadMostRelevantInMemoryState();
        if (best_state_block < 0) {
            // unable to recover easily, remove stale stale state bits and reparse from the beginning.
            clear_all_state();
        } else {
            LOCK(cs_tally);
            nWaterlineBlock = best_state_block;
        }
    }

    {
        LOCK(cs_tally);
        // clear the global wallet property list, perform a forced wallet update and tell the UI that state is no longer valid, and UI views need to be reinit
        global_wallet_property_list.clear();
        CheckWalletUpdate(true);
        uiInterface.OmniStateInvalidated();
        nWaterline = nWaterlineBlock;
    }

    if (nWaterline < nBlockPrev) {
        // scan from the block after the best active block to catch up to the active chain
        msc_initial_scan(nWaterline + 1);
    }
}

/**
 * Global handler to initialize Omni Core.
 *
 * @return An exit code, indicating success or failure
 */
int mastercore_init()
{
    AssertLockHeld(cs_main);

    bool wrongDBVersion, startClean = false;

    {
        LOCK(cs_tally);

        if (mastercoreInitialized) {
            // nothing to do
            return 0;
        }

        PrintToConsole("Initializing Omni Core v%s [%s]\n", OmniCoreVersion(), Params().NetworkIDString());

        PrintToLog("\nInitializing Omni Core v%s [%s]\n", OmniCoreVersion(), Params().NetworkIDString());
        PrintToLog("Startup time: %s\n", FormatISO8601DateTime(GetTime()));

        InitDebugLogLevels();
        ShrinkDebugLog();

        if (isNonMainNet()) {
            exodus_address = exodus_testnet;
        }

        // check for --omniautocommit option and set transaction commit flag accordingly
        if (!gArgs.GetBoolArg("-omniautocommit", true)) {
            PrintToLog("Process was started with --omniautocommit set to false. "
                    "Created Omni transactions will not be committed to wallet or broadcast.\n");
            autoCommit = false;
        }

        // check for --omnistartclean option and delete MP_ folders if present
        if (gArgs.GetBoolArg("-omnistartclean", false)) {
            PrintToLog("Process was started with --omnistartclean option, attempting to clear persistence files..\n");
            try {
                fs::path persistPath = GetOmniDataDir() / "MP_persist";
                fs::path txlistPath = GetOmniDataDir() / "MP_txlist";
                fs::path tradePath = GetOmniDataDir() / "MP_tradelist";
                fs::path spPath = GetOmniDataDir() / "MP_spinfo";
                fs::path stoPath = GetOmniDataDir() / "MP_stolist";
                fs::path omniTXDBPath = GetOmniDataDir() / "Omni_TXDB";
                fs::path feesPath = GetOmniDataDir() / "OMNI_feecache";
                fs::path feeHistoryPath = GetOmniDataDir() / "OMNI_feehistory";
                if (fs::exists(persistPath)) fs::remove_all(persistPath);
                if (fs::exists(txlistPath)) fs::remove_all(txlistPath);
                if (fs::exists(tradePath)) fs::remove_all(tradePath);
                if (fs::exists(spPath)) fs::remove_all(spPath);
                if (fs::exists(stoPath)) fs::remove_all(stoPath);
                if (fs::exists(omniTXDBPath)) fs::remove_all(omniTXDBPath);
                if (fs::exists(feesPath)) fs::remove_all(feesPath);
                if (fs::exists(feeHistoryPath)) fs::remove_all(feeHistoryPath);
                PrintToLog("Success clearing persistence files in datadir %s\n", GetOmniDataDir().string());
                startClean = true;
            } catch (const fs::filesystem_error& e) {
                PrintToLog("Failed to delete persistence folders: %s\n", e.what());
                PrintToConsole("Failed to delete persistence folders: %s\n", e.what());
            }
        }

        pDbTradeList = new CMPTradeList(GetOmniDataDir() / "MP_tradelist", fReindex);
        pDbStoList = new CMPSTOList(GetOmniDataDir() / "MP_stolist", fReindex);
        pDbTransactionList = new CMPTxList(GetOmniDataDir() / "MP_txlist", fReindex);
        pDbSpInfo = new CMPSPInfo(GetOmniDataDir() / "MP_spinfo", fReindex);
        pDbTransaction = new COmniTransactionDB(GetOmniDataDir() / "Omni_TXDB", fReindex);
        pDbFeeCache = new COmniFeeCache(GetOmniDataDir() / "OMNI_feecache", fReindex);
        pDbFeeHistory = new COmniFeeHistory(GetOmniDataDir() / "OMNI_feehistory", fReindex);

        pathStateFiles = GetOmniDataDir() / "MP_persist";
        TryCreateDirectories(pathStateFiles);

        wrongDBVersion = (pDbTransactionList->getDBVersion() != DB_VERSION);

        ++mastercoreInitialized;
    }

    int nWaterline = LoadMostRelevantInMemoryState();

    if (!startClean && nWaterline > 0 && nWaterline < GetHeight()) {
        RewindDBsAndState(nWaterline + 1, 0, true);
    }

    {
        LOCK(cs_tally);
        nWaterlineBlock = nWaterline;

        bool noPreviousState = (nWaterlineBlock <= 0);

        if (startClean) {
            assert(pDbTransactionList->setDBVersion() == DB_VERSION); // new set of databases, set DB version
        } else if (wrongDBVersion) {
            nWaterlineBlock = -1; // force a clear_all_state and parse from start
        }

        // consistency check
        bool inconsistentDb = !VerifyTransactionExistence(nWaterlineBlock);
        if (inconsistentDb) {
            nWaterlineBlock = -1; // force a clear_all_state and parse from start
        }

        if (nWaterlineBlock > 0) {
            PrintToConsole("Loading persistent state: OK [block %d]\n", nWaterlineBlock);
        } else {
            std::string strReason = "unknown";
            if (wrongDBVersion) strReason = "client version changed";
            if (noPreviousState) strReason = "no usable previous state found";
            if (startClean) strReason = "-omnistartclean parameter used";
            if (inconsistentDb) strReason = "INCONSISTENT DB DETECTED!\n"
                    "\n!!! WARNING !!!\n\n"
                    "IF YOU ARE USING AN OVERLAY DB, YOU MAY NEED TO REPROCESS\n"
                    "ALL OMNI LAYER TRANSACTIONS TO AVOID INCONSISTENCIES!\n"
                    "\n!!! WARNING !!!";
            PrintToConsole("Loading persistent state: NONE (%s)\n", strReason);
        }

        if (nWaterlineBlock < 0) {
            // persistence says we reparse!, nuke some stuff in case the partial loads left stale bits
            clear_all_state();
        }

        if (inconsistentDb) {
            std::string strAlert("INCONSISTENT DB DETECTED! IF YOU ARE USING AN OVERLAY DB, YOU MAY NEED TO REPROCESS"
                    "ALL OMNI LAYER TRANSACTIONS TO AVOID INCONSISTENCIES!");
            AddAlert("omnicore", ALERT_CLIENT_VERSION_EXPIRY, std::numeric_limits<uint32_t>::max(), strAlert);
            DoWarning(strAlert);
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
            int64_t exodus_balance = GetTokenBalance(exodus_address, OMNI_PROPERTY_MSC, BALANCE);
            PrintToLog("Exodus balance at start: %s\n", FormatDivisibleMP(exodus_balance));
        }
    }

    {
        LOCK(cs_tally);
        // load feature activation messages from txlistdb and process them accordingly
        pDbTransactionList->LoadActivations(nWaterlineBlock);

        // load all alerts from levelDB (and immediately expire old ones)
        pDbTransactionList->LoadAlerts(nWaterlineBlock);

        // load the state of any freeable properties and frozen addresses from levelDB
        if (!pDbTransactionList->LoadFreezeState(nWaterlineBlock)) {
            std::string strShutdownReason = "Failed to load freeze state from levelDB.  It is unsafe to continue.\n";
            PrintToLog(strShutdownReason);
            if (!gArgs.GetBoolArg("-omnioverrideforcedshutdown", false)) {
                DoAbortNode(strShutdownReason, strShutdownReason);
            }
        }

        nWaterline = nWaterlineBlock;
    }

    // initial scan
    msc_initial_scan(nWaterline);

    {
        LOCK(cs_tally);
        // display Exodus balance
        int64_t exodus_balance = GetTokenBalance(exodus_address, OMNI_PROPERTY_MSC, BALANCE);

        PrintToLog("Exodus balance after initialization: %s\n", FormatDivisibleMP(exodus_balance));
    }

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
    AssertLockHeld(cs_main);
    LOCK(cs_tally);

    if (pDbTransactionList) {
        delete pDbTransactionList;
        pDbTransactionList = nullptr;
    }
    if (pDbTradeList) {
        delete pDbTradeList;
        pDbTradeList = nullptr;
    }
    if (pDbStoList) {
        delete pDbStoList;
        pDbStoList = nullptr;
    }
    if (pDbSpInfo) {
        delete pDbSpInfo;
        pDbSpInfo = nullptr;
    }
    if (pDbTransaction) {
        delete pDbTransaction;
        pDbTransaction = nullptr;
    }
    if (pDbFeeCache) {
        delete pDbFeeCache;
        pDbFeeCache = nullptr;
    }
    if (pDbFeeHistory) {
        delete pDbFeeHistory;
        pDbFeeHistory = nullptr;
    }

    mastercoreInitialized = 0;

    PrintToLog("\nOmni Core shutdown completed\n");
    PrintToLog("Shutdown time: %s\n", FormatISO8601DateTime(GetTime()));

    PrintToConsole("Omni Core shutdown completed\n");

    return 0;
}

/**
 * This handler is called for every new transaction that comes in (actually in block parsing loop).
 *
 * @return True, if the transaction was an Exodus purchase, DEx payment or a valid Omni transaction
 */
bool mastercore_handler_tx(const CTransaction& tx, int nBlock, unsigned int idx, const CBlockIndex* pBlockIndex, const std::shared_ptr<std::map<COutPoint, Coin> > removedCoins)
{
    AssertLockHeld(cs_main);

    int nMastercoreInit, pop_ret;
    {
        LOCK(cs_tally);
        nMastercoreInit = mastercoreInitialized;
    }

    if (!nMastercoreInit) {
        mastercore_init();
    }

    {
        LOCK(cs_tally);

        // clear pending, if any
        // NOTE1: Every incoming TX is checked, not just MP-ones because:
        // if for some reason the incoming TX doesn't pass our parser validation steps successfully, I'd still want to clear pending amounts for that TX.
        // NOTE2: Plus I wanna clear the amount before that TX is parsed by our protocol, in case we ever consider pending amounts in internal calculations.
        PendingDelete(tx.GetHash());

        // we do not care about parsing blocks prior to our waterline (empty blockchain defense)
        if (nBlock < nWaterlineBlock) return false;
    }

    int64_t nBlockTime = pBlockIndex->GetBlockTime();
    CMPTransaction mp_obj;
    mp_obj.unlockLogic();
    bool fFoundTx = false;

    {
        LOCK2(cs_tally, cs_viewcache);
        pop_ret = parseTransaction(false, tx, nBlock, idx, mp_obj, nBlockTime, g_viewcache, removedCoins);
    }

    {
        LOCK(cs_tally);

        if (pop_ret >= 0) {
            assert(mp_obj.getEncodingClass() != NO_MARKER);
            assert(mp_obj.getSender().empty() == false);

            // extra iteration of the outputs for every transaction, not needed on mainnet after Exodus closed
            const CConsensusParams& params = ConsensusParams();
            if (isNonMainNet() || nBlock <= params.LAST_EXODUS_BLOCK) {
                fFoundTx |= HandleExodusPurchase(tx, nBlock, mp_obj.getSender(), nBlockTime);
            }
        }

        if (pop_ret > 0) {
            assert(mp_obj.getEncodingClass() == OMNI_CLASS_A);
            assert(mp_obj.getPayload().empty() == true);

            fFoundTx |= HandleDExPayments(tx, nBlock, mp_obj.getSender());
        }
    }

    if (0 == pop_ret) {
        int interp_ret = WITH_LOCK(cs_tally, return mp_obj.interpretPacket());
        if (interp_ret) PrintToLog("!!! interpretPacket() returned %d !!!\n", interp_ret);

        // Only structurally valid transactions get recorded in levelDB
        // PKT_ERROR - 2 = interpret_Transaction failed, structurally invalid payload
        if (interp_ret != PKT_ERROR - 2) {
            LOCK(cs_tally);
            bool bValid = (0 <= interp_ret);
            pDbTransactionList->recordTX(tx.GetHash(), bValid, nBlock, mp_obj.getType(), mp_obj.getNewAmount());
            pDbTransaction->RecordTransaction(tx.GetHash(), idx, interp_ret);
        }
        fFoundTx |= (interp_ret == 0);
    }

    LOCK(cs_tally);
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
    return true;
}

int mastercore_handler_block_begin(int nBlockPrev, CBlockIndex const * pBlockIndex)
{
    AssertLockHeld(cs_main);

    bool bRecoveryMode{false};
    {
        LOCK(cs_tally);

        if (reorgRecoveryMode > 0) {
            reorgRecoveryMode = 0; // clear reorgRecovery here as this is likely re-entrant
            bRecoveryMode = true;
        }
    }

    if (bRecoveryMode) {
        RewindDBsAndState(pBlockIndex->nHeight, nBlockPrev);
    }

    {
        LOCK(cs_tally);

        // handle any features that go live with this block
        CheckLiveActivations(pBlockIndex->nHeight);

        eraseExpiredCrowdsale(pBlockIndex);
    }

    return 0;
}

// called once per block, after the block has been processed
// TODO: consolidate into *handler_block_begin() << need to adjust Accept expiry check.............
// it performs cleanup and other functions
int mastercore_handler_block_end(int nBlockNow, CBlockIndex const * pBlockIndex, unsigned int countMP)
{
    AssertLockHeld(cs_main);

    int nMastercoreInit;
    {
        LOCK(cs_tally);
        nMastercoreInit = mastercoreInitialized;
    }

    if (!nMastercoreInit) {
        mastercore_init();
    }

    bool checkpointValid;
    {
        LOCK(cs_tally);

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
        devmsc = calculate_and_update_devmsc(pBlockIndex->GetBlockTime(), nBlockNow);

        if (msc_debug_exo) {
            int64_t balance = GetTokenBalance(exodus_address, OMNI_PROPERTY_MSC, BALANCE);
            PrintToLog("devmsc for block %d: %d, Exodus balance: %d\n", nBlockNow, devmsc, FormatDivisibleMP(balance));
        }

        // check the alert status, do we need to do anything else here?
        CheckExpiredAlerts(nBlockNow, pBlockIndex->GetBlockTime());

        // check that pending transactions are still in the mempool
        PendingCheck();

        // transactions were found in the block, signal the UI accordingly
        if (countMP > 0) CheckWalletUpdate(true);

        // calculate and print a consensus hash if required
        if (ShouldConsensusHashBlock(nBlockNow)) {
            uint256 consensusHash = GetConsensusHash();
            PrintToLog("Consensus hash for block %d: %s\n", nBlockNow, consensusHash.GetHex());
        }

        // request checkpoint verification
        checkpointValid = VerifyCheckpoint(nBlockNow, pBlockIndex->GetBlockHash());
        if (!checkpointValid) {
            // failed checkpoint, can't be trusted to provide valid data - shutdown client
            const std::string& msg = strprintf(
                    "Shutting down due to failed checkpoint for block %d (hash %s). "
                    "Please restart with -omnistartclean flag and if this doesn't work, please reach out to the support.\n",
                    nBlockNow, pBlockIndex->GetBlockHash().GetHex());
            PrintToLog(msg);
            if (!gArgs.GetBoolArg("-omnioverrideforcedshutdown", false)) {
                fs::path persistPath = GetOmniDataDir() / "MP_persist";
                if (fs::exists(persistPath)) fs::remove_all(persistPath); // prevent the node being restarted without a reparse after forced shutdown
                DoAbortNode(msg, msg);
            }
        }
    }

    if (checkpointValid) {
        LOCK(cs_tally);
        // save out the state after this block
        if (IsPersistenceEnabled(nBlockNow) && nBlockNow >= ConsensusParams().GENESIS_BLOCK) {
            PersistInMemoryState(pBlockIndex);
        }
    }

    return 0;
}

void mastercore_handler_disc_begin(const int nHeight)
{
    AssertLockHeld(cs_main);
    LOCK(cs_tally);

    reorgRecoveryMode = 1;
    reorgRecoveryMaxHeight = (nHeight > reorgRecoveryMaxHeight) ? nHeight: reorgRecoveryMaxHeight;
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
const CTxDestination ExodusAddress()
{
    if (isNonMainNet()) {
        static CTxDestination testAddress = DecodeDestination(exodus_testnet);
        return testAddress;
    } else {
        static CTxDestination mainAddress = DecodeDestination(exodus_mainnet);
        return mainAddress;
    }
}

/**
 * Returns the Exodus crowdsale address.
 *
 * Main network:
 *   3CXhBWrUw9Ls34nDGfE3TDAvEyeDRqbEV5
 *
 * Test network:
 *   2NEadRvxtx81rq7e1eKNaXq92RkPBkk4b5U
 *
 * @return The Exodus fundraiser address
 */
const CTxDestination ExodusCrowdsaleAddress(int nBlock)
{
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
