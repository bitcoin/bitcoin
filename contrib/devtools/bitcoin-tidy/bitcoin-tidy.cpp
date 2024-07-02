// Copyright (c) 2023 Bitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "logprintf.h"
#include "nontrivial-threadlocal.h"

#include <clang-tidy/ClangTidyModule.h>
#include <clang-tidy/ClangTidyModuleRegistry.h>

class BitcoinModule final : public clang::tidy::ClangTidyModule
{
public:
    void addCheckFactories(clang::tidy::ClangTidyCheckFactories& CheckFactories) override
    {
        CheckFactories.registerCheck<bitcoin::LogPrintfCheck>("bitcoin-unterminated-logprintf");
        CheckFactories.registerCheck<bitcoin::NonTrivialThreadLocal>("bitcoin-nontrivial-threadlocal");
    }
};

static clang::tidy::ClangTidyModuleRegistry::Add<BitcoinModule>
    X("bitcoin-module", "Adds bitcoin checks.");

volatile int BitcoinModuleAnchorSource = 0;
