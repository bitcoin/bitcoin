// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/settings.h>

#include <policy/feerate.h>
#include <policy/policy.h>

bool fEnableReplacement = DEFAULT_ENABLE_REPLACEMENT;
bool fReplacementHonourOptOut = DEFAULT_REPLACEMENT_HONOUR_OPTOUT;
bool fIsBareMultisigStd = DEFAULT_PERMIT_BAREMULTISIG;
CFeeRate incrementalRelayFee = CFeeRate(DEFAULT_INCREMENTAL_RELAY_FEE);
CFeeRate dustRelayFee = CFeeRate(DUST_RELAY_TX_FEE);
unsigned int nBytesPerSigOp = DEFAULT_BYTES_PER_SIGOP;
unsigned int nBytesPerSigOpStrict = DEFAULT_BYTES_PER_SIGOP_STRICT;
