// Copyright (c) 2019-2021 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_WALLET_EXTERNAL_SIGNER_SCRIPTPUBKEYMAN_H
#define TORTOISECOIN_WALLET_EXTERNAL_SIGNER_SCRIPTPUBKEYMAN_H

#include <wallet/scriptpubkeyman.h>

#include <memory>

struct bilingual_str;

namespace wallet {
class ExternalSignerScriptPubKeyMan : public DescriptorScriptPubKeyMan
{
  public:
  ExternalSignerScriptPubKeyMan(WalletStorage& storage, WalletDescriptor& descriptor, int64_t keypool_size)
      :   DescriptorScriptPubKeyMan(storage, descriptor, keypool_size)
      {}
  ExternalSignerScriptPubKeyMan(WalletStorage& storage, int64_t keypool_size)
      :   DescriptorScriptPubKeyMan(storage, keypool_size)
      {}

  /** Provide a descriptor at setup time
  * Returns false if already setup or setup fails, true if setup is successful
  */
  bool SetupDescriptor(WalletBatch& batch, std::unique_ptr<Descriptor>desc);

  static ExternalSigner GetExternalSigner();

  /**
  * Display address on the device and verify that the returned value matches.
  * @returns nothing or an error message
  */
 util::Result<void> DisplayAddress(const CTxDestination& dest, const ExternalSigner& signer) const;

  std::optional<common::PSBTError> FillPSBT(PartiallySignedTransaction& psbt, const PrecomputedTransactionData& txdata, int sighash_type = 1 /* SIGHASH_ALL */, bool sign = true, bool bip32derivs = false, int* n_signed = nullptr, bool finalize = true) const override;
};
} // namespace wallet
#endif // TORTOISECOIN_WALLET_EXTERNAL_SIGNER_SCRIPTPUBKEYMAN_H
