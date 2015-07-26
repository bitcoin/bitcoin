/**
 * @file rules.cpp
 *
 * This file contains consensus rules and restrictions.
 */

#include "omnicore/rules.h"

#include "omnicore/log.h"
#include "omnicore/omnicore.h"
#include "omnicore/utilsbitcoin.h"

#include "script/standard.h"

#include <openssl/sha.h>
#include <stdint.h>

namespace mastercore
{

struct ConsensusCheckpoint
{
    int blockHeight;
    uint256 blockHash;
    uint256 consensusHash;
};

/** Checkpoints - block, block hash and consensus hash.
 */
static const ConsensusCheckpoint vCheckpoints[] = {
    { 250000, uint256("000000000000003887df1f29024b06fc2200b55f8af8f35453d7be294df2d214"),
              uint256("f8ca97346756bbe3b530d052c884b2f54c14edc1498ba2176214857dee13f3ca") },
    { 260000, uint256("000000000000001fb91fbcebaaba0e2d926f04908d798a8b598c3bd962951080"),
              uint256("e8b5445bbfc915e59164a6c22e4d84b23539569ee3c95b49eefaeaca86ea98a0") },
    { 270000, uint256("0000000000000002a775aec59dc6a9e4bb1c025cf1b8c2195dd9dc3998c827c5"),
              uint256("4eb6f75c961308c18dad1e2677170465f30d367489e61cb2510e400dc3d21117") },
    { 280000, uint256("0000000000000001c091ada69f444dc0282ecaabe4808ddbb2532e5555db0c03"),
              uint256("5896745cfde2423d1c896865b49953cac76d87f8d29c619793698d06c14f11d6") },
    { 290000, uint256("0000000000000000fa0b2badd05db0178623ebf8dd081fe7eb874c26e27d0b3b"),
              uint256("28f4c64fa05adee082c7fb10f3b768747fef28bff1e29f22482ed17e9dfe130c") },
    { 300000, uint256("000000000000000082ccf8f1557c5d40b21edabb18d2d691cfbf87118bac7254"),
              uint256("51b429776f26ee4b9ebf2de669c7c01f420004ce29d4df92a2e3d7b0248cbe8e") },
    { 310000, uint256("0000000000000000125a28cc9e9209ddb75718f599a8039f6c9e7d9f1fb021e0"),
              uint256("f8ab77846e859ccc5094f4944e0b6fbf7f1e1435578fd3debffc0e0db4afda58") },
    { 320000, uint256("000000000000000015aab005b28a326ade60f07515c33517ea5cb598f28fb7ea"),
              uint256("ac76516b2be5fd6391ac1511c409e19eab0c5b55754883a72d2473457af9299e") },
    { 330000, uint256("00000000000000000faabab19f17c0178c754dbed023e6c871dcaf74159c5f02"),
              uint256("158c9640fd4376569e439f7c6f4b7ac17e14dffc9f9b99a7e6b38b57c7ff5ece") },
    { 340000, uint256("00000000000000000d9b2508615d569e18f00c034d71474fc44a43af8d4a5003"),
              uint256("85655b4e01fa328f5d1c955d00f27989e644a38931280d1448b83c98f78d3a25") },
    { 350000, uint256("0000000000000000053cf64f0400bb38e0c4b3872c38795ddde27acb40a112bb"),
              uint256("c562ebcc36c5dd094f7dff3d0083362701b8ad489b825a93dab3e51d7a0b59f2") },
    { 360000, uint256("00000000000000000ca6e07cf681390ff888b7f96790286a440da0f2b87c8ea6"),
              uint256("da08157c018429bf717989ec653151652fb95d3107b287e016c8b9483e9f86f7") },
};

/** A mapping of transaction types, versions and the blocks at which they are enabled.
 */
static const int txRestrictionsRules[][3] = {
    {MSC_TYPE_SIMPLE_SEND,                GENESIS_BLOCK,          MP_TX_PKT_V0},
    {MSC_TYPE_TRADE_OFFER,                MSC_DEX_BLOCK,          MP_TX_PKT_V1},
    {MSC_TYPE_ACCEPT_OFFER_BTC,           MSC_DEX_BLOCK,          MP_TX_PKT_V0},
    {MSC_TYPE_CREATE_PROPERTY_FIXED,      MSC_SP_BLOCK,           MP_TX_PKT_V0},
    {MSC_TYPE_CREATE_PROPERTY_VARIABLE,   MSC_SP_BLOCK,           MP_TX_PKT_V1},
    {MSC_TYPE_CLOSE_CROWDSALE,            MSC_SP_BLOCK,           MP_TX_PKT_V0},
    {MSC_TYPE_CREATE_PROPERTY_MANUAL,     MSC_MANUALSP_BLOCK,     MP_TX_PKT_V0},
    {MSC_TYPE_GRANT_PROPERTY_TOKENS,      MSC_MANUALSP_BLOCK,     MP_TX_PKT_V0},
    {MSC_TYPE_REVOKE_PROPERTY_TOKENS,     MSC_MANUALSP_BLOCK,     MP_TX_PKT_V0},
    {MSC_TYPE_CHANGE_ISSUER_ADDRESS,      MSC_MANUALSP_BLOCK,     MP_TX_PKT_V0},
    {MSC_TYPE_SEND_TO_OWNERS,             MSC_STO_BLOCK,          MP_TX_PKT_V0},
    {MSC_TYPE_METADEX_TRADE,              MSC_METADEX_BLOCK,      MP_TX_PKT_V0},
    {MSC_TYPE_METADEX_CANCEL_PRICE,       MSC_METADEX_BLOCK,      MP_TX_PKT_V0},
    {MSC_TYPE_METADEX_CANCEL_PAIR,        MSC_METADEX_BLOCK,      MP_TX_PKT_V0},
    {MSC_TYPE_METADEX_CANCEL_ECOSYSTEM,   MSC_METADEX_BLOCK,      MP_TX_PKT_V0},
    {MSC_TYPE_OFFER_ACCEPT_A_BET,         MSC_BET_BLOCK,          MP_TX_PKT_V0},

    // end of array marker, in addition to sizeof/sizeof
    {-1,-1},
};

/**
 * Checks, if the script type is allowed as input.
 */
bool IsAllowedInputType(int whichType, int nBlock)
{
    switch (whichType)
    {
        case TX_PUBKEYHASH:
            return true;

        case TX_SCRIPTHASH:
            return (P2SH_BLOCK <= nBlock || isNonMainNet());
    }

    return false;
}

/**
 * Checks, if the script type qualifies as output.
 */
bool IsAllowedOutputType(int whichType, int nBlock)
{
    switch (whichType)
    {
        case TX_PUBKEYHASH:
            return true;

        case TX_MULTISIG:
            return true;

        case TX_SCRIPTHASH:
            return (P2SH_BLOCK <= nBlock || isNonMainNet());

        case TX_NULL_DATA:
            return (OP_RETURN_BLOCK <= nBlock || isNonMainNet());
    }

    return false;
}

/**
 * Checks, if the transaction type and version is supported and enabled.
 *
 * Certain transaction types are not live/enabled until some specific block height.
 * Certain transactions will be unknown to the client, i.e. "black holes" based on their version.
 *
 * The restrictions array is as such:
 *   type, block-allowed-in, top-version-allowed
 */
bool IsTransactionTypeAllowed(int txBlock, uint32_t txProperty, uint16_t txType, uint16_t version, bool bAllowNullProperty) {
    bool bAllowed = false;
    bool bBlackHole = false;
    unsigned int type;
    int block_FirstAllowed;
    unsigned short version_TopAllowed;

    // bitcoin as property is never allowed, unless explicitly stated otherwise
    if ((OMNI_PROPERTY_BTC == txProperty) && !bAllowNullProperty) return false;

    // everything is always allowed on Bitcoin's TestNet or with TMSC/TestEcosystem on MainNet
    if (isNonMainNet() || isTestEcosystemProperty(txProperty)) {
        bAllowed = true;
    }

    for (unsigned int i = 0; i < sizeof(txRestrictionsRules)/sizeof(txRestrictionsRules[0]); i++) {
        type = txRestrictionsRules[i][0];
        block_FirstAllowed = txRestrictionsRules[i][1];
        version_TopAllowed = txRestrictionsRules[i][2];

        if (txType != type) continue;

        if (version_TopAllowed < version) {
            PrintToLog("Black Hole identified !!! %d, %u, %u, %u\n", txBlock, txProperty, txType, version);

            bBlackHole = true;

            // TODO: what else?
            // ...
        }

        if (0 > block_FirstAllowed) break; // array contains a negative -- nothing's allowed or done parsing

        if (block_FirstAllowed <= txBlock) bAllowed = true;
    }

    return bAllowed && !bBlackHole;
}

/** Obtains a hash of all balances to use for consensus verification & checkpointing. */
uint256 GetConsensusHash()
{
    // allocate and init a SHA256_CTX
    SHA256_CTX shaCtx;
    SHA256_Init(&shaCtx);

    LOCK(cs_tally);

    if (msc_debug_consensus_hash) PrintToLog("Beginning generation of current consensus hash...\n");

    // loop through the tally map, updating the sha context with the data from each balance & type
    for (std::map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
        const std::string& address = my_it->first;
        CMPTally& tally = my_it->second;
        tally.init();
        uint32_t propertyId;
        while (0 != (propertyId = (tally.next()))) {
            /*
            for maximum flexibility so other implementations (eg OmniWallet & Chest) can also apply this methodology without
            necessarily using the same exact data types (which would be needed to hash the data bytes directly), create a
            string in the following format for each address/property id combo to use for hashing:

                address|propertyid|balance|selloffer_reserve|accept_reserve|metadex_reserve

            note: pending tally is ignored
            */

            int64_t balance = tally.getMoney(propertyId, BALANCE);
            int64_t sellOfferReserve = tally.getMoney(propertyId, SELLOFFER_RESERVE);
            int64_t acceptReserve = tally.getMoney(propertyId, ACCEPT_RESERVE);
            int64_t metaDExReserve = tally.getMoney(propertyId, METADEX_RESERVE);

            // skip this entry if all balances are empty
            if (!balance && !sellOfferReserve && !acceptReserve && !metaDExReserve) continue;

            const std::string& dataStr = address + "|" + FormatIndivisibleMP(propertyId) + "|" +
                                         FormatIndivisibleMP(balance) + "|" +
                                         FormatIndivisibleMP(sellOfferReserve) + "|" +
                                         FormatIndivisibleMP(acceptReserve) + "|" +
                                         FormatIndivisibleMP(metaDExReserve);

            if (msc_debug_consensus_hash) PrintToLog("Adding data to consensus hash: %s\n", dataStr);

            // update the sha context with the data string
            SHA256_Update(&shaCtx, dataStr.c_str(), dataStr.length());
        }
    }

    // extract the final result and return the hash
    uint256 consensusHash;
    SHA256_Final((unsigned char*)&consensusHash, &shaCtx);
    if (msc_debug_consensus_hash) PrintToLog("Finished generation of consensus hash.  Result: %s\n", consensusHash.GetHex());

    return consensusHash;
}

/** Compares a supplied block, block hash and consensus hash against a hardcoded list of checkpoints */
bool VerifyCheckpoint(int block, uint256 blockHash)
{
    // checkpoints are only verified for mainnet
    if (isNonMainNet()) return true;

    for (unsigned int i = 0; i < sizeof(vCheckpoints)/sizeof(vCheckpoints[0]); i++) {
        if (block != vCheckpoints[i].blockHeight) continue;

        if (blockHash != vCheckpoints[i].blockHash) return false;

        // only verify if there is a checkpoint to verify against
        uint256 consensusHash = GetConsensusHash();
        if (consensusHash != vCheckpoints[i].consensusHash) return false;
    }

    // either checkpoint matched or we don't have a checkpoint for this block
    return true;
}

} // namespace mastercore
