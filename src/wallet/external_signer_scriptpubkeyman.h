// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_EXTERNAL_SIGNER_SCRIPTPUBKEYMAN_H
#define BITCOIN_WALLET_EXTERNAL_SIGNER_SCRIPTPUBKEYMAN_H

#include <wallet/scriptpubkeyman.h>

#include <memory>
#include <util/result.h>

struct bilingual_str;

namespace wallet {
class ExternalSignerScriptPubKeyMan : public DescriptorScriptPubKeyMan
{
private:
    //! Create an ExternalSPKM from existing wallet data
    ExternalSignerScriptPubKeyMan(WalletStorage& storage, WalletDescriptor& descriptor, int64_t keypool_size, const KeyMap& keys, const CryptedKeyMap& ckeys)
        : DescriptorScriptPubKeyMan(storage, descriptor, keypool_size, keys, ckeys)
    {}

    ExternalSignerScriptPubKeyMan(WalletStorage& storage, int64_t keypool_size)
        : DescriptorScriptPubKeyMan(storage, keypool_size)
    {}

public:
    static std::unique_ptr<ExternalSignerScriptPubKeyMan> LoadFromStorage(WalletStorage& storage, WalletDescriptor& descriptor, int64_t keypool_size, const KeyMap& keys, const CryptedKeyMap& ckeys);
    static std::unique_ptr<ExternalSignerScriptPubKeyMan> CreateNew(WalletStorage& storage, WalletBatch& batch, int64_t keypool_size, std::unique_ptr<Descriptor> desc);

  static util::Result<ExternalSigner> GetExternalSigner();

  /**
  * Display address on the device and verify that the returned value matches.
  * @returns nothing or an error message
  */
 util::Result<void> DisplayAddress(const CTxDestination& dest, const ExternalSigner& signer) const;

  std::optional<common::PSBTError> FillPSBT(PartiallySignedTransaction& psbt, const PrecomputedTransactionData& txdata, const common::PSBTFillOptions& options, int* n_signed = nullptr) const override;
};
} // namespace wallet
#endif // BITCOIN_WALLET_EXTERNAL_SIGNER_SCRIPTPUBKEYMAN_H
