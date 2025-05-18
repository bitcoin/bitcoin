// Copyright (c) 2023 Tortoisecoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NONTRIVIAL_THREADLOCAL_CHECK_H
#define NONTRIVIAL_THREADLOCAL_CHECK_H

#include <clang-tidy/ClangTidyCheck.h>

namespace tortoisecoin {

// Warn about any thread_local variable with a non-trivial destructor.
class NonTrivialThreadLocal final : public clang::tidy::ClangTidyCheck
{
public:
    NonTrivialThreadLocal(clang::StringRef Name, clang::tidy::ClangTidyContext* Context)
        : clang::tidy::ClangTidyCheck(Name, Context) {}

    bool isLanguageVersionSupported(const clang::LangOptions& LangOpts) const override
    {
        return LangOpts.CPlusPlus;
    }
    void registerMatchers(clang::ast_matchers::MatchFinder* Finder) override;
    void check(const clang::ast_matchers::MatchFinder::MatchResult& Result) override;
};

} // namespace tortoisecoin

#endif // NONTRIVIAL_THREADLOCAL_CHECK_H
