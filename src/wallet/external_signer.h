// Copyright (c) 2018-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_EXTERNAL_SIGNER_H
#define BITCOIN_WALLET_EXTERNAL_SIGNER_H

#include <stdexcept>
#include <string>
#include <univalue.h>

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
    ExternalSigner(const std::string& command, const std::string& fingerprint);

    //! Master key fingerprint of the signer
    std::string m_fingerprint;
};

#endif // BITCOIN_WALLET_EXTERNAL_SIGNER_H
