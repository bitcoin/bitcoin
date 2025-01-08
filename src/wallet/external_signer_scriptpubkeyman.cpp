// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <common/args.h>
#include <common/system.h>
#include <external_signer.h>
#include <node/types.h>
#include <wallet/external_signer_scriptpubkeyman.h>

#include <iostream>
#include <key_io.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <univalue.h>
#include <utility>
#include <vector>

using common::PSBTError;

namespace wallet {
bool ExternalSignerScriptPubKeyMan::SetupDescriptor(WalletBatch& batch, std::unique_ptr<Descriptor> desc)
{
    LOCK(cs_desc_man);
    assert(m_storage.IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));
    assert(m_storage.IsWalletFlagSet(WALLET_FLAG_EXTERNAL_SIGNER));

    int64_t creation_time = GetTime();

    // Make the descriptor
    WalletDescriptor w_desc(std::move(desc), creation_time, 0, 0, 0);
    m_wallet_descriptor = w_desc;

    // Store the descriptor
    if (!batch.WriteDescriptor(GetID(), m_wallet_descriptor)) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor failed");
    }

    // TopUp
    TopUpWithDB(batch);

    m_storage.UnsetBlankWalletFlag(batch);
    return true;
}

ExternalSigner ExternalSignerScriptPubKeyMan::GetExternalSigner() {
    const std::string command = gArgs.GetArg("-signer", "");
    if (command == "") throw std::runtime_error(std::string(__func__) + ": restart bitcoind with -signer=<cmd>");
    std::vector<ExternalSigner> signers;
    ExternalSigner::Enumerate(command, signers, Params().GetChainTypeString());
    if (signers.empty()) throw std::runtime_error(std::string(__func__) + ": No external signers found");
    // TODO: add fingerprint argument instead of failing in case of multiple signers.
    if (signers.size() > 1) throw std::runtime_error(std::string(__func__) + ": More than one external signer found. Please connect only one at a time.");
    return signers[0];
}

util::Result<void> ExternalSignerScriptPubKeyMan::DisplayAddress(const CTxDestination& dest, const ExternalSigner &signer) const
{
    // TODO: avoid the need to infer a descriptor from inside a descriptor wallet
    const CScript& scriptPubKey = GetScriptForDestination(dest);
    auto provider = GetSolvingProvider(scriptPubKey);
    auto descriptor = InferDescriptor(scriptPubKey, *provider);

    const UniValue& result = signer.DisplayAddress(descriptor->ToString());

    const UniValue& error = result.find_value("error");
    if (error.isStr()) return util::Error{strprintf(_("Signer returned error: %s"), error.getValStr())};

    const UniValue& ret_address = result.find_value("address");
    if (!ret_address.isStr()) return util::Error{_("Signer did not echo address")};

    if (ret_address.getValStr() != EncodeDestination(dest)) {
        return util::Error{strprintf(_("Signer echoed unexpected address %s"), ret_address.getValStr())};
    }

    return util::Result<void>();
}

// If sign is true, transaction must previously have been filled
std::optional<PSBTError> ExternalSignerScriptPubKeyMan::FillPSBT(PartiallySignedTransaction& psbt, const PrecomputedTransactionData& txdata, std::optional<int> sighash_type, bool sign, bool bip32derivs, int* n_signed, bool finalize) const
{
    if (!sign) {
        return DescriptorScriptPubKeyMan::FillPSBT(psbt, txdata, sighash_type, false, bip32derivs, n_signed, finalize);
    }

    // Already complete if every input is now signed
    bool complete = true;
    for (const auto& input : psbt.inputs) {
        // TODO: for multisig wallets, we should only care if all _our_ inputs are signed
        complete &= PSBTInputSigned(input);
    }
    if (complete) return {};

    std::string strFailReason;
    if(!GetExternalSigner().SignTransaction(psbt, strFailReason)) {
        tfm::format(std::cerr, "Failed to sign: %s\n", strFailReason);
        return PSBTError::EXTERNAL_SIGNER_FAILED;
    }
    if (finalize) FinalizePSBT(psbt); // This won't work in a multisig setup
    return {};
}
} // namespace wallet
