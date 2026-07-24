// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/ipc.h>
#include <util/string.h>
#include <util/translation.h>

#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

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
    //
    // Subsequent commas separate individual options.
    const size_t option_pos{address.find(',')};
    if (option_pos == std::string::npos) {
        return ipc::ListenAddress{.address = std::move(address)};
    }

    const std::string_view address_view{address};
    const std::string_view options_view{
        address_view.substr(option_pos + 1)};

    ipc::ListenAddress listen_address{
        .address = address.substr(0, option_pos),
    };

    size_t next{0};
    while (next <= options_view.size()) {
        const size_t end{options_view.find(',', next)};
        const std::string_view option{
            options_view.substr(next, end - next)};

        if (option.empty()) {
            return util::Error{Untranslated("Empty socket option")};
        }

        const size_t equals{option.find('=')};
        const std::string_view name{option.substr(0, equals)};
        const std::string_view value{
            equals == std::string_view::npos ? std::string_view{} : option.substr(equals + 1)};

        if (name == "max-connections") {
            if (value.empty()) {
                return util::Error{Untranslated("Missing value for max-connections option")};
            }

            int64_t parsed_limit{-1};
            const auto [last, ec]{std::from_chars(value.data(), value.data() + value.size(), parsed_limit)};

            if (ec != std::errc{} ||
                last != value.data() + value.size()) {
                return util::Error{Untranslated("Invalid max-connections value '" + std::string{value} + "'")};
            }
            if (parsed_limit < 1) {
                return util::Error{Untranslated("max-connections must be at least 1")};
            }
            if (parsed_limit > static_cast<int64_t>(ipc::MAX_CONNECTIONS)) {
                return util::Error{Untranslated("max-connections must be at most " + util::ToString(ipc::MAX_CONNECTIONS))};
            }

            listen_address.max_connections = static_cast<size_t>(parsed_limit);
        } else {
            return util::Error{Untranslated("Unknown socket option '" + std::string{name} + "'")};
        }

        if (end == std::string_view::npos) break;
        next = end + 1;
    }

    return listen_address;
}
} // namespace interfaces
