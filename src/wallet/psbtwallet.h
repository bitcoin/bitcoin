// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_PSBTWALLET_H
#define BITCOIN_WALLET_PSBTWALLET_H

#include <psbt.h>
#include <wallet/wallet.h>

/**
 * Fills out a PSBT with information from the wallet. Fills in UTXOs if we have
 * them. Tries to sign if sign=true. Sets `complete` if the PSBT is now complete
 * (i.e. has all required signatures or signature-parts, and is ready to
 * finalize.) Sets `error` and returns false if something goes wrong.
 *
 * @param[in]  pwallet pointer to a wallet
 * @param[in]  &psbtx reference to PartiallySignedTransaction to fill in
 * @param[out] &complete indicates whether the PSBT is now complete
 * @param[in]  sighash_type the sighash type to use when signing (if PSBT does not specify)
 * @param[in]  sign whether to sign or not
 * @param[in]  bip32derivs whether to fill in bip32 derivation information if available
 * return error
 */
NODISCARD TransactionError FillPSBT(const CWallet* pwallet,
              PartiallySignedTransaction& psbtx,
              bool& complete,
              int sighash_type = 1 /* SIGHASH_ALL */,
              bool sign = true,
              bool bip32derivs = true);

#endif // BITCOIN_WALLET_PSBTWALLET_H
