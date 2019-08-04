// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparamsbase.h>
#include <wallet/externalsigner.h>
#include <wallet/external_signer_scriptpubkeyman.h>

bool ExternalSignerScriptPubKeyMan::SetupDescriptor(std::unique_ptr<Descriptor> desc)
{
    LOCK(cs_desc_man);
    assert(m_storage.IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));
    assert(m_storage.IsWalletFlagSet(WALLET_FLAG_EXTERNAL_SIGNER));

    int64_t creation_time = GetTime();

    // Make the descriptor
    WalletDescriptor w_desc(std::move(desc), creation_time, 0, 0, 0);
    m_wallet_descriptor = w_desc;

    // Store the descriptor
    WalletBatch batch(m_storage.GetDatabase());
    if (!batch.WriteDescriptor(GetID(), m_wallet_descriptor)) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor failed");
    }

    // TopUp
    TopUp();

    m_storage.UnsetBlankWalletFlag(batch);
    return true;
}

ExternalSigner ExternalSignerScriptPubKeyMan::GetExternalSigner() {
#ifdef ENABLE_EXTERNAL_SIGNER
    const std::string command = gArgs.GetArg("-signer", ""); // DEFAULT_EXTERNAL_SIGNER);
    if (command == "") throw std::runtime_error(std::string(__func__) + ": restart bitcoind with -signer=<cmd>");

    std::string chain = gArgs.GetChainName();
    const bool mainnet = chain == CBaseChainParams::MAIN;
    std::vector<ExternalSigner> signers;
    ExternalSigner::Enumerate(command, signers, mainnet);
    if (signers.empty()) throw std::runtime_error(std::string(__func__) + ": No external signers found");
    // TODO: add fingerprint argument in case of multiple signers
    return signers[0];
#else
    throw std::runtime_error(std::string(__func__) + ": Wallets with external signers require Boost::System library.");
#endif

}

bool ExternalSignerScriptPubKeyMan::DisplayAddress(const CScript scriptPubKey, const ExternalSigner &signer) const
{
#ifdef ENABLE_EXTERNAL_SIGNER
    // TODO: avoid the need to infer a descriptor from inside a descriptor wallet
    auto provider = GetSolvingProvider(scriptPubKey);
    auto descriptor = InferDescriptor(scriptPubKey, *provider);

    signer.displayAddress(descriptor->ToString());
    // TODO inspect result
    return true;
#else
    return false;
#endif
}

// If sign is true, transaction must previously have been filled
TransactionError ExternalSignerScriptPubKeyMan::FillPSBT(PartiallySignedTransaction& psbtx, int sighash_type, bool sign, bool bip32derivs) const
{
#ifdef ENABLE_EXTERNAL_SIGNER
    if (!sign) {
        return DescriptorScriptPubKeyMan::FillPSBT(psbtx, sighash_type, false, bip32derivs);
    }

    // Already complete if every input is now signed
    bool complete = true;
    for (const auto& input : psbtx.inputs) {
        // TODO: for multisig wallets, we should only care if all _our_ inputs are signed
        complete &= PSBTInputSigned(input);
    }
    if (complete) return TransactionError::OK;

    const std::string command = gArgs.GetArg("-signer", ""); // DEFAULT_EXTERNAL_SIGNER);
    if (command == "") throw std::runtime_error(std::string(__func__) + ": restart bitcoind with -signer=<cmd>");

    std::string chain = gArgs.GetChainName();
    const bool mainnet = chain == CBaseChainParams::MAIN;
    std::vector<ExternalSigner> signers;
    ExternalSigner::Enumerate(command, signers, mainnet);
    if (signers.empty()) return TransactionError::EXTERNAL_SIGNER_NOT_FOUND;
    // TODO: add fingerprint argument in case of multiple signers
    ExternalSigner signer = signers[0];

    std::string strFailReason;
    if( !signer.signTransaction(psbtx, strFailReason)) {
        // TODO: identify and/or log failure reason
        return TransactionError::EXTERNAL_SIGNER_FAILED;
    }
    FinalizePSBT(psbtx); // This won't work in a multisig setup
    return TransactionError::OK;
#else
    throw std::runtime_error(std::string(__func__) + ": No external signer support compiled");
#endif
}
