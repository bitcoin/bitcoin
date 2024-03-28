// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_EXTERNAL_SIGNER_SCRIPTPUBKEYMAN_H
#define BITCOIN_WALLET_EXTERNAL_SIGNER_SCRIPTPUBKEYMAN_H

#include <wallet/scriptpubkeyman.h>

#include <memory>

namespace wallet {
class ExternalSignerScriptPubKeyMan : public DescriptorScriptPubKeyMan
{
public:
    //! Create an ExternalSPKM from existing wallet data
    ExternalSignerScriptPubKeyMan(WalletStorage& storage, const uint256& id, WalletDescriptor& descriptor, int64_t keypool_size, const KeyMap& keys, const CryptedKeyMap& ckeys)
      :   DescriptorScriptPubKeyMan(storage, id, descriptor, keypool_size, keys, ckeys)
      {}
    //! Create a new ExternalSPKM from just a descriptor
    ExternalSignerScriptPubKeyMan(WalletStorage& storage, WalletBatch& batch, int64_t keypool_size, std::unique_ptr<Descriptor> desc);

  static ExternalSigner GetExternalSigner();

  bool DisplayAddress(const CScript scriptPubKey, const ExternalSigner &signer) const;

  TransactionError FillPSBT(PartiallySignedTransaction& psbt, const PrecomputedTransactionData& txdata, int sighash_type = 1 /* SIGHASH_ALL */, bool sign = true, bool bip32derivs = false, int* n_signed = nullptr, bool finalize = true) const override;
};
} // namespace wallet
#endif // BITCOIN_WALLET_EXTERNAL_SIGNER_SCRIPTPUBKEYMAN_H
