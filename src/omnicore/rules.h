#ifndef BITCOIN_OMNICORE_RULES_H
#define BITCOIN_OMNICORE_RULES_H

#include <uint256.h>

#include <stdint.h>
#include <string>
#include <vector>

namespace mastercore
{
//! Block to enable the Exodus fundraiser address in regtest mode
const int MONEYMAN_REGTEST_BLOCK = 101;
//! Block to enable the Exodus fundraiser address on testnet
const int MONEYMAN_TESTNET_BLOCK = 270775;

//! Feature identifier to enable Class C transaction parsing and processing
const uint16_t FEATURE_CLASS_C = 1;
//! Feature identifier to enable the distributed token exchange
const uint16_t FEATURE_METADEX = 2;
//! Feature identifier to enable betting transactions
const uint16_t FEATURE_BETTING = 3;
//! Feature identifier to disable crowdsale participations when "granting tokens"
const uint16_t FEATURE_GRANTEFFECTS = 4;
//! Feature identifier to disable DEx "over-offers" and to switch to plain integer math
const uint16_t FEATURE_DEXMATH = 5;
//! Feature identifier to enable Send All transactions
const uint16_t FEATURE_SENDALL = 6;
//! Feature identifier disable ecosystem crossovers in crowdsale logic
const uint16_t FEATURE_SPCROWDCROSSOVER = 7;
//! Feature identifier to enable non-Omni pairs on the distributed exchange
const uint16_t FEATURE_TRADEALLPAIRS = 8;
//! Feature identifier to enable the fee cache and strip 0.05% fees from non-Omni pairs
const uint16_t FEATURE_FEES = 9;
//! Feature identifier to enable cross property (v1) Send To Owners
const uint16_t FEATURE_STOV1 = 10;
//! Feature identifier to activate the waiting period for enabling managed property address freezing
const uint16_t FEATURE_FREEZENOTICE = 14;
//! Feature identifier to activate trading of any token on the distributed exchange
const uint16_t FEATURE_FREEDEX = 15;
//! Feature identifier to enable non-fungible token support
const uint16_t FEATURE_NONFUNGIBLE = 16;
//! Feature identifier to enable delegated issuance support
const uint16_t FEATURE_DELEGATEDISSUANCE = 17;

//! When (propertyTotalTokens / OMNI_FEE_THRESHOLD) is reached fee distribution will occur
const int64_t OMNI_FEE_THRESHOLD = 100000; // 0.001%

/** A structure to represent transaction restrictions.
 */
struct TransactionRestriction
{
    //! Transaction type
    uint16_t txType;
    //! Transaction version
    uint16_t txVersion;
    //! Whether the property identifier can be 0 (= BTC)
    bool allowWildcard;
    //! Block after which the feature or transaction is enabled
    int activationBlock;
};

/** A structure to represent a verification checkpoint.
 */
struct ConsensusCheckpoint
{
    int blockHeight;
    uint256 blockHash;
    uint256 consensusHash;
};

/** A structure to represent a specific transaction checkpoint.
 */
struct TransactionCheckpoint
{
    int blockHeight;
    uint256 txHash;
};

// TODO: rename allcaps variable names
// TODO: remove remaining global heights
// TODO: add Exodus addresses to params

/** Base class for consensus parameters.
 */
class CConsensusParams
{
public:
    //! Earily bird bonus per week of Exodus crowdsale
    double exodusBonusPerWeek;
    //! Deadline of Exodus crowdsale as Unix timestamp
    unsigned int exodusDeadline;
    //! Number of MSC/TMSC generated per unit invested
    int64_t exodusReward;
    //! First block of the Exodus crowdsale
    int GENESIS_BLOCK;
    //! Last block of the Exodus crowdsale
    int LAST_EXODUS_BLOCK;

    //! Minimum number of blocks to use for notice rules on activation
    int MIN_ACTIVATION_BLOCKS;
    //! Maximum number of blocks to use for notice rules on activation
    int MAX_ACTIVATION_BLOCKS;

    //! Waiting period after enabling freezing before addresses may be frozen
    int OMNI_FREEZE_WAIT_PERIOD;

    //! Block to enable pay-to-pubkey-hash support
    int PUBKEYHASH_BLOCK;
    //! Block to enable pay-to-script-hash support
    int SCRIPTHASH_BLOCK;
    //! Block to enable bare-multisig based encoding
    int MULTISIG_BLOCK;
    //! Block to enable OP_RETURN based encoding
    int NULLDATA_BLOCK;

