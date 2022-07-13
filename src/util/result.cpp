// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <util/result.h>
#include <util/string.h>

namespace util {
bilingual_str ErrorDescription(const Result<void>& result)
{
    return Join(result.GetErrors(), Untranslated(", "));
}
} // namespace util

