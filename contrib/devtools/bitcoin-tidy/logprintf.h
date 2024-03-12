// Copyright (c) 2023 Bitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LOGPRINTF_CHECK_H
#define LOGPRINTF_CHECK_H

#include <clang-tidy/ClangTidyCheck.h>

namespace bitcoin {

// Warn about any use of LogInfo that does not end with a newline.
class LogPrintfCheck final : public clang::tidy::ClangTidyCheck
{
public:
    LogPrintfCheck(clang::StringRef Name, clang::tidy::ClangTidyContext* Context)
        : clang::tidy::ClangTidyCheck(Name, Context) {}

    bool isLanguageVersionSupported(const clang::LangOptions& LangOpts) const override
    {
        return LangOpts.CPlusPlus;
    }
    void registerMatchers(clang::ast_matchers::MatchFinder* Finder) override;
    void check(const clang::ast_matchers::MatchFinder::MatchResult& Result) override;
};

} // namespace bitcoin

#endif // LOGPRINTF_CHECK_H
