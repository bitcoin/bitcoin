#ifndef OMNICORE_RULES_H
#define OMNICORE_RULES_H

#include <stdint.h>

namespace mastercore
{
//! Starting block for parsing in regtest mode
const int START_REGTEST_BLOCK = 5;
//! Block to enable the Exodus fundraiser address in regtest mode
const int MONEYMAN_REGTEST_BLOCK = 101;
//! Starting block for parsing on testnet
const int START_TESTNET_BLOCK = 263000;
//! Block to enable the Exodus fundraiser address on testnet
const int MONEYMAN_TESTNET_BLOCK = 270775;
//! First block of the Exodus fundraiser
const int GENESIS_BLOCK = 249498;
//! Last block of the Exodus fundraiser
const int LAST_EXODUS_BLOCK = 255365;
//! Block to enable DEx transactions
const int MSC_DEX_BLOCK = 290630;
//! Block to enable smart property transactions
const int MSC_SP_BLOCK = 297110;
//! Block to enable managed properties
const int MSC_MANUALSP_BLOCK = 323230;
//! Block to enable send-to-owners transactions
const int MSC_STO_BLOCK = 342650;
//! Block to enable MetaDEx transactions
const int MSC_METADEX_BLOCK = 999999;
//! Block to enable betting transactions
const int MSC_BET_BLOCK = 999999;
//! Block to enable pay-to-script-hash support
const int P2SH_BLOCK = 322000;
//! Block to enable OP_RETURN based encoding
const int OP_RETURN_BLOCK = 999999;

/** Checks, if the script type is allowed as input. */
bool IsAllowedInputType(int whichType, int nBlock);
/** Checks, if the script type qualifies as output. */
bool IsAllowedOutputType(int whichType, int nBlock);
/** Checks, if the transaction type and version is supported and enabled. */
bool IsTransactionTypeAllowed(int txBlock, uint32_t txProperty, uint16_t txType, uint16_t version, bool bAllowNullProperty = false);
}

#endif // OMNICORE_RULES_H
