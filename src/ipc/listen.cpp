// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/url.h>
#include <interfaces/ipc.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/translation.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace {
util::Result<int64_t> ParseIntOption(std::string_view name, std::string_view value, int64_t min_val, int64_t max_val)
{
    if (value.empty()) return util::Error{Untranslated(strprintf("Missing value for %s option", name))};
    const auto parsed{ToIntegral<int64_t>(value)};
    if (!parsed) return util::Error{Untranslated(strprintf("Invalid %s value '%s'", name, value))};
    if (*parsed < min_val) return util::Error{Untranslated(strprintf("%s must be at least %d", name, min_val))};
    if (*parsed > max_val) return util::Error{Untranslated(strprintf("%s must be at most %d", name, max_val))};
    return *parsed;
}
} // namespace

namespace interfaces {
util::Result<ipc::ListenAddress> Ipc::parseListenAddress(std::string address)
{
    constexpr std::string_view UNIX_PREFIX{"unix:"};

    // Only Unix socket -ipcbind values support inline socket options. Leave
    // other address families untouched so ipc::Process can report address
    // scheme errors consistently with -ipcconnect.
    if (!address.starts_with(UNIX_PREFIX)) {
        return ipc::ListenAddress{.address = std::move(address)};
    }

    // Socket options follow the first comma:
    //   unix:,max-connections=8
    //   unix:/custom/path,max-connections=8
    // Commas in socket paths must be URL-encoded as %2C.
    //
    // Subsequent commas separate individual options.
    const size_t option_pos{address.find(',')};
    if (option_pos == std::string::npos) {
        return ipc::ListenAddress{.address = UrlDecode(address)};
    }

    const std::string_view address_view{address};

    ipc::ListenAddress listen_address{
        .address = UrlDecode(address_view.substr(0, option_pos)),
    };

    for (const std::string& option_str : util::SplitString(address_view.substr(option_pos + 1), ',')) {
        if (option_str.empty()) {
            return util::Error{Untranslated("Empty socket option")};
        }
        const size_t eq{option_str.find('=')};
        const std::string_view name{std::string_view{option_str}.substr(0, eq)};
        const std::string_view value{eq == std::string::npos ? std::string_view{} : std::string_view{option_str}.substr(eq + 1)};
        if (name == "max-connections") {
            auto parsed{ParseIntOption(name, value, 1, static_cast<int64_t>(ipc::MAX_CONNECTIONS))};
            if (!parsed) return util::Error{util::ErrorString(parsed)};
            listen_address.max_connections = static_cast<size_t>(*parsed);
        } else {
            return util::Error{Untranslated(strprintf("Unknown socket option '%s'", name))};
        }
    }

    return listen_address;
}
} // namespace interfaces
