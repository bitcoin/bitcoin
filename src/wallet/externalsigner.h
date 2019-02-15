// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_EXTERNALSIGNER_H
#define BITCOIN_WALLET_EXTERNALSIGNER_H

#include <stdexcept>
#include <string>
#include <univalue.h>
#include <util/system.h>

class ExternalSignerException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

//! Enables interaction with an external signing device or service, such as a
//! a hardware wallet. See doc/external-signer.md
class ExternalSigner
{
private:
    //! The command which handles interaction with the external signer.
    std::string m_command;

public:
    //! @param[in] command      the command which handles interaction with the external signer
    //! @param[in] fingerprint  master key fingerprint of the signer
    //! @param[in] mainnet      Bitcoin mainnet or testnet
    ExternalSigner(const std::string& command, const std::string& fingerprint, bool mainnet);

    //! Master key fingerprint of the signer
    std::string m_fingerprint;

    //! Bitcoin mainnet or testnet
    bool m_mainnet;

    //! Two signers with the same master key fingerprint are considered the same
    friend inline bool operator==( const ExternalSigner &a, const ExternalSigner &b )
    {
         return a.m_fingerprint == b.m_fingerprint;
    }

#ifdef HAVE_BOOST_PROCESS
    //! Obtain a list of signers. Calls `<command> enumerate`.
    //! @param[in]              command the command which handles interaction with the external signer
    //! @param[in,out] signers  vector to which new signers (with a unique master key fingerprint) are added
    //! @param mainnet          Bitcoin mainnet or testnet
    //! @param[out]  UniValue   see doc/external-signer.md
    static UniValue Enumerate(const std::string& command, std::vector<ExternalSigner>& signers, bool mainnet = true);

    //! Get receive and change Descriptor(s) from device for a given account.
    //! Calls `<command> getdescriptors --account <account>`
    //! @param[in] account  which BIP32 account to use (e.g. `m/44'/0'/account'`)
    //! @param[out] UniValue see doc/external-signer.md
    UniValue getDescriptors(int account);

#endif
};

#endif // BITCOIN_WALLET_EXTERNALSIGNER_H
