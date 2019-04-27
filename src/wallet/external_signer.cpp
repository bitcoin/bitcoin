// Copyright (c) 2018-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/external_signer.h>
#include <util/system.h>

ExternalSigner::ExternalSigner(const std::string& command, const std::string& fingerprint): m_command(command), m_fingerprint(fingerprint) {}
