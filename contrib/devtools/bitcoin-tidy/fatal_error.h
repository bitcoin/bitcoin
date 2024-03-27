// Copyright (c) 2024-present Bitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FATAL_ERROR_CHECK_H
#define FATAL_ERROR_CHECK_H

#include <clang-tidy/ClangTidyCheck.h>

namespace bitcoin {

/**
 * @class FatalErrorCheck
 *
 * Warn about incorrect usage of util::Result<T, FatalError>. The check enforces
 * the following rules:
 *
 * 1. If a function calls another function with a util::Result<T, FatalCondition>
 * return type, its return type has to be util::Result<T, FatalCondition> too,
 * or it has to handle the value returned by the function with one of
 * "CheckFatal", "HandleFatalError", "UnwrapFatalError", "CheckFatalFailure".
 *
 * 2. In functions returning a util::Result<T, FatalCondition> a call to a
 * function returning a util::Result<T, FatalCondition> needs to propagate the
 * value by either:
 *  a) Returning it immediately
 *  b) Assigning it immediately to an existing result with .MoveMessages() or
 *     .Set()
 *  c) Eventually passing it as an arugment to a .MoveMessages() call
 */
class FatalErrorCheck final : public clang::tidy::ClangTidyCheck {
public:
  FatalErrorCheck(clang::StringRef Name, clang::tidy::ClangTidyContext *Context)
      : clang::tidy::ClangTidyCheck(Name, Context) {}

  bool isLanguageVersionSupported(
      const clang::LangOptions &LangOpts) const override {
    return LangOpts.CPlusPlus;
  }
  void registerMatchers(clang::ast_matchers::MatchFinder *Finder) override;
  void
  check(const clang::ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace bitcoin

#endif // FATAL_ERROR_CHECK_H
