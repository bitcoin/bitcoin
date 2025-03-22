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
    std::string Format(util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args) const
    {
        return m_display_name + " " + BCLog::Source::Format(fmt, args...);
    }
};
} // namespace wallet

#endif // BITCOIN_WALLET_LOGGING_H
