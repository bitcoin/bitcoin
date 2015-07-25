#ifndef OMNICORE_RULES_H
#define OMNICORE_RULES_H

class uint256;

#include <stdint.h>
#include <string>
#include <vector>

namespace mastercore
{
//! Block to enable the Exodus fundraiser address in regtest mode
const int MONEYMAN_REGTEST_BLOCK = 101;
//! Block to enable the Exodus fundraiser address on testnet
const int MONEYMAN_TESTNET_BLOCK = 270775;

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

    //! Block to enable pay-to-pubkey-hash support
    int PUBKEYHASH_BLOCK;
    //! Block to enable pay-to-script-hash support
    int SCRIPTHASH_BLOCK;
    //! Block to enable bare-multisig based encoding
    int MULTISIG_BLOCK;
    //! Block to enable OP_RETURN based encoding
    int NULLDATA_BLOCK;

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
    //! Block to enable betting transactions
    int MSC_BET_BLOCK;

    /** Returns a mapping of transaction types, and the blocks at which they are enabled. */
    std::vector<TransactionRestriction> GetRestrictions() const;

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
};

/** Consensus parameters for testnet.
 */
class CTestNetConsensusParams: public CConsensusParams
{
public:
    /** Constructor for testnet consensus parameters. */
    CTestNetConsensusParams();
};

/** Consensus parameters for regtest mode.
 */
class CRegTestConsensusParams: public CConsensusParams
{
public:
    /** Constructor for regtest consensus parameters. */
    CRegTestConsensusParams();
};

/** Returns consensus parameters for the given network. */
CConsensusParams& ConsensusParams(const std::string& network);
/** Returns currently active consensus parameter. */
const CConsensusParams& ConsensusParams();
/** Checks, if the script type is allowed as input. */
bool IsAllowedInputType(int whichType, int nBlock);
/** Checks, if the script type qualifies as output. */
bool IsAllowedOutputType(int whichType, int nBlock);
/** Checks, if the transaction type and version is supported and enabled. */
bool IsTransactionTypeAllowed(int txBlock, uint32_t txProperty, uint16_t txType, uint16_t version);
/** Obtains a hash of all balances to use for consensus verification & checkpointing. */
uint256 GetConsensusHash();
/** Compares a supplied block, block hash and consensus hash against a hardcoded list of checkpoints */
bool VerifyCheckpoint(int block, uint256 blockHash);
}

#endif // OMNICORE_RULES_H
