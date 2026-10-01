// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/paymentdestination.h>

#include <addresstype.h>
#include <bech32.h>
#include <chainparams.h>
#include <common/bip352.h>
#include <key_io.h>
#include <script/script.h>
#include <util/expected.h>
#include <util/strencodings.h>

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

util::Expected<PaymentDestination, std::string> PaymentDestination::FromString(std::string_view str, std::vector<int>* error_locations)
{
    const CChainParams& params{Params()};
    const std::string address{str};

    if (ToLower(address.substr(0, params.SilentPaymentsHRP().size())) == params.SilentPaymentsHRP()) {
        auto sp_dest{bip352::DecodeSilentPaymentsAddress(address, params)};
        if (sp_dest) return PaymentDestination{std::move(*sp_dest)};
        if (bech32::Decode(address, bech32::CharLimit::SILENT_PAYMENTS).encoding == bech32::Encoding::INVALID) {
            // Perform Bech32 error location
            auto res{bech32::LocateErrors(address, bech32::CharLimit::SILENT_PAYMENTS)};
            if (error_locations) *error_locations = std::move(res.second);
            return util::Unexpected{std::move(res.first)};
        }
        return util::Unexpected{std::move(sp_dest.error())};
    }

    std::string error;
    CTxDestination dest{DecodeDestination(address, error, error_locations)};
    if (!IsValidDestination(dest)) return util::Unexpected{std::move(error)};
    return PaymentDestination{std::move(dest)};
}

std::optional<CScript> PaymentDestination::GetStaticScript() const
{
    if (const auto* dest{GetTxDestination()}) return GetScriptForDestination(*dest);
    return std::nullopt;
}

bool PaymentDestination::IsValid() const
{
    if (const auto* dest{GetTxDestination()}) {
        return !std::holds_alternative<CNoDestination>(*dest);
    }
    return IsSilentPayment();
}
