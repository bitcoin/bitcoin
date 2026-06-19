#include "observers-base.h"

#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace clang::ast_matchers;

void ObserversBase::registerMatchers(MatchFinder* Finder)
{
  Finder->addMatcher(
    memberExpr(
      unless(hasAncestor(functionDecl(anyOf(isImplicit(), isDefaulted())))),
      member(fieldDecl(hasParent(cxxRecordDecl(hasName(ClassName))))))
      .bind("member"),
    this);
}

void ObserversBase::check(MatchFinder::MatchResult const& Result)
{
  auto const* ME = Result.Nodes.getNodeAs<MemberExpr>("member");
  if (!ME || ME->isImplicitAccess()) {
    return;
  }

  SourceLocation Loc = ME->getMemberLoc();
  if (Loc.isMacroID()) {
    SourceManager const& SM = *Result.SourceManager;
    LangOptions const& LO = Result.Context->getLangOpts();

    StringRef MacroName = clang::Lexer::getImmediateMacroName(Loc, SM, LO);
    if (MacroName == "READWRITE" || MacroName == "VARINT") {
      return;
    }
  }

  auto const* FD = dyn_cast<FieldDecl>(ME->getMemberDecl());
  if (!FD) {
    return;
  }

  auto It = MemberToAccessor.find(FD->getNameAsString());
  if (It == MemberToAccessor.end()) {
    return;
  }

  std::string const& Accessor = It->second;

  // Skip obvious write accesses.
  auto Parents = Result.Context->getParents(*ME);
  if (!Parents.empty()) {
    if (auto const* CO = Parents[0].get<CXXOperatorCallExpr>()) {
      if (CO->getOperator() == OO_Equal) {
        return;
      }
    }

    if (auto const* BO = Parents[0].get<BinaryOperator>()) {
      if (BO->isAssignmentOp() && BO->getLHS() == ME) {
        return;
      }
    }

    if (auto const* UO = Parents[0].get<UnaryOperator>()) {
      if (UO->isIncrementDecrementOp() || UO->getOpcode() == UO_AddrOf) {
        return;
      }
    }
  }

  diag(ME->getMemberLoc(), "replace direct member access with accessor")
    << FixItHint::CreateReplacement(ME->getMemberLoc(), Accessor);
}
