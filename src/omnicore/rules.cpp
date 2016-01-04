/**
 * @file rules.cpp
 *
 * This file contains consensus rules and restrictions.
 */

#include "omnicore/rules.h"

#include "omnicore/activation.h"
#include "omnicore/consensushash.h"
#include "omnicore/log.h"
#include "omnicore/omnicore.h"
#include "omnicore/notifications.h"
#include "omnicore/utilsbitcoin.h"
#include "omnicore/version.h"

#include "alert.h"
#include "chainparams.h"
#include "main.h"
#include "script/standard.h"
#include "uint256.h"
#include "ui_interface.h"

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
                  uint256("15cecb88f2d0eb8a2a85e8c8c3e2f60df1b2b365404eac97abfecead8d92e371") },
        { 250000, uint256("000000000000003887df1f29024b06fc2200b55f8af8f35453d7be294df2d214"),
                  uint256("c2e1e0f3cf3c49d8ee08bd45ad39be27eb400041d6288864ee144892449c97df") },
        { 260000, uint256("000000000000001fb91fbcebaaba0e2d926f04908d798a8b598c3bd962951080"),
                  uint256("cfe2c574a9f969cfa26f23d3a0a7b3c3f416b50e7fb7b2adffe4524a4a7b0992") },
        { 270000, uint256("0000000000000002a775aec59dc6a9e4bb1c025cf1b8c2195dd9dc3998c827c5"),
                  uint256("46daa1df4cea9a1edc9624091d94839203239502bafcc3a20df2fee1a446cf42") },
        { 280000, uint256("0000000000000001c091ada69f444dc0282ecaabe4808ddbb2532e5555db0c03"),
                  uint256("4739e5d00fc94e079428cd5a29421df4de3f2b3a4903990a162d8afdd2605fd9") },
        { 290000, uint256("0000000000000000fa0b2badd05db0178623ebf8dd081fe7eb874c26e27d0b3b"),
                  uint256("51cb4219ae68cc4bf5bd835a1fadec4c4e587c3653304dfdff8109ea0795bfcb") },
        { 300000, uint256("000000000000000082ccf8f1557c5d40b21edabb18d2d691cfbf87118bac7254"),
                  uint256("3bc727b74dd660ac080c974af67bb627f5a059b82935839a37ce3309af68b7be") },
        { 310000, uint256("0000000000000000125a28cc9e9209ddb75718f599a8039f6c9e7d9f1fb021e0"),
                  uint256("1a1923a644bee373649e01a0a825daae8d0b862a3488e7f5b092599862169fb7") },
        { 320000, uint256("000000000000000015aab005b28a326ade60f07515c33517ea5cb598f28fb7ea"),
                  uint256("30f169f9bff9296157b6d116560af485dc6bccabaf827393d4683823e9dca1f4") },
        { 330000, uint256("00000000000000000faabab19f17c0178c754dbed023e6c871dcaf74159c5f02"),
                  uint256("52cb759cf37cf8aa25c14f988e1515b966e40cd29a310b8faff6cab0bfe0112e") },
        { 340000, uint256("00000000000000000d9b2508615d569e18f00c034d71474fc44a43af8d4a5003"),
                  uint256("1e21df8610d5ed32645df4e1aacebbb423e4ebe7097015a2392070bb53b3bdf1") },
        { 350000, uint256("0000000000000000053cf64f0400bb38e0c4b3872c38795ddde27acb40a112bb"),
                  uint256("05e89e25bb86688aac26bc796084638f10bc7564e391eb5c31e07e26f952f92f") },
        { 360000, uint256("00000000000000000ca6e07cf681390ff888b7f96790286a440da0f2b87c8ea6"),
                  uint256("0ffa97ffd5ac83030d50fbb23e0e953ff3717aa0b5181734e782a62ac39925af") },
        { 370000, uint256("000000000000000002cad3026f68357229dd6eaa6bcef6fe5166e1e53b039b8c"),
                  uint256("4cce696e822f390fc83a730095d39f5cca5121398829c087dd0c92154e1fb83c") },
        { 380000, uint256("00000000000000000b06cee3cee10d2617e2024a996f5c613f7d786b15a571ff"),
                  uint256("32b092620f37c02a1ca33acf5b1f3752642b23e8089ffc4ff0ae401ed41aa9d7") },
        { 390000, uint256("00000000000000000520000e60b56818523479ada2614806ba17ce0bbe6eaded"),
                  uint256("ef1812cf6cc1b1b89de173666126744e3f2441bb32c5e28233088f4c8757eb19") },
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
    SPCROWDCROSSOVER_FEATURE_BLOCK = 999999;
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
    SPCROWDCROSSOVER_FEATURE_BLOCK = 999999;
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
    SPCROWDCROSSOVER_FEATURE_BLOCK = 999999;
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
 * Resets consensus paramters.
 */
