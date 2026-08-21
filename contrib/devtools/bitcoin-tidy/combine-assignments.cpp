#include "combine-assignments.h"

#include <clang/AST/ExprObjC.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>

#include <cstddef>
#include <optional>

namespace {

using namespace clang;
using namespace clang::ast_matchers;

struct AnnotationInfo {
  unsigned Index;
  unsigned Total;
};

std::optional<AnnotationInfo> GetAnnotationInfo(FieldDecl const *FD) {
  auto const *Attr = FD->getAttr<AnnotateAttr>();
  if (!Attr)
    return std::nullopt;

  StringRef Annotation = Attr->getAnnotation();
  if (!Annotation.consume_front("constructor-argument:"))
    return std::nullopt;

  auto [IdxStr, TotalStr] = Annotation.split('/');

  unsigned Idx, Total;
  if (IdxStr.getAsInteger(10, Idx) || TotalStr.getAsInteger(10, Total))
    return std::nullopt; // parse error

  if (Idx == 0 || Idx > Total)
    return std::nullopt; // 1-based index out of range

  return AnnotationInfo{Idx - 1, Total}; // convert to 0-based
}

struct AssignmentInfo {
  MemberExpr const *ME = nullptr;
  Expr const *Base = nullptr;
  Expr const *RHS = nullptr;
  unsigned Index = 0;
  unsigned Total = 0;
};

// Given an assignment's LHS and RHS expressions, returns the annotated
// field assignment info if LHS is base.member with an annotated field.
std::optional<AssignmentInfo> ExtractAssignment(Expr const *LHS,
                                                Expr const *RHS) {
  auto *ME = dyn_cast<MemberExpr>(LHS->IgnoreParenImpCasts());
  if (!ME)
    return std::nullopt;

  auto *FD = dyn_cast<FieldDecl>(ME->getMemberDecl());
  if (!FD)
    return std::nullopt;

  auto Info = GetAnnotationInfo(FD);
  if (!Info)
    return std::nullopt;

  return AssignmentInfo{ME, ME->getBase()->IgnoreParenImpCasts(), RHS,
                        Info->Index, Info->Total};
}

std::optional<AssignmentInfo> GetAssignment(Stmt const *S) {
  // Built-in assignment: base.member = rhs
  if (auto *BO = dyn_cast<BinaryOperator>(S))
    if (BO->isAssignmentOp())
      return ExtractAssignment(BO->getLHS(), BO->getRHS());

  // Strip a surrounding expression statement.
  if (auto *E = dyn_cast<ExprWithCleanups>(S))
    S = E->getSubExpr();

  // Overloaded operator=: base.member = rhs
  if (auto *Op = dyn_cast<CXXOperatorCallExpr>(S))
    if (Op->getOperator() == OO_Equal)
      return ExtractAssignment(Op->getArg(0), Op->getArg(1));

  return std::nullopt;
}

/// Convenience: get the source text of an expression.
std::string GetText(Expr const *E, SourceManager const &SM,
                    LangOptions const &LO) {
  CharSourceRange Range = CharSourceRange::getTokenRange(E->getSourceRange());
  return Lexer::getSourceText(Range, SM, LO).str();
}

/// Infer the constructor name (class name) from the base expression's type.
/// Returns an empty string if the type is not a CXXRecordDecl.
std::string GetClassName(Expr const *Base) {
  auto const *Record = Base->getType().getCanonicalType()->getAsCXXRecordDecl();
  return Record ? Record->getName().str() : std::string{};
}

} // namespace

void CombineAssignments::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(compoundStmt().bind("body"), this);
}

void CombineAssignments::check(MatchFinder::MatchResult const &Result) {
  auto const *Body = Result.Nodes.getNodeAs<CompoundStmt>("body");
  if (!Body)
    return;

  auto &SM = *Result.SourceManager;
  auto const &LO = Result.Context->getLangOpts();
  llvm::SmallVector<Stmt const *, 16> Stmts(Body->body());

  for (size_t i = 0; i < Stmts.size(); ++i) {
    std::vector<std::optional<AssignmentInfo>> Members;
    auto AddMember = [&](AssignmentInfo Info) {
      if (Members.size() < Info.Total)
        Members.resize(Info.Total);
      Members[Info.Index] = Info;
    };

    auto First = GetAssignment(Stmts[i]);
    if (!First)
      continue;

    std::string BaseText = GetText(First->Base, SM, LO);

    AddMember(*First);

    size_t LastStmt = i;

    for (size_t j = i + 1; j < Stmts.size(); ++j) {
      auto A = GetAssignment(Stmts[j]);
      if (!A)
        break;

      if (GetText(A->Base, SM, LO) != BaseText)
        break;

      AddMember(*A);
      LastStmt = j;
    }

    auto IsComplete = [&]() {
      if (Members.size() < 2)
        return false;
      for (unsigned k = 0; k < Members.size(); ++k) {
        if (!Members[k])
          return false;
        if (Members.size() < Members[k]->Total)
          return false;
      }
      return true;
    };

    if (!IsComplete())
      continue;

    std::string ClassName = GetClassName(First->Base);
    if (ClassName.empty())
      continue;

    std::string Args;
    for (unsigned k = 0; k < Members.size(); ++k) {
      if (k > 0)
        Args += ", ";
      Args += GetText(Members[k]->RHS, SM, LO);
    }

    std::string Replacement = BaseText + " = " + ClassName + "(" + Args + ")";

    CharSourceRange ReplaceRange = CharSourceRange::getTokenRange(
        SourceRange(Stmts[i]->getBeginLoc(), Stmts[LastStmt]->getEndLoc()));

    diag(Stmts[i]->getBeginLoc(),
         "replace separate member assignments to %0 with a constructor call")
        << ClassName << FixItHint::CreateReplacement(ReplaceRange, Replacement);

    i = LastStmt;
  }
}
