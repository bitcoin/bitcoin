// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <util/result.h>

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <util/translation.h>

namespace util {
namespace detail {
bilingual_str JoinMessages(const std::vector<bilingual_str>& errors, const std::vector<bilingual_str>& warnings)
{
    bilingual_str result;
    for (const auto& messages : {errors, warnings}) {
        for (const auto& message : messages) {
            if (!result.empty()) result += Untranslated(" ");
            result += message;
        }
    }
    return result;
}

void MoveMessages(std::vector<bilingual_str>& src, std::vector<bilingual_str>& dest)
{
    dest.insert(dest.end(), std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()));
    src.clear();
}
} // namespace detail
} // namespace util
