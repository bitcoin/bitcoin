/**
 * @file rules.cpp
 *
 * This file contains consensus rules and restrictions.
 */

#include "omnicore/rules.h"

#include "omnicore/log.h"
#include "omnicore/omnicore.h"
#include "omnicore/notifications.h"
#include "omnicore/utilsbitcoin.h"
#include "omnicore/version.h"

#include "chainparams.h"
#include "script/standard.h"
#include "uint256.h"

#include <openssl/sha.h>

#include <stdint.h>
#include <limits>
#include <string>
#include <vector>

namespace mastercore
{
/**
 * Returns a mapping of transaction types, and the blocks at which they are enabled.
 */
std::vector<TransactionRestriction> CConsensusParams::GetRestrictions() const
{
    const TransactionRestriction vTxRestrictions[] =
    { //  transaction type                    version        allow 0  activation block
      //  ----------------------------------  -------------  -------  ------------------
        { OMNICORE_MESSAGE_TYPE_ALERT,        0xFFFF,        true,    MSC_ALERT_BLOCK    },
        { OMNICORE_MESSAGE_TYPE_ACTIVATION,   0xFFFF,        true,    MSC_ALERT_BLOCK    },

        { MSC_TYPE_SIMPLE_SEND,               MP_TX_PKT_V0,  false,   MSC_SEND_BLOCK     },

        { MSC_TYPE_TRADE_OFFER,               MP_TX_PKT_V0,  false,   MSC_DEX_BLOCK      },
        { MSC_TYPE_TRADE_OFFER,               MP_TX_PKT_V1,  false,   MSC_DEX_BLOCK      },
        { MSC_TYPE_ACCEPT_OFFER_BTC,          MP_TX_PKT_V0,  false,   MSC_DEX_BLOCK      },

        { MSC_TYPE_CREATE_PROPERTY_FIXED,     MP_TX_PKT_V0,  false,   MSC_SP_BLOCK       },
        { MSC_TYPE_CREATE_PROPERTY_VARIABLE,  MP_TX_PKT_V0,  false,   MSC_SP_BLOCK       },
        { MSC_TYPE_CREATE_PROPERTY_VARIABLE,  MP_TX_PKT_V1,  false,   MSC_SP_BLOCK       },
        { MSC_TYPE_CLOSE_CROWDSALE,           MP_TX_PKT_V0,  false,   MSC_SP_BLOCK       },

        { MSC_TYPE_CREATE_PROPERTY_MANUAL,    MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_GRANT_PROPERTY_TOKENS,     MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_REVOKE_PROPERTY_TOKENS,    MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },
        { MSC_TYPE_CHANGE_ISSUER_ADDRESS,     MP_TX_PKT_V0,  false,   MSC_MANUALSP_BLOCK },

        { MSC_TYPE_SEND_TO_OWNERS,            MP_TX_PKT_V0,  false,   MSC_STO_BLOCK      },

        { MSC_TYPE_METADEX_TRADE,             MP_TX_PKT_V0,  false,   MSC_METADEX_BLOCK  },
        { MSC_TYPE_METADEX_CANCEL_PRICE,      MP_TX_PKT_V0,  false,   MSC_METADEX_BLOCK  },
        { MSC_TYPE_METADEX_CANCEL_PAIR,       MP_TX_PKT_V0,  false,   MSC_METADEX_BLOCK  },
        { MSC_TYPE_METADEX_CANCEL_ECOSYSTEM,  MP_TX_PKT_V0,  false,   MSC_METADEX_BLOCK  },

        { MSC_TYPE_SEND_ALL,                  MP_TX_PKT_V0,  false,   MSC_SEND_ALL_BLOCK },

        { MSC_TYPE_OFFER_ACCEPT_A_BET,        MP_TX_PKT_V0,  false,   MSC_BET_BLOCK      },
    };

    const size_t nSize = sizeof(vTxRestrictions) / sizeof(vTxRestrictions[0]);

    return std::vector<TransactionRestriction>(vTxRestrictions, vTxRestrictions + nSize);
}

/**
 * Returns an empty vector of consensus checkpoints.
 *
 * This method should be overwriten by the child classes, if needed.
 */
std::vector<ConsensusCheckpoint> CConsensusParams::GetCheckpoints() const
{
    return std::vector<ConsensusCheckpoint>();
}

/**
 * Returns consensus checkpoints for mainnet, used to verify transaction processing.
 */
std::vector<ConsensusCheckpoint> CMainConsensusParams::GetCheckpoints() const
{
    // block height, block hash and consensus hash
    const ConsensusCheckpoint vCheckpoints[] = {
        {      0, uint256("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"),
                  uint256("55b852781b9995a44c939b64e441ae2724b96f99c8f4fb9a141cfc9842c4b0e3") },
        { 250000, uint256("000000000000003887df1f29024b06fc2200b55f8af8f35453d7be294df2d214"),
                  uint256("f8ca97346756bbe3b530d052c884b2f54c14edc1498ba2176214857dee13f3ca") },
        { 260000, uint256("000000000000001fb91fbcebaaba0e2d926f04908d798a8b598c3bd962951080"),
                  uint256("e8b5445bbfc915e59164a6c22e4d84b23539569ee3c95b49eefaeaca86ea98a0") },
        { 270000, uint256("0000000000000002a775aec59dc6a9e4bb1c025cf1b8c2195dd9dc3998c827c5"),
                  uint256("486a81cb5b0c89d26543e85f2635f225468e87bda89e752ab2dcebf1d2f74248") },
        { 280000, uint256("0000000000000001c091ada69f444dc0282ecaabe4808ddbb2532e5555db0c03"),
                  uint256("801a9d4099802a49bdea1e7567680e11a50e120e5446a3be0a1c3757eff3b2b3") },
        { 290000, uint256("0000000000000000fa0b2badd05db0178623ebf8dd081fe7eb874c26e27d0b3b"),
                  uint256("a6ad14928b6a96ca2c2b1426e264d5c461e723145c7169248dadb87b7693a7dd") },
        { 300000, uint256("000000000000000082ccf8f1557c5d40b21edabb18d2d691cfbf87118bac7254"),
                  uint256("a26c7c6cea0ebbb52b7b75262e3c7ac036f6812b0f5b5b09c64efa5afacc6a83") },
        { 310000, uint256("0000000000000000125a28cc9e9209ddb75718f599a8039f6c9e7d9f1fb021e0"),
                  uint256("73f207b07090f29aa9e7cc793e16b49ddd14ff201bbd1103c812ceabcf594b38") },
        { 320000, uint256("000000000000000015aab005b28a326ade60f07515c33517ea5cb598f28fb7ea"),
                  uint256("0b510ba8e4ba26e563337053922b31a4f2ec78eac62f73b4b70bad7d026a0b65") },
        { 330000, uint256("00000000000000000faabab19f17c0178c754dbed023e6c871dcaf74159c5f02"),
                  uint256("fb027d26256267583eed58d40c7330435f91b85014d7bbab452d1b47264a6e88") },
        { 340000, uint256("00000000000000000d9b2508615d569e18f00c034d71474fc44a43af8d4a5003"),
                  uint256("510f73407a607d6b8c684a8a0bb70187802e418769adb8d30080bd46df71dab1") },
        { 350000, uint256("0000000000000000053cf64f0400bb38e0c4b3872c38795ddde27acb40a112bb"),
                  uint256("7fdbd8b028dba2cb7253e29378c88e0411d883ccd49422230b446df4a17e3785") },
        { 360000, uint256("00000000000000000ca6e07cf681390ff888b7f96790286a440da0f2b87c8ea6"),
                  uint256("943732e46304fc30099c6a728616c998efe689384f81b7108a65b051381656a6") },
        { 370000, uint256("000000000000000002cad3026f68357229dd6eaa6bcef6fe5166e1e53b039b8c"),
                  uint256("31e126664805d1f5d994584eb9cdce0bb6a650f6ed494ec281b335b34b49d2b9") },
    };

    const size_t nSize = sizeof(vCheckpoints) / sizeof(vCheckpoints[0]);

    return std::vector<ConsensusCheckpoint>(vCheckpoints, vCheckpoints + nSize);
}

/**
 * Constructor for mainnet consensus parameters.
 */
CMainConsensusParams::CMainConsensusParams()
{
    // Exodus related:
    exodusBonusPerWeek = 0.10;
    exodusDeadline = 1377993600;
    exodusReward = 100;
    GENESIS_BLOCK = 249498;
    LAST_EXODUS_BLOCK = 255365;
    // Notice range for feature activations:
    MIN_ACTIVATION_BLOCKS = 2048;  // ~2 weeks
    MAX_ACTIVATION_BLOCKS = 12288; // ~12 weeks
    // Script related:
    PUBKEYHASH_BLOCK = 0;
    SCRIPTHASH_BLOCK = 322000;
    MULTISIG_BLOCK = 0;
    NULLDATA_BLOCK = 999999;
    // Transaction restrictions:
    MSC_ALERT_BLOCK = 0;
    MSC_SEND_BLOCK = 249498;
    MSC_DEX_BLOCK = 290630;
    MSC_SP_BLOCK = 297110;
    MSC_MANUALSP_BLOCK = 323230;
    MSC_STO_BLOCK = 342650;
    MSC_METADEX_BLOCK = 999999;
    MSC_SEND_ALL_BLOCK = 999999;
    MSC_BET_BLOCK = 999999;
    // Other feature activations:
    GRANTEFFECTS_FEATURE_BLOCK = 999999;
    DEXMATH_FEATURE_BLOCK = 999999;
}

/**
 * Constructor for testnet consensus parameters.
 */
CTestNetConsensusParams::CTestNetConsensusParams()
{
    // Exodus related:
    exodusBonusPerWeek = 0.00;
    exodusDeadline = 1377993600;
    exodusReward = 100;
    GENESIS_BLOCK = 263000;
    LAST_EXODUS_BLOCK = std::numeric_limits<int>::max();
    // Notice range for feature activations:
    MIN_ACTIVATION_BLOCKS = 0;
    MAX_ACTIVATION_BLOCKS = 999999;
    // Script related:
    PUBKEYHASH_BLOCK = 0;
    SCRIPTHASH_BLOCK = 0;
    MULTISIG_BLOCK = 0;
    NULLDATA_BLOCK = 0;
    // Transaction restrictions:
    MSC_ALERT_BLOCK = 0;
    MSC_SEND_BLOCK = 0;
    MSC_DEX_BLOCK = 0;
    MSC_SP_BLOCK = 0;
    MSC_MANUALSP_BLOCK = 0;
    MSC_STO_BLOCK = 0;
    MSC_METADEX_BLOCK = 0;
    MSC_SEND_ALL_BLOCK = 0;
    MSC_BET_BLOCK = 999999;
    // Other feature activations:
    GRANTEFFECTS_FEATURE_BLOCK = 999999;
    DEXMATH_FEATURE_BLOCK = 999999;
}

/**
 * Constructor for regtest consensus parameters.
 */
CRegTestConsensusParams::CRegTestConsensusParams()
{
    // Exodus related:
    exodusBonusPerWeek = 0.00;
    exodusDeadline = 1377993600;
    exodusReward = 100;
    GENESIS_BLOCK = 101;
    LAST_EXODUS_BLOCK = std::numeric_limits<int>::max();
    // Notice range for feature activations:
    MIN_ACTIVATION_BLOCKS = 5;
    MAX_ACTIVATION_BLOCKS = 10;
    // Script related:
    PUBKEYHASH_BLOCK = 0;
    SCRIPTHASH_BLOCK = 0;
    MULTISIG_BLOCK = 0;
    NULLDATA_BLOCK = 0;
    // Transaction restrictions:
    MSC_ALERT_BLOCK = 0;
    MSC_SEND_BLOCK = 0;
    MSC_DEX_BLOCK = 0;
    MSC_SP_BLOCK = 0;
    MSC_MANUALSP_BLOCK = 0;
    MSC_STO_BLOCK = 0;
    MSC_METADEX_BLOCK = 0;
    MSC_SEND_ALL_BLOCK = 0;
    MSC_BET_BLOCK = 999999;
    // Other feature activations:
    GRANTEFFECTS_FEATURE_BLOCK = 999999;
    DEXMATH_FEATURE_BLOCK = 999999;
}

//! Consensus parameters for mainnet
static CMainConsensusParams mainConsensusParams;
//! Consensus parameters for testnet
static CTestNetConsensusParams testNetConsensusParams;
//! Consensus parameters for regtest mode
static CRegTestConsensusParams regTestConsensusParams;

/**
 * Returns consensus parameters for the given network.
 */
CConsensusParams& ConsensusParams(const std::string& network)
{
    if (network == "main") {
        return mainConsensusParams;
    }
    if (network == "test") {
        return testNetConsensusParams;
    }
    if (network == "regtest") {
        return regTestConsensusParams;
    }
    // Fallback:
    return mainConsensusParams;
}

/**
 * Returns currently active consensus parameter.
 */
const CConsensusParams& ConsensusParams()
{
    const std::string& network = Params().NetworkIDString();

    return ConsensusParams(network);
}

/**
 * Returns currently active mutable consensus parameter.
 */
CConsensusParams& MutableConsensusParams()
{
    const std::string& network = Params().NetworkIDString();

    return ConsensusParams(network);
}

/**
 * Checks, if the script type is allowed as input.
 */
bool IsAllowedInputType(int whichType, int nBlock)
{
    const CConsensusParams& params = ConsensusParams();

    switch (whichType)
    {
        case TX_PUBKEYHASH:
            return (params.PUBKEYHASH_BLOCK <= nBlock);

        case TX_SCRIPTHASH:
            return (params.SCRIPTHASH_BLOCK <= nBlock);
    }

    return false;
}

/**
 * Checks, if the script type qualifies as output.
 */
bool IsAllowedOutputType(int whichType, int nBlock)
{
    const CConsensusParams& params = ConsensusParams();

    switch (whichType)
    {
        case TX_PUBKEYHASH:
            return (params.PUBKEYHASH_BLOCK <= nBlock);

        case TX_SCRIPTHASH:
            return (params.SCRIPTHASH_BLOCK <= nBlock);

        case TX_MULTISIG:
            return (params.MULTISIG_BLOCK <= nBlock);

        case TX_NULL_DATA:
            return (params.NULLDATA_BLOCK <= nBlock);
    }

    return false;
}

/**
 * Activates a feature at a specific block height, authorization has already been validated.
 *
 * Note: Feature activations are consensus breaking.  It is not permitted to activate a feature within
 *       the next 2048 blocks (roughly 2 weeks), nor is it permitted to activate a feature further out
 *       than 12288 blocks (roughly 12 weeks) to ensure sufficient notice.
 *       This does not apply for activation during initialization (where loadingActivations is set true).
 */
bool ActivateFeature(uint16_t featureId, int activationBlock, uint32_t minClientVersion, int transactionBlock)
{
    PrintToLog("Feature activation requested (ID %d to go active as of block: %d)\n", featureId, activationBlock);

    const CConsensusParams& params = ConsensusParams();

    // check activation block is allowed
    if ((activationBlock < (transactionBlock + params.MIN_ACTIVATION_BLOCKS)) ||
        (activationBlock > (transactionBlock + params.MAX_ACTIVATION_BLOCKS))) {
            PrintToLog("Feature activation of ID %d refused due to notice checks\n", featureId);
            return false;
    }

    // check if client version is high enough
    if (OMNICORE_VERSION < minClientVersion) {
        PrintToLog("Feature activation of ID %d refused due to unsupported client (current: %d, needed: %d)\n", featureId, OMNICORE_VERSION, minClientVersion);
        PrintToLog("WARNING!!! AS OF BLOCK %d THIS CLIENT WILL BE OUT OF CONSENSUS AND WILL AUTOMATICALLY SHUTDOWN.\n", activationBlock);
        return false;
    }

    // check feature is recognized and activation is successful
    switch (featureId) {
        case FEATURE_CLASS_C:
            MutableConsensusParams().NULLDATA_BLOCK = activationBlock;
            PrintToLog("Feature activation of ID %d succeeded. "
                       "Class C transaction encoding is going to be enabled at block %d.\n",
                       featureId, params.NULLDATA_BLOCK);
            AddAlert("omnicore", ALERT_BLOCK_EXPIRY, activationBlock, strprintf("Feature %d ('Class C') will go live at block %d", featureId, activationBlock));
            return true;
        case FEATURE_METADEX:
            MutableConsensusParams().MSC_METADEX_BLOCK = activationBlock;
            PrintToLog("Feature activation of ID %d succeeded. "
                       "The distributed token exchange is going to be enabled at block %d.\n",
                       featureId, params.MSC_METADEX_BLOCK);
            AddAlert("omnicore", ALERT_BLOCK_EXPIRY, activationBlock, strprintf("Feature %d ('MetaDEx') will go live at block %d", featureId, activationBlock));
            return true;
        case FEATURE_BETTING:
            MutableConsensusParams().MSC_BET_BLOCK = activationBlock;
            PrintToLog("Feature activation of ID %d succeeded. "
                       "Bet transactions are going to be enabled at block %d.\n",
                       featureId, params.MSC_BET_BLOCK);
            AddAlert("omnicore", ALERT_BLOCK_EXPIRY, activationBlock, strprintf("Feature %d ('Betting') will go live at block %d", featureId, activationBlock));
            return true;
        case FEATURE_GRANTEFFECTS:
            MutableConsensusParams().GRANTEFFECTS_FEATURE_BLOCK = activationBlock;
            PrintToLog("Feature activation of ID %d succeeded. "
                       "The potential side effect of crowdsale participations, when "
                       "granting tokens, is going to be disabled at block %d.\n",
                       featureId, params.GRANTEFFECTS_FEATURE_BLOCK);
            AddAlert("omnicore", ALERT_BLOCK_EXPIRY, activationBlock, strprintf("Feature %d ('Disable Grant Effects') will go live at block %d", featureId, activationBlock));
            return true;
        case FEATURE_DEXMATH:
            MutableConsensusParams().DEXMATH_FEATURE_BLOCK = activationBlock;
            PrintToLog("Feature activation of ID %d succeeded. "
                       "Offering more tokens than available on the traditional DEx "
                       "is no longer allowed and going to be disabled at block %d.\n",
                       featureId, params.DEXMATH_FEATURE_BLOCK);
            return true;
    }

    PrintToLog("Feature activation of id %d refused due to unknown feature ID\n", featureId);
    return false;
}

/**
 * Checks, whether a feature is activated at the given block.
 */
bool IsFeatureActivated(uint16_t featureId, int transactionBlock)
{
    const CConsensusParams& params = ConsensusParams();
    int activationBlock = std::numeric_limits<int>::max();

    switch (featureId) {
        case FEATURE_CLASS_C:
            activationBlock = params.NULLDATA_BLOCK;
            break;
        case FEATURE_METADEX:
            activationBlock = params.MSC_METADEX_BLOCK;
            break;
        case FEATURE_BETTING:
            activationBlock = params.MSC_BET_BLOCK;
            break;
        case FEATURE_GRANTEFFECTS:
            activationBlock = params.GRANTEFFECTS_FEATURE_BLOCK;
            break;
        case FEATURE_DEXMATH:
            activationBlock = params.DEXMATH_FEATURE_BLOCK;
            break;
        default:
            return false;
    }

    return (transactionBlock >= activationBlock);
}

/**
 * Checks, if the transaction type and version is supported and enabled.
 *
 * In the test ecosystem, transactions, which are known to the client are allowed
 * without height restriction.
 *
 * Certain transactions use a property identifier of 0 (= BTC) as wildcard, which
 * must explicitly be allowed.
 */
bool IsTransactionTypeAllowed(int txBlock, uint32_t txProperty, uint16_t txType, uint16_t version)
{
    const std::vector<TransactionRestriction>& vTxRestrictions = ConsensusParams().GetRestrictions();

    for (std::vector<TransactionRestriction>::const_iterator it = vTxRestrictions.begin(); it != vTxRestrictions.end(); ++it)
    {
        const TransactionRestriction& entry = *it;
        if (entry.txType != txType || entry.txVersion != version) {
            continue;
        }
        // a property identifier of 0 (= BTC) may be used as wildcard
        if (OMNI_PROPERTY_BTC == txProperty && !entry.allowWildcard) {
            continue;
        }
        // transactions are not restricted in the test ecosystem
        if (isTestEcosystemProperty(txProperty)) {
            return true;
        }
        if (txBlock >= entry.activationBlock) {
            return true;
        }
    }

    return false;
}

/**
 * Obtains a hash of all balances to use for consensus verification and checkpointing.
 *
 * For increased flexibility, so other implementations like OmniWallet and OmniChest can
 * also apply this methodology without necessarily using the same exact data types (which
 * would be needed to hash the data bytes directly), create a string in the following
 * format for each address/property identifier combo to use for hashing:
 *
 * Format specifiers:
 *   "%s|%d|%d|%d|%d|%d"
 *
 * With placeholders:
 *   "address|propertyid|balance|selloffer_reserve|accept_reserve|metadex_reserve"
 *
 * Example:
 *   SHA256 round 1: "1LCShN3ntEbeRrj8XBFWdScGqw5NgDXL5R|31|8000|0|0|0"
 *   SHA256 round 2: "1LCShN3ntEbeRrj8XBFWdScGqw5NgDXL5R|2147483657|0|0|0|234235245"
 *   SHA256 round 3: "1LP7G6uiHPaG8jm64aconFF8CLBAnRGYcb|1|0|100|0|0"
 *   SHA256 round 4: "1LP7G6uiHPaG8jm64aconFF8CLBAnRGYcb|2|0|0|0|50000"
 *   SHA256 round 5: "1LP7G6uiHPaG8jm64aconFF8CLBAnRGYcb|10|0|0|1|0"
 *   SHA256 round 6: "3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b|1|12345|5|777|9000"
 *
 * Result:
 *   "3580e94167f75620dfa8c267862aa47af46164ed8edaec3a800d732050ec0607"
 *
 * The byte order is important, and in the example we assume:
 *   SHA256("abc") = "ad1500f261ff10b49c7a1796a36103b02322ae5dde404141eacf018fbf1678ba"
 *
 * Note: empty balance records and the pending tally are ignored. Addresses are sorted based
 * on lexicographical order, and balance records are sorted by the property identifiers.
 */
uint256 GetConsensusHash()
{
    // allocate and init a SHA256_CTX
    SHA256_CTX shaCtx;
    SHA256_Init(&shaCtx);

    LOCK(cs_tally);

    if (msc_debug_consensus_hash) PrintToLog("Beginning generation of current consensus hash...\n");

    // loop through the tally map, updating the sha context with the data from each balance and tally type
    for (std::map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
        const std::string& address = my_it->first;
        CMPTally& tally = my_it->second;
        tally.init();
        uint32_t propertyId = 0;
        while (0 != (propertyId = (tally.next()))) {
            int64_t balance = tally.getMoney(propertyId, BALANCE);
            int64_t sellOfferReserve = tally.getMoney(propertyId, SELLOFFER_RESERVE);
            int64_t acceptReserve = tally.getMoney(propertyId, ACCEPT_RESERVE);
            int64_t metaDExReserve = tally.getMoney(propertyId, METADEX_RESERVE);

            // skip this entry if all balances are empty
            if (!balance && !sellOfferReserve && !acceptReserve && !metaDExReserve) continue;

            std::string dataStr = strprintf("%s|%d|%d|%d|%d|%d",
                    address, propertyId, balance, sellOfferReserve, acceptReserve, metaDExReserve);

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

/**
 * Compares a supplied block, block hash and consensus hash against a hardcoded list of checkpoints.
 */
bool VerifyCheckpoint(int block, const uint256& blockHash)
{
    const std::vector<ConsensusCheckpoint>& vCheckpoints = ConsensusParams().GetCheckpoints();

    for (std::vector<ConsensusCheckpoint>::const_iterator it = vCheckpoints.begin(); it != vCheckpoints.end(); ++it) {
        const ConsensusCheckpoint& checkpoint = *it;
        if (block != checkpoint.blockHeight) {
            continue;
        }

        if (blockHash != checkpoint.blockHash) {
            PrintToLog("%s(): block hash mismatch - expected %s, received %s\n", __func__, checkpoint.blockHash.GetHex(), blockHash.GetHex());
            return false;
        }

        // only verify if there is a checkpoint to verify against
        uint256 consensusHash = GetConsensusHash();
        if (consensusHash != checkpoint.consensusHash) {
            PrintToLog("%s(): consensus hash mismatch - expected %s, received %s\n", __func__, checkpoint.consensusHash.GetHex(), consensusHash.GetHex());
            return false;
        } else {
            break;
        }
    }

    // either checkpoint matched or we don't have a checkpoint for this block
    return true;
}

} // namespace mastercore
