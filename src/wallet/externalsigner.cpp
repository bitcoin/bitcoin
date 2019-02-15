// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/externalsigner.h>

ExternalSigner::ExternalSigner(const std::string& command, const std::string& fingerprint, bool mainnet): m_command(command), m_fingerprint(fingerprint), m_mainnet(mainnet) {}

#ifdef HAVE_BOOST_PROCESS

UniValue ExternalSigner::Enumerate(const std::string& command, std::vector<ExternalSigner>& signers, bool mainnet)
{
    // Call <command> enumerate
    const UniValue result = runCommandParseJSON(command + " enumerate");
    if (!result.isArray())
        throw ExternalSignerException(strprintf("'%s' received invalid response, expected array of signers", command));
    for (UniValue signer : result.getValues()) {
        const UniValue& fingerprint = find_value(signer, "fingerprint");
        if (result.isNull())
            throw ExternalSignerException(strprintf("'%s' received invalid response, missing signer fingerprint", command));
        std::string fingerprintStr = fingerprint.get_str();
        // Skip duplicate signer
        bool duplicate = false;
        for (ExternalSigner signer : signers) {
            if (signer.m_fingerprint.compare(fingerprintStr) == 0) duplicate = true;
        }
        if (duplicate) break;
        signers.push_back(ExternalSigner(command, fingerprintStr, mainnet));
    }
    return result;
}

#endif
