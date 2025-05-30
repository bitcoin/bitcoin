// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/messages.h>

#include <common/types.h>
#include <policy/fees.h>
#include <node/types.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/translation.h>

#include <cassert>
#include <map>
#include <string>
#include <utility>
#include <vector>

using node::TransactionError;
using util::Join;

namespace common {
std::string StringForFeeReason(FeeReason reason)
{
    static const std::map<FeeReason, std::string> fee_reason_strings = {
        {FeeReason::NONE, "None"},
        {FeeReason::HALF_ESTIMATE, "Half Target 60% Threshold"},
        {FeeReason::FULL_ESTIMATE, "Target 85% Threshold"},
        {FeeReason::DOUBLE_ESTIMATE, "Double Target 95% Threshold"},
        {FeeReason::CONSERVATIVE, "Conservative Double Target longer horizon"},
        {FeeReason::MEMPOOL_MIN, "Mempool Min Fee"},
        {FeeReason::PAYTXFEE, "PayTxFee set"},
        {FeeReason::FALLBACK, "Fallback fee"},
        {FeeReason::REQUIRED, "Minimum Required Fee"},
    };
    auto reason_string = fee_reason_strings.find(reason);

    if (reason_string == fee_reason_strings.end()) return "Unknown";

    return reason_string->second;
}

const std::vector<std::pair<std::string, FeeEstimateMode>>& FeeModeMap()
{
    static const std::vector<std::pair<std::string, FeeEstimateMode>> FEE_MODES = {
        {"unset", FeeEstimateMode::UNSET},
        {"economical", FeeEstimateMode::ECONOMICAL},
        {"conservative", FeeEstimateMode::CONSERVATIVE},
    };
    return FEE_MODES;
}

std::string FeeModeInfo(const std::pair<std::string, FeeEstimateMode>& mode, std::string& default_info)
{
    switch (mode.second) {
        case FeeEstimateMode::UNSET:
            return strprintf("%s means no mode set (%s). \n", mode.first, default_info);
        case FeeEstimateMode::ECONOMICAL:
            return strprintf("%s estimates use a shorter time horizon, making them more\n"
                   "responsive to short-term drops in the prevailing fee market. This mode\n"
                   "potentially returns a lower fee rate estimate.\n", mode.first);
        case FeeEstimateMode::CONSERVATIVE:
            return strprintf("%s estimates use a longer time horizon, making them\n"
                   "less responsive to short-term drops in the prevailing fee market. This mode\n"
                   "potentially returns a higher fee rate estimate.\n", mode.first);
        default:
            // Other modes apart from the ones handled are fee rate units; they should not be clarified.
            assert(false);
    }
}

std::string FeeModesDetail(std::string default_info)
{
    std::string info;
    for (const auto& fee_mode : FeeModeMap()) {
        info += FeeModeInfo(fee_mode, default_info);
    }
    return strprintf("%s \n%s", FeeModes(", "), info);
}

std::string FeeModes(const std::string& delimiter)
{
    return Join(FeeModeMap(), delimiter, [&](const std::pair<std::string, FeeEstimateMode>& i) { return i.first; });
}

std::string InvalidEstimateModeErrorMessage()
{
    return "Invalid estimate_mode parameter, must be one of: \"" + FeeModes("\", \"") + "\"";
}

bool FeeModeFromString(const std::string& mode_string, FeeEstimateMode& fee_estimate_mode)
{
    auto searchkey = ToUpper(mode_string);
    for (const auto& pair : FeeModeMap()) {
        if (ToUpper(pair.first) == searchkey) {
            fee_estimate_mode = pair.second;
            return true;
        }
    }
    return false;
}

bilingual_str PSBTErrorString(PSBTError err)
{
    switch (err) {
        case PSBTError::MISSING_INPUTS:
            return Untranslated("Inputs missing or spent");
        case PSBTError::SIGHASH_MISMATCH:
            return Untranslated("Specified sighash value does not match value stored in PSBT");
        case PSBTError::EXTERNAL_SIGNER_NOT_FOUND:
            return Untranslated("External signer not found");
        case PSBTError::EXTERNAL_SIGNER_FAILED:
            return Untranslated("External signer failed to sign");
        case PSBTError::UNSUPPORTED:
            return Untranslated("Signer does not support PSBT");
        case PSBTError::INCOMPLETE:
            return Untranslated("Input needs additional signatures or other data");
        case PSBTError::OK:
            return Untranslated("No errors");
        // no default case, so the compiler can warn about missing cases
    }
    assert(false);
}

bilingual_str TransactionErrorString(const TransactionError err)
{
    switch (err) {
        case TransactionError::OK:
            return Untranslated("No error");
        case TransactionError::MISSING_INPUTS:
            return Untranslated("Inputs missing or spent");
        case TransactionError::ALREADY_IN_UTXO_SET:
            return Untranslated("Transaction outputs already in utxo set");
        case TransactionError::MEMPOOL_REJECTED:
            return Untranslated("Transaction rejected by mempool");
        case TransactionError::MEMPOOL_ERROR:
            return Untranslated("Mempool internal error");
        case TransactionError::MAX_FEE_EXCEEDED:
            return Untranslated("Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)");
        case TransactionError::MAX_BURN_EXCEEDED:
            return Untranslated("Unspendable output exceeds maximum configured by user (maxburnamount)");
        case TransactionError::INVALID_PACKAGE:
            return Untranslated("Transaction rejected due to invalid package");
        // no default case, so the compiler can warn about missing cases
    }
    assert(false);
}

bilingual_str ResolveErrMsg(const std::string& optname, const std::string& strBind)
{
    return strprintf(_("Cannot resolve -%s address: '%s'"), optname, strBind);
}

bilingual_str InvalidPortErrMsg(const std::string& optname, const std::string& invalid_value)
{
    return strprintf(_("Invalid port specified in %s: '%s'"), optname, invalid_value);
}

bilingual_str AmountHighWarn(const std::string& optname)
{
    return strprintf(_("%s is set very high!"), optname);
}

bilingual_str AmountErrMsg(const std::string& optname, const std::string& strValue)
{
    return strprintf(_("Invalid amount for -%s=<amount>: '%s'"), optname, strValue);
}
} // namespace common
