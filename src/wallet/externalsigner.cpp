// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <psbt.h>
#include <util/strencodings.h>
#include <util/system.h>
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

UniValue ExternalSigner::displayAddress(const std::string& descriptor)
{
    return runCommandParseJSON(m_command + " --fingerprint \"" + m_fingerprint + "\"" + (m_mainnet ? "" : " --testnet ") + " displayaddress --desc \"" + descriptor + "\"");
}

UniValue ExternalSigner::getDescriptors(int account)
{
    return runCommandParseJSON(m_command + " --fingerprint \"" + m_fingerprint + "\"" + (m_mainnet ? "" : " --testnet ") + " getdescriptors --account " + std::to_string(account));
}

bool ExternalSigner::signTransaction(PartiallySignedTransaction& psbtx, std::string& error)
{
    // Check if signer fingerpint matches any input master key fingerprint
    bool match = false;
    for (unsigned int i = 0; i < psbtx.inputs.size(); ++i) {
        const PSBTInput& input = psbtx.inputs[i];
        for (auto entry : input.hd_keypaths) {
            if (m_fingerprint == strprintf("%08x", ReadBE32(entry.second.fingerprint))) match = true;
        }
    }
    if (!match) {
        error = "Signer fingerprint does not match any of the inputs";
        return false;
    }

    // Serialize the PSBT
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << psbtx;

    std::string command = m_command + " --stdin --fingerprint \"" + m_fingerprint + "\"" + (m_mainnet ? "" : " --testnet ");
    std::string stdinStr = "signtx \"" + EncodeBase64(ssTx.str()) + "\"";

    const UniValue signer_result = runCommandParseJSON(command, stdinStr);

    if (!find_value(signer_result, "psbt").isStr()) {
        error = "Unexpected result from signer";
        return false;
    }

    PartiallySignedTransaction signer_psbtx;
    std::string signer_psbt_error;
    if (!DecodeBase64PSBT(signer_psbtx, find_value(signer_result, "psbt").get_str(), signer_psbt_error)) {
        error = strprintf("TX decode failed %s", signer_psbt_error);
        return false;
    }

    psbtx = signer_psbtx;

    return true;
}

#endif
