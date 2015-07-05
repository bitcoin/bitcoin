#ifndef OMNICORE_RULES_H
#define OMNICORE_RULES_H

#include <stdint.h>

namespace mastercore
{
/** The block heights with which corresponding transactions or features are considered as valid or enabled.
 */
enum BLOCKHEIGHTRESTRICTIONS {
    //! Starting block for parsing in regtest mode
    START_REGTEST_BLOCK = 5,
    //! Block to enable the Exodus fundraiser address in regtest mode
    MONEYMAN_REGTEST_BLOCK = 101,
    //! Starting block for parsing on testnet
    START_TESTNET_BLOCK = 263000,
    //! Block to enable the Exodus fundraiser address on testnet
    MONEYMAN_TESTNET_BLOCK = 270775,
    //! First block of the Exodus fundraiser
    GENESIS_BLOCK = 249498,
    //! Last block of the Exodus fundraiser
    LAST_EXODUS_BLOCK = 255365,
    //! Block to enable DEx transactions
    MSC_DEX_BLOCK = 290630,
    //! Block to enable smart property transactions
    MSC_SP_BLOCK = 297110,
    //! Block to enable managed properties
    MSC_MANUALSP_BLOCK = 323230,
    //! Block to enable send-to-owners transactions
    MSC_STO_BLOCK = 342650,
    //! Block to enable MetaDEx transactions
    MSC_METADEX_BLOCK = 999999,
    //! Block to enable betting transactions
    MSC_BET_BLOCK = 999999,
    //! Block to enable pay-to-script-hash support
    P2SH_BLOCK = 322000,
    //! Block to enable OP_RETURN based encoding
    OP_RETURN_BLOCK = 999999
};

/** Checks, if the script type is allowed as input. */
bool IsAllowedInputType(int whichType, int nBlock);
/** Checks, if the script type qualifies as output. */
bool IsAllowedOutputType(int whichType, int nBlock);
/** Checks, if the transaction type and version is supported and enabled. */
bool IsTransactionTypeAllowed(int txBlock, uint32_t txProperty, uint16_t txType, uint16_t version, bool bAllowNullProperty = false);
}

#endif // OMNICORE_RULES_H
