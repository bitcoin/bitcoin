// Copyright (c) 2024-present Bitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "fatal_error.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

#include <string>

using namespace clang::ast_matchers;

namespace bitcoin {

/**
 * @class VisitHandledFatal
 *
 * Visits call expressions inside a given function to check if a specific call
 * expression is used as an argument by a certain list of "valid" functions.
 */
class VisitHandledFatal : public clang::RecursiveASTVisitor<VisitHandledFatal> {
private:
    const clang::CallExpr* m_fatal_call_expr;
    bool m_handled_fatal{false};
    const std::set<std::string> m_valid_function_names;

public:
    explicit VisitHandledFatal(const clang::CallExpr* fatal_call_expr,
                               const std::set<std::string> valid_function_names)
        : m_fatal_call_expr{fatal_call_expr},
          m_valid_function_names{valid_function_names} {}

    bool VisitCallExpr(const clang::CallExpr* call_expr) {
        // Find functions handling fatal errors
        if (const clang::FunctionDecl* callee{call_expr->getDirectCallee()}) {
            const auto name{callee->getNameAsString()};
            if (m_valid_function_names.contains(name)) {
                // Inspect the found function's argument
                if (call_expr->getNumArgs() > 0) {
                    const clang::Expr* check_fatal_argument{call_expr->getArg(0)->IgnoreImpCasts()};

                    // The call expression might be moved, so retrieve the relevant
                    // temporaries first
                    if (const auto* temp_expr{clang::dyn_cast<clang::MaterializeTemporaryExpr>(check_fatal_argument)}) {
                        if (const auto* bound_temp_expr{clang::dyn_cast<clang::CXXBindTemporaryExpr>(temp_expr->getSubExpr()->IgnoreImpCasts())}) {
                            if (const auto* argument{clang::dyn_cast<clang::CallExpr>(bound_temp_expr->getSubExpr()->IgnoreImpCasts())}) {
                                check_fatal_argument = argument;
                            }
                        }
                    }

                    // The call expression is usually held in a temporary, so retrieve the
                    // temporary first
                    if (const auto* temp_expr{clang::dyn_cast<clang::CXXBindTemporaryExpr>(check_fatal_argument)}) {
                        if (const auto* argument{clang::dyn_cast<clang::CallExpr>(temp_expr->getSubExpr()->IgnoreImpCasts())}) {
                            check_fatal_argument = argument;
                        }
                    }

                    // Do a direct pointer comparison here, since we expect them to be the
                    // same AST node
                    if (check_fatal_argument == m_fatal_call_expr) {
                        m_handled_fatal = true;
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool HandledFatal() { return m_handled_fatal; }
};

/**
 * @class VisitConsumedFatal
 *
 * Visits call expressions and return statements inside a given function to
 * check if a specific variable is either used by a specific "valid" function,
 * or used by a return statement.
 */
class VisitConsumedFatal
    : public clang::RecursiveASTVisitor<VisitConsumedFatal> {
private:
    const clang::VarDecl* m_fatal_var;
    bool m_consumed_fatal{false};

public:
    explicit VisitConsumedFatal(const clang::VarDecl* fatal_var)
        : m_fatal_var{fatal_var} {}

    bool VisitCXXMemberCallExpr(const clang::CXXMemberCallExpr* expr) {
        // Find method calls of interest
        if (const auto* member_expr{dyn_cast<clang::MemberExpr>(expr->getCallee())}) {
            if (member_expr->getMemberDecl()->getNameAsString() == "MoveMessages") {
                // Retrieve the first argument of the method call
                if (const auto* decl_ref{dyn_cast<clang::DeclRefExpr>(expr->getArg(0)->IgnoreImpCasts())}) {
                    // Once found, check if the arguments declaration matches the variable
                    // of interest.
                    if (decl_ref->getDecl() == m_fatal_var) {
                        m_consumed_fatal = true;
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool VisitReturnStmt(const clang::ReturnStmt* returnStmt) {
        // Retrieve the value returned by a return statement
        if (const clang::Expr* ret_expr{returnStmt->getRetValue()}) {
            // The Result type has a complicated move constructor, so retrieve the
            // actual value from behind the constructor
            if (const auto* construct_expr{dyn_cast<clang::CXXConstructExpr>(ret_expr)}) {
                if (construct_expr->getNumArgs() > 0) {
                    if (const auto* decl_ref_expr{dyn_cast<clang::DeclRefExpr>(construct_expr->getArg(0)->IgnoreImpCasts())}) {
                        // If the value could be retrieved, check if its declaration matches
                        // the variable of interest
                        if (decl_ref_expr->getDecl() == m_fatal_var) {
                            m_consumed_fatal = true;
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }

    bool ConsumedFatalVar() { return m_consumed_fatal; }
};

void FatalErrorCheck::registerMatchers(clang::ast_matchers::MatchFinder* finder) {
    auto fatal_error_type_matcher = callExpr(
        hasType(
            classTemplateSpecializationDecl(
                hasTemplateArgument(
                    1,
                    refersToType(
                        hasDeclaration(
                            enumDecl(
                                matchesName("FatalError")
                            )
                        )
                    )
                )
            )
        ),
        // Add an exception for fatal error results produced by calls to the
        // Result's Set method
        unless(
            callee(
                cxxMethodDecl(
                    hasName("Set"),
                    ofClass(
                        hasName("Result")
                    )
                )
            )
        )
    );

    finder->addMatcher(
        fatal_error_type_matcher.bind("call-to-fatal-error"),
        this);
}

void FatalErrorCheck::check(const clang::ast_matchers::MatchFinder::MatchResult &result) {
    const auto* fatal_call_expr{result.Nodes.getNodeAs<clang::CallExpr>("call-to-fatal-error")};
    if (!fatal_call_expr) return;

    const clang::FunctionDecl* fatal_enclosing_func{nullptr};

    // Traverse up the AST to find the function declaration enclosing the
    // FatalError call.
    auto parents{result.Context->getParents(*fatal_call_expr)};
    while (!parents.empty()) {
        auto current_node{parents[0]};
        parents = result.Context->getParents(current_node);

        if (const auto* func_decl{current_node.get<clang::FunctionDecl>()}) {
            // Ensure that func_decl is not a lambda.
            if (const auto* method_decl{clang::dyn_cast<clang::CXXMethodDecl>(func_decl)}) {
                if (method_decl->getParent()->isLambda()) {
                    continue;
                }
            }
            fatal_enclosing_func = func_decl;
            break;
        }
    }

    if (!fatal_enclosing_func) return;

    // Make an exception for functions returning ChainstateLoadError.
    if (fatal_enclosing_func->getReturnType().getAsString().find("ChainstateLoadError") != std::string::npos) {
        return;
    }

    // Check that the enclosing function also returns a FatalError.
    if (fatal_enclosing_func->getReturnType().getAsString().find("FatalError") == std::string::npos) {
        // Make an exception for functions that handle the FatalError.
        const std::set<std::string> function_names{
            "CheckFatal", "HandleFatalError", "UnwrapFatalError", "CheckFatalFailure"
        };
        VisitHandledFatal visitor{fatal_call_expr, function_names};
        visitor.TraverseStmt(fatal_enclosing_func->getBody());
        if (visitor.HandledFatal()) return;

        const clang::FunctionDecl* fatal_func{fatal_call_expr->getDirectCallee()};
        const std::string fatal_call_name = fatal_func ? fatal_func->getNameAsString() : "a function";

        const auto &source_manager{result.Context->getSourceManager()};
        const std::string message =
            fatal_enclosing_func->getNameAsString() +
            " does not return or handle a FatalError, but calls " +
            fatal_call_name + " on line " +
            std::to_string(source_manager.getSpellingLineNumber( fatal_call_expr->getBeginLoc())) +
            " which does return a FatalError.\n"
            "To fix this, either return a FatalError in this function, "
            "or handle the FatalError with one of `CheckFatal`, "
            "`HandleFatalError`, `UnwrapFatalError`, or `CheckFatalFailure`";

        // If the enclosing function was expanded from a macro, just report on the
        // line reporting the fatal call expression.
        const auto loc{fatal_enclosing_func->getBeginLoc()};
        if (loc.isMacroID()) {
            diag(fatal_call_expr->getBeginLoc(), message);
        } else {
            diag(loc, message);
        }
        return;
    }

    // Track whether the identified FatalError results returned from function
    // calls are consumed properly, meaning
    // 1. Check if they are returned immediately
    // 2. Check if they are assigned to an existing result with .MoveMessages or
    // .Set
    // 3. Check if they are eventually used as an arugment by a .MoveMessages call

    // Check if the result is immediately returned or get the variable it is
    // assigned to
    const clang::VarDecl* fatal_var_decl{nullptr};
    parents = result.Context->getParents(*fatal_call_expr);
    while (!parents.empty()) {
        // Move up the AST until a FunctionDecl is reached - in which case no
        // consuming return statement or variable declaration was found.
        if (parents[0].get<clang::FunctionDecl>()) {
            break;
        }
        if (parents[0].get<clang::ReturnStmt>()) {
            // If the value is returned immediately, the check is done.
            return;
        }
        if (const auto* var_decl{parents[0].get<clang::VarDecl>()}) {
            fatal_var_decl = var_decl;
            break;
        }
        parents = result.Context->getParents(parents[0]);
    }

    // Check if the result is immediately consumed by the .MoveMessages() or
    // .Set() methods.
    const std::set<std::string> function_names{"Set", "MoveMessages"};
    VisitHandledFatal visitor{fatal_call_expr, function_names};
    visitor.TraverseStmt(fatal_enclosing_func->getBody());
    // If the value is consumed immediately, the check is done.
    if (visitor.HandledFatal()) return;

    // If the declaration was found, ensure that it is either returned, or used by
    // a call to .MoveMessages.
    if (fatal_var_decl) {
      VisitConsumedFatal visitor{fatal_var_decl};
      visitor.TraverseStmt(fatal_enclosing_func->getBody());
      // If the value is eventually consumed, the check is done.
      if (visitor.ConsumedFatalVar()) return;
    }

    diag(fatal_call_expr->getBeginLoc(),
         "Call to function returning a FatalError result not properly consumed.\n"
         "It can either be immediately returned or directly consumed by "
         ".MoveMessages or .Set, "
         "or assigned to a variable which in turn has to be passed to "
         ".MoveMessages or be returned eventually.");
}

} // namespace bitcoin
