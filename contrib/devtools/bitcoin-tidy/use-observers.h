#pragma once

#include <clang-tidy/ClangTidyCheck.h>

class UseObservers : public clang::tidy::ClangTidyCheck
{
public:
  using ClangTidyCheck::ClangTidyCheck;

private:
  void registerMatchers(clang::ast_matchers::MatchFinder* Finder) override;
  void check(clang::ast_matchers::MatchFinder::MatchResult const& Res) override;
};
