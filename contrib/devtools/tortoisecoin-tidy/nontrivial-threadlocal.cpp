// Copyright (c) 2023 Tortoisecoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "nontrivial-threadlocal.h"

#include <clang/AST/ASTContext.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>


// Copied from clang-tidy's UnusedRaiiCheck
namespace {
AST_MATCHER(clang::CXXRecordDecl, hasNonTrivialDestructor) {
    // TODO: If the dtor is there but empty we don't want to warn either.
    return Node.hasDefinition() && Node.hasNonTrivialDestructor();
}
} // namespace

namespace tortoisecoin {

void NonTrivialThreadLocal::registerMatchers(clang::ast_matchers::MatchFinder* finder)
{
    using namespace clang::ast_matchers;

    /*
      thread_local std::string foo;
    */

    finder->addMatcher(
        varDecl(
            hasThreadStorageDuration(),
            hasType(hasCanonicalType(recordType(hasDeclaration(cxxRecordDecl(hasNonTrivialDestructor())))))
        ).bind("nontrivial_threadlocal"),
        this);
}

void NonTrivialThreadLocal::check(const clang::ast_matchers::MatchFinder::MatchResult& Result)
{
    if (const clang::VarDecl* var = Result.Nodes.getNodeAs<clang::VarDecl>("nontrivial_threadlocal")) {
        const auto user_diag = diag(var->getBeginLoc(), "Variable with non-trivial destructor cannot be thread_local.");
    }
}

} // namespace tortoisecoin