    //! Block to enable alerts and notifications
    int MSC_ALERT_BLOCK;
    //! Block to enable simple send transactions
    int MSC_SEND_BLOCK;
    //! Block to enable DEx transactions
    int MSC_DEX_BLOCK;
    //! Block to enable smart property transactions
    int MSC_SP_BLOCK;
    //! Block to enable managed properties
    int MSC_MANUALSP_BLOCK;
    //! Block to enable send-to-owners transactions
    int MSC_STO_BLOCK;
    //! Block to enable MetaDEx transactions
    int MSC_METADEX_BLOCK;
    //! Block to enable "send all" transactions
    int MSC_SEND_ALL_BLOCK;
    //! Block to enable betting transactions
    int MSC_BET_BLOCK;
    //! Block to enable cross property STO (v1)
    int MSC_STOV1_BLOCK;
    //! Block to enable any data payloads
    int MSC_ANYDATA_BLOCK;
    //! Block to enable non-fungible tokens
    int MSC_NONFUNGIBLE_BLOCK;
    //! Block to enable delegation of token issuance
    int MSC_DELEGATED_ISSUANCE_BLOCK;

    //! Block to deactivate crowdsale participations when "granting tokens"
    int GRANTEFFECTS_FEATURE_BLOCK;
    //! Block to disable DEx "over-offers" and to switch to plain integer math
    int DEXMATH_FEATURE_BLOCK;
    //! Block to disable ecosystem crossovers in crowdsale logic
    int SPCROWDCROSSOVER_FEATURE_BLOCK;
    //! Block to enable trading of non-Omni pairs
    int TRADEALLPAIRS_FEATURE_BLOCK;
    //! Block to enable the fee system & 0.05% fee for trading non-Omni pairs
    int FEES_FEATURE_BLOCK;
    //! Block to activate the waiting period for enabling managed property address freezing
    int FREEZENOTICE_FEATURE_BLOCK;
    //! Block to activate the waiting period to activate trading of any token on the distributed exchange
    int FREEDEX_FEATURE_BLOCK;

    /** Returns a mapping of transaction types, and the blocks at which they are enabled. */
    virtual std::vector<TransactionRestriction> GetRestrictions() const;

    /** Returns an empty vector of consensus checkpoints. */
    virtual std::vector<ConsensusCheckpoint> GetCheckpoints() const;

    /** Returns an empty vector of transaction checkpoints. */
    virtual std::vector<TransactionCheckpoint> GetTransactions() const;

    /** Destructor. */
    virtual ~CConsensusParams() {}

protected:
    /** Constructor, only to be called from derived classes. */
    CConsensusParams() {}
};

/** Consensus parameters for mainnet.
 */
class CMainConsensusParams: public CConsensusParams
{
public:
    /** Constructor for mainnet consensus parameters. */
    CMainConsensusParams();
    /** Destructor. */
    virtual ~CMainConsensusParams() {}

    /** Returns consensus checkpoints for mainnet, used to verify transaction processing. */
    virtual std::vector<ConsensusCheckpoint> GetCheckpoints() const;

    /** Returns transactions checkpoints for mainnet, used to verify DB consistency. */
    virtual std::vector<TransactionCheckpoint> GetTransactions() const;
};

/** Consensus parameters for testnet.
 */
class CTestNetConsensusParams: public CConsensusParams
{
public:
    /** Constructor for testnet consensus parameters. */
    CTestNetConsensusParams();
    /** Destructor. */
    virtual ~CTestNetConsensusParams() {}
};

/** Consensus parameters for regtest mode.
 */
class CRegTestConsensusParams: public CConsensusParams
{
public:
    /** Constructor for regtest consensus parameters. */
    CRegTestConsensusParams();
    /** Destructor. */
    virtual ~CRegTestConsensusParams() {}
};

/** Returns consensus parameters for the given network. */
CConsensusParams& ConsensusParams(const std::string& network);
/** Returns currently active consensus parameter. */
const CConsensusParams& ConsensusParams();
/** Returns currently active mutable consensus parameter. */
CConsensusParams& MutableConsensusParams();
/** Resets consensus parameters. */
void ResetConsensusParams();


/** Gets the display name for a feature ID */
std::string GetFeatureName(uint16_t featureId);
/** Activates a feature at a specific block height. */
bool ActivateFeature(uint16_t featureId, int activationBlock, uint32_t minClientVersion, int transactionBlock);
/** Deactivates a feature immediately, authorization has already been validated. */
bool DeactivateFeature(uint16_t featureId, int transactionBlock);
/** Checks, whether a feature is activated at the given block. */
bool IsFeatureActivated(uint16_t featureId, int transactionBlock);
/** Checks, if the script type is allowed as input. */
bool IsAllowedInputType(int whichType, int nBlock);
/** Checks, if the script type qualifies as output. */
bool IsAllowedOutputType(int whichType, int nBlock);
/** Checks, if the transaction type and version is supported and enabled. */
bool IsTransactionTypeAllowed(int txBlock, uint32_t txProperty, uint16_t txType, uint16_t version);

/** Compares a supplied block, block hash and consensus hash against a hardcoded list of checkpoints. */
bool VerifyCheckpoint(int block, const uint256& blockHash);
/** Checks, if a specific transaction exists in the database. */
bool VerifyTransactionExistence(int block);
}

#endif // BITCOIN_OMNICORE_RULES_H
