// Copyright (c) 2018-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_EXTERNAL_SIGNER_H
#define BITCOIN_WALLET_EXTERNAL_SIGNER_H

#include <stdexcept>
#include <string>
#include <univalue.h>
#include <util/system.h>

class ExternalSignerException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

//! Enables interaction with an external signing device or service, such as
//! a hardware wallet. See doc/external-signer.md
class ExternalSigner
{
private:
    //! The command which handles interaction with the external signer.
    std::string m_command;

public:
    //! @param[in] command      the command which handles interaction with the external signer
    //! @param[in] fingerprint  master key fingerprint of the signer
    //! @param[in] chain        "main", "test", "regtest" or "signet"
    //! @param[in] name         device name
    ExternalSigner(const std::string& command, const std::string& fingerprint, std::string chain, std::string name);

    //! Master key fingerprint of the signer
    std::string m_fingerprint;

    //! Bitcoin mainnet, testnet, etc
    std::string m_chain;

    //! Name of signer
    std::string m_name;

    const std::string NetworkArg() const;

#ifdef ENABLE_EXTERNAL_SIGNER
    //! Obtain a list of signers. Calls `<command> enumerate`.
    //! @param[in]              command the command which handles interaction with the external signer
    //! @param[in,out] signers  vector to which new signers (with a unique master key fingerprint) are added
    //! @param chain            "main", "test", "regtest" or "signet"
    //! @param[out] success     Boolean
    static bool Enumerate(const std::string& command, std::vector<ExternalSigner>& signers, std::string chain, bool ignore_errors = false);

#endif
};

#endif // BITCOIN_WALLET_EXTERNAL_SIGNER_H
