#pragma once

#include <clang-tidy/ClangTidyCheck.h>

class ObserversBase : public clang::tidy::ClangTidyCheck
{
public:
  using ClangTidyCheck::ClangTidyCheck;

private:
  void registerMatchers(clang::ast_matchers::MatchFinder* Finder) override;
  void check(clang::ast_matchers::MatchFinder::MatchResult const& Res) override;

protected:
  std::string ClassName;
  llvm::StringMap<std::string> MemberToAccessor;
};
