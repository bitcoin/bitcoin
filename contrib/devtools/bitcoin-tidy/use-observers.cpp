#include "use-observers.h"

#include <clang/Lex/Lexer.h>

using namespace clang;
using namespace clang::ast_matchers;

void UseObservers::registerMatchers(MatchFinder* Finder)
{
  Finder->addMatcher(
    memberExpr(
      unless(hasAncestor(functionDecl(anyOf(isImplicit(), isDefaulted())))),
      member(fieldDecl(hasAttr(attr::Annotate))))
      .bind("member"),
    this);
}

void UseObservers::check(MatchFinder::MatchResult const& Result)
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

  std::string Observer;
  for (auto const* Attr : FD->specific_attrs<AnnotateAttr>()) {
    StringRef Annotation = Attr->getAnnotation();
    if (Annotation.consume_front("observer:")) {
      Observer = Annotation.str();
      break;
    }
  }
  if (Observer.empty()) {
    return;
  }

  diag(ME->getMemberLoc(), "replace direct member access with observer")
    << FixItHint::CreateReplacement(ME->getMemberLoc(), Observer + "()");
}
