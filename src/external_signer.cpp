// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <external_signer.h>

#include <chainparams.h>
#include <common/run_command.h>
#include <core_io.h>
#include <psbt.h>
#include <util/strencodings.h>
#include <util/subprocess.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

ExternalSigner::ExternalSigner(std::vector<std::string> command, std::string chain, std::string fingerprint, std::string name)
    : m_command{std::move(command)}, m_chain{std::move(chain)}, m_fingerprint{std::move(fingerprint)}, m_name{std::move(name)} {}

std::vector<std::string> ExternalSigner::NetworkArg() const
{
    return {"--chain", m_chain};
}

bool ExternalSigner::Enumerate(const std::string& command, std::vector<ExternalSigner>& signers, const std::string& chain)
{
    // Call <command> enumerate
    std::vector<std::string> cmd_args = Cat(subprocess::util::split(command), {"enumerate"});

    const UniValue result = RunCommandParseJSON(cmd_args, "");
    if (!result.isArray()) {
        throw std::runtime_error(strprintf("'%s' received invalid response, expected array of signers", command));
    }
    for (const UniValue& signer : result.getValues()) {
        // Check for error
        const UniValue& error = signer.find_value("error");
        if (!error.isNull()) {
            if (!error.isStr()) {
                throw std::runtime_error(strprintf("'%s' error", command));
            }
            throw std::runtime_error(strprintf("'%s' error: %s", command, error.getValStr()));
        }
        // Check if fingerprint is present
        const UniValue& fingerprint = signer.find_value("fingerprint");
        if (fingerprint.isNull()) {
            throw std::runtime_error(strprintf("'%s' received invalid response, missing signer fingerprint", command));
        }
        const std::string& fingerprintStr{fingerprint.get_str()};
        if (fingerprintStr.size() != 8 || !IsHex(fingerprintStr)) {
            throw std::runtime_error(strprintf("'%s' received invalid fingerprint, must be 8 hex characters", command));
        }
        // Skip duplicate signer
        bool duplicate = false;
        for (const ExternalSigner& signer : signers) {
            if (signer.m_fingerprint.compare(fingerprintStr) == 0) duplicate = true;
        }
        if (duplicate) continue;
        std::string name;
        const UniValue& model_field = signer.find_value("model");
        if (model_field.isStr() && model_field.getValStr() != "") {
            name += model_field.getValStr();
        }
        signers.emplace_back(subprocess::util::split(command), chain, fingerprintStr, name);
    }
    return true;
}

UniValue ExternalSigner::DisplayAddress(const std::string& descriptor) const
{
    return RunCommandParseJSON(Cat(m_command, Cat(Cat({"--fingerprint", m_fingerprint}, NetworkArg()), {"displayaddress", "--desc", descriptor})), "");
}

UniValue ExternalSigner::GetDescriptors(const int account)
{
    return RunCommandParseJSON(Cat(m_command, Cat(Cat({"--fingerprint", m_fingerprint}, NetworkArg()), {"getdescriptors", "--account", strprintf("%d", account)})), "");
}

static std::string CheckSignerPSBTOutputs(
    const PartiallySignedTransaction& original,
    const PartiallySignedTransaction& signed_psbt)
{
    if (original.outputs.size() != signed_psbt.outputs.size()) {
        return strprintf("Signer modified output count: %d -> %d", original.outputs.size(), signed_psbt.outputs.size());
    }
    for (size_t i = 0; i < original.outputs.size(); ++i) {
        if (original.outputs[i].amount != signed_psbt.outputs[i].amount) {
            return strprintf("Signer modified output %d amount", i);
        }
        if (original.outputs[i].script != signed_psbt.outputs[i].script) {
            return strprintf("Signer modified output %d scriptPubKey", i);
        }
    }

    // SIGHASH_NONE (2) and SIGHASH_SINGLE (3) do not commit to all outputs.
    // SIGHASH_DEFAULT (0, taproot implicit ALL) and SIGHASH_ALL (1) are safe.
    // ANYONECANPAY (0x80) can be combined with ALL and remains output-binding.
    auto is_safe_sighash = [](int sh) -> bool {
        const int base = sh & ~SIGHASH_ANYONECANPAY;
        return base == SIGHASH_DEFAULT || base == SIGHASH_ALL;
    };

    for (auto& input : signed_psbt.inputs) {
        if (input.sighash_type.has_value() &&
            !is_safe_sighash(*input.sighash_type)) {
            return strprintf("Signer used unsafe sighash type in one of the inputs");
        }
        for (const auto& [_, sigpair] : input.partial_sigs) {
            const std::vector<unsigned char>& sig = sigpair.second;
            if (!sig.empty() && !is_safe_sighash(sig.back())) {
                return strprintf("Signer used unsafe sighash type in one of the inputs");
            }
        }
        // 64-byte taproot sig implies SIGHASH_DEFAULT (safe); 65-byte carries
        // an explicit sighash flag in the last byte.
        if (input.m_tap_key_sig.size() == 65 &&
            !is_safe_sighash(input.m_tap_key_sig.back())) {
            return strprintf("Signer used an unsafe sighash type in taproot keypath sig");
        }
        for (const auto& [_, sig] : input.m_tap_script_sigs) {
            if (sig.size() == 65 && !is_safe_sighash(sig.back())) {
                return strprintf("Signer used an unsafe sighash type in taproot scriptpath sig");
            }
        }
    }

    return {};
}

bool ExternalSigner::SignTransaction(PartiallySignedTransaction& psbtx, std::string& error)
{
    // Serialize the PSBT
    DataStream ssTx{};
    ssTx << psbtx;
    // parse ExternalSigner master fingerprint
    std::vector<unsigned char> parsed_m_fingerprint = ParseHex(m_fingerprint);
    // Check if signer fingerprint matches any input master key fingerprint
    auto matches_signer_fingerprint = [&](const PSBTInput& input) {
        for (const auto& entry : input.hd_keypaths) {
            if (std::ranges::equal(parsed_m_fingerprint, entry.second.fingerprint)) return true;
        }
        for (const auto& entry : input.m_tap_bip32_paths) {
            if (std::ranges::equal(parsed_m_fingerprint, entry.second.second.fingerprint)) return true;
        }
        return false;
    };

    if (!std::any_of(psbtx.inputs.begin(), psbtx.inputs.end(), matches_signer_fingerprint)) {
        error = "Signer fingerprint " + m_fingerprint + " does not match any of the inputs:\n" + EncodeBase64(ssTx.str());
        return false;
    }

    const std::vector<std::string> command = Cat(m_command, Cat({"--stdin", "--fingerprint", m_fingerprint}, NetworkArg()));
    const std::string stdinStr = "signtx " + EncodeBase64(ssTx.str());

    const UniValue signer_result = RunCommandParseJSON(command, stdinStr);

    if (signer_result.find_value("error").isStr()) {
        error = signer_result.find_value("error").get_str();
        return false;
    }

    if (!signer_result.find_value("psbt").isStr()) {
        error = "Unexpected result from signer";
        return false;
    }

    util::Result<PartiallySignedTransaction> signer_psbtx = DecodeBase64PSBT(signer_result.find_value("psbt").get_str());
    if (!signer_psbtx) {
        error = strprintf("TX decode failed %s", util::ErrorString(signer_psbtx).original);
        return false;
    }

    const auto invariant_error{CheckSignerPSBTOutputs(psbtx, *signer_psbtx)};
    if (!invariant_error.empty()) {
        error = invariant_error;
        return false;
    }

    psbtx = *signer_psbtx;

    return true;
}
