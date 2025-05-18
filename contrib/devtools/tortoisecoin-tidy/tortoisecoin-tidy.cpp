// Copyright (c) 2023 Tortoisecoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "logprintf.h"
#include "nontrivial-threadlocal.h"

#include <clang-tidy/ClangTidyModule.h>
#include <clang-tidy/ClangTidyModuleRegistry.h>

class TortoisecoinModule final : public clang::tidy::ClangTidyModule
{
public:
    void addCheckFactories(clang::tidy::ClangTidyCheckFactories& CheckFactories) override
    {
        CheckFactories.registerCheck<tortoisecoin::LogPrintfCheck>("tortoisecoin-unterminated-logprintf");
        CheckFactories.registerCheck<tortoisecoin::NonTrivialThreadLocal>("tortoisecoin-nontrivial-threadlocal");
    }
};

static clang::tidy::ClangTidyModuleRegistry::Add<TortoisecoinModule>
    X("tortoisecoin-module", "Adds tortoisecoin checks.");

volatile int TortoisecoinModuleAnchorSource = 0;
