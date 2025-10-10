// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/checks.h>

#include <dbwrapper.h>
#include <random.h>
#include <util/result.h>
#include <util/translation.h>

#include <memory>

namespace kernel {

bool Clang_IndVarSimplify_Bug_SanityCheck() {
    // See https://github.com/llvm/llvm-project/issues/96267
    const char s[] = {0, 0x75};
    signed int last = 0xff;
    for (const char *it = s; it < &s[2]; ++it) {
        if (*it <= 0x4e) {
        } else if (*it == 0x75 && last <= 0x4e) {
            return true;
        }
        last = *it;
    }
    return false;
}

util::Result<void> SanityChecks(const Context&)
{
    if (auto result{dbwrapper_SanityCheck()}; !result) {
        return util::Error{util::ErrorString(result) + Untranslated("\nDatabase sanity check failure. Aborting.")};
    }

    if (!Clang_IndVarSimplify_Bug_SanityCheck()) {
        return util::Error{Untranslated("Compiler optimization sanity check failure. Aborting.")};
    }

    if (!Random_SanityCheck()) {
        return util::Error{Untranslated("OS cryptographic RNG sanity check failure. Aborting.")};
    }

    return {};
}

}