void ResetConsensusParams()
{
    mainConsensusParams = CMainConsensusParams();
    testNetConsensusParams = CTestNetConsensusParams();
    regTestConsensusParams = CRegTestConsensusParams();
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

    // check whether the feature is already active
    if (IsFeatureActivated(featureId, transactionBlock)) {
        PrintToLog("Feature activation of ID %d refused as the feature is already live\n", featureId);
        return false;
    }

    // check feature is recognized and activation is successful
    std::string featureName;
    bool supported = OMNICORE_VERSION >= minClientVersion;
    switch (featureId) {
        case FEATURE_CLASS_C:
            MutableConsensusParams().NULLDATA_BLOCK = activationBlock;
            featureName = "Class C transaction encoding";
        break;
        case FEATURE_METADEX:
            MutableConsensusParams().MSC_METADEX_BLOCK = activationBlock;
            featureName = "Distributed Meta Token Exchange";
        break;
        case FEATURE_BETTING:
            MutableConsensusParams().MSC_BET_BLOCK = activationBlock;
            featureName = "Bet transactions";
        break;
        case FEATURE_GRANTEFFECTS:
            MutableConsensusParams().GRANTEFFECTS_FEATURE_BLOCK = activationBlock;
            featureName = "Remove grant side effects";
        break;
        case FEATURE_DEXMATH:
            MutableConsensusParams().DEXMATH_FEATURE_BLOCK = activationBlock;
            featureName = "DEx integer math update";
        break;
        case FEATURE_SENDALL:
            MutableConsensusParams().MSC_SEND_ALL_BLOCK = activationBlock;
            featureName = "Send All transactions";
        break;
        case FEATURE_SPCROWDCROSSOVER:
            MutableConsensusParams().SPCROWDCROSSOVER_FEATURE_BLOCK = activationBlock;
            featureName = "Disable crowdsale ecosystem crossovers";
        break;
        default:
            featureName = "Unknown feature";
            supported = false;
        break;
    }

    PrintToLog("Feature activation of ID %d processed. %s will be enabled at block %d.\n", featureId, featureName, activationBlock);
    AddPendingActivation(featureId, activationBlock, minClientVersion, featureName);

    if (!supported) {
        PrintToLog("WARNING!!! AS OF BLOCK %d THIS CLIENT WILL BE OUT OF CONSENSUS AND WILL AUTOMATICALLY SHUTDOWN.\n", activationBlock);
        std::string alertText = strprintf("Your client must be updated and will shutdown at block %d (unsupported feature %d ('%s') activated)\n",
                                          activationBlock, featureId, featureName);
        AddAlert("omnicore", ALERT_BLOCK_EXPIRY, activationBlock, alertText);
        CAlert::Notify(alertText, true);
    }

    return true;
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
        case FEATURE_SENDALL:
            activationBlock = params.MSC_SEND_ALL_BLOCK;
            break;
        case FEATURE_SPCROWDCROSSOVER:
            activationBlock = params.SPCROWDCROSSOVER_FEATURE_BLOCK;
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
