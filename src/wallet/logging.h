// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_LOGGING_H
#define BITCOIN_WALLET_LOGGING_H

#include <logging.h>

#include <string>

namespace wallet {
/** Prepends the wallet name in logging output to ease debugging in multi-wallet use cases */
class WalletLogSource : public BCLog::Source
{
    std::string m_display_name;

public:
    WalletLogSource(std::string display_name) : BCLog::Source{BCLog::ALL}, m_display_name{std::move(display_name)} {}

    template <typename... Args>
    std::string Format(const char* fmt, const Args&... args) const
    {
        return BCLog::Source::Format(("%s " + std::string{fmt}).c_str(), m_display_name, args...);
    }
};
} // namespace wallet

#endif // BITCOIN_WALLET_LOGGING_H
