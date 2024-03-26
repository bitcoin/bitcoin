// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <util/result.h>

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <util/translation.h>

namespace util {
namespace detail {
bilingual_str JoinMessages(const Messages& messages)
{
    bilingual_str result;
    for (const auto& messages : {messages.errors, messages.warnings}) {
        for (const auto& message : messages) {
            if (!result.empty()) result += Untranslated(" ");
            result += message;
        }
    }
    return result;
}
} // namespace detail

void ResultTraits<Messages>::Update(Messages& dst, Messages& src) {
    dst.errors.insert(dst.errors.end(), std::make_move_iterator(src.errors.begin()), std::make_move_iterator(src.errors.end()));
    dst.warnings.insert(dst.warnings.end(), std::make_move_iterator(src.warnings.begin()), std::make_move_iterator(src.warnings.end()));
    src.errors.clear();
    src.warnings.clear();
}
} // namespace util
